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

/*
 * modified by SimK
 */

#include "compiler.h"

#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "ia32/instructions/fpu/fp.h"

#ifdef USE_FPU

#define ST_SWITCH(op)  \
				case 0: \
					__asm{ op	ST(0) } \
					break; \
				case 1: \
					__asm{ op	ST(1) } \
					break; \
				case 2: \
					__asm{ op	ST(2) } \
					break; \
				case 3: \
					__asm{ op	ST(3) } \
					break; \
				case 4: \
					__asm{ op	ST(4) } \
					break; \
				case 5: \
					__asm{ op	ST(5) } \
					break; \
				case 6: \
					__asm{ op	ST(6) } \
					break; \
				case 7: \
					__asm{ op	ST(7) } \
					break; \
					
#define ST_SWITCH_2f(op)  \
				case 0: \
					__asm{ op	ST(0), ST(0) } \
					break; \
				case 1: \
					__asm{ op	ST(1), ST(0) } \
					break; \
				case 2: \
					__asm{ op	ST(2), ST(0) } \
					break; \
				case 3: \
					__asm{ op	ST(3), ST(0) } \
					break; \
				case 4: \
					__asm{ op	ST(4), ST(0) } \
					break; \
				case 5: \
					__asm{ op	ST(5), ST(0) } \
					break; \
				case 6: \
					__asm{ op	ST(6), ST(0) } \
					break; \
				case 7: \
					__asm{ op	ST(7), ST(0) } \
					break; \
					
#define ST_SWITCH_2s(op)  \
				case 0: \
					__asm{ op	ST(0), ST(0) } \
					break; \
				case 1: \
					__asm{ op	ST(0), ST(1) } \
					break; \
				case 2: \
					__asm{ op	ST(0), ST(2) } \
					break; \
				case 3: \
					__asm{ op	ST(0), ST(3) } \
					break; \
				case 4: \
					__asm{ op	ST(0), ST(4) } \
					break; \
				case 5: \
					__asm{ op	ST(0), ST(5) } \
					break; \
				case 6: \
					__asm{ op	ST(0), ST(6) } \
					break; \
				case 7: \
					__asm{ op	ST(0), ST(7) } \
					break; \
					
unsigned char fpusave[120] = {0}; // FPU環境
unsigned char fpuemuenv[28] = {0}; // FPU環境（エミュレーション）

/*
制御命令:
FINIT, FCLEX, FLDCW, FSTCW, FSTSW, GSTENV, FLDENV, FSAVE, FRSTOR, FWAIT
*/

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
static UINT8 MEMCALL
fpu_memoryread_b(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_b(seg, address);
}

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
fpu_memorywrite_b(UINT32 address, UINT8 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_b(seg, address, value);
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
//
///*
// * FPU memory load function
// */
//static void MEMCALL
//fp_load_integer(FP_REG *fp, int sign, UINT64 value)
//{
//	int exp;
//
//	fp->valid = 1;
//	fp->sign = sign ? 1 : 0;
//	fp->zero = (value == 0);
//	fp->nan = 0;
//	fp->inf = 0;
//	fp->denorm = 0;
//
//	if (!IS_ZERO(fp)) {
//		for (exp = 0; exp < 64; ++exp, value <<= 1) {
//			if (value & QWORD_CONST(0x8000000000000000)) {
//				break;
//			}
//		}
//		fp->num = value;
//		fp->exp = (UINT16)exp;
//	}
//}
//
//static void MEMCALL
//fp_load_word_integer(FP_REG *fp, UINT32 address)
//{
//	UINT64 n;
//	UINT16 v;
//	int sign;
//
//	v = fpu_memoryread_w(address);
//
//	sign = (v & 0x8000) ? 1 : 0;
//	n = sign ? -v : v;
//
//	fp_load_integer(fp, sign, n);
//}
//
//static void MEMCALL
//fp_load_short_integer(FP_REG *fp, UINT32 address)
//{
//	UINT64 n;
//	UINT32 v;
//	int sign;
//
//	v = fpu_memoryread_d(address);
//
//	sign = (v & 0x80000000UL) ? 1 : 0;
//	n = sign ? -v : v;
//
//	fp_load_integer(fp, sign, n);
//}
//
//static void MEMCALL
//fp_load_long_integer(FP_REG *fp, UINT32 address)
//{
//	UINT64 n;
//	UINT64 v;
//	int sign;
//
//	v = fpu_memoryread_q(address);
//
//	sign = (v & QWORD_CONST(0x8000000000000000)) ? 1 : 0;
//	n = sign ? -v : v;
//
//	fp_load_integer(fp, sign, n);
//}
//
//static void MEMCALL
//fp_load_bcd_integer(FP_REG *fp, UINT32 address)
//{
//	REG80 v;
//	UINT64 n;
//	int sign;
//	int i, t;
//
//	v = fpu_memoryread_f(address);
//
//	n = 0;
//	for (i = 0; i < 4; ++i) {
//		t = v.b[i] & 0xf;
//		if (t > 9) {
//			goto indefinite;
//		}
//		n = (n * 10) + t;
//
//		t = (v.b[i] >> 4) & 0xf;
//		if (t > 9) {
//			goto indefinite;
//		}
//		n = (n * 10) + t;
//	}
//	sign = v.b[9] & 0x80;
//
//	fp_load_integer(fp, sign, n);
//	return;
//
// indefinite:
//	*fp = indefinite_qnan;
//	return;
//}
//
///*
// * FPU memory strore function
// */
//static void MEMCALL
//fp_store_bcd(FP_REG *fp, UINT32 address)
//{
//	const REG80 *p;
//	REG80 v;
//	UINT64 n;
//	int valid;
//	int i;
//
//	if (IS_ZERO(fp)) {
//		memset(&v, 0, sizeof(v));
//		v.b[9] = fp->sign ? 0x80 : 0x00;
//		p = &v;
//	} else if (IS_NAN(fp) || IS_INF(fp)) {
//		FPU_EXCEPTION(IA_EXCEPTION, 0);
//		p = &indefinite_bcd;
//	} else if (IS_DENORMAL(fp)) {
//		memset(&v, 0, sizeof(v));
//		/* XXX */
//		p = &v;
//	} else {
//		if (fp->exp >= 0 && fp->exp < 59) {
//			n = fp->num >> (63 - fp->exp);
//			valid = 1;
//		} else if (fp->exp == 59) {
//			n = fp->num >> (63 - 59);
//			if (n < QWORD_CONST(1000000000000000000)) {
//				valid = 1;
//			} else {
//				valid = 0;
//			}
//		} else {
//			n = 0;
//			valid = 0;
//		}
//
//		if (valid) {
//			for (i = 0; i < 9; ++i) {
//				v.b[i] = (n % 10);
//				n /= 10;
//				v.b[i] += (n % 10) << 4;
//				n /= 10;
//			}
//			v.b[9] = fp->sign ? 0x80 : 0x00;
//			p = &v;
//		} else {
//			FPU_EXCEPTION(IA_EXCEPTION, 0);
//			p = &indefinite_bcd;
//		}
//	}
//
//	fpu_memorywrite_f(address, p);
//}
//
///*
// * FPU misc.
// */
//static UINT16
//fpu_get_tag(void)
//{
//	FP_REG *fp;
//	UINT16 tag;
//	UINT v;
//	int i;
//
//	tag = 0;
//	for (i = 0; i < FPU_REG_NUM; ++i) {
//		fp = &FPU_REG(i);
//		if (!IS_VALID(fp)) {
//			v = 0x11;
//		} else if (IS_ZERO(fp)) {
//			v = 0x01;
//		} else if (IS_NAN(fp) || IS_INF(fp) || IS_DENORMAL(fp)) {
//			v = 0x10;
//		} else {
//			v = 0x11;
//		}
//		tag |= v << (i * 2);
//	}
//	return tag;
//}
//

static const FPU_PTR zero_ptr = { 0, 0, 0 };

/*
 * FPU interface
 */
int fpu_updateEmuEnv(void);
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
	
	__asm{
		finit	
		fsave	fpusave
	}
	memset(fpuemuenv, 0, sizeof(fpuemuenv));
	fpu_updateEmuEnv();
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

	//snprintf(tmp, sizeof(tmp),
	//    "ctrl=%04x  status=%04x  tag=%04x\n"
	//    "inst=%04x:%08x  data=%04x:%08x  op=%03x\n",
	//    FPU_CTRLWORD, FPU_STATUSWORD, fpu_get_tag(),
	//    FPU_INSTPTR_SEG, FPU_INSTPTR_OFFSET,
	//    FPU_DATAPTR_SEG, FPU_DATAPTR_OFFSET,
	//    FPU_LASTINSTOP);
	//strcat(buf, tmp);

	return buf;
}


/*
 * FPU instruction
 */

void
fpu_checkexception(void){
	if (CPU_CR0 & CPU_CR0_NE) {
		//EXCEPTION(MF_EXCEPTION, 0);
	//	/* native mode */
	//	//if (!async) {
	//		//CPU_PREV_EIP = CPU_EIP;	/* XXX */
	//		EXCEPTION(MF_EXCEPTION, 0);
	//	//}
	//} else {
	//	/* MS-DOS compat. mode */
	//	/* XXX */
	}
}

void
fpu_fwait(void)
{

	/* XXX: check pending FPU exception */
	fpu_checkexception();
}

// XXX: この辺絶対おかしい
int
fpu_updateEmuEnv(void)
{
	int i;
	descriptor_t *sdp = &CPU_CS_DESC;
	if(SEG_IS_32BIT(sdp) && !CPU_STAT_VM86){
		if(CPU_STAT_PM){
			// 32bit Protected Mode
			memcpy(fpuemuenv, fpusave, 4*3);
			//FPU_STATUSWORD = *((UINT16*)(&fpuemuenv[4]));
			return 3;
		}else{
			// 32bit Real Mode
			memcpy(fpuemuenv, fpusave, 4*3);
			//FPU_STATUSWORD = *((UINT16*)(&fpuemuenv[4]));
			return 2;
		}
		// XXX: 12～
	}else{
		if(CPU_STAT_PM && !CPU_STAT_VM86){
			// 16bit Protected Mode
			for(i=0;i<3;i++){
				fpuemuenv[i*2+0] = fpusave[i*4+0];
				fpuemuenv[i*2+1] = fpusave[i*4+1];
			}
			//FPU_STATUSWORD = *((UINT16*)(&fpuemuenv[2]));
			return 1;
		}else{
			// 16bit Real Mode (or Virtual 8086 Mode)
			for(i=0;i<3;i++){
				fpuemuenv[i*2+0] = fpusave[i*4+0];
				fpuemuenv[i*2+1] = fpusave[i*4+1];
			}
			//FPU_STATUSWORD = *((UINT16*)(&fpuemuenv[2]));
			return 0;
		}
		// XXX: 6～
	}
}
void
fpu_updateStatusWord(void)
{
	int i;
	descriptor_t *sdp = &CPU_CS_DESC;
	if(SEG_IS_32BIT(sdp) && !CPU_STAT_VM86){
		if(CPU_STAT_PM){
			// 32bit Protected Mode
			FPU_STATUSWORD = *((UINT16*)(&fpuemuenv[4]));
		}else{
			// 32bit Real Mode
			FPU_STATUSWORD = *((UINT16*)(&fpuemuenv[4]));
		}
	}else{
		if(CPU_STAT_PM && !CPU_STAT_VM86){
			// 16bit Protected Mode
			FPU_STATUSWORD = *((UINT16*)(&fpuemuenv[2]));
		}else{
			// 16bit Real Mode (or Virtual 8086 Mode)
			FPU_STATUSWORD = *((UINT16*)(&fpuemuenv[2]));
		}
	}
}

void
fpu_syncfpuEnv(void)
{
	int i;
	descriptor_t *sdp = &CPU_CS_DESC;
	if(SEG_IS_32BIT(sdp) && !CPU_STAT_VM86){
		if(CPU_STAT_PM){
			// 32bit Protected Mode
			*((UINT16*)(&fpuemuenv[4])) = FPU_STATUSWORD;
			for(i=0;i<3;i++){
				fpusave[i*4+0] = fpuemuenv[i*4+0];
				fpusave[i*4+1] = fpuemuenv[i*4+1];
				fpusave[i*4+2] = fpuemuenv[i*4+2];
				fpusave[i*4+3] = fpuemuenv[i*4+3];
			}
		}else{
			// 32bit Real Mode
			*((UINT16*)(&fpuemuenv[4])) = FPU_STATUSWORD;
			for(i=0;i<3;i++){
				fpusave[i*4+0] = fpuemuenv[i*4+0];
				fpusave[i*4+1] = fpuemuenv[i*4+1];
				fpusave[i*4+2] = fpuemuenv[i*4+2];
				fpusave[i*4+3] = fpuemuenv[i*4+3];
			}
		}
		// XXX: 12～
	}else{
		if(CPU_STAT_PM && !CPU_STAT_VM86){
			// 16bit Protected Mode
			*((UINT16*)(&fpuemuenv[2])) = FPU_STATUSWORD;
			for(i=0;i<3;i++){
				fpusave[i*4+0] = fpuemuenv[i*2+0];
				fpusave[i*4+1] = fpuemuenv[i*2+1];
			}
		}else{
			// 16bit Real Mode (or Virtual 8086 Mode)
			*((UINT16*)(&fpuemuenv[2])) = FPU_STATUSWORD;
			for(i=0;i<3;i++){
				fpusave[i*4+0] = fpuemuenv[i*2+0];
				fpusave[i*4+1] = fpuemuenv[i*2+1];
			}
		}
		// XXX: 6～
	}
}

void
fpu_setEmuEnvPtr(UINT32 iptrofs, UINT16 iptrsel, UINT16 opcode, UINT32 opptrofs, UINT16 opptrsel)
{
	descriptor_t *sdp = &CPU_CS_DESC;

	if(FPU_STATUSWORD & 0x3f){
		// 例外有り
		fpu_checkexception();
		return;
	}
	if(SEG_IS_32BIT(sdp) && !CPU_STAT_VM86){
		if(CPU_STAT_PM){
			// 32bit Protected Mode
			*((UINT32*)(&fpuemuenv[12])) = iptrofs;
			*((UINT16*)(&fpuemuenv[16])) = iptrsel;
			*((UINT16*)(&fpuemuenv[18])) = opcode;
			*((UINT32*)(&fpuemuenv[20])) = opptrofs;
			*((UINT16*)(&fpuemuenv[24])) = opptrsel;
		}else{
			// 32bit Real Mode
			*((UINT32*)(&fpuemuenv[12])) = iptrofs;
			*((UINT16*)(&fpuemuenv[16])) = iptrsel;
			*((UINT16*)(&fpuemuenv[18])) = opcode;
			*((UINT32*)(&fpuemuenv[20])) = opptrofs;
			*((UINT16*)(&fpuemuenv[24])) = opptrsel;
		}
	}else{
		if(CPU_STAT_PM && !CPU_STAT_VM86){
			// 16bit Protected Mode
			*((UINT16*)(&fpuemuenv[6])) = (UINT16)iptrofs;
			*((UINT16*)(&fpuemuenv[8])) = iptrsel;
			*((UINT16*)(&fpuemuenv[10])) = (UINT16)opptrofs;
			*((UINT16*)(&fpuemuenv[12])) = opptrsel;
		}else{
			// 16bit Real Mode (or Virtual 8086 Mode)
			*((UINT16*)(&fpuemuenv[6])) = (UINT16)iptrofs;
			*((UINT16*)(&fpuemuenv[8])) = opcode;
			*((UINT16*)(&fpuemuenv[10])) = (UINT16)opptrofs;
			*((UINT16*)(&fpuemuenv[12])) = 0;
		}
		// XXX: 6～
	}

}

// d8
void
ESC0(void)
{
	UINT32 op, madr;
	UINT idx;
	UINT32 iptrofs;
	UINT16 iptrsel, opcode;
	int updateEnvPtr = 1;
	iptrofs = CPU_EIP;
	iptrsel = CPU_CS;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC0();
	//	return;
	//}

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	CPU_WORKCLOCK(16);

	GET_PCBYTE(op);
	opcode = (UINT16)op;
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		__asm{ frstor	fpusave }
		switch (idx) {
		case 0:	/* FADD */
			switch (op-0xc0) {
				ST_SWITCH_2s(fadd)
			}
			break;
		case 1:	/* FMUL */
			switch (op-0xc8) {
				ST_SWITCH_2s(fmul)
			}
			break;
		case 2:	/* FCOM */
			switch (op-0xd0) {
				ST_SWITCH(fcom)
			}
			break;
		case 3:	/* FCOMP */
			switch (op-0xd8) {
				ST_SWITCH(fcomp)
			}
			break;
		case 4:	/* FSUB */
			switch (op-0xe0) {
				ST_SWITCH_2s(fsub)
			}
			break;
		case 5:	/* FSUBR */
			switch (op-0xe8) {
				ST_SWITCH_2s(fsubr)
			}
			break;
		case 6:	/* FDIV */
			switch (op-0xf0) {
				ST_SWITCH_2s(fdiv)
			}
			break;
		case 7:	/* FDIVR */
			switch (op-0xf8) {
				ST_SWITCH_2s(fdivr)
			}
			break;
		}
		__asm{ 
			FNSTSW	FPU_STATUSWORD
			FNCLEX
			fsave	fpusave
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FADD (単精度実数) */
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fadd 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 1:	/* FMUL (単精度実数) */
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fmul 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 2:	/* FCOM (単精度実数) */
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fcom 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 3:	/* FCOMP (単精度実数) */
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fcomp 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 4:	/* FSUB (単精度実数) */
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fsub 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 5:	/* FSUBR (単精度実数) */
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fsubr 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 6:	/* FDIV (単精度実数) */
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fdiv	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 7:	/* FDIVR (単精度実数) */
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fdivr 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		}
	}
	if(updateEnvPtr) fpu_setEmuEnvPtr(iptrofs, iptrsel, opcode, FPU_DATAPTR_OFFSET, FPU_DATAPTR_SEG);
}

// d9
void
ESC1(void)
{
	FP_REG *st0;
	UINT32 op, madr;
	UINT idx;
	UINT32 iptrofs;
	UINT16 iptrsel, opcode;
	int updateEnvPtr = 1;
	iptrofs = CPU_EIP;
	iptrsel = CPU_CS;

	//if(!CPU_STAT_PM){
	//	dummy_ESC1();
	//	return;
	//}

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
	
	CPU_WORKCLOCK(16);

	GET_PCBYTE(op);
	opcode = (UINT16)op;
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		switch (op & 0xf0) {
		case 0xc0:
			__asm{ frstor	fpusave }
			if (!(op & 0x08)) {
				/* FLD ST(0), ST(i) */
				switch (op-0xc0) {
					ST_SWITCH(fld)
				}
			} else {
				/* FXCH ST(0), ST(i) */
				switch (op-0xc8) {
					ST_SWITCH(fxch)
				}
			}
			__asm{ 
				FNSTSW	FPU_STATUSWORD
				FNCLEX
				fsave	fpusave
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
				__asm{
					frstor	fpusave
					fchs
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;

			case 0x1:	/* FABS */
				__asm{
					frstor	fpusave
					fabs
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;

			case 0x4:	/* FTST */
				__asm{
					frstor	fpusave
					ftst
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;

			case 0x5:	/* FXAM */
				__asm{
					frstor	fpusave
					fxam
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;

			case 0x8:	/* FLD1 */
				__asm{
					frstor	fpusave
					fld1
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0x9:	/* FLDL2T */
				__asm{
					frstor	fpusave
					fldl2t
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xa:	/* FLDL2E */
				__asm{
					frstor	fpusave
					fldl2e
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xb:	/* FLDPI */
				__asm{
					frstor	fpusave
					fldpi
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xc:	/* FLDLG2 */
				__asm{
					frstor	fpusave
					fldlg2
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xd:	/* FLDLN2 */
				__asm{
					frstor	fpusave
					fldln2
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xe:	/* FLDZ */
				__asm{
					frstor	fpusave
					fldz
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;

			default:
				EXCEPTION(UD_EXCEPTION, 0);
				break;
			}
			break;

		case 0xf0:
			switch (op & 0xf) {
			case 0x0:	/* F2XM1 */
				__asm{
					frstor	fpusave
					f2xm1
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0x1:	/* FYL2X */
				__asm{
					frstor	fpusave
					fyl2x
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0x2:	/* FPTAN */
				__asm{
					frstor	fpusave
					fptan
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0x3:	/* FPATAN */
				__asm{
					frstor	fpusave
					fpatan
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0x4:	/* FXTRACT */
				__asm{
					frstor	fpusave
					fxtract
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0x5:	/* FPREM1 */
				__asm{
					frstor	fpusave
					fprem1
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0x6:	/* FDECSTP */
				__asm{
					frstor	fpusave
					fdecstp
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				updateEnvPtr = 0;
				break;
			case 0x7:	/* FINCSTP */
				__asm{
					frstor	fpusave
					fincstp
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				updateEnvPtr = 0;
				break;
			case 0x8:	/* FPREM */
				__asm{
					frstor	fpusave
					fprem
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0x9:	/* FYL2XP1 */
				__asm{
					frstor	fpusave
					fyl2xp1
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xa:	/* FSQRT */
				__asm{
					frstor	fpusave
					fsqrt
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xb:	/* FSINCOS */
				__asm{
					frstor	fpusave
					fsincos
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xc:	/* FRNDINT */
				__asm{
					frstor	fpusave
					frndint
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xd:	/* FSCALE */
				__asm{
					frstor	fpusave
					fscale
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xe:	/* FSIN */
				__asm{
					frstor	fpusave
					fsin
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				break;
			case 0xf:	/* FCOS */
				__asm{
					frstor	fpusave
					fcos
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
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
			{
				DWORD tmp = fpu_memoryread_d(madr);
				float data = *((float*)(&tmp));
				__asm{
					frstor	fpusave
					fld 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;

		case 2:	/* FST (単精度実数) */
			{
				float data;
				__asm{
					frstor	fpusave
					fst 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_d(madr, *((UINT32*)(&data)));
			}
			break;

		case 3:	/* FSTP (単精度実数) */
			{
				float data;
				__asm{
					frstor	fpusave
					fstp 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_d(madr, *((UINT32*)(&data)));
			}

		case 4:	/* FLDENV */
			{
				int i;
				int mode = fpu_updateEmuEnv();
				if(mode >= 2){
					// 32bit 
					for(i=0;i<28;i+=4){
						if(i==4){
							cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+i, 0xffff0000 | (UINT32)FPU_STATUSWORD);
						}else{
							cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+i, *((UINT32*)(&fpuemuenv[i])));
						}
					}
					//for(i=0;i<28;i++){
					//	cpu_vmemorywrite_b(CPU_INST_SEGREG_INDEX, madr+i, fpuemuenv[i]);
					//}
				}else{
					// 16bit
					for(i=0;i<14;i+=2){
						if(i==2){
							cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr+i, FPU_STATUSWORD);
						}else{
							cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr+i, *((UINT16*)(&fpuemuenv[i])));
						}
					}
					//for(i=0;i<14;i++){
					//	cpu_vmemorywrite_b(CPU_INST_SEGREG_INDEX, madr+i, fpuemuenv[i]);
					//}
				}
				updateEnvPtr = 0;
				//int i, j;
				//UINT8 buf[28];
				//for(i=0;i<28;i+=4){
				//	for(j=0;j<4;j++){
				//		buf[i+j] = fpu_memoryread_b(madr+i+j);
				//	}
				//}
				//__asm{
				//	frstor	fpusave
				//	fldenv 	buf
				//	//FNCLEX
				//	fsave	fpusave
				//}
			}
			break;

		case 5:	/* FLDCW */
			{
				UINT16 data = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
				__asm{
					frstor	fpusave
					fldcw 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_updateEmuEnv();
				updateEnvPtr = 0;
			}
			break;

		case 6:	/* FSTENV */
			{
				int i;
				int mode = fpu_updateEmuEnv();
				if(mode >= 2){
					// 32bit 
					for(i=0;i<28;i+=4){
						if(i==4){
							FPU_STATUSWORD = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr+i);
						}else{
							*((UINT32*)(&fpuemuenv[i])) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr+i);
						}
					}
					//for(i=0;i<28;i++){
					//	fpuemuenv[i] = fpu_memoryread_b(madr+i);
					//}
				}else{
					// 16bit
					for(i=0;i<14;i+=2){
						if(i==2){
							FPU_STATUSWORD = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr+i);
						}else{
							*((UINT16*)(&fpuemuenv[i])) = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr+i);
						}
					}
					//for(i=0;i<14;i++){
					//	fpuemuenv[i] = fpu_memoryread_b(madr+i);
					//}
				}
				fpu_syncfpuEnv();
				updateEnvPtr = 0;
				//int i, j;
				//UINT8 buf[28];
				//__asm{
				//	frstor	fpusave
				//	fstenv 	buf
				//	fsave	fpusave
				//}
				//for(i=0;i<28;i+=4){
				//	for(j=0;j<4;j++){
				//		fpu_memorywrite_b(madr+i+j, buf[i+j]);
				//	}
				//}
			}
			break;

		case 7:	/* FSTCW */
			{
				UINT16 data;
				fpu_syncfpuEnv();
				__asm{
					frstor	fpusave
					fstcw 	data
					fsave	fpusave
				}
				cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, data);
				updateEnvPtr = 0;
			}
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
	if(updateEnvPtr) fpu_setEmuEnvPtr(iptrofs, iptrsel, opcode, FPU_DATAPTR_OFFSET, FPU_DATAPTR_SEG);
}

// da
void
ESC2(void)
{
	UINT32 op, madr;
	UINT idx;
	UINT32 iptrofs;
	UINT16 iptrsel, opcode;
	int updateEnvPtr = 1;
	iptrofs = CPU_EIP;
	iptrsel = CPU_CS;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC2();
	//	return;
	//}

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
	
	CPU_WORKCLOCK(16);

	GET_PCBYTE(op);
	opcode = (UINT16)op;
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0:	/* FCMOVB */
			{
				__asm{ frstor	fpusave }
				switch (op-0xc0) {
					ST_SWITCH_2s(fcmovb)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 1:	/* FCMOVE */
			{
				__asm{ frstor	fpusave }
				switch (op-0xc8) {
					ST_SWITCH_2s(fcmove)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 2:	/* FCMOVBE */
				__asm{ frstor	fpusave }
				switch (op-0xd0) {
					ST_SWITCH_2s(fcmovbe)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			break;
		case 3:	/* FCMOVU */
				__asm{ frstor	fpusave }
				switch (op-0xd8) {
					ST_SWITCH_2s(fcmovu)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			break;

		case 5:
			if (op == 0xe9) {
				/* FUCOMPP */
				__asm{
					frstor	fpusave
					fucompp
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
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
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					fiadd 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 1:	/* FIMUL (DWORD) */
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					fimul 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 2:	/* FICOM (DWORD) */
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					ficom 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 3:	/* FICOMP (DWORD) */
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					ficomp 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 4:	/* FISUB (WORD) */
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					fisub 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 5:	/* FISUBR (DWORD) */
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					fisubr 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 6:	/* FIDIV (DWORD) */
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					fidiv 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 7:	/* FIDIVR (DWORD) */
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					fidivr 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		}
	}
	if(updateEnvPtr) fpu_setEmuEnvPtr(iptrofs, iptrsel, opcode, FPU_DATAPTR_OFFSET, FPU_DATAPTR_SEG);
}

// db
void
ESC3(void)
{
	UINT32 op, madr;
	UINT idx;
	UINT32 iptrofs;
	UINT16 iptrsel, opcode;
	int updateEnvPtr = 1;
	iptrofs = CPU_EIP;
	iptrsel = CPU_CS;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC3();
	//	return;
	//}

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
	
	CPU_WORKCLOCK(16);

	GET_PCBYTE(op);
	opcode = (UINT16)op;
	if (op == 0xe3) {
		/* FNINIT */
		fpu_init();
		return;
	}
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		__asm{ frstor	fpusave }
		switch (idx) {
		case 0:	/* FCMOVNB */
			{
				switch (op-0xc0) {
					ST_SWITCH_2s(fcmovnb)
				}
			}
			break;
		case 1:	/* FCMOVNE */
			{
				switch (op-0xc8) {
					ST_SWITCH_2s(fcmovne)
				}
			}
			break;
		case 2:	/* FCMOVNBE */
			{
				switch (op-0xd0) {
					ST_SWITCH_2s(fcmovnbe)
				}
			}
			break;
		case 3:	/* FCMOVNU */
			{
				switch (op-0xd8) {
					ST_SWITCH_2s(fcmovnu)
				}
			}
			break;
		case 5:	/* FUCOMI */
			{
				switch (op-0xe8) {
					ST_SWITCH_2s(fucomi)
				}
			}
			break;
		case 6:	/* FCOMI */
			{
				switch (op-0xf0) {
					ST_SWITCH_2s(fucomi)
				}
			}
			break;

		case 4:
			if (op == 0xe2) {
				/* FCLEX */
				{
					__asm{
						frstor	fpusave
						fclex
						fsave	fpusave
					}
				}
				fpu_updateStatusWord();
				fpu_updateEmuEnv();
				break;
			}
			/*FALLTHROUGH*/
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
		__asm{ 
			FNSTSW	FPU_STATUSWORD
			FNCLEX
			fsave	fpusave
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (DWORD) */
			{
				DWORD data = fpu_memoryread_d(madr);
				__asm{
					frstor	fpusave
					fild 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 2:	/* FIST (DWORD) */
			{
				DWORD data;
				__asm{
					frstor	fpusave
					fist 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_d(madr, data);
			}
			break;
		case 3:	/* FISTP (DWORD) */
			{
				DWORD data;
				__asm{
					frstor	fpusave
					fistp 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_d(madr, data);
			}
			break;
		case 5:	/* FLD (拡張実数) */
			{
				int i;
				UINT8 buf[10];
				for(i=0;i<10;i++){
					buf[i] = fpu_memoryread_b(madr+i);
				}
				__asm{
					frstor	fpusave
					fld 	buf
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 7:	/* FSTP (拡張実数) */
			{
				int i;
				UINT8 buf[10];
				__asm{
					frstor	fpusave
					fstp 	buf
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				for(i=0;i<10;i++){
					fpu_memorywrite_w(madr+i, buf[i]);
				}
			}
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
	if(updateEnvPtr) fpu_setEmuEnvPtr(iptrofs, iptrsel, opcode, FPU_DATAPTR_OFFSET, FPU_DATAPTR_SEG);
}

// dc
void
ESC4(void)
{
	UINT32 op, madr;
	UINT idx;
	UINT32 iptrofs;
	UINT16 iptrsel, opcode;
	int updateEnvPtr = 1;
	iptrofs = CPU_EIP;
	iptrsel = CPU_CS;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC4();
	//	return;
	//}

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
	
	CPU_WORKCLOCK(16);

	GET_PCBYTE(op);
	opcode = (UINT16)op;
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		__asm{ frstor	fpusave }
		switch (idx) {
		case 0:	/* FADD */
			switch (op-0xc0) {
				ST_SWITCH_2f(fadd)
			}
			break;
		case 1:	/* FMUL */
			switch (op-0xc8) {
				ST_SWITCH_2f(fmul)
			}
			break;
		case 4:	/* FSUBR */
			switch (op-0xe0) {
				ST_SWITCH_2f(fsubr)
			}
			break;
		case 5:	/* FSUB */
			switch (op-0xe8) {
				ST_SWITCH_2f(fsub)
			}
			break;
		case 6:	/* FDIVR */
			switch (op-0xf0) {
				ST_SWITCH_2f(fdivr)
			}
			break;
		case 7:	/* FDIV */
			switch (op-0xf8) {
				ST_SWITCH_2f(fdiv)
			}
			break;
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
		__asm{ 
			FNSTSW	FPU_STATUSWORD
			FNCLEX
			fsave	fpusave
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FADD (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fadd 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 1:	/* FMUL (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fmul 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 2:	/* FCOM (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fcom 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 3:	/* FCOMP (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fcomp 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 4:	/* FSUB (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fsub 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 5:	/* FSUBR (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fsubr 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 6:	/* FDIV (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fdiv	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 7:	/* FDIVR (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fdivr 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		}
	}
	if(updateEnvPtr) fpu_setEmuEnvPtr(iptrofs, iptrsel, opcode, FPU_DATAPTR_OFFSET, FPU_DATAPTR_SEG);
}

// dd
void
ESC5(void)
{
	UINT32 op, madr;
	UINT idx;
	UINT32 iptrofs;
	UINT16 iptrsel, opcode;
	int updateEnvPtr = 1;
	iptrofs = CPU_EIP;
	iptrsel = CPU_CS;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC5();
	//	return;
	//}

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
	
	CPU_WORKCLOCK(16);

	GET_PCBYTE(op);
	opcode = (UINT16)op;
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* FUCOM ST(i), ST(0) */
		/* Fxxx ST(i) */
		switch (idx) {
		case 0:	/* FFREE */
			__asm{ frstor	fpusave }
			switch (op-0xc0) {
				ST_SWITCH(ffree)
			}
			__asm{ 
				FNSTSW	FPU_STATUSWORD
				FNCLEX
				fsave	fpusave
			}
			break;
		case 2:	/* FST */
			{
				__asm{ frstor	fpusave }
				switch (op-0xd0) {
					ST_SWITCH(fst)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 3:	/* FSTP */
			{
				__asm{ frstor	fpusave }
				switch (op-0xd8) {
					ST_SWITCH(fstp)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 4:	/* FUCOM */
			{
				__asm{ frstor	fpusave }
				switch (op-0xd8) {
					ST_SWITCH(fucom)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 5:	/* FUCOMP */
			{
				__asm{ frstor	fpusave }
				switch (op-0xd8) {
					ST_SWITCH(fucomp)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FLD (倍精度実数) */
			{
				UINT64 tmp = fpu_memoryread_q(madr);
				double data = *((double*)(&tmp));
				__asm{
					frstor	fpusave
					fld 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 2:	/* FST (倍精度実数) */
			{
				double data;
				__asm{
					frstor	fpusave
					fst 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_q(madr, *((UINT64*)(&data)));
			}
			break;
		case 3:	/* FSTP (倍精度実数) */
			{
				double data;
				__asm{
					frstor	fpusave
					fst 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_q(madr, *((UINT64*)(&data)));
			}
			break;
		case 4:	/* FRSTOR */
			{
				int i;
				int mode = fpu_updateEmuEnv();
				if(mode >= 2){
					// 32bit 
					for(i=0;i<28;i+=4){
						if(i==4){
							FPU_STATUSWORD = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr+i);
						}else{
							*((UINT32*)(&fpuemuenv[i])) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr+i);
						}
					}
					for(i=28;i<108;i++){
						fpusave[i] = cpu_vmemoryread_b(CPU_INST_SEGREG_INDEX, madr+i);
					}
				}else{
					// 16bit
					for(i=0;i<14;i+=2){
						if(i==2){
							FPU_STATUSWORD = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr+i);
						}else{
							*((UINT16*)(&fpuemuenv[i])) = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr+i);
						}
					}
					for(i=28;i<108;i++){
						fpusave[i] = cpu_vmemoryread_b(CPU_INST_SEGREG_INDEX, madr+i-14);
					}
				}
				fpu_syncfpuEnv();
				updateEnvPtr = 0;
			}
			break;
		case 6:	/* FSAVE */
			{
				int i;
				int mode = fpu_updateEmuEnv();
				if(mode >= 2){
					// 32bit 
					for(i=0;i<28;i+=4){
						if(i==4){
							cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+i, 0xffff0000 | (UINT32)FPU_STATUSWORD);
						}else{
							cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+i, *((UINT32*)(&fpuemuenv[i])));
						}
					}
					//for(i=0;i<28;i++){
					//	fpu_memorywrite_b(madr+i, fpuemuenv[i]);
					//}
					for(i=28;i<108;i++){
						cpu_vmemorywrite_b(CPU_INST_SEGREG_INDEX, madr+i, fpusave[i]);
					}
				}else{
					// 16bit
					for(i=0;i<14;i+=2){
						if(i==2){
							cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr+i, FPU_STATUSWORD);
						}else{
							cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr+i, *((UINT16*)(&fpuemuenv[i])));
						}
					}
					//for(i=0;i<14;i++){
					//	fpu_memorywrite_b(madr+i, fpuemuenv[i]);
					//}
					for(i=28;i<108;i++){
						cpu_vmemorywrite_b(CPU_INST_SEGREG_INDEX, madr+i-14, fpusave[i]);
					}
				}
				updateEnvPtr = 0;
			}
			break;

		case 7:	/* FSTSW */
			{
				UINT16 data;
				__asm{
					frstor	fpusave
					fstsw 	data
				}
				cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, data);
			}
			break;
			//fpu_memorywrite_w(madr,
			//    FPU_STATUSWORD);
			//break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
	if(updateEnvPtr) fpu_setEmuEnvPtr(iptrofs, iptrsel, opcode, FPU_DATAPTR_OFFSET, FPU_DATAPTR_SEG);
}

// de
void
ESC6(void)
{
	UINT32 op, madr;
	UINT idx;
	UINT32 iptrofs;
	UINT16 iptrsel, opcode;
	int updateEnvPtr = 1;
	iptrofs = CPU_EIP;
	iptrsel = CPU_CS;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC6();
	//	return;
	//}

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
	
	CPU_WORKCLOCK(16);

	GET_PCBYTE(op);
	opcode = (UINT16)op;
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		__asm{ frstor	fpusave }
		switch (idx) {
		case 0:	/* FADDP */
			switch (op-0xc0) {
				ST_SWITCH_2f(faddp)
			}
			break;
		case 1:	/* FMULP */
			switch (op-0xc8) {
				ST_SWITCH_2f(fmulp)
			}
			break;
		case 4:	/* FSUBP */
			switch (op-0xe0) {
				ST_SWITCH_2f(fsubp)
			}
			break;
		case 5:	/* FSUBRP */
			switch (op-0xe8) {
				ST_SWITCH_2f(fsubrp)
			}
			break;
		case 6:	/* FDIVP */
			switch (op-0xf0) {
				ST_SWITCH_2f(fdivp)
			}
			break;
		case 7:	/* FDIVRP */
			switch (op-0xf8) {
				ST_SWITCH_2f(fdivrp)
			}
			break;

		case 3:
			if (op == 0xd9) {
				/* FCOMPP */
				__asm{
					fcompp
				}
				break;
			}
			/*FALLTHROUGH*/
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
		__asm{ 
			FNSTSW	FPU_STATUSWORD
			FNCLEX
			fsave	fpusave
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FIADD (WORD) */
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					fiadd 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 1:	/* FIMUL (WORD) */
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					fimul 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 2:	/* FICOM (WORD) */
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					ficom 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 3:	/* FICOMP (WORD) */
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					ficomp 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 4:	/* FISUB (WORD) */
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					fisub 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 5:	/* FISUBR (WORD) */
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					fisubr 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 6:	/* FIDIV (WORD) */
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					fidiv 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 7:	/* FIDIVR (WORD) */
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					fidivr 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		}
	}
	if(updateEnvPtr) fpu_setEmuEnvPtr(iptrofs, iptrsel, opcode, FPU_DATAPTR_OFFSET, FPU_DATAPTR_SEG);
}

// df
void
ESC7(void)
{
	FP_REG *st0;
	UINT32 op, madr;
	UINT idx;
	int valid;
	UINT32 iptrofs;
	UINT16 iptrsel, opcode;
	int updateEnvPtr = 1;
	iptrofs = CPU_EIP;
	iptrsel = CPU_CS;
	
	//if(!CPU_STAT_PM){
	//	dummy_ESC7();
	//	return;
	//}

	if (CPU_CR0 & (CPU_CR0_EM|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
	
	CPU_WORKCLOCK(16);

	GET_PCBYTE(op);
	opcode = (UINT16)op;
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0:
		case 7:	/* FCOMIP */
			{
				__asm{ frstor	fpusave }
				switch (op-0xf0) {
					ST_SWITCH_2s(fcomip)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 6:	/* FUCOMIP */
			{
				__asm{ frstor	fpusave }
				switch (op-0xe8) {
					ST_SWITCH_2s(fucomip)
				}
				__asm{ 
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;

		case 4: // 5?
			if (op == 0xe0) {
				/* FSTSW AX */
				__asm{
					frstor	fpusave
					fstsw 	FPU_STATUSWORD
					//FNCLEX
					fsave	fpusave
				}
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
			{
				WORD data = fpu_memoryread_w(madr);
				__asm{
					frstor	fpusave
					fild 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;
		case 2:	/* FIST (WORD) */
			{
				WORD data;
				__asm{
					frstor	fpusave
					fist 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_w(madr, data);
			}
			break;
		case 3:	/* FISTP (WORD) */
			{
				WORD data;
				__asm{
					frstor	fpusave
					fistp 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_w(madr, data);
			}
			break;

		case 4:	/* FBLD (BCD) */
			{
				int i;
				UINT8 buf[10];
				for(i=0;i<10;i++){
					buf[i] = fpu_memoryread_b(madr+i);
				}
				__asm{
					frstor	fpusave
					fbld 	buf
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;

		case 5:	/* FILD (QWORD) */
			{
				UINT64 data = fpu_memoryread_q(madr);
				__asm{
					frstor	fpusave
					fild 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
			}
			break;

		case 6:	/* FBSTP (BCD) */
			{
				int i;
				UINT8 buf[10];
				__asm{
					frstor	fpusave
					fbstp 	buf
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				for(i=0;i<10;i++){
					fpu_memorywrite_w(madr+i, buf[i]);
				}
			}
			break;

		case 7:	/* FISTP (QWORD) */
			{
				UINT64 data;
				__asm{
					frstor	fpusave
					fistp 	data
					FNSTSW	FPU_STATUSWORD
					FNCLEX
					fsave	fpusave
				}
				fpu_memorywrite_q(madr, data);
			}
			break;

		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
	if(updateEnvPtr) fpu_setEmuEnvPtr(iptrofs, iptrsel, opcode, FPU_DATAPTR_OFFSET, FPU_DATAPTR_SEG);
}

//void
//ESC0(void)
//{
//	UINT32 op, madr;
//
//	GET_PCBYTE(op);
//	TRACEOUT(("use FPU d8 %.2x", op));
//	if (op >= 0xc0) {
//		EXCEPTION(NM_EXCEPTION, 0);
//	} else {
//		madr = calc_ea_dst(op);
//		EXCEPTION(NM_EXCEPTION, 0);
//	}
//}
//
//void
//ESC1(void)
//{
//	UINT32 op, madr;
//
//	GET_PCBYTE(op);
//	TRACEOUT(("use FPU d9 %.2x", op));
//	if (op >= 0xc0) {
//		EXCEPTION(NM_EXCEPTION, 0);
//	} else {
//		madr = calc_ea_dst(op);
//		switch (op & 0x38) {
//		case 0x28:
//			TRACEOUT(("FLDCW"));
//			(void) fpu_memoryread_w(madr);
//			break;
//
//		case 0x38:
//			TRACEOUT(("FSTCW"));
//			fpu_memorywrite_w(madr, 0xffff);
//			break;
//
//		default:
//			EXCEPTION(NM_EXCEPTION, 0);
//			break;
//		}
//	}
//}
//
//void
//ESC2(void)
//{
//	UINT32 op, madr;
//
//	GET_PCBYTE(op);
//	TRACEOUT(("use FPU da %.2x", op));
//	if (op >= 0xc0) {
//		EXCEPTION(NM_EXCEPTION, 0);
//	} else {
//		madr = calc_ea_dst(op);
//		EXCEPTION(NM_EXCEPTION, 0);
//	}
//}
//
//void
//ESC3(void)
//{
//	UINT32 op, madr;
//
//	GET_PCBYTE(op);
//	TRACEOUT(("use FPU db %.2x", op));
//	if (op >= 0xc0) {
//		if (op != 0xe3) {
//			EXCEPTION(NM_EXCEPTION, 0);
//		}
//		/* FNINIT */
//		(void)madr;
//	} else {
//		madr = calc_ea_dst(op);
//		EXCEPTION(NM_EXCEPTION, 0);
//	}
//}
//
//void
//ESC4(void)
//{
//	UINT32 op, madr;
//
//	GET_PCBYTE(op);
//	TRACEOUT(("use FPU dc %.2x", op));
//	if (op >= 0xc0) {
//		EXCEPTION(NM_EXCEPTION, 0);
//	} else {
//		madr = calc_ea_dst(op);
//		EXCEPTION(NM_EXCEPTION, 0);
//	}
//}
//
//void
//ESC5(void)
//{
//	UINT32 op, madr;
//
//	GET_PCBYTE(op);
//	TRACEOUT(("use FPU dd %.2x", op));
//	if (op >= 0xc0) {
//		EXCEPTION(NM_EXCEPTION, 0);
//	} else {
//		madr = calc_ea_dst(op);
//		if (((op >> 3) & 7) != 7) {
//			EXCEPTION(NM_EXCEPTION, 0);
//		}
//		/* FSTSW */
//		TRACEOUT(("FSTSW"));
//		fpu_memorywrite_w(madr, 0xffff);
//	}
//}
//
//void
//ESC6(void)
//{
//	UINT32 op, madr;
//
//	GET_PCBYTE(op);
//	TRACEOUT(("use FPU de %.2x", op));
//	if (op >= 0xc0) {
//		EXCEPTION(NM_EXCEPTION, 0);
//	} else {
//		madr = calc_ea_dst(op);
//		EXCEPTION(NM_EXCEPTION, 0);
//	}
//}
//
//void
//ESC7(void)
//{
//	UINT32 op, madr;
//
//	GET_PCBYTE(op);
//	TRACEOUT(("use FPU df %.2x", op));
//	if (op >= 0xc0) {
//		if (op != 0xe0) {
//			EXCEPTION(NM_EXCEPTION, 0);
//		}
//		/* FSTSW AX */
//		TRACEOUT(("FSTSW AX"));
//		CPU_AX = 0xffff;
//	} else {
//		madr = calc_ea_dst(op);
//		EXCEPTION(NM_EXCEPTION, 0);
//	}
//}


#else

void
fpu_init(void)
{
}

void
fpu_fwait(void)
{
}

char *
fpu_reg2str(void)
{
	return NULL;
}

void
ESC0(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU d8 %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC1(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU d9 %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		switch (op & 0x38) {
		case 0x28:
			TRACEOUT(("FLDCW"));
			(void) cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
			break;

		case 0x38:
			TRACEOUT(("FSTCW"));
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, 0xffff);
			break;

		default:
			EXCEPTION(NM_EXCEPTION, 0);
			break;
		}
	}
}

void
ESC2(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU da %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC3(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU db %.2x", op));
	if (op >= 0xc0) {
		if (op != 0xe3) {
			EXCEPTION(NM_EXCEPTION, 0);
		}
		/* FNINIT */
		(void)madr;
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC4(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU dc %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC5(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU dd %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		if (((op >> 3) & 7) != 7) {
			EXCEPTION(NM_EXCEPTION, 0);
		}
		/* FSTSW */
		TRACEOUT(("FSTSW"));
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, 0xffff);
	}
}

void
ESC6(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU de %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC7(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU df %.2x", op));
	if (op >= 0xc0) {
		if (op != 0xe0) {
			EXCEPTION(NM_EXCEPTION, 0);
		}
		/* FSTSW AX */
		TRACEOUT(("FSTSW AX"));
		CPU_AX = 0xffff;
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}


#endif
