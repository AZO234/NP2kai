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
#include <float.h>

#if defined(_WIN32) && !defined(__LIBRETRO__)
#define isnan(x) (_isnan(x))
#endif

#include <ia32/cpu.h>
#include "ia32/ia32.mcr"

#include "ia32/instructions/sse/sse.h"

#if defined(USE_SSE) && defined(USE_FPU)

#define CPU_SSEWORKCLOCK	CPU_WORKCLOCK(8)

static INLINE void
SSE_check_NM_EXCEPTION(){
	// SSEなしならUD(無効オペコード例外)を発生させる
	if(!(i386cpuid.cpu_feature & CPU_FEATURE_SSE) && !(i386cpuid.cpu_feature_ex & CPU_FEATURE_EX_E3DNOW)){ // XXX: SSE命令にEnhanced 3DNow!命令が一部あるので例外的に認める
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
SSE_setTag(void)
{
//	int i;
//	
//	if(!FPU_STAT.mmxenable){
//		FPU_STAT.mmxenable = 1;
//		//FPU_CTRLWORD = 0x27F;
//		for (i = 0; i < FPU_REG_NUM; i++) {
//			FPU_STAT.tag[i] = TAG_Valid;
//#ifdef SUPPORT_FPU_DOSBOX2
//			FPU_STAT.int_regvalid[i] = 0;
//#endif
//			FPU_STAT.reg[i].ul.ext = 0xffff;
//		}
//	}
//	FPU_STAT_TOP = 0;
//	FPU_STATUSWORD &= ~0x3800;
//	FPU_STATUSWORD |= (FPU_STAT_TOP&7)<<11;
}

// mmx.cのものと同じ
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

float SSE_ROUND(float val){
	float floorval;
	int rndbit = (SSE_MXCSR >> 13) & 0x3;
	switch(rndbit){
	case 0:	
		floorval = (float)floor(val);
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
		break;
	case 1:
		return (float)floor(val);
	case 2:
		return (float)ceil(val);
	case 3:
		if(val < 0){
			return (float)ceil(val); // ゼロ方向への切り捨て
		}else{
			return (float)floor(val); // ゼロ方向への切り捨て
		}
		break;
	default:
		return val;
	}
}

/*
 * SSE interface
 */

// コードが長くなるのでやや強引に共通化
// xmm/m128 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_P(float **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*((UINT32*)(data2buf+ 1)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 4);
		*((UINT32*)(data2buf+ 2)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 8);
		*((UINT32*)(data2buf+ 3)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+12);
		*data2 = data2buf;
	}
}
static INLINE void SSE_PART_GETDATA1DATA2_P_UINT32(UINT32 **data1, UINT32 **data2, UINT32 *data2buf){
	SSE_PART_GETDATA1DATA2_P((float**)data1, (float**)data2, (float*)data2buf);
}
static INLINE void SSE_PART_GETDATA1DATA2_S(float **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}
static INLINE void SSE_PART_GETDATA1DATA2_S_UINT32(UINT32 **data1, UINT32 **data2, UINT32 *data2buf){
	SSE_PART_GETDATA1DATA2_S((float**)data1, (float**)data2, (float*)data2buf);
}

// mm/m64 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_P_MMX2XMM(float **data1, SINT32 **data2, SINT32 *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	MMX_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (SINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*((UINT32*)(data2buf+ 1)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 4);
		*data2 = data2buf;
	}
}
static INLINE void SSE_PART_GETDATA1DATA2_S_MMX2XMM(float **data1, SINT32 **data2, SINT32 *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	MMX_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (SINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}
// xmm/m64 -> mm
static INLINE void SSE_PART_GETDATA1DATA2_P_XMM2MMX(SINT32 **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	MMX_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (SINT32*)(&(FPU_STAT.reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*((UINT32*)(data2buf+ 1)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 4);
		*data2 = data2buf;
	}
}
static INLINE void SSE_PART_GETDATA1DATA2_S_XMM2MMX(SINT32 **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	MMX_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (SINT32*)(&(FPU_STAT.reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}

// reg/m32 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_S_REG2XMM(float **data1, SINT32 **data2, SINT32 *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (SINT32*)reg32_b20[(op)];
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}
static INLINE void SSE_PART_GETDATA1DATA2_S_XMM2REG(SINT32 **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (SINT32*)reg32_b53[(op)];
	if ((op) >= 0xc0) {
		*data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}

// mm/m64 -> mm
static INLINE void SSE_PART_GETDATA1DATA2_P_MMX2MMX_SB(SINT8 **data1, SINT8 **data2, SINT8 *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	MMX_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (SINT8*)(&(FPU_STAT.reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (SINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*((UINT32*)(data2buf+ 4)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 4);
		*data2 = data2buf;
	}
}
static INLINE void SSE_PART_GETDATA1DATA2_P_MMX2MMX_UB(UINT8 **data1, UINT8 **data2, UINT8 *data2buf){
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_SB((SINT8**)data1, (SINT8**)data2, (SINT8*)data2buf);
}
static INLINE void SSE_PART_GETDATA1DATA2_P_MMX2MMX_SW(SINT16 **data1, SINT16 **data2, SINT16 *data2buf){
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_SB((SINT8**)data1, (SINT8**)data2, (SINT8*)data2buf);
}
static INLINE void SSE_PART_GETDATA1DATA2_P_MMX2MMX_UW(UINT16 **data1, UINT16 **data2, UINT16 *data2buf){
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_SB((SINT8**)data1, (SINT8**)data2, (SINT8*)data2buf);
}
static INLINE void SSE_PART_GETDATA1DATA2_P_MMX2MMX_SD(SINT32 **data1, SINT32 **data2, SINT32 *data2buf){
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_SB((SINT8**)data1, (SINT8**)data2, (SINT8*)data2buf);
}
static INLINE void SSE_PART_GETDATA1DATA2_P_MMX2MMX_UD(UINT32 **data1, UINT32 **data2, UINT32 *data2buf){
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_SB((SINT8**)data1, (SINT8**)data2, (SINT8*)data2buf);
}


// 実際の命令群

void SSE_ADDPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = data1[i] + data2[i];
	}
}
void SSE_ADDSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	data1[0] = data1[0] + data2[0];
}
void SSE_ANDNPS(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_UINT32(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = ((~(data1[i])) & (data2[i]));
	}
}
void SSE_ANDPS(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_UINT32(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = ((data1[i]) & (data2[i]));
	}
}
void SSE_CMPPS(void)
{
	UINT32 idx;
	float data2buf[4];
	float *data1, *data2;
	UINT32 *data1ui32;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);

	data1ui32 = (UINT32*)data1;

	GET_PCBYTE((idx));
	switch(idx){
		case 0: // CMPEQPS
			for(i=0;i<4;i++){
				data1ui32[i] = (data1[i] == data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 1: // CMPLTPS
			for(i=0;i<4;i++){
				data1ui32[i] = (data1[i] < data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 2: // CMPLEPS
			for(i=0;i<4;i++){
				data1ui32[i] = (data1[i] <= data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 3: // CMPUNORDPS
			for(i=0;i<4;i++){
				data1ui32[i] = (isnan(data1[i]) || isnan(data2[i]) ? 0xffffffff : 0x00000000);
			}
			break;
		case 4: // CMPNEQPS
			for(i=0;i<4;i++){
				data1ui32[i] = (data1[i] != data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 5: // CMPNLTPS
			for(i=0;i<4;i++){
				data1ui32[i] = (data1[i] >= data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 6: // CMPNLEPS
			for(i=0;i<4;i++){
				data1ui32[i] = (data1[i] > data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 7: // CMPORDPS
			for(i=0;i<4;i++){
				data1ui32[i] = (!isnan(data1[i]) && !isnan(data2[i]) ? 0xffffffff : 0x00000000);
			}
			break;
	}
}
void SSE_CMPSS(void)
{
	UINT32 idx;
	float data2buf[4];
	float *data1, *data2;
	UINT32 *data1ui32;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);

	data1ui32 = (UINT32*)data1;

	GET_PCBYTE((idx));
	switch(idx){
		case 0: // CMPEQSS
			data1ui32[0] = (data1[0] == data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 1: // CMPLTSS
			data1ui32[0] = (data1[0] < data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 2: // CMPLESS
			data1ui32[0] = (data1[0] <= data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 3: // CMPUNORDSS
			data1ui32[0] = (isnan(data1[0]) || isnan(data2[0]) ? 0xffffffff : 0x00000000);
			break;
		case 4: // CMPNEQSS
			data1ui32[0] = (data1[0] != data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 5: // CMPNLTSS
			data1ui32[0] = (data1[0] >= data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 6: // CMPNLESS
			data1ui32[0] = (data1[0] > data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 7: // CMPORDSS
			data1ui32[0] = (!isnan(data1[0]) && !isnan(data2[0]) ? 0xffffffff : 0x00000000);
			break;
	}
}
void SSE_COMISS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);

	if(isnan(data1[0]) || isnan(data2[0])){
		CPU_FLAGL = (CPU_FLAGL & ~Z_FLAG) | Z_FLAG;
		CPU_FLAGL = (CPU_FLAGL & ~P_FLAG) | P_FLAG;
		CPU_FLAGL = (CPU_FLAGL & ~C_FLAG) | C_FLAG;
	}else if(data1[0] > data2[0]){
		CPU_FLAGL = (CPU_FLAGL & ~Z_FLAG) | 0;
		CPU_FLAGL = (CPU_FLAGL & ~P_FLAG) | 0;
		CPU_FLAGL = (CPU_FLAGL & ~C_FLAG) | 0;
	}else if(data1[0] < data2[0]){
		CPU_FLAGL = (CPU_FLAGL & ~Z_FLAG) | 0;
		CPU_FLAGL = (CPU_FLAGL & ~P_FLAG) | 0;
		CPU_FLAGL = (CPU_FLAGL & ~C_FLAG) | C_FLAG;
	}else{ // equal
		CPU_FLAGL = (CPU_FLAGL & ~Z_FLAG) | Z_FLAG;
		CPU_FLAGL = (CPU_FLAGL & ~P_FLAG) | 0;
		CPU_FLAGL = (CPU_FLAGL & ~C_FLAG) | 0;
	}
}
void SSE_CVTPI2PS(void)
{
	SINT32 data2buf[2];
	float *data1;
	SINT32 *data2;

	SSE_PART_GETDATA1DATA2_P_MMX2XMM(&data1, &data2, data2buf);

	data1[0] = (float)data2[0];
	data1[1] = (float)data2[1];
}
void SSE_CVTPS2PI(void)
{
	float data2buf[2];
	SINT32 *data1;
	float *data2;
	
	SSE_PART_GETDATA1DATA2_P_XMM2MMX(&data1, &data2, data2buf);

	data1[0] = (SINT32)SSE_ROUND(data2[0]);
	data1[1] = (SINT32)SSE_ROUND(data2[1]);
}
void SSE_CVTSI2SS(void)
{
	SINT32 data2buf[2];
	float *data1;
	SINT32 *data2;

	SSE_PART_GETDATA1DATA2_S_REG2XMM(&data1, &data2, data2buf);

	data1[0] = (float)data2[0];
}
void SSE_CVTSS2SI(void)
{
	float data2buf[2];
	SINT32 *data1;
	float *data2;
	
	SSE_PART_GETDATA1DATA2_S_XMM2REG(&data1, &data2, data2buf);

	data1[0] = (SINT32)SSE_ROUND(data2[0]);
}
void SSE_CVTTPS2PI(void)
{
	float data2buf[2];
	SINT32 *data1;
	float *data2;
	
	SSE_PART_GETDATA1DATA2_P_XMM2MMX(&data1, &data2, data2buf);

	data1[0] = (SINT32)(data2[0]);
	data1[1] = (SINT32)(data2[1]);
}
void SSE_CVTTSS2SI(void)
{
	float data2buf[2];
	SINT32 *data1;
	float *data2;
	
	SSE_PART_GETDATA1DATA2_S_XMM2REG(&data1, &data2, data2buf);

	data1[0] = (SINT32)(data2[0]);
}
void SSE_DIVPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = data1[i] / data2[i];
	}
}
void SSE_DIVSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	data1[0] = data1[0] / data2[0];
}
void SSE_LDMXCSR(UINT32 maddr)
{
	SSE_MXCSR = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
}
void SSE_MAXPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		if(isnan(data1[i]) || isnan(data2[i])){
			data1[i] = data2[i];
		}else{
			data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
		}
	}
}
void SSE_MAXSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	if(isnan(data1[0]) || isnan(data2[0])){
		data1[0] = data2[0];
	}else{
		data1[0] = (data1[0] > data2[0] ? data1[0] : data2[0]);
	}
}
void SSE_MINPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		if(isnan(data1[i]) || isnan(data2[i])){
			data1[i] = data2[i];
		}else{
			data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
		}
	}
}
void SSE_MINSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	if(isnan(data1[0]) || isnan(data2[0])){
		data1[0] = data2[0];
	}else{
		data1[0] = (data1[0] < data2[0] ? data1[0] : data2[0]);
	}
}
void SSE_MOVAPSmem2xmm(void)
{
	UINT32 op;
	UINT idx, sub;
	SSEREG data2buf;
	float *data1, *data2;
	int i;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		data2buf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		data2buf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 4);
		data2buf.d[2] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 8);
		data2buf.d[3] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+12);
		data2 = data2buf.f32;
	}
	for(i=0;i<4;i++){
		data1[i] = data2[i];
	}
}
void SSE_MOVAPSxmm2mem(void)
{
	UINT32 op;
	UINT idx, sub;
	float *data1, *data2;
	int i;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
		for(i=0;i<4;i++){
			data2[i] = data1[i];
		}
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 0, *((UINT32*)(data1+ 0)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 4, *((UINT32*)(data1+ 1)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 8, *((UINT32*)(data1+ 2)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+12, *((UINT32*)(data1+ 3)));
	}
}
void SSE_MOVHLPS(float *data1, float *data2)
{
	data1[0] = data2[2];
	data1[1] = data2[3];
}
void SSE_MOVHPSmem2xmm(void)
{
	UINT32 op;
	UINT idx, sub;
	SSEREG data2buf;
	float *data1, *data2;
	int i;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
		SSE_MOVLHPS(data1, data2);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		data2buf.d[2] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		data2buf.d[3] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 4);
		data2 = data2buf.f32;
		for(i=2;i<4;i++){
			data1[i] = data2[i];
		}
	}
}
void SSE_MOVHPSxmm2mem(void)
{
	UINT32 op;
	UINT idx, sub;
	float *data1;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		EXCEPTION(UD_EXCEPTION, 0);
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 0, *((UINT32*)(data1+ 2)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 4, *((UINT32*)(data1+ 3)));
	}
}
void SSE_MOVLHPS(float *data1, float *data2)
{
	data1[2] = data2[0];
	data1[3] = data2[1];
}
void SSE_MOVLPSmem2xmm(void)
{
	UINT32 op;
	UINT idx, sub;
	SSEREG data2buf;
	float *data1, *data2;
	int i;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
		SSE_MOVHLPS(data1, data2);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		data2buf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		data2buf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 4);
		data2 = data2buf.f32;
		for(i=0;i<2;i++){
			data1[i] = data2[i];
		}
	}
}
void SSE_MOVLPSxmm2mem(void)
{
	UINT32 op;
	UINT idx, sub;
	float *data1;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		EXCEPTION(UD_EXCEPTION, 0);
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 0, *((UINT32*)(data1+ 0)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 4, *((UINT32*)(data1+ 1)));
	}
}
void SSE_MOVMSKPS(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 *data1;
	UINT32 *data2;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = reg32_b53[(op)];
	if ((op) >= 0xc0) {
		data2 = (UINT32*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		EXCEPTION(UD_EXCEPTION, 0);
	}
	*data1 = ((data2[0] >> 31) & 0x1)|
			 ((data2[1] >> 30) & 0x2)|
			 ((data2[2] >> 29) & 0x4)|
			 ((data2[3] >> 28) & 0x8);
}
void SSE_MOVSSmem2xmm(void)
{
	UINT32 op;
	UINT idx, sub;
	SSEREG data2buf;
	float *data1, *data2;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		data2buf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		data2 = data2buf.f32;
	}
	data1[0] = data2[0];
	*(UINT32*)(data1+1) = *(UINT32*)(data1+2) = *(UINT32*)(data1+3) = 0;
}
void SSE_MOVSSxmm2mem(void)
{
	UINT32 op;
	UINT idx, sub;
	float *data1, *data2;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
		data2[0] = data1[0];
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 0, *((UINT32*)(data1+ 0)));
	}
}
void SSE_MOVUPSmem2xmm(void)
{
	SSE_MOVAPSmem2xmm(); // エミュレーションではアライメント制限がないのでMOVAPSと同じ
}
void SSE_MOVUPSxmm2mem(void)
{
	SSE_MOVAPSxmm2mem(); // エミュレーションではアライメント制限がないのでMOVAPSと同じ
}
void SSE_MULPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = data1[i] * data2[i];
	}
}
void SSE_MULSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	data1[0] = data1[0] * data2[0];
}
void SSE_ORPS(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_UINT32(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = ((data1[i]) | (data2[i]));
	}
}
void SSE_RCPPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = 1.0f / data2[i];
	}
}
void SSE_RCPSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	data1[0] = 1.0f / data2[0];
}
void SSE_RSQRTPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = (float)(1.0f / sqrt(data2[i]));
	}
}
void SSE_RSQRTSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	data1[0] = (float)(1.0f / sqrt(data2[0]));
}
void SSE_SHUFPS(void)
{
	UINT32 imm8;
	float data2buf[4];
	float data1buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);

	GET_PCBYTE((imm8));

	for(i=0;i<2;i++){
		data1buf[i] = data1[imm8 & 0x3];
		imm8 = (imm8 >> 2);
	}
	for(i=2;i<4;i++){
		data1buf[i] = data2[imm8 & 0x3];
		imm8 = (imm8 >> 2);
	}
	for(i=0;i<4;i++){
		data1[i] = data1buf[i];
	}
}
void SSE_SQRTPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = (float)sqrt(data2[i]);
	}
}
void SSE_SQRTSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	data1[0] = (float)sqrt(data2[0]);
}
void SSE_STMXCSR(UINT32 maddr)
{
	cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, maddr, SSE_MXCSR);
}
void SSE_SUBPS(void)
{
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = data1[i] - data2[i];
	}
}
void SSE_SUBSS(void)
{
	float data2buf[4];
	float *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_S(&data1, &data2, data2buf);
	data1[0] = data1[0] - data2[0];
}
void SSE_UCOMISS(void)
{
	SSE_COMISS(); // XXX: とりあえず例外は考えないのでCOMISSと同じ
}
void SSE_UNPCKHPS(void)
{
	float data1buf[4];
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1buf[i] = data1[i];
	}
	data1[0] = data1buf[2];
	data1[1] = data2[2];
	data1[2] = data1buf[3];
	data1[3] = data2[3];
}
void SSE_UNPCKLPS(void)
{
	float data1buf[4];
	float data2buf[4];
	float *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1buf[i] = data1[i];
	}
	data1[0] = data1buf[0];
	data1[1] = data2[0];
	data1[2] = data1buf[1];
	data1[3] = data2[1];
}
void SSE_XORPS(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_UINT32(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = ((data1[i]) ^ (data2[i]));
	}
}

void SSE_PAVGB(void)
{
	UINT8 data2buf[8];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_UB(&data1, &data2, data2buf);
	for(i=0;i<8;i++){
		data1[i] = (UINT8)(((UINT16)data1[i] + (UINT16)data2[i] + 1) / 2);
	}
}
void SSE_PAVGW(void)
{
	UINT16 data2buf[4];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_UW(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = (UINT16)(((UINT32)data1[i] + (UINT32)data2[i] + 1) / 2);
	}
}
void SSE_PEXTRW(void)
{
	UINT32 imm8;
	UINT32 op;
	UINT idx, sub;
	UINT32 *data1;
	UINT16 *data2;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT32*)reg32_b53[(op)];
	if ((op) >= 0xc0) {
		data2 = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		EXCEPTION(UD_EXCEPTION, 0);
	}
	GET_PCBYTE((imm8));
	*data1 = (UINT32)data2[imm8 & 0x3];
}
void SSE_PINSRW(void)
{
	UINT32 imm8;
	UINT32 op;
	UINT idx, sub;
	UINT16 *data1;
	UINT16 data2;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT16*)(&(FPU_STAT.reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (UINT16)((*reg32_b20[(op)]) & 0xffff);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		data2 = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, maddr+ 0);
	}
	GET_PCBYTE((imm8));
	data1[imm8 & 0x3] = data2;
}
void SSE_PMAXSW(void)
{
	SINT16 data2buf[4];
	SINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_SW(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
	}
}
void SSE_PMAXUB(void)
{
	UINT8 data2buf[8];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_UB(&data1, &data2, data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
	}
}
void SSE_PMINSW(void)
{
	SINT16 data2buf[4];
	SINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_SW(&data1, &data2, data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
	}
}
void SSE_PMINUB(void)
{
	UINT8 data2buf[8];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_UB(&data1, &data2, data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
	}
}
void SSE_PMOVMSKB(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 *data1;
	UINT8 *data2;

	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT32*)reg32_b53[(op)];
	if ((op) >= 0xc0) {
		data2 = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		EXCEPTION(UD_EXCEPTION, 0);
	}
	*data1 = ((data2[0] >> 7) & 0x1 )|
			 ((data2[1] >> 6) & 0x2 )|
			 ((data2[2] >> 5) & 0x4 )|
			 ((data2[3] >> 4) & 0x8 )|
			 ((data2[4] >> 3) & 0x10)|
			 ((data2[5] >> 2) & 0x20)|
			 ((data2[6] >> 1) & 0x40)|
			 ((data2[7] >> 0) & 0x80);
}
void SSE_PMULHUW(void)
{
	UINT32 op;
	UINT idx, sub;
	SSEREG data2buf;
	UINT16 *data1, *data2;
	int i;
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT16*)(&(FPU_STAT.reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		data2buf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		data2buf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4);
		data2 = data2buf.w;
	}
	for(i=0;i<4;i++){
		data1[i] = (UINT16)((((UINT32)data2[i] * (UINT32)data1[i]) >> 16) & 0xffff);
	}
}
void SSE_PSADBW(void)
{
	SINT16 temp1;
	UINT16 temp;
	UINT8 data2buf[8];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_UB(&data1, &data2, data2buf);
	temp = 0;
	for(i=0;i<8;i++){
		temp1 = (SINT16)data2[i] - (SINT16)data1[i];
		temp += (UINT16)(temp1 < 0 ? -temp1 : temp1);
	}
	*((UINT16*)data2 + 0) = temp;
	*((UINT16*)data2 + 1) = 0;
	*((UINT16*)data2 + 2) = 0;
	*((UINT16*)data2 + 3) = 0;
}
void SSE_PSHUFW(void)
{
	UINT32 imm8;
	UINT16 data2buf[4];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_P_MMX2MMX_UW(&data1, &data2, data2buf);
	GET_PCBYTE((imm8));
	for(i=0;i<4;i++){
		data1[i] = data2[imm8 & 0x3];
		imm8 = imm8 >> 2;
	}
}

void SSE_MASKMOVQ(void)
{
	UINT8 data2buf[8];
	UINT8 *data1, *data2;
	int i;

	SSE_PART_GETDATA1DATA2_P_MMX2MMX_UB(&data1, &data2, data2buf);
	for(i=0;i<8;i++){
		if (!CPU_INST_AS32) {
			if(data2[i] & 0x80){
				cpu_vmemorywrite(CPU_DS_INDEX, CPU_DI, data1[i]);
			}
			CPU_DI += 1;
		} else {
			if(data2[i] & 0x80){
				cpu_vmemorywrite(CPU_DS_INDEX, CPU_EDI, data1[i]);
			}
			CPU_EDI += 1;
		}
	}
	// 戻す
	if (!CPU_INST_AS32) {
		CPU_DI -= 8;
	} else {
		CPU_EDI -= 8;
	}
}
void SSE_MOVNTPS(void)
{
	UINT32 op;
	UINT idx, sub;
	float *data1;

	// 00001111:00101011 mod xmmreg r/m
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (float*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		// XXX: ここはどう扱う?
		EXCEPTION(UD_EXCEPTION, 0);
		//data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
		//for(i=0;i<4;i++){
		//	data2[i] = data1[i];
		//}
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 0, *((UINT32*)(data1+ 0)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 4, *((UINT32*)(data1+ 1)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 8, *((UINT32*)(data1+ 2)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+12, *((UINT32*)(data1+ 3)));
	}
}
void SSE_MOVNTQ(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 *data1;

	// 00001111:00101011 mod xmmreg r/m
	
	SSE_check_NM_EXCEPTION();
	SSE_setTag();
	MMX_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT32*)(&(FPU_STAT.reg[idx]));
	if ((op) >= 0xc0) {
		// XXX: ここはどう扱う?
		EXCEPTION(UD_EXCEPTION, 0);
		//data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
		//for(i=0;i<4;i++){
		//	data2[i] = data1[i];
		//}
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 0, *((UINT32*)(data1+ 0)));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+ 4, *((UINT32*)(data1+ 1)));
	}
}
void SSE_PREFETCHTx(void)
{
	UINT32 op;
	UINT idx, sub;
	
	//SSE_check_NM_EXCEPTION();
	//SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		// XXX: ここはどう扱う?
		//EXCEPTION(UD_EXCEPTION, 0);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		switch(idx){
		case 3:
			// PREFETCH2
			break;
		case 2:
			// PREFETCH1
			break;
		case 1:
			// PREFETCH0
			break;
		case 0:
			// PREFETCHNTA
			break;
		default:
			//EXCEPTION(UD_EXCEPTION, 0);
			break;
		}
		// XXX: 何もしない
	}
}
void SSE_NOPPREFETCH(void)
{
	UINT32 op;
	UINT idx, sub;
	
	//SSE_check_NM_EXCEPTION();
	//SSE_setTag();
	CPU_SSEWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		// XXX: ここはどう扱う?
		//EXCEPTION(UD_EXCEPTION, 0);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		// XXX: 何もしない
	}
}
void SSE_SFENCE(void)
{
	// Nothing to do
}
void SSE_LFENCE(void)
{
	// Nothing to do
}
void SSE_MFENCE(void)
{
	// Nothing to do
}
void SSE_CLFLUSH(UINT32 op)
{
	UINT idx, sub;
	
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		// XXX: ここはどう扱う?
		//EXCEPTION(UD_EXCEPTION, 0);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		// XXX: 何もしない
	}
}


#else

/*
 * SSE interface
 */

void SSE_ADDPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_ADDSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_ANDNPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_ANDPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CMPPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CMPSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_COMISS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CVTPI2PS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CVTPS2PI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CVTSI2SS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CVTSS2SI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CVTTPS2PI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CVTTSS2SI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_DIVPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_DIVSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_LDMXCSR(UINT32 maddr)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MAXPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MAXSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MINPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MINSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVAPSmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVAPSxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVHLPS(float *data1, float *data2)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVHPSmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVHPSxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVLHPS(float *data1, float *data2)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVLPSmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVLPSxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVMSKPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVSSmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVSSxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVUPSmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVUPSxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MULPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MULSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_ORPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_RCPPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_RCPSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_RSQRTPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_RSQRTSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_SHUFPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_SQRTPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_SQRTSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_STMXCSR(UINT32 maddr)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_SUBPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_SUBSS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_UCOMISS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_UNPCKHPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_UNPCKLPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_XORPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE_PAVGB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PAVGW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PEXTRW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PINSRW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PMAXSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PMAXUB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PMINSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PMINUB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PMOVMSKB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PMULHUW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PSADBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PSHUFW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE_MASKMOVQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVNTPS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MOVNTQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_PREFETCHTx(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_NOPPREFETCH(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_SFENCE(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_LFENCE(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_MFENCE(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE_CLFLUSH(UINT32 op)
{
	EXCEPTION(UD_EXCEPTION, 0);
}


#endif
