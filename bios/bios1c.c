#include	"compiler.h"
#include	"parts.h"
#include	"timemng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"calendar.h"
#include	"bios.h"
#include	"biosmem.h"

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
#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
			if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
				biosioemu_enq8(0x77, 0x36); 
			} else
#endif
			{
				iocore_out8(0x77, 0x36);
			}
			/*FALLTHROUGH*/

		case 0x03:					// continue interval timer
#if defined(BIOS_IO_EMULATION) && defined(CPUCORE_IA32)
			if (CPU_STAT_PM && CPU_STAT_VM86 && biosioemu.enable) {
				biosioemu_enq8(0x71, 0x00);
				if (pccore.cpumode & CPUMODE_8MHZ) {
					biosioemu_enq8(0x71, 0x4e);				// 4MHz
				}
				else {
					biosioemu_enq8(0x71, 0x60);				// 5MHz
				}
			} else
#endif
			{
				iocore_out8(0x71, 0x00);
				if (pccore.cpumode & CPUMODE_8MHZ) {
					iocore_out8(0x71, 0x4e);				// 4MHz
				}
				else {
					iocore_out8(0x71, 0x60);				// 5MHz
				}
			}
			pic.pi[0].imr &= ~(PIC_SYSTEMTIMER);
			break;
			
#if defined(SUPPORT_HRTIMER)
		case 0x80:					// hrtimer read
			CPU_AL = ((mem[0x04F3] >> 6) - 1) & 0x3; // Œo‰ß“ú”
			{
				UINT32 hrtimertimeuint;
				hrtimertimeuint = LOADINTELDWORD(mem+0x04F1) & 0x3fffff;
				CPU_CX = (UINT16)((hrtimertimeuint >> 16) & 0x3f);
				CPU_DX = (UINT16)(hrtimertimeuint & 0xffff);
			}
			break;

		case 0x81:					// hrtimer write
			mem[0x04F3] = (CPU_CX & 0x3f) | ((CPU_AL & 0x3) << 6);
			STOREINTELWORD(mem+0x04F1, CPU_DX);
			break;
#endif
	}
}

