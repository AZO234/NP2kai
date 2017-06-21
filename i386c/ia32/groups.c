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
#include "groups.h"
#include "inst_table.h"


/* group 1 */
void
Grp1_EbIb(void)
{
	UINT8 *out;
	UINT32 madr;
	UINT32 op, src;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg8_b20[op];
		GET_PCBYTE(src);
		(*insttable_G1EbIb[idx])(out, src);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		(*insttable_G1EbIb_ext[idx])(madr, src);
	}
}

void
Grp1_EwIb(void)
{
	UINT16 *out;
	UINT32 madr, src;
	UINT32 op;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg16_b20[op];
		GET_PCBYTES(src);
		(*insttable_G1EwIx[idx])(out, src);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		GET_PCBYTES(src);
		(*insttable_G1EwIx_ext[idx])(madr, src);
	}
}

void
Grp1_EdIb(void)
{
	UINT32 *out;
	UINT32 madr, src;
	UINT32 op;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg32_b20[op];
		GET_PCBYTESD(src);
		(*insttable_G1EdIx[idx])(out, src);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		GET_PCBYTESD(src);
		(*insttable_G1EdIx_ext[idx])(madr, src);
	}
}

void
Grp1_EwIw(void)
{
	UINT16 *out;
	UINT32 madr, src;
	UINT32 op;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg16_b20[op];
		GET_PCWORD(src);
		(*insttable_G1EwIx[idx])(out, src);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		GET_PCWORD(src);
		(*insttable_G1EwIx_ext[idx])(madr, src);
	}
}

void
Grp1_EdId(void)
{
	UINT32 *out;
	UINT32 madr, src;
	UINT32 op;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg32_b20[op];
		GET_PCDWORD(src);
		(*insttable_G1EdIx[idx])(out, src);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		GET_PCDWORD(src);
		(*insttable_G1EdIx_ext[idx])(madr, src);
	}
}


/* group 2 */
void
Grp2_EbIb(void)
{
	UINT8 *out;
	UINT32 madr;
	UINT32 op;
	UINT32 cl;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(5);
		out = reg8_b20[op];
		GET_PCBYTE(cl);
		CPU_WORKCLOCK(cl & 0x1f);
		(*insttable_G2EbCL[idx])(out, cl);
	} else {
		CPU_WORKCLOCK(8);
		madr = calc_ea_dst(op);
		GET_PCBYTE(cl);
		CPU_WORKCLOCK(cl & 0x1f);
		(*insttable_G2EbCL_ext[idx])(madr, cl);
	}
}

void
Grp2_EwIb(void)
{
	UINT16 *out;
	UINT32 madr;
	UINT32 op;
	UINT32 cl;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(5);
		out = reg16_b20[op];
		GET_PCBYTE(cl);
		CPU_WORKCLOCK(cl & 0x1f);
		(*insttable_G2EwCL[idx])(out, cl);
	} else {
		CPU_WORKCLOCK(8);
		madr = calc_ea_dst(op);
		GET_PCBYTE(cl);
		CPU_WORKCLOCK(cl & 0x1f);
		(*insttable_G2EwCL_ext[idx])(madr, cl);
	}
}

void
Grp2_EdIb(void)
{
	UINT32 *out;
	UINT32 madr;
	UINT32 op;
	UINT32 cl;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(5);
		out = reg32_b20[op];
		GET_PCBYTE(cl);
		CPU_WORKCLOCK(cl & 0x1f);
		(*insttable_G2EdCL[idx])(out, cl);
	} else {
		CPU_WORKCLOCK(8);
		madr = calc_ea_dst(op);
		GET_PCBYTE(cl);
		CPU_WORKCLOCK(cl & 0x1f);
		(*insttable_G2EdCL_ext[idx])(madr, cl);
	}
}

void
Grp2_Eb(void)
{
	UINT32 op;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		(*insttable_G2Eb[idx])(reg8_b20[op]);
	} else {
		CPU_WORKCLOCK(7);
		(*insttable_G2Eb_ext[idx])(calc_ea_dst(op));
	}
}

void
Grp2_Ew(void)
{
	UINT32 op;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		(*insttable_G2Ew[idx])(reg16_b20[op]);
	} else {
		CPU_WORKCLOCK(7);
		(*insttable_G2Ew_ext[idx])(calc_ea_dst(op));
	}
}

void
Grp2_Ed(void)
{
	UINT32 op;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		(*insttable_G2Ed[idx])(reg32_b20[op]);
	} else {
		CPU_WORKCLOCK(7);
		(*insttable_G2Ed_ext[idx])(calc_ea_dst(op));
	}
}

void
Grp2_EbCL(void)
{
	UINT8 *out;
	UINT32 madr;
	UINT32 op;
	UINT32 cl;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(5);
		out = reg8_b20[op];
		cl = CPU_CL;
		cl &= 0x1f;
		CPU_WORKCLOCK(cl);
		(*insttable_G2EbCL[idx])(out, cl);
	} else {
		CPU_WORKCLOCK(8);
		madr = calc_ea_dst(op);
		cl = CPU_CL;
		cl &= 0x1f;
		CPU_WORKCLOCK(cl);
		(*insttable_G2EbCL_ext[idx])(madr, cl);
	}
}

void
Grp2_EwCL(void)
{
	UINT16 *out;
	UINT32 madr;
	UINT32 op;
	UINT32 cl;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(5);
		out = reg16_b20[op];
		cl = CPU_CL;
		cl &= 0x1f;
		CPU_WORKCLOCK(cl);
		(*insttable_G2EwCL[idx])(out, cl);
	} else {
		CPU_WORKCLOCK(8);
		madr = calc_ea_dst(op);
		cl = CPU_CL;
		cl &= 0x1f;
		CPU_WORKCLOCK(cl);
		(*insttable_G2EwCL_ext[idx])(madr, cl);
	}
}

void
Grp2_EdCL(void)
{
	UINT32 *out;
	UINT32 madr;
	UINT32 op;
	UINT32 cl;
	int idx;

	GET_PCBYTE(op);
	idx = (op >> 3) & 7;
	if (op >= 0xc0) {
		CPU_WORKCLOCK(5);
		out = reg32_b20[op];
		cl = CPU_CL;
		cl &= 0x1f;
		CPU_WORKCLOCK(cl);
		(*insttable_G2EdCL[idx])(out, cl);
	} else {
		CPU_WORKCLOCK(8);
		madr = calc_ea_dst(op);
		cl = CPU_CL;
		cl &= 0x1f;
		CPU_WORKCLOCK(cl);
		(*insttable_G2EdCL_ext[idx])(madr, cl);
	}
}


/* group 3 */
void
Grp3_Eb(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G3Eb[(op >> 3) & 7])(op);
}

void
Grp3_Ew(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G3Ew[(op >> 3) & 7])(op);
}

void
Grp3_Ed(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G3Ed[(op >> 3) & 7])(op);
}


/* group 4 */
void
Grp4(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G4[(op >> 3) & 7])(op);
}


/* group 5 */
void
Grp5_Ew(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G5Ew[(op >> 3) & 7])(op);
}

void
Grp5_Ed(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G5Ed[(op >> 3) & 7])(op);
}


/* group 6 */
void
Grp6(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G6[(op >> 3) & 7])(op);
}


/* group 7 */
void
Grp7(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G7[(op >> 3) & 7])(op);
}


/* group 8 */
void
Grp8_EwIb(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G8EwIb[(op >> 3) & 7])(op);
}

void
Grp8_EdIb(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G8EdIb[(op >> 3) & 7])(op);
}


/* group 9 */
void
Grp9(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_G9[(op >> 3) & 7])(op);
}
