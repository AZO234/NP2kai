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

#include "compiler.h"
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "flag_ctrl.h"


/*
 * flag contol instructions
 */
void
STC(void)
{

	CPU_WORKCLOCK(2);
	CPU_FLAGL |= C_FLAG;
}

void
CLC(void)
{

	CPU_WORKCLOCK(2);
	CPU_FLAGL &= ~C_FLAG;
}

void
CMC(void)
{

	CPU_WORKCLOCK(2);
	CPU_FLAGL ^= C_FLAG;
}

void
CLD(void)
{

	CPU_WORKCLOCK(2);
	CPU_FLAG &= ~D_FLAG;
}

void
STD(void)
{

	CPU_WORKCLOCK(2);
	CPU_FLAG |= D_FLAG;
}

void
LAHF(void)
{

	CPU_WORKCLOCK(2);
	CPU_AH = (CPU_FLAGL & SZAPC_FLAG) | 0x2;	/* SZ0A0P1C */
}

void
SAHF(void)
{

	CPU_WORKCLOCK(2);
	CPU_FLAGL = (CPU_AH & SZAPC_FLAG) | 0x2;	/* SZ0A0P1C */
}

/*
 * PUSHF/POPF
 */
void
PUSHF_Fw(void)
{

	CPU_WORKCLOCK(3);
	if (!CPU_STAT_PM || !CPU_STAT_VM86 || (CPU_STAT_IOPL == CPU_IOPL3)) {
		UINT16 flags = REAL_FLAGREG;
		flags = (flags & ALL_FLAG) | 2;
		PUSH0_16(flags);
		return;
	}
	/* VM86 && IOPL != 3 */
	EXCEPTION(GP_EXCEPTION, 0);
}

void
PUSHFD_Fd(void)
{

	CPU_WORKCLOCK(3);
	if (!CPU_STAT_PM || !CPU_STAT_VM86 || (CPU_STAT_IOPL == CPU_IOPL3)) {
		UINT32 flags = REAL_EFLAGREG & ~(RF_FLAG|VM_FLAG);
		flags = (flags & ALL_EFLAG) | 2;
		PUSH0_32(flags);
		return;
	}
	/* VM86 && IOPL != 3 */
	EXCEPTION(GP_EXCEPTION, 0);
}

void
POPF_Fw(void)
{
	UINT16 flags, mask;

	CPU_WORKCLOCK(3);
	CPU_SET_PREV_ESP();
	if (!CPU_STAT_PM) {
		/* Real Mode */
		POP0_16(flags);
		mask = I_FLAG|IOPL_FLAG;
	} else if (!CPU_STAT_VM86) {
		/* Protected Mode */
		POP0_16(flags);
		if (CPU_STAT_CPL == 0) {
			mask = I_FLAG|IOPL_FLAG;
		} else if (CPU_STAT_CPL <= CPU_STAT_IOPL) {
			mask = I_FLAG;
		} else {
			mask = 0;
		}
	} else if (CPU_STAT_IOPL == CPU_IOPL3) {
		/* Virtual-8086 Mode, IOPL == 3 */
		POP0_16(flags);
		mask = I_FLAG;
	} else {
		EXCEPTION(GP_EXCEPTION, 0);
		flags = 0;
		mask = 0;
		/* compiler happy */
	}
	set_eflags(flags, mask);
	CPU_CLEAR_PREV_ESP();
	IRQCHECKTERM();
}

void
POPFD_Fd(void)
{
	UINT32 flags, mask;

	CPU_WORKCLOCK(3);
	CPU_SET_PREV_ESP();
	if (!CPU_STAT_PM) {
		/* Real Mode */
		POP0_32(flags);
		flags &= ~(RF_FLAG|VIF_FLAG|VIP_FLAG);
		mask = I_FLAG|IOPL_FLAG|RF_FLAG|VIF_FLAG|VIP_FLAG;
	} else if (!CPU_STAT_VM86) {
		/* Protected Mode */
		POP0_32(flags);
		flags &= ~RF_FLAG;
		if (CPU_STAT_CPL == 0) {
			flags &= ~(VIP_FLAG|VIF_FLAG);
			mask = I_FLAG|IOPL_FLAG|RF_FLAG|VIF_FLAG|VIP_FLAG;
		} else if (CPU_STAT_CPL <= CPU_STAT_IOPL) {
			flags &= ~(VIP_FLAG|VIF_FLAG);
			mask = I_FLAG|RF_FLAG|VIF_FLAG|VIP_FLAG;
		} else {
			mask = RF_FLAG;
		}
	} else if (CPU_STAT_IOPL == CPU_IOPL3) {
		/* Virtual-8086 Mode, IOPL == 3 */
		POP0_32(flags);
		mask = I_FLAG;
	} else {
		EXCEPTION(GP_EXCEPTION, 0);
		flags = 0;
		mask = 0;
		/* compiler happy */
	}
	set_eflags(flags, mask);
	CPU_CLEAR_PREV_ESP();
	IRQCHECKTERM();
}

void
STI(void)
{

	CPU_WORKCLOCK(2);
	if (CPU_STAT_PM) {
		if (!CPU_STAT_VM86) {
			if (CPU_STAT_CPL > CPU_STAT_IOPL) {
				EXCEPTION(GP_EXCEPTION, 0);
			}
		} else {
			if (CPU_STAT_IOPL < 3) {
				EXCEPTION(GP_EXCEPTION, 0);
			}
		}
	}
	CPU_FLAG |= I_FLAG;
	CPU_TRAP = (CPU_FLAG & (I_FLAG|T_FLAG)) == (I_FLAG|T_FLAG);
	exec_1step();
	IRQCHECKTERM();
}

void
CLI(void)
{

	CPU_WORKCLOCK(2);
	if (CPU_STAT_PM) {
		if (!CPU_STAT_VM86) {
			if (CPU_STAT_CPL > CPU_STAT_IOPL) {
				EXCEPTION(GP_EXCEPTION, 0);
			}
		} else {
			if (CPU_STAT_IOPL < 3) {
				EXCEPTION(GP_EXCEPTION, 0);
			}
		}
	}
	CPU_FLAG &= ~I_FLAG;
	CPU_TRAP = 0;
}
