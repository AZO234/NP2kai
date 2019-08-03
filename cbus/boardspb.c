/**
 * @file	boardspb.c
 * @brief	Implementation of Speak board
 */

#include "compiler.h"
#include "boardspb.h"
#include "iocore.h"
#include "cbuscore.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/soundrom.h"
#include "s98.h"

static int opna_idx = 0;

static void IOOUTCALL spb_o188(UINT port, REG8 dat)
{
	g_opna[opna_idx].s.addrl = dat;
//	g_opna[opna_idx].s.data = dat;

	(void)port;
}

static void IOOUTCALL spb_o18a(UINT port, REG8 dat)
{
//	g_opna[opna_idx].s.data = dat;
	opna_writeRegister(&g_opna[opna_idx], g_opna[opna_idx].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL spb_o18c(UINT port, REG8 dat)
{
	g_opna[opna_idx].s.addrh = dat;
//	g_opna[opna_idx].s.data = dat;
	(void)port;
}

static void IOOUTCALL spb_o18e(UINT port, REG8 dat)
{
//	g_opna[opna_idx].s.data = dat;
	opna_writeExtendedRegister(&g_opna[opna_idx], g_opna[opna_idx].s.addrh, dat);

	(void)port;
}

static REG8 IOINPCALL spb_i188(UINT port)
{
	(void)port;

	return opna_readExtendedStatus(&g_opna[opna_idx]);
}

static REG8 IOINPCALL spb_i18a(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[opna_idx].s.addrl;
	if (nAddress == 0x0e)
	{
		return fmboard_getjoy(&g_opna[opna_idx]);
	}

	(void)port;
	return opna_readRegister(&g_opna[opna_idx], nAddress);
}

static REG8 IOINPCALL spb_i18e(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[opna_idx].s.addrh;
	if ((nAddress == 0x08) || (nAddress == 0x0f))
	{
		return opna_readExtendedRegister(&g_opna[opna_idx], nAddress);
	}

	(void)port;
	return g_opna[opna_idx].s.reg[g_opna[opna_idx].s.addrl];
}


// ---- spark board

static void IOOUTCALL spr_o588(UINT port, REG8 dat)
{
	g_opna[1].s.addrl = dat;
//	g_opna[1].s.data = dat;
	(void)port;
}

static void IOOUTCALL spr_o58a(UINT port, REG8 dat)
{
//	g_opna[1].s.data = dat;
	S98_put(NORMAL2608_2, g_opna[1].s.addrl, dat);
	opna_writeRegister(&g_opna[1], g_opna[1].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL spr_o58c(UINT port, REG8 dat)
{
	g_opna[1].s.addrh = dat;
//	g_opna[1].s.data = dat;
	(void)port;
}

static void IOOUTCALL spr_o58e(UINT port, REG8 dat)
{
//	g_opna[1].s.data = dat;
	opna_writeExtendedRegister(&g_opna[1], g_opna[1].s.addrh, dat);

	(void)port;
}

static REG8 IOINPCALL spr_i588(UINT port)
{
	(void)port;
	return g_opna[0].s.status;
}

static REG8 IOINPCALL spr_i58a(UINT port)
{
	(void)port;
	return opna_readRegister(&g_opna[1], g_opna[1].s.addrl);
}

static REG8 IOINPCALL spr_i58c(UINT port)
{
	(void)port;
	return (g_opna[0].s.status & 3);
}

static REG8 IOINPCALL spr_i58e(UINT port)
{
	(void)port;
	return opna_read3438ExtRegister(&g_opna[1], g_opna[1].s.addrl);
}


// ----

static const IOOUT spb_o[4] =
{
	spb_o188,	spb_o18a,	spb_o18c,	spb_o18e
};

static const IOINP spb_i[4] =
{
	spb_i188,	spb_i18a,	spb_i188,	spb_i18e
};

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void boardspb_reset(const NP2CFG *pConfig, int opnaidx)
{
	opna_idx = opnaidx;

	opna_reset(&g_opna[opna_idx], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_HAS_ADPCM | OPNA_HAS_VR | OPNA_S98);
	if(opna_idx == 1){
		opna_timer(&g_opna[opna_idx], (pConfig->spbopt & 0xc0) | 0x10, NEVENT_FMTIMER2A, NEVENT_FMTIMER2B);
	}else{
		opna_timer(&g_opna[opna_idx], (pConfig->spbopt & 0xc0) | 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	}

	g_opna[opna_idx].s.extend = 1; // はじめからFM6音

	opngen_setcfg(&g_opna[opna_idx].opngen, 6, OPN_STEREO | 0x3f);
	soundrom_loadex(pConfig->spbopt & 7, OEMTEXT("SPB"));
	g_opna[opna_idx].s.base = ((pConfig->spbopt & 0x10) ? 0x000 : 0x100);
}

/**
 * Bind
 */
void boardspb_bind(void)
{
	opna_bind(&g_opna[opna_idx]);
	cbuscore_attachsndex(0x188 - g_opna[opna_idx].s.base, spb_o, spb_i);
}
void boardspb_unbind(void)
{
	cbuscore_detachsndex(0x188 - g_opna[opna_idx].s.base);
}


// ----

static const IOOUT spr_o[4] = {
			spr_o588,	spr_o58a,	spr_o58c,	spr_o58e};

static const IOINP spr_i[4] = {
			spr_i588,	spr_i58a,	spr_i58c,	spr_i58e};

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void boardspr_reset(const NP2CFG *pConfig)
{
	opna_idx = 0;

	opna_reset(&g_opna[0], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_HAS_ADPCM | OPNA_HAS_VR | OPNA_S98);
	opna_timer(&g_opna[0], (pConfig->spbopt & 0xc0) | 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	opna_reset(&g_opna[1], OPNA_MODE_3438 | OPNA_HAS_VR);
	
	g_opna[0].s.extend = 1; // はじめからFM6音
	g_opna[1].s.extend = 1; // はじめからFM6音

	opngen_setcfg(&g_opna[0].opngen, 6, OPN_STEREO | 0x0f);
	opngen_setcfg(&g_opna[1].opngen, 6, OPN_STEREO | 0x0f);
	soundrom_loadex(pConfig->spbopt & 7, OEMTEXT("SPB"));
	g_opna[0].s.base = (pConfig->spbopt & 0x10) ? 0x000 : 0x100;
}

/**
 * Bind
 */
void boardspr_bind(void)
{
	opna_bind(&g_opna[0]);
	opna_bind(&g_opna[1]);
	cbuscore_attachsndex(0x188 - g_opna[0].s.base, spb_o, spb_i);
	cbuscore_attachsndex(0x588 - g_opna[0].s.base, spr_o, spr_i);
}
void boardspr_unbind(void)
{
	cbuscore_detachsndex(0x188 - g_opna[0].s.base);
	cbuscore_detachsndex(0x588 - g_opna[0].s.base);
}


