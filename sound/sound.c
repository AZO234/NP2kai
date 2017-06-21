/**
 * @file	sound.c
 * @brief	Implementation of the sound
 */

#include "compiler.h"
#include "sound.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "sndcsec.h"
#include "beep.h"
#include "soundmng.h"
#if defined(SUPPORT_WAVEREC)
#include "common/wavefile.h"
#endif	/* defined(SUPPORT_WAVEREC) */

	SOUNDCFG	soundcfg;


#define	STREAM_CBMAX	16

typedef struct {
	void	*hdl;
	SOUNDCB	cbfn;
} CBTBL;

typedef struct {
	SINT32	*buffer;
	SINT32	*ptr;
	UINT	samples;
	UINT	reserve;
	UINT	remain;
#if defined(SUPPORT_WAVEREC)
	WAVEFILEH rec;
#endif	/* defined(SUPPORT_WAVEREC) */
	CBTBL	*cbreg;
	CBTBL	cb[STREAM_CBMAX];
} SNDSTREAM;

static	SNDSTREAM	sndstream;


static void streamreset(void) {

	SNDCSEC_ENTER;
	sndstream.ptr = sndstream.buffer;
	sndstream.remain = sndstream.samples + sndstream.reserve;
	sndstream.cbreg = sndstream.cb;
	SNDCSEC_LEAVE;
}

static void streamprepare(UINT samples) {

	CBTBL	*cb;
	UINT	count;

	count = min(sndstream.remain, samples);
	if (count) {
		ZeroMemory(sndstream.ptr, count * 2 * sizeof(SINT32));
		cb = sndstream.cb;
		while(cb < sndstream.cbreg) {
			cb->cbfn(cb->hdl, sndstream.ptr, count);
			cb++;
		}
		sndstream.ptr += count * 2;
		sndstream.remain -= count;
	}
}


#if defined(SUPPORT_WAVEREC)
// ---- wave rec

/**
 * Starts recording
 * @param[in] lpFilename The filename
 * @retval SUCCESS If succeeded
 * @retval FAILURE If failed
 */
BRESULT sound_recstart(const OEMCHAR *lpFilename)
{
	WAVEFILEH rec;

	sound_recstop();
	if (sndstream.buffer == NULL)
	{
		return FAILURE;
	}
	rec = wavefile_create(lpFilename, soundcfg.rate, 16, 2);
	sndstream.rec = rec;
	if (rec)
	{
		return SUCCESS;
	}
	return FAILURE;
}

/**
 * Stops recording
 */
void sound_recstop(void)
{
	WAVEFILEH rec;

	rec = sndstream.rec;
	sndstream.rec = NULL;
	wavefile_close(rec);
}

/**
 * is recording?
 * @retval TRUE Yes
 */
BOOL sound_isrecording(void)
{
	return (sndstream.rec != NULL) ? TRUE : FALSE;
}

/**
 * write
 * @param[in] samples The count of samples
 */
static void streamfilewrite(UINT nSamples)
{
	UINT nCount;
	SINT32 buf32[2 * 512];
	CBTBL *cb;
	UINT8 buf[2 * 512][2];
	UINT r;
	UINT i;
	SINT32 nSample;

	while (nSamples)
	{
		nCount = min(nSamples, 512);
		memset(buf32, 0, nCount * 2 * sizeof(buf32[0]));
		cb = sndstream.cb;
		while (cb < sndstream.cbreg)
		{
			cb->cbfn(cb->hdl, buf32, nCount);
			cb++;
		}
		r = min(sndstream.remain, nCount);
		if (r)
		{
			memcpy(sndstream.ptr, buf32, r * 2 * sizeof(buf32[0]));
			sndstream.ptr += r * 2;
			sndstream.remain -= r;
		}
		for (i = 0; i < nCount * 2; i++)
		{
			nSample = buf32[i];
			if (nSample > 32767)
			{
				nSample = 32767;
			}
			else if (nSample < -32768)
			{
				nSample = -32768;
			}
			// little endianなので satuation_s16は使えない
			buf[i][0] = (UINT8)nSample;
			buf[i][1] = (UINT8)(nSample >> 8);
		}
		wavefile_write(sndstream.rec, buf, nCount * 2 * sizeof(buf[0]));
		nSamples -= nCount;
	}
}

/**
 * fill
 * @param[in] samples The count of samples
 */
static void filltailsample(UINT nCount)
{
	SINT32 *ptr;
	UINT nOrgSize;
	SINT32 nSampleL;
	SINT32 nSampleR;

	nCount = min(sndstream.remain, nCount);
	if (nCount)
	{
		ptr = sndstream.ptr;
		nOrgSize = (UINT)((ptr - sndstream.buffer) / 2);
		if (nOrgSize == 0)
		{
			nSampleL = 0;
			nSampleR = 0;
		}
		else
		{
			nSampleL = *(ptr - 2);
			nSampleR = *(ptr - 1);
		}
		sndstream.ptr += nCount * 2;
		sndstream.remain -= nCount;
		do
		{
			ptr[0] = nSampleL;
			ptr[1] = nSampleR;
			ptr += 2;
		} while (--nCount);
	}
}
#endif	/* defined(SUPPORT_WAVEREC) */


// ----

BRESULT sound_create(UINT rate, UINT ms) {

	UINT	samples;
	UINT	reserve;

	ZeroMemory(&sndstream, sizeof(sndstream));
	switch(rate) {
		case 11025:
		case 22050:
		case 44100:
		case 48000:
		case 88200:
		case 96000:
		case 176400:
		case 192000:
			break;

		default:
			return(FAILURE);
	}
	samples = soundmng_create(rate, ms);
	if (samples == 0) {
		goto scre_err1;
	}
	soundmng_reset();

	soundcfg.rate = rate;
	sound_changeclock();

#if defined(SOUNDRESERVE)
	reserve = rate * SOUNDRESERVE / 1000;
#else
	reserve = 0;
#endif
	sndstream.buffer = (SINT32 *)_MALLOC((samples + reserve) * 2 
												* sizeof(SINT32), "stream");
	if (sndstream.buffer == NULL) {
		goto scre_err2;
	}
	sndstream.samples = samples;
	sndstream.reserve = reserve;

	SNDCSEC_INIT;
	streamreset();
	return(SUCCESS);

scre_err2:
	soundmng_destroy();

scre_err1:
	return(FAILURE);
}

void sound_destroy(void) {

	if (sndstream.buffer) {
#if defined(SUPPORT_WAVEREC)
		sound_recstop();
#endif	/* defined(SUPPORT_WAVEREC) */
		soundmng_stop();
		streamreset();
		soundmng_destroy();
		SNDCSEC_TERM;
		_MFREE(sndstream.buffer);
		sndstream.buffer = NULL;
	}
}

void sound_reset(void) {

	if (sndstream.buffer) {
		soundmng_reset();
		streamreset();
		soundcfg.lastclock = CPU_CLOCK;
		beep_eventreset();
	}
}

void sound_changeclock(void) {

	UINT32	clk;
	UINT	hz;
	UINT	hzmax;

	if (sndstream.buffer == NULL) {
		return;
	}

	// とりあえず 25で割り切れる。
	clk = pccore.realclock / 25;
	hz = soundcfg.rate / 25;

	// で、クロック数に合せて調整。(64bit演算しろよな的)
	hzmax = (1 << (32 - 8)) / (clk >> 8);
	while(hzmax < hz) {
		clk = (clk + 1) >> 1;
		hz = (hz + 1) >> 1;
	}
	TRACEOUT(("hzbase/clockbase = %d/%d", hz, clk));
	soundcfg.hzbase = hz;
	soundcfg.clockbase = clk;
	soundcfg.minclock = 2 * clk / hz;
	soundcfg.lastclock = CPU_CLOCK;
}

void sound_streamregist(void *hdl, SOUNDCB cbfn) {

	if (sndstream.buffer) {
		if ((cbfn) &&
			(sndstream.cbreg < (sndstream.cb + STREAM_CBMAX))) {
			sndstream.cbreg->hdl = hdl;
			sndstream.cbreg->cbfn = cbfn;
			sndstream.cbreg++;
		}
	}
}


// ----

void sound_sync(void) {

	UINT32	length;

	if (sndstream.buffer == NULL) {
		return;
	}

	length = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK - soundcfg.lastclock;
	if (length < soundcfg.minclock) {
		return;
	}
	length = (length * soundcfg.hzbase) / soundcfg.clockbase;
	if (length == 0) {
		return;
	}
	SNDCSEC_ENTER;
#if defined(SUPPORT_WAVEREC)
	if (sndstream.rec) {
		streamfilewrite(length);
	}
	else
#endif	/* defined(SUPPORT_WAVEREC) */
		streamprepare(length);
	soundcfg.lastclock += length * soundcfg.clockbase / soundcfg.hzbase;
	beep_eventreset();
	SNDCSEC_LEAVE;

	soundcfg.writecount += length;
	if (soundcfg.writecount >= 100) {
		soundcfg.writecount = 0;
		soundmng_sync();
	}
}

static volatile int locks = 0;

const SINT32 *sound_pcmlock(void) {

const SINT32 *ret;

	if (locks) {
		TRACEOUT(("sound pcm lock: already locked"));
		return(NULL);
	}
	locks++;
	ret = sndstream.buffer;
	if (ret) {
		SNDCSEC_ENTER;
		if (sndstream.remain > sndstream.reserve)
#if defined(SUPPORT_WAVEREC)
			if (sndstream.rec) {
				filltailsample(sndstream.remain - sndstream.reserve);
			}
			else
#endif	/* defined(SUPPORT_WAVEREC) */
		{
			streamprepare(sndstream.remain - sndstream.reserve);
			soundcfg.lastclock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
			beep_eventreset();
		}
	}
	else {
		locks--;
	}
	return(ret);
}

void sound_pcmunlock(const SINT32 *hdl) {

	int		leng;

	if (hdl) {
		leng = sndstream.reserve - sndstream.remain;
		if (leng > 0) {
			CopyMemory(sndstream.buffer,
						sndstream.buffer + (sndstream.samples * 2),
												leng * 2 * sizeof(SINT32));
		}
		sndstream.ptr = sndstream.buffer + (leng * 2);
		sndstream.remain = sndstream.samples + sndstream.reserve - leng;
//		sndstream.remain += sndstream.samples;
		SNDCSEC_LEAVE;
		locks--;
	}
}
