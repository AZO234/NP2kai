/**
 * @file	sndcsec.h
 * @brief	Interface of the critical section for sound
 */

#pragma once

#include "np2_thread.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(SOUND_CRITICAL) && defined(SUPPORT_NP2_THREAD)

extern NP2_Semaphore_t semSoundCritical;

#define	SNDCSEC_INIT	NP2_Semaphore_Create(&semSoundCritical, 1)
#define	SNDCSEC_TERM	NP2_Semaphore_Destroy(&semSoundCritical)
#define	SNDCSEC_ENTER	NP2_Semaphore_Wait(&semSoundCritical)
#define	SNDCSEC_LEAVE	NP2_Semaphore_Release(&semSoundCritical)

#else

#define	SNDCSEC_INIT
#define	SNDCSEC_TERM
#define	SNDCSEC_ENTER
#define	SNDCSEC_LEAVE

#endif	/* defined(SOUND_CRITICAL) defined(SUPPORT_NP2_THREAD) */

#ifdef __cplusplus
}
#endif
