/**
 * @file	pcm86.h
 * @brief	Interface of the 86-PCM
 */

#pragma once

#include "sound.h"
#include "nevent.h"

enum {
	PCM86_LOGICALBUF	= 0x8000,
	PCM86_BUFSIZE		= (1 << 16),
	PCM86_BUFMSK		= ((1 << 16) - 1),

	PCM86_DIVBIT		= 10,
	PCM86_DIVENV		= (1 << PCM86_DIVBIT),

	PCM86_RESCUE		= 20
};

#define	PCM86_EXTBUF		g_pcm86.rescue					/* 救済延滞… */
#define	PCM86_REALBUFSIZE	(PCM86_LOGICALBUF + PCM86_EXTBUF)

#define RECALC_NOWCLKWAIT(cnt)											\
	do																	\
	{																	\
		g_pcm86.virbuf -= (cnt << g_pcm86.stepbit);						\
		if (g_pcm86.virbuf < 0)											\
		{																\
			g_pcm86.virbuf &= g_pcm86.stepmask;							\
		}																\
	} while (0 /*CONSTCOND*/)

typedef struct {
	SINT32	divremain;
	SINT32	div;
	SINT32	div2;
	SINT32	smp;
	SINT32	lastsmp;
	SINT32	smp_l;
	SINT32	lastsmp_l;
	SINT32	smp_r;
	SINT32	lastsmp_r;

	UINT32	readpos;			/* DSOUND再生位置 */
	UINT32	wrtpos;				/* 書込み位置 */
	SINT32	realbuf;			/* DSOUND用のデータ数 */
	SINT32	virbuf;				/* 86PCM(bufsize:0x8000)のデータ数 */
	SINT32	rescue;

	SINT32	fifosize;
	SINT32	volume;
	SINT32	vol5;

	UINT32	lastclock;
	UINT32	stepclock;
	UINT	stepmask;

	UINT8	fifo;
	UINT8	extfunc;
	UINT8	dactrl;
	UINT8	_write;
	UINT8	stepbit;
	UINT8	irq;
	UINT8	reqirq;
	UINT8	irqflag;

	UINT8	buffer[PCM86_BUFSIZE];
} _PCM86, *PCM86;

typedef struct {
	UINT	rate;
	UINT	vol;
} PCM86CFG;


#ifdef __cplusplus
extern "C"
{
#endif

extern const UINT pcm86rate8[];

void pcm86_cb(NEVENTITEM item);

void pcm86gen_initialize(UINT rate);
void pcm86gen_setvol(UINT vol);

void pcm86_reset(void);
void pcm86gen_update(void);
void pcm86_setpcmrate(REG8 val);
void pcm86_setnextintr(void);

void SOUNDCALL pcm86gen_checkbuf(PCM86 pcm86);
void SOUNDCALL pcm86gen_getpcm(PCM86 pcm86, SINT32 *lpBuffer, UINT nCount);

BOOL pcm86gen_intrq(void);

#ifdef __cplusplus
}
#endif
