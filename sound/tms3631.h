/**
 * @file	tms3631.h
 * @brief	Interface of the TMS3631
 */

#pragma once

#include "sound.h"

enum
{
	TMS3631_FREQ	= 16,
	TMS3631_MUL		= 4
};

typedef struct
{
	UINT32	freq;
	UINT32	count;
} TMSCH;

typedef struct
{
	TMSCH	ch[8];
	UINT	enable;
} _TMS3631, *TMS3631;

typedef struct
{
	SINT32	left;
	SINT32	right;
	SINT32	feet[16];
	UINT32	freqtbl[64];
} TMS3631CFG;

#ifdef __cplusplus
extern "C"
{
#endif

void tms3631_initialize(UINT rate);
void tms3631_setvol(const UINT8 *vol);

void tms3631_reset(TMS3631 tms);
void tms3631_setkey(TMS3631 tms, REG8 ch, REG8 key);
void tms3631_setenable(TMS3631 tms, REG8 enable);

void SOUNDCALL tms3631_getpcm(TMS3631 tms, SINT32 *pcm, UINT count);

#ifdef __cplusplus
}
#endif
