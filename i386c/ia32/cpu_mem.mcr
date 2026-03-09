/*
 * Copyright (c) 2004 NONAKA Kimihiro
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

#define	DECLARE_VIRTUAL_ADDRESS_MEMORY_RW_FUNCTIONS(width, valtype, length) \
valtype MEMCALL \
cpu_vmemoryread_##width(int idx, UINT32 offset) \
{ \
	descriptor_t *sdp; \
	UINT32 addr; \
	int exc; \
\
	__ASSERT((unsigned int)idx < CPU_SEGREG_NUM); \
\
	sdp = &CPU_STAT_SREG(idx); \
	addr = sdp->u.seg.segbase + offset; \
\
	if (!CPU_STAT_PM) \
		return cpu_memoryread_##width(addr); \
\
	if (!SEG_IS_VALID(sdp)) { \
		exc = GP_EXCEPTION; \
		goto err; \
	} \
	if (!(sdp->flag & CPU_DESC_FLAG_READABLE)) { \
		cpu_memoryread_check(sdp, offset, (length), \
		    CHOOSE_EXCEPTION(idx)); \
	} else if (!(sdp->flag & CPU_DESC_FLAG_WHOLEADR)) { \
		if (!check_limit_upstairs(sdp, offset, (length), SEG_IS_32BIT(sdp))) \
			goto range_failure; \
	} \
	return cpu_lmemoryread_##width(addr, CPU_PAGE_READ_DATA | CPU_STAT_USER_MODE); \
\
range_failure: \
	VERBOSE(("cpu_vmemoryread_" #width ": type = %d, offset = %08x, length = %d, limit = %08x", sdp->type, offset, length, sdp->u.seg.limit)); \
	exc = CHOOSE_EXCEPTION(idx); \
err: \
	EXCEPTION(exc, 0); \
	return 0;	/* compiler happy */ \
} \
\
void MEMCALL \
cpu_vmemorywrite_##width(int idx, UINT32 offset, valtype value) \
{ \
	descriptor_t *sdp; \
	UINT32 addr; \
	int exc; \
\
	__ASSERT((unsigned int)idx < CPU_SEGREG_NUM); \
\
	sdp = &CPU_STAT_SREG(idx); \
	addr = sdp->u.seg.segbase + offset; \
\
	if (!CPU_STAT_PM) { \
		cpu_memorywrite_##width(addr, value); \
		return; \
	} \
\
	if (!SEG_IS_VALID(sdp)) { \
		exc = GP_EXCEPTION; \
		goto err; \
	} \
	if (!(sdp->flag & CPU_DESC_FLAG_WRITABLE)) { \
		cpu_memorywrite_check(sdp, offset, (length), \
		    CHOOSE_EXCEPTION(idx)); \
	} else if (!(sdp->flag & CPU_DESC_FLAG_WHOLEADR)) { \
		if (!check_limit_upstairs(sdp, offset, (length), SEG_IS_32BIT(sdp))) \
			goto range_failure; \
	} \
	cpu_lmemorywrite_##width(addr, value, CPU_PAGE_WRITE_DATA | CPU_STAT_USER_MODE); \
	return; \
\
range_failure: \
	VERBOSE(("cpu_vmemorywrite_" #width ": type = %d, offset = %08x, length = %d, limit = %08x", sdp->type, offset, length, sdp->u.seg.limit)); \
	exc = CHOOSE_EXCEPTION(idx); \
err: \
	EXCEPTION(exc, 0); \
}

#define	DECLARE_VIRTUAL_ADDRESS_MEMORY_RMW_FUNCTIONS(width, valtype, length) \
UINT32 MEMCALL \
cpu_vmemory_RMW_##width(int idx, UINT32 offset, UINT32 (CPUCALL *func)(UINT32, void *), void *arg) \
{ \
	descriptor_t *sdp; \
	UINT32 addr; \
	UINT32 result; \
	valtype value; \
	int exc; \
\
	__ASSERT((unsigned int)idx < CPU_SEGREG_NUM); \
\
	sdp = &CPU_STAT_SREG(idx); \
	addr = sdp->u.seg.segbase + offset; \
\
	if (!CPU_STAT_PM) { \
		value = cpu_memoryread_##width(addr); \
		result = (*func)(value, arg); \
		cpu_memorywrite_##width(addr, (valtype)result); \
		return value; \
	} \
\
	if (!SEG_IS_VALID(sdp)) { \
		exc = GP_EXCEPTION; \
		goto err; \
	} \
	if (!(sdp->flag & CPU_DESC_FLAG_WRITABLE)) { \
		cpu_memorywrite_check(sdp, offset, (length), \
		    CHOOSE_EXCEPTION(idx)); \
	} else if (!(sdp->flag & CPU_DESC_FLAG_WHOLEADR)) { \
		if (!check_limit_upstairs(sdp, offset, (length), SEG_IS_32BIT(sdp))) \
			goto range_failure; \
	} \
	return cpu_lmemory_RMW_##width(addr, func, arg); \
\
range_failure: \
	VERBOSE(("cpu_vmemory_RMW_" #width ": type = %d, offset = %08x, length = %d, limit = %08x", sdp->type, offset, length, sdp->u.seg.limit)); \
	exc = CHOOSE_EXCEPTION(idx); \
err: \
	EXCEPTION(exc, 0); \
	return 0;	/* compiler happy */ \
}
