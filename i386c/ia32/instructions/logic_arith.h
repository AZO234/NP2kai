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

#ifndef	IA32_CPU_INSTRUCTION_LOGIC_ARITH_H__
#define	IA32_CPU_INSTRUCTION_LOGIC_ARITH_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AND
 */
void AND_EbGb(void);
void AND_EwGw(void);
void AND_EdGd(void);
void AND_GbEb(void);
void AND_GwEw(void);
void AND_GdEd(void);
void AND_ALIb(void);
void AND_AXIw(void);
void AND_EAXId(void);
void CPUCALL AND_EbIb(UINT8 *, UINT32);
void CPUCALL AND_EwIx(UINT16 *, UINT32);
void CPUCALL AND_EdIx(UINT32 *, UINT32);
void CPUCALL AND_EbIb_ext(UINT32, UINT32);
void CPUCALL AND_EwIx_ext(UINT32, UINT32);
void CPUCALL AND_EdIx_ext(UINT32, UINT32);

/*
 * OR
 */
void OR_EbGb(void);
void OR_EwGw(void);
void OR_EdGd(void);
void OR_GbEb(void);
void OR_GwEw(void);
void OR_GdEd(void);
void OR_ALIb(void);
void OR_AXIw(void);
void OR_EAXId(void);
void CPUCALL OR_EbIb(UINT8 *, UINT32);
void CPUCALL OR_EwIx(UINT16 *, UINT32);
void CPUCALL OR_EdIx(UINT32 *, UINT32);
void CPUCALL OR_EbIb_ext(UINT32, UINT32);
void CPUCALL OR_EwIx_ext(UINT32, UINT32);
void CPUCALL OR_EdIx_ext(UINT32, UINT32);

/*
 * XOR
 */
void XOR_EbGb(void);
void XOR_EwGw(void);
void XOR_EdGd(void);
void XOR_GbEb(void);
void XOR_GwEw(void);
void XOR_GdEd(void);
void XOR_ALIb(void);
void XOR_AXIw(void);
void XOR_EAXId(void);
void CPUCALL XOR_EbIb(UINT8 *, UINT32);
void CPUCALL XOR_EwIx(UINT16 *, UINT32);
void CPUCALL XOR_EdIx(UINT32 *, UINT32);
void CPUCALL XOR_EbIb_ext(UINT32, UINT32);
void CPUCALL XOR_EwIx_ext(UINT32, UINT32);
void CPUCALL XOR_EdIx_ext(UINT32, UINT32);

/*
 * NOT
 */
void CPUCALL NOT_Eb(UINT32);
void CPUCALL NOT_Ew(UINT32);
void CPUCALL NOT_Ed(UINT32);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_LOGIC_ARITH_H__ */
