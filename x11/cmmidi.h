#ifndef	NP2_X11_CMMIDI_H__
#define	NP2_X11_CMMIDI_H__

// ---- com manager midi for unix

G_BEGIN_DECLS

extern COMMNG cm_mpu98;

enum {
	MIDI_MT32 = 0,
	MIDI_CM32L,
	MIDI_CM64,
	MIDI_CM300,
	MIDI_CM500LA,
	MIDI_CM500GS,
	MIDI_SC55,
	MIDI_SC88,
	MIDI_LA,
	MIDI_GM,
	MIDI_GS,
	MIDI_XG,
	MIDI_OTHER
};

#if defined(VERMOUTH_LIB)
extern const char cmmidi_vermouth[];
#endif
extern const char cmmidi_midiout_device[];
extern const char cmmidi_midiin_device[];

extern const char *cmmidi_mdlname[];

void cmmidi_initailize(void);
COMMNG cmmidi_create(const char *midiout, const char *midiin, const char *module);

G_END_DECLS

#endif	/* NP2_X11_CMMIDI_H__ */
