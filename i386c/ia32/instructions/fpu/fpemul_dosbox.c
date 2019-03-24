/*
 *  Copyright (C) 2002-2015  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

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
 *   拡張倍精度浮動小数点ではなく倍精度浮動小数点で計算されるので実際のx87 FPUより精度が劣ります
 */

#include "compiler.h"

#if defined(USE_FPU) && defined(SUPPORT_FPU_DOSBOX)

#include <float.h>
#include <math.h>
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "ia32/instructions/fpu/fp.h"
#include "ia32/instructions/fpu/fpumem.h"
#ifdef USE_SSE
#include "ia32/instructions/sse/sse.h"
#endif

#if 1
#undef	TRACEOUT
#define	TRACEOUT(s)	(void)(s)
#endif	/* 0 */

#define FPU_WORKCLOCK	6

#define PI		3.14159265358979323846
#define L2E		1.4426950408889634
#define L2T		3.3219280948873623
#define LN2		0.69314718055994531
#define LG2		0.3010299956639812

static void FPU_FINIT(void);

static void
fpu_check_NM_EXCEPTION(){
	// タスクスイッチまたはエミュレーション時にNM(デバイス使用不可例外)を発生させる
	if ((CPU_CR0 & (CPU_CR0_TS)) || (CPU_CR0 & CPU_CR0_EM)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
}
static void
fpu_check_NM_EXCEPTION2(){
	// タスクスイッチまたはエミュレーション時にNM(デバイス使用不可例外)を発生させる
	if ((CPU_CR0 & (CPU_CR0_TS)) || (CPU_CR0 & CPU_CR0_EM)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

static const FPU_PTR zero_ptr = { 0, 0, 0 };

/*
 * FPU interface
 */
 
static INLINE UINT FPU_GET_TOP(void) {
	return (FPU_STATUSWORD & 0x3800)>>11;
}

static INLINE void FPU_SET_TOP(UINT val){
	FPU_STATUSWORD &= ~0x3800;
	FPU_STATUSWORD |= (val&7)<<11;
}


static INLINE void FPU_SET_C0(UINT C){
	FPU_STATUSWORD &= ~0x0100;
	if(C) FPU_STATUSWORD |=  0x0100;
}

static INLINE void FPU_SET_C1(UINT C){
	FPU_STATUSWORD &= ~0x0200;
	if(C) FPU_STATUSWORD |=  0x0200;
}

static INLINE void FPU_SET_C2(UINT C){
	FPU_STATUSWORD &= ~0x0400;
	if(C) FPU_STATUSWORD |=  0x0400;
}

static INLINE void FPU_SET_C3(UINT C){
	FPU_STATUSWORD &= ~0x4000;
	if(C) FPU_STATUSWORD |= 0x4000;
}

static INLINE void FPU_SET_D(UINT C){
	FPU_STATUSWORD &= ~0x0002;
	if(C) FPU_STATUSWORD |= 0x0002;
}

static INLINE void FPU_SetCW(UINT16 cword)
{
	// HACK: Bits 13-15 are not defined. Apparently, one program likes to test for
	// Cyrix EMC87 by trying to set bit 15. We want the test program to see
	// us as an Intel 287 when cputype == 286.
	cword &= 0x7FFF;
	FPU_CTRLWORD = cword;
	FPU_STAT.round = (FP_RND)((cword >> 10) & 3);
}

static void FPU_FLDCW(UINT32 addr)
{
	UINT16 temp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, addr);
	FPU_SetCW(temp);
}

static UINT16 FPU_GetTag(void)
{
	UINT i;
	
	UINT16 tag=0;
	for(i=0;i<8;i++)
		tag |= ( (FPU_STAT.tag[i]&3) <<(2*i));
	return tag;
}
static UINT8 FPU_GetTag8(void)
{
	UINT i;
	
	UINT8 tag=0;
	for(i=0;i<8;i++)
		tag |= ( (FPU_STAT.tag[i]==TAG_Empty ? 0 : 1) <<(i));
	return tag;
}

static INLINE void FPU_SetTag(UINT16 tag)
{
	UINT i;
	
	for(i=0;i<8;i++){
		FPU_STAT.tag[i] = (FP_TAG)((tag >>(2*i))&3);

	}
}
static INLINE void FPU_SetTag8(UINT8 tag)
{
	UINT i;
	
	for(i=0;i<8;i++){
		FPU_STAT.tag[i] = (((tag>>i)&1) == 0 ? TAG_Empty : TAG_Valid);
	}
}

static void FPU_FCLEX(void){
	//FPU_STATUSWORD &= 0xff00;			//should clear exceptions
	FPU_STATUSWORD &= 0x7f00;			//should clear exceptions?
}

static void FPU_FNOP(void){
	return;
}

static void FPU_PUSH(double in){
	FPU_STAT_TOP = (FPU_STAT_TOP - 1) & 7;
	//actually check if empty
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Valid;
	FPU_STAT.reg[FPU_STAT_TOP].d64 = in;
//	LOG(LOG_FPU,LOG_ERROR)("Pushed at %d  %g to the stack",newtop,in);
	return;
}

static void FPU_PREP_PUSH(void){
	FPU_STAT_TOP = (FPU_STAT_TOP - 1) & 7;
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Valid;
}

static void FPU_FPOP(void){
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Empty;
	//maybe set zero in it as well
	FPU_STAT_TOP = ((FPU_STAT_TOP+1)&7);
//	LOG(LOG_FPU,LOG_ERROR)("popped from %d  %g off the stack",top,fpu.regs[top].d64);
	return;
}

static double FROUND(double in){
	switch(FPU_STAT.round){
	case ROUND_Nearest:	
		if (in-floor(in)>0.5) return (floor(in)+1);
		else if (in-floor(in)<0.5) return (floor(in));
		else return ((((SINT64)(floor(in)))&1)!=0)?(floor(in)+1):(floor(in));
		break;
	case ROUND_Down:
		return (floor(in));
		break;
	case ROUND_Up:
		return (ceil(in));
		break;
	case ROUND_Chop:
		return in; //the cast afterwards will do it right maybe cast here
		break;
	default:
		return in;
		break;
	}
}

#define BIAS80 16383
#define BIAS64 1023

static void FPU_FLD80(UINT32 addr, UINT reg) 
{
	FP_REG result;
	SINT64 exp64, exp64final;
	SINT64 blah;
	SINT64 mant64;
	SINT64 sign;
	
	struct {
		SINT16 begin;
		FP_REG eind;
	} test;
	
	test.eind.l.lower = fpu_memoryread_d(addr);
	test.eind.l.upper = fpu_memoryread_d(addr+4);
	test.begin = fpu_memoryread_w(addr+8);
   
	exp64 = (((test.begin&0x7fff) - BIAS80));
	blah = ((exp64 >0)?exp64:-exp64)&0x3ff;
	exp64final = ((exp64 >0)?blah:-blah) +BIAS64;

	mant64 = (test.eind.ll >> 11) & QWORD_CONST(0xfffffffffffff);
	sign = (test.begin&0x8000)?1:0;
	result.ll = (sign <<63)|(exp64final << 52)| mant64;

	if(test.eind.l.lower == 0 && (UINT32)test.eind.l.upper == (UINT32)0x80000000UL && (test.begin&0x7fff) == 0x7fff) {
		//Detect INF and -INF (score 3.11 when drawing a slur.)
		result.d64 = sign?-HUGE_VAL:HUGE_VAL;
	}
	FPU_STAT.reg[reg].d64 = result.d64;
	//return result.d64;

	//mant64= test.mant80/2***64    * 2 **53 
}

static void FPU_ST80(UINT32 addr,UINT reg) 
{
	SINT64 sign80;
	SINT64 exp80, exp80final;
	SINT64 mant80, mant80final;
	
	struct {
		SINT16 begin;
		FP_REG eind;
	} test;
	
	sign80 = (FPU_STAT.reg[reg].ll&QWORD_CONST(0x8000000000000000))?1:0;
	exp80 =  FPU_STAT.reg[reg].ll&QWORD_CONST(0x7ff0000000000000);
	exp80final = (exp80>>52);
	mant80 = FPU_STAT.reg[reg].ll&QWORD_CONST(0x000fffffffffffff);
	mant80final = (mant80 << 11);
	if(FPU_STAT.reg[reg].d64 != 0){ //Zero is a special case
		// Elvira wants the 8 and tcalc doesn't
		mant80final |= QWORD_CONST(0x8000000000000000);
		//Ca-cyber doesn't like this when result is zero.
		exp80final += (BIAS80 - BIAS64);
	}
	test.begin = ((SINT16)(sign80)<<15)| (SINT16)(exp80final);
	test.eind.ll = mant80final;
	fpu_memorywrite_d(addr,test.eind.l.lower);
	fpu_memorywrite_d(addr+4,test.eind.l.upper);
	fpu_memorywrite_w(addr+8,test.begin);
}


static void FPU_FLD_F32(UINT32 addr,UINT store_to) {
	union {
		float f;
		UINT32 l;
	}	blah;
	
	blah.l = fpu_memoryread_d(addr);
	FPU_STAT.reg[store_to].d64 = (double)(blah.f);
}

static void FPU_FLD_F64(UINT32 addr,UINT store_to) {
	FPU_STAT.reg[store_to].l.lower = fpu_memoryread_d(addr);
	FPU_STAT.reg[store_to].l.upper = fpu_memoryread_d(addr+4);
}

static void FPU_FLD_F80(UINT32 addr) {
	FPU_FLD80(addr, FPU_STAT_TOP);
}

static void FPU_FLD_I16(UINT32 addr,UINT store_to) {
	SINT16 blah;

	blah = fpu_memoryread_w(addr);
	FPU_STAT.reg[store_to].d64 = (double)(blah);
}

static void FPU_FLD_I32(UINT32 addr,UINT store_to) {
	SINT32 blah;

	blah = fpu_memoryread_d(addr);
	FPU_STAT.reg[store_to].d64 = (double)(blah);
}

static void FPU_FLD_I64(UINT32 addr,UINT store_to) {
	FP_REG blah;
	
	blah.l.lower = fpu_memoryread_d(addr);
	blah.l.upper = fpu_memoryread_d(addr+4);
	FPU_STAT.reg[store_to].d64 = (double)(blah.ll);
}

static void FPU_FBLD(UINT32 addr,UINT store_to) 
{
	UINT i;
	double temp;
	
	UINT64 val = 0;
	UINT in = 0;
	UINT64 base = 1;
	for(i = 0;i < 9;i++){
		in = fpu_memoryread_b(addr + i);
		val += ( (in&0xf) * base); //in&0xf shouldn't be higher then 9
		base *= 10;
		val += ((( in>>4)&0xf) * base);
		base *= 10;
	}

	//last number, only now convert to float in order to get
	//the best signification
	temp = (double)(val);
	in = fpu_memoryread_b(addr + 9);
	temp += ( (in&0xf) * base );
	if(in&0x80) temp *= -1.0;
	FPU_STAT.reg[store_to].d64 = temp;
}


static INLINE void FPU_FLD_F32_EA(UINT32 addr) {
	FPU_FLD_F32(addr,8);
}
static INLINE void FPU_FLD_F64_EA(UINT32 addr) {
	FPU_FLD_F64(addr,8);
}
static INLINE void FPU_FLD_I32_EA(UINT32 addr) {
	FPU_FLD_I32(addr,8);
}
static INLINE void FPU_FLD_I16_EA(UINT32 addr) {
	FPU_FLD_I16(addr,8);
}

static void FPU_FST_F32(UINT32 addr) {
	union {
		float f;
		UINT32 l;
	}	blah;
	
	//should depend on rounding method
	blah.f = (float)(FPU_STAT.reg[FPU_STAT_TOP].d64);
	fpu_memorywrite_d(addr,blah.l);
}

static void FPU_FST_F64(UINT32 addr) {
	fpu_memorywrite_d(addr,FPU_STAT.reg[FPU_STAT_TOP].l.lower);
	fpu_memorywrite_d(addr+4,FPU_STAT.reg[FPU_STAT_TOP].l.upper);
}

static void FPU_FST_F80(UINT32 addr) {
	FPU_ST80(addr,FPU_STAT_TOP);
}

static void FPU_FST_I16(UINT32 addr) {
	fpu_memorywrite_w(addr,(SINT16)(FROUND(FPU_STAT.reg[FPU_STAT_TOP].d64)));
}

static void FPU_FST_I32(UINT32 addr) {
	fpu_memorywrite_d(addr,(SINT32)(FROUND(FPU_STAT.reg[FPU_STAT_TOP].d64)));
}

static void FPU_FST_I64(UINT32 addr) {
	FP_REG blah;
	
	blah.ll = (SINT64)(FROUND(FPU_STAT.reg[FPU_STAT_TOP].d64));
	fpu_memorywrite_d(addr,blah.l.lower);
	fpu_memorywrite_d(addr+4,blah.l.upper);
}

static void FPU_FBST(UINT32 addr) 
{
	FP_REG val;
	UINT p;
	UINT i;
	BOOL sign;
	double temp;
	
	val = FPU_STAT.reg[FPU_STAT_TOP];
	sign = FALSE;
	if(FPU_STAT.reg[FPU_STAT_TOP].ll & QWORD_CONST(0x8000000000000000)) { //sign
		sign=TRUE;
		val.d64=-val.d64;
	}
	//numbers from back to front
	temp=val.d64;
	for(i=0;i<9;i++){
		val.d64=temp;
		temp = (double)((SINT64)(floor(val.d64/10.0)));
		p = (UINT)(val.d64 - 10.0*temp);  
		val.d64=temp;
		temp = (double)((SINT64)(floor(val.d64/10.0)));
		p |= ((UINT)(val.d64 - 10.0*temp)<<4);

		fpu_memorywrite_b(addr+i,p);
	}
	val.d64=temp;
	temp = (double)((SINT64)(floor(val.d64/10.0)));
	p = (UINT)(val.d64 - 10.0*temp);
	if(sign)
		p|=0x80;
	fpu_memorywrite_b(addr+9,p);
}

#if defined(_WIN32) && !defined(__LIBRETRO__)
#define isinf(x) (!(_finite(x) || _isnan(x)))
#else
#define isinf(x) (!(_finite(x) || isnan(x)))
#endif
#define isdenormal(x) (_fpclass(x) == _FPCLASS_ND || _fpclass(x) == _FPCLASS_PD)

static void FPU_FADD(UINT op1, UINT op2){
	//// HACK: Set the denormal flag according to whether the source or final result is a denormalized number.
	////       This is vital if we don't want certain DOS programs to mis-detect our FPU emulation as an IIT clone chip when cputype == 286
	//BOOL was_not_normal;

	//was_not_normal = isdenormal(FPU_STAT.reg[op1].d64);
	//FPU_STAT.reg[op1].d64 += FPU_STAT.reg[op2].d64;
	//FPU_SET_D(was_not_normal || isdenormal(FPU_STAT.reg[op1].d64) || isdenormal(FPU_STAT.reg[op2].d64));
	FPU_STAT.reg[op1].d64 += FPU_STAT.reg[op2].d64;
	//flags and such :)
	return;
}

static void FPU_FSIN(void){
	FPU_STAT.reg[FPU_STAT_TOP].d64 = sin(FPU_STAT.reg[FPU_STAT_TOP].d64);
	FPU_SET_C2(0);
	//flags and such :)
	return;
}

static void FPU_FSINCOS(void){
	double temp;

	temp = FPU_STAT.reg[FPU_STAT_TOP].d64;
	FPU_STAT.reg[FPU_STAT_TOP].d64 = sin(temp);
	FPU_PUSH(cos(temp));
	FPU_SET_C2(0);
	//flags and such :)
	return;
}

static void FPU_FCOS(void){
	FPU_STAT.reg[FPU_STAT_TOP].d64 = cos(FPU_STAT.reg[FPU_STAT_TOP].d64);
	FPU_SET_C2(0);
	//flags and such :)
	return;
}

static void FPU_FSQRT(void){
	FPU_STAT.reg[FPU_STAT_TOP].d64 = sqrt(FPU_STAT.reg[FPU_STAT_TOP].d64);
	//flags and such :)
	return;
}
static void FPU_FPATAN(void){
	FPU_STAT.reg[FPU_ST(1)].d64 = atan2(FPU_STAT.reg[FPU_ST(1)].d64,FPU_STAT.reg[FPU_STAT_TOP].d64);
	FPU_FPOP();
	//flags and such :)
	return;
}
static void FPU_FPTAN(void){
	FPU_STAT.reg[FPU_STAT_TOP].d64 = tan(FPU_STAT.reg[FPU_STAT_TOP].d64);
	FPU_PUSH(1.0);
	FPU_SET_C2(0);
	//flags and such :)
	return;
}
static void FPU_FDIV(UINT st, UINT other){
	if(FPU_STAT.reg[other].d64==0){
		FPU_STATUSWORD |= FP_ZE_FLAG;
		if(!(FPU_CTRLWORD & FP_ZE_FLAG))
			return;
	}
	FPU_STAT.reg[st].d64= FPU_STAT.reg[st].d64/FPU_STAT.reg[other].d64;
	//flags and such :)
	return;
}

static void FPU_FDIVR(UINT st, UINT other){
	if(FPU_STAT.reg[st].d64==0){
		FPU_STATUSWORD |= FP_ZE_FLAG;
		if(!(FPU_CTRLWORD & FP_ZE_FLAG))
			return;
	}
	FPU_STAT.reg[st].d64= FPU_STAT.reg[other].d64/FPU_STAT.reg[st].d64;
	// flags and such :)
	return;
}

static void FPU_FMUL(UINT st, UINT other){
	FPU_STAT.reg[st].d64*=FPU_STAT.reg[other].d64;
	//flags and such :)
	return;
}

static void FPU_FSUB(UINT st, UINT other){
	FPU_STAT.reg[st].d64 = FPU_STAT.reg[st].d64 - FPU_STAT.reg[other].d64;
	//flags and such :)
	return;
}

static void FPU_FSUBR(UINT st, UINT other){
	FPU_STAT.reg[st].d64 = FPU_STAT.reg[other].d64 - FPU_STAT.reg[st].d64;
	//flags and such :)
	return;
}

static void FPU_FXCH(UINT st, UINT other){
	FP_TAG tag; 
	FP_REG reg;

	tag = FPU_STAT.tag[other];
	reg = FPU_STAT.reg[other];
	FPU_STAT.tag[other] = FPU_STAT.tag[st];
	FPU_STAT.reg[other] = FPU_STAT.reg[st];
	FPU_STAT.tag[st] = tag;
	FPU_STAT.reg[st] = reg;
}

static void FPU_FST(UINT st, UINT other){
	FPU_STAT.tag[other] = FPU_STAT.tag[st];
	FPU_STAT.reg[other] = FPU_STAT.reg[st];
}

static void FPU_FCOM(UINT st, UINT other){
	if(((FPU_STAT.tag[st] != TAG_Valid) && (FPU_STAT.tag[st] != TAG_Zero)) || 
		((FPU_STAT.tag[other] != TAG_Valid) && (FPU_STAT.tag[other] != TAG_Zero))){
		FPU_SET_C3(1);
		FPU_SET_C2(1);
		FPU_SET_C0(1);
		return;
	}

	if(FPU_STAT.reg[st].d64 == FPU_STAT.reg[other].d64){
		FPU_SET_C3(1);
		FPU_SET_C2(0);
		FPU_SET_C0(0);
		return;
	}
	if(FPU_STAT.reg[st].d64 < FPU_STAT.reg[other].d64){
		FPU_SET_C3(0);
		FPU_SET_C2(0);
		FPU_SET_C0(1);
		return;
	}
	// st > other
	FPU_SET_C3(0);
	FPU_SET_C2(0);
	FPU_SET_C0(0);
	return;
}
static void FPU_FCOMI(UINT st, UINT other){
	if(((FPU_STAT.tag[st] != TAG_Valid) && (FPU_STAT.tag[st] != TAG_Zero)) || 
		((FPU_STAT.tag[other] != TAG_Valid) && (FPU_STAT.tag[other] != TAG_Zero))){
		CPU_FLAGL = (CPU_FLAGL & ~Z_FLAG) | Z_FLAG;
		CPU_FLAGL = (CPU_FLAGL & ~P_FLAG) | P_FLAG;
		CPU_FLAGL = (CPU_FLAGL & ~C_FLAG) | C_FLAG;
		return;
	}

	if(FPU_STAT.reg[st].d64 == FPU_STAT.reg[other].d64){
		CPU_FLAGL = (CPU_FLAGL & ~Z_FLAG) | Z_FLAG;
		CPU_FLAGL = (CPU_FLAGL & ~P_FLAG) | 0;
		CPU_FLAGL = (CPU_FLAGL & ~C_FLAG) | 0;
		return;
	}
	if(FPU_STAT.reg[st].d64 < FPU_STAT.reg[other].d64){
		CPU_FLAGL = (CPU_FLAGL & ~Z_FLAG) | 0;
		CPU_FLAGL = (CPU_FLAGL & ~P_FLAG) | 0;
		CPU_FLAGL = (CPU_FLAGL & ~C_FLAG) | C_FLAG;
		return;
	}
	// st > other
	CPU_FLAGL = (CPU_FLAGL & ~Z_FLAG) | 0;
	CPU_FLAGL = (CPU_FLAGL & ~P_FLAG) | 0;
	CPU_FLAGL = (CPU_FLAGL & ~C_FLAG) | 0;
	return;
}

static void FPU_FUCOM(UINT st, UINT other){
	//does atm the same as fcom 
	FPU_FCOM(st,other);
}
static void FPU_FUCOMI(UINT st, UINT other){
	//does atm the same as fcomi 
	FPU_FCOMI(st,other);
}

static void FPU_FCMOVB(UINT st, UINT other){
	if(CPU_FLAGL & C_FLAG){
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVE(UINT st, UINT other){
	if(CPU_FLAGL & Z_FLAG){
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVBE(UINT st, UINT other){
	if(CPU_FLAGL & (C_FLAG|Z_FLAG)){
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVU(UINT st, UINT other){
	if(CPU_FLAGL & P_FLAG){
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}

static void FPU_FCMOVNB(UINT st, UINT other){
	if(!(CPU_FLAGL & C_FLAG)){
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVNE(UINT st, UINT other){
	if(!(CPU_FLAGL & Z_FLAG)){
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVNBE(UINT st, UINT other){
	if(!(CPU_FLAGL & (C_FLAG|Z_FLAG))){
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVNU(UINT st, UINT other){
	if(!(CPU_FLAGL & P_FLAG)){
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}

static void FPU_FRNDINT(void){
	SINT64 temp;

	temp = (SINT64)(FROUND(FPU_STAT.reg[FPU_STAT_TOP].d64));
	FPU_STAT.reg[FPU_STAT_TOP].d64=(double)(temp);
}

static void FPU_FPREM(void){
	double valtop;
	double valdiv;
	SINT64 ressaved;

	valtop = FPU_STAT.reg[FPU_STAT_TOP].d64;
	valdiv = FPU_STAT.reg[FPU_ST(1)].d64;
	ressaved = (SINT64)( (valtop/valdiv) );
// Some backups
//	Real64 res=valtop - ressaved*valdiv; 
//      res= fmod(valtop,valdiv);
	FPU_STAT.reg[FPU_STAT_TOP].d64 = valtop - ressaved*valdiv;
	FPU_SET_C0((UINT)(ressaved&4));
	FPU_SET_C3((UINT)(ressaved&2));
	FPU_SET_C1((UINT)(ressaved&1));
	FPU_SET_C2(0);
}

static void FPU_FPREM1(void){
	double valtop;
	double valdiv, quot, quotf;
	SINT64 ressaved;
	
	valtop = FPU_STAT.reg[FPU_STAT_TOP].d64;
	valdiv = FPU_STAT.reg[FPU_ST(1)].d64;
	quot = valtop/valdiv;
	quotf = floor(quot);
	
	if (quot-quotf>0.5) ressaved = (SINT64)(quotf+1);
	else if (quot-quotf<0.5) ressaved = (SINT64)(quotf);
	else ressaved = (SINT64)(((((SINT64)(quotf))&1)!=0)?(quotf+1):(quotf));
	
	FPU_STAT.reg[FPU_STAT_TOP].d64 = valtop - ressaved*valdiv;
	FPU_SET_C0((UINT)(ressaved&4));
	FPU_SET_C3((UINT)(ressaved&2));
	FPU_SET_C1((UINT)(ressaved&1));
	FPU_SET_C2(0);
}

static void FPU_FXAM(void){
	if(FPU_STAT.reg[FPU_STAT_TOP].ll & QWORD_CONST(0x8000000000000000))	//sign
	{ 
		FPU_SET_C1(1);
	} 
	else 
	{
		FPU_SET_C1(0);
	}
	if(FPU_STAT.tag[FPU_STAT_TOP] == TAG_Empty)
	{
		FPU_SET_C3(1);FPU_SET_C2(0);FPU_SET_C0(1);
		return;
	}
	if(FPU_STAT.reg[FPU_STAT_TOP].d64 == 0.0)		//zero or normalized number.
	{ 
		FPU_SET_C3(1);FPU_SET_C2(0);FPU_SET_C0(0);
	}
	else
	{
		FPU_SET_C3(0);FPU_SET_C2(1);FPU_SET_C0(0);
	}
}

static void FPU_F2XM1(void){
	FPU_STAT.reg[FPU_STAT_TOP].d64 = pow(2.0,FPU_STAT.reg[FPU_STAT_TOP].d64) - 1;
	return;
}

static void FPU_FYL2X(void){
	FPU_STAT.reg[FPU_ST(1)].d64*=log(FPU_STAT.reg[FPU_STAT_TOP].d64)/log((double)(2.0));
	FPU_FPOP();
	return;
}

static void FPU_FYL2XP1(void){
	FPU_STAT.reg[FPU_ST(1)].d64*=log(FPU_STAT.reg[FPU_STAT_TOP].d64+1.0)/log((double)(2.0));
	FPU_FPOP();
	return;
}

static void FPU_FSCALE(void){
	FPU_STAT.reg[FPU_STAT_TOP].d64 *= pow(2.0,(double)((SINT64)(FPU_STAT.reg[FPU_ST(1)].d64)));
	return; //2^x where x is chopped.
}

static void FPU_FSTENV(UINT32 addr)
{
	descriptor_t *sdp = &CPU_CS_DESC;	
	FPU_SET_TOP(FPU_STAT_TOP);
	
	switch ((CPU_CR0 & 1) | (SEG_IS_32BIT(sdp) ? 0x100 : 0x000))
	{
	case 0x000: case 0x001:
		fpu_memorywrite_w(addr+0,FPU_CTRLWORD);
		fpu_memorywrite_w(addr+2,FPU_STATUSWORD);
		fpu_memorywrite_w(addr+4,FPU_GetTag());
		fpu_memorywrite_w(addr+10,FPU_LASTINSTOP);
		break;
		
	case 0x100: case 0x101:
		fpu_memorywrite_d(addr+0,(UINT32)(FPU_CTRLWORD));
		fpu_memorywrite_d(addr+4,(UINT32)(FPU_STATUSWORD));
		fpu_memorywrite_d(addr+8,(UINT32)(FPU_GetTag()));	
		fpu_memorywrite_d(addr+20,FPU_LASTINSTOP);			
		break;
	}
}

static void FPU_FLDENV(UINT32 addr)
{
	descriptor_t *sdp = &CPU_CS_DESC;	
	
	switch ((CPU_CR0 & 1) | (SEG_IS_32BIT(sdp) ? 0x100 : 0x000)) {
	case 0x000: case 0x001:
		FPU_SetCW(fpu_memoryread_w(addr+0));
		FPU_STATUSWORD = fpu_memoryread_w(addr+2);
		FPU_SetTag(fpu_memoryread_w(addr+4));
		FPU_LASTINSTOP = fpu_memoryread_w(addr+10);
		break;
		
	case 0x100: case 0x101:
		FPU_SetCW((UINT16)fpu_memoryread_d(addr+0));
		FPU_STATUSWORD = (UINT16)fpu_memoryread_d(addr+4);
		FPU_SetTag((UINT16)fpu_memoryread_d(addr+8));
		FPU_LASTINSTOP = (UINT16)fpu_memoryread_d(addr+20);		
		break;
	}
	FPU_STAT_TOP = FPU_GET_TOP();
}

static void FPU_FSAVE(UINT32 addr)
{
	UINT start;
	UINT i;
	
	descriptor_t *sdp = &CPU_CS_DESC;
	
	FPU_FSTENV(addr);
	start = ((SEG_IS_32BIT(sdp))?28:14);
	for(i = 0;i < 8;i++){
		FPU_ST80(addr+start,FPU_ST(i));
		start += 10;
	}
	FPU_FINIT();
}

static void FPU_FRSTOR(UINT32 addr)
{
	UINT start;
	UINT i;
	
	descriptor_t *sdp = &CPU_CS_DESC;
	
	FPU_FLDENV(addr);
	start = ((SEG_IS_32BIT(sdp))?28:14);
	for(i = 0;i < 8;i++){
		FPU_FLD80(addr+start, FPU_ST(i));
		start += 10;
	}
}

static void FPU_FXSAVE(UINT32 addr){
	UINT start;
	UINT i;
	
	descriptor_t *sdp = &CPU_CS_DESC;
	
	//FPU_FSTENV(addr);
	FPU_SET_TOP(FPU_STAT_TOP);
	fpu_memorywrite_w(addr+0,FPU_CTRLWORD);
	fpu_memorywrite_w(addr+2,FPU_STATUSWORD);
	fpu_memorywrite_b(addr+4,FPU_GetTag8());
#ifdef USE_SSE
	fpu_memorywrite_d(addr+24,SSE_MXCSR);
#endif
	start = 32;
	for(i = 0;i < 8;i++){
		//FPU_ST80(addr+start,FPU_ST(i));
		fpu_memorywrite_d(addr+start+0,FPU_STAT.reg[FPU_ST(i)].l.lower);
		fpu_memorywrite_d(addr+start+4,FPU_STAT.reg[FPU_ST(i)].l.upper);
		fpu_memorywrite_d(addr+start+8,0x0000ffff);
		fpu_memorywrite_d(addr+start+12,0x00000000);
		start += 16;
	}
#ifdef USE_SSE
	start = 160;
	for(i = 0;i < 8;i++){
		fpu_memorywrite_q(addr+start+0,SSE_XMMREG(i).ul64[0]);
		fpu_memorywrite_q(addr+start+8,SSE_XMMREG(i).ul64[1]);
		start += 16;
	}
#endif
}
static void FPU_FXRSTOR(UINT32 addr){
	UINT start;
	UINT i;
	
	descriptor_t *sdp = &CPU_CS_DESC;
	
	//FPU_FLDENV(addr);
	FPU_SetCW(fpu_memoryread_w(addr+0));
	FPU_STATUSWORD = fpu_memoryread_w(addr+2);
	FPU_SetTag8(fpu_memoryread_b(addr+4));
	FPU_STAT_TOP = FPU_GET_TOP();
#ifdef USE_SSE
	SSE_MXCSR = fpu_memoryread_d(addr+24);
#endif
	start = 32;
	for(i = 0;i < 8;i++){
		//FPU_STAT.reg[FPU_ST(i)].d64 = FPU_FLD80(addr+start);
		FPU_STAT.reg[FPU_ST(i)].l.lower = fpu_memoryread_d(addr+start+0);
		FPU_STAT.reg[FPU_ST(i)].l.upper = fpu_memoryread_d(addr+start+4);
		start += 16;
	}
#ifdef USE_SSE
	start = 160;
	for(i = 0;i < 8;i++){
		SSE_XMMREG(i).ul64[0] = fpu_memoryread_q(addr+start+0);
		SSE_XMMREG(i).ul64[1] = fpu_memoryread_q(addr+start+8);
		start += 16;
	}
#endif
}

void DB_FPU_FXSAVERSTOR(void){
	UINT32 op;
	UINT idx, sub;
	UINT32 maddr;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION2(); // XXX: 根拠無し
	switch(idx){
	case 0: // FXSAVE
		maddr = calc_ea_dst(op);
		FPU_FXSAVE(maddr);
		break;
	case 1: // FXRSTOR
		maddr = calc_ea_dst(op);
		FPU_FXRSTOR(maddr);
		break;
#ifdef USE_SSE
	case 2: // LDMXCSR
		maddr = calc_ea_dst(op);
		SSE_LDMXCSR(maddr);
		break;
	case 3: // STMXCSR
		maddr = calc_ea_dst(op);
		SSE_STMXCSR(maddr);
		break;
	case 4: // SFENCE
		SSE_SFENCE();
		break;
	case 5: // LFENCE
		SSE_LFENCE();
		break;
	case 6: // MFENCE
		SSE_MFENCE();
		break;
	case 7: // CLFLUSH;
		SSE_CLFLUSH(op);
		break;
#endif
	default:
		ia32_panic("invalid opcode = %02x\n", op);
		break;
	}
}

static void FPU_FXTRACT(void) {
	// function stores real bias in st and 
	// pushes the significant number onto the stack
	// if double ever uses a different base please correct this function
	FP_REG test;
	SINT64 exp80, exp80final;
	double mant;
	
	test = FPU_STAT.reg[FPU_STAT_TOP];
	exp80 =  test.ll&QWORD_CONST(0x7ff0000000000000);
	exp80final = (exp80>>52) - BIAS64;
	mant = test.d64 / (pow(2.0,(double)(exp80final)));
	FPU_STAT.reg[FPU_STAT_TOP].d64 = (double)(exp80final);
	FPU_PUSH(mant);
}

static void FPU_FCHS(void){
	FPU_STAT.reg[FPU_STAT_TOP].d64 = -1.0*(FPU_STAT.reg[FPU_STAT_TOP].d64);
}

static void FPU_FABS(void){
	FPU_STAT.reg[FPU_STAT_TOP].d64 = fabs(FPU_STAT.reg[FPU_STAT_TOP].d64);
}

static void FPU_FTST(void){
	FPU_STAT.reg[8].d64 = 0.0;
	FPU_FCOM(FPU_STAT_TOP,8);
}

static void FPU_FLD1(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d64 = 1.0;
}

static void FPU_FLDL2T(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d64 = L2T;
}

static void FPU_FLDL2E(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d64 = L2E;
}

static void FPU_FLDPI(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d64 = PI;
}

static void FPU_FLDLG2(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d64 = LG2;
}

static void FPU_FLDLN2(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d64 = LN2;
}

static void FPU_FLDZ(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d64 = 0.0;
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Zero;
}


static INLINE void FPU_FADD_EA(UINT op1){
	FPU_FADD(op1,8);
}
static INLINE void FPU_FMUL_EA(UINT op1){
	FPU_FMUL(op1,8);
}
static INLINE void FPU_FSUB_EA(UINT op1){
	FPU_FSUB(op1,8);
}
static INLINE void FPU_FSUBR_EA(UINT op1){
	FPU_FSUBR(op1,8);
}
static INLINE void FPU_FDIV_EA(UINT op1){
	FPU_FDIV(op1,8);
}
static INLINE void FPU_FDIVR_EA(UINT op1){
	FPU_FDIVR(op1,8);
}
static INLINE void FPU_FCOM_EA(UINT op1){
	FPU_FCOM(op1,8);
} 
 
/*
 * FPU interface
 */
//int fpu_updateEmuEnv(void);
static void
FPU_FINIT(void)
{
	int i;
	FPU_SetCW(0x37F);
	FPU_STATUSWORD = 0;
	FPU_STAT_TOP=FPU_GET_TOP();
	for(i=0;i<8;i++){
		FPU_STAT.tag[i] = TAG_Empty;
		// レジスタの内容は消してはいけない ver0.86 rev40
		//FPU_STAT.reg[i].d64 = 0;
		//FPU_STAT.reg[i].l.lower = 0;
		//FPU_STAT.reg[i].l.upper = 0;
		//FPU_STAT.reg[i].ll = 0;
	}
	FPU_STAT.tag[8] = TAG_Valid; // is only used by us
}
void DB_FPU_FINIT(void){
	int i;
	FPU_FINIT();
	for(i=0;i<8;i++){
		FPU_STAT.tag[i] = TAG_Empty;
		FPU_STAT.reg[i].l.ext = 0;
		FPU_STAT.reg[i].l.lower = 0;
		FPU_STAT.reg[i].l.upper = 0;
	}
}

/*
 * FPU instruction
 */

static void fpu_checkexception(){
	if((FPU_STATUSWORD & ~FPU_CTRLWORD) & 0x3F){
		EXCEPTION(MF_EXCEPTION, 0);
	}
}

static void EA_TREE(UINT op)
{
	UINT idx;

	idx = (op >> 3) & 7;
	
		switch (idx) {
		case 0:	/* FADD (単精度実数) */
			TRACEOUT(("FADD EA"));
			FPU_FADD_EA(FPU_STAT_TOP);
			break;
		case 1:	/* FMUL (単精度実数) */
			TRACEOUT(("FMUL EA"));
			FPU_FMUL_EA(FPU_STAT_TOP);
			break;
		case 2:	/* FCOM (単精度実数) */
			TRACEOUT(("FCOM EA"));
			FPU_FCOM_EA(FPU_STAT_TOP);
			break;
		case 3:	/* FCOMP (単精度実数) */
			TRACEOUT(("FCOMP EA"));
			FPU_FCOM_EA(FPU_STAT_TOP);
			FPU_FPOP();
			break;
		case 4:	/* FSUB (単精度実数) */
			TRACEOUT(("FSUB EA"));
			FPU_FSUB_EA(FPU_STAT_TOP);
			break;
		case 5:	/* FSUBR (単精度実数) */
			TRACEOUT(("FSUBR EA"));
			FPU_FSUBR_EA(FPU_STAT_TOP);
			break;
		case 6:	/* FDIV (単精度実数) */
			TRACEOUT(("FDIV EA"));
			FPU_FDIV_EA(FPU_STAT_TOP);
			break;
		case 7:	/* FDIVR (単精度実数) */
			TRACEOUT(("FDIVR EA"));
			FPU_FDIVR_EA(FPU_STAT_TOP);
			break;
		default:
			break;
		}	
}

// d8
void
DB_ESC0(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU d8 %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	fpu_checkexception();
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0:	/* FADD */
			TRACEOUT(("FADD"));
			FPU_FADD(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 1:	/* FMUL */
			TRACEOUT(("FMUL"));
			FPU_FMUL(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2:	/* FCOM */
			TRACEOUT(("FCOM"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3:	/* FCOMP */
			TRACEOUT(("FCOMP"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;
		case 4:	/* FSUB */
			TRACEOUT(("FSUB"));
			FPU_FSUB(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 5:	/* FSUBR */
			TRACEOUT(("FSUBR"));
			FPU_FSUBR(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 6:	/* FDIV */
			TRACEOUT(("FDIV"));
			FPU_FDIV(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 7:	/* FDIVR */
			TRACEOUT(("FDIVR"));
			FPU_FDIVR(FPU_STAT_TOP,FPU_ST(sub));
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_F32_EA(madr);
		EA_TREE(op);
	}
}

// d9
void
DB_ESC1(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU d9 %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	if(!(op < 0xc0 && idx>=4)){
		fpu_checkexception();
	}
	if (op >= 0xc0) 
	{
		switch (idx) {
		case 0: /* FLD ST(0), ST(i) */
			{
				UINT reg_from;
			
				TRACEOUT(("FLD STi"));
				reg_from = FPU_ST(sub);
				FPU_PREP_PUSH();
				FPU_FST(reg_from, FPU_STAT_TOP);
			}
			break;

		case 1:	/* FXCH ST(0), ST(i) */
			TRACEOUT(("FXCH STi"));
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;

		case 2: /* FNOP */
			TRACEOUT(("FNOP"));
			FPU_FNOP();
			break;

		case 3: /* FSTP STi */
			TRACEOUT(("FSTP STi"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;
		
		case 4:
			switch (sub) {
			case 0x0:	/* FCHS */
				TRACEOUT(("FCHS"));
				FPU_FCHS();
				break;

			case 0x1:	/* FABS */
				TRACEOUT(("FABS"));
				FPU_FABS();
				break;

			case 0x2:  /* UNKNOWN */
			case 0x3:  /* ILLEGAL */
				break;

			case 0x4:	/* FTST */
				TRACEOUT(("FTST"));
				FPU_FTST();
				break;

			case 0x5:	/* FXAM */
				TRACEOUT(("FXAM"));
				FPU_FXAM();
				break;

			case 0x06:       /* FTSTP (cyrix)*/
			case 0x07:       /* UNKNOWN */
				break;
			}
			break;
			
		case 5:
			switch (sub) {
			case 0x0:	/* FLD1 */
				TRACEOUT(("FLD1"));
				FPU_FLD1();
				break;
				
			case 0x1:	/* FLDL2T */
				TRACEOUT(("FLDL2T"));
				FPU_FLDL2T();
				break;
				
			case 0x2:	/* FLDL2E */
				TRACEOUT(("FLDL2E"));
				FPU_FLDL2E();
				break;
				
			case 0x3:	/* FLDPI */
				TRACEOUT(("FLDPI"));
				FPU_FLDPI();
				break;
				
			case 0x4:	/* FLDLG2 */
				TRACEOUT(("FLDLG2"));
				FPU_FLDLG2();
				break;
				
			case 0x5:	/* FLDLN2 */
				TRACEOUT(("FLDLN2"));
				FPU_FLDLN2();
				break;
				
			case 0x6:	/* FLDZ */
				TRACEOUT(("FLDZ"));
				FPU_FLDZ();
				break;
				
			case 0x07: /* ILLEGAL */
				break;
			}
			break;

		case 6:
			switch (sub) {
			case 0x0:	/* F2XM1 */
				TRACEOUT(("F2XM1"));
				FPU_F2XM1();
				break;
				
			case 0x1:	/* FYL2X */
				TRACEOUT(("FYL2X"));
				FPU_FYL2X();
				break;
				
			case 0x2:	/* FPTAN */
				TRACEOUT(("FPTAN"));
				FPU_FPTAN();
				break;
				
			case 0x3:	/* FPATAN */
				TRACEOUT(("FPATAN"));
				FPU_FPATAN();
				break;
				
			case 0x4:	/* FXTRACT */
				TRACEOUT(("FXTRACT"));
				FPU_FXTRACT();
				break;
				
			case 0x5:	/* FPREM1 */
				TRACEOUT(("FPREM1"));
				FPU_FPREM1();
				break;
				
			case 0x6:	/* FDECSTP */
				TRACEOUT(("FDECSTP"));
				FPU_STAT_TOP = (FPU_STAT_TOP - 1) & 7;
				break;
				
			case 0x7:	/* FINCSTP */
				TRACEOUT(("FINCSTP"));
				FPU_STAT_TOP = (FPU_STAT_TOP + 1) & 7;
				break;
			}
			break;
			
		case 7:
			switch (sub) {
			case 0x0:	/* FPREM */
				TRACEOUT(("FPREM"));
				FPU_FPREM();
				break;
				
			case 0x1:	/* FYL2XP1 */
				TRACEOUT(("FYL2XP1"));
				FPU_FYL2XP1();
				break;
				
			case 0x2:	/* FSQRT */
				TRACEOUT(("FSQRT"));
				FPU_FSQRT();
				break;
				
			case 0x3:	/* FSINCOS */
				TRACEOUT(("FSINCOS"));
				FPU_FSINCOS();
				break;
				
			case 0x4:	/* FRNDINT */
				TRACEOUT(("FRNDINT"));
				FPU_FRNDINT();
				break;
				
			case 0x5:	/* FSCALE */
				TRACEOUT(("FSCALE"));
				FPU_FSCALE();
				break;
				
			case 0x6:	/* FSIN */
				TRACEOUT(("FSIN"));
				FPU_FSIN();				
				break;
				
			case 0x7:	/* FCOS */
				TRACEOUT(("FCOS"));
				FPU_FCOS();	
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
			TRACEOUT(("FLD float"));
			FPU_PREP_PUSH();
			FPU_FLD_F32(madr,FPU_STAT_TOP);
			break;
			
		case 1:	/* UNKNOWN */
			break;

		case 2:	/* FST (単精度実数) */
			TRACEOUT(("FST float"));
			FPU_FST_F32(madr);			
			break;

		case 3:	/* FSTP (単精度実数) */
			TRACEOUT(("FSTP float"));
			FPU_FST_F32(madr);
			FPU_FPOP();
			break;

		case 4:	/* FLDENV */
			TRACEOUT(("FLDENV"));
			FPU_FLDENV(madr);		
			break;

		case 5:	/* FLDCW */
			TRACEOUT(("FLDCW"));
			FPU_FLDCW(madr);
			break;

		case 6:	/* FSTENV */
			TRACEOUT(("FSTENV"));
			FPU_FSTENV(madr);
			break;

		case 7:	/* FSTCW */
			TRACEOUT(("FSTCW/FNSTCW"));
			fpu_memorywrite_w(madr,FPU_CTRLWORD);
			break;

		default:
			break;
		}
	}
}

// da
void
DB_ESC2(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU da %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	fpu_checkexception();
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0: /* FCMOVB */
			TRACEOUT(("ESC2: FCMOVB"));
			FPU_FCMOVB(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 1: /* FCMOVE */
			TRACEOUT(("ESC2: FCMOVE"));
			FPU_FCMOVE(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2: /* FCMOVBE */
			TRACEOUT(("ESC2: FCMOVBE"));
			FPU_FCMOVBE(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3: /* FCMOVU */
			TRACEOUT(("ESC2: FCMOVU"));
			FPU_FCMOVU(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 5:
			switch (sub) {
			case 1: /* FUCOMPP */
				TRACEOUT(("FUCOMPP"));
				FPU_FUCOM(FPU_STAT_TOP,FPU_ST(1));
				FPU_FPOP();
				FPU_FPOP();
				break;
				
			default:
				break;
			}
			break;
			
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_I32_EA(madr);
		EA_TREE(op);
	}
}

// db
void
DB_ESC3(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU db %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	if(!(op >= 0xc0 && idx==4)){
		fpu_checkexception();
	}
	if (op >= 0xc0) 
	{
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0: /* FCMOVNB */
			TRACEOUT(("ESC3: FCMOVNB"));
			FPU_FCMOVNB(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 1: /* FCMOVNE */
			TRACEOUT(("ESC3: FCMOVNE"));
			FPU_FCMOVNE(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2: /* FCMOVNBE */
			TRACEOUT(("ESC3: FCMOVNBE"));
			FPU_FCMOVNBE(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3: /* FCMOVNU */
			TRACEOUT(("ESC3: FCMOVNU"));
			FPU_FCMOVNU(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 4:
			switch (sub) {
			case 0: /* FNENI */
			case 1: /* FNDIS */
				break;
				
			case 2: /* FCLEX */
				TRACEOUT(("FCLEX"));
				FPU_FCLEX();
				break;
				
			case 3: /* FNINIT/FINIT */
				TRACEOUT(("FNINIT/FINIT"));
				FPU_FINIT();
				break;
				
			case 4: /* FNSETPM */
			case 5: /* FRSTPM */
				FPU_FNOP();
				break;
				
			default:
				break;
			}
			break;
		case 5: /* FUCOMI */
			TRACEOUT(("ESC3: FUCOMI"));
			FPU_FUCOMI(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 6: /* FCOMI */
			TRACEOUT(("ESC3: FCOMI"));
			FPU_FCOMI(FPU_STAT_TOP,FPU_ST(sub));	
			break;
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (DWORD) */
			TRACEOUT(("FILD"));
			FPU_PREP_PUSH();
			FPU_FLD_I32(madr,FPU_STAT_TOP);
			break;
			
		case 1:	/* FISTTP (DWORD) */
			{
				FP_RND oldrnd = FPU_STAT.round;
				FPU_STAT.round = ROUND_Down;
				FPU_FST_I32(madr);
				FPU_STAT.round = oldrnd;
			}
			FPU_FPOP();
			break;
			
		case 2:	/* FIST (DWORD) */
			TRACEOUT(("FIST"));
			FPU_FST_I32(madr);
			break;
			
		case 3:	/* FISTP (DWORD) */
			TRACEOUT(("FISTP"));
			FPU_FST_I32(madr);
			FPU_FPOP();
			break;
			
		case 5:	/* FLD (拡張実数) */
			TRACEOUT(("FLD 80 Bits Real"));
			FPU_PREP_PUSH();
			FPU_FLD_F80(madr);
			break;
			
		case 7:	/* FSTP (拡張実数) */
			TRACEOUT(("FSTP 80 Bits Real"));
			FPU_FST_F80(madr);
			FPU_FPOP();
			break;

		default:
			break;
		}
	}
}

// dc
void
DB_ESC4(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU dc %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	fpu_checkexception();
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADD */
			TRACEOUT(("ESC4: FADD"));
			FPU_FADD(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 1:	/* FMUL */
			TRACEOUT(("ESC4: FMUL"));
			FPU_FMUL(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 2: /* FCOM */
			TRACEOUT(("ESC4: FCOM"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));			
			break;
		case 3: /* FCOMP */
			TRACEOUT(("ESC4: FCOMP"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));	
			FPU_FPOP();
			break;
		case 4:	/* FSUBR */
			TRACEOUT(("ESC4: FSUBR"));
			FPU_FSUBR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 5:	/* FSUB */
			TRACEOUT(("ESC4: FSUB"));
			FPU_FSUB(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 6:	/* FDIVR */
			TRACEOUT(("ESC4: FDIVR"));
			FPU_FDIVR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 7:	/* FDIV */
			TRACEOUT(("ESC4: FDIV"));
			FPU_FDIV(FPU_ST(sub),FPU_STAT_TOP);
			break;
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_F64_EA(madr);
		EA_TREE(op);
	}
}

// dd
void
DB_ESC5(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU dd %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	fpu_check_NM_EXCEPTION();
	//if(op < 0xc0 && (idx==6 || idx==4)){
	//	_ADD_EIP(-1); // XXX: 無理やり戻す
	//	fpu_check_NM_EXCEPTION2();
	//	_ADD_EIP(1);
	//}else{
	//	_ADD_EIP(-1); // XXX: 無理やり戻す
	//	fpu_check_NM_EXCEPTION();
	//	_ADD_EIP(1);
	//}
	if(op >= 0xc0 || (idx!=4 && idx!=6 && idx!=7)){
		fpu_checkexception();
	}
	if (op >= 0xc0) {
		/* FUCOM ST(i), ST(0) */
		/* Fxxx ST(i) */
		switch (idx) {
		case 0:	/* FFREE */
			TRACEOUT(("FFREE"));
			FPU_STAT.tag[FPU_ST(sub)]=TAG_Empty;
			break;
		case 1: /* FXCH */
			TRACEOUT(("FXCH"));
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2:	/* FST */
			TRACEOUT(("FST"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3:	/* FSTP */
			TRACEOUT(("FSTP"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;
		case 4:	/* FUCOM */
			TRACEOUT(("FUCOM"));
			FPU_FUCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 5:	/* FUCOMP */
			TRACEOUT(("FUCOMP"));
			FPU_FUCOM(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;

		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FLD (倍精度実数) */
			TRACEOUT(("FLD double real"));
			FPU_PREP_PUSH();
			FPU_FLD_F64(madr,FPU_STAT_TOP);
			break;
		case 1:	/* FISTTP (QWORD) */
			{
				FP_RND oldrnd = FPU_STAT.round;
				FPU_STAT.round = ROUND_Down;
				FPU_FST_I64(madr);
				FPU_STAT.round = oldrnd;
			}
			FPU_FPOP();
			break;
		case 2:	/* FST (倍精度実数) */
			TRACEOUT(("FST double real"));
			FPU_FST_F64(madr);
			break;
		case 3:	/* FSTP (倍精度実数) */
			TRACEOUT(("FSTP double real"));
			FPU_FST_F64(madr);
			FPU_FPOP();
			break;
		case 4:	/* FRSTOR */
			TRACEOUT(("FRSTOR"));
			FPU_FRSTOR(madr);
			break;
		case 6:	/* FSAVE */
			TRACEOUT(("FSAVE"));
			FPU_FSAVE(madr);
			break;

		case 7:	/* FSTSW */
			FPU_SET_TOP(FPU_STAT_TOP);
			//cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, FPU_CTRLWORD);
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, FPU_STATUSWORD);
			break;

		default:
			break;
		}
	}
}

// de
void
DB_ESC6(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU de %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	fpu_checkexception();
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADDP */
			TRACEOUT(("FADDP"));
			FPU_FADD(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 1:	/* FMULP */
			TRACEOUT(("FMULP"));
			FPU_FMUL(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 2:	/* FCOMP5 */
			TRACEOUT(("FCOMP5"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3: /* FCOMPP */
			TRACEOUT(("FCOMPP"));
			if(sub != 1) {
				return;
			}
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(1));
			FPU_FPOP(); /* extra pop at the bottom*/
			break;			
		case 4:	/* FSUBRP */
			TRACEOUT(("FSUBRP"));
			FPU_FSUBR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 5:	/* FSUBP */
			TRACEOUT(("FSUBP"));
			FPU_FSUB(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 6:	/* FDIVRP */
			TRACEOUT(("FDIVRP"));
			FPU_FDIVR(FPU_ST(sub),FPU_STAT_TOP);
			if((FPU_STATUSWORD & ~FPU_CTRLWORD) & FP_ZE_FLAG){
				return; // POPしないようにする
			}
			break;
		case 7:	/* FDIVP */
			TRACEOUT(("FDIVP"));
			FPU_FDIV(FPU_ST(sub),FPU_STAT_TOP);
			if((FPU_STATUSWORD & ~FPU_CTRLWORD) & FP_ZE_FLAG){
				return; // POPしないようにする
			}
			break;
			/*FALLTHROUGH*/
		default:
			break;
		}
		FPU_FPOP();
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_I16_EA(madr);
		EA_TREE(op);
	}
}

// df
void
DB_ESC7(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU df %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	if(!(op >= 0xc0 && idx==4 && sub==0)){
		fpu_checkexception();
	}
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0: /* FFREEP */
			TRACEOUT(("FFREEP"));
			FPU_STAT.tag[FPU_ST(sub)]=TAG_Empty;
			FPU_FPOP();
			break;
		case 1: /* FXCH */
			TRACEOUT(("FXCH"));
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;
		
		case 2:
		case 3: /* FSTP */
			TRACEOUT(("FSTP"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;

		case 4:
			switch (sub)
			{
			case 0: /* FSTSW AX */
				TRACEOUT(("FSTSW AX"));
				FPU_SET_TOP(FPU_STAT_TOP);
				CPU_AX = FPU_STATUSWORD;
				break;
				
			default:
				break;
			}
			break;
		case 5: /* FUCOMIP */
			TRACEOUT(("ESC7: FUCOMIP"));
			FPU_FUCOMI(FPU_STAT_TOP,FPU_ST(sub));	
			FPU_FPOP();
			break;
		case 6: /* FCOMIP */
			TRACEOUT(("ESC7: FCOMIP"));
			FPU_FCOMI(FPU_STAT_TOP,FPU_ST(sub));	
			FPU_FPOP();
			break;
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (WORD) */
			TRACEOUT(("FILD SINT16"));
			FPU_PREP_PUSH();
			FPU_FLD_I16(madr,FPU_STAT_TOP);
			break;
		case 1:	/* FISTTP (WORD) */
			{
				FP_RND oldrnd = FPU_STAT.round;
				FPU_STAT.round = ROUND_Down;
				FPU_FST_I16(madr);
				FPU_STAT.round = oldrnd;
			}
			FPU_FPOP();
			break;
		case 2:	/* FIST (WORD) */
			TRACEOUT(("FIST SINT16"));
			FPU_FST_I16(madr);
			break;
		case 3:	/* FISTP (WORD) */
			TRACEOUT(("FISTP SINT16"));
			FPU_FST_I16(madr);
			FPU_FPOP();
			break;

		case 4:	/* FBLD (BCD) */
			TRACEOUT(("FBLD packed BCD"));
			FPU_PREP_PUSH();
			FPU_FBLD(madr,FPU_STAT_TOP);
			break;

		case 5:	/* FILD (QWORD) */
			TRACEOUT(("FILD SINT64"));
			FPU_PREP_PUSH();
			FPU_FLD_I64(madr,FPU_STAT_TOP);
			break;

		case 6:	/* FBSTP (BCD) */
			TRACEOUT(("FBSTP packed BCD"));
			FPU_FBST(madr);
			FPU_FPOP();
			break;

		case 7:	/* FISTP (QWORD) */
			TRACEOUT(("FISTP SINT64"));
			FPU_FST_I64(madr);
			FPU_FPOP();
			break;

		default:
			break;
		}
	}
}
#endif
