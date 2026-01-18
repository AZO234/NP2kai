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

#include <compiler.h>
#include <dosio.h>
#include "cpu.h"
#include "ia32.mcr"

#include "inst_table.h"

#if defined(ENABLE_TRAP)
#include "trap/steptrap.h"
#endif

#if defined(SUPPORT_ASYNC_CPU)
#include <timing.h>
#include <nevent.h>
#include <pccore.h>
#include	<io/iocore.h>
#include	<sound/sound.h>
#include	<sound/beep.h>
#include	<sound/fmboard.h>
#include	<sound/soundrom.h>
#include	<cbus/mpu98ii.h>
#if defined(SUPPORT_SMPU98)
#include	<cbus/smpu98.h>
#endif
#endif

sigjmp_buf exec_1step_jmpbuf;

#if defined(IA32_INSTRUCTION_TRACE)
typedef struct {
	CPU_REGS		regs;
	disasm_context_t	disasm;

	BYTE			op[MAX_PREFIX + 2];
	int			opbytes;
} ia32_context_t;

#define	NCTX	1024

ia32_context_t ctx[NCTX];
int ctx_index = 0;

int cpu_inst_trace = 0;
#endif

#if defined(DEBUG)
int cpu_debug_rep_cont = 0;
CPU_REGS cpu_debug_rep_regs;
#endif

void
exec_1step(void)
{
	int prefix;
	UINT32 op;

	CPU_PREV_EIP = CPU_EIP;
	CPU_STATSAVE.cpu_inst = CPU_STATSAVE.cpu_inst_default;

#if defined(ENABLE_TRAP)
	steptrap(CPU_CS, CPU_EIP);
#endif

#if defined(IA32_INSTRUCTION_TRACE)
	ctx[ctx_index].regs = CPU_STATSAVE.cpu_regs;
	if (cpu_inst_trace) {
		disasm_context_t *d = &ctx[ctx_index].disasm;
		UINT32 eip = CPU_EIP;
		int rv;

		rv = disasm(&eip, d);
		if (rv == 0) {
			char buf[256];
			char tmp[32];
			int len = d->nopbytes > 8 ? 8 : d->nopbytes;
			int i;

			buf[0] = '¥0';
			for (i = 0; i < len; i++) {
				snprintf(tmp, sizeof(tmp), "%02x ", d->opbyte[i]);
				milstr_ncat(buf, tmp, sizeof(buf));
			}
			for (; i < 8; i++) {
				milstr_ncat(buf, "   ", sizeof(buf));
			}
			VERBOSE(("%04x:%08x: %s%s", CPU_CS, CPU_EIP, buf, d->str));

			buf[0] = '¥0';
			for (; i < d->nopbytes; i++) {
				snprintf(tmp, sizeof(tmp), "%02x ", d->opbyte[i]);
				milstr_ncat(buf, tmp, sizeof(buf));
				if ((i % 8) == 7) {
					VERBOSE(("             : %s", buf));
					buf[0] = '¥0';
				}
			}
			if ((i % 8) != 0) {
				VERBOSE(("             : %s", buf));
			}
		}
	}
	ctx[ctx_index].opbytes = 0;
#endif

	for (prefix = 0; prefix < MAX_PREFIX; prefix++) {
		GET_PCBYTE(op);
#if defined(IA32_INSTRUCTION_TRACE)
		ctx[ctx_index].op[prefix] = op;
		ctx[ctx_index].opbytes++;
#endif

		/* prefix */
		if (insttable_info[op] & INST_PREFIX) {
			(*insttable_1byte[0][op])();
			continue;
		}
		break;
	}
	if (prefix == MAX_PREFIX) {
		EXCEPTION(UD_EXCEPTION, 0);
	}

#if defined(IA32_INSTRUCTION_TRACE)
	if (op == 0x0f) {
		BYTE op2;
		op2 = cpu_codefetch(CPU_EIP);
		ctx[ctx_index].op[prefix + 1] = op2;
		ctx[ctx_index].opbytes++;
	}
	ctx_index = (ctx_index + 1) % NELEMENTS(ctx);
#endif
	
	/* normal / rep, but not use */
	if (!(insttable_info[op] & INST_STRING) || !CPU_INST_REPUSE) {
#if defined(DEBUG)
		cpu_debug_rep_cont = 0;
#endif
		(*insttable_1byte[CPU_INST_OP32][op])();
		return;
	}

	/* rep */
	CPU_WORKCLOCK(5);
#if defined(DEBUG)
	if (!cpu_debug_rep_cont) {
		cpu_debug_rep_cont = 1;
		cpu_debug_rep_regs = CPU_STATSAVE.cpu_regs;
	}
#endif
	if (!CPU_INST_AS32) {
		if (CPU_CX != 0) {
			if (!(insttable_info[op] & REP_CHECKZF)) {
				/* rep */
				for (;;) {
					(*insttable_1byte[CPU_INST_OP32][op])();
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
			} else if (CPU_INST_REPUSE != 0xf2) {
				/* repe */
				for (;;) {
					(*insttable_1byte[CPU_INST_OP32][op])();
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
			} else {
				/* repne */
				for (;;) {
					(*insttable_1byte[CPU_INST_OP32][op])();
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
			}
		}
	} else {
		if (CPU_ECX != 0) {
			if (!(insttable_info[op] & REP_CHECKZF)) {
				/* rep */
				for (;;) {
					(*insttable_1byte[CPU_INST_OP32][op])();
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
			} else if (CPU_INST_REPUSE != 0xf2) {
				/* repe */
				for (;;) {
					(*insttable_1byte[CPU_INST_OP32][op])();
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
			} else {
				/* repne */
				for (;;) {
					(*insttable_1byte[CPU_INST_OP32][op])();
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
			}
		}
	}
}

void
exec_allstep(void)
{
	int prefix;
	UINT32 op;
	void (*func)(void);
	
	do {

		CPU_PREV_EIP = CPU_EIP;
		CPU_STATSAVE.cpu_inst = CPU_STATSAVE.cpu_inst_default;

#if defined(ENABLE_TRAP)
		steptrap(CPU_CS, CPU_EIP);
#endif

#if defined(IA32_INSTRUCTION_TRACE)
		ctx[ctx_index].regs = CPU_STATSAVE.cpu_regs;
		if (cpu_inst_trace) {
			disasm_context_t *d = &ctx[ctx_index].disasm;
			UINT32 eip = CPU_EIP;
			int rv;

			rv = disasm(&eip, d);
			if (rv == 0) {
				char buf[256];
				char tmp[32];
				int len = d->nopbytes > 8 ? 8 : d->nopbytes;
				int i;

				buf[0] = '¥0';
				for (i = 0; i < len; i++) {
					snprintf(tmp, sizeof(tmp), "%02x ", d->opbyte[i]);
					milstr_ncat(buf, tmp, sizeof(buf));
				}
				for (; i < 8; i++) {
					milstr_ncat(buf, "   ", sizeof(buf));
				}
				VERBOSE(("%04x:%08x: %s%s", CPU_CS, CPU_EIP, buf, d->str));

				buf[0] = '¥0';
				for (; i < d->nopbytes; i++) {
					snprintf(tmp, sizeof(tmp), "%02x ", d->opbyte[i]);
					milstr_ncat(buf, tmp, sizeof(buf));
					if ((i % 8) == 7) {
						VERBOSE(("             : %s", buf));
						buf[0] = '¥0';
					}
				}
				if ((i % 8) != 0) {
					VERBOSE(("             : %s", buf));
				}
			}
		}
		ctx[ctx_index].opbytes = 0;
#endif

		for (prefix = 0; prefix < MAX_PREFIX; prefix++)
		{
#if defined(USE_CPU_MODRMPREFETCH)
			if ((CPU_EIP + 1) & CPU_PAGE_MASK)
			{
				op = cpu_opcodefetch(CPU_EIP);
#if defined(USE_CPU_EIPMASK)
				CPU_EIP = (CPU_EIP + 1) & CPU_EIPMASK;
#else
				if (CPU_STATSAVE.cpu_inst_default.op_32)
				{
					CPU_EIP = (CPU_EIP + 1);
				}
				else
				{
					CPU_EIP = (CPU_EIP + 1) & 0xffff;
				}
#endif
			}
			else
#endif
			{
				GET_PCBYTE(op);
			}
#if defined(IA32_INSTRUCTION_TRACE)
			ctx[ctx_index].op[prefix] = op;
			ctx[ctx_index].opbytes++;
#endif

			/* prefix */
			if (insttable_info[op] & INST_PREFIX)
			{
#if defined(USE_CPU_INLINEINST)
				// インライン命令群　関数テーブルよりも呼び出しが高速だが、多く置きすぎるとifが増えて逆に遅くなる。なので呼び出し頻度が高い物を優先して配置
				if (!(op & 0x26)) {
					(*insttable_1byte[0][op])();
					continue;
				}
				else {
					if (op == 0x66)
					{
						CPU_INST_OP32 = !CPU_STATSAVE.cpu_inst_default.op_32;
						continue;
					}
					else if (op == 0x26)
					{
						CPU_INST_SEGUSE = 1;
						CPU_INST_SEGREG_INDEX = CPU_ES_INDEX;
						continue;
					}
					else if (op == 0x67)
					{
						CPU_INST_AS32 = !CPU_STATSAVE.cpu_inst_default.as_32;
						continue;
					}
					else if (op == 0x2E)
					{
						CPU_INST_SEGUSE = 1;
						CPU_INST_SEGREG_INDEX = CPU_CS_INDEX;
						continue;
					}
					//else if (op == 0xF3)
					//{
					//	CPU_INST_REPUSE = 0xf3;
					//	continue;
					//}
					else
					{
						(*insttable_1byte[0][op])();
						continue;
					}
				}
#else

				(*insttable_1byte[0][op])();
				continue;
#endif
			}
			break;
		}
		if (prefix == MAX_PREFIX) {
			EXCEPTION(UD_EXCEPTION, 0);
		}

#if defined(IA32_INSTRUCTION_TRACE)
		if (op == 0x0f) {
			BYTE op2;
			op2 = cpu_codefetch(CPU_EIP);
			ctx[ctx_index].op[prefix + 1] = op2;
			ctx[ctx_index].opbytes++;
		}
		ctx_index = (ctx_index + 1) % NELEMENTS(ctx);
#endif
	
		/* normal / rep, but not use */
#if defined(USE_CPU_INLINEINST)
		if (CPU_INST_OP32)
		{
			// インライン命令群　関数テーブルよりも呼び出しが高速だが、多く置きすぎるとifが増えて逆に遅くなる。なので呼び出し頻度が高い物を優先して配置
			if (op == 0x8b)
			{
				UINT32* out;
				UINT32 op2, src;

				PREPART_REG32_EA(op2, src, out, 2, 5);
				*out = src;
				continue;
			}
			else if (op == 0x0f)
			{
				UINT8 repuse = CPU_INST_REPUSE;

				GET_MODRM_PCBYTE(op);
#ifdef USE_SSE
				if (insttable_2byte660F_32[op] && CPU_INST_OP32 == !CPU_STATSAVE.cpu_inst_default.op_32)
				{
					(*insttable_2byte660F_32[op])();
				}
				else if (insttable_2byteF20F_32[op] && repuse == 0xf2)
				{
					(*insttable_2byteF20F_32[op])();
				}
				else if (insttable_2byteF30F_32[op] && repuse == 0xf3)
				{
					(*insttable_2byteF30F_32[op])();
				}
				else
				{
					(*insttable_2byte[1][op])();
				}
#else
				(*insttable_2byte[1][op])();
#endif
				continue;
			}
			else if (op == 0x74)
			{
				if (CC_NZ)
				{
					JMPNOP(2, 1);
				}
				else
				{
					JMPSHORT(7);
				}
				continue;
			}
		}
#endif
		if (!(insttable_info[op] & INST_STRING) || !CPU_INST_REPUSE) {
#if defined(DEBUG)
			cpu_debug_rep_cont = 0;
#endif
			(*insttable_1byte[CPU_INST_OP32][op])();
			continue;
		}

		/* rep */
		CPU_WORKCLOCK(5);
#if defined(DEBUG)
		if (!cpu_debug_rep_cont) {
			cpu_debug_rep_cont = 1;
			cpu_debug_rep_regs = CPU_STATSAVE.cpu_regs;
		}
#endif
		func = insttable_1byte[CPU_INST_OP32][op];
		if (!CPU_INST_AS32) {
			if (CPU_CX != 0) {
				if(CPU_CX==1){
					(*func)();
					--CPU_CX;
				}else{
					if (!(insttable_info[op] & REP_CHECKZF)) {
						if(insttable_1byte_repfunc[CPU_INST_OP32][op]){
							(*insttable_1byte_repfunc[CPU_INST_OP32][op])(0);
						}else{
							/* rep */
							for (;;) {
								(*func)();
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
						}
					} else if (CPU_INST_REPUSE != 0xf2) {
						if(insttable_1byte_repfunc[CPU_INST_OP32][op]){
							(*insttable_1byte_repfunc[CPU_INST_OP32][op])(1);
						}else{
							/* repe */
							for (;;) {
								(*func)();
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
						}
					} else {
						if(insttable_1byte_repfunc[CPU_INST_OP32][op]){
							(*insttable_1byte_repfunc[CPU_INST_OP32][op])(2);
						}else{
							/* repne */
							for (;;) {
								(*func)();
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
						}
					}
				}
			}
		} else {
			if (CPU_ECX != 0) {
				if(CPU_ECX==1){
					(*func)();
					--CPU_ECX;
				}else{
					if (!(insttable_info[op] & REP_CHECKZF)) {
						if(insttable_1byte_repfunc[CPU_INST_OP32][op]){
							(*insttable_1byte_repfunc[CPU_INST_OP32][op])(0);
						}else{
							/* rep */
							for (;;) {
								(*func)();
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
					} else if (CPU_INST_REPUSE != 0xf2) {
						if(insttable_1byte_repfunc[CPU_INST_OP32][op]){
							(*insttable_1byte_repfunc[CPU_INST_OP32][op])(1);
						}else{
							/* repe */
							for (;;) {
								(*func)();
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
						}
					} else {
						if(insttable_1byte_repfunc[CPU_INST_OP32][op]){
							(*insttable_1byte_repfunc[CPU_INST_OP32][op])(2);
						}else{
							/* repne */
							for (;;) {
								(*func)();
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
						}
					}
				}
			}
		}
	} while (CPU_REMCLOCK > 0);
}