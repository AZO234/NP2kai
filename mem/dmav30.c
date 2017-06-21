#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"dmav30.h"


void dmav30(void) {

	DMACH	ch;
	REG8	bit;

	if (dmac.working) {
		ch = dmac.dmach;
		bit = 1;
		do {
			if (dmac.working & bit) {
				// DMA working !
				if (!ch->leng.w) {
					dmac.stat |= bit;
					dmac.working &= ~bit;
					ch->proc.extproc(DMAEXT_END);
				}
				ch->leng.w--;

				switch(ch->mode & 0x0c) {
					case 0x00:		// verifty
						ch->proc.inproc();
						break;

					case 0x04:		// port->mem
						MEMP_WRITE8(ch->adrs.d, ch->proc.inproc());
						break;

					default:
						ch->proc.outproc(MEMP_READ8(ch->adrs.d));
						break;
				}
				ch->adrs.w[DMA16_LOW] += ((ch->mode & 0x20)?-1:1);
			}
			ch++;
			bit <<= 1;
		} while(bit & 0x0f);
	}
}

