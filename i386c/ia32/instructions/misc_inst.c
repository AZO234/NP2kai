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

#include "compiler.h"
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "misc_inst.h"
#include "ia32/inst_table.h"


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
		CPU_EBX = CPU_VENDOR_1;
		CPU_EDX = CPU_VENDOR_2;
		CPU_ECX = CPU_VENDOR_3;
		break;

	case 1:
		CPU_EAX = (CPU_FAMILY << 8) | (CPU_MODEL << 4) | CPU_STEPPING;
		CPU_EBX = 0;
		CPU_ECX = 0;
		CPU_EDX = CPU_FEATURES;
		break;

	case 2:
		CPU_EAX = 0;
		CPU_EBX = 0;
		CPU_ECX = 0;
		CPU_EDX = 0;
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
	(*insttable_2byte[0][op])();
}

void
_2byte_ESC32(void)
{
	UINT32 op;

	GET_PCBYTE(op);
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
