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

UINT8 cpu_drawskip = 1;
UINT8 cpu_nowait = 0;
double np2cpu_lastTimingValue = 1.0;

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

			buf[0] = '\0';
			for (i = 0; i < len; i++) {
				snprintf(tmp, sizeof(tmp), "%02x ", d->opbyte[i]);
				milstr_ncat(buf, tmp, sizeof(buf));
			}
			for (; i < 8; i++) {
				milstr_ncat(buf, "   ", sizeof(buf));
			}
			VERBOSE(("%04x:%08x: %s%s", CPU_CS, CPU_EIP, buf, d->str));

			buf[0] = '\0';
			for (; i < d->nopbytes; i++) {
				snprintf(tmp, sizeof(tmp), "%02x ", d->opbyte[i]);
				milstr_ncat(buf, tmp, sizeof(buf));
				if ((i % 8) == 7) {
					VERBOSE(("             : %s", buf));
					buf[0] = '\0';
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
#if defined(SUPPORT_ASYNC_CPU)
	int remclkcnt = INT_MAX;
	static int latecount = 0;
	static int latecount2 = 0;
	static unsigned int hltflag = 0;

	if(latecount2==0){
		if(latecount > 0){
			//latecount--;
		}else if (latecount < 0){
			latecount++;
		}
	}
	latecount2 = (latecount2+1) & 0x1fff;
#endif
	
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

				buf[0] = '\0';
				for (i = 0; i < len; i++) {
					snprintf(tmp, sizeof(tmp), "%02x ", d->opbyte[i]);
					milstr_ncat(buf, tmp, sizeof(buf));
				}
				for (; i < 8; i++) {
					milstr_ncat(buf, "   ", sizeof(buf));
				}
				VERBOSE(("%04x:%08x: %s%s", CPU_CS, CPU_EIP, buf, d->str));

				buf[0] = '\0';
				for (; i < d->nopbytes; i++) {
					snprintf(tmp, sizeof(tmp), "%02x ", d->opbyte[i]);
					milstr_ncat(buf, tmp, sizeof(buf));
					if ((i % 8) == 7) {
						VERBOSE(("             : %s", buf));
						buf[0] = '\0';
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
			goto cpucontinue; //continue;
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
cpucontinue:;

	} while (CPU_REMCLOCK > 0);

#if defined(SUPPORT_ASYNC_CPU)
	// 非同期CPU処理
	if(np2cfg.asynccpu && !cpu_nowait){
#define LATECOUNTER_THRESHOLD	6
#define LATECOUNTER_THRESHOLDM	2
		if(CPU_STAT_HLT){
			hltflag = pccore.multiple;
		}
		if (!asynccpu_fastflag && !asynccpu_lateflag) {
			double timimg = np2cpu_lastTimingValue;
			if (timimg > cpu_drawskip) {
				latecount++;
				if (latecount > +LATECOUNTER_THRESHOLD) {
					if (pccore.multiple > 4) {
						UINT32 oldmultiple = pccore.multiple;
						if (pccore.multiple > 40) {
							if (timimg > 2.0) {
								pccore.multiple -= 10;
							}
							else if (timimg > 1.5) {
								pccore.multiple -= 5;
							}
							else if (timimg > 1.2) {
								pccore.multiple -= 3;
							}
							else {
								pccore.multiple -= 1;
							}
						}
						else if (pccore.multiple > 20) {
							if (timimg > 2.0) {
								pccore.multiple -= 6;
							}
							else if (timimg > 1.5) {
								pccore.multiple -= 3;
							}
							else if (timimg > 1.2) {
								pccore.multiple -= 2;
							}
							else {
								pccore.multiple -= 1;
							}
						}
						else {
							pccore.multiple -= 1;
						}
						pccore.realclock = pccore.baseclock * pccore.multiple;
						nevent_changeclock(oldmultiple, pccore.multiple);

						sound_changeclock();
						pcm86_changeclock(oldmultiple);
						beep_changeclock();
						mpu98ii_changeclock();
#if defined(SUPPORT_SMPU98)
						smpu98_changeclock();
#endif
						keyboard_changeclock();
						mouseif_changeclock();
						gdc_updateclock();
					}

					latecount = 0;
				}
				asynccpu_lateflag = 1;
			}
			else if(timimg < cpu_drawskip){
				if (!hltflag && g_nevent.item[NEVENT_FLAMES].proc == screendisp && g_nevent.item[NEVENT_FLAMES].clock >= CPU_BASECLOCK) {
					latecount--;
					if (latecount < -LATECOUNTER_THRESHOLDM) {
						if (pccore.multiple < pccore.maxmultiple) {
							UINT32 oldmultiple = pccore.multiple;
							if (timimg < 0.5) {
								pccore.multiple += 3;
							}
							else if (timimg < 0.7) {
								pccore.multiple += 2;
							}
							else {
								pccore.multiple += 1;
							}
							pccore.realclock = pccore.baseclock * pccore.multiple;
							nevent_changeclock(oldmultiple, pccore.multiple);

							sound_changeclock();
							pcm86_changeclock(oldmultiple);
							beep_changeclock();
							mpu98ii_changeclock();
#if defined(SUPPORT_SMPU98)
							smpu98_changeclock();
#endif
							keyboard_changeclock();
							mouseif_changeclock();
							gdc_updateclock();
						}
						latecount = 0;
					}
					asynccpu_fastflag = 1;
				}
			}
		}
	}
	if(hltflag > 0) hltflag--;
#endif
}
