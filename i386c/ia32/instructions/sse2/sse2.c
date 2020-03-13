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

#include "compiler.h"

#if 0
#undef	TRACEOUT
#define USE_TRACEOUT_VS
//#define MEM_BDA_TRACEOUT
//#define MEM_D8_TRACEOUT
#ifdef USE_TRACEOUT_VS
static void trace_fmt_ex(const char *fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
#else
#define	TRACEOUT(s)	(void)(s)
#endif
#endif	/* 1 */

#include <math.h>
#include <float.h>

#if defined(_WIN32) && !defined(__LIBRETRO__)
#define isnan(x) (_isnan(x))
#endif

#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "ia32/instructions/sse/sse.h"
#include "ia32/instructions/sse2/sse2.h"

#if defined(USE_SSE2) && defined(USE_SSE) && defined(USE_FPU)

#define CPU_SSE2WORKCLOCK	CPU_WORKCLOCK(8)

static INLINE void
SSE2_check_NM_EXCEPTION(){
	// SSE2なしならUD(無効オペコード例外)を発生させる
	if(!(i386cpuid.cpu_feature & CPU_FEATURE_SSE2)){
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
SSE2_setTag(void)
{
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

float SSE2_ROUND_FLOAT(float val){
	return SSE_ROUND(val);
}
double SSE2_ROUND_DOUBLE(double val){
	double floorval;
	int rndbit = (SSE_MXCSR >> 13) & 0x3;
	switch(rndbit){
	case 0:	
		floorval = floor(val);
		if (val - floorval > 0.5){
			return (floorval + 1); // 切り上げ
		}else if (val - floorval < 0.5){
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
		return floor(val);
	case 2:
		return ceil(val);
	case 3:
		if(val < 0){
			return ceil(val); // ゼロ方向への切り捨て
		}else{
			return floor(val); // ゼロ方向への切り捨て
		}
		break;
	default:
		return val;
	}
}

/*
 * SSE2 interface
 */

// コードが長くなるのでやや強引に共通化
// xmm/m128 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_PD(double **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
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
static INLINE void SSE_PART_GETDATA1DATA2_SD(double **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (double*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT64*)(data2buf+ 0)) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}
static INLINE void SSE_PART_GETDATA1DATA2_S_UINT32(UINT32 **data1, UINT32 **data2, UINT32 *data2buf){
	SSE_PART_GETDATA1DATA2_SD((double**)data1, (double**)data2, (double*)data2buf);
}
static INLINE void SSE_PART_GETDATA1DATA2_SD_UINT64(UINT64 **data1, UINT64 **data2, UINT64 *data2buf){
	SSE_PART_GETDATA1DATA2_SD((double**)data1, (double**)data2, (double*)data2buf);
}
// xmm/m64 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_PDm64(double **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
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
static INLINE void SSE_PART_GETDATA1DATA2_SDm64(double **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (float*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}

// mm/m128 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_PD_MMX2XMM(double **data1, SINT32 **data2, SINT32 *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
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
static INLINE void SSE_PART_GETDATA1DATA2_SD_MMX2XMM(double **data1, SINT32 **data2, SINT32 *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (SINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}
// xmm/m128 -> mm
static INLINE void SSE_PART_GETDATA1DATA2_PD_XMM2MMX(SINT32 **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (SINT32*)(&(FPU_STAT.reg[idx]));
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
static INLINE void SSE_PART_GETDATA1DATA2_SD_XMM2MMX(SINT32 **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (SINT32*)(&(FPU_STAT.reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (double*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT64*)(data2buf+ 0)) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}

// reg/m32 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_SD_REG2XMM(double **data1, SINT32 **data2, SINT32 *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		*data2 = (SINT32*)reg32_b20[(op)];
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+ 0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}
static INLINE void SSE_PART_GETDATA1DATA2_SD_XMM2REG(SINT32 **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	*data1 = (SINT32*)reg32_b53[(op)];
	if ((op) >= 0xc0) {
		*data2 = (double*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT64*)(data2buf+ 0)) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, maddr+ 0);
		*data2 = data2buf;
	}
}

void SSE2_ADDPD(void)
{
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1[i] = data1[i] + data2[i];
	}
	TRACEOUT(("SSE2_ADDPD"));
}
void SSE2_ADDSD(void)
{
	double data2buf[2];
	double *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);
	data1[0] = data1[0] + data2[0];
	TRACEOUT(("SSE2_ADDSD"));
}
void SSE2_ANDNPD(void)
{
	SSE_ANDNPS();
	TRACEOUT(("SSE2_ANDNPD"));
}
void SSE2_ANDPD(void)
{
	SSE_ANDPS();
	TRACEOUT(("SSE2_ANDPD"));
}
void SSE2_CMPPD(void)
{
	UINT32 idx;
	double data2buf[2];
	double *data1, *data2;
	UINT32 *data1ui32;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);

	data1ui32 = (UINT32*)data1;

	GET_PCBYTE((idx));
	switch(idx){
		case 0: // CMPEQPS
			for(i=0;i<2;i++){
				data1ui32[i*2+0] = data1ui32[i*2+1] = (data1[i] == data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 1: // CMPLTPS
			for(i=0;i<2;i++){
				data1ui32[i*2+0] = data1ui32[i*2+1] = (data1[i] < data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 2: // CMPLEPS
			for(i=0;i<2;i++){
				data1ui32[i*2+0] = data1ui32[i*2+1] = (data1[i] <= data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 3: // CMPUNORDPS
			for(i=0;i<2;i++){
				data1ui32[i*2+0] = data1ui32[i*2+1] = (isnan(data1[i]) || isnan(data2[i]) ? 0xffffffff : 0x00000000);
			}
			break;
		case 4: // CMPNEQPS
			for(i=0;i<2;i++){
				data1ui32[i*2+0] = data1ui32[i*2+1] = (data1[i] != data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 5: // CMPNLTPS
			for(i=0;i<2;i++){
				data1ui32[i*2+0] = data1ui32[i*2+1] = (data1[i] >= data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 6: // CMPNLEPS
			for(i=0;i<2;i++){
				data1ui32[i*2+0] = data1ui32[i*2+1] = (data1[i] > data2[i] ? 0xffffffff : 0x00000000);
			}
			break;
		case 7: // CMPORDPS
			for(i=0;i<2;i++){
				data1ui32[i*2+0] = data1ui32[i*2+1] = (!isnan(data1[i]) && !isnan(data2[i]) ? 0xffffffff : 0x00000000);
			}
			break;
	}
	TRACEOUT(("SSE2_CMPPD"));
}
void SSE2_CMPSD(void)
{
	UINT32 idx;
	double data2buf[2];
	double *data1, *data2;
	UINT32 *data1ui32;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);

	data1ui32 = (UINT32*)data1;

	GET_PCBYTE((idx));
	switch(idx){
		case 0: // CMPEQSS
			data1ui32[0] = data1ui32[1] = (data1[0] == data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 1: // CMPLTSS
			data1ui32[0] = data1ui32[1] = (data1[0] < data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 2: // CMPLESS
			data1ui32[0] = data1ui32[1] = (data1[0] <= data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 3: // CMPUNORDSS
			data1ui32[0] = data1ui32[1] = (isnan(data1[0]) || isnan(data2[0]) ? 0xffffffff : 0x00000000);
			break;
		case 4: // CMPNEQSS
			data1ui32[0] = data1ui32[1] = (data1[0] != data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 5: // CMPNLTSS
			data1ui32[0] = data1ui32[1] = (data1[0] >= data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 6: // CMPNLESS
			data1ui32[0] = data1ui32[1] = (data1[0] > data2[0] ? 0xffffffff : 0x00000000);
			break;
		case 7: // CMPORDSS
			data1ui32[0] = data1ui32[1] = (!isnan(data1[0]) && !isnan(data2[0]) ? 0xffffffff : 0x00000000);
			break;
	}
	TRACEOUT(("SSE2_CMPSD"));
}
void SSE2_COMISD(void)
{
	double data2buf[2];
	double *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);

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
	TRACEOUT(("SSE2_COMISD"));
}
void SSE2_CVTPI2PD(void)
{
	SINT32 data2buf[2];
	double *data1;
	SINT32 *data2;

	SSE_PART_GETDATA1DATA2_PD_MMX2XMM(&data1, &data2, data2buf);

	data1[0] = (double)data2[0];
	data1[1] = (double)data2[1];
	TRACEOUT(("SSE2_CVTPI2PD"));
}
void SSE2_CVTPD2PI(void)
{
	double data2buf[2];
	SINT32 *data1;
	double *data2;
	
	SSE_PART_GETDATA1DATA2_PD_XMM2MMX(&data1, &data2, data2buf);

	data1[0] = (SINT32)SSE2_ROUND_DOUBLE(data2[0]);
	data1[1] = (SINT32)SSE2_ROUND_DOUBLE(data2[1]);
	TRACEOUT(("SSE2_CVTPD2PI"));
}
void SSE2_CVTSI2SD(void)
{
	SINT32 data2buf[2];
	double *data1;
	SINT32 *data2;

	SSE_PART_GETDATA1DATA2_SD_REG2XMM(&data1, &data2, data2buf);

	data1[0] = (double)data2[0];
	TRACEOUT(("SSE2_CVTSI2SD"));
}
void SSE2_CVTSD2SI(void)
{
	double data2buf[2];
	SINT32 *data1;
	double *data2;
	
	SSE_PART_GETDATA1DATA2_SD_XMM2REG(&data1, &data2, data2buf);

	data1[0] = (SINT32)SSE2_ROUND_DOUBLE(data2[0]);
	TRACEOUT(("SSE2_CVTSD2SI"));
}
void SSE2_CVTTPD2PI(void)
{
	double data2buf[2];
	SINT32 *data1;
	double *data2;
	
	SSE_PART_GETDATA1DATA2_PD_XMM2MMX(&data1, &data2, data2buf);

	data1[0] = (SINT32)(data2[0]);
	data1[1] = (SINT32)(data2[1]);
	TRACEOUT(("SSE2_CVTTPD2PI"));
}
void SSE2_CVTTSD2SI(void)
{
	double data2buf[2];
	SINT32 *data1;
	double *data2;
	
	SSE_PART_GETDATA1DATA2_SD_XMM2REG(&data1, &data2, data2buf);
	
	data1[0] = (SINT32)(data2[0]);
	TRACEOUT(("SSE2_CVTTSD2SI"));
}
void SSE2_CVTPD2PS(void)
{
	double data2buf[2];
	float *data1;
	double *data2;
	
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), &data2, data2buf);
	
	data1[0] = (float)(data2[0]);
	data1[1] = (float)(data2[1]);
	data1[2] = data1[3] = 0;
	TRACEOUT(("SSE2_CVTPD2PS"));
}
void SSE2_CVTPS2PD(void)
{
	float data2buf[2];
	double *data1;
	float *data2;
	
	SSE_PART_GETDATA1DATA2_PDm64(&data1, &data2, data2buf);
	
	data1[0] = (double)(data2[0]);
	data1[1] = (double)(data2[1]);
	TRACEOUT(("SSE2_CVTPS2PD"));
}
void SSE2_CVTSD2SS(void)
{
	double data2buf[2];
	float *data1;
	double *data2;
	
	SSE_PART_GETDATA1DATA2_SD((double**)(&data1), &data2, data2buf);
	
	data1[0] = (float)(data2[0]);
	TRACEOUT(("SSE2_CVTSD2SS"));
}
void SSE2_CVTSS2SD(void)
{
	float data2buf[2];
	double *data1;
	float *data2;
	
	SSE_PART_GETDATA1DATA2_SDm64(&data1, &data2, data2buf);
	
	data1[0] = (double)(data2[0]);
	TRACEOUT(("SSE2_CVTSS2SD"));
}
void SSE2_CVTPD2DQ(void)
{
	double data2buf[2];
	SINT32 *data1;
	double *data2;
	
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), &data2, data2buf);
	
	data1[0] = (SINT32)SSE2_ROUND_DOUBLE(data2[0]);
	data1[1] = (SINT32)SSE2_ROUND_DOUBLE(data2[1]);
	data1[2] = data1[3] = 0;
	TRACEOUT(("SSE2_CVTPD2DQ"));
}
void SSE2_CVTTPD2DQ(void)
{
	double data2buf[2];
	SINT32 *data1;
	double *data2;
	
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), &data2, data2buf);
	
	data1[0] = (SINT32)(data2[0]);
	data1[1] = (SINT32)(data2[1]);
	data1[2] = data1[3] = 0;
	TRACEOUT(("SSE2_CVTTPD2DQ"));
}
void SSE2_CVTDQ2PD(void)
{
	SINT32 data2buf[2];
	double *data1;
	SINT32 *data2;
	
	SSE_PART_GETDATA1DATA2_PDm64(&data1, (float**)(&data2), (float*)data2buf);
	
	data1[0] = (double)(data2[0]);
	data1[1] = (double)(data2[1]);
	TRACEOUT(("SSE2_CVTDQ2PD"));
}
void SSE2_CVTPS2DQ(void)
{
	float data2buf[4];
	SINT32 *data1;
	float *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	
	for(i=0;i<4;i++){
		data1[i] = (SINT32)SSE2_ROUND_FLOAT(data2[i]);
	}
	TRACEOUT(("SSE2_CVTPS2DQ"));
}
void SSE2_CVTTPS2DQ(void)
{
	float data2buf[4];
	SINT32 *data1;
	float *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	
	for(i=0;i<4;i++){
		data1[i] = (SINT32)(data2[i]);
	}
	TRACEOUT(("SSE2_CVTTPS2DQ"));
}
void SSE2_CVTDQ2PS(void)
{
	SINT32 data2buf[4];
	float *data1;
	SINT32 *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	
	for(i=0;i<4;i++){
		data1[i] = (float)(data2[i]);
	}
	TRACEOUT(("SSE2_CVTDQ2PS"));
}
void SSE2_DIVPD(void)
{
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1[i] = data1[i] / data2[i];
	}
	TRACEOUT(("SSE2_DIVPD"));
}
void SSE2_DIVSD(void)
{
	double data2buf[2];
	double *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);
	data1[0] = data1[0] / data2[0];
	TRACEOUT(("SSE2_DIVSD"));
}
void SSE2_MAXPD(void)
{
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		if(isnan(data1[i]) || isnan(data2[i])){
			data1[i] = data2[i];
		}else{
			data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
		}
	}
	TRACEOUT(("SSE2_MAXPD"));
}
void SSE2_MAXSD(void)
{
	double data2buf[2];
	double *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);
	if(isnan(data1[0]) || isnan(data2[0])){
		data1[0] = data2[0];
	}else{
		data1[0] = (data1[0] > data2[0] ? data1[0] : data2[0]);
	}
	TRACEOUT(("SSE2_MAXSD"));
}
void SSE2_MINPD(void)
{
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		if(isnan(data1[i]) || isnan(data2[i])){
			data1[i] = data2[i];
		}else{
			data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
		}
	}
	TRACEOUT(("SSE2_MINPD"));
}
void SSE2_MINSD(void)
{
	double data2buf[2];
	double *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);
	if(isnan(data1[0]) || isnan(data2[0])){
		data1[0] = data2[0];
	}else{
		data1[0] = (data1[0] < data2[0] ? data1[0] : data2[0]);
	}
	TRACEOUT(("SSE2_MINSD"));
}
void SSE2_MOVAPDmem2xmm(void)
{
	SSE_MOVAPSmem2xmm();
	TRACEOUT(("SSE2_MOVAPDmem2xmm"));
}
void SSE2_MOVAPDxmm2mem(void)
{
	SSE_MOVAPSxmm2mem();
	TRACEOUT(("SSE2_MOVAPDxmm2mem"));
}
void SSE2_MOVHPDmem2xmm(void)
{
	SSE_MOVHPSmem2xmm();
	TRACEOUT(("SSE2_MOVHPDmem2xmm"));
}
void SSE2_MOVHPDxmm2mem(void)
{
	SSE_MOVHPSxmm2mem();
	TRACEOUT(("SSE2_MOVHPDxmm2mem"));
}
void SSE2_MOVLPDmem2xmm(void)
{
	SSE_MOVLPSmem2xmm();
	TRACEOUT(("SSE2_MOVLPDmem2xmm"));
}
void SSE2_MOVLPDxmm2mem(void)
{
	SSE_MOVLPSxmm2mem();
	TRACEOUT(("SSE2_MOVLPDxmm2mem"));
}
void SSE2_MOVMSKPD(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 *data1;
	UINT32 *data2;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = reg32_b53[(op)];
	if ((op) >= 0xc0) {
		data2 = (UINT32*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		EXCEPTION(UD_EXCEPTION, 0);
	}
	*data1 = ((data2[1] >> 31) & 0x1)|
			 ((data2[3] >> 30) & 0x2);
	TRACEOUT(("SSE2_MOVMSKPD"));
}
void SSE2_MOVSDmem2xmm(void)
{
	UINT32 op;
	UINT idx, sub;
	double data2buf[2];
	double *data1, *data2;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (double*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT64*)(data2buf+ 0)) = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, maddr+ 0);
		data2 = data2buf;
		*(UINT64*)(data1+1) = 0;
	}
	data1[0] = data2[0];
	TRACEOUT(("SSE2_MOVSDmem2xmm"));
}
void SSE2_MOVSDxmm2mem(void)
{
	UINT32 op;
	UINT idx, sub;
	double *data1, *data2;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (double*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (double*)(&(FPU_STAT.xmm_reg[sub]));
		data2[0] = data1[0];
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_q(CPU_INST_SEGREG_INDEX, madr+ 0, *((UINT64*)(data1+ 0)));
	}
	TRACEOUT(("SSE2_MOVSDxmm2mem"));
}
void SSE2_MOVUPDmem2xmm(void)
{
	SSE2_MOVAPDmem2xmm(); // エミュレーションではアライメント制限がないのでMOVAPDと同じ
	TRACEOUT(("SSE2_MOVUPDmem2xmm"));
}
void SSE2_MOVUPDxmm2mem(void)
{
	SSE2_MOVAPDxmm2mem(); // エミュレーションではアライメント制限がないのでMOVAPDと同じ
	TRACEOUT(("SSE2_MOVUPDxmm2mem"));
}
void SSE2_MULPD(void)
{
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1[i] = data1[i] * data2[i];
	}
	TRACEOUT(("SSE2_MULPD"));
}
void SSE2_MULSD(void)
{
	double data2buf[2];
	double *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);
	data1[0] = data1[0] * data2[0];
	TRACEOUT(("SSE2_MULSD"));
}
void SSE2_ORPD(void)
{
	SSE_ORPS();
	TRACEOUT(("SSE2_ORPD"));
}
void SSE2_SHUFPD(void)
{
	UINT32 imm8;
	double data2buf[2];
	double data1buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);

	GET_PCBYTE((imm8));

	for(i=0;i<2;i++){
		data1buf[i] = data1[i];
	}
	data1[0] = data1buf[imm8 & 0x1];
	imm8 = (imm8 >> 1);
	data1[1] = data2[imm8 & 0x1];
	TRACEOUT(("SSE2_SHUFPD"));
}
void SSE2_SQRTPD(void)
{
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1[i] = sqrt(data2[i]);
	}
	TRACEOUT(("SSE2_SQRTPD"));
}
void SSE2_SQRTSD(void)
{
	double data2buf[2];
	double *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);
	data1[0] = sqrt(data2[0]);
	TRACEOUT(("SSE2_SQRTSD"));
}
//void SSE2_STMXCSR(UINT32 maddr)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_SUBPD(void)
{
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1[i] = data1[i] - data2[i];
	}
	TRACEOUT(("SSE2_SUBPD"));
}
void SSE2_SUBSD(void)
{
	double data2buf[2];
	double *data1, *data2;
	
	SSE_PART_GETDATA1DATA2_SD(&data1, &data2, data2buf);
	data1[0] = data1[0] - data2[0];
	TRACEOUT(("SSE2_SUBSD"));
}
void SSE2_UCOMISD(void)
{
	SSE_COMISS(); // XXX: とりあえず例外は考えないのでCOMISSと同じ
	TRACEOUT(("SSE2_UCOMISD"));
}
void SSE2_UNPCKHPD(void)
{
	double data1buf[2];
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1buf[i] = data1[i];
	}
	data1[0] = data1buf[1];
	data1[1] = data2[1];
	TRACEOUT(("SSE2_UNPCKHPD"));
}
void SSE2_UNPCKLPD(void)
{
	double data1buf[2];
	double data2buf[2];
	double *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1buf[i] = data1[i];
	}
	data1[0] = data1buf[0];
	data1[1] = data2[0];
	TRACEOUT(("SSE2_UNPCKLPD"));
}
void SSE2_XORPD(void)
{
	SSE_XORPS();
	TRACEOUT(("SSE2_XORPD"));
}

void SSE2_MOVDrm2xmm(void)
{
	UINT32 op, src;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		(src) = *(reg32_b20[(op)]);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		(src) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
	}
	FPU_STAT.xmm_reg[idx].ul32[0] = src;
	FPU_STAT.xmm_reg[idx].ul32[1] = FPU_STAT.xmm_reg[idx].ul32[2] = FPU_STAT.xmm_reg[idx].ul32[3] = 0;
	TRACEOUT(("SSE2_MOVDrm2xmm"));
}
void SSE2_MOVDxmm2rm(void)
{
	UINT32 op, src, madr;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	src = FPU_STAT.xmm_reg[idx].ul32[0];
	if (op >= 0xc0) {
		*(reg32_b20[op]) = src;
	} else {
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, src);
	}
	TRACEOUT(("SSE2_MOVDxmm2rm"));
}
void SSE2_MOVDQAmem2xmm(void)
{
	SSE_MOVAPSmem2xmm();
	TRACEOUT(("SSE2_MOVDQAmem2xmm"));
}
void SSE2_MOVDQAxmm2mem(void)
{
	SSE_MOVAPSxmm2mem();
	TRACEOUT(("SSE2_MOVDQAxmm2mem"));
}
void SSE2_MOVDQUmem2xmm(void)
{
	SSE2_MOVDQAmem2xmm(); // エミュレーションではアライメント制限がないのでMOVDQAと同じ
	TRACEOUT(("SSE2_MOVDQUmem2xmm"));
}
void SSE2_MOVDQUxmm2mem(void)
{
	SSE2_MOVDQAxmm2mem(); // エミュレーションではアライメント制限がないのでMOVDQAと同じ
	TRACEOUT(("SSE2_MOVDQUxmm2mem"));
}
void SSE2_MOVQ2DQ(void)
{
	UINT32 op;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.xmm_reg[idx].ul64[0] = FPU_STAT.reg[sub].ll; // idxとsubが逆かも。要検証
		FPU_STAT.xmm_reg[idx].ul64[1] = 0;
	} else {
		EXCEPTION(UD_EXCEPTION, 0);
	}
	TRACEOUT(("SSE2_MOVQ2DQ"));
}
void SSE2_MOVDQ2Q(void)
{
	UINT32 op;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.reg[idx].ll = FPU_STAT.xmm_reg[sub].ul64[0]; // idxとsubが逆かも。要検証
	} else {
		EXCEPTION(UD_EXCEPTION, 0);
	}
	TRACEOUT(("SSE2_MOVDQ2Q"));
}
void SSE2_MOVQmem2xmm(void)
{
	UINT32 op;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.xmm_reg[idx].ul64[0] = FPU_STAT.xmm_reg[sub].ul64[0];
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		FPU_STAT.xmm_reg[idx].ul64[0] = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, madr);
		FPU_STAT.xmm_reg[idx].ul64[1] = 0;
	}
	TRACEOUT(("SSE2_MOVQmem2xmm"));
}
void SSE2_MOVQxmm2mem(void)
{
	UINT32 op;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.xmm_reg[sub].ul64[0] = FPU_STAT.xmm_reg[idx].ul64[0];
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_q(CPU_INST_SEGREG_INDEX, madr, FPU_STAT.xmm_reg[idx].ul64[0]);
	}
	TRACEOUT(("SSE2_MOVQxmm2mem"));
}
void SSE2_PACKSSDW(void)
{
	UINT32 op;
	UINT idx, sub;
	INT32 srcreg2buf[4];
	INT32 *srcreg1;
	INT32 *srcreg2;
	INT16 *dstreg;
	INT16 dstregbuf[8];
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg1 = (INT32*)(&(FPU_STAT.xmm_reg[idx]));
		srcreg2 = (INT32*)(&(FPU_STAT.xmm_reg[sub]));
		dstreg = (INT16*)(&(FPU_STAT.xmm_reg[idx]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(srcreg2buf+0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		*((UINT32*)(srcreg2buf+1)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4);
		srcreg1 = (INT32*)(&(FPU_STAT.xmm_reg[idx]));
		srcreg2 = (INT32*)(&srcreg2buf);
		dstreg = (INT16*)(&(FPU_STAT.xmm_reg[idx]));
	}
	for(i=0;i<4;i++){
		if(srcreg1[i] > 32767){
			dstregbuf[i] = 32767;
		}else if(srcreg1[i] < -32768){
			dstregbuf[i] = -32768;
		}else{
			dstregbuf[i] = (INT16)(srcreg1[i]);
		}
	}
	for(i=0;i<4;i++){
		if(srcreg2[i] > 32767){
			dstregbuf[i+4] = 32767;
		}else if(srcreg2[i] < -32768){
			dstregbuf[i+4] = -32768;
		}else{
			dstregbuf[i+4] = (INT16)(srcreg2[i]);
		}
	}
	for(i=0;i<8;i++){
		dstreg[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PACKSSDW"));
}
void SSE2_PACKSSWB(void)
{
	UINT32 op;
	UINT idx, sub;
	INT16 srcreg2buf[8];
	INT16 *srcreg1;
	INT16 *srcreg2;
	INT8 *dstreg;
	INT8 dstregbuf[16];
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg1 = (INT16*)(&(FPU_STAT.xmm_reg[idx]));
		srcreg2 = (INT16*)(&(FPU_STAT.xmm_reg[sub]));
		dstreg = (INT8*)(&(FPU_STAT.xmm_reg[idx]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(srcreg2buf+0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		*((UINT32*)(srcreg2buf+2)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4);
		srcreg1 = (INT16*)(&(FPU_STAT.xmm_reg[idx]));
		srcreg2 = (INT16*)(&srcreg2buf);
		dstreg = (INT8*)(&(FPU_STAT.xmm_reg[idx]));
	}
	for(i=0;i<8;i++){
		if(srcreg1[i] > 127){
			dstregbuf[i] = 127;
		}else if(srcreg1[i] < -128){
			dstregbuf[i] = -128;
		}else{
			dstregbuf[i] = (INT8)(srcreg1[i]);
		}
	}
	for(i=0;i<8;i++){
		if(srcreg2[i] > 127){
			dstregbuf[i+8] = 127;
		}else if(srcreg2[i] < -128){
			dstregbuf[i+8] = -128;
		}else{
			dstregbuf[i+8] = (INT8)(srcreg2[i]);
		}
	}
	for(i=0;i<16;i++){
		dstreg[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PACKSSWB"));
}
void SSE2_PACKUSWB(void)
{
	UINT32 op;
	UINT idx, sub;
	INT16 srcreg2buf[8];
	INT16 *srcreg1;
	INT16 *srcreg2;
	UINT8 *dstreg;
	UINT8 dstregbuf[16];
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg1 = (INT16*)(&(FPU_STAT.xmm_reg[idx]));
		srcreg2 = (INT16*)(&(FPU_STAT.xmm_reg[sub]));
		dstreg = (UINT8*)(&(FPU_STAT.xmm_reg[idx]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(srcreg2buf+0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		*((UINT32*)(srcreg2buf+2)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4);
		srcreg1 = (INT16*)(&(FPU_STAT.xmm_reg[idx]));
		srcreg2 = (INT16*)(&srcreg2buf);
		dstreg = (UINT8*)(&(FPU_STAT.xmm_reg[idx]));
	}
	for(i=0;i<8;i++){
		if(srcreg1[i] > 255){
			dstregbuf[i] = 255;
		}else if(srcreg1[i] < 0){
			dstregbuf[i] = 0;
		}else{
			dstregbuf[i] = (UINT8)(srcreg1[i]);
		}
	}
	for(i=0;i<8;i++){
		if(srcreg2[i] > 255){
			dstregbuf[i+8] = 255;
		}else if(srcreg2[i] < 0){
			dstregbuf[i+8] = 0;
		}else{
			dstregbuf[i+8] = (UINT8)(srcreg2[i]);
		}
	}
	for(i=0;i<16;i++){
		dstreg[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PACKUSWB"));
}
void SSE2_PADDQmm(void)
{
	UINT32 op;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.reg[idx].ll += FPU_STAT.reg[sub].ll;
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		FPU_STAT.reg[idx].ll += (SINT64)cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, madr);
	}
	TRACEOUT(("SSE2_PADDQmm"));
}
void SSE2_PADDQxmm(void)
{
	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1[i] = data1[i] + data2[i];
	}
	TRACEOUT(("SSE2_PADDQxmm"));
}
void SSE2_PADDB(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = data1[i] + data2[i];
	}
	TRACEOUT(("SSE2_PADDB"));
}
void SSE2_PADDW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = data1[i] + data2[i];
	}
	TRACEOUT(("SSE2_PADDW"));
}
void SSE2_PADDD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = data1[i] + data2[i];
	}
	TRACEOUT(("SSE2_PADDD"));
}
//void SSE2_PADDQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PADDSB(void)
{
	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		SINT16 cbuf = (SINT16)data1[i] + (SINT16)data2[i];
		if(cbuf > 127){
			data1[i] = 127;
		}else if(cbuf < -128){
			data1[i] = -128;
		}else{
			data1[i] = (SINT8)cbuf;
		}
	}
	TRACEOUT(("SSE2_PADDSB"));
}
void SSE2_PADDSW(void)
{
	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		SINT32 cbuf = (SINT32)data1[i] + (SINT32)data2[i];
		if(cbuf > 32767){
			data1[i] = 32767;
		}else if(cbuf < -32768){
			data1[i] = -32768;
		}else{
			data1[i] = (SINT16)cbuf;
		}
	}
	TRACEOUT(("SSE2_PADDSW"));
}
//void SSE2_PADDSD(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PADDSQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PADDUSB(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		UINT16 cbuf = (UINT16)data1[i] + (UINT16)data2[i];
		if(cbuf > 255){
			data1[i] = 255;
		}else{
			data1[i] = (UINT8)cbuf;
		}
	}
	TRACEOUT(("SSE2_PADDUSB"));
}
void SSE2_PADDUSW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		UINT32 cbuf = (UINT32)data1[i] + (UINT32)data2[i];
		if(cbuf > 65535){
			data1[i] = 65535;
		}else{
			data1[i] = (UINT16)cbuf;
		}
	}
	TRACEOUT(("SSE2_PADDUSW"));
}
//void SSE2_PADDUSD(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PADDUSQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PAND(void)
{
	SSE_ANDPS();
	TRACEOUT(("SSE2_PAND"));
}
void SSE2_PANDN(void)
{
	SSE_ANDNPS();
	TRACEOUT(("SSE2_PANDN"));
}
void SSE2_PAVGB(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = (UINT8)(((UINT16)data1[i] + (UINT16)data2[i] + 1) / 2);
	}
	TRACEOUT(("SSE2_PAVGB"));
}
void SSE2_PAVGW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = (UINT16)(((UINT32)data1[i] + (UINT32)data2[i] + 1) / 2);
	}
	TRACEOUT(("SSE2_PAVGW"));
}
void SSE2_PCMPEQB(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = (data1[i] == data2[i] ? 0xff : 0x00);
	}
	TRACEOUT(("SSE2_PCMPEQB"));
}
void SSE2_PCMPEQW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data1[i] == data2[i] ? 0xffff : 0x00);
	}
	TRACEOUT(("SSE2_PCMPEQW"));
}
void SSE2_PCMPEQD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data1[i] == data2[i] ? 0xffffffff : 0x00);
	}
	TRACEOUT(("SSE2_PCMPEQD"));
}
//void SSE2_PCMPEQQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PCMPGTB(void)
{
	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = (data1[i] > data2[i] ? 0xff : 0x00);
	}
	TRACEOUT(("SSE2_PCMPGTB"));
}
void SSE2_PCMPGTW(void)
{
	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data1[i] > data2[i] ? 0xffff : 0x00);
	}
	TRACEOUT(("SSE2_PCMPGTW"));
}
void SSE2_PCMPGTD(void)
{
	SINT32 data2buf[4];
	SINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data1[i] > data2[i] ? 0xffffffff : 0x00);
	}
	TRACEOUT(("SSE2_PCMPGTD"));
}
//void SSE2_PCMPGTQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PEXTRW(void)
{
	UINT32 imm8;
	UINT32 op;
	UINT idx, sub;
	UINT32 *data1;
	UINT16 *data2;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT32*)reg32_b53[(op)];
	if ((op) >= 0xc0) {
		data2 = (UINT16*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		EXCEPTION(UD_EXCEPTION, 0);
	}
	GET_PCBYTE((imm8));
	*data1 = (UINT32)data2[imm8 & 0x7];
	TRACEOUT(("SSE2_PEXTRW"));
}
void SSE2_PINSRW(void)
{
	UINT32 imm8;
	UINT32 op;
	UINT idx, sub;
	UINT16 *data1;
	UINT16 data2;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT16*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (UINT16)((*reg32_b20[(op)]) & 0xffff);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		data2 = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, maddr+ 0);
	}
	GET_PCBYTE((imm8));
	data1[imm8 & 0x7] = data2;
	TRACEOUT(("SSE2_PINSRW"));
}
void SSE2_PMADD(void)
{
	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	SINT32 data1dbuf[4];
	SINT32 *data1d;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	data1d = (SINT32*)data1;

	data1dbuf[0] = (INT32)data2[0] * (INT32)data1[0] + (INT32)data2[1] * (INT32)data1[1];
	data1dbuf[1] = (INT32)data2[2] * (INT32)data1[2] + (INT32)data2[3] * (INT32)data1[3];
	data1dbuf[2] = (INT32)data2[4] * (INT32)data1[4] + (INT32)data2[5] * (INT32)data1[5];
	data1dbuf[3] = (INT32)data2[6] * (INT32)data1[6] + (INT32)data2[7] * (INT32)data1[7];
	data1d[0] = data1dbuf[0];
	data1d[1] = data1dbuf[1];
	data1d[2] = data1dbuf[2];
	data1d[3] = data1dbuf[3];
	TRACEOUT(("SSE2_PMADD"));
}
void SSE2_PMAXSW(void)
{
	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE2_PMAXSW"));
}
void SSE2_PMAXUB(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = (data1[i] > data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE2_PMAXUB"));
}
void SSE2_PMINSW(void)
{
	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE2_PMINSW"));
}
void SSE2_PMINUB(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = (data1[i] < data2[i] ? data1[i] : data2[i]);
	}
	TRACEOUT(("SSE2_PMINUB"));
}
void SSE2_PMOVMSKB(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 *data1;
	UINT8 *data2;

	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT32*)reg32_b53[(op)];
	if ((op) >= 0xc0) {
		data2 = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		EXCEPTION(UD_EXCEPTION, 0);
	}
	*data1 = ((UINT32)(data2[0] >> 7) & 0x1 )|
			 ((UINT32)(data2[1] >> 6) & 0x2 )|
			 ((UINT32)(data2[2] >> 5) & 0x4 )|
			 ((UINT32)(data2[3] >> 4) & 0x8 )|
			 ((UINT32)(data2[4] >> 3) & 0x10)|
			 ((UINT32)(data2[5] >> 2) & 0x20)|
			 ((UINT32)(data2[6] >> 1) & 0x40)|
			 ((UINT32)(data2[7] >> 0) & 0x80)|
			 (((UINT32)(data2[8] >> 7) & 0x1 ) << 8)|
			 (((UINT32)(data2[9] >> 6) & 0x2 ) << 8)|
			 (((UINT32)(data2[10]>> 5) & 0x4 ) << 8)|
			 (((UINT32)(data2[11]>> 4) & 0x8 ) << 8)|
			 (((UINT32)(data2[12]>> 3) & 0x10) << 8)|
			 (((UINT32)(data2[13]>> 2) & 0x20) << 8)|
			 (((UINT32)(data2[14]>> 1) & 0x40) << 8)|
			 (((UINT32)(data2[15]>> 0) & 0x80) << 8);
	TRACEOUT(("SSE2_PMOVMSKB"));
}
void SSE2_PMULHUW(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (UINT16*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (UINT16*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		*((UINT32*)(data2buf+2)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4);
		*((UINT32*)(data2buf+4)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+8);
		*((UINT32*)(data2buf+6)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+12);
		data2 = data2buf;
	}
	for(i=0;i<8;i++){
		data1[i] = (UINT16)((((UINT32)data2[i] * (UINT32)data1[i]) >> 16) & 0xffff);
	}
	TRACEOUT(("SSE2_PMULHUW"));
}
void SSE2_PMULHW(void)
{
	UINT32 op;
	UINT idx, sub;
	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (SINT16*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (SINT16*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		*((UINT32*)(data2buf+2)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4);
		*((UINT32*)(data2buf+4)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+8);
		*((UINT32*)(data2buf+6)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+12);
		data2 = data2buf;
	}
	for(i=0;i<8;i++){
		data1[i] = (SINT16)((((SINT32)data2[i] * (SINT32)data1[i]) >> 16) & 0xffff);
	}
	TRACEOUT(("SSE2_PMULHW"));
}
void SSE2_PMULLW(void)
{
	UINT32 op;
	UINT idx, sub;
	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = (SINT16*)(&(FPU_STAT.xmm_reg[idx]));
	if ((op) >= 0xc0) {
		data2 = (SINT16*)(&(FPU_STAT.xmm_reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		*((UINT32*)(data2buf+0)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		*((UINT32*)(data2buf+2)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4);
		*((UINT32*)(data2buf+4)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+8);
		*((UINT32*)(data2buf+6)) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+12);
		data2 = data2buf;
	}
	for(i=0;i<8;i++){
		data1[i] = (SINT16)((((SINT32)data2[i] * (SINT32)data1[i])) & 0xffff);
	}
	TRACEOUT(("SSE2_PMULLW"));
}
void SSE2_PMULUDQmm(void)
{
	UINT32 op;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.reg[idx].ll = ((UINT64)FPU_STAT.reg[idx].l.lower * (UINT64)FPU_STAT.reg[sub].l.lower);
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		FPU_STAT.reg[idx].ll = ((UINT64)FPU_STAT.reg[idx].l.lower * (cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, madr) & 0xffffffff));
	}
	TRACEOUT(("SSE2_PMULUDQmm"));
}
void SSE2_PMULUDQxmm(void)
{
	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1[i] = (data1[i] & 0xffffffff) * (data2[i] & 0xffffffff);
	}
	TRACEOUT(("SSE2_PMULUDQxmm"));
}
void SSE2_POR(void)
{
	SSE_ORPS();
	TRACEOUT(("SSE2_POR"));
}
void SSE2_PSADBW(void)
{
	SINT16 temp1;
	UINT16 temp;
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	temp = 0;
	for(i=0;i<8;i++){
		temp1 = (SINT16)data2[i] - (SINT16)data1[i];
		temp += (UINT16)(temp1 < 0 ? -temp1 : temp1);
	}
	*((UINT16*)data2 + 0) = temp;
	*((UINT16*)data2 + 1) = 0;
	*((UINT16*)data2 + 2) = 0;
	*((UINT16*)data2 + 3) = 0;
	
	temp = 0;
	for(i=8;i<16;i++){
		temp1 = (SINT16)data2[i] - (SINT16)data1[i];
		temp += (UINT16)(temp1 < 0 ? -temp1 : temp1);
	}
	*((UINT16*)data2 + 4) = temp;
	*((UINT16*)data2 + 5) = 0;
	*((UINT16*)data2 + 6) = 0;
	*((UINT16*)data2 + 7) = 0;
	TRACEOUT(("SSE2_PSADBW"));
}
void SSE2_PSHUFLW(void)
{
	UINT32 imm8;
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	UINT16 dstbuf[8];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	GET_PCBYTE((imm8));
	for(i=0;i<4;i++){
		dstbuf[i] = data2[imm8 & 0x3];
		imm8 = imm8 >> 2;
	}
	for(i=0;i<4;i++){
		data1[i] = dstbuf[i];
	}
	for(i=4;i<8;i++){
		data1[i] = data2[i];
	}
	TRACEOUT(("SSE2_PSHUFLW"));
}
void SSE2_PSHUFHW(void)
{
	UINT32 imm8;
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	UINT16 dstbuf[8];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	GET_PCBYTE((imm8));
	for(i=0;i<4;i++){
		data1[i] = data2[i];
	}
	for(i=4;i<8;i++){
		dstbuf[i] = data2[4 + (imm8 & 0x3)];
		imm8 = imm8 >> 2;
	}
	for(i=4;i<8;i++){
		data1[i] = dstbuf[i];
	}
	TRACEOUT(("SSE2_PSHUFHW"));
}
void SSE2_PSHUFD(void)
{
	UINT32 imm8;
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	UINT32 dstbuf[4];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	GET_PCBYTE((imm8));
	for(i=0;i<4;i++){
		dstbuf[i] = data2[imm8 & 0x3];
		imm8 = imm8 >> 2;
	}
	for(i=0;i<4;i++){
		data1[i] = dstbuf[i];
	}
	TRACEOUT(("SSE2_PSHUFD"));
}
//void SSE2_PSLLDQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSLLB(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSLLW(void)
{
	UINT32 data2buf[4];
	UINT16 *data1;
	UINT32 *data2;
	UINT32 shift;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	shift = data2[0];
	if(data2[1] || data2[2] || data2[3]) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	
	for(i=0;i<8;i++){
		data1[i] = (shift >= 16 ? 0 : (data1[i] << (UINT16)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
	}
	TRACEOUT(("SSE2_PSLLW"));
}
void SSE2_PSLLD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1;
	UINT32 *data2;
	UINT32 shift;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	shift = data2[0];
	if(data2[1] || data2[2] || data2[3]) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	
	for(i=0;i<4;i++){
		data1[i] = (shift >= 32 ? 0 : (data1[i] << (UINT32)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
	}
	TRACEOUT(("SSE2_PSLLD"));
}
void SSE2_PSLLQ(void)
{
	UINT32 data2buf[4];
	UINT64 *data1;
	UINT32 *data2;
	UINT32 shift;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	shift = data2[0];
	if(data2[1] || data2[2] || data2[3]) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	
	for(i=0;i<2;i++){
		data1[i] = (shift >= 64 ? 0 : (data1[i] << (UINT64)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
	}
	TRACEOUT(("SSE2_PSLLQ"));
}
//void SSE2_PSLLBimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSLLWimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSLLDimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSLLQimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRAB(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSRAW(void)
{
	UINT32 data2buf[4];
	UINT16 *data1;
	UINT32 *data2;
	UINT32 shift;
	UINT16 signval;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	shift = data2[0];
	if(data2[1] || data2[2] || data2[3]) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	
	// 無理やり算術シフト（怪しい）
	if(16 <= shift){
		signval = 0xffff;
	}else{
		UINT32 rshift = 16 - shift;
		signval = (0xffff >> rshift) << rshift;
	}
	for(i=0;i<8;i++){
		if(((INT16*)data1)[i] < 0){
			data1[i] = (data1[i] >> shift) | signval;
		}else{
			data1[i] = (shift >= 16 ? 0 : (data1[i] >> (UINT16)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
		}
	}
	TRACEOUT(("SSE2_PSRAW"));
}
void SSE2_PSRAD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1;
	UINT32 *data2;
	UINT32 shift;
	UINT32 signval;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	shift = data2[0];
	if(data2[1] || data2[2] || data2[3]) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	
	// 無理やり算術シフト（怪しい）
	if(32 <= shift){
		signval = 0xffffffff;
	}else{
		UINT32 rshift = 32 - shift;
		signval = (0xffffffff >> rshift) << rshift;
	}
	for(i=0;i<2;i++){
		if(((INT32*)data1)[i] < 0){
			data1[i] = (data1[i] >> shift) | signval;
		}else{
			data1[i] = (shift >= 32 ? 0 : (data1[i] >> (UINT32)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
		}
	}
	TRACEOUT(("SSE2_PSRAD"));
}
//void SSE2_PSRAQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRABimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRAWimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRADimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRAQimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRLDQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRLB(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSRLW(void)
{
	UINT32 data2buf[4];
	UINT16 *data1;
	UINT32 *data2;
	UINT32 shift;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	shift = data2[0];
	if(data2[1] || data2[2] || data2[3]) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	
	for(i=0;i<8;i++){
		data1[i] = (shift >= 16 ? 0 : (data1[i] >> (UINT16)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
	}
	TRACEOUT(("SSE2_PSRLW"));
}
void SSE2_PSRLD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1;
	UINT32 *data2;
	UINT32 shift;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	shift = data2[0];
	if(data2[1] || data2[2] || data2[3]) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	
	for(i=0;i<4;i++){
		data1[i] = (shift >= 32 ? 0 : (data1[i] >> (UINT32)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
	}
	TRACEOUT(("SSE2_PSRLD"));
}
void SSE2_PSRLQ(void)
{
	UINT32 data2buf[4];
	UINT64 *data1;
	UINT32 *data2;
	UINT32 shift;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	shift = data2[0];
	if(data2[1] || data2[2] || data2[3]) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	
	for(i=0;i<2;i++){
		data1[i] = (shift >= 64 ? 0 : (data1[i] >> (UINT64)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
	}
	TRACEOUT(("SSE2_PSRLQ"));
}
//void SSE2_PSRLBimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRLWimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRLDimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSRLQimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSxxWimm(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT16 *dstreg;
	UINT16 signval;
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	dstreg = (UINT16*)(&(FPU_STAT.xmm_reg[sub]));
	GET_PCBYTE((shift));
	
	switch(idx){
	case 2: // PSRLW(imm8)
		for(i=0;i<8;i++){
			dstreg[i] = (shift >= 16 ? 0 : (dstreg[i] >> (UINT16)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
		}
		break;
	case 4: // PSRAW(imm8)
		// 無理やり算術シフト（怪しい）
		if(16 <= shift){
			signval = 0xffff;
		}else{
			UINT32 rshift = 16 - shift;
			signval = (0xffff >> rshift) << rshift;
		}
		for(i=0;i<8;i++){
			if(((INT16*)dstreg)[i] < 0){
				dstreg[i] = (dstreg[i] >> shift) | signval;
			}else{
				dstreg[i] = (shift >= 16 ? 0 : (dstreg[i] >> (UINT16)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
			}
		}
		break;
	case 6: // PSLLW(imm8)
		for(i=0;i<8;i++){
			dstreg[i] = (shift >= 16 ? 0 : (dstreg[i] << (UINT16)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
		}
		break;
	default:
		break;
	}
	TRACEOUT(("SSE2_PSxxWimm"));
}
void SSE2_PSxxDimm(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT32 *dstreg;
	UINT32 signval;
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	dstreg = (UINT32*)(&(FPU_STAT.xmm_reg[sub]));
	GET_PCBYTE((shift));
	
	switch(idx){
	case 2: // PSRLD(imm8)
		for(i=0;i<4;i++){
			dstreg[i] = (shift >= 32 ? 0 : (dstreg[i] >> (UINT32)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
		}
		break;
	case 4: // PSRAD(imm8)
		// 無理やり算術シフト（怪しい）
		if(32 <= shift){
			signval = 0xffffffff;
		}else{
			UINT32 rshift = 32 - shift;
			signval = (0xffffffff >> rshift) << rshift;
		}
		for(i=0;i<4;i++){
			if(((INT32*)dstreg)[i] < 0){
				dstreg[i] = (dstreg[i] >> shift) | signval;
			}else{
				dstreg[i] = (shift >= 32 ? 0 : (dstreg[i] >> (UINT16)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
			}
		}
		break;
	case 6: // PSLLD(imm8)
		for(i=0;i<4;i++){
			dstreg[i] = (shift >= 32 ? 0 : (dstreg[i] << (UINT32)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
		}
		break;
	default:
		break;
	}
	TRACEOUT(("SSE2_PSxxDimm"));
}
void SSE2_PSxxQimm(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT64 *dstreg;
	int i;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	dstreg = (UINT64*)(&(FPU_STAT.xmm_reg[sub]));
	GET_PCBYTE((shift));
	
	switch(idx){
	case 2: // PSRLQ(imm8)
		for(i=0;i<2;i++){
			dstreg[i] = (shift >= 64 ? 0 : (dstreg[i] >> (UINT64)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
		}
		break;
	case 3: // PSRLDQ
		// 無理やり128bit右シフト 怪しいので要検証
		shift *= 8; // シフト量はバイト数で指定
		if(shift == 0){
			// シフト無しなら何もしない
		}else if(shift >= 128){
			// シフトが128以上の時
			dstreg[0] = dstreg[1] = 0;
		}else if(shift >= 64){
			// シフトが64以上の時
			dstreg[0] = dstreg[1] >> (shift - 64);
			dstreg[1] = 0;
		}else{
			// シフトが64より小さい時
			dstreg[0] = (dstreg[0] >> shift) | (dstreg[1] << (64-shift)); // 下位QWORD右シフト＆上から降りてきたビットをOR
			dstreg[1] = (dstreg[1] >> shift);
		}
		break;
	case 4: // PSRAQ(imm8)
		EXCEPTION(UD_EXCEPTION, 0);
		break;
	case 6: // PSLLQ(imm8)
		for(i=0;i<2;i++){
			dstreg[i] = (shift >= 64 ? 0 : (dstreg[i] << (UINT64)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
		}
		break;
	case 7: // PSLLDQ
		// 無理やり128bit左シフト 怪しいので要検証
		shift *= 8; // シフト量はバイト数で指定
		if(shift == 0){
			// シフト無しなら何もしない
		}else if(shift >= 128){
			// シフトが128以上の時
			dstreg[0] = dstreg[1] = 0;
		}else if(shift >= 64){
			// シフトが64以上の時
			dstreg[1] = dstreg[0] << (shift - 64);
			dstreg[0] = 0;
		}else{
			// シフトが64より小さい時
			dstreg[1] = (dstreg[1] << shift) | (dstreg[0] >> (64-shift)); // 上位QWORD左シフト＆下から上がってきたビットをOR
			dstreg[0] = (dstreg[0] << shift);
		}
		break;
	default:
		break;
	}
	TRACEOUT(("SSE2_PSxxQimm"));
}
void SSE2_PSUBQmm(void)
{
	UINT32 op;
	UINT idx, sub;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	MMX_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.reg[idx].ll -= FPU_STAT.reg[sub].ll;
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		FPU_STAT.reg[idx].ll -= (SINT64)cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, madr);
	}
	TRACEOUT(("SSE2_PSUBQmm"));
}
void SSE2_PSUBQxmm(void)
{
	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64(&data1, &data2, data2buf);
	for(i=0;i<2;i++){
		data1[i] = data1[i] - data2[i];
	}
	TRACEOUT(("SSE2_PSUBQxmm"));
}
void SSE2_PSUBB(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = data1[i] - data2[i];
	}
	TRACEOUT(("SSE2_PSUBB"));
}
void SSE2_PSUBW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = data1[i] - data2[i];
	}
	TRACEOUT(("SSE2_PSUBW"));
}
void SSE2_PSUBD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = data1[i] - data2[i];
	}
	TRACEOUT(("SSE2_PSUBD"));
}
//void SSE2_PSUBQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSUBSB(void)
{
	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		SINT16 cbuf = (SINT16)data1[i] - (SINT16)data2[i];
		if(cbuf > 127){
			data1[i] = 127;
		}else if(cbuf < -128){
			data1[i] = -128;
		}else{
			data1[i] = (SINT8)cbuf;
		}
	}
	TRACEOUT(("SSE2_PSUBSB"));
}
void SSE2_PSUBSW(void)
{
	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		SINT32 cbuf = (SINT32)data1[i] - (SINT32)data2[i];
		if(cbuf > 32767){
			data1[i] = 32767;
		}else if(cbuf < -32768){
			data1[i] = -32768;
		}else{
			data1[i] = (SINT16)cbuf;
		}
	}
	TRACEOUT(("SSE2_PSUBSW"));
}
//void SSE2_PSUBSD(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSUBSQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSUBUSB(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
		SINT16 cbuf = (SINT16)data1[i] - (SINT16)data2[i];
		if(cbuf > 255){
			data1[i] = 255;
		}else if(cbuf < 0){
			data1[i] = 0;
		}else{
			data1[i] = (UINT8)cbuf;
		}
	}
	TRACEOUT(("SSE2_PSUBUSB"));
}
void SSE2_PSUBUSW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<8;i++){
		SINT32 cbuf = (SINT32)data1[i] - (SINT32)data2[i];
		if(cbuf > 65535){
			data1[i] = 65535;
		}else if(cbuf < 0){
			data1[i] = 0;
		}else{
			data1[i] = (UINT16)cbuf;
		}
	}
	TRACEOUT(("SSE2_PSUBUSW"));
}
//void SSE2_PSUBUSD(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSUBUSQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PUNPCKHBW(void)
{
	UINT8 data2buf[16];
	UINT8 *data1;
	UINT8 *data2;
	UINT8 dstregbuf[16];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	for(i=0;i<8;i++){
		dstregbuf[i*2] = data1[i+8];
		dstregbuf[i*2 + 1] = data2[i+8];
	}
	for(i=0;i<16;i++){
		data1[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PUNPCKHBW"));
}
void SSE2_PUNPCKHWD(void)
{
	UINT16 data2buf[8];
	UINT16 *data1;
	UINT16 *data2;
	UINT16 dstregbuf[8];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	for(i=0;i<4;i++){
		dstregbuf[i*2] = data1[i+4];
		dstregbuf[i*2 + 1] = data2[i+4];
	}
	for(i=0;i<8;i++){
		data1[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PUNPCKHWD"));
}
void SSE2_PUNPCKHDQ(void)
{
	UINT32 data2buf[4];
	UINT32 *data1;
	UINT32 *data2;
	UINT32 dstregbuf[4];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	for(i=0;i<2;i++){
		dstregbuf[i*2] = data1[i+2];
		dstregbuf[i*2 + 1] = data2[i+2];
	}
	for(i=0;i<4;i++){
		data1[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PUNPCKHDQ"));
}
void SSE2_PUNPCKHQDQ(void)
{
	UINT64 data2buf[2];
	UINT64 *data1;
	UINT64 *data2;
	UINT64 dstregbuf[2];
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	dstregbuf[0] = data1[1];
	dstregbuf[1] = data2[1];
	data1[0] = dstregbuf[0];
	data1[1] = dstregbuf[1];
	TRACEOUT(("SSE2_PUNPCKHQDQ"));
}
void SSE2_PUNPCKLBW(void)
{
	UINT8 data2buf[16];
	UINT8 *data1;
	UINT8 *data2;
	UINT8 dstregbuf[16];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	for(i=0;i<8;i++){
		dstregbuf[i*2] = data1[i];
		dstregbuf[i*2 + 1] = data2[i];
	}
	for(i=0;i<16;i++){
		data1[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PUNPCKLBW"));
}
void SSE2_PUNPCKLWD(void)
{
	UINT16 data2buf[8];
	UINT16 *data1;
	UINT16 *data2;
	UINT16 dstregbuf[8];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	for(i=0;i<4;i++){
		dstregbuf[i*2] = data1[i];
		dstregbuf[i*2 + 1] = data2[i];
	}
	for(i=0;i<8;i++){
		data1[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PUNPCKLWD"));
}
void SSE2_PUNPCKLDQ(void)
{
	UINT32 data2buf[4];
	UINT32 *data1;
	UINT32 *data2;
	UINT32 dstregbuf[4];
	int i;
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	for(i=0;i<2;i++){
		dstregbuf[i*2] = data1[i];
		dstregbuf[i*2 + 1] = data2[i];
	}
	for(i=0;i<4;i++){
		data1[i] = dstregbuf[i];
	}
	TRACEOUT(("SSE2_PUNPCKLDQ"));
}
void SSE2_PUNPCKLQDQ(void)
{
	UINT64 data2buf[2];
	UINT64 *data1;
	UINT64 *data2;
	UINT64 dstregbuf[2];
	
	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	
	dstregbuf[0] = data1[0];
	dstregbuf[1] = data2[0];
	data1[0] = dstregbuf[0];
	data1[1] = dstregbuf[1];
	TRACEOUT(("SSE2_PUNPCKLQDQ"));
}
void SSE2_PXOR(void)
{
	SSE_XORPS();
	TRACEOUT(("SSE2_PXOR"));
}

void SSE2_MASKMOVDQU(void)
{
	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	int i;

	SSE_PART_GETDATA1DATA2_PD_UINT64((UINT64**)(&data1), (UINT64**)(&data2), (UINT64*)data2buf);
	for(i=0;i<16;i++){
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
		CPU_DI -= 16;
	} else {
		CPU_EDI -= 16;
	}
	TRACEOUT(("SSE2_MASKMOVDQU"));
}
//void SSE2_CLFLUSH(UINT32 op)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_MOVNTPD(void)
{
	SSE_MOVNTPS();
	TRACEOUT(("SSE2_MOVNTPD"));
}
void SSE2_MOVNTDQ(void)
{
	SSE_MOVNTPS();
	TRACEOUT(("SSE2_MOVNTDQ"));
}
void SSE2_MOVNTI(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 *data1;
	
	SSE2_check_NM_EXCEPTION();
	SSE2_setTag();
	CPU_SSE2WORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	data1 = reg32_b53[(op)]; // これ合ってる？
	if ((op) >= 0xc0) {
		EXCEPTION(UD_EXCEPTION, 0);
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, maddr, *data1);
	}
	TRACEOUT(("SSE2_MOVNTI"));
}
void SSE2_PAUSE(void)
{
	// Nothing to do
	TRACEOUT(("SSE2_PAUSE"));
}
//void SSE2_LFENCE(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_MFENCE(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}

#else

/*
 * SSE2 interface
 */

void SSE2_ADDPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_ADDSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_ANDNPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_ANDPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CMPPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CMPSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_COMISD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTPI2PD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTPD2PI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTSI2SD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTSD2SI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTTPD2PI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTTSD2SI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTPD2PS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTPS2PD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTSD2SS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTSS2SD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTPD2DQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTTPD2DQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTDQ2PD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTPS2DQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTTPS2DQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_CVTDQ2PS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_DIVPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_DIVSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MAXPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MAXSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MINPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MINSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVAPDmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVAPDxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVHPDmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVHPDxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVLPDmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVLPDxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVMSKPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVSDmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVSDxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVUPDmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVUPDxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MULPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MULSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_ORPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_SHUFPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_SQRTPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_SQRTSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_STMXCSR(UINT32 maddr)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_SUBPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_SUBSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_UCOMISD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_UNPCKHPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_UNPCKLPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_XORPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE2_MOVDrm2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVDxmm2rm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVDQAmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVDQAxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVDQUmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVDQUxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVQ2DQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVDQ2Q(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVQmem2xmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVQxmm2mem(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PACKSSDW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PACKSSWB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PACKUSWB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PADDQmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PADDQxmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PADDB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PADDW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PADDD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PADDQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PADDSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PADDSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PADDSD(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PADDSQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PADDUSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PADDUSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PADDUSD(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PADDUSQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PAND(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PANDN(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PAVGB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PAVGW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PCMPEQB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PCMPEQW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PCMPEQD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PCMPEQQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PCMPGTB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PCMPGTW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PCMPGTD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PCMPGTQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PEXTRW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PINSRW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMADD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMAXSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMAXUB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMINSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMINUB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMOVMSKB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMULHUW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMULHW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMULLW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMULUDQmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PMULUDQxmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_POR(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSADBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSHUFLW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSHUFHW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSHUFD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSLLDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSLLB(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSLLW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSLLD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSLLQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSLLBimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSLLWimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSLLDimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSLLQimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSRAB(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSRAW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRAD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRAQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSRABimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSRAWimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRADimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRAQimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRLDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSRLB(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSRLW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRLD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRLQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSRLBimm(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSRLWimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRLDimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSRLQimm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSUBQmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSUBQxmm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSUBB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSUBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSUBD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSUBQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSUBSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSUBSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSUBSD(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSUBSQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PSUBUSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PSUBUSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_PSUBUSD(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_PSUBUSQ(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_PUNPCKHBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PUNPCKHWD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PUNPCKHDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PUNPCKHQDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PUNPCKLBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PUNPCKLWD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PUNPCKLDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PUNPCKLQDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PXOR(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE2_MASKMOVDQU(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_CLFLUSH(UINT32 op)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
void SSE2_MOVNTPD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVNTDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_MOVNTI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void SSE2_PAUSE(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
//void SSE2_LFENCE(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}
//void SSE2_MFENCE(void)
//{
//	EXCEPTION(UD_EXCEPTION, 0);
//}


#endif
