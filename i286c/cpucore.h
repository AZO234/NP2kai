//----------------------------------------------------------------------------
//
//  i286c : 80286 Engine for Pentium  ver0.05
//
//                               Copyright by Yui/Studio Milmake 1999-2003
//
//----------------------------------------------------------------------------

#include "cpumem.h"

#if defined(CPUCORE_IA32)
#error : not support CPUCORE_IA32
#endif

#if 1 // !defined(CPUDEBUG)
enum {
	I286_MEMREADMAX		= 0xa4000,
	I286_MEMWRITEMAX	= 0xa0000
};
#else									// ダイレクトアクセス範囲を狭める
enum {
	I286_MEMREADMAX		= 0x00400,
	I286_MEMWRITEMAX	= 0x00400
};
#endif

enum {
	C_FLAG			= 0x0001,
	P_FLAG			= 0x0004,
	A_FLAG			= 0x0010,
	Z_FLAG			= 0x0040,
	S_FLAG			= 0x0080,
	T_FLAG			= 0x0100,
	I_FLAG			= 0x0200,
	D_FLAG			= 0x0400,
	O_FLAG			= 0x0800
};

enum {
	MSW_PE			= 0x0001,
	MSW_MP			= 0x0002,
	MSW_EM			= 0x0004,
	MSW_TS			= 0x0008
};

enum {
	CPUTYPE_V30		= 0x01
};

#ifndef CPUCALL
#define	CPUCALL
#endif

#if defined(BYTESEX_LITTLE)

typedef struct {
	UINT8	al;
	UINT8	ah;
	UINT8	cl;
	UINT8	ch;
	UINT8	dl;
	UINT8	dh;
	UINT8	bl;
	UINT8	bh;
	UINT8	sp_l;
	UINT8	sp_h;
	UINT8	bp_l;
	UINT8	bp_h;
	UINT8	si_l;
	UINT8	si_h;
	UINT8	di_l;
	UINT8	di_h;
	UINT8	es_l;
	UINT8	es_h;
	UINT8	cs_l;
	UINT8	cs_h;
	UINT8	ss_l;
	UINT8	ss_h;
	UINT8	ds_l;
	UINT8	ds_h;
	UINT8	flag_l;
	UINT8	flag_h;
	UINT8	ip_l;
	UINT8	ip_h;
} I286REG8;

#else

typedef struct {
	UINT8	ah;
	UINT8	al;
	UINT8	ch;
	UINT8	cl;
	UINT8	dh;
	UINT8	dl;
	UINT8	bh;
	UINT8	bl;
	UINT8	sp_h;
	UINT8	sp_l;
	UINT8	bp_h;
	UINT8	bp_l;
	UINT8	si_h;
	UINT8	si_l;
	UINT8	di_h;
	UINT8	di_l;
	UINT8	es_h;
	UINT8	es_l;
	UINT8	cs_h;
	UINT8	cs_l;
	UINT8	ss_h;
	UINT8	ss_l;
	UINT8	ds_h;
	UINT8	ds_l;
	UINT8	flag_h;
	UINT8	flag_l;
	UINT8	ip_h;
	UINT8	ip_l;
} I286REG8;

#endif

typedef struct {
	UINT16	ax;
	UINT16	cx;
	UINT16	dx;
	UINT16	bx;
	UINT16	sp;
	UINT16	bp;
	UINT16	si;
	UINT16	di;
	UINT16	es;
	UINT16	cs;
	UINT16	ss;
	UINT16	ds;
	UINT16	flag;
	UINT16	ip;
} I286REG16;

typedef struct {
	UINT16	limit;
	UINT16	base;
	UINT8	base24;
	UINT8	reserved;
} I286DTR;

typedef struct {
	union {
		I286REG8	b;
		I286REG16	w;
	}		r;
	UINT32	es_base;
	UINT32	cs_base;
	UINT32	ss_base;
	UINT32	ds_base;
	UINT32	ss_fix;
	UINT32	ds_fix;
	UINT32	adrsmask;						// ver0.72
	UINT16	prefix;
	UINT8	trap;
	UINT8	resetreq;						// ver0.72
	UINT32	ovflag;
	I286DTR	GDTR;
	UINT16	MSW;
	I286DTR	IDTR;
	UINT16	LDTR;							// ver0.73
	I286DTR	LDTRC;
	UINT16	TR;
	I286DTR	TRC;
	UINT8	padding[2];

	UINT8	cpu_type;
	UINT8	itfbank;						// ver0.72
	UINT16	ram_d0;
	SINT32	remainclock;
	SINT32	baseclock;
	UINT32	clock;
} I286STAT;

typedef struct {							// for ver0.73
	UINT8	*ext;
	UINT32	extsize;
	UINT8	*extbase;						// ext - 0x100000
	UINT32	extlimit16mb;					// extsize + 0x100000
	UINT8	*ems[4];
	UINT32	inport;
#if defined(CPUSTRUC_MEMWAIT)
	UINT8	tramwait;
	UINT8	vramwait;
	UINT8	grcgwait;
	UINT8	padding;
#endif
} I286EXT;

typedef struct {
	I286STAT	s;							// STATsaveされる奴
	I286EXT		e;
} I286CORE;


#ifdef __cplusplus
extern "C" {
#endif

extern	I286CORE	i286core;
extern	const UINT8	iflags[];

void i286c_initialize(void);
void i286c_deinitialize(void);
void i286c_reset(void);
void i286c_shut(void);
void i286c_setextsize(UINT32 size);
void i286c_setemm(UINT frame, UINT32 addr);

void CPUCALL i286c_interrupt(REG8 vect);

void i286c(void);
void i286c_step(void);

void v30c(void);
void v30c_step(void);

#ifdef __cplusplus
}
#endif


// ---- macros

#define	CPU_STATSAVE	i286core.s

#define	CPU_AX			i286core.s.r.w.ax
#define	CPU_BX			i286core.s.r.w.bx
#define	CPU_CX			i286core.s.r.w.cx
#define	CPU_DX			i286core.s.r.w.dx
#define	CPU_SI			i286core.s.r.w.si
#define	CPU_DI			i286core.s.r.w.di
#define	CPU_BP			i286core.s.r.w.bp
#define	CPU_SP			i286core.s.r.w.sp
#define	CPU_CS			i286core.s.r.w.cs
#define	CPU_DS			i286core.s.r.w.ds
#define	CPU_ES			i286core.s.r.w.es
#define	CPU_SS			i286core.s.r.w.ss
#define	CPU_IP			i286core.s.r.w.ip

#define	CPU_EAX			i286core.s.r.w.ax
#define	CPU_EBX			i286core.s.r.w.bx
#define	CPU_ECX			i286core.s.r.w.cx
#define	CPU_EDX			i286core.s.r.w.dx
#define	CPU_ESI			i286core.s.r.w.si
#define	CPU_EDI			i286core.s.r.w.di
#define	CPU_EBP			i286core.s.r.w.bp
#define	CPU_ESP			i286core.s.r.w.sp
#define	CPU_EIP			i286core.s.r.w.ip

#define	ES_BASE			i286core.s.es_base
#define	CS_BASE			i286core.s.cs_base
#define	SS_BASE			i286core.s.ss_base
#define	DS_BASE			i286core.s.ds_base

#define	CPU_AL			i286core.s.r.b.al
#define	CPU_BL			i286core.s.r.b.bl
#define	CPU_CL			i286core.s.r.b.cl
#define	CPU_DL			i286core.s.r.b.dl
#define	CPU_AH			i286core.s.r.b.ah
#define	CPU_BH			i286core.s.r.b.bh
#define	CPU_CH			i286core.s.r.b.ch
#define	CPU_DH			i286core.s.r.b.dh

#define	CPU_FLAG		i286core.s.r.w.flag
#define	CPU_FLAGL		i286core.s.r.b.flag_l

#define	CPU_REMCLOCK	i286core.s.remainclock
#define	CPU_BASECLOCK	i286core.s.baseclock
#define	CPU_CLOCK		i286core.s.clock
#define	CPU_ADRSMASK	i286core.s.adrsmask
#define	CPU_MSW			i286core.s.MSW
#define	CPU_RESETREQ	i286core.s.resetreq
#define	CPU_ITFBANK		i286core.s.itfbank
#define	CPU_RAM_D000	i286core.s.ram_d0

#define	CPU_EXTMEM		i286core.e.ext
#define	CPU_EXTMEMSIZE	i286core.e.extsize
#define	CPU_EXTMEMBASE	i286core.e.extbase
#define	CPU_EXTLIMIT16	i286core.e.extlimit16mb
#define	CPU_INPADRS		i286core.e.inport
#define	CPU_EMSPTR		i286core.e.ems

#define	CPU_TYPE		i286core.s.cpu_type

#if defined(CPUSTRUC_MEMWAIT)
#define	MEMWAIT_TRAM	i286core.e.tramwait
#define	MEMWAIT_VRAM	i286core.e.vramwait
#define	MEMWAIT_GRCG	i286core.e.grcgwait
#endif


#define	CPU_isDI		(!(i286core.s.r.w.flag & I_FLAG))
#define	CPU_isEI		(i286core.s.r.w.flag & I_FLAG)
#define	CPU_CLI			i286core.s.r.w.flag &= ~I_FLAG;						\
						i286core.s.trap = 0;
#define	CPU_STI			i286core.s.r.w.flag |= I_FLAG;						\
						i286core.s.trap = (i286core.s.r.w.flag >> 8) & 1;
#define	CPU_A20EN(en)	CPU_ADRSMASK = (en)?0x00ffffff:0x000fffff;

#define	CPU_INITIALIZE				i286c_initialize
#define	CPU_DEINITIALIZE			i286c_deinitialize
#define	CPU_RESET					i286c_reset
#define	CPU_CLEARPREFETCH()			
#define	CPU_INTERRUPT(vect, soft)	i286c_interrupt(vect)
#define	CPU_EXEC					i286c
#define	CPU_EXECV30					v30c
#define	CPU_SHUT					i286c_shut
#define	CPU_SETEXTSIZE(size)		i286c_setextsize((UINT32)(size) << 20)
#define	CPU_SETEMM(frame, addr)		i286c_setemm(frame, addr)

#define	CPU_STEPEXEC				i286c_step

