/**
 * @file	sndcsec.c
 * @brief	Implementation of the critical section for sound
 */

#include "compiler.h"
#include "sndcsec.h"

#if defined(SOUND_CRITICAL) && defined(SUPPORT_NP2_THREAD)

	NP2_Semaphore_t semSoundCritical;

#endif	/* defined(SOUND_CRITICAL) defined(SUPPORT_NP2_THREAD) */
