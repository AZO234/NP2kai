#include	<compiler.h>
#include	<pccore.h>
#include	<fdd/diskdrv.h>
#include	<fdd/fdd_mtr.h>
#include	<timing.h>


typedef struct {
	UINT32	tick;
	UINT32	msstep;
	UINT	cnt;
	UINT32	fraction;
} TIMING;

static	TIMING	timing;

static	UINT32 timimg_speed = 100;

void timing_reset(void) {

	timing.tick = GETTICK();
	timing.cnt = 0;
	timing.fraction = 0;
}

void timing_setrate(UINT lines, UINT crthz) {

	timing.msstep = (crthz << (TIMING_MSSHIFT - 3)) / lines / (1000 >> 3);
}

void timing_setcount(UINT value) {

	timing.cnt = value;
}

void timing_setspeed(UINT32 value)
{
	timimg_speed = value;
}

UINT32 timing_getmsstep(void)
{
	return(timing.msstep);
}


UINT timing_getcount(void) {

	UINT32	ticknow;
	UINT32	span;
	UINT32	fraction;

	ticknow = GETTICK();
	span = ticknow - timing.tick;
	span = span * timimg_speed / 128;
	if (span) {
		timing.tick = ticknow;
		fddmtr_callback(ticknow);

		if (span >= 1000) {
			span = 1000;
		}
		fraction = timing.fraction + (span * timing.msstep);
		timing.cnt += fraction >> TIMING_MSSHIFT;
		timing.fraction = fraction & ((1 << TIMING_MSSHIFT) - 1);
	}
	return(timing.cnt);
}

UINT timing_getcount_baseclock(void) {

	UINT32	ticknow;
	UINT32	span;
	UINT32	fraction;
	UINT32	ret = 0;

	ticknow = GETTICK();
	span = ticknow - timing.tick;
	span = span * timimg_speed / 128;
	if (span) {
		if (span >= 1000) {
			span = 1000;
		}
		fraction = timing.fraction + (span * timing.msstep);
		ret = timing.cnt + (fraction >> TIMING_MSSHIFT);
	}
	return(ret);
}

UINT32 timing_getcount_raw(void) {

	UINT32	ticknow;
	UINT32	span;
	UINT32	fraction;

	ticknow = GETTICK();
	span = ticknow - timing.tick;
	span = span * timimg_speed / 128;
	if (span >= 1000) {
		span = 1000;
	}
	fraction = timing.fraction + (span * timing.msstep);
	return((timing.cnt << TIMING_MSSHIFT) + fraction);
}