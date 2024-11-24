/*
 * Copyright (c) 2024 Gocaine Project
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

#ifndef	IA32_CPU_INSTRUCTION_SSE4_SSE4_1_H__
#define	IA32_CPU_INSTRUCTION_SSE4_SSE4_1_H__

//#ifdef __cplusplus
//extern "C" {
//#endif

void SSE4_1_PBLENDVB(void);
void SSE4_1_BLENDVPS(void);
void SSE4_1_BLENDVPD(void);
void SSE4_1_VPTEST(void);
void SSE4_1_PMOVSXBW(void);
void SSE4_1_PMOVSXBD(void);
void SSE4_1_PMOVSXBQ(void);
void SSE4_1_PMOVSXWD(void);
void SSE4_1_PMOVSXWQ(void);
void SSE4_1_PMOVSXDQ(void);
void SSE4_1_PMOVZXBW(void);
void SSE4_1_PMOVZXBD(void);
void SSE4_1_PMOVZXBQ(void);
void SSE4_1_PMOVZXWD(void);
void SSE4_1_PMOVZXWQ(void);
void SSE4_1_PMOVZXDQ(void);
void SSE4_1_PMULLD(void);
void SSE4_1_PHMINPOSUW(void);
void SSE4_1_PMULDQ(void);
void SSE4_1_PCMPEQQ(void);
void SSE4_1_MOVNTDQA(void);
void SSE4_1_PACKUSDW(void);
void SSE4_1_PMINSB(void);
void SSE4_1_PMINSD(void);
void SSE4_1_PMINUW(void);
void SSE4_1_PMINUD(void);
void SSE4_1_PMAXSB(void);
void SSE4_1_PMAXSD(void);
void SSE4_1_PMAXUW(void);
void SSE4_1_PMAXUD(void);
void SSE4_1_PEXTRB(void);
void SSE4_1_PEXTRW(void);
void SSE4_1_PEXTRD(void);
void SSE4_1_PINSRB(void);
void SSE4_1_PINSRW(void);
void SSE4_1_PINSRD(void);
void SSE4_1_PEXTRACTPS(void);
void SSE4_1_INSERTPS(void);
void SSE4_1_DPPS(void);
void SSE4_1_DPPD(void);
void SSE4_1_MPSADBW(void);
void SSE4_1_ROUNDPS(void);
void SSE4_1_ROUNDPD(void);
void SSE4_1_ROUNDSS(void);
void SSE4_1_ROUNDSD(void);
void SSE4_1_PBLENDPS(void);
void SSE4_1_PBLENDPD(void);
void SSE4_1_PBLENDW(void);

//#ifdef __cplusplus
//}
//#endif

#endif	/* IA32_CPU_INSTRUCTION_SSE4_SSE4_1_H__ */
