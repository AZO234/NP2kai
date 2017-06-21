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

#ifndef	IA32_CPU_INSTRUCTION_MISC_H__
#define	IA32_CPU_INSTRUCTION_MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

void LEA_GwM(void);
void LEA_GdM(void);
void _NOP(void);
void UD2(void);
void XLAT(void);
void _CPUID(void);

void SALC(void);	/* undoc 8086 */
void LOADALL286(void);	/* undoc 286 */
void LOADALL(void);	/* undoc 386 */

void OpSize(void);
void AddrSize(void);
void _2byte_ESC16(void);
void _2byte_ESC32(void);
void Prefix_ES(void);
void Prefix_CS(void);
void Prefix_SS(void);
void Prefix_DS(void);
void Prefix_FS(void);
void Prefix_GS(void);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_MISC_H__ */
