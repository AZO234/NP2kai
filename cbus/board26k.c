/**
 * @file	board26k.c
 * @brief	Implementation of PC-9801-26K
 */

#include "compiler.h"
#include "board26k.h"
#include "iocore.h"
#include "cbuscore.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/soundrom.h"

static void IOOUTCALL opn_o188(UINT port, REG8 dat)
{
	g_opna[0].s.addrl = dat;
	g_opna[0].s.data = dat;
	(void)port;
}

static void IOOUTCALL opn_o18a(UINT port, REG8 dat)
{
	g_opna[0].s.data = dat;
	opna_writeRegister(&g_opna[0], g_opna[0].s.addrl, dat);

	(void)port;
}

static REG8 IOINPCALL opn_i188(UINT port)
{
	(void)port;
	return g_opna[0].s.status;
}

static REG8 IOINPCALL opn_i18a(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[0].s.addrl;
	if (nAddress == 0x0e)
	{
		return fmboard_getjoy(&g_opna[0]);
	}
	else if (nAddress < 0x10)
	{
		return opna_readRegister(&g_opna[0], nAddress);
	}

	(void)port;
	return g_opna[0].s.data;
}


// ----

static const IOOUT opn_o[4] = {
			opn_o188,	opn_o18a,	NULL,		NULL};

static const IOINP opn_i[4] = {
			opn_i188,	opn_i18a,	NULL,		NULL};

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void board26k_reset(const NP2CFG *pConfig)
{
	opna_reset(&g_opna[0], OPNA_MODE_2203 | OPNA_HAS_TIMER | OPNA_S98);
	opna_timer(&g_opna[0], (pConfig->snd26opt & 0xc0) | 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);

	opngen_setcfg(&g_opna[0].opngen, 3, 0x00);
	soundrom_loadex(pConfig->snd26opt & 7, OEMTEXT("26"));
	g_opna[0].s.base = (pConfig->snd26opt & 0x10) ? 0x000 : 0x100;
}

/**
 * Bind
 */
void board26k_bind(void)
{
	opna_bind(&g_opna[0]);
	cbuscore_attachsndex(0x188 - g_opna[0].s.base, opn_o, opn_i);
}
