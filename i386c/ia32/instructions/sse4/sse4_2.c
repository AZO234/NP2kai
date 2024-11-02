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
#include "sse4_2.h"

#if defined(USE_SSE4_2) && defined(USE_SSE4_1) && defined(USE_SSSE3) && defined(USE_SSE3) && defined(USE_SSE2) && defined(USE_SSE) && defined(USE_FPU)

#define CPU_SSSE3WORKCLOCK	CPU_WORKCLOCK(2)

static INLINE void
SSE4_2_check_NM_EXCEPTION(){
	// SSE4.2 Ȃ  Ȃ UD(     I y R [ h  O) 𔭐       
	if(!(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_SSE4_2)){
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
SSE4_2_setTag(void)
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
 * SSE4.2 interface
 */

//  R [ h       Ȃ ̂ł ⋭   ɋ  ʉ 
// xmm/m128 -> xmm
static INLINE void SSE_PART_GETDATA1DATA2_PD(double **data1, double **data2, double *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSE4_2_check_NM_EXCEPTION();
	SSE4_2_setTag();
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

static INLINE void SSE_PART_GETDATA1DATA2_P_UINT32(UINT32 **data1, UINT32 **data2, UINT32 *data2buf){
	SSE_PART_GETDATA1DATA2_PD((double**)data1, (double**)data2, (double*)data2buf);
}
static INLINE void SSE_PART_GETDATA1DATA2_PD_UINT64(UINT64 **data1, UINT64 **data2, UINT64 *data2buf){
	SSE_PART_GETDATA1DATA2_PD((double**)data1, (double**)data2, (double*)data2buf);
}

static INLINE void MMX_PART_GETDATA1DATA2_PD(float **data1, float **data2, float *data2buf){
	UINT32 op;
	UINT idx, sub;
	
	SSE4_2_check_NM_EXCEPTION();
	SSE4_2_setTag();
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

// Referenced code at box86/64 for string simd implementation, thanks ptitSeb!

static INLINE int overrideIfDataInvalid(UINT8 **data1, UINT8 **data2, UINT8 *data2buf, int cmpmode, int lmem, int lreg, int iii0, int iii1){
	int valid1 = (iii1 < lreg);
	int valid2 = (iii0 < lmem);
	if(!valid1 && !valid2)
	{
		switch((cmpmode>>2)&3) {
			case 0:
			case 1:
			return 0;
			case 2:
			case 3:
			return 1;
		}
	}
	if(!valid1 && valid2)
	{
		switch((cmpmode>>2)&3) {
			case 0:
			case 1:
			case 2:
			return 0;
			case 3:
			return 1;
		}
	}
	if(valid1 && !valid2)
	{
		if (((cmpmode>>2)&3) == 1){
			switch((cmpmode>>2)&3) {
				case 0:
				return (iii1&1)?(( *((UINT8*)(data2 + (iii1 * 1))) >= *((UINT8*)(data1 + (iii0 * 1))) )):(( *((UINT8*)(data2 + (iii1 * 1))) <= *((UINT8*)(data1 + (iii0 * 1))) ));
				case 1:
				return (iii1&1)?(( *((UINT16*)(data2 + (iii1 * 2))) >= *((UINT16*)(data1 + (iii0 * 2))) )):(( *((UINT16*)(data2 + (iii1 * 2))) <= *((UINT16*)(data1 + (iii0 * 2))) ));
				case 2:
				return (iii1&1)?(( *((SINT8*)(data2 + (iii1 * 1))) >= *((SINT8*)(data1 + (iii0 * 1))) )):(( *((SINT8*)(data2 + (iii1 * 1))) <= *((SINT8*)(data1 + (iii0 * 1))) ));
				case 3:
				return (iii1&1)?(( *((SINT16*)(data2 + (iii1 * 2))) >= *((SINT16*)(data1 + (iii0 * 2))) )):(( *((SINT16*)(data2 + (iii1 * 2))) <= *((SINT16*)(data1 + (iii0 * 2))) ));
			}
		} else {
			switch (cmpmode & 1){
				case 0:
				return (( *((UINT8*)(data2 + (iii1 * 1))) == *((UINT8*)(data1 + (iii0 * 1))) ));
				case 1:
				return (( *((UINT16*)(data2 + (iii1 * 2))) >= *((UINT16*)(data1 + (iii0 * 2))) ));
			}
		}
	}
	return 0; // コンパイルエラー回避仮
}

static INLINE UINT32 sse42_compare_string_explicit_len(UINT8 **data1, UINT8 **data2, UINT8 *data2buf, int cmpmode, int lmem, int lreg){
	UINT32 IntRes1 = 0;
	UINT32 IntRes2 = 0;
	int n_packed = (cmpmode&1)?8:16;
	int iii0, iii1;
	if (lreg<0) { lreg = -lreg; }
	if (lmem<0) { lmem = -lmem; }
	if (lreg>n_packed) { lreg = n_packed; }
	if (lmem>n_packed) { lmem = n_packed; }
	switch((cmpmode >> 2) & 3){
		case 0:
		for(iii0=0;iii0<n_packed;iii0++){
			for(iii1=0;iii1<n_packed;iii1++){
				IntRes1 |= overrideIfDataInvalid(data1,data2,data2buf,cmpmode,lmem,lreg,iii0,iii1) << iii0;
			}
		}
		break;
		case 1:
		for(iii0=0;iii0<n_packed;iii0++){
			for(iii1=0;iii1<n_packed;iii1++){
				IntRes1 |= (overrideIfDataInvalid(data1,data2,data2buf,cmpmode,lmem,lreg,iii0,iii1) & overrideIfDataInvalid(data1,data2,data2buf,cmpmode,lmem,lreg,iii0,iii1+1)) << iii0;
			}
		}
		break;
		case 2:
		for(iii0=0;iii0<n_packed;iii0++){
			IntRes1 |= overrideIfDataInvalid(data1,data2,data2buf,cmpmode,lmem,lreg,iii0,iii0) << iii0;
		}
		break;
		case 3:
		IntRes1 = (1<<n_packed)-1;
		for(iii0=0;iii0<n_packed;iii0++){
			for(iii1=0;iii1<n_packed;iii1++){
				int iii2 = iii0 + iii1;
				IntRes1 &= (((1 << n_packed)-1) ^ (1 << iii0)) | overrideIfDataInvalid(data1,data2,data2buf,cmpmode,lmem,lreg,iii2,iii1) << iii0;
			}
		}
		break;
	}
	IntRes2 = IntRes1;
	switch((cmpmode>>4)&3){
		case 1:
		IntRes2 ^= ((1<<n_packed)-1);
		break;
		case 3:
		IntRes2 ^= ((1<<lmem)-1);
		break;
	}
		if (IntRes2){CPU_FLAG |=  C_FLAG;}else{CPU_FLAG &=  ~C_FLAG;}
		if (lmem<n_packed){CPU_FLAG |=  Z_FLAG;}else{CPU_FLAG &=  ~Z_FLAG;}
		if (lreg<n_packed){CPU_FLAG |=  S_FLAG;}else{CPU_FLAG &=  ~S_FLAG;}
		if (IntRes2&1){CPU_FLAG |=  O_FLAG;}else{CPU_FLAG &=  ~O_FLAG;}
		CPU_FLAG &=  ~A_FLAG;
		CPU_FLAG &=  ~P_FLAG;
		return IntRes2;
}

static INLINE UINT32 sse42_compare_string_inplicit_len(UINT8 **data1, UINT8 **data2, UINT8 *data2buf, int cmpmode){
	int lmem = 0;
	int lreg = 0;
	if (cmpmode&1){
		while(lmem<8 && *((UINT16*)(data1+(lmem * 2))) ){ ++lmem; }
		while(lreg<8 && *((UINT16*)(data2+(lreg * 2))) ){ ++lreg; }
	}else{
		while(lmem<16 && *((UINT8*)(data1+(lmem * 1))) ){ ++lmem; }
		while(lreg<16 && *((UINT8*)(data2+(lreg * 1))) ){ ++lreg; }
	}
	return sse42_compare_string_explicit_len(data1, data2, data2buf, cmpmode, lmem, lreg);
}

void SSE4_2_PCMPGTQ(void)
{
	int i;

	SINT64 data2buf[2];
	SINT64 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	for(i=0;i<2;i++){
		if (data1[i] > data2[i]){
			data1[i] = 0xFFFFFFFFFFFFFFFF;
		} else {
			data1[i] = 0;
		}
	}
	TRACEOUT(("SSE4_2_PCMPGTQ"));
}

void SSE4_2_PCMPESTRM(void)
{
	int i;
	UINT32 tmp;

	UINT32 op;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	GET_PCBYTE((op));
	tmp = sse42_compare_string_explicit_len((UINT8**)(&data1), (UINT8**)(&data2), (UINT8*)data2buf,op,CPU_EDX,CPU_EAX);
	if (op & 64){
		switch (op & 1){
			case 0:
			for(i=0;i<16;++i){*((UINT8*)(data1 + (i * 1)))=((tmp>>i)&1)?0xff:0x00;}
			break;
			case 1:
			for(i=0;i<8;++i){*((UINT16*)(data1 + (i * 2)))=((tmp>>i)&1)?0xffff:0x0000;}
			break;
		}
	} else {
		for(i=0;i<16;++i){data1[i] = 0;}
		*((UINT16*)(data1 + (i * 2))) = tmp;
	}
	TRACEOUT(("SSE4_2_PCMPESTRM"));
}

void SSE4_2_PCMPESTRI(void)
{
	int i;
	UINT32 tmp;

	UINT32 op;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	GET_PCBYTE((op));
	tmp = sse42_compare_string_explicit_len((UINT8**)(&data1), (UINT8**)(&data2), (UINT8*)data2buf,op,CPU_EDX,CPU_EAX);
	if (!tmp){
		CPU_ECX = (op&1)?8:16;
	} else if (op & 64) {
		CPU_ECX = 31;
		for(i=0;i<32;i++){ CPU_ECX -= ((tmp>>i)&1)?0:1; }
	}else{
		CPU_ECX = 0;
		for(i=0;i<32;i++){ if (((tmp>>i)&1)){ break; } CPU_ECX++; }
	}
	TRACEOUT(("SSE4_2_PCMPESTRI"));
}

void SSE4_2_PCMPISTRM(void)
{
	int i;
	UINT32 tmp;

	UINT32 op;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	GET_PCBYTE((op));
	tmp = sse42_compare_string_inplicit_len((UINT8**)(&data1), (UINT8**)(&data2), (UINT8*)data2buf,op);
	if (op & 64){
		switch (op & 1){
			case 0:
			for(i=0;i<16;++i){*((UINT8*)(data1 + (i * 1)))=((tmp>>i)&1)?0xff:0x00;}
			break;
			case 1:
			for(i=0;i<8;++i){*((UINT16*)(data1 + (i * 2)))=((tmp>>i)&1)?0xffff:0x0000;}
			break;
		}
	} else {
		for(i=0;i<16;++i){data1[i] = 0;}
		*((UINT16*)(data1 + (i * 2))) = tmp;
	}
	TRACEOUT(("SSE4_2_PCMPISTRM"));
}

void SSE4_2_PCMPISTRI(void)
{
	int i;
	UINT32 tmp;

	UINT32 op;

	UINT8 data2buf[16];
	UINT8 *data1, *data2;
	SSE_PART_GETDATA1DATA2_PD((double**)(&data1), (double**)(&data2), (double*)data2buf);
	GET_PCBYTE((op));
	tmp = sse42_compare_string_inplicit_len((UINT8**)(&data1), (UINT8**)(&data2), (UINT8*)data2buf,op);
	if (!tmp){
		CPU_ECX = (op&1)?8:16;
	} else if (op & 64) {
		CPU_ECX = 31;
		for(i=0;i<32;i++){ CPU_ECX -= ((tmp>>i)&1)?0:1; }
	}else{
		CPU_ECX = 0;
		for(i=0;i<32;i++){ if (((tmp>>i)&1)){ break; } CPU_ECX++; }
	}
	TRACEOUT(("SSE4_2_PCMPISTRI"));
}

void SSE4_2_CRC32_Gy_Eb(void)
{
	int i;

	UINT32 op;

	UINT32* out;
	UINT32 dst, madr, tmp, src;

	tmp = 0;

	GET_PCBYTE((op));

	if (op >= 0xc0)
	{
		CPU_WORKCLOCK(21);
		src = *(reg8_b20[op]);
	}
	else
	{
		CPU_WORKCLOCK(24);
		madr = calc_ea_dst(op);
		src = cpu_vmemoryread_b(CPU_INST_SEGREG_INDEX, madr);
	}
	out = reg32_b53[op];
	tmp = *out;

	tmp ^= src;
	for(i=0;i<8;i++){
		if (tmp & 1)
		{
			tmp = (tmp >> 1) ^ 0x82f63b78;
		}else{
			tmp = (tmp >> 1);
		}
	}
	*out = (UINT32)tmp;
	TRACEOUT(("SSE4_2_CRC32_Gy_Eb"));
}

void SSE4_2_CRC32_Gy_Eb_16(void)
{
	int i;

	UINT32 op;

	UINT32* out;
	UINT32 dst, madr, tmp, src;

	tmp = 0;

	GET_PCBYTE((op));

	if (op >= 0xc0)
	{
		CPU_WORKCLOCK(21);
		src = *(reg8_b20[op]);
	}
	else
	{
		CPU_WORKCLOCK(24);
		madr = calc_ea_dst(op);
		src = cpu_vmemoryread_b(CPU_INST_SEGREG_INDEX, madr);
	}
	out = (UINT32*)reg16_b53[op];
	tmp = *out;

	tmp ^= src;
	for (i = 0; i < 8; i++)
	{
		if (tmp & 1)
		{
			tmp = (tmp >> 1) ^ 0x82f63b78;
		}
		else
		{
			tmp = (tmp >> 1);
		}
	}
	*out = (UINT32)tmp;
	TRACEOUT(("SSE4_2_CRC32_Gy_Eb"));
}

void SSE4_2_CRC32_Gy_Ev(void)
{
	int i, j;

	UINT32 op;

	UINT32* out;
	UINT32 dst, madr, tmp, src;

	tmp = 0;

	GET_PCBYTE((op));

	if (op >= 0xc0)
	{
		CPU_WORKCLOCK(21);
		src = *(reg32_b20[op]);
	}
	else
	{
		CPU_WORKCLOCK(24);
		madr = calc_ea_dst(op);
		src = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}
	out = reg32_b53[op];
	tmp = *out;

	for(j=0;j<4;j++){
		tmp ^= ((src >> (j * 8)) & 0xFF);
		for(i=0;i<8;i++){
			if (tmp & 1)
			{
				tmp = (tmp >> 1) ^ 0x82f63b78;
			}else{
				tmp = (tmp >> 1);
			}
		}
	}
	*out = (UINT32)tmp;
	TRACEOUT(("SSE4_2_CRC32_Gy_Ev"));
}

void SSE4_2_CRC32_Gy_Ev_16(void)
{
	int i, j;

	UINT32 op;

	UINT32* out;
	UINT32 dst, madr, tmp, src;

	tmp = 0;

	GET_PCBYTE((op));

	if (op >= 0xc0)
	{
		CPU_WORKCLOCK(21);
		src = *(reg16_b20[op]);
	}
	else
	{
		CPU_WORKCLOCK(24);
		madr = calc_ea_dst(op);
		src = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}
	out = (UINT32*)reg16_b53[op];
	tmp = *out;

	for (j = 0; j < 2; j++)
	{
		tmp ^= ((src >> (j * 8)) & 0xFF);
		for (i = 0; i < 8; i++)
		{
			if (tmp & 1)
			{
				tmp = (tmp >> 1) ^ 0x82f63b78;
			}
			else
			{
				tmp = (tmp >> 1);
			}
		}
	}
	*out = (UINT32)tmp;
	TRACEOUT(("SSE4_2_CRC32_Gy_Ev"));
}

void SSE4_2_POPCNT_16(void)
{
	int i;

	UINT32 op;

	UINT16 *out;
	UINT32 dst, madr,tmp,src;

	if (!(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_POPCNT)) {
		EXCEPTION(UD_EXCEPTION, 0);
		return;
	}

	tmp = 0;

	GET_PCBYTE((op));

	if (op >= 0xc0) {
		CPU_WORKCLOCK(21);
		src = *(reg16_b20[op]);
	} else {
		CPU_WORKCLOCK(24);
		madr = calc_ea_dst(op);
		src = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
	}

	for(i=0;i<16;i++){
		tmp += ((src>>i)&1);
	}

	CPU_WORKCLOCK(2);
	out = reg16_b53[op];
	*out = (UINT16)tmp;

	CPU_FLAG &=  ~C_FLAG;
	CPU_FLAG &=  ~Z_FLAG;
	CPU_FLAG &=  ~S_FLAG;
	CPU_FLAG &=  ~O_FLAG;
	CPU_FLAG &=  ~A_FLAG;
	CPU_FLAG &=  ~P_FLAG;
	if (tmp == 0){CPU_FLAG |=  Z_FLAG;}
}

void SSE4_2_POPCNT_32(void)
{
	int i;

	UINT32 op;

	UINT32 *out;
	UINT32 dst, madr,tmp,src;
	
	if(!(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_POPCNT)){
		EXCEPTION(UD_EXCEPTION, 0);
		return;
	}

	tmp = 0;

	GET_PCBYTE((op));

	if (op >= 0xc0) {
		CPU_WORKCLOCK(21);
		src = *(reg32_b20[op]);
	} else {
		CPU_WORKCLOCK(24);
		madr = calc_ea_dst(op);
		src = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}

	for(i=0;i<32;i++){
		tmp += ((src>>i)&1);
	}

	CPU_WORKCLOCK(2);
	out = reg32_b53[op];
	*out = (UINT32)tmp;

	CPU_FLAG &=  ~C_FLAG;
	CPU_FLAG &=  ~Z_FLAG;
	CPU_FLAG &=  ~S_FLAG;
	CPU_FLAG &=  ~O_FLAG;
	CPU_FLAG &=  ~A_FLAG;
	CPU_FLAG &=  ~P_FLAG;
	if (tmp == 0){CPU_FLAG |=  Z_FLAG;}
}

#else

void SSE4_2_PCMPGTQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_PCMPESTRM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_PCMPESTRI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_PCMPISTRM(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_PCMPISTRI(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_CRC32_Gy_Eb(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_CRC32_Gy_Ev(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_CRC32_Gy_Eb_16(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_CRC32_Gy_Ev_16(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_POPCNT_16(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void SSE4_2_POPCNT_32(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

#endif
