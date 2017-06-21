/**
 * @file	board118.c
 * @brief	Implementation of PC-9801-118
 */

#include "compiler.h"
#include "board118.h"
#include "pccore.h"
#include "iocore.h"
#include "cbuscore.h"
#include "cs4231io.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/soundrom.h"

static void IOOUTCALL ymf_o188(UINT port, REG8 dat)
{
	g_opna[0].s.addrl = dat;
	g_opna[0].s.addrh = 0;
	g_opna[0].s.data = dat;
	(void)port;
}

static void IOOUTCALL ymf_o18a(UINT port, REG8 dat)
{
	g_opna[0].s.data = dat;
	if (g_opna[0].s.addrh != 0) {
		return;
	}

	opna_writeRegister(&g_opna[0], g_opna[0].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL ymf_o18c(UINT port, REG8 dat)
{
	if (g_opna[0].s.extend)
	{
		g_opna[0].s.addrl = dat;
		g_opna[0].s.addrh = 1;
		g_opna[0].s.data = dat;
	}
	(void)port;
}

static void IOOUTCALL ymf_o18e(UINT port, REG8 dat)
{
	if (!g_opna[0].s.extend)
	{
		return;
	}
	g_opna[0].s.data = dat;

	if (g_opna[0].s.addrh != 1)
	{
		return;
	}

	opna_writeExtendedRegister(&g_opna[0], g_opna[0].s.addrl, dat);

	(void)port;
}

static REG8 IOINPCALL ymf_i188(UINT port)
{
	(void)port;
	return g_opna[0].s.status;
}

static REG8 IOINPCALL ymf_i18a(UINT port)
{
	UINT nAddress;

	if (g_opna[0].s.addrh == 0)
	{
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
	}

	(void)port;
	return g_opna[0].s.data;
}

static REG8 IOINPCALL ymf_i18c(UINT port)
{
	if (g_opna[0].s.extend)
	{
		return (g_opna[0].s.status & 3);
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

static void IOOUTCALL ymf_oa460(UINT port, REG8 dat)
{
	cs4231.extfunc = dat;
	extendchannel((REG8)(dat & 1));
	(void)port;
}

static REG8 IOINPCALL ymf_ia460(UINT port)
{
	(void)port;
	return (0x80 | (cs4231.extfunc & 1));
}


// ----

static const IOOUT ymf_o[4] = {
			ymf_o188,	ymf_o18a,	ymf_o18c,	ymf_o18e};

static const IOINP ymf_i[4] = {
			ymf_i188,	ymf_i18a,	ymf_i18c,	NULL};

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void board118_reset(const NP2CFG *pConfig)
{
	opna_reset(&g_opna[0], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_S98);
	opna_timer(&g_opna[0], 0xd0, NEVENT_FMTIMERA, NEVENT_FMTIMERB);

	opngen_setcfg(&g_opna[0].opngen, 3, OPN_STEREO | 0x038);
	cs4231io_reset();
	soundrom_load(0xcc000, OEMTEXT("118"));
	fmboard_extreg(extendchannel);

	(void)pConfig;
}

/**
 * Bind
 */
void board118_bind(void)
{
	opna_bind(&g_opna[0]);
	cs4231io_bind();
	cbuscore_attachsndex(0x188, ymf_o, ymf_i);
	iocore_attachout(0xa460, ymf_oa460);
	iocore_attachinp(0xa460, ymf_ia460);
}
