/*
 * Copyright (c) 2002-2003 NONAKA Kimihiro
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

#include "misc_inst.h"
#include "ia32/inst_table.h"

#include <pccore.h>

#ifdef USE_SSE2
#include "ia32/instructions/sse2/sse2.h"
#endif

#ifdef SUPPORT_IA32_HAXM
#include <bios/bios.h>
#endif
void
LEA_GwM(void)
{
	UINT16 *out;
	UINT32 op, dst;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg16_b53[op];
		dst = calc_ea_dst(op);
		*out = (UINT16)dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LEA_GdM(void)
{
	UINT32 *out;
	UINT32 op, dst;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg32_b53[op];
		dst = calc_ea_dst(op);
		*out = dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
_NOP(void)
{
#if defined(SUPPORT_IA32_HAXM) && defined(USE_CUSTOM_HOOKINST)
	if(bioshookinfo.hookinst == 0x90)
#endif
	ia32_bioscall();
}

void
UD2(void)
{

	EXCEPTION(UD_EXCEPTION, 0);
}

void
XLAT(void)
{

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		CPU_AL = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_BX + CPU_AL);
	} else {
		CPU_AL = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_EBX + CPU_AL);
	}
}

void
_CPUID(void)
{
	switch (CPU_EAX) {
	case 0:
		CPU_EAX = 1;
		CPU_EBX = LOADINTELDWORD(((UINT8*)(i386cpuid.cpu_vendor+0)));
		CPU_EDX = LOADINTELDWORD(((UINT8*)(i386cpuid.cpu_vendor+4)));
		CPU_ECX = LOADINTELDWORD(((UINT8*)(i386cpuid.cpu_vendor+8)));
		break;

	case 1:
		CPU_EAX = (((i386cpuid.cpu_family >> 4) & 0xff) << 20) | (((i386cpuid.cpu_model >> 4) & 0xf) << 16) | 
			((i386cpuid.cpu_family & 0xf) << 8) | ((i386cpuid.cpu_model & 0xf) << 4) | (i386cpuid.cpu_stepping & 0xf);
		CPU_EBX = i386cpuid.cpu_brandid;
		CPU_ECX = i386cpuid.cpu_feature_ecx & CPU_FEATURES_ECX_ALL;
		CPU_EDX = i386cpuid.cpu_feature & CPU_FEATURES_ALL;
		break;

	case 2:
		if(i386cpuid.cpu_family >= 6){
			CPU_EAX = 0x1;
			CPU_EBX = 0;
			CPU_ECX = 0;
			CPU_EDX = 0x43; // 512KB L2 Cache のふり
		}else{
			CPU_EAX = 0;
			CPU_EBX = 0;
			CPU_ECX = 0;
			CPU_EDX = 0;
		}
		break;
		
	case 0x80000000:
		CPU_EAX = 0x80000004;
		if(strncmp(i386cpuid.cpu_vendor, CPU_VENDOR_AMD, 12)==0){ // AMD判定
			CPU_EBX = LOADINTELDWORD(((UINT8*)(i386cpuid.cpu_vendor+0)));
			CPU_EDX = LOADINTELDWORD(((UINT8*)(i386cpuid.cpu_vendor+4)));
			CPU_ECX = LOADINTELDWORD(((UINT8*)(i386cpuid.cpu_vendor+8)));
		}else{
			CPU_EBX = 0;
			CPU_ECX = 0;
			CPU_EDX = 0;
		}
		break;

	case 0x80000001:
		if(strncmp(i386cpuid.cpu_vendor, CPU_VENDOR_AMD, 12)==0){ // AMD判定
			if(i386cpuid.cpu_family >= 6 || (i386cpuid.cpu_family==5 && i386cpuid.cpu_model >= 6)){
				CPU_EAX = ((i386cpuid.cpu_family+1) << 8) | (i386cpuid.cpu_model << 4) | i386cpuid.cpu_stepping;
			}else{
				CPU_EAX = (i386cpuid.cpu_family << 8) | (i386cpuid.cpu_model << 4) | i386cpuid.cpu_stepping;
			}
		}else{
			CPU_EAX = 0;
		}
		CPU_EBX = 0;
		CPU_ECX = 0;
		CPU_EDX = i386cpuid.cpu_feature_ex & CPU_FEATURES_EX_ALL;
		break;
		
	case 0x80000002:
	case 0x80000003:
	case 0x80000004:
		{
			UINT32 clkMHz;
			char cpu_brandstringbuf[64] = {0};
			int stroffset = (CPU_EAX - 0x80000002) * 16;
			clkMHz = pccore.realclock/1000/1000;
			sprintf(cpu_brandstringbuf, "%s%u MHz", i386cpuid.cpu_brandstring, clkMHz);
			CPU_EAX = LOADINTELDWORD(((UINT8*)(cpu_brandstringbuf + stroffset + 0)));
			CPU_EBX = LOADINTELDWORD(((UINT8*)(cpu_brandstringbuf + stroffset + 4)));
			CPU_ECX = LOADINTELDWORD(((UINT8*)(cpu_brandstringbuf + stroffset + 8)));
			CPU_EDX = LOADINTELDWORD(((UINT8*)(cpu_brandstringbuf + stroffset + 12)));
		}

		break;
	}
}

/* undoc 8086 */
void
SALC(void)
{

	CPU_WORKCLOCK(2);
	CPU_AL = (CPU_FLAGL & C_FLAG) ? 0xff : 0;
}

/* undoc 286 */
void
LOADALL286(void)
{

	ia32_panic("LOADALL286: not implemented yet.");
}

/* undoc 386 */
void
LOADALL(void)
{

	ia32_panic("LOADALL: not implemented yet.");
}

void
OpSize(void)
{

	CPU_INST_OP32 = !CPU_STATSAVE.cpu_inst_default.op_32;
}

void
AddrSize(void)
{

	CPU_INST_AS32 = !CPU_STATSAVE.cpu_inst_default.as_32;
}

void
_2byte_ESC16(void)
{
	UINT32 op;

	GET_PCBYTE(op);
#ifdef USE_SSE
	if(insttable_2byte660F_32[op] && CPU_INST_OP32 == !CPU_STATSAVE.cpu_inst_default.op_32){
		(*insttable_2byte660F_32[op])();
		return;
	}else if(insttable_2byteF20F_32[op] && CPU_INST_REPUSE == 0xf2){
		(*insttable_2byteF20F_32[op])();
		return;
	}else if(insttable_2byteF30F_32[op] && CPU_INST_REPUSE == 0xf3){
		(*insttable_2byteF30F_32[op])();
		return;
	}
#endif
	(*insttable_2byte[0][op])();
}

void
_2byte_ESC32(void)
{
	UINT32 op;

	GET_PCBYTE(op);
#ifdef USE_SSE
	if(insttable_2byte660F_32[op] && CPU_INST_OP32 == !CPU_STATSAVE.cpu_inst_default.op_32){
		(*insttable_2byte660F_32[op])();
		return;
	}else if(insttable_2byteF20F_32[op] && CPU_INST_REPUSE == 0xf2){
		(*insttable_2byteF20F_32[op])();
		return;
	}else if(insttable_2byteF30F_32[op] && CPU_INST_REPUSE == 0xf3){
		(*insttable_2byteF30F_32[op])();
		return;
	}
#endif
	(*insttable_2byte[1][op])();
}

void
Prefix_ES(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_ES_INDEX;
}

void
Prefix_CS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_CS_INDEX;
}

void
Prefix_SS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_SS_INDEX;
}

void
Prefix_DS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_DS_INDEX;
}

void
Prefix_FS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_FS_INDEX;
}

void
Prefix_GS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_GS_INDEX;
}
//
//void
//_2byte_Prefix660F_32(void)
//{
//#ifdef USE_SSE2
//	UINT32 op;
//
//	GET_PCBYTE(op);
//	if(op==0x0f){
//		GET_PCBYTE(op);
//		(*insttable_2byte660F_32[op])();
//	}else{
//		EXCEPTION(UD_EXCEPTION, 0);
//	}
//#else
//	EXCEPTION(UD_EXCEPTION, 0);
//#endif
//}
//void
//_2byte_PrefixF20F_32(void)
//{
//#ifdef USE_SSE2
//	UINT32 op;
//
//	GET_PCBYTE(op);
//	if(op==0x0f){
//		GET_PCBYTE(op);
//		(*insttable_2byteF20F_32[op])();
//	}else{
//		EXCEPTION(UD_EXCEPTION, 0);
//	}
//#else
//	EXCEPTION(UD_EXCEPTION, 0);
//#endif
//}
//void
//_2byte_PrefixF30F_32(void)
//{
//#ifdef USE_SSE
//	UINT32 op;
//
//	GET_PCBYTE(op);
//	if(op==0x0f){
//		GET_PCBYTE(op);
//		(*insttable_2byteF30F_32[op])();
//#ifdef USE_SSE2
//	}else if(op==0x90){
//		SSE2_PAUSE();
//#endif
//	}else{
//		EXCEPTION(UD_EXCEPTION, 0);
//	}
//#else
//	EXCEPTION(UD_EXCEPTION, 0);
//#endif
//}
