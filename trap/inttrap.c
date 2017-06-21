/**
 * @file	inttrap.c
 * @brief	Implementation of the trap of interrupt
 */

#include "compiler.h"

#if defined(ENABLE_TRAP)
#include "inttrap.h"
#include "cpucore.h"

void CPUCALL softinttrap(UINT cs, UINT32 eip, UINT vect) {

// ---- ここにトラップ条件コードを書きます
	if (vect == 0x7f) {
		switch(CPU_AH) {
			case 0:
				TRACEOUT(("INT-7F AH=00 Load data DS:DX = %.4x:%.4x", CPU_DS, CPU_DX));
				break;

			case 1:
				TRACEOUT(("INT-7F AH=01 Play data AL=%.2x", CPU_AL));
				break;

			case 2:
				TRACEOUT(("INT-7F AH=02 Stop Data"));
				break;

//			case 3:
//				TRACEOUT(("INT-7F AH=03 Get Status"));
//				break;

			case 4:
				TRACEOUT(("INT-7F AH=04 Set Parameter AL=%.2x", CPU_AL));
				break;
		}
	}
	return;

	if (vect == 0x50) {
		if (CPU_AX != 9)
		TRACEOUT(("%.4x:%.4x INT-50 AX=%.4x BX=%.4x DX=%.4x", cs, eip, CPU_AX, CPU_BX, CPU_DX));
	}
	if (vect == 0x51) {
		TRACEOUT(("%.4x:%.4x INT-51 AX=%.4x BX=%.4x DX=%.4x", cs, eip, CPU_AX, CPU_BX, CPU_DX));
	}
	return;

#if 0
//	if (vect == 0x2f) {
//		TRACEOUT(("%.4x:%.4x INT-2F AX=%.4x", cs, eip, CPU_AX));
//	}
	if (vect == 0x67) {
		TRACEOUT(("%.4x:%.4x INT-67 AX=%.4x BX=%.4x DX=%.4x", cs, eip, CPU_AX, CPU_BX, CPU_DX));
	}
#endif
#if 0
	if ((vect == 0x42) && (CPU_AL != 6)) {
		TRACEOUT(("%.4x:%.4x INT-42 AL=%.2x", cs, eip, CPU_AL));
	}
#endif
#if 0
	if (vect == 0x2f) {
		TRACEOUT(("%.4x:%.4x INT-2f BX=%.4x/DX=%.4x", cs, eip, CPU_BX, CPU_DX));
	}
#endif
#if 0
	if (vect == 0xd2) {
		TRACEOUT(("%.4x:%.4x INT-d2 AX=%.4x", cs, eip, CPU_AX));
	}
#endif
#if 0
	if (vect == 0x60) {
		TRACEOUT(("%.4x:%.4x INT-60 AX=%.4x", cs, eip, CPU_AX));
	}
#endif
#if 0
	if (vect == 0xa0) {
		TRACEOUT(("%.4x:%.4x INT-a0 AX=%.4x", cs, eip, CPU_AX));
	}
	if (vect == 0xa2) {
		TRACEOUT(("%.4x:%.4x INT-a2", cs, eip));
	}
	if (vect == 0xa4) {
		TRACEOUT(("%.4x:%.4x INT-a4", cs, eip));
	}
#endif
#if 0
	if (vect == 0x60) {
		TRACEOUT(("%.4x:%.4x INT-60 AH=%.2x", cs, eip, CPU_AH));
		if (CPU_AH == 1) {
			TRACEOUT(("->%.4x:%.4x", CPU_ES, CPU_BX));
		}
	}
#endif
#if 0
	if (vect == 0x40) {
		TRACEOUT(("%.4x:%.4x INT-40 AH=%.2x", cs, eip, CPU_AH));
	}
	if (vect == 0x66) {
		switch(CPU_AL) {
			case 1:
				TRACEOUT(("%.4x:%.4x INT-66:01 play", cs, eip));
				break;
			case 2:
				TRACEOUT(("%.4x:%.4x INT-66:02 stop", cs, eip));
				break;
			case 9:
				TRACEOUT(("%.4x:%.4x INT-66:09 setdata AH=%.2x ES:BX=%.4x:%.4x DX=%.4x", cs, eip, CPU_AH, CPU_ES, CPU_BX, CPU_DX));
				break;
			case 0x0d:
				TRACEOUT(("%.4x:%.4x INT-66:0d setdata ES:BX=%.4x:%.4x", cs, eip, CPU_ES, CPU_BX));
				break;

			default:
				TRACEOUT(("%.4x:%.4x INT-66 AX=%.4x", cs, eip, CPU_AX));
				break;
		}
	}
#endif
#if 0
	if (vect == 0x40) {
		TRACEOUT(("%.4x:%.4x INT-40 AX=%.4x DS=%.4x DI=%.4x", cs, eip, CPU_AX, CPU_DS, CPU_DI));
	}
	if (vect == 0x41) {
		TRACEOUT(("%.4x:%.4x INT-41 AX=%.4x DX=%.4x", cs, eip, CPU_AX, CPU_DX));
	}
#endif
#if 0
	if (vect == 0x41) {
		TRACEOUT(("%.4x:%.4x INT-41 AX=%.4x %.4x:%.4x",
							cs, eip, CPU_AX, CPU_DS, CPU_SI));
	}
	if (vect == 0x42) {
		switch(CPU_AH) {
			case 0xd3:
			case 0xd0:
				break;

			case 0xfd:
			case 0xfc:
			case 0xfa:
			case 0xf8:
			case 0xe3:
				TRACEOUT(("%.4x:%.4x INT-42 AX=%.4x %.4x:%.4x",
							cs, eip, CPU_AX, CPU_BX, CPU_BP));
				break;

			default:
				TRACEOUT(("%.4x:%.4x INT-42 AX=%.4x", cs, eip, CPU_AX));
				break;
		}
	}
#endif
#if 0
	if (vect == 0x40) {
		TRACEOUT(("%.4x:%.4x INT-40 SI=%.4x %.4x:%.4x:%.4x",
									cs, eip, CPU_SI,
									MEMR_READ16(CPU_DS, CPU_SI + 0),
									MEMR_READ16(CPU_DS, CPU_SI + 2),
									MEMR_READ16(CPU_DS, CPU_SI + 4)));
	}
	if (vect == 0xd2) {
		TRACEOUT(("%.4x:%.4x INT-D2 AX=%.4x", cs, eip, CPU_AX));
	}
#endif
#if 0
	if (vect == 0x40) {
		TRACEOUT(("INT 40H - AL=%.2x", CPU_AL));
	}
#endif
#if 1
	if (vect == 0x21) {
		char f[128];
		UINT i;
		char c;
		switch(CPU_AH) {
			case 0x3d:
				for (i=0; i<127; i++) {
					c = MEMR_READ8(CPU_DS, CPU_DX + i);
					if (c == '\0') break;
					f[i] = c;
				}
				f[i] = 0;
				TRACEOUT(("DOS: %.4x:%.4x Open Handle AL=%.2x DS:DX=%.4x:%.4x[%s]", cs, eip, CPU_AL, CPU_DS, CPU_DX, f));
				break;

			case 0x3f:
				TRACEOUT(("DOS: %.4x:%.4x Read Handle BX=%.4x DS:DX=%.4x:%.4x CX=%.4x", cs, eip, CPU_BX, CPU_DS, CPU_DX, CPU_CX));
				break;

			case 0x42:
				TRACEOUT(("DOS: %.4x:%.4x Move File Pointer BX=%.4x CX:DX=%.4x:%.4x AL=%.2x", cs, eip, CPU_BX, CPU_CX, CPU_DX, CPU_AL));
				break;
		}
	}
#endif
#if 0
	if (vect == 0xf5) {
		TRACEOUT(("%.4x:%.4x INT-F5 AH=%.2x STACK=%.4x", cs, eip,
								CPU_AH, MEMR_READ16(CPU_SS, CPU_SP + 2)));
	}
#endif
#if 0
	if (vect == 0x69) {
		TRACEOUT(("%.4x:%.4x INT-69 AX=%.4x", cs, eip, CPU_AX));
	}
#endif
#if 0
	if ((vect == 0x40) && (CPU_AX != 4)) {
		TRACEOUT(("%.4x:%.4x INT-40 AX=%.4x", cs, eip, CPU_AX));
	}
#endif
#if 0
	if (vect == 0x7f) {
		switch(CPU_AH) {
			case 0:
				TRACEOUT(("INT-7F AH=00 Load data DS:DX = %.4x:%.4x", CPU_DS, CPU_DX));
				break;

			case 1:
				TRACEOUT(("INT-7F AH=01 Play data AL=%.2x", CPU_AL));
				break;

			case 2:
				TRACEOUT(("INT-7F AH=02 Stop Data"));
				break;

			case 3:
				TRACEOUT(("INT-7F AH=03 Get Status"));
				break;

			case 4:
				TRACEOUT(("INT-7F AH=04 Set Parameter AL=%.2x", CPU_AL));
				break;
		}
	}
#endif
#if defined(TRACE)
	if (vect == 0x7f) {
		UINT i, j;
		switch(CPU_AH) {
			case 0:
				TRACEOUT(("INT-7F AH=00 Load data DS:DX = %.4x:%.4x", CPU_DS, CPU_DX));
				for (i=0; i<16; i+=4) {
					char buf[256];
					for (j=0; j<4; j++) {
						sprintf(buf + (j * 6), "0x%.2x, ",
										MEMR_READ8(CPU_DS, CPU_DX + i + j));
					}
					TRACEOUT(("%s", buf));
				}
				break;

			case 1:
				TRACEOUT(("INT-7F AH=01 Play data AL=%.2x", CPU_AL));
				break;

			case 2:
				TRACEOUT(("INT-7F AH=02 Stop Data"));
				break;

			case 3:
//				TRACEOUT(("INT-7F AH=03 Get Status"));
				break;

			case 4:
				TRACEOUT(("INT-7F AH=04 Set Parameter AL=%.2x", CPU_AL));
				break;

			default:
				TRACEOUT(("INT-7F AH=%.2x", CPU_AH));
				break;
		}
	}
#endif
#if 0 // defined(TRACE)
	if ((vect >= 0xa0) && (vect < 0xb0)) {
extern void lio_look(UINT vect);
		lio_look(vect);
	}
#endif
}

#endif

