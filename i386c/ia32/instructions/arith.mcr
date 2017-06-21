/*
 * Copyright (c) 2004 NONAKA Kimihiro
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

#ifndef	IA32_CPU_ARITH_MCR__
#define	IA32_CPU_ARITH_MCR__

/* args == 1 */
#define	ARITH_INSTRUCTION_1(inst) \
static UINT32 CPUCALL \
inst##1(UINT32 dst, void *arg) \
{ \
	BYTE_##inst(dst); \
	return dst; \
} \
static UINT32 CPUCALL \
inst##2(UINT32 dst, void *arg) \
{ \
	WORD_##inst(dst); \
	return dst; \
} \
static UINT32 CPUCALL \
inst##4(UINT32 dst, void *arg) \
{ \
	DWORD_##inst(dst); \
	return dst; \
} \
\
void CPUCALL \
inst##_Eb(UINT32 op) \
{ \
	UINT8 *out; \
	UINT32 dst, madr; \
\
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg8_b20[op]; \
		dst = *out; \
		BYTE_##inst(dst); \
		*out = (UINT8)dst; \
	} else { \
		CPU_WORKCLOCK(5); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_b(CPU_INST_SEGREG_INDEX, madr, inst##1, 0); \
	} \
} \
\
void CPUCALL \
inst##_Ew(UINT32 op) \
{ \
	UINT16 *out; \
	UINT32 dst, madr; \
\
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg16_b20[op]; \
		dst = *out; \
		WORD_##inst(dst); \
		*out = (UINT16)dst; \
	} else { \
		CPU_WORKCLOCK(5); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_w(CPU_INST_SEGREG_INDEX, madr, inst##2, 0); \
	} \
} \
\
void CPUCALL \
inst##_Ed(UINT32 op) \
{ \
	UINT32 *out; \
	UINT32 dst, madr; \
\
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg32_b20[op]; \
		dst = *out; \
		DWORD_##inst(dst); \
		*out = dst; \
	} else { \
		CPU_WORKCLOCK(5); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_d(CPU_INST_SEGREG_INDEX, madr, inst##4, 0); \
	} \
}

/* args == 2 */
#define	ARITH_INSTRUCTION_2(inst) \
static UINT32 CPUCALL \
inst##1(UINT32 dst, void *arg) \
{ \
	UINT32 src = PTR_TO_UINT32(arg); \
	BYTE_##inst(dst, src); \
	return dst; \
} \
static UINT32 CPUCALL \
inst##2(UINT32 dst, void *arg) \
{ \
	UINT32 src = PTR_TO_UINT32(arg); \
	WORD_##inst(dst, src); \
	return dst; \
} \
static UINT32 CPUCALL \
inst##4(UINT32 dst, void *arg) \
{ \
	UINT32 src = PTR_TO_UINT32(arg); \
	DWORD_##inst(dst, src); \
	return dst; \
} \
\
void \
inst##_EbGb(void) \
{ \
	UINT8 *out; \
	UINT32 op, src, dst, madr; \
\
	PREPART_EA_REG8(op, src); \
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg8_b20[op]; \
		dst = *out; \
		BYTE_##inst(dst, src); \
		*out = (UINT8)dst; \
	} else { \
		CPU_WORKCLOCK(7); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_b(CPU_INST_SEGREG_INDEX, madr, inst##1, UINT32_TO_PTR(src)); \
	} \
} \
\
void \
inst##_EwGw(void) \
{ \
	UINT16 *out; \
	UINT32 op, src, dst, madr; \
\
	PREPART_EA_REG16(op, src); \
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg16_b20[op]; \
		dst = *out; \
		WORD_##inst(dst, src); \
		*out = (UINT16)dst; \
	} else { \
		CPU_WORKCLOCK(7); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_w(CPU_INST_SEGREG_INDEX, madr, inst##2, UINT32_TO_PTR(src)); \
	} \
} \
\
void \
inst##_EdGd(void) \
{ \
	UINT32 *out; \
	UINT32 op, src, dst, madr; \
\
	PREPART_EA_REG32(op, src); \
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg32_b20[op]; \
		dst = *out; \
		DWORD_##inst(dst, src); \
		*out = dst; \
	} else { \
		CPU_WORKCLOCK(7); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_d(CPU_INST_SEGREG_INDEX, madr, inst##4, UINT32_TO_PTR(src)); \
	} \
} \
\
void \
inst##_GbEb(void) \
{ \
	UINT8 *out; \
	UINT32 op, src, dst; \
\
	PREPART_REG8_EA(op, src, out, 2, 7); \
	dst = *out; \
	BYTE_##inst(dst, src); \
	*out = (UINT8)dst; \
} \
\
void \
inst##_GwEw(void) \
{ \
	UINT16 *out; \
	UINT32 op, src, dst; \
\
	PREPART_REG16_EA(op, src, out, 2, 7); \
	dst = *out; \
	WORD_##inst(dst, src); \
	*out = (UINT16)dst; \
} \
\
void \
inst##_GdEd(void) \
{ \
	UINT32 *out; \
	UINT32 op, src, dst; \
\
	PREPART_REG32_EA(op, src, out, 2, 7); \
	dst = *out; \
	DWORD_##inst(dst, src); \
	*out = dst; \
} \
\
void \
inst##_ALIb(void) \
{ \
	UINT32 src, dst; \
\
	CPU_WORKCLOCK(3); \
	GET_PCBYTE(src); \
	dst = CPU_AL; \
	BYTE_##inst(dst, src); \
	CPU_AL = (UINT8)dst; \
} \
\
void \
inst##_AXIw(void) \
{ \
	UINT32 src, dst; \
\
	CPU_WORKCLOCK(3); \
	GET_PCWORD(src); \
	dst = CPU_AX; \
	WORD_##inst(dst, src); \
	CPU_AX = (UINT16)dst; \
} \
\
void \
inst##_EAXId(void) \
{ \
	UINT32 src, dst; \
\
	CPU_WORKCLOCK(3); \
	GET_PCDWORD(src); \
	dst = CPU_EAX; \
	DWORD_##inst(dst, src); \
	CPU_EAX = dst; \
} \
\
void CPUCALL \
inst##_EbIb(UINT8 *regp, UINT32 src) \
{ \
	UINT32 dst; \
\
	dst = *regp; \
	BYTE_##inst(dst, src); \
	*regp = (UINT8)dst; \
} \
\
void CPUCALL \
inst##_EbIb_ext(UINT32 madr, UINT32 src) \
{ \
\
	cpu_vmemory_RMW_b(CPU_INST_SEGREG_INDEX, madr, inst##1, UINT32_TO_PTR(src)); \
} \
\
void CPUCALL \
inst##_EwIx(UINT16 *regp, UINT32 src) \
{ \
	UINT32 dst; \
\
	dst = *regp; \
	WORD_##inst(dst, src); \
	*regp = (UINT16)dst; \
} \
\
void CPUCALL \
inst##_EwIx_ext(UINT32 madr, UINT32 src) \
{ \
\
	cpu_vmemory_RMW_w(CPU_INST_SEGREG_INDEX, madr, inst##2, UINT32_TO_PTR(src)); \
} \
\
void CPUCALL \
inst##_EdIx(UINT32 *regp, UINT32 src) \
{ \
	UINT32 dst; \
\
	dst = *regp; \
	DWORD_##inst(dst, src); \
	*regp = dst; \
} \
\
void CPUCALL \
inst##_EdIx_ext(UINT32 madr, UINT32 src) \
{ \
\
	cpu_vmemory_RMW_d(CPU_INST_SEGREG_INDEX, madr, inst##4, UINT32_TO_PTR(src)); \
}

/* args == 3 */
#define	ARITH_INSTRUCTION_3(inst) \
static UINT32 CPUCALL \
inst##1(UINT32 dst, void *arg) \
{ \
	UINT32 src = PTR_TO_UINT32(arg); \
	UINT32 res; \
	BYTE_##inst(res, dst, src); \
	return res; \
} \
static UINT32 CPUCALL \
inst##2(UINT32 dst, void *arg) \
{ \
	UINT32 src = PTR_TO_UINT32(arg); \
	UINT32 res; \
	WORD_##inst(res, dst, src); \
	return res; \
} \
static UINT32 CPUCALL \
inst##4(UINT32 dst, void *arg) \
{ \
	UINT32 src = PTR_TO_UINT32(arg); \
	UINT32 res; \
	DWORD_##inst(res, dst, src); \
	return res; \
} \
\
void \
inst##_EbGb(void) \
{ \
	UINT8 *out; \
	UINT32 op, src, dst, res, madr; \
\
	PREPART_EA_REG8(op, src); \
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg8_b20[op]; \
		dst = *out; \
		BYTE_##inst(res, dst, src); \
		*out = (UINT8)res; \
	} else { \
		CPU_WORKCLOCK(7); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_b(CPU_INST_SEGREG_INDEX, madr, inst##1, UINT32_TO_PTR(src)); \
	} \
} \
\
void \
inst##_EwGw(void) \
{ \
	UINT16 *out; \
	UINT32 op, src, dst, res, madr; \
\
	PREPART_EA_REG16(op, src); \
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg16_b20[op]; \
		dst = *out; \
		WORD_##inst(res, dst, src); \
		*out = (UINT16)res; \
	} else { \
		CPU_WORKCLOCK(7); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_w(CPU_INST_SEGREG_INDEX, madr, inst##2, UINT32_TO_PTR(src)); \
	} \
} \
\
void \
inst##_EdGd(void) \
{ \
	UINT32 *out; \
	UINT32 op, src, dst, res, madr; \
\
	PREPART_EA_REG32(op, src); \
	if (op >= 0xc0) { \
		CPU_WORKCLOCK(2); \
		out = reg32_b20[op]; \
		dst = *out; \
		DWORD_##inst(res, dst, src); \
		*out = res; \
	} else { \
		CPU_WORKCLOCK(7); \
		madr = calc_ea_dst(op); \
		cpu_vmemory_RMW_d(CPU_INST_SEGREG_INDEX, madr, inst##4, UINT32_TO_PTR(src)); \
	} \
} \
\
void \
inst##_GbEb(void) \
{ \
	UINT8 *out; \
	UINT32 op, src, dst, res; \
\
	PREPART_REG8_EA(op, src, out, 2, 7); \
	dst = *out; \
	BYTE_##inst(res, dst, src); \
	*out = (UINT8)res; \
} \
\
void \
inst##_GwEw(void) \
{ \
	UINT16 *out; \
	UINT32 op, src, dst, res; \
\
	PREPART_REG16_EA(op, src, out, 2, 7); \
	dst = *out; \
	WORD_##inst(res, dst, src); \
	*out = (UINT16)res; \
} \
\
void \
inst##_GdEd(void) \
{ \
	UINT32 *out; \
	UINT32 op, src, dst, res; \
\
	PREPART_REG32_EA(op, src, out, 2, 7); \
	dst = *out; \
	DWORD_##inst(res, dst, src); \
	*out = res; \
} \
\
void \
inst##_ALIb(void) \
{ \
	UINT32 src, dst, res; \
\
	CPU_WORKCLOCK(2); \
	GET_PCBYTE(src); \
	dst = CPU_AL; \
	BYTE_##inst(res, dst, src); \
	CPU_AL = (UINT8)res; \
} \
\
void \
inst##_AXIw(void) \
{ \
	UINT32 src, dst, res; \
\
	CPU_WORKCLOCK(2); \
	GET_PCWORD(src); \
	dst = CPU_AX; \
	WORD_##inst(res, dst, src); \
	CPU_AX = (UINT16)res; \
} \
\
void \
inst##_EAXId(void) \
{ \
	UINT32 src, dst, res; \
\
	CPU_WORKCLOCK(2); \
	GET_PCDWORD(src); \
	dst = CPU_EAX; \
	DWORD_##inst(res, dst, src); \
	CPU_EAX = res; \
} \
\
void CPUCALL \
inst##_EbIb(UINT8 *regp, UINT32 src) \
{ \
	UINT32 dst, res; \
\
	dst = *regp; \
	BYTE_##inst(res, dst, src); \
	*regp = (UINT8)res; \
} \
\
void CPUCALL \
inst##_EbIb_ext(UINT32 madr, UINT32 src) \
{ \
\
	cpu_vmemory_RMW_b(CPU_INST_SEGREG_INDEX, madr, inst##1, UINT32_TO_PTR(src)); \
} \
\
void CPUCALL \
inst##_EwIx(UINT16 *regp, UINT32 src) \
{ \
	UINT32 dst, res; \
\
	dst = *regp; \
	WORD_##inst(res, dst, src); \
	*regp = (UINT16)res; \
} \
\
void CPUCALL \
inst##_EwIx_ext(UINT32 madr, UINT32 src) \
{ \
\
	cpu_vmemory_RMW_w(CPU_INST_SEGREG_INDEX, madr, inst##2, UINT32_TO_PTR(src)); \
} \
\
void CPUCALL \
inst##_EdIx(UINT32 *regp, UINT32 src) \
{ \
	UINT32 dst, res; \
\
	dst = *regp; \
	DWORD_##inst(res, dst, src); \
	*regp = res; \
} \
\
void CPUCALL \
inst##_EdIx_ext(UINT32 madr, UINT32 src) \
{ \
\
	cpu_vmemory_RMW_d(CPU_INST_SEGREG_INDEX, madr, inst##4, UINT32_TO_PTR(src)); \
}

#endif	/* IA32_CPU_ARITH_MCR__ */
