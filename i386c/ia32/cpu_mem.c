/*
 * Copyright (c) 2002-2004 NONAKA Kimihiro
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

#include "compiler.h"
#include "cpu.h"
#include "cpumem.h"


/*
 * memory access check
 */
static int MEMCALL check_limit_upstairs(descriptor_t *sdp, UINT32 offset, UINT len, BOOL is32bit);
static void MEMCALL cpu_memoryread_check(descriptor_t *sdp, UINT32 offset, UINT len, int e);
static void MEMCALL cpu_memorywrite_check(descriptor_t *sdp, UINT32 offset, UINT len, int e);

static int MEMCALL
check_limit_upstairs(descriptor_t *sdp, UINT32 offset, UINT len, BOOL is32bit)
{
	UINT32 limit;
	UINT32 end;

	__ASSERT(sdp != NULL);
	__ASSERT(len > 0);

	len--;
	end = offset + len;

	if (SEG_IS_DATA(sdp) && SEG_IS_EXPANDDOWN_DATA(sdp)) {
		/* expand-down data segment */
		limit = SEG_IS_32BIT(sdp) ? 0xffffffff : 0x0000ffff;
		if (sdp->u.seg.limit == 0) {
			/*
			 *   32bit       16bit
			 * +-------+   +-------+ FFFFFFFFh
			 * |       |   |       |
			 * |       |   +  [1]  + 0000FFFFh
			 * | valid |   |       |
			 * |       |   +-------+ 0000FFFFh - len -1
			 * |       |   | valid |
			 * +-------+   +-------+ 00000000h
			 */
			if (!SEG_IS_32BIT(sdp)) {
				if ((len > limit)		/* len check */
				 || (end > limit)) {		/* [1] */
					goto exc;
				}
			} else {
				sdp->flag |= CPU_DESC_FLAG_WHOLEADR;
			}
		} else {
			/*
			 *   32bit       16bit
			 * +-------+   +-------+ FFFFFFFFh
			 * |  [2]  |   |       |
			 * +-------+   +.......+ FFFFFFFFh - len - 1
			 * |       |   |  [2]  |
			 * |       |   +.......+ 0000FFFFh
			 * | valid |   |       |
			 * |       |   +-------+ 0000FFFFh - len - 1
			 * |       |   | valid |
			 * +-------+   +-------+ seg.limit
			 * |  [1]  |   |  [1]  |
			 * +-------+   +-------+ 00000000h
			 */
			if ((len > limit - sdp->u.seg.limit)	/* len check */
			 || (end < offset)			/* wrap check */
			 || (offset < sdp->u.seg.limit) 	/* [1] */
			 || (end > limit)) {			/* [2] */
				goto exc;
			}
		}
	} else {
		/* expand-up data or code segment */
		if (sdp->u.seg.limit == 0xffffffff) {
			/*
			 *  16/32bit
			 * +-------+ FFFFFFFFh
			 * |       |
			 * |       |
			 * | valid |
			 * |       |
			 * |       |
			 * +-------+ 00000000h
			 */
			sdp->flag |= CPU_DESC_FLAG_WHOLEADR;
		} else {
			/*
			 *  16/32bit
			 * +-------+ FFFFFFFFh
			 * |       |
			 * |       |
			 * |  [1]  |
			 * +.......+ seg.limit
			 * |       |
			 * +-------+ seg.limit - len - 1
			 * | valid |
			 * +-------+ 00000000h
			 */
			if ((len > sdp->u.seg.limit)		/* len check */
			 || (end < offset)			/* wrap check */
			 || (end > sdp->u.seg.limit + 1)) {	/* [1] */
				goto exc;
			}
		}
	}
	return 1;	/* Ok! */

exc:
	VERBOSE(("check_limit_upstairs: check failure: offset = 0x%08x, len = %d", offset, len + 1));
#if defined(DEBUG)
	segdesc_dump(sdp);
#endif
	return 0;
}

static void MEMCALL
cpu_memoryread_check(descriptor_t *sdp, UINT32 offset, UINT len, int e)
{

	__ASSERT(sdp != NULL);
	__ASSERT(len > 0);

	if (!SEG_IS_VALID(sdp)) {
		e = GP_EXCEPTION;
		goto exc;
	}
	if (!SEG_IS_PRESENT(sdp)
	 || SEG_IS_SYSTEM(sdp)
	 || (SEG_IS_CODE(sdp) && !SEG_IS_READABLE_CODE(sdp))) {
		goto exc;
	}

	switch (sdp->type) {
	case 0:	 case 1:	/* ro */
	case 2:  case 3:	/* rw */
	case 4:  case 5:	/* ro (expand down) */
	case 6:  case 7:	/* rw (expand down) */
	case 10: case 11:	/* rx */
	case 14: case 15:	/* rxc */
		if (!check_limit_upstairs(sdp, offset, len, SEG_IS_32BIT(sdp)))
			goto exc;
		break;

	default:
		goto exc;
	}
	sdp->flag |= CPU_DESC_FLAG_READABLE;
	return;

exc:
	VERBOSE(("cpu_memoryread_check: check failure: offset = 0x%08x, len = %d", offset, len));
#if defined(DEBUG)
	segdesc_dump(sdp);
#endif
	EXCEPTION(e, 0);
}

static void MEMCALL
cpu_memorywrite_check(descriptor_t *sdp, UINT32 offset, UINT len, int e)
{

	__ASSERT(sdp != NULL);
	__ASSERT(len > 0);

	if (!SEG_IS_VALID(sdp)) {
		e = GP_EXCEPTION;
		goto exc;
	}
	if (!SEG_IS_PRESENT(sdp)
	 || SEG_IS_SYSTEM(sdp)
	 || SEG_IS_CODE(sdp)
	 || (SEG_IS_DATA(sdp) && !SEG_IS_WRITABLE_DATA(sdp))) {
		goto exc;
	}

	switch (sdp->type) {
	case 2: case 3:	/* rw */
	case 6: case 7:	/* rw (expand down) */
		if (!check_limit_upstairs(sdp, offset, len, SEG_IS_32BIT(sdp)))
			goto exc;
		break;

	default:
		goto exc;
	}
	sdp->flag |= CPU_DESC_FLAG_WRITABLE | CPU_DESC_FLAG_READABLE;
	return;

exc:
	VERBOSE(("cpu_memorywrite_check: check failure: offset = 0x%08x, len = %d", offset, len));
#if defined(DEBUG)
	segdesc_dump(sdp);
#endif
	EXCEPTION(e, 0);
}

void MEMCALL
cpu_stack_push_check(UINT16 s, descriptor_t *sdp, UINT32 sp, UINT len,
    BOOL is32bit)
{
	UINT32 limit;
	UINT32 start;

	__ASSERT(sdp != NULL);
	__ASSERT(len > 0);

	len--;

	if (!SEG_IS_VALID(sdp)
	 || !SEG_IS_PRESENT(sdp)
	 || SEG_IS_SYSTEM(sdp)
	 || SEG_IS_CODE(sdp)
	 || !SEG_IS_WRITABLE_DATA(sdp)) {
		goto exc;
	}

	start = sp - len;
	limit = is32bit ? 0xffffffff : 0x0000ffff;

	if (SEG_IS_EXPANDDOWN_DATA(sdp)) {
		/* expand-down stack */
		if (!SEG_IS_32BIT(sdp)) {
			if (sp > limit) {			/* [*] */
				goto exc;
			}
		}
		if (sdp->u.seg.limit == 0) {
			/*
			 *   32bit       16bit
			 * +-------+   +-------+ FFFFFFFFh
			 * |       |   |  [*]  |
			 * |       |   +-------+ 0000FFFFh
			 * | valid |   |       |
			 * |       |   | valid |
			 * |       |   |       |
			 * +-------+   +-------+ 00000000h
			 */
			if (!SEG_IS_32BIT(sdp)) {
				if (sp > limit) {		/* [1] */
					goto exc;
				}
			} else {
				sdp->flag |= CPU_DESC_FLAG_WHOLEADR;
			}
		} else {
			/*
			 *   32bit       16bit
			 * +-------+   +-------+ FFFFFFFFh
			 * |       |   |  [*]  |
			 * | valid |   +-------+ 0000FFFFh
			 * |       |   | valid |
			 * +-------+   +-------+ seg.limit + len - 1
			 * |       |   |       |
			 * +..[1]..+   +..[1]..+ seg.limit
			 * |       |   |       |
			 * +-------+   +-------+ 00000000h
			 */
			if ((len > limit - sdp->u.seg.limit)	/* len check */
			 || (start > sp)			/* wrap check */
			 || (start < sdp->u.seg.limit)) {	/* [1] */
				goto exc;
			}
		}
	} else {
		/* expand-up stack */
		if (sdp->u.seg.limit == limit) {
			/*
			 *   32bit       16bit
			 * +-------+   +-------+ FFFFFFFFh
			 * |       |   |  [1]  |
			 * |       |   +-------+ 0000FFFFh
			 * | valid |   |       |
			 * |       |   | valid |
			 * |       |   |       |
			 * +-------+   +-------+ 00000000h
			 */
			if (!SEG_IS_32BIT(sdp)) {
				if (sp > limit) {		/* [1] */
					goto exc;
				}
			} else {
				sdp->flag |= CPU_DESC_FLAG_WHOLEADR;
			}
		} else {
			/*
			 *   32bit       16bit
			 * +-------+   +-------+ FFFFFFFFh
			 * |       |   |       |
			 * |  [1]  |   +  [1]  + 0000FFFFh
			 * |       |   |       |
			 * +-------+   +-------+ seg.limit
			 * | valid |   | valid |
			 * +.......+   +.......+ len - 1
			 * |  [+]  |   |  [+]  |
			 * +-------+   +-------+ 00000000h
			 *
			 * [+]: wrap check
			 */
			if ((len > sdp->u.seg.limit)		/* len check */
			 || (start > sp)			/* wrap check */
			 || (sp > sdp->u.seg.limit + 1)) {	/* [1] */
				goto exc;
			}
		}
	}
	return;

exc:
	VERBOSE(("cpu_stack_push_check: check failure: selector = 0x%04x, sp = 0x%08x, len = %d", s, sp, len));
#if defined(DEBUG)
	segdesc_dump(sdp);
#endif
	EXCEPTION(SS_EXCEPTION, s & 0xfffc);
}

void MEMCALL
cpu_stack_pop_check(UINT16 s, descriptor_t *sdp, UINT32 sp, UINT len,
    BOOL is32bit)
{

	__ASSERT(sdp != NULL);
	__ASSERT(len > 0);

	if (!SEG_IS_VALID(sdp)
	 || !SEG_IS_PRESENT(sdp)
	 || SEG_IS_SYSTEM(sdp)
	 || SEG_IS_CODE(sdp)
	 || !SEG_IS_WRITABLE_DATA(sdp)) {
		goto exc;
	}

	if (!check_limit_upstairs(sdp, sp, len, is32bit))
		goto exc;
	return;

exc:
	VERBOSE(("cpu_stack_pop_check: check failure: selector = 0x%04x, sp = 0x%08x, len = %d", s, sp, len));
#if defined(DEBUG)
	segdesc_dump(sdp);
#endif
	EXCEPTION(SS_EXCEPTION, s & 0xfffc);
}


/*
 * code fetch
 */
UINT8 MEMCALL
cpu_codefetch(UINT32 offset)
{
	const int ucrw = CPU_PAGE_READ_CODE | CPU_STAT_USER_MODE;
	descriptor_t *sdp;
	UINT32 addr;

	sdp = &CPU_CS_DESC;
	addr = sdp->u.seg.segbase + offset;

	if (!CPU_STAT_PM)
		return cpu_memoryread(addr);
	if (offset <= sdp->u.seg.limit)
		return cpu_lmemoryread(addr, ucrw);

	EXCEPTION(GP_EXCEPTION, 0);
	return 0;	/* compiler happy */
}

UINT16 MEMCALL
cpu_codefetch_w(UINT32 offset)
{
	const int ucrw = CPU_PAGE_READ_CODE | CPU_STAT_USER_MODE;
	descriptor_t *sdp;
	UINT32 addr;

	sdp = &CPU_CS_DESC;
	addr = sdp->u.seg.segbase + offset;

	if (!CPU_STAT_PM)
		return cpu_memoryread_w(addr);
	if (offset <= sdp->u.seg.limit - 1)
		return cpu_lmemoryread_w(addr, ucrw);

	EXCEPTION(GP_EXCEPTION, 0);
	return 0;	/* compiler happy */
}

UINT32 MEMCALL
cpu_codefetch_d(UINT32 offset)
{
	const int ucrw = CPU_PAGE_READ_CODE | CPU_STAT_USER_MODE;
	descriptor_t *sdp;
	UINT32 addr;

	sdp = &CPU_CS_DESC;
	addr = sdp->u.seg.segbase + offset;

	if (!CPU_STAT_PM)
		return cpu_memoryread_d(addr);

	if (offset <= sdp->u.seg.limit - 3)
		return cpu_lmemoryread_d(addr, ucrw);

	EXCEPTION(GP_EXCEPTION, 0);
	return 0;	/* compiler happy */
}

/*
 * additional physical address memory access functions
 */
UINT64 MEMCALL
cpu_memoryread_q(UINT32 paddr)
{
	UINT64 value;

	value = cpu_memoryread_d(paddr);
	value += (UINT64)cpu_memoryread_d(paddr + 4) << 32;

	return value;
}

void MEMCALL
cpu_memorywrite_q(UINT32 paddr, UINT64 value)
{

	cpu_memorywrite_d(paddr, (UINT32)value);
	cpu_memorywrite_d(paddr + 4, (UINT32)(value >> 32));
}

REG80 MEMCALL
cpu_memoryread_f(UINT32 paddr)
{
	REG80 value;
	int i;

	for (i = 0; i < (int)sizeof(REG80); ++i) {
		value.b[i] = cpu_memoryread(paddr + i);
	}
	return value;
}

void MEMCALL
cpu_memorywrite_f(UINT32 paddr, const REG80 *value)
{
	int i;

	for (i = 0; i < (int)sizeof(REG80); ++i) {
		cpu_memorywrite(paddr + i, value->b[i]);
	}
}

/*
 * virtual address memory access functions
 */
#define	CHOOSE_EXCEPTION(sreg) \
	(((sreg) == CPU_SS_INDEX) ? SS_EXCEPTION : GP_EXCEPTION)

#include "cpu_mem.mcr"

DECLARE_VIRTUAL_ADDRESS_MEMORY_RW_FUNCTIONS(b, UINT8, 1)
DECLARE_VIRTUAL_ADDRESS_MEMORY_RMW_FUNCTIONS(b, UINT8, 1)
DECLARE_VIRTUAL_ADDRESS_MEMORY_RW_FUNCTIONS(w, UINT16, 2)
DECLARE_VIRTUAL_ADDRESS_MEMORY_RMW_FUNCTIONS(w, UINT16, 2)
DECLARE_VIRTUAL_ADDRESS_MEMORY_RW_FUNCTIONS(d, UINT32, 4)
DECLARE_VIRTUAL_ADDRESS_MEMORY_RMW_FUNCTIONS(d, UINT32, 4)
DECLARE_VIRTUAL_ADDRESS_MEMORY_RW_FUNCTIONS(q, UINT64, 8)

REG80 MEMCALL
cpu_vmemoryread_f(int idx, UINT32 offset)
{
	descriptor_t *sdp;
	UINT32 addr;
	int exc;

	__ASSERT((unsigned int)idx < CPU_SEGREG_NUM);

	sdp = &CPU_STAT_SREG(idx);
	addr = sdp->u.seg.segbase + offset;

	if (!CPU_STAT_PM)
		return cpu_memoryread_f(addr);

	if (!SEG_IS_VALID(sdp)) {
		exc = GP_EXCEPTION;
		goto err;
	}
	if (!(sdp->flag & CPU_DESC_FLAG_READABLE)) {
		cpu_memoryread_check(sdp, offset, 10, CHOOSE_EXCEPTION(idx));
	} else if (!(sdp->flag & CPU_DESC_FLAG_WHOLEADR)) {
		if (!check_limit_upstairs(sdp, offset, 10, SEG_IS_32BIT(sdp)))
			goto range_failure;
	} 
	return cpu_lmemoryread_f(addr, CPU_PAGE_READ_DATA | CPU_STAT_USER_MODE);

range_failure:
	VERBOSE(("cpu_vmemoryread_f: type = %d, offset = %08x, limit = %08x", sdp->type, offset, sdp->u.seg.limit));
	exc = CHOOSE_EXCEPTION(idx);
err:
	EXCEPTION(exc, 0);
	{
		REG80 dummy;
		memset(&dummy, 0, sizeof(dummy));
		return dummy;	/* compiler happy */
	}
}

void MEMCALL
cpu_vmemorywrite_f(int idx, UINT32 offset, const REG80 *value)
{
	descriptor_t *sdp;
	UINT32 addr;
	int exc;

	__ASSERT((unsigned int)idx < CPU_SEGREG_NUM);

	sdp = &CPU_STAT_SREG(idx);
	addr = sdp->u.seg.segbase + offset;

	if (!CPU_STAT_PM) {
		cpu_memorywrite_f(addr, value);
		return;
	}

	if (!SEG_IS_VALID(sdp)) {
		exc = GP_EXCEPTION;
		goto err;
	}
	if (!(sdp->flag & CPU_DESC_FLAG_WRITABLE)) {
		cpu_memorywrite_check(sdp, offset, 10, CHOOSE_EXCEPTION(idx));
	} else if (!(sdp->flag & CPU_DESC_FLAG_WHOLEADR)) {
		if (!check_limit_upstairs(sdp, offset, 10, SEG_IS_32BIT(sdp)))
			goto range_failure;
	}
	cpu_lmemorywrite_f(addr, value, CPU_PAGE_WRITE_DATA | CPU_STAT_USER_MODE);
	return;

range_failure:
	VERBOSE(("cpu_vmemorywrite_f: type = %d, offset = %08x, limit = %08x", sdp->type, offset, sdp->u.seg.limit));
	exc = CHOOSE_EXCEPTION(idx);
err:
	EXCEPTION(exc, 0);
}
