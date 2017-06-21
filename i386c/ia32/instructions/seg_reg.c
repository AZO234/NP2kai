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

#include "seg_reg.h"


void
LES_GwMp(void)
{
	UINT16 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg16_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 2);
		LOAD_SEGREG(CPU_ES_INDEX, sreg);
		*out = (UINT16)dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LES_GdMp(void)
{
	UINT32 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg32_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 4);
		LOAD_SEGREG(CPU_ES_INDEX, sreg);
		*out = dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LSS_GwMp(void)
{
	UINT16 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg16_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 2);
		LOAD_SEGREG(CPU_SS_INDEX, sreg);
		*out = (UINT16)dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LSS_GdMp(void)
{
	UINT32 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg32_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 4);
		LOAD_SEGREG(CPU_SS_INDEX, sreg);
		*out = dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LDS_GwMp(void)
{
	UINT16 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg16_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 2);
		LOAD_SEGREG(CPU_DS_INDEX, sreg);
		*out = (UINT16)dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LDS_GdMp(void)
{
	UINT32 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg32_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 4);
		LOAD_SEGREG(CPU_DS_INDEX, sreg);
		*out = dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LFS_GwMp(void)
{
	UINT16 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg16_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 2);
		LOAD_SEGREG(CPU_FS_INDEX, sreg);
		*out = (UINT16)dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LFS_GdMp(void)
{
	UINT32 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg32_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 4);
		LOAD_SEGREG(CPU_FS_INDEX, sreg);
		*out = dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LGS_GwMp(void)
{
	UINT16 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg16_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 2);
		LOAD_SEGREG(CPU_GS_INDEX, sreg);
		*out = (UINT16)dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LGS_GdMp(void)
{
	UINT32 *out;
	UINT32 op, dst, madr;
	UINT16 sreg;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		out = reg32_b53[op];
		madr = calc_ea_dst(op);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		sreg = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 4);
		LOAD_SEGREG(CPU_GS_INDEX, sreg);
		*out = dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}
