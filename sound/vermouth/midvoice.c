#include	"compiler.h"
#include	"midiout.h"


#define	RELBIT		6

#define	SAMPMULREL(a, b)		((a) * (b >> RELBIT))
#define	SAMPMULVOL(a, b)		((a) * (b))

#define RESAMPLING(d, s, p) {									\
	int		dat1;												\
	int		dat2;												\
	int		_div;												\
	dat1 = (s)[(p) >> FREQ_SHIFT];								\
	_div = (p) & FREQ_MASK;										\
	if (_div) {													\
		dat2 = (s)[((p) >> FREQ_SHIFT) + 1];					\
		dat1 += ((dat2 - dat1) * _div) >> FREQ_SHIFT;			\
	}															\
	*(d) = (_SAMPLE)dat1;										\
}


#if defined(ENABLE_VIRLATE)

// ---- vibrate

static int VERMOUTHCL vibrate_update(VOICE v) {

	int		depth;
	int		phase;
	int		pitch;
	float	step;

	depth = v->sample->vibrate_depth << 6;
	if (v->vibrate.sweepstep) {
		v->vibrate.sweepcount += v->vibrate.sweepstep;
		if (v->vibrate.sweepcount >= (1 << VIBSWEEP_SHIFT)) {
			v->vibrate.sweepstep = 0;
		}
		else {
			depth *= v->vibrate.sweepcount;
			depth >>= VIBSWEEP_SHIFT;
		}
	}

	phase = v->vibrate.phase++;
	phase &= (1 << VIBRATE_SHIFT) - 1;
	pitch = (vibsin12[phase] * depth) >> 12;
	step = v->freqnow * bendhtbl[(pitch >> (6 + 7)) + 24] *
												bendltbl[(pitch >> 7) & 0x3f];
	return((int)(step * (float)(1 << FREQ_SHIFT)));
}

static SAMPLE VERMOUTHCL resample_vibrate(VOICE v, SAMPLE dst,
															SAMPLE dstterm) {

	SAMPLE	src;
	SAMPLE	dstbreak;
	int		pos;
	int		step;
	int		last;
	int		cnt;

	src = v->sample->data;
	pos = v->samppos;
	step = v->sampstep;
	if (step < 0) {
		step *= -1;
	}
	last = v->sample->datasize;

	cnt = v->vibrate.count;
	if (cnt == 0) {
		cnt = v->vibrate.rate;
		step = vibrate_update(v);
		v->sampstep = step;
	}

	dstbreak = dst + cnt;
	if (dstbreak < dstterm) {
		do {
			do {
				RESAMPLING(dst, src, pos);
				dst++;
				pos += step;
				if (pos > last) {
					voice_setfree(v);
					return(dst);
				}
			} while(dst < dstbreak);

			step = vibrate_update(v);
			cnt = v->vibrate.rate;
			dstbreak += cnt;
		} while(dstbreak < dstterm);
		v->sampstep = step;
	}
	v->vibrate.count = cnt - (int)(dstterm - dst);
	do {
		RESAMPLING(dst, src, pos);
		dst++;
		pos += step;
		if (pos > last) {
			voice_setfree(v);
			return(dst);
		}
	} while(dst < dstterm);
	v->samppos = pos;
	return(dst);
}

static SAMPLE VERMOUTHCL resample_vibloop(VOICE v, SAMPLE dst,
															SAMPLE dstterm) {

	SAMPLE	src;
	SAMPLE	dstbreak;
	int		pos;
	int		step;
	int		last;
	int		cnt;

	src = v->sample->data;
	pos = v->samppos;
	step = v->sampstep;
	last = v->sample->loopend;

	cnt = v->vibrate.count;
	if (cnt == 0) {
		step = vibrate_update(v);
		cnt = v->vibrate.rate;
		v->sampstep = step;
	}

	dstbreak = dst + cnt;
	if (dstbreak < dstterm) {
		do {
			do {
				RESAMPLING(dst, src, pos);
				dst++;
				pos += step;
				if (pos > last) {
					pos -= last - v->sample->loopstart;
				}
			} while(dst < dstbreak);
			step = vibrate_update(v);
			cnt = v->vibrate.rate;
			dstbreak += cnt;
		} while(dstbreak < dstterm);
		v->sampstep = step;
	}
	v->vibrate.count = cnt - (int)(dstterm - dst);
	do {
		RESAMPLING(dst, src, pos);
		dst++;
		pos += step;
		if (pos > last) {
			pos -= last - v->sample->loopstart;
		}
	} while(dst < dstterm);

	v->samppos = pos;
	return(dst);
}

static SAMPLE VERMOUTHCL resample_vibround(VOICE v, SAMPLE dst,
															SAMPLE dstterm) {

	SAMPLE	src;
	SAMPLE	dstbreak;
	int		pos;
	int		step;
	int		start;
	int		last;
	int		cnt;
	BOOL	stepsign;

	src = v->sample->data;
	pos = v->samppos;
	step = v->sampstep;
	start = v->sample->loopstart;
	last = v->sample->loopend;

	cnt = v->vibrate.count;
	if (cnt == 0) {
		cnt = v->vibrate.rate;
		stepsign = (step < 0);
		step = vibrate_update(v);
		if (stepsign) {
			step = 0 - step;
		}
		v->sampstep = step;
	}
	dstbreak = dst + cnt;

	if (step < 0) {
		goto rr_backward;
	}

	if (dstbreak < dstterm) {
		do {
			do {
				RESAMPLING(dst, src, pos);
				dst++;
				pos += step;
				if (pos > last) {
					pos = last - (pos - last);
					step *= -1;
					goto rr_backward1;
				}
rr_forward1:;
			} while(dst < dstbreak);
			step = vibrate_update(v);
			cnt = v->vibrate.rate;
			dstbreak += cnt;
		} while(dstbreak < dstterm);
	}
	v->vibrate.count = cnt - (int)(dstterm - dst);
	do {
		RESAMPLING(dst, src, pos);
		dst++;
		pos += step;
		if (pos > last) {
			pos = last - (pos - last);
			step *= -1;
			goto rr_backward2;
		}
rr_forward2:;
	} while(dst < dstterm);
	goto rr_done;

rr_backward:
	if (dstbreak < dstterm) {
		do {
			do {
				RESAMPLING(dst, src, pos);
				dst++;
				pos += step;
				if (pos < start) {
					pos = start + (start - pos);
					step *= -1;
					goto rr_forward1;
				}
rr_backward1:;
			} while(dst < dstbreak);
			step = 0 - vibrate_update(v);
			cnt = v->vibrate.rate;
			dstbreak += cnt;
		} while(dstbreak < dstterm);
	}
	v->vibrate.count = cnt - (int)(dstterm - dst);
	do {
		RESAMPLING(dst, src, pos);
		dst++;
		pos += step;
		if (pos < start) {
			pos = start + (start - pos);
			step *= -1;
			goto rr_forward2;
		}
rr_backward2:;
	} while(dst < dstterm);

rr_done:
	v->samppos = pos;
	v->sampstep = step;
	return(dst);
}

#endif


// ---- normal

static SAMPLE VERMOUTHCL resample_normal(VOICE v, SAMPLE dst, SAMPLE dstterm) {

	SAMPLE	src;
	int		pos;
	int		step;
	int		last;

	src = v->sample->data;
	pos = v->samppos;
	step = v->sampstep;
	if (step < 0) {
		step *= -1;
	}
	last = v->sample->datasize;

	do {
		RESAMPLING(dst, src, pos);
		dst++;
		pos += step;
		if (pos > last) {
			voice_setfree(v);
			return(dst);
		}
	} while(dst < dstterm);

	v->samppos = pos;
	return(dst);
}

static SAMPLE VERMOUTHCL resample_loop(VOICE v, SAMPLE dst, SAMPLE dstterm) {

	SAMPLE	src;
	int		pos;
	int		step;
	int		last;

	src = v->sample->data;
	pos = v->samppos;
	step = v->sampstep;
	last = v->sample->loopend;
	do {
		RESAMPLING(dst, src, pos);
		dst++;
		pos += step;
		if (pos > last) {
			pos -= last - v->sample->loopstart;
		}
	} while(dst < dstterm);
	v->samppos = pos;
	return(dst);
}

static SAMPLE VERMOUTHCL resample_round(VOICE v, SAMPLE dst, SAMPLE dstterm) {

	SAMPLE	src;
	int		pos;
	int		step;
	int		start;
	int		last;

	src = v->sample->data;
	pos = v->samppos;
	step = v->sampstep;
	start = v->sample->loopstart;
	last = v->sample->loopend;

	if (step < 0) {
		goto rr_backward;
	}
rr_forward:
	do {
		RESAMPLING(dst, src, pos);
		dst++;
		pos += step;
		if (pos > last) {
			pos = last - (pos - last);
			step *= -1;
			if (dst < dstterm) {
				goto rr_backward;
			}
		}
	} while(dst < dstterm);
	goto rr_done;

rr_backward:
	do {
		RESAMPLING(dst, src, pos);
		dst++;
		pos += step;
		if (pos < start) {
			pos = start + (start - pos);
			step *= -1;
			if (dst < dstterm) {
				goto rr_forward;
			}
		}
	} while(dst < dstterm);

rr_done:
	v->samppos = pos;
	v->sampstep = step;
	return(dst);
}


// ----

int VERMOUTHCL envlope_setphase(VOICE v, int phase) {

	do {
		if (phase >= 6) {
			voice_setfree(v);
			return(1);
		}
		if (v->sample->mode & MODE_ENVELOPE) {
			if (v->phase & (VOICE_ON | VOICE_SUSTAIN)) {
				if (phase >= 3) {
					v->envstep = 0;
					return(0);
				}
			}
		}
		phase += 1;
	} while(v->envvol == v->sample->envpostbl[phase-1]);
	v->envphase = phase;
	v->envterm = v->sample->envpostbl[phase-1];
	v->envstep = v->sample->envratetbl[phase-1];
	if (v->envterm < v->envvol) {
		v->envstep *= -1;
	}
	return(0);
}

void VERMOUTHCL envelope_updates(VOICE v) {

	int		envl;
	int		envr;
	int		vol;

	envl = v->volleft;
	if ((v->flag & VOICE_MIXMASK) == VOICE_MIXNORMAL) {
		envr = v->volright;
#if defined(ENABLE_TREMOLO)
		if (v->tremolo.step) {
			envl *= v->tremolo.volume;
			envl >>= 12;
			envr *= v->tremolo.volume;
			envr >>= 12;
		}
#endif
		if (v->sample->mode & MODE_ENVELOPE) {
			vol = voltbl12[v->envvol >> (7 + 9 + 1)];
			envl *= vol;
			envl >>= 12;
			envr *= vol;
			envr >>= 12;
		}
		envl >>= (16 - SAMP_SHIFT);
		if (envl > SAMP_LIMIT) {
			envl = SAMP_LIMIT;
		}
		v->envleft = envl;
		envr >>= (16 - SAMP_SHIFT);
		if (envr > SAMP_LIMIT) {
			envr = SAMP_LIMIT;
		}
		v->envright = envr;
	}
	else {
#if defined(ENABLE_TREMOLO)
		if (v->tremolo.step) {
			envl *= v->tremolo.volume;
			envl >>= 12;
		}
#endif
		if (v->sample->mode & MODE_ENVELOPE) {
			envl *= voltbl12[v->envvol >> (7 + 9 + 1)];
			envl >>= 12;
		}
		envl >>= (16 - SAMP_SHIFT);
		if (envl > SAMP_LIMIT) {
			envl = SAMP_LIMIT;
		}
		v->envleft = envl;
	}
}

#if defined(ENABLE_TREMOLO)
static void VERMOUTHCL tremolo_update(VOICE v) {

	int		depth;
	int		cnt;
	int		vol;
	int		pos;

	depth = v->sample->tremolo_depth << 8;
	if (v->tremolo.sweepstep) {
		v->tremolo.sweepcount += v->tremolo.sweepstep;
		if (v->tremolo.sweepcount < (1 << TRESWEEP_SHIFT)) {
			depth *= v->tremolo.sweepcount;
			depth >>= TRESWEEP_SHIFT;
		}
		else {
			v->tremolo.sweepstep = 0;
		}
	}
	v->tremolo.count += v->tremolo.step;
	cnt = v->tremolo.count >> TRERATE_SHIFT;
	pos = cnt & ((1 << (SINENT_BIT - 2)) - 1);
	if (cnt & (1 << (SINENT_BIT - 2))) {
		pos ^= ((1 << (SINENT_BIT - 2)) - 1);
	}
	vol = envsin12q[pos];
	if (cnt & (1 << (SINENT_BIT - 1))) {
		vol = 0 - vol;
	}
	v->tremolo.volume = (1 << 12) - ((vol * depth) >> (2 + 8 + 8));
}
#endif

static int envelope_update(VOICE v) {

	if (v->envstep) {
		v->envvol += v->envstep;
		if (((v->envstep < 0) && (v->envvol <= v->envterm)) ||
			((v->envstep > 0) && (v->envvol >= v->envterm))) {
			v->envvol = v->envterm;
			if (envlope_setphase(v, v->envphase)) {
				return(1);
			}
			if (v->envstep == 0) {
				voice_setmix(v);
			}
		}
	}
#if defined(ENABLE_TREMOLO)
	if (v->tremolo.step) {
		tremolo_update(v);
	}
#endif
	envelope_updates(v);
	return(0);
}


// ---- release

static void VERMOUTHCL mixrel_normal(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	int		samples;
	SINT32	voll;
	SINT32	rell;
	SINT32	volr;
	SINT32	relr;
	_SAMPLE	s;

	samples = (int)(srcterm - src);
	voll = v->envleft << RELBIT;
	rell = -(voll / samples);
	if (!rell) {
		rell = -1;
	}
	volr = v->envright << RELBIT;
	relr = -(volr / samples);
	if (!relr) {
		relr = -1;
	}
	do {
		s = *src++;
		voll += rell;
		if (voll > 0) {
			dst[0] += SAMPMULREL(s, voll);
		}
		volr += relr;
		if (volr > 0) {
			dst[1] += SAMPMULREL(s, volr);
		}
		dst += 2;
	} while(src < srcterm);
}

static void VERMOUTHCL mixrel_left(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	SINT32	vol;
	SINT32	rel;
	_SAMPLE	s;

	vol = v->envleft << RELBIT;
	rel = - (vol / (int)(srcterm - src));
	if (!rel) {
		rel = -1;
	}
	do {
		vol += rel;
		if (vol <= 0) {
			break;
		}
		s = *src++;
		dst[0] += SAMPMULREL(s, vol);
		dst += 2;
	} while(src < srcterm);
}

static void VERMOUTHCL mixrel_right(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	mixrel_left(v, dst + 1, src, srcterm);
}

static void VERMOUTHCL mixrel_centre(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	SINT32	vol;
	SINT32	rel;
	_SAMPLE	s;
	SINT32	d;

	vol = v->envleft << RELBIT;
	rel = - (vol / (int)(srcterm - src));
	if (!rel) {
		rel = -1;
	}
	do {
		vol += rel;
		if (vol <= 0) {
			break;
		}
		s = *src++;
		d = SAMPMULREL(s, vol);
		dst[0] += d;
		dst[1] += d;
		dst += 2;
	} while(src < srcterm);
}


// ---- normal

static void VERMOUTHCL mixnor_normal(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	SINT32	voll;
	SINT32	volr;
	_SAMPLE	s;

	voll = v->envleft;
	volr = v->envright;
	do {
		s = *src++;
		dst[0] += SAMPMULVOL(s, voll);
		dst[1] += SAMPMULVOL(s, volr);
		dst += 2;
	} while(src < srcterm);
}

static void VERMOUTHCL mixnor_left(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	SINT32	vol;
	_SAMPLE	s;

	vol = v->envleft;
	do {
		s = *src++;
		dst[0] += SAMPMULVOL(s, vol);
		dst += 2;
	} while(src < srcterm);
}

static void VERMOUTHCL mixnor_right(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	SINT32	vol;
	_SAMPLE	s;

	vol = v->envleft;
	do {
		s = *src++;
		dst[1] += SAMPMULVOL(s, vol);
		dst += 2;
	} while(src < srcterm);
}

static void VERMOUTHCL mixnor_centre(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	SINT32	vol;
	_SAMPLE	s;
	SINT32	d;

	vol = v->envleft;
	do {
		s = *src++;
		d = SAMPMULVOL(s, vol);
		dst[0] += d;
		dst[1] += d;
		dst += 2;
	} while(src < srcterm);
}


// ---- env

static void VERMOUTHCL mixenv_normal(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	int		cnt;
	SINT32	voll;
	SINT32	volr;
	SAMPLE	srcbreak;
	_SAMPLE	s;

	cnt = v->envcount;
	if (!cnt) {
		cnt = ENV_RATE;
		if (envelope_update(v)) {
			return;
		}
	}
	voll = v->envleft;
	volr = v->envright;

	srcbreak = src + cnt;
	if (srcbreak < srcterm) {
		do {
			do {
				s = *src++;
				dst[0] += SAMPMULVOL(s, voll);
				dst[1] += SAMPMULVOL(s, volr);
				dst += 2;
			} while(src < srcbreak);
			cnt = ENV_RATE;
			if (envelope_update(v)) {
				return;
			}
			voll = v->envleft;
			volr = v->envright;
			srcbreak = src + cnt;
		} while(srcbreak < srcterm);
	}

	v->envcount = cnt - (int)(srcterm - src);
	do {
		s = *src++;
		dst[0] += SAMPMULVOL(s, voll);
		dst[1] += SAMPMULVOL(s, volr);
		dst += 2;
	} while(src < srcterm);
}

static void VERMOUTHCL mixenv_left(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	int		cnt;
	SINT32	vol;
	SAMPLE	srcbreak;
	_SAMPLE	s;

	cnt = v->envcount;
	if (!cnt) {
		cnt = ENV_RATE;
		if (envelope_update(v)) {
			return;
		}
	}
	vol = v->envleft;

	srcbreak = src + cnt;
	if (srcbreak < srcterm) {
		do {
			do {
				s = *src++;
				dst[0] += SAMPMULVOL(s, vol);
				dst += 2;
			} while(src < srcbreak);
			cnt = ENV_RATE;
			if (envelope_update(v)) {
				return;
			}
			vol = v->envleft;
			srcbreak = src + cnt;
		} while(srcbreak < srcterm);
	}

	v->envcount = cnt - (int)(srcterm - src);
	do {
		s = *src++;
		dst[0] += SAMPMULVOL(s, vol);
		dst += 2;
	} while(src < srcterm);
}

static void VERMOUTHCL mixenv_right(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	mixenv_left(v, dst + 1, src, srcterm);
}

static void VERMOUTHCL mixenv_centre(VOICE v, SINT32 *dst, SAMPLE src,
															SAMPLE srcterm) {

	int		cnt;
	SINT32	vol;
	SAMPLE	srcbreak;
	_SAMPLE	s;
	SINT32	d;

	cnt = v->envcount;
	if (!cnt) {
		cnt = ENV_RATE;
		if (envelope_update(v)) {
			return;
		}
	}
	vol = v->envleft;

	srcbreak = src + cnt;
	if (srcbreak < srcterm) {
		do {
			do {
				s = *src++;
				d = SAMPMULVOL(s, vol);
				dst[0] += d;
				dst[1] += d;
				dst += 2;
			} while(src < srcbreak);
			cnt = ENV_RATE;
			if (envelope_update(v)) {
				return;
			}
			vol = v->envleft;
			srcbreak = src + cnt;
		} while(srcbreak < srcterm);
	}

	v->envcount = cnt - (int)(srcterm - src);
	do {
		s = *src++;
		d = SAMPMULVOL(s, vol);
		dst[0] += d;
		dst[1] += d;
		dst += 2;
	} while(src < srcterm);
}


// ----

static const RESPROC resproc[] = {
		resample_normal,	resample_loop,		resample_round,
#if defined(ENABLE_VIRLATE)
		resample_vibrate,	resample_vibloop,	resample_vibround,
#endif
};

void VERMOUTHCL voice_setphase(VOICE v, UINT8 phase) {

	int		proc;
	int		mode;

	v->phase = phase;

#if defined(ENABLE_VIRLATE)
	proc = (v->vibrate.rate)?3:0;
#else
	proc = 0;
#endif
	mode = v->sample->mode;
	if ((mode & MODE_LOOPING) &&
		((mode & MODE_ENVELOPE) || (phase & (VOICE_ON | VOICE_SUSTAIN)))) {
		proc++;
		if (mode & MODE_PINGPONG) {
			proc++;
		}
	}
	v->resamp = resproc[proc];
}


// ----

static const MIXPROC mixproc[] = {
			mixnor_normal,	mixnor_left,	mixnor_right,	mixnor_centre,
			mixenv_normal,	mixenv_left,	mixenv_right,	mixenv_centre,
			mixrel_normal,	mixrel_left,	mixrel_right,	mixrel_centre};

void VERMOUTHCL voice_setmix(VOICE v) {

	int		proc;

	proc = v->flag & VOICE_MIXMASK;
	if (!(v->phase & VOICE_REL)) {
#if defined(ENABLE_TREMOLO)
		if ((v->envstep != 0) || (v->tremolo.step != 0))
#else
		if (v->envstep != 0)
#endif
		{
			proc += 1 << 2;
		}
	}
	else {
		proc += 2 << 2;
	}
	v->mix = mixproc[proc];
}

