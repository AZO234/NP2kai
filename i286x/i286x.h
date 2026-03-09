
#define	I286_MEM		mem

#define	I286_STAT		i286core.s.r

#define	I286_REG		i286core.s.r
#define	I286_SEGREG		i286core.s.r.w.es

#define	I286_AX			i286core.s.r.w.ax
#define	I286_BX			i286core.s.r.w.bx
#define	I286_CX			i286core.s.r.w.cx
#define	I286_DX			i286core.s.r.w.dx
#define	I286_SI			i286core.s.r.w.si
#define	I286_DI			i286core.s.r.w.di
#define	I286_BP			i286core.s.r.w.bp
#define	I286_SP			i286core.s.r.w.sp
#define	I286_CS			i286core.s.r.w.cs
#define	I286_DS			i286core.s.r.w.ds
#define	I286_ES			i286core.s.r.w.es
#define	I286_SS			i286core.s.r.w.ss
#define	I286_IP			i286core.s.r.w.ip

#define	SEG_BASE		i286core.s.es_base
#define	ES_BASE			i286core.s.es_base
#define	CS_BASE			i286core.s.cs_base
#define	SS_BASE			i286core.s.ss_base
#define	DS_BASE			i286core.s.ds_base
#define	SS_FIX			i286core.s.ss_fix
#define	DS_FIX			i286core.s.ds_fix

#define	I286_AL			i286core.s.r.b.al
#define	I286_BL			i286core.s.r.b.bl
#define	I286_CL			i286core.s.r.b.cl
#define	I286_DL			i286core.s.r.b.dl
#define	I286_AH			i286core.s.r.b.ah
#define	I286_BH			i286core.s.r.b.bh
#define	I286_CH			i286core.s.r.b.ch
#define	I286_DH			i286core.s.r.b.dh

#define	I286_FLAG		i286core.s.r.w.flag
#define	I286_FLAGL		i286core.s.r.b.flag_l
#define	I286_FLAGH		i286core.s.r.b.flag_h
#define	I286_TRAP		i286core.s.trap
#define	I286_OV			i286core.s.ovflag

#define	I286_GDTR		i286core.s.GDTR
#define	I286_MSW		i286core.s.MSW
#define	I286_IDTR		i286core.s.IDTR
#define	I286_LDTR		i286core.s.LDTR
#define	I286_LDTRC		i286core.s.LDTRC
#define	I286_TR			i286core.s.TR
#define	I286_TRC		i286core.s.TRC

#define	I286_ADRSMASK	i286core.s.adrsmask
#define	I286_REMCLOCK	i286core.s.remainclock
#define	I286_BASECLOCK	i286core.s.baseclock
#define	I286_CLOCK		i286core.s.clock

#define	I286_REPPOSBAK	i286core.e.repbak
#define	I286_INPADRS	i286core.e.inport


#define I286 __declspec(naked) static void
#define I286EXT __declspec(naked) void

typedef void (*I286TBL)(void);


#define		I286IRQCHECKTERM								\
				__asm {	xor		eax, eax				}	\
				__asm { cmp		I286_REMCLOCK, eax		}	\
				__asm {	jle		short nonremainclr		}	\
				__asm { xchg	I286_REMCLOCK, eax		}	\
				__asm {	sub		I286_BASECLOCK, eax		}	\
		nonremainclr:										\
				__asm {	ret								}


#define		I286PREFIX(proc)								\
				__asm {	bts		i286core.s.prefix, 0	}	\
				__asm {	jc		fixed					}	\
				__asm {	mov		I286_REPPOSBAK, esi		}	\
				__asm {	push	offset removeprefix		}	\
			fixed:											\
				GET_NEXTPRE1								\
				__asm {	movzx	eax, bl					}	\
				__asm {	jmp		(proc)[eax*4]			}


extern void __fastcall i286x_localint(void);
extern void __fastcall i286x_selector(void);
extern void removeprefix(void);

extern const I286TBL i286op[256];
extern const I286TBL i286op_repne[256];
extern const I286TBL i286op_repe[256];

extern I286TBL v30op[256];
extern I286TBL v30op_repne[256];
extern I286TBL v30op_repe[256];

void i286xadr_init(void);

