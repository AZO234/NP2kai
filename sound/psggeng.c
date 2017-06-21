/**
 * @file	psggeng.c
 * @brief	Implementation of the PSG
 */

#include "compiler.h"
#include "psggen.h"
#include "parts.h"

extern	PSGGENCFG	psggencfg;


void SOUNDCALL psggen_getpcm(PSGGEN psg, SINT32 *pcm, UINT count) {

	SINT32	noisevol;
	UINT8	mixer;
	UINT	noisetbl;
	PSGTONE	*tone;
	PSGTONE	*toneterm;
	SINT32	samp;
	SINT32	vol;
	UINT	i;
	UINT	noise;

	if ((psg->mixer & 0x3f) == 0) {
		count = min(count, psg->puchicount);
		psg->puchicount -= count;
	}
	if (count == 0) {
		return;
	}
	do {
		noisevol = 0;
		if (psg->envcnt) {
			psg->envcnt--;
			if (psg->envcnt == 0) {
				psg->envvolcnt--;
				if (psg->envvolcnt < 0) {
					if (psg->envmode & PSGENV_ONESHOT) {
						psg->envvol = (psg->envmode & PSGENV_LASTON)?15:0;
					}
					else {
						psg->envvolcnt = 15;
						if (!(psg->envmode & PSGENV_ONECYCLE)) {
							psg->envmode ^= PSGENV_INC;
						}
						psg->envcnt = psg->envmax;
						psg->envvol = (psg->envvolcnt ^ psg->envmode) & 0x0f;
					}
				}
				else {
					psg->envcnt = psg->envmax;
					psg->envvol = (psg->envvolcnt ^ psg->envmode) & 0x0f;
				}
				psg->evol = psggencfg.volume[psg->envvol];
			}
		}
		mixer = psg->mixer;
		noisetbl = 0;
		if (mixer & 0x38) {
			for (i=0; i<(1 << PSGADDEDBIT); i++) {
				if (psg->noise.count > psg->noise.freq) {
					psg->noise.lfsr = (psg->noise.lfsr >> 1) ^ ((psg->noise.lfsr & 1) * 0x12000);
				}
				psg->noise.count -= psg->noise.freq;
				noisetbl |= (psg->noise.lfsr & 1) << i;
			}
		}
		tone = psg->tone;
		toneterm = tone + 3;
		do {
			vol = *(tone->pvol);
			if (vol) {
				samp = 0;
				switch(mixer & 9) {
					case 0:							// no mix
						if (tone->puchi) {
							tone->puchi--;
							samp += vol << PSGADDEDBIT;
						}
						break;

					case 1:							// tone only
						for (i=0; i<(1 << PSGADDEDBIT); i++) {
							tone->count += tone->freq;
							samp += vol * ((tone->count>=0)?1:-1);
						}
						break;

					case 8:							// noise only
						noise = noisetbl;
						for (i=0; i<(1 << PSGADDEDBIT); i++) {
							samp += vol * ((noise & 1)?1:-1);
							noise >>= 1;
						}
						break;

					case 9:
						noise = noisetbl;
						for (i=0; i<(1 << PSGADDEDBIT); i++) {
							tone->count += tone->freq;
							if ((tone->count >= 0) || (noise & 1)) {
								samp += vol;
							}
							else {
								samp -= vol;
							}
							noise >>= 1;
						}
						break;
				}
				if (!(tone->pan & 1)) {
					pcm[0] += samp;
				}
				if (!(tone->pan & 2)) {
					pcm[1] += samp;
				}
			}
			mixer >>= 1;
		} while(++tone < toneterm);
		pcm += 2;
	} while(--count);
}

