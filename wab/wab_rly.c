#include	<compiler.h>
#include	<soundmng.h>
#include	<pccore.h>
#include	<wab/wab_rly.h>
#include	<soundmng.h>
#if defined(SUPPORT_SWWABRLYSND)
#include	<sound/pcmmix.h>
#include	"wab_rly.res"
#endif

#if defined(SUPPORT_WAB)

#if defined(SUPPORT_SWWABRLYSND)

static struct {
	UINT	enable;
	struct {
		PMIXHDR	hdr;
		PMIXTRK	trk[1];
	}		snd;
} rlysnd;

void wabrlysnd_initialize(UINT rate) {

	ZeroMemory(&rlysnd, sizeof(rlysnd));
	if (np2cfg.MOTORVOL) {
		rlysnd.enable = 1;
		rlysnd.snd.hdr.enable = 3;
		pcmmix_regist(&rlysnd.snd.trk[0].data,
								(void *)relay1, sizeof(relay1), rate);
		rlysnd.snd.trk[0].flag = PMIXFLAG_L | PMIXFLAG_R | PMIXFLAG_LOOP;
		rlysnd.snd.trk[0].volume = (np2cfg.MOTORVOL << 12) / 100;
	}
}

void wabrlysnd_bind(void) {

	if (rlysnd.enable) {
		sound_streamregist(&rlysnd.snd, (SOUNDCB)pcmmix_getpcm);
	}
}

void wabrlysnd_deinitialize(void) {

	int		i;
	void	*ptr;

	for (i=0; i<1; i++) {
		ptr = rlysnd.snd.trk[i].data.sample;
		rlysnd.snd.trk[i].data.sample = NULL;
		if (ptr) {
			_MFREE(ptr);
		}
	}
}

void wabrlysnd_play(UINT num, BOOL play) {

	PMIXTRK	*trk;
	if ((rlysnd.enable) && (num < 1)) {
		sound_sync();
		trk = rlysnd.snd.trk + num;
		if (play) {
			if (trk->data.sample) {
				trk->pcm = trk->data.sample;
				trk->remain = trk->data.samples;
				rlysnd.snd.hdr.playing |= (1 << num);
			}
		}
		else {
			rlysnd.snd.hdr.playing &= ~(1 << num);
		}
	}
}
#endif

#endif

// ----

	_WABRLY		wabrly;

static void wabrly_event(void) {

	switch(wabrly.curevent) {
		case 100:
#if defined(SUPPORT_SWWABRLYSND)
			wabrlysnd_play(0, FALSE);
#else
#if !defined(NP2_SDL)
			soundmng_pcmstop(SOUND_RELAY1);
#endif
#endif
			wabrly.curevent = 0;
			break;

		default:
			wabrly.curevent = 0;
			break;
	}
}

void wabrly_initialize(void) {

#if defined(SUPPORT_SWWABRLYSND)
	wabrlysnd_play(0, FALSE);
#else
#if !defined(NP2_SDL)
	soundmng_pcmstop(SOUND_RELAY1);
#endif
#endif
	ZeroMemory(&wabrly, sizeof(wabrly));
	FillMemory(wabrly.head, sizeof(wabrly.head), 42);
}

void wabrly_callback(UINT nowtime) {

	if ((wabrly.curevent) && (nowtime >= wabrly.nextevent)) {
		wabrly_event();
	}
}

void wabrlyout(NEVENTITEM item) {

	(void)item;
}

void wabrly_switch(void) {

#if defined(SUPPORT_WAB)
	if (np2cfg.wabasw) {
		return;
	}
#endif

	wabrly_event();
#if defined(SUPPORT_SWWABRLYSND)
	wabrlysnd_play(0, TRUE);
#else
#if !defined(NP2_SDL)
	soundmng_pcmplay(SOUND_RELAY1, FALSE);
#endif
#endif
	wabrly.curevent = 100;
	wabrly.nextevent = GETTICK() + 30;

	nevent_setbyms(NEVENT_FDBIOSBUSY,
									30, wabrlyout, NEVENT_ABSOLUTE);
}

