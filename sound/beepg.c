#include	"compiler.h"
#include	"sound.h"
#include	"beep.h"


extern	BEEPCFG		beepcfg;

static void oneshot(BEEP bp, SINT32 *pcm, UINT count) {

	SINT32		vol;
const BPEVENT	*bev;
	SINT32		clk;
	int			event;
	SINT32		remain;
	SINT32		samp;
	SINT32		onevt;

//#define BEEP_44K_CNT 65536
#define BEEP_44K_CNT 61604

	bev = bp->event;
	onevt = bp->lastonevt;
	event = bp->lastenable;
	remain = bp->lastremain;
	clk = bp->lastclk;

	while(count--) {
		if (!onevt) {
			if (bp->events) {
				clk=-1;
				while(clk < 0 && bp->events) { 
					bp->events--;
					clk = bev->clock - remain;
					if(clk < 0)
						remain -= bev->clock;
					event = bev->enable;
					bev++;
					onevt = 1;
				}
			}
		}
		samp = event ? 1 : -1;
		samp *= beepcfg.vol;
		samp <<= 10;
		pcm[0] += samp;
		pcm[1] += samp;
		pcm += 2;
		if (onevt) {
			if(clk <= BEEP_44K_CNT) {
				remain = BEEP_44K_CNT - clk;
				onevt = 0;
			} else {
				clk -= BEEP_44K_CNT;
			}
		} else {
			remain = 0;
		}
	}

	bp->lastonevt = onevt;
	bp->lastremain = remain;
	bp->lastclk = clk;
	bp->lastenable = event;
}

static void rategenerator(BEEP bp, SINT32 *pcm, UINT count) {

	SINT32		vol;
const BPEVENT	*bev;
	SINT32		samp;
	SINT32		remain;
	SINT32		clk;
	int			event;
	UINT		r;

	vol = beepcfg.vol;
	bev = bp->event;
	if (bp->events) {
		bp->events--;
		clk = bev->clock;
		event = bev->enable;
		bev++;
	}
	else {
		clk = 0x40000000;
		event = bp->lastenable;
	}
	do {
		if (clk >= (1 << 16)) {
			r = clk >> 16;
			r = min(r, count);
			clk -= r << 16;
			count -= r;
			if (bp->lastenable) {
				do {
					samp = (bp->cnt & 0x8000)?1:-1;
					bp->cnt += bp->hz;
					samp += (bp->cnt & 0x8000)?1:-1;
					bp->cnt += bp->hz;
					samp += (bp->cnt & 0x8000)?1:-1;
					bp->cnt += bp->hz;
					samp += (bp->cnt & 0x8000)?1:-1;
					bp->cnt += bp->hz;
					samp *= vol;
					samp <<= (10 - 2);
					pcm[0] += samp;
					pcm[1] += samp;
					pcm += 2;
				} while(--r);
			}
			else {
				pcm += 2 * r;
			}
		}
		else {
			remain = (1 << 16);
			samp = 0;
			while(remain >= clk) {
				remain -= clk;
				if (bp->lastenable) {
					samp += clk;
				}
				bp->lastenable = event;
				bp->cnt = 0;
				if (bp->events) {
					bp->events--;
					clk = bev->clock;
					event = bev->enable;
					bev++;
				}
				else {
					clk = 0x40000000;
				}
			}
			clk -= remain;
			if (bp->lastenable) {
				samp += remain;
			}
			samp *= vol;
			samp >>= (16 - 10);
			pcm[0] += samp;
			pcm[1] += samp;
			pcm += 2;
			count--;
		}
	} while(count);
	bp->lastenable = event;
	bp->events = 0;
}

void SOUNDCALL beep_getpcm(BEEP bp, SINT32 *pcm, UINT count) {

	if ((count) && (beepcfg.vol)) {
		if (bp->mode == 0) {
			if (bp->events) {
				oneshot(bp, pcm, count);
			}
		}
		else if (bp->mode == 1) {
			rategenerator(bp, pcm, count);
		}
	}
}

