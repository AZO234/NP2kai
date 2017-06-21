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

#ifndef	IA32_CPU_INSTRUCTION_BIN_ARITH_H__
#define	IA32_CPU_INSTRUCTION_BIN_ARITH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ADD */
void ADD_EbGb(void);
void ADD_EwGw(void);
void ADD_EdGd(void);
void ADD_GbEb(void);
void ADD_GwEw(void);
void ADD_GdEd(void);
void ADD_ALIb(void);
void ADD_AXIw(void);
void ADD_EAXId(void);
void CPUCALL ADD_EbIb(UINT8 *, UINT32);
void CPUCALL ADD_EwIx(UINT16 *, UINT32);
void CPUCALL ADD_EdIx(UINT32 *, UINT32);
void CPUCALL ADD_EbIb_ext(UINT32, UINT32);
void CPUCALL ADD_EwIx_ext(UINT32, UINT32);
void CPUCALL ADD_EdIx_ext(UINT32, UINT32);

/* ADC */
void ADC_EbGb(void);
void ADC_EwGw(void);
void ADC_EdGd(void);
void ADC_GbEb(void);
void ADC_GwEw(void);
void ADC_GdEd(void);
void ADC_ALIb(void);
void ADC_AXIw(void);
void ADC_EAXId(void);
void CPUCALL ADC_EbIb(UINT8 *, UINT32);
void CPUCALL ADC_EwIx(UINT16 *, UINT32);
void CPUCALL ADC_EdIx(UINT32 *, UINT32);
void CPUCALL ADC_EbIb_ext(UINT32, UINT32);
void CPUCALL ADC_EwIx_ext(UINT32, UINT32);
void CPUCALL ADC_EdIx_ext(UINT32, UINT32);

/* SUB */
void SUB_EbGb(void);
void SUB_EwGw(void);
void SUB_EdGd(void);
void SUB_GbEb(void);
void SUB_GwEw(void);
void SUB_GdEd(void);
void SUB_ALIb(void);
void SUB_AXIw(void);
void SUB_EAXId(void);
void CPUCALL SUB_EbIb(UINT8 *, UINT32);
void CPUCALL SUB_EwIx(UINT16 *, UINT32);
void CPUCALL SUB_EdIx(UINT32 *, UINT32);
void CPUCALL SUB_EbIb_ext(UINT32, UINT32);
void CPUCALL SUB_EwIx_ext(UINT32, UINT32);
void CPUCALL SUB_EdIx_ext(UINT32, UINT32);

/* SBB */
void SBB_EbGb(void);
void SBB_EwGw(void);
void SBB_EdGd(void);
void SBB_GbEb(void);
void SBB_GwEw(void);
void SBB_GdEd(void);
void SBB_ALIb(void);
void SBB_AXIw(void);
void SBB_EAXId(void);
void CPUCALL SBB_EbIb(UINT8 *, UINT32);
void CPUCALL SBB_EwIx(UINT16 *, UINT32);
void CPUCALL SBB_EdIx(UINT32 *, UINT32);
void CPUCALL SBB_EbIb_ext(UINT32, UINT32);
void CPUCALL SBB_EwIx_ext(UINT32, UINT32);
void CPUCALL SBB_EdIx_ext(UINT32, UINT32);

/* IMUL */
void CPUCALL IMUL_ALEb(UINT32 op);
void CPUCALL IMUL_AXEw(UINT32 op);
void CPUCALL IMUL_EAXEd(UINT32 op);
void IMUL_GwEw(void);
void IMUL_GdEd(void);
void IMUL_GwEwIb(void);
void IMUL_GdEdIb(void);
void IMUL_GwEwIw(void);
void IMUL_GdEdId(void);

/* MUL */
void CPUCALL MUL_ALEb(UINT32 op);
void CPUCALL MUL_AXEw(UINT32 op);
void CPUCALL MUL_EAXEd(UINT32 op);

/* IDIV */
void CPUCALL IDIV_ALEb(UINT32 op);
void CPUCALL IDIV_AXEw(UINT32 op);
void CPUCALL IDIV_EAXEd(UINT32 op);

/* DIV */
void CPUCALL DIV_ALEb(UINT32 op);
void CPUCALL DIV_AXEw(UINT32 op);
void CPUCALL DIV_EAXEd(UINT32 op);

/* INC */
void CPUCALL INC_Eb(UINT32 op);
void CPUCALL INC_Ew(UINT32 op);
void CPUCALL INC_Ed(UINT32 op);
void INC_AX(void);
void INC_CX(void);
void INC_DX(void);
void INC_BX(void);
void INC_SP(void);
void INC_BP(void);
void INC_SI(void);
void INC_DI(void);
void INC_EAX(void);
void INC_ECX(void);
void INC_EDX(void);
void INC_EBX(void);
void INC_ESP(void);
void INC_EBP(void);
void INC_ESI(void);
void INC_EDI(void);

/* DEC */
void CPUCALL DEC_Eb(UINT32 op);
void CPUCALL DEC_Ew(UINT32 op);
void CPUCALL DEC_Ed(UINT32 op);
void DEC_AX(void);
void DEC_CX(void);
void DEC_DX(void);
void DEC_BX(void);
void DEC_SP(void);
void DEC_BP(void);
void DEC_SI(void);
void DEC_DI(void);
void DEC_EAX(void);
void DEC_ECX(void);
void DEC_EDX(void);
void DEC_EBX(void);
void DEC_ESP(void);
void DEC_EBP(void);
void DEC_ESI(void);
void DEC_EDI(void);

/* NEG */
void CPUCALL NEG_Eb(UINT32 op);
void CPUCALL NEG_Ew(UINT32 op);
void CPUCALL NEG_Ed(UINT32 op);

/* CMP */
void CMP_EbGb(void);
void CMP_EwGw(void);
void CMP_EdGd(void);
void CMP_GbEb(void);
void CMP_GwEw(void);
void CMP_GdEd(void);
void CMP_ALIb(void);
void CMP_AXIw(void);
void CMP_EAXId(void);
void CPUCALL CMP_EbIb(UINT8 *, UINT32);
void CPUCALL CMP_EwIx(UINT16 *, UINT32);
void CPUCALL CMP_EdIx(UINT32 *, UINT32);
void CPUCALL CMP_EbIb_ext(UINT32, UINT32);
void CPUCALL CMP_EwIx_ext(UINT32, UINT32);
void CPUCALL CMP_EdIx_ext(UINT32, UINT32);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_BIN_ARITH_H__ */
