/**
 * @file	cs4231g.c
 * @brief	Implementation of the CS4231
 */

#include "compiler.h"
#include "cs4231.h"
#include "iocore.h"
#include "sound/fmboard.h"
#include <math.h>

extern	CS4231CFG	cs4231cfg;

static SINT32 cs4231_DACvolume_L = 1024;
static SINT32 cs4231_DACvolume_R = 1024;
static UINT16 cs4231_DACvolumereg_L = 0xff;
static UINT16 cs4231_DACvolumereg_R = 0xff;

// 8bit モノラル
static void SOUNDCALL pcm8m(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;

	leng = cs->bufdatas >> 0;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 0) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 0) + 1) & CS4231_BUFMASK);
		samp1 = (ptr1[0] - 0x80) << 8;
		samp2 = (ptr2[0] - 0x80) << 8;
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * cs4231_DACvolume_L * np2cfg.vol_pcm) >> 15;
		pcm[1] += (samp1 * cs4231_DACvolume_R * np2cfg.vol_pcm) >> 15;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);

	leng = MIN(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 0);
	cs->bufpos = (cs->bufpos + (leng << 0)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

// 8bit ステレオ
static void SOUNDCALL pcm8s(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;

	leng = cs->bufdatas >> 1;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 2) & CS4231_BUFMASK);
		samp1 = (ptr1[0] - 0x80) << 8;
		samp2 = (ptr2[0] - 0x80) << 8;
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * cs4231_DACvolume_L * np2cfg.vol_pcm) >> 15;
		samp1 = (ptr1[1] - 0x80) << 8;
		samp2 = (ptr2[1] - 0x80) << 8;
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[1] += (samp1 * cs4231_DACvolume_R * np2cfg.vol_pcm) >> 15;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);

	leng = MIN(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 1);
	cs->bufpos = (cs->bufpos + (leng << 1)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

// 16bit モノラル
static void SOUNDCALL pcm16m(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;

	leng = cs->bufdatas >> 1;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 2) & CS4231_BUFMASK);
		samp1 = (((SINT8)ptr1[0]) << 8) + ptr1[1];
		samp2 = (((SINT8)ptr2[0]) << 8) + ptr2[1];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * cs4231_DACvolume_L * np2cfg.vol_pcm) >> 15;
		pcm[1] += (samp1 * cs4231_DACvolume_R * np2cfg.vol_pcm) >> 15;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);

	leng = MIN(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 1);
	cs->bufpos = (cs->bufpos + (leng << 1)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

// 16bit ステレオ
static void SOUNDCALL pcm16s(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;
	
	leng = cs->bufdatas >> 2;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 2) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 2) + 4) & CS4231_BUFMASK);
		samp1 = (((SINT8)ptr1[0]) << 8) + ptr1[1];
		samp2 = (((SINT8)ptr2[0]) << 8) + ptr2[1];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * cs4231_DACvolume_L * np2cfg.vol_pcm) >> 15;
		samp1 = (((SINT8)ptr1[2]) << 8) + ptr1[3];
		samp2 = (((SINT8)ptr2[2]) << 8) + ptr2[3];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[1] += (samp1 * cs4231_DACvolume_R * np2cfg.vol_pcm) >> 15;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);
	
	leng = MIN(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 2);
	cs->bufpos = (cs->bufpos + (leng << 2)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

// 16bit モノラル(little endian)
static void SOUNDCALL pcm16m_ex(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;
	
	leng = cs->bufdatas >> 1;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 2) & CS4231_BUFMASK);
		samp1 = (((SINT8)ptr1[1]) << 8) + ptr1[0];
		samp2 = (((SINT8)ptr2[1]) << 8) + ptr2[0];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * cs4231_DACvolume_L * np2cfg.vol_pcm) >> 15;
		pcm[1] += (samp1 * cs4231_DACvolume_R * np2cfg.vol_pcm) >> 15;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);
	
	leng = MIN(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 1);
	cs->bufpos = (cs->bufpos + (leng << 1)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

// 16bit ステレオ(little endian)
static void SOUNDCALL pcm16s_ex(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;

	leng = cs->bufdatas >> 2;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 2) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 2) + 4) & CS4231_BUFMASK);
		samp1 = (((SINT8)ptr1[1]) << 8) + ptr1[0];
		samp2 = (((SINT8)ptr2[1]) << 8) + ptr2[0];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * cs4231_DACvolume_L * np2cfg.vol_pcm) >> 15;
		samp1 = (((SINT8)ptr1[3]) << 8) + ptr1[2];
		samp2 = (((SINT8)ptr2[3]) << 8) + ptr2[2];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[1] += (samp1 * cs4231_DACvolume_R * np2cfg.vol_pcm) >> 15;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);
	
	leng = MIN(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 2);
	cs->bufpos = (cs->bufpos + (leng << 2)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

static void SOUNDCALL nomake(CS4231 cs, SINT32 *pcm, UINT count) {

	(void)cs;
	(void)pcm;
	(void)count;
}


typedef void (SOUNDCALL * CS4231FN)(CS4231 cs, SINT32 *pcm, UINT count);

static const CS4231FN cs4231fn[16] = {
			pcm8m,		// 0: 8bit PCM
			pcm8s,
			nomake,		// 1: u-Law
			nomake,
			pcm16m_ex,	// 2: 16bit PCM(little endian)?
			pcm16s_ex,
			nomake,		// 3: A-Law
			nomake,
			nomake,		// 4:
			nomake,
			nomake,		// 5: ADPCM
			nomake,
			pcm16m,		// 6: 16bit PCM
			pcm16s,
			nomake,		// 7: ADPCM
			nomake};


// ----

void SOUNDCALL cs4231_getpcm(CS4231 cs, SINT32 *pcm, UINT count) {

	static int bufdelaycounter = 0;

	if (((cs->reg.iface & 1) || bufdelaycounter > 0) && (count)) {
		// CS4231内蔵ボリューム 
		if(cs4231_DACvolumereg_L != cs->reg.dac_l){
			cs4231_DACvolumereg_L = cs->reg.dac_l;
			if(cs->reg.dac_l & 0x80){ // DAC L Mute
				cs4231_DACvolume_L = 0;
			}else{
				cs4231_DACvolume_L = (int)(pow(10, -1.5 * ((cs4231_DACvolumereg_L) & 0x3f) / 20.0) * 1024); // DAC L Volume (1bit毎に-1.5dB)
			}
		}
		if(cs4231_DACvolumereg_R != cs->reg.dac_r){
			cs4231_DACvolumereg_R = cs->reg.dac_r;
			if(cs->reg.dac_r & 0x80){ // DAC R Mute
				cs4231_DACvolume_R = 0;
			}else{
				cs4231_DACvolume_R = (int)(pow(10, -1.5 * ((cs4231_DACvolumereg_R) & 0x3f) / 20.0) * 1024); // DAC R Volume (1bit毎に-1.5dB)
			}
		}
		
		// 再生用バッファに送る
		(*cs4231fn[cs->reg.datafmt >> 4])(cs, pcm, count);

		//// CS4231タイマー割り込みTI（手抜き）
		//if ((cs->reg.pinctrl & 2) && (cs->dmairq != 0xff) && LOADINTELWORD(cs->reg.timer)) {
		//	static double timercount = 0;
		//	int decval = 0;
		//	timercount += (double)count/44100 * 1000 * 100; // 10usec timer
		//	decval = (int)(timercount);
		//	timercount -= (double)decval;
		//	cs->timercounter -= decval;
		//	if(cs->timercounter < 0){
		//		cs->timercounter = LOADINTELWORD(cs->reg.timer);
		//		cs->intflag |= INt;
		//		cs->reg.featurestatus |= TI;
		//		pic_setirq(cs->dmairq);
		//	}
		//}

		// Playback Enableがローになってもバッファのディレイ分は再生する
		if((cs->reg.iface & 1)){
			bufdelaycounter = cs->bufdatas;
		}else if(cs->bufdatas == 0){
			bufdelaycounter = 0;
		}else{
			bufdelaycounter--;
		}
	}
}

