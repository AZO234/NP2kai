/**
 * @file	boardso.c
 * @brief	Implementation of Sound Orchestra
 */

#include "compiler.h"
#include "boardso.h"
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

static void IOOUTCALL opl2_o18c(UINT port, REG8 dat)
{
	g_opl3.s.addrl = dat;
}

static void IOOUTCALL opl2_o18e(UINT port, REG8 dat)
{
	opl3_writeRegister(&g_opl3, g_opl3.s.addrl, dat);
}

static REG8 IOINPCALL opl2_i18c(UINT port)
{
	return opl3_readStatus(&g_opl3);
}

static REG8 IOINPCALL opl2_i18e(UINT port)
{
	return opl3_readRegister(&g_opl3, g_opl3.s.addrl);
}



// ----

static const IOOUT opn_o[4] = {
			opn_o188,	opn_o18a,	opl2_o18c,	opl2_o18e};

static const IOINP opn_i[4] = {
			opn_i188,	opn_i18a,	opl2_i18c,	opl2_i18e};

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void boardso_reset(const NP2CFG *pConfig, BOOL v)
{
	opna_reset(&g_opna[0], OPNA_MODE_2203 | OPNA_HAS_TIMER | OPNA_S98);
	opna_timer(&g_opna[0], (pConfig->snd26opt & 0xc0) | 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	opl3_reset(&g_opl3, (REG8)((v) ? OPL3_MODE_8950 : OPL3_MODE_3812));

	opngen_setcfg(&g_opna[0].opngen, 3, 0x00);
	soundrom_loadex(pConfig->snd26opt & 7, OEMTEXT("26"));
	g_opna[0].s.base = (pConfig->snd26opt & 0x10) ? 0x000 : 0x100;
}

/**
 * Bind
 */
void boardso_bind(void)
{
	opna_bind(&g_opna[0]);
	opl3_bind(&g_opl3);
	cbuscore_attachsndex(0x188 - g_opna[0].s.base, opn_o, opn_i);
}
