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

#ifndef	IA32_CPU_EXCEPTION_H__
#define	IA32_CPU_EXCEPTION_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	DE_EXCEPTION = 0,	/* F */
	DB_EXCEPTION = 1,	/* F/T */
	NMI_EXCEPTION = 2,	/* I */
	BP_EXCEPTION = 3,	/* T */
	OF_EXCEPTION = 4,	/* T */
	BR_EXCEPTION = 5,	/* F */
	UD_EXCEPTION = 6,	/* F */
	NM_EXCEPTION = 7,	/* F */
	DF_EXCEPTION = 8,	/* A, Err(0) */
	/* CoProcesser Segment Overrun = 9 */
	TS_EXCEPTION = 10,	/* F */
	NP_EXCEPTION = 11,	/* F, Err */
	SS_EXCEPTION = 12,	/* F, Err */
	GP_EXCEPTION = 13,	/* F, Err */
	PF_EXCEPTION = 14,	/* F, Err */
	/* Reserved = 15 */
	MF_EXCEPTION = 16,	/* F */
	AC_EXCEPTION = 17,	/* F, Err(0) */
	MC_EXCEPTION = 18,	/* A, Err(?) */
	XF_EXCEPTION = 19,	/* F */
	EXCEPTION_NUM
};

enum {
	INTR_TYPE_SOFTINTR = -1,	/* software interrupt (INTn) */
	INTR_TYPE_EXTINTR = 0,		/* external interrupt */
	INTR_TYPE_EXCEPTION = 1,	/* exception */
};

#define	EXCEPTION(num, vec) \
	exception(num, vec);
#define	INTERRUPT(num, softintp) \
	interrupt(num, softintp, 0, 0)

void CPUCALL exception(int num, int vec);
void CPUCALL interrupt(int num, int intrtype, int errorp, int error_code);

#ifdef __cplusplus
}
#endif

#endif	/* !IA32_CPU_EXCEPTION_H__ */
