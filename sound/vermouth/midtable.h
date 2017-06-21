
#define	SINENT_BIT			9

enum {
	MIDI_NOTE_OFF	= 0x80,
	MIDI_NOTE_ON	= 0x90,
	MIDI_KEYPRESS	= 0xa0,
	MIDI_CTRLCHANGE	= 0xb0,
	MIDI_PROGCHANGE	= 0xc0,
	MIDI_CHPRESS	= 0xd0,
	MIDI_PITCHBEND	= 0xe0
};

enum {
	CTRL_PGBANK		= 0,
	CTRL_MODULAT	= 1,
	CTRL_PORTA_T	= 5,
	CTRL_DATA_M		= 6,
	CTRL_VOLUME		= 7,
	CTRL_PANPOT		= 10,
	CTRL_EXPRESS	= 11,
	CTRL_PGBANKH	= 32,
	CTRL_DATA_L		= 38,
	CTRL_PEDAL		= 64,
	CTRL_PORTAM		= 65,
	CTRL_SOSTEN		= 66,
	CTRL_SOFT		= 67,
	CTRL_LEGART		= 84,
	CTRL_REVERB		= 91,
	CTRL_CHORUS		= 93,
	CTRL_NRPN_L		= 98,
	CTRL_NRPN_M		= 99,
	CTRL_RPN_L		= 100,
	CTRL_RPN_M		= 101,
	CTRL_SOUNDOFF	= 120,
	CTRL_RESETCTRL	= 121,
	CTRL_LOCALCTRL	= 122,
	CTRL_NOTEOFF	= 123,
	CTRL_OMNIOFF	= 124,
	CTRL_OMNION		= 125,
	CTRL_MONOON		= 126,
	CTRL_POLYON		= 127
};


#ifdef __cplusplus
extern "C" {
#endif

extern const int freq_table[128];
extern const SINT16 envsin12q[1 << (SINENT_BIT - 2)];
extern const SINT16 vibsin12[1 << VIBRATE_SHIFT];
extern const SINT16 voltbl12[128];
extern const float bendltbl[64];
extern const float bendhtbl[48];

#if defined(PANPOT_REVA)
extern const UINT8 revacurve[];
#endif
#if defined(VOLUME_ACURVE)
extern const UINT8 acurve[];
#endif

#ifdef __cplusplus
}
#endif

