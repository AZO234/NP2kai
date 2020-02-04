#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"gdc_sub.h"
#include	"bios.h"
#include	"biosmem.h"
#include	"font/font.h"


typedef struct {
	UINT8	GBON_PTN;
	UINT8	GBBCC;
	UINT8	GBDOTU;
	UINT8	GBDSP;
	UINT8	GBCPC[4];
	UINT8	GBSX1[2];
	UINT8	GBSY1[2];
	UINT8	GBLNG1[2];
	UINT8	GBWDPA[2];
	UINT8	GBRBUF[2][3];
	UINT8	GBSX2[2];
	UINT8	GBSY2[2];
	UINT8	GBMDOT[2];
	UINT8	GBCIR[2];
	UINT8	GBLNG2[2];
	UINT8	GBMDOTI[8];
	UINT8	GBDTYP;
	UINT8	GBFILL;
} UCWTBL;

typedef struct {
	UINT8	raster;
	UINT8	pl;
	UINT8	bl;
	UINT8	cl;
} CRTDATA;

static const UINT8 modenum[4] = {3, 1, 0, 2};

static const CRTDATA crtdata[7] = {
						{0x09,	0x1f, 0x08, 0x08},		// 200-20
						{0x07,	0x00, 0x07, 0x08},		// 200-25
						{0x13,	0x1e, 0x11, 0x10},		// 400-20
						{0x0f,	0x00, 0x0f, 0x10},		// 400-25
						{0x17,	0x1c, 0x13, 0x10},		// 480-20
						{0x12,	0x1f, 0x11, 0x10},		// 480-25
						{0x0f,	0x00, 0x0f, 0x10}};		// 480-30

#if defined(SUPPORT_CRT31KHZ)
static const UINT8 gdcmastersync[6][8] = {
				{0x10,0x4e,0x07,0x25,0x0d,0x0f,0xc8,0x94},		/* 15 */
				{0x10,0x4e,0x07,0x25,0x07,0x07,0x90,0x65},		/* 24 */
				{0x10,0x4e,0x47,0x0c,0x07,0x0d,0x90,0x89},		/* 31 */
				{0x10,0x4e,0x4b,0x0c,0x03,0x06,0xe0,0x95},		/* 31-480:20 */
				{0x10,0x4e,0x4b,0x0c,0x03,0x0b,0xdb,0x95},		/* 31-480:25 */
				{0x10,0x4e,0x4b,0x0c,0x03,0x06,0xe0,0x95}};		/* 31-480:30 */
#endif	/* defined(SUPPORT_CRT31KHZ) */

static const UINT8 gdcslavesync[6][8] = {
				{0x02,0x26,0x03,0x11,0x86,0x0f,0xc8,0x94},		// 15-L
				{0x02,0x4e,0x4b,0x0c,0x83,0x06,0xe0,0x95},		// 31-H
				{0x02,0x26,0x03,0x11,0x83,0x07,0x90,0x65},		// 24-L
				{0x02,0x4e,0x07,0x25,0x87,0x07,0x90,0x65},		// 24-M
				{0x02,0x26,0x41,0x0c,0x83,0x0d,0x90,0x89},		// 31-L
				{0x02,0x4e,0x47,0x0c,0x87,0x0d,0x90,0x89}};		// 31-M

typedef struct {
	UINT8	lr;
	UINT8	cfi;
} CSRFORM;

static const CSRFORM csrform[4] = {
						{0x07, 0x3b}, {0x09, 0x4b},
						{0x0f, 0x7b}, {0x13, 0x9b}};

#if 0
static const UINT8 sync200l[8] = {0x02,0x26,0x03,0x11,0x86,0x0f,0xc8,0x94};
static const UINT8 sync200m[8] = {0x02,0x26,0x03,0x11,0x83,0x07,0x90,0x65};
static const UINT8 sync400m[8] = {0x02,0x4e,0x07,0x25,0x87,0x07,0x90,0x65};
#endif	/* 0 */

static UINT16 keyget(void) {

	UINT	pos;
	UINT	kbbufhead;

	if (mem[MEMB_KB_COUNT]) {
		mem[MEMB_KB_COUNT]--;
		pos = GETBIOSMEM16(MEMW_KB_BUF_HEAD);
		kbbufhead = pos + 2;
		if (kbbufhead >= 0x522) {
			kbbufhead = 0x502;
		}
		SETBIOSMEM16(MEMW_KB_BUF_HEAD, kbbufhead);
		return(GETBIOSMEM16(pos));
	}
	return(0xffff);
}


// ---- master

void bios0x18_0a(REG8 mode) {

const CRTDATA	*crt;

	gdc_forceready(GDCWORK_MASTER);

	gdc.mode1 &= ~(0x2d);
	mem[MEMB_CRT_STS_FLAG] = mode;
	crt = crtdata;
	if (!(pccore.dipsw[0] & 1)) {
		mem[MEMB_CRT_STS_FLAG] |= 0x80;
		gdc.mode1 |= 0x08;
		crt += 2;
	}
	if (!(mode & 0x01)) {
		crt += 1;						// 25行
	}
	if (mode & 0x02) {
		gdc.mode1 |= 0x04;				// 40桁
	}
	if (mode & 0x04) {
		gdc.mode1 |= 0x01;				// アトリビュート
	}
	if (mode & 0x08) {
		gdc.mode1 |= 0x20;				// コードアクセス
	}
	mem[MEMB_CRT_RASTER] = crt->raster;
	crtc.reg.pl = crt->pl;
	crtc.reg.bl = crt->bl;
	crtc.reg.cl = crt->cl;
	crtc.reg.ssl = 0;
	gdc_restorekacmode();
	bios0x18_10(0);
//#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
//	// np21w ver0.86 rev62 BIOS I/O emulation
//	if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
//		if (!(pccore.dipsw[0] & 1)) {
//			biosioemu_enq8(0x68, 0x08|0x01);
//		}else{
//			biosioemu_enq8(0x68, 0x08|0x00);
//		}
//		if (mode & 0x02) {
//			biosioemu_enq8(0x68, 0x04|0x01);
//		}else{
//			biosioemu_enq8(0x68, 0x04|0x00);
//		}
//		if (mode & 0x04) {
//			biosioemu_enq8(0x68, 0x00|0x01);
//		}else{
//			biosioemu_enq8(0x68, 0x00|0x00);
//		}
//		if (mode & 0x08) {
//			biosioemu_enq8(0x68, 0x20|0x01);
//		}else{
//			biosioemu_enq8(0x68, 0x20|0x00);
//		}
//	}
//#endif
}

void bios0x18_0c(void) {

	if (!(gdcs.textdisp & GDCSCRN_ENABLE)) {
		gdcs.textdisp |= GDCSCRN_ENABLE;
		pcstat.screenupdate |= 2;
 	}
}

static void bios0x18_0f(UINT seg, UINT off, REG8 num, REG8 cnt) {

	UINT8	*p;
	UINT	raster;
	UINT	t;

	SETBIOSMEM16(0x0053e, (UINT16)off);
	SETBIOSMEM16(0x00540, (UINT16)seg);
	mem[0x00547] = num;
	mem[0x0053D] = cnt;
	p = gdc.m.para + GDC_SCROLL + (num << 2);

#if defined(SUPPORT_CRT31KHZ)
	if (mem[MEMB_CRT_BIOS] & 0x80) {
		raster = (mem[MEMB_CRT_RASTER] + 1) << 4;
	}
	else {
#endif
		if (!(mem[MEMB_CRT_STS_FLAG] & 0x01)) {		// 25
			raster = 8 << 4;
		}
		else {										// 20
			raster = 16 << 4;
			}
		if (mem[MEMB_CRT_STS_FLAG] & 0x80) {
			raster <<= 1;
		}
#if defined(SUPPORT_CRT31KHZ)
	}
#endif

	while((cnt--) && (p < (gdc.m.para + GDC_SCROLL + 0x10))) {
		t = MEMR_READ16(seg, off);
		t >>= 1;
		STOREINTELWORD(p, t);
		t = MEMR_READ16(seg, off + 2);
		t *= raster;
		STOREINTELWORD(p + 2, t);
		off += 4;
		p += 4;
	}
	gdcs.textdisp |= GDCSCRN_ALLDRAW2;
	pcstat.screenupdate |= 2;
}

void bios0x18_10(REG8 curdel) {

	UINT8	sts;
	UINT	pos;

	sts = mem[MEMB_CRT_STS_FLAG];
	mem[MEMB_CRT_STS_FLAG] = sts & (~0x40);
	pos = sts & 0x01;
	if (sts & 0x80) {
		pos += 2;
	}
	mem[MEMB_CRT_CNT] = (curdel << 5);
	gdc.m.para[GDC_CSRFORM + 0] = csrform[pos].lr;
	gdc.m.para[GDC_CSRFORM + 1] = curdel << 5;
	gdc.m.para[GDC_CSRFORM + 2] = csrform[pos].cfi;
	gdcs.textdisp |= GDCSCRN_ALLDRAW2 | GDCSCRN_EXT;
}

REG16 bios0x18_14(REG16 seg, REG16 off, REG16 code) {

	UINT16	size;
const UINT8	*p;
	UINT8	buf[32];
	UINT	i;

	switch(code >> 8) {
		case 0x00:			// 8x8
			size = 0x0101;
			MEMR_WRITE16(seg, off, 0x0101);
			p = fontrom + 0x82000 + ((code & 0xff) << 4);
			MEMR_WRITES(seg, off + 2, p, 8);
			break;

//		case 0x28:
		case 0x29:			// 8x16 KANJI
		case 0x2a:
		case 0x2b:
			size = 0x0102;
			MEMR_WRITE16(seg, off, 0x0102);
			p = fontrom;
			p += (code & 0x7f) << 12;
			p += (((code >> 8) - 0x20) & 0x7f) << 4;
			MEMR_WRITES(seg, off + 2, p, 16);
			break;

		case 0x80:			// 8x16 ANK
			size = 0x0102;
			p = fontrom + 0x80000 + ((code & 0xff) << 4);
			MEMR_WRITES(seg, off + 2, p, 16);
			break;

		default:
			size = 0x0202;
			p = fontrom;
			p += (code & 0x7f) << 12;
			p += (((code >> 8) - 0x20) & 0x7f) << 4;
			for (i=0; i<16; i++, p++) {
				buf[i*2+0] = *p;
				buf[i*2+1] = *(p+0x800);
			}
			MEMR_WRITES(seg, off + 2, buf, 32);
			break;
	}
	MEMR_WRITE16(seg, off, size);
	return(size);
}

static void bios0x18_1a(REG16 seg, REG16 off, REG16 code) {

	UINT8	*p;
	UINT8	buf[32];
	UINT	i;

	if (((code >> 8) & 0x7e) == 0x76) {
		MEMR_READS(seg, off + 2, buf, 32);
		p = fontrom;
		p += (code & 0x7f) << 12;
		p += (((code >> 8) - 0x20) & 0x7f) << 4;
		for (i=0; i<16; i++, p++) {
			*p = buf[i*2+0];
			*(p+0x800) = buf[i*2+1];
		}
		cgwindow.writable |= 0x80;
	}
}

void bios0x18_16(REG8 chr, REG8 atr) {

	UINT32	i;

	for (i=0xa0000; i<0xa2000; i+=2) {
		mem[i+0] = chr;
		mem[i+1] = 0;
	}
	for (; i<0xa3fe0; i+=2) {
		mem[i] = atr;
	}
	gdcs.textdisp |= GDCSCRN_ALLDRAW;
}


// ---- 31khz

#if defined(SUPPORT_CRT31KHZ)
static REG8 bios0x18_30(REG8 rate, REG8 scrn) {

	int			crt;
	int			master;
	int			slave;
const CRTDATA	*p;

	if (((rate & 0xf8) != 0x08) || (scrn & (~0x33)) || ((scrn & 3) == 3)) {
		return(0);
	}
	if ((scrn & 0x30) == 0x30) {				// 640x480
#if defined(SUPPORT_PC9821)
		//if (rate & 4) {
		if (rate & 0xc) { // np21w ver0.86 rev47 workaround
//#if defined(BIOS_IO_EMULATION)
//			// XXX: Windows3.1 DOSプロンプト用 無理やり
//			if (CPU_STAT_PM && CPU_STAT_VM86) {
//				biosioemu_enq8(0x6a, 0x21);
//				mem[MEMB_PRXDUPD] |= 0x80;
//				crt = 4;
//				master = 3 + (scrn & 3);
//				slave = 1;
//				biosioemu_enq8(0x6a, 0x69);
//			}else
//#endif	
			{
				gdc_analogext(TRUE);
				mem[MEMB_PRXDUPD] |= 0x80;
				crt = 4;
				master = 3 + (scrn & 3);
				slave = 1;
				gdc.analog |= (1 << GDCANALOG_256E);
			}
		}
		else
#endif
		return(0);
	}
	else {
		if ((scrn & 3) >= 2) {
			return(0);
		}
		if (rate & 4) {							// 31khz
			crt = 2;
			master = 2;
			slave = 4;
		}
		else if (mem[MEMB_PRXCRT] & 0x40) {		// 24khz
			crt = 2;
			master = 1;
			slave = 2;
		}
		else {
			crt = 0;
			master = 0;
			slave = 0;
		}
		if ((scrn & 0x20) && (mem[MEMB_PRXDUPD] & 0x04)) {
			slave += 1;
		}
#if defined(SUPPORT_PC9821)
		else {
//#if defined(BIOS_IO_EMULATION)
//			// XXX: Windows3.1 DOSプロンプト用 無理やり
//			if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
//				biosioemu_enq8(0x6a, 0x20); // これは駄目
//			}else
//#endif	
			{
				gdc_analogext(FALSE);
			}
			mem[MEMB_PRXDUPD] &= ~0x80;
		}
#if defined(BIOS_IO_EMULATION)
		// XXX: Windows3.1 DOSプロンプト用 無理やり
		if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
			biosioemu_enq8(0x6a, 0x68);
		}else
#endif	
		{
			gdc.analog &= ~(1 << (GDCANALOG_256E));
		}
#endif
	}
	crt += (scrn & 3);
	
//#if defined(BIOS_IO_EMULATION)
//	// XXX: Windows3.1 DOSプロンプト用 無理やり
//	if (CPU_STAT_PM && CPU_STAT_VM86) {
//		if (rate & 4) {
//			biosioemu_push8(0x09a8, 0x01);
//		}
//		else {
//			biosioemu_push8(0x09a8, 0x00);
//		}
//	}else
//#endif	
	{
		if (rate & 4) {
			gdc.display |= (1 << GDCDISP_31);
		}
		else {
			gdc.display &= ~(1 << GDCDISP_31);
		}
	}

	CopyMemory(gdc.m.para + GDC_SYNC, gdcmastersync[master], 8);
	ZeroMemory(gdc.m.para + GDC_SCROLL, 4);
	gdc.m.para[GDC_PITCH] = 80;

	p = crtdata + crt;
	gdc.m.para[GDC_CSRFORM + 0] = p->raster;
	gdc.m.para[GDC_CSRFORM + 1] = 0;
	gdc.m.para[GDC_CSRFORM + 2] = (p->raster << 3) + 3;
	crtc.reg.pl = p->pl;
	crtc.reg.bl = p->bl;
	crtc.reg.cl = p->cl;
	crtc.reg.ssl = 0;
	crtc.reg.sur = 1;
	crtc.reg.sdr = 0;

	CopyMemory(gdc.s.para + GDC_SYNC, gdcslavesync[slave], 8);
	ZeroMemory(gdc.s.para + GDC_SCROLL, 4);
	if (slave & 1) {
#if defined(BIOS_IO_EMULATION)
		// XXX: Windows3.1 DOSプロンプト用 無理やり
		if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
			biosioemu_enq8(0xa2, CMD_PITCH);
			biosioemu_enq8(0xa0, 80);
		}else
#endif	
		{
			gdc.s.para[GDC_PITCH] = 80;
		}
		gdc.clock |= 3;
		mem[MEMB_PRXDUPD] |= 0x04;
		gdc.s.para[GDC_SCROLL+3] = 0x40;
	}
	else {
#if defined(BIOS_IO_EMULATION)
		// XXX: Windows3.1 DOSプロンプト用 無理やり
		if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
			biosioemu_enq8(0xa2, CMD_PITCH);
			biosioemu_enq8(0xa0, 40);
		}else
#endif	
		{
			gdc.s.para[GDC_PITCH] = 40;
		}
		gdc.clock &= ~3;
		mem[MEMB_PRXDUPD] &= ~0x04;
	}
	if ((scrn & 0x30) == 0x10) {
		gdc.s.para[GDC_SCROLL+0] = (200*40) & 0xff;
		gdc.s.para[GDC_SCROLL+1] = (200*40) >> 8;
	}
	if ((scrn & 0x20) || (!(mem[MEMB_PRXCRT] & 0x40))) {
		gdc.mode1 &= ~(0x10);
		gdc.s.para[GDC_CSRFORM] = 0;
	}
	else {
		gdc.mode1 |= 0x10;
		gdc.s.para[GDC_CSRFORM] = 1;
	}

#if defined(BIOS_IO_EMULATION)
	// XXX: Windows3.1 DOSプロンプト用 無理やり
	if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
		biosioemu_enq8(0x6a, 0x40 | ((gdc.display & (1 << GDCDISP_PLAZMA)) ? 1 : 0)); // gdcs.textdisp |= GDCSCRN_EXT; の代わり・・・
		biosioemu_enq8(0x6a, 0x82 | (gdc.clock & 1)); // gdcs.grphdisp |= GDCSCRN_EXT;
		biosioemu_enq8(0x62, CMD_STOP); // gdcs.textdisp &= ~GDCSCRN_ENABLE; pcstat.screenupdate |= 2;
	}else
#endif	
	{
		gdcs.textdisp &= ~GDCSCRN_ENABLE;
		gdcs.textdisp |= GDCSCRN_EXT;
		gdcs.grphdisp |= GDCSCRN_EXT;
		pcstat.screenupdate |= 2;
	}
	gdcs.textdisp |= GDCSCRN_ALLDRAW2;
	gdcs.grphdisp |= GDCSCRN_ALLDRAW2;

	mem[0x597] &= ~3;
	mem[0x597] |= (scrn >> 4) & 3;
	mem[MEMB_CRT_STS_FLAG] &= ~0x11;
	if (!(scrn & 1)) {
		mem[MEMB_CRT_STS_FLAG] |= 0x01;
	}
	if (scrn & 2) {
		mem[MEMB_CRT_STS_FLAG] |= 0x10;
	}
	return(5);			// 最後にGDCへ送ったデータ…
}

static REG8 bios0x18_31al(void) {

	UINT8	rate;

	rate = 0x08 + ((gdc.display >> (GDCDISP_31 - 5)) & 4);
	return(rate);
}

static REG8 bios0x18_31bh(void) {

	UINT8	scrn;

	scrn = (mem[0x597] & 3) << 4;
	if (!(mem[MEMB_CRT_STS_FLAG] & 0x01)) {
		scrn |= 0x01;
	}
	if (mem[MEMB_CRT_STS_FLAG] & 0x10) {
		scrn |= 0x02;
	}
	return(scrn);
}
#endif


// ---- slave

void bios0x18_40(void) {

	gdc_forceready(GDCWORK_SLAVE);
	if (!(gdcs.grphdisp & GDCSCRN_ENABLE)) {
		gdcs.grphdisp |= GDCSCRN_ENABLE;
		pcstat.screenupdate |= 2;
	}
	mem[MEMB_PRXCRT] |= 0x80;
}

void bios0x18_41(void) {

	gdc_forceready(GDCWORK_SLAVE);
	if (gdcs.grphdisp & GDCSCRN_ENABLE) {
		gdcs.grphdisp &= ~(GDCSCRN_ENABLE);
		pcstat.screenupdate |= 2;
	}
	mem[MEMB_PRXCRT] &= 0x7f;
}

void bios0x18_42(REG8 mode) {

	UINT8	crtmode;
#if defined(SUPPORT_CRT31KHZ)
	UINT8	rate;
	UINT8	scrn;
#endif
	int		slave;

	gdc_forceready(GDCWORK_MASTER);
	gdc_forceready(GDCWORK_SLAVE);

	crtmode = modenum[mode >> 6];
#if defined(SUPPORT_CRT31KHZ)
	rate = bios0x18_31al();
	scrn = bios0x18_31bh();
	if ((mem[MEMB_CRT_BIOS] & 0x80) &&
		(((scrn & 0x30) == 0x30) || (crtmode == 3))) {
		bios0x18_30(rate, (REG8)((crtmode << 4) + 1));
	}
	else {
#endif
		ZeroMemory(gdc.s.para + GDC_SCROLL, 4);
		if (crtmode == 2) {							// ALL
			crtmode = 2;
			if ((mem[MEMB_PRXDUPD] & 0x24) == 0x20) {
				mem[MEMB_PRXDUPD] ^= 4;
				gdc.clock |= 3;
				CopyMemory(gdc.s.para + GDC_SYNC, gdcslavesync[3], 8);
				gdc.s.para[GDC_PITCH] = 80;
				gdcs.grphdisp |= GDCSCRN_EXT;
				mem[MEMB_PRXDUPD] |= 0x08;
			}
		}
		else {
			if ((mem[MEMB_PRXDUPD] & 0x24) == 0x24) {
				mem[MEMB_PRXDUPD] ^= 4;
				gdc.clock &= ~3;
#if defined(SUPPORT_CRT31KHZ)
				if (rate & 4) slave = 4;
				else
#endif
				slave = (mem[MEMB_PRXCRT] & 0x40)?2:0;
				CopyMemory(gdc.s.para + GDC_SYNC, gdcslavesync[slave], 8);
				gdc.s.para[GDC_PITCH] = 40;
				gdcs.grphdisp |= GDCSCRN_EXT;
				mem[MEMB_PRXDUPD] |= 0x08;
			}
			if (crtmode & 1) {				// UPPER
				gdc.s.para[GDC_SCROLL+0] = (200*40) & 0xff;
				gdc.s.para[GDC_SCROLL+1] = (200*40) >> 8;
			}
		}
		if (mem[MEMB_PRXDUPD] & 4) {
			gdc.s.para[GDC_SCROLL+3] = 0x40;
		}
		if ((crtmode == 2) || (!(mem[MEMB_PRXCRT] & 0x40))) {
			gdc.mode1 &= ~(0x10);
			gdc.s.para[GDC_CSRFORM] = 0;
		}
		else {
			gdc.mode1 |= 0x10;
			gdc.s.para[GDC_CSRFORM] = 1;
		}
#if defined(SUPPORT_CRT31KHZ)
		mem[MEMB_CRT_BIOS] &= ~3;
		mem[MEMB_CRT_BIOS] |= crtmode;
	}
#endif
	if (crtmode != 3) {
		gdcs.disp = (mode >> 4) & 1;
//#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
//		// np21w ver0.86 rev62 BIOS I/O emulation
//		if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
//			biosioemu_enq8(0xa4, (mode >> 4));
//		}
//#endif
	}
	if (!(mode & 0x20)) {
		gdc.mode2 &= ~0x04;
	}
	else {
		gdc.mode2 |= 0x04;
	}
	gdcs.mode2 = gdc.mode2;
	gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
	pcstat.screenupdate |= 2;
}

static void setbiosgdc(UINT32 csrw, const GDCVECT *vect, UINT8 ope) {

	gdc.s.para[GDC_CSRW + 0] = (UINT8)csrw;
	gdc.s.para[GDC_CSRW + 1] = (UINT8)(csrw >> 8);
	gdc.s.para[GDC_CSRW + 2] = (UINT8)(csrw >> 16);

	gdc.s.para[GDC_VECTW] = vect->ope;
	gdc_vectreset(&gdc.s);

	gdc.s.para[GDC_WRITE] = ope;
	mem[MEMB_PRXDUPD] &= ~3;
	mem[MEMB_PRXDUPD] |= ope;
}

/*	↓ここから	何処か(2ch?)で公開されたソース	*/
static void bios0x18_45(void) {

	UCWTBL		ucw;
	UINT		i;
	UINT8		pat[2];
	UINT16		tmp;
	GDCVECT		vect;
	UINT16		GBSX1;
	UINT16		GBSY1;
	UINT16		GBLNG1;
	UINT16		GBWDPA;
	UINT32		csrw;
	UINT8		ope;

	//スレーブの初期化
	gdc_forceready(GDCWORK_SLAVE);

	//BX読み込み
	MEMR_READS(CPU_DS, CPU_BX, &ucw, sizeof(ucw));

	//代入
	GBSX1  = LOADINTELWORD(ucw.GBSX1);
	GBSY1  = LOADINTELWORD(ucw.GBSY1);
	GBWDPA = LOADINTELWORD(ucw.GBWDPA);
	GBLNG1 = LOADINTELWORD(ucw.GBLNG1);

	//200ラインでずらす
	if ((CPU_CH & 0xc0) == 0x40) {
		GBSY1 += 200;
	}

	//描画方向を右下に設定
	vect.ope =0x02 ;

	i = 0;
	for(;;){
		//パターン読み込み
		MEMR_READS(CPU_DS, GBWDPA+i, pat, 1);

		//端数処理(gdcsub_setvectの必要な値だけを使って後は適当)
		if((GBLNG1 - i*8) < 8){
			gdcsub_setvectl(&vect, 1, 1, (GBLNG1 - i*8), 1);
			tmp = 0xFF << (8- (GBLNG1 - i*8));
			tmp &= 0xFF;
			tmp &= pat[0];
			tmp = GDCPATREVERSE(tmp) & 0xFF;
		}else{
			gdcsub_setvectl(&vect, 1, 1, 8, 1);
			tmp = GDCPATREVERSE(pat[0]) & 0xFF;
		}


		//描画開始(0x49の真似)
		csrw = (GBSY1 * 40) + ((GBSX1 + i*8) >> 4);
		csrw += ((GBSX1 + i*8) & 0xf) << 20;

		if ((CPU_CH & 0x30) == 0x30) {
			ope = (ucw.GBON_PTN & 1)?GDCOPE_SET:GDCOPE_CLEAR;
			gdcsub_vectl(csrw + 0x4000, &vect, tmp, ope);
			ope = (ucw.GBON_PTN & 2)?GDCOPE_SET:GDCOPE_CLEAR;
			gdcsub_vectl(csrw + 0x8000, &vect, tmp, ope);
			ope = (ucw.GBON_PTN & 4)?GDCOPE_SET:GDCOPE_CLEAR;
			csrw += 0xc000;
			gdcsub_vectl(csrw, &vect, tmp, ope);
		}
		else {
			ope = ucw.GBDOTU & 3;
			csrw += 0x4000 + ((CPU_CH & 0x30) << 10);
			gdcsub_vectl(csrw, &vect, tmp, ope);
		}

		//次のデータが無ければ抜ける
		i++;
		if(i*8 >= GBLNG1) break;
	}

	// 最後に使った奴を記憶
	setbiosgdc(csrw, &vect, ope);
}
/*	↑ここまで	何処か(2ch?)で公開されたソース	*/

static void bios0x18_47(void) {

	UCWTBL		ucw;
	GDCVECT		vect;
	UINT16		GBSX1;
	UINT16		GBSY1;
	UINT16		GBSX2;
	UINT16		GBSY2;
	GDCSUBFN	func;
	UINT32		csrw;
	UINT16		data;
	UINT16		data2;
	UINT16		GBMDOTI;
	UINT8		ope;
	SINT16		dx;
	SINT16		dy;

	gdc_forceready(GDCWORK_SLAVE);
	MEMR_READS(CPU_DS, CPU_BX, &ucw, sizeof(ucw));
	GBSX1 = LOADINTELWORD(ucw.GBSX1);
	GBSY1 = LOADINTELWORD(ucw.GBSY1);
	GBSX2 = LOADINTELWORD(ucw.GBSX2);
	GBSY2 = LOADINTELWORD(ucw.GBSY2);
	ZeroMemory(&vect, sizeof(vect));
	data = 0;
	data2 = 0;
	if (ucw.GBDTYP == 0x01) {
		func = gdcsub_vectl;
		gdcsub_setvectl(&vect, GBSX1, GBSY1, GBSX2, GBSY2);
	}
	else if (ucw.GBDTYP <= 0x02) {
		func = gdcsub_vectr;
		vect.ope = 0x40 + (ucw.GBDSP & 7);
		dx = GBSX2 - GBSX1;
		if (dx < 0) {
			dx = 0 - dx;
		}
		dy = GBSY2 - GBSY1;
		if (dy < 0) {
			dy = 0 - dy;
		}
		switch(ucw.GBDSP & 3) {
			case 0:
				data = dy;
				data2 = dx;
				break;

			case 1:
				data2 = (UINT16)dx + (UINT16)dy;
				data2 >>= 1;
				data = (UINT16)dx - (UINT16)dy;
				data = (data >> 1) & 0x3fff;
				break;

			case 2:
				data = dx;
				data2 = dy;
				break;

			case 3:
				data2 = (UINT16)dx + (UINT16)dy;
				data2 >>= 1;
				data = (UINT16)dy - (UINT16)dx;
				data = (data >> 1) & 0x3fff;
				break;
		}
		STOREINTELWORD(vect.DC, 3);
		STOREINTELWORD(vect.D, data);
		STOREINTELWORD(vect.D2, data2);
		STOREINTELWORD(vect.D1, 0xffff);
		STOREINTELWORD(vect.DM, data);
	}
	else {
		func = gdcsub_vectc;
		vect.ope = 0x20 + (ucw.GBDSP & 7);
		vect.DC[0] = ucw.GBLNG1[0];
		vect.DC[1] = ucw.GBLNG1[1];
//		data = LOADINTELWORD(ucw.GBLNG2) - 1;
		data = LOADINTELWORD(ucw.GBCIR) - 1;
		STOREINTELWORD(vect.D, data);
		data >>= 1;
		STOREINTELWORD(vect.D2, data);
		STOREINTELWORD(vect.D1, 0x3fff);
		if (ucw.GBDTYP == 0x04) {
			vect.DM[0] = ucw.GBMDOT[0];
			vect.DM[1] = ucw.GBMDOT[1];
		}
	}
	if ((CPU_CH & 0xc0) == 0x40) {
		GBSY1 += 200;
	}
	csrw = (GBSY1 * 40) + (GBSX1 >> 4);
	csrw += (GBSX1 & 0xf) << 20;
	GBMDOTI = (GDCPATREVERSE(ucw.GBMDOTI[0]) << 8) +
											GDCPATREVERSE(ucw.GBMDOTI[1]);
	if ((CPU_CH & 0x30) == 0x30) {
		ope = (ucw.GBON_PTN & 1)?GDCOPE_SET:GDCOPE_CLEAR;
		func(csrw + 0x4000, &vect, GBMDOTI, ope);
		ope = (ucw.GBON_PTN & 2)?GDCOPE_SET:GDCOPE_CLEAR;
		func(csrw + 0x8000, &vect, GBMDOTI, ope);
		ope = (ucw.GBON_PTN & 4)?GDCOPE_SET:GDCOPE_CLEAR;
		csrw += 0xc000;
		func(csrw, &vect, GBMDOTI, ope);
	}
	else {
		ope = ucw.GBDOTU & 3;
		csrw += 0x4000 + ((CPU_CH & 0x30) << 10);
		func(csrw, &vect, GBMDOTI, ope);
	}

	// 最後に使った奴を記憶
	*(UINT16 *)(mem + MEMW_PRXGLS) = *(UINT16 *)(ucw.GBMDOTI);
	STOREINTELWORD(mem + GDC_TEXTW, GBMDOTI);
	setbiosgdc(csrw, &vect, ope);
}

static void bios0x18_49(void) {

	UCWTBL		ucw;
	UINT		i;
	UINT8		pat[8];
	UINT16		tmp;
	GDCVECT		vect;
	UINT16		GBSX1;
	UINT16		GBSY1;
	UINT32		csrw;
	UINT8		ope;

	gdc_forceready(GDCWORK_SLAVE);

	MEMR_READS(CPU_DS, CPU_BX, &ucw, sizeof(ucw));
	for (i=0; i<8; i++) {
		mem[MEMW_PRXGLS + i] = ucw.GBMDOTI[i];
		pat[i] = GDCPATREVERSE(ucw.GBMDOTI[i]);
		gdc.s.para[GDC_TEXTW + i] = pat[i];
	}
	vect.ope = 0x10 + (ucw.GBDSP & 7);
	if (*(UINT16 *)ucw.GBLNG1) {
		tmp = (LOADINTELWORD(ucw.GBLNG2) - 1) & 0x3fff;
		STOREINTELWORD(vect.DC, tmp);
		vect.D[0] = ucw.GBLNG1[0];
		vect.D[1] = ucw.GBLNG1[1];
	}
	else {
		STOREINTELWORD(vect.DC, 7);
		STOREINTELWORD(vect.D, 7);
	}

	GBSX1 = LOADINTELWORD(ucw.GBSX1);
	GBSY1 = LOADINTELWORD(ucw.GBSY1);
	if ((CPU_CH & 0xc0) == 0x40) {
		GBSY1 += 200;
	}
	csrw = (GBSY1 * 40) + (GBSX1 >> 4);
	csrw += (GBSX1 & 0xf) << 20;
	if ((CPU_CH & 0x30) == 0x30) {
		ope = (ucw.GBON_PTN & 1)?GDCOPE_SET:GDCOPE_CLEAR;
		gdcsub_text(csrw + 0x4000, &vect, pat, ope);
		ope = (ucw.GBON_PTN & 2)?GDCOPE_SET:GDCOPE_CLEAR;
		gdcsub_text(csrw + 0x8000, &vect, pat, ope);
		ope = (ucw.GBON_PTN & 4)?GDCOPE_SET:GDCOPE_CLEAR;
		csrw += 0xc000;
		gdcsub_text(csrw, &vect, pat, ope);
	}
	else {
		ope = ucw.GBDOTU & 3;
		csrw += 0x4000 + ((CPU_CH & 0x30) << 10);
		gdcsub_text(csrw, &vect, pat, ope);
	}

	// 最後に使った奴を記憶
	setbiosgdc(csrw, &vect, ope);
}


// ---- PC-9821

#if defined(SUPPORT_PC9821)
static void bios0x18_4d(REG8 mode) {

	if ((mem[0x45c] & 0x40) &&
		((mem[MEMB_CRT_BIOS] & 3) == 2)) {
		if (mode == 0) {
			gdc_analogext(FALSE);
			mem[MEMB_PRXDUPD] &= ~0x7f;
			mem[MEMB_PRXDUPD] |= 0x04;
		}
		else if (mode == 1) {
			gdc_analogext(TRUE);
			mem[MEMB_PRXDUPD] |= 0x80;
		}
		else {
			mem[MEMB_PRXDUPD] |= 0x04;
		}
	}
}
#endif


// ----

void bios0x18(void) {

	union {
		BOOL	b;
		REG8	r8;
		UINT16	w;
		UINT32	d;
		UINT8	col[4];
	}		tmp;
	int		i;

#if 0
	TRACEOUT(("int18 AX=%.4x %.4x:%.4x", CPU_AX,
							MEMR_READ16(CPU_SS, CPU_SP+2),
							MEMR_READ16(CPU_SS, CPU_SP)));
#endif
	
	switch(CPU_AH) {
		case 0x00:						// キー・データの読みだし
			if (mem[MEMB_KB_COUNT]) {
				CPU_AX = keyget();
			}
			else {
				CPU_IP--;
				CPU_REMCLOCK = -1;
				break;
			}
			break;

   		case 0x01:						// キー・バッファ状態のセンス
			if (mem[MEMB_KB_COUNT]) {
				tmp.d = GETBIOSMEM16(MEMW_KB_BUF_HEAD);
				CPU_AX = GETBIOSMEM16(tmp.d);
				CPU_BH = 1;
			}
			else {
				CPU_BH = 0;
			}
			break;

   		case 0x02:						// シフト・キー状態のセンス
			CPU_AL = mem[MEMB_SHIFT_STS];
			break;

   		case 0x03:						// キーボード・インタフェイスの初期化
#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
			// np21w ver0.86 rev47 BIOS I/O emulation
			if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
				biosioemu_enq8(0x43, 0x3a);
				biosioemu_enq8(0x43, 0x32);
				biosioemu_enq8(0x43, 0x16);
				ZeroMemory(mem + 0x00502, 0x20);
				ZeroMemory(mem + 0x00528, 0x13);
				SETBIOSMEM16(MEMW_KB_SHIFT_TBL, 0x0e00);
				SETBIOSMEM16(MEMW_KB_BUF_HEAD, 0x0502);
				SETBIOSMEM16(MEMW_KB_BUF_TAIL, 0x0502);
				SETBIOSMEM16(MEMW_KB_CODE_OFF, 0x0e00);
				SETBIOSMEM16(MEMW_KB_CODE_SEG, 0xfd80);
			}else
#endif
			{
				bios0x09_init();
			}
			break;

   		case 0x04:						// キー入力状態のセンス
			CPU_AH = mem[MEMX_KB_KY_STS + (CPU_AL & 0x0f)];
 			break;

   		case 0x05:						// キー入力センス
			if (mem[MEMB_KB_COUNT]) {
				CPU_AX = keyget();
				CPU_BH = 1;
			}
			else {
				CPU_BH = 0;
			}
 			break;

   		case 0x0a:						// CRTモードの設定(15/24khz)
			bios0x18_0a(CPU_AL);
			break;

   		case 0x0b:						// CRTモードのセンス
			CPU_AL = mem[MEMB_CRT_STS_FLAG];
 			break;

   		case 0x0c:						// テキスト画面の表示開始
#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
			// np21w ver0.86 rev47 BIOS I/O emulation
			if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
				if (!(gdcs.textdisp & GDCSCRN_ENABLE)) {
					pcstat.screenupdate |= 2;
 				}
				biosioemu_push8(0x62, CMD_START);
			} else
#endif
			{
				bios0x18_0c();
			}
 			break;

   		case 0x0d:						// テキスト画面の表示終了
#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
			// np21w ver0.86 rev47 BIOS I/O emulation
			if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
				if (gdcs.textdisp & GDCSCRN_ENABLE) {
					pcstat.screenupdate |= 2;
				}
				biosioemu_push8(0x62, CMD_STOP);
			} else
#endif
			{
				if (gdcs.textdisp & GDCSCRN_ENABLE) {
					gdcs.textdisp &= ~(GDCSCRN_ENABLE);
					pcstat.screenupdate |= 2;
				}
			}
 			break;

		case 0x0e:						// 一つの表示領域の設定
			gdc_forceready(GDCWORK_MASTER);
			ZeroMemory(&gdc.m.para[GDC_SCROLL], 16);
			tmp.w = CPU_DX >> 1;
			SETBIOSMEM16(MEMW_CRT_W_VRAMADR, tmp.w);
			STOREINTELWORD(gdc.m.para + GDC_SCROLL + 0, tmp.w);
			tmp.w = 200 << 4;
			if (mem[MEMB_CRT_STS_FLAG] & 0x80) {
				tmp.w <<= 1;
			}
			SETBIOSMEM16(MEMW_CRT_W_RASTER, tmp.w);
			STOREINTELWORD(gdc.m.para + GDC_SCROLL + 2, tmp.w);
			gdcs.textdisp |= GDCSCRN_ALLDRAW2;
//			pcstat.screenupdate |= 2;
 			break;

		case 0x0f:						// 複数の表示領域の設定
			gdc_forceready(GDCWORK_MASTER);
			bios0x18_0f(CPU_BX, CPU_CX, CPU_DH, CPU_DL);
			break;

   		case 0x10:						// カーソルタイプの設定(15/24khz)
			gdc_forceready(GDCWORK_MASTER);
			bios0x18_10((REG8)(CPU_AL & 1));
 			break;

   		case 0x11:						// カーソルの表示開始
			gdc_forceready(GDCWORK_MASTER);
			if (gdc.m.para[GDC_CSRFORM] != (mem[MEMB_CRT_RASTER] | 0x80)) {
				gdc.m.para[GDC_CSRFORM] = mem[MEMB_CRT_RASTER] | 0x80;
			}
			gdcs.textdisp |= GDCSCRN_ALLDRAW | GDCSCRN_EXT;
			break;

   		case 0x12:						// カーソルの表示停止
			gdc_forceready(GDCWORK_MASTER);
			if (gdc.m.para[GDC_CSRFORM] != mem[MEMB_CRT_RASTER]) {
				gdc.m.para[GDC_CSRFORM] = mem[MEMB_CRT_RASTER];
				gdcs.textdisp |= GDCSCRN_ALLDRAW | GDCSCRN_EXT;
			}
			break;

   		case 0x13:						// カーソル位置の設定
			gdc_forceready(GDCWORK_MASTER);
			tmp.w = CPU_DX >> 1;
			if (LOADINTELWORD(gdc.m.para + GDC_CSRW) != tmp.w) {
				STOREINTELWORD(gdc.m.para + GDC_CSRW, tmp.w);
				gdcs.textdisp |= GDCSCRN_EXT;
			}
 			break;

   		case 0x14:						// フォントパターンの読み出し
			bios0x18_14(CPU_BX, CPU_CX, CPU_DX);
 			break;

 		case 0x15:						// ライトペン位置読みだし
 			break;

   		case 0x16:						// テキストVRAMの初期化
			bios0x18_16(CPU_DL, CPU_DH);
 			break;

		case 0x17:						// ブザーの起呼
			iocore_out8(0x37, 0x06);
			break;

		case 0x18:						// ブザーの停止
			iocore_out8(0x37, 0x07);
			break;

		case 0x19:						// ライトペン押下状態の初期化
			break;

   		case 0x1a:						// ユーザー文字の定義
			bios0x18_1a(CPU_BX, CPU_CX, CPU_DX);
			break;

		case 0x1b:						// KCGアクセスモードの設定
			switch(CPU_AL) {
				case 0:
					mem[MEMB_CRT_STS_FLAG] &= ~0x08;
					gdc.mode1 &= ~0x20;
					gdc_restorekacmode();
					break;

				case 1:
					mem[MEMB_CRT_STS_FLAG] |= 0x08;
					gdc.mode1 |= 0x20;
					gdc_restorekacmode();
					break;
			}
			break;
#if defined(SUPPORT_CRT31KHZ)
		case 0x30:
			if (mem[MEMB_CRT_BIOS] & 0x80) {
				gdc_forceready(GDCWORK_MASTER);
				gdc_forceready(GDCWORK_SLAVE);
				tmp.r8 = bios0x18_30(CPU_AL, CPU_BH);
				CPU_AH = tmp.r8;
				if (tmp.r8 == 0x05) {
					CPU_AL = 0;
					CPU_BH = 0;
				}
				else {
					CPU_AL = 1;
					CPU_BH = 1;
				}
			}
			break;

		case 0x31:
			if (mem[MEMB_CRT_BIOS] & 0x80) {
				CPU_AL = bios0x18_31al();
				CPU_BH = bios0x18_31bh();
			}
			break;
#endif
   		case 0x40:						// グラフィック画面の表示開始
#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
			// np21w ver0.86 rev47 BIOS I/O emulation
			if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
				gdc_forceready(GDCWORK_SLAVE);
				if (!(gdcs.grphdisp & GDCSCRN_ENABLE)) {
					pcstat.screenupdate |= 2;
				}
				biosioemu_push8(0xa2, CMD_START); 
				mem[MEMB_PRXCRT] |= 0x80;
			} else
#endif
			{
				bios0x18_40();
			}
 			break;

   		case 0x41:						// グラフィック画面の表示終了
#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
			// np21w ver0.86 rev47 BIOS I/O emulation
			if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
				gdc_forceready(GDCWORK_SLAVE);
				if (gdcs.grphdisp & GDCSCRN_ENABLE) {
					pcstat.screenupdate |= 2;
				}
				biosioemu_push8(0xa2, CMD_STOP);
				mem[MEMB_PRXCRT] &= 0x7f;
			} else
#endif
			{
				bios0x18_41();
			}
 			break;

   		case 0x42:						// 表示領域の設定
			bios0x18_42(CPU_CH);
 			break;

		case 0x43:						// パレットの設定
			MEMR_READS(CPU_DS, CPU_BX + offsetof(UCWTBL, GBCPC), tmp.col, 4);
			for (i=0; i<4; i++) {
				gdc_setdegitalpal(6 - (i*2), (REG8)(tmp.col[i] >> 4));
				gdc_setdegitalpal(7 - (i*2), (REG8)(tmp.col[i] & 15));
			}
			break;

		case 0x44:						// ボーダカラーの設定
//			if (!(mem[MEMB_PRXCRT] & 0x40)) {
//				color = MEMR_READ8(CPU_DS, CPU_BX + 1);
//			}
			break;
		case 0x45:	/*	int18,45hを追加(from Kai2, np21w ver0.86 rev16)	*/
			bios0x18_45();
			break;
		case 0x46:
			TRACEOUT(("unsupport bios 18-%.2x", CPU_AH));
			break;

		case 0x47:						// 直線、矩形の描画
		case 0x48:						// 円の描画
			bios0x18_47();
			break;

		case 0x49:						// グラフィック文字の描画
			bios0x18_49();
			break;

		case 0x4a:						// 描画モードの設定
			if (!(mem[MEMB_PRXCRT] & 0x01)) {
				gdc.s.para[GDC_SYNC] = CPU_CH;
				gdcs.grphdisp |= GDCSCRN_EXT;
				if (CPU_CH & 0x10) {
					mem[MEMB_PRXDUPD] &= ~0x08;
				}
				else {
					mem[MEMB_PRXDUPD] |= 0x08;
				}
			}
			break;
#if defined(SUPPORT_PC9821)
		case 0x4d:
			bios0x18_4d(CPU_CH);
			break;
#endif
	}
}


