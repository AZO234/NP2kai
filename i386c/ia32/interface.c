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

#include "compiler.h"
#include "cpu.h"
#include "ia32.mcr"
#if defined(USE_FPU)
#include "instructions/fpu/fp.h"
#endif

#include "pccore.h"
#include "iocore.h"
#include "dmax86.h"
#include "bios/bios.h"


void
ia32_initreg(void)
{
	int i;

	CPU_STATSAVE.cpu_inst_default.seg_base = (UINT32)-1;

	CPU_EDX = (CPU_FAMILY << 8) | (CPU_MODEL << 4) | CPU_STEPPING;
	CPU_EFLAG = 2;
	CPU_CR0 = CPU_CR0_CD | CPU_CR0_NW;
#if defined(USE_FPU)
	CPU_CR0 &= ~CPU_CR0_EM;
	CPU_CR0 |= CPU_CR0_ET;
#else
	CPU_CR0 |= CPU_CR0_EM | CPU_CR0_NE;
	CPU_CR0 &= ~(CPU_CR0_MP | CPU_CR0_ET);
#endif
	CPU_MXCSR = 0x1f80;

	CPU_GDTR_BASE = 0x0;
	CPU_GDTR_LIMIT = 0xffff;
	CPU_IDTR_BASE = 0x0;
	CPU_IDTR_LIMIT = 0xffff;
	CPU_LDTR_BASE = 0x0;
	CPU_LDTR_LIMIT = 0xffff;
	CPU_TR_BASE = 0x0;
	CPU_TR_LIMIT = 0xffff;

	CPU_STATSAVE.cpu_regs.dr[6] = 0xffff1ff0;

	for (i = 0; i < CPU_SEGREG_NUM; ++i) {
		segdesc_init(i, 0, &CPU_STAT_SREG(i));
	}
	LOAD_SEGREG(CPU_CS_INDEX, 0xf000);
	CPU_STAT_CS_BASE = 0xffff0000;
	CPU_EIP = 0xfff0;
	CPU_ADRSMASK = 0x000fffff;

	tlb_init();
#if defined(USE_FPU)
	fpu_init();
#endif
}

void
ia32reset(void)
{

	memset(&i386core.s, 0, sizeof(i386core.s));
	ia32_initreg();
}

void
ia32shut(void)
{

	memset(&i386core.s, 0, offsetof(I386STAT, cpu_type));
	ia32_initreg();
}

void
ia32a20enable(BOOL enable)
{

	CPU_ADRSMASK = (enable)?0xffffffff:0x00ffffff;
}

void
ia32(void)
{
	int rv;

	rv = sigsetjmp(exec_1step_jmpbuf, 1);
	switch (rv) {
	case 0:
		break;

	case 1:
		VERBOSE(("ia32: return from exception"));
		break;

	case 2:
		VERBOSE(("ia32: return from panic"));
		return;

	default:
		VERBOSE(("ia32: return from unknown cause"));
		break;
	}

	if (CPU_TRAP) {
		do {
			exec_1step();
			if (CPU_TRAP) {
				CPU_DR6 |= CPU_DR6_BS;
				INTERRUPT(1, INTR_TYPE_EXCEPTION);
			}
			dmax86();
		} while (CPU_REMCLOCK > 0);
	} else if (dmac.working) {
		do {
			exec_1step();
			dmax86();
		} while (CPU_REMCLOCK > 0);
	} else {
		do {
			exec_1step();
		} while (CPU_REMCLOCK > 0);
	}
}

void
ia32_step(void)
{
	int rv;

	rv = sigsetjmp(exec_1step_jmpbuf, 1);
	switch (rv) {
	case 0:
		break;

	case 1:
		VERBOSE(("ia32_step: return from exception"));
		break;

	case 2:
		VERBOSE(("ia32_step: return from panic"));
		return;

	default:
		VERBOSE(("ia32_step: return from unknown cause"));
		break;
	}

	do {
		exec_1step();
		if (CPU_TRAP) {
			CPU_DR6 |= CPU_DR6_BS;
			INTERRUPT(1, INTR_TYPE_EXCEPTION);
		}
		if (dmac.working) {
			dmax86();
		}
	} while (CPU_REMCLOCK > 0);
}

void CPUCALL
ia32_interrupt(int vect, int soft)
{

//	TRACEOUT(("int (%x, %x) PE=%d VM=%d",  vect, soft, CPU_STAT_PM, CPU_STAT_VM86));
	if (!soft) {
		INTERRUPT(vect, INTR_TYPE_EXTINTR);
	} else {
		if (CPU_STAT_PM && CPU_STAT_VM86 && CPU_STAT_IOPL < CPU_IOPL3) {
			VERBOSE(("ia32_interrupt: VM86 && IOPL < 3 && INTn"));
			EXCEPTION(GP_EXCEPTION, 0);
		}
		INTERRUPT(vect, INTR_TYPE_SOFTINTR);
	}
}


/*
 * error function
 */
void
ia32_panic(const char *str, ...)
{
	extern char *cpu_reg2str(void);
	char buf[2048];
	va_list ap;

	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);
	strcat(buf, "\n");
	strcat(buf, cpu_reg2str());
	VERBOSE(("%s", buf));

	msgbox("ia32_panic", buf);

#if defined(IA32_REBOOT_ON_PANIC)
	VERBOSE(("ia32_panic: reboot"));
	pccore_reset();
	siglongjmp(exec_1step_jmpbuf, 2);
#else
	__ASSERT(0);
	exit(1);
#endif
}

void
ia32_warning(const char *str, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);
	strcat(buf, "\n");

	msgbox("ia32_warning", buf);
}

void
ia32_printf(const char *str, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);
	strcat(buf, "\n");

	msgbox("ia32_printf", buf);
}


/*
 * bios call interface
 */
void
ia32_bioscall(void)
{
	UINT32 adrs;

	if (!CPU_STAT_PM || CPU_STAT_VM86) {
#if 1
		adrs = CPU_PREV_EIP + (CPU_CS << 4);
#else
		adrs = CPU_PREV_EIP + CPU_STAT_CS_BASE;
#endif
		if ((adrs >= 0xf8000) && (adrs < 0x100000)) {
			if (biosfunc(adrs)) {
				/* Nothing to do */
			}
			LOAD_SEGREG(CPU_ES_INDEX, CPU_ES);
			LOAD_SEGREG(CPU_CS_INDEX, CPU_CS);
			LOAD_SEGREG(CPU_SS_INDEX, CPU_SS);
			LOAD_SEGREG(CPU_DS_INDEX, CPU_DS);
		}
	}
}
