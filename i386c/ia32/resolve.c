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
#include "cpu.h"
#include "ia32.mcr"


UINT32 (*calc_ea_dst_tbl[0x100])(void);
UINT32 (*calc_ea32_dst_tbl[0x100])(void);


/*
 * common
 */
static UINT32
ea_nop(void)
{

	ia32_panic("ea_nop");
	return 0;
}


/*
 * 16bit
 */

/*
 * ea_dest
 */
static UINT32
ea_bx_si(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (CPU_BX + CPU_SI);
}

static UINT32
ea_bx_si_disp8(void)
{
	UINT32 adrs;

	GET_PCBYTES(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_BX + CPU_SI);
}

static UINT32
ea_bx_si_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_BX + CPU_SI);
}

static UINT32
ea_bx_di(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (CPU_BX + CPU_DI);
}

static UINT32
ea_bx_di_disp8(void)
{
	UINT32 adrs;

	GET_PCBYTES(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_BX + CPU_DI);
}

static UINT32
ea_bx_di_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_BX + CPU_DI);
}

static UINT32
ea_bp_si(void)
{

	CPU_INST_SEGREG_INDEX = SS_FIX;
	return (CPU_BP + CPU_SI);
}

static UINT32
ea_bp_si_disp8(void)
{
	UINT32 adrs;

	GET_PCBYTES(adrs);
	CPU_INST_SEGREG_INDEX = SS_FIX;
	return (adrs + CPU_BP + CPU_SI);
}

static UINT32
ea_bp_si_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = SS_FIX;
	return (adrs + CPU_BP + CPU_SI);
}

static UINT32
ea_bp_di(void)
{

	CPU_INST_SEGREG_INDEX = SS_FIX;
	return (CPU_BP + CPU_DI);
}

static UINT32
ea_bp_di_disp8(void)
{
	UINT32 adrs;

	GET_PCBYTES(adrs);
	CPU_INST_SEGREG_INDEX = SS_FIX;
	return (adrs + CPU_BP + CPU_DI);
}

static UINT32
ea_bp_di_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = SS_FIX;
	return (adrs + CPU_BP + CPU_DI);
}

static UINT32
ea_si(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_SI;
}

static UINT32
ea_si_disp8(void)
{
	UINT32 adrs;

	GET_PCBYTES(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_SI);
}

static UINT32
ea_si_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_SI);
}

static UINT32
ea_di(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_DI;
}

static UINT32
ea_di_disp8(void)
{
	UINT32 adrs;

	GET_PCBYTES(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_DI);
}

static UINT32
ea_di_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_DI);
}

static UINT32
ea_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs;
}

static UINT32
ea_bp_disp8(void)
{
	UINT32 adrs;

	GET_PCBYTES(adrs);
	CPU_INST_SEGREG_INDEX = SS_FIX;
	return (adrs + CPU_BP);
}

static UINT32
ea_bp_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = SS_FIX;
	return (adrs + CPU_BP);
}

static UINT32
ea_bx(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_BX;
}

static UINT32
ea_bx_disp8(void)
{
	UINT32 adrs;

	GET_PCBYTES(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_BX);
}

static UINT32
ea_bx_disp16(void)
{
	UINT32 adrs;

	GET_PCWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return (adrs + CPU_BX);
}

static UINT32 (*c_ea_dst_tbl[])(void) = {
	ea_bx_si,		ea_bx_di,
	ea_bp_si,		ea_bp_di,
	ea_si,			ea_di,
	ea_disp16,		ea_bx,
	ea_bx_si_disp8,		ea_bx_di_disp8,
	ea_bp_si_disp8,		ea_bp_di_disp8,
	ea_si_disp8,		ea_di_disp8,
	ea_bp_disp8,		ea_bx_disp8,
	ea_bx_si_disp16,	ea_bx_di_disp16,
	ea_bp_si_disp16,	ea_bp_di_disp16,
	ea_si_disp16,		ea_di_disp16,
	ea_bp_disp16,		ea_bx_disp16,
};


/*
 * 32bit
 */
/*
 * ea_dest 32
 */
static UINT32
ea32_eax(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_EAX;
}

static UINT32
ea32_ecx(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_ECX;
}

static UINT32
ea32_edx(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_EDX;
}

static UINT32
ea32_ebx(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_EBX;
}

static UINT32
ea32_sib(void)
{
	UINT32 dst;
	UINT32 op;
	UINT32 base, idx, scale;

	GET_PCBYTE(op);
	base = op & 7;
	idx = (op >> 3) & 7;
	scale = (op >> 6) & 3;

	switch (base) {
	case 0: case 1: case 2: case 3: case 6: case 7:
		CPU_INST_SEGREG_INDEX = DS_FIX;
		dst = CPU_REGS_DWORD(base);
		break;

	case 4:
		CPU_INST_SEGREG_INDEX = SS_FIX;
		dst = CPU_ESP;
		break;

	case 5:
		CPU_INST_SEGREG_INDEX = DS_FIX;
		GET_PCDWORD(dst);
		break;

	default:
		dst = 0;	/* compiler happy */
		ia32_panic("ea32_sib: invalid base = %d", base);
		break;
	}
	if (idx != 4)
		dst += CPU_REGS_DWORD(idx) << scale;
	return dst;
}

static UINT32
ea32_disp32(void)
{
	UINT32 adrs;

	GET_PCDWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs;
}

static UINT32
ea32_esi(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_ESI;
}

static UINT32
ea32_edi(void)
{

	CPU_INST_SEGREG_INDEX = DS_FIX;
	return CPU_EDI;
}

static UINT32
ea32_eax_disp8(void)
{
	SINT32 adrs;

	GET_PCBYTESD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_EAX;
}

static UINT32
ea32_ecx_disp8(void)
{
	SINT32 adrs;

	GET_PCBYTESD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_ECX;
}

static UINT32
ea32_edx_disp8(void)
{
	SINT32 adrs;

	GET_PCBYTESD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_EDX;
}

static UINT32
ea32_ebx_disp8(void)
{
	SINT32 adrs;

	GET_PCBYTESD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_EBX;
}

static UINT32
ea32_sib_disp8(void)
{
	SINT32 adrs;
	UINT32 op;
	UINT32 base, idx, scale;

	GET_PCBYTE(op);
	base = op & 7;
	idx = (op >> 3) & 7;
	scale = (op >> 6) & 3;

	GET_PCBYTESD(adrs);

	switch (base) {
	case 0: case 1: case 2: case 3: case 6: case 7:
		CPU_INST_SEGREG_INDEX = DS_FIX;
		break;

	case 4: case 5:
		CPU_INST_SEGREG_INDEX = SS_FIX;
		break;
	}
	if (idx != 4)
		adrs += CPU_REGS_DWORD(idx) << scale;
	return CPU_REGS_DWORD(base) + adrs;
}

static UINT32
ea32_ebp_disp8(void)
{
	SINT32 adrs;

	GET_PCBYTESD(adrs);
	CPU_INST_SEGREG_INDEX = SS_FIX;
	return adrs + CPU_EBP;
}

static UINT32
ea32_esi_disp8(void)
{
	SINT32 adrs;

	GET_PCBYTESD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_ESI;
}

static UINT32
ea32_edi_disp8(void)
{
	SINT32 adrs;

	GET_PCBYTESD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_EDI;
}

static UINT32
ea32_eax_disp32(void)
{
	UINT32 adrs;

	GET_PCDWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_EAX;
}

static UINT32
ea32_ecx_disp32(void)
{
	UINT32 adrs;

	GET_PCDWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_ECX;
}

static UINT32
ea32_edx_disp32(void)
{
	UINT32 adrs;

	GET_PCDWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_EDX;
}

static UINT32
ea32_ebx_disp32(void)
{
	UINT32 adrs;

	GET_PCDWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_EBX;
}

static UINT32
ea32_sib_disp32(void)
{
	UINT32 adrs;
	UINT32 op;
	UINT32 base, idx, scale;

	GET_PCBYTE(op);
	base = op & 7;
	idx = (op >> 3) & 7;
	scale = (op >> 6) & 3;

	GET_PCDWORD(adrs);

	switch (base) {
	case 0: case 1: case 2: case 3: case 6: case 7:
		CPU_INST_SEGREG_INDEX = DS_FIX;
		break;

	case 4: case 5:
		CPU_INST_SEGREG_INDEX = SS_FIX;
		break;
	}
	if (idx != 4)
		adrs += CPU_REGS_DWORD(idx) << scale;
	return CPU_REGS_DWORD(base) + adrs;
}

static UINT32
ea32_ebp_disp32(void)
{
	UINT32 adrs;

	GET_PCDWORD(adrs);
	CPU_INST_SEGREG_INDEX = SS_FIX;
	return adrs + CPU_EBP;
}

static UINT32
ea32_esi_disp32(void)
{
	UINT32 adrs;

	GET_PCDWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_ESI;
}

static UINT32
ea32_edi_disp32(void)
{
	UINT32 adrs;

	GET_PCDWORD(adrs);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	return adrs + CPU_EDI;
}

static UINT32 (*c_ea32_dst_tbl[])(void) = {
	ea32_eax,		ea32_ecx, 
	ea32_edx,		ea32_ebx,
	ea32_sib,		ea32_disp32,
	ea32_esi,		ea32_edi,
	ea32_eax_disp8,		ea32_ecx_disp8, 
	ea32_edx_disp8,		ea32_ebx_disp8,
	ea32_sib_disp8,		ea32_ebp_disp8,
	ea32_esi_disp8,		ea32_edi_disp8,
	ea32_eax_disp32,	ea32_ecx_disp32, 
	ea32_edx_disp32,	ea32_ebx_disp32,
	ea32_sib_disp32,	ea32_ebp_disp32,
	ea32_esi_disp32,	ea32_edi_disp32,
};


/*
 * init table
 */
void
resolve_init(void)
{
	int i, pos;

	for (i = 0; i < 0xc0; ++i) {
		pos = ((i >> 3) & 0x18) + (i & 0x07);
		calc_ea_dst_tbl[i] = c_ea_dst_tbl[pos];
		calc_ea32_dst_tbl[i] = c_ea32_dst_tbl[pos];
	}
	for (; i < 0x100; ++i) {
		calc_ea_dst_tbl[i] = ea_nop;
		calc_ea32_dst_tbl[i] = ea_nop;
	}
}
