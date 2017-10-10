#include	"compiler.h"
#include	"sound.h"
#include	"beep.h"


extern	BEEPCFG		beepcfg;
extern  UINT16 beep_data[BEEPDATACOUNT];
int beep_mode_freq = 56;	// 56:normal 42:TOKIO
int beep_mode_temp = 0;
int beep_mode_bit = 0;
int beep_mode_bit_c = 0;

static void oneshot(BEEP bp, SINT32 *pcm, UINT count) {

	SINT32		samp;
	double		sampbias = soundcfg.rate / 44100.0;

	while(count--) {
		samp = (double)beep_data[bp->beep_data_curr_loc] / 0x100 * (0x1000 * beepcfg.vol) - (0x800 * beepcfg.vol);
		pcm[0] += samp;
		pcm[1] += samp;
		pcm += 2;
		bp->beep_cnt += 20 * (1.0 / sampbias);
		if((bp->beep_data_curr_loc < bp->beep_cnt / beep_mode_freq) && (bp->beep_data_curr_loc != bp->beep_data_load_loc)) {
			bp->beep_data_curr_loc = bp->beep_cnt / beep_mode_freq;
			if(bp->beep_data_curr_loc >= BEEPDATACOUNT) {
				bp->beep_data_curr_loc = 0;
				bp->beep_cnt %= beep_mode_freq;
			}
		}
	}
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

