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

#ifndef	IA32_CPU_IA32XC_MCR__
#define	IA32_CPU_IA32XC_MCR__

#if defined(IA32_CROSS_CHECK) && defined(GCC_CPU_ARCH_IA32)

#define	IA32_CPU_ENABLE_XC

/*
 * arith
 */
#define	XC_ADD_BYTE(r, d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __d = (d) & 0xff; \
	UINT8 __r = __d; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_ADD_BYTE((r), (d), (s)); \
	__R = (r) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushl %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"addb %4, %%al\n\t" \
		"movb %%al, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ADD_BYTE: __s = %02x, __d = %02x", __s, __d); \
		ia32_warning("XC_ADD_BYTE: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_ADD_BYTE: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_ADD_BYTE: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_ADD_WORD(r, d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_ADD_WORD((r), (d), (s)); \
	__R = (r) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushl %%eax\n\t" \
		"movw %3, %%ax\n\t" \
		"addw %4, %%ax\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax", "ecx"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ADD_WORD: __s = %04x, __d = %04x", __s, __d); \
		ia32_warning("XC_ADD_WORD: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_ADD_WORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_ADD_WORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_ADD_DWORD(r, d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_ADD_DWORD((r), (d), (s)); \
	__R = (r); \
	\
	__asm__ __volatile__ ( \
		"pushl %%eax\n\t" \
		"movl %3, %%eax\n\t" \
		"addl %4, %%eax\n\t" \
		"movl %%eax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax", "ecx"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ADD_DWORD: __s = %08x, __d = %08x", __s, __d);\
		ia32_warning("XC_ADD_DWORD: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_ADD_DWORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_ADD_DWORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_OR_BYTE(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __d = (d) & 0xff; \
	UINT8 __r = __d; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_OR_BYTE((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushl %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"orb %4, %%al\n\t" \
		"movb %%al, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_OR_BYTE: __s = %02x, __d = %02x", __s, __d); \
		ia32_warning("XC_OR_BYTE: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_OR_BYTE: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_OR_BYTE: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_OR_WORD(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_OR_WORD((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushl %%eax\n\t" \
		"movw %3, %%ax\n\t" \
		"orw %4, %%ax\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_OR_WORD: __s = %04x, __d = %04x", __s, __d); \
		ia32_warning("XC_OR_WORD: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_OR_WORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_OR_WORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_OR_DWORD(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_OR_DWORD((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushl %%eax\n\t" \
		"movl %3, %%eax\n\t" \
		"orl %4, %%eax\n\t" \
		"movl %%eax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_OR_DWORD: __s = %08x, __d = %08x", __s, __d); \
		ia32_warning("XC_OR_DWORD: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_OR_DWORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_OR_DWORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

/* flag no check */
#define	XC_ADC_BYTE(r, d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __d = (d) & 0xff; \
	UINT8 __r = __d; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_ADC_BYTE((r), (d), (s)); \
	__R = (r) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"adcb %4, %%al\n\t" \
		"movb %%al, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ADC_BYTE: __s = %02x, __d = %02x", __s, __d); \
		ia32_warning("XC_ADC_BYTE: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_ADC_BYTE: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_ADC_BYTE: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_ADC_WORD(r, d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_ADC_WORD((r), (d), (s)); \
	__R = (r) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"movw %3, %%ax\n\t" \
		"adcw %4, %%ax\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ADC_WORD: __s = %04x, __d = %04x", __s, __d); \
		ia32_warning("XC_ADC_WORD: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_ADC_WORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_ADC_WORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_ADC_DWORD(r, d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_ADC_DWORD((r), (d), (s)); \
	__R = (r); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"movl %3, %%eax\n\t" \
		"adcl %4, %%eax\n\t" \
		"movl %%eax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ADC_DWORD: __s = %08x, __d = %08x", __s, __d);\
		ia32_warning("XC_ADC_DWORD: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_ADC_DWORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_ADC_DWORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

/* flag no check */
#define	XC_BYTE_SBB(r, d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __d = (d) & 0xff; \
	UINT8 __r = __d; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_BYTE_SBB((r), (d), (s)); \
	__R = (r) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"sbbb %4, %%al\n\t" \
		"movb %%al, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_SBB: __s = %02x, __d = %02x", __s, __d); \
		ia32_warning("XC_BYTE_SBB: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_SBB: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_BYTE_SBB: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SBB(r, d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_WORD_SBB((r), (d), (s)); \
	__R = (r) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"movw %3, %%ax\n\t" \
		"sbbw %4, %%ax\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_SBB: __s = %04x, __d = %04x", __s, __d); \
		ia32_warning("XC_WORD_SBB: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_SBB: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_WORD_SBB: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SBB(r, d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_DWORD_SBB((r), (d), (s)); \
	__R = (r); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"movl %3, %%eax\n\t" \
		"sbbl %4, %%eax\n\t" \
		"movl %%eax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_SBB: __s = %08x, __d = %08x", __s, __d);\
		ia32_warning("XC_DWORD_SBB: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_SBB: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_DWORD_SBB: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_AND_BYTE(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __d = (d) & 0xff; \
	UINT8 __r = __d; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_AND_BYTE((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"andb %4, %%al\n\t" \
		"movb %%al, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ANDBYTE: __s = %02x, __d = %02x", __s, __d); \
		ia32_warning("XC_ANDBYTE: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_ANDBYTE: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_ANDBYTE: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_AND_WORD(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_AND_WORD((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movw %3, %%ax\n\t" \
		"andw %4, %%ax\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ANDWORD: __s = %04x, __d = %04x", __s, __d); \
		ia32_warning("XC_ANDWORD: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_ANDWORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_ANDWORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_AND_DWORD(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_AND_DWORD((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movl %3, %%eax\n\t" \
		"andl %4, %%eax\n\t" \
		"movl %%eax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_ANDDWORD: __s = %08x, __d = %08x", __s, __d); \
		ia32_warning("XC_ANDDWORD: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_ANDDWORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_ANDDWORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_SUB(r, d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __d = (d) & 0xff; \
	UINT8 __r = __d; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_SUB((r), (d), (s)); \
	__R = (r) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"subb %4, %%al\n\t" \
		"movb %%al, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_SUB: __s = %02x, __d = %02x", __s, __d); \
		ia32_warning("XC_BYTE_SUB: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_SUB: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_BYTE_SUB: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SUB(r, d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SUB((r), (d), (s)); \
	__R = (r) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movw %3, %%ax\n\t" \
		"subw %4, %%ax\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_SUB: __s = %04x, __d = %04x", __s, __d); \
		ia32_warning("XC_WORD_SUB: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_SUB: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_WORD_SUB: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SUB(r, d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SUB((r), (d), (s)); \
	__R = (r); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movl %3, %%eax\n\t" \
		"subl %4, %%eax\n\t" \
		"movl %%eax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_SUB: __s = %08x, __d = %08x", __s, __d);\
		ia32_warning("XC_DWORD_SUB: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_SUB: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_DWORD_SUB: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_XOR(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __d = (d) & 0xff; \
	UINT8 __r = __d; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_XOR((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"xorb %4, %%al\n\t" \
		"movb %%al, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_XORBYTE: __s = %02x, __d = %02x", __s, __d); \
		ia32_warning("XC_XORBYTE: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_XORBYTE: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_XORBYTE: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_XOR(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_XOR((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movw %3, %%ax\n\t" \
		"xorw %4, %%ax\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_XORWORD: __s = %04x, __d = %04x", __s, __d); \
		ia32_warning("XC_XORWORD: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_XORWORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_XORWORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_XOR(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_XOR((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movl %3, %%eax\n\t" \
		"xorl %4, %%eax\n\t" \
		"movl %%eax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_XORDWORD: __s = %08x, __d = %08x", __s, __d); \
		ia32_warning("XC_XORDWORD: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_XORDWORD: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZP_FLAG); \
		ia32_warning("XC_XORDWORD: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_NEG(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_NEG((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"negb %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_NEG: __s = %02x", __s); \
		ia32_warning("XC_BYTE_NEG: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_NEG: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_BYTE_NEG: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_NEG(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_NEG((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"negw %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_NEG: __s = %04x", __s); \
		ia32_warning("XC_WORD_NEG: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_NEG: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_WORD_NEG: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_NEG(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_NEG((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"negl %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_NEG: __s = %08x", __s);\
		ia32_warning("XC_DWORD_NEG: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_NEG: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAPC_FLAG); \
		ia32_warning("XC_DWORD_NEG: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_MUL(r, d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __d = (d) & 0xff; \
	UINT16 __r; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_MUL((r), (d), (s)); \
	__R = (r) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"movb %4, %%ah\n\t" \
		"mulb %%ah\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "m" (__d), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_MUL: __s = %02x, __d = %02x", __s, __d); \
		ia32_warning("XC_BYTE_MUL: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_MUL: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_BYTE_MUL: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_MUL(r, d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT32 __r; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_MUL((r), (d), (s)); \
	__R = (r); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"push %%edx\n\t" \
		"movw %3, %%ax\n\t" \
		"movw %4, %%dx\n\t" \
		"mulw %%dx\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"andl $0x0000ffff, %0\n\t" \
		"shll $16, %%edx\n\t" \
		"orl %%edx, %0\n\t" \
		"popl %%edx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "m" (__d), "m" (__s) \
		: "eax", "edx"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_MUL: __s = %04x, __d = %04x", __s, __d); \
		ia32_warning("XC_WORD_MUL: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_WORD_MUL: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_WORD_MUL: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_MUL(r, d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r; \
	UINT32 __h; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_MUL((r), (d), (s)); \
	__R = (r); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"push %%edx\n\t" \
		"movl %4, %%eax\n\t" \
		"movl %5, %%edx\n\t" \
		"mull %%edx\n\t" \
		"movl %%eax, %0\n\t" \
		"movl %%edx, %1\n\t" \
		"lahf\n\t" \
		"movb %%ah, %2\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %3\n\t" \
		"popl %%edx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__h), "=m" (__f), "=m" (__o) \
		: "m" (__d), "m" (__s) \
		: "eax", "edx"); \
	if ((__R != __r) || \
	    (CPU_OV != __h) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_MUL: __s = %08x, __d = %08x", __s, __d);\
		ia32_warning("XC_DWORD_MUL: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_MUL: CPU_OV == %08x, __h == %08x", \
		    CPU_OV, __h); \
		ia32_warning("XC_DWORD_MUL: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_DWORD_MUL: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_IMUL(r, d, s) \
do { \
	SINT8 __s = (s) & 0xff; \
	SINT8 __d = (d) & 0xff; \
	SINT16 __R; \
	SINT16 __r; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_IMUL((r), (d), (s)); \
	__R = (r) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"movb %3, %%al\n\t" \
		"movb %4, %%ah\n\t" \
		"imulb %%ah\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "m" (__d), "m" (__s) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_IMUL: __s = %02x, __d = %02x", __s, __d);\
		ia32_warning("XC_BYTE_IMUL: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_IMUL: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_BYTE_IMUL: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_IMUL(r, d, s) \
do { \
	SINT16 __s = (s) & 0xffff; \
	SINT16 __d = (d) & 0xffff; \
	SINT32 __r; \
	SINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_IMUL((r), (d), (s)); \
	__R = (r); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"push %%edx\n\t" \
		"movw %3, %%ax\n\t" \
		"movw %4, %%dx\n\t" \
		"imulw %%dx\n\t" \
		"movw %%ax, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"andl $0x0000ffff, %0\n\t" \
		"shll $16, %%edx\n\t" \
		"orl %%edx, %0\n\t" \
		"popl %%edx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "m" (__d), "m" (__s) \
		: "eax", "edx"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_IMUL: __s = %04x, __d = %04x", __s, __d);\
		ia32_warning("XC_WORD_IMUL: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_WORD_IMUL: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_WORD_IMUL: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_IMUL(r, d, s) \
do { \
	SINT64 __R; \
	SINT32 __s = (s); \
	SINT32 __d = (d); \
	UINT32 __r; \
	UINT32 __h; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_IMUL((r), (d), (s)); \
	__R = (r); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"push %%edx\n\t" \
		"movl %4, %%eax\n\t" \
		"movl %5, %%edx\n\t" \
		"imull %%edx\n\t" \
		"movl %%eax, %0\n\t" \
		"movl %%edx, %1\n\t" \
		"lahf\n\t" \
		"movb %%ah, %2\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %3\n\t" \
		"popl %%edx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__h), "=m" (__f), "=m" (__o) \
		: "m" (__d), "m" (__s) \
		: "eax", "edx"); \
	if (((UINT32)__R != __r) || \
	    ((UINT32)(__R >> 32) != __h) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_IMUL: __s = %08x, __d = %08x",__s, __d);\
		ia32_warning("XC_DWORD_IMUL: __Rl = %08x, __r = %08x", \
		    (UINT32)__R, __r); \
		ia32_warning("XC_DWORD_IMUL: __Rh == %08x, __h == %08x", \
		    (UINT32)(__R >> 32), __h); \
		ia32_warning("XC_DWORD_IMUL: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_DWORD_IMUL: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert((UINT32)__R == __r); \
		assert((UINT32)(__R >> 32) == __h); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

/* flag no check */
#define	XC_BYTE_INC(s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_INC((s)); \
	__R = (s) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"incb %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_INC: __s = %02x", __s); \
		ia32_warning("XC_BYTE_INC: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_INC: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAP_FLAG); \
		ia32_warning("XC_BYTE_INC: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_INC(s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_INC((s)); \
	__R = (s) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"incw %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_INC: __s = %04x", __s); \
		ia32_warning("XC_WORD_INC: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_INC: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAP_FLAG); \
		ia32_warning("XC_WORD_INC: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_INC(s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_INC((s)); \
	__R = (s); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"incl %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_INC: __s = %08x", __s); \
		ia32_warning("XC_DWORD_INC: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_INC: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAP_FLAG); \
		ia32_warning("XC_DWORD_INC: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

/* flag no check */
#define	XC_BYTE_DEC(s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_DEC((s)); \
	__R = (s) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"decb %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_DEC: __s = %02x", __s); \
		ia32_warning("XC_BYTE_DEC: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_DEC: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAP_FLAG); \
		ia32_warning("XC_BYTE_DEC: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_DEC(s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_DEC((s)); \
	__R = (s) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"decw %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_DEC: __s = %04x", __s); \
		ia32_warning("XC_WORD_DEC: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_DEC: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAP_FLAG); \
		ia32_warning("XC_WORD_DEC: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_DEC(s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_DEC((s)); \
	__R = (s); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"push %%eax\n\t" \
		"decl %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & SZAP_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_DEC: __s = %08x", __s); \
		ia32_warning("XC_DWORD_DEC: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_DEC: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZAP_FLAG); \
		ia32_warning("XC_DWORD_DEC: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZAP_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	BYTE_ADD(r, d, s)	XC_ADD_BYTE(r, d, s)
#define	WORD_ADD(r, d, s)	XC_ADD_WORD(r, d, s)
#define	DWORD_ADD(r, d, s)	XC_ADD_DWORD(r, d, s)
#define	BYTE_OR(d, s)		XC_OR_BYTE(d, s)
#define	WORD_OR(d, s)		XC_OR_WORD(d, s)
#define	DWORD_OR(d, s)		XC_OR_DWORD(d, s)
#define	BYTE_ADC(r, d, s)	XC_ADC_BYTE(r, d, s)
#define	WORD_ADC(r, d, s)	XC_ADC_WORD(r, d, s)
#define	DWORD_ADC(r, d, s)	XC_ADC_DWORD(r, d, s)
#define	BYTE_SBB(r, d, s)	XC_BYTE_SBB(r, d, s)
#define	WORD_SBB(r, d, s)	XC_WORD_SBB(r, d, s)
#define	DWORD_SBB(r, d, s)	XC_DWORD_SBB(r, d, s)
#define	BYTE_AND(d, s)		XC_AND_BYTE(d, s)
#define	WORD_AND(d, s)		XC_AND_WORD(d, s)
#define	DWORD_AND(d, s)		XC_AND_DWORD(d, s)
#define	BYTE_SUB(r, d, s)	XC_BYTE_SUB(r, d, s)
#define	WORD_SUB(r, d, s)	XC_WORD_SUB(r, d, s)
#define	DWORD_SUB(r, d, s)	XC_DWORD_SUB(r, d, s)
#define	BYTE_XOR(d, s)		XC_BYTE_XOR(d, s)
#define	WORD_XOR(d, s)		XC_WORD_XOR(d, s)
#define	DWORD_XOR(d, s)		XC_DWORD_XOR(d, s)
#define	BYTE_NEG(d, s)		XC_BYTE_NEG(d, s)
#define	WORD_NEG(d, s)		XC_WORD_NEG(d, s)
#define	DWORD_NEG(d, s)		XC_DWORD_NEG(d, s)
#define	BYTE_MUL(r, d, s)	XC_BYTE_MUL(r, d, s)
#define	WORD_MUL(r, d, s)	XC_WORD_MUL(r, d, s)
#define	DWORD_MUL(r, d, s)	XC_DWORD_MUL(r, d, s)
#define	BYTE_IMUL(r, d, s)	XC_BYTE_IMUL(r, d, s)
#define	WORD_IMUL(r, d, s)	XC_WORD_IMUL(r, d, s)
#define	DWORD_IMUL(r, d, s)	XC_DWORD_IMUL(r, d, s)
#define	BYTE_INC(s)		XC_BYTE_INC(s)
#define	WORD_INC(s)		XC_WORD_INC(s)
#define	DWORD_INC(s)		XC_DWORD_INC(s)
#define	BYTE_DEC(s)		XC_BYTE_DEC(s)
#define	WORD_DEC(s)		XC_WORD_DEC(s)
#define	DWORD_DEC(s)		XC_DWORD_DEC(s)

#define	ADD_BYTE(r, d, s)	XC_ADD_BYTE(r, d, s)
#define	ADD_WORD(r, d, s)	XC_ADD_WORD(r, d, s)
#define	ADD_DWORD(r, d, s)	XC_ADD_DWORD(r, d, s)
#define	OR_BYTE(d, s)		XC_OR_BYTE(d, s)
#define	OR_WORD(d, s)		XC_OR_WORD(d, s)
#define	OR_DWORD(d, s)		XC_OR_DWORD(d, s)
#define	ADC_BYTE(r, d, s)	XC_ADC_BYTE(r, d, s)
#define	ADC_WORD(r, d, s)	XC_ADC_WORD(r, d, s)
#define	ADC_DWORD(r, d, s)	XC_ADC_DWORD(r, d, s)
#define	AND_BYTE(d, s)		XC_AND_BYTE(d, s)
#define	AND_WORD(d, s)		XC_AND_WORD(d, s)
#define	AND_DWORD(d, s)		XC_AND_DWORD(d, s)

#define	XC_STORE_FLAGL()	UINT8 __xc_flagl = CPU_FLAGL

#elif defined(IA32_CROSS_CHECK) && defined(_MSC_VER)

#include	"ia32xc_msc.mcr"

#else	/* !(IA32_CROSS_CHECK && __GNUC__ && (i386) || __i386__)) */

#define	BYTE_ADD(r, d, s)	_ADD_BYTE(r, d, s)
#define	WORD_ADD(r, d, s)	_ADD_WORD(r, d, s)
#define	DWORD_ADD(r, d, s)	_ADD_DWORD(r, d, s)
#define	BYTE_OR(d, s)		_OR_BYTE(d, s)
#define	WORD_OR(d, s)		_OR_WORD(d, s)
#define	DWORD_OR(d, s)		_OR_DWORD(d, s)
#define	BYTE_ADC(r, d, s)	_ADC_BYTE(r, d, s)
#define	WORD_ADC(r, d, s)	_ADC_WORD(r, d, s)
#define	DWORD_ADC(r, d, s)	_ADC_DWORD(r, d, s)
#define	BYTE_SBB(r, d, s)	_BYTE_SBB(r, d, s)
#define	WORD_SBB(r, d, s)	_WORD_SBB(r, d, s)
#define	DWORD_SBB(r, d, s)	_DWORD_SBB(r, d, s)
#define	BYTE_AND(d, s)		_AND_BYTE(d, s)
#define	WORD_AND(d, s)		_AND_WORD(d, s)
#define	DWORD_AND(d, s)		_AND_DWORD(d, s)
#define	BYTE_SUB(r, d, s)	_BYTE_SUB(r, d, s)
#define	WORD_SUB(r, d, s)	_WORD_SUB(r, d, s)
#define	DWORD_SUB(r, d, s)	_DWORD_SUB(r, d, s)
#define	BYTE_XOR(d, s)		_BYTE_XOR(d, s)
#define	WORD_XOR(d, s)		_WORD_XOR(d, s)
#define	DWORD_XOR(d, s)		_DWORD_XOR(d, s)
#define	BYTE_NEG(d, s)		_BYTE_NEG(d, s)
#define	WORD_NEG(d, s)		_WORD_NEG(d, s)
#define	DWORD_NEG(d, s)		_DWORD_NEG(d, s)
#define	BYTE_MUL(r, d, s)	_BYTE_MUL(r, d, s)
#define	WORD_MUL(r, d, s)	_WORD_MUL(r, d, s)
#define	DWORD_MUL(r, d, s)	_DWORD_MUL(r, d, s)
#define	BYTE_IMUL(r, d, s)	_BYTE_IMUL(r, d, s)
#define	WORD_IMUL(r, d, s)	_WORD_IMUL(r, d, s)
#define	DWORD_IMUL(r, d, s)	_DWORD_IMUL(r, d, s)
#define	BYTE_INC(s)		_BYTE_INC(s)
#define	WORD_INC(s)		_WORD_INC(s)
#define	DWORD_INC(s)		_DWORD_INC(s)
#define	BYTE_DEC(s)		_BYTE_DEC(s)
#define	WORD_DEC(s)		_WORD_DEC(s)
#define	DWORD_DEC(s)		_DWORD_DEC(s)

#define	ADD_BYTE(r, d, s)	_ADD_BYTE(r, d, s)
#define	ADD_WORD(r, d, s)	_ADD_WORD(r, d, s)
#define	ADD_DWORD(r, d, s)	_ADD_DWORD(r, d, s)
#define	OR_BYTE(d, s)		_OR_BYTE(d, s)
#define	OR_WORD(d, s)		_OR_WORD(d, s)
#define	OR_DWORD(d, s)		_OR_DWORD(d, s)
#define	ADC_BYTE(r, d, s)	_ADC_BYTE(r, d, s)
#define	ADC_WORD(r, d, s)	_ADC_WORD(r, d, s)
#define	ADC_DWORD(r, d, s)	_ADC_DWORD(r, d, s)
#define	AND_BYTE(d, s)		_AND_BYTE(d, s)
#define	AND_WORD(d, s)		_AND_WORD(d, s)
#define	AND_DWORD(d, s)		_AND_DWORD(d, s)

#define	XC_STORE_FLAGL()

#endif	/* IA32_CROSS_CHECK && GCC_CPU_ARCH_IA32 */

#endif	/* IA32_CPU_IA32_MCR__ */
