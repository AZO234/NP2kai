/**
 * @file	board86.c
 * @brief	Implementation of PC-9801-86
 */

#include "compiler.h"
#include "board86.h"
#include "iocore.h"
#include "cbuscore.h"
#include "pcm86io.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/soundrom.h"

static void IOOUTCALL opna_o188(UINT port, REG8 dat)
{
	g_opna[0].s.addrl = dat;
	g_opna[0].s.data = dat;
	(void)port;
}

static void IOOUTCALL opna_o18a(UINT port, REG8 dat)
{
	g_opna[0].s.data = dat;
	opna_writeRegister(&g_opna[0], g_opna[0].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL opna_o18c(UINT port, REG8 dat)
{
	if (g_opna[0].s.extend)
	{
		g_opna[0].s.addrh = dat;
		g_opna[0].s.data = dat;
	}

	(void)port;
}

static void IOOUTCALL opna_o18e(UINT port, REG8 dat)
{
	if (g_opna[0].s.extend)
	{
		g_opna[0].s.data = dat;
		opna_writeExtendedRegister(&g_opna[0], g_opna[0].s.addrh, dat);
	}

	(void)port;
}

static REG8 IOINPCALL opna_i188(UINT port)
{
	(void)port;
	return g_opna[0].s.status;
}

static REG8 IOINPCALL opna_i18a(UINT port)
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
	else if (nAddress == 0xff)
	{
		return 1;
	}

	(void)port;
	return g_opna[0].s.data;
}

static REG8 IOINPCALL opna_i18c(UINT port)
{
	if (g_opna[0].s.extend)
	{
		return opna_readExtendedStatus(&g_opna[0]);
	}

	(void)port;
	return 0xff;
}

static REG8 IOINPCALL opna_i18e(UINT port)
{
	UINT nAddress;

	if (g_opna[0].s.extend)
	{
		nAddress = g_opna[0].s.addrh;
		if ((nAddress == 0x08) || (nAddress == 0x0f))
		{
			return opna_readExtendedRegister(&g_opna[0], nAddress);
		}
		return g_opna[0].s.data;
	}

	(void)port;
	return 0xff;
}

static void extendchannel(REG8 enable)
{
	g_opna[0].s.extend = enable;
	if (enable)
	{
		opngen_setcfg(&g_opna[0].opngen, 6, OPN_STEREO | 0x07);
	}
	else
	{
		opngen_setcfg(&g_opna[0].opngen, 3, OPN_MONORAL | 0x07);
		rhythm_setreg(&g_opna[0].rhythm, 0x10, 0xff);
	}
}


// ----

static const IOOUT opna_o[4] = {
			opna_o188,	opna_o18a,	opna_o18c,	opna_o18e};

static const IOINP opna_i[4] = {
			opna_i188,	opna_i18a,	opna_i18c,	opna_i18e};


/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 * @param[in] adpcm Enable ADPCM
 */
void board86_reset(const NP2CFG *pConfig, BOOL adpcm)
{
	REG8 cCaps;
	UINT nIrq;

	cCaps = OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_S98;
	if (adpcm)
	{
		cCaps |= OPNA_HAS_ADPCM;
	}
	nIrq = (pConfig->snd86opt & 0x10) | ((pConfig->snd86opt & 0x4) << 5) | ((pConfig->snd86opt & 0x8) << 3);

	opna_reset(&g_opna[0], cCaps);
	opna_timer(&g_opna[0], nIrq, NEVENT_FMTIMERA, NEVENT_FMTIMERB);

	opngen_setcfg(&g_opna[0].opngen, 3, OPN_STEREO | 0x38);
	if (pConfig->snd86opt & 2)
	{
		soundrom_load(0xcc000, OEMTEXT("86"));
	}
	g_opna[0].s.base = (pConfig->snd86opt & 0x01) ? 0x000 : 0x100;
	fmboard_extreg(extendchannel);
	pcm86io_setopt(pConfig->snd86opt);
}

/**
 * Bind
 */
void board86_bind(void)
{
	opna_bind(&g_opna[0]);
	pcm86io_bind();
	cbuscore_attachsndex(0x188 + g_opna[0].s.base, opna_o, opna_i);
}
