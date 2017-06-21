/**
 * @file	board14.c
 * @brief	Implementation of PC-9801-14
 */

#include "compiler.h"
#include "pccore.h"
#include "iocore.h"
#include "board14.h"
#include "cbuscore.h"
#include "sound.h"
#include "soundrom.h"
#include "tms3631.h"

/* どうも 8253C-2は 4MHz/16らすい？ */
/* とりあえず 1996800/8を入力してみる... (ver0.71) */

		MUSICGEN	g_musicgen;
static	_TMS3631	s_tms3631;

/* ---- 8253C-2 */

/**
 * Get counter
 * @return The counter
 */
UINT board14_pitcount(void)
{
	SINT32	clk;

	clk = nevent_getremain(NEVENT_MUSICGEN);
	if (clk >= 0)
	{
		clk /= pccore.multiple;
		clk /= 8;
		if (!(pccore.cpumode & CPUMODE_8MHZ))
		{
			clk = clk * 13 / 16;
		}
		return clk;
	}
	return 0;
}


/* ---- intr */

static void setmusicgenevent(UINT32 cnt, NEVENTPOSITION absolute)
{
	if (cnt > 4)								/* 根拠なし */
	{
		cnt *= pccore.multiple;
	}
	else
	{
		cnt = pccore.multiple << 16;
	}
	if (!(pccore.cpumode & CPUMODE_8MHZ))
	{
		cnt = cnt * 16 / 13;					/* cnt * 2457600 / 1996800 */
	}
	cnt *= 8;
	nevent_set(NEVENT_MUSICGEN, cnt, musicgenint, absolute);
}

/**
 * Interrupt
 * @param[in] item A pointer to an event-item structure
 */
void musicgenint(NEVENTITEM item)
{
	PITCH	pitch;

	if (item->flag & NEVENT_SETEVENT)
	{
		pitch = pit.ch + 3;
		if ((pitch->ctrl & 0x0c) == 0x04)
		{
			/* レートジェネレータ */
			setmusicgenevent(pitch->value, NEVENT_RELATIVE);
		}
	}
	pic_setirq(0x0c);
}


/* ---- I/O */

static void IOOUTCALL musicgen_o088(UINT port, REG8 dat)
{
	g_musicgen.porta = dat;
	(void)port;
}

static void IOOUTCALL musicgen_o08a(UINT port, REG8 dat)
{
	g_musicgen.portb = dat;
	(void)port;
}

static void IOOUTCALL musicgen_o08c(UINT port, REG8 dat)
{
	if (dat & 0x80)
	{
		if (!(g_musicgen.portc & 0x80))
		{
			g_musicgen.sync = 1;
			g_musicgen.ch = 0;
		}
		else if (g_musicgen.sync)
		{
			g_musicgen.sync = 0;
			sound_sync();
			g_musicgen.key[g_musicgen.ch] = dat;
			tms3631_setkey(&s_tms3631, (REG8)g_musicgen.ch, dat);
		}
		else if ((!(dat & 0x40)) && (g_musicgen.portc & 0x40))
		{
			g_musicgen.sync = 1;
			g_musicgen.ch = (g_musicgen.ch + 1) & 7;
		}
	}
	g_musicgen.portc = dat;
	(void)port;
}

static void IOOUTCALL musicgen_o188(UINT port, REG8 dat)
{
	sound_sync();
	g_musicgen.mask = dat;
	tms3631_setenable(&s_tms3631, dat);
	(void)port;
}

static void IOOUTCALL musicgen_o18c(UINT port, REG8 dat)
{
	PITCH	pitch;

	pitch = pit.ch + 3;
	if (!pit_setcount(pitch, dat))
	{
		setmusicgenevent(pitch->value, NEVENT_ABSOLUTE);
	}
	(void)port;
}

static void IOOUTCALL musicgen_o18e(UINT port, REG8 dat)
{
	pit_setflag(pit.ch + 3, dat);
	(void)port;
}

static REG8 IOINPCALL musicgen_i088(UINT port)
{
	(void)port;
	return g_musicgen.porta;
}

static REG8 IOINPCALL musicgen_i08a(UINT port)
{
	(void)port;
	return g_musicgen.portb;
}

static REG8 IOINPCALL musicgen_i08c(UINT port)
{
	(void)port;
	return g_musicgen.portc;
}

static REG8 IOINPCALL musicgen_i08e(UINT port)
{

	(void)port;
	return 0x08;
}

static REG8 IOINPCALL musicgen_i188(UINT port)
{
	(void)port;
	return g_musicgen.mask;
}

static REG8 IOINPCALL musicgen_i18c(UINT port)
{
	(void)port;
	return pit_getstat(pit.ch + 3);
}

static REG8 IOINPCALL musicgen_i18e(UINT port)
{
	(void)port;
	return 0x80;					/* INT-5 */
}


/* ---- interface */

static const IOOUT musicgen_o0[4] = {
		musicgen_o088,	musicgen_o08a,	musicgen_o08c,	NULL};

static const IOOUT musicgen_o1[4] = {
		musicgen_o188,	musicgen_o188,	musicgen_o18c,	musicgen_o18e};

static const IOINP musicgen_i0[4] = {
		musicgen_i088,	musicgen_i08a,	musicgen_i08c,	musicgen_i08e};

static const IOINP musicgen_i1[4] = {
		musicgen_i188,	musicgen_i188,	musicgen_i18c,	musicgen_i18e};


/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 * @param[in] bEnable Indicates whether to enable or disable the board
 */
void board14_reset(const NP2CFG *pConfig, BOOL bEnable)
{
	memset(&g_musicgen, 0, sizeof(g_musicgen));
	tms3631_reset(&s_tms3631);

	if (bEnable)
	{
		soundrom_load(0xcc000, OEMTEXT("14"));
	}
	(void)pConfig;
}

/**
 * Bind
 */
void board14_bind(void)
{
	sound_streamregist(&s_tms3631, (SOUNDCB)tms3631_getpcm);
	cbuscore_attachsndex(0x088, musicgen_o0, musicgen_i0);
	cbuscore_attachsndex(0x188, musicgen_o1, musicgen_i1);
}

/**
 * Restore
 */
void board14_allkeymake(void)
{
	REG8	i;

	for (i = 0; i < 8; i++)
	{
		tms3631_setkey(&s_tms3631, i, g_musicgen.key[i]);
	}
}
