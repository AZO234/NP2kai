#include	"compiler.h"
#include	"midiout.h"


#define	MIDIOUT_VERSION		0x116
#define	MIDIOUT_VERSTRING	"VERMOUTH 1.16"

static const char vermouthver[] = MIDIOUT_VERSTRING;

static const int gaintbl[24+1] =
				{ 16,  19,  22,  26,  32,  38,  45,  53,
				  64,  76,  90, 107, 128, 152, 181, 215,
				 256, 304, 362, 430, 512, 608, 724, 861, 1024};


// ---- voice

#define	VOICEHDLPTR(hdl)	((hdl)->voice)
#define	VOICEHDLEND(hdl)	(VOICE)((hdl) + 1)

static void VERMOUTHCL voice_volupdate(VOICE v) {

	CHANNEL	ch;
	int		vol;

	ch = v->channel;
#if defined(VOLUME_ACURVE)
	vol = ch->level * acurve[v->velocity];
	vol >>= 8;
#else
	vol = ch->level * v->velocity;
	vol >>= 7;
#endif
	vol *= v->sample->volume;
	vol >>= (21 - 16);
#if defined(PANPOT_REVA)
	switch(v->flag & VOICE_MIXMASK) {
		case VOICE_MIXNORMAL:
			v->volleft = (vol * revacurve[v->panpot ^ 127]) >> 8;
			v->volright = (vol * revacurve[v->panpot]) >> 8;
			break;

		case VOICE_MIXCENTRE:
			v->volleft = (vol * 155) >> 8;
			break;

		default:
			v->volleft = vol;
			break;
	}
#else
	v->volleft = vol;
	if ((v->flag & VOICE_MIXMASK) == VOICE_MIXNORMAL) {
		if (v->panpot < 64) {
			vol *= (v->panpot + 1);
			vol >>= 6;
			v->volright = vol;
		}
		else {
			v->volright = vol;
			vol *= (127 - v->panpot);
			vol >>= 6;
			v->volleft = vol;
		}
	}
#endif
}

static INSTLAYER VERMOUTHCL selectlayer(VOICE v, INSTRUMENT inst) {

	int			layers;
	INSTLAYER	layer;
	INSTLAYER	layerterm;
	int			freq;
	int			diffmin;
	int			diff;
	INSTLAYER	layersel;

	layers = inst->layers;
	layer = (INSTLAYER)(inst + 1);

	if (layers == 1) {
		return(layer);
	}

	layerterm = layer + layers;
	freq = v->frequency;
	do {
		if ((freq >= layer->freqlow) && (freq <= layer->freqhigh)) {
			return(layer);
		}
		layer++;
	} while(layer < layerterm);

	layer = (INSTLAYER)(inst + 1);
	layersel = layer;
	diffmin = layer->freqroot - freq;
	if (diffmin < 0) {
		diffmin *= -1;
	}
	layer++;
	do {
		diff = layer->freqroot - freq;
		if (diff < 0) {
			diff *= -1;
		}
		if (diffmin > diff) {
			diffmin = diff;
			layersel = layer;
		}
		layer++;
	} while(layer < layerterm);
	return(layersel);
}

static void VERMOUTHCL freq_update(VOICE v) {

	CHANNEL	ch;
	float	step;

	if (v->flag & VOICE_FIXPITCH) {
		return;
	}

	ch = v->channel;
	step = v->freq;
	if (ch->pitchbend != 0x2000) {
		step *= ch->pitchfactor;
	}
#if defined(ENABLE_VIRLATE)
	v->freqnow = step;
#endif
	step *= (float)(1 << FREQ_SHIFT);
	if (v->sampstep < 0) {
		step *= -1.0;
	}
	v->sampstep = (int)step;
}

static void VERMOUTHCL voice_on(MIDIHDL hdl, CHANNEL ch, VOICE v, int key,
																	int vel) {

	INSTRUMENT	inst;
	INSTLAYER	layer;
	int			panpot;

	key &= 0x7f;
	if (!(ch->flag & CHANNEL_RHYTHM)) {
		inst = ch->inst;
		if (inst == NULL) {
			return;
		}
		if (inst->freq) {
			v->frequency = inst->freq;
		}
		else {
			v->frequency = freq_table[key];
		}
		layer = selectlayer(v, inst);
	}
	else {
#if !defined(MIDI_GMONLY)
		inst = ch->rhythm[key];
		if (inst == NULL) {
			inst = hdl->bank0[1][key];
		}
#else
		inst = hdl->bank0[1][key];
#endif
		if (inst == NULL) {
			return;
		}
		layer = (INSTLAYER)(inst + 1);
		if (inst->freq) {
			v->frequency = inst->freq;
		}
		else {
			v->frequency = freq_table[key];
		}
	}
	v->sample = layer;

	v->phase = VOICE_ON;
	v->channel = ch;
	v->note = key;
	v->velocity = vel;
	v->samppos = 0;
	v->sampstep = 0;

#if defined(ENABLE_TREMOLO)
	v->tremolo.count = 0;
	v->tremolo.step = layer->tremolo_step;
	v->tremolo.sweepstep = layer->tremolo_sweep;
	v->tremolo.sweepcount = 0;
#endif

#if defined(ENABLE_VIRLATE)
	v->vibrate.sweepstep = layer->vibrate_sweep;
	v->vibrate.sweepcount = 0;
	v->vibrate.rate = layer->vibrate_rate;
	v->vibrate.count = 0;
	v->vibrate.phase = 0;
#endif

	if (!(ch->flag & CHANNEL_RHYTHM)) {
		panpot = ch->panpot;
	}
	else {
		panpot = layer->panpot;
	}
#if defined(PANPOT_REVA)
	if (panpot == 64) {
		v->flag = VOICE_MIXCENTRE;
	}
	else if (panpot < 3) {
		v->flag = VOICE_MIXLEFT;
	}
	else if (panpot >= 126) {
		v->flag = VOICE_MIXRIGHT;
	}
	else {
		v->flag = VOICE_MIXNORMAL;
		v->panpot = panpot;
	}
#else
	if ((panpot >= 60) && (panpot < 68)) {
		v->flag = VOICE_MIXCENTRE;
	}
	else if (panpot < 5) {
		v->flag = VOICE_MIXLEFT;
	}
	else if (panpot >= 123) {
		v->flag = VOICE_MIXRIGHT;
	}
	else {
		v->flag = VOICE_MIXNORMAL;
		v->panpot = panpot;
	}
#endif
	if (!layer->samprate) {
		v->flag |= VOICE_FIXPITCH;
	}
	else {
		v->freq = (float)layer->samprate / (float)hdl->samprate *
					(float)v->frequency / (float)layer->freqroot;
	}
	voice_setphase(v, VOICE_ON);
	freq_update(v);
	voice_volupdate(v);
	v->envcount = 0;
	if (layer->mode & MODE_ENVELOPE) {
		v->envvol = 0;
		envlope_setphase(v, 0);
	}
	else {
		v->envstep = 0;
	}
	voice_setmix(v);
	envelope_updates(v);
}

static void VERMOUTHCL voice_off(VOICE v) {

	voice_setphase(v, VOICE_OFF);
	if (v->sample->mode & MODE_ENVELOPE) {
		envlope_setphase(v, 3);
		voice_setmix(v);
		envelope_updates(v);
	}
}

static void VERMOUTHCL allresetvoices(MIDIHDL hdl) {

	VOICE	v;
	VOICE	vterm;

	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		voice_setfree(v);
		v++;
	} while(v < vterm);
}


// ---- key

static void VERMOUTHCL key_on(MIDIHDL hdl, CHANNEL ch, int key, int vel) {

	VOICE	v;
	VOICE	v1;
	VOICE	v2;
	int		vol;
	int		volmin;

	v = NULL;
	v1 = VOICEHDLPTR(hdl);
	v2 = VOICEHDLEND(hdl);
	do {
		v2--;
		if (v2->phase == VOICE_FREE) {
			v = v2;
		}
		else if ((v2->channel == ch) &&
				((v2->note == key) || (ch->flag & CHANNEL_MONO))) {
			voice_setphase(v2, VOICE_REL);
			voice_setmix(v2);
		}
	} while(v1 < v2);

	if (v != NULL) {
		voice_on(hdl, ch, v, key, vel);
		return;
	}

	volmin = 0x7fffffff;
	v2 = VOICEHDLEND(hdl);
	do {
		v2--;
		if (!(v2->phase & (VOICE_ON | VOICE_REL))) {
			vol = v2->envleft;
			if ((v2->flag & VOICE_MIXMASK) == VOICE_MIXNORMAL) {
				vol = max(vol, v2->envright);
			}
			if (volmin > vol) {
				volmin = vol;
				v = v2;
			}
		}
	} while(v1 < v2);

	if (v != NULL) {
		voice_setfree(v);
		voice_on(hdl, ch, v, key, vel);
	}
}

static void VERMOUTHCL key_off(MIDIHDL hdl, CHANNEL ch, int key) {

	VOICE	v;
	VOICE	vterm;

	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		if ((v->phase & VOICE_ON) &&
			(v->channel == ch) && (v->note == key)) {
			if (ch->flag & CHANNEL_SUSTAIN) {
				voice_setphase(v, VOICE_SUSTAIN);
			}
			else {
				voice_off(v);
			}
			return;
		}
		v++;
	} while(v < vterm);
}

static void VERMOUTHCL key_pressure(MIDIHDL hdl, CHANNEL ch, int key,
																	int vel) {

	VOICE	v;
	VOICE	vterm;

	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		if ((v->phase & VOICE_ON) &&
			(v->channel == ch) && (v->note == key)) {
			v->velocity = vel;
			voice_volupdate(v);
			envelope_updates(v);
			break;
		}
		v++;
	} while(v < vterm);
}


// ---- control

static void VERMOUTHCL volumeupdate(MIDIHDL hdl, CHANNEL ch) {

	VOICE	v;
	VOICE	vterm;

#if defined(VOLUME_ACURVE)
	ch->level = (hdl->level * acurve[ch->volume] * ch->expression) >> 15;
#else
	ch->level = (hdl->level * ch->volume * ch->expression) >> 14;
#endif
	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		if ((v->phase & (VOICE_ON | VOICE_SUSTAIN)) && (v->channel == ch)) {
			voice_volupdate(v);
			envelope_updates(v);
		}
		v++;
	} while(v < vterm);
}

static void VERMOUTHCL pedaloff(MIDIHDL hdl, CHANNEL ch) {

	VOICE	v;
	VOICE	vterm;

	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		if ((v->phase & VOICE_SUSTAIN) && (v->channel == ch)) {
			voice_off(v);
		}
		v++;
	} while(v < vterm);
}

static void VERMOUTHCL allsoundsoff(MIDIHDL hdl, CHANNEL ch) {

	VOICE	v;
	VOICE	vterm;

	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		if ((v->phase != VOICE_FREE) && (v->channel == ch)) {
			voice_setphase(v, VOICE_REL);
			voice_setmix(v);
		}
		v++;
	} while(v < vterm);
}

static void VERMOUTHCL resetallcontrollers(CHANNEL ch) {

	ch->flag &= CHANNEL_MASK;
	if (ch->flag == 9) {
		ch->flag |= CHANNEL_RHYTHM;
	}
	ch->volume = 90;
	ch->expression = 127;
	ch->pitchbend = 0x2000;
	ch->pitchfactor = 1.0;
}

static void VERMOUTHCL allnotesoff(MIDIHDL hdl, CHANNEL ch) {

	VOICE	v;
	VOICE	vterm;

	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
#if 1
		if ((v->phase & (VOICE_ON | VOICE_SUSTAIN)) && (v->channel == ch)) {
			voice_off(v);
		}
#else
		if ((v->phase & VOICE_ON) && (v->channel == ch)) {
			if (ch->flag & CHANNEL_SUSTAIN) {
				voice_setphase(v, VOICE_SUSTAIN);
			}
			else {
				voice_off(v);
			}
		}
#endif
		v++;
	} while(v < vterm);
}

static void VERMOUTHCL ctrlchange(MIDIHDL hdl, CHANNEL ch, int ctrl,
																	int val) {

	val &= 0x7f;
	switch(ctrl & 0x7f) {
#if !defined(MIDI_GMONLY)
		case CTRL_PGBANK:
#if defined(ENABLE_GSRX)
			if (!(ch->gsrx[2] & GSRX2_BANKSELECT)) {
				break;
			}
#endif
			ch->bank = val;
			break;
#endif

		case CTRL_DATA_M:
//			TRACEOUT(("data: %x %x %x", c->rpn_l, c->rpn_m, val));
			if ((ch->rpn_l == 0) && (ch->rpn_m == 0)) {
				if (val >= 24) {
					val = 24;
				}
				ch->pitchsens = val;
			}
			break;

		case CTRL_VOLUME:
			ch->volume = val;
			volumeupdate(hdl, ch);
			break;

		case CTRL_PANPOT:
			ch->panpot = val;
			break;

		case CTRL_EXPRESS:
			ch->expression = val;
			volumeupdate(hdl, ch);
			break;

		case CTRL_PEDAL:
			if (val == 0) {
				ch->flag &= ~CHANNEL_SUSTAIN;
				pedaloff(hdl, ch);
			}
			else {
				ch->flag |= CHANNEL_SUSTAIN;
			}
			break;

		case CTRL_RPN_L:
			ch->rpn_l = val;
			break;

		case CTRL_RPN_M:
			ch->rpn_m = val;
			break;

		case CTRL_SOUNDOFF:
			allsoundsoff(hdl, ch);
			break;

		case CTRL_RESETCTRL:
			resetallcontrollers(ch);
			/*FALLTHROUGH*/

		case CTRL_NOTEOFF:
			allnotesoff(hdl, ch);
			break;

		case CTRL_MONOON:
			ch->flag |= CHANNEL_MONO;
			break;

		case CTRL_POLYON:
			ch->flag &= ~CHANNEL_MONO;
			break;

		default:
//			TRACEOUT(("ctrl: %x %x", ctrl, val);
			break;
	}
}

static void VERMOUTHCL progchange(MIDIHDL hdl, CHANNEL ch, int val) {

#if !defined(MIDI_GMONLY)
	MIDIMOD		mod;
	INSTRUMENT	*bank;
	INSTRUMENT	inst;

	mod = hdl->module;
	inst = NULL;
	if (ch->bank < MIDI_BANKS) {
		bank = mod->tone[ch->bank * 2];
		if (bank) {
			inst = bank[val];
		}
	}
	if (inst == NULL) {
		bank = hdl->bank0[0];
		inst = bank[val];
	}
	ch->inst = inst;

	bank = NULL;
	if (ch->bank < MIDI_BANKS) {
		bank = mod->tone[ch->bank * 2 + 1];
	}
	if (bank == NULL) {
		bank = hdl->bank0[1];
	}
	ch->rhythm = bank;
#else
	ch->inst = hdl->bank0[0][val];
#endif
	ch->program = val;
}

static void VERMOUTHCL chpressure(MIDIHDL hdl, CHANNEL ch, int vel) {

	VOICE	v;
	VOICE	vterm;

	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		if ((v->phase & VOICE_ON) && (v->channel == ch)) {
			v->velocity = vel;
			voice_volupdate(v);
			envelope_updates(v);
			break;
		}
		v++;
	} while(v < vterm);
}

static void VERMOUTHCL pitchbendor(MIDIHDL hdl, CHANNEL ch, int val1,
																int val2) {

	VOICE	v;
	VOICE	vterm;

	val1 &= 0x7f;
	val1 += (val2 & 0x7f) << 7;
	if (1) {
		ch->pitchbend = val1;
		val1 -= 0x2000;
		if (!val1) {
			ch->pitchfactor = 1.0;
		}
		else {
			val1 *= ch->pitchsens;
			ch->pitchfactor = bendhtbl[(val1 >> (6 + 7)) + 24] *
												bendltbl[(val1 >> 7) & 0x3f];
		}
		v = VOICEHDLPTR(hdl);
		vterm = VOICEHDLEND(hdl);
		do {
			if ((v->phase != VOICE_FREE) && (v->channel == ch)) {
				freq_update(v);
			}
			v++;
		} while(v < vterm);
	}
}

static void VERMOUTHCL allvolupdate(MIDIHDL hdl) {

	int		level;
	CHANNEL	ch;
	CHANNEL	chterm;
	VOICE	v;
	VOICE	vterm;

	level = gaintbl[hdl->gain + 16] >> 1;
	level *= hdl->master;
	hdl->level = level;
	ch = hdl->channel;
	chterm = ch + CHANNEL_MAX;
	do {
		ch->level = (level * ch->volume * ch->expression) >> 14;
		ch++;
	} while(ch < chterm);
	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		if (v->phase & (VOICE_ON | VOICE_SUSTAIN)) {
			voice_volupdate(v);
			envelope_updates(v);
		}
		v++;
	} while(v < vterm);
}

#if defined(ENABLE_GSRX)
static void VERMOUTHCL allresetmidi(MIDIHDL hdl, BOOL gs)
#else
#define allresetmidi(m, g)		_allresetmidi(m)
static void VERMOUTHCL _allresetmidi(MIDIHDL hdl)
#endif
{
	CHANNEL	ch;
	CHANNEL	chterm;
	UINT	flag;

	hdl->master = 127;
	ch = hdl->channel;
	chterm = ch + CHANNEL_MAX;
	ZeroMemory(ch, sizeof(_CHANNEL) * CHANNEL_MAX);
	flag = 0;
	do {
		ch->flag = flag++;
		ch->pitchsens = 2;
#if !defined(MIDI_GMONLY)
		ch->bank = 0;
#endif
		ch->panpot = 64;
		progchange(hdl, ch, 0);
		resetallcontrollers(ch);
#if defined(ENABLE_GSRX)
		ch->keyshift = 0x40;
		ch->noterange[0] = 0x00;
		ch->noterange[1] = 0x7f;
		if (gs) {
			ch->gsrx[0] = 0xff;
			ch->gsrx[1] = 0xff;
			ch->gsrx[2] = 0xff;
		}
		else {
			ch->gsrx[0] = 0x7f;
			ch->gsrx[1] = 0xff;
			ch->gsrx[2] = 0x02;
		}
#endif
		ch++;
	} while(ch < chterm);
	allresetvoices(hdl);
	allvolupdate(hdl);
}


// ----

VEXTERN UINT VEXPORT midiout_getver(char *string, UINT leng) {

	leng = min(leng, sizeof(vermouthver));
	CopyMemory(string, vermouthver, leng);
	return((MIDIOUT_VERSION << 8) | 0x00);
}

VEXTERN MIDIHDL VEXPORT midiout_create(MIDIMOD mod, UINT worksize) {

	UINT	size;
	MIDIHDL	ret;

	if (mod == NULL) {
		return(NULL);
	}
	worksize = min(worksize, 512U);
	worksize = max(worksize, 16384U);
	size = sizeof(_MIDIHDL);
	size += sizeof(SINT32) * 2 * worksize;
	size += sizeof(_SAMPLE) * worksize;
	ret = (MIDIHDL)_MALLOC(size, "MIDIHDL");
	if (ret) {
		midimod_lock(mod);
		ZeroMemory(ret, size);
		ret->samprate = mod->samprate;
		ret->worksize = worksize;
		ret->module = mod;
	//	ret->master = 127;
		ret->bank0[0] = mod->tone[0];
		ret->bank0[1] = mod->tone[1];
		ret->sampbuf = (SINT32 *)(ret + 1);
		ret->resampbuf = (SAMPLE)(ret->sampbuf + worksize * 2);
		allresetmidi(ret, FALSE);
	}
	return(ret);
}

VEXTERN void VEXPORT midiout_destroy(MIDIHDL hdl) {

	MIDIMOD mod;

	if (hdl) {
		mod = hdl->module;
		_MFREE(hdl);
		midimod_lock(mod);
	}
}

VEXTERN void VEXPORT midiout_shortmsg(MIDIHDL hdl, UINT32 msg) {

	UINT	cmd;
	CHANNEL	ch;

	if (hdl == NULL) {
		return;
	}
	cmd = msg & 0xff;
	if (cmd & 0x80) {
		hdl->status = cmd;
	}
	else {
		msg <<= 8;
		msg += hdl->status;
	}
	ch = hdl->channel + (cmd & 0x0f);
	switch((cmd >> 4) & 7) {
		case (MIDI_NOTE_OFF >> 4) & 7:
			key_off(hdl, ch, (msg >> 8) & 0x7f);
			break;

		case (MIDI_NOTE_ON >> 4) & 7:
			if (msg & (0x7f << 16)) {
				key_on(hdl, ch, (msg >> 8) & 0x7f, (msg >> 16) & 0x7f);
			}
			else {
				key_off(hdl, ch, (msg >> 8) & 0x7f);
			}
			break;

		case (MIDI_KEYPRESS >> 4) & 7:
			key_pressure(hdl, ch, (msg >> 8) & 0x7f, (msg >> 16) & 0x7f);
			break;

		case (MIDI_CTRLCHANGE >> 4) & 7:
			ctrlchange(hdl, ch, (msg >> 8) & 0x7f, (msg >> 16) & 0x7f);
			break;

		case (MIDI_PROGCHANGE >> 4) & 7:
			progchange(hdl, ch, (msg >> 8) & 0x7f);
			break;

		case (MIDI_CHPRESS >> 4) & 7:
			chpressure(hdl, ch, (msg >> 8) & 0x7f);
			break;

		case (MIDI_PITCHBEND >> 4) & 7:
			pitchbendor(hdl, ch, (msg >> 8) & 0x7f, (msg >> 16) & 0x7f);
			break;
	}
}

static void VERMOUTHCL longmsg_uni(MIDIHDL hdl, const UINT8 *msg, UINT size) {

	if ((size >= 6) && (msg[2] == 0x7f)) {
		switch(msg[3]) {
			case 0x04:
				if ((msg[4] == 0x01) && (size >= 8)) {
					hdl->master = msg[6] & 0x7f;
					allvolupdate(hdl);
				}
				break;
		}
	}
}

static void VERMOUTHCL longmsg_gm(MIDIHDL hdl, const UINT8 *msg, UINT size) {

	if ((size >= 6) && (msg[2] == 0x7f)) {
		switch(msg[3]) {
			case 0x09:
				if (msg[4] == 0x01) {
					allresetmidi(hdl, FALSE);					// GM reset
					break;
				}
#if !defined(MIDI_GMONLY)
				else if ((msg[4] == 0x02) || (msg[4] == 0x03)) {
					allresetmidi(hdl, TRUE);					// GM reset
					break;
				}
#endif
				break;
		}
	}
}

static void VERMOUTHCL rolandcmd4(MIDIHDL hdl, UINT addr, UINT8 data) {

	UINT	part;
	CHANNEL	ch;
#if defined(ENABLE_GSRX)
	UINT8	bit;
#endif

	addr = addr & 0x000fffff;
	if (addr == 0x00004) {				// Vol
		hdl->master = data;
		allvolupdate(hdl);
	}
	else if ((addr & (~0xff)) == 0x00100) {
		const UINT pos = addr & 0xff;
		if (pos < 0x30) {		// Patch Name
		}
		else {
			switch(addr & 0xff) {
				case 0x30:		// Reverb Macro
				case 0x31:		// Reverb Charactor
				case 0x32:		// Reverb Pre-LPF
				case 0x33:		// Reverb Level
				case 0x34:		// Reverb Time
				case 0x35:		// Reverb Delay FeedBack
				case 0x37:		// Reverb Predelay Time
				case 0x38:		// Chorus Macro
				case 0x39:		// Chorus Pre-LPF
				case 0x3a:		// Chorus Level
				case 0x3b:		// Chorus FeedBack
				case 0x3c:		// Chorus Delay
				case 0x3d:		// Chorus Rate
				case 0x3e:		// Chorus Depth
				case 0x3f:		// Chorus send level to reverb
				case 0x40:		// Chorus send level to delay
				case 0x50:		// Delay Macro
				case 0x51:		// Delay Time Pre-LPF
				case 0x52:		// Delay Time Center
				case 0x53:		// Delay Time Ratio Left
				case 0x54:		// Delay Time Ratio Right
				case 0x55:		// Delay Level Center
				case 0x56:		// Delay Level Left
				case 0x57:		// Delay Level Right
				case 0x58:		// Delay Level
				case 0x59:		// Delay Freeback
				case 0x5a:		// Delay sendlevel to Reverb
					break;
			}
		}
	}
	else if ((addr & (~(0x0fff))) == 0x01000) {	// GS CH
		part = (addr >> 8) & 0x0f;
		if (part == 0) {						// part10
			part = 9;
		}
		else if (part < 10) {					// part1-9
			part--;
		}
		ch = hdl->channel + part;
		switch(addr & 0xff) {
#if !defined(MIDI_GMONLY)
			case 0x00:							// TONE NUMBER
				ch->bank = data;
				break;
#endif

			case 0x01:							// PROGRAM NUMBER
				progchange(hdl, ch, data);
				break;

			case 0x02:							// Rx.CHANNEL
				TRACEOUT(("RxCHANNEL: %d", data));
				break;

#if defined(ENABLE_GSRX)
			case 0x03:							// Rx.PITCHBEND
			case 0x04:							// Rx.CH PRESSURE
			case 0x05:							// Rx.PROGRAM CHANGE
			case 0x06:							// Rx.CONTROL CHANGE
			case 0x07:							// Rx.POLY PRESSURE
			case 0x08:							// Rx.NOTE MESSAGE
			case 0x09:							// Rx.PRN
			case 0x0a:							// Rx.NRPN
				bit = 1 << ((addr - 0x03) & 7);
				if (data == 0) {
					ch->gsrx[0] = ch->gsrx[0] & (~bit);
				}
				else if (data == 1) {
					ch->gsrx[0] = ch->gsrx[0] | bit;
				}
				break;

			case 0x0b:							// Rx.MODULATION
			case 0x0c:							// Rx.VOLUME
			case 0x0d:							// Rx.PANPOT
			case 0x0e:							// Rx.EXPRESSION
			case 0x0f:							// Rx.HOLD1
			case 0x10:							// Rx.PORTAMENTO
			case 0x11:							// Rx.SOSTENUTO
			case 0x12:							// Rx.SOFT
				bit = 1 << ((addr - 0x0b) & 7);
				if (data == 0) {
					ch->gsrx[1] = ch->gsrx[1] & (~bit);
				}
				else if (data == 1) {
					ch->gsrx[1] = ch->gsrx[1] | bit;
				}
				break;
#endif
			case 0x15:							// USE FOR RHYTHM PART
				if (data == 0) {
					ch->flag &= ~CHANNEL_RHYTHM;
					TRACEOUT(("ch%d - tone", part + 1));
				}
				else if ((data == 1) || (data == 2)) {
					ch->flag |= CHANNEL_RHYTHM;
					TRACEOUT(("ch%d - rhythm", part + 1));
				}
				break;

#if defined(ENABLE_GSRX)
			case 0x16:							// PITCH KEY SHIFT
				if ((data >= 0x28) && (data <= 0x58)) {
					ch->keyshift = data;
				}
				break;

			case 0x1d:							// KEYBOARD RANGE LOW
				ch->noterange[0] = data;
				break;

			case 0x1e:							// KEYBOARD RANGE HIGH
				ch->noterange[1] = data;
				break;

			case 0x23:							// Rx.BANK SELECT
			case 0x24:							// Rx.BANK SELECT LSB
				bit = 1 << ((addr - 0x23) & 7);
				if (data == 0) {
					ch->gsrx[2] = ch->gsrx[2] & (~bit);
				}
				else if (data == 1) {
					ch->gsrx[2] = ch->gsrx[2] | bit;
				}
				break;
#endif
			default:
				TRACEOUT(("Roland GS - %.6x %.2x", addr, data));
				break;
		}
	}
	else {
		TRACEOUT(("Roland GS - %.6x %.2x", addr, data));
	}
}

static void VERMOUTHCL longmsg_roland(MIDIHDL hdl, const UINT8 *msg,
																UINT size) {

	UINT	addr;
	UINT8	data;

	if (size <= 10) {
		return;
	}
	// GS data set
	if ((msg[2] != 0x10) || (msg[3] != 0x42) || (msg[4] != 0x12)) {
		return;
	}
	addr = (msg[5] << 16) + (msg[6] << 8) + msg[7];
	msg += 8;
	size -= 10;
	while(size) {
		size--;
		data = (*msg++) & 0x7f;
		if ((addr & (~0x400000)) == 0x7f) {			// GS reset
			allresetmidi(hdl, TRUE);
			TRACEOUT(("GS-Reset"));
		}
		else if ((addr & 0xfff00000) == 0x00400000) {
			rolandcmd4(hdl, addr, data);
		}
#if defined(ENABLE_PORTB)
		else if ((addr & 0xfff00000) == 0x00500000) {
			if (hdl->portb) {
				rolandcmd4(hdl->portb, addr, data);
			}
		}
#endif	// defined(ENABLE_PORTB)
		addr++;
	}
}

VEXTERN void VEXPORT midiout_longmsg(MIDIHDL hdl, const UINT8 *msg, UINT size) {

	UINT	id;

	if ((hdl == NULL) || (msg == NULL)) {
		return;
	}
	if (size > 3) {							// (msg[size - 1] == 0xf7)
		id = msg[1];
		if (id == 0x7f) {					// Universal realtime
			longmsg_uni(hdl, msg, size);
		}
		else if (id == 0x7e) {				// GM
			longmsg_gm(hdl, msg, size);
		}
		else if (id == 0x41) {				// Roland
			longmsg_roland(hdl, msg, size);
		}
		else {
			TRACEOUT(("long msg unknown id:%02x", id));
		}
	}
}

static UINT	VERMOUTHCL preparepcm(MIDIHDL hdl, UINT size) {

	UINT	ret;
	SINT32	*buf;
	VOICE	v;
	VOICE	vterm;
	SAMPLE	src;
	SAMPLE	srcterm;
	UINT	cnt;
	UINT	pos;
	UINT	rem;

	ret = 0;
	size = min(size, hdl->worksize);
	buf = hdl->sampbuf;
	ZeroMemory(buf, size * 2 * sizeof(SINT32));
	v = VOICEHDLPTR(hdl);
	vterm = VOICEHDLEND(hdl);
	do {
		if (v->phase != VOICE_FREE) {
			ret = size;
			cnt = size;
			if (v->phase & VOICE_REL) {
				voice_setfree(v);
				if (cnt > REL_COUNT) {
					cnt = REL_COUNT;
				}
			}
			if (v->flag & VOICE_FIXPITCH) {
				pos = v->samppos >> FREQ_SHIFT;
				src = v->sample->data + pos;
				rem = (v->sample->datasize >> FREQ_SHIFT) - pos;
				if (cnt < rem) {
					v->samppos += cnt << FREQ_SHIFT;
					srcterm = src + cnt;
				}
				else {
					voice_setfree(v);
					srcterm = src + rem;
				}
			}
			else {
				src = hdl->resampbuf;
				srcterm = v->resamp(v, src, src + cnt);
			}
			if (src != srcterm) {
				v->mix(v, buf, src, srcterm);
			}
		}
		v++;
	} while(v < vterm);
	return(ret);
}

VEXTERN const SINT32 * VEXPORT midiout_get(MIDIHDL hdl, UINT *samples) {

	UINT	size;
	SINT32	*buf;
	SINT32	*bufterm;

	if ((hdl == NULL) || (samples == NULL)) {
		goto moget_err;
	}
	size = *samples;
	if (size == 0) {
		goto moget_err;
	}
	size = preparepcm(hdl, size);
	if (size == 0) {
		goto moget_err;
	}

	*samples = size;
	buf = hdl->sampbuf;
	bufterm = buf + (size * 2);
	do {
		buf[0] >>= (SAMP_SHIFT + 1);
		buf[1] >>= (SAMP_SHIFT + 1);
		buf += 2;
	} while(buf < bufterm);
	return(hdl->sampbuf);

moget_err:
	return(NULL);
}

VEXTERN UINT VEXPORT midiout_get16(MIDIHDL hdl, SINT16 *pcm, UINT size) {

	UINT	step;
	SINT32	*buf;
	SINT32	l;
	SINT32	r;

	if (hdl != NULL) {
		while(size) {
			step = preparepcm(hdl, size);
			if (step == 0) {
				break;
			}
			size -= step;
			buf = hdl->sampbuf;
			do {
				l = pcm[0];
				r = pcm[1];
				l += buf[0] >> (SAMP_SHIFT + 1);
				r += buf[1] >> (SAMP_SHIFT + 1);
				if (l < -32768) {
					l = -32768;
				}
				else if (l > 32767) {
					l = 32767;
				}
				if (r < -32768) {
					r = -32768;
				}
				else if (r > 32767) {
					r = 32767;
				}
				pcm[0] = l;
				pcm[1] = r;
				buf += 2;
				pcm += 2;
			} while(--step);
		}
	}
	return(0);
}

VEXTERN UINT VEXPORT midiout_get32(MIDIHDL hdl, SINT32 *pcm, UINT size) {

	UINT	step;
	SINT32	*buf;

	if (hdl != NULL) {
		while(size) {
			step = preparepcm(hdl, size);
			if (step == 0) {
				break;
			}
			size -= step;
			buf = hdl->sampbuf;
			do {
				pcm[0] += buf[0] >> (SAMP_SHIFT + 1);
				pcm[1] += buf[1] >> (SAMP_SHIFT + 1);
				buf += 2;
				pcm += 2;
			} while(--step);
		}
	}
	return(0);
}

VEXTERN void VEXPORT midiout_setgain(MIDIHDL hdl, int gain) {

	if (hdl) {
		if (gain < -16) {
			gain = 16;
		}
		else if (gain > 8) {
			gain = 8;
		}
		hdl->gain = (SINT8)gain;
		allvolupdate(hdl);
	}
}

VEXTERN void VEXPORT midiout_setmoduleid(MIDIHDL hdl, UINT8 moduleid) {

	if (hdl) {
		hdl->moduleid = moduleid;
	}
}

VEXTERN void VEXPORT midiout_setportb(MIDIHDL hdl, MIDIHDL portb) {

#if defined(ENABLE_PORTB)
	if (hdl) {
		hdl->portb = portb;
	}
#endif	// defined(ENABLE_PORTB)
}

