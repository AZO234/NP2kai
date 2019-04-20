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

#ifndef	IA32_CPU_INSTRUCTION_SSE_SSE_H__
#define	IA32_CPU_INSTRUCTION_SSE_SSE_H__

#ifdef __cplusplus
extern "C" {
#endif

float SSE_ROUND(float val);
	
void SSE_ADDPS(void);
void SSE_ADDSS(void);
void SSE_ANDNPS(void);
void SSE_ANDPS(void);
void SSE_CMPPS(void);
void SSE_CMPSS(void);
void SSE_COMISS(void);
void SSE_CVTPI2PS(void);
void SSE_CVTPS2PI(void);
void SSE_CVTSI2SS(void);
void SSE_CVTSS2SI(void);
void SSE_CVTTPS2PI(void);
void SSE_CVTTSS2SI(void);
void SSE_DIVPS(void);
void SSE_DIVSS(void);
void SSE_LDMXCSR(UINT32 maddr);
void SSE_MAXPS(void);
void SSE_MAXSS(void);
void SSE_MINPS(void);
void SSE_MINSS(void);
void SSE_MOVAPSmem2xmm(void);
void SSE_MOVAPSxmm2mem(void);
void SSE_MOVHLPS(float *data1, float *data2);
void SSE_MOVHPSmem2xmm(void);
void SSE_MOVHPSxmm2mem(void);
void SSE_MOVLHPS(float *data1, float *data2);
void SSE_MOVLPSmem2xmm(void);
void SSE_MOVLPSxmm2mem(void);
void SSE_MOVMSKPS(void);
void SSE_MOVSSmem2xmm(void);
void SSE_MOVSSxmm2mem(void);
void SSE_MOVUPSmem2xmm(void);
void SSE_MOVUPSxmm2mem(void);
void SSE_MULPS(void);
void SSE_MULSS(void);
void SSE_ORPS(void);
void SSE_RCPPS(void);
void SSE_RCPSS(void);
void SSE_RSQRTPS(void);
void SSE_RSQRTSS(void);
void SSE_SHUFPS(void);
void SSE_SQRTPS(void);
void SSE_SQRTSS(void);
void SSE_STMXCSR(UINT32 maddr);
void SSE_SUBPS(void);
void SSE_SUBSS(void);
void SSE_UCOMISS(void);
void SSE_UNPCKHPS(void);
void SSE_UNPCKLPS(void);
void SSE_XORPS(void);

void SSE_PAVGB(void);
void SSE_PAVGW(void);
void SSE_PEXTRW(void);
void SSE_PINSRW(void);
void SSE_PMAXSW(void);
void SSE_PMAXUB(void);
void SSE_PMINSW(void);
void SSE_PMINUB(void);
void SSE_PMOVMSKB(void);
void SSE_PMULHUW(void);
void SSE_PSADBW(void);
void SSE_PSHUFW(void);

void SSE_MASKMOVQ(void);
void SSE_MOVNTPS(void);
void SSE_MOVNTQ(void);
void SSE_PREFETCHTx(void);
void SSE_NOPPREFETCH(void);
//void SSE_PREFETCHNTA(void); // -> SSE_PREFETCHTx
void SSE_SFENCE(void);
void SSE_LFENCE(void);
void SSE_MFENCE(void);
void SSE_CLFLUSH(UINT32 op);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_SSE_SSE_H__ */
