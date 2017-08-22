#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"


// ---- CMT

void bios0x1a_cmt(void) {

	if (CPU_AH == 0x04) {
		CPU_AH = 0x02;
	}
	else {
		CPU_AH = 0x00;
	}
}


// ---- Printer

static void printerbios_11(void) {

	if (iocore_inp8(0x42) & 0x04) {				// busy?
		CPU_AH = 0x01;
		iocore_out8(0x40, CPU_AL);
#if 0
		iocore_out8(0x46, 0x0e);
		iocore_out8(0x46, 0x0f);
#endif
	}
	else {
		CPU_AH = 0x02;
	}
}

void bios0x1a_prt(void) {

	switch(CPU_AH & 0x0f) {
		case 0x00:
			if (CPU_AH == 0x30) {
				if (CPU_CX) {
					do {
						CPU_AL = MEMR_READ8(CPU_ES, CPU_BX);
						printerbios_11();
						if (CPU_AH & 0x02) {
							CPU_AH = 0x02;
							return;
						}
						CPU_BX++;
					} while(--CPU_CX);
					CPU_AH = 0x00;
				}
				else {
					CPU_AH = 0x02;
				}
			}
			else {
				iocore_out8(0x37, 0x0d);				// printer f/f
				iocore_out8(0x46, 0x82);				// reset
				iocore_out8(0x46, 0x0f);				// PSTB inactive
				iocore_out8(0x37, 0x0c);				// printer f/f
				CPU_AH = (iocore_inp8(0x42) >> 2) & 1;
			}
			break;

		case 0x01:
			printerbios_11();
			break;

		case 0x02:
			CPU_AH = (iocore_inp8(0x42) >> 2) & 1;
			break;

		default:
			CPU_AH = 0x00;
			break;
	}
}

