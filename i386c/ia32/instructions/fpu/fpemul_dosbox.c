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
 */

#include "compiler.h"

#include <float.h>
#include <math.h>
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "ia32/instructions/fpu/fp.h"

#if 1
#undef	TRACEOUT
#define	TRACEOUT(s)	(void)(s)
#endif	/* 0 */

#ifdef USE_FPU

#define FPU_WORKCLOCK	6

#define PI		3.14159265358979323846
#define L2E		1.4426950408889634
#define L2T		3.3219280948873623
#define LN2		0.69314718055994531
#define LG2		0.3010299956639812

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

static INLINE void FPU_SetTag(UINT16 tag)
{
	UINT i;
	
	for(i=0;i<8;i++)
		FPU_STAT.tag[i] = (FP_TAG)((tag >>(2*i))&3);
}

static void FPU_FCLEX(void){
	FPU_STATUSWORD &= 0xff00;			//should clear exceptions
}

static void FPU_FNOP(void){
	return;
}

static void FPU_PUSH(double in){
	FPU_STAT_TOP = (FPU_STAT_TOP - 1) &7;
	//actually check if empty
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Valid;
	FPU_STAT.reg[FPU_STAT_TOP].d = in;
//	LOG(LOG_FPU,LOG_ERROR)("Pushed at %d  %g to the stack",newtop,in);
	return;
}

static void FPU_PREP_PUSH(void){
	FPU_STAT_TOP = (FPU_STAT_TOP - 1) &7;
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Valid;
}

static void FPU_FPOP(void){
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Empty;
	//maybe set zero in it as well
	FPU_STAT_TOP = ((FPU_STAT_TOP+1)&7);
//	LOG(LOG_FPU,LOG_ERROR)("popped from %d  %g off the stack",top,fpu.regs[top].d);
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

static double FPU_FLD80(UINT32 addr) 
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
		result.d = sign?-HUGE_VAL:HUGE_VAL;
	}
	return result.d;

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
	if(FPU_STAT.reg[reg].d != 0){ //Zero is a special case
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
	FPU_STAT.reg[store_to].d = (double)(blah.f);
}

static void FPU_FLD_F64(UINT32 addr,UINT store_to) {
	FPU_STAT.reg[store_to].l.lower = fpu_memoryread_d(addr);
	FPU_STAT.reg[store_to].l.upper = fpu_memoryread_d(addr+4);
}

static void FPU_FLD_F80(UINT32 addr) {
	FPU_STAT.reg[FPU_STAT_TOP].d = FPU_FLD80(addr);
}

static void FPU_FLD_I16(UINT32 addr,UINT store_to) {
	SINT16 blah;

	blah = fpu_memoryread_w(addr);
	FPU_STAT.reg[store_to].d = (double)(blah);
}

static void FPU_FLD_I32(UINT32 addr,UINT store_to) {
	SINT32 blah;

	blah = fpu_memoryread_d(addr);
	FPU_STAT.reg[store_to].d = (double)(blah);
}

static void FPU_FLD_I64(UINT32 addr,UINT store_to) {
	FP_REG blah;
	
	blah.l.lower = fpu_memoryread_d(addr);
	blah.l.upper = fpu_memoryread_d(addr+4);
	FPU_STAT.reg[store_to].d = (double)(blah.ll);
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
	FPU_STAT.reg[store_to].d = temp;
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
	blah.f = (float)(FPU_STAT.reg[FPU_STAT_TOP].d);
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
	fpu_memorywrite_w(addr,(SINT16)(FROUND(FPU_STAT.reg[FPU_STAT_TOP].d)));
}

static void FPU_FST_I32(UINT32 addr) {
	fpu_memorywrite_d(addr,(SINT32)(FROUND(FPU_STAT.reg[FPU_STAT_TOP].d)));
}

static void FPU_FST_I64(UINT32 addr) {
	FP_REG blah;
	
	blah.ll = (SINT64)(FROUND(FPU_STAT.reg[FPU_STAT_TOP].d));
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
		val.d=-val.d;
	}
	//numbers from back to front
	temp=val.d;
	for(i=0;i<9;i++){
		val.d=temp;
		temp = (double)((SINT64)(floor(val.d/10.0)));
		p = (UINT)(val.d - 10.0*temp);  
		val.d=temp;
		temp = (double)((SINT64)(floor(val.d/10.0)));
		p |= ((UINT)(val.d - 10.0*temp)<<4);

		fpu_memorywrite_b(addr+i,p);
	}
	val.d=temp;
	temp = (double)((SINT64)(floor(val.d/10.0)));
	p = (UINT)(val.d - 10.0*temp);
	if(sign)
		p|=0x80;
	fpu_memorywrite_b(addr+9,p);
}

//#define isinf(x) (!(_finite(x) || _isnan(x)))
#define isdenormal(x) (fpclassify(x) == FP_SUBNORMAL)

static void FPU_FADD(UINT op1, UINT op2){
	// HACK: Set the denormal flag according to whether the source or final result is a denormalized number.
	//       This is vital if we don't want certain DOS programs to mis-detect our FPU emulation as an IIT clone chip when cputype == 286
	BOOL was_not_normal;

	was_not_normal = isdenormal(FPU_STAT.reg[op1].d);
	FPU_STAT.reg[op1].d+=FPU_STAT.reg[op2].d;
	FPU_SET_D(was_not_normal || isdenormal(FPU_STAT.reg[op1].d) || isdenormal(FPU_STAT.reg[op2].d));
	//flags and such :)
	return;
}

static void FPU_FSIN(void){
	FPU_STAT.reg[FPU_STAT_TOP].d = sin(FPU_STAT.reg[FPU_STAT_TOP].d);
	FPU_SET_C2(0);
	//flags and such :)
	return;
}

static void FPU_FSINCOS(void){
	double temp;

	temp = FPU_STAT.reg[FPU_STAT_TOP].d;
	FPU_STAT.reg[FPU_STAT_TOP].d = sin(temp);
	FPU_PUSH(cos(temp));
	FPU_SET_C2(0);
	//flags and such :)
	return;
}

static void FPU_FCOS(void){
	FPU_STAT.reg[FPU_STAT_TOP].d = cos(FPU_STAT.reg[FPU_STAT_TOP].d);
	FPU_SET_C2(0);
	//flags and such :)
	return;
}

static void FPU_FSQRT(void){
	FPU_STAT.reg[FPU_STAT_TOP].d = sqrt(FPU_STAT.reg[FPU_STAT_TOP].d);
	//flags and such :)
	return;
}
static void FPU_FPATAN(void){
	FPU_STAT.reg[FPU_ST(1)].d = atan2(FPU_STAT.reg[FPU_ST(1)].d,FPU_STAT.reg[FPU_STAT_TOP].d);
	FPU_FPOP();
	//flags and such :)
	return;
}
static void FPU_FPTAN(void){
	FPU_STAT.reg[FPU_STAT_TOP].d = tan(FPU_STAT.reg[FPU_STAT_TOP].d);
	FPU_PUSH(1.0);
	FPU_SET_C2(0);
	//flags and such :)
	return;
}
static void FPU_FDIV(UINT st, UINT other){
	FPU_STAT.reg[st].d= FPU_STAT.reg[st].d/FPU_STAT.reg[other].d;
	//flags and such :)
	return;
}

static void FPU_FDIVR(UINT st, UINT other){
	FPU_STAT.reg[st].d= FPU_STAT.reg[other].d/FPU_STAT.reg[st].d;
	// flags and such :)
	return;
}

static void FPU_FMUL(UINT st, UINT other){
	FPU_STAT.reg[st].d*=FPU_STAT.reg[other].d;
	//flags and such :)
	return;
}

static void FPU_FSUB(UINT st, UINT other){
	FPU_STAT.reg[st].d = FPU_STAT.reg[st].d - FPU_STAT.reg[other].d;
	//flags and such :)
	return;
}

static void FPU_FSUBR(UINT st, UINT other){
	FPU_STAT.reg[st].d = FPU_STAT.reg[other].d - FPU_STAT.reg[st].d;
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

	if(FPU_STAT.reg[st].d == FPU_STAT.reg[other].d){
		FPU_SET_C3(1);
		FPU_SET_C2(0);
		FPU_SET_C0(0);
		return;
	}
	if(FPU_STAT.reg[st].d < FPU_STAT.reg[other].d){
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

static void FPU_FUCOM(UINT st, UINT other){
	//does atm the same as fcom 
	FPU_FCOM(st,other);
}

static void FPU_FRNDINT(void){
	SINT64 temp;

	temp = (SINT64)(FROUND(FPU_STAT.reg[FPU_STAT_TOP].d));
	FPU_STAT.reg[FPU_STAT_TOP].d=(double)(temp);
}

static void FPU_FPREM(void){
	double valtop;
	double valdiv;
	SINT64 ressaved;

	valtop = FPU_STAT.reg[FPU_STAT_TOP].d;
	valdiv = FPU_STAT.reg[FPU_ST(1)].d;
	ressaved = (SINT64)( (valtop/valdiv) );
// Some backups
//	Real64 res=valtop - ressaved*valdiv; 
//      res= fmod(valtop,valdiv);
	FPU_STAT.reg[FPU_STAT_TOP].d = valtop - ressaved*valdiv;
	FPU_SET_C0((UINT)(ressaved&4));
	FPU_SET_C3((UINT)(ressaved&2));
	FPU_SET_C1((UINT)(ressaved&1));
	FPU_SET_C2(0);
}

static void FPU_FPREM1(void){
	double valtop;
	double valdiv, quot, quotf;
	SINT64 ressaved;
	
	valtop = FPU_STAT.reg[FPU_STAT_TOP].d;
	valdiv = FPU_STAT.reg[FPU_ST(1)].d;
	quot = valtop/valdiv;
	quotf = floor(quot);
	
	if (quot-quotf>0.5) ressaved = (SINT64)(quotf+1);
	else if (quot-quotf<0.5) ressaved = (SINT64)(quotf);
	else ressaved = (SINT64)(((((SINT64)(quotf))&1)!=0)?(quotf+1):(quotf));
	
	FPU_STAT.reg[FPU_STAT_TOP].d = valtop - ressaved*valdiv;
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
	if(FPU_STAT.reg[FPU_STAT_TOP].d == 0.0)		//zero or normalized number.
	{ 
		FPU_SET_C3(1);FPU_SET_C2(0);FPU_SET_C0(0);
	}
	else
	{
		FPU_SET_C3(0);FPU_SET_C2(1);FPU_SET_C0(0);
	}
}

static void FPU_F2XM1(void){
	FPU_STAT.reg[FPU_STAT_TOP].d = pow(2.0,FPU_STAT.reg[FPU_STAT_TOP].d) - 1;
	return;
}

static void FPU_FYL2X(void){
	FPU_STAT.reg[FPU_ST(1)].d*=log(FPU_STAT.reg[FPU_STAT_TOP].d)/log((double)(2.0));
	FPU_FPOP();
	return;
}

static void FPU_FYL2XP1(void){
	FPU_STAT.reg[FPU_ST(1)].d*=log(FPU_STAT.reg[FPU_STAT_TOP].d+1.0)/log((double)(2.0));
	FPU_FPOP();
	return;
}

static void FPU_FSCALE(void){
	FPU_STAT.reg[FPU_STAT_TOP].d *= pow(2.0,(double)((SINT64)(FPU_STAT.reg[FPU_ST(1)].d)));
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
		break;
		
	case 0x100: case 0x101:
		fpu_memorywrite_w(addr+0,FPU_CTRLWORD);
		fpu_memorywrite_w(addr+4,FPU_STATUSWORD);
		fpu_memorywrite_w(addr+8,(FPU_GetTag()));				
		break;
	}
}

static void FPU_FLDENV(UINT32 addr)
{
	descriptor_t *sdp = &CPU_CS_DESC;	
	FPU_STAT_TOP = FPU_GET_TOP();
	
	switch ((CPU_CR0 & 1) | (SEG_IS_32BIT(sdp) ? 0x100 : 0x000)) {
	case 0x000: case 0x001:
		FPU_SetCW(fpu_memoryread_w(addr+0));
		FPU_STATUSWORD = fpu_memoryread_w(addr+2);
		FPU_SetTag(fpu_memoryread_w(addr+4));
		break;
		
	case 0x100: case 0x101:
		FPU_SetCW(fpu_memoryread_w(addr+0));
		FPU_STATUSWORD = fpu_memoryread_w(addr+4);
		FPU_SetTag(fpu_memoryread_w(addr+8));			
		break;
	}
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
	fpu_init();
}

static void FPU_FRSTOR(UINT32 addr)
{
	UINT start;
	UINT i;
	
	descriptor_t *sdp = &CPU_CS_DESC;
	
	FPU_FLDENV(addr);
	start = ((SEG_IS_32BIT(sdp))?28:14);
	for(i = 0;i < 8;i++){
		FPU_STAT.reg[FPU_ST(i)].d = FPU_FLD80(addr+start);
		start += 10;
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
	mant = test.d / (pow(2.0,(double)(exp80final)));
	FPU_STAT.reg[FPU_STAT_TOP].d = (double)(exp80final);
	FPU_PUSH(mant);
}

static void FPU_FCHS(void){
	FPU_STAT.reg[FPU_STAT_TOP].d = -1.0*(FPU_STAT.reg[FPU_STAT_TOP].d);
}

static void FPU_FABS(void){
	FPU_STAT.reg[FPU_STAT_TOP].d = fabs(FPU_STAT.reg[FPU_STAT_TOP].d);
}

static void FPU_FTST(void){
	FPU_STAT.reg[8].d = 0.0;
	FPU_FCOM(FPU_STAT_TOP,8);
}

static void FPU_FLD1(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d = 1.0;
}

static void FPU_FLDL2T(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d = L2T;
}

static void FPU_FLDL2E(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d = L2E;
}

static void FPU_FLDPI(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d = PI;
}

static void FPU_FLDLG2(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d = LG2;
}

static void FPU_FLDLN2(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d = LN2;
}

static void FPU_FLDZ(void){
	FPU_PREP_PUSH();
	FPU_STAT.reg[FPU_STAT_TOP].d = 0.0;
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
int fpu_updateEmuEnv(void);
void
fpu_init(void)
{
	int i;
	FPU_SetCW(0x37F);
	FPU_STATUSWORD = 0;
	FPU_STAT_TOP=FPU_GET_TOP();
	for(i=0;i<8;i++){
		FPU_STAT.tag[i] = TAG_Empty;
		FPU_STAT.reg[i].d = 0;
		FPU_STAT.reg[i].l.lower = 0;
		FPU_STAT.reg[i].l.upper = 0;
		FPU_STAT.reg[i].ll = 0;
	}
	FPU_STAT.tag[8] = TAG_Valid; // is only used by us
}

char *
fpu_reg2str(void)
{
	return NULL;
}


/*
 * FPU instruction
 */

void
fpu_checkexception(void){
}

void
fpu_fwait(void)
{

	/* XXX: check pending FPU exception */
	fpu_checkexception();
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
ESC0(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU d8 %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
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
ESC1(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU d9 %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
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
			break;
		}

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

			case 0x4:	/* FTST */
				TRACEOUT(("FTST"));
				FPU_FTST();
				break;

			case 0x5:	/* FXAM */
				TRACEOUT(("FXAM"));
				FPU_FXAM();
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
ESC2(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU da %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
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
ESC3(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU db %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) 
	{
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 4:
			switch (sub) {
				case 0:
				case 1:
				break;
				
				case 2: /* FCLEX */
				TRACEOUT(("FCLEX"));
				FPU_FCLEX();
				break;
				
				case 3: /* FNINIT/FINIT */
				TRACEOUT(("FNINIT/FINIT"));
				fpu_init();
				break;
				
				case 4:
				case 5:
				FPU_FNOP();
				break;
				
				default:
				break;
			}
			break;
			/*FALLTHROUGH*/
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
ESC4(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC4();
	//	return;
	//}
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU dc %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
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
ESC5(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC5();
	//	return;
	//}
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU dd %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
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
ESC6(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU de %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADDP */
			TRACEOUT(("FADDP"));
			FPU_FADD(FPU_ST(sub),FPU_STAT_TOP);
			FPU_FPOP();
			break;
		case 1:	/* FMULP */
			TRACEOUT(("FMULP"));
			FPU_FMUL(FPU_ST(sub),FPU_STAT_TOP);
			FPU_FPOP();
			break;
		case 3: /* FCOMPP */
			TRACEOUT(("FCOMPP"));
			if(sub != 1) {
				return;
			}
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(1));
			FPU_FPOP(); /* extra pop at the bottom*/
			FPU_FPOP();
			break;			
		case 4:	/* FSUBRP */
			TRACEOUT(("FSUBRP"));
			FPU_FSUBR(FPU_ST(sub),FPU_STAT_TOP);
			FPU_FPOP();
			break;
		case 5:	/* FSUBP */
			TRACEOUT(("FSUBP"));
			FPU_FSUB(FPU_ST(sub),FPU_STAT_TOP);
			FPU_FPOP();
			break;
		case 6:	/* FDIVRP */
			TRACEOUT(("FDIVRP"));
			FPU_FDIVR(FPU_ST(sub),FPU_STAT_TOP);
			FPU_FPOP();
			break;
		case 7:	/* FDIVP */
			TRACEOUT(("FDIVP"));
			FPU_FDIV(FPU_ST(sub),FPU_STAT_TOP);
			FPU_FPOP();
			break;
			/*FALLTHROUGH*/
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_I16_EA(madr);
		EA_TREE(op);
	}
}

// df
void
ESC7(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU df %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
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
			/*FALLTHROUGH*/
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

#else
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
