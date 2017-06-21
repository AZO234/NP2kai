#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"biosmem.h"


#define	baseport 0x00c8

void bios0x12(void) {

	UINT8	status;
	UINT8	result;
	UINT8	*p;
	UINT8	drv;
	UINT8	drvbit;

//	TRACE_("BIOS", 0x12);
	iocore_out8(0x08, 0x20);
	if (!pic.pi[1].isr) {
		iocore_out8(0x00, 0x20);
	}

	/* @TODO: 本来はセンスするまでベクタをセットしてはいけない (ココに来てはならない) */
	if (((baseport >> 4) ^ fdc.chgreg) & 1)
	{
		return;
	}

	status = iocore_inp8(baseport);
	while(1) {
		if (!(status & FDCSTAT_CB)) {
			if ((status & (FDCSTAT_RQM | FDCSTAT_DIO)) != FDCSTAT_RQM) {
				break;
			}
			iocore_out8(baseport+2, 0x08);
			status = iocore_inp8(baseport);
		}
		if ((status & (FDCSTAT_RQM | FDCSTAT_DIO | FDCSTAT_CB))
							!= (FDCSTAT_RQM | FDCSTAT_DIO | FDCSTAT_CB)) {
			break;
		}
		result = iocore_inp8(baseport+2);
		if (result == FDCRLT_IC1) {
			if (mem[0x005d7]) {
				mem[0x005d7]--;
			}
			break;
		}
		drv = result & 3;
		drvbit = 0x10 << drv;
		if (result & (FDCRLT_IC1 | FDCRLT_SE)) {
			p = mem + 0x005d8 + (drv * 2);
		}
		else {
			p = mem + 0x005d0;
		}
		while(1) {
			*p++ = result;
			status = iocore_inp8(baseport);
			if ((status & (FDCSTAT_RQM | FDCSTAT_DIO | FDCSTAT_CB))
							!= (FDCSTAT_RQM | FDCSTAT_DIO | FDCSTAT_CB)) {
				break;
			}
			result = iocore_inp8(baseport+2);
		}
		mem[MEMB_DISK_INTH] |= drvbit;
	}
}

