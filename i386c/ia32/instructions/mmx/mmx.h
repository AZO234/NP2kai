/*
 * Copyright (c) 2018 SimK
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

#ifndef	IA32_CPU_INSTRUCTION_MMX_MMX_H__
#define	IA32_CPU_INSTRUCTION_MMX_MMX_H__

typedef union {
	UINT8 b[8];
	UINT16 w[4];
	UINT32 d[2];
	UINT64 q[1];
} MMXREG;

#ifdef __cplusplus
extern "C" {
#endif
	
void MMX_EMMS(void);

void MMX_MOVD_mm_rm32(void);
void MMX_MOVD_rm32_mm(void);

void MMX_MOVQ_mm_mmm64(void);
void MMX_MOVQ_mmm64_mm(void);

void MMX_PACKSSWB(void);
void MMX_PACKSSDW(void);

void MMX_PACKUSWB(void);

void MMX_PADDB(void);
void MMX_PADDW(void);
void MMX_PADDD(void);

void MMX_PADDSB(void);
void MMX_PADDSW(void);

void MMX_PADDUSB(void);
void MMX_PADDUSW(void);

void MMX_PAND(void);
void MMX_PANDN(void);
void MMX_POR(void);
void MMX_PXOR(void);

void MMX_PCMPEQB(void);
void MMX_PCMPEQW(void);
void MMX_PCMPEQD(void);

void MMX_PCMPGTB(void);
void MMX_PCMPGTW(void);
void MMX_PCMPGTD(void);

void MMX_PMADDWD(void);

void MMX_PMULHW(void);
void MMX_PMULLW(void);

void MMX_PSLLW(void);
void MMX_PSLLD(void);
void MMX_PSLLQ(void);

void MMX_PSRAW(void);
void MMX_PSRAD(void);

void MMX_PSRLW(void);
void MMX_PSRLD(void);
void MMX_PSRLQ(void);

void MMX_PSxxW_imm8(void);
void MMX_PSxxD_imm8(void);
void MMX_PSxxQ_imm8(void);

void MMX_PSUBB(void);
void MMX_PSUBW(void);
void MMX_PSUBD(void);

void MMX_PSUBSB(void);
void MMX_PSUBSW(void);

void MMX_PSUBUSB(void);
void MMX_PSUBUSW(void);

void MMX_PUNPCKHBW(void);
void MMX_PUNPCKHWD(void);
void MMX_PUNPCKHDQ(void);
void MMX_PUNPCKLBW(void);
void MMX_PUNPCKLWD(void);
void MMX_PUNPCKLDQ(void);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_MMX_MMX_H__ */
