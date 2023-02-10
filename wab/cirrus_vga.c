
/*
 * QEMU Cirrus CLGD 54xx VGA Emulator.
 *
 * Copyright (c) 2004 Fabrice Bellard
 * Copyright (c) 2004 Makoto Suzuki (suzu)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
 * Reference: Finn Thogersons' VGADOC4b
 *   available at http://home.worldonline.dk/~finth/
 */

/*
	Neko Project 21/W 開発者のコメント:
	・元はQEMU/9821のコードですが強引に移植するためにあちこち削ったりしています
	・このファイルに関してはOS依存性は少ないはず（画面転送部分cirrusvga_drawGraphicだけOS依存・他のOS依存っぽいところは消してもたぶん害無し）
	・MMIOとかVRAMウィンドウとかリニアVRAMとかはcpumem.cで乗っ取っているのでそちらを参照
	・16bit/32bit I/Oはiocore.cで乗っ取っているのでそちらを参照
	・VRAM画面はcirrusvga->vram_ptrに、パレットはcirrusvga->paletteに入ってます
	・転送の仕方はcirrusvga_drawGraphicを参考に（自分もよく分かってないけど）
	・細かいところはQEMU/9821の同名ファイルを参照
*/

#include	<compiler.h>


#if defined(SUPPORT_CL_GD5430)

#include	<pccore.h>
#include	<wab/wab.h>
#include	<wab/cirrus_vga_extern.h>
#include	"cirrus_vga.h"
#include	"vga_int.h"
#include	<dosio.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<soundmng.h>

#if defined(SUPPORT_IA32_HAXM)
#include <i386hax/haxfunc.h>
#include <i386hax/haxcore.h>
#endif

#if defined(NP2_SDL)
#include <SDL.h>
#elif defined(NP2_X)
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

//#if 1
//#undef	TRACEOUT
//#define USE_TRACEOUT_VS
////#define MEM_BDA_TRACEOUT
////#define MEM_D8_TRACEOUT
//#ifdef USE_TRACEOUT_VS
//static void trace_fmt_ex(const char *fmt, ...)
//{
//	char stmp[2048];
//	va_list ap;
//	va_start(ap, fmt);
//	vsprintf(stmp, fmt, ap);
//	strcat(stmp, "¥n");
//	va_end(ap);
//	OutputDebugStringA(stmp);
//}
//#define	TRACEOUT(s)	trace_fmt_ex s
//#else
//#define	TRACEOUT(s)	(void)(s)
//#endif
//#endif	/* 1 */
/* force some bits to zero */
const uint8_t sr_mask[8] = {
    (uint8_t)~0xfc,
    (uint8_t)~0xc2,
    (uint8_t)~0x00, // np21w ver0.86 rev29  (uint8_t)~0xf0, 上位ビットも残さないとWin9xで文字表示がおかしくなる
    (uint8_t)~0xc0,
    (uint8_t)~0xf1,
    (uint8_t)~0xff,
    (uint8_t)~0xff,
    (uint8_t)~0x00,
};

const uint8_t gr_mask[16] = {
    (uint8_t)~0xf0, /* 0x00 */
    (uint8_t)~0xf0, /* 0x01 */
    (uint8_t)~0xf0, /* 0x02 */
    (uint8_t)~0xe0, /* 0x03 */
    (uint8_t)~0xfc, /* 0x04 */
    (uint8_t)~0x84, /* 0x05 */
    (uint8_t)~0xf0, /* 0x06 */
    (uint8_t)~0xf0, /* 0x07 */
    (uint8_t)~0x00, /* 0x08 */
    (uint8_t)~0xff, /* 0x09 */
    (uint8_t)~0xff, /* 0x0a */
    (uint8_t)~0xff, /* 0x0b */
    (uint8_t)~0xff, /* 0x0c */
    (uint8_t)~0xff, /* 0x0d */
    (uint8_t)~0xff, /* 0x0e */
    (uint8_t)~0xff, /* 0x0f */
};

int pcidev_cirrus_deviceid = 10;

// 内蔵アクセラレータ用
REG8 cirrusvga_regindexA2 = 0; // I/OポートFA2hで指定されているレジスタ番号
REG8 cirrusvga_regindex = 0; // I/OポートFAAhで指定されているレジスタ番号

// WAB, WSN用
int cirrusvga_wab_59e1 = 0x06;	// この値じゃないとWSN Win95ドライバがNGを返す
int cirrusvga_wab_51e1 = 0xC2;	// WSN CHECK IO RETURN VALUE
int cirrusvga_wab_5be1 = 0xf7;	// bit3:0=4M,1=2M ??????
int cirrusvga_wab_40e1 = 0x7b;
int cirrusvga_wab_42e1 = 0x00;
//int cirrusvga_wab_0fe1 = 0xC2;
int cirrusvga_wab_46e8 = 0x18;
int cirrusvga_melcowab_ofs = CIRRUS_MELCOWAB_OFS_DEFAULT;

NP2CLVGA	np2clvga = {0};
void *cirrusvga_opaque = NULL; // CIRRUS VGAの変数をグローバルアクセス出来るようにしておく･･･（良くない実装）
UINT8	cirrusvga_statsavebuf[CIRRUS_VRAM_SIZE_4MB + 1024 * 1024]; // ステートセーブ用のバッファ（無駄が多いけど互換性は保ちやすいはず）

int g_cirrus_linear_map_enabled = 0; // CIRRUS VGAの変数のリニアメモリアクセス(cpumem.cのmemp_*)有効フラグ
CPUWriteMemoryFunc *g_cirrus_linear_write[3] = {0}; // CIRRUS VGAの変数のリニアメモリアクセスWRITEで呼ばれる関数（[0]=8bit, [0]=16bit, [0]=32bit）

uint8_t* vramptr; // CIRRUS VGAのVRAMへのポインタ（メモリサイズはCIRRUS_VRAM_SIZEで十分のはずだが念のため2倍確保されている）
uint8_t* cursorptr; // CIRRUS VGAのカーソル画像バッファへのポインタ

DisplayState ds = {0}; // np21/wでは実質的に使われない

#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
BITMAPINFO *ga_bmpInfo; // CIRRUS VGAのVRAM映像を転送する時に使うBITMAPINFO構造体
BITMAPINFO *ga_bmpInfo_cursor; // CIRRUS VGAのカーソル画像を転送する時に使うBITMAPINFO構造体
HBITMAP		ga_hbmp_cursor; // CIRRUS VGAのカーソルのHBITMAP
HDC			ga_hdc_cursor; // CIRRUS VGAのカーソル画像のHDC

static HCURSOR ga_hFakeCursor = NULL; // ハードウェアカーソル（仮）CIRRUS VGAのカーソル画像が上手く表示出来ない場合用
#endif

#if defined(SUPPORT_IA32_HAXM)
#define MMIO_MODE_MMIO	0
#define MMIO_MODE_VRAM1	1
#define MMIO_MODE_VRAM2	2
int mmio_mode = MMIO_MODE_MMIO; // 0==MMIO, 1==VRAM
UINT32 mmio_mode_region1 = 0; // 0==MMIO, 1==VRAM
UINT32 mmio_mode_region2 = 0; // 0==MMIO, 1==VRAM
UINT8 lastlinmmio = 0;
#endif

void pcidev_cirrus_cfgreg_w(UINT32 devNumber, UINT8 funcNumber, UINT8 cfgregOffset, UINT8 sizeinbytes, UINT32 value);
void pc98_cirrus_setWABreg(void);
static void cirrusvga_setAutoWABID(void);

// QEMUで使われているけどよく分からなかったので無視されている関数や変数達(ｫｨ
static void cpu_register_physical_memory(target_phys_addr_t start_addr, ram_addr_t size, ram_addr_t phys_offset){
}
void np2vga_ds_dpy_update(struct DisplayState *s, int x, int y, int w, int h)
{
}
void np2vga_ds_dpy_resize(struct DisplayState *s)
{
}
void np2vga_ds_dpy_setdata(struct DisplayState *s)
{
}
void np2vga_ds_dpy_refresh(struct DisplayState *s)
{
}
void np2vga_ds_dpy_copy(struct DisplayState *s, int src_x, int src_y,
                    int dst_x, int dst_y, int w, int h)
{
}
void np2vga_ds_dpy_fill(struct DisplayState *s, int x, int y,
                    int w, int h, uint32_t_ c)
{
}
void np2vga_ds_dpy_text_cursor(struct DisplayState *s, int x, int y)
{
}
DisplaySurface np2vga_ds_surface = {0};
DisplayChangeListener np2vga_ds_listeners = {0, 0, np2vga_ds_dpy_update, np2vga_ds_dpy_resize, 
											  np2vga_ds_dpy_setdata, np2vga_ds_dpy_refresh, 
											  np2vga_ds_dpy_copy, np2vga_ds_dpy_fill, 
											  np2vga_ds_dpy_text_cursor, NULL};
void np2vga_ds_mouse_set(int x, int y, int on){
}
void np2vga_ds_cursor_define(int width, int height, int bpp, int hot_x, int hot_y,
                          uint8_t *image, uint8_t *mask){
}

DisplayState *graphic_console_init(vga_hw_update_ptr update,
                                   vga_hw_invalidate_ptr invalidate,
                                   vga_hw_screen_dump_ptr screen_dump,
                                   vga_hw_text_update_ptr text_update,
								   void *opaque)
{
	ds.opaque = opaque;
	np2vga_ds_surface.width = 640;
	np2vga_ds_surface.height = 480;
	np2vga_ds_surface.pf.bits_per_pixel = 32;
	np2vga_ds_surface.pf.bytes_per_pixel = 4;

	return &ds;
}

/***************************************
 *
 *  definitions
 *
 ***************************************/

#define qemu_MIN(a,b) ((a) < (b) ? (a) : (b))

// ID
#define CIRRUS_ID_CLGD5422  (0x23<<2)
#define CIRRUS_ID_CLGD5426  (0x24<<2)
#define CIRRUS_ID_CLGD5424  (0x25<<2)
#define CIRRUS_ID_CLGD5428  (0x26<<2)
#define CIRRUS_ID_CLGD5430  (0x28<<2)
#define CIRRUS_ID_CLGD5434  (0x2A<<2)
#define CIRRUS_ID_CLGD5436  (0x2B<<2)
#define CIRRUS_ID_CLGD5440  (0x2C<<2)
#define CIRRUS_ID_CLGD5446  (0x2E<<2)

// sequencer 0x07
#define CIRRUS_SR7_BPP_VGA            0x00
#define CIRRUS_SR7_BPP_SVGA           0x01
#define CIRRUS_SR7_BPP_MASK           0x0e
#define CIRRUS_SR7_BPP_8              0x00
#define CIRRUS_SR7_BPP_16_DOUBLEVCLK  0x02
#define CIRRUS_SR7_BPP_24             0x04
#define CIRRUS_SR7_BPP_16             0x06
#define CIRRUS_SR7_BPP_32             0x08
//#define CIRRUS_SR7_ISAADDR_MASK       0xe0
#define CIRRUS_SR7_ISAADDR_MASK       0xf0

// sequencer 0x0f
#define CIRRUS_MEMSIZE_512k        0x08
#define CIRRUS_MEMSIZE_1M          0x10
#define CIRRUS_MEMSIZE_2M          0x18
#define CIRRUS_MEMFLAGS_BANKSWITCH 0x80	// bank switching is enabled.

// sequencer 0x12
#define CIRRUS_CURSOR_SHOW         0x01
#define CIRRUS_CURSOR_HIDDENPEL    0x02
#define CIRRUS_CURSOR_LARGE        0x04	// 64x64 if set, 32x32 if clear

// sequencer 0x17
#define CIRRUS_BUSTYPE_VLBFAST   0x10
#define CIRRUS_BUSTYPE_PCI       0x20
#define CIRRUS_BUSTYPE_VLBSLOW   0x30
#define CIRRUS_BUSTYPE_ISA       0x38
#define CIRRUS_MMIO_ENABLE       0x04
#define CIRRUS_MMIO_USE_PCIADDR  0x40	// 0xb8000 if cleared.
#define CIRRUS_MEMSIZEEXT_DOUBLE 0x80

// control 0x0b
#define CIRRUS_BANKING_DUAL             0x01
#define CIRRUS_BANKING_GRANULARITY_16K  0x20	// set:16k, clear:4k

// control 0x30
#define CIRRUS_BLTMODE_BACKWARDS        0x01
#define CIRRUS_BLTMODE_MEMSYSDEST       0x02
#define CIRRUS_BLTMODE_MEMSYSSRC        0x04
#define CIRRUS_BLTMODE_TRANSPARENTCOMP  0x08
#define CIRRUS_BLTMODE_PATTERNCOPY      0x40
#define CIRRUS_BLTMODE_COLOREXPAND      0x80
#define CIRRUS_BLTMODE_PIXELWIDTHMASK   0x30
#define CIRRUS_BLTMODE_PIXELWIDTH8      0x00
#define CIRRUS_BLTMODE_PIXELWIDTH16     0x10
#define CIRRUS_BLTMODE_PIXELWIDTH24     0x20
#define CIRRUS_BLTMODE_PIXELWIDTH32     0x30

// control 0x31
#define CIRRUS_BLT_BUSY                 0x01
#define CIRRUS_BLT_START                0x02
#define CIRRUS_BLT_RESET                0x04
#define CIRRUS_BLT_FIFOUSED             0x10
#define CIRRUS_BLT_AUTOSTART            0x80

// control 0x32
#define CIRRUS_ROP_0                    0x00
#define CIRRUS_ROP_SRC_AND_DST          0x05
#define CIRRUS_ROP_NOP                  0x06
#define CIRRUS_ROP_SRC_AND_NOTDST       0x09
#define CIRRUS_ROP_NOTDST               0x0b
#define CIRRUS_ROP_SRC                  0x0d
#define CIRRUS_ROP_1                    0x0e
#define CIRRUS_ROP_NOTSRC_AND_DST       0x50
#define CIRRUS_ROP_SRC_XOR_DST          0x59
#define CIRRUS_ROP_SRC_OR_DST           0x6d
#define CIRRUS_ROP_NOTSRC_OR_NOTDST     0x90
#define CIRRUS_ROP_SRC_NOTXOR_DST       0x95
#define CIRRUS_ROP_SRC_OR_NOTDST        0xad
#define CIRRUS_ROP_NOTSRC               0xd0
#define CIRRUS_ROP_NOTSRC_OR_DST        0xd6
#define CIRRUS_ROP_NOTSRC_AND_NOTDST    0xda

#define CIRRUS_ROP_NOP_INDEX 2
#define CIRRUS_ROP_SRC_INDEX 5

// control 0x33
#define CIRRUS_BLTMODEEXT_BLTSYNCDISP      0x10
#define CIRRUS_BLTMODEEXT_BGNDONLYCLIP     0x08
#define CIRRUS_BLTMODEEXT_SOLIDFILL        0x04
#define CIRRUS_BLTMODEEXT_COLOREXPINV      0x02
#define CIRRUS_BLTMODEEXT_DWORDGRANULARITY 0x01

// memory-mapped IO
#define CIRRUS_MMIO_BLTBGCOLOR        0x00	// dword
#define CIRRUS_MMIO_BLTFGCOLOR        0x04	// dword
#define CIRRUS_MMIO_BLTWIDTH          0x08	// word
#define CIRRUS_MMIO_BLTHEIGHT         0x0a	// word
#define CIRRUS_MMIO_BLTDESTPITCH      0x0c	// word
#define CIRRUS_MMIO_BLTSRCPITCH       0x0e	// word
#define CIRRUS_MMIO_BLTDESTADDR       0x10	// dword
#define CIRRUS_MMIO_BLTSRCADDR        0x14	// dword
#define CIRRUS_MMIO_BLTWRITEMASK      0x17	// byte
#define CIRRUS_MMIO_BLTMODE           0x18	// byte
#define CIRRUS_MMIO_BLTROP            0x1a	// byte
#define CIRRUS_MMIO_BLTMODEEXT        0x1b	// byte
#define CIRRUS_MMIO_BLTTRANSPARENTCOLOR 0x1c	// word?
#define CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK 0x20	// word?
#define CIRRUS_MMIO_LINEARDRAW_START_X 0x24	// word
#define CIRRUS_MMIO_LINEARDRAW_START_Y 0x26	// word
#define CIRRUS_MMIO_LINEARDRAW_END_X  0x28	// word
#define CIRRUS_MMIO_LINEARDRAW_END_Y  0x2a	// word
#define CIRRUS_MMIO_LINEARDRAW_LINESTYLE_INC 0x2c	// byte
#define CIRRUS_MMIO_LINEARDRAW_LINESTYLE_ROLLOVER 0x2d	// byte
#define CIRRUS_MMIO_LINEARDRAW_LINESTYLE_MASK 0x2e	// byte
#define CIRRUS_MMIO_LINEARDRAW_LINESTYLE_ACCUM 0x2f	// byte
#define CIRRUS_MMIO_BRESENHAM_K1      0x30	// word
#define CIRRUS_MMIO_BRESENHAM_K3      0x32	// word
#define CIRRUS_MMIO_BRESENHAM_ERROR   0x34	// word
#define CIRRUS_MMIO_BRESENHAM_DELTA_MAJOR 0x36	// word
#define CIRRUS_MMIO_BRESENHAM_DIRECTION 0x38	// byte
#define CIRRUS_MMIO_LINEDRAW_MODE     0x39	// byte
#define CIRRUS_MMIO_BLTSTATUS         0x40	// byte

// PCI 0x02: device
#define PCI_DEVICE_CLGD5462           0x00d0
#define PCI_DEVICE_CLGD5465           0x00d6

// PCI 0x04: command(word), 0x06(word): status
#define PCI_COMMAND_IOACCESS                0x0001
#define PCI_COMMAND_MEMACCESS               0x0002
#define PCI_COMMAND_BUSMASTER               0x0004
#define PCI_COMMAND_SPECIALCYCLE            0x0008
#define PCI_COMMAND_MEMWRITEINVALID         0x0010
#define PCI_COMMAND_PALETTESNOOPING         0x0020
#define PCI_COMMAND_PARITYDETECTION         0x0040
#define PCI_COMMAND_ADDRESSDATASTEPPING     0x0080
#define PCI_COMMAND_SERR                    0x0100
#define PCI_COMMAND_BACKTOBACKTRANS         0x0200
// PCI 0x08, 0xff000000 (0x09-0x0b:class,0x08:rev)
#define PCI_CLASS_BASE_DISPLAY        0x03
// PCI 0x08, 0x00ff0000
#define PCI_CLASS_SUB_VGA             0x00
// PCI 0x0c, 0x00ff0000 (0x0c:cacheline,0x0d:latency,0x0e:headertype,0x0f:Built-in self test)
#define PCI_CLASS_HEADERTYPE_00h  0x00
// 0x10-0x3f (headertype 00h)
// PCI 0x10,0x14,0x18,0x1c,0x20,0x24: base address mapping registers
//   0x10: MEMBASE, 0x14: IOBASE(hard-coded in XFree86 3.x)
#define PCI_MAP_MEM                 0x0
#define PCI_MAP_IO                  0x1
#define PCI_MAP_MEM_ADDR_MASK       (~0xf)
#define PCI_MAP_IO_ADDR_MASK        (~0x3)
#define PCI_MAP_MEMFLAGS_32BIT      0x0
#define PCI_MAP_MEMFLAGS_32BIT_1M   0x1
#define PCI_MAP_MEMFLAGS_64BIT      0x4
#define PCI_MAP_MEMFLAGS_CACHEABLE  0x8
// PCI 0x28: cardbus CIS pointer
// PCI 0x2c: subsystem vendor id, 0x2e: subsystem id
// PCI 0x30: expansion ROM base address
#define PCI_ROMBIOS_ENABLED         0x1
// PCI 0x34: 0xffffff00=reserved, 0x000000ff=capabilities pointer
// PCI 0x38: reserved
// PCI 0x3c: 0x3c=int-line, 0x3d=int-pin, 0x3e=min-gnt, 0x3f=maax-lat

#define CIRRUS_PNPMMIO_SIZE         0x1000


/* I/O and memory hook */
#define CIRRUS_HOOK_NOT_HANDLED 0
#define CIRRUS_HOOK_HANDLED 1

#define ABS(a) ((signed)(a) > 0 ? a : -a)

// XXX: WAB系をとりあえず使えるようにするために4MB VRAMサイズまで書き込み許可
#define BLTUNSAFE_DST(s) \
        ( /* check dst is within bounds */ \
            ((s)->cirrus_blt_dstpitch >= 0) ? /* ピッチの正負を確認 */ \
			((s)->cirrus_blt_height * (s)->cirrus_blt_dstpitch + ((s)->cirrus_blt_dstaddr & (s)->cirrus_addr_mask) > CIRRUS_VRAM_SIZE_4MB) : /* ピッチが正の時、VRAM最大を超えないか確認 */ \
			(((s)->cirrus_blt_height-1) * -(s)->cirrus_blt_dstpitch > ((s)->cirrus_blt_dstaddr & (s)->cirrus_addr_mask)) /* ピッチが負の時、0未満にならないか確認 */ \
        )
#define BLTUNSAFE_SRC(s) \
        ( /* check src is within bounds */ \
            ((s)->cirrus_blt_srcpitch >= 0) ? /* ピッチの正負を確認 */ \
            ((s)->cirrus_blt_height * (s)->cirrus_blt_srcpitch + ((s)->cirrus_blt_srcaddr & (s)->cirrus_addr_mask) > CIRRUS_VRAM_SIZE_4MB) : /* ピッチが正の時、VRAM最大を超えないか確認 */ \
			(((s)->cirrus_blt_height-1) * -(s)->cirrus_blt_srcpitch > ((s)->cirrus_blt_srcaddr & (s)->cirrus_addr_mask)) /* ピッチが負の時、0未満にならないか確認 */ \
        )
#define BLTUNSAFE(s) \
    ( \
        BLTUNSAFE_DST(s) || BLTUNSAFE_SRC(s) \
    )
// SRC未使用なら1
#define BLTUNSAFE_NOSRC(s) \
    ( \
        rop_to_index[(s)->gr[0x32]]==0 || rop_to_index[(s)->gr[0x32]]==2 || rop_to_index[(s)->gr[0x32]]==4 || rop_to_index[(s)->gr[0x32]]==6 \
    )
//#define BLTUNSAFE(s) \
//    ( \
//        ( /* check dst is within bounds */ \
//            (s)->cirrus_blt_height * ABS((s)->cirrus_blt_dstpitch) \
//                + ((s)->cirrus_blt_dstaddr & (s)->cirrus_addr_mask) > \
//                    (s)->vram_size \
//        ) || \
//        ( /* check src is within bounds */ \
//            (s)->cirrus_blt_height * ABS((s)->cirrus_blt_srcpitch) \
//                + ((s)->cirrus_blt_srcaddr & (s)->cirrus_addr_mask) > \
//                    (s)->vram_size \
//        ) \
//    )

struct CirrusVGAState;
typedef void (*cirrus_bitblt_rop_t) (struct CirrusVGAState *s,
                                     uint8_t * dst, const uint8_t * src,
				     int dstpitch, int srcpitch,
				     int bltwidth, int bltheight);
typedef void (*cirrus_fill_t)(struct CirrusVGAState *s,
                              uint8_t *dst, int dst_pitch, int width, int height);

typedef struct CirrusVGAState {
    VGA_STATE_COMMON

    int cirrus_linear_io_addr;
    int cirrus_linear_bitblt_io_addr;
    int cirrus_mmio_io_addr;
    uint32_t_ cirrus_addr_mask;
    uint32_t_ linear_mmio_mask;
    uint8_t cirrus_shadow_gr0;
    uint8_t cirrus_shadow_gr1;
    uint8_t cirrus_hidden_dac_lockindex;
    uint8_t cirrus_hidden_dac_data;
    uint32_t_ cirrus_bank_base[2];
    uint32_t_ cirrus_bank_limit[2];
    uint8_t cirrus_hidden_palette[48];
    uint32_t_ hw_cursor_x;
    uint32_t_ hw_cursor_y;
    int cirrus_blt_pixelwidth;
    int cirrus_blt_width;
    int cirrus_blt_height;
    int cirrus_blt_dstpitch;
    int cirrus_blt_srcpitch;
    uint32_t_ cirrus_blt_fgcol;
    uint32_t_ cirrus_blt_bgcol;
    uint32_t_ cirrus_blt_dstaddr;
    uint32_t_ cirrus_blt_srcaddr;
    uint8_t cirrus_blt_mode;
    uint8_t cirrus_blt_modeext;
    cirrus_bitblt_rop_t cirrus_rop;
#define CIRRUS_BLTBUFSIZE (2048 * 4) /* one line width */
    uint8_t cirrus_bltbuf[CIRRUS_BLTBUFSIZE];
    uint8_t *cirrus_srcptr;
    uint8_t *cirrus_srcptr_end;
    uint32_t_ cirrus_srccounter;
    /* hwcursor display state */
    int last_hw_cursor_size;
    int last_hw_cursor_x;
    int last_hw_cursor_y;
    int last_hw_cursor_y_start;
    int last_hw_cursor_y_end;
    int real_vram_size; /* XXX: suppress that */
    //CPUWriteMemoryFunc **cirrus_linear_write;
    int device_id;
    int bustype;
    uint8_t videowindow_dblbuf_index;
    uint8_t graphics_dblbuf_index;
} CirrusVGAState;

CirrusVGAState *cirrusvga = NULL;

#define CIRRUS_FMC_W 16
#define CIRRUS_FMC_H 32

UINT8 FakeMouseCursorData[CIRRUS_FMC_W * CIRRUS_FMC_H] = {
	// 0:transparent 1:black 2:white 
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	1, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 1, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0
};

static uint8_t vga_dumb_retrace(VGAState *s)
{
    return s->st01 ^ (ST01_V_RETRACE | ST01_DISP_ENABLE);
}

static uint8_t rop_to_index[256];

typedef unsigned int rgb_to_pixel_dup_func(unsigned int r, unsigned int g, unsigned b);
#define NB_DEPTHS 7

static unsigned int rgb_to_pixel8(unsigned int r, unsigned int g,
                                         unsigned int b)
{
    return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6);
}

static unsigned int rgb_to_pixel15(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    return ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
}

static unsigned int rgb_to_pixel15bgr(unsigned int r, unsigned int g,
                                             unsigned int b)
{
    return ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
}

static unsigned int rgb_to_pixel16(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

static unsigned int rgb_to_pixel16bgr(unsigned int r, unsigned int g,
                                             unsigned int b)
{
    return ((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3);
}

static unsigned int rgb_to_pixel24(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    return (r << 16) | (g << 8) | b;
}

static unsigned int rgb_to_pixel24bgr(unsigned int r, unsigned int g,
                                             unsigned int b)
{
    return (b << 16) | (g << 8) | r;
}

static unsigned int rgb_to_pixel32(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    return (r << 16) | (g << 8) | b;
}

static unsigned int rgb_to_pixel32bgr(unsigned int r, unsigned int g,
                                             unsigned int b)
{
    return (b << 16) | (g << 8) | r;
}

static unsigned int rgb_to_pixel8_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel8(r, g, b);
    col |= col << 8;
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel15_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel15(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel15bgr_dup(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    unsigned int col;
    col = rgb_to_pixel15bgr(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel16_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel16(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel16bgr_dup(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    unsigned int col;
    col = rgb_to_pixel16bgr(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel32_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel32(r, g, b);
    return col;
}

static unsigned int rgb_to_pixel32bgr_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel32bgr(r, g, b);
    return col;
}


static int ds_get_linesize(DisplayState *ds)
{
    return np2wab.realWidth*4;
}

static uint8_t* ds_get_data(DisplayState *ds)
{
    return ds->surface->data;
}

static int ds_get_width(DisplayState *ds)
{
    return np2wab.realWidth;//ds->surface->width;
}

static int ds_get_height(DisplayState *ds)
{
    return np2wab.realHeight;//ds->surface->height;
}

static int ds_get_bits_per_pixel(DisplayState *ds)
{
    return 32;//ds->surface->pf.bits_per_pixel;
}

static int ds_get_bytes_per_pixel(DisplayState *ds)
{
    return 4;//ds->surface->pf.bytes_per_pixel;
}

static void dpy_update(DisplayState *s, int x, int y, int w, int h)
{
    struct DisplayChangeListener *dcl = s->listeners;
    while (dcl != NULL) {
        dcl->dpy_update(s, x, y, w, h);
        dcl = dcl->next;
    }
}

static rgb_to_pixel_dup_func *rgb_to_pixel_dup_table[NB_DEPTHS] = {
    rgb_to_pixel8_dup,
    rgb_to_pixel15_dup,
    rgb_to_pixel16_dup,
    rgb_to_pixel32_dup,
    rgb_to_pixel32bgr_dup,
    rgb_to_pixel15bgr_dup,
    rgb_to_pixel16bgr_dup,
};

/***************************************
 *
 *  prototypes.
 *
 ***************************************/

static void cirrus_bitblt_dblbufferswitch();
static void cirrus_bitblt_reset(CirrusVGAState *s);
static void cirrus_update_memory_access(CirrusVGAState *s);

/***************************************
 *
 *  raster operations
 *
 ***************************************/

static void cirrus_bitblt_rop_nop(CirrusVGAState *s,
                                  uint8_t *dst,const uint8_t *src,
                                  int dstpitch,int srcpitch,
                                  int bltwidth,int bltheight)
{
}

static void cirrus_bitblt_fill_nop(CirrusVGAState *s,
                                   uint8_t *dst,
                                   int dstpitch, int bltwidth,int bltheight)
{
}

#define ROP_NAME 0
#define ROP_OP(d, s) d = 0
#include "cirrus_vga_rop.h"

#define ROP_NAME src_and_dst
#define ROP_OP(d, s) d = (s) & (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME src_and_notdst
#define ROP_OP(d, s) d = (s) & (~(d))
#include "cirrus_vga_rop.h"

#define ROP_NAME notdst
#define ROP_OP(d, s) d = ~(d)
#include "cirrus_vga_rop.h"

#define ROP_NAME src
#define ROP_OP(d, s) d = s
#include "cirrus_vga_rop.h"

#define ROP_NAME 1
#define ROP_OP(d, s) d = ~0
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc_and_dst
#define ROP_OP(d, s) d = (~(s)) & (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME src_xor_dst
#define ROP_OP(d, s) d = (s) ^ (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME src_or_dst
#define ROP_OP(d, s) d = (s) | (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc_or_notdst
#define ROP_OP(d, s) d = (~(s)) | (~(d))
#include "cirrus_vga_rop.h"

#define ROP_NAME src_notxor_dst
#define ROP_OP(d, s) d = ~((s) ^ (d))
#include "cirrus_vga_rop.h"

#define ROP_NAME src_or_notdst
#define ROP_OP(d, s) d = (s) | (~(d))
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc
#define ROP_OP(d, s) d = (~(s))
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc_or_dst
#define ROP_OP(d, s) d = (~(s)) | (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc_and_notdst
#define ROP_OP(d, s) d = (~(s)) & (~(d))
#include "cirrus_vga_rop.h"

static const cirrus_bitblt_rop_t cirrus_fwd_rop[16] = {
    cirrus_bitblt_rop_fwd_0,
    cirrus_bitblt_rop_fwd_src_and_dst,
    cirrus_bitblt_rop_nop,
    cirrus_bitblt_rop_fwd_src_and_notdst,
    cirrus_bitblt_rop_fwd_notdst,
    cirrus_bitblt_rop_fwd_src,
    cirrus_bitblt_rop_fwd_1,
    cirrus_bitblt_rop_fwd_notsrc_and_dst,
    cirrus_bitblt_rop_fwd_src_xor_dst,
    cirrus_bitblt_rop_fwd_src_or_dst,
    cirrus_bitblt_rop_fwd_notsrc_or_notdst,
    cirrus_bitblt_rop_fwd_src_notxor_dst,
    cirrus_bitblt_rop_fwd_src_or_notdst,
    cirrus_bitblt_rop_fwd_notsrc,
    cirrus_bitblt_rop_fwd_notsrc_or_dst,
    cirrus_bitblt_rop_fwd_notsrc_and_notdst,
};

static const cirrus_bitblt_rop_t cirrus_bkwd_rop[16] = {
    cirrus_bitblt_rop_bkwd_0,
    cirrus_bitblt_rop_bkwd_src_and_dst,
    cirrus_bitblt_rop_nop,
    cirrus_bitblt_rop_bkwd_src_and_notdst,
    cirrus_bitblt_rop_bkwd_notdst,
    cirrus_bitblt_rop_bkwd_src,
    cirrus_bitblt_rop_bkwd_1,
    cirrus_bitblt_rop_bkwd_notsrc_and_dst,
    cirrus_bitblt_rop_bkwd_src_xor_dst,
    cirrus_bitblt_rop_bkwd_src_or_dst,
    cirrus_bitblt_rop_bkwd_notsrc_or_notdst,
    cirrus_bitblt_rop_bkwd_src_notxor_dst,
    cirrus_bitblt_rop_bkwd_src_or_notdst,
    cirrus_bitblt_rop_bkwd_notsrc,
    cirrus_bitblt_rop_bkwd_notsrc_or_dst,
    cirrus_bitblt_rop_bkwd_notsrc_and_notdst,
};

#define TRANSP_ROP(name) {\
    name ## _8,\
    name ## _16,\
        }
#define TRANSP_NOP(func) {\
    func,\
    func,\
        }

static const cirrus_bitblt_rop_t cirrus_fwd_transp_rop[16][2] = {
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_0),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_and_dst),
    TRANSP_NOP(cirrus_bitblt_rop_nop),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_and_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_1),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc_and_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_xor_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_or_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc_or_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_notxor_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_or_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc_or_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_bkwd_transp_rop[16][2] = {
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_0),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_and_dst),
    TRANSP_NOP(cirrus_bitblt_rop_nop),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_and_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_1),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc_and_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_xor_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_or_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc_or_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_notxor_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_or_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc_or_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc_and_notdst),
};

#define ROP2(name) {\
    name ## _8,\
    name ## _16,\
    name ## _24,\
    name ## _32,\
        }

#define ROP_NOP2(func) {\
    func,\
    func,\
    func,\
    func,\
        }

static const cirrus_bitblt_rop_t cirrus_patternfill[16][4] = {
    ROP2(cirrus_patternfill_0),
    ROP2(cirrus_patternfill_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_patternfill_src_and_notdst),
    ROP2(cirrus_patternfill_notdst),
    ROP2(cirrus_patternfill_src),
    ROP2(cirrus_patternfill_1),
    ROP2(cirrus_patternfill_notsrc_and_dst),
    ROP2(cirrus_patternfill_src_xor_dst),
    ROP2(cirrus_patternfill_src_or_dst),
    ROP2(cirrus_patternfill_notsrc_or_notdst),
    ROP2(cirrus_patternfill_src_notxor_dst),
    ROP2(cirrus_patternfill_src_or_notdst),
    ROP2(cirrus_patternfill_notsrc),
    ROP2(cirrus_patternfill_notsrc_or_dst),
    ROP2(cirrus_patternfill_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_colorexpand_transp[16][4] = {
    ROP2(cirrus_colorexpand_transp_0),
    ROP2(cirrus_colorexpand_transp_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_colorexpand_transp_src_and_notdst),
    ROP2(cirrus_colorexpand_transp_notdst),
    ROP2(cirrus_colorexpand_transp_src),
    ROP2(cirrus_colorexpand_transp_1),
    ROP2(cirrus_colorexpand_transp_notsrc_and_dst),
    ROP2(cirrus_colorexpand_transp_src_xor_dst),
    ROP2(cirrus_colorexpand_transp_src_or_dst),
    ROP2(cirrus_colorexpand_transp_notsrc_or_notdst),
    ROP2(cirrus_colorexpand_transp_src_notxor_dst),
    ROP2(cirrus_colorexpand_transp_src_or_notdst),
    ROP2(cirrus_colorexpand_transp_notsrc),
    ROP2(cirrus_colorexpand_transp_notsrc_or_dst),
    ROP2(cirrus_colorexpand_transp_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_colorexpand[16][4] = {
    ROP2(cirrus_colorexpand_0),
    ROP2(cirrus_colorexpand_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_colorexpand_src_and_notdst),
    ROP2(cirrus_colorexpand_notdst),
    ROP2(cirrus_colorexpand_src),
    ROP2(cirrus_colorexpand_1),
    ROP2(cirrus_colorexpand_notsrc_and_dst),
    ROP2(cirrus_colorexpand_src_xor_dst),
    ROP2(cirrus_colorexpand_src_or_dst),
    ROP2(cirrus_colorexpand_notsrc_or_notdst),
    ROP2(cirrus_colorexpand_src_notxor_dst),
    ROP2(cirrus_colorexpand_src_or_notdst),
    ROP2(cirrus_colorexpand_notsrc),
    ROP2(cirrus_colorexpand_notsrc_or_dst),
    ROP2(cirrus_colorexpand_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_colorexpand_pattern_transp[16][4] = {
    ROP2(cirrus_colorexpand_pattern_transp_0),
    ROP2(cirrus_colorexpand_pattern_transp_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_colorexpand_pattern_transp_src_and_notdst),
    ROP2(cirrus_colorexpand_pattern_transp_notdst),
    ROP2(cirrus_colorexpand_pattern_transp_src),
    ROP2(cirrus_colorexpand_pattern_transp_1),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc_and_dst),
    ROP2(cirrus_colorexpand_pattern_transp_src_xor_dst),
    ROP2(cirrus_colorexpand_pattern_transp_src_or_dst),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc_or_notdst),
    ROP2(cirrus_colorexpand_pattern_transp_src_notxor_dst),
    ROP2(cirrus_colorexpand_pattern_transp_src_or_notdst),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc_or_dst),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_colorexpand_pattern[16][4] = {
    ROP2(cirrus_colorexpand_pattern_0),
    ROP2(cirrus_colorexpand_pattern_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_colorexpand_pattern_src_and_notdst),
    ROP2(cirrus_colorexpand_pattern_notdst),
    ROP2(cirrus_colorexpand_pattern_src),
    ROP2(cirrus_colorexpand_pattern_1),
    ROP2(cirrus_colorexpand_pattern_notsrc_and_dst),
    ROP2(cirrus_colorexpand_pattern_src_xor_dst),
    ROP2(cirrus_colorexpand_pattern_src_or_dst),
    ROP2(cirrus_colorexpand_pattern_notsrc_or_notdst),
    ROP2(cirrus_colorexpand_pattern_src_notxor_dst),
    ROP2(cirrus_colorexpand_pattern_src_or_notdst),
    ROP2(cirrus_colorexpand_pattern_notsrc),
    ROP2(cirrus_colorexpand_pattern_notsrc_or_dst),
    ROP2(cirrus_colorexpand_pattern_notsrc_and_notdst),
};

static const cirrus_fill_t cirrus_fill[16][4] = {
    ROP2(cirrus_fill_0),
    ROP2(cirrus_fill_src_and_dst),
    ROP_NOP2(cirrus_bitblt_fill_nop),
    ROP2(cirrus_fill_src_and_notdst),
    ROP2(cirrus_fill_notdst),
    ROP2(cirrus_fill_src),
    ROP2(cirrus_fill_1),
    ROP2(cirrus_fill_notsrc_and_dst),
    ROP2(cirrus_fill_src_xor_dst),
    ROP2(cirrus_fill_src_or_dst),
    ROP2(cirrus_fill_notsrc_or_notdst),
    ROP2(cirrus_fill_src_notxor_dst),
    ROP2(cirrus_fill_src_or_notdst),
    ROP2(cirrus_fill_notsrc),
    ROP2(cirrus_fill_notsrc_or_dst),
    ROP2(cirrus_fill_notsrc_and_notdst),
};

static void cirrus_bitblt_fgcol(CirrusVGAState *s)
{
    unsigned int color;
    switch (s->cirrus_blt_pixelwidth) {
    case 1:
        s->cirrus_blt_fgcol = s->cirrus_shadow_gr1;
        break;
    case 2:
        color = s->cirrus_shadow_gr1 | (s->gr[0x11] << 8);
        s->cirrus_blt_fgcol = le16_to_cpu(color);
        break;
    case 3:
        s->cirrus_blt_fgcol = s->cirrus_shadow_gr1 |
            (s->gr[0x11] << 8) | (s->gr[0x13] << 16);
        break;
    default:
    case 4:
        color = s->cirrus_shadow_gr1 | (s->gr[0x11] << 8) |
            (s->gr[0x13] << 16) | (s->gr[0x15] << 24);
        s->cirrus_blt_fgcol = le32_to_cpu(color);
        break;
    }
}

static void cirrus_bitblt_bgcol(CirrusVGAState *s)
{
    unsigned int color;
    switch (s->cirrus_blt_pixelwidth) {
    case 1:
        s->cirrus_blt_bgcol = s->cirrus_shadow_gr0;
        break;
    case 2:
        color = s->cirrus_shadow_gr0 | (s->gr[0x10] << 8);
        s->cirrus_blt_bgcol = le16_to_cpu(color);
        break;
    case 3:
        s->cirrus_blt_bgcol = s->cirrus_shadow_gr0 |
            (s->gr[0x10] << 8) | (s->gr[0x12] << 16);
        break;
    default:
    case 4:
        color = s->cirrus_shadow_gr0 | (s->gr[0x10] << 8) |
            (s->gr[0x12] << 16) | (s->gr[0x14] << 24);
        s->cirrus_blt_bgcol = le32_to_cpu(color);
        break;
    }
}

static void cirrus_invalidate_region(CirrusVGAState * s, int off_begin,
				     int off_pitch, int bytesperline,
				     int lines)
{
    int y;
    int off_cur;
    int off_cur_end;

    for (y = 0; y < lines; y++) {
		off_cur = off_begin;
		off_cur_end = (off_cur + bytesperline) & s->cirrus_addr_mask;
		off_cur &= TARGET_PAGE_MASK;
		while (off_cur < off_cur_end) {
			cpu_physical_memory_set_dirty(s->vram_offset + off_cur);
			off_cur += TARGET_PAGE_SIZE;
		}
		off_begin += off_pitch;
    }
}

static int cirrus_bitblt_common_patterncopy(CirrusVGAState * s,
					    const uint8_t * src)
{
    uint8_t *dst;

    dst = s->vram_ptr + (s->cirrus_blt_dstaddr & s->cirrus_addr_mask);

	if(BLTUNSAFE_DST(s)){
		return 0;
	}

    (*s->cirrus_rop) (s, dst, src,
                      s->cirrus_blt_dstpitch, 0,
                      s->cirrus_blt_width, s->cirrus_blt_height);
    cirrus_invalidate_region(s, s->cirrus_blt_dstaddr,
                             s->cirrus_blt_dstpitch, s->cirrus_blt_width,
                             s->cirrus_blt_height);
    return 1;
}

/* fill */

static int cirrus_bitblt_solidfill(CirrusVGAState *s, int blt_rop)
{
    cirrus_fill_t rop_func;
	
	if(BLTUNSAFE_DST(s)){
		return 0;
	}

    rop_func = cirrus_fill[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
    rop_func(s, s->vram_ptr + (s->cirrus_blt_dstaddr & s->cirrus_addr_mask),
             s->cirrus_blt_dstpitch,
             s->cirrus_blt_width, s->cirrus_blt_height);
    cirrus_invalidate_region(s, s->cirrus_blt_dstaddr,
			     s->cirrus_blt_dstpitch, s->cirrus_blt_width,
			     s->cirrus_blt_height);
    cirrus_bitblt_reset(s);
    return 1;
}

/***************************************
 *
 *  bitblt (video-to-video)
 *
 ***************************************/

static int cirrus_bitblt_videotovideo_patterncopy(CirrusVGAState * s)
{
    return cirrus_bitblt_common_patterncopy(s,
					    s->vram_ptr + ((s->cirrus_blt_srcaddr & ~7) &
                                            s->cirrus_addr_mask));
}

static void cirrus_do_copy(CirrusVGAState *s, int dst, int src, int w, int h)
{
    int sx, sy;
    int dx, dy;
    int width, height;
    int depth;
    int notify = 0;

    depth = s->get_bpp((VGAState *)s) / 8;
	if(depth==0) return;
	if(s->cirrus_blt_srcpitch==0) return;
	if(s->cirrus_blt_dstpitch==0) return;
    s->get_resolution((VGAState *)s, &width, &height);

    /* extra x, y */
    sx = (src % ABS(s->cirrus_blt_srcpitch)) / depth;
    sy = (src / ABS(s->cirrus_blt_srcpitch));
    dx = (dst % ABS(s->cirrus_blt_dstpitch)) / depth;
    dy = (dst / ABS(s->cirrus_blt_dstpitch));

    /* normalize width */
    w /= depth;

    /* if we're doing a backward copy, we have to adjust
       our x/y to be the upper left corner (instead of the lower
       right corner) */
    if (s->cirrus_blt_dstpitch < 0) {
		sx -= (s->cirrus_blt_width / depth) - 1;
		dx -= (s->cirrus_blt_width / depth) - 1;
		sy -= s->cirrus_blt_height - 1;
		dy -= s->cirrus_blt_height - 1;
    }

    /* are we in the visible portion of memory? */
    if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 &&
		(sx + w) <= width && (sy + h) <= height &&
		(dx + w) <= width && (dy + h) <= height) {
		notify = 1;
    }

    /* make to sure only copy if it's a plain copy ROP */
    if (*s->cirrus_rop != cirrus_bitblt_rop_fwd_src &&
		*s->cirrus_rop != cirrus_bitblt_rop_bkwd_src)
		notify = 0;

    /* we have to flush all pending changes so that the copy
       is generated at the appropriate moment in time */
    if (notify){
		vga_hw_update();
	}
    (*s->cirrus_rop) (s, s->vram_ptr +
		      (s->cirrus_blt_dstaddr & s->cirrus_addr_mask),
		      s->vram_ptr +
		      (s->cirrus_blt_srcaddr & s->cirrus_addr_mask),
		      s->cirrus_blt_dstpitch, s->cirrus_blt_srcpitch,
		      s->cirrus_blt_width, s->cirrus_blt_height);

    if (notify){
		qemu_console_copy(s->ds,
				  sx, sy, dx, dy,
				  s->cirrus_blt_width / depth,
				  s->cirrus_blt_height);
	}

    /* we don't have to notify the display that this portion has
       changed since qemu_console_copy implies this */

    cirrus_invalidate_region(s, s->cirrus_blt_dstaddr,
				s->cirrus_blt_dstpitch, s->cirrus_blt_width,
				s->cirrus_blt_height);
}

static int cirrus_bitblt_videotovideo_copy(CirrusVGAState * s)
{
	if(BLTUNSAFE_DST(s) || (!BLTUNSAFE_NOSRC(s) && BLTUNSAFE_SRC(s))){ // XXX: ソースを使用しないならソースの範囲外チェック不要。抜本解決必要
		return 0;
	}

    cirrus_do_copy(s, s->cirrus_blt_dstaddr - s->start_addr,
            s->cirrus_blt_srcaddr - s->start_addr,
            s->cirrus_blt_width, s->cirrus_blt_height);

    return 1;
}

/***************************************
 *
 *  bitblt (cpu-to-video)
 *
 ***************************************/

static void cirrus_bitblt_cputovideo_next(CirrusVGAState * s)
{
    int copy_count;
    uint8_t *end_ptr;

    if (s->cirrus_srccounter > 0) {
        if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
			cirrus_bitblt_common_patterncopy(s, s->cirrus_bltbuf);
        the_end:
            s->cirrus_srccounter = 0;
			cirrus_bitblt_dblbufferswitch();
            cirrus_bitblt_reset(s);
        } else {
            /* at least one scan line */
            do {
				(*s->cirrus_rop)(s, s->vram_ptr +
                                (s->cirrus_blt_dstaddr & s->cirrus_addr_mask),
                                s->cirrus_bltbuf, 0, 0, s->cirrus_blt_width, 1);
                cirrus_invalidate_region(s, s->cirrus_blt_dstaddr, 0,
                                         s->cirrus_blt_width, 1);
                s->cirrus_blt_dstaddr += s->cirrus_blt_dstpitch;
                s->cirrus_srccounter -= s->cirrus_blt_srcpitch;
                if (s->cirrus_srccounter <= 0)
                    goto the_end;
                /* more bytes than needed can be transfered because of
                   word alignment, so we keep them for the next line */
                /* XXX: keep alignment to speed up transfer */
                end_ptr = s->cirrus_bltbuf + s->cirrus_blt_srcpitch;
                copy_count = (int)(s->cirrus_srcptr_end - end_ptr);
				if(copy_count >= 0 && s->cirrus_blt_srcpitch + copy_count <= sizeof(s->cirrus_bltbuf)) // 範囲外になっていないかチェック
					memmove(s->cirrus_bltbuf, end_ptr, copy_count);
                s->cirrus_srcptr = s->cirrus_bltbuf + copy_count;
				if(s->cirrus_blt_srcpitch <= sizeof(s->cirrus_bltbuf)){
					s->cirrus_srcptr_end = s->cirrus_bltbuf + s->cirrus_blt_srcpitch;
				}else{
					s->cirrus_srcptr_end = s->cirrus_bltbuf + sizeof(s->cirrus_bltbuf);
				}
            } while (s->cirrus_srcptr >= s->cirrus_srcptr_end);
        }
    }
}

/***************************************
 *
 *  bitblt (video-to-cpu)
 *
 ***************************************/

static void cirrus_bitblt_videotocpu_next(CirrusVGAState * s)
{
    if (s->cirrus_srccounter > 0) {
        if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
			// XXX: not implemented
			//cirrus_bitblt_common_patterncopy(s, s->cirrus_bltbuf);
        the_end:
            s->cirrus_srccounter = 0;
			cirrus_bitblt_dblbufferswitch();
            cirrus_bitblt_reset(s);
        } else {
            s->cirrus_blt_srcaddr += s->cirrus_blt_srcpitch;
            s->cirrus_srccounter -= s->cirrus_blt_dstpitch;
            if (s->cirrus_srccounter <= 0)
                goto the_end;
			(*s->cirrus_rop)(s, s->cirrus_bltbuf, s->vram_ptr +
                            (s->cirrus_blt_srcaddr & s->cirrus_addr_mask),
                            0, 0, s->cirrus_blt_width, 1);
			s->cirrus_srcptr = s->cirrus_bltbuf;
			if(s->cirrus_blt_srcpitch <= sizeof(s->cirrus_bltbuf)){
				s->cirrus_srcptr_end = s->cirrus_bltbuf + s->cirrus_blt_srcpitch;
			}else{
				s->cirrus_srcptr_end = s->cirrus_bltbuf + sizeof(s->cirrus_bltbuf);
			}
        }
    }
}

/***************************************
 *
 *  bitblt wrapper
 *
 ***************************************/

static void cirrus_bitblt_reset(CirrusVGAState * s)
{
    int need_update;

    s->gr[0x31] &=
	~(CIRRUS_BLT_START | CIRRUS_BLT_BUSY | CIRRUS_BLT_FIFOUSED);
    need_update = s->cirrus_srcptr != &s->cirrus_bltbuf[0]
        || s->cirrus_srcptr_end != &s->cirrus_bltbuf[0];
    s->cirrus_srcptr = &s->cirrus_bltbuf[0];
    s->cirrus_srcptr_end = &s->cirrus_bltbuf[0];
    s->cirrus_srccounter = 0;
    if (!need_update)
        return;
    cirrus_update_memory_access(s);
}

static int cirrus_bitblt_cputovideo(CirrusVGAState * s)
{
    int w;
	
    s->cirrus_blt_mode &= ~CIRRUS_BLTMODE_MEMSYSSRC;
    s->cirrus_srcptr = &s->cirrus_bltbuf[0];
    s->cirrus_srcptr_end = &s->cirrus_bltbuf[0];
	
    if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
		if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
			s->cirrus_blt_srcpitch = 8;
		} else {
				/* XXX: check for 24 bpp */
			s->cirrus_blt_srcpitch = 8 * 8 * s->cirrus_blt_pixelwidth;
		}
		s->cirrus_srccounter = s->cirrus_blt_srcpitch;
    } else {
		if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
            w = s->cirrus_blt_width / s->cirrus_blt_pixelwidth;
            if (s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_DWORDGRANULARITY)
                s->cirrus_blt_srcpitch = ((w + 31) >> 5);
            else
                s->cirrus_blt_srcpitch = ((w + 7) >> 3);
		} else {
				/* always align input size to 32 bits */
			s->cirrus_blt_srcpitch = (s->cirrus_blt_width + 3) & ~3;
		}
        s->cirrus_srccounter = s->cirrus_blt_srcpitch * s->cirrus_blt_height;
    }
    s->cirrus_srcptr = s->cirrus_bltbuf;
	if(s->cirrus_blt_srcpitch <= sizeof(s->cirrus_bltbuf)){
		s->cirrus_srcptr_end = s->cirrus_bltbuf + s->cirrus_blt_srcpitch;
	}else{
		s->cirrus_srcptr_end = s->cirrus_bltbuf + sizeof(s->cirrus_bltbuf);
	}
    cirrus_update_memory_access(s);
    return 1;
}

static int cirrus_bitblt_videotocpu(CirrusVGAState * s)
{
	// np21/w  bitblt (video to cpu) 暫定プログラム（cputovideoからの推測・正しいかは不明）
    int w;
	
    s->cirrus_blt_mode &= ~CIRRUS_BLTMODE_MEMSYSSRC;
    s->cirrus_srcptr = &s->cirrus_bltbuf[0];
    s->cirrus_srcptr_end = &s->cirrus_bltbuf[0];
	
    if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
		if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
			s->cirrus_blt_dstpitch = 8;
		} else {
				/* XXX: check for 24 bpp */
			s->cirrus_blt_dstpitch = 8 * 8 * s->cirrus_blt_pixelwidth;
		}
		s->cirrus_srccounter = s->cirrus_blt_dstpitch;
    } else {
		if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
            w = s->cirrus_blt_width / s->cirrus_blt_pixelwidth;
            if (s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_DWORDGRANULARITY)
                s->cirrus_blt_dstpitch = ((w + 31) >> 5);
            else
                s->cirrus_blt_dstpitch = ((w + 7) >> 3);
		} else {
				/* always align input size to 32 bits */
			s->cirrus_blt_dstpitch = (s->cirrus_blt_width + 3) & ~3;
		}
        s->cirrus_srccounter = s->cirrus_blt_dstpitch * s->cirrus_blt_height;
    }
    s->cirrus_srcptr = s->cirrus_bltbuf;
	if(s->cirrus_blt_dstpitch <= sizeof(s->cirrus_bltbuf)){
		s->cirrus_srcptr_end = s->cirrus_bltbuf + s->cirrus_blt_dstpitch;
	}else{
		s->cirrus_srcptr_end = s->cirrus_bltbuf + sizeof(s->cirrus_bltbuf);
	}
	if(s->cirrus_blt_dstpitch<0){
		cirrus_update_memory_access(s);
	}
    cirrus_update_memory_access(s);

	memset(s->cirrus_bltbuf, 0, s->cirrus_blt_width);

	// Copy 1st data
	(*s->cirrus_rop)(s, s->cirrus_bltbuf, s->vram_ptr +
                    (s->cirrus_blt_srcaddr & s->cirrus_addr_mask),
                    0, 0, s->cirrus_blt_width, 1);
    return 1;
}

static int cirrus_bitblt_videotovideo(CirrusVGAState * s)
{
    int ret;

    if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
		ret = cirrus_bitblt_videotovideo_patterncopy(s);
    } else {
		ret = cirrus_bitblt_videotovideo_copy(s);
    }
    if (ret)
		cirrus_bitblt_reset(s);

	cirrus_bitblt_dblbufferswitch();

    return ret;
}

static void cirrus_bitblt_start(CirrusVGAState * s)
{
    uint8_t blt_rop;
	
    s->gr[0x31] |= CIRRUS_BLT_BUSY;

    s->cirrus_blt_width = (s->gr[0x20] | (s->gr[0x21] << 8)) + 1;
    s->cirrus_blt_height = (s->gr[0x22] | (s->gr[0x23] << 8)) + 1;
    s->cirrus_blt_dstpitch = (s->gr[0x24] | (s->gr[0x25] << 8));
    s->cirrus_blt_srcpitch = (s->gr[0x26] | (s->gr[0x27] << 8));
    s->cirrus_blt_dstaddr = (s->gr[0x28] | (s->gr[0x29] << 8) | (s->gr[0x2a] << 16));
    s->cirrus_blt_srcaddr = (s->gr[0x2c] | (s->gr[0x2d] << 8) | (s->gr[0x2e] << 16));
    s->cirrus_blt_mode = s->gr[0x30];
    blt_rop = s->gr[0x32];
	if(s->device_id == CIRRUS_ID_CLGD5446){
		s->cirrus_blt_modeext = s->gr[0x33];
	}else{
		s->cirrus_blt_modeext = 0;  // ver0.86 rev8
	}

#ifdef DEBUG_BITBLT
    printf("rop=0x%02x mode=0x%02x modeext=0x%02x w=%d h=%d dpitch=%d spitch=%d daddr=0x%08x saddr=0x%08x writemask=0x%02x\n",
           blt_rop,
           s->cirrus_blt_mode,
           s->cirrus_blt_modeext,
           s->cirrus_blt_width,
           s->cirrus_blt_height,
           s->cirrus_blt_dstpitch,
           s->cirrus_blt_srcpitch,
           s->cirrus_blt_dstaddr,
           s->cirrus_blt_srcaddr,
           s->gr[0x2f]);
#endif

    switch (s->cirrus_blt_mode & CIRRUS_BLTMODE_PIXELWIDTHMASK) {
		case CIRRUS_BLTMODE_PIXELWIDTH8:
			s->cirrus_blt_pixelwidth = 1;
			break;
		case CIRRUS_BLTMODE_PIXELWIDTH16:
			s->cirrus_blt_pixelwidth = 2;
			break;
		case CIRRUS_BLTMODE_PIXELWIDTH24:
			s->cirrus_blt_pixelwidth = 3;
			break;
		case CIRRUS_BLTMODE_PIXELWIDTH32:
			s->cirrus_blt_pixelwidth = 4;
			break;
		default:
#ifdef DEBUG_BITBLT
			printf("cirrus: bitblt - pixel width is unknown\n");
#endif
			goto bitblt_ignore;
    }
    s->cirrus_blt_mode &= ~CIRRUS_BLTMODE_PIXELWIDTHMASK;

    if ((s->
	 cirrus_blt_mode & (CIRRUS_BLTMODE_MEMSYSSRC | CIRRUS_BLTMODE_MEMSYSDEST))
				== (CIRRUS_BLTMODE_MEMSYSSRC | CIRRUS_BLTMODE_MEMSYSDEST)) {
#ifdef DEBUG_BITBLT
		printf("cirrus: bitblt - memory-to-memory copy is requested\n");
#endif
		goto bitblt_ignore;
    }

    if ((s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_SOLIDFILL) &&
        (s->cirrus_blt_mode & (CIRRUS_BLTMODE_MEMSYSDEST |
                               CIRRUS_BLTMODE_TRANSPARENTCOMP |
                               CIRRUS_BLTMODE_PATTERNCOPY |
                               CIRRUS_BLTMODE_COLOREXPAND)) ==
         (CIRRUS_BLTMODE_PATTERNCOPY | CIRRUS_BLTMODE_COLOREXPAND)) {
        cirrus_bitblt_fgcol(s);
        cirrus_bitblt_solidfill(s, blt_rop);
    } else {
        if ((s->cirrus_blt_mode & (CIRRUS_BLTMODE_COLOREXPAND |
                                   CIRRUS_BLTMODE_PATTERNCOPY)) ==
            CIRRUS_BLTMODE_COLOREXPAND) {

            if (s->cirrus_blt_mode & CIRRUS_BLTMODE_TRANSPARENTCOMP) {
                if (s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_COLOREXPINV)
                    cirrus_bitblt_bgcol(s);
                else
                    cirrus_bitblt_fgcol(s);
                s->cirrus_rop = cirrus_colorexpand_transp[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
            } else {
                cirrus_bitblt_fgcol(s);
                cirrus_bitblt_bgcol(s);
                s->cirrus_rop = cirrus_colorexpand[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
            }
        } else if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
            if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
                if (s->cirrus_blt_mode & CIRRUS_BLTMODE_TRANSPARENTCOMP) {
                    if (s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_COLOREXPINV)
                        cirrus_bitblt_bgcol(s);
                    else
                        cirrus_bitblt_fgcol(s);
                    s->cirrus_rop = cirrus_colorexpand_pattern_transp[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
                } else {
                    cirrus_bitblt_fgcol(s);
                    cirrus_bitblt_bgcol(s);
                    s->cirrus_rop = cirrus_colorexpand_pattern[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
                }
            } else {
                s->cirrus_rop = cirrus_patternfill[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
            }
        } else {
			if (s->cirrus_blt_mode & CIRRUS_BLTMODE_TRANSPARENTCOMP) {
				if (s->cirrus_blt_pixelwidth > 2) {
					printf("src transparent without colorexpand must be 8bpp or 16bpp\n");
					goto bitblt_ignore;
				}
				if (s->cirrus_blt_mode & CIRRUS_BLTMODE_BACKWARDS) {
					s->cirrus_blt_dstpitch = -s->cirrus_blt_dstpitch;
					s->cirrus_blt_srcpitch = -s->cirrus_blt_srcpitch;
					s->cirrus_rop = cirrus_bkwd_transp_rop[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
				} else {
					s->cirrus_rop = cirrus_fwd_transp_rop[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
				}
			} else {
				if (s->cirrus_blt_mode & CIRRUS_BLTMODE_BACKWARDS) {
					s->cirrus_blt_dstpitch = -s->cirrus_blt_dstpitch;
					s->cirrus_blt_srcpitch = -s->cirrus_blt_srcpitch;
					s->cirrus_rop = cirrus_bkwd_rop[rop_to_index[blt_rop]];
				} else {
					s->cirrus_rop = cirrus_fwd_rop[rop_to_index[blt_rop]];
				}
			}
		}
        // setup bitblt engine.
        if (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSSRC) {
            if (!cirrus_bitblt_cputovideo(s))
                goto bitblt_ignore;
        } else if (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSDEST) {
            if (!cirrus_bitblt_videotocpu(s))
                goto bitblt_ignore;
        } else {
            if (!cirrus_bitblt_videotovideo(s))
                goto bitblt_ignore;
        }
    }
    return;
  bitblt_ignore:;
    cirrus_bitblt_reset(s);
}

static void cirrus_write_bitblt(CirrusVGAState * s, unsigned reg_value)
{
    unsigned old_value;

    old_value = s->gr[0x31];
    s->gr[0x31] = reg_value & 0xee;

    if (((old_value & CIRRUS_BLT_RESET) != 0) &&
		((reg_value & CIRRUS_BLT_RESET) == 0)) {
		if(s->device_id == CIRRUS_ID_CLGD5446){
			cirrus_bitblt_reset(s);
		}else{
			cirrus_bitblt_start(s);// XXX: Win2000のハードウェアアクセラレーションを正常に動かすのに必要。根拠無し。
			if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
				// XXX: Win3.1の最初のBitBltが無視される問題の回避策
				if(!(old_value & 0x04)){
					cirrus_bitblt_reset(s);
				}
			}else{
				cirrus_bitblt_reset(s);
			}
		}
    } else if (((old_value & CIRRUS_BLT_START) == 0) &&
			   ((reg_value & CIRRUS_BLT_START) != 0)) {
		cirrus_bitblt_start(s);
	}
}

static void cirrus_bitblt_dblbufferswitch(){
	CirrusVGAState *s = cirrusvga;
	if(s->device_id == CIRRUS_ID_CLGD5446){
		if(s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_BLTSYNCDISP){
			if((s->cr[0x5e] & 0x7) == 0x7){
				s->graphics_dblbuf_index = (s->graphics_dblbuf_index + 1) & 0x1;
			}
			if((s->cr[0x5e] & 0x30) == 0x30){
				s->videowindow_dblbuf_index = (s->videowindow_dblbuf_index + 1) & 0x1;
			}
		}
	}
}


/***************************************
 *
 *  basic parameters
 *
 ***************************************/

static void cirrus_get_offsets(VGAState *s1,
                               uint32_t_ *pline_offset,
                               uint32_t_ *pstart_addr,
                               uint32_t_ *pline_compare)
{
    CirrusVGAState * s = (CirrusVGAState *)s1;
    uint32_t_ start_addr, line_offset, line_compare;

    line_offset = s->cr[0x13]
	| ((s->cr[0x1b] & 0x10) << 4);
    line_offset <<= 3;
    *pline_offset = line_offset;

    start_addr = (s->cr[0x0c] << 8)
	| s->cr[0x0d]
	| ((s->cr[0x1b] & 0x01) << 16)
	| ((s->cr[0x1b] & 0x0c) << 15)
	| ((s->cr[0x1d] & 0x80) << 12);
    *pstart_addr = start_addr;

    line_compare = s->cr[0x18] |
        ((s->cr[0x07] & 0x10) << 4) |
        ((s->cr[0x09] & 0x40) << 3);
    *pline_compare = line_compare;
}

static uint32_t_ cirrus_get_bpp16_depth(CirrusVGAState * s)
{
    uint32_t_ ret = 16;

    switch (s->cirrus_hidden_dac_data & 0xf) {
    case 0:
	ret = 15;
	break;			/* Sierra HiColor */
    case 1:
	ret = 16;
	break;			/* XGA HiColor */
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: invalid DAC value %x in 16bpp\n",
	       (s->cirrus_hidden_dac_data & 0xf));
#endif
	ret = 15;		/* XXX */
	break;
    }
    return ret;
}

static int cirrus_get_bpp(VGAState *s1)
{
    CirrusVGAState * s = (CirrusVGAState *)s1;
    uint32_t_ ret = 8;
	
    if ((s->sr[0x07] & 0x01) != 0) {
		/* Cirrus SVGA */
		switch (s->sr[0x07] & CIRRUS_SR7_BPP_MASK) {
		case CIRRUS_SR7_BPP_8:
			ret = 8;
			break;
		case CIRRUS_SR7_BPP_16_DOUBLEVCLK:
			ret = cirrus_get_bpp16_depth(s);
			break;
		case CIRRUS_SR7_BPP_24:
			ret = 24;
			break;
		case CIRRUS_SR7_BPP_16:
			ret = cirrus_get_bpp16_depth(s);
			break;
		case CIRRUS_SR7_BPP_32:
			ret = 32;
			break;
		default:
#ifdef DEBUG_CIRRUS
			printf("cirrus: unknown bpp - sr7=%x\n", s->sr[0x7]);
#endif
			ret = 8;
			break;
		}
    } else {
		//ret = 8;
		/* VGA */
		ret = 0;
    }
	
    return ret;
}

static void cirrus_get_resolution(VGAState *s, int *pwidth, int *pheight)
{
    int width, height;

    width = (s->cr[0x01] + 1) * 8;
    height = s->cr[0x12] |
        ((s->cr[0x07] & 0x02) << 7) |
        ((s->cr[0x07] & 0x40) << 3);
    height = (height + 1);
    /* interlace support */
    if (s->cr[0x1a] & 0x01)
        height = height * 2;
	if(width==320) height /= 2; // XXX: Win98で表示がおかしくなるのでとりあえず仮
	if(width==400) height = 300; // XXX: Win98で表示がおかしくなるのでとりあえず仮
	if(width==512) height = 384; // XXX: Win98で表示がおかしくなるのでとりあえず仮
	if(height >= width * 3 / 4 * 2){
		height /= 2; // XXX: 縦長過ぎるとき、高さ半分にしておく
	}
	
	// WSN 1280x1024
	if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WAB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		// XXX: WSN用やっつけ修正
		if(width==1280){
			height = 1024;
		}
	}

    *pwidth = width;
    *pheight = height;
}

/***************************************
 *
 * bank memory
 *
 ***************************************/

static void cirrus_update_bank_ptr(CirrusVGAState * s, unsigned bank_index)
{
    unsigned offset;
    unsigned limit;

    if ((s->gr[0x0b] & 0x01) != 0)	/* dual bank */
		offset = s->gr[0x09 + bank_index];
    else			/* single bank */
		offset = s->gr[0x09];

    if ((s->gr[0x0b] & 0x20) != 0)
		offset <<= 14;
    else
		offset <<= 12;

    if ((unsigned int)s->real_vram_size <= offset)
		limit = 0;
    else
		limit = s->real_vram_size - offset;

    if (((s->gr[0x0b] & 0x01) == 0) && (bank_index != 0)) {
		if (limit > 0x8000) {
			offset += 0x8000;
			limit -= 0x8000;
		} else {
			limit = 0;
		}
    }

    if (limit > 0) {
        /* Thinking about changing bank base? First, drop the dirty bitmap information
         * on the current location, otherwise we lose this pointer forever */
        if (s->lfb_vram_mapped) {
            target_phys_addr_t base_addr = isa_mem_base + 0xF80000 + bank_index * 0x8000;
            cpu_physical_sync_dirty_bitmap(base_addr, base_addr + 0x8000);
			TRACEOUT(("UNSUPPORT1"));
        }
		s->cirrus_bank_base[bank_index] = offset;
		s->cirrus_bank_limit[bank_index] = limit;
    } else {
		s->cirrus_bank_base[bank_index] = 0;
		s->cirrus_bank_limit[bank_index] = 0;
    }
}

/***************************************
 *
 *  I/O access between 0x3c4-0x3c5
 *
 ***************************************/

static int
cirrus_hook_read_sr(CirrusVGAState * s, unsigned reg_index, int *reg_value)
{
    switch (reg_index) {
    case 0x00:			// Standard VGA
    case 0x01:			// Standard VGA
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x06:			// Unlock Cirrus extensions
		*reg_value = s->sr[reg_index];
		break;
    case 0x10:
    case 0x30:
    case 0x50:
    case 0x70:			// Graphics Cursor X
    case 0x90:
    case 0xb0:
    case 0xd0:
    case 0xf0:			// Graphics Cursor X
		*reg_value = s->sr[0x10];
		break;
    case 0x11:
    case 0x31:
    case 0x51:
    case 0x71:			// Graphics Cursor Y
    case 0x91:
    case 0xb1:
    case 0xd1:
    case 0xf1:			// Graphics Cursor Y
		*reg_value = s->sr[0x11];
		break;
    case 0x05:			// ???
    case 0x07:			// Extended Sequencer Mode
//		cirrus_update_memory_access(s);
#ifdef DEBUG_CIRRUS
		printf("cirrus: handled inport sr_index %02x\n", reg_index);
#endif
		*reg_value = s->sr[reg_index];
		break;
    case 0x08:			// EEPROM Control
#if defined(SUPPORT_PCI)
		// XXX: Win2000でPnPデバイス検出を動かすのに必要。理由は謎
		if(pcidev.enable && np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
			*reg_value = cirrusvga->sr[0x08] = 0xFE;
			break;
		}
#endif
    case 0x09:			// Scratch Register 0
    case 0x0a:			// Scratch Register 1
    case 0x0b:			// VCLK 0
    case 0x0c:			// VCLK 1
    case 0x0d:			// VCLK 2
    case 0x0e:			// VCLK 3
    case 0x0f:			// DRAM Control
    case 0x12:			// Graphics Cursor Attribute
    case 0x13:			// Graphics Cursor Pattern Address
    case 0x14:			// Scratch Register 2
    case 0x15:			// Scratch Register 3
    case 0x16:			// Performance Tuning Register
    case 0x17:			// Configuration Readback and Extended Control
    case 0x18:			// Signature Generator Control
    case 0x19:			// Signal Generator Result
    case 0x1a:			// Signal Generator Result
    case 0x1b:			// VCLK 0 Denominator & Post
    case 0x1c:			// VCLK 1 Denominator & Post
    case 0x1d:			// VCLK 2 Denominator & Post
    case 0x1e:			// VCLK 3 Denominator & Post
    case 0x1f:			// BIOS Write Enable and MCLK select
#ifdef DEBUG_CIRRUS
		printf("cirrus: handled inport sr_index %02x\n", reg_index);
#endif
		*reg_value = s->sr[reg_index];
		break;
    default:
#ifdef DEBUG_CIRRUS
		printf("cirrus: inport sr_index %02x\n", reg_index);
#endif
		*reg_value = 0xff;
		break;
    }

    return CIRRUS_HOOK_HANDLED;
}

static int
cirrus_hook_write_sr(CirrusVGAState * s, unsigned reg_index, int reg_value)
{
    switch (reg_index) {
    case 0x00:			// Standard VGA
    case 0x01:			// Standard VGA
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x06:			// Unlock Cirrus extensions
		reg_value &= 0x17;
		if (reg_value == 0x12) {
			s->sr[reg_index] = 0x12;
		} else {
			s->sr[reg_index] = 0x0f;
		}
		break;
    case 0x10:
    case 0x30:
    case 0x50:
    case 0x70:			// Graphics Cursor X
    case 0x90:
    case 0xb0:
    case 0xd0:
    case 0xf0:			// Graphics Cursor X
		s->sr[0x10] = reg_value;
		s->hw_cursor_x = (reg_value << 3) | (reg_index >> 5);
	break;
    case 0x11:
    case 0x31:
    case 0x51:
    case 0x71:			// Graphics Cursor Y
    case 0x91:
    case 0xb1:
    case 0xd1:
    case 0xf1:			// Graphics Cursor Y
		s->sr[0x11] = reg_value;
		s->hw_cursor_y = (reg_value << 3) | (reg_index >> 5);
	break;
	case 0x07:			// Extended Sequencer Mode
//		cirrus_update_memory_access(s);
    case 0x08:			// EEPROM Control
    case 0x09:			// Scratch Register 0
    case 0x0a:			// Scratch Register 1
    case 0x0b:			// VCLK 0
    case 0x0c:			// VCLK 1
    case 0x0d:			// VCLK 2
    case 0x0e:			// VCLK 3
    case 0x0f:			// DRAM Control
    case 0x12:			// Graphics Cursor Attribute
    case 0x13:			// Graphics Cursor Pattern Address
    case 0x14:			// Scratch Register 2
    case 0x15:			// Scratch Register 3
    case 0x16:			// Performance Tuning Register
    case 0x18:			// Signature Generator Control
    case 0x19:			// Signature Generator Result
    case 0x1a:			// Signature Generator Result
    case 0x1b:			// VCLK 0 Denominator & Post
    case 0x1c:			// VCLK 1 Denominator & Post
    case 0x1d:			// VCLK 2 Denominator & Post
    case 0x1e:			// VCLK 3 Denominator & Post
    case 0x1f:			// BIOS Write Enable and MCLK select
		if(reg_index==0x07 && np2clvga.gd54xxtype < 0xff && np2clvga.gd54xxtype != CIRRUS_98ID_PCI){
			s->sr[reg_index] = (s->sr[reg_index] & 0xf0) | (reg_value & 0x0f);
		}else{
			s->sr[reg_index] = reg_value;
		}
		s->sr[reg_index] = reg_value;
		cirrus_update_memory_access(s);
#ifdef DEBUG_CIRRUS
		printf("cirrus: handled outport sr_index %02x, sr_value %02x\n",
			   reg_index, reg_value);
#endif
		break;
    case 0x17:			// Configuration Readback and Extended Control
		s->sr[reg_index] = (s->sr[reg_index] & 0x38) | (reg_value & 0xc7);
        cirrus_update_memory_access(s);
        break;
    default:
#ifdef DEBUG_CIRRUS
		printf("cirrus: outport sr_index %02x, sr_value %02x\n", reg_index,
			   reg_value);
#endif
		break;
    }

    return CIRRUS_HOOK_HANDLED;
}

/***************************************
 *
 *  I/O access at 0x3c6
 *
 ***************************************/

static void cirrus_read_hidden_dac(CirrusVGAState * s, int *reg_value)
{
    *reg_value = 0xff;
    if (++s->cirrus_hidden_dac_lockindex == 5) {
        *reg_value = s->cirrus_hidden_dac_data;
	s->cirrus_hidden_dac_lockindex = 0;
    }
}

static void cirrus_write_hidden_dac(CirrusVGAState * s, int reg_value)
{
    if (s->cirrus_hidden_dac_lockindex == 4) {
	s->cirrus_hidden_dac_data = reg_value;
#if defined(DEBUG_CIRRUS)
	printf("cirrus: outport hidden DAC, value %02x\n", reg_value);
#endif
    }
    s->cirrus_hidden_dac_lockindex = 0;
}

/***************************************
 *
 *  I/O access at 0x3c9
 *
 ***************************************/

static int cirrus_hook_read_palette(CirrusVGAState * s, int *reg_value)
{
    if (!(s->sr[0x12] & CIRRUS_CURSOR_HIDDENPEL))
	return CIRRUS_HOOK_NOT_HANDLED;
    *reg_value =
        s->cirrus_hidden_palette[(s->dac_read_index & 0x0f) * 3 +
                                 s->dac_sub_index];
    if (++s->dac_sub_index == 3) {
	s->dac_sub_index = 0;
	s->dac_read_index++;
    }
    return CIRRUS_HOOK_HANDLED;
}

static int cirrus_hook_write_palette(CirrusVGAState * s, int reg_value)
{
    if (!(s->sr[0x12] & CIRRUS_CURSOR_HIDDENPEL))
	return CIRRUS_HOOK_NOT_HANDLED;
    s->dac_cache[s->dac_sub_index] = reg_value;
    if (++s->dac_sub_index == 3) {
        memcpy(&s->cirrus_hidden_palette[(s->dac_write_index & 0x0f) * 3],
               s->dac_cache, 3);
        /* XXX update cursor */
	s->dac_sub_index = 0;
	s->dac_write_index++;
    }
    return CIRRUS_HOOK_HANDLED;
}

/***************************************
 *
 *  I/O access between 0x3ce-0x3cf
 *
 ***************************************/

static int
cirrus_hook_read_gr(CirrusVGAState * s, unsigned reg_index, int *reg_value)
{
    switch (reg_index) {
    case 0x00: // Standard VGA, BGCOLOR 0x000000ff
      *reg_value = s->cirrus_shadow_gr0;
      return CIRRUS_HOOK_HANDLED;
    case 0x01: // Standard VGA, FGCOLOR 0x000000ff
      *reg_value = s->cirrus_shadow_gr1;
      return CIRRUS_HOOK_HANDLED;
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x06:			// Standard VGA
		*reg_value = s->gr[0x06];
		return CIRRUS_HOOK_HANDLED;
    case 0x07:			// Standard VGA
    case 0x08:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x05:			// Standard VGA, Cirrus extended mode
	default:
		break;
    }

    if (reg_index < 0x3a) {
		*reg_value = s->gr[reg_index];
    } else {
#ifdef DEBUG_CIRRUS
		printf("cirrus: inport gr_index %02x\n", reg_index);
#endif
		*reg_value = 0xff;
    }

    return CIRRUS_HOOK_HANDLED;
}

static int
cirrus_hook_write_gr(CirrusVGAState * s, unsigned reg_index, int reg_value)
{
#if defined(DEBUG_BITBLT) && 0
    printf("gr%02x: %02x\n", reg_index, reg_value);
#endif
    switch (reg_index) {
    case 0x00:			// Standard VGA, BGCOLOR 0x000000ff
		s->cirrus_shadow_gr0 = reg_value;
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x01:			// Standard VGA, FGCOLOR 0x000000ff
		s->cirrus_shadow_gr1 = reg_value;
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
	case 0x06:			// Standard VGA
		s->gr[reg_index] = reg_value & 0x0f;
		cirrus_update_memory_access(s);
		break;
    case 0x07:			// Standard VGA
    case 0x08:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x05:			// Standard VGA, Cirrus extended mode
		s->gr[reg_index] = reg_value & 0x7f;
        cirrus_update_memory_access(s);
		break;
    case 0x09:			// bank offset #0
    case 0x0A:			// bank offset #1
		s->gr[reg_index] = reg_value;
		cirrus_update_bank_ptr(s, 0);
		cirrus_update_bank_ptr(s, 1);
        cirrus_update_memory_access(s);
        break;
    case 0x0B:
		s->gr[reg_index] = reg_value;
		cirrus_update_bank_ptr(s, 0);
		cirrus_update_bank_ptr(s, 1);
        cirrus_update_memory_access(s);
		break;
    case 0x0e:			// Power Management (CL-GD5446)
    case 0x10:			// BGCOLOR 0x0000ff00
    case 0x11:			// FGCOLOR 0x0000ff00
    case 0x12:			// BGCOLOR 0x00ff0000
    case 0x13:			// FGCOLOR 0x00ff0000
    case 0x14:			// BGCOLOR 0xff000000
    case 0x15:			// FGCOLOR 0xff000000
    case 0x20:			// BLT WIDTH 0x0000ff
    case 0x22:			// BLT HEIGHT 0x0000ff
    case 0x24:			// BLT DEST PITCH 0x0000ff
    case 0x26:			// BLT SRC PITCH 0x0000ff
    case 0x28:			// BLT DEST ADDR 0x0000ff
    case 0x29:			// BLT DEST ADDR 0x00ff00
    case 0x2c:			// BLT SRC ADDR 0x0000ff
    case 0x2d:			// BLT SRC ADDR 0x00ff00
    case 0x2f:          // BLT WRITEMASK
    case 0x30:			// BLT MODE
    case 0x32:			// RASTER OP
    case 0x34:			// BLT TRANSPARENT COLOR 0x00ff
    case 0x35:			// BLT TRANSPARENT COLOR 0xff00
    case 0x38:			// BLT TRANSPARENT COLOR MASK 0x00ff
    case 0x39:			// BLT TRANSPARENT COLOR MASK 0xff00
		s->gr[reg_index] = reg_value;
		break;
    case 0x21:			// BLT WIDTH 0x001f00
    case 0x23:			// BLT HEIGHT 0x001f00
    case 0x25:			// BLT DEST PITCH 0x001f00
    case 0x27:			// BLT SRC PITCH 0x001f00
		s->gr[reg_index] = reg_value & 0x1f;
		break;
    case 0x2a:			// BLT DEST ADDR 0x3f0000
		s->gr[reg_index] = reg_value & 0x3f;
        /* if auto start mode, starts bit blt now */
        if (s->gr[0x31] & CIRRUS_BLT_AUTOSTART) {
            cirrus_bitblt_start(s);
        }
		break;
    case 0x2e:			// BLT SRC ADDR 0x3f0000
		s->gr[reg_index] = reg_value & 0x3f;
		break;
    case 0x31:			// BLT STATUS/START
		cirrus_write_bitblt(s, reg_value);
		//cirrus_write_bitblt(s, reg_value);
		break;
    case 0x33:			// BLT MODEEXT
		if(s->device_id != CIRRUS_ID_CLGD5446 ||
		   ((s->gr[0x0e] & 0x20) || (s->gr[0x31] & 0x80)) && (s->cr[0x5e] & 0x20)) {
			s->gr[reg_index] = reg_value;
		}
		
		break;
	default:
#ifdef DEBUG_CIRRUS
		printf("cirrus: outport gr_index %02x, gr_value %02x\n", reg_index,
			   reg_value);
#endif
		break;
    }

    return CIRRUS_HOOK_HANDLED;
}

/***************************************
 *
 *  I/O access between 0x3d4-0x3d5
 *
 ***************************************/

static int
cirrus_hook_read_cr(CirrusVGAState * s, unsigned reg_index, int *reg_value)
{
    switch (reg_index) {
    case 0x00:			// Standard VGA
    case 0x01:			// Standard VGA
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
    case 0x05:			// Standard VGA
    case 0x06:			// Standard VGA
    case 0x07:			// Standard VGA
    case 0x08:			// Standard VGA
    case 0x09:			// Standard VGA
    case 0x0a:			// Standard VGA
    case 0x0b:			// Standard VGA
    case 0x0c:			// Standard VGA
    case 0x0d:			// Standard VGA
    case 0x0e:			// Standard VGA
    case 0x0f:			// Standard VGA
    case 0x10:			// Standard VGA
    case 0x11:			// Standard VGA
    case 0x12:			// Standard VGA
    case 0x13:			// Standard VGA
    case 0x14:			// Standard VGA
    case 0x15:			// Standard VGA
    case 0x16:			// Standard VGA
    case 0x17:			// Standard VGA
    case 0x18:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x24:			// Attribute Controller Toggle Readback (R)
        *reg_value = (s->ar_flip_flop << 7);
        break;
    case 0x19:			// Interlace End
    case 0x1a:			// Miscellaneous Control
    case 0x1b:			// Extended Display Control
    case 0x1c:			// Sync Adjust and Genlock
    case 0x1d:			// Overlay Extended Control
    case 0x22:			// Graphics Data Latches Readback (R)
		*reg_value = s->cr[reg_index];
        break;
    case 0x25:			// Part Status
		*reg_value = s->cr[reg_index];
        break;
    case 0x27:			// Part ID (R)
		if(np2clvga.gd54xxtype <= 0xff){
			*reg_value = s->cr[reg_index];
		}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
			*reg_value = CIRRUS_ID_CLGD5428;
		}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
			*reg_value = CIRRUS_ID_CLGD5434;
		}else if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
			*reg_value = CIRRUS_ID_CLGD5434;
		}else{
			*reg_value = s->cr[reg_index];
		}
		s->cr[0x5e] |= 0x20; // XXX: Part IDを読んだらGR33を書き込み可能にする（WinNT4専用の不具合回避）
        break;
    case 0x28:			// Class ID Register
		if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
			*reg_value = 0x03;
		}else if ((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC) {
 			*reg_value = 0x03;
		}
		break;
    case 0x26:			// Attribute Controller Index Readback (R)
		*reg_value = s->ar_index & 0x3f;
		break;
    case 0x31:			// Video Window Horizontal Zoom Control
    case 0x32:			// Video Window Vertical Zoom Control
    case 0x33:			// Video Window Horizontal Region 1 Size
    case 0x34:			// Video Window Region 2 Skip Size
    case 0x35:			// Video Window Region 2 Active Size
    case 0x36:			// Video Window Horizontal Overflow
    case 0x37:			// Video Window Vertical Start
    case 0x38:			// Video Window Vertical End
    case 0x39:			// Video Window Vertical Overflow
    case 0x3a:			// Video Buffer 1 Start Address Byte 0
    case 0x3b:			// Video Buffer 1 Start Address Byte 1
    case 0x3c:			// Video Buffer 1 Start Address Byte 2
    case 0x3d:			// Video Window Address Offset
    case 0x3e:			// Video Window Master Control
    case 0x3f:			// Host Video Data Path Control
    case 0x5d:			// Video Window Pixel Alignment
		if(s->device_id >= CIRRUS_ID_CLGD5440){
			*reg_value = s->cr[reg_index];
		}
		break;
    case 0x5e:			// Double Buffer Control (CL-GD5446)
		if(s->device_id == CIRRUS_ID_CLGD5446){
			*reg_value = s->cr[reg_index];
		}
        break;
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: inport cr_index %02x\n", reg_index);
	*reg_value = 0xff;
#endif
	break;
    }

    return CIRRUS_HOOK_HANDLED;
}

static int
cirrus_hook_write_cr(CirrusVGAState * s, unsigned reg_index, int reg_value)
{
    switch (reg_index) {
    case 0x00:			// Standard VGA
    case 0x01:			// Standard VGA
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
    case 0x05:			// Standard VGA
    case 0x06:			// Standard VGA
    case 0x07:			// Standard VGA
    case 0x08:			// Standard VGA
    case 0x09:			// Standard VGA
    case 0x0a:			// Standard VGA
    case 0x0b:			// Standard VGA
    case 0x0c:			// Standard VGA
    case 0x0d:			// Standard VGA
    case 0x0e:			// Standard VGA
    case 0x0f:			// Standard VGA
    case 0x10:			// Standard VGA
    case 0x11:			// Standard VGA
    case 0x12:			// Standard VGA
    case 0x13:			// Standard VGA
    case 0x14:			// Standard VGA
    case 0x15:			// Standard VGA
    case 0x16:			// Standard VGA
    case 0x17:			// Standard VGA
    case 0x18:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x19:			// Interlace End
    case 0x1a:			// Miscellaneous Control
    case 0x1b:			// Extended Display Control
    case 0x1c:			// Sync Adjust and Genlock
    case 0x1d:			// Overlay Extended Control
		s->cr[reg_index] = reg_value;
#ifdef DEBUG_CIRRUS
		printf("cirrus: handled outport cr_index %02x, cr_value %02x\n",
		       reg_index, reg_value);
#endif
		break;
    case 0x22:			// Graphics Data Latches Readback (R)
    case 0x24:			// Attribute Controller Toggle Readback (R)
    case 0x26:			// Attribute Controller Index Readback (R)
    case 0x27:			// Part ID (R)
		break;
    case 0x31:			// Video Window Horizontal Zoom Control
    case 0x32:			// Video Window Vertical Zoom Control
    case 0x33:			// Video Window Horizontal Region 1 Size
    case 0x34:			// Video Window Region 2 Skip Size
    case 0x35:			// Video Window Region 2 Active Size
    case 0x36:			// Video Window Horizontal Overflow
    case 0x37:			// Video Window Vertical Start
    case 0x38:			// Video Window Vertical End
    case 0x39:			// Video Window Vertical Overflow
    case 0x3a:			// Video Buffer 1 Start Address Byte 0
    case 0x3b:			// Video Buffer 1 Start Address Byte 1
    case 0x3c:			// Video Buffer 1 Start Address Byte 2
    case 0x3d:			// Video Window Address Offset
    case 0x3e:			// Video Window Master Control
    case 0x3f:			// Host Video Data Path Control
    case 0x5d:			// Video Window Pixel Alignment
		if(s->device_id >= CIRRUS_ID_CLGD5440){
			s->cr[reg_index] = reg_value;
		}
		break;
    case 0x5e:			// Double Buffer Control (CL-GD5446)
		if(s->device_id == CIRRUS_ID_CLGD5446){
			s->cr[reg_index] = reg_value;
			switch(reg_value & 0x7){
			case 0: // Compatible VGA display address control
				s->graphics_dblbuf_index = 0;
				break;
			case 1: // VSYNC switching
				break;
			case 2: // Forces graphics buffer 1 as display
				s->graphics_dblbuf_index = 0;
				break;
			case 3: // Forces graphics buffer 2 as display
				s->graphics_dblbuf_index = 1;
				break;
			case 4: // A18 controls switching
				break;
			case 5: // A19 controls switching
				break;
			case 6: // Reserved
				break;
			case 7: // Bitblt switches
				break;
			}
		}
        break;
    case 0x25:			// Part Status
    default:
#ifdef DEBUG_CIRRUS
		printf("cirrus: outport cr_index %02x, cr_value %02x\n", reg_index,
		       reg_value);
#endif
		break;
    }

    return CIRRUS_HOOK_HANDLED;
}

/***************************************
 *
 *  memory-mapped I/O (bitblt)
 *
 ***************************************/

static uint8_t cirrus_mmio_blt_read(CirrusVGAState * s, unsigned address)
{
    int value = 0xff;

    switch (address) {
    case (CIRRUS_MMIO_BLTBGCOLOR + 0):
	cirrus_hook_read_gr(s, 0x00, &value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 1):
	cirrus_hook_read_gr(s, 0x10, &value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 2):
	cirrus_hook_read_gr(s, 0x12, &value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 3):
	cirrus_hook_read_gr(s, 0x14, &value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 0):
	cirrus_hook_read_gr(s, 0x01, &value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 1):
	cirrus_hook_read_gr(s, 0x11, &value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 2):
	cirrus_hook_read_gr(s, 0x13, &value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 3):
	cirrus_hook_read_gr(s, 0x15, &value);
	break;
    case (CIRRUS_MMIO_BLTWIDTH + 0):
	cirrus_hook_read_gr(s, 0x20, &value);
	break;
    case (CIRRUS_MMIO_BLTWIDTH + 1):
	cirrus_hook_read_gr(s, 0x21, &value);
	break;
    case (CIRRUS_MMIO_BLTHEIGHT + 0):
	cirrus_hook_read_gr(s, 0x22, &value);
	break;
    case (CIRRUS_MMIO_BLTHEIGHT + 1):
	cirrus_hook_read_gr(s, 0x23, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTPITCH + 0):
	cirrus_hook_read_gr(s, 0x24, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTPITCH + 1):
	cirrus_hook_read_gr(s, 0x25, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCPITCH + 0):
	cirrus_hook_read_gr(s, 0x26, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCPITCH + 1):
	cirrus_hook_read_gr(s, 0x27, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 0):
	cirrus_hook_read_gr(s, 0x28, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 1):
	cirrus_hook_read_gr(s, 0x29, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 2):
	cirrus_hook_read_gr(s, 0x2a, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 0):
	cirrus_hook_read_gr(s, 0x2c, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 1):
	cirrus_hook_read_gr(s, 0x2d, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 2):
	cirrus_hook_read_gr(s, 0x2e, &value);
	break;
    case CIRRUS_MMIO_BLTWRITEMASK:
	cirrus_hook_read_gr(s, 0x2f, &value);
	break;
    case CIRRUS_MMIO_BLTMODE:
	cirrus_hook_read_gr(s, 0x30, &value);
	break;
    case CIRRUS_MMIO_BLTROP:
	cirrus_hook_read_gr(s, 0x32, &value);
	break;
    case CIRRUS_MMIO_BLTMODEEXT:
	cirrus_hook_read_gr(s, 0x33, &value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLOR + 0):
	cirrus_hook_read_gr(s, 0x34, &value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLOR + 1):
	cirrus_hook_read_gr(s, 0x35, &value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK + 0):
	cirrus_hook_read_gr(s, 0x38, &value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK + 1):
	cirrus_hook_read_gr(s, 0x39, &value);
	break;
    case CIRRUS_MMIO_BLTSTATUS:
	cirrus_hook_read_gr(s, 0x31, &value);
	break;
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: mmio read - address 0x%04x\n", address);
#endif
	break;
    }

    return (uint8_t) value;
}

static void cirrus_mmio_blt_write(CirrusVGAState * s, unsigned address,
				  uint8_t value)
{
    switch (address) {
    case (CIRRUS_MMIO_BLTBGCOLOR + 0):
	cirrus_hook_write_gr(s, 0x00, value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 1):
	cirrus_hook_write_gr(s, 0x10, value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 2):
	cirrus_hook_write_gr(s, 0x12, value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 3):
	cirrus_hook_write_gr(s, 0x14, value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 0):
	cirrus_hook_write_gr(s, 0x01, value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 1):
	cirrus_hook_write_gr(s, 0x11, value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 2):
	cirrus_hook_write_gr(s, 0x13, value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 3):
	cirrus_hook_write_gr(s, 0x15, value);
	break;
    case (CIRRUS_MMIO_BLTWIDTH + 0):
	cirrus_hook_write_gr(s, 0x20, value);
	break;
    case (CIRRUS_MMIO_BLTWIDTH + 1):
	cirrus_hook_write_gr(s, 0x21, value);
	break;
    case (CIRRUS_MMIO_BLTHEIGHT + 0):
	cirrus_hook_write_gr(s, 0x22, value);
	break;
    case (CIRRUS_MMIO_BLTHEIGHT + 1):
	cirrus_hook_write_gr(s, 0x23, value);
	break;
    case (CIRRUS_MMIO_BLTDESTPITCH + 0):
	cirrus_hook_write_gr(s, 0x24, value);
	break;
    case (CIRRUS_MMIO_BLTDESTPITCH + 1):
	cirrus_hook_write_gr(s, 0x25, value);
	break;
    case (CIRRUS_MMIO_BLTSRCPITCH + 0):
	cirrus_hook_write_gr(s, 0x26, value);
	break;
    case (CIRRUS_MMIO_BLTSRCPITCH + 1):
	cirrus_hook_write_gr(s, 0x27, value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 0):
	cirrus_hook_write_gr(s, 0x28, value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 1):
	cirrus_hook_write_gr(s, 0x29, value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 2):
	cirrus_hook_write_gr(s, 0x2a, value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 3):
	/* ignored */
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 0):
	cirrus_hook_write_gr(s, 0x2c, value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 1):
	cirrus_hook_write_gr(s, 0x2d, value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 2):
	cirrus_hook_write_gr(s, 0x2e, value);
	break;
    case CIRRUS_MMIO_BLTWRITEMASK:
	cirrus_hook_write_gr(s, 0x2f, value);
	break;
    case CIRRUS_MMIO_BLTMODE:
	cirrus_hook_write_gr(s, 0x30, value);
	break;
    case CIRRUS_MMIO_BLTROP:
	cirrus_hook_write_gr(s, 0x32, value);
	break;
    case CIRRUS_MMIO_BLTMODEEXT:
	cirrus_hook_write_gr(s, 0x33, value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLOR + 0):
	cirrus_hook_write_gr(s, 0x34, value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLOR + 1):
	cirrus_hook_write_gr(s, 0x35, value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK + 0):
	cirrus_hook_write_gr(s, 0x38, value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK + 1):
	cirrus_hook_write_gr(s, 0x39, value);
	break;
    case CIRRUS_MMIO_BLTSTATUS:
	cirrus_hook_write_gr(s, 0x31, value);
	break;
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: mmio write - addr 0x%04x val 0x%02x (ignored)\n",
	       address, value);
#endif
	break;
    }
}

/***************************************
 *
 *  write mode 4/5
 *
 * assume TARGET_PAGE_SIZE >= 16
 *
 ***************************************/

static void cirrus_mem_writeb_mode4and5_8bpp(CirrusVGAState * s,
					     unsigned mode,
					     unsigned offset,
					     uint32_t_ mem_value)
{
    int x;
    unsigned val = mem_value;
    unsigned mask = s->sr[0x2];
    uint8_t *dst;
	if(s->gr[0xb] & 0x04){
	}else{
		mask = 0xff;
	}

    dst = s->vram_ptr + (offset &= s->cirrus_addr_mask);
    for (x = 0; x < 8; x++) {
		if (mask & 0x80) {
			if (val & 0x80) {
				*dst = s->cirrus_shadow_gr1;
			} else if (mode == 5) {
				*dst = s->cirrus_shadow_gr0;
			}
		}
		val <<= 1;
		mask <<= 1;
		dst++;
    }
    cpu_physical_memory_set_dirty(s->vram_offset + offset);
    cpu_physical_memory_set_dirty(s->vram_offset + offset + 7);
}

static void cirrus_mem_writeb_mode4and5_16bpp(CirrusVGAState * s,
					      unsigned mode,
					      unsigned offset,
					      uint32_t_ mem_value)
{
    int x;
    unsigned val = mem_value;
    unsigned mask = s->sr[0x2];
    uint8_t *dst;
	
	if(s->gr[0xb] & 0x04){
	}else{
		mask = 0xffff;
	}
	if(s->gr[0xb] & 0x10){
		// SR2 Doubling Enabled
		mask |= mask << 8;
	}

    dst = s->vram_ptr + (offset &= s->cirrus_addr_mask);
    for (x = 0; x < 8; x++) {
		if (mask & 0x80) {
			if (val & 0x80) {
				*dst = s->cirrus_shadow_gr1;
				*(dst + 1) = s->gr[0x11];
			} else if (mode == 5) {
				*dst = s->cirrus_shadow_gr0;
				*(dst + 1) = s->gr[0x10];
			}
		}
		val <<= 1;
		mask <<= 1;
		dst += 2;
    }
    cpu_physical_memory_set_dirty(s->vram_offset + offset);
    cpu_physical_memory_set_dirty(s->vram_offset + offset + 15);
}

/***************************************
 *
 *  memory access between 0xa0000-0xbffff
 *
 ***************************************/

#define ADDR_SH1	0x000

uint32_t_ cirrus_vga_mem_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState*)opaque;
    unsigned bank_index;
    unsigned bank_offset;
    uint32_t_ val;
	

	addr += ADDR_SH1;

    if ((s->sr[0x07] & 0x01) == 0) {
	return vga_mem_readb(s, addr);
    }
    addr &= 0x1ffff;

    if (addr < 0x10000) {
		/* XXX handle bitblt */
		/* video memory */
		bank_index = addr >> 15;
		bank_offset = addr & 0x7fff;
		if (bank_offset < s->cirrus_bank_limit[bank_index]) {
			bank_offset += s->cirrus_bank_base[bank_index];
			if ((s->gr[0x0B] & 0x14) == 0x14) {
				bank_offset <<= 4;
			} else if (s->gr[0x0B] & 0x02) {
				bank_offset <<= 3;
			}
			bank_offset &= s->cirrus_addr_mask;
			val = *(s->vram_ptr + bank_offset);
		} else
			val = 0xff;
	} else if (addr >= 0x18000 && addr < 0x18100) {
		/* memory-mapped I/O */
		val = 0xff;
		if ((s->sr[0x17] & 0x44) == 0x04) {
			val = cirrus_mmio_blt_read(s, addr & 0xff);
		}
	} else {
		val = 0xff;
#ifdef DEBUG_CIRRUS
		printf("cirrus: mem_readb %06x\n", addr);
#endif
    }
    return val;
}

uint32_t_ cirrus_vga_mem_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_vga_mem_readb(opaque, addr) << 8;
    v |= cirrus_vga_mem_readb(opaque, addr + 1);
#else
    v = cirrus_vga_mem_readb(opaque, addr);
    v |= cirrus_vga_mem_readb(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_vga_mem_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_vga_mem_readb(opaque, addr) << 24;
    v |= cirrus_vga_mem_readb(opaque, addr + 1) << 16;
    v |= cirrus_vga_mem_readb(opaque, addr + 2) << 8;
    v |= cirrus_vga_mem_readb(opaque, addr + 3);
#else
    v = cirrus_vga_mem_readb(opaque, addr);
    v |= cirrus_vga_mem_readb(opaque, addr + 1) << 8;
    v |= cirrus_vga_mem_readb(opaque, addr + 2) << 16;
    v |= cirrus_vga_mem_readb(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_vga_mem_writeb(void *opaque, target_phys_addr_t addr,
                                  uint32_t_ mem_value)
{
    CirrusVGAState *s = (CirrusVGAState*)opaque;
    unsigned bank_index;
    unsigned bank_offset;
    unsigned mode;
	
	addr += ADDR_SH1;

    if ((s->sr[0x07] & 0x01) == 0) {
	vga_mem_writeb(s, addr, mem_value);
        return;
    }
	
    addr &= 0x1ffff;

    if (addr < 0x10000) {
		if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
			/* bitblt */
			*s->cirrus_srcptr++ = (uint8_t) mem_value;
			*s->cirrus_srcptr++;
			if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
				cirrus_bitblt_cputovideo_next(s);
			}
		} else {
			/* video memory */
			bank_index = addr >> 15;
			bank_offset = addr & 0x7fff;
			if (bank_offset < s->cirrus_bank_limit[bank_index]) {
				bank_offset += s->cirrus_bank_base[bank_index];
				if ((s->gr[0x0B] & 0x14) == 0x14) {
					bank_offset <<= 4;
				} else if (s->gr[0x0B] & 0x02) {
					bank_offset <<= 3;
				}
				bank_offset &= s->cirrus_addr_mask;
				mode = s->gr[0x05] & 0x7;
				if (mode < 4 || mode > 5 || ((s->gr[0x0B] & 0x4) == 0)) {
					*(s->vram_ptr + bank_offset) = mem_value;
					cpu_physical_memory_set_dirty(s->vram_offset +
								  bank_offset);
				} else {
					if ((s->gr[0x0B] & 0x14) != 0x14) {
						cirrus_mem_writeb_mode4and5_8bpp(s, mode,
									 bank_offset,
									 mem_value);
					} else {
						cirrus_mem_writeb_mode4and5_16bpp(s, mode,
									  bank_offset,
									  mem_value);
					}
				}
			}
		}
    } else if (addr >= 0x18000 && addr < 0x18100) {
		/* memory-mapped I/O */
		if ((s->sr[0x17] & 0x44) == 0x04) {
			cirrus_mmio_blt_write(s, addr & 0xff, mem_value);
		}
    } else {
#ifdef DEBUG_CIRRUS
	printf("cirrus: mem_writeb %06x value %02x\n", addr, mem_value);
#endif
    }
}

void cirrus_vga_mem_writew(void *opaque, target_phys_addr_t addr, uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_vga_mem_writeb(opaque, addr, (val >> 8) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 1, val & 0xff);
#else
    cirrus_vga_mem_writeb(opaque, addr, val & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_vga_mem_writel(void *opaque, target_phys_addr_t addr, uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_vga_mem_writeb(opaque, addr, (val >> 24) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 3, val & 0xff);
#else
    cirrus_vga_mem_writeb(opaque, addr, val & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}

static CPUReadMemoryFunc *cirrus_vga_mem_read[3] = {
    cirrus_vga_mem_readb,
    cirrus_vga_mem_readw,
    cirrus_vga_mem_readl,
};

static CPUWriteMemoryFunc *cirrus_vga_mem_write[3] = {
    cirrus_vga_mem_writeb,
    cirrus_vga_mem_writew,
    cirrus_vga_mem_writel,
};

/***************************************
 *
 *  hardware cursor
 *
 ***************************************/

static void invalidate_cursor1(CirrusVGAState *s)
{
    if (s->last_hw_cursor_size) {
        vga_invalidate_scanlines((VGAState *)s,
                                 s->last_hw_cursor_y + s->last_hw_cursor_y_start,
                                 s->last_hw_cursor_y + s->last_hw_cursor_y_end);
    }
}

static void cirrus_cursor_compute_yrange(CirrusVGAState *s)
{
    const uint8_t *src;
    uint32_t_ content;
    int y, y_min, y_max;

    src = s->vram_ptr + s->real_vram_size - 16 * 1024;
    if (s->sr[0x12] & CIRRUS_CURSOR_LARGE) {
        src += (s->sr[0x13] & 0x3c) * 256;
        y_min = 64;
        y_max = -1;
        for(y = 0; y < 64; y++) {
            content = ((uint32_t_ *)src)[0] |
                ((uint32_t_ *)src)[1] |
                ((uint32_t_ *)src)[2] |
                ((uint32_t_ *)src)[3];
            if (content) {
                if (y < y_min)
                    y_min = y;
                if (y > y_max)
                    y_max = y;
            }
            src += 16;
        }
    } else {
        src += (s->sr[0x13] & 0x3f) * 256;
        y_min = 32;
        y_max = -1;
        for(y = 0; y < 32; y++) {
            content = ((uint32_t_ *)src)[0] |
                ((uint32_t_ *)(src + 128))[0];
            if (content) {
                if (y < y_min)
                    y_min = y;
                if (y > y_max)
                    y_max = y;
            }
            src += 4;
        }
    }
    if (y_min > y_max) {
        s->last_hw_cursor_y_start = 0;
        s->last_hw_cursor_y_end = 0;
    } else {
        s->last_hw_cursor_y_start = y_min;
        s->last_hw_cursor_y_end = y_max + 1;
    }
}

/* NOTE: we do not currently handle the cursor bitmap change, so we
   update the cursor only if it moves. */
static void cirrus_cursor_invalidate(VGAState *s1)
{
    CirrusVGAState *s = (CirrusVGAState *)s1;
    int size;

    if (!s->sr[0x12] & CIRRUS_CURSOR_SHOW) {
        size = 0;
    } else {
        if (s->sr[0x12] & CIRRUS_CURSOR_LARGE)
            size = 64;
        else
            size = 32;
    }
    /* invalidate last cursor and new cursor if any change */
    if (s->last_hw_cursor_size != size ||
        s->last_hw_cursor_x != s->hw_cursor_x ||
        s->last_hw_cursor_y != s->hw_cursor_y) {

        invalidate_cursor1(s);

        s->last_hw_cursor_size = size;
        s->last_hw_cursor_x = s->hw_cursor_x;
        s->last_hw_cursor_y = s->hw_cursor_y;
        /* compute the real cursor min and max y */
        cirrus_cursor_compute_yrange(s);
        invalidate_cursor1(s);
    }
}

static int get_depth_index(DisplayState *s)
{
    switch(ds_get_bits_per_pixel(s)) {
    default:
    case 8:
		return 0;
    case 15:
        return 1;
    case 16:
        return 2;
    case 32:
        return 3;
    }
}
static void cirrus_cursor_draw_line(VGAState *s1, uint8_t *d1, int scr_y)
{
}

/***************************************
 *
 *  LFB memory access
 *
 ***************************************/

void cirrus_linear_mmio_update(void *opaque)
{
#if defined(SUPPORT_IA32_HAXM)
    CirrusVGAState *s = (CirrusVGAState *) opaque;
    uint32_t_ ret;
	uint8_t mode;
	uint8_t linmmio = (s->sr[0x17] & 0x44) == 0x44;
	mode = s->gr[0x05] & 0x7;

	if((0) ||
		(s->cirrus_srcptr != s->cirrus_srcptr_end) ||
		((s->gr[0x0B] & 0x14) == 0x14) ||
		(s->gr[0x0B] & 0x02) ||
		!(mode < 4 || mode > 5 || ((s->gr[0x0B] & 0x4) == 0))){
		if(mmio_mode != MMIO_MODE_MMIO){
			if(np2clvga.VRAMWindowAddr){
				if(mmio_mode_region1) i386hax_vm_removememoryarea(vramptr, mmio_mode_region1, cirrusvga->real_vram_size - (mmio_mode==MMIO_MODE_VRAM1 ? 0x1000 : 0));
			}
			mmio_mode_region1 = 0;
			if(np2clvga.pciLFB_Addr){
				if(mmio_mode_region2) i386hax_vm_removememoryarea(vramptr, mmio_mode_region2, cirrusvga->real_vram_size);
			}
			mmio_mode_region2 = 0;
			lastlinmmio = linmmio;
			mmio_mode = MMIO_MODE_MMIO;
		}
	}else{
		//if((s->sr[0x17] & 0x44) == 0x44 && (s->gr[0x6] & 0x0c) == 0x04){
		//	if(mmio_mode != MMIO_MODE_VRAM1 || np2clvga.VRAMWindowAddr!=mmio_mode_region1  || np2clvga.pciLFB_Addr!=mmio_mode_region2 || lastlinmmio!=linmmio){
		//		if(np2clvga.VRAMWindowAddr){
		//			if(mmio_mode_region1) i386hax_vm_removememoryarea(vramptr, mmio_mode_region1, cirrusvga->real_vram_size);
		//			i386hax_vm_setmemoryarea(vramptr, np2clvga.VRAMWindowAddr, cirrusvga->real_vram_size - 0x1000);
		//		}
		//		mmio_mode_region1 = np2clvga.VRAMWindowAddr;
		//		if(np2clvga.pciLFB_Addr){
		//			if(mmio_mode_region2) i386hax_vm_removememoryarea(vramptr, mmio_mode_region2, linmmio ? s->linear_mmio_mask : cirrusvga->real_vram_size);
		//			i386hax_vm_setmemoryarea(vramptr, np2clvga.pciLFB_Addr, linmmio ? s->linear_mmio_mask : cirrusvga->real_vram_size);
		//		}
		//		mmio_mode_region2 = np2clvga.pciLFB_Addr;
		//		lastlinmmio = linmmio;
		//		mmio_mode = MMIO_MODE_VRAM1;
		//	}
		//}else{
			if(mmio_mode != MMIO_MODE_VRAM2 || np2clvga.VRAMWindowAddr!=mmio_mode_region1  || np2clvga.pciLFB_Addr!=mmio_mode_region2 || lastlinmmio!=linmmio){
				if(np2clvga.VRAMWindowAddr){
					if(mmio_mode_region1) i386hax_vm_removememoryarea(vramptr, mmio_mode_region1, cirrusvga->real_vram_size - (mmio_mode==MMIO_MODE_VRAM1 ? 0x1000 : 0));
					i386hax_vm_setmemoryarea(vramptr, np2clvga.VRAMWindowAddr, cirrusvga->real_vram_size);
				}
				mmio_mode_region1 = np2clvga.VRAMWindowAddr;
				if(np2clvga.pciLFB_Addr){
					if(mmio_mode_region2) i386hax_vm_removememoryarea(vramptr, mmio_mode_region2, linmmio ? s->linear_mmio_mask : cirrusvga->real_vram_size);
					i386hax_vm_setmemoryarea(vramptr, np2clvga.pciLFB_Addr, linmmio ? s->linear_mmio_mask : cirrusvga->real_vram_size);
				}
				mmio_mode_region2 = np2clvga.pciLFB_Addr;
				lastlinmmio = linmmio;
				mmio_mode = MMIO_MODE_VRAM2;
			}
		//}
	}
#endif
}

uint32_t_ cirrus_linear_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
    uint32_t_ ret;

    addr &= s->cirrus_addr_mask;

    if (((s->sr[0x17] & 0x44) == 0x44) &&
        ((addr & s->linear_mmio_mask) == s->linear_mmio_mask)) {
		/* memory-mapped I/O */
		ret = cirrus_mmio_blt_read(s, addr & 0xff);
    } else if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
		/* XXX handle bitblt */
		//ret = 0xff;
		ret = *s->cirrus_srcptr;
		s->cirrus_srcptr++;
		if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
			cirrus_bitblt_videotocpu_next(s);
		}
    } else {
		/* video memory */
		if ((s->gr[0x0B] & 0x14) == 0x14) {
			addr <<= 4;
		} else if (s->gr[0x0B] & 0x02) {
			addr <<= 3;
		}
		addr &= s->cirrus_addr_mask;
		ret = *(s->vram_ptr + addr);
		//if(addr >= 0x1ff000){
		//	TRACEOUT(("vga: read 0x%x", addr));
		//}
		//if (0x1fff00 <= addr && addr < 0x1fff40){
		//	ret = s->cr[addr & 0xff];
		//}
		//if (addr == 0x1fff40){
		//	if(np2wab.relaystateint & 0x2){
		//		ret = 0x80;
		//	}else{
		//		ret = 0x00;
		//	}
		//}
     }

    return ret;
}

uint32_t_ cirrus_linear_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_linear_readb(opaque, addr) << 8;
    v |= cirrus_linear_readb(opaque, addr + 1);
#else
    v = cirrus_linear_readb(opaque, addr);
    v |= cirrus_linear_readb(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_linear_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_linear_readb(opaque, addr) << 24;
    v |= cirrus_linear_readb(opaque, addr + 1) << 16;
    v |= cirrus_linear_readb(opaque, addr + 2) << 8;
    v |= cirrus_linear_readb(opaque, addr + 3);
#else
    v = cirrus_linear_readb(opaque, addr);
    v |= cirrus_linear_readb(opaque, addr + 1) << 8;
    v |= cirrus_linear_readb(opaque, addr + 2) << 16;
    v |= cirrus_linear_readb(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_linear_writeb(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
    unsigned mode;

    addr &= s->cirrus_addr_mask;

    if (((s->sr[0x17] & 0x44) == 0x44) &&
        ((addr & s->linear_mmio_mask) ==  s->linear_mmio_mask)) {
		/* memory-mapped I/O */
		cirrus_mmio_blt_write(s, addr & 0xff, val);
    } else if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
		/* bitblt */
		*s->cirrus_srcptr++ = (uint8_t) val;
		if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
			cirrus_bitblt_cputovideo_next(s);
		}
    } else {
		/* video memory */
		if ((s->gr[0x0B] & 0x14) == 0x14) {
			addr <<= 4;
		} else if (s->gr[0x0B] & 0x02) {
			addr <<= 3;
		}
		addr &= s->cirrus_addr_mask;

		mode = s->gr[0x05] & 0x7;
		if (mode < 4 || mode > 5 || ((s->gr[0x0B] & 0x4) == 0)) {
			*(s->vram_ptr + addr) = (uint8_t) val;
			cpu_physical_memory_set_dirty(s->vram_offset + addr);
		} else {
			if ((s->gr[0x0B] & 0x14) != 0x14) {
				cirrus_mem_writeb_mode4and5_8bpp(s, mode, addr, val);
			} else {
				cirrus_mem_writeb_mode4and5_16bpp(s, mode, addr, val);
			}
		}
		//if (0x1fff00 <= addr && addr < 0x1fff40){
		//	cirrus_mmio_blt_write(s, addr & 0xff, val);
		//	//cirrus_hook_write_cr(s, addr & 0xff, val);
		//}
		//if (addr == 0x1fff40){
		//	char dat = 0x0;
		//	if(val == 0x80){
		//		dat = 0x2;
		//	}
		//	if((!!np2wab.relaystateint) != (!!(dat & 0x2))){
		//		np2wab.relaystateint = dat & 0x2;
		//		np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
		//	}
		//	np2clvga.mmioenable = (dat&0x1);
		//	TRACEOUT(("vga: write 0x%x=%02x", addr, val));
		//}
		//if(0x1ff000 <= addr && addr < 0x1fff00){
		//	TRACEOUT(("vga: write 0x%x=%02x", addr, val));
		//}
		//if(addr >= 0x1ff000){
		//	TRACEOUT(("vga: write 0x%x=%02x", addr, val));
		//}
    }
}

void cirrus_linear_writew(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_linear_writeb(opaque, addr, (val >> 8) & 0xff);
    cirrus_linear_writeb(opaque, addr + 1, val & 0xff);
#else
    cirrus_linear_writeb(opaque, addr, val & 0xff);
    cirrus_linear_writeb(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_linear_writel(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
	// XXX: Workaround for Win9x Driver
	CirrusVGAState *s = (CirrusVGAState *) opaque;
	if(s->device_id == CIRRUS_ID_CLGD5446){
		if (((s->sr[0x17] & 0x44) == 0x44) &&
			((addr & s->cirrus_addr_mask & s->linear_mmio_mask) == s->linear_mmio_mask)) {
			if((addr & 0xff) == 0x0c && val==0){
				return;
			}
		}
	}

#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_linear_writeb(opaque, addr, (val >> 24) & 0xff);
    cirrus_linear_writeb(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_linear_writeb(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_linear_writeb(opaque, addr + 3, val & 0xff);
#else
    cirrus_linear_writeb(opaque, addr, val & 0xff);
    cirrus_linear_writeb(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_linear_writeb(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_linear_writeb(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}


static CPUReadMemoryFunc *cirrus_linear_read[3] = {
    cirrus_linear_readb,
    cirrus_linear_readw,
    cirrus_linear_readl,
};

static CPUWriteMemoryFunc *cirrus_linear_write[3] = {
    cirrus_linear_writeb,
    cirrus_linear_writew,
    cirrus_linear_writel,
};

void cirrus_linear_mem_writeb(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= s->cirrus_addr_mask;
    *(s->vram_ptr + addr) = val;
    cpu_physical_memory_set_dirty(s->vram_offset + addr);
}

void cirrus_linear_mem_writew(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= s->cirrus_addr_mask;
    cpu_to_le16w((uint16_t_ *)(s->vram_ptr + addr), val);
    cpu_physical_memory_set_dirty(s->vram_offset + addr);
}

void cirrus_linear_mem_writel(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= s->cirrus_addr_mask;
    cpu_to_le32w((uint32_t_ *)(s->vram_ptr + addr), val);
    cpu_physical_memory_set_dirty(s->vram_offset + addr);
}

/***************************************
 *
 *  E-BANK memory access
 *
 ***************************************/

// convert E-BANK VRAM window address to linear address 
void cirrus_linear_memwnd_addr_convert(void *opaque, target_phys_addr_t *addrval){
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	int offset;
	target_phys_addr_t addr = *addrval;
	
	if(np2clvga.gd54xxtype <= 0xff){
		//addr &= s->cirrus_addr_mask;
		addr -= np2clvga.VRAMWindowAddr2;
		//addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x4000){
				offset = s->gr[0x09];
			}else{
				addr -= 0x4000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank */
			offset = s->gr[0x09];
		}
		if ((s->gr[0x0b] & 0x20) != 0)
			offset <<= 14;
		else
			offset <<= 12;

		addr += (offset);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x4000){
				offset = s->gr[0x09];
			}else{
				addr -= 0x4000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank */
			offset = s->gr[0x09];
		}
		if ((s->gr[0x0b] & 0x20) != 0)
			offset <<= 14;
		else
			offset <<= 12;

		addr += (offset);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x4000){
				offset = s->gr[0x09];
			}else{
				addr -= 0x4000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank */
			offset = s->gr[0x09];
		}
		if ((s->gr[0x0b] & 0x20) != 0)
			offset <<= 14;
		else
			offset <<= 12;

		addr += (offset);
	}else{
		addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x4000){
				offset = s->gr[0x09];
			}else{
				addr -= 0x4000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank */
			offset = s->gr[0x09];
		}
		//if ((s->gr[0x0b] & 0x01) != 0)	/* dual bank */
		//	offset = s->gr[0x09/* + bank_index*/];
		//else			/* single bank */
		//	offset = s->gr[0x09];

		if ((s->gr[0x0b] & 0x20) != 0)
			addr += (offset) << 14L;
		else
			addr += (offset) << 12L;
	}
	addr &= s->cirrus_addr_mask;
	*addrval = addr;
}
// I-O DATA用
int cirrus_linear_memwnd_addr_convert_iodata(void *opaque, target_phys_addr_t *addrval){
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	int offset;
	target_phys_addr_t addr = *addrval;
	int ret = 0;
	
	// MMIO判定
	// 本当は...
	// SR17[2]=1でMMIOイネーブル If this bit is set to '1', the BlT source will be system memory rather than display memory.
	// 本当はそれに加えGR6[3-2]=01じゃないとMMIOモードにならない
	// MMIOアドレスはSR17[6]=0:0xb8000に割り当て、1:リニアメモリの最後256byteに割り当て(CL-GD5430/'36/'40 only)
	if ((s->sr[0x17] & CIRRUS_MMIO_ENABLE) != 0 && /*(s->gr[0x06] & 0x0c)==0x04 &&*/ ((addr & 0xff000) == 0xb8000)) {
		ret = 1;	// MMIO
	}

	addr &= 0x7fff;
	if ((s->gr[0x0b] & 0x01) != 0){
		/* dual bank */
		if(addr < 0x4000){
			offset = s->gr[0x09];
		}else{
			addr -= 0x4000;
			offset = s->gr[0x0a];
		}
	}else{
		/* single bank */
		offset = s->gr[0x09];
		if(addr >= 0x4000){
			ret = 1; // XXX: 光栄製ゲーム用??? 
		}
	}
	if ((s->gr[0x0b] & 0x20) != 0)
		offset <<= 14;
	else
		offset <<= 12;
	
	addr += (offset);
	addr &= s->cirrus_addr_mask;

	*addrval = addr;

	return ret;
}

void cirrus_linear_memwnd_writeb(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd_addr_convert(opaque, &addr);
		g_cirrus_linear_write[0](opaque, addr, val);
		//cirrus_linear_mem_writeb(opaque, addr, val);
		//cirrus_linear_writeb(opaque, addr, val);
	}else{
		int r = cirrus_linear_memwnd_addr_convert_iodata(opaque, &addr);
		if(r==0){
			g_cirrus_linear_write[0](opaque, addr, val);
		}else{
			cirrus_mmio_writeb_iodata(opaque, addr, val);
		}
	}
}

void cirrus_linear_memwnd_writew(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd_addr_convert(opaque, &addr);
		g_cirrus_linear_write[1](opaque, addr, val);
		//cirrus_linear_mem_writew(opaque, addr, val);
		//cirrus_linear_writew(opaque, addr, val);
	}else{
		int r = cirrus_linear_memwnd_addr_convert_iodata(opaque, &addr);
		if(r==0){
			g_cirrus_linear_write[1](opaque, addr, val);
		}else{
			cirrus_mmio_writew_iodata(opaque, addr, val);
		}
	}
}

void cirrus_linear_memwnd_writel(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd_addr_convert(opaque, &addr);
		g_cirrus_linear_write[2](opaque, addr, val);
		//cirrus_linear_mem_writel(opaque, addr, val);
		//cirrus_linear_writel(opaque, addr, val);
	}else{
		int r = cirrus_linear_memwnd_addr_convert_iodata(opaque, &addr);
		if(r==0){
			g_cirrus_linear_write[2](opaque, addr, val);
		}else{
			cirrus_mmio_writel_iodata(opaque, addr, val);
		}
	}
}

uint32_t_ cirrus_linear_memwnd_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd_addr_convert(opaque, &addr);
		return cirrus_linear_readb(opaque, addr);
	}else{
		int r = cirrus_linear_memwnd_addr_convert_iodata(opaque, &addr);
		//if (r == 0 && (s->gr[0x31] & CIRRUS_BLT_RESET)!=0 || (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSDEST)!=0) { // XXX: 明らかに正しくないけどとりあえず動くように調整
		if ((cirrusvga_wab_40e1 & 0x02) == 0 ) { 
			return 0xff;	//DRAM REFRESH Mode
		}else if(r == 0) {
			return cirrus_linear_readb(opaque, addr);
		}else{
			return cirrus_mmio_readb_iodata(opaque, addr);
			//return cirrus_linear_readb(opaque, addr);
		}
	}
}

uint32_t_ cirrus_linear_memwnd_readw(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd_addr_convert(opaque, &addr);
		return cirrus_linear_readw(opaque, addr);
	}else{
		int r = cirrus_linear_memwnd_addr_convert_iodata(opaque, &addr);
		//if (r == 0 && (s->gr[0x31] & CIRRUS_BLT_RESET)!=0 || (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSDEST)!=0) { // XXX: 明らかに正しくないけどとりあえず動くように調整
		if ((cirrusvga_wab_40e1 & 0x02) == 0 ) { 
			return 0xff;	//DRAM REFRESH Mode
		}else if (r == 0) {
			return cirrus_linear_readw(opaque, addr);
		}else{
			return cirrus_mmio_readw_iodata(opaque, addr);
			//return cirrus_linear_readw(opaque, addr);
		}
	}
}

uint32_t_ cirrus_linear_memwnd_readl(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd_addr_convert(opaque, &addr);
		return cirrus_linear_readl(opaque, addr);
	}else{
		int r = cirrus_linear_memwnd_addr_convert_iodata(opaque, &addr);
		//if (r == 0 && (s->gr[0x31] & CIRRUS_BLT_RESET)!=0 || (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSDEST)!=0) { // XXX: 明らかに正しくないけどとりあえず動くように調整
		if ((cirrusvga_wab_40e1 & 0x02) == 0 ) { 
			return 0xff;	//DRAM REFRESH Mode
		}else if (r == 0) {
			return cirrus_linear_readl(opaque, addr);
		}else{
			return cirrus_mmio_readl_iodata(opaque, addr);
			//return cirrus_linear_readl(opaque, addr);
		}
	}
}

/***************************************
 *
 *  XXX: F00000 memory access ?
 *
 ***************************************/

// XXX: convert F00000 VRAM window address to linear address（どうしたら良いか分からん）
void cirrus_linear_memwnd3_addr_convert(void *opaque, target_phys_addr_t *addrval){
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	int offset;
	target_phys_addr_t addr = *addrval;
	
	addr -= np2clvga.VRAMWindowAddr3;
	addr &= 0xffff;
	if ((s->gr[0x0b] & 0x01) != 0){
		/* dual bank */
		if(addr < 0x8000){
			offset = s->gr[0x09];
		}else{
			addr -= 0x8000;
			offset = s->gr[0x0a];
		}
	}else{
		/* single bank */
		offset = s->gr[0x09];
	}
	if ((s->gr[0x0b] & 0x20) != 0)
		offset <<= 14;
	else
		offset <<= 12;

	addr += (offset);

	addr &= s->cirrus_addr_mask;
	*addrval = addr;
}
// I-O DATA用
int cirrus_linear_memwnd3_addr_convert_iodata(void *opaque, target_phys_addr_t *addrval){
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	int offset;
	target_phys_addr_t addr = *addrval;
	int ret = 0;

	// MMIO判定
	// 本当は...
	// SR17[2]=1でMMIOイネーブル If this bit is set to '1', the BlT source will be system memory rather than display memory.
	// 本当はそれに加えGR6[3-2]=01じゃないとMMIOモードにならない
	// MMIOアドレスはSR17[6]=0:0xb8000に割り当て、1:リニアメモリの最後256byteに割り当て(CL-GD5430/'36/'40 only)
	if ((s->sr[0x17] & CIRRUS_MMIO_ENABLE) != 0 /*&& (s->gr[0x06] & 0x0c)==0x04*/ && ((addr & 0xff000) == 0xb8000)) {
		ret = 1;	// MMIO
	}

	addr -= np2clvga.VRAMWindowAddr3;

	if ((s->gr[0x0b] & 0x01) != 0){
		/* dual bank */
		if(addr < 0x8000){
			offset = s->gr[0x09];
		}else{
			addr -= 0x8000;
			offset = s->gr[0x0a];
		}
	}else{
		/* single bank */
		offset = s->gr[0x09];
	//	if(addr >= 0x8000){
	//		ret = 1;
	//	}
	}
	if ((s->gr[0x0b] & 0x20) != 0)
		offset <<= 14;
	else
		offset <<= 12;
	
	addr += (offset);
	addr &= s->cirrus_addr_mask;

	*addrval = addr;

	return ret;
}

void cirrus_linear_memwnd3_writeb(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd3_addr_convert(opaque, &addr);

		//cirrus_linear_bitblt_writeb(opaque, addr, val);
		g_cirrus_linear_write[0](opaque, addr, val);
		//cirrus_linear_mem_writeb(opaque, addr, val);
		//cirrus_linear_writeb(opaque, addr, val);
	}else{
		int r = cirrus_linear_memwnd3_addr_convert_iodata(opaque, &addr);
		if(r==0){
			g_cirrus_linear_write[0](opaque, addr, val);
		}else{
			cirrus_mmio_writeb_iodata(opaque, addr, val);
		}
	}
}

void cirrus_linear_memwnd3_writew(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd3_addr_convert(opaque, &addr);
	
		//cirrus_linear_bitblt_writew(opaque, addr, val);
		g_cirrus_linear_write[1](opaque, addr, val);
		//cirrus_linear_mem_writew(opaque, addr, val);
		//cirrus_linear_writew(opaque, addr, val);
	}else{
		int r = cirrus_linear_memwnd3_addr_convert_iodata(opaque, &addr);
		if(r==0){
			g_cirrus_linear_write[1](opaque, addr, val);
		}else{
			cirrus_mmio_writew_iodata(opaque, addr, val);
		}
	}
}

void cirrus_linear_memwnd3_writel(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd3_addr_convert(opaque, &addr);
	
		//cirrus_linear_bitblt_writel(opaque, addr, val);
		g_cirrus_linear_write[2](opaque, addr, val);
		//cirrus_linear_mem_writel(opaque, addr, val);
		//cirrus_linear_writel(opaque, addr, val);
	}else{
		int r = cirrus_linear_memwnd3_addr_convert_iodata(opaque, &addr);
		if(r==0){
			g_cirrus_linear_write[2](opaque, addr, val);
		}else{
			cirrus_mmio_writel_iodata(opaque, addr, val);
		}
	}
}

uint32_t_ cirrus_linear_memwnd3_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd3_addr_convert(opaque, &addr);

		return cirrus_linear_readb(opaque, addr);
	}else{
		int r = cirrus_linear_memwnd3_addr_convert_iodata(opaque, &addr);
	//	if (r == 0 && (s->gr[0x31] & CIRRUS_BLT_RESET)!=0 || (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSDEST)!=0) { // XXX: 明らかに正しくないけどとりあえず動くように調整
		if ((cirrusvga_wab_40e1 & 0x02) == 0 ) { 
			return 0xff;	//DRAM REFRESH Mode
		}else if(r == 0) {
			return cirrus_linear_readb(opaque, addr);
		}else{
			return cirrus_mmio_readb_iodata(opaque, addr);
		}
	}
}

uint32_t_ cirrus_linear_memwnd3_readw(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd3_addr_convert(opaque, &addr);

		return cirrus_linear_readw(opaque, addr);
	}else{
		int r = cirrus_linear_memwnd3_addr_convert_iodata(opaque, &addr);
	//	if (r == 0 && (s->gr[0x31] & CIRRUS_BLT_RESET)!=0 || (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSDEST)!=0) { // XXX: 明らかに正しくないけどとりあえず動くように調整
		if ((cirrusvga_wab_40e1 & 0x02) == 0) {
			return 0xffff;	//DRAM REFRESH Mode
		}else if (r == 0) {
			return cirrus_linear_readw(opaque, addr);
		}else{
			return cirrus_mmio_readw_iodata(opaque, addr);
		}
	}
}

uint32_t_ cirrus_linear_memwnd3_readl(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) != CIRRUS_98ID_GA98NBIC){
		cirrus_linear_memwnd3_addr_convert(opaque, &addr);

		return cirrus_linear_readl(opaque, addr);
	}else{
		int r = cirrus_linear_memwnd3_addr_convert_iodata(opaque, &addr);
	//	if (r == 0 && (s->gr[0x31] & CIRRUS_BLT_RESET)!=0 || (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSDEST)!=0) { // XXX: 明らかに正しくないけどとりあえず動くように調整
		if ((cirrusvga_wab_40e1 & 0x02) == 0) {
			return 0xffffffff;	//DRAM REFRESH Mode
		}else if (r == 0) {
			return cirrus_linear_readl(opaque, addr);
		}else{
			return cirrus_mmio_readl_iodata(opaque, addr);
		}
	}
}


/***************************************
 *
 *  system to screen memory access
 *
 ***************************************/


uint32_t_ cirrus_linear_bitblt_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
    uint32_t_ ret = 0;

    /* handle bitblt */
    if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
		ret = *s->cirrus_srcptr;
		s->cirrus_srcptr++;
		if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
			cirrus_bitblt_videotocpu_next(s);
		}
	}
    return ret;
}

uint32_t_ cirrus_linear_bitblt_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_linear_bitblt_readb(opaque, addr) << 8;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 1);
#else
    v = cirrus_linear_bitblt_readb(opaque, addr);
    v |= cirrus_linear_bitblt_readb(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_linear_bitblt_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_linear_bitblt_readb(opaque, addr) << 24;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 1) << 16;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 2) << 8;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 3);
#else
    v = cirrus_linear_bitblt_readb(opaque, addr);
    v |= cirrus_linear_bitblt_readb(opaque, addr + 1) << 8;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 2) << 16;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_linear_bitblt_writeb(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
		/* bitblt */
		*s->cirrus_srcptr++ = (uint8_t) val;
		if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
			cirrus_bitblt_cputovideo_next(s);
		}
    }
}

void cirrus_linear_bitblt_writew(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_linear_bitblt_writeb(opaque, addr, (val >> 8) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 1, val & 0xff);
#else
    cirrus_linear_bitblt_writeb(opaque, addr, val & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_linear_bitblt_writel(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_linear_bitblt_writeb(opaque, addr, (val >> 24) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 3, val & 0xff);
#else
    cirrus_linear_bitblt_writeb(opaque, addr, val & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}


static CPUReadMemoryFunc *cirrus_linear_bitblt_read[3] = {
    cirrus_linear_bitblt_readb,
    cirrus_linear_bitblt_readw,
    cirrus_linear_bitblt_readl,
};

static CPUWriteMemoryFunc *cirrus_linear_bitblt_write[3] = {
    cirrus_linear_bitblt_writeb,
    cirrus_linear_bitblt_writew,
    cirrus_linear_bitblt_writel,
};

static void map_linear_vram(CirrusVGAState *s)
{
	g_cirrus_linear_map_enabled = 1;

    vga_dirty_log_stop((VGAState *)s);

    if (!s->map_addr && s->lfb_addr && s->lfb_end) {
        s->map_addr = s->lfb_addr;
        s->map_end = s->lfb_end;
        cpu_register_physical_memory(s->map_addr, s->map_end - s->map_addr, s->vram_offset);
    }

    if (!s->map_addr)
        return;

    s->lfb_vram_mapped = 0;
	
	//　このcpu_register_physical_memoryの詳細がさっぱり分からない･･･
    cpu_register_physical_memory(isa_mem_base + 0xF80000, 0x8000,
                                (s->vram_offset + s->cirrus_bank_base[0]) | IO_MEM_UNASSIGNED);
    cpu_register_physical_memory(isa_mem_base + 0xF88000, 0x8000,
                                (s->vram_offset + s->cirrus_bank_base[1]) | IO_MEM_UNASSIGNED);
    if (!(s->cirrus_srcptr != s->cirrus_srcptr_end)
        && !((s->sr[0x07] & 0x01) == 0)
        && !((s->gr[0x0B] & 0x14) == 0x14)
        && !(s->gr[0x0B] & 0x02)) {

        vga_dirty_log_stop((VGAState *)s);
		//　ここも
        cpu_register_physical_memory(isa_mem_base + 0xF80000, 0x8000,
                                    (s->vram_offset + s->cirrus_bank_base[0]) | IO_MEM_RAM);
        cpu_register_physical_memory(isa_mem_base + 0xF88000, 0x8000,
                                    (s->vram_offset + s->cirrus_bank_base[1]) | IO_MEM_RAM);

        s->lfb_vram_mapped = 1;
    }
    else {
        cpu_register_physical_memory(isa_mem_base + 0xF80000, 0x20000,
                                     s->vga_io_memory);
    }

    vga_dirty_log_start((VGAState *)s);
}

static void unmap_linear_vram(CirrusVGAState *s)
{
	g_cirrus_linear_map_enabled = 0;

    vga_dirty_log_stop((VGAState *)s);

    if (s->map_addr && s->lfb_addr && s->lfb_end)
        s->map_addr = s->map_end = 0;
	
	//　ここも
    cpu_register_physical_memory(isa_mem_base + 0xF80000, 0x20000,
                                 s->vga_io_memory);

    vga_dirty_log_start((VGAState *)s);
}

/* Compute the memory access functions */
static void cirrus_update_memory_access(CirrusVGAState *s)
{
    unsigned mode;

	// メモリ割り付けアドレス更新

	// inear address
	// sr7[7-4]<>0:sr7[7-4]をaddress23-20に割り当て、ただしgrb[5]=1の時sr7[4]はd.c.
	// sr7[7-4] =0:gr6[3-2]=00,01:A0000に割り当て 10,11:B0000に割り当て
	// WSNは42e1hがかかわっているかも?????

	if (np2clvga.gd54xxtype > 0xff && (cirrusvga_wab_42e1 & 0x18) == 0x18){
		np2clvga.VRAMWindowAddr3 = 0xf00000;
	}
	else if (
#if defined(SUPPORT_VGA_MODEX)
		!np2clvga.modex && 
#endif
		(s->sr[0x07] & CIRRUS_SR7_ISAADDR_MASK) != 0) {
		np2clvga.VRAMWindowAddr3 = (s->sr[0x07] & CIRRUS_SR7_ISAADDR_MASK & ~((s->gr[0x0b] >> 1) & 0x10)) << 16;
	}
	else {
#if defined(SUPPORT_VGA_MODEX)
		if(np2clvga.modex){
			if (s->gr[0x06] & 0x08) {
				np2clvga.VRAMWindowAddr3 = 0xb0000;
			}
			else {
				np2clvga.VRAMWindowAddr3 = 0xa0000;
			}
		}else
#endif
		{
			// アクセス不可にしておく
			np2clvga.VRAMWindowAddr3 = 0;
		}
	}

    if ((s->sr[0x17] & 0x44) == 0x44) {
        goto generic_io;
    } else if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
        goto generic_io;
    } else {
	if ((s->gr[0x0B] & 0x14) == 0x14) {
            goto generic_io;
	} else if (s->gr[0x0B] & 0x02) {
            goto generic_io;
        }

	mode = s->gr[0x05] & 0x7;
	if (mode < 4 || mode > 5 || ((s->gr[0x0B] & 0x4) == 0)) {
            map_linear_vram(s);
            g_cirrus_linear_write[0] = cirrus_linear_mem_writeb;
            g_cirrus_linear_write[1] = cirrus_linear_mem_writew;
            g_cirrus_linear_write[2] = cirrus_linear_mem_writel;
        } else {
        generic_io:
            unmap_linear_vram(s);
            g_cirrus_linear_write[0] = cirrus_linear_writeb;
            g_cirrus_linear_write[1] = cirrus_linear_writew;
            g_cirrus_linear_write[2] = cirrus_linear_writel;
        }
    }
	cirrus_linear_mmio_update(s);
}


/* I/O ports */
// PC-98用I/Oポート -> CL-GD54xxネイティブポート変換
uint32_t_ vga_convert_ioport(uint32_t_ addr){
#if defined(SUPPORT_PCI)
	// PCI版はポート番号そのまま
	if(pcidev.enable && 
	   (np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WS_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_W4_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WA_PCI ||
	   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G1_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G2_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G4_PCI)){
		if((addr & 0xFF0) == 0x3C0 || (addr & 0xFF0) == 0x3B0 || (addr & 0xFF0) == 0x3D0){
			np2clvga.gd54xxtype = CIRRUS_98ID_PCI;
			cirrusvga->sr[0x1F] = 0x2d;		// MemClock
			cirrusvga->gr[0x18] = 0x0f;             // fastest memory configuration
			cirrusvga->sr[0x0f] = CIRRUS_MEMSIZE_2M;
			cirrusvga->sr[0x17] = 0x20;
			cirrusvga->sr[0x15] = 0x03; /* memory size, 3=2MB, 4=4MB */
			cirrusvga->device_id = CIRRUS_ID_CLGD5446;
			cirrusvga->cr[0x27] = cirrusvga->device_id;
			cirrusvga->bustype = CIRRUS_BUSTYPE_PCI;
			cirrus_update_memory_access(cirrusvga);
			pc98_cirrus_vga_setvramsize();
			pc98_cirrus_vga_initVRAMWindowAddr();
		}
	}
#endif
	if(np2clvga.gd54xxtype <= 0xff){
		// 内蔵・純正ボード用
		if((addr & 0xFF0) == 0xCA0 || (addr & 0xFF0) == 0xC50){
			addr = 0x3C0 | (addr & 0xf);
		}else{
			if(addr==0xBA4 || addr==0xB54) addr = 0x3B4;
			if(addr==0xBA5 || addr==0xB55) addr = 0x3B5;
			if(addr==0xDA4 || addr==0xD54) addr = 0x3D4;
			if(addr==0xDA5 || addr==0xD55) addr = 0x3D5;
			if(addr==0xBAA || addr==0xB5A) addr = 0x3BA;
			if(addr==0xDAA || addr==0xD5A) addr = 0x3DA;
		}
	}else{
		// WAB用
		if((addr & 0xF0FF) == (0x40E0 | cirrusvga_melcowab_ofs)){
			addr = 0x3C0 | ((addr >> 8) & 0xf);
		}else{
			//if(addr==0x51E1+cirrusvga_melcowab_ofs) addr = 0x3B4; // ???
			//if(addr==0x57E1+cirrusvga_melcowab_ofs) addr = 0x3B5; // ???
			//if(addr==0x54E0+cirrusvga_melcowab_ofs) addr = 0x3D4;
			//if(addr==0x55E0+cirrusvga_melcowab_ofs) addr = 0x3D5;
			//if(addr==0x5BE1+cirrusvga_melcowab_ofs) addr = 0x3BA; // ???
			//if(addr==0x5AE0+cirrusvga_melcowab_ofs) addr = 0x3DA;
			if (addr == 0x58E0 + cirrusvga_melcowab_ofs) addr = 0x3B4;
 			if (addr == 0x59E0 + cirrusvga_melcowab_ofs) addr = 0x3B5;
 			////if (addr == 0x3AE0 + cirrusvga_melcowab_ofs) addr = 0x3BA;
 			if (addr == 0x54E0 + cirrusvga_melcowab_ofs) addr = 0x3D4;
 			if (addr == 0x55E0 + cirrusvga_melcowab_ofs) addr = 0x3D5;
 			if (addr == 0x5AE0 + cirrusvga_melcowab_ofs) addr = 0x3DA;
		}
	}
	return addr;
}

static uint32_t_ vga_ioport_read(void *opaque, uint32_t_ addr)
{
    CirrusVGAState *s = (CirrusVGAState *)opaque;
    int val, index;
	
	//　ポート決め打ちなので無理矢理変換
	addr = vga_convert_ioport(addr);
	
	//TRACEOUT(("CIRRUS VGA: read %04X", addr));

    /* check port range access depending on color/monochrome mode */
    if ((addr >= 0x3b0 && addr <= 0x3bf && (s->msr & MSR_COLOR_EMULATION))
	|| (addr >= 0x3d0 && addr <= 0x3df
	    && !(s->msr & MSR_COLOR_EMULATION))) {
		val = 0xff;
    } else {
		switch (addr) {
		case 0x3c0:
			if (s->ar_flip_flop == 0) {
			val = s->ar_index;
			} else {
			val = 0;
			}
			break;
		case 0x3c1:
			index = s->ar_index & 0x1f;
			if (index < 21)
			val = s->ar[index];
			else
			val = 0;
			break;
		case 0x3c2:
			val = s->st00;
			break;
		case 0x3c4:
			val = s->sr_index;
			break;
		case 0x3c5:
			if (cirrus_hook_read_sr(s, s->sr_index, &val))
				break;
			val = s->sr[s->sr_index];
#ifdef DEBUG_VGA_REG
			printf("vga: read SR%x = 0x%02x\n", s->sr_index, val);
#endif
			break;
		case 0x3c6:
			cirrus_read_hidden_dac(s, &val);
			break;
		case 0x3c7:
			val = s->dac_state;
			break;
		case 0x3c8:
			val = s->dac_write_index;
			s->cirrus_hidden_dac_lockindex = 0;
			break;
		case 0x3c9:
			if (cirrus_hook_read_palette(s, &val))
				break;
			val = s->palette[s->dac_read_index * 3 + s->dac_sub_index];
			if (++s->dac_sub_index == 3) {
				s->dac_sub_index = 0;
				s->dac_read_index++;
			}
			break;
		case 0x3ca:
			val = s->fcr;
			break;
		case 0x3cc:
			val = s->msr;
			break;
		case 0x3ce:
			val = s->gr_index;
			break;
		case 0x3cf:
			if (cirrus_hook_read_gr(s, s->gr_index, &val))
				break;
			val = s->gr[s->gr_index];
#ifdef DEBUG_VGA_REG
			printf("vga: read GR%x = 0x%02x\n", s->gr_index, val);
#endif
			break;
		case 0x3b4:
		case 0x3d4:
			val = s->cr_index;
			break;
		case 0x3b5:
		case 0x3d5:
			if (cirrus_hook_read_cr(s, s->cr_index, &val))
			break;
			val = s->cr[s->cr_index];
#ifdef DEBUG_VGA_REG
			printf("vga: read CR%x = 0x%02x\n", s->cr_index, val);
#endif
			break;
		case 0x3ba:
		case 0x3da:
			/* just toggle to fool polling */
			val = s->st01 = s->retrace((VGAState *) s);
			//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
			//	val = 0xC2;
			//}
			//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
			//	val = 0xc7;
			//}
			s->ar_flip_flop = 0;
			break;
		default:
			val = 0x00;
			break;
		}
    }
#if defined(TRACE)
    //TRACEOUT(("VGA: read addr=0x%04x data=0x%02x\n", addr, val));
#endif
    return val;
}
static REG8 IOOUTCALL vga_ioport_read_wrap(UINT addr)
{
	return vga_ioport_read(cirrusvga, addr);
}
UINT IOOUTCALL cirrusvga_ioport_read_wrap16(UINT addr)
{
	UINT16 ret;
	addr = vga_convert_ioport(addr);
	ret  = ((REG16)vga_ioport_read(cirrusvga, addr  )     );
	ret |= ((REG16)vga_ioport_read(cirrusvga, addr+1) << 8);
	return ret;
}
UINT IOOUTCALL cirrusvga_ioport_read_wrap32(UINT addr)
{
	UINT32 ret;
	addr = vga_convert_ioport(addr);
	ret  = ((UINT32)vga_ioport_read(cirrusvga, addr  )      );
	ret |= ((UINT32)vga_ioport_read(cirrusvga, addr+1) <<  8);
	ret |= ((UINT32)vga_ioport_read(cirrusvga, addr+2) << 16);
	ret |= ((UINT32)vga_ioport_read(cirrusvga, addr+3) << 24);
	return ret;
}

static void vga_ioport_write(void *opaque, uint32_t_ addr, uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *)opaque;
    int index;
	
	//　ポート決め打ちなので無理矢理変換
	addr = vga_convert_ioport(addr);
	
	//TRACEOUT(("CIRRUS VGA: write %04X %02X", addr, val));

    /* check port range access depending on color/monochrome mode */
    if ((addr >= 0x3b0 && addr <= 0x3bf && (s->msr & MSR_COLOR_EMULATION))
		|| (addr >= 0x3d0 && addr <= 0x3df
			&& !(s->msr & MSR_COLOR_EMULATION)))
		return;

#ifdef TRACE
    //TRACEOUT(("VGA: write addr=0x%04x data=0x%02x\n", addr, val));
#endif

    switch (addr) {
    case 0x3c0:
		if (s->ar_flip_flop == 0) {
			val &= 0x3f;
			s->ar_index = val;
		} else {
			index = s->ar_index & 0x1f;
			switch (index) {
			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0b:
			case 0x0c:
			case 0x0d:
			case 0x0e:
			case 0x0f:
				s->ar[index] = val & 0x3f;
				break;
			case 0x10:
				s->ar[index] = val & ~0x10;
				break;
			case 0x11:
				s->ar[index] = val;
				break;
			case 0x12:
				s->ar[index] = val & ~0xc0;
				break;
			case 0x13:
				s->ar[index] = val & ~0xf0;
				break;
			case 0x14:
				s->ar[index] = val & ~0xf0;
				break;
			default:
				break;
			}
		}
		s->ar_flip_flop ^= 1;
		break;
    case 0x3c2:
		s->msr = val & ~0x10;
		s->update_retrace_info((VGAState *) s);
		break;
    case 0x3c4:
		s->sr_index = val;
		break;
    case 0x3c5:
		if (cirrus_hook_write_sr(s, s->sr_index, val))
			break;
#ifdef DEBUG_VGA_REG
		printf("vga: write SR%x = 0x%02x\n", s->sr_index, val);
#endif
		s->sr[s->sr_index] = val & sr_mask[s->sr_index];
		if (s->sr_index == 1) s->update_retrace_info((VGAState *) s);
			break;
    case 0x3c6:
		cirrus_write_hidden_dac(s, val);
		break;
    case 0x3c7:
		s->dac_read_index = val;
		s->dac_sub_index = 0;
		s->dac_state = 3;
		break;
    case 0x3c8:
		s->dac_write_index = val;
		s->dac_sub_index = 0;
		s->dac_state = 0;
		break;
    case 0x3c9:
		if (cirrus_hook_write_palette(s, val))
			break;
		s->dac_cache[s->dac_sub_index] = val;
		if (++s->dac_sub_index == 3) {
			memcpy(&s->palette[s->dac_write_index * 3], s->dac_cache, 3);
			s->dac_sub_index = 0;
			s->dac_write_index++;
			np2wab.paletteChanged = 1; // パレット変えました
		}
		break;
	//case 0x3cc:
	//	s->msr = val;
	//	break;
    case 0x3ce:
		s->gr_index = val;
		break;
    case 0x3cf:
		if (cirrus_hook_write_gr(s, s->gr_index, val))
			break;
#ifdef DEBUG_VGA_REG
		printf("vga: write GR%x = 0x%02x\n", s->gr_index, val);
#endif
		s->gr[s->gr_index] = val & gr_mask[s->gr_index];
		break;
    case 0x3b4:
    case 0x3d4:
		s->cr_index = val;
		break;
    case 0x3b5:
    case 0x3d5:
		if (cirrus_hook_write_cr(s, s->cr_index, val))
			break;
#ifdef DEBUG_VGA_REG
		printf("vga: write CR%x = 0x%02x\n", s->cr_index, val);
#endif
		/* handle CR0-7 protection */
		if ((s->cr[0x11] & 0x80) && s->cr_index <= 7) {
			/* can always write bit 4 of CR7 */
			if (s->cr_index == 7)
			s->cr[7] = (s->cr[7] & ~0x10) | (val & 0x10);
			return;
		}
		switch (s->cr_index) {
		case 0x01:		/* horizontal display end */
		case 0x07:
		case 0x09:
		case 0x0c:
		case 0x0d:
		case 0x12:		/* vertical display end */
			s->cr[s->cr_index] = val;
			break;

		default:
			s->cr[s->cr_index] = val;
			break;
		}

		switch(s->cr_index) {
		case 0x00:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x11:
		case 0x17:
			s->update_retrace_info((VGAState *) s);
			break;
		}
		//if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		//	int width, height;
		//	cirrus_get_resolution((VGAState *) s, &width, &height);
		//	switch(s->cr_index) {
		//	case 0x04:
		//		if(width==640) 
		//			np2wab.shiftX = val - 0x54;
		//		else if(width==800) 
		//			np2wab.shiftX = val - 0x69;
		//		else if(width==1024) 
		//			np2wab.shiftX = val - 0x85;
		//		else 
		//			np2wab.shiftX = 0;
		//		break;
		//	case 0x10:
		//		if(width==640) 
		//			np2wab.shiftX = val - 0xEA;
		//		else if(width==800) 
		//			np2wab.shiftX = val - 0x5D;
		//		else if(width==1024) 
		//			np2wab.shiftX = val - 0x68;
		//		else 
		//			np2wab.shiftX = 0;
		//		break;
		//	}
		//}
		break;
    case 0x3ba:
    case 0x3da:
		s->fcr = val & 0x10;
		break;
    }
}
static void IOOUTCALL vga_ioport_write_wrap(UINT addr, REG8 dat)
{
	vga_ioport_write(cirrusvga, addr, dat);
}
void IOOUTCALL cirrusvga_ioport_write_wrap16(UINT addr, UINT dat)
{
	addr = vga_convert_ioport(addr);
	vga_ioport_write(cirrusvga, addr  , ((UINT32)dat     ) & 0xff);
	vga_ioport_write(cirrusvga, addr+1, ((UINT32)dat >> 8) & 0xff);
}
void IOOUTCALL cirrusvga_ioport_write_wrap32(UINT addr, UINT dat)
{
	addr = vga_convert_ioport(addr);
	vga_ioport_write(cirrusvga, addr  , (dat      ) & 0xff);
	vga_ioport_write(cirrusvga, addr+1, (dat >>  8) & 0xff);
	vga_ioport_write(cirrusvga, addr+2, (dat >> 16) & 0xff);
	vga_ioport_write(cirrusvga, addr+3, (dat >> 24) & 0xff);
}

/***************************************
 *
 *  memory-mapped I/O access
 *
 ***************************************/

uint32_t_ cirrus_mmio_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= CIRRUS_PNPMMIO_SIZE - 1;

    if (addr >= 0x100) {
        return cirrus_mmio_blt_read(s, addr - 0x100);
    } else {
        return vga_ioport_read(s, addr + 0x3c0);
    }
}

uint32_t_ cirrus_mmio_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_mmio_readb(opaque, addr) << 8;
    v |= cirrus_mmio_readb(opaque, addr + 1);
#else
    v = cirrus_mmio_readb(opaque, addr);
    v |= cirrus_mmio_readb(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_mmio_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_mmio_readb(opaque, addr) << 24;
    v |= cirrus_mmio_readb(opaque, addr + 1) << 16;
    v |= cirrus_mmio_readb(opaque, addr + 2) << 8;
    v |= cirrus_mmio_readb(opaque, addr + 3);
#else
    v = cirrus_mmio_readb(opaque, addr);
    v |= cirrus_mmio_readb(opaque, addr + 1) << 8;
    v |= cirrus_mmio_readb(opaque, addr + 2) << 16;
    v |= cirrus_mmio_readb(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_mmio_writeb(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= CIRRUS_PNPMMIO_SIZE - 1;

    if (addr >= 0x100) {
		cirrus_mmio_blt_write(s, addr - 0x100, val);
    } else {
        vga_ioport_write(s, addr + 0x3c0, val);
    }
}

void cirrus_mmio_writew(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_mmio_writeb(opaque, addr, (val >> 8) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 1, val & 0xff);
#else
    cirrus_mmio_writeb(opaque, addr, val & 0xff);
    cirrus_mmio_writeb(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_mmio_writel(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_mmio_writeb(opaque, addr, (val >> 24) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 3, val & 0xff);
#else
    cirrus_mmio_writeb(opaque, addr, val & 0xff);
    cirrus_mmio_writeb(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}

uint32_t_ cirrus_mmio_readb_wab(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	addr &= ~np2clvga.pciMMIO_Mask;
    if (addr >= 0x8000) {
        return cirrus_mmio_blt_read(s, (addr - 0x8000) & 0x7fff);
    } else {
        return vga_ioport_read(s, addr & 0x7fff);
    }
}

uint32_t_ cirrus_mmio_readw_wab(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_mmio_readb_wab(opaque, addr) << 8;
    v |= cirrus_mmio_readb_wab(opaque, addr + 1);
#else
    v = cirrus_mmio_readb_wab(opaque, addr);
    v |= cirrus_mmio_readb_wab(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_mmio_readl_wab(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_mmio_readb_wab(opaque, addr) << 24;
    v |= cirrus_mmio_readb_wab(opaque, addr + 1) << 16;
    v |= cirrus_mmio_readb_wab(opaque, addr + 2) << 8;
    v |= cirrus_mmio_readb_wab(opaque, addr + 3);
#else
    v = cirrus_mmio_readb_wab(opaque, addr);
    v |= cirrus_mmio_readb_wab(opaque, addr + 1) << 8;
    v |= cirrus_mmio_readb_wab(opaque, addr + 2) << 16;
    v |= cirrus_mmio_readb_wab(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_mmio_writeb_wab(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

	addr &= ~np2clvga.pciMMIO_Mask;
    if (addr >= 0x8000) {
		//addr = (addr - 0x8000) & 0x7fff;
		cirrus_mmio_blt_write(s, (addr - 0x8000) & 0x7fff, val);
    } else {
		//addr = addr & 0x7fff;
		//if(addr > CIRRUS_PNPMMIO_SIZE) return;
        vga_ioport_write(s, addr & 0x7fff, val);
    }
}

void cirrus_mmio_writew_wab(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_mmio_writeb_wab(opaque, addr, (val >> 8) & 0xff);
    cirrus_mmio_writeb_wab(opaque, addr + 1, val & 0xff);
#else
    cirrus_mmio_writeb_wab(opaque, addr, val & 0xff);
    cirrus_mmio_writeb_wab(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_mmio_writel_wab(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_mmio_writeb_wab(opaque, addr, (val >> 24) & 0xff);
    cirrus_mmio_writeb_wab(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_mmio_writeb_wab(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_mmio_writeb_wab(opaque, addr + 3, val & 0xff);
#else
    cirrus_mmio_writeb_wab(opaque, addr, val & 0xff);
    cirrus_mmio_writeb_wab(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_mmio_writeb_wab(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_mmio_writeb_wab(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}

uint32_t_ cirrus_mmio_readb_iodata(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

	addr &= ~np2clvga.pciMMIO_Mask;
	addr = (addr - 0x8000) & 0x7fff;
	return cirrus_mmio_blt_read(s, (addr/* - 0x8000*/) & (CIRRUS_PNPMMIO_SIZE-1));
}

uint32_t_ cirrus_mmio_readw_iodata(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_mmio_readb_iodata(opaque, addr) << 8;
    v |= cirrus_mmio_readb_iodata(opaque, addr + 1);
#else
    v = cirrus_mmio_readb_iodata(opaque, addr);
    v |= cirrus_mmio_readb_iodata(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_mmio_readl_iodata(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_mmio_readb_iodata(opaque, addr) << 24;
    v |= cirrus_mmio_readb_iodata(opaque, addr + 1) << 16;
    v |= cirrus_mmio_readb_iodata(opaque, addr + 2) << 8;
    v |= cirrus_mmio_readb_iodata(opaque, addr + 3);
#else
    v = cirrus_mmio_readb_iodata(opaque, addr);
    v |= cirrus_mmio_readb_iodata(opaque, addr + 1) << 8;
    v |= cirrus_mmio_readb_iodata(opaque, addr + 2) << 16;
    v |= cirrus_mmio_readb_iodata(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_mmio_writeb_iodata(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

	addr &= ~np2clvga.pciMMIO_Mask;
    if ((s->gr[0x31] & CIRRUS_BLT_BUSY)==0) {
		addr = (addr - 0x8000) & 0x7fff;
		cirrus_mmio_blt_write(s, (addr/* - 0x8000*/) & (CIRRUS_PNPMMIO_SIZE-1), val);
	}
	/*}*/
  //  } else {
		////addr = addr & 0x7fff;
		////if(addr > CIRRUS_PNPMMIO_SIZE) return;
  //      //vga_ioport_write(s, addr & 0x7fff, val);
  //  }
}

void cirrus_mmio_writew_iodata(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_mmio_writeb_iodata(opaque, addr, (val >> 8) & 0xff);
    cirrus_mmio_writeb_iodata(opaque, addr + 1, val & 0xff);
#else
    cirrus_mmio_writeb_iodata(opaque, addr, val & 0xff);
    cirrus_mmio_writeb_iodata(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_mmio_writel_iodata(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_mmio_writeb_iodata(opaque, addr, (val >> 24) & 0xff);
    cirrus_mmio_writeb_iodata(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_mmio_writeb_iodata(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_mmio_writeb_iodata(opaque, addr + 3, val & 0xff);
#else
    cirrus_mmio_writeb_iodata(opaque, addr, val & 0xff);
    cirrus_mmio_writeb_iodata(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_mmio_writeb_iodata(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_mmio_writeb_iodata(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}


CPUReadMemoryFunc *cirrus_mmio_read[3] = {
    cirrus_mmio_readb,
    cirrus_mmio_readw,
    cirrus_mmio_readl,
};

CPUWriteMemoryFunc *cirrus_mmio_write[3] = {
    cirrus_mmio_writeb,
    cirrus_mmio_writew,
    cirrus_mmio_writel,
};

#define array_write(ary, pos, data, len) \
	memcpy(ary+pos, data, len); \
	pos += len;  

#define array_read(ary, pos, data, len) \
	memcpy(data, ary+pos, len); \
	pos += len;  


/* load/save state */
void pc98_cirrus_vga_save()
{
    CirrusVGAState *s = cirrusvga;
    int pos = 0;
	UINT8 *f = cirrusvga_statsavebuf; 
	//char test[500] = {0};
	uint32_t_ state_ver = 6;
	uint32_t_ intbuf;
	char en[3] = "en";
	
    array_write(f, pos, &state_ver, sizeof(state_ver)); // ステートセーブ バージョン番号
	
	if(vramptr == NULL || s == NULL) {
		strcpy(en, "di");
		array_write(f, pos, en, 2);
		return;
	}
	array_write(f, pos, en, 2);

	// この際全部保存
	array_write(f, pos, vramptr, CIRRUS_VRAM_SIZE);
	array_write(f, pos, &s->vram_offset, sizeof(s->vram_offset));
	array_write(f, pos, &s->vram_size, sizeof(s->vram_size));
	array_write(f, pos, &s->lfb_addr, sizeof(s->lfb_addr));
	array_write(f, pos, &s->lfb_end, sizeof(s->lfb_end));
	array_write(f, pos, &s->map_addr, sizeof(s->map_addr));
	array_write(f, pos, &s->map_end, sizeof(s->map_end));
	array_write(f, pos, &s->lfb_vram_mapped, sizeof(s->lfb_vram_mapped));
	array_write(f, pos, &s->bios_offset, sizeof(s->bios_offset));
	array_write(f, pos, &s->bios_size, sizeof(s->bios_size));
	array_write(f, pos, &s->it_shift, sizeof(s->it_shift));
	
	array_write(f, pos, &s->latch, sizeof(s->latch));
	array_write(f, pos, &s->sr_index, sizeof(s->sr_index));
	array_write(f, pos, s->sr, sizeof(s->sr));
	array_write(f, pos, &s->gr_index, sizeof(s->gr_index));
	array_write(f, pos, s->gr, sizeof(s->gr));
	array_write(f, pos, &s->ar_index, sizeof(s->ar_index));
	array_write(f, pos, s->ar, sizeof(s->ar));
	array_write(f, pos, &s->ar_flip_flop, sizeof(s->ar_flip_flop));
	array_write(f, pos, &s->cr_index, sizeof(s->cr_index));
	array_write(f, pos, s->cr, sizeof(s->cr));
	array_write(f, pos, &s->msr, sizeof(s->msr));
	array_write(f, pos, &s->fcr, sizeof(s->fcr));
	array_write(f, pos, &s->st00, sizeof(s->st00));
	array_write(f, pos, &s->st01, sizeof(s->st01));
	array_write(f, pos, &s->dac_state, sizeof(s->dac_state));
	array_write(f, pos, &s->dac_sub_index, sizeof(s->dac_sub_index));
	array_write(f, pos, &s->dac_read_index, sizeof(s->dac_read_index));
	array_write(f, pos, &s->dac_write_index, sizeof(s->dac_write_index));
	array_write(f, pos, s->dac_cache, sizeof(s->dac_cache));
	array_write(f, pos, &s->dac_8bit, sizeof(s->dac_8bit));
	array_write(f, pos, s->palette, sizeof(s->palette));
	array_write(f, pos, &s->bank_offset, sizeof(s->bank_offset));
	array_write(f, pos, &s->vga_io_memory, sizeof(s->vga_io_memory));
	array_write(f, pos, &s->vbe_index, sizeof(s->vbe_index));
	array_write(f, pos, s->vbe_regs, sizeof(s->vbe_regs));
	array_write(f, pos, &s->vbe_start_addr, sizeof(s->vbe_start_addr));
	array_write(f, pos, &s->vbe_line_offset, sizeof(s->vbe_line_offset));
	array_write(f, pos, &s->vbe_bank_mask, sizeof(s->vbe_bank_mask));
	
	array_write(f, pos, s->font_offsets, sizeof(s->font_offsets));
	array_write(f, pos, &s->graphic_mode, sizeof(s->graphic_mode));
	array_write(f, pos, &s->shift_control, sizeof(s->shift_control));
	array_write(f, pos, &s->double_scan, sizeof(s->double_scan));
	array_write(f, pos, &s->line_offset, sizeof(s->line_offset));
	array_write(f, pos, &s->line_compare, sizeof(s->line_compare));
	array_write(f, pos, &s->start_addr, sizeof(s->start_addr));
	array_write(f, pos, &s->plane_updated, sizeof(s->plane_updated));
	array_write(f, pos, &s->last_line_offset, sizeof(s->last_line_offset));
	array_write(f, pos, &s->last_cw, sizeof(s->last_cw));
	array_write(f, pos, &s->last_ch, sizeof(s->last_ch));
	array_write(f, pos, &s->last_width, sizeof(s->last_width));
	array_write(f, pos, &s->last_height, sizeof(s->last_height));
	array_write(f, pos, &s->last_scr_width, sizeof(s->last_scr_width));
	array_write(f, pos, &s->last_scr_height, sizeof(s->last_scr_height));
	array_write(f, pos, &s->last_depth, sizeof(s->last_depth));
	array_write(f, pos, &s->cursor_start, sizeof(s->cursor_start));
	array_write(f, pos, &s->cursor_end, sizeof(s->cursor_end));
	array_write(f, pos, &s->cursor_offset, sizeof(s->cursor_offset));
	
	array_write(f, pos, s->invalidated_y_table, sizeof(s->invalidated_y_table));
	array_write(f, pos, s->last_palette, sizeof(s->last_palette));
	array_write(f, pos, s->last_ch_attr, sizeof(s->last_ch_attr));
	
	array_write(f, pos, &s->cirrus_linear_io_addr, sizeof(s->cirrus_linear_io_addr));
	array_write(f, pos, &s->cirrus_linear_bitblt_io_addr, sizeof(s->cirrus_linear_bitblt_io_addr));
	array_write(f, pos, &s->cirrus_mmio_io_addr, sizeof(s->cirrus_mmio_io_addr));
	array_write(f, pos, &s->cirrus_addr_mask, sizeof(s->cirrus_addr_mask));
	array_write(f, pos, &s->linear_mmio_mask, sizeof(s->linear_mmio_mask));
	array_write(f, pos, &s->cirrus_shadow_gr0, sizeof(s->cirrus_shadow_gr0));
	array_write(f, pos, &s->cirrus_shadow_gr1, sizeof(s->cirrus_shadow_gr1));
	array_write(f, pos, &s->cirrus_hidden_dac_lockindex, sizeof(s->cirrus_hidden_dac_lockindex));
	array_write(f, pos, &s->cirrus_hidden_dac_data, sizeof(s->cirrus_hidden_dac_data));
	array_write(f, pos, s->cirrus_bank_base, sizeof(s->cirrus_bank_base));
	array_write(f, pos, s->cirrus_bank_limit, sizeof(s->cirrus_bank_limit));
	array_write(f, pos, s->cirrus_hidden_palette, sizeof(s->cirrus_hidden_palette));
	array_write(f, pos, &s->hw_cursor_x, sizeof(s->hw_cursor_x));
	array_write(f, pos, &s->hw_cursor_y, sizeof(s->hw_cursor_y));
	array_write(f, pos, &s->cirrus_blt_pixelwidth, sizeof(s->cirrus_blt_pixelwidth));
	array_write(f, pos, &s->cirrus_blt_width, sizeof(s->cirrus_blt_width));
	array_write(f, pos, &s->cirrus_blt_height, sizeof(s->cirrus_blt_height));
	array_write(f, pos, &s->cirrus_blt_dstpitch, sizeof(s->cirrus_blt_dstpitch));
	array_write(f, pos, &s->cirrus_blt_srcpitch, sizeof(s->cirrus_blt_srcpitch));
	array_write(f, pos, &s->cirrus_blt_fgcol, sizeof(s->cirrus_blt_fgcol));
	array_write(f, pos, &s->cirrus_blt_bgcol, sizeof(s->cirrus_blt_bgcol));
	array_write(f, pos, &s->cirrus_blt_dstaddr, sizeof(s->cirrus_blt_dstaddr));
	array_write(f, pos, &s->cirrus_blt_srcaddr, sizeof(s->cirrus_blt_srcaddr));
	array_write(f, pos, &s->cirrus_blt_mode, sizeof(s->cirrus_blt_mode));
	array_write(f, pos, &s->cirrus_blt_modeext, sizeof(s->cirrus_blt_modeext));
	array_write(f, pos, s->cirrus_bltbuf, sizeof(s->cirrus_bltbuf));
	intbuf = (UINT32)(s->cirrus_srcptr - s->cirrus_bltbuf);
	array_write(f, pos, &intbuf, sizeof(intbuf));
	intbuf = (UINT32)(s->cirrus_srcptr_end - s->cirrus_bltbuf);
	array_write(f, pos, &intbuf, sizeof(intbuf));
	array_write(f, pos, &s->cirrus_srccounter, sizeof(s->cirrus_srccounter));
	array_write(f, pos, &s->last_hw_cursor_size, sizeof(s->last_hw_cursor_size));
	array_write(f, pos, &s->last_hw_cursor_x, sizeof(s->last_hw_cursor_x));
	array_write(f, pos, &s->last_hw_cursor_y, sizeof(s->last_hw_cursor_y));
	array_write(f, pos, &s->last_hw_cursor_y_start, sizeof(s->last_hw_cursor_y_start));
	array_write(f, pos, &s->last_hw_cursor_y_end, sizeof(s->last_hw_cursor_y_end));
	array_write(f, pos, &s->real_vram_size, sizeof(s->real_vram_size));
	array_write(f, pos, &s->device_id, sizeof(s->device_id));
	array_write(f, pos, &s->bustype, sizeof(s->bustype));
	
	array_write(f, pos, &np2clvga.VRAMWindowAddr3, sizeof(np2clvga.VRAMWindowAddr3));
	
	array_write(f, pos, &s->videowindow_dblbuf_index, sizeof(s->videowindow_dblbuf_index));
	array_write(f, pos, &s->graphics_dblbuf_index, sizeof(s->graphics_dblbuf_index));
	
	array_write(f, pos, &cirrusvga_wab_59e1, sizeof(cirrusvga_wab_59e1))
	array_write(f, pos, &cirrusvga_wab_51e1, sizeof(cirrusvga_wab_51e1));
	array_write(f, pos, &cirrusvga_wab_5be1, sizeof(cirrusvga_wab_5be1));
	array_write(f, pos, &cirrusvga_wab_40e1, sizeof(cirrusvga_wab_40e1));
	array_write(f, pos, &cirrusvga_wab_46e8, sizeof(cirrusvga_wab_46e8));
	
	array_write(f, pos, &cirrusvga_melcowab_ofs, sizeof(cirrusvga_melcowab_ofs));
	
	array_write(f, pos, &cirrusvga_wab_42e1, sizeof(cirrusvga_wab_42e1));
	
	TRACEOUT(("CIRRUS VGA datalen=%d, (max %d bytes)", pos, sizeof(cirrusvga_statsavebuf)));
#if defined(_WIN32)
	// テスト用
	if(pos > sizeof(cirrusvga_statsavebuf)){
		MessageBox(NULL, _T("State save: Buffer Full"), _T("Warning"), 0);
	}
#endif
}

void pc98_cirrus_vga_load()
{
    CirrusVGAState *s = cirrusvga;
	UINT8 *f = cirrusvga_statsavebuf; 
    int pos = 0;
	uint32_t_ state_ver = 0;
	uint32_t_ intbuf;
	//int width, height;
	char en[3];
	
	array_read(f, pos, &state_ver, sizeof(state_ver)); // バージョン番号
	switch(state_ver){
	case 0:
		s->latch = state_ver; // ver0.86 rev22以前はlatchは常にゼロ（のはず）
		array_read(f, pos, &s->sr_index, sizeof(s->sr_index));
		array_read(f, pos, s->sr, 256);
		array_read(f, pos, &s->gr_index, sizeof(s->gr_index));
		array_read(f, pos, &s->cirrus_shadow_gr0, sizeof(s->cirrus_shadow_gr0));
		array_read(f, pos, &s->cirrus_shadow_gr1, sizeof(s->cirrus_shadow_gr1));
		s->gr[0x00] = s->cirrus_shadow_gr0 & 0x0f;
		s->gr[0x01] = s->cirrus_shadow_gr1 & 0x0f;
		array_read(f, pos, s->gr + 2, 254);
		array_read(f, pos, &s->ar_index, sizeof(s->ar_index));
		array_read(f, pos, s->ar, 21);
		array_read(f, pos, &s->ar_flip_flop, sizeof(s->ar_flip_flop));
		array_read(f, pos, &s->cr_index, sizeof(s->cr_index));
		array_read(f, pos, s->cr, 256);
		array_read(f, pos, &s->msr, sizeof(s->msr));
		array_read(f, pos, &s->fcr, sizeof(s->fcr));
		array_read(f, pos, &s->st00, sizeof(s->st00));
		array_read(f, pos, &s->st01, sizeof(s->st01));

		array_read(f, pos, &s->dac_state, sizeof(s->dac_state));
		array_read(f, pos, &s->dac_sub_index, sizeof(s->dac_sub_index));
		array_read(f, pos, &s->dac_read_index, sizeof(s->dac_read_index));
		array_read(f, pos, &s->dac_write_index, sizeof(s->dac_write_index));
		array_read(f, pos, s->dac_cache, 3);
		array_read(f, pos, s->palette, 768);

		array_read(f, pos, &s->bank_offset, sizeof(s->bank_offset));

		array_read(f, pos, &s->cirrus_hidden_dac_lockindex, sizeof(s->cirrus_hidden_dac_lockindex));
		array_read(f, pos, &s->cirrus_hidden_dac_data, sizeof(s->cirrus_hidden_dac_data));

		array_read(f, pos, &s->hw_cursor_x, sizeof(s->hw_cursor_x));
		array_read(f, pos, &s->hw_cursor_y, sizeof(s->hw_cursor_y));
	
		array_read(f, pos, vramptr, CIRRUS_VRAM_SIZE);

		break;
	case 3:
	case 4:
	case 5:
	case 6:
		array_read(f, pos, en, 2);
		if(en[0] != 'e' || en[0] != 'n')
			break;

	case 1:
	case 2:
		// この際全部保存
		array_read(f, pos, vramptr, CIRRUS_VRAM_SIZE);
		array_read(f, pos, &s->vram_offset, sizeof(s->vram_offset));
		array_read(f, pos, &s->vram_size, sizeof(s->vram_size));
		array_read(f, pos, &s->lfb_addr, sizeof(s->lfb_addr));
		array_read(f, pos, &s->lfb_end, sizeof(s->lfb_end));
		array_read(f, pos, &s->map_addr, sizeof(s->map_addr));
		array_read(f, pos, &s->map_end, sizeof(s->map_end));
		array_read(f, pos, &s->lfb_vram_mapped, sizeof(s->lfb_vram_mapped));
		array_read(f, pos, &s->bios_offset, sizeof(s->bios_offset));
		array_read(f, pos, &s->bios_size, sizeof(s->bios_size));
		array_read(f, pos, &s->it_shift, sizeof(s->it_shift));
	
		array_read(f, pos, &s->latch, sizeof(s->latch));
		array_read(f, pos, &s->sr_index, sizeof(s->sr_index));
		array_read(f, pos, s->sr, sizeof(s->sr));
		array_read(f, pos, &s->gr_index, sizeof(s->gr_index));
		array_read(f, pos, s->gr, sizeof(s->gr));
		array_read(f, pos, &s->ar_index, sizeof(s->ar_index));
		array_read(f, pos, s->ar, sizeof(s->ar));
		array_read(f, pos, &s->ar_flip_flop, sizeof(s->ar_flip_flop));
		array_read(f, pos, &s->cr_index, sizeof(s->cr_index));
		array_read(f, pos, s->cr, sizeof(s->cr));
		array_read(f, pos, &s->msr, sizeof(s->msr));
		array_read(f, pos, &s->fcr, sizeof(s->fcr));
		array_read(f, pos, &s->st00, sizeof(s->st00));
		array_read(f, pos, &s->st01, sizeof(s->st01));
		array_read(f, pos, &s->dac_state, sizeof(s->dac_state));
		array_read(f, pos, &s->dac_sub_index, sizeof(s->dac_sub_index));
		array_read(f, pos, &s->dac_read_index, sizeof(s->dac_read_index));
		array_read(f, pos, &s->dac_write_index, sizeof(s->dac_write_index));
		array_read(f, pos, s->dac_cache, sizeof(s->dac_cache));
		array_read(f, pos, &s->dac_8bit, sizeof(s->dac_8bit));
		array_read(f, pos, s->palette, sizeof(s->palette));
		array_read(f, pos, &s->bank_offset, sizeof(s->bank_offset));
		array_read(f, pos, &s->vga_io_memory, sizeof(s->vga_io_memory));
		array_read(f, pos, &s->vbe_index, sizeof(s->vbe_index));
		array_read(f, pos, s->vbe_regs, sizeof(s->vbe_regs));
		array_read(f, pos, &s->vbe_start_addr, sizeof(s->vbe_start_addr));
		array_read(f, pos, &s->vbe_line_offset, sizeof(s->vbe_line_offset));
		array_read(f, pos, &s->vbe_bank_mask, sizeof(s->vbe_bank_mask));
	
		array_read(f, pos, s->font_offsets, sizeof(s->font_offsets));
		array_read(f, pos, &s->graphic_mode, sizeof(s->graphic_mode));
		array_read(f, pos, &s->shift_control, sizeof(s->shift_control));
		array_read(f, pos, &s->double_scan, sizeof(s->double_scan));
		array_read(f, pos, &s->line_offset, sizeof(s->line_offset));
		array_read(f, pos, &s->line_compare, sizeof(s->line_compare));
		array_read(f, pos, &s->start_addr, sizeof(s->start_addr));
		array_read(f, pos, &s->plane_updated, sizeof(s->plane_updated));
		array_read(f, pos, &s->last_line_offset, sizeof(s->last_line_offset));
		array_read(f, pos, &s->last_cw, sizeof(s->last_cw));
		array_read(f, pos, &s->last_ch, sizeof(s->last_ch));
		array_read(f, pos, &s->last_width, sizeof(s->last_width));
		array_read(f, pos, &s->last_height, sizeof(s->last_height));
		array_read(f, pos, &s->last_scr_width, sizeof(s->last_scr_width));
		array_read(f, pos, &s->last_scr_height, sizeof(s->last_scr_height));
		array_read(f, pos, &s->last_depth, sizeof(s->last_depth));
		array_read(f, pos, &s->cursor_start, sizeof(s->cursor_start));
		array_read(f, pos, &s->cursor_end, sizeof(s->cursor_end));
		array_read(f, pos, &s->cursor_offset, sizeof(s->cursor_offset));
	
		array_read(f, pos, s->invalidated_y_table, sizeof(s->invalidated_y_table));
		array_read(f, pos, s->last_palette, sizeof(s->last_palette));
		array_read(f, pos, s->last_ch_attr, sizeof(s->last_ch_attr));
	
		array_read(f, pos, &s->cirrus_linear_io_addr, sizeof(s->cirrus_linear_io_addr));
		array_read(f, pos, &s->cirrus_linear_bitblt_io_addr, sizeof(s->cirrus_linear_bitblt_io_addr));
		array_read(f, pos, &s->cirrus_mmio_io_addr, sizeof(s->cirrus_mmio_io_addr));
		array_read(f, pos, &s->cirrus_addr_mask, sizeof(s->cirrus_addr_mask));
		array_read(f, pos, &s->linear_mmio_mask, sizeof(s->linear_mmio_mask));
		array_read(f, pos, &s->cirrus_shadow_gr0, sizeof(s->cirrus_shadow_gr0));
		array_read(f, pos, &s->cirrus_shadow_gr1, sizeof(s->cirrus_shadow_gr1));
		array_read(f, pos, &s->cirrus_hidden_dac_lockindex, sizeof(s->cirrus_hidden_dac_lockindex));
		array_read(f, pos, &s->cirrus_hidden_dac_data, sizeof(s->cirrus_hidden_dac_data));
		array_read(f, pos, s->cirrus_bank_base, sizeof(s->cirrus_bank_base));
		array_read(f, pos, s->cirrus_bank_limit, sizeof(s->cirrus_bank_limit));
		array_read(f, pos, s->cirrus_hidden_palette, sizeof(s->cirrus_hidden_palette));
		array_read(f, pos, &s->hw_cursor_x, sizeof(s->hw_cursor_x));
		array_read(f, pos, &s->hw_cursor_y, sizeof(s->hw_cursor_y));
		array_read(f, pos, &s->cirrus_blt_pixelwidth, sizeof(s->cirrus_blt_pixelwidth));
		array_read(f, pos, &s->cirrus_blt_width, sizeof(s->cirrus_blt_width));
		array_read(f, pos, &s->cirrus_blt_height, sizeof(s->cirrus_blt_height));
		array_read(f, pos, &s->cirrus_blt_dstpitch, sizeof(s->cirrus_blt_dstpitch));
		array_read(f, pos, &s->cirrus_blt_srcpitch, sizeof(s->cirrus_blt_srcpitch));
		array_read(f, pos, &s->cirrus_blt_fgcol, sizeof(s->cirrus_blt_fgcol));
		array_read(f, pos, &s->cirrus_blt_bgcol, sizeof(s->cirrus_blt_bgcol));
		array_read(f, pos, &s->cirrus_blt_dstaddr, sizeof(s->cirrus_blt_dstaddr));
		array_read(f, pos, &s->cirrus_blt_srcaddr, sizeof(s->cirrus_blt_srcaddr));
		array_read(f, pos, &s->cirrus_blt_mode, sizeof(s->cirrus_blt_mode));
		array_read(f, pos, &s->cirrus_blt_modeext, sizeof(s->cirrus_blt_modeext));
		array_read(f, pos, s->cirrus_bltbuf, sizeof(s->cirrus_bltbuf));
		array_read(f, pos, &intbuf, sizeof(intbuf));
		s->cirrus_srcptr = s->cirrus_bltbuf + intbuf;
		array_read(f, pos, &intbuf, sizeof(intbuf));
		s->cirrus_srcptr_end = s->cirrus_bltbuf + intbuf;
		array_read(f, pos, &s->cirrus_srccounter, sizeof(s->cirrus_srccounter));
		array_read(f, pos, &s->last_hw_cursor_size, sizeof(s->last_hw_cursor_size));
		array_read(f, pos, &s->last_hw_cursor_x, sizeof(s->last_hw_cursor_x));
		array_read(f, pos, &s->last_hw_cursor_y, sizeof(s->last_hw_cursor_y));
		array_read(f, pos, &s->last_hw_cursor_y_start, sizeof(s->last_hw_cursor_y_start));
		array_read(f, pos, &s->last_hw_cursor_y_end, sizeof(s->last_hw_cursor_y_end));
		array_read(f, pos, &s->real_vram_size, sizeof(s->real_vram_size));
		array_read(f, pos, &s->device_id, sizeof(s->device_id));
		array_read(f, pos, &s->bustype, sizeof(s->bustype));

		if(state_ver >= 2){
			array_read(f, pos, &np2clvga.VRAMWindowAddr3, sizeof(np2clvga.VRAMWindowAddr3));
		}
		if(state_ver >= 4){
			array_read(f, pos, &s->videowindow_dblbuf_index, sizeof(s->videowindow_dblbuf_index));
			array_read(f, pos, &s->graphics_dblbuf_index, sizeof(s->graphics_dblbuf_index));
		}
		if(state_ver >= 5){
			array_read(f, pos, &cirrusvga_wab_59e1, sizeof(cirrusvga_wab_59e1));
			array_read(f, pos, &cirrusvga_wab_51e1, sizeof(cirrusvga_wab_51e1));
			array_read(f, pos, &cirrusvga_wab_5be1, sizeof(cirrusvga_wab_5be1));
			array_read(f, pos, &cirrusvga_wab_40e1, sizeof(cirrusvga_wab_40e1));
			array_read(f, pos, &cirrusvga_wab_46e8, sizeof(cirrusvga_wab_46e8));
			array_read(f, pos, &cirrusvga_melcowab_ofs, sizeof(cirrusvga_melcowab_ofs));
		}
		if(state_ver >= 6){
			array_read(f, pos, &cirrusvga_wab_42e1, sizeof(cirrusvga_wab_42e1));
		}
		//

		break;
	default:
		break;
	}
	
	s->cirrus_rop = cirrus_bitblt_rop_nop; // XXX: 本当はステートセーブで保存しないと駄目
	
#ifdef SUPPORT_PCI
	// 関数アドレス入れ直し
	pcidev.devices[pcidev_cirrus_deviceid].regwfn = &pcidev_cirrus_cfgreg_w;
#endif
		
	pc98_cirrus_vga_updatePCIaddr();

    cirrus_update_memory_access(s);
    
    s->graphic_mode = -1;
    cirrus_update_bank_ptr(s, 0);
    cirrus_update_bank_ptr(s, 1);
	
	// WAB画面サイズ強制更新
	np2wab.realWidth = 0;
	np2wab.realHeight = 0;

	np2wab.paletteChanged = 1; // パレット変えました
}

/***************************************
 *
 *  initialize
 *
 ***************************************/
void cirrus_reset(void *opaque)
{
    CirrusVGAState *s = (CirrusVGAState*)opaque;

	memset(s->sr, 0, sizeof(s->sr));
	memset(s->cr, 0, sizeof(s->cr));
	memset(s->gr, 0, sizeof(s->gr));

    vga_reset(s);
    unmap_linear_vram(s);
    s->sr[0x06] = 0x0f;
    if (s->device_id == CIRRUS_ID_CLGD5446) {
        /* 4MB 64 bit memory config, always PCI */
        s->sr[0x1F] = 0x2d;		// MemClock
        s->gr[0x18] = 0x0f;             // fastest memory configuration
        s->sr[0x0f] = CIRRUS_MEMSIZE_2M; //0x98;
        s->sr[0x17] = 0x20;
        s->sr[0x15] = 0x03; /* memory size, 3=2MB, 4=4MB */
    } else {
        s->sr[0x1F] = 0x22;		// MemClock
        s->sr[0x0F] = CIRRUS_MEMSIZE_2M;
        s->sr[0x17] = s->bustype;
        s->sr[0x15] = 0x03; /* memory size, 3=2MB, 4=4MB */
    }
    s->cr[0x27] = s->device_id;
	if (np2clvga.gd54xxtype == CIRRUS_98ID_WAB) {
        s->sr[0x0F] = CIRRUS_MEMSIZE_1M;
        s->sr[0x15] = 0x02;
	}
	pc98_cirrus_setWABreg();
	
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	BitBlt(np2wabwnd.hDCBuf, 0, 0, WAB_MAX_WIDTH, WAB_MAX_HEIGHT, NULL, 0, 0, BLACKNESS);
#endif
	if ((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC || np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F) {
		memset(s->vram_ptr, 0x00, s->real_vram_size);
	}else{
		/* Win2K seems to assume that the pattern buffer is at 0xff
		   initially ! */
		memset(s->vram_ptr, 0xff, s->real_vram_size);
	}
	memset(s->palette, 0, sizeof(s->palette));
	memset(s->cirrus_hidden_palette, 0, sizeof(s->cirrus_hidden_palette));
	
    s->cirrus_hidden_dac_lockindex = 5;
    //s->cirrus_hidden_dac_data = 0;

	// XXX: for WinNT4.0
	s->cirrus_hidden_dac_data = 1;
	
	// XXX: Win2000のハードウェアアクセラレーションを動かすのに必要。理由は謎
	s->gr[0x25] = 0x06;
	s->gr[0x26] = 0x20;
	
	// XXX: Win2000で動かすのに必要。理由は謎
#if defined(SUPPORT_PCI)
	if(pcidev.enable && (np2clvga.gd54xxtype == CIRRUS_98ID_PCI || 
	   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WS_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_W4_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WA_PCI ||
	   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G1_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G2_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G4_PCI)){
		s->msr = 0x03;
		s->sr[0x08] = 0xFE;
		s->gr[0x0e] &= ~0x20; // XXX: for WinNT4.0
		s->gr[0x33] = 0x04; // XXX: for WinNT4.0
		s->cr[0x5e] &= ~0x20;//s->cr[0x5e] |= 0x20; // XXX: Part IDを読むまでGR33を書き込み禁止にする（WinNT4専用の不具合回避）
	}
#endif
	//fh = fopen("vgadump.bin", "w+");
	//if (fh != FILEH_INVALID) {
	//	fwrite(s, sizeof(CirrusVGAState), 1, fh);
	//	fclose(fh);
	//}
}

#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
LOGPALETTE * NewLogPal(const uint8_t *pCirrusPalette , int iSize) {
	LOGPALETTE *lpPalette;
	int count;

	lpPalette = (LOGPALETTE*)malloc(sizeof (LOGPALETTE) + iSize * sizeof (PALETTEENTRY));
	lpPalette->palVersion = 0x0300;
	lpPalette->palNumEntries = iSize;
	
	for (count = 0 ; count < iSize ; count++) {
		lpPalette->palPalEntry[count].peRed = c6_to_8(pCirrusPalette[count*3]);
		lpPalette->palPalEntry[count].peGreen = c6_to_8(pCirrusPalette[count*3+1]);
		lpPalette->palPalEntry[count].peBlue = c6_to_8(pCirrusPalette[count*3+2]);
		lpPalette->palPalEntry[count].peFlags = 0;
	}
	return lpPalette;
}
#endif

void ConvertYUV2RGB(int width, unsigned char *srcYUV16, unsigned char *dstRGB32Line){
	int j;
	int offset = 128;
	if(cirrusvga->cr[0x3f] & 0x10){
		//if(cirrusvga->cr[0x3f] & 0x08){
		//	// RGB555
		//	//memcpy(dstRGB32Line, srcYUV16, width);
		//}else{
			// YCC422
			for(j=0;j<width/2;j++){
				int u0 = srcYUV16[j * 4 + 0];
				int v0 = srcYUV16[j * 4 + 1];
				int y1 = srcYUV16[j * 4 + 2];
				int y0 = srcYUV16[j * 4 + 3];
				int r0 = (298 * (y0 - 16) + 409 * (v0 - offset) + 128) >> 8;
				int g0 = (298 * (y0 - 16) - 100 * (u0 - offset) - 208 * (v0 - offset) + 128) >> 8;
				int b0 = (298 * (y0 - 16) + 516 * (u0 - offset) + 128) >> 8;
				int r1 = (298 * (y1 - 16) + 409 * (v0 - offset) + 128) >> 8;
				int g1 = (298 * (y1 - 16) - 100 * (u0 - offset) - 208 * (v0 - offset) + 128) >> 8;
				int b1 = (298 * (y1 - 16) + 516 * (u0 - offset) + 128) >> 8;
				dstRGB32Line[j*8 + 0] = (b0 < 0 ? 0 : (b0 > 255 ? 255 : b0));
				dstRGB32Line[j*8 + 1] = (g0 < 0 ? 0 : (g0 > 255 ? 255 : g0));
				dstRGB32Line[j*8 + 2] = (r0 < 0 ? 0 : (r0 > 255 ? 255 : r0));
				dstRGB32Line[j*8 + 4] = (b1 < 0 ? 0 : (b1 > 255 ? 255 : b1));
				dstRGB32Line[j*8 + 5] = (g1 < 0 ? 0 : (g1 > 255 ? 255 : g1));
				dstRGB32Line[j*8 + 6] = (r1 < 0 ? 0 : (r1 > 255 ? 255 : r1));
			}
		//}
	}else{
		for(j=0;j<width/2;j++){
			int u0 = srcYUV16[j * 4 + 0];
			int y0 = srcYUV16[j * 4 + 1];
			int v0 = srcYUV16[j * 4 + 2];
			int y1 = srcYUV16[j * 4 + 3];
			int r0 = (298 * (y0 - 16) + 409 * (v0 - offset) + 128) >> 8;
			int g0 = (298 * (y0 - 16) - 100 * (u0 - offset) - 208 * (v0 - offset) + 128) >> 8;
			int b0 = (298 * (y0 - 16) + 516 * (u0 - offset) + 128) >> 8;
			int r1 = (298 * (y1 - 16) + 409 * (v0 - offset) + 128) >> 8;
			int g1 = (298 * (y1 - 16) - 100 * (u0 - offset) - 208 * (v0 - offset) + 128) >> 8;
			int b1 = (298 * (y1 - 16) + 516 * (u0 - offset) + 128) >> 8;
			dstRGB32Line[j*8 + 0] = (b0 < 0 ? 0 : (b0 > 255 ? 255 : b0));
			dstRGB32Line[j*8 + 1] = (g0 < 0 ? 0 : (g0 > 255 ? 255 : g0));
			dstRGB32Line[j*8 + 2] = (r0 < 0 ? 0 : (r0 > 255 ? 255 : r0));
			dstRGB32Line[j*8 + 4] = (b1 < 0 ? 0 : (b1 > 255 ? 255 : b1));
			dstRGB32Line[j*8 + 5] = (g1 < 0 ? 0 : (g1 > 255 ? 255 : g1));
			dstRGB32Line[j*8 + 6] = (r1 < 0 ? 0 : (r1 > 255 ? 255 : r1));
		}
	}
}

//　画面表示(仮)　本当はQEMUのオリジナルのコードを移植すべきなんだけど･･･
//  Cirrus VRAM (screen & cursor) -> GDI Device Independent Bitmap
void cirrusvga_drawGraphic(){
//#define DEBUG_CIRRUS_VRAM
#if defined(DEBUG_CIRRUS_VRAM)
	//static UINT32 kdown = 0;
	//static UINT32 kdownc = 0;
	static INT32 memshift = 0; // DEBUG 
	static INT32 sysmemmode = 0; // DEBUG 
	static INT32 tabon = 0; // DEBUG 
#endif
	int i, j, width, height, bpp;
	uint32_t_ line_offset = 0;
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	LOGPALETTE * lpPalette;
	static HPALETTE hPalette = NULL, oldPalette = NULL;
	HDC hdc = np2wabwnd.hDCBuf;
#endif
	static int waitscreenchange = 0;
	int r;
	int scanW = 0; // VRAM上の1ラインのデータ幅(byte)
	int scanpixW = 0; // 実際に転送すべき1ラインのピクセル数(pixel)
	int scanshift = 0;
	uint8_t *scanptr;
	uint8_t *vram_ptr;
	
#if defined(NP2_SDL) || defined(__LIBRETRO__)
	unsigned int *VRAMBuf = NULL;
	char* p;
#elif defined(NP2_X)
	GdkPixbuf *VRAMBuf = NULL;
	char* p;
#endif

	int cursot_ofs_x = 0;
	int cursot_ofs_y = 0;
	
	int realWidth = 0;
	int realHeight = 0;

	// VRAM上での1ラインのサイズ（表示幅と等しくない場合有り）
	line_offset = cirrusvga->cr[0x13] | ((cirrusvga->cr[0x1b] & 0x10) << 4);
	line_offset <<= 3;

	vram_ptr = cirrusvga->vram_ptr + np2wab.vramoffs;
	
	//if(cirrusvga->device_id == CIRRUS_ID_CLGD5446 || (np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC || (np2clvga.gd54xxtype & CIRRUS_98ID_WABMASK) == CIRRUS_98ID_WAB){
		//if((cirrusvga->cr[0x5e] & 0x7) == 0x1){
		//	cirrusvga->graphics_dblbuf_index = (cirrusvga->graphics_dblbuf_index + 1) & 0x1;
		//}
		//if(cirrusvga->graphics_dblbuf_index != 0 || (cirrusvga->cr[0x1a] & 0x2) || (np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC || (np2clvga.gd54xxtype & CIRRUS_98ID_WABMASK) == CIRRUS_98ID_WAB)
		{
			// Screen Start A
			int addroffset = 
				(((int)(cirrusvga->cr[0x1d] >> 7) & 0x01) << 19)|
				(((int)(cirrusvga->cr[0x1b] >> 2) & 0x03) << 17)|
				(((int)(cirrusvga->cr[0x1b] >> 0) & 0x01) << 16)|
				(((int)(cirrusvga->cr[0x0c] >> 0) & 0xff) << 8)|
				(((int)(cirrusvga->cr[0x0d] >> 0) & 0xff) << 0);
			vram_ptr += addroffset * 4; // ???
		}
	//}
	
#if defined(DEBUG_CIRRUS_VRAM)
	// DEBUG 
	//////	vram_ptr = mem + 640*16*memshift;
	//vram_ptr += 640*16*memshift;
	if(GetKeyState(VK_SHIFT)<0){
		memshift++;
	//	kdown = 1;
	//}else if(kdown){
	//	//vram_ptr = vram_ptr + 1024*768*1;
	//	kdown = 0;
	}
	if(GetKeyState(VK_CONTROL)<0){
		memshift--;
		if(memshift<0) memshift = 0;
		//vram_ptr = mem + 1024*768*memshift;
	//	kdownc = 1;
	//}else if(kdownc){
	//	//vram_ptr = vram_ptr + 1024*768*1;
	//	kdownc = 0;
	}
	if(GetKeyState(VK_TAB)<0){
		if(!tabon){
			sysmemmode = (sysmemmode + 1) % 3;
			tabon = 1;
		}
	}else{
		tabon = 0;
	}
	//if(GetKeyState(VK_CONTROL)<0){
	switch(sysmemmode){
	case 0:
		vram_ptr = vram_ptr + 256*16*memshift;
		break;
	case 1:
		vram_ptr = mem + 256*16*memshift;
		break;
	case 2:
		vram_ptr = CPU_EXTMEMBASE + 256*16*memshift;
		break;
	}
	//}
	// DEBUG (END)
#endif

	// Cirrusの色数と解像度を取得
    bpp = cirrusvga->get_bpp((VGAState*)cirrusvga);
    cirrusvga->get_resolution((VGAState*)cirrusvga, &width, &height);
	//bpp = 16;
	//width = 1024;
	//height = 768;
	
#if defined(SUPPORT_VGA_MODEX)
	// PC/AT MODE X compatible
	if (np2clvga.gd54xxtype <= 0xff){
		static UINT8 lastmodex = 0;
		if(np2clvga.modex){
			if(!lastmodex){
				cirrusvga->sr[0x07] &= ~0x01;
			}
			bpp = 8;
			width = 320;
			height = 240;
			line_offset = 320;
		}else{
			if(!lastmodex){
				cirrusvga->sr[0x07] |= 0x01;
			}
		}
		lastmodex = np2clvga.modex;
	}
#endif

	if(bpp==0) return; 

	// Palette mode > 85MHz (1280x1024)
	if((cirrusvga->cirrus_hidden_dac_data & 0xCF) == 0x4A){
		bpp = 8;
		width *= 2;
		height *= 2;
	}

	// GA-98NB用 1280x1024
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
		// XXX: Win3.1用 やっつけ修正
		if(width==640 && height==512 && bpp==15/* && 
		   cirrusvga->cr[0x00]==0x63 && cirrusvga->cr[0x04]==0x53 && cirrusvga->cr[0x05]==0x1e && 
		   cirrusvga->cr[0x06]==0x15 && cirrusvga->cr[0x10]==0x04 && cirrusvga->cr[0x11]==0x88*/){
			bpp = 8;
			width *= 2;
			height *= 2;
		}
	}
	
	// WAB解像度設定
	realWidth = width;
	realHeight = height;
	
	// CRTC offset 設定
	scanW = width*(bpp/8);
	scanpixW = width;
	if(bpp && line_offset){
		// 32bit color用やっつけ修正 for GA-98NB & WSN-A2F/A4F
		if(bpp==32){
			if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC || (np2clvga.gd54xxtype & CIRRUS_98ID_WABMASK) == CIRRUS_98ID_WAB){
				line_offset <<= 1;
			}
		}
		scanW = line_offset;
	}

	// 色数判定
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	if(bpp==16){
		// ビットフィールドでRGB565を指定
		uint32_t_* bitfleld = (uint32_t_*)(ga_bmpInfo->bmiColors);
		bitfleld[0] = 0x0000F800;
		bitfleld[1] = 0x000007E0;
		bitfleld[2] = 0x0000001F;
		ga_bmpInfo->bmiHeader.biCompression = BI_BITFIELDS;
	}else{
		ga_bmpInfo->bmiHeader.biCompression = BI_RGB;
	}
	// Windowsの16bitカラーは標準でRGB555なのでそのままbpp=16に変更
	if(bpp==15){
		bpp = 16;
	}
#endif
	
	// GA-98NB用
	if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
		if(np2cfg.ga98nb_bigscrn_ex && (bpp==8 || bpp==16) && (width==1024 || width==1280) && scanW*8/bpp==1600){
			// 1600x1024 Big Screen Extension
			int addroffset = 
				(((int)(cirrusvga->cr[0x1d] >> 7) & 0x01) << 19)|
				(((int)(cirrusvga->cr[0x1b] >> 2) & 0x03) << 17)|
				(((int)(cirrusvga->cr[0x1b] >> 0) & 0x01) << 16)|
				(((int)(cirrusvga->cr[0x0c] >> 0) & 0xff) << 8)|
				(((int)(cirrusvga->cr[0x0d] >> 0) & 0xff) << 0);
			addroffset *= 4;
			realWidth = 1600;
			realHeight = 1024;
			width = 1600;
			height = 1024;
			scanpixW = 1600;
			vram_ptr = cirrusvga->vram_ptr + np2wab.vramoffs;
			cursot_ofs_x = (addroffset*8/bpp) % 1600;
			cursot_ofs_y = (addroffset*8/bpp) / 1600;
		}
	}
	
	// WAB解像度更新
	np2wab.realWidth = realWidth;
	np2wab.realHeight = realHeight;
    
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	if(ga_bmpInfo->bmiHeader.biBitCount!=8 && bpp==8){
		np2wab.paletteChanged = 1;
	}

	ga_bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ga_bmpInfo->bmiHeader.biWidth = width; // 仮セット
	ga_bmpInfo->bmiHeader.biHeight = 1; // 仮セット
	ga_bmpInfo->bmiHeader.biPlanes = 1;
	ga_bmpInfo->bmiHeader.biBitCount = bpp;
#endif
	if(bpp<=8){
		if(np2wab.paletteChanged){
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
			WORD* PalIndexes = (WORD*)((char*)ga_bmpInfo + sizeof(BITMAPINFOHEADER));
			for (i = 0; i < 256; ++i) PalIndexes[i] = i;
			lpPalette = NewLogPal(cirrusvga->palette , 1<<bpp);
			if(hPalette){
				SelectPalette(hdc , oldPalette , FALSE);
				DeleteObject(hPalette);
			}
			hPalette = CreatePalette(lpPalette);
			free(lpPalette);
			oldPalette = SelectPalette(hdc , hPalette , FALSE);
			RealizePalette(hdc);
#endif
			np2wab.paletteChanged = 0;
		}
		// 256モードなら素直に転送して良し
		if(scanpixW*bpp/8==scanW){
			if(scanshift){
				// XXX: WABのスキャン位置シフト無視してるけど･･･
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
				ga_bmpInfo->bmiHeader.biWidth = scanpixW;
				ga_bmpInfo->bmiHeader.biHeight = -height;
				scanptr = vram_ptr;
				SetDIBitsToDevice(
					hdc , 0 , 0 ,
					ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
					0 , 0 , 0 , -ga_bmpInfo->bmiHeader.biHeight ,
					vram_ptr+scanshift*bpp/8 , ga_bmpInfo , DIB_PAL_COLORS
				);
				//int scanshiftY = scanshift/scanpixW;
				//ga_bmpInfo->bmiHeader.biWidth = scanpixW;
				//ga_bmpInfo->bmiHeader.biHeight = -height;
				//SetDIBitsToDevice(
				//	hdc , 0 , 0 ,
				//	ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
				//	0 , 0 , scanshiftY , -ga_bmpInfo->bmiHeader.biHeight ,
				//	vram_ptr , ga_bmpInfo , DIB_PAL_COLORS
				//);
				//SetDIBitsToDevice(
				//	hdc , 0 , 0 ,
				//	ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
				//	0 , 0 , 0 , scanshiftY ,
				//	vram_ptr , ga_bmpInfo , DIB_PAL_COLORS
				//);
#elif defined(NP2_SDL) || defined(__LIBRETRO__)
				scanptr = vram_ptr;
				VRAMBuf = (unsigned int*)malloc(width * height * sizeof(unsigned int));
				p = (unsigned char*)VRAMBuf;
				for(j = 0; j < height; j++) {
					for(i = 0; i < width; i++) {
						p[(j * width + i) * 4 + 2] = cirrusvga->palette[vram_ptr[j * width + i] * 3    ] << 2;
						p[(j * width + i) * 4 + 1] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 1] << 2;
						p[(j * width + i) * 4    ] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 2] << 2;
					}
				}
#elif defined(NP2_X)
				scanptr = vram_ptr;
				if(np2wabwnd.pPixbuf) {
					VRAMBuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
					p = gdk_pixbuf_get_pixels(VRAMBuf);
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 3    ] = cirrusvga->palette[vram_ptr[j * width + i] * 3    ] << 2;
							p[(j * width + i) * 3 + 1] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 1] << 2;
							p[(j * width + i) * 3 + 2] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 2] << 2;
						}
					}
				}
#endif
			}else{
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
				ga_bmpInfo->bmiHeader.biWidth = scanpixW;
				ga_bmpInfo->bmiHeader.biHeight = -height;
				scanptr = vram_ptr;
				SetDIBitsToDevice(
					hdc , 0 , 0 ,
					ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
					0 , 0 , 0 , -ga_bmpInfo->bmiHeader.biHeight ,
					vram_ptr , ga_bmpInfo , DIB_PAL_COLORS
				);
#elif defined(NP2_SDL) || defined(__LIBRETRO__)
				scanptr = vram_ptr;
				VRAMBuf = (unsigned int*)malloc(width * height * sizeof(unsigned int));
				p = (unsigned char*)VRAMBuf;
				for(j = 0; j < height; j++) {
					for(i = 0; i < width; i++) {
						p[(j * width + i) * 4 + 2] = cirrusvga->palette[vram_ptr[j * width + i] * 3    ] << 2;
						p[(j * width + i) * 4 + 1] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 1] << 2;
						p[(j * width + i) * 4    ] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 2] << 2;
					}
				}
#elif defined(NP2_X)
				scanptr = vram_ptr;
				if(np2wabwnd.pPixbuf) {
					VRAMBuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
					p = gdk_pixbuf_get_pixels(VRAMBuf);
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 3    ] = cirrusvga->palette[vram_ptr[j * width + i] * 3    ] << 2;
							p[(j * width + i) * 3 + 1] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 1] << 2;
							p[(j * width + i) * 3 + 2] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 2] << 2;
						}
					}
				}
#endif
			}
		}else{
			// ズレがあるなら1ラインずつ転送
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
			scanptr = vram_ptr;
			for(i=0;i<height;i++){
				ga_bmpInfo->bmiHeader.biWidth = width;
				ga_bmpInfo->bmiHeader.biHeight = 1;
				r = SetDIBitsToDevice(
					hdc , 0 , i ,
					width , 1 ,
					0 , 0 , 0 , 1 ,
					scanptr , ga_bmpInfo , DIB_PAL_COLORS
				);
				scanptr += scanW;
			}
#elif defined(NP2_SDL) || defined(__LIBRETRO__)
			scanptr = vram_ptr;
			VRAMBuf = (unsigned int*)malloc(width * height * sizeof(unsigned int));
			p = (unsigned char*)VRAMBuf;
			for(j = 0; j < height; j++) {
				for(i = 0; i < width; i++) {
					p[(j * width + i) * 4 + 2] = cirrusvga->palette[vram_ptr[j * width + i] * 3    ] << 2;
					p[(j * width + i) * 4 + 1] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 1] << 2;
					p[(j * width + i) * 4    ] = cirrusvga->palette[vram_ptr[j * width + i] * 3 + 2] << 2;
				}
			}
#elif defined(NP2_X)
			scanptr = vram_ptr;
			if(np2wabwnd.pPixbuf) {
				GdkPixbuf *VRAMBuf;
				VRAMBuf = gdk_pixbuf_new_from_data(vram_ptr, GDK_COLORSPACE_RGB, FALSE, 8, width, height, width*bpp/8, NULL,NULL);
			}
#endif
		}
	}else{
		if(scanpixW*bpp/8==scanW){
			if(scanshift){
				// XXX: スキャン位置シフト
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
				ga_bmpInfo->bmiHeader.biWidth = scanpixW;
				ga_bmpInfo->bmiHeader.biHeight = -height;
				scanptr = vram_ptr;
				SetDIBitsToDevice(
					hdc , 0 , 0 ,
					ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
					0 , 0 , 0 , -ga_bmpInfo->bmiHeader.biHeight ,
					vram_ptr+scanshift*bpp/8 , ga_bmpInfo , DIB_RGB_COLORS
				);
#elif defined(NP2_SDL) || defined(__LIBRETRO__)
				scanptr = vram_ptr;
				VRAMBuf = (unsigned int*)malloc(width * height * sizeof(unsigned int));
				p = (unsigned char*)VRAMBuf;
				for(j = 0; j < height; j++) {
					for(i = 0; i < width; i++) {
						p[(j * width + i) * 4    ] = vram_ptr[(j * width + i) * 3    ];
						p[(j * width + i) * 4 + 1] = vram_ptr[(j * width + i) * 3 + 1];
						p[(j * width + i) * 4 + 2] = vram_ptr[(j * width + i) * 3 + 2];
					}
				}
#elif defined(NP2_X)
				scanptr = vram_ptr;
				if(np2wabwnd.pPixbuf) {
					VRAMBuf = gdk_pixbuf_new_from_data(vram_ptr, GDK_COLORSPACE_RGB, FALSE, 8, width, height, width*bpp/8, NULL,NULL);
					p = gdk_pixbuf_get_pixels(VRAMBuf);
				}
#endif
			}else{
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
				ga_bmpInfo->bmiHeader.biWidth = scanpixW;
				ga_bmpInfo->bmiHeader.biHeight = -height;
				scanptr = vram_ptr;
				SetDIBitsToDevice(
					hdc , 0 , 0 ,
					ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
					0 , 0 , 0 , -ga_bmpInfo->bmiHeader.biHeight ,
					vram_ptr , ga_bmpInfo , DIB_RGB_COLORS
				);
#elif defined(NP2_SDL) || defined(__LIBRETRO__)
				scanptr = vram_ptr;
				VRAMBuf = (unsigned int*)malloc(width * height * sizeof(unsigned int));
				p = (unsigned char*)VRAMBuf;
				if(bpp == 24) {
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 4    ] = vram_ptr[(j * width + i) * 3    ];
							p[(j * width + i) * 4 + 1] = vram_ptr[(j * width + i) * 3 + 1];
							p[(j * width + i) * 4 + 2] = vram_ptr[(j * width + i) * 3 + 2];
						}
					}
				} else if(bpp == 16) {
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 4 + 2] = (((((UINT16*)vram_ptr)[j * width + i]) & 0xF800) >> 8) & 0xFF;
							p[(j * width + i) * 4 + 1] = (((((UINT16*)vram_ptr)[j * width + i]) & 0x07C0) >> 3) & 0xFF;
							p[(j * width + i) * 4    ] = (((((UINT16*)vram_ptr)[j * width + i]) & 0x001F) << 3) & 0xFF;
						}
					}
				} else if(bpp == 32) {
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 4 + 2] = (((((UINT32*)vram_ptr)[j * width + i]) & 0x00FF0000) >> 16) & 0xFF;
							p[(j * width + i) * 4 + 1] = (((((UINT32*)vram_ptr)[j * width + i]) & 0x0000FF00) >>  8) & 0xFF;
							p[(j * width + i) * 4    ] = (((((UINT32*)vram_ptr)[j * width + i]) & 0x000000FF)      ) & 0xFF;
						}
					}
				}
#elif defined(NP2_X)
				scanptr = vram_ptr;
				if(np2wabwnd.pPixbuf) {
					if(bpp == 24) {
						VRAMBuf = gdk_pixbuf_new_from_data(vram_ptr, GDK_COLORSPACE_RGB, FALSE, 8, width, height, width*bpp/8, NULL,NULL);
					} else if(bpp == 16) {
						VRAMBuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
						p = gdk_pixbuf_get_pixels(VRAMBuf);
						for(j = 0; j < height; j++) {
							for(i = 0; i < width; i++) {
								p[(j * width + i) * 3    ] = (((((UINT16*)vram_ptr)[j * width + i]) & 0xF800) >> 8) & 0xFF;
								p[(j * width + i) * 3 + 1] = (((((UINT16*)vram_ptr)[j * width + i]) & 0x07C0) >> 3) & 0xFF;
								p[(j * width + i) * 3 + 2] = (((((UINT16*)vram_ptr)[j * width + i]) & 0x001F) << 3) & 0xFF;
							}
						}
					} else if(bpp == 32) {
						VRAMBuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
						p = gdk_pixbuf_get_pixels(VRAMBuf);
						for(j = 0; j < height; j++) {
							for(i = 0; i < width; i++) {
								p[(j * width + i) * 3    ] = (((((UINT32*)vram_ptr)[j * width + i]) & 0x00FF0000) >> 16) & 0xFF;
								p[(j * width + i) * 3 + 1] = (((((UINT32*)vram_ptr)[j * width + i]) & 0x0000FF00) >>  8) & 0xFF;
								p[(j * width + i) * 3 + 2] = (((((UINT32*)vram_ptr)[j * width + i]) & 0x000000FF)      ) & 0xFF;
							}
						}
					}
				}
#endif
			}
		}else{
			// ズレがあるなら1ラインずつ転送
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
			scanptr = vram_ptr;
			for(i=0;i<height;i++){
				ga_bmpInfo->bmiHeader.biWidth = width;
				ga_bmpInfo->bmiHeader.biHeight = 1;
				r = SetDIBitsToDevice(
					hdc , 0 , i ,
					width , 1 ,
					0 , 0 , 0 , 1 ,
					scanptr , ga_bmpInfo , DIB_RGB_COLORS
				);
				scanptr += scanW;
			}
#elif defined(NP2_SDL) || defined(__LIBRETRO__)
			scanptr = vram_ptr;
			VRAMBuf = (unsigned int*)malloc(width * height * sizeof(unsigned int));
			p = (unsigned char*)VRAMBuf;
			if(bpp == 15) {
				for(j = 0; j < height; j++) {
					for(i = 0; i < width; i++) {
						p[(j * width + i) * 4 + 2] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x7C00) >> 7) & 0xFF;
						p[(j * width + i) * 4 + 1] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x03E0) >> 2) & 0xFF;
						p[(j * width + i) * 4    ] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x001F) << 3) & 0xFF;
					}
				}
			} else if(bpp == 16) {
				for(j = 0; j < height; j++) {
					for(i = 0; i < width; i++) {
						p[(j * width + i) * 4    ] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0xF800) >> 8) & 0xFF;
						p[(j * width + i) * 4 + 1] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x07C0) >> 3) & 0xFF;
						p[(j * width + i) * 4 + 2] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x001F) << 3) & 0xFF;
					}
				}
			} else if(bpp == 24) {
				for(j = 0; j < height; j++) {
					for(i = 0; i < width; i++) {
						p[(j * width + i) * 4    ] = vram_ptr[j * scanW + i * 3    ];
						p[(j * width + i) * 4 + 1] = vram_ptr[j * scanW + i * 3 + 1];
						p[(j * width + i) * 4 + 2] = vram_ptr[j * scanW + i * 3 + 2];
					}
				}
			} else if(bpp == 32) {
				for(j = 0; j < height; j++) {
					for(i = 0; i < width; i++) {
						p[(j * width + i) * 4 + 2] = ((*(UINT32*)(vram_ptr + j * scanW + i * 4) & 0x00FF0000) >> 16) & 0xFF;
						p[(j * width + i) * 4 + 1] = ((*(UINT32*)(vram_ptr + j * scanW + i * 4) & 0x0000FF00) >>  8) & 0xFF;
						p[(j * width + i) * 4    ] = ((*(UINT32*)(vram_ptr + j * scanW + i * 4) & 0x000000FF)      ) & 0xFF;
					}
				}
			} else {
				for(j = 0; j < height; j++) {
					for(i = 0; i < width; i++) {
						p[(j * width + i) * 4    ] = vram_ptr[j * scanW + i * 3    ];
						p[(j * width + i) * 4 + 1] = vram_ptr[j * scanW + i * 3 + 1];
						p[(j * width + i) * 4 + 2] = vram_ptr[j * scanW + i * 3 + 2];
					}
				}
			}
#elif defined(NP2_X)
			scanptr = vram_ptr;
			if(np2wabwnd.pPixbuf) {
				if(bpp == 15) {
					VRAMBuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
					p = gdk_pixbuf_get_pixels(VRAMBuf);
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 3    ] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x7C00) >> 7) & 0xFF;
							p[(j * width + i) * 3 + 1] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x03E0) >> 2) & 0xFF;
							p[(j * width + i) * 3 + 2] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x001F) << 3) & 0xFF;
						}
					}
				} else if(bpp == 16) {
					VRAMBuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
					p = gdk_pixbuf_get_pixels(VRAMBuf);
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 3    ] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0xF800) >> 8) & 0xFF;
							p[(j * width + i) * 3 + 1] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x07C0) >> 3) & 0xFF;
							p[(j * width + i) * 3 + 2] = ((*(UINT16*)(vram_ptr + j * scanW + i * 2) & 0x001F) << 3) & 0xFF;
						}
					}
				} else if(bpp == 24) {
					VRAMBuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
					p = gdk_pixbuf_get_pixels(VRAMBuf);
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 3    ] = vram_ptr[(j * scanW) + i * 3    ];
							p[(j * width + i) * 3 + 1] = vram_ptr[(j * scanW) + i * 3 + 1];
							p[(j * width + i) * 3 + 2] = vram_ptr[(j * scanW) + i * 3 + 2];
						}
					}
				} else if(bpp == 32) {
					VRAMBuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
					p = gdk_pixbuf_get_pixels(VRAMBuf);
					for(j = 0; j < height; j++) {
						for(i = 0; i < width; i++) {
							p[(j * width + i) * 3    ] = vram_ptr[(j * scanW) + i * 4    ];
							p[(j * width + i) * 3 + 1] = vram_ptr[(j * scanW) + i * 4 + 1];
							p[(j * width + i) * 3 + 2] = vram_ptr[(j * scanW) + i * 4 + 2];
						}
					}
				} else {
					VRAMBuf = gdk_pixbuf_new_from_data(vram_ptr, GDK_COLORSPACE_RGB, FALSE, 8, width, height, width*(bpp/8), NULL,NULL);
				}
			}
#endif
		}
	}
	
	// CL-GD544x Video Window
	if(cirrusvga->cr[0x3e] & 0x01){ // Check Video Window Master Enable bit
		uint32_t_* bitfleld;
		int vidwnd_format = ((cirrusvga->cr[0x3e] >> 1) & 0x7);
		int vidwnd_bpp = (vidwnd_format==0 || vidwnd_format==4 || vidwnd_format==5) ? 16 : 8;
		int vidwnd_horizontalZoom = (cirrusvga->cr[0x31]);
		int vidwnd_verticalZoom = (cirrusvga->cr[0x32]);
		int vidwnd_region1Adj = (cirrusvga->cr[0x5d]>>0) & 0x3;
		int vidwnd_region2Adj = (cirrusvga->cr[0x5d]>>4) & 0x3;
		int vidwnd_region1Size = (((cirrusvga->cr[0x36] >> 0) & 0x03) << 8)|(cirrusvga->cr[0x33]);
		int vidwnd_region2Size = (((cirrusvga->cr[0x36] >> 2) & 0x03) << 8)|(cirrusvga->cr[0x34]);
		int vidwnd_region2SDSize = (((cirrusvga->cr[0x36] >> 4) & 0x03) << 8)|(cirrusvga->cr[0x35]);
		int vidwnd_verticalStart = (((cirrusvga->cr[0x39] >> 0) & 0x03) << 8)|(cirrusvga->cr[0x37]);
		int vidwnd_verticalEnd = (((cirrusvga->cr[0x39] >> 2) & 0x03) << 8)|(cirrusvga->cr[0x38]);
		int vidwnd_startAddress = ((cirrusvga->cr[0x3c] & 0x0f) << 18)|(cirrusvga->cr[0x3b] << 10)|(cirrusvga->cr[0x3a] << 2);
		int vidwnd_bufAddressOffset = (((cirrusvga->cr[0x3c] >> 5) & 0x01) << 11)|(cirrusvga->cr[0x3d] << 3);
		int vidwnd_srcwidth = vidwnd_region2SDSize * vidwnd_bpp/8;
		int vidwnd_srcheight = vidwnd_verticalEnd - vidwnd_verticalStart;
		int vidwnd_srcpitch = vidwnd_bufAddressOffset;
		int vidwnd_dstwidth = vidwnd_srcwidth;//vidwnd_region2Size * bpp/8;
		int vidwnd_dstheight = vidwnd_verticalEnd - vidwnd_verticalStart;
		int vidwnd_dstX = vidwnd_region1Size * 32 / bpp + vidwnd_region1Adj * 8 / bpp;
		int vidwnd_dstY = vidwnd_verticalStart;
		int vidwnd_yuv = 0;
		if(vidwnd_horizontalZoom > 0) vidwnd_dstwidth = vidwnd_srcwidth * 256 / vidwnd_horizontalZoom;
		if(vidwnd_verticalZoom > 0) vidwnd_srcheight = vidwnd_dstheight * vidwnd_verticalZoom / 256;
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
		switch(vidwnd_format){
		case 0: // YUV 4:2:2 UYVY
			vidwnd_yuv = 1;
			vidwnd_bpp = 32;
			ga_bmpInfo->bmiHeader.biCompression = BI_RGB;
			break;
		case 4: // RGB555
			//// XXX: RGB555になってない？？？
			//ga_bmpInfo->bmiHeader.biCompression = BI_RGB;
			//break;
		case 5: // RGB565
			// ビットフィールドでRGB565を指定
			bitfleld = (uint32_t_*)(ga_bmpInfo->bmiColors);
			bitfleld[0] = 0x0000F800;
			bitfleld[1] = 0x000007E0;
			bitfleld[2] = 0x0000001F;
			ga_bmpInfo->bmiHeader.biCompression = BI_BITFIELDS;
			break;
		default:
			break;
		}
		ga_bmpInfo->bmiHeader.biBitCount = vidwnd_bpp;
		scanptr = vram_ptr + vidwnd_startAddress;
#if defined(DEBUG_CIRRUS_VRAM)
		scanptr = scanptr - 1280*16*memshift; // DEBUG
#endif
		if(vidwnd_dstwidth == vidwnd_srcwidth && vidwnd_dstheight == vidwnd_srcheight){
			if(!vidwnd_yuv){
				for(i=0;i<vidwnd_srcheight;i++){
					ga_bmpInfo->bmiHeader.biWidth = width;
					ga_bmpInfo->bmiHeader.biHeight = 1;
					r = SetDIBitsToDevice(
						hdc , vidwnd_dstX , vidwnd_dstY + i ,
						vidwnd_srcwidth , 1 ,
						0 , 0 , 0 , 1 ,
						scanptr , ga_bmpInfo , DIB_RGB_COLORS
					);
					scanptr += vidwnd_srcpitch;
				}
			}else{
				unsigned char *linebuf = (unsigned char*)malloc(vidwnd_srcwidth*4);
				for(i=0;i<vidwnd_srcheight;i++){
					ConvertYUV2RGB(vidwnd_srcwidth, (unsigned char *)scanptr, linebuf);
					ga_bmpInfo->bmiHeader.biWidth = width;
					ga_bmpInfo->bmiHeader.biHeight = 1;
					r = SetDIBitsToDevice(
						hdc , vidwnd_dstX , vidwnd_dstY + i ,
						vidwnd_srcwidth , 1 ,
						0 , 0 , 0 , 1 ,
						linebuf , ga_bmpInfo , DIB_RGB_COLORS
					);
					scanptr += vidwnd_srcpitch;
				}
				free(linebuf);
			}
		}else{
			if(!vidwnd_yuv){
				for(i=0;i<vidwnd_dstheight;i++){
					ga_bmpInfo->bmiHeader.biWidth = width;
					ga_bmpInfo->bmiHeader.biHeight = 1;
					r = StretchDIBits(
						hdc , vidwnd_dstX , vidwnd_dstY + i ,
						vidwnd_dstwidth , 1 ,
						0 , 0 , vidwnd_srcwidth , 1 ,
						scanptr + (i * vidwnd_srcheight / vidwnd_dstheight) * vidwnd_srcpitch, ga_bmpInfo , DIB_RGB_COLORS , SRCCOPY
					);
				}
			}else{
				int lastline = -1;
				unsigned char *linebuf = (unsigned char*)malloc(vidwnd_srcwidth*4);
				for(i=0;i<vidwnd_dstheight;i++){
					int curline = (i * vidwnd_srcheight / vidwnd_dstheight);
					if(lastline != curline){
						ConvertYUV2RGB(vidwnd_srcwidth, (unsigned char *)(scanptr + curline * vidwnd_srcpitch), linebuf);
						lastline = curline;
					}
					ga_bmpInfo->bmiHeader.biWidth = width;
					ga_bmpInfo->bmiHeader.biHeight = 1;
					r = StretchDIBits(
						hdc , vidwnd_dstX , vidwnd_dstY + i ,
						vidwnd_dstwidth , 1 ,
						0 , 0 , vidwnd_srcwidth , 1 ,
						linebuf, ga_bmpInfo , DIB_RGB_COLORS , SRCCOPY
					);
				}
				free(linebuf);
			}
		}
#elif defined(NP2_X)
		// TODO: 非Windows用コードを書く
#endif
	}

    if ((cirrusvga->sr[0x12] & CIRRUS_CURSOR_SHOW)){
		int hwcur_x = cirrusvga->hw_cursor_x + cursot_ofs_x;
		int hwcur_y = cirrusvga->hw_cursor_y + cursot_ofs_y;
		// GA-98NB用 カーソル位置調整
		if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
			if(width==320 && height==240){
				hwcur_y /= 2;
			}
		}
		if(np2cfg.gd5430fakecur){
			// ハードウェアカーソルが上手く表示できない場合用
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
			DrawIcon(hdc, hwcur_x, hwcur_y, ga_hFakeCursor);
#elif defined(NP2_SDL) || defined(__LIBRETRO__)
			int fcx, fcy;
			UINT8 col;
			for(fcy = 0; fcy < CIRRUS_FMC_H; fcy++) {
				for(fcx = 0; fcx < CIRRUS_FMC_W; fcx++) {
					col = FakeMouseCursorData[fcy * CIRRUS_FMC_W + fcx];
					if(col && cirrusvga->hw_cursor_x + fcx < width && cirrusvga->hw_cursor_y + fcy < height) {
						col--;
						p[((cirrusvga->hw_cursor_y + fcy) * width + cirrusvga->hw_cursor_x + fcx) * 4    ] = \
						p[((cirrusvga->hw_cursor_y + fcy) * width + cirrusvga->hw_cursor_x + fcx) * 4 + 1] = \
						p[((cirrusvga->hw_cursor_y + fcy) * width + cirrusvga->hw_cursor_x + fcx) * 4 + 2] = col * 0xFF;
					}
				}
			}
#elif defined(NP2_X)
			int fcx, fcy;
			UINT8 col;
			p = gdk_pixbuf_get_pixels(VRAMBuf);
			for(fcy = 0; fcy < CIRRUS_FMC_H; fcy++) {
				for(fcx = 0; fcx < CIRRUS_FMC_W; fcx++) {
					col = FakeMouseCursorData[fcy * CIRRUS_FMC_W + fcx];
					if(col && cirrusvga->hw_cursor_x + fcx < width && cirrusvga->hw_cursor_y + fcy < height) {
						col--;
						p[((cirrusvga->hw_cursor_y + fcy) * width + cirrusvga->hw_cursor_x + fcx) * 3    ] = \
						p[((cirrusvga->hw_cursor_y + fcy) * width + cirrusvga->hw_cursor_x + fcx) * 3 + 1] = \
						p[((cirrusvga->hw_cursor_y + fcy) * width + cirrusvga->hw_cursor_x + fcx) * 3 + 2] = col * 0xFF;
					}
				}
			}
#endif
		}else{
			//HPALETTE oldCurPalette;
			int cursize = 32;
			int x, y;
			int x1, x2, h, w;
			//unsigned int basecolor;
			unsigned int color0, color1;
			uint8_t *d1 = cirrusvga->vram_ptr;
			uint8_t *palette, *src, *base;
			uint32_t_ *dst;
			uint32_t_ content;
			uint32_t_ colortmp;
			const uint8_t *plane0, *plane1;
			int b0, b1;
			int poffset;
			if (cirrusvga->sr[0x12] & CIRRUS_CURSOR_LARGE) {
				cursize = 64;
			}
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
			BitBlt(ga_hdc_cursor , 0 , 0 , cursize , cursize , hdc , hwcur_x , hwcur_y , SRCCOPY);
		
			if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
				base = cirrusvga->vram_ptr + cirrusvga->real_vram_size - 16 * 1024;
			}else if(np2clvga.gd54xxtype <= 0xff){
				base = cirrusvga->vram_ptr + 1024 * 1024 - 16 * 1024; // ??? 1MB前提？
			}else{
				base = cirrusvga->vram_ptr + cirrusvga->real_vram_size - 16 * 1024;
			}
			if (height >= hwcur_y && 0 <= (hwcur_y + cursize)){
				int y1, y2;
				y1 = hwcur_y; 
				y2 = hwcur_y+cursize; 
				h = cursize;
				dst = (uint32_t_ *)cursorptr;
				dst += 64*64;
				dst -= 64;
				palette = cirrusvga->cirrus_hidden_palette;
				color0 = rgb_to_pixel32(c6_to_8(palette[0x0 * 3]),
											c6_to_8(palette[0x0 * 3 + 1]),
											c6_to_8(palette[0x0 * 3 + 2]));
				color1 = rgb_to_pixel32(c6_to_8(palette[0xf * 3]),
											c6_to_8(palette[0xf * 3 + 1]),
											c6_to_8(palette[0xf * 3 + 2]));
				for(y=y1;y<y2;y++){
					src = base;
					if (cirrusvga->sr[0x12] & CIRRUS_CURSOR_LARGE) {
						src += (cirrusvga->sr[0x13] & 0x3c) * 256;
						src += (y - (int)hwcur_y) * 16;
						poffset = 8;
						content = ((uint32_t_ *)src)[0] |
							((uint32_t_ *)src)[1] |
							((uint32_t_ *)src)[2] |
							((uint32_t_ *)src)[3];
					} else {
						src += (cirrusvga->sr[0x13] & 0x3f) * 256;
						src += (y - (int)hwcur_y) * 4;
						poffset = 128;
						content = ((uint32_t_ *)src)[0] |
							((uint32_t_ *)(src + 128))[0];
					}
					/* if nothing to draw, no need to continue */
					x1 = hwcur_x;
					if (x1 < width){
						x2 = hwcur_x + cursize;
						w = x2 - x1;
						for(x=0;x<w;x++){
							colortmp = *dst;
							plane0 = src;
							plane1 = src + poffset;
							b0 = (plane0[(x) >> 3] >> (7 - ((x) & 7))) & 1;
							b1 = (plane1[(x) >> 3] >> (7 - ((x) & 7))) & 1;
							switch(b0 | (b1 << 1)) {
							case 0:
								break;
							case 1:
								colortmp ^= 0xffffff;
								break;
							case 2:
								colortmp = color0;
								break;
							case 3:
								colortmp = color1;
								break;
							}
							*dst = colortmp;
							dst++;
						}
						dst += (64 - w);
					}
					dst -= 64*2;
				}
			}
			BitBlt(hdc , hwcur_x , hwcur_y , cursize , cursize , ga_hdc_cursor , 0 , 0 , SRCCOPY);
#elif defined(NP2_SDL) || defined(__LIBRETRO__)
			if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
				base = cirrusvga->vram_ptr + cirrusvga->real_vram_size - 16 * 1024;
			}else if(np2clvga.gd54xxtype <= 0xff){
				base = cirrusvga->vram_ptr + 1024 * 1024 - 16 * 1024; // ??? 1MB前提？
			}else{
				base = cirrusvga->vram_ptr + cirrusvga->real_vram_size - 16 * 1024;
			}
			if (height >= hwcur_y && 0 <= (hwcur_y + cursize)){
				UINT8 color0_r, color0_g, color0_b;
				UINT8 color1_r, color1_g, color1_b;
				int y1, y2;
				y1 = hwcur_y; 
				y2 = hwcur_y+cursize; 
				h = cursize;
				p += (cirrusvga->hw_cursor_y * width + cirrusvga->hw_cursor_x) * 4; // カーソル位置のポインタ
				palette = cirrusvga->cirrus_hidden_palette;
				color0_r = c6_to_8(palette[0x0 * 3]);
				color0_g = c6_to_8(palette[0x0 * 3 + 1]);
				color0_b = c6_to_8(palette[0x0 * 3 + 2]);
				color1_r = c6_to_8(palette[0xf * 3]);
				color1_g = c6_to_8(palette[0xf * 3 + 1]);
				color1_b = c6_to_8(palette[0xf * 3 + 2]);
				if(y2 > height) y2 = height;
				for(y=y1;y<y2;y++){
					UINT8 colortmp_r, colortmp_g, colortmp_b;
					const uint8_t *plane0, *plane1;
					int b0, b1;
					src = base;
					if (cirrusvga->sr[0x12] & CIRRUS_CURSOR_LARGE) {
						src += (cirrusvga->sr[0x13] & 0x3c) * 256;
						src += (y - (int)hwcur_y) * 16;
						poffset = 8;
						content = ((uint32_t_ *)src)[0] |
							((uint32_t_ *)src)[1] |
							((uint32_t_ *)src)[2] |
							((uint32_t_ *)src)[3];
					} else {
						src += (cirrusvga->sr[0x13] & 0x3f) * 256;
						src += (y - (int)hwcur_y) * 4;
						poffset = 128;
						content = ((uint32_t_ *)src)[0] |
							((uint32_t_ *)(src + 128))[0];
					}
					/* if nothing to draw, no need to continue */
					x1 = hwcur_x;
					if (x1 < width){
						x2 = hwcur_x + cursize;
						w = x2 - x1;
						for(x=0;x<w;x++){
							if(hwcur_x + x < width){
								colortmp_r = *p;
								colortmp_g = *(p+1);
								colortmp_b = *(p+2);
								plane0 = src;
								plane1 = src + poffset;
								b0 = (plane0[(x) >> 3] >> (7 - ((x) & 7))) & 1;
								b1 = (plane1[(x) >> 3] >> (7 - ((x) & 7))) & 1;
								switch(b0 | (b1 << 1)) {
								case 0:
									break;
								case 1:
									colortmp_r ^= 0xff;
									colortmp_g ^= 0xff;
									colortmp_b ^= 0xff;
									break;
								case 2:
									colortmp_r = color0_r;
									colortmp_g = color0_g;
									colortmp_b = color0_b;
									break;
								case 3:
									colortmp_r = color1_r;
									colortmp_g = color1_g;
									colortmp_b = color1_b;
									break;
								}
								*p = colortmp_r;
								*(p+1) = colortmp_g;
								*(p+2)     = colortmp_b;
							}
							p += 4;
						}
						p += (width - w) * 4;
					}
				}
			}
#elif defined(NP2_X)
			if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
				base = cirrusvga->vram_ptr + cirrusvga->real_vram_size - 16 * 1024;
			}else if(np2clvga.gd54xxtype <= 0xff){
				base = cirrusvga->vram_ptr + 1024 * 1024 - 16 * 1024; // ??? 1MB前提？
			}else{
				base = cirrusvga->vram_ptr + cirrusvga->real_vram_size - 16 * 1024;
			}
			if (height >= hwcur_y && 0 <= (hwcur_y + cursize)){
				UINT8 color0_r, color0_g, color0_b;
				UINT8 color1_r, color1_g, color1_b;
				int y1, y2;
				y1 = hwcur_y; 
				y2 = hwcur_y+cursize; 
				h = cursize;
				p = gdk_pixbuf_get_pixels(VRAMBuf);
				p += (cirrusvga->hw_cursor_y * width + cirrusvga->hw_cursor_x) * 3; // カーソル位置のポインタ
				palette = cirrusvga->cirrus_hidden_palette;
				color0_r = c6_to_8(palette[0x0 * 3]);
				color0_g = c6_to_8(palette[0x0 * 3 + 1]);
				color0_b = c6_to_8(palette[0x0 * 3 + 2]);
				color1_r = c6_to_8(palette[0xf * 3]);
				color1_g = c6_to_8(palette[0xf * 3 + 1]);
				color1_b = c6_to_8(palette[0xf * 3 + 2]);
				if(y2 > height) y2 = height;
				for(y=y1;y<y2;y++){
					src = base;
					if (cirrusvga->sr[0x12] & CIRRUS_CURSOR_LARGE) {
						src += (cirrusvga->sr[0x13] & 0x3c) * 256;
						src += (y - (int)hwcur_y) * 16;
						poffset = 8;
						content = ((uint32_t_ *)src)[0] |
							((uint32_t_ *)src)[1] |
							((uint32_t_ *)src)[2] |
							((uint32_t_ *)src)[3];
					} else {
						src += (cirrusvga->sr[0x13] & 0x3f) * 256;
						src += (y - (int)hwcur_y) * 4;
						poffset = 128;
						content = ((uint32_t_ *)src)[0] |
							((uint32_t_ *)(src + 128))[0];
					}
					/* if nothing to draw, no need to continue */
					UINT8 colortmp_r, colortmp_g, colortmp_b;
					const uint8_t *plane0, *plane1;
					int b0, b1;
					x1 = hwcur_x;
					if (x1 < width){
						x2 = hwcur_x + cursize;
						w = x2 - x1;
						for(x=0;x<w;x++){
							if(hwcur_x + x < width){
								colortmp_r = *p;
								colortmp_g = *(p+1);
								colortmp_b = *(p+2);
								plane0 = src;
								plane1 = src + poffset;
								b0 = (plane0[(x) >> 3] >> (7 - ((x) & 7))) & 1;
								b1 = (plane1[(x) >> 3] >> (7 - ((x) & 7))) & 1;
								switch(b0 | (b1 << 1)) {
								case 0:
									break;
								case 1:
									colortmp_r ^= 0xff;
									colortmp_g ^= 0xff;
									colortmp_b ^= 0xff;
									break;
								case 2:
									colortmp_r = color0_r;
									colortmp_g = color0_g;
									colortmp_b = color0_b;
									break;
								case 3:
									colortmp_r = color1_r;
									colortmp_g = color1_g;
									colortmp_b = color1_b;
									break;
								}
								*p = colortmp_r;
								*(p+1) = colortmp_g;
								*(p+2) = colortmp_b;
							}
							p += 3;
						}
						p += (width - w) * 3;
					}
				}
			}
#endif
		}
	}
#if defined(NP2_SDL) || defined(__LIBRETRO__)
	memcpy(np2wabwnd.pBuffer, VRAMBuf, width * height * sizeof(unsigned int));
	free(VRAMBuf);
#elif defined(NP2_X)
	if(VRAMBuf && np2wabwnd.pPixbuf) {
		gdk_pixbuf_scale(VRAMBuf, np2wabwnd.pPixbuf,
			0, 0, width, height,
			0, 0, 1, 1,
			GDK_INTERP_NEAREST);
		g_object_unref(VRAMBuf);
		if(bpp == 24) {
			UINT8* ptr = (UINT8 *)gdk_pixbuf_get_pixels(np2wabwnd.pPixbuf);
			for(i = 0; i < 1280 * 1024; i++) {
				j = ptr[i * 3];
				ptr[i * 3] = ptr[i * 3 + 2];
				ptr[i * 3 + 2] = j;
			}
		}
	}
#endif
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	ga_bmpInfo->bmiHeader.biWidth = width; // 前回の解像度を保存
	ga_bmpInfo->bmiHeader.biHeight = height; // 前回の解像度を保存
#endif
}

/***************************************
 *
 *  PC-9821 support
 *
 ***************************************/
static void IOOUTCALL cirrusvga_ofa2(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: set register index %02X", dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_regindexA2 = dat;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_ifa2(UINT port) {
	TRACEOUT(("CIRRUS VGA: get register index %02X", cirrusvga_regindex));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	return cirrusvga_regindexA2;
}
static void IOOUTCALL cirrusvga_ofa3(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: out %04X d=%.2X", port, dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	switch(cirrusvga_regindexA2){
	case 0x00:
		// 機種判定？
		break;
	case 0x01:
		// VRAMウィンドウアドレス設定
		switch(dat){
		case 0x10:
			np2clvga.VRAMWindowAddr2 = 0x0b0000;
			break;
		case 0x80:
			np2clvga.VRAMWindowAddr2 = 0xf20000;
			break;
		case 0xA0:
			np2clvga.VRAMWindowAddr2 = 0xf00000;
			break;
		case 0xC0:
			np2clvga.VRAMWindowAddr2 = 0xf40000;
			break;
		case 0xE0:
			np2clvga.VRAMWindowAddr2 = 0xf60000;
			break;
		}
		break;
	case 0x02:
		// リニアVRAMアクセス用アドレス設定
		if(np2clvga.gd54xxtype != CIRRUS_98ID_PCI){
			if(np2clvga.gd54xxtype <= 0xff){
				if(dat!=0x00 && dat!=0xff) np2clvga.VRAMWindowAddr = (dat<<24);
			}
		}
		break;
	case 0x03:
		// 出力切替リレー制御
		if((!!np2wab.relaystateint) != (!!(dat&0x2))){
			np2wab.relaystateint = dat & 0x2;
			np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
		}
		np2clvga.mmioenable = (dat&0x1);
		break;
	}
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_ifa3(UINT port) {
	REG8 ret = 0xff;

	TRACEOUT(("CIRRUS VGA: inp %04X", port));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	switch(cirrusvga_regindexA2){
	case 0x00:
		// 機種判定？
		if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
			ret = 0xff;
		}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
			ret = (REG8)np2clvga.gd54xxtype;
		}else{
			ret = 0xff;
		}
		break;
	case 0x01:
		// VRAMウィンドウアドレス設定
		//if(np2clvga.gd54xxtype <= 0xff){
			switch(np2clvga.VRAMWindowAddr2){
			case 0x0b0000:
				ret = 0x10;
				break;
			case 0xf00000:
				ret = 0xA0;
				break;
			case 0xf20000:
				ret = 0x80;
				break;
			case 0xf40000:
				ret = 0xC0;
				break;
			case 0xf60000:
				ret = 0xE0;
				break;
			}
		//}else{
		//	ret = 0xff;
		//}
		break;
	case 0x02:
		// リニアVRAMアクセス用アドレス設定
		if(np2clvga.gd54xxtype <= 0xff){
			ret = (np2clvga.VRAMWindowAddr>>24)&0xff;
		}else{
			ret = 0xff;
		}
		break;
	case 0x03:
		// 出力切替リレー制御
		ret = ((np2wab.relaystateint&0x2) ? 0x2 : 0x0) | np2clvga.mmioenable;
		break;
	case 0x04:
		// ？
		ret = 0x00;
		break;
	}
	return ret;
}

static void IOOUTCALL cirrusvga_ofaa(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: set register index %02X", dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_regindex = dat;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_ifaa(UINT port) {
	TRACEOUT(("CIRRUS VGA: get register index %02X", cirrusvga_regindex));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	return cirrusvga_regindex;
}
static void IOOUTCALL cirrusvga_ofab(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: out %04X d=%.2X", port, dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	switch(cirrusvga_regindex){
	case 0x00:
		// 機種判定
		break;
	case 0x01:
		// VRAMウィンドウアドレス設定
		switch(dat){
		case 0x10:
			np2clvga.VRAMWindowAddr2 = 0x0b0000;
			break;
		case 0x80:
			np2clvga.VRAMWindowAddr2 = 0xf20000;
			break;
		case 0xA0:
			np2clvga.VRAMWindowAddr2 = 0xf00000;
			break;
		case 0xC0:
			np2clvga.VRAMWindowAddr2 = 0xf40000;
			break;
		case 0xE0:
			np2clvga.VRAMWindowAddr2 = 0xf60000;
			break;
		}
		break;
	case 0x02:
		// リニアVRAMアクセス用アドレス設定
		if(dat!=0x00 && dat!=0xff) np2clvga.VRAMWindowAddr = (dat<<24);
		//cirrusvga->vram_offset = np2clvga.VRAMWindowAddr;
		break;
	case 0x03:
		// 出力切替リレー制御
		if((!!np2wab.relaystateint) != (!!(dat&0x2))){
			np2wab.relaystateint = dat & 0x2;
			np2wab.relaystateext = dat & 0x2; // （￣∀￣;）
			np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
		}
		np2clvga.mmioenable = (dat&0x1);
		break;
	}
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_ifab(UINT port) {
	REG8 ret = 0xff;
	TRACEOUT(("CIRRUS VGA: inp %04X", port));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	switch(cirrusvga_regindex){
	case 0x00:
		// 機種判定
		if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
			ret = 0xff;
		}else if(np2clvga.gd54xxtype <= 0xff){
			ret = (REG8)np2clvga.gd54xxtype;//0x5B;
		}else{
			ret = 0xff;
		}
		break;
	case 0x01:
		// VRAMウィンドウアドレス設定
		if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
			ret = 0x80;
		}else{
			if(np2clvga.gd54xxtype <= 0xff){
				switch(np2clvga.VRAMWindowAddr2){
				case 0x0b0000:
					ret = 0x10;
					break;
				case 0xf20000:
					ret = 0x80;
					break;
				case 0xf00000:
					ret = 0xA0;
					break;
				case 0xf40000:
					ret = 0xC0;
					break;
				case 0xf60000:
					ret = 0xE0;
					break;
				}
			}else{
				ret = 0xff;
			}
		}
		break;
	case 0x02:
		// リニアVRAMアクセス用アドレス設定
		if(np2clvga.gd54xxtype <= 0xff){
			ret = (np2clvga.VRAMWindowAddr>>24)&0xff;
		}else{
			ret = 0xff;
		}
		break;
	case 0x03:
		// 出力切替リレー制御
		ret = (np2wab.relay ? 0x2 : 0x0) | np2clvga.mmioenable;
		break;
	}
	return ret;
}

int cirrusvga_videoenable = 0x00;
static void IOOUTCALL cirrusvga_off82(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: out %04X d=%02X", port, dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_videoenable = dat & 0x1;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_iff82(UINT port) {
	TRACEOUT(("CIRRUS VGA: inp %04X", port));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	return cirrusvga_videoenable;
}

int cirrusvga_reg0904 = 0x00;
static void IOOUTCALL cirrusvga_o0904(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: out %04X d=%02X", port, dat));
	cirrusvga_reg0904 = dat;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_i0904(UINT port) {
	TRACEOUT(("CIRRUS VGA: inp %04X", port));
	return cirrusvga_reg0904;
}

// WAB, WSN用
static void cirrusvga_setAutoWABID() {
	switch(np2clvga.defgd54xxtype){
	case CIRRUS_98ID_AUTO_XE_G1_PCI:
		np2clvga.gd54xxtype = CIRRUS_98ID_GA98NBIC;
		memset(cirrusvga->vram_ptr, 0x00, cirrusvga->real_vram_size);
		cirrusvga_wab_59e1 = 0x06;	// d.c.
		cirrusvga_wab_51e1 = 0xC2;	// d.c.
		cirrusvga_wab_5be1 = 0xf7;	// d.c.
		cirrusvga_wab_40e1 = 0xC2;	// bit1=0:DRAM REFRESH MODE?? とりあえず初期値はC2hじゃないとWin95ドライバはボードを認識しない
		cirrusvga_wab_42e1 = 0x18;  // 存在しない
		cirrusvga_wab_46e8 = 0x18;
		break;
	case CIRRUS_98ID_AUTO_XE_G2_PCI:
		np2clvga.gd54xxtype = CIRRUS_98ID_GA98NBII;
		memset(cirrusvga->vram_ptr, 0x00, cirrusvga->real_vram_size);
		cirrusvga_wab_59e1 = 0x06;	// d.c.
		cirrusvga_wab_51e1 = 0xC2;	// d.c.
		cirrusvga_wab_5be1 = 0xf7;	// d.c.
		cirrusvga_wab_40e1 = 0xC2;	// bit1=0:DRAM REFRESH MODE?? とりあえず初期値はC2hじゃないとWin95ドライバはボードを認識しない
		cirrusvga_wab_42e1 = 0x18;  // 存在しない
		cirrusvga_wab_46e8 = 0x18;
		break;
	case CIRRUS_98ID_AUTO_XE_G4_PCI:
		np2clvga.gd54xxtype = CIRRUS_98ID_GA98NBIV;
		memset(cirrusvga->vram_ptr, 0x00, cirrusvga->real_vram_size);
		cirrusvga_wab_59e1 = 0x06;	// d.c.
		cirrusvga_wab_51e1 = 0xC2;	// d.c.
		cirrusvga_wab_5be1 = 0xf7;	// d.c.
		cirrusvga_wab_40e1 = 0xC2;	// bit1=0:DRAM REFRESH MODE?? とりあえず初期値はC2hじゃないとWin95ドライバはボードを認識しない
		cirrusvga_wab_42e1 = 0x18;  // 存在しない
		cirrusvga_wab_46e8 = 0x18;
		break;
	case CIRRUS_98ID_AUTO_XE10_WABS:
	case CIRRUS_98ID_AUTO_XE_WA_PCI:
		np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN4:
	case CIRRUS_98ID_AUTO_XE_W4_PCI:
		np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
		break;
	default:
		np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
		break;
	}
	pc98_cirrus_setWABreg();
	pc98_cirrus_vga_setvramsize();
	pc98_cirrus_vga_initVRAMWindowAddr();
}

static REG8 IOINPCALL cirrusvga_i59e1(UINT port) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga_setAutoWABID();
	}
	return cirrusvga_wab_59e1;
}
static REG8 IOINPCALL cirrusvga_i51e1(UINT port) {
	REG8 ret = cirrusvga_wab_51e1;
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga_setAutoWABID();
	}
	//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
	//	ret = 0xC2;
	//}
	if (port == 0x51e1) {
 		return 0xff;
 	}
 	else {
 		return ret;
 	}
}
static void IOOUTCALL cirrusvga_o51e1(UINT port, REG8 dat) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga_setAutoWABID();
	}
	cirrusvga_wab_51e1 = dat;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_i5be1(UINT port) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga_setAutoWABID();
	}
//	return 0xf7; // XXX: 0x08 is VRAM 4M flag?
	if (np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F) {
		cirrusvga_wab_5be1 |= 0x08;
	}
	return cirrusvga_wab_5be1;
}
//static void IOOUTCALL cirrusvga_o5be3(UINT port, REG8 dat) {
//	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
//		cirrusvga_setAutoWABID();
//	}
//	(void)port;
//	(void)dat;
//}

static REG8 IOINPCALL cirrusvga_i40e1(UINT port) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || 
		(np2clvga.defgd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK && (np2clvga.gd54xxtype == CIRRUS_98ID_Xe10 || np2clvga.gd54xxtype == CIRRUS_98ID_PCI)){ // 強制変更を許す
		cirrusvga_setAutoWABID();
	}
	//cirrusvga_wab_40e1--;
	//TRACEOUT(("CIRRUS VGA: out %04X d=%02X", port, cirrusvga_wab_40e1));
	return cirrusvga_wab_40e1;
}
static void IOOUTCALL cirrusvga_o40e1(UINT port, REG8 dat) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga_setAutoWABID();
	}
	cirrusvga_wab_40e1 = dat;

	np2wab.relaystateint = (np2wab.relaystateint & ~0x1) | (cirrusvga_wab_40e1 & 0x1);
	np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_i42e1(UINT port) {
	if ((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK) {
		cirrusvga_setAutoWABID();
	}
	return cirrusvga_wab_42e1;
}
static void IOOUTCALL cirrusvga_o42e1(UINT port, REG8 dat) {
	if ((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK) {
		cirrusvga_setAutoWABID();
	}
	cirrusvga_wab_42e1 = dat;
	cirrus_update_memory_access(cirrusvga);
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_i46e8(UINT port) {
	REG8 ret = cirrusvga_wab_46e8;
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga_setAutoWABID();
	}
	//np2wab.relaystateint = (np2wab.relaystateint & ~0x1) | (cirrusvga_wab_40e1 & 0x1);
	//np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
	return ret;
}
static void IOOUTCALL cirrusvga_o46e8(UINT port, REG8 dat) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga_setAutoWABID();
	}
	cirrusvga_wab_46e8 = dat;
	(void)port;
	(void)dat;
}

//int cirrusvga_wab_52E2 = 0x18;
//static REG8 IOINPCALL cirrusvga_i52E2(UINT port) {
//	REG8 ret = cirrusvga_wab_52E2;
//	return ret;
//}
//static void IOOUTCALL cirrusvga_o52E2(UINT port, REG8 dat) {
//	cirrusvga_wab_52E2 = dat;
//	(void)port;
//	(void)dat;
//}
//int cirrusvga_wab_56E2 = 0x80;
//static REG8 IOINPCALL cirrusvga_i56E2(UINT port) {
//	REG8 ret = cirrusvga_wab_56E2;
//	return ret;
//}
//static void IOOUTCALL cirrusvga_o56E2(UINT port, REG8 dat) {
//	cirrusvga_wab_56E2 = dat;
//	(void)port;
//	(void)dat;
//}
//int cirrusvga_wab_59E2 = 0x18;
//static REG8 IOINPCALL cirrusvga_i59E2(UINT port) {
//	REG8 ret = cirrusvga_wab_59E2;
//	return ret;
//}
//static void IOOUTCALL cirrusvga_o59E2(UINT port, REG8 dat) {
//	cirrusvga_wab_59E2 = dat;
//	(void)port;
//	(void)dat;
//}
//int cirrusvga_wab_5BE2 = 0x18;
//static REG8 IOINPCALL cirrusvga_i5BE2(UINT port) {
//	REG8 ret = cirrusvga_wab_5BE2;
//	return ret;
//}
//static void IOOUTCALL cirrusvga_o5BE2(UINT port, REG8 dat) {
//	cirrusvga_wab_5BE2 = dat;
//	(void)port;
//	(void)dat;
//}

static void vga_dumb_update_retrace_info(VGAState *s)
{
    (void) s;
}

// MMIOウィンドウを設定する
void pc98_cirrus_setMMIOWindowAddr(){
	if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		cirrus_mmio_read[0] = cirrus_mmio_readb_wab;
		cirrus_mmio_read[1] = cirrus_mmio_readw_wab;
		cirrus_mmio_read[2] = cirrus_mmio_readl_wab;
		cirrus_mmio_write[0] = cirrus_mmio_writeb_wab;
		cirrus_mmio_write[1] = cirrus_mmio_writew_wab;
		cirrus_mmio_write[2] = cirrus_mmio_writel_wab;
	}else if ((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC) {
		cirrus_mmio_read[0] = cirrus_mmio_readb_wab;
		cirrus_mmio_read[1] = cirrus_mmio_readw_wab;
		cirrus_mmio_read[2] = cirrus_mmio_readl_wab;
		cirrus_mmio_write[0] = cirrus_mmio_writeb_wab;
		cirrus_mmio_write[1] = cirrus_mmio_writew_wab;
		cirrus_mmio_write[2] = cirrus_mmio_writel_wab;
	}else{
		cirrus_mmio_read[0] = cirrus_mmio_readb;
		cirrus_mmio_read[1] = cirrus_mmio_readw;
		cirrus_mmio_read[2] = cirrus_mmio_readl;
		cirrus_mmio_write[0] = cirrus_mmio_writeb;
		cirrus_mmio_write[1] = cirrus_mmio_writew;
		cirrus_mmio_write[2] = cirrus_mmio_writel;
	}
}

void pc98_cirrus_vga_updatePCIaddr(){
	if((np2clvga.gd54xxtype & CIRRUS_98ID_WABMASK) == CIRRUS_98ID_WAB || (np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
		pc98_cirrus_setMMIOWindowAddr();
		return;
	}
#if defined(SUPPORT_PCI)
	if((pcidev.devices[pcidev_cirrus_deviceid].header.baseaddrregs[0] & 0xfffffff0) != ~pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[0]){
		np2clvga.pciLFB_Addr = pcidev.devices[pcidev_cirrus_deviceid].header.baseaddrregs[0] & 0xfffffff0;
		np2clvga.pciLFB_Mask = ~pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[0];

		cirrusvga->map_addr = cirrusvga->map_end = 0;
		cirrusvga->lfb_addr = np2clvga.pciLFB_Addr & TARGET_PAGE_MASK;
		cirrusvga->lfb_end = ((np2clvga.pciLFB_Addr + cirrusvga->real_vram_size) + TARGET_PAGE_SIZE - 1) & TARGET_PAGE_MASK;
		/* account for overflow */
		if (cirrusvga->lfb_end < np2clvga.pciLFB_Addr + cirrusvga->real_vram_size)
			cirrusvga->lfb_end = np2clvga.pciLFB_Addr + cirrusvga->real_vram_size;
	}else{
		np2clvga.pciLFB_Addr = 0;
	}
	if((pcidev.devices[pcidev_cirrus_deviceid].header.baseaddrregs[1] & 0xfffffff0) != ~pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[1]){
		np2clvga.pciMMIO_Addr = pcidev.devices[pcidev_cirrus_deviceid].header.baseaddrregs[1] & 0xfffffff0;
		np2clvga.pciMMIO_Mask = ~pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[1];
	}else{
		np2clvga.pciMMIO_Addr = 0;
	}

	pc98_cirrus_setMMIOWindowAddr();
	cirrus_update_memory_access(cirrusvga);
#endif
}

// VRAMウィンドウアドレスをデフォルト値に設定する
void pc98_cirrus_vga_initVRAMWindowAddr(){
	np2clvga.pciLFB_Addr = 0;
	np2clvga.pciLFB_Mask = 0;
	np2clvga.pciMMIO_Addr = 0;
	np2clvga.pciMMIO_Mask = 0;
#if defined(SUPPORT_PCI)
	pcidev.devices[pcidev_cirrus_deviceid].enable = 0;
#endif
	if(np2clvga.gd54xxtype == CIRRUS_98ID_Be){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xf00000;
		np2clvga.VRAMWindowAddr3 = 0;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xf00000;
		np2clvga.VRAMWindowAddr3 = 0;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0;
		np2clvga.VRAMWindowAddr3 = 0;
#if defined(SUPPORT_PCI)
		pcidev.devices[pcidev_cirrus_deviceid].enable = 1;
#endif
		pc98_cirrus_vga_updatePCIaddr();
	}else if(np2clvga.gd54xxtype <= 0xff){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xf60000;
		np2clvga.VRAMWindowAddr3 = 0;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xE0000;
		np2clvga.VRAMWindowAddr3 = 0xF00000;
		np2clvga.pciMMIO_Addr = 0xf10000;
		np2clvga.pciMMIO_Mask = ~(0x10000-1);
	}else if ((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC) {
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xE0000;
		np2clvga.VRAMWindowAddr3 = 0xF00000;
		np2clvga.pciMMIO_Addr = 0xf10000;
		np2clvga.pciMMIO_Mask = ~(0x10000-1);
	}else{
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0;
		np2clvga.VRAMWindowAddr3 = 0;
		if(np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WS_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_W4_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WA_PCI ||
		   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G1_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G2_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G4_PCI){
#if defined(SUPPORT_PCI)
			pcidev.devices[pcidev_cirrus_deviceid].enable = 1;
#endif
			pc98_cirrus_vga_updatePCIaddr();
		}
	}
	pc98_cirrus_setMMIOWindowAddr();
	cirrus_update_memory_access(cirrusvga);
}

// ボード種類からVRAMサイズを決定する
void pc98_cirrus_vga_setvramsize(){
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_1MB; //(cirrusvga->device_id == CIRRUS_ID_CLGD5446) ? CIRRUS_VRAM_SIZE : CIRRUS_VRAM_SIZE;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_Be){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_1MB; //(cirrusvga->device_id == CIRRUS_ID_CLGD5446) ? CIRRUS_VRAM_SIZE : CIRRUS_VRAM_SIZE;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_2MB;
	}else if(np2clvga.gd54xxtype <= 0xff){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE; //(cirrusvga->device_id == CIRRUS_ID_CLGD5446) ? CIRRUS_VRAM_SIZE : CIRRUS_VRAM_SIZE;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_WAB;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_2MB;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_4MB;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NBIC){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_1MB;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NBII){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_2MB;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NBIV){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_4MB;
	}else{
		cirrusvga->real_vram_size = (cirrusvga->device_id == CIRRUS_ID_CLGD5446) ? CIRRUS_VRAM_SIZE : CIRRUS_VRAM_SIZE;
	}
	cirrusvga->vram_ptr = vramptr;
    cirrusvga->vram_offset = 0;
    cirrusvga->vram_size = cirrusvga->real_vram_size;

    /* XXX: s->vram_size must be a power of two */
	cirrusvga->cirrus_addr_mask = cirrusvga->real_vram_size - 1;
	cirrusvga->linear_mmio_mask = cirrusvga->real_vram_size - 256;

}
void pc98_cirrus_vga_setVRAMWindowAddr3(UINT32 addr)
{
	cirrusvga->sr[0x07] = (cirrusvga->sr[0x07] & 0x0f) | ((addr >> 16) & 0xf0);
	cirrus_update_memory_access(cirrusvga);
}
static void pc98_cirrus_reset(CirrusVGAState * s, int device_id, int is_pci)
{
	//np2clvga.VRAMWindowAddr = (0x0F<<24);
	//np2clvga.VRAMWindowAddr2 = (0xf20000);
	
	np2clvga.VRAMWindowAddr2 = 0;
	np2clvga.VRAMWindowAddr3 = 0;
	
	pc98_cirrus_vga_initVRAMWindowAddr();
	
	s->cirrus_rop = cirrus_bitblt_rop_nop;
	
	np2wab.relaystateext = 0;
	np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext);
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	ShowWindow(np2wabwnd.hWndWAB, SW_HIDE); // 設定変更対策
#endif

	np2clvga.mmioenable = 0;
	np2wab.paletteChanged = 1;
}
void pcidev_cirrus_cfgreg_w(UINT32 devNumber, UINT8 funcNumber, UINT8 cfgregOffset, UINT8 sizeinbytes, UINT32 value){
	if(0x10 <= cfgregOffset && cfgregOffset < 0x28){
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
}
static void pc98_cirrus_init_common(CirrusVGAState * s, int device_id, int is_pci)
{
    int i;

    for(i = 0;i < 256; i++)
        rop_to_index[i] = CIRRUS_ROP_NOP_INDEX; /* nop rop */
    rop_to_index[CIRRUS_ROP_0] = 0;
    rop_to_index[CIRRUS_ROP_SRC_AND_DST] = 1;
    rop_to_index[CIRRUS_ROP_NOP] = 2;
    rop_to_index[CIRRUS_ROP_SRC_AND_NOTDST] = 3;
    rop_to_index[CIRRUS_ROP_NOTDST] = 4;
    rop_to_index[CIRRUS_ROP_SRC] = 5;
    rop_to_index[CIRRUS_ROP_1] = 6;
    rop_to_index[CIRRUS_ROP_NOTSRC_AND_DST] = 7;
    rop_to_index[CIRRUS_ROP_SRC_XOR_DST] = 8;
    rop_to_index[CIRRUS_ROP_SRC_OR_DST] = 9;
    rop_to_index[CIRRUS_ROP_NOTSRC_OR_NOTDST] = 10;
    rop_to_index[CIRRUS_ROP_SRC_NOTXOR_DST] = 11;
    rop_to_index[CIRRUS_ROP_SRC_OR_NOTDST] = 12;
    rop_to_index[CIRRUS_ROP_NOTSRC] = 13;
    rop_to_index[CIRRUS_ROP_NOTSRC_OR_DST] = 14;
    rop_to_index[CIRRUS_ROP_NOTSRC_AND_NOTDST] = 15;
    s->device_id = device_id;
    s->bustype = CIRRUS_BUSTYPE_ISA;

	cirrusvga_wab_46e8 = 0x18; // デフォルトで有効
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype <= 0xff){
		// ONBOARD
#if defined(SUPPORT_PCI)
		if(pcidev.enable && (np2clvga.gd54xxtype == CIRRUS_98ID_PCI || 
		   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WS_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_W4_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WA_PCI || 
		   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G1_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G2_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G4_PCI)){
			// Cirrus CL-GD5446 PCI
			ZeroMemory(pcidev.devices+pcidev_cirrus_deviceid, sizeof(_PCIDEVICE));
			pcidev.devices[pcidev_cirrus_deviceid].enable = 1;
			pcidev.devices[pcidev_cirrus_deviceid].skipirqtbl = 1;
			pcidev.devices[pcidev_cirrus_deviceid].regwfn = &pcidev_cirrus_cfgreg_w;
			pcidev.devices[pcidev_cirrus_deviceid].header.vendorID = 0x1013;
			pcidev.devices[pcidev_cirrus_deviceid].header.deviceID = CIRRUS_ID_CLGD5446;
			pcidev.devices[pcidev_cirrus_deviceid].header.command = 0x0003;//0x0006;//;0x0003;
			pcidev.devices[pcidev_cirrus_deviceid].header.status = 0x0000;//0x0000;//0x0280;
			pcidev.devices[pcidev_cirrus_deviceid].header.revisionID = 0x00;
			pcidev.devices[pcidev_cirrus_deviceid].header.classcode[0] = 0x00; // レジスタレベルプログラミングインタフェース
			pcidev.devices[pcidev_cirrus_deviceid].header.classcode[1] = 0x00; // サブクラスコード
			pcidev.devices[pcidev_cirrus_deviceid].header.classcode[2] = 0x03; // ベースクラスコード
			pcidev.devices[pcidev_cirrus_deviceid].header.cachelinesize = 0;
			pcidev.devices[pcidev_cirrus_deviceid].header.latencytimer = 0x00;
			pcidev.devices[pcidev_cirrus_deviceid].header.headertype = 0;
			pcidev.devices[pcidev_cirrus_deviceid].header.BIST = 0x00;
			pcidev.devices[pcidev_cirrus_deviceid].header.subsysID = 0x0000;
			pcidev.devices[pcidev_cirrus_deviceid].header.subsysventorID = 0x0000;
			pcidev.devices[pcidev_cirrus_deviceid].header.interruptpin = 0xff;
			pcidev.devices[pcidev_cirrus_deviceid].header.interruptline = 0x00;
#if defined(SUPPORT_IA32_HAXM)
			pcidev.devices[pcidev_cirrus_deviceid].header.baseaddrregs[0] = 0xFC000000;
			pcidev.devices[pcidev_cirrus_deviceid].header.baseaddrregs[1] = 0xFE000000;
#else
			pcidev.devices[pcidev_cirrus_deviceid].header.baseaddrregs[0] = 0xF0000000;
			pcidev.devices[pcidev_cirrus_deviceid].header.baseaddrregs[1] = 0xF2000000;
#endif
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[0] = 0x02000000-1;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[1] = CIRRUS_PNPMMIO_SIZE-1;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[2] = 0xffffffff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[3] = 0xffffffff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[4] = 0xffffffff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.baseaddrregs[5] = 0xffffffff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.expROMbaseaddr = 0xffffffff;
			pc98_cirrus_vga_initVRAMWindowAddr();
            s->bustype = CIRRUS_BUSTYPE_PCI;
			
			// ROM領域設定
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.vendorID = 0xffff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.deviceID = 0xffff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.status = 0xffff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.command = 0xffdd;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.revisionID = 0xff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.classcode[0] = 0xff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.classcode[1] = 0xff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.classcode[2] = 0xff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.subsysID = 0xffff;
			pcidev.devices[pcidev_cirrus_deviceid].headerrom.subsysventorID = 0xffff;
	
			for(i=0;i<16;i++){
				iocore_attachout(0x3c0 + i, vga_ioport_write_wrap);
				iocore_attachinp(0x3c0 + i, vga_ioport_read_wrap);
			}
	
			iocore_attachout(0x3b4, vga_ioport_write_wrap);
			iocore_attachinp(0x3b4, vga_ioport_read_wrap);
			iocore_attachout(0x3b5, vga_ioport_write_wrap);
			iocore_attachinp(0x3b5, vga_ioport_read_wrap);
			iocore_attachout(0x3ba, vga_ioport_write_wrap);
			iocore_attachinp(0x3ba, vga_ioport_read_wrap);
			iocore_attachout(0x3d4, vga_ioport_write_wrap);
			iocore_attachinp(0x3d4, vga_ioport_read_wrap);
			iocore_attachout(0x3d5, vga_ioport_write_wrap);
			iocore_attachinp(0x3d5, vga_ioport_read_wrap);
			iocore_attachout(0x3da, vga_ioport_write_wrap);
			iocore_attachinp(0x3da, vga_ioport_read_wrap);
			
			pcidev_updateRoutingTable();
		} 
		if(np2clvga.gd54xxtype != CIRRUS_98ID_PCI)
#endif
		{
			iocore_attachout(0xfa2, cirrusvga_ofa2);
			iocore_attachinp(0xfa2, cirrusvga_ifa2);
	
			iocore_attachout(0xfa3, cirrusvga_ofa3);
			iocore_attachinp(0xfa3, cirrusvga_ifa3);
	
			iocore_attachout(0xfaa, cirrusvga_ofaa);
			iocore_attachinp(0xfaa, cirrusvga_ifaa);
	
			iocore_attachout(0xfab, cirrusvga_ofab);
			iocore_attachinp(0xfab, cirrusvga_ifab);
	
			if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype == CIRRUS_98ID_96){
				iocore_attachout(0x0902, cirrusvga_off82);
				iocore_attachinp(0x0902, cirrusvga_iff82);

				// XXX: 102Access Control Register 0904h は無視

				for(i=0;i<16;i++){
					iocore_attachout(0xc50 + i, vga_ioport_write_wrap);	// 0x3C0 to 0x3CF
					iocore_attachinp(0xc50 + i, vga_ioport_read_wrap);	// 0x3C0 to 0x3CF
				}
	
				//　この辺のマッピング本当にあってる？
				iocore_attachout(0xb54, vga_ioport_write_wrap);	// 0x3B4
				iocore_attachinp(0xb54, vga_ioport_read_wrap);	// 0x3B4
				iocore_attachout(0xb55, vga_ioport_write_wrap);	// 0x3B5
				iocore_attachinp(0xb55, vga_ioport_read_wrap);	// 0x3B5

				iocore_attachout(0xd54, vga_ioport_write_wrap);	// 0x3D4
				iocore_attachinp(0xd54, vga_ioport_read_wrap);	// 0x3D4
				iocore_attachout(0xd55, vga_ioport_write_wrap);	// 0x3D5
				iocore_attachinp(0xd55, vga_ioport_read_wrap);	// 0x3D5
	
				iocore_attachout(0xb5a, vga_ioport_write_wrap);	// 0x3BA
				iocore_attachinp(0xb5a, vga_ioport_read_wrap);	// 0x3BA

				iocore_attachout(0xd5a, vga_ioport_write_wrap);	// 0x3DA
				iocore_attachinp(0xd5a, vga_ioport_read_wrap);	// 0x3DA

				//iocore_attachout(0x46E8, cirrusvga_o46e8);
				//iocore_attachinp(0x46E8, cirrusvga_i46e8);
			}
			if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype != CIRRUS_98ID_96){
				iocore_attachout(0xff82, cirrusvga_off82);
				iocore_attachinp(0xff82, cirrusvga_iff82);
				
				//iocore_attachout(0x904, cirrusvga_o0904);
				//iocore_attachinp(0x904, cirrusvga_i0904);	

				for(i=0;i<16;i++){
					iocore_attachout(0xca0 + i, vga_ioport_write_wrap);	// 0x3C0 to 0x3CF
					iocore_attachinp(0xca0 + i, vga_ioport_read_wrap);	// 0x3C0 to 0x3CF
				}
	
				//　この辺のマッピング本当にあってる？
				iocore_attachout(0xba4, vga_ioport_write_wrap);	// 0x3B4
				iocore_attachinp(0xba4, vga_ioport_read_wrap);	// 0x3B4
				iocore_attachout(0xba5, vga_ioport_write_wrap);	// 0x3B5
				iocore_attachinp(0xba5, vga_ioport_read_wrap);	// 0x3B5

				iocore_attachout(0xda4, vga_ioport_write_wrap);	// 0x3D4
				iocore_attachinp(0xda4, vga_ioport_read_wrap);	// 0x3D4
				iocore_attachout(0xda5, vga_ioport_write_wrap);	// 0x3D5
				iocore_attachinp(0xda5, vga_ioport_read_wrap);	// 0x3D5
	
				iocore_attachout(0xbaa, vga_ioport_write_wrap);	// 0x3BA
				iocore_attachinp(0xbaa, vga_ioport_read_wrap);	// 0x3BA

				iocore_attachout(0xdaa, vga_ioport_write_wrap);	// 0x3DA
				iocore_attachinp(0xdaa, vga_ioport_read_wrap);	// 0x3DA
			}
		}
	}
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype > 0xff){
		// WAB, WSN, GA-98NB
		for(i=0;i<0x1000;i+=0x100){
			iocore_attachout(0x40E0 + cirrusvga_melcowab_ofs + i, vga_ioport_write_wrap);	// 0x3C0 to 0x3CF
			iocore_attachinp(0x40E0 + cirrusvga_melcowab_ofs + i, vga_ioport_read_wrap);	// 0x3C0 to 0x3CF
		}
	
		//　この辺のマッピング本当にあってる？
		//				|
		//				V
		// Odd Address:ボード制御，FM音源，PCM音源(Melco)
		// Even Address:CIRRUS (Melco,IO-Data)
		// モノクロは自信無し,WSN Win95ドライバは58e2h,59e2hをアクセスしてレジスタが
		// ffhになっている(カラーモードなので)ことを確認している
		//******************************************************
		//	port <<= 8;
		//	port &= 0x7f00; /* Mask 0111 1111 0000 0000 */
		//	port |= 0xE0;
		//******************************************************

		iocore_attachout(0x58E0 + cirrusvga_melcowab_ofs, vga_ioport_write_wrap);	// 0x3B4	これは使っている
		iocore_attachinp(0x58E0 + cirrusvga_melcowab_ofs, vga_ioport_read_wrap);	// 0x3B4	これは使っている
		iocore_attachout(0x59E0 + cirrusvga_melcowab_ofs, vga_ioport_write_wrap);	// 0x3B5	これは使っている
		iocore_attachinp(0x59E0 + cirrusvga_melcowab_ofs, vga_ioport_read_wrap);	// 0x3B5	これは使っている
		////iocore_attachout(0x3AE0 + cirrusvga_melcowab_ofs, vga_ioport_write_wrap);	// 0x3BA	使ってないからいいや
		////iocore_attachinp(0x3AE0 + cirrusvga_melcowab_ofs, vga_ioport_read_wrap);	// 0x3BA	使ってないからいいや

		iocore_attachout(0x54E0 + cirrusvga_melcowab_ofs, vga_ioport_write_wrap);	// 0x3D4
		iocore_attachinp(0x54E0 + cirrusvga_melcowab_ofs, vga_ioport_read_wrap);	// 0x3D4
		iocore_attachout(0x55E0 + cirrusvga_melcowab_ofs, vga_ioport_write_wrap);	// 0x3D5
		iocore_attachinp(0x55E0 + cirrusvga_melcowab_ofs, vga_ioport_read_wrap);	// 0x3D5
		iocore_attachout(0x5AE0 + cirrusvga_melcowab_ofs, vga_ioport_write_wrap);	// 0x3DA
		iocore_attachinp(0x5AE0 + cirrusvga_melcowab_ofs, vga_ioport_read_wrap);	// 0x3DA
		
		//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
		//	iocore_attachout(0x5BE3, cirrusvga_o5be3);
		//	iocore_attachinp(0x5BE3, cirrusvga_i5be3);
		//}

//		iocore_attachout(0x51E1 + cirrusvga_melcowab_ofs, vga_ioport_write_wrap);	// 0x3BA
//		iocore_attachinp(0x51E1 + cirrusvga_melcowab_ofs, vga_ioport_read_wrap);		// 0x3BA

//		iocore_attachout(0x57E1 + cirrusvga_melcowab_ofs, vga_ioport_write_wrap);	// 0x3DA
//		iocore_attachinp(0x57E1 + cirrusvga_melcowab_ofs, vga_ioport_read_wrap);		// 0x3DA
	
		iocore_attachout(0x40E1 + cirrusvga_melcowab_ofs, cirrusvga_o40e1);
		iocore_attachinp(0x40E1 + cirrusvga_melcowab_ofs, cirrusvga_i40e1);

		iocore_attachout(0x46E8, cirrusvga_o46e8);
		iocore_attachinp(0x46E8, cirrusvga_i46e8);

		// WSN
		if ((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F) {
			iocore_attachout(0x51E1, cirrusvga_o51e1); // CHECK IO 空振り用
			iocore_attachinp(0x51E1, cirrusvga_i51e1);
			iocore_attachout(0x51E3, cirrusvga_o51e1); // CHECK IO
			iocore_attachinp(0x51E3, cirrusvga_i51e1);

			////iocore_attachinp(0x59E0 + cirrusvga_melcowab_ofs, cirrusvga_i59e0);
			iocore_attachinp(0x59E1 + cirrusvga_melcowab_ofs, cirrusvga_i59e1);	// これがないとドライバがNGを返す
			iocore_attachinp(0x5BE1 + cirrusvga_melcowab_ofs, cirrusvga_i5be1);

			iocore_attachout(0x42E1 + cirrusvga_melcowab_ofs, cirrusvga_o42e1);
			iocore_attachinp(0x42E1 + cirrusvga_melcowab_ofs, cirrusvga_i42e1);
		}

		if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) != CIRRUS_98ID_AUTOMSK){
			np2clvga.VRAMWindowAddr2 = 0xE0000;
		}

		//// GA-98NB
		//if (np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB) {
		//	np2clvga.VRAMWindowAddr2 = 0xf00000;
		//}

		if (np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F ||
			np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE10_WSN2 || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE10_WSN4 || 
			np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WS_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_W4_PCI) {
			cirrusvga_wab_59e1 = 0x06;	// この値じゃないとWSN Win95ドライバがNGを返す
			cirrusvga_wab_51e1 = 0xC2;	// WSN CHECK IO RETURN VALUE
			cirrusvga_wab_5be1 = 0xf7;	// bit3:0=4M,1=2M ??????
			cirrusvga_wab_40e1 = 0x7b;
			cirrusvga_wab_42e1 = 0x00;
			cirrusvga_wab_46e8 = 0x18; // 最初からON
		}else if ((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC ||
			np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G1_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G2_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G4_PCI) {
			memset(cirrusvga->vram_ptr, 0x00, cirrusvga->real_vram_size);
			cirrusvga_wab_59e1 = 0x06;	// d.c.
			cirrusvga_wab_51e1 = 0xC2;	// d.c.
			cirrusvga_wab_5be1 = 0xf7;	// d.c.
			cirrusvga_wab_40e1 = 0xC2;	// bit1=0:DRAM REFRESH MODE?? とりあえず初期値はC2hじゃないとWin95ドライバはボードを認識しない
			cirrusvga_wab_42e1 = 0x18;  // 存在しない
			cirrusvga_wab_46e8 = 0x10;
		}
		
		//np2clvga.VRAMWindowAddr3 = 0xF00000; // XXX
		//np2clvga.VRAMWindowAddr3size = 256*1024;
		
		//iocore_attachout(0x52E2, cirrusvga_o52E2);
		//iocore_attachinp(0x52E2, cirrusvga_i52E2);
		//
		//iocore_attachout(0x56E2, cirrusvga_o56E2);
		//iocore_attachinp(0x56E2, cirrusvga_i56E2);
		//iocore_attachout(0x56E3, cirrusvga_o56E2);
		//iocore_attachinp(0x56E3, cirrusvga_i56E2);
		//
		//iocore_attachout(0x59E2, cirrusvga_o59E2);
		//iocore_attachinp(0x59E2, cirrusvga_i59E2);
		//iocore_attachout(0x59E3, cirrusvga_o59E2);
		//iocore_attachinp(0x59E3, cirrusvga_i59E2);
		//
		//iocore_attachout(0x58E2, cirrusvga_o5BE2);
		//iocore_attachinp(0x58E2, cirrusvga_i5BE2);
		//iocore_attachout(0x5BE2, cirrusvga_o5BE2);
		//iocore_attachinp(0x5BE2, cirrusvga_i5BE2);
	}

	pc98_cirrus_vga_setvramsize();

    s->get_bpp = cirrus_get_bpp;
    s->get_offsets = cirrus_get_offsets;
    s->get_resolution = cirrus_get_resolution;
    s->cursor_invalidate = cirrus_cursor_invalidate;
    s->cursor_draw_line = cirrus_cursor_draw_line;

	s->update_retrace_info = vga_dumb_update_retrace_info;
	s->retrace = vga_dumb_retrace;

    qemu_register_reset(cirrus_reset, s);
    cirrus_reset(s);
    cirrus_update_memory_access(s);
    //register_savevm("cirrus_vga", 0, 2, cirrus_vga_save, cirrus_vga_load, s);// XXX:
	//if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
	//	//s->sr[0x06] = 0x12; // Unlock Cirrus extensions by default
	//}
}
static void pc98_cirrus_deinit_common(CirrusVGAState * s, int device_id, int is_pci)
{
    int i;

	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype <= 0xff){
		// ONBOARD
#if defined(SUPPORT_PCI)
		if(pcidev.enable && (np2clvga.gd54xxtype == CIRRUS_98ID_PCI || 
		   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WS_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_W4_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WA_PCI || 
		   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G1_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G2_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G4_PCI)){
			// Cirrus CL-GD5446 PCI
			pcidev.devices[pcidev_cirrus_deviceid].enable = 0;
			for(i=0;i<16;i++){
				iocore_detachout(0x3c0 + i);
				iocore_detachinp(0x3c0 + i);
			}
			iocore_detachout(0x3b4);
			iocore_detachinp(0x3b4);
			iocore_detachout(0x3ba);
			iocore_detachinp(0x3ba);
			iocore_detachout(0x3d4);
			iocore_detachinp(0x3d4);
			iocore_detachout(0x3da);
			iocore_detachinp(0x3da);
		}
		if(np2clvga.gd54xxtype != CIRRUS_98ID_PCI)
#endif
		{
			iocore_detachout(0xfa2);
			iocore_detachinp(0xfa2);
	
			iocore_detachout(0xfa3);
			iocore_detachinp(0xfa3);
	
			iocore_detachout(0xfaa);
			iocore_detachinp(0xfaa);
	
			iocore_detachout(0xfab);
			iocore_detachinp(0xfab);
	
			if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype == CIRRUS_98ID_96){
				iocore_detachout(0x0902);
				iocore_detachinp(0x0902);

				// XXX: 102Access Control Register 0904h は無視

				for(i=0;i<16;i++){
					iocore_detachout(0xc50 + i);	// 0x3C0 to 0x3CF
					iocore_detachinp(0xc50 + i);	// 0x3C0 to 0x3CF
				}
	
				//　この辺のマッピング本当にあってる？
				iocore_detachout(0xb54);	// 0x3B4
				iocore_detachinp(0xb54);	// 0x3B4
				iocore_detachout(0xb55);	// 0x3B5
				iocore_detachinp(0xb55);	// 0x3B5

				iocore_detachout(0xd54);	// 0x3D4
				iocore_detachinp(0xd54);	// 0x3D4
				iocore_detachout(0xd55);	// 0x3D5
				iocore_detachinp(0xd55);	// 0x3D5
	
				iocore_detachout(0xb5a);	// 0x3BA
				iocore_detachinp(0xb5a);	// 0x3BA

				iocore_detachout(0xd5a);	// 0x3DA
				iocore_detachinp(0xd5a);	// 0x3DA
			}
			if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype != CIRRUS_98ID_96){
				iocore_detachout(0xff82);
				iocore_detachinp(0xff82);

				for(i=0;i<16;i++){
					iocore_detachout(0xca0 + i);	// 0x3C0 to 0x3CF
					iocore_detachinp(0xca0 + i);	// 0x3C0 to 0x3CF
				}
	
				//　この辺のマッピング本当にあってる？
				iocore_detachout(0xba4);	// 0x3B4
				iocore_detachinp(0xba4);	// 0x3B4
				iocore_detachout(0xba5);	// 0x3B5
				iocore_detachinp(0xba5);	// 0x3B5

				iocore_detachout(0xda4);	// 0x3D4
				iocore_detachinp(0xda4);	// 0x3D4
				iocore_detachout(0xda5);	// 0x3D5
				iocore_detachinp(0xda5);	// 0x3D5
	
				iocore_detachout(0xbaa);	// 0x3BA
				iocore_detachinp(0xbaa);	// 0x3BA

				iocore_detachout(0xdaa);	// 0x3DA
				iocore_detachinp(0xdaa);	// 0x3DA

#ifdef SUPPORT_VGA_MODEX
				if(np2cfg.usemodex){
					for(i=0;i<16;i++){
						iocore_attachout(0x3c0 + i, vga_ioport_write_wrap);
						iocore_attachinp(0x3c0 + i, vga_ioport_read_wrap);
					}
	
					iocore_attachout(0x3b4, vga_ioport_write_wrap);
					iocore_attachinp(0x3b4, vga_ioport_read_wrap);
					iocore_attachout(0x3b5, vga_ioport_write_wrap);
					iocore_attachinp(0x3b5, vga_ioport_read_wrap);
					iocore_attachout(0x3ba, vga_ioport_write_wrap);
					iocore_attachinp(0x3ba, vga_ioport_read_wrap);
					iocore_attachout(0x3d4, vga_ioport_write_wrap);
					iocore_attachinp(0x3d4, vga_ioport_read_wrap);
					iocore_attachout(0x3d5, vga_ioport_write_wrap);
					iocore_attachinp(0x3d5, vga_ioport_read_wrap);
					iocore_attachout(0x3da, vga_ioport_write_wrap);
					iocore_attachinp(0x3da, vga_ioport_read_wrap);
				}
#endif
			}
		}
	}
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype > 0xff){
		// WAB, WSN, GA-98NB
		for(i=0;i<0x1000;i+=0x100){
			iocore_detachout(0x40E0 + cirrusvga_melcowab_ofs + i);	// 0x3C0 to 0x3CF
			iocore_detachinp(0x40E0 + cirrusvga_melcowab_ofs + i);	// 0x3C0 to 0x3CF
		}
	
		//　この辺のマッピング本当にあってる？
		iocore_detachout(0x58E0 + cirrusvga_melcowab_ofs);	// 0x3B4
		iocore_detachinp(0x58E0 + cirrusvga_melcowab_ofs);		// 0x3B4
		iocore_detachout(0x59E0 + cirrusvga_melcowab_ofs);	// 0x3B5
		iocore_detachinp(0x59E0 + cirrusvga_melcowab_ofs);		// 0x3B5
		////iocore_detachout(0x3AE0 + cirrusvga_melcowab_ofs);	// 0x3BA
		////iocore_detachinp(0x3AE0 + cirrusvga_melcowab_ofs);		// 0x3BA

		iocore_detachout(0x54E0 + cirrusvga_melcowab_ofs);	// 0x3D4
		iocore_detachinp(0x54E0 + cirrusvga_melcowab_ofs);		// 0x3D4
		iocore_detachout(0x55E0 + cirrusvga_melcowab_ofs);	// 0x3D5
		iocore_detachinp(0x55E0 + cirrusvga_melcowab_ofs);		// 0x3D5
		iocore_detachout(0x5AE0 + cirrusvga_melcowab_ofs);	// 0x3DA
		iocore_detachinp(0x5AE0 + cirrusvga_melcowab_ofs);		// 0x3DA

//		iocore_detachout(0x51E1 + cirrusvga_melcowab_ofs);	// 0x3BA
//		iocore_detachinp(0x51E1 + cirrusvga_melcowab_ofs);		// 0x3BA

//		iocore_detachout(0x57E1 + cirrusvga_melcowab_ofs);	// 0x3DA
//		iocore_detachinp(0x57E1 + cirrusvga_melcowab_ofs);		// 0x3DA
	
		iocore_detachout(0x40E1 + cirrusvga_melcowab_ofs);
		iocore_detachinp(0x40E1 + cirrusvga_melcowab_ofs);
	
		iocore_detachout(0x46E8);
		iocore_detachinp(0x46E8);

		iocore_detachout(0x51E1);
		iocore_detachinp(0x51E1);
		iocore_detachout(0x51E3);
		iocore_detachinp(0x51E3);

		////iocore_detachinp(0x59E0 + cirrusvga_melcowab_ofs);
		iocore_detachinp(0x59E1 + cirrusvga_melcowab_ofs);
		iocore_detachinp(0x5BE1 + cirrusvga_melcowab_ofs);

		iocore_detachinp(0x42E1 + cirrusvga_melcowab_ofs);
		iocore_detachout(0x42E1 + cirrusvga_melcowab_ofs);
	}
}


void pc98_cirrus_vga_init(void)
{
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	HDC hdc;
	UINT i;
	WORD* PalIndexes;
    //CirrusVGAState *s;
	//HBITMAP hbmp;
	//BOOL b;

	ga_bmpInfo = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO)+sizeof(WORD)*256);	
	PalIndexes = (WORD*)((char*)ga_bmpInfo + sizeof(BITMAPINFOHEADER));
	for (i = 0; i < 256; ++i) PalIndexes[i] = i;
	
	ga_bmpInfo_cursor = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO));	
	
#if defined(SUPPORT_IA32_HAXM)
	vramptr = (uint8_t*)_aligned_malloc(CIRRUS_VRAM_SIZE*2, 4096); // 2倍取っておく
#else
	vramptr = (uint8_t*)malloc(CIRRUS_VRAM_SIZE*2); // 2倍取っておく
#endif
	
	ga_bmpInfo_cursor->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ga_bmpInfo_cursor->bmiHeader.biPlanes = 1;
	ga_bmpInfo_cursor->bmiHeader.biBitCount = 32;
	ga_bmpInfo_cursor->bmiHeader.biCompression = BI_RGB;
	ga_bmpInfo_cursor->bmiHeader.biWidth = 64;
	ga_bmpInfo_cursor->bmiHeader.biHeight = 64;
	ga_hbmp_cursor = CreateDIBSection(NULL, ga_bmpInfo_cursor, DIB_RGB_COLORS, (void **)(&cursorptr), NULL, 0);
	hdc = GetDC(NULL);
	ga_hdc_cursor = CreateCompatibleDC(hdc);
	ReleaseDC(NULL, hdc);
	SelectObject(ga_hdc_cursor, ga_hbmp_cursor);
	
	ds.surface = &np2vga_ds_surface;
	ds.listeners = &np2vga_ds_listeners;
	ds.mouse_set = np2vga_ds_mouse_set;
	ds.cursor_define = np2vga_ds_cursor_define;
	ds.next = NULL;

	ga_hFakeCursor = LoadCursor(NULL, IDC_ARROW);

#else
	vramptr = (uint8_t*)malloc(CIRRUS_VRAM_SIZE*2); // 2倍取っておく

	ds.surface = &np2vga_ds_surface;
	ds.listeners = &np2vga_ds_listeners;
	ds.mouse_set = np2vga_ds_mouse_set;
	ds.cursor_define = np2vga_ds_cursor_define;
	ds.next = NULL;
#endif

	if(cirrusvga_opaque){
		free(cirrusvga_opaque);
		cirrusvga_opaque = cirrusvga = NULL;
	}

	cirrusvga_opaque = cirrusvga = (CirrusVGAState*)calloc(1, sizeof(CirrusVGAState));
}
void pc98_cirrus_vga_reset(const NP2CFG *pConfig)
{
    CirrusVGAState *s;

	np2clvga.enabled = np2cfg.usegd5430;
	if(!np2clvga.enabled){
		TRACEOUT(("CL-GD54xx: Window Accelerator Disabled"));
		return;
	}
	
#if defined(SUPPORT_IA32_HAXM)
	if(!np2haxcore.allocwabmem){
		i386hax_vm_allocmemoryex(vramptr, CIRRUS_VRAM_SIZE*2);
		np2haxcore.allocwabmem = 1;
	}
#endif
	
#if defined(SUPPORT_VGA_MODEX)
	np2clvga.modex = 0;
#endif
	np2clvga.defgd54xxtype = np2cfg.gd5430type;
	np2clvga.gd54xxtype = np2cfg.gd5430type;
	//np2clvga.defgd54xxtype = CIRRUS_98ID_PCI;
	//np2clvga.gd54xxtype = CIRRUS_98ID_PCI;
	
	s = cirrusvga;
	//memset(s, 0, sizeof(CirrusVGAState));
	if(np2clvga.gd54xxtype <= 0x57){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5446, 0);
	}else if(np2clvga.gd54xxtype <= 0xff){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5434, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5426, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5434, 0);
	}else if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5434, 0);
	}else{
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5430, 0);
	}
}
void pc98_cirrus_vga_bind(void)
{
    CirrusVGAState *s;
	if(!np2clvga.enabled){
		TRACEOUT(("CL-GD54xx: Window Accelerator Disabled"));
		return;
	}
	if (np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F || 
		np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE10_WSN2 || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE10_WSN4 || 
		np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WS_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_W4_PCI) {
		cirrusvga_melcowab_ofs = 0x2; // WSNだけ2で固定
	}
	else {
		cirrusvga_melcowab_ofs = np2cfg.gd5430melofs;	// WSN以外は自由選択可
	}
	
	s = cirrusvga;
	//memset(s, 0, sizeof(CirrusVGAState));
	if(np2clvga.gd54xxtype <= 0x57){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5446, 0);
	}else if(np2clvga.gd54xxtype <= 0xff){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5430, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5426, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5434, 0);
	}else if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5434, 0);
	}else{
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5430, 0);
	}
	s->ds = graphic_console_init(s->update, s->invalidate, s->screen_dump, s->text_update, s);
	
	np2wabwnd.drawframe = cirrusvga_drawGraphic;

#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	ga_bmpInfo->bmiHeader.biWidth = 0;
	ga_bmpInfo->bmiHeader.biHeight = 0;
#endif

	TRACEOUT(("CL-GD54xx: Window Accelerator Enabled"));
}
void pc98_cirrus_vga_unbind(void)
{
    CirrusVGAState *s;

	s = cirrusvga;
	if(np2clvga.gd54xxtype <= 0x57){
		pc98_cirrus_deinit_common(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		pc98_cirrus_deinit_common(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_PCI){
		pc98_cirrus_deinit_common(s, CIRRUS_ID_CLGD5446, 0);
	}else if(np2clvga.gd54xxtype <= 0xff){
		pc98_cirrus_deinit_common(s, CIRRUS_ID_CLGD5430, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		pc98_cirrus_deinit_common(s, CIRRUS_ID_CLGD5426, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		pc98_cirrus_deinit_common(s, CIRRUS_ID_CLGD5434, 0);
	}else if((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
		pc98_cirrus_deinit_common(s, CIRRUS_ID_CLGD5434, 0);
	}else{
		pc98_cirrus_deinit_common(s, CIRRUS_ID_CLGD5430, 0);
	}
}

void pc98_cirrus_vga_shutdown(void)
{
	np2wabwnd.drawframe = NULL;
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	free(ga_bmpInfo_cursor);
	free(ga_bmpInfo);
#endif
#if defined(SUPPORT_IA32_HAXM)
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	_aligned_free(vramptr);
#else
	free(vramptr);
#endif
#else
	free(vramptr);
#endif
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
	DeleteDC(ga_hdc_cursor);
	DeleteObject(ga_hbmp_cursor);
#endif
	if(cirrusvga_opaque){
		free(cirrusvga_opaque);
		cirrusvga_opaque = cirrusvga = NULL;
	}
	//free(cursorptr);
}

void pc98_cirrus_vga_resetresolution(void)
{
	if(!np2clvga.enabled) return;

	cirrusvga->cr[0x01] = 0;
	cirrusvga->cr[0x12] = 0;
	cirrusvga->cr[0x07] &= ~0x42;
	// ついでにVRAMもクリア
//	if ((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC || np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F) {
	if (np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F) {
		memset(cirrusvga->vram_ptr, 0x00, cirrusvga->real_vram_size);
		cirrusvga_wab_59e1 = 0x06;	// この値じゃないとWSN Win95ドライバがNGを返す
		cirrusvga_wab_51e1 = 0xC2;	// WSN CHECK IO RETURN VALUE
		cirrusvga_wab_5be1 = 0xf7;	// bit3:0=4M,1=2M ??????
		cirrusvga_wab_40e1 = 0x7b;
		cirrusvga_wab_42e1 = 0x00;
		cirrusvga_wab_46e8 = 0x18; // 最初からON
	}else if ((np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC) {
		memset(cirrusvga->vram_ptr, 0x00, cirrusvga->real_vram_size);
		cirrusvga_wab_59e1 = 0x06;	// d.c.
		cirrusvga_wab_51e1 = 0xC2;	// d.c.
		cirrusvga_wab_5be1 = 0xf7;	// d.c.
		cirrusvga_wab_40e1 = 0xC2;	// bit1=0:DRAM REFRESH MODE?? とりあえず初期値はC2hじゃないとWin95ドライバはボードを認識しない
		cirrusvga_wab_42e1 = 0x18;  // 存在しない
		cirrusvga_wab_46e8 = 0x10;
	}else{
		memset(cirrusvga->vram_ptr, 0x00, cirrusvga->real_vram_size);
		cirrusvga_wab_46e8 = 0x18;
	}
#if defined(SUPPORT_PCI)
	// XXX: Win2000で動かすのに必要。理由は謎
	if(pcidev.enable && (np2clvga.gd54xxtype == CIRRUS_98ID_PCI || 
	   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WS_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_W4_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_WA_PCI || 
	   np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G1_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G2_PCI || np2clvga.gd54xxtype == CIRRUS_98ID_AUTO_XE_G4_PCI)){
		cirrusvga->msr = 0x03;
		cirrusvga->sr[0x08] = 0xFE;
	}
#endif
}

// MELCO WAB系ポートならTRUE
int pc98_cirrus_isWABport(UINT port){
	if((port & 0xF0FF) == (0x40E0 + cirrusvga_melcowab_ofs)) return 1;
 	if (port == 0x58E0 + cirrusvga_melcowab_ofs) return 1;
 	if (port == 0x59E0 + cirrusvga_melcowab_ofs) return 1;
 	////if (port == 0x3AE0 + cirrusvga_melcowab_ofs) return 1;
 	if (port == 0x54E0 + cirrusvga_melcowab_ofs) return 1;
 	if (port == 0x55E0 + cirrusvga_melcowab_ofs) return 1;
 	if (port == 0x5AE0 + cirrusvga_melcowab_ofs) return 1;
	return 0;
}

// MELCO WAB / I-O DATA GA-98NB　自動選択　レジスタ設定
void pc98_cirrus_setWABreg(){
    CirrusVGAState *s;

	s = cirrusvga;
	switch(np2clvga.gd54xxtype){
	case CIRRUS_98ID_WAB:
		s->device_id = CIRRUS_ID_CLGD5426;
		s->bustype = CIRRUS_BUSTYPE_ISA;
		s->cr[0x27] = s->device_id;
        s->sr[0x0F] = CIRRUS_MEMSIZE_1M;
        s->sr[0x15] = 0x02;
		break;
	case CIRRUS_98ID_WSN_A2F:
		s->device_id = CIRRUS_ID_CLGD5434;
		s->bustype = CIRRUS_BUSTYPE_ISA;
		s->cr[0x27] = s->device_id;
		//s->sr[0x1F] = 0x2d;		// MemClock
		//s->gr[0x18] = 0x0f;             // fastest memory configuration
		s->sr[0x0f] = CIRRUS_MEMSIZE_2M;
		//s->sr[0x17] = 0x20;
		s->sr[0x15] = 0x03; /* memory size, 3=2MB, 4=4MB */
		//s->sr[0x0F] = 0x20;
		break;
	case CIRRUS_98ID_WSN: // WSN-A4F
		s->device_id = CIRRUS_ID_CLGD5434;
		s->bustype = CIRRUS_BUSTYPE_ISA;
		s->cr[0x27] = s->device_id;
        //s->sr[0x1F] = 0x2d;		// MemClock
        //s->gr[0x18] = 0x0f;             // fastest memory configuration
        s->sr[0x0f] = CIRRUS_MEMSIZE_2M | CIRRUS_MEMFLAGS_BANKSWITCH;
        //s->sr[0x17] = 0x20;
        s->sr[0x15] = 0x04; /* memory size, 3=2MB, 4=4MB */
        //s->sr[0x0F] = 0x20;
		break;

	case CIRRUS_98ID_GA98NBIC:
		s->device_id = CIRRUS_ID_CLGD5434;
		s->bustype = CIRRUS_BUSTYPE_ISA;
		s->cr[0x27] = s->device_id;
        s->sr[0x0f] = CIRRUS_MEMSIZE_1M;
        s->sr[0x15] = 0x02;
		break;
	case CIRRUS_98ID_GA98NBII:
		s->device_id = CIRRUS_ID_CLGD5434;
		s->bustype = CIRRUS_BUSTYPE_ISA;
		s->cr[0x27] = s->device_id;
        s->sr[0x0f] = CIRRUS_MEMSIZE_2M;
        s->sr[0x15] = 0x03; /* memory size, 3=2MB, 4=4MB */
		break;
	case CIRRUS_98ID_GA98NBIV:
		s->device_id = CIRRUS_ID_CLGD5434;
		s->bustype = CIRRUS_BUSTYPE_ISA;
		s->cr[0x27] = s->device_id;
        s->sr[0x0f] = CIRRUS_MEMSIZE_2M | CIRRUS_MEMFLAGS_BANKSWITCH;
        s->sr[0x15] = 0x04; /* memory size, 3=2MB, 4=4MB */
		break;
	}
}

#endif
