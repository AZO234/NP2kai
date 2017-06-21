/**
 * @file	amd98.h
 * @brief	Interface of AMD-98
 */

#pragma once

#include "pccore.h"
#include "statsave.h"
#include "sound/psggen.h"

/**
 * @breif The sturecture of AMD-98
 */
struct amd98_t
{
	struct
	{
		UINT8	psg1reg;
		UINT8	psg2reg;
		UINT8	psg3reg;
		UINT8	rhythm;
	} s;
	_PSGGEN psg[3];
};

typedef struct amd98_t AMD98;

#ifdef __cplusplus
extern "C"
{
#endif

extern	AMD98	g_amd98;

void amd98_initialize(UINT rate);
void amd98_deinitialize(void);

void amd98int(NEVENTITEM item);

void amd98_reset(const NP2CFG *pConfig);
void amd98_bind(void);

int amd98_sfsave(STFLAGH sfh, const SFENTRY *tbl);
int amd98_sfload(STFLAGH sfh, const SFENTRY *tbl);

#ifdef __cplusplus
}
#endif

