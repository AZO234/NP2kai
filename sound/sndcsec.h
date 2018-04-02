/**
 * @file	sndcsec.h
 * @brief	Interface of the critical section for sound
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(SOUND_CRITICAL)

#if defined(NP2_SDL2)

extern SDL_mutex* g_sndcsec;

#define	SNDCSEC_INIT	g_sndcsec = SDL_CreateMutex()
#define	SNDCSEC_TERM	SDL_DestroyMutex(g_sndcsec)
#define	SNDCSEC_ENTER	SDL_LockMutex(g_sndcsec)
#define	SNDCSEC_LEAVE	SDL_UnlockMutex(g_sndcsec)

#elif defined(NP2_X11)

extern pthread_mutex_t g_sndcsec;

#define	SNDCSEC_INIT	pthread_mutex_init(&g_sndcsec, NULL)
#define	SNDCSEC_TERM	pthread_mutex_destroy(&g_sndcsec)
#define	SNDCSEC_ENTER	pthread_mutex_lock(&g_sndcsec)
#define	SNDCSEC_LEAVE	pthread_mutex_unlock(&g_sndcsec)

#elif defined(__LIBRETRO__)

#elif (defined(_WIN32) || defined(_WIN32_WCE))

extern CRITICAL_SECTION g_sndcsec;

#define	SNDCSEC_INIT	InitializeCriticalSection(&g_sndcsec)
#define	SNDCSEC_TERM	DeleteCriticalSection(&g_sndcsec)
#define	SNDCSEC_ENTER	EnterCriticalSection(&g_sndcsec)
#define	SNDCSEC_LEAVE	LeaveCriticalSection(&g_sndcsec)

#elif defined(MACOS) && !defined(__LIBRETRO__)

extern MPCriticalRegionID g_sndcsec;

#define	SNDCSEC_INIT	MPCreateCriticalRegion(&g_sndcsec)
#define	SNDCSEC_TERM	MPDeleteCriticalRegion(g_sndcsec)
#define	SNDCSEC_ENTER	MPEnterCriticalRegion(g_sndcsec, kDurationForever)
#define	SNDCSEC_LEAVE	MPExitCriticalRegion(g_sndcsec)

#endif

#else	/* defined(SOUND_CRITICAL) */

#define	SNDCSEC_INIT		/*!< Initialize */
#define	SNDCSEC_TERM		/*!< Deinitialize */
#define	SNDCSEC_ENTER		/*!< Enter critical section */
#define	SNDCSEC_LEAVE		/*!< Leave critical section */

#endif	/* defined(SOUND_CRITICAL) */

#ifdef __cplusplus
}
#endif
