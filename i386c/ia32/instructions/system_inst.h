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

#ifndef	IA32_CPU_INSTRUCTION_SYSTEM_H__
#define	IA32_CPU_INSTRUCTION_SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Load/Store system register */
void CPUCALL LGDT_Ms(UINT32);
void CPUCALL SGDT_Ms(UINT32);
void CPUCALL LLDT_Ew(UINT32);
void CPUCALL SLDT_Ew(UINT32);
void CPUCALL LTR_Ew(UINT32);
void CPUCALL STR_Ew(UINT32);
void CPUCALL LIDT_Ms(UINT32);
void CPUCALL SIDT_Ms(UINT32);

/* ctrl reg */
void MOV_CdRd(void);
void MOV_RdCd(void);

/* msw */
void CPUCALL LMSW_Ew(UINT32);
void CPUCALL SMSW_Ew(UINT32);

/* */
void CLTS(void);
void ARPL_EwGw(void);

/* dpl */
void LAR_GwEw(void);
void LAR_GdEw(void);
void LSL_GwEw(void);
void LSL_GdEw(void);

/* */
void CPUCALL VERR_Ew(UINT32);
void CPUCALL VERW_Ew(UINT32);

/* dbg reg */
void MOV_DdRd(void);
void MOV_RdDd(void);

/* cache */
void INVD(void);
void WBINVD(void);

/* */
void CPUCALL INVLPG(UINT32);

/* */
void _LOCK(void);
void HLT(void);

/* ctrl reg */
void RSM(void);

/* msr */
void RDMSR(void);
void WRMSR(void);

/* ctrl reg */
void RDTSC(void);

/* test reg */
void MOV_TdRd(void);
void MOV_RdTd(void);


#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_SYSTEM_H__ */
