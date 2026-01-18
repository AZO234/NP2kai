/**
 * @file	ct1741io.h
 * @brief	Interface of the Creative SoundBlaster16 CT1741 DSP I/O
 */

#pragma once

#ifdef SUPPORT_SOUND_SB16

#ifdef __cplusplus
extern "C" {
#endif

void ct1741io_reset();
void ct1741io_bind(void);
void ct1741io_unbind(void);

#ifdef __cplusplus
}
#endif

#endif