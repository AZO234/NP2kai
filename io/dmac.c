/**
 * @file	dmac.c
 * @brief	Implementation of the DMA controller
 */

#include "compiler.h"
#include "dmac.h"
#include	"cpucore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"cs4231.h"
#include	"sasiio.h"

void DMACCALL dma_dummyout(REG8 data) {
	
	TRACEOUT(("dma_dummyout"));
	(void)data;
}

REG8 DMACCALL dma_dummyin(void) {
	TRACEOUT(("dma_dummyin"));
	return(0xff);
}

REG8 DMACCALL dma_dummyproc(REG8 func) {

	(void)func;
	return(0);
}

static const DMAPROC dmaproc[] = {
		{dma_dummyout,		dma_dummyin,		dma_dummyproc},		// NONE
		{fdc_datawrite,		fdc_dataread,	fdc_dmafunc},		// 2HD
		{fdc_datawrite,		fdc_dataread,	fdc_dmafunc},		// 2DD
#if defined(SUPPORT_SASI)
		{sasi_datawrite,		sasi_dataread,	sasi_dmafunc},		// SASI
#else
		{dma_dummyout,		dma_dummyin,		dma_dummyproc},		// SASI
#endif
		{dma_dummyout,		dma_dummyin,		dma_dummyproc},		// SCSI
#if !defined(DISABLE_SOUND)
		{dma_dummyout,		dma_dummyin,		cs4231dmafunc},		// CS4231
#else
		{dma_dummyout,		dma_dummyin,		dma_dummyproc},		// SASI
#endif
};


// ----
UINT8 bank;
UINT8 bank2;
void dmac_check(void) {

	BOOL	workchg;
	DMACH	ch;
	REG8	bit;

	workchg = FALSE;
	ch = dmac.dmach;
	bit = 1;
	do {
		if ((!(dmac.mask & bit)) && (ch->ready)) {
			if (!(dmac.work & bit)) {
				dmac.work |= bit;
				if (ch->proc.extproc(DMAEXT_START)) {
					dmac.stat &= ~bit;
					dmac.working |= bit;
					workchg = TRUE;
				}
			}
		}
		else {
			if (dmac.work & bit) {
				dmac.work &= ~bit;
				dmac.working &= ~bit;
				ch->proc.extproc(DMAEXT_BREAK);
				workchg = TRUE;
			}
		}
		bit <<= 1;
		ch++;
	} while(bit & 0x0f);
	if (workchg) {
		nevent_forceexit();
	}
}

UINT dmac_getdatas(DMACH dmach, UINT8 *buf, UINT size) {

	UINT	leng;
	UINT32	addr;
	UINT	i;

	leng = min(dmach->leng.w, size);
	if (leng) {
		addr = dmach->adrs.d;					// + mask
		if (!(dmach->mode & 0x20)) {			// dir +
			for (i=0; i<leng; i++) {
				buf[i] = MEMP_READ8(addr + i);
			}
			dmach->adrs.d += leng;
		}
		else {									// dir -
			for (i=0; i<leng; i++) {
				buf[i] = MEMP_READ8(addr - i);
			}
			dmach->adrs.d -= leng;
		}
		dmach->leng.w -= leng;
		if (dmach->leng.w == 0) {
			dmach->proc.extproc(DMAEXT_END);
		}
	}
	return(leng);
}

// ---- I/O

static void IOOUTCALL dmac_o01(UINT port, REG8 dat) {

	DMACH	dmach;
	int		lh;

	dmach = dmac.dmach + ((port >> 2) & 3);
	lh = dmac.lh;
	dmac.lh = (UINT8)(lh ^ 1);
	dmach->adrs.b[lh + DMA32_LOW] = dat;
	dmach->adrsorg.b[lh] = dat;
	dmach->startaddr = dmach->adrs.d;
}

static void IOOUTCALL dmac_o03(UINT port, REG8 dat) {

	int		ch;
	DMACH	dmach;
	int		lh;

	ch = (port >> 2) & 3;
	dmach = dmac.dmach + ch;
	lh = dmac.lh;
	dmac.lh = lh ^ 1;
	dmach->leng.b[lh] = dat;
	dmach->lengorg.b[lh] = dat;
	dmac.stat &= ~(1 << ch);
	//dmach->startcount =dmach->leng.w;
	//dmach->lastaddr = dmach->startaddr + dmach->leng.w;
	dmach->startcount = dmach->leng.w;
	dmach->lastaddr = dmach->startaddr + dmach->leng.w;
}

static void IOOUTCALL dmac_o13_(UINT port, REG8 dat) {
	if (dat& 4)
	dmac.stat |= 1 << ((dat & 3) +4);
	else
	dmac.stat &= ~(1 << ((dat & 3) +4));
	dmac.stat &=~(1 << (dat & 3));
	dmac_check();
	(void)port;
}


static void IOOUTCALL dmac_o13(UINT port, REG8 dat) {
	dmac.dmach[dat & 3].sreq = dat;
	dmac_check();
	(void)port;
}

static void IOOUTCALL dmac_o15(UINT port, REG8 dat) {

	if (dat & 4) {
		dmac.mask |= (1 << (dat & 3));
	}
	else {
		dmac.mask &= ~(1 << (dat & 3));
	}
	dmac_check();
	(void)port;
}

static void IOOUTCALL dmac_o17(UINT port, REG8 dat) {

	dmac.dmach[dat & 3].mode = dat;
	(void)port;
}

static void IOOUTCALL dmac_o19(UINT port, REG8 dat) {

	dmac.lh = DMA16_LOW;
	(void)port;
	(void)dat;
}

static void IOOUTCALL dmac_o1b(UINT port, REG8 dat) {

	dmac.mask = 0x0f;
	dmac.dmach[0].adrs.d = 0;
	dmac.dmach[0].leng.w = 0;
	dmac.dmach[1].adrs.d = 0;
	dmac.dmach[1].leng.w = 0;
	dmac.dmach[2].adrs.d = 0;
	dmac.dmach[2].leng.w = 0;
	dmac.dmach[3].adrs.d = 0;
	dmac.dmach[3].leng.w = 0;
	(void)port;
	(void)dat;
}

static void IOOUTCALL dmac_o1d(UINT port, REG8 dat) {

	dmac.mask = 0;
	dmac_check();
	(void)port;
	(void)dat;
}

static void IOOUTCALL dmac_o1f(UINT port, REG8 dat) {

	dmac.mask = dat;
	dmac_check();
	(void)port;
}

static void IOOUTCALL dmac_o21(UINT port, REG8 dat) {//バンク設定

	DMACH	dmach;
	dmach = dmac.dmach + (((port >> 1) + 1) & 3);
#if defined(CPUCORE_IA32)
	dmach->adrs.b[DMA32_HIGH + DMA16_LOW] = dat;
#else
	// IA16では ver0.75で無効、ver0.76で修正
	dmach->adrs.b[DMA32_HIGH + DMA16_LOW] = dat & 0x0f;//V30は20bitまで
#endif
	dmach->adrs.b[DMA32_HIGH + DMA16_HIGH] = 0;//25bitより上はここでは転送できない→0E05
	bank = dat;
	if (dmach->bound != 3){
	dmach->startaddr = dmach->adrs.d;
	dmach->lastaddr = (bank <<16 | dmach->lastaddr);
	}
	else
	{
	dmach->startaddr = bank << 16 | dmach->adrs.d;
	dmach->lastaddr = (bank <<16 | dmach->lastaddr);
	TRACEOUT(("port =%x ch=%x bank = %x dma_address = %x\n",port,((port >> 1) + 1) & 3, dat, dmach->adrs.d));
	}
}

static void IOOUTCALL dmac_o29(UINT port, REG8 dat) {

	DMACH	dmach;

	dmach = dmac.dmach + (dat & 3);
//	dmach = dmac.dmach + (dat & 0xf);
//	TRACEOUT (("dmach %x",dat));// PC-98は4chしか持ってない
	dmach->bound = (dat >> 2) & 3;
	(void)port;
	TRACEOUT(("port =%x ch= %x dma bound =%x\n",port,dat & 03 ,dmach->bound));
}

static REG8 IOINPCALL dmac_i01(UINT port) {

	DMACH	dmach;
	int		lh;

	dmach = dmac.dmach + ((port >> 2) & 3);
	lh = dmac.lh;
	dmac.lh = lh ^ 1;
	return(dmach->adrs.b[lh + DMA32_LOW]);
}

static REG8 IOINPCALL dmac_i03(UINT port) {

	DMACH	dmach;
	int		lh;

	dmach = dmac.dmach + ((port >> 2) & 3);
	lh = dmac.lh;
	dmac.lh = lh ^ 1;
	return(dmach->leng.b[lh]);
}

static REG8 IOINPCALL dmac_i11(UINT port) {

	(void)port;
	return(dmac.stat &= 0xf0);												// ToDo!!
}
static void IOOUTCALL dmac_oe05(UINT port, REG8 dat) {

	DMACH	dmach;

//	dmach = dmac.dmach + (((port >> 1) - 2) & 3);//qemu
//	dmach = dmac.dmach + ((port >> 1) & 0x07) - 2;//np21/w

	char channel;
	//switch(port){//シフト計算は頭がおかしくなるのでswitchでごまかしました
	//	case 0xe05:channel = 0;break;
	//	case 0xe07:channel = 1;break;
	//	case 0xe09:channel = 2;break;
	//	case 0xe0b:channel = 3;
	//}
	channel = ((port - 0x05) & 0x07) >> 1;
	dmach = dmac.dmach + channel;
	dmach->adrs.b[DMA32_HIGH + DMA16_HIGH] = dat;
	bank2 = dat;
	dmach->startaddr = ((bank2 &0x7f) << 24) | dmach->adrs.d;
	dmach->lastaddr = ((bank2 &0x7f) << 24) | (dmach->lastaddr);
	TRACEOUT(("32bit DMA ch %x bank %x\n",channel, dmach->adrs.b[3]));	
}
static void IOOUTCALL dmac_o2b(UINT port, REG8 dat) {
	(void)port;
}

// ---- I/F

static const IOOUT dmaco00[16] = {
					dmac_o01,	dmac_o03,	dmac_o01,	dmac_o03,
					dmac_o01,	dmac_o03,	dmac_o01,	dmac_o03,
					NULL,		dmac_o13,	dmac_o15,	dmac_o17,
					dmac_o19,	dmac_o1b,	dmac_o1d,	dmac_o1f};

static const IOINP dmaci00[16] = {
					dmac_i01,	dmac_i03,	dmac_i01,	dmac_i03,
					dmac_i01,	dmac_i03,	dmac_i01,	dmac_i03,
					dmac_i11,	NULL,		NULL,		NULL,
					NULL,		NULL,		NULL,		NULL};

static const IOOUT dmaco21[8] = {
					dmac_o21,	dmac_o21,	dmac_o21,	dmac_o21,
					dmac_o29,	NULL,		NULL,		NULL};

void dmac_reset(const NP2CFG *pConfig) {

	ZeroMemory(&dmac, sizeof(dmac));
	dmac.lh = DMA16_LOW;
	dmac.mask = 0xf;
	dmac_procset();
	TRACEOUT(("sizeof(_DMACH) = %d", sizeof(_DMACH)));
	(void)pConfig;
}

void dmac_bind(void) {

	iocore_attachsysoutex(0x0001, 0x0ce1, dmaco00, 16);
	iocore_attachsysinpex(0x0001, 0x0ce1, dmaci00, 16);
	iocore_attachsysoutex(0x0021, 0x0cf1, dmaco21, 8);
	iocore_attachout(0x0e05, dmac_oe05); //DMA ch.0
	iocore_attachout(0x0e07, dmac_oe05); //DMA ch.1
	iocore_attachout(0x0e09, dmac_oe05); //DMA ch.2
	iocore_attachout(0x0e0b, dmac_oe05); //DMA ch.3
//0x489はnecio.cで使ってる
//PC-H98??? EMM386.exeを組み込むと現れる
//	iocore_attachout(0x2b,dmac_o11);
//	iocore_attachinp(0x2d,dmac_i11);//when 0x2b = c8???
}


// ----

static void dmacset(REG8 channel) {

	DMADEV		*dev;
	DMADEV		*devterm;
	UINT		dmadev;

	dev = dmac.device;
	devterm = dev + dmac.devices;
	dmadev = DMADEV_NONE;
	while(dev < devterm) {
		if (dev->channel == channel) {
			dmadev = dev->device;
		}
		dev++;
	}
	if (dmadev >= NELEMENTS(dmaproc)) {
		dmadev = 0;
	}
	
	switch(dmadev){
		case 0:TRACEOUT(("dmac set %d - dummy", channel));break;
		case 1:TRACEOUT(("dmac set %d - 2HD", channel));break;
		case 2:TRACEOUT(("dmac set %d - 2DD", channel));break;
		case 3:TRACEOUT(("dmac set %d - SASI", channel));break;
		case 4:TRACEOUT(("dmac set %d - SCSI", channel));break;
		case 5:TRACEOUT(("dmac set %d - cs4231p", channel));break;
	}
	dmac.dmach[channel].proc = dmaproc[dmadev];
}

void dmac_procset(void) {

	REG8	i;

	for (i=0; i<4; i++) {
		dmacset(i);
	}
}

void dmac_attach(REG8 device, REG8 channel) {

	dmac_detach(device);

	if (dmac.devices < NELEMENTS(dmac.device)) {
		dmac.device[dmac.devices].device = device;
		dmac.device[dmac.devices].channel = channel;
		dmac.devices++;
		dmacset(channel);
	}
}

void dmac_detach(REG8 device) {

	DMADEV	*dev;
	DMADEV	*devterm;
	REG8	ch;

	dev = dmac.device;
	devterm = dev + dmac.devices;
	while(dev < devterm) {
		if (dev->device == device) {
			break;
		}
		dev++;
	}
	if (dev < devterm) {
		ch = dev->channel;
		dev++;
		while(dev < devterm) {
			*(dev - 1) = *dev;
			dev++;
		}
		dmac.devices--;
		//if (ch < 5)
		dmacset(ch);
	}
}