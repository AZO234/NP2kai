/**
 * @file	fmboard.h
 * @brief	Interface of The board manager
 */

#pragma once

#if !defined(DISABLE_SOUND)

#include <sound/cs4231.h>
#include "opl3.h"
#include "opna.h"
#include "opntimer.h"
#include <sound/pcm86.h>
#include "ct1741.h"

#if defined(SUPPORT_PX)
#define OPNA_MAX	5
#else	/* defined(SUPPORT_PX) */
#define OPNA_MAX	3
#endif	/* defined(SUPPORT_PX) */
#define OPL3_MAX	8

#ifdef __cplusplus
extern "C"
{
#endif

extern	SOUNDID		g_nSoundID;
extern	OPL3		g_opl3[OPL3_MAX];
extern	OPNA		g_opna[OPNA_MAX];
#ifdef USE_MAME
extern	void		*g_mame_opl3[OPL3_MAX];
#endif
extern	_PCM86		g_pcm86;
extern	_CS4231		cs4231;
extern	SB16		g_sb16;

REG8 fmboard_getjoy(POPNA opna);

void fmboard_updatevolume(void);

void fmboard_extreg(void (*ext)(REG8 enable));
void fmboard_extenable(REG8 enable);

void fmboard_construct(void);
void fmboard_destruct(void);
void fmboard_reset(const NP2CFG *pConfig, SOUNDID nSoundID);
void fmboard_bind(void);
void fmboard_unbind(void);

#ifdef __cplusplus
}
#endif

#else

#define	fmboard_reset(c, t)
#define	fmboard_bind()

#endif
