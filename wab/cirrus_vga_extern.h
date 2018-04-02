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

#pragma once

#define CIRRUS_VRAM_SIZE_1MB	(1024 * 1024)
#define CIRRUS_VRAM_SIZE_2MB	(2048 * 1024)
#define CIRRUS_VRAM_SIZE_4MB	(4096 * 1024)
#define CIRRUS_VRAM_SIZE		CIRRUS_VRAM_SIZE_4MB
#define CIRRUS_VRAM_SIZE_WAB	CIRRUS_VRAM_SIZE_1MB

#define CIRRUS_98ID_96		0x60
#define CIRRUS_98ID_Be		0x50
#define CIRRUS_98ID_Xe		0x58
#define CIRRUS_98ID_Cb		0x59
#define CIRRUS_98ID_Cf		0x5A
#define CIRRUS_98ID_Xe10	0x5B
#define CIRRUS_98ID_Cb2		0x5C
#define CIRRUS_98ID_Cx2		0x5D
#define CIRRUS_98ID_WAB		0x100
#define CIRRUS_98ID_WSN_A2F	0x101
#define CIRRUS_98ID_WSN		0x102
#define CIRRUS_98ID_GA98NB	0x200
#define CIRRUS_98ID_AUTOMSK	0xFFF0
#define CIRRUS_98ID_AUTO_XE10_WABS	0xFFFD
#define CIRRUS_98ID_AUTO_XE10_WSN2	0xFFFE
#define CIRRUS_98ID_AUTO_XE10_WSN4	0xFFFF

#define VRAMWINDOW_SIZE	0x200000UL  // VRAM マッピングサイズ
#define EXT_WINDOW_SIZE	0x200000UL  // 謎
#define EXT_WINDOW_SHFT	0x000000UL  // 謎
#define BBLTWINDOW_ADSH	0x200000UL // VRAM BITBLT
#define BBLTWINDOW_SIZE	0x000000UL  // VRAM BITBLT マッピングサイズ
#define MMIOWINDOW_ADDR	0xF80000UL  // MMIO マッピングアドレス（場所不明）
#define MMIOWINDOW_SIZE	0x000000UL   // MMIO マッピングサイズ（サイズ不明）
#define VRA2WINDOW_ADDR	0x0F2000UL  // VRAMウィンドウ マッピングアドレス（場所不明）
#define VRA2WINDOW_SIZE	0x000000UL   // VRAMウィンドウ マッピングサイズ（サイズ不明）
#define VRA2WINDOW_SIZEX  0x8000UL   // VRAMウィンドウ マッピングサイズ（サイズ不明）
#define VRA3WINDOW_SIZEX  0x20000UL  // VRAMウィンドウ F00000
#define CIRRUS_VRAMWND2_FUNC_rb(a,b)	cirrus_linear_memwnd_readb(a,b)
#define CIRRUS_VRAMWND2_FUNC_rw(a,b)	cirrus_linear_memwnd_readw(a,b)
#define CIRRUS_VRAMWND2_FUNC_rl(a,b)	cirrus_linear_memwnd_readl(a,b)
#define CIRRUS_VRAMWND2_FUNC_wb(a,b,c)	cirrus_linear_memwnd_writeb(a,b,c)
#define CIRRUS_VRAMWND2_FUNC_ww(a,b,c)	cirrus_linear_memwnd_writew(a,b,c)
#define CIRRUS_VRAMWND2_FUNC_wl(a,b,c)	cirrus_linear_memwnd_writel(a,b,c)
#define CIRRUS_VRAMWND3_FUNC_rb(a,b)	cirrus_linear_memwnd3_readb(a,b)
#define CIRRUS_VRAMWND3_FUNC_rw(a,b)	cirrus_linear_memwnd3_readw(a,b)
#define CIRRUS_VRAMWND3_FUNC_rl(a,b)	cirrus_linear_memwnd3_readl(a,b)
#define CIRRUS_VRAMWND3_FUNC_wb(a,b,c)	cirrus_linear_memwnd3_writeb(a,b,c)
#define CIRRUS_VRAMWND3_FUNC_ww(a,b,c)	cirrus_linear_memwnd3_writew(a,b,c)
#define CIRRUS_VRAMWND3_FUNC_wl(a,b,c)	cirrus_linear_memwnd3_writel(a,b,c)

#define CIRRUS_VRAMWINDOW2MASK	(~((np2clvga.gd54xxtype==CIRRUS_98ID_96||np2clvga.gd54xxtype==CIRRUS_98ID_Be ? VRA2WINDOW_SIZEX*2 : VRA2WINDOW_SIZEX)-1))

#define TEST_ADDR		0xF0000000
#define TEST_ADDR_SIZE	0//0x8000

#define CIRRUS_MELCOWAB_OFS	0x2

typedef	signed char		int8_t;
typedef	unsigned char	uint8_t;
typedef	signed short	int16_t;
typedef	unsigned short	uint16_t_;
typedef	signed int		int32_t;
typedef	unsigned int	uint32_t_;
#if !defined(NP2_X11) && !defined(NP2_SDL2) && !defined(__LIBRETRO__)
typedef	signed __int64	int64_t;
typedef	unsigned __int64	uint64_t;
#endif

typedef uint32_t_ ram_addr_t;
typedef uint32_t_ target_phys_addr_t;

typedef void CPUWriteMemoryFunc(void *opaque, target_phys_addr_t addr, uint32_t_ value);
typedef uint32_t_ CPUReadMemoryFunc(void *opaque, target_phys_addr_t addr);

extern CPUWriteMemoryFunc *g_cirrus_linear_write[3];

uint32_t_ cirrus_vga_mem_readb(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_vga_mem_readw(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_vga_mem_readl(void *opaque, target_phys_addr_t addr);
void cirrus_vga_mem_writeb(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_vga_mem_writew(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_vga_mem_writel(void *opaque, target_phys_addr_t addr, uint32_t_ val);

uint32_t_ cirrus_linear_readb(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_linear_readw(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_linear_readl(void *opaque, target_phys_addr_t addr);
void cirrus_linear_writeb(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_linear_writew(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_linear_writel(void *opaque, target_phys_addr_t addr, uint32_t_ val);

uint32_t_ cirrus_linear_bitblt_readb(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_linear_bitblt_readw(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_linear_bitblt_readl(void *opaque, target_phys_addr_t addr);
void cirrus_linear_bitblt_writeb(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_linear_bitblt_writew(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_linear_bitblt_writel(void *opaque, target_phys_addr_t addr, uint32_t_ val);

uint32_t_ cirrus_linear_memwnd_readb(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_linear_memwnd_readw(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_linear_memwnd_readl(void *opaque, target_phys_addr_t addr);
void cirrus_linear_memwnd_writeb(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_linear_memwnd_writew(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_linear_memwnd_writel(void *opaque, target_phys_addr_t addr, uint32_t_ val);

uint32_t_ cirrus_linear_memwnd3_readb(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_linear_memwnd3_readw(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_linear_memwnd3_readl(void *opaque, target_phys_addr_t addr);
void cirrus_linear_memwnd3_writeb(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_linear_memwnd3_writew(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_linear_memwnd3_writel(void *opaque, target_phys_addr_t addr, uint32_t_ val);

uint32_t_ cirrus_mmio_readb(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_mmio_readw(void *opaque, target_phys_addr_t addr);
uint32_t_ cirrus_mmio_readl(void *opaque, target_phys_addr_t addr);
void cirrus_mmio_writeb(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_mmio_writew(void *opaque, target_phys_addr_t addr, uint32_t_ val);
void cirrus_mmio_writel(void *opaque, target_phys_addr_t addr, uint32_t_ val);

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	UINT8	enabled;
	UINT32	VRAMWindowAddr;
	UINT32	VRAMWindowAddr2;
	UINT32	VRAMWindowAddr3;
	//UINT32	VRAMWindowAddr3;
	//UINT32	VRAMWindowAddr3size;
	REG8	mmioenable;
	UINT32	gd54xxtype;
	UINT32	defgd54xxtype;
} NP2CLVGA;
//typedef struct {
//} NP2CLVGA2;


extern UINT8		cirrusvga_statsavebuf[CIRRUS_VRAM_SIZE + 1024 * 1024];

extern void			*cirrusvga_opaque;
extern NP2CLVGA		np2clvga;
//extern NP2CLVGA2	np2clvga;
	
void cirrusvga_drawGraphic();

// 無理矢理外から呼べるように
void pc98_cirrus_vga_init(void);
void pc98_cirrus_vga_reset(const NP2CFG *pConfig);
void pc98_cirrus_vga_bind(void);
void pc98_cirrus_vga_shutdown(void);

void pc98_cirrus_vga_save(void);
void pc98_cirrus_vga_load(void);

#if defined(NP2_X11) || defined(NP2_SDL2) || defined(__LIBRETRO__)
#define __fastcall
#endif
UINT16 __fastcall cirrusvga_ioport_read_wrap16(UINT addr);
UINT32 __fastcall cirrusvga_ioport_read_wrap32(UINT addr);
void __fastcall cirrusvga_ioport_write_wrap16(UINT addr, UINT16 dat);
void __fastcall cirrusvga_ioport_write_wrap32(UINT addr, UINT32 dat);

int pc98_cirrus_isWABport(UINT port);

#ifdef __cplusplus
}
#endif

