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

#ifndef	IA32_CPU_INSTRUCTION_CTRL_TRANS_H__
#define	IA32_CPU_INSTRUCTION_CTRL_TRANS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * JMP
 */
void JMP_Jb(void);
void JMP_Jw(void);
void JMP_Jd(void);
void CPUCALL JMP_Ew(UINT32);
void CPUCALL JMP_Ed(UINT32);
void JMP16_Ap(void);
void JMP32_Ap(void);
void CPUCALL JMP16_Ep(UINT32);
void CPUCALL JMP32_Ep(UINT32);

/*
 * Jcc
 */
void JO_Jb(void);
void JO_Jw(void);
void JO_Jd(void);
void JNO_Jb(void);
void JNO_Jw(void);
void JNO_Jd(void);
void JC_Jb(void);
void JC_Jw(void);
void JC_Jd(void);
void JNC_Jb(void);
void JNC_Jw(void);
void JNC_Jd(void);
void JZ_Jb(void);
void JZ_Jw(void);
void JZ_Jd(void);
void JNZ_Jb(void);
void JNZ_Jw(void);
void JNZ_Jd(void);
void JA_Jb(void);
void JA_Jw(void);
void JA_Jd(void);
void JNA_Jb(void);
void JNA_Jw(void);
void JNA_Jd(void);
void JS_Jb(void);
void JS_Jw(void);
void JS_Jd(void);
void JNS_Jb(void);
void JNS_Jw(void);
void JNS_Jd(void);
void JP_Jb(void);
void JP_Jw(void);
void JP_Jd(void);
void JNP_Jb(void);
void JNP_Jw(void);
void JNP_Jd(void);
void JL_Jb(void);
void JL_Jw(void);
void JL_Jd(void);
void JNL_Jb(void);
void JNL_Jw(void);
void JNL_Jd(void);
void JLE_Jb(void);
void JLE_Jw(void);
void JLE_Jd(void);
void JNLE_Jb(void);
void JNLE_Jw(void);
void JNLE_Jd(void);
void JeCXZ_Jb(void);

/*
 * LOOPcc
 */
void LOOPNE_Jb(void);
void LOOPE_Jb(void);
void LOOP_Jb(void);

/*
 * CALL
 */
void CALL_Aw(void);
void CALL_Ad(void);
void CPUCALL CALL_Ew(UINT32);
void CPUCALL CALL_Ed(UINT32);
void CALL16_Ap(void);
void CALL32_Ap(void);
void CPUCALL CALL16_Ep(UINT32);
void CPUCALL CALL32_Ep(UINT32);

/*
 * RET
 */
void RETnear16(void);
void RETnear32(void);
void RETnear16_Iw(void);
void RETnear32_Iw(void);
void RETfar16(void);
void RETfar32(void);
void RETfar16_Iw(void);
void RETfar32_Iw(void);
void IRET(void);

/*
 * INT
 */
void INT1(void);
void INT3(void);
void INTO(void);
void INT_Ib(void);

void BOUND_GwMa(void);
void BOUND_GdMa(void);

/*
 * STACK
 */
void ENTER16_IwIb(void);
void ENTER32_IwIb(void);
void LEAVE(void);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_CTRL_TRANS_H__ */
