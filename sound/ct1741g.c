/**
 * @file	ct1741g.c
 * @brief	Implementation of the Creative SoundBlaster16 CT1741 DSP
 */

#ifdef SUPPORT_SOUND_SB16

#include <compiler.h>
#include "ct1741.h"
#include <cbus/ct1745io.h>
#include <io/iocore.h>
#include <sound/fmboard.h>
#include <math.h>

#define CT1741_WAVE_VOL_SHIFT	13

typedef void (SOUNDCALL* CT1741FN)(DMA_INFO* ct, SINT32* pcm, UINT count);

 // 各再生モードのアライメント
int CT1741_BUF_ALIGN[] = {
	1,		// 0: STOP
	1,
	1,
	1,		// 8Bit Unsigned PCM mono
	1,		// 8Bit Signed PCM mono
	2,		//16Bit Signed PCM mono
	1,
	1,

	1,
	1,
	1,
	2,		// 8Bit Unsigned PCM stereo
	2,		// 8Bit UnSigned PCM stereo
	4,		//16Bit signed PCM stereo
	1,
	1
};

// 再生用情報
CT1741_PLAYINFO ct1741_playinfo = { 0 };

// エミュレータ側の再生サンプリングレート
int ct1741_rate = 44100;

// サンプリングレート変換用
static double sample_rem = 0;

// PIO 8bit モノラル
static void SOUNDCALL pcm8mPIO(DMA_INFO* ct, SINT32* pcm, UINT count) {
	UINT32	leng;
	UINT32	samppos = 0;
	UINT8* ptr1;
	UINT8* ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src = g_sb16.dsp_info.freq;
	double sample_srclenf = sample_rem + (double)count * samplen_src / samplen_dst;
	int sample_srclen = (int)sample_srclenf;
	sample_rem = sample_srclenf - sample_srclen;

	leng = ct1741_playinfo.pio.bufdatas;
	if (!leng) {
		return;
	}

	for (i = 0; i < count; i++) {
		samppos = (i * samplen_src / samplen_dst);
		if (samppos >= leng) {
			break;
		}
		ptr1 = ct1741_playinfo.pio.buffer + ((ct1741_playinfo.pio.bufpos + samppos + 0) & CT1741_PIO_BUFMASK);
		ptr2 = ct1741_playinfo.pio.buffer + ((ct1741_playinfo.pio.bufpos + samppos + 0) & CT1741_PIO_BUFMASK);
		samp1 = ((SINT32)ptr1[0] - 0x80) << 8;
		samp2 = ((SINT32)ptr2[0] - 0x80) << 8;
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_LEFT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_LEFT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm[1] += (samp1 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_RIGHT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm += 2;
	}

	leng = MIN(leng, sample_srclen);
	ct1741_playinfo.pio.bufdatas -= leng;
	ct1741_playinfo.pio.bufpos = (ct1741_playinfo.pio.bufpos + leng) & CT1741_PIO_BUFMASK;
	if (ct1741_playinfo.pio.bufdatas < CT1741_PIO_BUFSIZE / 2) {
		g_sb16.dsp_info.wbusy = 0;
	}
	if (ct1741_playinfo.bufdatasrem < leng) {
		ct1741_playinfo.bufdatasrem = 0;
	}
	else {
		ct1741_playinfo.bufdatasrem -= leng;
	}
}

// 8bit モノラル
static void SOUNDCALL pcm8m(DMA_INFO* ct, SINT32* pcm, UINT count) {
	UINT32	leng;
	UINT32	samppos = 0;
	UINT8* ptr1;
	UINT8* ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src = g_sb16.dsp_info.freq;
	double sample_srclenf = sample_rem + (double)count * samplen_src / samplen_dst;
	int sample_srclen = (int)sample_srclenf;
	sample_rem = sample_srclenf - sample_srclen;

	leng = ct->bufdatas;
	if (!leng) {
		return;
	}

	for (i = 0; i < count; i++) {
		samppos = (i * samplen_src / samplen_dst);
		if (samppos >= leng) {
			break;
		}
		ptr1 = ct->buffer + ((ct->bufpos + samppos + 0) & CT1741_DMA_BUFMASK);
		ptr2 = ct->buffer + ((ct->bufpos + samppos + 0) & CT1741_DMA_BUFMASK);
		samp1 = ((SINT32)ptr1[0] - 0x80) << 8;
		samp2 = ((SINT32)ptr2[0] - 0x80) << 8;
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_LEFT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_LEFT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm[1] += (samp1 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_RIGHT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm += 2;
	}

	leng = MIN(leng, sample_srclen);
	ct->bufdatas -= leng;
	ct->bufpos = (ct->bufpos + leng) & CT1741_DMA_BUFMASK;
	if (ct1741_playinfo.bufdatasrem < leng) {
		ct1741_playinfo.bufdatasrem = 0;
	}
	else {
		ct1741_playinfo.bufdatasrem -= leng;
	}
}

// 8bit ステレオ
static void SOUNDCALL pcm8s(DMA_INFO* ct, SINT32* pcm, UINT count) {

	UINT32	leng;
	UINT32	samppos = 0;
	UINT8* ptr1;
	UINT8* ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src = g_sb16.dsp_info.freq;
	double sample_srclenf = sample_rem + (double)2 * count * samplen_src / samplen_dst;
	int sample_srclen = ((int)sample_srclenf) & ~0x1;
	sample_rem = sample_srclenf - sample_srclen;

	leng = ct->bufdatas;
	if (!leng) {
		return;
	}

	for (i = 0; i < count; i++) {
		samppos = 2 * (i * samplen_src / samplen_dst);
		if (samppos >= leng) {
			break;
		}
		ptr1 = ct->buffer + ((ct->bufpos + samppos + 0) & CT1741_DMA_BUFMASK);
		ptr2 = ct->buffer + ((ct->bufpos + samppos + 1) & CT1741_DMA_BUFMASK);
		samp1 = ((SINT32)ptr1[0] - 0x80) << 8;
		samp2 = ((SINT32)ptr2[0] - 0x80) << 8;
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_LEFT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_LEFT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm[1] += (samp2 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_RIGHT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm += 2;
	}

	leng = MIN(leng, sample_srclen);
	ct->bufdatas -= leng;
	ct->bufpos = (ct->bufpos + leng) & CT1741_DMA_BUFMASK;
	if (ct1741_playinfo.bufdatasrem < leng) {
		ct1741_playinfo.bufdatasrem = 0;
	}
	else {
		ct1741_playinfo.bufdatasrem -= leng;
	}
}


// 16bit モノラル
static void SOUNDCALL Spcm16m(DMA_INFO* ct, SINT32* pcm, UINT count) {

	UINT32	leng;
	UINT32	samppos = 0;
	UINT8* ptr1;
	UINT8* ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src = g_sb16.dsp_info.freq;
	double sample_srclenf = sample_rem + (double)2 * count * samplen_src / samplen_dst;
	int sample_srclen = ((int)sample_srclenf) & ~0x1;
	sample_rem = sample_srclenf - sample_srclen;

	leng = ct->bufdatas & ~0x1;
	if (!leng) {
		return;
	}

	for (i = 0; i < count; i++) {
		samppos = 2 * (i * samplen_src / samplen_dst);
		if (samppos >= leng) {
			break;
		}
		ptr1 = (ct->buffer + ((ct->bufpos + samppos + 0) & CT1741_DMA_BUFMASK));
		ptr2 = (ct->buffer + ((ct->bufpos + samppos + 0) & CT1741_DMA_BUFMASK));
		samp1 = ((SINT32)((SINT8)ptr1[1]) << 8) + ptr1[0];
		samp2 = ((SINT32)((SINT8)ptr2[1]) << 8) + ptr2[0];
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_LEFT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_LEFT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm[1] += (samp1 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_RIGHT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm += 2;
	}

	leng = MIN(leng, sample_srclen);
	ct->bufdatas -= leng;
	ct->bufpos = (ct->bufpos + leng) & CT1741_DMA_BUFMASK;
	if (ct1741_playinfo.bufdatasrem < leng) {
		ct1741_playinfo.bufdatasrem = 0;
	}
	else {
		ct1741_playinfo.bufdatasrem -= leng;
	}
}

// 16bit ステレオ(little endian)
static void SOUNDCALL Spcm16s(DMA_INFO* ct, SINT32* pcm, UINT count) {

	UINT32	leng;
	UINT32	samppos = 0;
	UINT8* ptr1;
	UINT8* ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src = g_sb16.dsp_info.freq;
	double sample_srclenf = sample_rem + (double)4 * count * samplen_src / samplen_dst;
	int sample_srclen = ((int)sample_srclenf) & ~0x3;
	sample_rem = sample_srclenf - sample_srclen;

	leng = ct->bufdatas & ~0x3;
	if (!leng) {
		return;
	}

	for (i = 0; i < count; i++) {
		samppos = 4 * (i * samplen_src / samplen_dst);
		if (samppos >= leng) {
			break;
		}
		ptr1 = (ct->buffer + ((ct->bufpos + samppos + 0) & CT1741_DMA_BUFMASK));
		ptr2 = (ct->buffer + ((ct->bufpos + samppos + 2) & CT1741_DMA_BUFMASK));
		samp1 = ((SINT32)((SINT8)ptr1[1]) << 8) + ptr1[0];
		samp2 = ((SINT32)((SINT8)ptr2[1]) << 8) + ptr2[0];
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_LEFT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_LEFT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm[1] += (samp2 * ((int)np2cfg.vol_pcm * np2cfg.vol_master / 100) * (SINT32)g_sb16.mixregexp[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_RIGHT]) >> CT1741_WAVE_VOL_SHIFT;
		pcm += 2;
	}

	leng = MIN(leng, sample_srclen);
	ct->bufdatas -= leng;
	ct->bufpos = (ct->bufpos + leng) & CT1741_DMA_BUFMASK;
	if (ct1741_playinfo.bufdatasrem < leng) {
		ct1741_playinfo.bufdatasrem = 0;
	}
	else {
		ct1741_playinfo.bufdatasrem -= leng;
	}
}
static void SOUNDCALL nomake(DMA_INFO* ct, SINT32* pcm, UINT count) {
	(void)ct;
	(void)pcm;
	(void)count;
	ct->bufdatas = 0;
	ct1741_playinfo.bufdatasrem = 0;
	sample_rem = 0;
	if (g_sb16.dsp_info.dma.dmach) g_sb16.dsp_info.dma.dmach->leng.w = 0;
}

static const CT1741FN ct1741fn[16] = {
	nomake,		// 0: STOP
	nomake,
	nomake,
	pcm8m,		// 8Bit Unsigned PCM mono
	pcm8m,		// 8Bit Signed PCM mono
	Spcm16m,	//16Bit Signed PCM mono
	nomake,
	nomake,
	nomake,
	nomake,
	nomake,
	pcm8s,		// 8Bit Unsigned PCM stereo
	pcm8s,		// 8Bit UnSigned PCM stereo
	Spcm16s,	//16Bit signed PCM stereo
	nomake,
	nomake
};

void SOUNDCALL ct1741_getpcm(DMA_INFO* ct, SINT32* pcm, UINT count) {
	static CT1741FN lastplayfn = nomake; // 最後に使用した再生用関数

	// 再生用バッファに送る
	if (ct1741_playinfo.playwaitcounter <= 0) {
		int idx = g_sb16.dsp_info.dma.mode | g_sb16.dsp_info.dma.stereo << 3;
#if defined(SUPPORT_MULTITHREAD)
		ct1741cs_enter_criticalsection();
#endif
		if (g_sb16.dsp_info.mode == CT1741_DSPMODE_DAC)
		{
			(*pcm8mPIO)(ct, pcm, count);
			lastplayfn = pcm8mPIO;
		}
		else
		{
			if (g_sb16.dsp_info.dma.mode == CT1741_DMAMODE_NONE && ct1741_playinfo.bufdatasrem < CT1741_BUF_ALIGN[idx])
			{
				ct->bufdatas = 0; // 全部捨てる
				sample_rem = 0;
			}
			if (idx != 0)
			{
				(*ct1741fn[idx])(ct, pcm, count);
				lastplayfn = ct1741fn[idx];
			}
			else if (ct->bufdatas >= CT1741_BUF_ALIGN[idx])
			{
				(*lastplayfn)(ct, pcm, count);
			}
			else
			{
				(*ct1741fn[idx])(ct, pcm, count);
			}
		}
#if defined(SUPPORT_MULTITHREAD)
		ct1741cs_leave_criticalsection();
#endif
	}
}

#endif