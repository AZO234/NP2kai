#include "compiler.h"

#include "np2.h"
#include "commng.h"
#include "kdispwin.h"
#include "mimpidef.h"
#include "sound.h"

#include "sound/vermouth/vermouth.h"

#if defined(VERMOUTH_LIB)
extern MIDIMOD vermouth_module;

const char cmmidi_vermouth[] = "VERMOUTH";
#endif

#define	MIDI_RESETWAIT	80000

const char cmmidi_midiout_device[] = "MIDI-OUT device";
const char cmmidi_midiin_device[] = "MIDI-IN device";

#define MIDIOUTS(a, b, c)	(((c) << 16) + (b << 8) + (a))
#define MIDIOUTS2(a)		((a)[0] + ((a)[1] << 8))
#define MIDIOUTS3(a)		((a)[0] + ((a)[1] << 8) + ((a)[2] << 16))

const char *cmmidi_mdlname[] = {
	"MT-32",	"CM-32L",	"CM-64",
	"CM-300",	"CM-500(LA)",	"CM-500(GS)",
	"SC-55",	"SC-88",	"LA",
	"GM",		"GS",		"XG"
};

static const UINT8 EXCV_MTRESET[] = {
	0xfe, 0xfe, 0xfe
};
static const UINT8 EXCV_GMRESET[] = {
	0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7
};
static const UINT8 EXCV_GSRESET[] = {
	0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7
};
static const UINT8 EXCV_XGRESET[] = {
	0xf0, 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00, 0xf7
};
static const UINT8 EXCV_GMVOLUME[] = {
	0xf0, 0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00, 0xf7
};

enum {
	MIDI_EXCLUSIVE		= 0xf0,
	MIDI_TIMECODE		= 0xf1,
	MIDI_SONGPOS		= 0xf2,
	MIDI_SONGSELECT		= 0xf3,
	MIDI_TUNEREQUEST	= 0xf6,
	MIDI_EOX		= 0xf7,
	MIDI_TIMING		= 0xf8,
	MIDI_START		= 0xfa,
	MIDI_CONTINUE		= 0xfb,
	MIDI_STOP		= 0xfc,
	MIDI_ACTIVESENSE	= 0xfe,
	MIDI_SYSTEMRESET	= 0xff
};

enum {
	MIDI_BUFFER		= (1 << 10),
	MIDIIN_MAX		= 4,

	CMMIDI_MIDIOUT		= 0x01,
	CMMIDI_MIDIIN		= 0x02,
	CMMIDI_MIDIINSTART	= 0x04,
	CMMIDI_VERMOUTH		= 0x08,

	MIDICTRL_READY		= 0,
	MIDICTRL_2BYTES,
	MIDICTRL_3BYTES,
	MIDICTRL_EXCLUSIVE,
	MIDICTRL_TIMECODE,
	MIDICTRL_SYSTEM
};

struct _cmmidi;
typedef struct _cmmidi	_CMMIDI;
typedef struct _cmmidi	*CMMIDI;

typedef struct {
	UINT8	prog;
	UINT8	press;
	UINT16	bend;
	UINT8	ctrl[28];
} _MIDICH, *MIDICH;

struct _cmmidi {
	int		opened;
	void		(*outfn)(CMMIDI self, UINT32 msg, UINT cnt);
	int		hmidiin;
	int		hmidiout;
	struct timeval	hmidiout_nextstart;
	MIDIHDL		vermouth;

	UINT		midictrl;
	UINT		midisyscnt;
	UINT		mpos;

	UINT8		midilast;
	UINT8		midiexcvwait;
	UINT8		midimodule;

	UINT8		buffer[MIDI_BUFFER];
	_MIDICH		mch[16];
	UINT8		excvbuf[MIDI_BUFFER];

	UINT8		def_en;
	MIMPIDEF	def;

	UINT		recvpos;
	UINT		recvsize;
	UINT8		recvbuf[MIDI_BUFFER];
	UINT8		midiinbuf[MIDI_BUFFER];
};

static const UINT8 midictrltbl[] = {
	0, 1, 5, 7, 10, 11, 64,
	65, 66, 67, 84, 91, 93,
	94,			// for SC-88
	71, 72, 73, 74		// for XG
};

static UINT8 midictrlindex[128];


// ----

static int
getmidiout(const char *midiout)
{
	int hmidiout = -1;

	if (midiout && midiout[0] != '\0') {
		if ((!milstr_cmp(midiout, cmmidi_midiout_device))
		 && (np2oscfg.MIDIDEV[0][0] != '\0')) {
			hmidiout = open(np2oscfg.MIDIDEV[0], O_WRONLY | O_NONBLOCK);
			if (hmidiout < 0) {
				perror("getmidiout");
			}
		}
	}
	return hmidiout;
}

static int
getmidiin(const char *midiin)
{
	int hmidiin = -1;

	if (midiin && midiin[0] != '\0') {
		if ((!milstr_cmp(midiin, cmmidi_midiin_device))
		 && (np2oscfg.MIDIDEV[1][0] != '\0')) {
			hmidiin = open(np2oscfg.MIDIDEV[1], O_RDONLY | O_NONBLOCK);
			if (hmidiin < 0) {
				perror("getmidiin");
			}
		}
	}
	return hmidiin;
}

static UINT
module2number(const char *module)
{
	int i;

	for (i = 0; i < NELEMENTS(cmmidi_mdlname); i++) {
		if (milstr_extendcmp(module, cmmidi_mdlname[i])) {
			return i;
		}
	}
	return MIDI_OTHER;
}

static INLINE void 
midi_write(CMMIDI midi, const UINT8 *cmd, UINT cnt)
{
	struct timeval ct;
	int ds, du;
	int rv;
	UINT i;

	for (;;) {
		gettimeofday(&ct, NULL);
		ds = ct.tv_sec - midi->hmidiout_nextstart.tv_sec;
		if (ds > 0)
			break;
		du = ct.tv_usec - midi->hmidiout_nextstart.tv_usec;
		if ((ds == 0) && (du >= 0))
			break;
	}

	for (i = 0; i < cnt; i++) {
		do {
			rv = write(midi->hmidiout, cmd + i, 1);
		} while (rv != 1);
	}

	gettimeofday(&midi->hmidiout_nextstart, NULL);
	midi->hmidiout_nextstart.tv_usec += np2oscfg.MIDIWAIT * cnt;
	if ((memcmp(cmd, EXCV_GMRESET, sizeof(EXCV_GMRESET)) == 0)
	 || (memcmp(cmd, EXCV_GSRESET, sizeof(EXCV_GSRESET)) == 0)
	 || (memcmp(cmd, EXCV_XGRESET, sizeof(EXCV_XGRESET)) == 0)
	 || (memcmp(cmd, EXCV_MTRESET, sizeof(EXCV_MTRESET)) == 0)) {
		midi->hmidiout_nextstart.tv_usec += MIDI_RESETWAIT;
	}
	while (midi->hmidiout_nextstart.tv_usec >= 1000000) {
		midi->hmidiout_nextstart.tv_usec -= 1000000;
		midi->hmidiout_nextstart.tv_sec++;
	}
}

#if 1
#define	waitlastexclusiveout(midip)	midip->midiexcvwait = 0
#else
static void
waitlastexclusiveout(CMMIDI midi)
{
	struct timeval ct;
	int ds, du;

	if (midi->midiexcvwait) {
		for (;;) {
			gettimeofday(&ct, NULL);
			ds = ct.tv_sec - midi->hmidiout_nextstart.tv_sec;
			if (ds > 0)
				break;
			du = ct.tv_usec - midi->hmidiout_nextstart.tv_usec;
			if ((ds == 0) && (du >= 0))
				break;
		}
		midi->midiexcvwait = 0;
	}
}
#endif

static void
sendexclusive(CMMIDI midi, const UINT8 *buf, UINT leng)
{

	CopyMemory(midi->excvbuf, buf, leng);
	midi_write(midi, midi->excvbuf, leng);
	midi->midiexcvwait = 1;
}

static void
midiout_none(CMMIDI midi, UINT32 msg, UINT cnt)
{

	/* Nothing to do */
}

static void
midiout_device(CMMIDI midi, UINT32 msg, UINT cnt)
{
	UINT8 buf[3];
	UINT i;

	for (i = 0; i < cnt; i++, msg >>= 8) {
		buf[i] = msg & 0xff;
	}
	waitlastexclusiveout(midi);
	midi_write(midi, buf, cnt);
}

#if defined(VERMOUTH_LIB)
static void
midiout_vermouth(CMMIDI midi, UINT32 msg, UINT cnt)
{

	sound_sync();
	midiout_shortmsg(midi->vermouth, msg);
}

static void SOUNDCALL
vermouth_getpcm(MIDIHDL hdl, SINT32 *pcm, UINT count)
{
	const SINT32 *ptr;
	UINT size;

	while (count) {
		size = count;
		ptr = midiout_get(hdl, &size);
		if (ptr == NULL) {
			break;
		}
		count -= size;
		do {
			pcm[0] += ptr[0];
			pcm[1] += ptr[1];
			ptr += 2;
			pcm += 2;
		} while (--size);
	}
}
#endif

static void
midireset(CMMIDI midi)
{
	const UINT8 *excv;
	UINT excvsize;
	UINT8 work[4];

	switch (midi->midimodule) {
	case MIDI_GM:
		excv = EXCV_GMRESET;
		excvsize = sizeof(EXCV_GMRESET);
		break;

	case MIDI_CM300:
	case MIDI_CM500GS:
	case MIDI_SC55:
	case MIDI_SC88:
	case MIDI_GS:
		excv = EXCV_GSRESET;
		excvsize = sizeof(EXCV_GSRESET);
		break;

	case MIDI_XG:
		excv = EXCV_XGRESET;
		excvsize = sizeof(EXCV_XGRESET);
		break;

	case MIDI_MT32:
	case MIDI_CM32L:
	case MIDI_CM64:
	case MIDI_CM500LA:
	case MIDI_LA:
		excv = EXCV_MTRESET;
		excvsize = sizeof(EXCV_MTRESET);
		break;

	default:
		excv = NULL;
		excvsize = 0;
		break;
	}
	if (excv) {
		if (midi->opened & CMMIDI_MIDIOUT) {
			waitlastexclusiveout(midi);
			sendexclusive(midi, excv, excvsize);
		}
#if defined(VERMOUTH_LIB)
		else if (midi->opened & CMMIDI_VERMOUTH) {
			midiout_longmsg(midi->vermouth, excv, excvsize);
		}
#endif
	}

	work[1] = 0x7b;
	work[2] = 0x00;
	for (work[0] = 0xb0; work[0] < 0xc0; work[0]++) {
		keydisp_midi(work);
		(*midi->outfn)(midi, MIDIOUTS3(work), 3);
	}
}

static void
midisetparam(CMMIDI midi)
{
	MIDICH mch;
	UINT i, j;

	mch = midi->mch;
	sound_sync();
	for (i = 0; i < 16; i++, mch++) {
		if (mch->press != 0xff) {
			(*midi->outfn)(midi, MIDIOUTS(0xa0+i, mch->press, 0),3);
		}
		if (mch->bend != 0xffff) {
			(*midi->outfn)(midi, (mch->bend << 8) + 0xe0+i, 3);
		}
		for (j = 0; j < sizeof(midictrltbl) / sizeof(UINT8); j++) {
			if (mch->ctrl[j+1] != 0xff) {
				(*midi->outfn)(midi, MIDIOUTS(0xb0+i, midictrltbl[j], mch->ctrl[j+1]), 3);
			}
		}
		if (mch->prog != 0xff) {
			(*midi->outfn)(midi, MIDIOUTS(0xc0+i, mch->prog, 0), 3);
		}
	}
}


// ----

static UINT
midiread(COMMNG self, UINT8 *data)
{
	CMMIDI midi = (CMMIDI)(self + 1);

	if (midi->recvsize > 0) {
		midi->recvsize--;
		*data = midi->recvbuf[midi->recvpos];
		midi->recvpos = (midi->recvpos + 1) & (MIDI_BUFFER - 1);
		return 1;
	}
	return 0;
}

static UINT
midiwrite(COMMNG self, UINT8 data)
{
	CMMIDI midi;
	MIDICH mch;
	int type;

	midi = (CMMIDI)(self + 1);
	switch (data) {
	case MIDI_TIMING:
	case MIDI_START:
	case MIDI_CONTINUE:
	case MIDI_STOP:
	case MIDI_ACTIVESENSE:
	case MIDI_SYSTEMRESET:
		return 1;
	}
	if (midi->midictrl == MIDICTRL_READY) {
		if (data & 0x80) {
			midi->mpos = 0;
			switch (data & 0xf0) {
			case 0xc0:
			case 0xd0:
				midi->midictrl = MIDICTRL_2BYTES;
				break;

			case 0x80:
			case 0x90:
			case 0xa0:
			case 0xb0:
			case 0xe0:
				midi->midictrl = MIDICTRL_3BYTES;
				midi->midilast = data;
				break;

			default:
				switch (data) {
				case MIDI_EXCLUSIVE:
					midi->midictrl = MIDICTRL_EXCLUSIVE;
					break;

				case MIDI_TIMECODE:
					midi->midictrl = MIDICTRL_TIMECODE;
					break;

				case MIDI_SONGPOS:
					midi->midictrl = MIDICTRL_SYSTEM;
					midi->midisyscnt = 3;
					break;

				case MIDI_SONGSELECT:
					midi->midictrl = MIDICTRL_SYSTEM;
					midi->midisyscnt = 2;
					break;

				case MIDI_TUNEREQUEST:
					midi->midictrl = MIDICTRL_SYSTEM;
					midi->midisyscnt = 1;
					break;

#if 0
				case MIDI_EOX:
#endif
				default:
					return(1);
				}
				break;
			}
		} else { /* Key-onのみな気がしたんだけど忘れた… */
			/* running status */
			midi->buffer[0] = midi->midilast;
			midi->mpos = 1;
			midi->midictrl = MIDICTRL_3BYTES;
		}
	}
	midi->buffer[midi->mpos] = data;
	midi->mpos++;

	switch (midi->midictrl) {
	case MIDICTRL_2BYTES:
		if (midi->mpos >= 2) {
			midi->buffer[1] &= 0x7f;
			mch = midi->mch + (midi->buffer[0] & 0xf);
			switch (midi->buffer[0] & 0xf0) {
			case 0xa0:
				mch->press = midi->buffer[1];
				break;

			case 0xc0:
				if (midi->def_en) {
					type = midi->def.ch[midi->buffer[0] & 0x0f];
					if (type < MIMPI_RHYTHM) {
						midi->buffer[1] = midi->def.map[type][midi->buffer[1]];
					}
				}
				mch->prog = midi->buffer[1];
				break;
			}
			keydisp_midi(midi->buffer);
			(*midi->outfn)(midi, MIDIOUTS2(midi->buffer), 2);
			midi->midictrl = MIDICTRL_READY;
			return 2;
		}
		break;

	case MIDICTRL_3BYTES:
		if (midi->mpos >= 3) {
			midi->buffer[1] &= 0x7f;
			midi->buffer[2] &= 0x7f;
			mch = midi->mch + (midi->buffer[0] & 0xf);
			switch (midi->buffer[0] & 0xf0) {
			case 0xb0:
				if (midi->buffer[1] == 123) {
					mch->press = 0;
					mch->bend = 0x4000;
					mch->ctrl[1+1] = 0;	// Modulation
					mch->ctrl[5+1] = 127;	// Explession
					mch->ctrl[6+1] = 0;	// Hold
					mch->ctrl[7+1] = 0;	// Portament
					mch->ctrl[8+1] = 0;	// Sostenute
					mch->ctrl[9+1] = 0;	// Soft
				} else {
					mch->ctrl[midictrlindex[midi->buffer[1]]] = midi->buffer[2];
				}
				break;

			case 0xe0:
				mch->bend = LOADINTELWORD(midi->buffer + 1);
				break;
			}
			keydisp_midi(midi->buffer);
			(*midi->outfn)(midi, MIDIOUTS3(midi->buffer), 3);
			midi->midictrl = MIDICTRL_READY;
			return 3;
		}
		break;

	case MIDICTRL_EXCLUSIVE:
		if (data == MIDI_EOX) {
			if (midi->opened & CMMIDI_MIDIOUT) {
				waitlastexclusiveout(midi);
				sendexclusive(midi, midi->buffer, midi->mpos);
			}
#if defined(VERMOUTH_LIB)
			else if (midi->opened & CMMIDI_VERMOUTH) {
				midiout_longmsg(midi->vermouth, midi->buffer, midi->mpos);
			}
#endif
			midi->midictrl = MIDICTRL_READY;
			return midi->mpos;
		} else if (midi->mpos >= MIDI_BUFFER) {	// おーばーふろー
			midi->midictrl = MIDICTRL_READY;
		}
		break;

	case MIDICTRL_TIMECODE:
		if (midi->mpos >= 2) {
			if ((data == 0x7e) || (data == 0x7f)) {
				// exclusiveと同じでいい筈…
				midi->midictrl = MIDICTRL_EXCLUSIVE;
			} else {
				midi->midictrl = MIDICTRL_READY;
				return 2;
			}
		}
		break;

	case MIDICTRL_SYSTEM:
		if (midi->mpos >= midi->midisyscnt) {
			midi->midictrl = MIDICTRL_READY;
			return midi->midisyscnt;
		}
		break;
	}
	return 0;
}

static UINT8
midigetstat(COMMNG self)
{

	return 0x00;
}

static INTPTR
midimsg(COMMNG self, UINT msg, INTPTR param)
{
	CMMIDI midi;
	COMFLAG	flag;

	midi = (CMMIDI)(self + 1);
	switch (msg) {
	case COMMSG_MIDIRESET:
		midireset(midi);
		return 1;

	case COMMSG_SETFLAG:
		flag = (COMFLAG)param;
		if ((flag) &&
		    (flag->size == sizeof(_COMFLAG) + sizeof(midi->mch)) &&
		    (flag->sig == COMSIG_MIDI)) {
			CopyMemory(midi->mch, flag + 1, sizeof(midi->mch));
			midisetparam(midi);
			return 1;
		}
		break;

	case COMMSG_GETFLAG:
		flag = (COMFLAG)_MALLOC(sizeof(_COMFLAG) + sizeof(midi->mch), "MIDI FLAG");
		if (flag) {
			flag->size = sizeof(_COMFLAG) + sizeof(midi->mch);
			flag->sig = COMSIG_MIDI;
			flag->ver = 0;
			flag->param = 0;
			CopyMemory(flag + 1, midi->mch, sizeof(midi->mch));
			return (INTPTR)flag;
		}
		break;
	}
	return 0;
}

static void
midirelease(COMMNG self)
{
	CMMIDI midi;

	midi = (CMMIDI)(self + 1);
	if (midi->opened & CMMIDI_MIDIIN) {
		if (midi->opened & CMMIDI_MIDIINSTART) {
			/* XXX */
		}
		close(midi->hmidiin);
	}
	if (midi->opened & CMMIDI_MIDIOUT) {
		waitlastexclusiveout(midi);
		close(midi->hmidiout);
	}
#if defined(VERMOUTH_LIB)
	if (midi->opened & CMMIDI_VERMOUTH) {
		midiout_destroy(midi->vermouth);
	}
#endif
	_MFREE(self);
}


// ----

void
cmmidi_initailize(void)
{
	UINT i;

	ZeroMemory(midictrlindex, sizeof(midictrlindex));
	for (i = 0; i < sizeof(midictrltbl) / sizeof(UINT8); i++) {
		midictrlindex[midictrltbl[i]] = (UINT8)(i + 1);
	}
	midictrlindex[32] = 1;
}

COMMNG
cmmidi_create(const char *midiout, const char *midiin, const char *module)
{
	COMMNG ret;
	CMMIDI midi;
	void (*outfn)(CMMIDI midi, UINT32 msg, UINT cnt);
	int hmidiout;
	int hmidiin;
#if defined(VERMOUTH_LIB)
	MIDIHDL vermouth = NULL;
#endif
	int opened = 0;

	/* MIDI-IN */
	hmidiin = getmidiin(midiin);
	if (hmidiin >= 0) {
		opened |= CMMIDI_MIDIIN;
	}

	/* MIDI-OUT */
	outfn = midiout_none;
	hmidiout = getmidiout(midiout);
	if (hmidiout >= 0) {
		outfn = midiout_device;
		opened |= CMMIDI_MIDIOUT;
	}
#if defined(VERMOUTH_LIB)
	else if (!milstr_cmp(midiout, cmmidi_vermouth)) {
		vermouth = midiout_create(vermouth_module, 512);
		if (vermouth != NULL) {
			outfn = midiout_vermouth;
			opened |= CMMIDI_VERMOUTH;
		}
	}
#endif
	if (!opened) {
		goto cmcre_err1;
	}
	ret = (COMMNG)_MALLOC(sizeof(_COMMNG) + sizeof(_CMMIDI), "MIDI");
	if (ret == NULL) {
		goto cmcre_err2;
	}
	ret->connect = COMCONNECT_MIDI;
	ret->read = midiread;
	ret->write = midiwrite;
	ret->getstat = midigetstat;
	ret->msg = midimsg;
	ret->release = midirelease;

	midi = (CMMIDI)(ret + 1);
	ZeroMemory(midi, sizeof(_CMMIDI));
	midi->opened = opened;
	midi->outfn = outfn;
	midi->midictrl = MIDICTRL_READY;
	midi->hmidiout = hmidiout;
	if (opened & CMMIDI_MIDIOUT) {
		gettimeofday(&midi->hmidiout_nextstart, NULL);
	}
#if defined(VERMOUTH_LIB)
	midi->vermouth = vermouth;
	if (opened & CMMIDI_VERMOUTH) {
		sound_streamregist((void *)vermouth, (SOUNDCB)vermouth_getpcm);
	}
#endif
	midi->midilast = 0x80;
	midi->midimodule = (UINT8)module2number(module);
	FillMemory(midi->mch, sizeof(midi->mch), 0xff);
	return ret;

cmcre_err2:
	if (opened & CMMIDI_MIDIIN) {
		if (hmidiin >= 0) {
			close(hmidiin);
		}
	}
	if (opened & CMMIDI_MIDIOUT) {
		if (hmidiout >= 0) {
			close(hmidiout);
		}
	}
#if defined(VERMOUTH_LIB)
	if (opened & CMMIDI_VERMOUTH) {
		midiout_destroy(vermouth);
	}
#endif
cmcre_err1:
	return NULL;
}


// ---- midiin callback
