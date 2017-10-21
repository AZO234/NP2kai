/**
 * @file	bios.c
 * @brief	Implementation of BIOS
 */

#include "compiler.h"
#include "bios.h"
#include "biosmem.h"
#include "sxsibios.h"
#include "strres.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "lio/lio.h"
#include "vram.h"
#include "diskimage/fddfile.h"
#include "fdd/fdd_mtr.h"
#include "fdfmt.h"
#include "dosio.h"
#include "keytable.res"
#include "itfrom.res"
#include "startup.res"
#include "biosfd80.res"
#if defined(SUPPORT_IDEIO)
#include	"fdd/sxsi.h"
#endif
#if defined(SUPPORT_HRTIMER)
#include	"timemng.h"
#endif
#include	"fmboard.h"

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
	UINT8	extmem; // LARGE_MEM //UINT16	extmem;
	UINT8	boot;
	//FILEH	fh;
	//OEMCHAR	path[MAX_PATH];

	if (!(pccore.dipsw[2] & 0x80)) {
#if defined(CPUCORE_IA32)
		mem[MEMB_SYS_TYPE] = 0x03;		// 80386～
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
	extmem = min(extmem, 14);
	mem[MEMB_EXPMMSZ] = (UINT8)(extmem << 3);
	if (pccore.extmem >= 15) {
		mem[0x0594] = pccore.extmem - 15;
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
	
#if defined(SUPPORT_IDEIO)
	mem[0xF8E80+0x0010] = (sxsi_getdevtype(3)!=SXSIDEV_NC ? 0x8 : 0x0)|(sxsi_getdevtype(2)!=SXSIDEV_NC ? 0x4 : 0x0)|
						  (sxsi_getdevtype(1)!=SXSIDEV_NC ? 0x2 : 0x0)|(sxsi_getdevtype(0)!=SXSIDEV_NC ? 0x1 : 0x0);
	//mem[0x0457] = (sxsi_getdevtype(1)==SXSIDEV_HDD ? 0x42 : 0x07)|(sxsi_getdevtype(0)==SXSIDEV_HDD ? 0x90 : 0x38);
	//mem[0x045D] |= 0x0C;
	//mem[0x045E] |= 0x60;
	//mem[0x0481] |= 0x03;
	if(np2cfg.winntfix){
		// WinNT4.0でHDDが認識するようになる（ただしWin9xではHDD認識失敗の巻き添えになってCDが認識しなくなる）
		mem[0x05ba] = (sxsi_getdevtype(3)==SXSIDEV_HDD ? 0x8 : 0x0)|(sxsi_getdevtype(2)==SXSIDEV_HDD ? 0x4 : 0x0)|
					  (sxsi_getdevtype(1)==SXSIDEV_HDD ? 0x2 : 0x0)|(sxsi_getdevtype(0)==SXSIDEV_HDD ? 0x1 : 0x0);
	}
	//mem[0x055D] |= (sxsi_getdevtype(3)==SXSIDEV_HDD ? 0x8 : 0x0)|(sxsi_getdevtype(2)==SXSIDEV_HDD ? 0x4 : 0x0)|
	//			   (sxsi_getdevtype(1)==SXSIDEV_HDD ? 0x2 : 0x0)|(sxsi_getdevtype(0)==SXSIDEV_HDD ? 0x1 : 0x0);
	//mem[0x05ba] = (sxsi_getdevtype(3)==SXSIDEV_HDD ? 0x8 : 0x0)|(sxsi_getdevtype(2)==SXSIDEV_HDD ? 0x4 : 0x0)|
	//			  (sxsi_getdevtype(1)==SXSIDEV_HDD ? 0x2 : 0x0)|(sxsi_getdevtype(0)==SXSIDEV_HDD ? 0x1 : 0x0);
	//mem[0x05A9] = 0xF3;
	//mem[0x05AA] = 0x6D;
	//mem[0x05AB] = 0xCB;
	//mem[0x05b0] = 0xff;
	//mem[0x05E8] = 0x8F;
	//mem[0x05E9] = 0x07;
	//mem[0x05EA] = 0x00;
	//mem[0x05EB] = 0xD8;
	//mem[0x05EC] = 0x89;
	//mem[0x05ED] = 0x07;
	//mem[0x05EE] = 0x00;
	//mem[0x05EF] = 0xD8;
	//mem[0x04DA] = 0xAA;
	//mem[0x04DB] = 0xAA;
	//getbiospath(path, _T("test.bin"), NELEMENTS(path));
	//fh = file_open_rb(path);
	//if (fh != FILEH_INVALID) {
	//	file_read(fh, mem + 0x0DA000, 0x2B0);
	//	file_close(fh);
	//}
	//getbiospath(path, _T("ide.romemu"), NELEMENTS(path));
//	fh = file_open_rb(path);
//	if (fh != FILEH_INVALID) {
//#define READSIZE 0
//		file_seek(fh, READSIZE, SEEK_CUR);
//		file_read(fh, mem + 0x0D8000+READSIZE, 0x2000-READSIZE);
//		file_close(fh);
//	}
	mem[0x45B] |= 0x80; // XXX: TEST
#endif
	mem[0xF8E80+0x0011] = mem[0xF8E80+0x0011] & ~0x20; // 0x20のビットがONだとWin2000でマウスがカクカクする？
	if(np2cfg.modelnum) mem[0xF8E80+0x003F] = np2cfg.modelnum; // PC-9821 Model Number
#endif
	
#if defined(SUPPORT_HRTIMER)
	{
		_SYSTIME hrtimertime;
		UINT32 hrtimertimeuint;

		timemng_gettime(&hrtimertime);
		hrtimertimeuint = (((UINT32)hrtimertime.hour*60 + (UINT32)hrtimertime.minute)*60 + (UINT32)hrtimertime.second)*32 + ((UINT32)hrtimertime.milli*32)/1000;
		hrtimertimeuint |= 0x400000;
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
	UINT	i;
	UINT32	tmp;
	UINT	pos;

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
	mem[0xf8e80] = 0x98;
	mem[0xf8e81] = 0x21;
	mem[0xf8e82] = 0x1f;
	mem[0xf8e83] = 0x20;	// Model Number?
	mem[0xf8e84] = 0x2c;
	mem[0xf8e85] = 0xb0;

	// mem[0xf8eaf] = 0x21;		// <- これって何だっけ？
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

	CopyMemory(mem + 0x0fd800 + 0x0e00, keytable[0], 0x300);
	
	//fh = file_create_c(_T("emuitf.rom"));
	//if (fh != FILEH_INVALID) {
	//	file_write(fh, itfrom, sizeof(itfrom));
	//	file_close(fh);
	//	TRACEOUT(("write emuitf.rom"));
	//}
	CopyMemory(mem + ITF_ADRS, itfrom, sizeof(itfrom));
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


UINT MEMCALL biosfunc(UINT32 adrs) {

	UINT16	bootseg;

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
				if(g_nSoundID == SOUNDID_MATE_X_PCM || g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_WSS){
					iocore_out8(0x188, 0x27);
					iocore_out8(0x18a, 0x30);
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
			bios0x18();
			return(1);

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
			bios0x1a_prt();
			return(1);

		case BIOS_BASE + BIOSOFST_1b:
			CPU_STI;
			CPU_REMCLOCK -= 200;
			bios0x1b();
			return(1);

		case BIOS_BASE + BIOSOFST_1c:
			CPU_REMCLOCK -= 200;
			bios0x1c();
			return(1);

		case BIOS_BASE + BIOSOFST_1f:
			CPU_REMCLOCK -= 200;
			bios0x1f();
			return(1);

		case BIOS_BASE + BIOSOFST_WAIT:
			CPU_STI;
			return(bios0x1b_wait());								// ver0.78

		case 0xfffe8:					// ブートストラップロード
			CPU_REMCLOCK -= 2000;
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

