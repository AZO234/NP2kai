
#ifndef IOOUTCALL
#define	IOOUTCALL
#endif
#ifndef IOINPCALL
#define	IOINPCALL
#endif

typedef void (*FNIORESET)(const NP2CFG *pConfig);
typedef void (*FNIOBIND)(void);
typedef	void (IOOUTCALL *IOOUT)(UINT port, REG8 val);
typedef	REG8 (IOINPCALL *IOINP)(UINT port);

#include	"lsidef.h"

#include	"artic.h"
#include	"cgrom.h"
#include	"cpuio.h"
#include	"crtc.h"
#include	"dipsw.h"
#include	"dmac.h"
#include	"egc.h"
#include	"epsonio.h"
#include	"emsio.h"
#include	"fdc.h"
#include	"fdd320.h"
#include	"gdc.h"
#include	"gdc_cmd.h"
#include	"mouseif.h"
#include	"necio.h"
#include	"nmiio.h"
#include	"np2sysp.h"
#include	"pic.h"
#include	"pit.h"
#include	"printif.h"
#include	"serial.h"
#include	"sysport.h"
#include	"upd4990.h"

#if defined(SUPPORT_PC9821)
#include	"pcidev.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern	_ARTIC		artic;
extern	_CGROM		cgrom;
extern	_CGWINDOW	cgwindow;
extern	_CRTC		crtc;
extern	_DMAC		dmac;
extern	_EGC		egc;
extern	_EPSONIO	epsonio;
extern	_EMSIO		emsio;
extern	_FDC		fdc;
extern	_GDC		gdc;
extern	_GDCS		gdcs;
extern	_GRCG		grcg;
extern	_KEYBRD		keybrd;
extern	_MOUSEIF	mouseif;
extern	_NECIO		necio;
extern	_NMIIO		nmiio;
extern	_NP2SYSP	np2sysp;
extern	_PIC		pic;
extern	_PIT		pit;
extern	_RS232C		rs232c;
extern	_SYSPORT	sysport;
extern	_UPD4990	uPD4990;

#if defined(SUPPORT_PC9821)
extern	_PCIDEV		pcidev;
#endif


// I/O - 8bit decode
void iocore_attachcmnout(UINT port, IOOUT func);
void iocore_attachcmninp(UINT port, IOINP func);
void iocore_attachcmnoutex(UINT port, UINT mask,
											const IOOUT *func, UINT funcs);
void iocore_attachcmninpex(UINT port, UINT mask,
											const IOINP *func, UINT funcs);

// システムI/O - 10bit decode
void iocore_attachsysout(UINT port, IOOUT func);
void iocore_attachsysinp(UINT port, IOINP func);
void iocore_attachsysoutex(UINT port, UINT mask,
											const IOOUT *func, UINT funcs);
void iocore_attachsysinpex(UINT port, UINT mask,
											const IOINP *func, UINT funcs);

// サウンドI/O - 12bit decode
BRESULT iocore_attachsndout(UINT port, IOOUT func);
BRESULT iocore_attachsndinp(UINT port, IOINP func);

// 拡張I/O - 16bit decode
BRESULT iocore_attachout(UINT port, IOOUT func);
BRESULT iocore_attachinp(UINT port, IOINP func);

void iocore_create(void);
void iocore_destroy(void);
BRESULT iocore_build(void);

void iocore_cbreset(const FNIORESET *pfn, UINT uCount, const NP2CFG *pConfig);
void iocore_cbbind(const FNIOBIND *pfn, UINT uCount);
void iocore_reset(const NP2CFG *pConfig);
void iocore_bind(void);

void IOOUTCALL iocore_out8(UINT port, REG8 dat);
REG8 IOINPCALL iocore_inp8(UINT port);

void IOOUTCALL iocore_out16(UINT port, REG16 dat);
REG16 IOINPCALL iocore_inp16(UINT port);

void IOOUTCALL iocore_out32(UINT port, UINT32 dat);
UINT32 IOINPCALL iocore_inp32(UINT port);

#ifdef __cplusplus
}
#endif

