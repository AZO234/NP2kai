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

#ifndef	IA32_CPU_INSTRUCTION_SHIFT_ROTATE_H__
#define	IA32_CPU_INSTRUCTION_SHIFT_ROTATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SAR
 */
void CPUCALL SAR_Eb(UINT8 *);
void CPUCALL SAR_Ew(UINT16 *);
void CPUCALL SAR_Ed(UINT32 *);
void CPUCALL SAR_Eb_ext(UINT32);
void CPUCALL SAR_Ew_ext(UINT32);
void CPUCALL SAR_Ed_ext(UINT32);
void CPUCALL SAR_EbCL(UINT8 *, UINT32);
void CPUCALL SAR_EbCL_ext(UINT32, UINT32);
void CPUCALL SAR_EwCL(UINT16 *, UINT32);
void CPUCALL SAR_EwCL_ext(UINT32, UINT32);
void CPUCALL SAR_EdCL(UINT32 *, UINT32);
void CPUCALL SAR_EdCL_ext(UINT32, UINT32);

/*
 * SHR
 */
void CPUCALL SHR_Eb(UINT8 *);
void CPUCALL SHR_Ew(UINT16 *);
void CPUCALL SHR_Ed(UINT32 *);
void CPUCALL SHR_Eb_ext(UINT32);
void CPUCALL SHR_Ew_ext(UINT32);
void CPUCALL SHR_Ed_ext(UINT32);
void CPUCALL SHR_EbCL(UINT8 *, UINT32);
void CPUCALL SHR_EbCL_ext(UINT32, UINT32);
void CPUCALL SHR_EwCL(UINT16 *, UINT32);
void CPUCALL SHR_EwCL_ext(UINT32, UINT32);
void CPUCALL SHR_EdCL(UINT32 *, UINT32);
void CPUCALL SHR_EdCL_ext(UINT32, UINT32);

/*
 * SHL
 */
void CPUCALL SHL_Eb(UINT8 *);
void CPUCALL SHL_Ew(UINT16 *);
void CPUCALL SHL_Ed(UINT32 *);
void CPUCALL SHL_Eb_ext(UINT32);
void CPUCALL SHL_Ew_ext(UINT32);
void CPUCALL SHL_Ed_ext(UINT32);
void CPUCALL SHL_EbCL(UINT8 *, UINT32);
void CPUCALL SHL_EbCL_ext(UINT32, UINT32);
void CPUCALL SHL_EwCL(UINT16 *, UINT32);
void CPUCALL SHL_EwCL_ext(UINT32, UINT32);
void CPUCALL SHL_EdCL(UINT32 *, UINT32);
void CPUCALL SHL_EdCL_ext(UINT32, UINT32);

/*
 * SHRD
 */
void SHRD_EwGwIb(void);
void SHRD_EdGdIb(void);
void SHRD_EwGwCL(void);
void SHRD_EdGdCL(void);

/*
 * SHLD
 */
void SHLD_EwGwIb(void);
void SHLD_EdGdIb(void);
void SHLD_EwGwCL(void);
void SHLD_EdGdCL(void);

/*
 * ROR
 */
void CPUCALL ROR_Eb(UINT8 *);
void CPUCALL ROR_Ew(UINT16 *);
void CPUCALL ROR_Ed(UINT32 *);
void CPUCALL ROR_Eb_ext(UINT32);
void CPUCALL ROR_Ew_ext(UINT32);
void CPUCALL ROR_Ed_ext(UINT32);
void CPUCALL ROR_EbCL(UINT8 *, UINT32);
void CPUCALL ROR_EbCL_ext(UINT32, UINT32);
void CPUCALL ROR_EwCL(UINT16 *, UINT32);
void CPUCALL ROR_EwCL_ext(UINT32, UINT32);
void CPUCALL ROR_EdCL(UINT32 *, UINT32);
void CPUCALL ROR_EdCL_ext(UINT32, UINT32);

/*
 * ROL
 */
void CPUCALL ROL_Eb(UINT8 *);
void CPUCALL ROL_Ew(UINT16 *);
void CPUCALL ROL_Ed(UINT32 *);
void CPUCALL ROL_Eb_ext(UINT32);
void CPUCALL ROL_Ew_ext(UINT32);
void CPUCALL ROL_Ed_ext(UINT32);
void CPUCALL ROL_EbCL(UINT8 *, UINT32);
void CPUCALL ROL_EbCL_ext(UINT32, UINT32);
void CPUCALL ROL_EwCL(UINT16 *, UINT32);
void CPUCALL ROL_EwCL_ext(UINT32, UINT32);
void CPUCALL ROL_EdCL(UINT32 *, UINT32);
void CPUCALL ROL_EdCL_ext(UINT32, UINT32);

/*
 * RCR
 */
void CPUCALL RCR_Eb(UINT8 *);
void CPUCALL RCR_Ew(UINT16 *);
void CPUCALL RCR_Ed(UINT32 *);
void CPUCALL RCR_Eb_ext(UINT32);
void CPUCALL RCR_Ew_ext(UINT32);
void CPUCALL RCR_Ed_ext(UINT32);
void CPUCALL RCR_EbCL(UINT8 *, UINT32);
void CPUCALL RCR_EbCL_ext(UINT32, UINT32);
void CPUCALL RCR_EwCL(UINT16 *, UINT32);
void CPUCALL RCR_EwCL_ext(UINT32, UINT32);
void CPUCALL RCR_EdCL(UINT32 *, UINT32);
void CPUCALL RCR_EdCL_ext(UINT32, UINT32);

/*
 * RCL
 */
void CPUCALL RCL_Eb(UINT8 *);
void CPUCALL RCL_Ew(UINT16 *);
void CPUCALL RCL_Ed(UINT32 *);
void CPUCALL RCL_Eb_ext(UINT32);
void CPUCALL RCL_Ew_ext(UINT32);
void CPUCALL RCL_Ed_ext(UINT32);
void CPUCALL RCL_EbCL(UINT8 *, UINT32);
void CPUCALL RCL_EbCL_ext(UINT32, UINT32);
void CPUCALL RCL_EwCL(UINT16 *, UINT32);
void CPUCALL RCL_EwCL_ext(UINT32, UINT32);
void CPUCALL RCL_EdCL(UINT32 *, UINT32);
void CPUCALL RCL_EdCL_ext(UINT32, UINT32);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_SHIFT_ROTATE_H__ */
