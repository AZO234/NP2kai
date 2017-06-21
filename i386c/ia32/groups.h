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

#ifndef	IA32_CPU_GROUPS_H__
#define	IA32_CPU_GROUPS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* group 1 */
void Grp1_EbIb(void);
void Grp1_EwIb(void);
void Grp1_EdIb(void);
void Grp1_EwIw(void);
void Grp1_EdId(void);

/* group 2 */
void Grp2_Eb(void);
void Grp2_Ew(void);
void Grp2_Ed(void);
void Grp2_EbCL(void);
void Grp2_EwCL(void);
void Grp2_EdCL(void);
void Grp2_EbIb(void);
void Grp2_EwIb(void);
void Grp2_EdIb(void);

/* group 3 */
void Grp3_Eb(void);
void Grp3_Ew(void);
void Grp3_Ed(void);

/* group 4 */
void Grp4(void);

/* group 5 */
void Grp5_Ew(void);
void Grp5_Ed(void);

/* group 6 */
void Grp6(void);

/* group 7 */
void Grp7(void);

/* group 8 */
void Grp8_EwIb(void);
void Grp8_EdIb(void);

/* group 9 */
void Grp9(void);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_GROUPS_H__ */
