#include	"compiler.h"
#include	"cpucore.h"
#include	"sound.h"
#include	"beep.h"


extern	BEEPCFG		beepcfg;

static void oneshot(BEEP bp, SINT32 *pcm, UINT count) {

	SINT32		samp;
	UINT32		firsttime = beep_time[bp->beep_data_curr_loc];
	UINT32		time = firsttime;
	UINT32		bound;

	if(bp->beep_data_load_loc != 0)
		bound = (beep_time[bp->beep_data_load_loc - 1] - firsttime) / count;
	else
		bound = (beep_time[BEEPDATACOUNT - 1] - firsttime) / count;

	while(count--) {
		while(time >= beep_time[bp->beep_data_curr_loc] && bp->beep_data_curr_loc != bp->beep_data_load_loc) {
			bp->beep_data_curr_loc++;
			if(bp->beep_data_curr_loc >= BEEPDATACOUNT)
				bp->beep_data_curr_loc = 0;
		}
		if(bp->beep_data_curr_loc != 0)
			samp = beep_data[bp->beep_data_curr_loc - 1];
		else
			samp = beep_data[BEEPDATACOUNT - 1];
		samp = (SINT32)((double)samp / 0x100 * (0x3000 * beepcfg.vol) - (0x1500 * beepcfg.vol));
		pcm[0] += samp;
		pcm[1] += samp;
		pcm += 2;
		time += bound;
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
			r = MIN(r, count);
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
					if(samp > 32767) samp = 0; // XXX: 処理落ち時のノイズ回避 np21w ver0.86 rev42
					if(samp < -32768) samp = 0; // XXX: 処理落ち時のノイズ回避 np21w ver0.86 rev42
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
			if(samp > 32767) samp = 0; // XXX: 処理落ち時のノイズ回避 np21w ver0.86 rev42
			if(samp < -32768) samp = 0; // XXX: 処理落ち時のノイズ回避 np21w ver0.86 rev42
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

