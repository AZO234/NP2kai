#include	"compiler.h"
#include	"pccore.h"
#include	"fdd/diskdrv.h"
#include	"fdd/fdd_mtr.h"
#include	"timing.h"


#define	MSSHIFT		16

typedef struct {
	UINT32	tick;
	UINT32	msstep;
	UINT	cnt;
	UINT32	fraction;
} TIMING;

static	TIMING	timing;


void timing_reset(void) {

	timing.tick = GETTICK();
	timing.cnt = 0;
	timing.fraction = 0;
}

void timing_setrate(UINT lines, UINT crthz) {

	timing.msstep = (crthz << (MSSHIFT - 3)) / lines / (1000 >> 3);
}

void timing_setcount(UINT value) {

	timing.cnt = value;
}

UINT timing_getcount(void) {

	UINT32	ticknow;
	UINT32	span;
	UINT32	fraction;

	ticknow = GETTICK();
	span = ticknow - timing.tick;
	if (span) {
		timing.tick = ticknow;
		fddmtr_callback(ticknow);

		if (span >= 1000) {
			span = 1000;
		}
		fraction = timing.fraction + (span * timing.msstep);
		timing.cnt += fraction >> MSSHIFT;
		timing.fraction = fraction & ((1 << MSSHIFT) - 1);
	}
	return(timing.cnt);
}

