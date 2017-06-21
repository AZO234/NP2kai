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

#ifndef	IA32_CPU_INSTRUCTION_SHIFT_ROTATEXC_H__
#define	IA32_CPU_INSTRUCTION_SHIFT_ROTATEXC_H__

#define	XC_BYTE_SAR1(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_SAR1((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"sarb $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_SAR1: __s = %02x", __s); \
		ia32_warning("XC_BYTE_SAR1: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_SAR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZPC_FLAG); \
		ia32_warning("XC_BYTE_SAR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SAR1(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SAR1((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"sarw $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_SAR1: __s = %04x", __s); \
		ia32_warning("XC_WORD_SAR1: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_SAR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZPC_FLAG); \
		ia32_warning("XC_WORD_SAR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SAR1(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SAR1((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"sarl $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_SAR1: __s = %08x", __s); \
		ia32_warning("XC_DWORD_SAR1: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_SAR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZPC_FLAG); \
		ia32_warning("XC_DWORD_SAR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_SARCL(d, s, c) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_SARCL((d), (s), (c)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"sarb %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_BYTE_SARCL: __s = %02x", __s); \
			ia32_warning("XC_BYTE_SARCL: __c = %02x", __c); \
			ia32_warning("XC_BYTE_SARCL: __R = %02x, __r = %02x", \
			    __R, __r); \
			ia32_warning("XC_BYTE_SARCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_BYTE_SARCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SARCL(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SARCL((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"sarw %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_SARCL: __s = %04x", __s); \
			ia32_warning("XC_WORD_SARCL: __c = %02x", __c); \
			ia32_warning("XC_WORD_SARCL: __R = %04x, __r = %04x", \
			    __R, __r); \
			ia32_warning("XC_WORD_SARCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_WORD_SARCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SARCL(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SARCL((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"sarl %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_SARCL: __s = %08x", __s); \
			ia32_warning("XC_DWORD_SARCL: __c = %02x", __c); \
			ia32_warning("XC_DWORD_SARCL: __R = %08x, __r = %08x", \
			    __R, __r); \
			ia32_warning("XC_DWORD_SARCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_DWORD_SARCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_SHR1(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_SHR1((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"shrb $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_SHR1: __s = %02x", __s); \
		ia32_warning("XC_BYTE_SHR1: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_SHR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZPC_FLAG); \
		ia32_warning("XC_BYTE_SHR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SHR1(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SHR1((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"shrw $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_SHR1: __s = %04x", __s); \
		ia32_warning("XC_WORD_SHR1: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_SHR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZPC_FLAG); \
		ia32_warning("XC_WORD_SHR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SHR1(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SHR1((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"shrl $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_SHR1: __s = %08x", __s); \
		ia32_warning("XC_DWORD_SHR1: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_SHR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZPC_FLAG); \
		ia32_warning("XC_DWORD_SHR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_SHRCL(d, s, c) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_SHRCL((d), (s), (c)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"shrb %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_BYTE_SHRCL: __s = %02x", __s); \
			ia32_warning("XC_BYTE_SHRCL: __c = %02x", __c); \
			ia32_warning("XC_BYTE_SHRCL: __R = %02x, __r = %02x", \
			    __R, __r); \
			ia32_warning("XC_BYTE_SHRCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_BYTE_SHRCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SHRCL(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SHRCL((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"shrw %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_SHRCL: __s = %04x", __s); \
			ia32_warning("XC_WORD_SHRCL: __c = %02x", __c); \
			ia32_warning("XC_WORD_SHRCL: __R = %04x, __r = %04x", \
			    __R, __r); \
			ia32_warning("XC_WORD_SHRCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_WORD_SHRCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SHRCL(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SHRCL((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"shrl %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_SHRCL: __s = %08x", __s); \
			ia32_warning("XC_DWORD_SHRCL: __c = %02x", __c); \
			ia32_warning("XC_DWORD_SHRCL: __R = %08x, __r = %08x", \
			    __R, __r); \
			ia32_warning("XC_DWORD_SHRCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_DWORD_SHRCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_SHL1(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_SHL1((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"shlb $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_SHL1: __s = %02x", __s); \
		ia32_warning("XC_BYTE_SHL1: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_SHL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZPC_FLAG); \
		ia32_warning("XC_BYTE_SHL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/0)

#define	XC_WORD_SHL1(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SHL1((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"shlw $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_SHL1: __s = %04x", __s); \
		ia32_warning("XC_WORD_SHL1: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_SHL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, SZPC_FLAG); \
		ia32_warning("XC_WORD_SHL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SHL1(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SHL1((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"shll $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_SHL1: __s = %08x", __s); \
		ia32_warning("XC_DWORD_SHL1: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_SHL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_DWORD_SHL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_SHLCL(d, s, c) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_SHLCL((d), (s), (c)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"shlb %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_BYTE_SHLCL: __s = %02x", __s); \
			ia32_warning("XC_BYTE_SHLCL: __c = %02x", __c); \
			ia32_warning("XC_BYTE_SHLCL: __R = %02x, __r = %02x", \
			    __R, __r); \
			ia32_warning("XC_BYTE_SHLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_BYTE_SHLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SHLCL(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SHLCL((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"shlw %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_SHLCL: __s = %04x", __s); \
			ia32_warning("XC_WORD_SHLCL: __c = %02x", __c); \
			ia32_warning("XC_WORD_SHLCL: __R = %04x, __r = %04x", \
			    __R, __r); \
			ia32_warning("XC_WORD_SHLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_WORD_SHLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SHLCL(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SHLCL((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"shll %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_SHLCL: __s = %08x", __s); \
			ia32_warning("XC_DWORD_SHLCL: __c = %02x", __c); \
			ia32_warning("XC_DWORD_SHLCL: __R = %08x, __r = %08x", \
			    __R, __r); \
			ia32_warning("XC_DWORD_SHLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_DWORD_SHLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SHRD(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SHRD((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"pushl %%edx\n\t" \
		"movb %5, %%cl\n\t" \
		"movw %4, %%dx\n\t" \
		"shrdw %%cl, %%dx, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%edx\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__c) \
		: "eax", "ecx", "edx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_SHRD: __s = %04x, __d = %04x", \
			    __s, __d); \
			ia32_warning("XC_WORD_SHRD: __c = %02x", __c); \
			ia32_warning("XC_WORD_SHRD: __R = %04x, __r = %04x",\
			    __R, __r); \
			ia32_warning("XC_WORD_SHRD: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_WORD_SHRD: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert((__c != 1) || (!CPU_OV == !__o)); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SHRD(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SHRD((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"pushl %%edx\n\t" \
		"movb %5, %%cl\n\t" \
		"movl %4, %%edx\n\t" \
		"shrdl %%cl, %%edx, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%edx\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__c) \
		: "eax", "ecx", "edx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_SHRD: __s = %08x, __d = %08x", \
			    __s, __d); \
			ia32_warning("XC_DWORD_SHRD: __c = %02x", __c); \
			ia32_warning("XC_DWORD_SHRD: __R = %08x, __r = %08x",\
			    __R, __r); \
			ia32_warning("XC_DWORD_SHRD: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_DWORD_SHRD: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert((__c != 1) || (!CPU_OV == !__o)); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_SHLD(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __d = (d) & 0xffff; \
	UINT16 __r = __d; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_SHLD((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"pushl %%edx\n\t" \
		"movb %5, %%cl\n\t" \
		"movw %4, %%dx\n\t" \
		"shldw %%cl, %%dx, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%edx\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__c) \
		: "eax", "ecx", "edx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_SHLD: __s = %04x, __d = %04x", \
			    __s, __d); \
			ia32_warning("XC_WORD_SHLD: __c = %02x", __c); \
			ia32_warning("XC_WORD_SHLD: __R = %04x, __r = %04x", \
			    __R, __r); \
			ia32_warning("XC_WORD_SHLD: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_WORD_SHLD: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert((__c != 1) || (!CPU_OV == !__o)); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_SHLD(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __d = (d); \
	UINT32 __r = __d; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_SHLD((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"pushl %%edx\n\t" \
		"movb %5, %%cl\n\t" \
		"movl %4, %%edx\n\t" \
		"shldl %%cl, %%edx, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%edx\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__s), "m" (__c) \
		: "eax", "ecx", "edx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & SZPC_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_SHLD: __s = %08x, __d = %08x", \
			    __s, __d); \
			ia32_warning("XC_DWORD_SHLD: __c = %02x", __c); \
			ia32_warning("XC_DWORD_SHLD: __R = %08x, __r = %08x", \
			    __R, __r); \
			ia32_warning("XC_DWORD_SHLD: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, SZPC_FLAG); \
			ia32_warning("XC_DWORD_SHLD: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & SZPC_FLAG) == 0); \
			assert((__c != 1) || (!CPU_OV == !__o)); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_ROR1(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_ROR1((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"rorb $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_ROR1: __s = %02x", __s); \
		ia32_warning("XC_BYTE_ROR1: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_ROR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_BYTE_ROR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_ROR1(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_ROR1((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"rorw $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_ROR1: __s = %04x", __s); \
		ia32_warning("XC_WORD_ROR1: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_ROR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_WORD_ROR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_ROR1(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_ROR1((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"rorl $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_ROR1: __s = %08x", __s); \
		ia32_warning("XC_DWORD_ROR1: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_ROR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_DWORD_ROR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_RORCL(d, s, c) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_RORCL((d), (s), (c)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"rorb %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_BYTE_RORCL: __s = %02x", __s); \
			ia32_warning("XC_BYTE_RORCL: __c = %02x", __c); \
			ia32_warning("XC_BYTE_RORCL: __R = %02x, __r = %02x", \
			    __R, __r); \
			ia32_warning("XC_BYTE_RORCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_BYTE_RORCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_RORCL(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_RORCL((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"rorw %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_RORCL: __s = %04x", __s); \
			ia32_warning("XC_WORD_RORCL: __c = %02x", __c); \
			ia32_warning("XC_WORD_RORCL: __R = %04x, __r = %04x", \
			    __R, __r); \
			ia32_warning("XC_WORD_RORCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_WORD_RORCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_RORCL(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_RORCL((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"rorl %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_RORCL: __s = %08x", __s); \
			ia32_warning("XC_DWORD_RORCL: __c = %02x", __c); \
			ia32_warning("XC_DWORD_RORCL: __R = %08x, __r = %08x", \
			    __R, __r); \
			ia32_warning("XC_DWORD_RORCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_DWORD_RORCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_ROL1(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_ROL1((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"rolb $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_ROL1: __s = %02x", __s); \
		ia32_warning("XC_BYTE_ROL1: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_ROL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_BYTE_ROL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_ROL1(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_ROL1((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"rolw $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_ROL1: __s = %04x", __s); \
		ia32_warning("XC_WORD_ROL1: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_ROL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_WORD_ROL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_ROL1(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_ROL1((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"roll $1, %0\n\t" \
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
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_ROL1: __s = %08x", __s); \
		ia32_warning("XC_DWORD_ROL1: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_ROL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_DWORD_ROL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_ROLCL(d, s, c) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_BYTE_ROLCL((d), (s), (c)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"rolb %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_BYTE_ROLCL: __s = %02x", __s); \
			ia32_warning("XC_BYTE_ROLCL: __c = %02x", __c); \
			ia32_warning("XC_BYTE_ROLCL: __R = %02x, __r = %02x", \
			    __R, __r); \
			ia32_warning("XC_BYTE_ROLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_BYTE_ROLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_ROLCL(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_WORD_ROLCL((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"rolw %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_ROLCL: __s = %04x", __s); \
			ia32_warning("XC_WORD_ROLCL: __c = %02x", __c); \
			ia32_warning("XC_WORD_ROLCL: __R = %04x, __r = %04x", \
			    __R, __r); \
			ia32_warning("XC_WORD_ROLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_WORD_ROLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_ROLCL(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	\
	_DWORD_ROLCL((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"roll %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
	    : "=m" (__r), "=m" (__f), "=m" (__o) \
	    : "0" (__r), "m" (__c) \
	    : "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_ROLCL: __s = %08x", __s); \
			ia32_warning("XC_DWORD_ROLCL: __c = %02x", __c); \
			ia32_warning("XC_DWORD_ROLCL: __R = %08x, __r = %08x", \
			    __R, __r); \
			ia32_warning("XC_DWORD_ROLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_DWORD_ROLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_RCR1(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_BYTE_RCR1((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"movzbl %4, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rcrb $1, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_RCR1: __s = %02x", __s); \
		ia32_warning("XC_BYTE_RCR1: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_RCR1: __xc_flagl = %02x", __xc_flagl); \
		ia32_warning("XC_BYTE_RCR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_BYTE_RCR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_RCR1(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_WORD_RCR1((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"movzbl %4, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rcrw $1, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_RCR1: __s = %04x", __s); \
		ia32_warning("XC_WORD_RCR1: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_RCR1: __xc_flagl = %02x", __xc_flagl); \
		ia32_warning("XC_WORD_RCR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_WORD_RCR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_RCR1(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_DWORD_RCR1((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"movzbl %4, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rcrl $1, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_RCR1: __s = %08x", __s); \
		ia32_warning("XC_DWORD_RCR1: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_RCR1: __xc_flagl = %02x", __xc_flagl); \
		ia32_warning("XC_DWORD_RCR1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_DWORD_RCR1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_RCRCL(d, s, c) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_BYTE_RCRCL((d), (s), (c)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rcrb %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c), "m" (__xc_flagl) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_BYTE_RCRCL: __s = %02x", __s); \
			ia32_warning("XC_BYTE_RCRCL: __c = %02x", __c); \
			ia32_warning("XC_BYTE_RCRCL: __R = %02x, __r = %02x", \
			    __R, __r); \
			ia32_warning("XC_BYTE_RCRCL: __xc_flagl = %02x", \
			    __xc_flagl); \
			ia32_warning("XC_BYTE_RCRCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_BYTE_RCRCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_RCRCL(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_WORD_RCRCL((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rcrw %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c), "m" (__xc_flagl) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_RCRCL: __s = %04x", __s); \
			ia32_warning("XC_WORD_RCRCL: __c = %02x", __c); \
			ia32_warning("XC_WORD_RCRCL: __R = %04x, __r = %04x", \
			    __R, __r); \
			ia32_warning("XC_WORD_RCRCL: __xc_flagl = %02x", \
			    __xc_flagl); \
			ia32_warning("XC_WORD_RCRCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_WORD_RCRCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_RCRCL(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_DWORD_RCRCL((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rcrl %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c), "m" (__xc_flagl) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_RCRCL: __s = %08x", __s); \
			ia32_warning("XC_DWORD_RCRCL: __c = %02x", __c); \
			ia32_warning("XC_DWORD_RCRCL: __R = %08x, __r = %08x", \
			    __R, __r); \
			ia32_warning("XC_DWORD_RCRCL: __xc_flagl = %02x", \
			    __xc_flagl); \
			ia32_warning("XC_DWORD_RCRCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_DWORD_RCRCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_RCL1(d, s) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_BYTE_RCL1((d), (s)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"movzbl %4, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rclb $1, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_BYTE_RCL1: __s = %02x", __s); \
		ia32_warning("XC_BYTE_RCL1: __R = %02x, __r = %02x", \
		    __R, __r); \
		ia32_warning("XC_BYTE_RCL1: __xc_flagl = %02x", __xc_flagl); \
		ia32_warning("XC_BYTE_RCL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_BYTE_RCL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_RCL1(d, s) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_WORD_RCL1((d), (s)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"movzbl %4, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rclw $1, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_WORD_RCL1: __s = %04x", __s); \
		ia32_warning("XC_WORD_RCL1: __R = %04x, __r = %04x", \
		    __R, __r); \
		ia32_warning("XC_WORD_RCL1: __xc_flagl = %02x", __xc_flagl); \
		ia32_warning("XC_WORD_RCL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_WORD_RCL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_RCL1(d, s) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_DWORD_RCL1((d), (s)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"movzbl %4, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rcll $1, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__xc_flagl) \
		: "eax"); \
	if ((__R != __r) || \
	    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
	    (!CPU_OV != !__o)) { \
		ia32_warning("XC_DWORD_RCL1: __s = %08x", __s); \
		ia32_warning("XC_DWORD_RCL1: __R = %08x, __r = %08x", \
		    __R, __r); \
		ia32_warning("XC_DWORD_RCL1: __xc_flagl = %02x", __xc_flagl); \
		ia32_warning("XC_DWORD_RCL1: CPU_FLAGL = %02x, __f = %02x, " \
		    "mask = %02x", CPU_FLAGL, __f, C_FLAG); \
		ia32_warning("XC_DWORD_RCL1: CPU_OV = %s, __o = %s", \
		    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
		assert(__R == __r); \
		assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
		assert(!CPU_OV == !__o); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_BYTE_RCLCL(d, s, c) \
do { \
	UINT8 __s = (s) & 0xff; \
	UINT8 __r = __s; \
	UINT8 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_BYTE_RCLCL((d), (s), (c)); \
	__R = (d) & 0xff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rclb %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c), "m" (__xc_flagl) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_BYTE_RCLCL: __s = %02x", __s); \
			ia32_warning("XC_BYTE_RCLCL: __c = %02x", __c); \
			ia32_warning("XC_BYTE_RCLCL: __R = %02x, __r = %02x", \
			    __R, __r); \
			ia32_warning("XC_BYTE_RCLCL: __xc_flagl = %02x", \
			    __xc_flagl); \
			ia32_warning("XC_BYTE_RCLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_BYTE_RCLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_WORD_RCLCL(d, s, c) \
do { \
	UINT16 __s = (s) & 0xffff; \
	UINT16 __r = __s; \
	UINT16 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_WORD_RCLCL((d), (s), (c)); \
	__R = (d) & 0xffff; \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rclw %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c), "m" (__xc_flagl) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_WORD_RCLCL: __s = %04x", __s); \
			ia32_warning("XC_WORD_RCLCL: __c = %02x", __c); \
			ia32_warning("XC_WORD_RCLCL: __R = %04x, __r = %04x", \
			    __R, __r); \
			ia32_warning("XC_WORD_RCLCL: __xc_flagl = %02x", \
			    __xc_flagl); \
			ia32_warning("XC_WORD_RCLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_WORD_RCLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#define	XC_DWORD_RCLCL(d, s, c) \
do { \
	UINT32 __s = (s); \
	UINT32 __r = __s; \
	UINT32 __R; \
	UINT8 __c = (c) & 0xff; \
	UINT8 __f; \
	UINT8 __o; \
	UINT8 __xc_flagl = CPU_FLAGL; \
	\
	_DWORD_RCLCL((d), (s), (c)); \
	__R = (d); \
	\
	__asm__ __volatile__ ( \
		"pushf\n\t" \
		"pushl %%eax\n\t" \
		"pushl %%ecx\n\t" \
		"movb %4, %%cl\n\t" \
		"movzbl %5, %%eax\n\t" \
		"bt $0, %%eax\n\t" \
		"rcll %%cl, %0\n\t" \
		"lahf\n\t" \
		"movb %%ah, %1\n\t" \
		"seto %%ah\n\t" \
		"movb %%ah, %2\n\t" \
		"popl %%ecx\n\t" \
		"popl %%eax\n\t" \
		"popf\n\t" \
		: "=m" (__r), "=m" (__f), "=m" (__o) \
		: "0" (__r), "m" (__c), "m" (__xc_flagl) \
		: "eax", "ecx"); \
	if (__c != 0) { \
		if ((__R != __r) || \
		    (((__f ^ CPU_FLAGL) & C_FLAG) != 0) || \
		    ((__c == 1) && (!CPU_OV != !__o))) { \
			ia32_warning("XC_DWORD_RCLCL: __s = %08x", __s); \
			ia32_warning("XC_DWORD_RCLCL: __c = %02x", __c); \
			ia32_warning("XC_DWORD_RCLCL: __R = %08x, __r = %08x", \
			    __R, __r); \
			ia32_warning("XC_DWORD_RCLCL: __xc_flagl = %02x", \
			    __xc_flagl); \
			ia32_warning("XC_DWORD_RCLCL: CPU_FLAGL = %02x, " \
			    "__f = %02x, mask = %02x", \
			    CPU_FLAGL, __f, C_FLAG); \
			ia32_warning("XC_DWORD_RCLCL: CPU_OV = %s, __o = %s", \
			    CPU_OV ? "OV" : "NV", __o ? "OV" : "NV"); \
			assert(__R == __r); \
			assert(((__f ^ CPU_FLAGL) & C_FLAG) == 0); \
			assert(!CPU_OV == !__o); \
		} \
	} \
} while (/*CONSTCOND*/ 0)

#endif	/* IA32_CPU_INSTRUCTION_SHIFT_ROTATEXC_H__ */
