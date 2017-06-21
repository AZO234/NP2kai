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

#ifndef	IA32_CPU_INST_TABLE_H__
#define	IA32_CPU_INST_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* info of instruction */
extern UINT8 insttable_info[256];

/* table of instruction */
extern void (*insttable_1byte[2][256])(void);
extern void (*insttable_2byte[2][256])(void);


/*
 * for group
 */

/* group 1 */
extern void (CPUCALL *insttable_G1EbIb[])(UINT8 *, UINT32);
extern void (CPUCALL *insttable_G1EwIx[])(UINT16 *, UINT32);
extern void (CPUCALL *insttable_G1EdIx[])(UINT32 *, UINT32);
extern void (CPUCALL *insttable_G1EbIb_ext[])(UINT32, UINT32);
extern void (CPUCALL *insttable_G1EwIx_ext[])(UINT32, UINT32);
extern void (CPUCALL *insttable_G1EdIx_ext[])(UINT32, UINT32);

/* group 2 */
extern void (CPUCALL *insttable_G2Eb[])(UINT8 *);
extern void (CPUCALL *insttable_G2Ew[])(UINT16 *);
extern void (CPUCALL *insttable_G2Ed[])(UINT32 *);
extern void (CPUCALL *insttable_G2EbCL[])(UINT8 *, UINT);
extern void (CPUCALL *insttable_G2EwCL[])(UINT16 *, UINT);
extern void (CPUCALL *insttable_G2EdCL[])(UINT32 *, UINT);
extern void (CPUCALL *insttable_G2Eb_ext[])(UINT32);
extern void (CPUCALL *insttable_G2Ew_ext[])(UINT32);
extern void (CPUCALL *insttable_G2Ed_ext[])(UINT32);
extern void (CPUCALL *insttable_G2EbCL_ext[])(UINT32, UINT);
extern void (CPUCALL *insttable_G2EwCL_ext[])(UINT32, UINT);
extern void (CPUCALL *insttable_G2EdCL_ext[])(UINT32, UINT);

/* group 3 */
extern void (CPUCALL *insttable_G3Eb[])(UINT32);
extern void (CPUCALL *insttable_G3Ew[])(UINT32);
extern void (CPUCALL *insttable_G3Ed[])(UINT32);

/* group 4 */
extern void (CPUCALL *insttable_G4[])(UINT32);

/* group 5 */
extern void (CPUCALL *insttable_G5Ew[])(UINT32);
extern void (CPUCALL *insttable_G5Ed[])(UINT32);

/* group 6 */
extern void (CPUCALL *insttable_G6[])(UINT32);

/* group 7 */
extern void (CPUCALL *insttable_G7[])(UINT32);

/* group 8 */
extern void (CPUCALL *insttable_G8EwIb[])(UINT32);
extern void (CPUCALL *insttable_G8EdIb[])(UINT32);

/* group 9 */
extern void (CPUCALL *insttable_G9[])(UINT32);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INST_TABLE_H__ */
