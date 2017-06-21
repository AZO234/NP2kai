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

#ifndef	IA32_CPU_CPU_MEM_H__
#define	IA32_CPU_CPU_MEM_H__

#include "cpumem.h"
#include "segments.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * memory access check
 */
void MEMCALL cpu_stack_push_check(UINT16 s, descriptor_t *sdp, UINT32 sp, UINT len, BOOL is32bit);
void MEMCALL cpu_stack_pop_check(UINT16 s, descriptor_t *sdp, UINT32 sp, UINT len, BOOL is32bit);
#define	SS_PUSH_CHECK1(sp, len, is32bit) \
	cpu_stack_push_check(CPU_SS_INDEX, &CPU_SS_DESC, (sp), (len), (is32bit))
#define	SS_POP_CHECK1(sp, len, is32bit) \
	cpu_stack_pop_check(CPU_SS_INDEX, &CPU_SS_DESC, (sp), (len), (is32bit))
#define	SS_PUSH_CHECK(sp, len) \
	SS_PUSH_CHECK1((sp), (len), CPU_SS_DESC.d)
#define	SS_POP_CHECK(sp, len) \
	SS_POP_CHECK1((sp), (len), CPU_SS_DESC.d)

/*
 * virtual address function
 */
void MEMCALL cpu_vmemorywrite_b(int idx, UINT32 offset, UINT8 value);
#define	cpu_vmemorywrite(i,o,v)		cpu_vmemorywrite_b(i,o,v)
void MEMCALL cpu_vmemorywrite_w(int idx, UINT32 offset, UINT16 value);
void MEMCALL cpu_vmemorywrite_d(int idx, UINT32 offset, UINT32 value);
void MEMCALL cpu_vmemorywrite_q(int idx, UINT32 offset, UINT64 value);
void MEMCALL cpu_vmemorywrite_f(int idx, UINT32 offset, const REG80 *value);
UINT8 MEMCALL cpu_vmemoryread_b(int idx, UINT32 offset);
#define	cpu_vmemoryread(i,o)		cpu_vmemoryread_b(i,o)
UINT16 MEMCALL cpu_vmemoryread_w(int idx, UINT32 offset);
UINT32 MEMCALL cpu_vmemoryread_d(int idx, UINT32 offset);
UINT64 MEMCALL cpu_vmemoryread_q(int idx, UINT32 offset);
REG80 MEMCALL cpu_vmemoryread_f(int idx, UINT32 offset);
UINT32 MEMCALL cpu_vmemory_RMW_b(int idx, UINT32 offset, UINT32 (CPUCALL *func)(UINT32, void *), void *arg);
UINT32 MEMCALL cpu_vmemory_RMW_w(int idx, UINT32 offset, UINT32 (CPUCALL *func)(UINT32, void *), void *arg);
UINT32 MEMCALL cpu_vmemory_RMW_d(int idx, UINT32 offset, UINT32 (CPUCALL *func)(UINT32, void *), void *arg);

/*
 * code fetch
 */
UINT8 MEMCALL cpu_codefetch(UINT32 offset);
UINT16 MEMCALL cpu_codefetch_w(UINT32 offset);
UINT32 MEMCALL cpu_codefetch_d(UINT32 offset);

/*
 * additional physical address function
 */
UINT64 MEMCALL cpu_memoryread_q(UINT32 paddr);
REG80 MEMCALL cpu_memoryread_f(UINT32 paddr);
void MEMCALL cpu_memorywrite_q(UINT32 paddr, UINT64 value);
void MEMCALL cpu_memorywrite_f(UINT32 paddr, const REG80 *value);

#ifdef __cplusplus
}
#endif

#endif	/* !IA32_CPU_CPU_MEM_H__ */
