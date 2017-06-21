#include	"compiler.h"
#include	"parts.h"
#include	"timemng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"calendar.h"
#include	"bios.h"
#include	"biosmem.h"

#ifdef CPU_STAT_PM
static int
workaround_check(UINT port, UINT len) 
{
	UINT off;
	UINT16 map;
	UINT16 mask;

	if (np2cfg.timerfix && CPU_STAT_PM && (CPU_STAT_VM86 || (CPU_STAT_CPL > CPU_STAT_IOPL))) {
		if (CPU_STAT_IOLIMIT == 0) {
			return 1;
		}
		if ((port + len) / 8 >= CPU_STAT_IOLIMIT) {
			return 1;
		}
		off = port / 8;
		mask = ((1 << len) - 1) << (port % 8);
		map = cpu_kmemoryread_w(CPU_STAT_IOADDR + off);
		if (map & mask) {
			return 1;
		}
	}
	return 0;
}
#endif

void bios0x1c(void) {

	UINT8	buf[6];

	switch(CPU_AH) {
		case 0x00:					// get system timer
			calendar_get(buf);
			MEMR_WRITES(CPU_ES, CPU_BX, buf, 6);
			break;

		case 0x01:					// put system timer
			MEMR_READS(CPU_ES, CPU_BX, buf, 6);
			mem[MEMB_MSW8] = buf[0];
			calendar_set(buf);
			break;

		case 0x02:					// set interval timer (single)
			SETBIOSMEM16(0x0001c, CPU_BX);
			SETBIOSMEM16(0x0001e, CPU_ES);
			SETBIOSMEM16(0x0058a, CPU_CX);
			iocore_out8(0x77, 0x36);
			/*FALLTHROUGH*/

		case 0x03:					// continue interval timer
#ifdef CPU_STAT_PM
			if(workaround_check(0x71, 1)) {
				break;
			}
#endif
			iocore_out8(0x71, 0x00);
			if (pccore.cpumode & CPUMODE_8MHZ) {
				iocore_out8(0x71, 0x4e);				// 4MHz
			}
			else {
				iocore_out8(0x71, 0x60);				// 5MHz
			}
			pic.pi[0].imr &= ~(PIC_SYSTEMTIMER);
			break;
	}
}

