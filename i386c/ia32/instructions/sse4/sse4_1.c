/*
 * Copyright (c) 2024 Gocaine Project
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

#include <math.h>
#include <float.h>

#define isnan(x) (_isnan(x))

#include "../../cpu.h"
#include "../../ia32.mcr"

#include "../sse/sse.h"
#include "../sse2/sse2.h"
#include "../sse3/sse3.h"
#include "../ssse3/ssse3.h"
#include "sse4_1.h"

#if defined(USE_SSE4_1) && defined(USE_SSSE3) && defined(USE_SSE3) && defined(USE_SSE2) && defined(USE_SSE) && defined(USE_FPU)

#define CPU_SSSE3WORKCLOCK	CPU_WORKCLOCK(2)

static INLINE void
SSE4_1_check_NM_EXCEPTION(){
	// SSE4.1 Ȃ  Ȃ UD(     I y R [ h  O) 𔭐       
	if(!(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_SSE4_1)){
		EXCEPTION(UD_EXCEPTION, 0);
	}
	//  G ~     [ V     Ȃ UD(     I y R [ h  O) 𔭐       
	if(CPU_CR0 & CPU_CR0_EM){
		EXCEPTION(UD_EXCEPTION, 0);
	}
	//  ^ X N X C b `    NM( f o C X g p s  O) 𔭐       
	if (CPU_CR0 & CPU_CR0_TS) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

static INLINE void
SSE4_1_setTag(void)
{
}

// mmx.c ̂ ̂Ɠ   
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


static INLINE float NEARBYINTF(float val)
{
	float floorval;
	floorval = floorf(val);
	if (val - floorval > 0.5f){
		return (floorval + 1); // 切り上げ
	}else if (val - floorval < 0.5f){
		return (floorval); // 切り捨て
	}else{
		if(floor(floorval / 2) == floorval/2){
			return (floorval); // 偶数
		}else{
			return (floorval+1); // 奇数
		}
	}
}
static INLINE float TRUNCF(float val)
{
	if(val < 0){
		return ceilf(val); // ゼロ方向への切り捨て
	}else{
		return floorf(val); // ゼロ方向への切り捨て
	}
}
static INLINE double NEARBYINT(double val)
{
	double floorval;
	floorval = floor(val);
	if (val - floorval > 0.5f){
		return (floorval + 1); // 切り上げ
	}else if (val - floorval < 0.5f){
		return (floorval); // 切り捨て
	}else{
		if(floor(floorval / 2) == floorval/2){
			return (floorval); // 偶数
		}else{
			return (floorval+1); // 奇数
		}
	}
}
static INLINE double TRUNC(double val)
{
	if(val < 0){
		return ceil(val); // ゼロ方向への切り捨て
	}else{
		return floor(val); // ゼロ方向への切り捨て
	}
}

/*
 * SSE3 interface
 */

//  R [ h       Ȃ ̂ł ⋭   ɋ  ʉ 
// xmm/m128 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_PD(double **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
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

static INLINE void SSE_PART_GETDATA1DATA2_P_UINT32(UINT32 **data1, UINT32 **data2, UINT32 *data2buf){
	SSE_PART_GETDATA1DATA2_PD((double**)data1, (double**)data2, (double*)data2buf);
}
static INLINE void SSE_PART_GETDATA1DATA2_PD_UINT64(UINT64 **data1, UINT64 **data2, UINT64 *data2buf){
	SSE_PART_GETDATA1DATA2_PD((double**)data1, (double**)data2, (double*)data2buf);
}

static INLINE UINT32 SSE_PART_GETDATA1DATA2_PD_MODRM(double **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
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
	return op;
}

static INLINE void MMX_PART_GETDATA1DATA2_PD(float **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
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

void SSE4_1_PBLENDVB(void)
{
	int i;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<16;i++){
		if (FPU_STAT.xmm_reg[0].ul8[i]&128){
			data1[i] = data2[i];
		} else {
			data1[i] = data1[i];
		}
	}
	TRACEOUT(("SSE4_1_PBLENDVB"));
}

void SSE4_1_BLENDVPS(void)
{
	int i;

	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		if (FPU_STAT.xmm_reg[0].ul32[i]&((UINT32)0x80000000)){
			data1[i] = data2[i];
		} else {
			data1[i] = data1[i];
		}
	}
	TRACEOUT(("SSE4_1_BLENDVPS"));
}

void SSE4_1_BLENDVPD(void)
{
	int i;

	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		if (FPU_STAT.xmm_reg[0].ul32[i]&((UINT64)0x8000000000000000)){
			data1[i] = data2[i];
		} else {
			data1[i] = data1[i];
		}
	}
	TRACEOUT(("SSE4_1_BLENDVPD"));
}

void SSE4_1_VPTEST(void)
{
	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	if ((data1[0] & data2[0]) == 0 && (data1[1] & data2[1]) == 0){
		CPU_FLAG |=  Z_FLAG;
		CPU_FLAG &= ~C_FLAG;
	} else {
		CPU_FLAG &= ~Z_FLAG;
		CPU_FLAG |=  C_FLAG;
	}
	TRACEOUT(("SSE4_1_VPTEST"));
}

void SSE4_1_PMOVSXBW(void)
{
	int i;

	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<8;i++){
		*((SINT16*)(data1 + (i * 2))) = (SINT16)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVSXBW"));
}

void SSE4_1_PMOVSXBD(void)
{
	int i;

	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		*((SINT32*)(data1 + (i * 4))) = (SINT32)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVSXBD"));
}

void SSE4_1_PMOVSXBQ(void)
{
	int i;

	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		*((SINT64*)(data1 + (i * 8))) = (SINT64)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVSXBQ"));
}

void SSE4_1_PMOVSXWD(void)
{
	int i;

	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		*((SINT32*)(data1 + (i * 4))) = (SINT32)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVSXWD"));
}

void SSE4_1_PMOVSXWQ(void)
{
	int i;

	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		*((SINT64*)(data1 + (i * 8))) = (SINT64)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVSXWQ"));
}

void SSE4_1_PMOVSXDQ(void)
{
	int i;

	SINT32 data2buf[4];
	SINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		*((SINT64*)(data1 + (i * 8))) = (SINT64)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVSXDQ"));
}

void SSE4_1_PMOVZXBW(void)
{
	int i;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<8;i++){
		*((UINT16*)(data1 + (i * 2))) = (UINT16)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVZXBW"));
}

void SSE4_1_PMOVZXBD(void)
{
	int i;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		*((UINT32*)(data1 + (i * 4))) = (UINT32)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVZXBD"));
}

void SSE4_1_PMOVZXBQ(void)
{
	int i;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		*((UINT64*)(data1 + (i * 8))) = (UINT64)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVZXBQ"));
}

void SSE4_1_PMOVZXWD(void)
{
	int i;

	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		*((UINT32*)(data1 + (i * 4))) = (UINT32)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVZXWD"));
}

void SSE4_1_PMOVZXWQ(void)
{
	int i;

	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		*((UINT64*)(data1 + (i * 8))) = (UINT64)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVZXWQ"));
}

void SSE4_1_PMOVZXDQ(void)
{
	int i;

	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		*((UINT64*)(data1 + (i * 8))) = (UINT64)data2[i];
	}
	TRACEOUT(("SSE4_1_PMOVZXDQ"));
}

void SSE4_1_PMULLD(void)
{
	int i;

	UINT64 tmp[4];
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		tmp[i] = data1[i] * data2[i];
		data1[i] = tmp[i];
	}
	TRACEOUT(("SSE4_1_PMULLD"));
}

void SSE4_1_PHMINPOSUW(void)
{
	int i;

	UINT16 min;
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	min = data2[0];
	data1[1] = 7;
	for(i=0;i<7;i++){
		if (data2[i+1] < min){data1[1] = ((i+1) & 7); data1[0] = data2[i+1]; break;}
		min = data2[i+1];
	}
	for(i=2;i<8;i++){
		data1[i] = 0;
	}
	TRACEOUT(("SSE4_1_PHMINPOSUW"));
}

void SSE4_1_PMULDQ(void)
{
	int i;

	SINT64 tmp[2];
	SINT32 data2buf[4];
	SINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		tmp[i] = (SINT64)data1[i * 2] * (SINT64)data2[i * 2];
		*((SINT64*)(data1 + (i * 8))) = tmp[i];
	}
	TRACEOUT(("SSE4_1_PMULDQ"));
}

void SSE4_1_PCMPEQQ(void)
{
	int i;

	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		if (data1[i] == data2[i]){
			data1[i] = 0xFFFFFFFFFFFFFFFF;
		} else {
			data1[i] = 0;
		}
	}
	TRACEOUT(("SSE4_1_PCMPEQQ"));
}

void SSE4_1_MOVNTDQA(void)
{
	int i;

	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		data1[i] = data2[i];
	}
	TRACEOUT(("SSE4_1_MOVNTDQA"));
}

void SSE4_1_PACKUSDW(void)
{
	int i;

	UINT16 tmp[8];
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		tmp[i + 0] = ((*((UINT32*)(data1 + (i * 2)))) < 0) ? 0 : data1[i * 2];
		data1[i] = ((*((UINT32*)(data1 + (i * 2)))) > 0xFFFF) ? 0xFFFF : tmp[i];
	}
	for(i=0;i<4;i++){
		tmp[i + 4] = ((*((UINT32*)(data2 + (i * 2)))) < 0) ? 0 : data2[i * 2];
		data1[i] = ((*((UINT32*)(data2 + (i * 2)))) > 0xFFFF) ? 0xFFFF : tmp[i + 4];
	}
	TRACEOUT(("SSE4_1_PACKUSDW"));
}

void SSE4_1_PMINSB(void)
{
	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE4_1_PMINSB"));
}

void SSE4_1_PMINSD(void)
{
	SINT32 data2buf[16];
	SINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE4_1_PMINSD"));
}

void SSE4_1_PMINUW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE4_1_PMINUW"));
}

void SSE4_1_PMINUD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE4_1_PMINUD"));
}

void SSE4_1_PMAXSB(void)
{
	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE4_1_PMAXSB"));
}

void SSE4_1_PMAXSD(void)
{
	SINT32 data2buf[4];
	SINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE4_1_PMAXSD"));
}

void SSE4_1_PMAXUW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE4_1_PMAXUW"));
}

void SSE4_1_PMAXUD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE4_1_PMAXUW"));
}

void SSE4_1_PEXTRB(void)
{
	UINT32 op2;
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;

	UINT32 *out;

	UINT32 op;
	UINT idx, sub;

	UINT32 maddr;

	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT8*)(&(FPU_STAT.xmm_reg[idx]));
	GET_PCBYTE((op2));
	if ((op) >= 0xc0) {
		out = CPU_REG32_B20(op);
		*out = (UINT32)(data1[op2&0xF]&0xFF);
	} else {
		maddr = calc_ea_dst((op));
		cpu_vmemorywrite_b(CPU_INST_SEGREG_INDEX, maddr+ 0, (UINT8)data1[op2&0xF]);
	}
	TRACEOUT(("SSE4_1_PEXTRB"));
}

void SSE4_1_PEXTRW(void)
{
	UINT32 op2;
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;

	UINT32 *out;

	UINT32 op;
	UINT idx, sub;

	UINT32 maddr;

	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT16*)(&(FPU_STAT.xmm_reg[idx]));
	GET_PCBYTE((op2));
	if ((op) >= 0xc0) {
		out = CPU_REG32_B20(op);
		*out = (UINT32)(data1[op2&0x7]&0xFFFF);
	} else {
		maddr = calc_ea_dst((op));
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, maddr+ 0, (UINT16)data1[op2&0x7]);
	}
	TRACEOUT(("SSE4_1_PEXTRW"));
}

void SSE4_1_PEXTRD(void)
{
	UINT32 op2;
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;

	UINT32 *out;

	UINT32 op;
	UINT idx, sub;

	UINT32 maddr;

	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT32*)(&(FPU_STAT.xmm_reg[idx]));
	GET_PCBYTE((op2));
	if ((op) >= 0xc0) {
		out = CPU_REG32_B20(op);
		*out = (UINT32)(data1[op2&0x3]&0xFFFFFFFF);
	} else {
		maddr = calc_ea_dst((op));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, maddr+ 0, (UINT32)data1[op2&0x3]);
	}
	TRACEOUT(("SSE4_1_PEXTRD"));
}

void SSE4_1_PINSRB(void)
{
	UINT32 op2;
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;

	UINT32 *out;

	UINT32 op;
	UINT idx, sub;

	UINT32 maddr;

	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT8*)(&(FPU_STAT.xmm_reg[idx]));
	GET_PCBYTE((op2));
	if ((op) >= 0xc0) {
		out = CPU_REG32_B20(op);
		data1[op2&0xF] = *out;
	} else {
		maddr = calc_ea_dst((op));
		data1[op2&0xF] = cpu_vmemoryread_b(CPU_INST_SEGREG_INDEX, maddr+ 0);
	}
	TRACEOUT(("SSE4_1_PINSRB"));
}

void SSE4_1_PINSRW(void)
{
	UINT32 op2;
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;

	UINT32 *out;

	UINT32 op;
	UINT idx, sub;

	UINT32 maddr;

	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT16*)(&(FPU_STAT.xmm_reg[idx]));
	GET_PCBYTE((op2));
	if ((op) >= 0xc0) {
		out = CPU_REG32_B20(op);
		data1[op2&0x7] = *out;
	} else {
		maddr = calc_ea_dst((op));
		data1[op2&0x7] = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, maddr+ 0);
	}
	TRACEOUT(("SSE4_1_PINSRW"));
}

void SSE4_1_PINSRD(void)
{
	UINT32 op2;
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;

	UINT32 *out;

	UINT32 op;
	UINT idx, sub;

	UINT32 maddr;

	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT32*)(&(FPU_STAT.xmm_reg[idx]));
	GET_PCBYTE((op2));
	if ((op) >= 0xc0) {
		out = CPU_REG32_B20(op);
		data1[op2&0x3] = *out;
	} else {
		maddr = calc_ea_dst((op));
		data1[op2&0x3] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
	}
	TRACEOUT(("SSE4_1_PINSRD"));
}

void SSE4_1_PEXTRACTPS(void)
{
	UINT32 op2;
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;

	UINT32 *out; 
	UINT32 maddr; 
	
	UINT32 op;
	UINT idx, sub;
	
	SSE4_1_check_NM_EXCEPTION();
	SSE4_1_setTag();
	CPU_SSSE3WORKCLOCK;
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		data2 = (UINT32*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		maddr = calc_ea_dst((op));
		*((UINT64*)(data2buf+ 0)) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*((UINT64*)(data2buf+ 1)) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, maddr+ 8);
		data2 = data2buf;
	}
	GET_PCBYTE((op2));

	if (op >= 0xc0) {
		out = CPU_REG32_B20(op);
		*out = data2[op2&0x3] & 0xFFFFFFFF;
	} else {
		maddr = calc_ea_dst((op));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, maddr+ 0, (UINT32)(data2[op2&0x3] & 0xFFFFFFFF));
	}
	TRACEOUT(("SSE4_1_PEXTRACTPS"));
}

void SSE4_1_INSERTPS(void)
{
	int i;

	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	UINT32 op;
	UINT32 tmp = SSE_PART_GETDATA1DATA2_PD_MODRM((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	data1[(op>>4)&3] = data2[(tmp >= 0xc0) ? ((op>>6)&3) : 0];
	for(i=0;i<4;i++){
		if (op & (1 << i)){
			data1[i] = 0;
		}
	}
	TRACEOUT(("SSE4_1_INSERTPS"));
}

void SSE4_1_DPPS(void)
{
	int i;

	float data2buf[4];
	float *data1, *data2;
	float tmpsinedcalc = 0.0f;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	for(i=0;i<4;i++){
		if (op&(1 << (i + 4))){
			tmpsinedcalc += data1[i] * data2[i];
		}
	}
	for(i=0;i<4;i++){
		data1[i] = (op&(1 << i)) ? tmpsinedcalc : 0.0f;
	}
	TRACEOUT(("SSE4_1_DPPS"));
}

void SSE4_1_DPPD(void)
{
	int i;

	double data2buf[2];
	double *data1, *data2;
	double tmpsinedcalc = 0.0;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	for(i=0;i<2;i++){
		if (op&(1 << (i + 4))){
			tmpsinedcalc += data1[i] * data2[i];
		}
	}
	for(i=0;i<2;i++){
		data1[i] = (op&(1 << i)) ? tmpsinedcalc : 0.0;
	}
	TRACEOUT(("SSE4_1_DPPD"));
}

void SSE4_1_MPSADBW(void)
{
	int i;

	UINT8 data2buf[8];
	UINT8 *data1, *data2;
	SINT32 tmpsinedcalcb[11];
	SINT32 tmpsinedcalc;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	for(i=0;i<11;i++){
		tmpsinedcalcb[i] = data1[((op >> 2) & 1)*4 + i];
	}
	for(i=0;i<8;i++){
		tmpsinedcalc  = abs(tmpsinedcalcb[i+0]-data2[(op & 3) * 4 + 0]);
		tmpsinedcalc += abs(tmpsinedcalcb[i+1]-data2[(op & 3) * 4 + 1]);
		tmpsinedcalc += abs(tmpsinedcalcb[i+2]-data2[(op & 3) * 4 + 2]);
		tmpsinedcalc += abs(tmpsinedcalcb[i+3]-data2[(op & 3) * 4 + 3]);
		*((UINT16*)(data1[2 * i])) = tmpsinedcalc;
	}
	TRACEOUT(("SSE4_1_MPSADBW"));
}

void SSE4_1_ROUNDPS(void)
{
	int i;

	float data2buf[4];
	float *data1, *data2;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	for(i=0;i<4;i++){
		switch ((op & 4) ? ((CPU_MXCSR >> 13) & 3) : (op & 3)){
			case 0:
				data1[i] = NEARBYINTF(data2[i]);
			break;
			case 1:
				data1[i] = floorf(data2[i]);
			break;
			case 2:
				data1[i] = ceilf(data2[i]);
			break;
			case 3:
				data1[i] = TRUNCF(data2[i]);
			break;
		}
	}
	TRACEOUT(("SSE4_1_ROUNDPS"));
}

void SSE4_1_ROUNDPD(void)
{
	int i;

	double data2buf[2];
	double *data1, *data2;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	for(i=0;i<2;i++){
		switch ((op & 4) ? ((CPU_MXCSR >> 13) & 3) : (op & 3)){
			case 0:
				data1[i] = NEARBYINT(data2[i]);
			break;
			case 1:
				data1[i] = floor(data2[i]);
			break;
			case 2:
				data1[i] = ceil(data2[i]);
			break;
			case 3:
				data1[i] = TRUNC(data2[i]);
			break;
		}
	}
	TRACEOUT(("SSE4_1_ROUNDPD"));
}

void SSE4_1_ROUNDSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	switch ((op & 4) ? ((CPU_MXCSR >> 13) & 3) : (op & 3)){
		case 0:
			data1[0] = NEARBYINTF(data2[0]);
		break;
		case 1:
			data1[0] = floorf(data2[0]);
		break;
		case 2:
			data1[0] = ceilf(data2[0]);
		break;
		case 3:
			data1[0] = TRUNCF(data2[0]);
		break;
	}
	TRACEOUT(("SSE4_1_ROUNDSS"));
}

void SSE4_1_ROUNDSD(void)
{
	double data2buf[2];
	double *data1, *data2;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	switch ((op & 4) ? ((CPU_MXCSR >> 13) & 3) : (op & 3)){
		case 0:
			data1[0] = NEARBYINT(data2[0]);
		break;
		case 1:
			data1[0] = floor(data2[0]);
		break;
		case 2:
			data1[0] = ceil(data2[0]);
		break;
		case 3:
			data1[0] = TRUNC(data2[0]);
		break;
	}
	TRACEOUT(("SSE4_1_ROUNDSD"));
}

void SSE4_1_PBLENDPS(void)
{
	int i;

	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	for(i=0;i<4;i++){
		if (op & (1 << i)){
			data1[i] = data2[i];
		}
	}
	TRACEOUT(("SSE4_1_PBLENDPS"));
}

void SSE4_1_PBLENDPD(void)
{
	int i;

	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	for(i=0;i<2;i++){
		if (op & (1 << i)){
			data1[i] = data2[i];
		}
	}
	TRACEOUT(("SSE4_1_PBLENDPD"));
}

void SSE4_1_PBLENDW(void)
{
	int i;

	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	UINT32 op;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);

	GET_MODRM_PCBYTE((op));
	for(i=0;i<8;i++){
		if (op & (1 << i)){
			data1[i] = data2[i];
		}
	}
	TRACEOUT(("SSE4_1_PBLENDPD"));
}

#else

void SSE4_1_PBLENDVB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_BLENDVPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_BLENDVPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_VPTEST(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVSXBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVSXBD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVSXBQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVSXWD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVSXWQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVSXDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVZXBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVZXBD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVZXBQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVZXWD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVZXWQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMOVZXDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMULLD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PHMINPOSUW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMULDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PCMPEQQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_MOVNTDQA(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PACKUSDW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMINSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMINSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMINUW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMINUD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMAXSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMAXSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMAXUW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PMAXUD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PEXTRB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PEXTRW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PEXTRD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PINSRB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PINSRW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PINSRD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PEXTRACTPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_INSERTPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_DPPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_DPPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_MPSADBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_ROUNDPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_ROUNDPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_ROUNDSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_ROUNDSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PBLENDPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PBLENDPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_1_PBLENDW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}


#endif