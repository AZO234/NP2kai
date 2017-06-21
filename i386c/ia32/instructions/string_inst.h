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

#ifndef	IA32_CPU_INSTRUCTION_STRING_H__
#define	IA32_CPU_INSTRUCTION_STRING_H__

#ifdef __cplusplus
extern "C" {
#endif

#define	STRING_DIR	((CPU_FLAG & D_FLAG) ? -1 : 1)
#define	STRING_DIRx2	((CPU_FLAG & D_FLAG) ? -2 : 2)
#define	STRING_DIRx4	((CPU_FLAG & D_FLAG) ? -4 : 4)

/* movs */
void MOVSB_XbYb(void);
void MOVSW_XwYw(void);
void MOVSD_XdYd(void);

/* cmps */
void CMPSB_XbYb(void);
void CMPSW_XwYw(void);
void CMPSD_XdYd(void);

/* scas */
void SCASB_ALXb(void);
void SCASW_AXXw(void);
void SCASD_EAXXd(void);

/* lods */
void LODSB_ALXb(void);
void LODSW_AXXw(void);
void LODSD_EAXXd(void);

/* stos */
void STOSB_YbAL(void);
void STOSW_YwAX(void);
void STOSD_YdEAX(void);

/* repeat */
void _REPNE(void);
void _REPE(void);

/* ins */
void INSB_YbDX(void);
void INSW_YwDX(void);
void INSD_YdDX(void);

/* outs */
void OUTSB_DXXb(void);
void OUTSW_DXXw(void);
void OUTSD_DXXd(void);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_STRING_H__ */
