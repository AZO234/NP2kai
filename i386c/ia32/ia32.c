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

#if defined(SUPPORT_IA32_HAXM)
#include "i386hax/haxfunc.h"
#include "i386hax/haxcore.h"
#endif

I386CORE	i386core;
I386CPUID	i386cpuid = {I386CPUID_VERSION, CPU_VENDOR, CPU_FAMILY, CPU_MODEL, CPU_STEPPING, CPU_FEATURES, CPU_FEATURES_EX, CPU_BRAND_STRING, CPU_BRAND_ID, CPU_FEATURES_ECX, CPU_EFLAGS_MASK};
I386MSR		i386msr = {0};

UINT8	*reg8_b20[0x100];
UINT8	*reg8_b53[0x100];
UINT16	*reg16_b20[0x100];
UINT16	*reg16_b53[0x100];
UINT32	*reg32_b20[0x100];
UINT32	*reg32_b53[0x100];

void
ia32_init(void)
{
	int i;

	i386msr.version = I386MSR_VERSION;
	i386cpuid.version = I386CPUID_VERSION;

	memset(&i386core.s, 0, sizeof(i386core.s));
	ia32_initreg();
	memset(&i386msr.regs, 0, sizeof(i386msr.regs));

	for (i = 0; i < 0x100; ++i) {
		/* 8bit */
		if (i & 0x20) {
			/* h */
			reg8_b53[i] = &CPU_REGS_BYTEH((i >> 3) & 3);
		} else {
			/* l */
			reg8_b53[i] = &CPU_REGS_BYTEL((i >> 3) & 3);
		}

		if (i & 0x04) {
			/* h */
			reg8_b20[i] = &CPU_REGS_BYTEH(i & 3);
		} else {
			/* l */
			reg8_b20[i] = &CPU_REGS_BYTEL(i & 3);
		}

		/* 16bit */
		reg16_b53[i] = &CPU_REGS_WORD((i >> 3) & 7);
		reg16_b20[i] = &CPU_REGS_WORD(i & 7);

		/* 32bit */
		reg32_b53[i] = &CPU_REGS_DWORD((i >> 3) & 7);
		reg32_b20[i] = &CPU_REGS_DWORD(i & 7);
	}

	resolve_init();
}

void
ia32_setextsize(UINT32 size)
{
//#if defined(SUPPORT_LARGE_MEMORY)&&defined(_WIN32) && !defined(MEMTRACE) && !defined(MEMCHECK)
//	static int vallocflag = 0;
//	static int vallocsize = 0;
//	static LPVOID memblock = NULL;
//#endif

	if (CPU_EXTMEMSIZE != size) {
		UINT8 *extmem;
		extmem = CPU_EXTMEM;
		if (extmem != NULL) {
//#if defined(SUPPORT_LARGE_MEMORY) && defined(_WIN32) && !defined(MEMTRACE) && !defined(MEMCHECK)
//			if(vallocflag){
//				VirtualFree((LPVOID)extmem, vallocsize, MEM_DECOMMIT);
//				VirtualFree(memblock, 0, MEM_RELEASE);
//				vallocflag = 0;
//			}else
//#endif
			{
#if defined(SUPPORT_IA32_HAXM)
#if defined(NP2_WIN)
				_aligned_free(extmem);
#else
				free(extmem);
#endif
#else
				_MFREE(extmem);
#endif
			}
			extmem = NULL;
		}
		if (size != 0) {
//#if defined(SUPPORT_LARGE_MEMORY) && defined(_WIN32) && !defined(MEMTRACE) && !defined(MEMCHECK)
//			if(size > (255 << 20)){
//				HANDLE hp = OpenProcess(PROCESS_ALL_ACCESS, TRUE, GetCurrentProcessId());
//				vallocsize = size + 16;
//				SetProcessWorkingSetSize(hp, vallocsize + 50*1024*1024, vallocsize + 50*1024*1024);
//				CloseHandle(hp);
//				memblock = VirtualAlloc(NULL, vallocsize, MEM_RESERVE, PAGE_READWRITE);
//				extmem = (UINT8 *)VirtualAlloc(memblock, vallocsize, MEM_COMMIT, PAGE_READWRITE);
//				if(!extmem){
//					extmem = (UINT8 *)_MALLOC(size + 16, "EXTMEM");
//				}else{
//					vallocflag = 1;
//				}
//			}else
//#endif
			{
#if defined(SUPPORT_IA32_HAXM)
#if defined(NP2_WIN)
				extmem = (UINT8*)_aligned_malloc(size + 4096, 4096);
#else
				posix_memalign(&extmem, 4096, size + 4096);
#endif
#else
				extmem = (UINT8 *)_MALLOC(size + 16, "EXTMEM");
#endif
			}
		}
		if (extmem != NULL) {
			ZeroMemory(extmem, size + 16);
			CPU_EXTMEM = extmem;
			CPU_EXTMEMSIZE = size;
			CPU_EXTMEMBASE = CPU_EXTMEM - 0x100000;
			CPU_EXTLIMIT16 = MIN(size + 0x100000, 0xf00000);
			CPU_EXTLIMIT = size + 0x100000;
		}
		else {
#if defined(SUPPORT_LARGE_MEMORY)
			if(size != 0){
				msgbox("Error", "Cannot allocate extended memory.");
			}
#endif
			CPU_EXTMEM = NULL;
			CPU_EXTMEMSIZE = 0;
			CPU_EXTMEMBASE = NULL;
			CPU_EXTLIMIT16 = 0;
			CPU_EXTLIMIT = 0;
		}
	}
	CPU_EMSPTR[0] = mem + 0xc0000;
	CPU_EMSPTR[1] = mem + 0xc4000;
	CPU_EMSPTR[2] = mem + 0xc8000;
	CPU_EMSPTR[3] = mem + 0xcc000;
}

void
ia32_setemm(UINT frame, UINT32 addr) {

	UINT8	*ptr;

	frame &= 3;
	if (addr < USE_HIMEM) {
		ptr = mem + addr;
	}
	else if ((addr - 0x100000 + 0x4000) <= CPU_EXTMEMSIZE) {
		ptr = CPU_EXTMEM + (addr - 0x100000);
	}
	else {
		ptr = mem + 0xc0000 + (frame << 14);
	}
	CPU_EMSPTR[frame] = ptr;
}


/*
 * モード遷移
 */
void CPUCALL
change_pm(BOOL onoff)
{

	if (onoff) {
		VERBOSE(("change_pm: Entering to Protected-Mode..."));
	} else {
		VERBOSE(("change_pm: Leaveing from Protected-Mode..."));
	}

	CPU_INST_OP32 = CPU_INST_AS32 =
	    CPU_STATSAVE.cpu_inst_default.op_32 = 
	    CPU_STATSAVE.cpu_inst_default.as_32 = 0;
	CPU_STAT_SS32 = 0;
	set_cpl(0);
	CPU_STAT_PM = onoff;
}

void CPUCALL
change_pg(BOOL onoff)
{

	if (onoff) {
		VERBOSE(("change_pg: Entering to Paging-Mode..."));
	} else {
		VERBOSE(("change_pg: Leaveing from Paging-Mode..."));
	}

	CPU_STAT_PAGING = onoff;
}

void CPUCALL
change_vm(BOOL onoff)
{
	int i;

	CPU_STAT_VM86 = onoff;
	if (onoff) {
		VERBOSE(("change_vm: Entering to Virtual-8086-Mode..."));
		for (i = 0; i < CPU_SEGREG_NUM; i++) {
			LOAD_SEGREG(i, CPU_REGS_SREG(i));
		}
		CPU_INST_OP32 = CPU_INST_AS32 =
		    CPU_STATSAVE.cpu_inst_default.op_32 =
		    CPU_STATSAVE.cpu_inst_default.as_32 = 0;
		CPU_STAT_SS32 = 0;
		set_cpl(3);
	} else {
		VERBOSE(("change_vm: Leaveing from Virtual-8086-Mode..."));
	}
}

/*
 * flags
 */
static void CPUCALL
modify_eflags(UINT32 new_flags, UINT32 mask)
{
	UINT32 orig = CPU_EFLAG;

	new_flags &= ALL_EFLAG;
	mask &= ALL_EFLAG;
	CPU_EFLAG = (REAL_EFLAGREG & ~mask) | (new_flags & mask);

	CPU_OV = CPU_FLAG & O_FLAG;
	CPU_TRAP = (CPU_FLAG & (I_FLAG|T_FLAG)) == (I_FLAG|T_FLAG);
	if (CPU_STAT_PM) {
		if ((orig ^ CPU_EFLAG) & VM_FLAG) {
			if (CPU_EFLAG & VM_FLAG) {
				change_vm(1);
			} else {
				change_vm(0);
			}
		}
	}
}

void CPUCALL
set_flags(UINT16 new_flags, UINT16 mask)
{

	mask &= I_FLAG|IOPL_FLAG;
	mask |= SZAPC_FLAG|T_FLAG|D_FLAG|O_FLAG|NT_FLAG;
	modify_eflags(new_flags, mask);
}

void CPUCALL
set_eflags(UINT32 new_flags, UINT32 mask)
{

	mask &= I_FLAG|IOPL_FLAG|RF_FLAG|VM_FLAG|VIF_FLAG|VIP_FLAG;
	mask |= SZAPC_FLAG|T_FLAG|D_FLAG|O_FLAG|NT_FLAG;
	mask |= AC_FLAG|ID_FLAG;
	mask &= ~i386cpuid.cpu_eflags_mask;
	modify_eflags(new_flags, mask);
}

/*
 * CR3 (Page Directory Entry base physical address)
 */
void CPUCALL
set_cr3(UINT32 new_cr3)
{

	VERBOSE(("set_CR3: old = %08x, new = 0x%08x", CPU_CR3, new_cr3));

	CPU_CR3 = new_cr3 & CPU_CR3_MASK;
	CPU_STAT_PDE_BASE = CPU_CR3 & CPU_CR3_PD_MASK;
	tlb_flush();
}

/*
 * CPL (Current Privilege Level)
 */
void CPUCALL
set_cpl(int new_cpl)
{
	int cpl = new_cpl & 3;

	CPU_STAT_CPL = (UINT8)cpl;
	CPU_STAT_USER_MODE = (cpl == 3) ? CPU_MODE_USER : CPU_MODE_SUPERVISER;
}
