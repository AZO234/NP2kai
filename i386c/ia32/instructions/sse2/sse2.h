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

#ifndef	IA32_CPU_INSTRUCTION_SSE2_SSE2_H__
#define	IA32_CPU_INSTRUCTION_SSE2_SSE2_H__

#ifdef __cplusplus
extern "C" {
#endif

float SSE2_ROUND_FLOAT(float val);
double SSE2_ROUND_DOUBLE(double val);

void SSE2_ADDPD(void);
void SSE2_ADDSD(void);
void SSE2_ANDNPD(void);
void SSE2_ANDPD(void);
void SSE2_CMPPD(void);
void SSE2_CMPSD(void);
void SSE2_COMISD(void);
void SSE2_CVTPI2PD(void);
void SSE2_CVTPD2PI(void);
void SSE2_CVTSI2SD(void);
void SSE2_CVTSD2SI(void);
void SSE2_CVTTPD2PI(void);
void SSE2_CVTTSD2SI(void);
void SSE2_CVTPD2PS(void);
void SSE2_CVTPS2PD(void);
void SSE2_CVTSD2SS(void);
void SSE2_CVTSS2SD(void);
void SSE2_CVTPD2DQ(void);
void SSE2_CVTTPD2DQ(void);
void SSE2_CVTDQ2PD(void);
void SSE2_CVTPS2DQ(void);
void SSE2_CVTTPS2DQ(void);
void SSE2_CVTDQ2PS(void);
void SSE2_DIVPD(void);
void SSE2_DIVSD(void);
void SSE2_MAXPD(void);
void SSE2_MAXSD(void);
void SSE2_MINPD(void);
void SSE2_MINSD(void);
void SSE2_MOVAPDmem2xmm(void);
void SSE2_MOVAPDxmm2mem(void);
void SSE2_MOVHPDmem2xmm(void);
void SSE2_MOVHPDxmm2mem(void);
void SSE2_MOVLPDmem2xmm(void);
void SSE2_MOVLPDxmm2mem(void);
void SSE2_MOVMSKPD(void);
void SSE2_MOVSDmem2xmm(void);
void SSE2_MOVSDxmm2mem(void);
void SSE2_MOVUPDmem2xmm(void);
void SSE2_MOVUPDxmm2mem(void);
void SSE2_MULPD(void);
void SSE2_MULSD(void);
void SSE2_ORPD(void);
void SSE2_SHUFPD(void);
void SSE2_SQRTPD(void);
void SSE2_SQRTSD(void);
//void SSE2_STMXCSR(UINT32 maddr); // -> SSE_STMXCSR‚Æ“¯‚¶
void SSE2_SUBPD(void);
void SSE2_SUBSD(void);
void SSE2_UCOMISD(void);
void SSE2_UNPCKHPD(void);
void SSE2_UNPCKLPD(void);
void SSE2_XORPD(void);

void SSE2_MOVDrm2xmm(void);
void SSE2_MOVDxmm2rm(void);
void SSE2_MOVDQAmem2xmm(void);
void SSE2_MOVDQAxmm2mem(void);
void SSE2_MOVDQUmem2xmm(void);
void SSE2_MOVDQUxmm2mem(void);
void SSE2_MOVQ2DQ(void);
void SSE2_MOVDQ2Q(void);
void SSE2_MOVQmem2xmm(void);
void SSE2_MOVQxmm2mem(void);
void SSE2_PACKSSDW(void);
void SSE2_PACKSSWB(void);
void SSE2_PACKUSWB(void);
void SSE2_PADDQmm(void);
void SSE2_PADDQxmm(void);
void SSE2_PADDB(void);
void SSE2_PADDW(void);
void SSE2_PADDD(void);
//void SSE2_PADDQ(void);
void SSE2_PADDSB(void);
void SSE2_PADDSW(void);
//void SSE2_PADDSD(void);
//void SSE2_PADDSQ(void);
void SSE2_PADDUSB(void);
void SSE2_PADDUSW(void);
//void SSE2_PADDUSD(void);
//void SSE2_PADDUSQ(void);
void SSE2_PAND(void);
void SSE2_PANDN(void);
void SSE2_PAVGB(void);
void SSE2_PAVGW(void);
void SSE2_PCMPEQB(void);
void SSE2_PCMPEQW(void);
void SSE2_PCMPEQD(void);
//void SSE2_PCMPEQQ(void);
void SSE2_PCMPGTB(void);
void SSE2_PCMPGTW(void);
void SSE2_PCMPGTD(void);
//void SSE2_PCMPGTQ(void);
void SSE2_PEXTRW(void);
void SSE2_PINSRW(void);
void SSE2_PMADD(void);
void SSE2_PMAXSW(void);
void SSE2_PMAXUB(void);
void SSE2_PMINSW(void);
void SSE2_PMINUB(void);
void SSE2_PMOVMSKB(void);
void SSE2_PMULHUW(void);
void SSE2_PMULHW(void);
void SSE2_PMULLW(void);
void SSE2_PMULUDQmm(void);
void SSE2_PMULUDQxmm(void);
void SSE2_POR(void);
void SSE2_PSADBW(void);
void SSE2_PSHUFLW(void);
void SSE2_PSHUFHW(void);
void SSE2_PSHUFD(void);
//void SSE2_PSLLDQ(void);
//void SSE2_PSLLB(void);
void SSE2_PSLLW(void);
void SSE2_PSLLD(void);
void SSE2_PSLLQ(void);
//void SSE2_PSLLBimm(void);
//void SSE2_PSLLWimm(void);
//void SSE2_PSLLDimm(void);
//void SSE2_PSLLQimm(void);
//void SSE2_PSRAB(void);
void SSE2_PSRAW(void);
void SSE2_PSRAD(void);
//void SSE2_PSRAQ(void);
//void SSE2_PSRABimm(void);
//void SSE2_PSRAWimm(void);
//void SSE2_PSRADimm(void);
//void SSE2_PSRAQimm(void);
//void SSE2_PSRLDQ(void);
//void SSE2_PSRLB(void);
void SSE2_PSRLW(void);
void SSE2_PSRLD(void);
void SSE2_PSRLQ(void);
//void SSE2_PSRLBimm(void);
//void SSE2_PSRLWimm(void);
//void SSE2_PSRLDimm(void);
//void SSE2_PSRLQimm(void);
void SSE2_PSxxWimm(void);
void SSE2_PSxxDimm(void);
void SSE2_PSxxQimm(void);
void SSE2_PSUBQmm(void);
void SSE2_PSUBQxmm(void);
void SSE2_PSUBB(void);
void SSE2_PSUBW(void);
void SSE2_PSUBD(void);
//void SSE2_PSUBQ(void);
void SSE2_PSUBSB(void);
void SSE2_PSUBSW(void);
//void SSE2_PSUBSD(void);
//void SSE2_PSUBSQ(void);
void SSE2_PSUBUSB(void);
void SSE2_PSUBUSW(void);
//void SSE2_PSUBUSD(void);
//void SSE2_PSUBUSQ(void);
void SSE2_PUNPCKHBW(void);
void SSE2_PUNPCKHWD(void);
void SSE2_PUNPCKHDQ(void);
void SSE2_PUNPCKHQDQ(void);
void SSE2_PUNPCKLBW(void);
void SSE2_PUNPCKLWD(void);
void SSE2_PUNPCKLDQ(void);
void SSE2_PUNPCKLQDQ(void);
void SSE2_PXOR(void);

void SSE2_MASKMOVDQU(void);
//void SSE2_CLFLUSH(UINT32 op); // --> SSE_CLFLUSH(UINT32 op)‚Ö
void SSE2_MOVNTPD(void);
void SSE2_MOVNTDQ(void);
void SSE2_MOVNTI(void);
void SSE2_PAUSE(void);
//void SSE2_LFENCE(void); // --> SSE_LFENCE(void)‚Ö
//void SSE2_MFENCE(void); // --> SSE_MFENCE(void)‚Ö

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_SSE2_SSE2_H__ */
