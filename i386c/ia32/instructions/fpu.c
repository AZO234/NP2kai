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

#include "compiler.h"
#include "pccore.h"
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"
#include "ia32/inst_table.h"

#include "ia32/instructions/fpu/fp.h"
#include "ia32/instructions/fpu/fpumem.h"

#if defined(USE_FPU) && !defined(SUPPORT_FPU_DOSBOX) && !defined(SUPPORT_FPU_DOSBOX2) && !defined(SUPPORT_FPU_SOFTFLOAT)
#error No FPU detected. Please define SUPPORT_FPU_DOSBOX, SUPPORT_FPU_DOSBOX2 or SUPPORT_FPU_SOFTFLOAT.
#endif

void
fpu_initialize(void)
{
#if defined(USE_FPU)
	if(i386cpuid.cpu_feature & CPU_FEATURE_FPU){
		switch(i386cpuid.fpu_type){
#if defined(SUPPORT_FPU_DOSBOX)
		case FPU_TYPE_DOSBOX:
			insttable_2byte[0][0xae] = insttable_2byte[1][0xae] = DB_FPU_FXSAVERSTOR;
			insttable_1byte[0][0xd8] = insttable_1byte[1][0xd8] = DB_ESC0;
			insttable_1byte[0][0xd9] = insttable_1byte[1][0xd9] = DB_ESC1;
			insttable_1byte[0][0xda] = insttable_1byte[1][0xda] = DB_ESC2;
			insttable_1byte[0][0xdb] = insttable_1byte[1][0xdb] = DB_ESC3;
			insttable_1byte[0][0xdc] = insttable_1byte[1][0xdc] = DB_ESC4;
			insttable_1byte[0][0xdd] = insttable_1byte[1][0xdd] = DB_ESC5;
			insttable_1byte[0][0xde] = insttable_1byte[1][0xde] = DB_ESC6;
			insttable_1byte[0][0xdf] = insttable_1byte[1][0xdf] = DB_ESC7;
			DB_FPU_FINIT();
			break;
#endif
#if defined(SUPPORT_FPU_DOSBOX2)
		case FPU_TYPE_DOSBOX2:
			insttable_2byte[0][0xae] = insttable_2byte[1][0xae] = DB2_FPU_FXSAVERSTOR;
			insttable_1byte[0][0xd8] = insttable_1byte[1][0xd8] = DB2_ESC0;
			insttable_1byte[0][0xd9] = insttable_1byte[1][0xd9] = DB2_ESC1;
			insttable_1byte[0][0xda] = insttable_1byte[1][0xda] = DB2_ESC2;
			insttable_1byte[0][0xdb] = insttable_1byte[1][0xdb] = DB2_ESC3;
			insttable_1byte[0][0xdc] = insttable_1byte[1][0xdc] = DB2_ESC4;
			insttable_1byte[0][0xdd] = insttable_1byte[1][0xdd] = DB2_ESC5;
			insttable_1byte[0][0xde] = insttable_1byte[1][0xde] = DB2_ESC6;
			insttable_1byte[0][0xdf] = insttable_1byte[1][0xdf] = DB2_ESC7;
			DB2_FPU_FINIT();
			break;
#endif
#if defined(SUPPORT_FPU_SOFTFLOAT)
		case FPU_TYPE_SOFTFLOAT:
			insttable_2byte[0][0xae] = insttable_2byte[1][0xae] = SF_FPU_FXSAVERSTOR;
			insttable_1byte[0][0xd8] = insttable_1byte[1][0xd8] = SF_ESC0;
			insttable_1byte[0][0xd9] = insttable_1byte[1][0xd9] = SF_ESC1;
			insttable_1byte[0][0xda] = insttable_1byte[1][0xda] = SF_ESC2;
			insttable_1byte[0][0xdb] = insttable_1byte[1][0xdb] = SF_ESC3;
			insttable_1byte[0][0xdc] = insttable_1byte[1][0xdc] = SF_ESC4;
			insttable_1byte[0][0xdd] = insttable_1byte[1][0xdd] = SF_ESC5;
			insttable_1byte[0][0xde] = insttable_1byte[1][0xde] = SF_ESC6;
			insttable_1byte[0][0xdf] = insttable_1byte[1][0xdf] = SF_ESC7;
			SF_FPU_FINIT();
			break;
#endif
		default:
#if defined(SUPPORT_FPU_SOFTFLOAT)
			insttable_2byte[0][0xae] = insttable_2byte[1][0xae] = SF_FPU_FXSAVERSTOR;
			insttable_1byte[0][0xd8] = insttable_1byte[1][0xd8] = SF_ESC0;
			insttable_1byte[0][0xd9] = insttable_1byte[1][0xd9] = SF_ESC1;
			insttable_1byte[0][0xda] = insttable_1byte[1][0xda] = SF_ESC2;
			insttable_1byte[0][0xdb] = insttable_1byte[1][0xdb] = SF_ESC3;
			insttable_1byte[0][0xdc] = insttable_1byte[1][0xdc] = SF_ESC4;
			insttable_1byte[0][0xdd] = insttable_1byte[1][0xdd] = SF_ESC5;
			insttable_1byte[0][0xde] = insttable_1byte[1][0xde] = SF_ESC6;
			insttable_1byte[0][0xdf] = insttable_1byte[1][0xdf] = SF_ESC7;
			SF_FPU_FINIT();
#elif defined(SUPPORT_FPU_DOSBOX)
			insttable_2byte[0][0xae] = insttable_2byte[1][0xae] = DB_FPU_FXSAVERSTOR;
			insttable_1byte[0][0xd8] = insttable_1byte[1][0xd8] = DB_ESC0;
			insttable_1byte[0][0xd9] = insttable_1byte[1][0xd9] = DB_ESC1;
			insttable_1byte[0][0xda] = insttable_1byte[1][0xda] = DB_ESC2;
			insttable_1byte[0][0xdb] = insttable_1byte[1][0xdb] = DB_ESC3;
			insttable_1byte[0][0xdc] = insttable_1byte[1][0xdc] = DB_ESC4;
			insttable_1byte[0][0xdd] = insttable_1byte[1][0xdd] = DB_ESC5;
			insttable_1byte[0][0xde] = insttable_1byte[1][0xde] = DB_ESC6;
			insttable_1byte[0][0xdf] = insttable_1byte[1][0xdf] = DB_ESC7;
			DB_FPU_FINIT();
#elif defined(SUPPORT_FPU_DOSBOX2)
			insttable_2byte[0][0xae] = insttable_2byte[1][0xae] = DB2_FPU_FXSAVERSTOR;
			insttable_1byte[0][0xd8] = insttable_1byte[1][0xd8] = DB2_ESC0;
			insttable_1byte[0][0xd9] = insttable_1byte[1][0xd9] = DB2_ESC1;
			insttable_1byte[0][0xda] = insttable_1byte[1][0xda] = DB2_ESC2;
			insttable_1byte[0][0xdb] = insttable_1byte[1][0xdb] = DB2_ESC3;
			insttable_1byte[0][0xdc] = insttable_1byte[1][0xdc] = DB2_ESC4;
			insttable_1byte[0][0xdd] = insttable_1byte[1][0xdd] = DB2_ESC5;
			insttable_1byte[0][0xde] = insttable_1byte[1][0xde] = DB2_ESC6;
			insttable_1byte[0][0xdf] = insttable_1byte[1][0xdf] = DB2_ESC7;
			DB2_FPU_FINIT();
#else
			insttable_2byte[0][0xae] = insttable_2byte[1][0xae] = NOFPU_FPU_FXSAVERSTOR;
			insttable_1byte[0][0xd8] = insttable_1byte[1][0xd8] = NOFPU_ESC0;
			insttable_1byte[0][0xd9] = insttable_1byte[1][0xd9] = NOFPU_ESC1;
			insttable_1byte[0][0xda] = insttable_1byte[1][0xda] = NOFPU_ESC2;
			insttable_1byte[0][0xdb] = insttable_1byte[1][0xdb] = NOFPU_ESC3;
			insttable_1byte[0][0xdc] = insttable_1byte[1][0xdc] = NOFPU_ESC4;
			insttable_1byte[0][0xdd] = insttable_1byte[1][0xdd] = NOFPU_ESC5;
			insttable_1byte[0][0xde] = insttable_1byte[1][0xde] = NOFPU_ESC6;
			insttable_1byte[0][0xdf] = insttable_1byte[1][0xdf] = NOFPU_ESC7;
			NOFPU_FPU_FINIT();
#endif
			break;
		}
	}else{
#endif
		insttable_2byte[0][0xae] = insttable_2byte[1][0xae] = NOFPU_FPU_FXSAVERSTOR;
		insttable_1byte[0][0xd8] = insttable_1byte[1][0xd8] = NOFPU_ESC0;
		insttable_1byte[0][0xd9] = insttable_1byte[1][0xd9] = NOFPU_ESC1;
		insttable_1byte[0][0xda] = insttable_1byte[1][0xda] = NOFPU_ESC2;
		insttable_1byte[0][0xdb] = insttable_1byte[1][0xdb] = NOFPU_ESC3;
		insttable_1byte[0][0xdc] = insttable_1byte[1][0xdc] = NOFPU_ESC4;
		insttable_1byte[0][0xdd] = insttable_1byte[1][0xdd] = NOFPU_ESC5;
		insttable_1byte[0][0xde] = insttable_1byte[1][0xde] = NOFPU_ESC6;
		insttable_1byte[0][0xdf] = insttable_1byte[1][0xdf] = NOFPU_ESC7;
		NOFPU_FPU_FINIT();
#if defined(USE_FPU)
	}
#endif
}

char *
fpu_reg2str(void)
{
	return NULL;
}

/*
 * FPU memory access function
 */
#if defined(USE_FPU)
UINT8 MEMCALL
fpu_memoryread_b(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_b(seg, address);
}

UINT16 MEMCALL
fpu_memoryread_w(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_w(seg, address);
}

UINT32 MEMCALL
fpu_memoryread_d(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_d(seg, address);
}

UINT64 MEMCALL
fpu_memoryread_q(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_q(seg, address);
}

REG80 MEMCALL
fpu_memoryread_f(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_f(seg, address);
}

void MEMCALL
fpu_memorywrite_b(UINT32 address, UINT8 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_b(seg, address, value);
}

void MEMCALL
fpu_memorywrite_w(UINT32 address, UINT16 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_w(seg, address, value);
}

void MEMCALL
fpu_memorywrite_d(UINT32 address, UINT32 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_d(seg, address, value);
}

void MEMCALL
fpu_memorywrite_q(UINT32 address, UINT64 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_q(seg, address, value);
}

void MEMCALL
fpu_memorywrite_f(UINT32 address, REG80 *value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_f(seg, address, value);
}
#endif

void
FPU_FWAIT(void)
{
#if defined(USE_FPU)
	// FPUなしなら何もしない
	if(!(i386cpuid.cpu_feature & CPU_FEATURE_FPU)){
		return;
	}
	// タスクスイッチまたはMPでNM(デバイス使用不可例外)を発生させる
	if ((CPU_CR0 & (CPU_CR0_MP|CPU_CR0_TS))==(CPU_CR0_MP|CPU_CR0_TS)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
	
	// Check exception
	if((FPU_STATUSWORD & ~FPU_CTRLWORD) & 0x3F){
		EXCEPTION(MF_EXCEPTION, 0);
	}
#endif
}