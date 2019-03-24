/**
 * @file	sndcsec.c
 * @brief	Implementation of the critical section for sound
 */

#include "compiler.h"
#include "sndcsec.h"

#if defined(SOUND_CRITICAL)

#if defined(NP2_SDL2)

	SDL_mutex* g_sndcsec;

#elif defined(NP2_X11)

	pthread_mutex_t g_sndcsec;		/* = PTHREAD_MUTEX_INITIALIZER; */

#elif defined(__LIBRETRO__)

#elif defined(_WIN32) || defined(_WIN32_WCE)

	CRITICAL_SECTION g_sndcsec;

#elif defined(MACOS)

	MPCriticalRegionID g_sndcsec;

#endif

#endif	/* defined(SOUND_CRITICAL) */
