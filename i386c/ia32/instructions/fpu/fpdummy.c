#include "compiler.h"
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"
#include "fp.h"


void
ESC0(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU d8 %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC1(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU d9 %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		switch (op & 0x38) {
		case 0x28:
			TRACEOUT(("FLDCW"));
			(void) cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
			break;

		case 0x38:
			TRACEOUT(("FSTCW"));
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, 0xffff);
			break;

		default:
			EXCEPTION(NM_EXCEPTION, 0);
			break;
		}
	}
}

void
ESC2(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU da %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC3(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU db %.2x", op));
	if (op >= 0xc0) {
		if (op != 0xe3) {
			EXCEPTION(NM_EXCEPTION, 0);
		}
		/* FNINIT */
		(void)madr;
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC4(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU dc %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC5(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU dd %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		if (((op >> 3) & 7) != 7) {
			EXCEPTION(NM_EXCEPTION, 0);
		}
		/* FSTSW */
		TRACEOUT(("FSTSW"));
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, 0xffff);
	}
}

void
ESC6(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU de %.2x", op));
	if (op >= 0xc0) {
		EXCEPTION(NM_EXCEPTION, 0);
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

void
ESC7(void)
{
	UINT32 op, madr;

	GET_PCBYTE(op);
	TRACEOUT(("use FPU df %.2x", op));
	if (op >= 0xc0) {
		if (op != 0xe0) {
			EXCEPTION(NM_EXCEPTION, 0);
		}
		/* FSTSW AX */
		TRACEOUT(("FSTSW AX"));
		CPU_AX = 0xffff;
	} else {
		madr = calc_ea_dst(op);
		EXCEPTION(NM_EXCEPTION, 0);
	}
}
