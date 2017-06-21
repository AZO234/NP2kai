/**
 * @file	steptrap.c
 * @brief	Implementation of the step trap
 */

#include "compiler.h"

#define	IPTRACE			(1 << 12)

#if defined(ENABLE_TRAP)
#include "steptrap.h"
#include "dosio.h"
#include "pccore.h"
#include "cpucore.h"
#include "sound.h"
#include "fmboard.h"


#if IPTRACE
static	UINT	trpos = 0;
static	UINT32	trcs[IPTRACE];
static	UINT32	treip[IPTRACE];
#endif

void CPUCALL steptrap(UINT cs, UINT32 eip) {

//	TRACEOUT(("%.4x:%.4x", cs, eip));

#if IPTRACE
	trcs[trpos & (IPTRACE - 1)] = cs;
	treip[trpos & (IPTRACE - 1)] = eip;
	trpos++;
#endif


// ---- ここにトラップ条件コードを書きます
{
	static UINT32 x;
	UINT32 tmp;
	tmp = *(UINT32 *)(mem + 0x7ade1);
	if (x != tmp) {
		TRACEOUT(("%.4x:%.4x: %.8x -> %.8x", cs, eip, x, tmp));
		x = tmp;
	}
}

	return;

	if ((cs == 0x1ea) && (eip == 0x0617)) {
		TRACEOUT(("ES:BP+1 = %.2x", CPU_AL));
	}
	return;

	if ((cs == 0x0601) && (eip == 0x025c)) {
		TRACEOUT(("INT-D2 AX=%.4x BX=%.4x DX=%.4x", CPU_AX, CPU_BX, CPU_DX));
	}
	return;

// if (cs4231.intflag) TRACEOUT(("%.4x:%.4x", cs, eip));
	if (cs == 0x1311) TRACEOUT(("%.4x:%.4x", cs, eip));
	if (cs == 0x0d77) TRACEOUT(("%.4x:%.4x", cs, eip));
	if (cs == 0x0f5d) TRACEOUT(("%.4x:%.4x", cs, eip));
	if (cs == 0x0e91) TRACEOUT(("%.4x:%.4x", cs, eip));

#if 0
{
	static UINT tmp = 0;
	if (tmp != *(UINT16 *)(mem + 0xa0000 + 18 * 2)) {
		TRACEOUT(("change text %.4x -> %.4x at %.4x:%.4x", tmp, *(UINT16 *)(mem + 0xa0000 + 18 * 2), cs, eip));
		tmp = *(UINT16 *)(mem + 0xa0000 + 18 * 2);
		if (tmp == 0x4303) {
			steptrap_hisfileout();
		}
	}
}
#endif


	// IDEテスト用
#if 0
	if ((cs == 0x1300) && (eip == 0x1E97)) {
		TRACEOUT(("-------- NECCD: function: %.2x", CPU_AL));
	}
	if (cs == 0xdf6) TRACEOUT(("%.4x:%.4x", CPU_CS, CPU_IP));
#endif
#if 1
	if ((cs == 0x0620) && (eip == 0x1E97)) {
		TRACEOUT(("-------- NECCD: function: %.2x", CPU_AL));
	}
#endif

	return;

#if 0
	if (cs == 0x05a0) {
		if (eip == 0xe2) {
			TRACEOUT(("result: %.2x", CPU_AH));
		}
	}
#endif
#if 0
	if (cs == 0x0000) {
		if (eip == 0x1191) {
			char buf[10];
			int i;
			for (i=0; i<6; i++) {
				buf[i] = MEML_READ8(0x1000, CPU_BX + i);
			}
			buf[6] = '\0';
			TRACEOUT(("load: %s", buf));
		}
		if (eip == 0x1265) {
			TRACEOUT(("%.4x:%.4x addr=%.4x ret=%.4x",
									cs, eip, CPU_DX,
										MEML_READ16(CPU_SS, CPU_SP)));
		}
	}
#endif
#if 0
	if (cs == 0x0080) {
		if (eip == 0x052A) {
			UINT i;
			UINT addr;
			char fname[9];
			addr = MEML_READ16(CPU_SS, CPU_BP + 4);
			for (i=0; i<8; i++) {
				fname[i] = MEML_READ8(CPU_DS, addr + i);
			}
			fname[8] = 0;
			TRACEOUT(("%.4x:%.4x play... addr:%.4x %s",
											cs, eip, addr, fname));
		}
	}
#endif
#if 0
	if (cs == 0x800) {
		if (eip == 0x017A) {
			TRACEOUT(("%.4x:%.4x solve... DS=%.4x SIZE=%.4x KEY=%.4x",
								cs, eip,
								MEML_READ16(CPU_SS, CPU_BP - 0x06),
								CPU_DX,
								MEML_READ16(CPU_SS, CPU_BP - 0x08)));
		}
	}
#endif
#if 0
	if (cs == 0x3d52) {
		if (eip == 0x4A57) {
			TRACEOUT(("%.4x:%.4x %.4x:%.4x/%.4x/%.4x",
								CPU_DX, CPU_BX, CPU_DS,
								MEML_READ16(CPU_SS, CPU_BP + 0x06),
								MEML_READ16(CPU_SS, CPU_BP + 0x08),
								MEML_READ16(CPU_SS, CPU_BP + 0x0a)));
		}
		if (eip == 0x4BF8) {
			debugsub_memorydump();
		}
#if 0
		if (eip == 0x4B7A) {
			TRACEOUT(("datum = %x", CPU_AX));
		}
		if (eip == 0x4B87) {
			TRACEOUT(("leng = %x", CPU_AX));
		}
		if (eip == 0x4BD5) {
			TRACEOUT(("%.4x:%.4x <- %.2x[%.4x]",
						CPU_ES, CPU_BX, CPU_AL, CPU_DI));
		}
#endif
	}
#endif
#if 0	// DC
	if (cs == 0x1000) {
		if (eip == 0x5924) {
			TRACEOUT(("%.4x:%.4x -> %.4x:%.4x", cs, eip,
							MEML_READ16(CPU_DS, 0x6846),
							MEML_READ16(CPU_DS, 0x6848)));
		}
	}
#endif
#if 0	// 羅針盤
	if (cs == 0x60) {
		if (eip == 0xADF9) {
			TRACEOUT(("%.4x:%.4x -> %.4x:%.4x:%.4x", cs, eip, CPU_BX, CPU_SI, CPU_AX));
		}
		else if (eip == 0xC7E1) {
			TRACEOUT(("%.4x:%.4x -> %.4x:%.4x", cs, eip, CPU_ES, CPU_BX));
		}
	}
#endif
#if 0
	if (cs == 0x60) {
		if (eip == 0x8AC2) {
			UINT pos = CPU_SI + (CPU_AX * 6);
			TRACEOUT(("%.4x:%.4x -> %.4x:%.4x-%.4x:%.4x [%.2x %.2x %.2x %.2x %.2x %.2x]", cs, eip, CPU_SI, CPU_AX, CPU_DX, CPU_DI,
									MEML_READ8(CPU_DS, pos+0),
									MEML_READ8(CPU_DS, pos+1),
									MEML_READ8(CPU_DS, pos+2),
									MEML_READ8(CPU_DS, pos+3),
									MEML_READ8(CPU_DS, pos+4),
									MEML_READ8(CPU_DS, pos+5)));
		}
	}
#endif
}


#if IPTRACE
void steptrap_hisfileout(void) {

	UINT	s;
	FILEH	fh;
	UINT	pos;
	char	buf[32];

	s = trpos;
	if (s > IPTRACE) {
		s -= IPTRACE;
	}
	else {
		s = 0;
	}
	fh = file_create_c(OEMTEXT("his.txt"));
	while(s < trpos) {
		pos = s & (IPTRACE - 1);
		s++;
		SPRINTF(buf, "%.4x:%.8x\r\n", trcs[pos], treip[pos]);
		file_write(fh, buf, STRLEN(buf));
	}
	file_close(fh);
}
#else
void steptrap_hisfileout(void) {
}
#endif

#endif

