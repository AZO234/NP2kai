#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"ideio.h"
#include	"cs4231io.h"
#include	"iocore16.tbl"
#ifdef SUPPORT_LGY98
#include	"network/lgy98.h"
#endif
#ifdef SUPPORT_WAB
#include	"wab/wab.h"
#endif
#ifdef SUPPORT_CL_GD5430
#include	"wab/cirrus_vga_extern.h"
#endif
#ifdef SUPPORT_SMPU98
#include	"cbus/mpu98ii.h"
#include	"cbus/smpu98.h"
#endif


	_ARTIC		artic;
	_CGROM		cgrom;
	_CGWINDOW	cgwindow;
	_CRTC		crtc;
	_DMAC		dmac;
	_EGC		egc;
	_EPSONIO	epsonio;
	_EMSIO		emsio;
	_FDC		fdc;
	_GDC		gdc;
	_GDCS		gdcs;
	_GRCG		grcg;
	_KEYBRD		keybrd;
	_MOUSEIF	mouseif;
	_NECIO		necio;
	_NMIIO		nmiio;
	_NP2SYSP	np2sysp;
	_PIC		pic;
	_PIT		pit;
	_RS232C		rs232c;
#if defined(SUPPORT_RS232C_FIFO)
	_RS232CFIFO	rs232cfifo;
#endif
	_SYSPORT	sysport;
	_UPD4990	uPD4990;

#if defined(SUPPORT_PC9821)
	_PCIDEV		pcidev;
#endif
#if defined(SUPPORT_PEGC)
	_PEGC		pegc;
#endif


// ----

enum {
	IOFUNC_SYS	= 0x01,
	IOFUNC_SND	= 0x02,
	IOFUNC_EXT	= 0x04
};

typedef struct {
	IOOUT	ioout[256];
	IOINP	ioinp[256];
	UINT	type;
	UINT	port;
} _IOFUNC, *IOFUNC;

typedef struct {
	IOFUNC		base[256];
	UINT		busclock;
	LISTARRAY	iotbl;
} _IOCORE, *IOCORE;

static	_IOCORE		iocore;
static	UINT8		ioterminate[0x100];

//extern UINT lgy98_baseaddr;
//extern UINT ne2000_baseaddr;

// ----

static void IOOUTCALL defout8(UINT port, REG8 dat) {

#if !defined(DISABLE_SOUND)
	if(cs4231.enable){
		UINT	tmp;

		tmp = port - cs4231.port[0];
		if (tmp < 8) {
			cs4231io0_w8(port, dat);
			return;
		}
		tmp = port - cs4231.port[5];
		if (tmp < 2) {
			cs4231io5_w8(port, dat);
			return;
		}
	}
#endif
	if ((port & 0xf0ff) == 0x801e) {
		dipsw_w8(port, dat);
		return;
	}
	//TRACEOUT(("defout8 - %x %x %.4x %.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL definp8(UINT port) {

#if !defined(DISABLE_SOUND)
	if(cs4231.enable){
		UINT	tmp;

		tmp = port - cs4231.port[0];
		if (tmp < 8) {
			return(cs4231io0_r8(port));
		}
		tmp = port - cs4231.port[5];
		if (tmp < 2) {
			return(cs4231io5_r8(port));
		}
	}
#endif
	if ((port & 0xf0ff) == 0x801e) {
		return(dipsw_r8(port));
	}
	//TRACEOUT(("definp8 - %x %.4x %.4x", port, CPU_CS, CPU_IP));
	return(0xff);
}


static void attachout(IOFUNC iof, UINT port, IOOUT func) {

	if (func) {
		iof->ioout[port] = func;
	}
}
static void detachout(IOFUNC iof, UINT port) {

	iof->ioout[port] = &defout8;
}

static void attachinp(IOFUNC iof, UINT port, IOINP func) {

	if (func) {
		iof->ioinp[port] = func;
	}
}
static void detachinp(IOFUNC iof, UINT port) {

	iof->ioinp[port] = &definp8;
}


// ---- out

typedef struct {
	UINT	port;
	UINT	mask;
const IOOUT	*func;
	UINT	funcs;
} _ATTOUT, *ATTOUT;

static void attachoutex(IOFUNC iof, ATTOUT attout) {

	UINT	port;
	UINT	mask;
	UINT	num;
	UINT	i;

	port = attout->port & 0xff;
	mask = attout->mask & 0xff;
	num = 0;
	for (i=0; i<0x100; i++) {
		if ((i & mask) == port) {
			attachout(iof, i, attout->func[num]);
			num = (num + 1) & (attout->funcs - 1);
		}
	}
}

static BOOL attachcmnout(void *iotbl, void *attout) {

	attachoutex((IOFUNC)iotbl, (ATTOUT)attout);
	return(FALSE);
}

void iocore_attachcmnoutex(UINT port, UINT mask,
										const IOOUT *func, UINT funcs) {

	_ATTOUT	ao;

	ao.port = port;
	ao.mask = mask;
	ao.func = func;
	ao.funcs = funcs;
	listarray_enum(iocore.iotbl, attachcmnout, &ao);
}

static BOOL attachsysout(void *iotbl, void *attout) {

	if ((((IOFUNC)iotbl)->type) & IOFUNC_SYS) {
		attachoutex((IOFUNC)iotbl, (ATTOUT)attout);
	}
	return(FALSE);
}

void iocore_attachsysoutex(UINT port, UINT mask,
										const IOOUT *func, UINT funcs) {

	_ATTOUT	ao;

	ao.port = port;
	ao.mask = mask;
	ao.func = func;
	ao.funcs = funcs;
	listarray_enum(iocore.iotbl, attachsysout, &ao);
}


// ---- inp

typedef struct {
	UINT	port;
	UINT	mask;
const IOINP	*func;
	UINT	funcs;
} _ATTINP, *ATTINP;

static void attachinpex(IOFUNC iof, ATTINP attinp) {

	UINT	port;
	UINT	mask;
	UINT	num;
	UINT	i;

	port = attinp->port & 0xff;
	mask = attinp->mask & 0xff;
	num = 0;
	for (i=0; i<0x100; i++) {
		if ((i & mask) == port) {
			attachinp(iof, i, attinp->func[num]);
			num = (num + 1) & (attinp->funcs - 1);
		}
	}
}

static BOOL attachcmninp(void *iotbl, void *attinp) {

	attachinpex((IOFUNC)iotbl, (ATTINP)attinp);
	return(FALSE);
}

void iocore_attachcmninpex(UINT port, UINT mask,
										const IOINP *func, UINT funcs) {

	_ATTINP	ai;

	ai.port = port;
	ai.mask = mask;
	ai.func = func;
	ai.funcs = funcs;
	listarray_enum(iocore.iotbl, attachcmninp, &ai);
}

static BOOL attachsysinp(void *iotbl, void *attinp) {

	if ((((IOFUNC)iotbl)->type) & IOFUNC_SYS) {
		attachinpex((IOFUNC)iotbl, (ATTINP)attinp);
	}
	return(FALSE);
}

void iocore_attachsysinpex(UINT port, UINT mask,
										const IOINP *func, UINT funcs) {

	_ATTINP	ai;

	ai.port = port;
	ai.mask = mask;
	ai.func = func;
	ai.funcs = funcs;
	listarray_enum(iocore.iotbl, attachsysinp, &ai);
}

// ----

static BRESULT makesndiofunc(UINT port) {

	IOFUNC	tbl[2];
	UINT	num;
	IOFUNC	iof;
	UINT	type;
	IOFUNC	set;

	ZeroMemory(tbl, sizeof(tbl));
	num = (port >> 8) & 15;
	do {
		iof = iocore.base[num];
		type = iof->type;
		if (!(type & (IOFUNC_SND | IOFUNC_EXT))) {
			set = tbl[type & IOFUNC_SYS];
			if (set == NULL) {
				set = (IOFUNC)listarray_append(iocore.iotbl, iof);
				if (set == NULL) {
					return(FAILURE);
				}
				set->type |= IOFUNC_SND;
				set->port = port & 0x0f00;
			}
			iocore.base[num] = set;
		}
		num += 0x10;
	} while(num < 0x100);
	return(SUCCESS);
}

BRESULT iocore_attachsndout(UINT port, IOOUT func) {

	BRESULT	r;
	UINT	num;

	r = makesndiofunc(port);
	if (r == SUCCESS) {
		num = (port >> 8) & 15;
		do {
			attachout(iocore.base[num], port & 0xff, func);
			num += 0x10;
		} while(num < 0x100);
	}
	return(r);
}
BRESULT iocore_detachsndout(UINT port) {

	BRESULT	r;
	UINT	num;

	r = makesndiofunc(port);
	if (r == SUCCESS) {
		num = (port >> 8) & 15;
		do {
			detachout(iocore.base[num], port & 0xff);
			num += 0x10;
		} while(num < 0x100);
	}
	return(r);
}

BRESULT iocore_attachsndinp(UINT port, IOINP func) {

	BRESULT	r;
	UINT	num;

	r = makesndiofunc(port);
	if (r == SUCCESS) {
		num = (port >> 8) & 15;
		do {
			attachinp(iocore.base[num], port & 0xff, func);
			num += 0x10;
		} while(num < 0x100);
	}
	return(r);
}
BRESULT iocore_detachsndinp(UINT port) {

	BRESULT	r;
	UINT	num;

	r = makesndiofunc(port);
	if (r == SUCCESS) {
		num = (port >> 8) & 15;
		do {
			detachinp(iocore.base[num], port & 0xff);
			num += 0x10;
		} while(num < 0x100);
	}
	return(r);
}


// ----

static IOFUNC getextiofunc(UINT port) {

	IOFUNC	iof;

	iof = iocore.base[(port >> 8) & 0xff];
	if (!(iof->type & IOFUNC_EXT)) {
		iof = (IOFUNC)listarray_append(iocore.iotbl, iof);
		if (iof != NULL) {
			iocore.base[(port >> 8) & 0xff] = iof;
			iof->type |= IOFUNC_EXT;
			iof->port = port & 0xff00;
		}
	}
	return(iof);
}

BRESULT iocore_attachout(UINT port, IOOUT func) {

	IOFUNC	iof;

	iof = getextiofunc(port);
	if (iof) {
		attachout(iof, port & 0xff, func);
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}
BRESULT iocore_detachout(UINT port) {

	IOFUNC	iof;

	iof = getextiofunc(port);
	if (iof) {
		detachout(iof, port & 0xff);
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}

BRESULT iocore_attachinp(UINT port, IOINP func) {

	IOFUNC	iof;

	iof = getextiofunc(port);
	if (iof) {
		attachinp(iof, port & 0xff, func);
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}
BRESULT iocore_detachinp(UINT port) {

	IOFUNC	iof;

	iof = getextiofunc(port);
	if (iof) {
		detachinp(iof, port & 0xff);
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}


// ----

void iocore_create(void) {

	UINT	i;
const UINT8	*p;
	UINT	r;

	ZeroMemory(&iocore, sizeof(iocore));
	ZeroMemory(ioterminate, sizeof(ioterminate));
	for (i=0; i<NELEMENTS(termtbl); i++) {
		p = termtbl[i].item;
		r = termtbl[i].items;
		do {
			ioterminate[*p++] = (UINT8)(i + 1);
		} while(--r);
	}
}

void iocore_destroy(void) {

	IOCORE		ioc;

	ioc = &iocore;
	listarray_destroy(ioc->iotbl);
	ioc->iotbl = NULL;
}

BRESULT iocore_build(void) {

	IOCORE		ioc;
	IOFUNC		cmn;
	IOFUNC		sys;
	int			i;
	LISTARRAY	iotbl;

	ioc = &iocore;
	listarray_destroy(ioc->iotbl);
	iotbl = listarray_new(sizeof(_IOFUNC), 32);
	ioc->iotbl = iotbl;
	if (iotbl == NULL) {
		goto icbld_err;
	}
	cmn = (IOFUNC)listarray_append(iotbl, NULL);
	if (cmn == NULL) {
		goto icbld_err;
	}
	for (i=0; i<256; i++) {
		cmn->ioout[i] = defout8;
		cmn->ioinp[i] = definp8;
	}
	sys = (IOFUNC)listarray_append(iotbl, cmn);
	if (sys == NULL) {
		goto icbld_err;
	}
	sys->type = IOFUNC_SYS;
	for (i=0; i<256; i++) {
#ifdef SUPPORT_PC9821
		if (!(i & (0x0c|(np2cfg.sysiomsk >> 8)))) { // PC-9821用 追加マスク(0x0c -> 0xff) np21w ver0.86 rev37
#else
		if (!(i & 0x0c)) {
#endif
			ioc->base[i] = sys;
		}
		else {
			ioc->base[i] = cmn;
		}
	}
	return(SUCCESS);

icbld_err:
	return(FAILURE);
}


// ----

static const FNIORESET resetfn[] =
{
			// PC-9801 System...
			cgrom_reset,							crtc_reset,
			dmac_reset,			gdc_reset,			fdc_reset,
			keyboard_reset,		nmiio_reset,		pic_reset,
			printif_reset,		rs232c_reset,		systemport_reset,
			uPD4990_reset,		fdd320_reset,

			// sys+extend
			itimer_reset,		mouseif_reset,

			// extend
			artic_reset,		egc_reset,			np2sysp_reset,
			necio_reset,		epsonio_reset,		emsio_reset,
#if defined(SUPPORT_PC9821)
			pcidev_reset,
#endif
};

static const FNIOBIND bindfn[] =
{
			// PC-9801 System...
			cgrom_bind,			cpuio_bind,			crtc_bind,
			dmac_bind,			gdc_bind,			fdc_bind,
			keyboard_bind,		nmiio_bind,			pic_bind,
			printif_bind,		rs232c_bind,		systemport_bind,
			uPD4990_bind,		fdd320_bind,

			// sys+extend
			itimer_bind,		mouseif_bind,

			// extend
			artic_bind,			egc_bind,			np2sysp_bind,
			necio_bind,			epsonio_bind,		emsio_bind,
#if defined(SUPPORT_PC9821)
			pcidev_bind,
#endif
};


void iocore_cbreset(const FNIORESET *pfn, UINT uCount, const NP2CFG *pConfig)
{
	while(uCount--)
	{
		(*pfn)(pConfig);
		pfn++;
	}
}

void iocore_cbbind(const FNIOBIND *pfn, UINT uCount)
{
	while(uCount--)
	{
		(*pfn)();
		pfn++;
	}
}

void iocore_reset(const NP2CFG *pConfig)
{
	iocore_cbreset(resetfn, NELEMENTS(resetfn), pConfig);
}

void iocore_bind(void)
{
	iocore.busclock = pccore.multiple;
	iocore_cbbind(bindfn, NELEMENTS(bindfn));
}

void IOOUTCALL iocore_out8(UINT port, REG8 dat) {

	IOFUNC	iof;

//	TRACEOUT(("iocore_out8(%.2x, %.2x)", port, dat));
	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, dat);
}

REG8 IOINPCALL iocore_inp8(UINT port) {

	IOFUNC	iof;
	REG8	ret;
	
	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.base[(port >> 8) & 0xff];
	ret = iof->ioinp[port & 0xff](port);
//	TRACEOUT(("iocore_inp8(%.2x) -> %.2x", port, ret));
	return(ret);
}

void IOOUTCALL iocore_out16(UINT port, REG16 dat) {

	IOFUNC	iof;

//	TRACEOUT(("iocore_out16(%.4x, %.4x)", port, dat));
	CPU_REMCLOCK -= iocore.busclock;
#if defined(SUPPORT_PC9821)&&defined(SUPPORT_PCI)
	if (0x0cfc <= port && port <= 0x0cff) {
		pcidev_w16_0xcfc(port, dat);
		return;
	}
#endif
#if defined(SUPPORT_IDEIO)
	if (port == 0x0640) {
		ideio_w16(port, dat);
		return;
	}
#endif
#if defined(SUPPORT_LGY98)
	if (lgy98cfg.enabled && port == lgy98cfg.baseaddr + 0x200) {
		lgy98_ob200_16(port, dat);
		return;
	}
#endif
#if defined(SUPPORT_SMPU98)
	if (smpu98.enable && smpu98.port <= port && port <= smpu98.port + 0xf) {
		if(smpu98_io16outfunc[port - smpu98.port]){
			(*smpu98_io16outfunc[port - smpu98.port])(port, dat);
			return;
		}
	}
#endif
#if defined(SUPPORT_CL_GD5430)
	if(np2clvga.enabled && cirrusvga_opaque){
		if((np2clvga.gd54xxtype & CIRRUS_98ID_WABMASK) == CIRRUS_98ID_WAB || (np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
			if(pc98_cirrus_isWABport(port)){
				cirrusvga_ioport_write_wrap16(port, (UINT16)dat);
				return;
			}
		}else if(np2clvga.gd54xxtype <= 0xff){
			if(port == 0xc44){
				if(dat == 0xffff){
					np2clvga.VRAMWindowAddr3 = 0;
				}else{
					np2clvga.VRAMWindowAddr3 = ((UINT32)dat) << 16;
				}
				return;
			}
		}
	}
#endif
	if ((port & 0xfff1) == 0x04a0) {
		egc_w16(port, dat);
		return;
	}
	if (!(port & 0x0c00)) {
		switch(ioterminate[port & 0xff]) {
			case TERM_WORD:
				return;

			case TERM_ACTIVE:
			case TERM_PLUS:
			case TERM_MINUS:
			case TERM_EXT08:
				iof = iocore.base[(port >> 8) & 0xff];
				iof->ioout[port & 0xff](port, (UINT8)dat);
				return;
		}
	}
	iof = iocore.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, (UINT8)dat);
	port++;
	iof = iocore.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, (UINT8)(dat >> 8));
}

REG16 IOINPCALL iocore_inp16(UINT port) {

	IOFUNC	iof;
	REG8	ret;

	CPU_REMCLOCK -= iocore.busclock;
#if defined(SUPPORT_PC9821)&&defined(SUPPORT_PCI)
	if (0x0cfc <= port && port <= 0x0cff) {
		return(pcidev_r16_0xcfc(port));
	}
#endif
#if defined(SUPPORT_IDEIO)
	if (port == 0x0640) {
		return(ideio_r16(port));
	}
#endif
#if defined(SUPPORT_LGY98)
	if (lgy98cfg.enabled && port == lgy98cfg.baseaddr + 0x200) {
		return(lgy98_ib200_16(port));
	}
#endif
#if defined(SUPPORT_SMPU98)
	if (smpu98.enable && smpu98.port <= port && port <= smpu98.port + 0xf) {
		if(smpu98_io16inpfunc[port - smpu98.port]){
			return (*smpu98_io16inpfunc[port - smpu98.port])(port);
		}
	}
#endif
#if defined(SUPPORT_CL_GD5430)
	if(np2clvga.enabled && cirrusvga_opaque){
		if((np2clvga.gd54xxtype & CIRRUS_98ID_WABMASK) == CIRRUS_98ID_WAB || (np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
			if(pc98_cirrus_isWABport(port)){
				return(cirrusvga_ioport_read_wrap16(port));
			}
		}else if(np2clvga.gd54xxtype <= 0xff){
			if(port == 0xc44){
				return(np2clvga.VRAMWindowAddr3 >> 16);
			}
		}
	}
#endif
	if ((port & 0xfffc) == 0x005c) {
		return(artic_r16(port));
	}
	iof = iocore.base[(port >> 8) & 0xff];
	if (!(port & 0x0c00)) {
		switch(ioterminate[port & 0xff]) {
			case TERM_WORD:
				return(WORD_TERMINATE);

			case TERM_ACTIVE:
				ret = iof->ioinp[port & 0xff](port);
				return((CPU_AX & 0xff00) + ret);

			case TERM_PLUS:
				ret = iof->ioinp[port & 0xff](port);
				return(0xff00 + ret);

			case TERM_MINUS:
				return(iof->ioinp[port & 0xff](port));

			case TERM_EXT08:
				ret = iof->ioinp[port & 0xff](port);
				return(0x0800 + ret);
		}
	}
	ret = iof->ioinp[port & 0xff](port);
	port++;
	iof = iocore.base[(port >> 8) & 0xff];
	return((UINT16)((iof->ioinp[port & 0xff](port) << 8) + ret));
}

void IOOUTCALL iocore_out32(UINT port, UINT32 dat) {

	CPU_REMCLOCK -= iocore.busclock;
#if defined(SUPPORT_PC9821)
	if ((port & 0xfffb) == 0x0cf8) {
		pcidev_w32(port, dat);
		return;
	}
#endif
#if defined(SUPPORT_CL_GD5430)
	if(np2clvga.enabled && cirrusvga_opaque){
		if((np2clvga.gd54xxtype & CIRRUS_98ID_WABMASK) == CIRRUS_98ID_WAB || (np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
			if(pc98_cirrus_isWABport(port)){
				cirrusvga_ioport_write_wrap32(port, (UINT32)dat);
				return;
			}
		}
	}
#endif
	iocore_out16(port, (UINT16)dat);
	iocore_out16(port+2, (UINT16)(dat >> 16));
}

UINT32 IOINPCALL iocore_inp32(UINT port) {

	UINT32	ret;

	CPU_REMCLOCK -= iocore.busclock;
#if defined(SUPPORT_PC9821)
	if ((port & 0xfffb) == 0x0cf8) {
		return(pcidev_r32(port));
	}
#endif
#if defined(SUPPORT_CL_GD5430)
	if(np2clvga.enabled && cirrusvga_opaque){
		if((np2clvga.gd54xxtype & CIRRUS_98ID_WABMASK) == CIRRUS_98ID_WAB || (np2clvga.gd54xxtype & CIRRUS_98ID_GA98NBMASK) == CIRRUS_98ID_GA98NBIC){
			if(pc98_cirrus_isWABport(port)){
				return(cirrusvga_ioport_read_wrap32(port));
			}
		}
	}
#endif
	ret = iocore_inp16(port);
	return(ret + (iocore_inp16(port+2) << 16));
}

