/*
 * Copyright (c) 2018 SimK
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

#include <compiler.h>

#include <math.h>

#include <ia32/cpu.h>
#include "ia32/ia32.mcr"

#include "ia32/instructions/mmx/3dnow.h"

#if defined(USE_3DNOW) && defined(USE_MMX) && defined(USE_FPU)

#define CPU_MMXWORKCLOCK(a)	CPU_WORKCLOCK(2)

// 3DNow!
void AMD3DNOW_PAVGUSB_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PAVGUSB_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PF2ID_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PF2ID_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFACC_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFACC_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFADD_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFADD_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFCMPEQ_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFCMPEQ_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFCMPGE_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFCMPGE_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFCMPGT_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFCMPGT_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFMAX_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFMAX_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFMIN_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFMIN_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFMUL_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFMUL_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFRCP_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFRCP_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFRCPIT1_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFRCPIT1_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFRCPIT2_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFRCPIT2_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFRSQIT1_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFRSQIT1_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFRSQRT_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFRSQRT_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFSUB_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFSUB_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFSUBR_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFSUBR_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PI2FD_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PI2FD_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PMULHRW_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PMULHRW_MEM(UINT8 reg1, UINT32 memaddr);

// Enhanced 3DNow!
void AMD3DNOW_PF2IW_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PF2IW_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PI2FW_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PI2FW_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFNACC_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFNACC_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PFPNACC_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PFPNACC_MEM(UINT8 reg1, UINT32 memaddr);

void AMD3DNOW_PSWAPD_REG(UINT8 reg1, UINT8 reg2);
void AMD3DNOW_PSWAPD_MEM(UINT8 reg1, UINT32 memaddr);


static INLINE void
AMD3DNOW_check_NM_EXCEPTION(){
	// 3DNow!なしならUD(無効オペコード例外)を発生させる
	if(!(i386cpuid.cpu_feature & CPU_FEATURE_MMX) || !(i386cpuid.cpu_feature_ex & CPU_FEATURE_EX_3DNOW)){
		EXCEPTION(UD_EXCEPTION, 0);
	}
	// エミュレーションならUD(無効オペコード例外)を発生させる
	if(CPU_CR0 & CPU_CR0_EM){
		EXCEPTION(UD_EXCEPTION, 0);
	}
	// タスクスイッチ時にNM(デバイス使用不可例外)を発生させる
	if (CPU_CR0 & CPU_CR0_TS) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

static INLINE void
AMD3DNOW_setTag(void)
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
 * 3DNow! interface
 */
void
AMD3DNOW_FEMMS(void)
{
	int i;
	
	// MMXなしならUD(無効オペコード例外)を発生させる
	if(!(i386cpuid.cpu_feature & CPU_FEATURE_MMX)){
		EXCEPTION(UD_EXCEPTION, 0);
	}
	// エミュレーションならUD(無効オペコード例外)を発生させる
	if(CPU_CR0 & CPU_CR0_EM){
		EXCEPTION(UD_EXCEPTION, 0);
	}
	// タスクスイッチ時にNM(デバイス使用不可例外)を発生させる
	if ((CPU_CR0 & (CPU_CR0_TS)) || (CPU_CR0 & CPU_CR0_EM)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}

	CPU_WORKCLOCK(2);
	for (i = 0; i < FPU_REG_NUM; i++) {
		FPU_STAT.tag[i] = TAG_Empty;
	}
	FPU_STAT_TOP = 0;
	FPU_STATUSWORD &= ~0x3800;
	FPU_STATUSWORD |= (FPU_STAT_TOP&7)<<11;
	FPU_STAT.mmxenable = 0;
}

void
AMD3DNOW_PREFETCH(void)
{
	UINT32 op;
	UINT idx, sub;
	
	AMD3DNOW_check_NM_EXCEPTION();
	AMD3DNOW_setTag();
	CPU_MMXWORKCLOCK(a);
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		// Invalid Opcode
		EXCEPTION(UD_EXCEPTION, 0);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		switch(idx){
		case 1:
			// PREFETCHW
			break;
		case 0:
			// PREFETCH
		default:
			// Reserved(=PREFETCH)
			break;
		}
		// XXX: 何もしない
	}
}

void
AMD3DNOW_F0(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT8 suffix;
	UINT8 reg1;

	AMD3DNOW_check_NM_EXCEPTION();
	AMD3DNOW_setTag();
	CPU_MMXWORKCLOCK(a);
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		// mmreg1, mmreg2
		UINT8 reg2;
		reg1 = idx;
		reg2 = sub;
		GET_PCBYTE((suffix));
		switch(suffix){
		case AMD3DNOW_SUFFIX_PAVGUSB:
			AMD3DNOW_PAVGUSB_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFADD:
			AMD3DNOW_PFADD_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFSUB:
			AMD3DNOW_PFSUB_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFSUBR:
			AMD3DNOW_PFSUBR_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFACC:
			AMD3DNOW_PFACC_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFCMPGE:
			AMD3DNOW_PFCMPGE_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFCMPGT:
			AMD3DNOW_PFCMPGT_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFCMPEQ:
			AMD3DNOW_PFCMPEQ_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFMIN:
			AMD3DNOW_PFMIN_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFMAX:
			AMD3DNOW_PFMAX_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PI2FD:
			AMD3DNOW_PI2FD_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PF2ID:
			AMD3DNOW_PI2FD_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFRCP:
			AMD3DNOW_PFRCP_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFRSQRT:
			AMD3DNOW_PFRSQRT_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFMUL:
			AMD3DNOW_PFMUL_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFRCPIT1:
			AMD3DNOW_PFRCPIT1_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFRSQIT1:
			AMD3DNOW_PFRSQIT1_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFRCPIT2:
			AMD3DNOW_PFRCPIT2_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PMULHRW:
			AMD3DNOW_PMULHRW_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PF2IW:
			AMD3DNOW_PF2IW_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PI2FW:
			AMD3DNOW_PI2FW_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFNACC:
			AMD3DNOW_PFNACC_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PFPNACC:
			AMD3DNOW_PFPNACC_REG(reg1, reg2);
			break;
		case AMD3DNOW_SUFFIX_PSWAPD:
			AMD3DNOW_PSWAPD_REG(reg1, reg2);
			break;
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	} else {
		// mmreg1, mem
		UINT32 memaddr;
		reg1 = idx;
		memaddr = calc_ea_dst(op);
		GET_PCBYTE((suffix));
		switch(suffix){
		case AMD3DNOW_SUFFIX_PAVGUSB:
			AMD3DNOW_PAVGUSB_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFADD:
			AMD3DNOW_PFADD_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFSUB:
			AMD3DNOW_PFSUB_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFSUBR:
			AMD3DNOW_PFSUBR_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFACC:
			AMD3DNOW_PFACC_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFCMPGE:
			AMD3DNOW_PFCMPGE_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFCMPGT:
			AMD3DNOW_PFCMPGT_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFCMPEQ:
			AMD3DNOW_PFCMPEQ_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFMIN:
			AMD3DNOW_PFMIN_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFMAX:
			AMD3DNOW_PFMAX_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PI2FD:
			AMD3DNOW_PI2FD_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PF2ID:
			AMD3DNOW_PI2FD_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFRCP:
			AMD3DNOW_PFRCP_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFRSQRT:
			AMD3DNOW_PFRSQRT_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFMUL:
			AMD3DNOW_PFMUL_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFRCPIT1:
			AMD3DNOW_PFRCPIT1_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFRSQIT1:
			AMD3DNOW_PFRSQIT1_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFRCPIT2:
			AMD3DNOW_PFRCPIT2_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PMULHRW:
			AMD3DNOW_PMULHRW_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PF2IW:
			AMD3DNOW_PF2IW_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PI2FW:
			AMD3DNOW_PI2FW_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFNACC:
			AMD3DNOW_PFNACC_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PFPNACC:
			AMD3DNOW_PFPNACC_MEM(reg1, memaddr);
			break;
		case AMD3DNOW_SUFFIX_PSWAPD:
			AMD3DNOW_PSWAPD_MEM(reg1, memaddr);
			break;
		default:
			EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
	}
}

// PAVGUSB
void AMD3DNOW_PAVGUSB(UINT8 *data1, UINT8 *data2){
	int i;
	for(i=0;i<8;i++){
		data1[i] = (UINT8)(((UINT16)(data1[i]) + (UINT16)(data2[i]) + 1) / 2);
	}
}
void AMD3DNOW_PAVGUSB_REG(UINT8 reg1, UINT8 reg2){
	UINT8 *data1 = (UINT8*)(&(FPU_STAT.reg[reg1]));
	UINT8 *data2 = (UINT8*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PAVGUSB(data1, data2);
}
void AMD3DNOW_PAVGUSB_MEM(UINT8 reg1, UINT32 memaddr){
	UINT8 *data1 = (UINT8*)(&(FPU_STAT.reg[reg1]));
	UINT8 data2[8];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PAVGUSB(data1, data2);
}

// PF2ID
void AMD3DNOW_PF2ID(SINT32 *data1, float *data2){
	int i;
	for(i=0;i<2;i++){
		if (data2[i] >= (float)((SINT32)0x7fffffff)){
			data1[i] = (SINT32)0x7fffffff;
		}else if(data2[i] <= (float)((SINT32)0x80000000)){
			data1[i] = (SINT32)0x80000000;
		}else{
			data1[i] = (SINT32)data2[i];
		}
	}
}
void AMD3DNOW_PF2ID_REG(UINT8 reg1, UINT8 reg2){
	SINT32 *data1 = (SINT32*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PF2ID(data1, data2);
}
void AMD3DNOW_PF2ID_MEM(UINT8 reg1, UINT32 memaddr){
	SINT32 *data1 = (SINT32*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PF2ID(data1, data2);
}

// PFACC
void AMD3DNOW_PFACC(float *data1, float *data2){
	float temp[2];
	temp[0] = data2[0];
	temp[1] = data2[1];
	data1[0] = data1[0] + data1[1];
	data1[1] = temp[0] + temp[1];
}
void AMD3DNOW_PFACC_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFACC(data1, data2);
}
void AMD3DNOW_PFACC_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFACC(data1, data2);
}

// PFADD
void AMD3DNOW_PFADD(float *data1, float *data2){
	data1[0] = data1[0] + data2[0];
	data1[1] = data1[1] + data2[1];
}
void AMD3DNOW_PFADD_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFADD(data1, data2);
}
void AMD3DNOW_PFADD_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFADD(data1, data2);
}

// PFCMPEQ
void AMD3DNOW_PFCMPEQ(float *data1, float *data2){
	*((UINT32*)(data1+0)) = (data1[0] == data2[0] ? 0xffffffff : 0x00000000);
	*((UINT32*)(data1+1)) = (data1[1] == data2[1] ? 0xffffffff : 0x00000000);
}
void AMD3DNOW_PFCMPEQ_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFCMPEQ(data1, data2);
}
void AMD3DNOW_PFCMPEQ_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFCMPEQ(data1, data2);
}

// PFCMPGE
void AMD3DNOW_PFCMPGE(float *data1, float *data2){
	*((UINT32*)(data1+0)) = (data1[0] >= data2[0] ? 0xffffffff : 0x00000000);
	*((UINT32*)(data1+1)) = (data1[1] >= data2[1] ? 0xffffffff : 0x00000000);
}
void AMD3DNOW_PFCMPGE_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFCMPGE(data1, data2);
}
void AMD3DNOW_PFCMPGE_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFCMPGE(data1, data2);
}

// PFCMPGT
void AMD3DNOW_PFCMPGT(float *data1, float *data2){
	*((UINT32*)(data1+0)) = (data1[0] > data2[0] ? 0xffffffff : 0x00000000);
	*((UINT32*)(data1+1)) = (data1[1] > data2[1] ? 0xffffffff : 0x00000000);
}
void AMD3DNOW_PFCMPGT_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFCMPGT(data1, data2);
}
void AMD3DNOW_PFCMPGT_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFCMPGT(data1, data2);
}

// PFMAX
void AMD3DNOW_PFMAX(float *data1, float *data2){
	data1[0] = (data1[0] > data2[0] ? data1[0] : data2[0]);
	data1[1] = (data1[1] > data2[1] ? data1[1] : data2[1]);
}
void AMD3DNOW_PFMAX_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFMAX(data1, data2);
}
void AMD3DNOW_PFMAX_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFMAX(data1, data2);
}

// PFMIN
void AMD3DNOW_PFMIN(float *data1, float *data2){
	data1[0] = (data1[0] < data2[0] ? data1[0] : data2[0]);
	data1[1] = (data1[1] < data2[1] ? data1[1] : data2[1]);
}
void AMD3DNOW_PFMIN_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFMIN(data1, data2);
}
void AMD3DNOW_PFMIN_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFMIN(data1, data2);
}

// PFMUL
void AMD3DNOW_PFMUL(float *data1, float *data2){
	data1[0] = data1[0] * data2[0];
	data1[1] = data1[1] * data2[1];
}
void AMD3DNOW_PFMUL_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFMUL(data1, data2);
}
void AMD3DNOW_PFMUL_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFMUL(data1, data2);
}

// PFRCP
void AMD3DNOW_PFRCP(float *data1, float *data2){
	// XXX: ただのコピー
	data1[0] = 1.0f / data2[0];
	data1[1] = 1.0f / data2[1];
}
void AMD3DNOW_PFRCP_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFRCP(data1, data2);
}
void AMD3DNOW_PFRCP_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFRCP(data1, data2);
}

// PFRCPIT1
void AMD3DNOW_PFRCPIT1(float *data1, float *data2){
	// XXX: ただのコピーで代替
	data1[0] = data2[0];
	data1[1] = data2[1];
}
void AMD3DNOW_PFRCPIT1_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFRCPIT1(data1, data2);
}
void AMD3DNOW_PFRCPIT1_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFRCPIT1(data1, data2);
}

// PFRCPIT2
void AMD3DNOW_PFRCPIT2(float *data1, float *data2){
	// XXX: ただのコピーで代替
	data1[0] = data2[0];
	data1[1] = data2[1];
}
void AMD3DNOW_PFRCPIT2_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFRCPIT2(data1, data2);
}
void AMD3DNOW_PFRCPIT2_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFRCPIT2(data1, data2);
}

// PFRSQIT1
void AMD3DNOW_PFRSQIT1(float *data1, float *data2){
	// XXX: ただのコピーで代替
	data1[0] = data2[0];
	data1[1] = data2[1];
}
void AMD3DNOW_PFRSQIT1_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFRSQIT1(data1, data2);
}
void AMD3DNOW_PFRSQIT1_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFRSQIT1(data1, data2);
}

// PFRSQRT
void AMD3DNOW_PFRSQRT(float *data1, float *data2){
	data1[0] = (float)(1.0f / sqrt(data2[0]));
	data1[1] = (float)(1.0f / sqrt(data2[1]));
}
void AMD3DNOW_PFRSQRT_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFRSQRT(data1, data2);
}
void AMD3DNOW_PFRSQRT_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFRSQRT(data1, data2);
}

// PFSUB
void AMD3DNOW_PFSUB(float *data1, float *data2){
	data1[0] = data1[0] - data2[0];
	data1[1] = data1[1] - data2[1];
}
void AMD3DNOW_PFSUB_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFSUB(data1, data2);
}
void AMD3DNOW_PFSUB_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFSUB(data1, data2);
}

// PFSUBR
void AMD3DNOW_PFSUBR(float *data1, float *data2){
	data1[0] = data2[0] - data1[0];
	data1[1] = data2[1] - data1[1];
}
void AMD3DNOW_PFSUBR_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFSUBR(data1, data2);
}
void AMD3DNOW_PFSUBR_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFSUBR(data1, data2);
}

// PI2FD
void AMD3DNOW_PI2FD(float *data1, SINT32 *data2){
	data1[0] = (float)(data2[0]);
	data1[1] = (float)(data2[1]);
}
void AMD3DNOW_PI2FD_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	SINT32 *data2 = (SINT32*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PI2FD(data1, data2);
}
void AMD3DNOW_PI2FD_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	SINT32 data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PI2FD(data1, data2);
}

// PMULHRW
void AMD3DNOW_PMULHRW(SINT16 *data1, SINT16 *data2){
	int i;
	for(i=0;i<4;i++){
		data1[i] = (UINT16)((((UINT32)data1[i] * (UINT32)data2[i] + 0x8000) >> 16) & 0xffff);
	}
}
void AMD3DNOW_PMULHRW_REG(UINT8 reg1, UINT8 reg2){
	SINT16 *data1 = (SINT16*)(&(FPU_STAT.reg[reg1]));
	SINT16 *data2 = (SINT16*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PMULHRW(data1, data2);
}
void AMD3DNOW_PMULHRW_MEM(UINT8 reg1, UINT32 memaddr){
	SINT16 *data1 = (SINT16*)(&(FPU_STAT.reg[reg1]));
	SINT16 data2[4];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PMULHRW(data1, data2);
}

/*
 * Enhanced 3DNow! interface
 */
// PF2IW
void AMD3DNOW_PF2IW(SINT32 *data1, float *data2){
	int i;
	for(i=0;i<2;i++){
		if (data2[i] >= (float)((SINT16)0x7fff)){
			data1[i] = (SINT16)0x7fff;
		}else if(data2[i] <= (float)((SINT16)0x8000)){
			data1[i] = (SINT16)0x8000;
		}else{
			data1[i] = (SINT16)data2[i];
		}
	}
}
void AMD3DNOW_PF2IW_REG(UINT8 reg1, UINT8 reg2){
	SINT32 *data1 = (SINT32*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PF2IW(data1, data2);
}
void AMD3DNOW_PF2IW_MEM(UINT8 reg1, UINT32 memaddr){
	SINT32 *data1 = (SINT32*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PF2IW(data1, data2);
}

// PI2FW
void AMD3DNOW_PI2FW(float *data1, SINT16 *data2){
	data1[0] = (float)(data2[0]);
	data1[1] = (float)(data2[2]);
}
void AMD3DNOW_PI2FW_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	SINT16 *data2 = (SINT16*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PI2FW(data1, data2);
}
void AMD3DNOW_PI2FW_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	SINT16 data2[4];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PI2FW(data1, data2);
}

// PFNACC
void AMD3DNOW_PFNACC(float *data1, float *data2){
	float temp[2];
	temp[0] = data2[0];
	temp[1] = data2[1];
	data1[0] = data1[0] - data1[1];
	data1[1] = temp[0] - temp[1];
}
void AMD3DNOW_PFNACC_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFNACC(data1, data2);
}
void AMD3DNOW_PFNACC_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFNACC(data1, data2);
}

// PFPNACC
void AMD3DNOW_PFPNACC(float *data1, float *data2){
	float temp[2];
	temp[0] = data2[0];
	temp[1] = data2[1];
	data1[0] = data1[0] - data1[1];
	data1[1] = temp[0] + temp[1];
}
void AMD3DNOW_PFPNACC_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PFPNACC(data1, data2);
}
void AMD3DNOW_PFPNACC_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PFPNACC(data1, data2);
}

// PSWAPD
void AMD3DNOW_PSWAPD(float *data1, float *data2){
	float temp[2];
	temp[0] = data2[0];
	temp[1] = data2[1];
	data1[0] = temp[1];
	data1[1] = temp[0];
}
void AMD3DNOW_PSWAPD_REG(UINT8 reg1, UINT8 reg2){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float *data2 = (float*)(&(FPU_STAT.reg[reg2]));
	AMD3DNOW_PSWAPD(data1, data2);
}
void AMD3DNOW_PSWAPD_MEM(UINT8 reg1, UINT32 memaddr){
	float *data1 = (float*)(&(FPU_STAT.reg[reg1]));
	float data2[2];
	*((UINT64*)data2) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, memaddr);
	AMD3DNOW_PSWAPD(data1, data2);
}

#else

/*
 * 3DNow! interface
 */
void AMD3DNOW_F0(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void AMD3DNOW_FEMMS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void AMD3DNOW_PREFETCH(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

#endif