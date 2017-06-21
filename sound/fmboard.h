/**
 * @file	fmboard.h
 * @brief	Interface of The board manager
 */

#pragma once

#if !defined(DISABLE_SOUND)

#include "cs4231.h"
#include "opl3.h"
#include "opna.h"
#include "opntimer.h"
#include "pcm86.h"

#if defined(SUPPORT_PX)
#define OPNA_MAX	5
#else	/* defined(SUPPORT_PX) */
#define OPNA_MAX	2
#endif	/* defined(SUPPORT_PX) */

#ifdef __cplusplus
extern "C"
{
#endif

extern	SOUNDID		g_nSoundID;
extern	OPL3		g_opl3;
extern	OPNA		g_opna[OPNA_MAX];
extern	_PCM86		g_pcm86;
extern	_CS4231		cs4231;

REG8 fmboard_getjoy(POPNA opna);

void fmboard_extreg(void (*ext)(REG8 enable));
void fmboard_extenable(REG8 enable);

void fmboard_construct(void);
void fmboard_destruct(void);
void fmboard_reset(const NP2CFG *pConfig, SOUNDID nSoundID);
void fmboard_bind(void);

#ifdef __cplusplus
}
#endif

#else

#define	fmboard_reset(c, t)
#define	fmboard_bind()

#endif
