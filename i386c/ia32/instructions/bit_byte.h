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

#ifndef	IA32_CPU_INSTRUCTION_BIT_BYTE_H__
#define	IA32_CPU_INSTRUCTION_BIT_BYTE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BTx
 */
void BT_EwGw(void);
void BT_EdGd(void);
void CPUCALL BT_EwIb(UINT32);
void CPUCALL BT_EdIb(UINT32);
void BTS_EwGw(void);
void BTS_EdGd(void);
void CPUCALL BTS_EwIb(UINT32);
void CPUCALL BTS_EdIb(UINT32);
void BTR_EwGw(void);
void BTR_EdGd(void);
void CPUCALL BTR_EwIb(UINT32);
void CPUCALL BTR_EdIb(UINT32);
void BTC_EwGw(void);
void BTC_EdGd(void);
void CPUCALL BTC_EwIb(UINT32);
void CPUCALL BTC_EdIb(UINT32);

/*
 * BSx
 */
void BSF_GwEw(void);
void BSF_GdEd(void);
void BSR_GwEw(void);
void BSR_GdEd(void);

/*
 * SETcc
 */
void SETO_Eb(void);
void SETNO_Eb(void);
void SETC_Eb(void);
void SETNC_Eb(void);
void SETZ_Eb(void);
void SETNZ_Eb(void);
void SETA_Eb(void);
void SETNA_Eb(void);
void SETS_Eb(void);
void SETNS_Eb(void);
void SETP_Eb(void);
void SETNP_Eb(void);
void SETL_Eb(void);
void SETNL_Eb(void);
void SETLE_Eb(void);
void SETNLE_Eb(void);

/*
 * TEST
 */
void TEST_EbGb(void);
void TEST_EwGw(void);
void TEST_EdGd(void);
void TEST_ALIb(void);
void TEST_AXIw(void);
void TEST_EAXId(void);

void CPUCALL TEST_EbIb(UINT32);
void CPUCALL TEST_EwIw(UINT32);
void CPUCALL TEST_EdId(UINT32);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_BIT_BYTE_H__ */
