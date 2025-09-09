/**
 * @file	bios.c
 * @brief	Implementation of BIOS
 */

#include <compiler.h>
#include <bios/bios.h>
#include <bios/biosmem.h>
#include <bios/sxsibios.h>
#include <common/strres.h>
#include <cpucore.h>
#include <pccore.h>
#include <io/iocore.h>
#include "lio/lio.h"
#include <vram/vram.h>
#include <diskimage/fddfile.h>
#include <fdd/fdd_mtr.h>
#include "fdfmt.h"
#include <dosio.h>
#include "keytable.res"
#include "itfrom.res"
#include "startup.res"
#if defined(SUPPORT_IA32_HAXM)
#include "biosfd80_hax.res"
#else
#include "biosfd80.res"
#endif
#if defined(SUPPORT_IDEIO)
#include	<fdd/sxsi.h>
#include	<cbus/ideio.h>
#endif
#if defined(SUPPORT_HRTIMER)
#include	<timemng.h>
#endif
#include	<sound/fmboard.h>

#if defined(SUPPORT_VGA_MODEX)
#if defined(SUPPORT_WAB)
#include	<wab/wab.h>
#endif
#if defined(SUPPORT_CL_GD5430)
#include	<wab/cirrus_vga_extern.h>
#endif
#endif

#if defined(SUPPORT_IA32_HAXM)
#include	<i386hax/haxfunc.h>
#include	<i386hax/haxcore.h>
#define USE_CUSTOM_HOOKINST
#endif

#if 0
#undef	TRACEOUT
#define USE_TRACEOUT_VS
#ifdef USE_TRACEOUT_VS
static void trace_fmt_ex(const char *fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "¥n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
#else
#define	TRACEOUT(s)	(void)(s)
#endif
#endif	/* 1 */

#ifdef USE_CUSTOM_HOOKINST
#define BIOS_HOOKINST	bioshookinfo.hookinst
#else
#define BIOS_HOOKINST	0x90	// NOP命令（固定）
#endif

#define	BIOS_SIMULATE

static const char neccheck[] = "Copyright (C) 1983 by NEC Corporation";

typedef struct {
	UINT8	port;
	UINT8	data;
} IODATA;

static const IODATA iodata[] = {
			// DMA
				{0x29, 0x00}, {0x29, 0x01}, {0x29, 0x02}, {0x29, 0x03},
				{0x27, 0x00}, {0x21, 0x00}, {0x23, 0x00}, {0x25, 0x00},
				{0x1b, 0x00}, {0x11, 0x40},

			// PIT
				{0x77, 0x30}, {0x71, 0x00}, {0x71, 0x00},
				{0x77, 0x76}, {0x73, 0xcd}, {0x73, 0x04},
				{0x77, 0xb6},

			// PIC
				{0x00, 0x11}, {0x02, 0x08}, {0x02, 0x80}, {0x02, 0x1d},
				{0x08, 0x11}, {0x0a, 0x10}, {0x0a, 0x07}, {0x0a, 0x09},
				{0x02, 0x7d}, {0x0a, 0x71}};

static const UINT8 msw_default[8] =
							{0x48, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x6e};

#ifdef USE_CUSTOM_HOOKINST
BIOSHOOKINFO	bioshookinfo;
#endif
#if defined(BIOS_IO_EMULATION)
BIOSIOEMU	biosioemu; // np21w ver0.86 rev46 BIOS I/O emulation
#endif

#ifdef USE_CUSTOM_HOOKINST
// BIOSフック命令をデフォルト設定（NOP命令 0x90）から書き換える
static void bios_updatehookinst(UINT8 *mem, UINT32 updatesize) {

	UINT32 i;

	if(bioshookinfo.hookinst == HOOKINST_DEFAULT) return; // 書き換え不要

	// XXX: 命令をちゃんと見て書き換えないとやばい
	for(i=0;i<updatesize-1;i++){
		if(*mem == HOOKINST_DEFAULT){
			// 次の命令がNOP, STI, RET, IRETっぽければ書き換え（間違って書き換えるのを回避）
			if(*(mem+1) == 0x90 || *(mem+1) == 0xFB || *(mem+1) == 0xC2 || *(mem+1) == 0xC3 || *(mem+1) == 0xCB || *(mem+1) == 0xCA || *(mem+1) == 0xCF || *(mem+1) == 0xE8 || *(mem+1) == 0xE9 || *(mem+1) == 0xEE){
				*mem = bioshookinfo.hookinst;
			}else if(*(mem+1) == 0x51 && *(mem+2) == 0xB9){
				*mem = bioshookinfo.hookinst;
			}else if(*(mem+1) == 0xec && *(mem+2) == 0x3c){
				*mem = bioshookinfo.hookinst;
			}else{
				*mem = 0x90;
			}
		}
		mem++;
	}
}
#endif

//             DA/UA = 80h,00h 81h,01h 82h,01h 83h,01h
int sxsi_unittbl[4] = {0,      1,      2,      3}; // DA/UAをインデックスに変換する

#define SXSI_WORKAROUND_BOOTWAIT	150
int sxsi_workaround_bootwait = 0;

static void bios_itfprepare(void) {

const IODATA	*p;
const IODATA	*pterm;

	crtc_biosreset();
	gdc_biosreset();

	p = iodata;
	pterm = iodata + NELEMENTS(iodata);
	while(p < pterm) {
		iocore_out8(p->port, p->data);
		p++;
	}
}

static void bios_memclear(void) {

	ZeroMemory(mem, 0xa0000);
	ZeroMemory(mem + 0x100000, 0x10000);
	if (CPU_EXTMEM) {
		ZeroMemory(CPU_EXTMEM, CPU_EXTMEMSIZE);
	}
	bios0x18_16(0x20, 0xe1);
	ZeroMemory(mem + VRAM0_B, 0x18000);
	ZeroMemory(mem + VRAM0_E, 0x08000);
	ZeroMemory(mem + VRAM1_B, 0x18000);
	ZeroMemory(mem + VRAM1_E, 0x08000);
#if defined(SUPPORT_PC9821)
	ZeroMemory(vramex, sizeof(vramex));
#endif
}

static void bios_reinitbyswitch(void) {

	UINT8	prxcrt;
	UINT8	prxdupd;
	UINT8	biosflag;
	UINT16	extmem; // LARGE_MEM //UINT16	extmem;
	UINT8	boot;
	int		i;
#if defined(SUPPORT_IDEIO)
	int		idx, ncidx;
#endif
	//FILEH	fh;
	//OEMCHAR	path[MAX_PATH];

	if (!(pccore.dipsw[2] & 0x80)) {
#if defined(CPUCORE_IA32)
		mem[MEMB_SYS_TYPE] = 0x03;		// 80386〜
#else
		mem[MEMB_SYS_TYPE] = 0x01;		// 80286
#endif
	}
	else {
		mem[MEMB_SYS_TYPE] = 0x00;		// V30
	}

	mem[MEMB_BIOS_FLAG0] = 0x01;
	prxcrt = 0x08;
	if (!(pccore.dipsw[0] & 0x01)) {			// dipsw1-1 on
		prxcrt |= 0x40;
	}
	if (gdc.display & (1 << GDCDISP_ANALOG)) {
		prxcrt |= 0x04;							// color16
	}
	if (!(pccore.dipsw[0] & 0x80)) {			// dipsw1-8 on
		prxcrt |= 0x01;
	}
	if (grcg.chip) {
		prxcrt |= 0x02;
	}
	mem[MEMB_PRXCRT] = prxcrt;

	prxdupd = 0x18;
	if (grcg.chip >= 3) {
		prxdupd |= 0x40;
	}
	if (!(pccore.dipsw[1] & 0x80)) {			// dipsw2-8 on
		prxdupd |= 0x20;
	}
	mem[MEMB_PRXDUPD] = prxdupd;

	biosflag = 0x20;
	if (pccore.cpumode & CPUMODE_8MHZ) {
		biosflag |= 0x80;
	}
	biosflag |= mem[0xa3fea] & 7;
	if (pccore.dipsw[2] & 0x80) {
		biosflag |= 0x40;
	}
	mem[MEMB_BIOS_FLAG1] = biosflag;
	extmem = pccore.extmem;
	extmem = MIN(extmem, 14);
	mem[MEMB_EXPMMSZ] = (UINT8)(extmem << 3);
	if (pccore.extmem >= 15) {
		//mem[0x0594] = pccore.extmem - 15;
		STOREINTELWORD((mem+0x0594), (pccore.extmem - 15));
	}
	mem[MEMB_CRT_RASTER] = 0x0f;

	// FDD initialize
	SETBIOSMEM32(MEMD_F2DD_POINTER, 0xfd801ad7);
	SETBIOSMEM32(MEMD_F2HD_POINTER, 0xfd801aaf);
	boot = mem[MEMB_MSW5] & 0xf0;
	if (boot != 0x20) {		// 1MB
		fddbios_equip(3, TRUE);
		mem[MEMB_BIOS_FLAG0] |= 0x02;
	}
	else {					// 640KB
		fddbios_equip(0, TRUE);
		mem[MEMB_BIOS_FLAG0] &= ~0x02;
	}
	mem[MEMB_F2HD_MODE] = 0xff;
	mem[MEMB_F2DD_MODE] = 0xff;

#if defined(SUPPORT_CRT31KHZ)
	mem[MEMB_CRT_BIOS] |= 0x80;
#endif
#if defined(SUPPORT_PC9821)
	mem[MEMB_CRT_BIOS] |= 0x04;		// 05/02/03
	mem[0x45c] = 0x40;
	
	// DA/UAと要素番号の対応関係を初期化
	for(i=0;i<4;i++){
		sxsi_unittbl[i] = i;
	}
#if defined(SUPPORT_IDEIO)
	if (pccore.hddif & PCHDD_IDE) {
		int compmode = (sxsi_getdevtype(0)!=SXSIDEV_CDROM && sxsi_getdevtype(1)!=SXSIDEV_CDROM && sxsi_getdevtype(2)==SXSIDEV_CDROM && sxsi_getdevtype(3)!=SXSIDEV_CDROM); // 旧機種互換モード？

		// 未接続のものを無視して接続順にDA/UAを割り当てる
		ncidx = idx = 0;
		for(i=0;i<4;i++){
			if(sxsi_getdevtype(i)==SXSIDEV_HDD){
				sxsi_unittbl[idx] = i;
				idx++;
			}else{
				ncidx = i;
			}
		}
		for(;idx<4;idx++){
			sxsi_unittbl[idx] = ncidx; // XXX: 余ったDA/UAはとりあえず未接続の番号に設定
		}
		
		mem[0xF8E80+0x0010] = (sxsi_getdevtype(3)!=SXSIDEV_NC ? 0x8 : 0x0)|(sxsi_getdevtype(2)!=SXSIDEV_NC ? 0x4 : 0x0)|
								(sxsi_getdevtype(1)!=SXSIDEV_NC ? 0x2 : 0x0)|(sxsi_getdevtype(0)!=SXSIDEV_NC ? 0x1 : 0x0);

		// WORKAROUND for WinNT4.0　ideio.cのideio_basereset()も参照のこと
		mem[0x05bb] = (sxsi_getdevtype(3)==SXSIDEV_HDD ? 0x8 : 0x0)|(sxsi_getdevtype(2)==SXSIDEV_HDD ? 0x4 : 0x0)|
						(sxsi_getdevtype(1)==SXSIDEV_HDD ? 0x2 : 0x0)|(sxsi_getdevtype(0)==SXSIDEV_HDD ? 0x1 : 0x0); // XXX: 未使用って書いてあったので勝手に借りる
		if(compmode){
			mem[0x05ba] = mem[0x05bb];
		}else{
			mem[0x05ba] = (sxsi_getdevtype(3)!=SXSIDEV_NC ? 0x8 : 0x0)|(sxsi_getdevtype(2)!=SXSIDEV_NC ? 0x4 : 0x0)|
							(sxsi_getdevtype(1)!=SXSIDEV_NC ? 0x2 : 0x0)|(sxsi_getdevtype(0)!=SXSIDEV_NC ? 0x1 : 0x0);
		}

		if(np2cfg.winntfix){
			// WinNT3.50で必要
			if(sxsi_getdevtype(1)==SXSIDEV_NC && sxsi_getdevtype(3)==SXSIDEV_NC){
				mem[0x0457] = (sxsi_getdevtype(2)==SXSIDEV_HDD ? 0x42 : 0x07)|(sxsi_getdevtype(0)==SXSIDEV_HDD ? 0x90 : 0x38);//0xd2; // 接続なしは111でないと駄目
				mem[0x05b0] = 0xff; // 接続状況に関係なし？
			}
		}
	}else{
		mem[0xF8E80+0x0010] &= ~0x0f;
		mem[0x05ba] &= ~0x0f;
	}
#endif
	mem[0xF8E80+0x0011] = mem[0xF8E80+0x0011] & ~0x20; // 0x20のビットがONだとWin2000でマウスがカクカクする？
	if(np2cfg.modelnum) mem[0xF8E80+0x003F] = np2cfg.modelnum; // PC-9821 Model Number
	
#endif
	mem[0x45B] |= 0x80; // XXX: TEST OUT 5Fh,AL wait

	mem[0xF8E80+0x0011] &= ~0x80; // for 17KB version NECCDD.SYS
	
#if defined(SUPPORT_PCI)
	mem[0xF8E80+0x0004] |= 0x2c;
	//mem[0x5B7] = (0x277 >> 2); // READ_DATA port address
	mem[0x5B8] = 0x00; // No C-Bus PnP boards
#endif
	mem[0xF8E80+0x0002] |= 0x04; // set 19200bps support flag
#if defined(SUPPORT_RS232C_FIFO)
	mem[0xF8E80+0x0011] |= 0x10; // set 115200bps support flag
#else
	mem[0xF8E80+0x0011] &= ~0x10; // clear 115200bps support flag
#endif
	
#if defined(SUPPORT_HRTIMER)
	{
		_SYSTIME hrtimertime;
		UINT32 hrtimertimeuint;

		timemng_gettime(&hrtimertime);
		hrtimertimeuint = (((UINT32)hrtimertime.hour*60 + (UINT32)hrtimertime.minute)*60 + (UINT32)hrtimertime.second)*32 + ((UINT32)hrtimertime.milli*32)/1000;
		hrtimertimeuint |= 0x400000; // こうしないとWin98の時計が1日ずれる?
		STOREINTELDWORD(mem+0x04F1, hrtimertimeuint); // XXX: 04F4にも書いちゃってるけど差し当たっては問題なさそうなので･･･
	}
#endif	/* defined(SUPPORT_HRTIMER) */

#if defined(SUPPORT_PC9801_119)
	mem[MEMB_BIOS_FLAG3] |= 0x40;
#endif	/* defined(SUPPORT_PC9801_119) */

	// FDC
	if (fdc.support144) {
		mem[MEMB_F144_SUP] |= fdc.equip;
	}

	// IDE initialize
	if (pccore.hddif & PCHDD_IDE) {
		mem[MEMB_SYS_TYPE] |= 0x80;		// IDE
		CPU_AX = 0x8300;
		sasibios_operate();
		//mem[0x457] = 0x97; // 10010111
	}
}

static void bios_vectorset(void) {

	UINT	i;

	for (i=0; i<0x20; i++) {
		*(UINT16 *)(mem + (i*4)) = *(UINT16 *)(mem + BIOS_BASE + BIOS_TABLE + (i*2));
		SETBIOSMEM16((i * 4) + 2, BIOS_SEG);
	}
	SETBIOSMEM32(0x1e*4, 0xe8000000);
}

static void bios_screeninit(void) {

	REG8	al;

	al = 4;
	al += (pccore.dipsw[1] & 0x04) >> 1;
	al += (pccore.dipsw[1] & 0x08) >> 3;
	bios0x18_0a(al);
}

static void setbiosseed(UINT8 *ptr, UINT size, UINT seedpos) {

	UINT8	x;
	UINT8	y;
	UINT	i;

	x = 0;
	y = 0;
	for (i=0; i<size; i+=2) {
		x += ptr[i + 0];
		y += ptr[i + 1];
	}
	ptr[seedpos + 0] -= x;
	ptr[seedpos + 1] -= y;
}

void bios_initialize(void) {

	BOOL	biosrom;
	OEMCHAR	path[MAX_PATH];
	FILEH	fh;
	UINT	i, j;
	UINT32	tmp;
	UINT	pos;
	
#if defined(USE_CUSTOM_HOOKINST)
#if defined(SUPPORT_IA32_HAXM)
	if (np2hax.enable) {
		bioshookinfo.hookinst = 0xCC;//0xF4;//;0xCC;//HOOKINST_DEFAULT; // BIOSフックに使う命令（デフォルトはNOP命令をフック）
	}else
#endif
	{
		bioshookinfo.hookinst = HOOKINST_DEFAULT; // BIOSフックに使う命令（デフォルトはNOP命令をフック）
	}
#endif

#if defined(BIOS_IO_EMULATION)
	// np21w ver0.86 rev46 BIOS I/O emulation
	memset(&biosioemu, 0, sizeof(biosioemu));
	biosioemu.enable = np2cfg.biosioemu;
#endif

	biosrom = FALSE;
	getbiospath(path, str_biosrom, NELEMENTS(path));
	if(np2cfg.usebios){
		fh = file_open_rb(path);
		if (fh != FILEH_INVALID) {
			biosrom = (file_read(fh, mem + 0x0e8000, 0x18000) == 0x18000);
			file_close(fh);
		}
	}
	if (biosrom) {
		TRACEOUT(("load bios.rom"));
		pccore.rom |= PCROM_BIOS;
		// PnP BIOSを潰す
		for (i=0; i<0x10000; i+=0x10) {
			tmp = LOADINTELDWORD(mem + 0xf0000 + i);
			if (tmp == 0x506e5024) {
				TRACEOUT(("found PnP BIOS at %.5x", 0xf0000 + i));
				mem[0xf0000 + i] = 0x6e;
				mem[0xf0002 + i] = 0x24;
				break;
			}
		}
	}
	else {
		CopyMemory(mem + 0x0e8000, nosyscode, sizeof(nosyscode));
		if ((!biosrom) && (!(pccore.model & PCMODEL_EPSON))) {
			CopyMemory(mem + 0xe8dd8, neccheck, 0x25);
			pos = LOADINTELWORD(itfrom + 2);
			CopyMemory(mem + 0xf538e, itfrom + pos, 0x27);
		}
		setbiosseed(mem + 0x0e8000, 0x10000, 0xb1f0);
	}

#if defined(SUPPORT_PC9821)
	// ideio.cへ移動
	//getbiospath(path, OEMTEXT("bios9821.rom"), NELEMENTS(path));
	//fh = file_open_rb(path);
	//if (fh != FILEH_INVALID) {
	//	if (file_read(fh, mem + 0x0d8000, 0x2000) == 0x2000) {
	//		// IDE BIOSを潰す
	//		TRACEOUT(("load bios9821.rom"));
	//		STOREINTELWORD(mem + 0x0d8009, 0);
	//	}
	//	file_close(fh);
	//}
#if defined(BIOS_SIMULATE)
	// PC-9821用 機能フラグ F8E8:0000〜003F
	mem[0xf8e80] = 0x98;
	mem[0xf8e81] = 0x21;
	mem[0xf8e82] = 0x1f;
	mem[0xf8e83] = 0x20;
	mem[0xf8e84] = 0x2c;
	mem[0xf8e85] = 0xb0;

	mem[0xF8E80+0x0011] = 0;
	//mem[0xF8E80+0x003f] = 0x21; // 機種ID PC-9821 Xa7,9,10,12/C

	// mem[0xf8eaf] = 0x21;		// <- これって何だっけ？ 0xf8ebfの間違い？
#endif
#endif

#if defined(BIOS_SIMULATE)
	CopyMemory(mem + BIOS_BASE, biosfd80, sizeof(biosfd80));
	if (!biosrom) {
		lio_initialize();
	}

	for (i=0; i<8; i+=2) {
		STOREINTELWORD(mem + 0xfd800 + 0x1aaf + i, 0x1ab7);
		STOREINTELWORD(mem + 0xfd800 + 0x1ad7 + i, 0x1adf);
		STOREINTELWORD(mem + 0xfd800 + 0x2361 + i, 0x1980);
	}
	CopyMemory(mem + 0xfd800 + 0x1ab7, fdfmt2hd, sizeof(fdfmt2hd));
	CopyMemory(mem + 0xfd800 + 0x1adf, fdfmt2dd, sizeof(fdfmt2dd));
	CopyMemory(mem + 0xfd800 + 0x1980, fdfmt144, sizeof(fdfmt144));

	SETBIOSMEM16(0xfffe8, 0xcb90);
	SETBIOSMEM16(0xfffec, 0xcb90);
	mem[0xffff0] = 0xea;
	STOREINTELDWORD(mem + 0xffff1, 0xfd800000);

	CopyMemory(mem + 0x0fd800 + 0x0e00, keytable[0], 0x60 * 8);
	
	//fh = file_create_c(_T("emuitf.rom"));
	//if (fh != FILEH_INVALID) {
	//	file_write(fh, itfrom, sizeof(itfrom));
	//	file_close(fh);
	//	TRACEOUT(("write emuitf.rom"));
	//}
	memset(mem + ITF_ADRS, 0, sizeof(itfrom)+1);
	CopyMemory(mem + ITF_ADRS, itfrom, sizeof(itfrom));
#if defined(SUPPORT_FAST_MEMORYCHECK)
	// 高速メモリチェック
	if(np2cfg.memcheckspeed > 1){
		// 猫メモリチェックのDEC r/m16 を強制フック(実行時はアドレスが変わるので注意)
		STOREINTELWORD((mem + ITF_ADRS + 5886), 128 * np2cfg.memcheckspeed);
		mem[ITF_ADRS + 5924] = BIOS_HOOKINST;
	}
#endif
	np2cfg.memchkmx = 0; // 無効化 (obsolete)
	if(np2cfg.memchkmx){ // メモリカウント最大値変更
		mem[ITF_ADRS + 6057] = mem[ITF_ADRS + 6061] = (UINT8)MAX((int)np2cfg.memchkmx-14, 1); // XXX: 場所決め打ち
	}else{
#if defined(SUPPORT_LARGE_MEMORY)
		if(np2cfg.EXTMEM >= 256){ // 大容量メモリカウント
			// XXX: 場所決め打ち注意
			for(i=ITF_ADRS + 6066; i >= ITF_ADRS + 6058; i--){
				mem[i] = mem[i-1]; // 1byteずらし
			}
			mem[ITF_ADRS + 6067] = mem[ITF_ADRS + 6068] = BIOS_HOOKINST; // call	WAITVSYNC を NOP化
			mem[ITF_ADRS + 6055] = 0x81; // CMP r/m16, imm8 を CMP r/m16, imm16 に変える
			STOREINTELWORD((mem + ITF_ADRS + 6057), (MEMORY_MAXSIZE-14)); // cmp　bx, (EXTMEMORYMAX - 16)の部分をいじる
			STOREINTELWORD((mem + ITF_ADRS + 6061+1), (MEMORY_MAXSIZE-14)); // mov　bx, (EXTMEMORYMAX - 16)の部分をいじる（1byteずらしたのでオフセット注意）
		}
#endif
	}
	if(np2cfg.sbeeplen || np2cfg.sbeepadj){ // ピポ音長さ変更
		UINT16 beeplen = (np2cfg.sbeeplen ? np2cfg.sbeeplen : mem[ITF_ADRS + 5553]); // XXX: 場所決め打ち
		if(np2cfg.sbeepadj){ // 自動調節
			beeplen = beeplen * np2cfg.multiple / 10;
			if(beeplen == 0) beeplen = 1;
			if(beeplen > 255) beeplen = 255;
		}
#if defined(SUPPORT_IA32_HAXM)
		if (np2hax.enable && np2cfg.sbeepadj) {
			mem[ITF_ADRS + 5553] = 255;
		}else
#endif
		mem[ITF_ADRS + 5553] = (UINT8)beeplen; // XXX: 場所決め打ち
	}
	mem[ITF_ADRS + 0x7ff0] = 0xea;
	STOREINTELDWORD(mem + ITF_ADRS + 0x7ff1, 0xf8000000);
	if (pccore.model & PCMODEL_EPSON) {
		mem[ITF_ADRS + 0x7ff1] = 0x04;
	}
	else if ((pccore.model & PCMODELMASK) == PCMODEL_VM) {
		mem[ITF_ADRS + 0x7ff1] = 0x08;
	}
	setbiosseed(mem + 0x0f8000, 0x08000, 0x7ffe);
#else
	fh = file_open_c("itf.rom");
	if (fh != FILEH_INVALID) {
		file_read(fh, mem + ITF_ADRS, 0x8000);
		file_close(fh);
		TRACEOUT(("load itf.rom"));
	}
#endif

	CopyMemory(mem + 0x1c0000, mem + ITF_ADRS, 0x08000);
	CopyMemory(mem + 0x1e8000, mem + 0x0e8000, 0x10000);
	
#if defined(SUPPORT_PCI)
	// PCI BIOS32 Service Directoryを探す
	for (i=0; i<0x10000; i+=0x4) {
		tmp = LOADINTELDWORD(mem + 0xf0000 + i);
		if (tmp == 0x5F32335F) { // "_32_"
			UINT8 checksum = 0;
			for(j=0;j<16;j++){
				checksum += mem[0xf0000 + i + j];
			}
			if(checksum==0){
				// 発見した場合はその位置を使う
				TRACEOUT(("found BIOS32 Service Directory at %.5x", 0xf0000 + i));
				pcidev.bios32svcdir = 0xf0000 + i;
				pcidev_updateBIOS32data();
				break;
			}
		}
	}
	if(i==0x10000){
		int emptyflag = 0;
		TRACEOUT(("BIOS32 Service Directory not found."));
		
		// PCI BIOS32 Service Directoryを割り当てる
		// XXX: 多分この辺なら空いてるだろーということで･･･
		pcidev.bios32svcdir = 0xffa00;
		for(i=0;i<0x400;i+=0x10){
			emptyflag = 1;
			// 16byte分空いてるかチェック（0だからといって空いてるとは限らないけど･･･）
			for(j=0;j<16;j++){
				if(mem[pcidev.bios32svcdir+i+j] != 0){
					emptyflag = 0;
				}
			}
			if(emptyflag){
				// BIOS32 Service Directoryを置く
				TRACEOUT(("Allocate BIOS32 Service Directory at 0x%.5x", pcidev.bios32svcdir));
				pcidev.bios32svcdir += i;
				pcidev_updateBIOS32data();
				break;
			}
		}
		if(i==0x400){
			// 空きがないのでBIOS32 Service Directoryを置けず･･･
			TRACEOUT(("Error: Cannot allocate memory for BIOS32 Service Directory."));
			pcidev.bios32svcdir = 0;
		}
	}
#endif
	
// np21w ver0.86 rev46-69 BIOS I/O emulation
#if defined(BIOS_IO_EMULATION)
	// エミュレーション用に書き換え。とりあえずINT 18HとINT 1BHとINT 1CHのみ対応
	if(biosioemu.enable){
		mem[BIOS_BASE + BIOSOFST_18 + 1] = 0xee; // 0xcf(IRET) -> 0xee(OUT DX, AL)
		mem[BIOS_BASE + BIOSOFST_18 + 2] = BIOS_HOOKINST; // 0x90(NOP) BIOS hook
		mem[BIOS_BASE + BIOSOFST_18 + 3] = 0xcf; // 0xcf(IRET)
		mem[BIOS_BASE + BIOSOFST_1b + 1] = 0xee; // 0xcf(IRET) -> 0xee(OUT DX, AL)
		mem[BIOS_BASE + BIOSOFST_1b + 2] = BIOS_HOOKINST; // 0x90(NOP) BIOS hook
		mem[BIOS_BASE + BIOSOFST_1b + 3] = 0xcf; // 0xcf(IRET)
		mem[BIOS_BASE + BIOSOFST_1c + 1] = 0xee; // 0xcf(IRET) -> 0xee(OUT DX, AL)
		mem[BIOS_BASE + BIOSOFST_1c + 2] = BIOS_HOOKINST; // 0x90(NOP) BIOS hook
		mem[BIOS_BASE + BIOSOFST_1c + 3] = 0xcf; // 0xcf(IRET)
	}
#endif
	
// np21w ver0.86 rev70 VGA BIOS for MODE X
#if defined(SUPPORT_VGA_MODEX)
	if(np2cfg.usemodex){
		mem[BIOS_BASE + BIOSOFST_10 + 0] = 0x90; // 0x90(NOP) BIOS hook
		mem[BIOS_BASE + BIOSOFST_10 + 1] = 0xcf; // 0xcf(IRET)
		mem[BIOS_BASE + BIOSOFST_10 + 0] = 0x90; // 0x90(NOP) BIOS hook
		mem[BIOS_BASE + BIOS_TABLE + 0x20] = 0x8e;
		mem[BIOS_BASE + BIOS_TABLE + 0x21] = 0x00;
	}
#endif
	
#ifdef USE_CUSTOM_HOOKINST
	bios_updatehookinst(mem + 0xf8000, 0x100000 - 0xf8000);
#endif
}

static void bios_itfcall(void) {

	int		i;

	bios_itfprepare();
	bios_memclear();
	bios_vectorset();
	bios0x09_init();
	bios_reinitbyswitch();
	bios0x18_0c();

	if (!np2cfg.ITF_WORK) {
		for (i=0; i<8; i++) {
			mem[MEMX_MSW + (i*4)] = msw_default[i];
		}
		CPU_FLAGL |= C_FLAG;
	}
	else {
		CPU_DX = 0x43d;
		CPU_AL = 0x10;
		mem[0x004f8] = 0xee;		// out	dx, al
		mem[0x004f9] = 0xea;		// call	far
		SETBIOSMEM16(0x004fa, 0x0000);
		SETBIOSMEM16(0x004fc, 0xffff);
		CPU_FLAGL &= ~C_FLAG;
	}
}

// np21w ver0.86 rev46-69 BIOS I/O emulation
#if defined(BIOS_IO_EMULATION)
// LIFO（若干高速だが逆順のため注意）
void biosioemu_push8(UINT16 port, UINT8 data) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		biosioemu.data[biosioemu.count].flag = BIOSIOEMU_FLAG_NONE;
		biosioemu.data[biosioemu.count].port = port;
		biosioemu.data[biosioemu.count].data = data;
		biosioemu.count++;
	}
}
void biosioemu_push16(UINT16 port, UINT32 data) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		biosioemu.data[biosioemu.count].flag = BIOSIOEMU_FLAG_MB;
		biosioemu.data[biosioemu.count].port = port;
		biosioemu.data[biosioemu.count].data = data;
		biosioemu.count++;
	}
}
void biosioemu_push8_read(UINT16 port) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		biosioemu.data[biosioemu.count].flag = BIOSIOEMU_FLAG_READ;
		biosioemu.data[biosioemu.count].port = port;
		biosioemu.data[biosioemu.count].data = 0;
		biosioemu.count++;
	}
}
void biosioemu_push16_read(UINT16 port) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		biosioemu.data[biosioemu.count].flag = BIOSIOEMU_FLAG_READ|BIOSIOEMU_FLAG_MB;
		biosioemu.data[biosioemu.count].port = port;
		biosioemu.data[biosioemu.count].data = 0;
		biosioemu.count++;
	}
}
// FIFO
void biosioemu_enq8(UINT16 port, UINT8 data) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		int i;
		for(i=biosioemu.count-1;i>=0;i--){
			biosioemu.data[i+1].flag = biosioemu.data[i].flag;
			biosioemu.data[i+1].port = biosioemu.data[i].port;
			biosioemu.data[i+1].data = biosioemu.data[i].data;
		}
		biosioemu.data[0].flag = BIOSIOEMU_FLAG_NONE;
		biosioemu.data[0].port = port;
		biosioemu.data[0].data = data;
		biosioemu.count++;
	}
}
void biosioemu_enq16(UINT16 port, UINT32 data) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		int i;
		for(i=biosioemu.count-1;i>=0;i--){
			biosioemu.data[i+1].flag = biosioemu.data[i].flag;
			biosioemu.data[i+1].port = biosioemu.data[i].port;
			biosioemu.data[i+1].data = biosioemu.data[i].data;
		}
		biosioemu.data[0].flag = BIOSIOEMU_FLAG_MB;
		biosioemu.data[0].port = port;
		biosioemu.data[0].data = data;
		biosioemu.count++;
	}
}
void biosioemu_enq8_read(UINT16 port) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		int i;
		for(i=biosioemu.count-1;i>=0;i--){
			biosioemu.data[i+1].flag = biosioemu.data[i].flag;
			biosioemu.data[i+1].port = biosioemu.data[i].port;
			biosioemu.data[i+1].data = biosioemu.data[i].data;
		}
		biosioemu.data[0].flag = BIOSIOEMU_FLAG_READ;
		biosioemu.data[0].port = port;
		biosioemu.data[0].data = 0;
		biosioemu.count++;
	}
}
void biosioemu_enq16_read(UINT16 port) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		int i;
		for(i=biosioemu.count-1;i>=0;i--){
			biosioemu.data[i+1].flag = biosioemu.data[i].flag;
			biosioemu.data[i+1].port = biosioemu.data[i].port;
			biosioemu.data[i+1].data = biosioemu.data[i].data;
		}
		biosioemu.data[0].flag = BIOSIOEMU_FLAG_READ|BIOSIOEMU_FLAG_MB;
		biosioemu.data[0].port = port;
		biosioemu.data[0].data = 0;
		biosioemu.count++;
	}
}
void biosioemu_begin(void) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count==0){
		// データが無いのでI/Oポート出力をスキップ
		biosioemu.oldEAX = 0;
		biosioemu.oldEDX = 0;
		CPU_EIP += 2;
	}else{
		int idx = biosioemu.count-1;
		// レジスタ退避
		biosioemu.oldEAX = CPU_EAX;
		biosioemu.oldEDX = CPU_EDX;
		// I/O設定
		if(biosioemu.data[idx].flag & BIOSIOEMU_FLAG_READ){
			if(biosioemu.data[idx].flag & BIOSIOEMU_FLAG_MB){
				// I/Oポート設定
				CPU_DX = biosioemu.data[idx].port;
				//CPU_EAX = biosioemu.data[idx].data;
				// 入力サイズ設定
				mem[CPU_EIP + (CPU_CS << 4)] = 0xed;
			}else{
				// I/Oポート設定
				CPU_DX = biosioemu.data[idx].port;
				//CPU_AL = biosioemu.data[idx].data & 0xff;
				// 入力サイズ設定
				mem[CPU_EIP + (CPU_CS << 4)] = 0xec;
			}
		}else{
			if(biosioemu.data[idx].flag & BIOSIOEMU_FLAG_MB){
				// I/O出力データ設定
				CPU_DX = biosioemu.data[idx].port;
				CPU_EAX = biosioemu.data[idx].data;
				// 出力サイズ設定
				mem[CPU_EIP + (CPU_CS << 4)] = 0xef;
			}else{
				// I/O出力データ設定
				CPU_DX = biosioemu.data[idx].port;
				CPU_AL = biosioemu.data[idx].data & 0xff;
				// 出力サイズ設定
				mem[CPU_EIP + (CPU_CS << 4)] = 0xee;
			}
		}
		biosioemu.count--;
	}
}
void biosioemu_proc(void) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count==0){
		// レジスタ戻す
		CPU_EAX = biosioemu.oldEAX;
		CPU_EDX = biosioemu.oldEDX;
		biosioemu.oldEAX = 0;
		biosioemu.oldEDX = 0;
	}else{
		int idx = biosioemu.count-1;
		// 命令位置を戻す
		CPU_EIP -= 2;
		// I/O設定
		if(biosioemu.data[idx].flag & BIOSIOEMU_FLAG_READ){
			if(biosioemu.data[idx].flag & BIOSIOEMU_FLAG_MB){
				// I/Oポート設定
				CPU_DX = biosioemu.data[idx].port;
				//CPU_EAX = biosioemu.data[idx].data;
				// 入力サイズ設定
				mem[CPU_EIP + (CPU_CS << 4)] = 0xed;
			}else{
				// I/Oポート設定
				CPU_DX = biosioemu.data[idx].port;
				//CPU_AL = biosioemu.data[idx].data & 0xff;
				// 入力サイズ設定
				mem[CPU_EIP + (CPU_CS << 4)] = 0xec;
			}
		}else{
			if(biosioemu.data[idx].flag & BIOSIOEMU_FLAG_MB){
				// I/O出力データ設定
				CPU_DX = biosioemu.data[idx].port;
				CPU_EAX = biosioemu.data[idx].data;
				// 出力サイズ設定
				mem[CPU_EIP + (CPU_CS << 4)] = 0xef;
			}else{
				// I/O出力データ設定
				CPU_DX = biosioemu.data[idx].port;
				CPU_AL = biosioemu.data[idx].data & 0xff;
				// 出力サイズ設定
				mem[CPU_EIP + (CPU_CS << 4)] = 0xee;
			}
		}
		biosioemu.count--;
	}
}
#endif

UINT MEMCALL biosfunc(UINT32 adrs) {

	UINT16	bootseg;
	
// np21w ver0.86 rev46 BIOS I/O emulation
#if defined(BIOS_IO_EMULATION)
	UINT32	oldEIP;
#endif
	
#if defined(SUPPORT_FAST_MEMORYCHECK)
	// 高速メモリチェック
	if (CPU_ITFBANK && adrs == 0xf9724) {
		UINT16 subvalue = LOADINTELWORD((mem + ITF_ADRS + 5886)) / 128;
		UINT16 counter = MEMR_READ16(CPU_SS, CPU_EBP + 6);
		if(subvalue == 0) subvalue = 1;
		if(counter >= subvalue){
			counter -= subvalue;
		}else{
			counter = 0;
		}
		if(counter == 0){
			CPU_FLAG |= Z_FLAG;
		}else{
			CPU_FLAG &= ~Z_FLAG;
		}
		MEMR_WRITE16(CPU_SS, CPU_EBP + 6, counter);
		CPU_IP += 2;
	}
#endif

	if ((CPU_ITFBANK) && (adrs >= 0xf8000) && (adrs < 0x100000)) {
		// for epson ITF
		return(0);
	}

//	TRACEOUT(("biosfunc(%x)", adrs));
#if defined(CPUCORE_IA32) && defined(TRACE)
	if (CPU_STAT_PAGING) {
		UINT32 pde = MEMP_READ32(CPU_STAT_PDE_BASE);
		if (!(pde & CPU_PDE_PRESENT)) {
			TRACEOUT(("page0: PTE not present"));
		}
		else {
			UINT32 pte = MEMP_READ32(pde & CPU_PDE_BASEADDR_MASK);
			if (!(pte & CPU_PTE_PRESENT)) {
				TRACEOUT(("page0: not present"));
			}
			else if (pte & CPU_PTE_BASEADDR_MASK) {
				TRACEOUT(("page0: physical address != 0 (pte = %.8x)", pte));
			}
		}
	}
#endif

	switch(adrs) {
		case BIOS_BASE + BIOSOFST_ITF:		// リセット
			bios_itfcall();
			return(1);

		case BIOS_BASE + BIOSOFST_INIT:		// ブート
#if 1		// for RanceII
			bios_memclear();
#endif
			bios_vectorset();
#if 1
			bios0x09_init();
#endif
			bios_reinitbyswitch();
			bios_vectorset();
			bios_screeninit();
			if (((pccore.model & PCMODELMASK) >= PCMODEL_VX) &&
				(pccore.sound & 0x7e)) {
				if(g_nSoundID == SOUNDID_MATE_X_PCM || ((g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16) && np2cfg.snd118irqf == np2cfg.snd118irqp) || g_nSoundID == SOUNDID_PC_9801_86_WSS || g_nSoundID == SOUNDID_WAVESTAR || g_nSoundID == SOUNDID_PC_9801_86_WSS_SB16){
					iocore_out8(0x188, 0x27);
					iocore_out8(0x18a, 0x30);
					if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
						iocore_out8(cs4231.port[4], 0x27);
						iocore_out8(cs4231.port[4]+2, 0x30);
					}
				}else{
					iocore_out8(0x188, 0x27);
					iocore_out8(0x18a, 0x3f);
				}
			}
			return(1);

		case BIOS_BASE + BIOSOFST_09:
			CPU_REMCLOCK -= 500;
			bios0x09();
			return(1);

		case BIOS_BASE + BIOSOFST_0c:
			CPU_REMCLOCK -= 500;
			bios0x0c();
			return(1);
			
// np21w ver0.86 rev70 VGA BIOS for MODE X
#if defined(SUPPORT_VGA_MODEX)
		case BIOS_BASE + BIOSOFST_10:
			CPU_REMCLOCK -= 500;
			TRACEOUT(("VGA INT: AH=%02x, AL=%02x", CPU_AH, CPU_AL));
			switch(CPU_AH){
			case 0x00:
#if defined(SUPPORT_CL_GD5430)
				if(CPU_AL == 0x13){
					// MODE X
					np2clvga.modex = 1;
					np2clvga.VRAMWindowAddr3 = 0xa0000;
					np2wab.relaystateext |= 0x02;
					np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext);
				}else{
					np2clvga.modex = 0;
					np2clvga.VRAMWindowAddr3 = 0;
					//np2wab.relaystateext &= ~0x01;
					np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext);
				}
#endif
				break;
			case 0x1a:
				// XXX: WAB有効の時だけ返す
				if(np2clvga.modex || np2wab.relaystateint || np2wab.relaystateext){
					if(CPU_AL==0x00){
						CPU_BH = 0x00;
						CPU_BL = 0x08;
					}
					CPU_AL = 0x1a;
				}
				break;
			default:
				// nothing to do
				break;
			}
			return(1);
#endif

		case BIOS_BASE + BIOSOFST_12:
			CPU_REMCLOCK -= 500;
			bios0x12();
			return(1);

		case BIOS_BASE + BIOSOFST_13:
			CPU_REMCLOCK -= 500;
			bios0x13();
			return(1);

		case BIOS_BASE + BIOSOFST_18:
			CPU_REMCLOCK -= 200;
#if defined(BIOS_IO_EMULATION)
			oldEIP = CPU_EIP;
			biosioemu.count = 0; 
#endif
			bios0x18();
#if defined(BIOS_IO_EMULATION)
			// np21w ver0.86 rev46 BIOS I/O emulation
			if(oldEIP == CPU_EIP){
				biosioemu_begin(); 
			}else{
				biosioemu.count = 0; 
			}
#endif
			return(1);
			
#if defined(BIOS_IO_EMULATION)
		case BIOS_BASE + BIOSOFST_18 + 2: // np21w ver0.86 rev46 BIOS I/O emulation
			biosioemu_proc();
			return(1);
#endif

		case BIOS_BASE + BIOSOFST_19:
			CPU_REMCLOCK -= 200;
			bios0x19();
			return(1);

		case BIOS_BASE + BIOSOFST_CMT:
			CPU_REMCLOCK -= 200;
			bios0x1a_cmt();
			return(0);											// return(1);

		case BIOS_BASE + BIOSOFST_PRT:
			CPU_REMCLOCK -= 200;
#if defined(SUPPORT_PCI)
			if(CPU_AH == 0xb1){
				bios0x1a_pci();
			}else if(CPU_AH == 0xb4){
				bios0x1a_pcipnp();
			}else
#endif
			{
				bios0x1a_prt();
			}
			return(1);

		case BIOS_BASE + BIOSOFST_1b:
			CPU_STI;
			CPU_REMCLOCK -= 200;
#if defined(BIOS_IO_EMULATION)
			oldEIP = CPU_EIP;
			biosioemu.count = 0;
#endif
			bios0x1b();
#if defined(BIOS_IO_EMULATION)
			// np21w ver0.86 rev69 BIOS I/O emulation
			if(oldEIP == CPU_EIP){
				biosioemu_begin(); 
			}else{
				biosioemu.count = 0; 
			}
#endif
			return(1);
			
#if defined(BIOS_IO_EMULATION)
		case BIOS_BASE + BIOSOFST_1b + 2: // np21w ver0.86 rev69 BIOS I/O emulation
			biosioemu_proc();
			return(1);
#endif

		case BIOS_BASE + BIOSOFST_1c:
			CPU_REMCLOCK -= 200;
#if defined(BIOS_IO_EMULATION)
			oldEIP = CPU_EIP;
			biosioemu.count = 0; 
#endif
			bios0x1c();
#if defined(BIOS_IO_EMULATION)
			// np21w ver0.86 rev47 BIOS I/O emulation
			if(oldEIP == CPU_EIP){
				biosioemu_begin(); 
			}else{
				biosioemu.count = 0; 
			}
#endif
			return(1);
			
#if defined(BIOS_IO_EMULATION)
		case BIOS_BASE + BIOSOFST_1c + 2: // np21w ver0.86 rev47 BIOS I/O emulation
			biosioemu_proc();
			return(1);
#endif

		case BIOS_BASE + BIOSOFST_1f:
			CPU_REMCLOCK -= 200;
			bios0x1f();
			return(1);

		case BIOS_BASE + BIOSOFST_WAIT:
			CPU_STI;
			return(bios0x1b_wait());								// ver0.78

		case 0xfffe8:					// ブートストラップロード
			CPU_REMCLOCK -= 2000;
			sxsi_workaround_bootwait = SXSI_WORKAROUND_BOOTWAIT;
			bootseg = bootstrapload();
			if (bootseg) {
				CPU_STI;
				CPU_CS = bootseg;
				CPU_IP = 0x0000;
				return(1);
			}
			return(0);

		case 0xfffec:
			CPU_REMCLOCK -= 2000;
			bootstrapload();
			return(0);
	}

	if ((adrs >= 0xf9950) && (adrs <= 0x0f9990) && (!(adrs & 3))) {
		CPU_REMCLOCK -= 500;
		bios_lio((REG8)((adrs - 0xf9950) >> 2));
	}
	else if (adrs == 0xf9994) {
		if (nevent_iswork(NEVENT_GDCSLAVE)) {
			CPU_IP--;
			CPU_REMCLOCK = -1;
			return(1);
		}
	}
	
	return(0);
}

#ifdef SUPPORT_PCI
UINT MEMCALL bios32func(UINT32 adrs) {
	
	// アドレスがBIOS32 Entry Pointなら処理
	if (pcidev.bios32entrypoint && adrs == pcidev.bios32entrypoint) {
		CPU_REMCLOCK -= 200;
		bios0x1a_pci_part(1);
	}
	return(0);
}
#endif
