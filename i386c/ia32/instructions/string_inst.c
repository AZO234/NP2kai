/*
 * Copyright (c) 2003 NONAKA Kimihiro
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

#include "string_inst.h"


/* movs */
void
MOVSB_XbYb(void)
{
	UINT8 tmp;

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		tmp = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_DI, tmp);
		CPU_SI += STRING_DIR;
		CPU_DI += STRING_DIR;
	} else {
		tmp = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_EDI, tmp);
		CPU_ESI += STRING_DIR;
		CPU_EDI += STRING_DIR;
	}
}

void
MOVSW_XwYw(void)
{
	UINT16 tmp;

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		tmp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_DI, tmp);
		CPU_SI += STRING_DIRx2;
		CPU_DI += STRING_DIRx2;
	} else {
		tmp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_EDI, tmp);
		CPU_ESI += STRING_DIRx2;
		CPU_EDI += STRING_DIRx2;
	}
}

void
MOVSD_XdYd(void)
{
	UINT32 tmp;

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		tmp = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_DI, tmp);
		CPU_SI += STRING_DIRx4;
		CPU_DI += STRING_DIRx4;
	} else {
		tmp = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_EDI, tmp);
		CPU_ESI += STRING_DIRx4;
		CPU_EDI += STRING_DIRx4;
	}
}


/* cmps */
void
CMPSB_XbYb(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(8);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		dst = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_DI);
		BYTE_SUB(res, dst, src);
		CPU_SI += STRING_DIR;
		CPU_DI += STRING_DIR;
	} else {
		dst = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_EDI);
		BYTE_SUB(res, dst, src);
		CPU_ESI += STRING_DIR;
		CPU_EDI += STRING_DIR;
	}
}

void
CMPSW_XwYw(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(8);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_DI);
		WORD_SUB(res, dst, src);
		CPU_SI += STRING_DIRx2;
		CPU_DI += STRING_DIRx2;
	} else {
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_EDI);
		WORD_SUB(res, dst, src);
		CPU_ESI += STRING_DIRx2;
		CPU_EDI += STRING_DIRx2;
	}
}

void
CMPSD_XdYd(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(8);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_DI);
		DWORD_SUB(res, dst, src);
		CPU_SI += STRING_DIRx4;
		CPU_DI += STRING_DIRx4;
	} else {
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_EDI);
		DWORD_SUB(res, dst, src);
		CPU_ESI += STRING_DIRx4;
		CPU_EDI += STRING_DIRx4;
	}
}


/* scas */
void
SCASB_ALXb(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(7);
	dst = CPU_AL;
	if (!CPU_INST_AS32) {
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_DI);
		BYTE_SUB(res, dst, src);
		CPU_DI += STRING_DIR;
	} else {
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_EDI);
		BYTE_SUB(res, dst, src);
		CPU_EDI += STRING_DIR;
	}
}

void
SCASW_AXXw(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(7);
	dst = CPU_AX;
	if (!CPU_INST_AS32) {
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_DI);
		WORD_SUB(res, dst, src);
		CPU_DI += STRING_DIRx2;
	} else {
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_EDI);
		WORD_SUB(res, dst, src);
		CPU_EDI += STRING_DIRx2;
	}
}

void
SCASD_EAXXd(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(7);
	dst = CPU_EAX;
	if (!CPU_INST_AS32) {
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_DI);
		DWORD_SUB(res, dst, src);
		CPU_DI += STRING_DIRx4;
	} else {
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_EDI);
		DWORD_SUB(res, dst, src);
		CPU_EDI += STRING_DIRx4;
	}
}


/* lods */
void
LODSB_ALXb(void)
{

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		CPU_AL = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);
		CPU_SI += STRING_DIR;
	} else {
		CPU_AL = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);
		CPU_ESI += STRING_DIR;
	}
}

void
LODSW_AXXw(void)
{

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		CPU_AX = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);
		CPU_SI += STRING_DIRx2;
	} else {
		CPU_AX = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);
		CPU_ESI += STRING_DIRx2;
	}
}

void
LODSD_EAXXd(void)
{

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		CPU_EAX = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);
		CPU_SI += STRING_DIRx4;
	} else {
		CPU_EAX = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);
		CPU_ESI += STRING_DIRx4;
	}
}


/* stos */
void
STOSB_YbAL(void)
{

	CPU_WORKCLOCK(3);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_DI, CPU_AL);
		CPU_DI += STRING_DIR;
	} else {
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_EDI, CPU_AL);
		CPU_EDI += STRING_DIR;
	}
}

void
STOSW_YwAX(void)
{

	CPU_WORKCLOCK(3);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_DI, CPU_AX);
		CPU_DI += STRING_DIRx2;
	} else {
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_EDI, CPU_AX);
		CPU_EDI += STRING_DIRx2;
	}
}

void
STOSD_YdEAX(void)
{

	CPU_WORKCLOCK(3);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_DI, CPU_EAX);
		CPU_DI += STRING_DIRx4;
	} else {
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_EDI, CPU_EAX);
		CPU_EDI += STRING_DIRx4;
	}
}


/* repeat */
void
_REPNE(void)
{

	CPU_INST_REPUSE = 0xf2;
}

void
_REPE(void)
{

	CPU_INST_REPUSE = 0xf3;
}


/* ins */
void
INSB_YbDX(void)
{
	UINT8 data;

	CPU_WORKCLOCK(12);
	data = cpu_in(CPU_DX);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_DI, data);
		CPU_DI += STRING_DIR;
	} else {
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_EDI, data);
		CPU_EDI += STRING_DIR;
	}
}

void
INSW_YwDX(void)
{
	UINT16 data;

	CPU_WORKCLOCK(12);
	data = cpu_in_w(CPU_DX);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_DI, data);
		CPU_DI += STRING_DIRx2;
	} else {
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_EDI, data);
		CPU_EDI += STRING_DIRx2;
	}
}

void
INSD_YdDX(void)
{
	UINT32 data;

	CPU_WORKCLOCK(12);
	data = cpu_in_d(CPU_DX);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_DI, data);
		CPU_DI += STRING_DIRx4;
	} else {
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_EDI, data);
		CPU_EDI += STRING_DIRx4;
	}
}


/* outs */
void
OUTSB_DXXb(void)
{
	UINT8 data;

	CPU_WORKCLOCK(14);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		data = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_out(CPU_DX, data);
		CPU_SI += STRING_DIR;
	} else {
		data = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_out(CPU_DX, data);
		CPU_ESI += STRING_DIR;
	}
}

void
OUTSW_DXXw(void)
{
	UINT16 data;

	CPU_WORKCLOCK(14);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		data = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_out_w(CPU_DX, data);
		CPU_SI += STRING_DIRx2;
	} else {
		data = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_out_w(CPU_DX, data);
		CPU_ESI += STRING_DIRx2;
	}
}

void
OUTSD_DXXd(void)
{
	UINT32 data;

	CPU_WORKCLOCK(14);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		data = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_out_d(CPU_DX, data);
		CPU_SI += STRING_DIRx4;
	} else {
		data = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_out_d(CPU_DX, data);
		CPU_ESI += STRING_DIRx4;
	}
}
