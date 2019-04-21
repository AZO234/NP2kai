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

#if defined(SOUND_CRITICAL)

extern NP2_Semaphore_t semSoundCritical;

#define	SNDCSEC_INIT	NP2_Semaphore_Create(&semSoundCritical, 1)
#define	SNDCSEC_TERM	NP2_Semaphore_Destroy(&semSoundCritical)
#define	SNDCSEC_ENTER	NP2_Semaphore_Wait(&semSoundCritical)
#define	SNDCSEC_LEAVE	NP2_Semaphore_Release(&semSoundCritical)

#endif	/* defined(SOUND_CRITICAL) */

#ifdef __cplusplus
}
#endif
