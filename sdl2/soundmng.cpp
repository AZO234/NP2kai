#include "compiler.h"
#include "soundmng.h"
#include <algorithm>
#include "parts.h"
#include "sound.h"
#if defined(VERMOUTH_LIB)
#include	"vermouth.h"

#ifdef __cplusplus
extern "C" {
#endif
MIDIMOD vermouth_module = NULL;
#ifdef __cplusplus
}
#endif
#endif
#if defined(SUPPORT_EXTERNALCHIP)
#include "ext/externalchipmanager.h"
#endif

#define	NSNDBUF				2

typedef struct {
	BOOL	opened;
	int		nsndbuf;
	int		samples;
	SINT16	*buf[NSNDBUF];
} SOUNDMNG;

static	SOUNDMNG	soundmng;


static void sound_play_cb(void *userdata, UINT8 *stream, int len) {

	int			length;
	SINT16		*dst;
const SINT32	*src;

	length = (std::min)(len, (int)(soundmng.samples * 2 * sizeof(SINT16)));
	dst = soundmng.buf[soundmng.nsndbuf];
	src = sound_pcmlock();
	if (src) {
		satuation_s16(dst, src, length);
		sound_pcmunlock(src);
	}
	else {
		ZeroMemory(dst, length);
	}
	SDL_memset(stream, 0, len);
	SDL_MixAudio(stream, (UINT8 *)dst, length, SDL_MIX_MAXVOLUME);
	soundmng.nsndbuf = (soundmng.nsndbuf + 1) % NSNDBUF;
	(void)userdata;
}

UINT8
snddrv_drv2num(const char* cfgstr)
{

	if (strcasecmp(cfgstr, "SDL") == 0)
		return SNDDRV_SDL;
	return SNDDRV_NODRV;
}

const char *
snddrv_num2drv(UINT8 num)
{

	switch (num) {
	case SNDDRV_SDL:
		return "SDL";
	}
	return "nosound";
}

UINT soundmng_create(UINT rate, UINT ms) {

	SDL_AudioSpec	fmt;
	UINT			s;
	UINT			samples;
	SINT16			*tmp;

	if (soundmng.opened) {
		goto smcre_err1;
	}
	if (SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
		goto smcre_err1;
	}

	s = rate * ms / (NSNDBUF * 1000);
	samples = 1;
	while(s > samples) {
		samples <<= 1;
	}
	soundmng.nsndbuf = 0;
	soundmng.samples = samples;
	for (s=0; s<NSNDBUF; s++) {
		tmp = (SINT16 *)_MALLOC(samples * 2 * sizeof(SINT16), "buf");
		if (tmp == NULL) {
			goto smcre_err2;
		}
		soundmng.buf[s] = tmp;
		ZeroMemory(tmp, samples * 2 * sizeof(SINT16));
	}

	ZeroMemory(&fmt, sizeof(fmt));
	fmt.freq = rate;
	fmt.format = AUDIO_S16SYS;
	fmt.channels = 2;
	fmt.samples = samples;
	fmt.callback = sound_play_cb;
	if (SDL_OpenAudio(&fmt, NULL) < 0) {
		fprintf(stderr, "Error: SDL_OpenAudio: %s\n", SDL_GetError());
		return(FAILURE);
	}
#if defined(VERMOUTH_LIB)
	vermouth_module = midimod_create(rate);
	midimod_loadall(vermouth_module);
#endif
	soundmng.opened = TRUE;
	return(samples);

smcre_err2:
	for (s=0; s<NSNDBUF; s++) {
		tmp = soundmng.buf[s];
		soundmng.buf[s] = NULL;
		if (tmp) {
			_MFREE(tmp);
		}
	}

smcre_err1:
	return(0);
}

void soundmng_destroy(void) {

	int		i;
	SINT16	*tmp;

	if (soundmng.opened) {
		soundmng.opened = FALSE;
		SDL_PauseAudio(1);
		SDL_CloseAudio();
		for (i=0; i<NSNDBUF; i++) {
			tmp = soundmng.buf[i];
			soundmng.buf[i] = NULL;
			_MFREE(tmp);
		}
#if defined(VERMOUTH_LIB)
		midimod_destroy(vermouth_module);
		vermouth_module = NULL;
#endif
	}
}

void soundmng_play(void)
{
	if (soundmng.opened)
	{
		SDL_PauseAudio(0);
#if defined(SUPPORT_EXTERNALCHIP)
		CExternalChipManager::GetInstance()->Mute(false);
#endif
	}
}

void soundmng_stop(void)
{
	if (soundmng.opened)
	{
		SDL_PauseAudio(1);
#if defined(SUPPORT_EXTERNALCHIP)
		CExternalChipManager::GetInstance()->Mute(true);
#endif
	}
}


// ----

void soundmng_initialize()
{
#if defined(SUPPORT_EXTERNALCHIP)
	CExternalChipManager::GetInstance()->Initialize();
#endif
}

void soundmng_deinitialize()
{
#if defined(SUPPORT_EXTERNALCHIP)
	CExternalChipManager::GetInstance()->Deinitialize();
#endif

	soundmng_destroy();
}
