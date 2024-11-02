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

#ifndef	IA32_CPU_INSTRUCTION_SSSE3_SSSE3_H__
#define	IA32_CPU_INSTRUCTION_SSSE3_SSSE3_H__

//#ifdef __cplusplus
//extern "C" {
//#endif

void SSSE3_PSHUFB(void);
void SSSE3_PSHUFB_MM(void);
void SSSE3_PHADDW(void);
void SSSE3_PHADDW_MM(void);
void SSSE3_PHADDD(void);
void SSSE3_PHADDD_MM(void);
void SSSE3_PHADDSW(void);
void SSSE3_PHADDSW_MM(void);
void SSSE3_PMADDUBSW(void);
void SSSE3_PMADDUBSW_MM(void);
void SSSE3_PHSUBW(void);
void SSSE3_PHSUBW_MM(void);
void SSSE3_PHSUBD(void);
void SSSE3_PHSUBD_MM(void);
void SSSE3_PHSUBSW(void);
void SSSE3_PHSUBSW_MM(void);
void SSSE3_PSIGNB(void);
void SSSE3_PSIGNB_MM(void);
void SSSE3_PSIGNW(void);
void SSSE3_PSIGNW_MM(void);
void SSSE3_PSIGND(void);
void SSSE3_PSIGND_MM(void);
void SSSE3_PMULHRSW(void);
void SSSE3_PMULHRSW_MM(void);
void SSSE3_PABSB(void);
void SSSE3_PABSB_MM(void);
void SSSE3_PABSW(void);
void SSSE3_PABSW_MM(void);
void SSSE3_PABSD(void);
void SSSE3_PABSD_MM(void);
void SSSE3_PALIGNR(void);
void SSSE3_PALIGNR_MM(void);

//#ifdef __cplusplus
//}
//#endif

#endif	/* IA32_CPU_INSTRUCTION_SSSE3_SSSE3_H__ */
