/**
 * @file	ymzadpcm.h
 * @brief	Interface of the YMZ ADPCM
 */

#pragma once

#include "sound.h"
#include "adpcm.h" // ébíË OPNAÇÃADPCMèÛë‘Çï€ë∂êÊÇ∆ÇµÇƒéÿÇËÇÈ

#ifdef __cplusplus
extern "C" {
#endif

void ymzadpcm_initialize(UINT rate);
void ymzadpcm_setvol(UINT vol);

void ymzadpcm_reset(ADPCM ad);
void ymzadpcm_update(ADPCM ad);
void ymzadpcm_setreg(ADPCM ad, UINT reg, REG8 value);
REG8 ymzadpcm_status(ADPCM ad);

REG8 SOUNDCALL ymzadpcm_readsample(ADPCM ad);
void SOUNDCALL ymzadpcm_datawrite(ADPCM ad, REG8 data);
void SOUNDCALL ymzadpcm_getpcm(ADPCM ad, SINT32 *buf, UINT count);
void SOUNDCALL ymzadpcm_getpcm_dummy(ADPCM ad, SINT32 *buf, UINT count);

#ifdef __cplusplus
}
#endif
