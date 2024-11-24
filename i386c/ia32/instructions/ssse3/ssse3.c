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
#include "ssse3.h"

#if defined(USE_SSSE3) && defined(USE_SSE3) && defined(USE_SSE2) && defined(USE_SSE) && defined(USE_FPU)

#define CPU_SSSE3WORKCLOCK	CPU_WORKCLOCK(2)

static INLINE void
SSSE3_check_NM_EXCEPTION(){
	// SSSE3 Ȃ  Ȃ UD(     I y R [ h  O) 𔭐       
	if(!(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_SSSE3)){
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
SSSE3_setTag(void)
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

/*
 * SSE3 interface
 */

//  R [ h       Ȃ ̂ł ⋭   ɋ  ʉ 
// xmm/m128 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_PD(double **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSSE3_check_NM_EXCEPTION();
	SSSE3_setTag();
	CPU_SSSE3WORKCLOCK;
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

static INLINE void MMX_PART_GETDATA1DATA2_PD(float **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSSE3_check_NM_EXCEPTION();
	SSSE3_setTag();
	CPU_SSSE3WORKCLOCK;
	GET_PCBYTE((op));
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

void SSSE3_PSHUFB(void)
{
	int i;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<16;i++){
		if (data2[i]&128){
			data1[i] = 0;
		} else {
			data1[i] = data2buf[data2[i]&7];
		}
	}
	TRACEOUT(("SSSE3_PSHUFB"));
}

void SSSE3_PSHUFB_MM(void)
{
	int i;

	UINT8 data2buf[8];
	UINT8 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<8;i++){
		if (data2[i]&128){
			data1[i] = 0;
		} else {
			data1[i] = data2buf[data2[i]&7];
		}
	}
	TRACEOUT(("SSSE3_PSHUFB"));
}

void SSSE3_PHADDW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	data1[0] = data1[1] + data1[0];
	data1[1] = data1[3] + data1[2];
	data1[2] = data1[5] + data1[4];
	data1[3] = data1[7] + data1[6];
	data1[4] = data2[1] + data2[0];
	data1[5] = data2[3] + data2[2];
	data1[6] = data2[5] + data2[4];
	data1[7] = data2[7] + data2[6];
	TRACEOUT(("SSSE3_PHADDW"));
}

void SSSE3_PHADDW_MM(void)
{
	UINT16 data2buf[4];
	UINT16 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	data1[0] = data1[1] + data1[0];
	data1[1] = data1[3] + data1[2];
	data1[2] = data2[1] + data2[0];
	data1[3] = data2[3] + data2[2];
	TRACEOUT(("SSSE3_PHADDW"));
}

void SSSE3_PHADDD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	data1[0] = data1[1] + data1[0];
	data1[1] = data1[3] + data1[2];
	data1[2] = data2[1] + data2[0];
	data1[3] = data2[3] + data2[2];
	TRACEOUT(("SSSE3_PHADDD"));
}

void SSSE3_PHADDD_MM(void)
{
	UINT32 data2buf[2];
	UINT32 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	data1[0] = data1[1] + data1[0];
	data1[1] = data2[1] + data2[0];
	TRACEOUT(("SSSE3_PHADDD"));
}

void SSSE3_PHADDSW(void)
{
	int i;

	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	SINT32 tmpsinedcalc;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		tmpsinedcalc = data1[i * 2 + 0] + data1[i * 2 + 1]; data1[i + 0] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
		tmpsinedcalc = data2[i * 2 + 0] + data2[i * 2 + 1]; data1[i + 4] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	}
	TRACEOUT(("SSSE3_PHADDSW"));
}

void SSSE3_PHADDSW_MM(void)
{
	int i;

	SINT16 data2buf[4];
	SINT16 *data1, *data2;
	SINT32 tmpsinedcalc;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<2;i++){
		tmpsinedcalc = data1[i * 2 + 0] + data1[i * 2 + 1]; data1[i + 0] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
		tmpsinedcalc = data2[i * 2 + 0] + data2[i * 2 + 1]; data1[i + 2] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	}
	TRACEOUT(("SSSE3_PHADDSW"));
}

void SSSE3_PMADDUBSW(void)
{
	int i;

	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	SINT32 tmpsinedcalc;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<8;i++){
		tmpsinedcalc = (INT32)(data1[i * 2 + 0]) * data2[i * 2 + 0] + (INT32)(data1[i * 2 + 1]) * data2[i * 2 + 1];
		data1[i] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	}
	TRACEOUT(("SSSE3_PMADDUBSW"));
}

void SSSE3_PMADDUBSW_MM(void)
{
	int i;

	SINT16 data2buf[4];
	SINT16 *data1, *data2;
	SINT32 tmpsinedcalc;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<4;i++){
		tmpsinedcalc = (INT32)(data1[i * 2 + 0]) * data2[i * 2 + 0] + (INT32)(data1[i * 2 + 1]) * data2[i * 2 + 1];
		data1[i] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	}
	TRACEOUT(("SSSE3_PMADDUBSW"));
}

void SSSE3_PHSUBW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	data1[0] = data1[0] - data1[1];
	data1[1] = data1[2] - data1[3];
	data1[2] = data1[4] - data1[5];
	data1[3] = data1[6] - data1[7];
	data1[4] = data2[0] - data2[1];
	data1[5] = data2[2] - data2[3];
	data1[6] = data2[4] - data2[5];
	data1[7] = data2[6] - data2[7];
	TRACEOUT(("SSSE3_PHSUBW"));
}

void SSSE3_PHSUBW_MM(void)
{
	UINT16 data2buf[4];
	UINT16 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	data1[0] = data1[0] - data1[1];
	data1[1] = data1[2] - data1[3];
	data1[2] = data2[0] - data2[1];
	data1[3] = data2[2] - data2[3];
	TRACEOUT(("SSSE3_PHSUBW"));
}

void SSSE3_PHSUBD(void)
{
	UINT32 data2buf[4];
	UINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	data1[0] = data1[0] - data1[1];
	data1[1] = data1[2] - data1[3];
	data1[2] = data2[0] - data2[1];
	data1[3] = data2[2] - data2[3];
	TRACEOUT(("SSSE3_PHSUBD"));
}

void SSSE3_PHSUBD_MM(void)
{
	UINT32 data2buf[2];
	UINT32 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	data1[0] = data1[0] - data1[1];
	data1[1] = data2[0] - data2[1];
	TRACEOUT(("SSSE3_PHSUBD"));
}

void SSSE3_PHSUBSW(void)
{
	UINT16 data2buf[8];
	UINT16 *data1, *data2;
	INT32 tmpsinedcalc;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	tmpsinedcalc = data1[0] - data1[1]; data1[0] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data1[2] - data1[3]; data1[1] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data1[4] - data1[5]; data1[2] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data1[6] - data1[7]; data1[3] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data2[0] - data2[1]; data1[4] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data2[2] - data2[3]; data1[5] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data2[4] - data2[5]; data1[6] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data2[6] - data2[7]; data1[7] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	TRACEOUT(("SSSE3_PHSUBSW"));
}

void SSSE3_PHSUBSW_MM(void)
{
	UINT16 data2buf[4];
	UINT16 *data1, *data2;
	INT32 tmpsinedcalc;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	tmpsinedcalc = data1[0] - data1[1]; data1[0] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data1[2] - data1[3]; data1[1] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data2[0] - data2[1]; data1[2] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	tmpsinedcalc = data2[2] - data2[3]; data1[3] = (tmpsinedcalc>32767)?32767:(((tmpsinedcalc)<-32768)?-32768:tmpsinedcalc);
	TRACEOUT(("SSSE3_PHSUBSW"));
}

void SSSE3_PSIGNB(void)
{
	int i;

	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = (data2[i]<0)?-1:((data2[i]>0)?1:0);
	}
	TRACEOUT(("SSSE3_PSIGNB"));
}

void SSSE3_PSIGNB_MM(void)
{
	int i;

	SINT8 data2buf[8];
	SINT8 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<8;i++){
		if (data2[i] < 0){
			data1[i] = -data1[i];
		} else if (data2[i] == 0){
			data1[i] = 0;
		}
	}
	TRACEOUT(("SSSE3_PSIGNB"));
}

void SSSE3_PSIGNW(void)
{
	int i;

	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = (data2[i]<0)?-1:((data2[i]>0)?1:0);
	}
	TRACEOUT(("SSSE3_PSIGNW"));
}

void SSSE3_PSIGNW_MM(void)
{
	int i;

	SINT16 data2buf[4];
	SINT16 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<4;i++){
		if (data2[i] < 0){
			data1[i] = -data1[i];
		} else if (data2[i] == 0){
			data1[i] = 0;
		}
	}
	TRACEOUT(("SSSE3_PSIGNW"));
}

void SSSE3_PSIGND(void)
{
	int i;

	SINT32 data2buf[4];
	SINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = (data2[i]<0)?-1:((data2[i]>0)?1:0);
	}
	TRACEOUT(("SSSE3_PSIGND"));
}

void SSSE3_PSIGND_MM(void)
{
	int i;

	SINT32 data2buf[2];
	SINT32 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<2;i++){
		if (data2[i] < 0){
			data1[i] = -data1[i];
		} else if (data2[i] == 0){
			data1[i] = 0;
		}
	}
	TRACEOUT(("SSSE3_PSIGND"));
}

void SSSE3_PMULHRSW(void)
{
	int i;

	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	SINT32 tmpsinedcalc;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<8;i++){
		tmpsinedcalc = ((((INT32)(data1[i])*(INT32)(data2[i]))>>14) + 1)>>1;
		data1[i] = tmpsinedcalc&0xffff;
	}
	TRACEOUT(("SSSE3_PMULHRSW"));
}

void SSSE3_PMULHRSW_MM(void)
{
	int i;

	SINT16 data2buf[4];
	SINT16 *data1, *data2;
	SINT32 tmpsinedcalc;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<4;i++){
		tmpsinedcalc = ((((INT32)(data1[i])*(INT32)(data2[i]))>>14) + 1)>>1;
		data1[i] = tmpsinedcalc&0xffff;
	}
	TRACEOUT(("SSSE3_PMULHRSW"));
}

void SSSE3_PABSB(void)
{
	int i;

	SINT8 data2buf[16];
	SINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<16;i++){
		data1[i] = abs(data2[i]);
	}
	TRACEOUT(("SSSE3_PABSB"));
}

void SSSE3_PABSB_MM(void)
{
	int i;

	SINT8 data2buf[8];
	SINT8 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = abs(data2[i]);
	}
	TRACEOUT(("SSSE3_PABSB"));
}

void SSSE3_PABSW(void)
{
	int i;

	SINT16 data2buf[8];
	SINT16 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<8;i++){
		data1[i] = abs(data2[i]);
	}
	TRACEOUT(("SSSE3_PABSW"));
}

void SSSE3_PABSW_MM(void)
{
	int i;

	SINT16 data2buf[4];
	SINT16 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = abs(data2[i]);
	}
	TRACEOUT(("SSSE3_PABSW"));
}

void SSSE3_PABSD(void)
{
	int i;

	SINT32 data2buf[4];
	SINT32 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<4;i++){
		data1[i] = abs(data2[i]);
	}
	TRACEOUT(("SSSE3_PABSD"));
}

void SSSE3_PABSD_MM(void)
{
	int i;

	SINT32 data2buf[2];
	SINT32 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	for(i=0;i<2;i++){
		data1[i] = abs(data2[i]);
	}
	TRACEOUT(("SSSE3_PABSD"));
}

void SSSE3_PALIGNR(void)
{
	UINT32 op;
	UINT64 data2buf[2];
	UINT64 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	GET_PCBYTE((op));
	if (op > 15) {
		op -= 16;
		if (op > 15){
			data1[0] = 0;
			data1[1] = 0;
		} else {
			if (op > 7){
				data1[0] = data1[1];
				op -= 8;
			}
			op <<= 3;
			if (op != 0){
				data1[1] = (data1[1] << op)|(data1[0] >> (64 - op));
				data1[0] = (data1[0] << op);
			}
		}
	} else {
		op <<= 3;
		if (op > 64){
			op -= 64;
			data1[0] = (data2[1] >> op)|(data1[0] << (64 - op));
			data1[1] = (data1[0] >> op)|(data1[1] << (64 - op));
		} else if (op == 64){
			data1[0] = data1[1];
			data1[1] = data2[1];
		} else if (op != 0){
			op -= 64;
			data1[0] = (data2[0] >> op)|(data2[1] << (64 - op));
			data1[1] = (data2[1] >> op)|(data1[0] << (64 - op));
		}
	}
	TRACEOUT(("SSSE3_PALIGNR"));
}

void SSSE3_PALIGNR_MM(void)
{
	UINT32 op;
	UINT32 data2buf[2];
	UINT32 *data1, *data2;
	MMX_PART_GETDATA1DATA2_PD((float**)(&data1), (float**)(&data2), (float*)data2buf);
	GET_PCBYTE((op));
	if (op > 7) {
		op -= 8;
		if (op > 7){
			data1[0] = 0;
			data1[1] = 0;
		} else {
			if (op > 3){
				data1[0] = data1[1];
				op -= 4;
			}
			op <<= 3;
			if (op != 0){
				data1[1] = (data1[1] << op)|(data1[0] >> (32 - op));
				data1[0] = (data1[0] << op);
			}
		}
	} else {
		op <<= 3;
		if (op > 32){
			op -= 32;
			data1[0] = (data2[1] >> op)|(data1[0] << (32 - op));
			data1[1] = (data1[0] >> op)|(data1[1] << (32 - op));
		} else if (op == 32){
			data1[0] = data1[1];
			data1[1] = data2[1];
		} else if (op != 0){
			op -= 32;
			data1[0] = (data2[0] >> op)|(data2[1] << (32 - op));
			data1[1] = (data2[1] >> op)|(data1[0] << (32 - op));
		}
	}
	TRACEOUT(("SSSE3_PALIGNR"));
}

#else

void SSSE3_PSHUFB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PSHUFB_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHADDW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHADDW_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHADDD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHADDD_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHADDSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHADDSW_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PMADDUBSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PMADDUBSW_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHSUBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHSUBW_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHSUBD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHSUBD_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHSUBSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PHSUBSW_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PSIGNB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PSIGNB_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PSIGNW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PSIGNW_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PSIGND(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PSIGND_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PMULHRSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PMULHRSW_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PABSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PABSB_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PABSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PABSW_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PABSD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PABSD_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PALIGNR(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSSE3_PALIGNR_MM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

#endif
