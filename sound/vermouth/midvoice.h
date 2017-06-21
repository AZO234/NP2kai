
struct _midivoice;
typedef	struct _midivoice	_VOICE;
typedef	struct _midivoice	*VOICE;

typedef void (VERMOUTHCL *MIXPROC)(VOICE v, SINT32 *dst, SAMPLE src,
														SAMPLE srcterm);
typedef SAMPLE (VERMOUTHCL *RESPROC)(VOICE v, SAMPLE dst, SAMPLE dstterm);

enum {
	CHANNEL_MASK	= 0x0f,
	CHANNEL_RHYTHM	= 0x10,
	CHANNEL_SUSTAIN	= 0x20,
	CHANNEL_MONO	= 0x40
};

enum {
	VOICE_FREE		= 0x00,
	VOICE_ON		= 0x01,
	VOICE_SUSTAIN	= 0x02,
	VOICE_OFF		= 0x04,
	VOICE_REL		= 0x08
};

enum {
	VOICE_MIXNORMAL	= 0x00,
	VOICE_MIXLEFT	= 0x01,
	VOICE_MIXRIGHT	= 0x02,
	VOICE_MIXCENTRE	= 0x03,
	VOICE_MIXMASK	= 0x03,
	VOICE_FIXPITCH	= 0x04
};

#if !defined(MIDI_GMONLY)
enum {
	GSRX0_PITCHBEND			= 0x01,
	GSRX0_CHPRESSURE		= 0x02,
	GSRX0_PROGRAMCHANGE		= 0x04,
	GSRX0_CONTROLCHANGE		= 0x08,
	GSRX0_POLYPRESSURE		= 0x10,
	GSRX0_NOTEMESSAGE		= 0x20,
	GSRX0_RPN				= 0x40,
	GSRX0_NRPN				= 0x80,
	GSRX1_MODULATION		= 0x01,
	GSRX1_VOLUE				= 0x02,
	GSRX1_PANPOT			= 0x04,
	GSRX1_EXPRESSION		= 0x08,
	GSRX1_HOLD1				= 0x10,
	GSRX1_PORTAMENTO		= 0x20,
	GSRX1_SOSTENUTO			= 0x40,
	GSRX1_SOFT				= 0x80,
	GSRX2_BANKSELECT		= 0x01,
	GSRX2_BANKSELECTLSB		= 0x02
};
#endif

typedef struct {
	UINT		flag;
	int			level;
	int			pitchbend;
	int			pitchsens;
	float		pitchfactor;
	INSTRUMENT	inst;
#if !defined(MIDI_GMONLY)
	INSTRUMENT	*rhythm;
#endif

#if !defined(MIDI_GMONLY)
	UINT8		bank;
#endif
	UINT8		program;
	UINT8		volume;
	UINT8		expression;
	UINT8		panpot;
	UINT8		rpn_l;
	UINT8		rpn_m;
#if defined(ENABLE_GSRX)
	UINT8		keyshift;
	UINT8		noterange[2];
	UINT8		gsrx[4];
#endif
} _CHANNEL, *CHANNEL;

typedef struct {
	int			sweepstep;
	int			sweepcount;
	int			count;
	int			step;
	int			volume;
} VOICETRE;

typedef struct {
	int			sweepstep;
	int			sweepcount;
	int			phase;
	int			rate;
	int			count;
} VOICEVIB;

struct _midivoice {
	UINT8		phase;
	UINT8		flag;
	UINT8		note;
	UINT8		velocity;

	CHANNEL		channel;
	int			frequency;
	float		freq;
	int			panpot;

	MIXPROC		mix;
	RESPROC		resamp;
	INSTLAYER	sample;
	int			samppos;
	int			sampstep;
	int			envvol;
	int			envterm;
	int			envstep;
	int			envleft;
	int			envright;
	int			envphase;
	int			envcount;

	int			volleft;
	int			volright;

#if defined(ENABLE_TREMOLO)
	VOICETRE	tremolo;
#endif
#if defined(ENABLE_VIRLATE)
	float		freqnow;
	VOICEVIB	vibrate;
#endif
};


#ifdef __cplusplus
extern "C" {
#endif

int VERMOUTHCL envlope_setphase(VOICE v, int phase);
void VERMOUTHCL envelope_updates(VOICE v);

void VERMOUTHCL voice_setphase(VOICE v, UINT8 phase);
void VERMOUTHCL voice_setmix(VOICE v);

#ifdef __cplusplus
}
#endif


// ---- macro

#define voice_setfree(v)			\
	do {							\
		(v)->phase = VOICE_FREE;	\
	} while(0 /*CONSTCOND*/)

