/**
 * @file	pcmmix.h
 * @brief	Interface of the pcm sound
 */

#pragma once

#include "sound.h"

enum {
	PMIXFLAG_L		= 0x0001,
	PMIXFLAG_R		= 0x0002,
	PMIXFLAG_LOOP	= 0x0004
};

typedef struct {
	UINT32	playing;
	UINT32	enable;
} PMIXHDR;

typedef struct {
	SINT16	*sample;
	UINT	samples;
} PMIXDAT;

typedef struct {
const SINT16	*pcm;
	UINT		remain;
	PMIXDAT		data;
	UINT		flag;
	SINT32		volume;
} PMIXTRK;

typedef struct {
	PMIXHDR		hdr;
	PMIXTRK		trk[1];
} _PCMMIX, *PCMMIX;


#ifdef __cplusplus
extern "C"
{
#endif

BRESULT pcmmix_regist(PMIXDAT *dat, void *datptr, UINT datsize, UINT rate);
BRESULT pcmmix_regfile(PMIXDAT *dat, const OEMCHAR *fname, UINT rate);

void SOUNDCALL pcmmix_getpcm(PCMMIX hdl, SINT32 *pcm, UINT count);

#ifdef __cplusplus
}
#endif
