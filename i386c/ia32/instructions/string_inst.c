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

#include <compiler.h>
#include <ia32/cpu.h>
#include "ia32/ia32.mcr"

#include "string_inst.h"

#ifdef USE_SSE
#include "misc_inst.h"
#endif

#if defined(USE_CPU_BULKREP)

UINT8* MEMCALL cpu_lmemory_get_direct_host_ptr(UINT32 laddr, UINT leng, int ucrw);

/*
 * REP MOVS/STOS bulk fast path.
 *
 * This optimization is intentionally conservative.  It is used only for
 * unconditional REP MOVS/STOS, and only when the next chunk is contained in
 * one 4KB page and all guest addresses resolve to ordinary direct host RAM.
 * If any condition is not met, the code falls back to the original
 * one-element path, preserving MMIO, page fault, and segment-limit behavior.
 */
static UINT32
bulkrep_count(void)
{
	return CPU_INST_AS32 ? CPU_ECX : CPU_CX;
}

static void
bulkrep_set_count(UINT32 count)
{
	if (CPU_INST_AS32) {
		CPU_ECX = count;
	} else {
		CPU_CX = (UINT16)count;
	}
}

static UINT32
bulkrep_src_offset(void)
{
	return CPU_INST_AS32 ? CPU_ESI : CPU_SI;
}

static UINT32
bulkrep_dst_offset(void)
{
	return CPU_INST_AS32 ? CPU_EDI : CPU_DI;
}

static void
bulkrep_add_src(UINT32 bytes, int dir)
{
	if (CPU_INST_AS32) {
		CPU_ESI = (UINT32)(CPU_ESI + (dir > 0 ? bytes : -bytes));
	} else {
		CPU_SI = (UINT16)(CPU_SI + (dir > 0 ? bytes : -bytes));
	}
}

static void
bulkrep_add_dst(UINT32 bytes, int dir)
{
	if (CPU_INST_AS32) {
		CPU_EDI = (UINT32)(CPU_EDI + (dir > 0 ? bytes : -bytes));
	} else {
		CPU_DI = (UINT16)(CPU_DI + (dir > 0 ? bytes : -bytes));
	}
}

static UINT32
bulkrep_clock_limit(UINT32 count, UINT clock)
{
	UINT32 byclock;

	if (count == 0) {
		return 0;
	}
	if (CPU_REMCLOCK > 0) {
		byclock = (UINT32)((CPU_REMCLOCK + (SINT32)clock - 1) / (SINT32)clock);
		if (byclock == 0) {
			byclock = 1;
		}
		if (count > byclock) {
			count = byclock;
		}
	} else if (count > 1) {
		count = 1;
	}
	return count;
}

static UINT32
bulkrep_page_elems(int idx, UINT32 offset, UINT size, int dir)
{
	UINT32 laddr;
	UINT32 off;

	laddr = CPU_STAT_SREG(idx).u.seg.segbase + offset;
	off = laddr & CPU_PAGE_MASK;
	if (dir > 0) {
		return (CPU_PAGE_SIZE - off) / size;
	}
	if (off + size > CPU_PAGE_SIZE) {
		return 0;
	}
	return (off / size) + 1;
}

static UINT32
bulkrep_as16_elems(UINT32 offset, UINT size, int dir)
{
	if (CPU_INST_AS32) {
		return 0xffffffffUL;
	}
	if (dir > 0) {
		return ((0xffffUL - (offset & 0xffffUL)) / size) + 1;
	}
	return ((offset & 0xffffUL) / size) + 1;
}

static int
bulkrep_overlap(UINT8 *a, UINT32 alen, UINT8 *b, UINT32 blen)
{
	return (a < b + blen) && (b < a + alen);
}

static void
bulkrep_stos_pattern(UINT8 *dst, UINT32 bytes, UINT size, UINT32 value)
{
	UINT32 i;

	if (size == 1) {
		for (i = 0; i < bytes; i++) {
			dst[i] = (UINT8)value;
		}
		return;
	}
	if (size == 2) {
		for (i = 0; i < bytes; i += 2) {
			STOREINTELWORD(dst + i, (UINT16)value);
		}
		return;
	}
	for (i = 0; i < bytes; i += 4) {
		STOREINTELDWORD(dst + i, value);
	}
}

static void
bulkrep_movs_one(UINT size)
{
	UINT32 src;
	UINT32 bytes;

	CPU_WORKCLOCK(5);
	if (size == 1) {
		src = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, bulkrep_src_offset());
		cpu_vmemorywrite(CPU_ES_INDEX, bulkrep_dst_offset(), (UINT8)src);
	} else if (size == 2) {
		src = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, bulkrep_src_offset());
		cpu_vmemorywrite_w(CPU_ES_INDEX, bulkrep_dst_offset(), (UINT16)src);
	} else {
		src = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, bulkrep_src_offset());
		cpu_vmemorywrite_d(CPU_ES_INDEX, bulkrep_dst_offset(), src);
	}
	bytes = size;
	bulkrep_add_src(bytes, (CPU_FLAG & D_FLAG) ? -1 : 1);
	bulkrep_add_dst(bytes, (CPU_FLAG & D_FLAG) ? -1 : 1);
}

static void
bulkrep_stos_one(UINT size, UINT32 value)
{
	UINT32 bytes;

	CPU_WORKCLOCK(3);
	if (size == 1) {
		cpu_vmemorywrite(CPU_ES_INDEX, bulkrep_dst_offset(), (UINT8)value);
	} else if (size == 2) {
		cpu_vmemorywrite_w(CPU_ES_INDEX, bulkrep_dst_offset(), (UINT16)value);
	} else {
		cpu_vmemorywrite_d(CPU_ES_INDEX, bulkrep_dst_offset(), value);
	}
	bytes = size;
	bulkrep_add_dst(bytes, (CPU_FLAG & D_FLAG) ? -1 : 1);
}

static void
bulkrep_finish_iteration(UINT32 done)
{
	UINT32 count;

	count = bulkrep_count() - done;
	bulkrep_set_count(count);
	if (count == 0) {
#if defined(DEBUG)
		cpu_debug_rep_cont = 0;
#endif
	} else if (CPU_REMCLOCK <= 0) {
		CPU_EIP = CPU_PREV_EIP;
	}
}

static int
bulkrep_movs(UINT size)
{
	UINT32 count;
	UINT32 n;
	UINT32 src_off;
	UINT32 dst_off;
	UINT32 src_low;
	UINT32 dst_low;
	UINT32 bytes;
	UINT32 lim;
	UINT8 *srcp;
	UINT8 *dstp;
	int dir;

	CPU_INST_SEGREG_INDEX = DS_FIX;
	dir = (CPU_FLAG & D_FLAG) ? -1 : 1;
	for (;;) {
		count = bulkrep_count();
		if (count == 0) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
			return 1;
		}

		n = bulkrep_clock_limit(count, 5);
		src_off = bulkrep_src_offset();
		dst_off = bulkrep_dst_offset();
		lim = bulkrep_page_elems(CPU_INST_SEGREG_INDEX, src_off, size, dir);
		if (n > lim) n = lim;
		lim = bulkrep_page_elems(CPU_ES_INDEX, dst_off, size, dir);
		if (n > lim) n = lim;
		lim = bulkrep_as16_elems(src_off, size, dir);
		if (n > lim) n = lim;
		lim = bulkrep_as16_elems(dst_off, size, dir);
		if (n > lim) n = lim;

		if (n > 1) {
			bytes = n * size;
			if (dir > 0) {
				src_low = src_off;
				dst_low = dst_off;
			} else {
				src_low = src_off - bytes + size;
				dst_low = dst_off - bytes + size;
			}
			srcp = cpu_vmemory_get_direct_host_ptr(CPU_INST_SEGREG_INDEX, src_low,
			    bytes, CPU_PAGE_READ_DATA | CPU_STAT_USER_MODE);
			dstp = cpu_vmemory_get_direct_host_ptr(CPU_ES_INDEX, dst_low,
			    bytes, CPU_PAGE_WRITE_DATA | CPU_STAT_USER_MODE);
			if (srcp != NULL && dstp != NULL) {
				if (srcp == dstp) {
					/* Copying the exact same RAM range is a no-op. */
				} else if (!bulkrep_overlap(dstp, bytes, srcp, bytes)) {
					CopyMemory(dstp, srcp, bytes);
				} else {
					/* Overlapped MOVS can differ from memmove; use old path. */
					n = 0;
				}
				if (n != 0) {
					CPU_WORKCLOCK(5 * n);
					bulkrep_add_src(bytes, dir);
					bulkrep_add_dst(bytes, dir);
					bulkrep_finish_iteration(n);
					if (bulkrep_count() == 0 || CPU_REMCLOCK <= 0) {
						return 1;
					}
					continue;
				}
			}
		}

		bulkrep_movs_one(size);
		bulkrep_finish_iteration(1);
		if (bulkrep_count() == 0 || CPU_REMCLOCK <= 0) {
			return 1;
		}
	}
}

static int
bulkrep_stos(UINT size, UINT32 value)
{
	UINT32 count;
	UINT32 n;
	UINT32 dst_off;
	UINT32 dst_low;
	UINT32 bytes;
	UINT32 lim;
	UINT8 *dstp;
	int dir;

	dir = (CPU_FLAG & D_FLAG) ? -1 : 1;
	for (;;) {
		count = bulkrep_count();
		if (count == 0) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
			return 1;
		}

		n = bulkrep_clock_limit(count, 3);
		dst_off = bulkrep_dst_offset();
		lim = bulkrep_page_elems(CPU_ES_INDEX, dst_off, size, dir);
		if (n > lim) n = lim;
		lim = bulkrep_as16_elems(dst_off, size, dir);
		if (n > lim) n = lim;

		if (n > 1) {
			bytes = n * size;
			if (dir > 0) {
				dst_low = dst_off;
			} else {
				dst_low = dst_off - bytes + size;
			}
			dstp = cpu_vmemory_get_direct_host_ptr(CPU_ES_INDEX, dst_low,
			    bytes, CPU_PAGE_WRITE_DATA | CPU_STAT_USER_MODE);
			if (dstp != NULL) {
				bulkrep_stos_pattern(dstp, bytes, size, value);
				CPU_WORKCLOCK(3 * n);
				bulkrep_add_dst(bytes, dir);
				bulkrep_finish_iteration(n);
				if (bulkrep_count() == 0 || CPU_REMCLOCK <= 0) {
					return 1;
				}
				continue;
			}
		}

		bulkrep_stos_one(size, value);
		bulkrep_finish_iteration(1);
		if (bulkrep_count() == 0 || CPU_REMCLOCK <= 0) {
			return 1;
		}
	}
}
#endif


/* movs */
void
MOVSB_XbYb(void)
{
	UINT8 tmp;

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		tmp = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_DI, tmp);
		CPU_SI += STRING_DIR;
		CPU_DI += STRING_DIR;
	} else {
		tmp = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_EDI, tmp);
		CPU_ESI += STRING_DIR;
		CPU_EDI += STRING_DIR;
	}
}

void
MOVSW_XwYw(void)
{
	UINT16 tmp;

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		tmp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_DI, tmp);
		CPU_SI += STRING_DIRx2;
		CPU_DI += STRING_DIRx2;
	} else {
		tmp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_EDI, tmp);
		CPU_ESI += STRING_DIRx2;
		CPU_EDI += STRING_DIRx2;
	}
}

void
MOVSD_XdYd(void)
{
	UINT32 tmp;

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		tmp = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_DI, tmp);
		CPU_SI += STRING_DIRx4;
		CPU_DI += STRING_DIRx4;
	} else {
		tmp = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_EDI, tmp);
		CPU_ESI += STRING_DIRx4;
		CPU_EDI += STRING_DIRx4;
	}
}

#define	MOVSB_XbYb_rep16_part	\
  do { \
	CPU_WORKCLOCK(5);\
	tmp = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI); \
	cpu_vmemorywrite(CPU_ES_INDEX, CPU_DI, tmp); \
	CPU_SI += STRING_DIR; \
	CPU_DI += STRING_DIR; \
  } while (0)

#define	MOVSW_XwYw_rep16_part	\
  do { \
	CPU_WORKCLOCK(5);\
	tmp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI); \
	cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_DI, tmp); \
	CPU_SI += STRING_DIRx2; \
	CPU_DI += STRING_DIRx2; \
  } while (0)

#define	MOVSD_XdYd_rep16_part	\
  do { \
	CPU_WORKCLOCK(5);\
	tmp = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI); \
	cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_DI, tmp); \
	CPU_SI += STRING_DIRx4; \
	CPU_DI += STRING_DIRx4; \
  } while (0)

#define	MOVSB_XbYb_rep32_part	\
  do { \
	CPU_WORKCLOCK(5);\
	tmp = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI); \
	cpu_vmemorywrite(CPU_ES_INDEX, CPU_EDI, tmp); \
	CPU_ESI += STRING_DIR; \
	CPU_EDI += STRING_DIR; \
  } while (0)

#define	MOVSW_XwYw_rep32_part	\
  do { \
	CPU_WORKCLOCK(5);\
	tmp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI); \
	cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_EDI, tmp); \
	CPU_ESI += STRING_DIRx2; \
	CPU_EDI += STRING_DIRx2; \
  } while (0)

#define	MOVSD_XdYd_rep32_part	\
  do { \
	CPU_WORKCLOCK(5);\
	tmp = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI); \
	cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_EDI, tmp); \
	CPU_ESI += STRING_DIRx4; \
	CPU_EDI += STRING_DIRx4; \
  } while (0)

void
MOVSB_XbYb_rep(int reptype)
{
	UINT8 tmp;
#if defined(USE_CPU_BULKREP)
	if (reptype == 0 && bulkrep_movs(1)) {
		return;
	}
#endif
	/* rep */
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if(!CPU_INST_AS32){
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				MOVSB_XbYb_rep16_part;
				if (--CPU_CX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				MOVSB_XbYb_rep16_part;
				if (--CPU_CX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				MOVSB_XbYb_rep16_part;
				if (--CPU_CX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}else{
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				MOVSB_XbYb_rep32_part;
				if (--CPU_ECX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				MOVSB_XbYb_rep32_part;
				if (--CPU_ECX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				MOVSB_XbYb_rep32_part;
				if (--CPU_ECX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}
}

void
MOVSW_XwYw_rep(int reptype)
{
	UINT16 tmp;
#if defined(USE_CPU_BULKREP)
	if (reptype == 0 && bulkrep_movs(2)) {
		return;
	}
#endif
	/* rep */
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if(!CPU_INST_AS32){
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				MOVSW_XwYw_rep16_part;
				if (--CPU_CX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				MOVSW_XwYw_rep16_part;
				if (--CPU_CX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				MOVSW_XwYw_rep16_part;
				if (--CPU_CX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}else{
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				MOVSW_XwYw_rep32_part;
				if (--CPU_ECX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				MOVSW_XwYw_rep32_part;
				if (--CPU_ECX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				MOVSW_XwYw_rep32_part;
				if (--CPU_ECX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}
}

void
MOVSD_XdYd_rep(int reptype)
{
	UINT32 tmp;
#if defined(USE_CPU_BULKREP)
	if (reptype == 0 && bulkrep_movs(4)) {
		return;
	}
#endif
	/* rep */
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if(!CPU_INST_AS32){
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				MOVSD_XdYd_rep16_part;
				if (--CPU_CX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				MOVSD_XdYd_rep16_part;
				if (--CPU_CX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				MOVSD_XdYd_rep16_part;
				if (--CPU_CX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}else{
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				MOVSD_XdYd_rep32_part;
				if (--CPU_ECX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				MOVSD_XdYd_rep32_part;
				if (--CPU_ECX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				MOVSD_XdYd_rep32_part;
				if (--CPU_ECX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}
}


/* cmps */
void
CMPSB_XbYb(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(8);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		dst = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_DI);
		BYTE_SUB(res, dst, src);
		CPU_SI += STRING_DIR;
		CPU_DI += STRING_DIR;
	} else {
		dst = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_EDI);
		BYTE_SUB(res, dst, src);
		CPU_ESI += STRING_DIR;
		CPU_EDI += STRING_DIR;
	}
}

void
CMPSW_XwYw(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(8);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_DI);
		WORD_SUB(res, dst, src);
		CPU_SI += STRING_DIRx2;
		CPU_DI += STRING_DIRx2;
	} else {
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_EDI);
		WORD_SUB(res, dst, src);
		CPU_ESI += STRING_DIRx2;
		CPU_EDI += STRING_DIRx2;
	}
}

void
CMPSD_XdYd(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(8);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_DI);
		DWORD_SUB(res, dst, src);
		CPU_SI += STRING_DIRx4;
		CPU_DI += STRING_DIRx4;
	} else {
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_EDI);
		DWORD_SUB(res, dst, src);
		CPU_ESI += STRING_DIRx4;
		CPU_EDI += STRING_DIRx4;
	}
}

#define	CMPSB_XbYb_rep16_part	\
  do { \
		CPU_WORKCLOCK(8);\
		dst = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);\
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_DI);\
		BYTE_SUB(res, dst, src);\
		CPU_SI += STRING_DIR;\
		CPU_DI += STRING_DIR;\
  } while (0)

#define	CMPSW_XwYw_rep16_part	\
  do { \
		CPU_WORKCLOCK(8);\
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);\
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_DI);\
		WORD_SUB(res, dst, src);\
		CPU_SI += STRING_DIRx2;\
		CPU_DI += STRING_DIRx2;\
  } while (0)

#define	CMPSD_XdYd_rep16_part	\
  do { \
		CPU_WORKCLOCK(8);\
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);\
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_DI);\
		DWORD_SUB(res, dst, src);\
		CPU_SI += STRING_DIRx4;\
		CPU_DI += STRING_DIRx4;\
  } while (0)

#define	CMPSB_XbYb_rep32_part	\
  do { \
		CPU_WORKCLOCK(8);\
		dst = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);\
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_EDI);\
		BYTE_SUB(res, dst, src);\
		CPU_ESI += STRING_DIR;\
		CPU_EDI += STRING_DIR;\
  } while (0)

#define	CMPSW_XwYw_rep32_part	\
  do { \
		CPU_WORKCLOCK(8);\
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);\
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_EDI);\
		WORD_SUB(res, dst, src);\
		CPU_ESI += STRING_DIRx2;\
		CPU_EDI += STRING_DIRx2;\
  } while (0)

#define	CMPSD_XdYd_rep32_part	\
  do { \
		CPU_WORKCLOCK(8);\
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);\
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_EDI);\
		DWORD_SUB(res, dst, src);\
		CPU_ESI += STRING_DIRx4;\
		CPU_EDI += STRING_DIRx4;\
  } while (0)
void
CMPSB_XbYb_rep(int reptype)
{
	UINT32 src, dst, res;
	
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				CMPSB_XbYb_rep16_part;
				if (--CPU_CX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				CMPSB_XbYb_rep16_part;
				if (--CPU_CX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				CMPSB_XbYb_rep16_part;
				if (--CPU_CX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}else{
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				CMPSB_XbYb_rep32_part;
				if (--CPU_ECX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				CMPSB_XbYb_rep32_part;
				if (--CPU_ECX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				CMPSB_XbYb_rep32_part;
				if (--CPU_ECX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}
}

void
CMPSW_XwYw_rep(int reptype)
{
	UINT32 src, dst, res;
	
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				CMPSW_XwYw_rep16_part;
				if (--CPU_CX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				CMPSW_XwYw_rep16_part;
				if (--CPU_CX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				CMPSW_XwYw_rep16_part;
				if (--CPU_CX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}else{
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				CMPSW_XwYw_rep32_part;
				if (--CPU_ECX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				CMPSW_XwYw_rep32_part;
				if (--CPU_ECX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				CMPSW_XwYw_rep32_part;
				if (--CPU_ECX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}
}

void
CMPSD_XdYd_rep(int reptype)
{
	UINT32 src, dst, res;
	
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				CMPSD_XdYd_rep16_part;
				if (--CPU_CX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				CMPSD_XdYd_rep16_part;
				if (--CPU_CX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				CMPSD_XdYd_rep16_part;
				if (--CPU_CX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}else{
		switch(reptype){
		case 0: /* rep */
			for (;;) {
				CMPSD_XdYd_rep32_part;
				if (--CPU_ECX == 0) {
	#if defined(DEBUG)
				cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 1: /* repe */
			for (;;) {
				CMPSD_XdYd_rep32_part;
				if (--CPU_ECX == 0 || CC_NZ) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		case 2: /* repne */
			for (;;) {
				CMPSD_XdYd_rep32_part;
				if (--CPU_ECX == 0 || CC_Z) {
	#if defined(DEBUG)
					cpu_debug_rep_cont = 0;
	#endif
					break;
				}
				if (CPU_REMCLOCK <= 0) {
					CPU_EIP = CPU_PREV_EIP;
					break;
				}
			}
			break;
		}
	}
}


/* scas */
void
SCASB_ALXb(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(7);
	dst = CPU_AL;
	if (!CPU_INST_AS32) {
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_DI);
		BYTE_SUB(res, dst, src);
		CPU_DI += STRING_DIR;
	} else {
		src = cpu_vmemoryread(CPU_ES_INDEX, CPU_EDI);
		BYTE_SUB(res, dst, src);
		CPU_EDI += STRING_DIR;
	}
}

void
SCASW_AXXw(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(7);
	dst = CPU_AX;
	if (!CPU_INST_AS32) {
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_DI);
		WORD_SUB(res, dst, src);
		CPU_DI += STRING_DIRx2;
	} else {
		src = cpu_vmemoryread_w(CPU_ES_INDEX, CPU_EDI);
		WORD_SUB(res, dst, src);
		CPU_EDI += STRING_DIRx2;
	}
}

void
SCASD_EAXXd(void)
{
	UINT32 src, dst, res;

	CPU_WORKCLOCK(7);
	dst = CPU_EAX;
	if (!CPU_INST_AS32) {
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_DI);
		DWORD_SUB(res, dst, src);
		CPU_DI += STRING_DIRx4;
	} else {
		src = cpu_vmemoryread_d(CPU_ES_INDEX, CPU_EDI);
		DWORD_SUB(res, dst, src);
		CPU_EDI += STRING_DIRx4;
	}
}


/* lods */
void
LODSB_ALXb(void)
{

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		CPU_AL = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);
		CPU_SI += STRING_DIR;
	} else {
		CPU_AL = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);
		CPU_ESI += STRING_DIR;
	}
}

void
LODSW_AXXw(void)
{

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		CPU_AX = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);
		CPU_SI += STRING_DIRx2;
	} else {
		CPU_AX = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);
		CPU_ESI += STRING_DIRx2;
	}
}

void
LODSD_EAXXd(void)
{

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		CPU_EAX = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);
		CPU_SI += STRING_DIRx4;
	} else {
		CPU_EAX = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);
		CPU_ESI += STRING_DIRx4;
	}
}


/* stos */
void
STOSB_YbAL(void)
{

	CPU_WORKCLOCK(3);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_DI, CPU_AL);
		CPU_DI += STRING_DIR;
	} else {
		cpu_vmemorywrite(CPU_ES_INDEX, CPU_EDI, CPU_AL);
		CPU_EDI += STRING_DIR;
	}
}

void
STOSW_YwAX(void)
{

	CPU_WORKCLOCK(3);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_DI, CPU_AX);
		CPU_DI += STRING_DIRx2;
	} else {
		cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_EDI, CPU_AX);
		CPU_EDI += STRING_DIRx2;
	}
}

void
STOSD_YdEAX(void)
{

	CPU_WORKCLOCK(3);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_DI, CPU_EAX);
		CPU_DI += STRING_DIRx4;
	} else {
		cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_EDI, CPU_EAX);
		CPU_EDI += STRING_DIRx4;
	}
}

// repのみ
void
STOSB_YbAL_rep(int reptype)
{
#if defined(USE_CPU_BULKREP)
	if (bulkrep_stos(1, CPU_AL)) {
		return;
	}
#endif
	if (!CPU_INST_AS32) {
		for (;;) {
			CPU_WORKCLOCK(3);
			cpu_vmemorywrite(CPU_ES_INDEX, CPU_DI, CPU_AL);
			CPU_DI += STRING_DIR;
			if (--CPU_CX == 0) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
				break;
			}
			if (CPU_REMCLOCK <= 0) {
				CPU_EIP = CPU_PREV_EIP;
				break;
			}
		}
	} else {
		for (;;) {
			CPU_WORKCLOCK(3);
			cpu_vmemorywrite(CPU_ES_INDEX, CPU_EDI, CPU_AL);
			CPU_EDI += STRING_DIR;
			if (--CPU_ECX == 0) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
				break;
			}
			if (CPU_REMCLOCK <= 0) {
				CPU_EIP = CPU_PREV_EIP;
				break;
			}
		}
	}
}

void
STOSW_YwAX_rep(int reptype)
{
#if defined(USE_CPU_BULKREP)
	if (bulkrep_stos(2, CPU_AX)) {
		return;
	}
#endif
	
	if (!CPU_INST_AS32) {
		for (;;) {
			CPU_WORKCLOCK(3);
			cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_DI, CPU_AX);
			CPU_DI += STRING_DIRx2;
			if (--CPU_CX == 0) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
				break;
			}
			if (CPU_REMCLOCK <= 0) {
				CPU_EIP = CPU_PREV_EIP;
				break;
			}
		}
	} else {
		for (;;) {
			CPU_WORKCLOCK(3);
			cpu_vmemorywrite_w(CPU_ES_INDEX, CPU_EDI, CPU_AX);
			CPU_EDI += STRING_DIRx2;
			if (--CPU_ECX == 0) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
				break;
			}
			if (CPU_REMCLOCK <= 0) {
				CPU_EIP = CPU_PREV_EIP;
				break;
			}
		}
	}
}

void
STOSD_YdEAX_rep(int reptype)
{
#if defined(USE_CPU_BULKREP)
	if (bulkrep_stos(4, CPU_EAX)) {
		return;
	}
#endif
	
	if (!CPU_INST_AS32) {
		for (;;) {
			CPU_WORKCLOCK(3);
			cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_DI, CPU_EAX);
			CPU_DI += STRING_DIRx4;
			if (--CPU_CX == 0) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
				break;
			}
			if (CPU_REMCLOCK <= 0) {
				CPU_EIP = CPU_PREV_EIP;
				break;
			}
		}
	} else {
		for (;;) {
			CPU_WORKCLOCK(3);
			cpu_vmemorywrite_d(CPU_ES_INDEX, CPU_EDI, CPU_EAX);
			CPU_EDI += STRING_DIRx4;
			if (--CPU_ECX == 0) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
				break;
			}
			if (CPU_REMCLOCK <= 0) {
				CPU_EIP = CPU_PREV_EIP;
				break;
			}
		}
	}
}


/* repeat */
void
_REPNE(void)
{
	CPU_INST_REPUSE = 0xf2;
}

void
_REPE(void)
{
	CPU_INST_REPUSE = 0xf3;
}


/* ins */
void
INSB_YbDX(void)
{
	UINT8 data;
	UINT32 paddr;

	CPU_WORKCLOCK(12);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_prepare_b(CPU_ES_INDEX, CPU_DI, &paddr);
		data = cpu_in(CPU_DX);
		cpu_vmemorywrite_commit_b(paddr, data);
		CPU_DI += STRING_DIR;
	} else {
		cpu_vmemorywrite_prepare_b(CPU_ES_INDEX, CPU_EDI, &paddr);
		data = cpu_in(CPU_DX);
		cpu_vmemorywrite_commit_b(paddr, data);
		CPU_EDI += STRING_DIR;
	}
}

void
INSW_YwDX(void)
{
	UINT16 data;
	UINT32 paddr[2];
	UINT remain;

	CPU_WORKCLOCK(12);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_prepare_w(CPU_ES_INDEX, CPU_DI, paddr, &remain);
		data = cpu_in_w(CPU_DX);
		cpu_vmemorywrite_commit_w(paddr, remain, data);
		CPU_DI += STRING_DIRx2;
	} else {
		cpu_vmemorywrite_prepare_w(CPU_ES_INDEX, CPU_EDI, paddr, &remain);
		data = cpu_in_w(CPU_DX);
		cpu_vmemorywrite_commit_w(paddr, remain, data);
		CPU_EDI += STRING_DIRx2;
	}
}

void
INSD_YdDX(void)
{
	UINT32 data;
	UINT32 paddr[2];
	UINT remain;

	CPU_WORKCLOCK(12);
	if (!CPU_INST_AS32) {
		cpu_vmemorywrite_prepare_d(CPU_ES_INDEX, CPU_DI, paddr, &remain);
		data = cpu_in_d(CPU_DX);
		cpu_vmemorywrite_commit_d(paddr, remain, data);
		CPU_DI += STRING_DIRx4;
	} else {
		cpu_vmemorywrite_prepare_d(CPU_ES_INDEX, CPU_EDI, paddr, &remain);
		data = cpu_in_d(CPU_DX);
		cpu_vmemorywrite_commit_d(paddr, remain, data);
		CPU_EDI += STRING_DIRx4;
	}
}


/* outs */
void
OUTSB_DXXb(void)
{
	UINT8 data;

	CPU_WORKCLOCK(14);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		data = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_out(CPU_DX, data);
		CPU_SI += STRING_DIR;
	} else {
		data = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_out(CPU_DX, data);
		CPU_ESI += STRING_DIR;
	}
}

void
OUTSW_DXXw(void)
{
	UINT16 data;

	CPU_WORKCLOCK(14);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		data = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_out_w(CPU_DX, data);
		CPU_SI += STRING_DIRx2;
	} else {
		data = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_out_w(CPU_DX, data);
		CPU_ESI += STRING_DIRx2;
	}
}

void
OUTSD_DXXd(void)
{
	UINT32 data;

	CPU_WORKCLOCK(14);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		data = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_SI);
		cpu_out_d(CPU_DX, data);
		CPU_SI += STRING_DIRx4;
	} else {
		data = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, CPU_ESI);
		cpu_out_d(CPU_DX, data);
		CPU_ESI += STRING_DIRx4;
	}
}
