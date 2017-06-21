/**
 * @file	boardx2.c
 * @brief	Implementation of PC-9801-86 + 26K
 */

#include "compiler.h"
#include "boardx2.h"
#include "iocore.h"
#include "cbuscore.h"
#include "pcm86io.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/soundrom.h"

static void IOOUTCALL opn_o088(UINT port, REG8 dat)
{
	g_opna[1].s.addrl = dat;
	g_opna[1].s.data = dat;
	(void)port;
}

static void IOOUTCALL opn_o08a(UINT port, REG8 dat)
{
	g_opna[1].s.data = dat;
	opna_writeRegister(&g_opna[1], g_opna[1].s.addrl, dat);

	(void)port;
}

static REG8 IOINPCALL opn_i088(UINT port)
{
	(void)port;
	return g_opna[1].s.status;
}

static REG8 IOINPCALL opn_i08a(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[1].s.addrl;
	if (nAddress == 0x0e)
	{
		return fmboard_getjoy(&g_opna[1]);
	}
	else if (nAddress < 0x10)
	{
		return opna_readRegister(&g_opna[1], nAddress);
	}

	(void)port;
	return g_opna[1].s.data;
}


// ----

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
		opngen_setcfg(&g_opna[0].opngen, 6, OPN_STEREO | 0x007);
	}
	else
	{
		opngen_setcfg(&g_opna[0].opngen, 3, OPN_MONORAL | 0x007);
		rhythm_setreg(&g_opna[0].rhythm, 0x10, 0xff);
	}
}


// ----

static const IOOUT opn_o[4] = {
			opn_o088,	opn_o08a,	NULL,		NULL};

static const IOINP opn_i[4] = {
			opn_i088,	opn_i08a,	NULL,		NULL};

static const IOOUT opna_o[4] = {
			opna_o188,	opna_o18a,	opna_o18c,	opna_o18e};

static const IOINP opna_i[4] = {
			opna_i188,	opna_i18a,	opna_i18c,	opna_i18e};


/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 * @param[in] adpcm Enable ADPCM
 */
void boardx2_reset(const NP2CFG *pConfig)
{
	UINT nIrq1;
	UINT nIrq2;

	nIrq1 = (pConfig->snd86opt & 0x10) | ((pConfig->snd86opt & 0x4) << 5) | ((pConfig->snd86opt & 0x8) << 3);
	nIrq2 = (pConfig->snd26opt & 0xc0) | 0x10;
	if (nIrq1 == nIrq2)
	{
		nIrq2 = (nIrq2 == 0xd0) ? 0x90 : 0xd0;
	}

	opna_reset(&g_opna[0], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_S98);
	opna_timer(&g_opna[0], nIrq1, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	opna_reset(&g_opna[1], OPNA_MODE_2203);
	opna_timer(&g_opna[1], nIrq2, NEVENT_FMTIMER2A, NEVENT_FMTIMER2B);

	opngen_setcfg(&g_opna[0].opngen, 3, OPN_STEREO | 0x038);
	opngen_setcfg(&g_opna[1].opngen, 3, 0);
	if (pConfig->snd86opt & 2)
	{
		soundrom_load(0xcc000, OEMTEXT("86"));
	}
	fmboard_extreg(extendchannel);
	pcm86io_setopt(pConfig->snd86opt);
}

void boardx2_bind(void)
{
	opna_bind(&g_opna[0]);
	opna_bind(&g_opna[1]);
	pcm86io_bind();
	cbuscore_attachsndex(0x088, opn_o, opn_i);
	cbuscore_attachsndex(0x188, opna_o, opna_i);
}

