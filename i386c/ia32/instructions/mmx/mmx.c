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

#include <ia32/cpu.h>
#include "ia32/ia32.mcr"

#include "ia32/instructions/mmx/mmx.h"

#if defined(USE_MMX) && defined(USE_FPU)

#define CPU_MMXWORKCLOCK	CPU_WORKCLOCK(2)

static INLINE void
MMX_check_NM_EXCEPTION(){
	// MMXなしならUD(無効オペコード例外)を発生させる
	if(!(i386cpuid.cpu_feature & CPU_FEATURE_MMX)){
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
 * MMX interface
 */
void
MMX_EMMS(void)
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

// *********** MOV

void MMX_MOVD_mm_rm32(void)
{
	UINT32 op, src;
	UINT idx, sub;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
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
	FPU_STAT.reg[idx].ul.lower = src;
	FPU_STAT.reg[idx].ul.upper = 0;
}
void MMX_MOVD_rm32_mm(void)
{
	UINT32 op, src, madr;
	UINT idx, sub;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	src = FPU_STAT.reg[idx].ul.lower;
	if (op >= 0xc0) {
		*(reg32_b20[op]) = src;
	} else {
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, src);
	}
}

void MMX_MOVQ_mm_mmm64(void)
{
	UINT32 op;
	UINT idx, sub;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.reg[idx].ll = FPU_STAT.reg[sub].ll;
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		FPU_STAT.reg[idx].ll = cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, madr);
		//FPU_STAT.reg[idx].ul.lower = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		//FPU_STAT.reg[idx].ul.upper = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr+4);
	}
}
void MMX_MOVQ_mmm64_mm(void)
{
	UINT32 op;
	UINT idx, sub;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		FPU_STAT.reg[sub].ll = FPU_STAT.reg[idx].ll;
	} else {
		UINT32 madr;
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_q(CPU_INST_SEGREG_INDEX, madr, FPU_STAT.reg[idx].ll);
		//cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, FPU_STAT.reg[idx].ul.lower);
		//cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr+4, FPU_STAT.reg[idx].ul.upper);
	}
}


// *********** PACK

void MMX_PACKSSWB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT16 *srcreg1;
	INT16 *srcreg2;
	INT8 *dstreg;
	INT8 dstregbuf[8];
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg1 = (INT16*)(&(FPU_STAT.reg[idx]));
		srcreg2 = (INT16*)(&(FPU_STAT.reg[sub]));
		dstreg = (INT8*)(&(FPU_STAT.reg[idx]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg1 = (INT16*)(&(FPU_STAT.reg[idx]));
		srcreg2 = srcregbuf.w;
		dstreg = (INT8*)(&(FPU_STAT.reg[idx]));
	}
	for(i=0;i<4;i++){
		if(srcreg1[i] > 127){
			dstregbuf[i] = 127;
		}else if(srcreg1[i] < -128){
			dstregbuf[i] = -128;
		}else{
			dstregbuf[i] = (INT8)(srcreg1[i]);
		}
	}
	for(i=0;i<4;i++){
		if(srcreg2[i] > 127){
			dstregbuf[i+4] = 127;
		}else if(srcreg2[i] < -128){
			dstregbuf[i+4] = -128;
		}else{
			dstregbuf[i+4] = (INT8)(srcreg2[i]);
		}
	}
	for(i=0;i<8;i++){
		dstreg[i] = dstregbuf[i];
	}
}
void MMX_PACKSSDW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT32 *srcreg1;
	INT32 *srcreg2;
	INT16 *dstreg;
	INT16 dstregbuf[4];
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg1 = (INT32*)(&(FPU_STAT.reg[idx]));
		srcreg2 = (INT32*)(&(FPU_STAT.reg[sub]));
		dstreg = (INT16*)(&(FPU_STAT.reg[idx]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg1 = (INT32*)(&(FPU_STAT.reg[idx]));
		srcreg2 = srcregbuf.d;
		dstreg = (INT16*)(&(FPU_STAT.reg[idx]));
	}
	for(i=0;i<2;i++){
		if(srcreg1[i] > 32767){
			dstregbuf[i] = 32767;
		}else if(srcreg1[i] < -32768){
			dstregbuf[i] = -32768;
		}else{
			dstregbuf[i] = (INT16)(srcreg1[i]);
		}
	}
	for(i=0;i<2;i++){
		if(srcreg2[i] > 32767){
			dstregbuf[i+2] = 32767;
		}else if(srcreg2[i] < -32768){
			dstregbuf[i+2] = -32768;
		}else{
			dstregbuf[i+2] = (INT16)(srcreg2[i]);
		}
	}
	for(i=0;i<4;i++){
		dstreg[i] = dstregbuf[i];
	}
}

void MMX_PACKUSWB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT16 *srcreg1;
	INT16 *srcreg2;
	UINT8 *dstreg;
	UINT8 dstregbuf[8];
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg1 = (INT16*)(&(FPU_STAT.reg[idx]));
		srcreg2 = (INT16*)(&(FPU_STAT.reg[sub]));
		dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg1 = (INT16*)(&(FPU_STAT.reg[idx]));
		srcreg2 = srcregbuf.w;
		dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	}
	for(i=0;i<4;i++){
		if(srcreg1[i] > 255){
			dstregbuf[i] = 255;
		}else if(srcreg1[i] < 0){
			dstregbuf[i] = 0;
		}else{
			dstregbuf[i] = (UINT8)(srcreg1[i]);
		}
	}
	for(i=0;i<4;i++){
		if(srcreg2[i] > 255){
			dstregbuf[i+4] = 255;
		}else if(srcreg2[i] < 0){
			dstregbuf[i+4] = 0;
		}else{
			dstregbuf[i+4] = (UINT8)(srcreg2[i]);
		}
	}
	for(i=0;i<8;i++){
		dstreg[i] = dstregbuf[i];
	}
}

// *********** PADD

void MMX_PADDB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT8 *srcreg;
	UINT8 *dstreg;
	int i;

	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<8;i++){
		dstreg[i] += srcreg[i];
	}
}
void MMX_PADDW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT16 *srcreg;
	UINT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		dstreg[i] += srcreg[i];
	}
}
void MMX_PADDD(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<2;i++){
		dstreg[i] += srcreg[i];
	}
}

void MMX_PADDSB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT8 *srcreg;
	INT8 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (INT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<8;i++){
		INT16 cbuf = (INT16)dstreg[i] + (INT16)srcreg[i];
		if(cbuf > 127){
			dstreg[i] = 127;
		}else if(cbuf < -128){
			dstreg[i] = -128;
		}else{
			dstreg[i] = (INT8)cbuf;
		}
	}
}
void MMX_PADDSW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT16 *srcreg;
	INT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (INT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		INT32 cbuf = (INT32)dstreg[i] + (INT32)srcreg[i];
		if(cbuf > 32767){
			dstreg[i] = 32767;
		}else if(cbuf < -32768){
			dstreg[i] = -32768;
		}else{
			dstreg[i] = (INT16)cbuf;
		}
	}
}

void MMX_PADDUSB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT8 *srcreg;
	UINT8 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<8;i++){
		UINT16 cbuf = (UINT16)dstreg[i] + (UINT16)srcreg[i];
		if(cbuf > 255){
			dstreg[i] = 255;
		}else{
			dstreg[i] = (UINT8)cbuf;
		}
	}
}
void MMX_PADDUSW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT16 *srcreg;
	UINT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		UINT32 cbuf = (UINT32)dstreg[i] + (UINT32)srcreg[i];
		if(cbuf > 65535){
			dstreg[i] = 65535;
		}else{
			dstreg[i] = (UINT16)cbuf;
		}
	}
}

// *********** PAND/ANDN,OR,XOR

void MMX_PAND(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));

	dstreg[0] = dstreg[0] & srcreg[0];
	dstreg[1] = dstreg[1] & srcreg[1];
}
void MMX_PANDN(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));

	//dstreg[0] = ~(dstreg[0] & srcreg[0]);
	//dstreg[1] = ~(dstreg[1] & srcreg[1]);
	dstreg[0] = (~dstreg[0]) & srcreg[0];
	dstreg[1] = (~dstreg[1]) & srcreg[1];
}
void MMX_POR(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));

	dstreg[0] = dstreg[0] | srcreg[0];
	dstreg[1] = dstreg[1] | srcreg[1];
}
void MMX_PXOR(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));

	dstreg[0] = dstreg[0] ^ srcreg[0];
	dstreg[1] = dstreg[1] ^ srcreg[1];
}

// *********** PCMPEQ

void MMX_PCMPEQB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT8 *srcreg;
	UINT8 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<8;i++){
		if(dstreg[i] == srcreg[i]){
			dstreg[i] = 0xff;
		}else{
			dstreg[i] = 0;
		}
	}
}
void MMX_PCMPEQW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT16 *srcreg;
	UINT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		if(dstreg[i] == srcreg[i]){
			dstreg[i] = 0xffff;
		}else{
			dstreg[i] = 0;
		}
	}
}
void MMX_PCMPEQD(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<2;i++){
		if(dstreg[i] == srcreg[i]){
			dstreg[i] = 0xffffffff;
		}else{
			dstreg[i] = 0;
		}
	}
}

// *********** PCMPGT

void MMX_PCMPGTB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT8 *srcreg;
	INT8 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (INT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<8;i++){
		if(dstreg[i] > srcreg[i]){
			dstreg[i] = 0xff;
		}else{
			dstreg[i] = 0;
		}
	}
}
void MMX_PCMPGTW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT16 *srcreg;
	INT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (INT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		if(dstreg[i] > srcreg[i]){
			dstreg[i] = 0xffff;
		}else{
			dstreg[i] = 0;
		}
	}
}
void MMX_PCMPGTD(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT32 *srcreg;
	INT32 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (INT32*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<2;i++){
		if(dstreg[i] > srcreg[i]){
			dstreg[i] = 0xffffffff;
		}else{
			dstreg[i] = 0;
		}
	}
}

// *********** PMADDWD
void MMX_PMADDWD(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT16 *srcreg;
	INT16 *dstreg;
	INT32 *dstreg32;
	INT32 dstregbuf[2];
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (INT16*)(&(FPU_STAT.reg[idx]));
	dstreg32 = (INT32*)(&(FPU_STAT.reg[idx]));
	
	dstregbuf[0] = (INT32)srcreg[0] * (INT32)dstreg[0] + (INT32)srcreg[1] * (INT32)dstreg[1];
	dstregbuf[1] = (INT32)srcreg[2] * (INT32)dstreg[2] + (INT32)srcreg[3] * (INT32)dstreg[3];
	dstreg32[0] = dstregbuf[0];
	dstreg32[1] = dstregbuf[1];
}

// *********** PMUL
void MMX_PMULHW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT16 *srcreg;
	INT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (INT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		dstreg[i] = (INT16)((((INT32)srcreg[i] * (INT32)dstreg[i]) >> 16) & 0xffff);
	}
}
void MMX_PMULLW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT16 *srcreg;
	INT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (INT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		dstreg[i] = (INT16)((((INT32)srcreg[i] * (INT32)dstreg[i])) & 0xffff);
	}
}

// *********** PSLL
void MMX_PSLLW(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		shift = FPU_STAT.reg[sub].ul.lower;
		if(FPU_STAT.reg[sub].ul.upper) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		shift = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		if(cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4)) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		//dstreg[i] = (dstreg[i] << shift);
		dstreg[i] = (shift >= 16 ? 0 : (dstreg[i] << (UINT16)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
	}
}
void MMX_PSLLD(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT32 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		shift = FPU_STAT.reg[sub].ul.lower;
		if(FPU_STAT.reg[sub].ul.upper) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		shift = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		if(cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4)) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<2;i++){
		//dstreg[i] = (dstreg[i] << shift);
		dstreg[i] = (shift >= 32 ? 0 : (dstreg[i] << (UINT32)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
	}
}
void MMX_PSLLQ(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT64 *dstreg;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		shift = FPU_STAT.reg[sub].ul.lower;
		if(FPU_STAT.reg[sub].ul.upper) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		shift = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		if(cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4)) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	}
	dstreg = (UINT64*)(&(FPU_STAT.reg[idx]));
	
	//dstreg[0] = (dstreg[0] << shift);
	dstreg[0] = (shift >= 64 ? 0 : (dstreg[0] << (UINT64)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
}

// *********** PSRA

void MMX_PSRAW(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT16 *dstreg;
	UINT16 signval;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		shift = FPU_STAT.reg[sub].ul.lower;
		if(FPU_STAT.reg[sub].ul.upper) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		shift = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		if(cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4)) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	// 無理やり算術シフト（怪しい）
	if(16 <= shift){
		signval = 0xffff;
	}else{
		UINT32 rshift = 16 - shift;
		signval = (0xffff >> rshift) << rshift;
	}
	for(i=0;i<4;i++){
		if(((INT16*)dstreg)[i] < 0){
			dstreg[i] = (dstreg[i] >> shift) | signval;
		}else{
			dstreg[i] = (shift >= 16 ? 0 : (dstreg[i] >> (UINT16)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
		}
	}
}
void MMX_PSRAD(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT32 *dstreg;
	UINT32 signval;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		shift = FPU_STAT.reg[sub].ul.lower;
		if(FPU_STAT.reg[sub].ul.upper) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		shift = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		if(cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4)) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));
	
	// 無理やり算術シフト（怪しい）
	if(32 <= shift){
		signval = 0xffffffff;
	}else{
		UINT32 rshift = 32 - shift;
		signval = (0xffffffff >> rshift) << rshift;
	}
	for(i=0;i<2;i++){
		if(((INT32*)dstreg)[i] < 0){
			dstreg[i] = (dstreg[i] >> shift) | signval;
		}else{
			dstreg[i] = (shift >= 32 ? 0 : (dstreg[i] >> (UINT32)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
		}
	}
}

// *********** PSRL
void MMX_PSRLW(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		shift = FPU_STAT.reg[sub].ul.lower;
		if(FPU_STAT.reg[sub].ul.upper) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		shift = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		if(cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4)) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		//dstreg[i] = (dstreg[i] >> shift);
		dstreg[i] = (shift >= 16 ? 0 : (dstreg[i] >> (UINT16)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
	}
}
void MMX_PSRLD(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT32 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		shift = FPU_STAT.reg[sub].ul.lower;
		if(FPU_STAT.reg[sub].ul.upper) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		shift = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		if(cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4)) shift = 0xffffffff; // XXX: とりあえずレジスタ内容が消えるくらい大きなシフト量にしておく
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<2;i++){
		//dstreg[i] = (dstreg[i] >> shift);
		dstreg[i] = (shift >= 32 ? 0 : (dstreg[i] >> (UINT32)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
	}
}
void MMX_PSRLQ(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT64 *dstreg;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		shift = FPU_STAT.reg[sub].ul.lower;
		if(FPU_STAT.reg[sub].ul.upper) shift = 0xffffffff; // XXX: シフトしすぎ
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		shift = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		if(cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr+4)) shift = 0xffffffff; // XXX: シフトしすぎ
	}
	dstreg = (UINT64*)(&(FPU_STAT.reg[idx]));
	
	//dstreg[0] = (dstreg[0] >> shift);
	dstreg[0] = (shift >= 64 ? 0 : (dstreg[0] >> (UINT64)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
}

// *********** PSLL(imm8),PSRL(imm8),PSRA(imm8) 
void MMX_PSxxW_imm8(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT16 *dstreg;
	UINT16 signval;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	dstreg = (UINT16*)(&(FPU_STAT.reg[sub]));
	GET_PCBYTE((shift));
	
	switch(idx){
	case 2: // PSRLW(imm8)
		for(i=0;i<4;i++){
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
		for(i=0;i<4;i++){
			if(((INT16*)dstreg)[i] < 0){
				dstreg[i] = (dstreg[i] >> shift) | signval;
			}else{
				dstreg[i] = (shift >= 16 ? 0 : (dstreg[i] >> (UINT16)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
			}
		}
		break;
	case 6: // PSLLW(imm8)
		for(i=0;i<4;i++){
			dstreg[i] = (shift >= 16 ? 0 : (dstreg[i] << (UINT16)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
		}
		break;
	default:
		break;
	}
}
void MMX_PSxxD_imm8(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT32 *dstreg;
	UINT32 signval;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	dstreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	GET_PCBYTE((shift));
	
	switch(idx){
	case 2: // PSRLD(imm8)
		for(i=0;i<2;i++){
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
		for(i=0;i<2;i++){
			if(((INT32*)dstreg)[i] < 0){
				dstreg[i] = (dstreg[i] >> shift) | signval;
			}else{
				dstreg[i] = (shift >= 32 ? 0 : (dstreg[i] >> (UINT32)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
			}
		}
		break;
	case 6: // PSLLD(imm8)
		for(i=0;i<2;i++){
			dstreg[i] = (shift >= 32 ? 0 : (dstreg[i] << (UINT32)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
		}
		break;
	default:
		break;
	}
}
void MMX_PSxxQ_imm8(void)
{
	UINT32 op;
	UINT idx, sub;
	UINT32 shift;
	UINT64 *dstreg;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	dstreg = (UINT64*)(&(FPU_STAT.reg[sub]));
	GET_PCBYTE((shift));
	
	switch(idx){
	case 2: // PSRLQ(imm8)
		dstreg[0] = (shift >= 64 ? 0 : (dstreg[0] >> (UINT64)shift)); // XXX: LSBが取り残されるのでごまかし（環境依存？）
		break;
	case 4: // PSRAQ(imm8)
		EXCEPTION(UD_EXCEPTION, 0);
		break;
	case 6: // PSLLQ(imm8)
		dstreg[0] = (shift >= 64 ? 0 : (dstreg[0] << (UINT64)shift)); // XXX: MSBが取り残されるのでごまかし（環境依存？）
		break;
	default:
		break;
	}
}

// *********** PSUB

void MMX_PSUBB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT8 *srcreg;
	UINT8 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<8;i++){
		dstreg[i] -= srcreg[i];
	}
}
void MMX_PSUBW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT16 *srcreg;
	UINT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		dstreg[i] -= srcreg[i];
	}
}
void MMX_PSUBD(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<2;i++){
		dstreg[i] -= srcreg[i];
	}
}

void MMX_PSUBSB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT8 *srcreg;
	INT8 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (INT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<8;i++){
		INT16 cbuf = (INT16)dstreg[i] - (INT16)srcreg[i];
		if(cbuf > 127){
			dstreg[i] = 127;
		}else if(cbuf < -128){
			dstreg[i] = -128;
		}else{
			dstreg[i] = (INT8)cbuf;
		}
	}
}
void MMX_PSUBSW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	INT16 *srcreg;
	INT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (INT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (INT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		INT32 cbuf = (INT32)dstreg[i] - (INT32)srcreg[i];
		if(cbuf > 32767){
			dstreg[i] = 32767;
		}else if(cbuf < -32768){
			dstreg[i] = -32768;
		}else{
			dstreg[i] = (INT16)cbuf;
		}
	}
}

void MMX_PSUBUSB(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT8 *srcreg;
	UINT8 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<8;i++){
		INT16 cbuf = (INT16)dstreg[i] - (INT16)srcreg[i];
		if(cbuf > 255){
			dstreg[i] = 255;
		}else if(cbuf < 0){
			dstreg[i] = 0;
		}else{
			dstreg[i] = (UINT8)cbuf;
		}
	}
}
void MMX_PSUBUSW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT16 *srcreg;
	UINT16 *dstreg;
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		INT32 cbuf = (INT32)dstreg[i] - (INT32)srcreg[i];
		if(cbuf > 65535){
			dstreg[i] = 65535;
		}else if(cbuf < 0){
			dstreg[i] = 0;
		}else{
			dstreg[i] = (UINT16)cbuf;
		}
	}
}

// *********** PUNPCK

void MMX_PUNPCKHBW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT8 *srcreg;
	UINT8 *dstreg;
	UINT8 dstregbuf[8];
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		dstregbuf[i*2] = dstreg[i+4];
		dstregbuf[i*2 + 1] = srcreg[i+4];
	}
	for(i=0;i<8;i++){
		dstreg[i] = dstregbuf[i];
	}
}
void MMX_PUNPCKHWD(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT16 *srcreg;
	UINT16 *dstreg;
	UINT16 dstregbuf[4];
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<2;i++){
		dstregbuf[i*2] = dstreg[i+2];
		dstregbuf[i*2 + 1] = srcreg[i+2];
	}
	for(i=0;i<4;i++){
		dstreg[i] = dstregbuf[i];
	}
}
void MMX_PUNPCKHDQ(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	UINT32 dstregbuf[2];
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));
	
	dstregbuf[0] = dstreg[1];
	dstregbuf[1] = srcreg[1];
	dstreg[0] = dstregbuf[0];
	dstreg[1] = dstregbuf[1];
}
void MMX_PUNPCKLBW(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT8 *srcreg;
	UINT8 *dstreg;
	UINT8 dstregbuf[8];
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT8*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.b;
	}
	dstreg = (UINT8*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<4;i++){
		dstregbuf[i*2] = dstreg[i];
		dstregbuf[i*2 + 1] = srcreg[i];
	}
	for(i=0;i<8;i++){
		dstreg[i] = dstregbuf[i];
	}
}
void MMX_PUNPCKLWD(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT16 *srcreg;
	UINT16 *dstreg;
	UINT16 dstregbuf[4];
	int i;
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT16*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.w;
	}
	dstreg = (UINT16*)(&(FPU_STAT.reg[idx]));
	
	for(i=0;i<2;i++){
		dstregbuf[i*2] = dstreg[i];
		dstregbuf[i*2 + 1] = srcreg[i];
	}
	for(i=0;i<4;i++){
		dstreg[i] = dstregbuf[i];
	}
}
void MMX_PUNPCKLDQ(void)
{
	UINT32 op;
	UINT idx, sub;
	MMXREG srcregbuf;
	UINT32 *srcreg;
	UINT32 *dstreg;
	UINT32 dstregbuf[2];
	
	MMX_check_NM_EXCEPTION();
	MMX_setTag();
	CPU_MMXWORKCLOCK;
	GET_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if ((op) >= 0xc0) {
		srcreg = (UINT32*)(&(FPU_STAT.reg[sub]));
	} else {
		UINT32 maddr;
		maddr = calc_ea_dst((op));
		srcregbuf.d[0] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr);
		srcregbuf.d[1] = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, maddr + 4);
		srcreg = srcregbuf.d;
	}
	dstreg = (UINT32*)(&(FPU_STAT.reg[idx]));
	
	dstregbuf[0] = dstreg[0];
	dstregbuf[1] = srcreg[0];
	dstreg[0] = dstregbuf[0];
	dstreg[1] = dstregbuf[1];
}

#else

/*
 * MMX interface
 */
void
MMX_EMMS(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_MOVD_mm_rm32(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_MOVD_rm32_mm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_MOVQ_mm_mmm64(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_MOVQ_mmm64_mm(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PACKSSWB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PACKSSDW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PACKUSWB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PADDB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PADDW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PADDD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PADDSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PADDSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PADDUSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PADDUSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PAND(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PANDN(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_POR(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PXOR(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PCMPEQB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PCMPEQW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PCMPEQD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PCMPGTB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PCMPGTW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PCMPGTD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PMADDWD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PMULHW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PMULLW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PSLLW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSLLD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSLLQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PSRAW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSRAD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PSRLW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSRLD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSRLQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PSxxW_imm8(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSxxD_imm8(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSxxQ_imm8(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PSUBB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSUBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSUBD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PSUBSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSUBSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PSUBUSB(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PSUBUSW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}

void MMX_PUNPCKHBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PUNPCKHWD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PUNPCKHDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PUNPCKLBW(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PUNPCKLWD(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
void MMX_PUNPCKLDQ(void)
{
	EXCEPTION(UD_EXCEPTION, 0);
}
#endif