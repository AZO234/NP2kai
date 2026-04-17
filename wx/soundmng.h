/* === sound management for wx port (SDL3 audio backend) === */

#ifndef NP2_WX_SOUNDMNG_H
#define NP2_WX_SOUNDMNG_H

#include <compiler.h>

enum {
	SOUND_PCMSEEK,
	SOUND_PCMSEEK1,
	SOUND_RELAY1,
	SOUND_MAXPCM
};

enum {
	SNDDRV_NODRV,
	SNDDRV_SDL,
	SNDDRV_DRVMAX
};

UINT8       snddrv_drv2num(const char *cfgstr);
const char *snddrv_num2drv(UINT8 num);

#if !defined(NOSOUND)

#ifdef __cplusplus
extern "C" {
#endif

UINT  soundmng_create(UINT rate, UINT ms);
void  soundmng_destroy(void);
void  soundmng_reset(void);
void  soundmng_play(void);
void  soundmng_stop(void);
void  soundmng_sync(void);
void  soundmng_setreverse(BOOL reverse);

BRESULT soundmng_pcmplay(UINT num, BOOL loop);
void    soundmng_pcmstop(UINT num);

BRESULT soundmng_initialize(void);
void    soundmng_deinitialize(void);

BRESULT soundmng_pcmload(UINT num, const char *filename);
void    soundmng_pcmvolume(UINT num, int volume);

extern int pcm_volume_default;

#if defined(VERMOUTH_LIB)
#include "sound/vermouth/vermouth.h"
extern MIDIMOD vermouth_module;
#endif

#ifdef __cplusplus
}
#endif

#else /* NOSOUND */

#define soundmng_create(rate, ms)       0
#define soundmng_destroy()
#define soundmng_reset()
#define soundmng_play()
#define soundmng_stop()
#define soundmng_sync()
#define soundmng_setreverse(reverse)

#define soundmng_pcmplay(num, loop)
#define soundmng_pcmstop(num)

#define soundmng_initialize()           (np2cfg.SOUND_SW = 0, FAILURE)
#define soundmng_deinitialize()

#define soundmng_pcmvolume(num, volume)

#endif /* !NOSOUND */

#endif /* NP2_WX_SOUNDMNG_H */
