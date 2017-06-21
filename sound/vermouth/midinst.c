#include	"compiler.h"
#include	"dosio.h"
#include	"midiout.h"


#if defined(SUPPORT_ARC)
#include	"arc.h"
#define	_FILEH				ARCFH
#define	_FILEH_INVALID		NULL
#define _file_open			arcex_fileopen
#define	_file_read			arc_fileread
#define	_file_close			arc_fileclose
#else
#define	_FILEH				FILEH
#define	_FILEH_INVALID		FILEH_INVALID
#define _file_open			file_open
#define	_file_read			file_read
#define	_file_close			file_close
#endif


// GUS format references:  http://www.onicos.com/staff/iz/formats/guspat.html

typedef struct {
	char	sig[12];
	char	id[10];
	char	description[60];

	UINT8	instruments;
	UINT8	voice;
	UINT8	channels;
	UINT8	waveforms[2];
	UINT8	volume[2];
	UINT8	datasize[4];
	UINT8	reserved1[36];

	UINT8	instrument[2];
	UINT8	instname[16];
	UINT8	instsize[4];
	UINT8	layers;
	UINT8	reserved2[40];

	UINT8	layerdupe;
	UINT8	layer;
	UINT8	layersize[4];
	UINT8	layersamples;
	UINT8	reserved3[40];
} GUSHEAD;

typedef struct {
	UINT8	wavename[7];
	UINT8	fractions;
	UINT8	datasize[4];
	UINT8	loopstart[4];
	UINT8	loopend[4];
	UINT8	samprate[2];
	UINT8	freqlow[4];
	UINT8	freqhigh[4];
	UINT8	freqroot[4];
	UINT8	tune[2];
	UINT8	balance;
	UINT8	env[6];
	UINT8	envpos[6];
	UINT8	tre_sweep;
	UINT8	tre_rate;
	UINT8	tre_depth;
	UINT8	vib_sweep;
	UINT8	vib_rate;
	UINT8	vib_depth;
	UINT8	mode;
	UINT8	scalefreq[2];
	UINT8	scalefactor[2];
	UINT8	reserved[36];
} GUSWAVE;


static void VERMOUTHCL inst_destroy(INSTRUMENT inst);


// ---- resample

#define	BASEBITS	9
#define	MIXBASE		(1 << BASEBITS)

static SAMPLE VERMOUTHCL downsamp(SAMPLE dst, SAMPLE src, int count,
																int mrate) {

	int		rem;
	SINT32	pcm;

	rem = MIXBASE;
	pcm = 0;
	do {
		if (rem > mrate) {
			rem -= mrate;
			pcm += (*src++) * mrate;
		}
		else {
			pcm += (*src) * rem;
			pcm >>= BASEBITS;
			*dst++ = (_SAMPLE)pcm;
			pcm = mrate - rem;
			rem = MIXBASE - pcm;
			pcm *= (*src++);
		}
	} while(--count);
	if (rem != MIXBASE) {
		*dst++ = (_SAMPLE)(pcm >> BASEBITS);
	}
	return(dst);
}

static SAMPLE VERMOUTHCL upsamp(SAMPLE dst, SAMPLE src, int count,
																int mrate) {

	int		rem;
	SINT32	tmp;
	SINT32	pcm;
	SINT32	dat;

	rem = 0;
	pcm = 0;
	do {
		tmp = MIXBASE - rem;
		if (tmp >= 0) {
			dat = (pcm * rem);
			pcm = *src++;
			dat += (pcm * tmp);
			dat >>= BASEBITS;
			*dst++ = (_SAMPLE)dat;
			rem = mrate - tmp;
			count--;
		}
		while(rem >= MIXBASE) {
			rem -= MIXBASE;
			*dst++ = (_SAMPLE)pcm;
		}
	} while(count);
	if (rem) {
		*dst++ = rem >> BASEBITS;
	}
	return(dst);
}

static void VERMOUTHCL resample(MIDIMOD mod, INSTLAYER inst, int freq) {

	int		mrate;
	int		orgcnt;
	int		newcnt;
	SAMPLE	dst;
	SAMPLE	dstterm;

	mrate = (int)((float)MIXBASE *
					(float)mod->samprate / (float)inst->samprate *
					(float)inst->freqroot / (float)freq);
	if (mrate != MIXBASE) {
		orgcnt = inst->datasize >> FREQ_SHIFT;
		newcnt = (orgcnt * mrate + MIXBASE - 1) >> BASEBITS;
		dst = (SAMPLE)_MALLOC((newcnt + 1) * sizeof(_SAMPLE), "resampled");
		if (dst == NULL) {
			return;
		}
		dst[newcnt] = 0;
		if (mrate > MIXBASE) {
			dstterm = upsamp(dst, inst->data, orgcnt, mrate);
		}
		else {
			dstterm = downsamp(dst, inst->data, orgcnt, mrate);
		}
#if 0
		if ((dstterm - dst) != newcnt) {
			TRACEOUT(("resample error %d %d", newcnt, dstterm - dst));
		}
#endif
		inst->datasize = newcnt << FREQ_SHIFT;
		inst->loopstart = 0;
		inst->loopend = newcnt << FREQ_SHIFT;
		_MFREE(inst->data);
		inst->data = dst;
	}
	inst->samprate = 0;
}


// ---- load

static const OEMCHAR ext_pat[] = OEMTEXT(".pat");
static const char sig_GF1PATCH100[] = "GF1PATCH100";
static const char sig_GF1PATCH110[] = "GF1PATCH110";
static const char sig_ID000002[] = "ID#000002";
static const char str_question6[] = "??????";

static INSTRUMENT VERMOUTHCL inst_create(MIDIMOD mod, const _TONECFG *cfg) {

	OEMCHAR		filename[MAX_PATH];
	OEMCHAR		path[MAX_PATH];
	_FILEH		fh;
	INSTRUMENT	ret;
	GUSHEAD		head;
	int			layers;
	int			size;
	INSTLAYER	layer;
	GUSWAVE		wave;
	int			i;
	SAMPLE		dat;
	_SAMPLE		tmp;
const UINT8		*d;
	SINT16		*p;
	SINT16		*q;
	int			cnt;

	if (cfg->name == NULL) {
		goto li_err1;
	}
	file_cpyname(filename, cfg->name, NELEMENTS(filename));
	file_cutext(filename);
	file_catname(filename, ext_pat, NELEMENTS(filename));
	if (midimod_getfile(mod, filename, path, NELEMENTS(path)) != SUCCESS) {
		goto li_err1;
	}
	fh = _file_open(path);
	if (fh == _FILEH_INVALID) {
//		TRACEOUT(("not found: %s", path));
		goto li_err1;
	}

	// head check
	if ((_file_read(fh, &head, sizeof(head)) != sizeof(head)) &&
		(memcmp(head.sig, sig_GF1PATCH100, 12)) &&
		(memcmp(head.sig, sig_GF1PATCH110, 12)) &&
		(memcmp(head.id, sig_ID000002, 10)) &&
		(head.instruments != 0) && (head.instruments != 1) &&
		(head.layers != 0) && (head.layers != 1)) {
		goto li_err2;
	}

	layers = head.layersamples;
	if (!layers) {
		goto li_err2;
	}
	size = sizeof(_INSTRUMENT) + (layers * sizeof(_INSTLAYER));
	ret = (INSTRUMENT)_MALLOC(size, "instrument");
	if (ret == NULL) {
		goto li_err2;
	}
	ZeroMemory(ret, size);
	layer = (INSTLAYER)(ret + 1);
	ret->layers = layers;
	if (cfg->note != TONECFG_VARIABLE) {
		ret->freq = freq_table[cfg->note];
	}

	do {
		UINT8 fractions;

		if (_file_read(fh, &wave, sizeof(wave)) != sizeof(wave)) {
			goto li_err3;
		}
		fractions = wave.fractions;
		layer->datasize = LOADINTELDWORD(wave.datasize);
		layer->loopstart = LOADINTELDWORD(wave.loopstart);
		layer->loopend = LOADINTELDWORD(wave.loopend);
		layer->samprate = LOADINTELWORD(wave.samprate);
		layer->freqlow = LOADINTELDWORD(wave.freqlow);
		layer->freqhigh = LOADINTELDWORD(wave.freqhigh);
		layer->freqroot = LOADINTELDWORD(wave.freqroot);
		if (cfg->pan == TONECFG_VARIABLE) {
			layer->panpot = ((wave.balance * 8) + 4) & 0x7f;
		}
		else {
			layer->panpot = cfg->pan & 0x7f;
		}

		if (wave.mode & MODE_LOOPING) {
			wave.mode |= MODE_SUSTAIN;
		}
		if (cfg->flag & TONECFG_NOLOOP) {
			wave.mode &= ~(MODE_SUSTAIN | MODE_LOOPING | MODE_PINGPONG |
															MODE_REVERSE);
		}
		if (cfg->flag & TONECFG_NOENV) {
			wave.mode &= ~MODE_ENVELOPE;
		}
		else if (!(cfg->flag & TONECFG_KEEPENV)) {
			if (wave.mode & (MODE_LOOPING | MODE_PINGPONG | MODE_REVERSE)) {
				if ((!(wave.mode & MODE_SUSTAIN)) ||
					(!memcmp(wave.env, str_question6, 6)) ||
					(wave.envpos[5] >= 100)) {
					wave.mode &= ~MODE_ENVELOPE;
				}
			}
			else {
				wave.mode &= ~(MODE_SUSTAIN | MODE_ENVELOPE);
			}
		}

		for (i=0; i<6; i++) {
			int sft;
			sft = ((wave.env[i] >> 6) ^ 3) * 3;
			layer->envratetbl[i] = ((((wave.env[i] & 0x3f) << sft) * 44100
									/ mod->samprate) * ENV_RATE) << (3 + 1);
			layer->envpostbl[i] = wave.envpos[i] << (7 + 9);
		}
		if ((wave.tre_rate != 0) && (wave.tre_depth != 0)) {
			layer->tremolo_step = ((ENV_RATE * wave.tre_rate)
										<< (SINENT_BIT + TRERATE_SHIFT))
										/ (TRERATE_TUNE * mod->samprate);
			if (wave.tre_sweep) {
				layer->tremolo_sweep = ((ENV_RATE * TRESWEEP_TUNE)
										<< TRESWEEP_SHIFT)
										/ (wave.tre_sweep * mod->samprate);
			}
			layer->tremolo_depth = wave.tre_depth;
		}
		if ((wave.vib_rate != 0) && (wave.vib_depth != 0)) {
			layer->vibrate_rate = (VIBRATE_TUNE * mod->samprate) /
											(wave.vib_rate << VIBRATE_SHIFT);
			if (wave.vib_sweep) {
				layer->vibrate_sweep =
						((VIBRATE_TUNE * VIBSWEEP_TUNE) << VIBSWEEP_SHIFT) /
						(wave.vib_sweep * wave.vib_rate << VIBRATE_SHIFT);
			}
			layer->vibrate_depth = wave.vib_depth;
		}

		cnt = layer->datasize;
		if (!cnt) {
			goto li_err3;
		}
		if (wave.mode & MODE_16BIT) {
			cnt >>= 1;
		}
		dat = (SAMPLE)_MALLOC((cnt + 1) * sizeof(_SAMPLE), "data");
		if (dat == NULL) {
			goto li_err3;
		}
		layer->data = dat;
		if (_file_read(fh, dat, layer->datasize) != (UINT)layer->datasize) {
			goto li_err3;
		}
		dat[cnt] = 0;
		if (wave.mode & MODE_16BIT) {
			layer->datasize >>= 1;
			layer->loopstart >>= 1;
			layer->loopend >>= 1;
#if defined(BYTESEX_LITTLE)
			if (sizeof(_SAMPLE) != 2) {				// Ara!?
#endif
				d = (UINT8 *)dat;
				d += layer->datasize * 2;
				q = dat + layer->datasize;
				do {
					d -= 2;
					q--;
					*q = (_SAMPLE)LOADINTELWORD(d);
				} while(q > dat);
#if defined(BYTESEX_LITTLE)
			}
#endif
		}
		else {
			d = (UINT8 *)dat;
			d += layer->datasize;
			q = dat + layer->datasize;
			do {
				d--;
				q--;
				*q = (_SAMPLE)((*d) * 257);
			} while(q > dat);
		}
		if (wave.mode & MODE_UNSIGNED) {
			q = dat + layer->datasize;
			do {
				q--;
				*q -= (_SAMPLE)0x8000;
			} while(q > dat);
		}
		if (wave.mode & MODE_REVERSE) {
			p = dat;
			q = dat + layer->datasize;
			do {
				q--;
				tmp = *p;
				*p = *q;
				*q = tmp;
				p++;
			} while(p < q);
		}

		if (cfg->amp == TONECFG_AUTOAMP) {
			int	sampdat;
			int sampmax;
			q = (SAMPLE)dat;
			cnt = layer->datasize;
			sampmax = 32768 / 4;
			do {
				sampdat = *q++;
				if (sampdat < 0) {
					sampdat *= -1;
				}
				sampmax = max(sampmax, sampdat);
			} while(--cnt);
			layer->volume = 32768 * 128 / sampmax;
		}
		else {
			layer->volume = cfg->amp * 128 / 100;
		}

		layer->datasize <<= FREQ_SHIFT;
		layer->loopstart <<= FREQ_SHIFT;
		layer->loopend <<= FREQ_SHIFT;
		layer->loopstart |= (fractions & 0x0F) << (FREQ_SHIFT - 4);
		layer->loopend |= ((fractions >> 4) & 0x0F) << (FREQ_SHIFT - 4);

		if (layer->loopstart > layer->datasize) {
			layer->loopstart = layer->datasize;
		}
		if (layer->loopend > layer->datasize) {
			layer->loopend = layer->datasize;
		}
		if (wave.mode & MODE_REVERSE) {
			cnt = layer->loopstart;
			layer->loopstart = layer->datasize - layer->loopend;
			layer->loopend = layer->datasize - cnt;
			wave.mode &= ~MODE_REVERSE;
			wave.mode |= MODE_LOOPING;
		}

		if ((ret->freq) && (!(wave.mode & MODE_LOOPING))) {
//			TRACEOUT(("resample: %s", cfg->name));
			resample(mod, layer, ret->freq);
		}
		if (cfg->flag & TONECFG_NOTAIL) {
			layer->datasize = layer->loopend;
		}
		layer->mode = wave.mode;

		layer++;
	} while(--layers);

	_file_close(fh);
	return(ret);

li_err3:
	inst_destroy(ret);

li_err2:
	_file_close(fh);

li_err1:
	return(NULL);
}

static void VERMOUTHCL inst_destroy(INSTRUMENT inst) {

	int			layers;
	INSTLAYER	layer;

	if (inst) {
		layers = inst->layers;
		layer = (INSTLAYER)(inst + 1);
		while(layers--) {
			if (layer->data) {
				_MFREE(layer->data);
			}
			layer++;
		}
		_MFREE(inst);
	}
}

int VERMOUTHCL inst_singleload(MIDIMOD mod, UINT bank, UINT num) {

	INSTRUMENT	*inst;
	INSTRUMENT	tone;
const _TONECFG	*cfg;

	if (bank >= (MIDI_BANKS * 2)) {
		return(MIDIOUT_FAILURE);
	}
	cfg = mod->tonecfg[bank];
	if (cfg == NULL) {
		return(MIDIOUT_FAILURE);
	}
	inst = mod->tone[bank];
	num &= 0x7f;
	if ((inst == NULL) || (inst[num] == NULL)) {
		tone = inst_create(mod, cfg + num);
		if (tone == NULL) {
			return(MIDIOUT_FAILURE);
		}
		if (inst == NULL) {
			inst = (INSTRUMENT *)_MALLOC(sizeof(INSTRUMENT) * 128, "INST");
			if (inst == NULL) {
				inst_destroy(tone);
				return(MIDIOUT_FAILURE);
			}
			mod->tone[bank] = inst;
			ZeroMemory(inst, sizeof(INSTRUMENT) * 128);
		}
		inst[num] = tone;
	}
	return(MIDIOUT_SUCCESS);
}

int VERMOUTHCL inst_bankload(MIDIMOD mod, UINT bank) {

	return(inst_bankloadex(mod, bank, NULL, NULL));
}

int VERMOUTHCL inst_bankloadex(MIDIMOD mod, UINT bank,
							FNMIDIOUTLAEXCB cb, MIDIOUTLAEXPARAM *param) {

	INSTRUMENT	*inst;
	INSTRUMENT	tone;
const _TONECFG	*cfg;
	UINT		num;

	if (bank >= (MIDI_BANKS * 2)) {
		return(MIDIOUT_FAILURE);
	}
	cfg = mod->tonecfg[bank];
	if (cfg == NULL) {
		return(MIDIOUT_FAILURE);
	}
	inst = mod->tone[bank];
	for (num = 0; num<0x80; num++) {
		if ((inst == NULL) || (inst[num] == NULL)) {
			if ((cb != NULL) && (cfg[num].name != NULL)) {
				if (param) {
					param->progress++;
					param->num = num;
				}
				if ((*cb)(param) != SUCCESS) {
					return(MIDIOUT_ABORT);
				}
			}
			tone = inst_create(mod, cfg + num);
			if (tone) {
//				TRACEOUT(("load %d %d", bank, num));
				if (inst == NULL) {
					inst = (INSTRUMENT *)_MALLOC(
										sizeof(INSTRUMENT) * 128, "INST");
					if (inst == NULL) {
						inst_destroy(tone);
						return(MIDIOUT_FAILURE);
					}
					mod->tone[bank] = inst;
					ZeroMemory(inst, sizeof(INSTRUMENT) * 128);
				}
				inst[num] = tone;
			}
		}
	}
	return(MIDIOUT_SUCCESS);
}

void VERMOUTHCL inst_bankfree(MIDIMOD mod, UINT bank) {

	INSTRUMENT	*inst;
	INSTRUMENT	*i;

	if (bank >= (MIDI_BANKS * 2)) {
		return;
	}
	inst = mod->tone[bank];
	if (inst == NULL) {
		return;
	}
	i = inst + 128;
	do {
		i--;
		inst_destroy(*i);
	} while(i > inst);
	if (bank >= 2) {
		mod->tone[bank] = NULL;
		_MFREE(inst);
	}
	else {
		ZeroMemory(inst, sizeof(INSTRUMENT) * 128);
	}
}

UINT VERMOUTHCL inst_gettones(MIDIMOD mod, UINT bank) {

	INSTRUMENT	*inst;
const _TONECFG	*cfg;
	UINT		ret;
	UINT		num;

	if (bank >= (MIDI_BANKS * 2)) {
		return(0);
	}
	cfg = mod->tonecfg[bank];
	if (cfg == NULL) {
		return(0);
	}
	inst = mod->tone[bank];
	ret = 0;
	for (num = 0; num<0x80; num++) {
		if ((inst == NULL) || (inst[num] == NULL)) {
			if (cfg[num].name != NULL) {
				ret++;
			}
		}
	}
	return(ret);
}
