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

#ifndef	IA32_CPU_INSTRUCTION_SSE4_SSE4_2_H__
#define	IA32_CPU_INSTRUCTION_SSE4_SSE4_2_H__

//#ifdef __cplusplus
//extern "C" {
//#endif

void SSE4_2_PCMPGTQ(void);
void SSE4_2_PCMPESTRM(void);
void SSE4_2_PCMPESTRI(void);
void SSE4_2_PCMPISTRM(void);
void SSE4_2_PCMPISTRI(void);
void SSE4_2_CRC32_Gy_Eb(void);
void SSE4_2_CRC32_Gy_Ev(void);
void SSE4_2_CRC32_Gy_Eb_16(void);
void SSE4_2_CRC32_Gy_Ev_16(void);
void SSE4_2_POPCNT_16(void);
void SSE4_2_POPCNT_32(void);

//#ifdef __cplusplus
//}
//#endif

#endif	/* IA32_CPU_INSTRUCTION_SSE4_SSE4_2_H__ */
