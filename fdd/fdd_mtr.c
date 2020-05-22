#include	<compiler.h>
#include	<soundmng.h>
#include	<pccore.h>
#include	<fdd/fdd_mtr.h>
#if defined(SUPPORT_SWSEEKSND)
#include	<sound/pcmmix.h>
#include	<fdd/fdd_mtr.res>
#endif


#if defined(SUPPORT_SWSEEKSND)

static struct {
	UINT	enable;
	struct {
		PMIXHDR	hdr;
		PMIXTRK	trk[2];
	}		snd;
} mtrsnd;

void fddmtrsnd_initialize(UINT rate) {

	ZeroMemory(&mtrsnd, sizeof(mtrsnd));
	if (np2cfg.MOTORVOL) {
		mtrsnd.enable = 1;
		mtrsnd.snd.hdr.enable = 3;
		pcmmix_regist(&mtrsnd.snd.trk[0].data,
								(void *)fddseek, sizeof(fddseek), rate);
		mtrsnd.snd.trk[0].flag = PMIXFLAG_L | PMIXFLAG_R | PMIXFLAG_LOOP;
		mtrsnd.snd.trk[0].volume = (np2cfg.MOTORVOL << 12) / 100;
		pcmmix_regist(&mtrsnd.snd.trk[1].data,
								(void *)fddseek1, sizeof(fddseek1), rate);
		mtrsnd.snd.trk[1].flag = PMIXFLAG_L | PMIXFLAG_R;
		mtrsnd.snd.trk[1].volume = (np2cfg.MOTORVOL << 12) / 100;
	}
}

void fddmtrsnd_bind(void) {

	if (mtrsnd.enable) {
		sound_streamregist(&mtrsnd.snd, (SOUNDCB)pcmmix_getpcm);
	}
}

void fddmtrsnd_deinitialize(void) {

	int		i;
	void	*ptr;

	for (i=0; i<2; i++) {
		ptr = mtrsnd.snd.trk[i].data.sample;
		mtrsnd.snd.trk[i].data.sample = NULL;
		if (ptr) {
			_MFREE(ptr);
		}
	}
}

void fddmtrsnd_play(UINT num, BOOL play) {

	PMIXTRK	*trk;

	if ((mtrsnd.enable) && (num < 2)) {
		sound_sync();
		trk = mtrsnd.snd.trk + num;
		if (play) {
			if (trk->data.sample) {
				trk->pcm = trk->data.sample;
				trk->remain = trk->data.samples;
				mtrsnd.snd.hdr.playing |= (1 << num);
			}
		}
		else {
			mtrsnd.snd.hdr.playing &= ~(1 << num);
		}
	}
}
#endif


// ----

enum {
	MOVE1TCK_MS		= 15,
	MOVEMOTOR1_MS	= 25,
	DISK1ROL_MS		= 166
};

	_FDDMTR		fddmtr;

static void fddmtr_event(void) {

	switch(fddmtr.curevent) {
		case 100:
#if defined(SUPPORT_SWSEEKSND)
			fddmtrsnd_play(0, FALSE);
#else
			soundmng_pcmstop(SOUND_PCMSEEK);
#endif
			fddmtr.curevent = 0;
			break;

		default:
			fddmtr.curevent = 0;
			break;
	}
}

void fddmtr_initialize(void) {

#if defined(SUPPORT_SWSEEKSND)
	fddmtrsnd_play(0, FALSE);
#else
	soundmng_pcmstop(SOUND_PCMSEEK);
#endif
	ZeroMemory(&fddmtr, sizeof(fddmtr));
	FillMemory(fddmtr.head, sizeof(fddmtr.head), 42);
}

void fddmtr_callback(UINT nowtime) {

	if ((fddmtr.curevent) && (nowtime >= fddmtr.nextevent)) {
		fddmtr_event();
	}
}

void fdbiosout(NEVENTITEM item) {

	fddmtr.busy = 0;
	(void)item;
}

void fddmtr_seek(REG8 drv, REG8 c, UINT size) {

	int		regmove;
	SINT32	waitcnt;

	drv &= 3;
	regmove = c - fddmtr.head[drv];
	fddmtr.head[drv] = c;

	if (!np2cfg.MOTOR) {
		if (size) {
			fddmtr.busy = 1;
			nevent_set(NEVENT_FDBIOSBUSY, size * pccore.multiple,
												fdbiosout, NEVENT_ABSOLUTE);
		}
		return;
	}

	waitcnt = (size * DISK1ROL_MS) / (1024 * 8);
	if (regmove < 0) {
		regmove = 0 - regmove;
	}
	if (regmove == 1) {
		if (fddmtr.curevent < 80) {
			fddmtr_event();
#if defined(SUPPORT_SWSEEKSND)
			fddmtrsnd_play(1, TRUE);
#else
			soundmng_pcmplay(SOUND_PCMSEEK1, FALSE);
#endif
			fddmtr.curevent = 80;
			fddmtr.nextevent = GETTICK() + MOVEMOTOR1_MS;
		}
	}
	else if (regmove) {
		if (fddmtr.curevent < 100) {
			fddmtr_event();
#if defined(SUPPORT_SWSEEKSND)
			fddmtrsnd_play(0, TRUE);
#else
			soundmng_pcmplay(SOUND_PCMSEEK, TRUE);
#endif
			fddmtr.curevent = 100;
			fddmtr.nextevent = GETTICK() + (regmove * MOVE1TCK_MS);
		}
		if (regmove >= 32) {
			waitcnt += DISK1ROL_MS;
		}
	}
	if (waitcnt) {
		fddmtr.busy = 1;
		nevent_setbyms(NEVENT_FDBIOSBUSY,
										waitcnt, fdbiosout, NEVENT_ABSOLUTE);
	}
	(void)drv;
}

void fddmtr_reset(void) {

	fddmtr.busy = 0;
	nevent_reset(NEVENT_FDBIOSBUSY);
}

