/**
 * @file	pcmmix.c
 * @brief	Implementation of the pcm sound
 */

#include "compiler.h"
#include "pcmmix.h"
#include "getsnd/getsnd.h"
#include "dosio.h"

BRESULT pcmmix_regist(PMIXDAT *dat, void *datptr, UINT datsize, UINT rate) {

	GETSND	gs;
	UINT8	tmp[256];
	UINT	size;
	UINT	r;
	SINT16	*buf;

	gs = getsnd_create(datptr, datsize);
	if (gs == NULL) {
		goto pmr_err1;
	}
	if (getsnd_setmixproc(gs, rate, 1) != SUCCESS) {
		goto pmr_err2;
	}
	size = 0;
	do {
		r = getsnd_getpcmbyleng(gs, tmp, sizeof(tmp));
		size += r;
	} while(r);
	getsnd_destroy(gs);
	if (size == 0) {
		goto pmr_err1;
	}

	buf = (SINT16 *)_MALLOC(size, "PCM DATA");
	if (buf == NULL) {
		goto pmr_err1;
	}
	gs = getsnd_create(datptr, datsize);
	if (gs == NULL) {
		goto pmr_err1;
	}
	if (getsnd_setmixproc(gs, rate, 1) != SUCCESS) {
		goto pmr_err2;
	}
	r = getsnd_getpcmbyleng(gs, buf, size);
	getsnd_destroy(gs);
	dat->sample = buf;
	dat->samples = r / 2;
	return(SUCCESS);

pmr_err2:
	getsnd_destroy(gs);

pmr_err1:
	return(FAILURE);
}

BRESULT pcmmix_regfile(PMIXDAT *dat, const OEMCHAR *fname, UINT rate) {

	FILEH	fh;
	UINT	size;
	UINT8	*ptr;
	BRESULT	r;

	r = FAILURE;
	fh = file_open_rb(fname);
	if (fh == FILEH_INVALID) {
		goto pmrf_err1;
	}
	size = file_getsize(fh);
	if (size == 0) {
		goto pmrf_err2;
	}
	ptr = (UINT8 *)_MALLOC(size, fname);
	if (ptr == NULL) {
		goto pmrf_err2;
	}
	file_read(fh, ptr, size);
	file_close(fh);
	r = pcmmix_regist(dat, ptr, size, rate);
	_MFREE(ptr);
	return(r);

pmrf_err2:
	file_close(fh);

pmrf_err1:
	return(FAILURE);
}

void SOUNDCALL pcmmix_getpcm(PCMMIX hdl, SINT32 *pcm, UINT count) {

	UINT32		bitmap;
	PMIXTRK		*t;
const SINT16	*s;
	UINT		srem;
	SINT32		*d;
	UINT		drem;
	UINT		r;
	UINT		j;
	UINT		flag;
	SINT32		vol;
	SINT32		samp;

	if ((hdl->hdr.playing == 0) || (count == 0))  {
		return;
	}
	t = hdl->trk;
	bitmap = 1;
	do {
		if (hdl->hdr.playing & bitmap) {
			s = t->pcm;
			srem = t->remain;
			d = pcm;
			drem = count;
			flag = t->flag;
			vol = t->volume;
			do {
				r = min(srem, drem);
				switch(flag & (PMIXFLAG_L | PMIXFLAG_R)) {
					case PMIXFLAG_L:
						for (j=0; j<r; j++) {
							d[j*2+0] += (s[j] * vol) >> 12;
						}
						break;

					case PMIXFLAG_R:
						for (j=0; j<r; j++) {
							d[j*2+1] += (s[j] * vol) >> 12;
						}
						break;

					case PMIXFLAG_L | PMIXFLAG_R:
						for (j=0; j<r; j++) {
							samp = (s[j] * vol) >> 12;
							d[j*2+0] += samp;
							d[j*2+1] += samp;
						}
						break;
				}
				s += r;
				d += r*2;
				srem -= r;
				if (srem == 0) {
					if (flag & PMIXFLAG_LOOP) {
						s = t->data.sample;
						srem = t->data.samples;
					}
					else {
						hdl->hdr.playing &= ~bitmap;
						break;
					}
				}
				drem -= r;
			} while(drem);
			t->pcm = s;
			t->remain = srem;
		}
		t++;
		bitmap <<= 1;
	} while(bitmap < hdl->hdr.enable);
}
