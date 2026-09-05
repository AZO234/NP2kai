/**
 * @file	ymzadpcmg.c
 * @brief	Implementation of the YMZ ADPCM
 */

#include "compiler.h"
#include "pccore.h"
#include <io/pic.h>
#include "ymzadpcm.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#define	ADPCM_NBR	0x80000000

#if 0
static void trace_fmt_ex2(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT2(s)	trace_fmt_ex2 s
#else
#define	TRACEOUT2(s)	(void)s
#endif	/* 1 */


static const UINT adpcmdeltatable[8] = {
		//	0.89,	0.89,	0.89,	0.89,	1.2,	1.6,	2.0,	2.4
			230,	230,	230,	230,	307,	409,	512,	614}; // OPNA�Ə����Ⴄ

static SINT32 SOUNDCALL ymzadpcm_decode_ymz_nibble(ADPCM ad, UINT data)
{
	UINT code;
	UINT delta_index;
	SINT32 diff;
	SINT32 samp;
	SINT32 next_delta;

	code = data & 0x0f;
	delta_index = code & 7;

	/*
	 * YMZ280B�n:
	 *   diff = step * ((delta * 2) + 1) / 8
	 *   sign bit �������Ă���Ό��Z�A�Ȃ���Ή��Z
	 */
	diff = (ad->delta * ((delta_index * 2) + 1)) >> 3;

	if (code & 8) {
		samp = ad->samp - diff;
		if (samp < -32768) {
			samp = -32768;
		}
	}
	else {
		samp = ad->samp + diff;
		if (samp > 32767) {
			samp = 32767;
		}
	}

	/*
	 * step �X�V:
	 *   step = step * table[delta] / 256
	 *   clamp 0x7f .. 0x6000
	 */
	next_delta = (ad->delta * adpcmdeltatable[delta_index]) >> 8;

	if (next_delta < 0x7f) {
		next_delta = 0x7f;
	}
	else if (next_delta > 0x6000) {
		next_delta = 0x6000;
	}

	ad->samp = samp;
	ad->delta = next_delta;

	return samp;
}

int mmo_irqflag = 0;
int mmo_playing = 0;
int mmo_stoppending = 0;
static UINT cpu_stream_bytes_since_irq = 0;

static int SOUNDCALL ymzadpcm_cpugetnibble(ADPCM ad, UINT* data) {

	if (!ad->cpufifolow) {
		if (ad->cfifo.cpufifocount == 0) {
			return(0);
		}

		ad->cfifo.cpufifocur = ad->cfifo.cpufifo[ad->cfifo.cpufiford & ADPCM_CPUFIFO_MASK];
		ad->cfifo.cpufiford++;
		ad->cfifo.cpufifocount--;

#if ADPCM_YMZ_LOW_NIBBLE_FIRST
		* data = ad->cfifo.cpufifocur & 0x0f;
#else
		* data = ad->cfifo.cpufifocur >> 4;
#endif

		ad->cpufifolow = 1;
	}
	else {
#if ADPCM_YMZ_LOW_NIBBLE_FIRST
		* data = ad->cfifo.cpufifocur >> 4;
#else
		* data = ad->cfifo.cpufifocur & 0x0f;
#endif
		ad->cpufifolow = 0;
	}

	return(1);
}

REG8 SOUNDCALL ymzadpcm_readsample(ADPCM ad) {

	UINT32	pos;
	REG8	data;
	REG8	ret;

	if (ad->cpustream) {
		UINT	data32;
		if (!ymzadpcm_cpugetnibble(ad, &data32)) {
			ad->out0 = 0;
			ad->out1 = 0;
			ad->fb = 0;
			return 0;
		}
		data = data32 & 0xff;
	}
	else if (!(ad->reg.ctrl2 & 2)) {
		pos = ad->pos & 0x1fffff;
		if (!(ad->reg.ctrl2 & 2)) {
			data = ad->buf[pos >> 3];
			pos += 8;
		}
		else {
			const UINT8 *ptr;
			REG8 bit;
			UINT tmp;
			ptr = ad->buf + ((pos >> 3) & 0x7fff);
			bit = 1 << (pos & 7);
			tmp = (ptr[0x00000] & bit);
			tmp += (ptr[0x08000] & bit) << 1;
			tmp += (ptr[0x10000] & bit) << 2;
			tmp += (ptr[0x18000] & bit) << 3;
			tmp += (ptr[0x20000] & bit) << 4;
			tmp += (ptr[0x28000] & bit) << 5;
			tmp += (ptr[0x30000] & bit) << 6;
			tmp += (ptr[0x38000] & bit) << 7;
			data = (REG8)(tmp >> (pos & 7));
			pos++;
		}
		if (pos != ad->stop) {
			pos &= 0x1fffff;
			ad->status |= 4;
		}
		if (pos >= ad->limit) {
			pos = 0;
		}
		ad->pos = pos;
	}
	else {
		data = 0;
	}
	pos = ad->fifopos;
	ret = ad->fifo[ad->fifopos];
	ad->fifo[ad->fifopos] = data;
	ad->fifopos ^= 1;
	return(ret);
}

void SOUNDCALL ymzadpcm_datawrite(ADPCM ad, REG8 data) {

	UINT32	pos;

	pos = ad->pos & 0x1fffff;
	if (!(ad->reg.ctrl2 & 2)) {
		ad->buf[pos >> 3] = data;
		pos += 8;
	}
	else {
		UINT8 *ptr;
		UINT8 bit;
		UINT8 mask;
		ptr = ad->buf + ((pos >> 3) & 0x7fff);
		bit = 1 << (pos & 7);
		mask = ~bit;
		ptr[0x00000] &= mask;
		if (data & 0x01) {
			ptr[0x00000] |= bit;
		}
		ptr[0x08000] &= mask;
		if (data & 0x02) {
			ptr[0x08000] |= bit;
		}
		ptr[0x10000] &= mask;
		if (data & 0x04) {
			ptr[0x10000] |= bit;
		}
		ptr[0x18000] &= mask;
		if (data & 0x08) {
			ptr[0x18000] |= bit;
		}
		ptr[0x20000] &= mask;
		if (data & 0x10) {
			ptr[0x20000] |= bit;
		}
		ptr[0x28000] &= mask;
		if (data & 0x20) {
			ptr[0x28000] |= bit;
		}
		ptr[0x30000] &= mask;
		if (data & 0x40) {
			ptr[0x30000] |= bit;
		}
		ptr[0x38000] &= mask;
		if (data & 0x80) {
			ptr[0x38000] |= bit;
		}
		pos++;
	}
	if (pos == ad->stop) {
		pos &= 0x1fffff;
		ad->status |= 4;
	}
	if (pos >= ad->limit) {
		pos = 0;
	}
	ad->pos = pos;
}

static void SOUNDCALL getadpcmdata(ADPCM ad) {

	UINT32	pos;
	UINT	data;
	SINT32	samp;

	pos = ad->pos;
	if (ad->cpustream) {
		if (!ymzadpcm_cpugetnibble(ad, &data)) {
			ad->out0 = 0;
			ad->out1 = 0;
			ad->fb = 0;
			return;
		}
	}
	else if (!(ad->reg.ctrl2 & 2)) {
		data = ad->buf[(pos >> 3) & 0x3ffff];
		if (!(pos & ADPCM_NBR)) {
			data >>= 4;
		}
		pos += ADPCM_NBR + 4;
	}
	else {
		const UINT8 *ptr;
		REG8 bit;
		UINT tmp;
		ptr = ad->buf + ((pos >> 3) & 0x7fff);
		bit = 1 << (pos & 7);
		if (!(pos & ADPCM_NBR)) {
			tmp = (ptr[0x20000] & bit);
			tmp += (ptr[0x28000] & bit) << 1;
			tmp += (ptr[0x30000] & bit) << 2;
			tmp += (ptr[0x38000] & bit) << 3;
			data = tmp >> (pos & 7);
			pos += ADPCM_NBR;
		}
		else {
			tmp = (ptr[0x00000] & bit);
			tmp += (ptr[0x08000] & bit) << 1;
			tmp += (ptr[0x10000] & bit) << 2;
			tmp += (ptr[0x18000] & bit) << 3;
			data = tmp >> (pos & 7);
			pos += ADPCM_NBR + 1;
		}
	}
	samp = ymzadpcm_decode_ymz_nibble(ad, data);

	if (ad->cpustream) {
		samp *= ad->level;
		samp >>= 12;

		ad->out0 = ad->out1;
		ad->out1 = samp;
		ad->fb = 0;
	}
	else {
		if (!(pos & ADPCM_NBR)) {
			if (pos == ad->stop) {
				if (ad->reg.ctrl1 & 0x10) {
					pos = ad->start;
					ad->samp = 0;
					ad->delta = 127;
				}
				else {
					pos &= 0x1fffff;
					ad->status |= 4;
					ad->play = 0;
				}
			}
			else if (pos >= ad->limit) {
				pos = 0;
			}
		}
		ad->pos = pos;
		samp *= ad->level;
		samp >>= (10 + 1);
		ad->out0 = ad->out1;
		ad->out1 = samp + ad->fb;
		ad->fb = samp >> 1;
	}
}

void SOUNDCALL ymzadpcm_getpcm(ADPCM ad, SINT32 *pcm, UINT count) {

	SINT32	remain;
	SINT32	samp;

	if ((count == 0) || (ad->play == 0)) {
		return;
	}
	remain = ad->remain;
	if (ad->step <= ADTIMING) {
		do {
			if (remain < 0) {
				remain += ADTIMING;
				getadpcmdata(ad);
				if (ad->play == 0) {
					if (remain > 0) {
						do {
							samp = (ad->out0 * remain) >> ADTIMING_BIT;
							if (ad->reg.ctrl2 & 0x80) {
								pcm[0] += samp;
							}
							if (ad->reg.ctrl2 & 0x40) {
								pcm[1] += samp;
							}
							pcm += 2;
							remain -= ad->step;
						} while((remain > 0) && (--count));
					}
					goto adpcmstop;
				}
			}
			samp = (ad->out0 * remain) + (ad->out1 * (ADTIMING - remain));
			samp >>= ADTIMING_BIT;
			if (ad->reg.ctrl2 & 0x80) {
				pcm[0] += samp;
			}
			if (ad->reg.ctrl2 & 0x40) {
				pcm[1] += samp;
			}
			pcm += 2;
			remain -= ad->step;
		} while(--count);
	}
	else {
		do {
			if (remain > 0) {
				samp = ad->out0 * (ADTIMING - remain);
				do {
					getadpcmdata(ad);
					if (ad->play == 0) {
						goto adpcmstop;
					}
					samp += ad->out0 * min(remain, ad->pertim);
					remain -= ad->pertim;
				} while(remain > 0);
			}
			else {
				samp = ad->out0 * ADTIMING;
			}
			remain += ADTIMING;
			samp >>= ADTIMING_BIT;
			if (ad->reg.ctrl2 & 0x80) {
				pcm[0] += samp;
			}
			if (ad->reg.ctrl2 & 0x40) {
				pcm[1] += samp;
			}
			pcm += 2;
		} while(--count);
	}
	ad->remain = remain;
	return;

adpcmstop:
	ad->out0 = 0;
	ad->out1 = 0;
	ad->fb = 0;
	ad->remain = 0;
}

/**
 * Step adpcm
 * @param[in] ad An instance of ADPCM
 * @param[out] pcm A pointer to a buffer
 * @param[in] count The size of the buffer
 */
void SOUNDCALL ymzadpcm_getpcm_dummy(ADPCM ad, SINT32 *pcm, UINT count)
{
	SINT32	remain;

	if ((count == 0) || (ad->play == 0))
	{
		return;
	}
	remain = ad->remain;
	if (ad->step <= ADTIMING)
	{
		do
		{
			if (remain < 0)
			{
				remain += ADTIMING;
				getadpcmdata(ad);
				if (ad->play == 0)
				{
					if (remain > 0)
					{
						do
						{
							remain -= ad->step;
						} while ((remain > 0) && (--count));
					}
					goto adpcmstop;
				}
			}
			remain -= ad->step;
		} while(--count);
	}
	else
	{
		do
		{
			if (remain > 0)
			{
				do {
					getadpcmdata(ad);
					if (ad->play == 0)
					{
						goto adpcmstop;
					}
					remain -= ad->pertim;
				} while (remain > 0);
			}
			remain += ADTIMING;
		} while (--count);
	}
	ad->remain = remain;
	return;

adpcmstop:
	ad->out0 = 0;
	ad->out1 = 0;
	ad->fb = 0;
	ad->remain = 0;
}
