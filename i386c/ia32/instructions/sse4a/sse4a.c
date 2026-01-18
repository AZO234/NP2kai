/*
 * Copyright (c) 2024 SimK
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

// AMD64 Architecture Programmer’s Manual Volume 4 の説明を見て実装
	
#include "compiler.h"

#include <math.h>
#include <float.h>

#define isnan(x) (_isnan(x))

#include "../../cpu.h"
#include "../../ia32.mcr"

#include "../sse/sse.h"
#include "../sse2/sse2.h"
#include "../sse3/sse3.h"
#include "sse4a.h"

#if defined(USE_SSE4A) && defined(USE_SSE3) && defined(USE_SSE2) && defined(USE_SSE) && defined(USE_FPU)

#define CPU_SSE4AWORKCLOCK	CPU_WORKCLOCK(2)

static INLINE void
SSE4a_check_NM_EXCEPTION(){
	if(!(i386cpuid.cpu_feature_ex_ecx & CPU_FEATURE_EX_ECX_SSE4A)){
		EXCEPTION(UD_EXCEPTION, 0);
	}     
	if(CPU_CR0 & CPU_CR0_EM){
		EXCEPTION(UD_EXCEPTION, 0);
	}     
	if (CPU_CR0 & CPU_CR0_TS) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

static INLINE void
SSE4a_setTag(void)
{
}

static INLINE void
MMX_setTag(void)
{
	int i;
	
	if(!FPU_STAT.mmxenable){
		FPU_STAT.mmxenable = 1;
		//FPU_CTRLWORD = 0x27F;
		for (i = 0; i < FPU_REG_NUM; i++) {
			FPU_STAT.tag[i] = TAG_Valid;
#ifdef SUPPORT_FPU_DOSBOX2
			FPU_STAT.int_regvalid[i] = 0;
#endif
			FPU_STAT.reg[i].ul.ext = 0xffff;
		}
	}
	FPU_STAT_TOP = 0;
	FPU_STATUSWORD &= ~0x3800;
	FPU_STATUSWORD |= (FPU_STAT_TOP&7)<<11;
}

/*
 * SSE4a interface
 */

// xmm/m128 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_PD(double **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSE4a_check_NM_EXCEPTION();
	SSE4a_setTag();
	CPU_SSE4AWORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (double*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT64*)(data2buf+ 0)) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*((UINT64*)(data2buf+ 1)) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, maddr+ 8);
		*data2 = data2buf;
	}
}

static INLINE void MMX_PART_GETDATA1DATA2_PD(float **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSE4a_check_NM_EXCEPTION();
	SSE4a_setTag();
	CPU_SSE4AWORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (float*)(&(FPU_STAT.reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (float*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		*((UINT32*)(data2buf+ 1)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		*data2 = data2buf;
	}
}

void SSE4a_MOVNTSD(void)
{
	UINT32 op;
	UINT idx, sub;
	float* data1;

	// 00001111:00101011 mod xmmreg r/m

	SSE4a_check_NM_EXCEPTION();
	SSE4a_setTag();
	CPU_SSE4AWORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0)
	{
		// XXX: ここはどう扱う?
		EXCEPTION(UD_EXCEPTION, 0);
		//data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
		//for(i=0;i<4;i++){
		//	data2[i] = data1[i];
		//}
	}
	else
	{
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr + 0, *((UINT32*)(data1 + 0)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr + 4, *((UINT32*)(data1 + 1)));
	}
	TRACEOUT(("SSE4a_MOVNTSD"));
}
void SSE4a_MOVNTSS(void)
{
	UINT32 op;
	UINT idx, sub;
	float* data1;

	// 00001111:00101011 mod xmmreg r/m

	SSE4a_check_NM_EXCEPTION();
	SSE4a_setTag();
	CPU_SSE4AWORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0)
	{
		// XXX: ここはどう扱う?
		EXCEPTION(UD_EXCEPTION, 0);
		//data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
		//for(i=0;i<4;i++){
		//	data2[i] = data1[i];
		//}
}
	else
	{
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr + 0, *((UINT32*)(data1 + 0)));
	}
	TRACEOUT(("SSE4a_MOVNTSS"));
}
void SSE4a_INSERTQimm(void)
{
	UINT32 op, op2, op3;
	UINT idx, sub;
	UINT32 madr;
	UINT64 data2buf[2];
	UINT64* data1, * data2;
	UINT64 bitrangedx;

	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	GET_PCBYTE((op2));
	GET_PCBYTE((op3));
	bitrangedx = ((op2 & 63) == 0) ? ((UINT64)0xffffffffffffffff) : ((UINT64)((((SINT64)1) << (op2 & 63)) - ((SINT64)1)));
	data1[0] = (data1[0] & (~(bitrangedx << (op3 & 63)))) | ((data2[0] & bitrangedx) << (op3 & 63));

	TRACEOUT(("SSE4a_INSERTQimm"));
}
void SSE4a_INSERTQxmm(void)
{
	UINT32 op, op2, op3;
	UINT idx, sub;
	UINT32 madr;
	UINT64 data2buf[2];
	UINT64* data1, * data2;
	UINT64 bitrangedx;

	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	op2 = ((data2[1] >> (8 * 0)) & 63);
	op3 = ((data2[1] >> (8 * 1)) & 63);
	bitrangedx = ((op2 & 63) == 0) ? ((UINT64)0xffffffffffffffff) : ((UINT64)((((SINT64)1) << (op2 & 63)) - ((SINT64)1)));
	data1[0] = (data1[0] & (~(bitrangedx << (op3 & 63)))) | ((data2[0] & bitrangedx) << (op3 & 63));

	TRACEOUT(("SSE4a_INSERTQxmm"));
}
void SSE4a_EXTRQimm(void)
{
	UINT32 op, op2, op3;
	UINT idx, sub;
	UINT32 madr;
	UINT64 data2buf[2];
	UINT64* data1, * data2;
	UINT64 bitrangedx;

	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	GET_PCBYTE((op2));
	GET_PCBYTE((op3));
	bitrangedx = ((op2 & 63) == 0) ? ((UINT64)0xffffffffffffffff) : ((UINT64)((((SINT64)1) << (op2 & 63)) - ((SINT64)1)));
	data1[0] = (data1[0] & (~(bitrangedx))) | ((data1[0] >> (op3 & 63)) & bitrangedx);

	TRACEOUT(("SSE4a_EXTRQimm"));
}
void SSE4a_EXTRQxmm(void)
{
	UINT32 op, op2, op3;
	UINT idx, sub;
	UINT32 madr;
	UINT64 data2buf[2];
	UINT64* data1, * data2;
	UINT64 bitrangedx;

	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	op2 = ((data2[0] >> (8 * 0)) & 63);
	op3 = ((data2[0] >> (8 * 1)) & 63);
	bitrangedx = ((op2 & 63) == 0) ? ((UINT64)0xffffffffffffffff) : ((UINT64)((((SINT64)1) << (op2 & 63)) - ((SINT64)1)));
	data1[0] = (data1[0] & (~(bitrangedx))) | ((data1[0] >> (op3 & 63)) & bitrangedx);

	TRACEOUT(("SSE4a_EXTRQxmm"));
}

#else

void SSE4a_MOVNTSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE4a_MOVNTSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE4a_INSERTQimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE4a_INSERTQxmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE4a_EXTRQimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE4a_EXTRQxmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

#endif