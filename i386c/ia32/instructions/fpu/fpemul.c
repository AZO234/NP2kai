/*
 * Copyright (c) 2012 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"

#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "ia32/instructions/fpu/fp.h"

/*
Short Real
   31: sign (符号)
30-23: exp-8 (指数部: exponet)
22-00: num-23 (小数部)

Long Real
   63: sign
62-52: exp-11
51-00: num-52

Temp Real
   79: sign
78-64: exp-15
   63: 1(?)
62-00: num-63

--
指数部:
2 の 0 乗のとき 0111 1111 となる
1000 0001: +2 乗
1000 0000: +1 乗
0111 1111:  0 乗
0111 1110: -1 乗

仮数部:
2 を基数として整数部が一桁になるように正規化した数の 2 進数表現となる。
正規化によって仮数部の最上位ビットは常に 1 になるので実際に用意しておく
必要はなく、倍精度の 52 ビットであれば最上位の 1 を hidden bit にして
含めなければ、53 ビット分の情報が含まれることになる。

小数の二進数表現:
0.1000    1/2         = 0.5    
0.0100    1/(2*2)     = 0.25   
0.0010    1/(2*2*2)   = 0.125  
0.0001    1/(2*2*2*2) = 0.0625 
*/

#define	PRECISION_NBIT	64
#define	EXP_OFFSET	0x3fff

#define	IS_VALID(f)	(f)->valid
#define	IS_ZERO(f)	(f)->zero
#define	IS_INF(f)	(f)->inf
#define	IS_NAN(f)	(f)->nan
#define	IS_DENORMAL(f)	(f)->denorm

#define	SET_ZERO(f) \
do \
{ \
	(f)->zero = 1; \
	(f)->inf = 0; \
	(f)->nan = 0; \
	(f)->denorm = 0; \
} while (/*CONSTCOND*/0)

#define	SET_INF(f) \
do \
{ \
	(f)->zero = 0; \
	(f)->inf = 1; \
	(f)->nan = 0; \
	(f)->denorm = 0; \
} while (/*CONSTCOND*/0)

#define	SET_NAN(f) \
do \
{ \
	(f)->zero = 0; \
	(f)->inf = 0; \
	(f)->nan = 1; \
	(f)->denorm = 0; \
} while (/*CONSTCOND*/0)


/*
制御命令:
FINIT, FCLEX, FLDCW, FSTCW, FSTSW, GSTENV, FLDENV, FSAVE, FRSTOR, FWAIT
*/

void F2XM1(void);
void FABS(void);
void FADD(void);
void FADDP(void);
void FIADD(void);
void FBLD(void);
void FBSTP(void);
void FCHS(void);
void FCLEX(void);
void FNCLEX(void);
void FCOM(void);
void FCOMP(void);
void FCOMPP(void);
void FCOMI(void);
void FCOMIP(void);
void FUCOMI(void);
void FUCOMIP(void);
void FCOS(void);
void FDECSTP(void);
void FDIV(void);
void FDIVP(void);
void FIDIV(void);
void FDIVR(void);
void FDIVRP(void);
void FIDIVR(void);
void FFREE(void);
void FICOM(void);
void FICOMP(void);
void FILD(void);
void FINCSTP(void);
void FIST(void);
void FISTP(void);
void FLD(void);
void FLDCW(void);
void FLDENV(void);
void FMUL(void);
void FMULP(void);
void FNINIT(void);
void FIMUL(void);
void FNOP(void);
void FPATAN(void);
void FPREM(void);
void FPREM1(void);
void FPTAN(void);
void FRNDINT(void);
void FRSTOR(void);
void FSAVE(void);
void FNSAVE(void);
void _FSCALE(void);
void FSIN(void);
void FSINCOS(void);
void FSQRT(void);
void FST(void);
void FSTP(void);
void FNSTCW(void);
void FSTENV(void);
void FNSTENV(void);
void FNSTSW(void);
void FSUB(void);
void FSUBP(void);
void FISUB(void);
void FSUBR(void);
void FSUBRP(void);
void FISUBR(void);
void FTST(void);
void FUCOM(void);
void FUCOMP(void);
void FUCOMPP(void);
void FXAM(void);
void FXCH(void);
void FXTRACT(void);
void FYL2X(void);
void FYL2XP1(void);

/* FMOVcc */
void FCMOVB(void);
void FCMOVE(void);
void FCMOVBE(void);
void FCMOVU(void);
void FCMOVNB(void);
void FCMOVNE(void);
void FCMOVNBE(void);
void FCMOVNU(void);

/* Load constant */
void FLD1(void);
void FLDL2T(void);
void FLDL2E(void);
void FLDPI(void);
void FLDLG2(void);
void FLDLN2(void);
void FLDZ(void);


static const FPU_PTR zero_ptr = { 0, 0, 0 };

static const FP_REG fp_const_0 = { 1, 0, 1, 0, 0, 0, 0, 0 };
static const FP_REG fp_const_1 = { 1, 0, 0, 0, 0, 0, 0, 0 };

static const FP_REG snan = {
	1, 1, 0, 0, 1, 0, 0, QWORD_CONST(0x00)	/* SNaN */
};
static const FP_REG qnan = {
	1, 1, 0, 0, 1, 0, 0, QWORD_CONST(0x01)	/* QNaN */
};
static const FP_REG indefinite_qnan = {
	1, 1, 0, 0, 1, 0, 0, QWORD_CONST(0x11)	/* indef QNaN */
};

static const REG80 indefinite_bcd = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff }
};


/*
 * FPU exception
 */
enum {
	IS_EXCEPTION,
	IA_EXCEPTION,
	Z_EXCEPTION,
	D_EXCEPTION,
	O_EXCEPTION,
	U_EXCEPTION,
	P_EXCEPTION,
	FPU_EXCEPTION_NUM
};

static void CPUCALL
fpu_exception(int num, int async)
{
	UINT16 bit;

	switch (num) {
	case IS_EXCEPTION:
		FPU_STATUSWORD |= FP_SF_FLAG;
		/*FALLTHROUGH*/
	case IA_EXCEPTION:
		bit = FP_IE_FLAG;
		break;

	case Z_EXCEPTION:
		bit = FP_ZE_FLAG;
		break;

	case D_EXCEPTION:
		bit = FP_DE_FLAG;
		break;

	case O_EXCEPTION:
		bit = FP_OE_FLAG;
		break;

	case U_EXCEPTION:
		bit = FP_UE_FLAG;
		break;

	case P_EXCEPTION:
		bit = FP_PE_FLAG;
		break;

	case FPU_EXCEPTION_NUM:
	default:
		ia32_panic("fpu_exception: unknown exception (%d)\n", num);
		return;
	}

	FPU_STATUSWORD |= bit;

	if (!(bit & FPU_CTRLWORD)) {
		FPU_STATUSWORD |= FP_ES_FLAG|FP_B_FLAG;
		if (CPU_CR0 & CPU_CR0_NE) {
			/* native mode */
			if (!async) {
				CPU_PREV_EIP = CPU_EIP;	/* XXX */
				EXCEPTION(MF_EXCEPTION, 0);
			}
		} else {
			/* MS-DOS compat. mode */
			/* XXX */
		}
	}
}

#define	FPU_EXCEPTION(n,a)	fpu_exception(n,a)


/*
 * FPU memory access function
 */
static UINT16 MEMCALL
fpu_memoryread_w(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_w(seg, address);
}

static UINT32 MEMCALL
fpu_memoryread_d(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_d(seg, address);
}

static UINT64 MEMCALL
fpu_memoryread_q(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_q(seg, address);
}

static REG80 MEMCALL
fpu_memoryread_f(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_f(seg, address);
}

static void MEMCALL
fpu_memorywrite_w(UINT32 address, UINT16 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_w(seg, address, value);
}

static void MEMCALL
fpu_memorywrite_d(UINT32 address, UINT32 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_d(seg, address, value);
}

static void MEMCALL
fpu_memorywrite_q(UINT32 address, UINT64 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_q(seg, address, value);
}

static void MEMCALL
fpu_memorywrite_f(UINT32 address, REG80 *value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_f(seg, address, value);
}

/*
 * FPU memory load function
 */
static void MEMCALL
fp_load_integer(FP_REG *fp, int sign, UINT64 value)
{
	int exp;

	fp->valid = 1;
	fp->sign = sign ? 1 : 0;
	fp->zero = (value == 0);
	fp->nan = 0;
	fp->inf = 0;
	fp->denorm = 0;

	if (!IS_ZERO(fp)) {
		for (exp = 0; exp < 64; ++exp, value <<= 1) {
			if (value & QWORD_CONST(0x8000000000000000)) {
				break;
			}
		}
		fp->num = value;
		fp->exp = (UINT16)exp;
	}
}

static void MEMCALL
fp_load_word_integer(FP_REG *fp, UINT32 address)
{
	UINT64 n;
	UINT16 v;
	int sign;

	v = fpu_memoryread_w(address);

	sign = (v & 0x8000) ? 1 : 0;
	n = sign ? -v : v;

	fp_load_integer(fp, sign, n);
}

static void MEMCALL
fp_load_short_integer(FP_REG *fp, UINT32 address)
{
	UINT64 n;
	UINT32 v;
	int sign;

	v = fpu_memoryread_d(address);

	sign = (v & 0x80000000UL) ? 1 : 0;
	n = sign ? -v : v;

	fp_load_integer(fp, sign, n);
}

static void MEMCALL
fp_load_long_integer(FP_REG *fp, UINT32 address)
{
	UINT64 n;
	UINT64 v;
	int sign;

	v = fpu_memoryread_q(address);

	sign = (v & QWORD_CONST(0x8000000000000000)) ? 1 : 0;
	n = sign ? -v : v;

	fp_load_integer(fp, sign, n);
}

static void MEMCALL
fp_load_bcd_integer(FP_REG *fp, UINT32 address)
{
	REG80 v;
	UINT64 n;
	int sign;
	int i, t;

	v = fpu_memoryread_f(address);

	n = 0;
	for (i = 0; i < 4; ++i) {
		t = v.b[i] & 0xf;
		if (t > 9) {
			goto indefinite;
		}
		n = (n * 10) + t;

		t = (v.b[i] >> 4) & 0xf;
		if (t > 9) {
			goto indefinite;
		}
		n = (n * 10) + t;
	}
	sign = v.b[9] & 0x80;

	fp_load_integer(fp, sign, n);
	return;

 indefinite:
	*fp = indefinite_qnan;
	return;
}

/*
 * FPU memory strore function
 */
static void MEMCALL
fp_store_bcd(FP_REG *fp, UINT32 address)
{
	const REG80 *p;
	REG80 v;
	UINT64 n;
	int valid;
	int i;

	if (IS_ZERO(fp)) {
		memset(&v, 0, sizeof(v));
		v.b[9] = fp->sign ? 0x80 : 0x00;
		p = &v;
	} else if (IS_NAN(fp) || IS_INF(fp)) {
		FPU_EXCEPTION(IA_EXCEPTION, 0);
		p = &indefinite_bcd;
	} else if (IS_DENORMAL(fp)) {
		memset(&v, 0, sizeof(v));
		/* XXX */
		p = &v;
	} else {
		if (fp->exp >= 0 && fp->exp < 59) {
			n = fp->num >> (63 - fp->exp);
			valid = 1;
		} else if (fp->exp == 59) {
			n = fp->num >> (63 - 59);
			if (n < QWORD_CONST(1000000000000000000)) {
				valid = 1;
			} else {
				valid = 0;
			}
		} else {
			n = 0;
			valid = 0;
		}

		if (valid) {
			for (i = 0; i < 9; ++i) {
				v.b[i] = (n % 10);
				n /= 10;
				v.b[i] += (n % 10) << 4;
				n /= 10;
			}
			v.b[9] = fp->sign ? 0x80 : 0x00;
			p = &v;
		} else {
			FPU_EXCEPTION(IA_EXCEPTION, 0);
			p = &indefinite_bcd;
		}
	}

	fpu_memorywrite_f(address, p);
}

/*
 * FPU misc.
 */
static UINT16
fpu_get_tag(void)
{
	FP_REG *fp;
	UINT16 tag;
	UINT v;
	int i;

	tag = 0;
	for (i = 0; i < FPU_REG_NUM; ++i) {
		fp = &FPU_REG(i);
		if (!IS_VALID(fp)) {
			v = 0x11;
		} else if (IS_ZERO(fp)) {
			v = 0x01;
		} else if (IS_NAN(fp) || IS_INF(fp) || IS_DENORMAL(fp)) {
			v = 0x10;
		} else {
			v = 0x11;
		}
		tag |= v << (i * 2);
	}
	return tag;
}


/*
 * FPU interface
 */
void
fpu_init(void)
{
	int i;

	for (i = 0; i < FPU_REG_NUM; i++) {
		memset(&FPU_STAT.reg[i], 0, sizeof(FP_REG));
	}

	FPU_CTRLWORD = 0x037f;
	FPU_STATUSWORD = 0;
	FPU_INSTPTR = zero_ptr;
	FPU_DATAPTR = zero_ptr;
	FPU_LASTINSTOP = 0;

	FPU_STAT_TOP = 0;
	FPU_STAT_PC = FP_CTRL_PC_64;
	FPU_STAT_RC = FP_CTRL_RC_NEAREST_EVEN;
}

char *
fpu_reg2str(void)
{
	static char buf[512];
	char tmp[128];

#if 0
	strcpy(buf, "st=\n");
		for (i = 0; i < FPU_REG_NUM; i++) {
			snprintf(tmp, sizeof(tmp), "%02x", FPU_REG(i));
			strcat(buf, tmp);
		}
		strcat(buf, "\n");
	}
#endif

	snprintf(tmp, sizeof(tmp),
	    "ctrl=%04x  status=%04x  tag=%04x\n"
	    "inst=%04x:%08x  data=%04x:%08x  op=%03x\n",
	    FPU_CTRLWORD, FPU_STATUSWORD, fpu_get_tag(),
	    FPU_INSTPTR_SEG, FPU_INSTPTR_OFFSET,
	    FPU_DATAPTR_SEG, FPU_DATAPTR_OFFSET,
	    FPU_LASTINSTOP);
	strcat(buf, tmp);

	return buf;
}


/*
 * FPU instruction
 */
void
fpu_fwait(void)
{

	/* XXX: check pending FPU exception */
}

void
ESC0(void)
{
	UINT32 op, madr;
	UINT idx;

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0:	/* FADD */
		case 1:	/* FMUL */
		case 2:	/* FCOM */
		case 3:	/* FCOMP */
		case 4:	/* FSUB */
		case 5:	/* FSUBR */
		case 6:	/* FDIV */
		case 7:	/* FDIVR */
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FADD (単精度実数) */
		case 1:	/* FMUL (単精度実数) */
		case 2:	/* FCOM (単精度実数) */
		case 3:	/* FCOMP (単精度実数) */
		case 4:	/* FSUB (単精度実数) */
		case 5:	/* FSUBR (単精度実数) */
		case 6:	/* FDIV (単精度実数) */
		case 7:	/* FDIVR (単精度実数) */
			break;
		}
	}
}

void
ESC1(void)
{
	FP_REG *st0;
	UINT32 op, madr;
	UINT idx;

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		switch (op & 0xf0) {
		case 0xc0:
			if (!(op & 0x08)) {
				/* FLD ST(0), ST(i) */
			} else {
				/* FXCH ST(0), ST(i) */
			}
			break;

		case 0xd0:
			if (op == 0xd0) {
				/* FNOP */
				break;
			}
			EXCEPTION(UD_EXCEPTION, 0);
			break;

		case 0xe0:
			switch (op & 0x0f) {
			case 0x0:	/* FCHS */
				break;

			case 0x1:	/* FABS */
				st0 = &FPU_ST(0);
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				if (IS_VALID(st0)) {
					if (!IS_NAN(st0)) {
						st0->sign = 0;
					}
				} else {
					FPU_EXCEPTION(IS_EXCEPTION, 0);
				}
				break;

			case 0x4:	/* FTST */
			case 0x5:	/* FXAM */
			case 0x8:	/* FLD1 */
			case 0x9:	/* FLDL2T */
			case 0xa:	/* FLDL2E */
			case 0xb:	/* FLDPI */
			case 0xc:	/* FLDLG2 */
			case 0xd:	/* FLDLN2 */
			case 0xe:	/* FLDZ */
				break;

			default:
				EXCEPTION(UD_EXCEPTION, 0);
				break;
			}
			break;

		case 0xf0:
			switch (op & 0xf) {
			case 0x0:	/* F2XM1 */
			case 0x1:	/* FYL2X */
			case 0x2:	/* FPTAN */
			case 0x3:	/* FPATAN */
			case 0x4:	/* FXTRACT */
			case 0x5:	/* FPREM1 */
			case 0x6:	/* FDECSTP */
			case 0x7:	/* FINCSTP */
			case 0x8:	/* FPREM */
			case 0x9:	/* FYL2XP1 */
			case 0xa:	/* FSQRT */
			case 0xb:	/* FSINCOS */
			case 0xc:	/* FRNDINT */
			case 0xd:	/* FSCALE */
			case 0xe:	/* FSIN */
			case 0xf:	/* FCOS */
				break;
			}
			break;

		default:
			ia32_panic("ESC1: invalid opcode = %02x\n", op);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FLD (単精度実数) */
		case 2:	/* FST (単精度実数) */
		case 3:	/* FSTP (単精度実数) */
		case 4:	/* FLDENV */
		case 5:	/* FLDCW */
		case 6:	/* FSTENV */
			break;

		case 7:	/* FSTCW */
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr,
			    FPU_CTRLWORD);
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
}

void
ESC2(void)
{
	UINT32 op, madr;
	UINT idx;

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0:	/* FCMOVB */
		case 1:	/* FCMOVE */
		case 2:	/* FCMOVBE */
		case 3:	/* FCMOVU */
			break;

		case 5:
			if (op == 0xe9) {
				/* FUCOMPP */
				break;
			}
			/*FALLTHROUGH*/
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FIADD (DWORD) */
		case 1:	/* FIMUL (DWORD) */
		case 2:	/* FICOM (DWORD) */
		case 3:	/* FICOMP (DWORD) */
		case 4:	/* FISUB (DWORD) */
		case 5:	/* FISUBR (DWORD) */
		case 6:	/* FIDIV (DWORD) */
		case 7:	/* FIDIVR (DWORD) */
			break;
		}
	}
}

void
ESC3(void)
{
	UINT32 op, madr;
	UINT idx;

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	GET_PCBYTE(op);
	if (op == 0xe3) {
		/* FNINIT */
		fpu_init();
		return;
	}
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0:	/* FCMOVNB */
		case 1:	/* FCMOVNE */
		case 2:	/* FCMOVBE */
		case 3:	/* FCMOVNU */
		case 5:	/* FUCOMI */
		case 6:	/* FCOMI */
			break;

		case 4:
			if (op == 0xe2) {
				/* FCLEX */
				break;
			}
			/*FALLTHROUGH*/
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (DWORD) */
		case 2:	/* FIST (DWORD) */
		case 3:	/* FISTP (DWORD) */
		case 5:	/* FLD (拡張実数) */
		case 7:	/* FSTP (拡張実数) */
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
}

void
ESC4(void)
{
	UINT32 op, madr;
	UINT idx;

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADD */
		case 1:	/* FMUL */
		case 4:	/* FSUBR */
		case 5:	/* FSUB */
		case 6:	/* FDIVR */
		case 7:	/* FDIV */
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FADD (倍精度実数) */
		case 1:	/* FMUL (倍精度実数) */
		case 2:	/* FCOM (倍精度実数) */
		case 3:	/* FCOMP (倍精度実数) */
		case 4:	/* FSUB (倍精度実数) */
		case 5:	/* FSUBR (倍精度実数) */
		case 6:	/* FDIV (倍精度実数) */
		case 7:	/* FDIVR (倍精度実数) */
			break;
		}
	}
}

void
ESC5(void)
{
	UINT32 op, madr;
	UINT idx;

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* FUCOM ST(i), ST(0) */
		/* Fxxx ST(i) */
		switch (idx) {
		case 0:	/* FFREE */
		case 2:	/* FST */
		case 3:	/* FSTP */
		case 4:	/* FUCOM */
		case 5:	/* FUCOMP */
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FLD (倍精度実数) */
		case 2:	/* FST (倍精度実数) */
		case 3:	/* FSTP (倍精度実数) */
		case 4:	/* FRSTOR */
		case 6:	/* FSAVE */
			break;

		case 7:	/* FSTSW */
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr,
			    FPU_STATUSWORD);
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
}

void
ESC6(void)
{
	UINT32 op, madr;
	UINT idx;

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADDP */
		case 1:	/* FMULP */
		case 4:	/* FSUBRP */
		case 5:	/* FSUBP */
		case 6:	/* FDIVRP */
		case 7:	/* FFIVP */
			break;

		case 3:
			if (op == 0xd9) {
				/* FCOMPP */
				break;
			}
			/*FALLTHROUGH*/
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FIADD (WORD) */
		case 1:	/* FIMUL (WORD) */
		case 2:	/* FICOM (WORD) */
		case 3:	/* FICOMP (WORD) */
		case 4:	/* FISUB (WORD) */
		case 5:	/* FISUBR (WORD) */
		case 6:	/* FIDIV (WORD) */
		case 7:	/* FIDIVR (WORD) */
			break;
		}
	}
}

void
ESC7(void)
{
	FP_REG *st0;
	UINT32 op, madr;
	UINT idx;
	int valid;

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 6:	/* FCOMIP */
		case 7:	/* FUCOMIP */
			break;

		case 5:
			if (op == 0xe0) {
				/* FSTSW AX */
				CPU_AX = FPU_STATUSWORD;
				break;
			}
			/*FALLTHROUGH*/
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (WORD) */
		case 2:	/* FIST (WORD) */
		case 3:	/* FISTP (WORD) */
			break;

		case 4:	/* FBLD (BCD) */
			st0 = &FPU_ST(-1);
			valid = IS_VALID(st0);
			if (valid) {
				/* stack overflow */
				FPU_STATUSWORD |= FP_C1_FLAG;
				FPU_EXCEPTION(IS_EXCEPTION, 0);
			}
			fp_load_bcd_integer(st0, madr);
			if (!valid) {
				FPU_STATUSWORD &= ~FP_C1_FLAG;
			}
			FPU_STAT_TOP_DEC();
			break;

		case 5:	/* FILD (QWORD) */
			break;

		case 6:	/* FBSTP (BCD) */
			st0 = &FPU_ST(0);
			if (!IS_VALID(st0)) {
				/* stack underflow */
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_EXCEPTION(IS_EXCEPTION, 0);
			}
			fp_store_bcd(st0, madr);
			st0->valid = 0;
			FPU_STAT_TOP_INC();
			break;

		case 7:	/* FISTP (QWORD) */
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
}


#if 0
/* ----- */

/*
 * 加減算
 */
void
fp_add_sub(FP_REG *t, FP_REG *d, FP_REG *s)
{
	FP_REG *u, *v;
	SINT32 diff_exp;

	/* チェック */
	if (IS_NAN(s) || IS_NAN(d)) {
		SET_NAN(t);
		return;
	}
	if (IS_INF(s) || IS_INF(d)) {
		if (IS_INF(s) && IS_INF(d) && (s->sign != d->sign)) {
			/* #IA 浮動小数点無効算術オペランド例外 */
		} else {
			t->sign = IS_INF(d) ? s->sign : d->sign;
			SET_INF(t);
		}
		return;
	}
	if (IS_ZERO(a) || IS_ZERO(d)) {
		*t = IS_ZERO(d) ? *s : *d;
		return;
	}
	if (IS_DENORM(s)) {
		/* cause #D, s is DENORM */
		return;
	}
	if (IS_DENORM(d)) {
	}

	/* 計算 */
	diff_exp = d->exp - s->exp;
	if (diff_exp >= PRECISION_NBIT + 2) {
		*t = *d;
		u = d;
		v = s;
	} else if (diff_exp <= -(PRECISION_NBIT + 2)) {
		*t = *s;
		u = s;
		v = d;
	} else {
		if (diff_exp == 0) {
			u = d;
			v = s;
			/* XXX */
		} else if (diff_exp > 0) {
			u = d;
			v = s;
			t->sign = u->sign;
		} else if (diff_exp < 0) {
			diff_exp = diff_exp * -1;
			u = s;
			v = d;
			t->sign = u->sign;
		}
	}

	/* 正規化 */
}
#endif
