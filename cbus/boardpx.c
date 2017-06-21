/**
 * @file	boardpx.c
 * @brief	Implementation of PX
 */

#include "compiler.h"

#if defined(SUPPORT_PX)

#include "boardpx.h"
#include "iocore.h"
#include "cbuscore.h"
#include "pcm86io.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/soundrom.h"

static void IOOUTCALL spb_o088(UINT port, REG8 dat)
{
	g_opna[0].s.addrl = dat;
//	g_opna[0].s.data = dat;
	(void)port;
}

static void IOOUTCALL spb_o08a(UINT port, REG8 dat)
{
//	g_opna[0].s.data = dat;
	opna_writeRegister(&g_opna[0], g_opna[0].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL spb_o08c(UINT port, REG8 dat)
{
	g_opna[0].s.addrh = dat;
//	g_opna[0].s.data = dat;
	(void)port;
}

static void IOOUTCALL spb_o08e(UINT port, REG8 dat)
{
//	g_opna[0].data = dat;
	opna_writeExtendedRegister(&g_opna[0], g_opna[0].s.addrh, dat);

	(void)port;
}

static REG8 IOINPCALL spb_i088(UINT port)
{
	(void)port;

	return opna_readExtendedStatus(&g_opna[0]);
}

static REG8 IOINPCALL spb_i08a(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[0].s.addrl;
	if (nAddress == 0x0e)
	{
		return fmboard_getjoy(&g_opna[0]);
	}

	(void)port;
	return opna_readRegister(&g_opna[0], nAddress);
}

static REG8 IOINPCALL spb_i08e(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[0].s.addrh;
	if ((nAddress == 0x08) || (nAddress == 0x0f))
	{
		return opna_readExtendedRegister(&g_opna[0], nAddress);
	}

	(void)port;
	return g_opna[0].s.reg[g_opna[0].s.addrl];
}



static void IOOUTCALL spb_o188(UINT port, REG8 dat)
{
	g_opna[1].s.addrl = dat;
//	g_opna[1].s.data = dat;

	(void)port;
}

static void IOOUTCALL spb_o18a(UINT port, REG8 dat)
{
//	g_opna[1].s.data = dat;
	opna_writeRegister(&g_opna[1], g_opna[1].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL spb_o18c(UINT port, REG8 dat)
{
	g_opna[1].s.addrh = dat;
//	g_opna[1].s.data = dat;
	(void)port;
}

static void IOOUTCALL spb_o18e(UINT port, REG8 dat)
{
//	g_opna[1].s.data = dat;
	opna_writeExtendedRegister(&g_opna[1], g_opna[1].s.addrh, dat);

	(void)port;
}

static REG8 IOINPCALL spb_i188(UINT port)
{
	(void)port;

	return opna_readExtendedStatus(&g_opna[1]);
}

static REG8 IOINPCALL spb_i18a(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[1].s.addrl;
	if (nAddress == 0x0e)
	{
		return fmboard_getjoy(&g_opna[1]);
	}

	(void)port;
	return opna_readRegister(&g_opna[1], nAddress);
}

static REG8 IOINPCALL spb_i18e(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[1].s.addrh;
	if ((nAddress == 0x08) || (nAddress == 0x0f))
	{
		return opna_readExtendedRegister(&g_opna[1], nAddress);
	}

	(void)port;
	return g_opna[1].s.reg[g_opna[1].s.addrl];
}



static void IOOUTCALL p86_o288(UINT port, REG8 dat)
{
	g_opna[4].s.addrl = dat;
	g_opna[4].s.data = dat;
	(void)port;
}

static void IOOUTCALL p86_o28a(UINT port, REG8 dat)
{
	g_opna[4].s.data = dat;
	opna_writeRegister(&g_opna[4], g_opna[4].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL p86_o28c(UINT port, REG8 dat)
{
	if (g_opna[4].s.extend)
	{
		g_opna[4].s.addrh = dat;
		g_opna[4].s.data = dat;
	}

	(void)port;
}

static void IOOUTCALL p86_o28e(UINT port, REG8 dat)
{
	if (g_opna[4].s.extend)
	{
		g_opna[4].s.data = dat;
		opna_writeExtendedRegister(&g_opna[4], g_opna[4].s.addrh, dat);
	}

	(void)port;
}

static REG8 IOINPCALL p86_i288(UINT port)
{
	(void)port;
	return g_opna[4].s.status;
}

static REG8 IOINPCALL p86_i28a(UINT port)
{
	UINT nAddress;

	nAddress = g_opna[4].s.addrl;
	if (nAddress == 0x0e)
	{
		return fmboard_getjoy(&g_opna[4]);
	}
	else if (nAddress < 0x10)
	{
		return opna_readRegister(&g_opna[4], nAddress);
	}
	else if (nAddress == 0xff)
	{
		return 1;
	}

	(void)port;
	return g_opna[4].s.data;
}

static REG8 IOINPCALL p86_i28e(UINT port)
{
	UINT nAddress;

	if (g_opna[4].s.extend)
	{
		nAddress = g_opna[4].s.addrh;
		if ((nAddress == 0x08) || (nAddress == 0x0f))
		{
			return opna_readExtendedRegister(&g_opna[4], nAddress);
		}
		return g_opna[4].s.data;
	}

	(void)port;
	return 0xff;
}


// ---- spark board
static void IOOUTCALL spr_o48a(UINT port, REG8 dat)
{
//	g_opna[2].s.data = dat;
	opna_writeRegister(&g_opna[2], g_opna[2].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL spr_o48c(UINT port, REG8 dat)
{
	g_opna[2].s.addrh = dat;
//	g_opna[2].s.data = dat;
	(void)port;
}

static void IOOUTCALL spr_o48e(UINT port, REG8 dat)
{
//	g_opn.s.data = dat;
	opna_writeExtendedRegister(&g_opna[2], g_opna[2].s.addrh, dat);

	(void)port;
}

static REG8 IOINPCALL spr_i488(UINT port)
{
	(void)port;
	return g_opna[0].s.status;
}

static REG8 IOINPCALL spr_i48a(UINT port)
{
	(void)port;
	return opna_readRegister(&g_opna[2], g_opna[2].s.addrl);
}

static REG8 IOINPCALL spr_i48c(UINT port)
{
	(void)port;
	return (g_opna[0].s.status & 3);
}

static REG8 IOINPCALL spr_i48e(UINT port)
{
	(void)port;
	return opna_read3438ExtRegister(&g_opna[2], g_opna[2].s.addrl);
}

static void IOOUTCALL spr_o488(UINT port, REG8 dat)
{
	g_opna[2].s.addrl = dat;
//	g_opna[2].s.data = dat;
	(void)port;
}



static void IOOUTCALL spr_o588(UINT port, REG8 dat)
{
	g_opna[3].s.addrl = dat;
//	g_opna[3].s.data = dat;
	(void)port;
}

static void IOOUTCALL spr_o58a(UINT port, REG8 dat)
{
//	g_opna[3].s.data = dat;
	opna_writeRegister(&g_opna[3], g_opna[3].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL spr_o58c(UINT port, REG8 dat)
{
	g_opna[3].s.addrh = dat;
//	g_opna[3].s.data = dat;
	(void)port;
}

static void IOOUTCALL spr_o58e(UINT port, REG8 dat)
{
//	g_opna[3].s.data = dat;
	opna_writeExtendedRegister(&g_opna[3], g_opna[3].s.addrh, dat);

	(void)port;
}

static REG8 IOINPCALL spr_i588(UINT port)
{
	(void)port;
	return g_opna[1].s.status;
}

static REG8 IOINPCALL spr_i58a(UINT port)
{
	(void)port;
	return opna_readRegister(&g_opna[3], g_opna[3].s.addrl);
}

static REG8 IOINPCALL spr_i58c(UINT port)
{
	(void)port;
	return (g_opna[1].s.status & 3);
}

static REG8 IOINPCALL spr_i58e(UINT port)
{
	(void)port;
	return opna_read3438ExtRegister(&g_opna[3], g_opna[3].s.addrl);
}



// ----

static const IOOUT spb_o[4] = {
			spb_o188,	spb_o18a,	spb_o18c,	spb_o18e};

static const IOINP spb_i[4] = {
			spb_i188,	spb_i18a,	spb_i188,	spb_i18e};

static const IOOUT spb_o2[4] = {
			spb_o088,	spb_o08a,	spb_o08c,	spb_o08e};

static const IOINP spb_i2[4] = {
			spb_i088,	spb_i08a,	spb_i088,	spb_i08e};

static const IOOUT p86_o3[4] = {
			p86_o288,	p86_o28a,	p86_o28c,	p86_o28e};

static const IOINP p86_i3[4] = {
			p86_i288,	p86_i28a,	p86_i288,	p86_i28e};

// ----

static const IOOUT spr_o[4] = {
			spr_o588,	spr_o58a,	spr_o58c,	spr_o58e};

static const IOINP spr_i[4] = {
			spr_i588,	spr_i58a,	spr_i58c,	spr_i58e};

static const IOOUT spr_o2[4] = {
			spr_o488,	spr_o48a,	spr_o48c,	spr_o48e};

static const IOINP spr_i2[4] = {
			spr_i488,	spr_i48a,	spr_i48c,	spr_i48e};

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void boardpx1_reset(const NP2CFG *pConfig)
{
	UINT nIrq1;
	UINT nIrq2;

	nIrq1 = (pConfig->spbopt & 0xc0) | 0x10;
	nIrq2 = (nIrq1 == 0xd0) ? 0x90 : 0xd0;

	opna_reset(&g_opna[0], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_HAS_ADPCM | OPNA_HAS_VR | OPNA_S98);
	opna_timer(&g_opna[0], nIrq1, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	opna_reset(&g_opna[1], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_HAS_ADPCM | OPNA_HAS_VR);
	opna_timer(&g_opna[1], nIrq2, NEVENT_FMTIMER2A, NEVENT_FMTIMER2B);
	opna_reset(&g_opna[2], OPNA_MODE_3438 | OPNA_HAS_VR);
	opna_reset(&g_opna[3], OPNA_MODE_3438 | OPNA_HAS_VR);

	opngen_setcfg(&g_opna[0].opngen, 6, OPN_STEREO | 0x3f);
	opngen_setcfg(&g_opna[1].opngen, 6, OPN_STEREO | 0x3f);
	opngen_setcfg(&g_opna[2].opngen, 6, OPN_STEREO | 0x3f);
	opngen_setcfg(&g_opna[3].opngen, 6, OPN_STEREO | 0x3f);
	soundrom_loadex(pConfig->spbopt & 7, OEMTEXT("SPB"));
	g_opna[0].s.base = (pConfig->spbopt & 0x10) ? 0x000 : 0x100;
}

/**
 * Bind
 */
void boardpx1_bind(void)
{
	opna_bind(&g_opna[0]);
	opna_bind(&g_opna[1]);
	opna_bind(&g_opna[2]);
	opna_bind(&g_opna[3]);
	cbuscore_attachsndex(0x188, spb_o, spb_i);
	cbuscore_attachsndex(0x588, spr_o, spr_i);
	cbuscore_attachsndex(0x088, spb_o2, spb_i2);
	cbuscore_attachsndex(0x488, spr_o2, spr_i2);
}


static void extendchannelx2(REG8 enable) {

	g_opna[4].s.extend = enable;
	if (enable)
	{
		opngen_setcfg(&g_opna[4].opngen, 6, OPN_STEREO | 0x07);
	}
	else
	{
		opngen_setcfg(&g_opna[4].opngen, 3, OPN_MONORAL | 0x07);
		rhythm_setreg(&g_opna[4].rhythm, 0x10, 0xff);
	}
}

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void boardpx2_reset(const NP2CFG *pConfig)
{
	UINT nIrq1;
	UINT nIrq2;

	nIrq1 = (pConfig->spbopt & 0xc0) | 0x10;
	nIrq2 = (nIrq1 == 0xd0) ? 0x90 : 0xd0;

	opna_reset(&g_opna[0], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_HAS_ADPCM | OPNA_HAS_VR | OPNA_S98);
	opna_timer(&g_opna[0], nIrq1, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	opna_reset(&g_opna[1], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_HAS_ADPCM | OPNA_HAS_VR);
	opna_timer(&g_opna[1], nIrq2, NEVENT_FMTIMER2A, NEVENT_FMTIMER2B);
	opna_reset(&g_opna[2], OPNA_MODE_3438 | OPNA_HAS_VR);
	opna_reset(&g_opna[3], OPNA_MODE_3438 | OPNA_HAS_VR);
	opna_reset(&g_opna[4], OPNA_MODE_2608 | OPNA_HAS_ADPCM);

	opngen_setcfg(&g_opna[0].opngen, 6, OPN_STEREO | 0x3f);
	opngen_setcfg(&g_opna[1].opngen, 6, OPN_STEREO | 0x3f);
	opngen_setcfg(&g_opna[2].opngen, 6, OPN_STEREO | 0x3f);
	opngen_setcfg(&g_opna[3].opngen, 6, OPN_STEREO | 0x3f);
	opngen_setcfg(&g_opna[4].opngen, 3, OPN_STEREO | 0x38);
	soundrom_loadex(pConfig->spbopt & 7, OEMTEXT("SPB"));
	g_opna[0].s.base = (pConfig->spbopt & 0x10) ? 0x000 : 0x100;
	fmboard_extreg(extendchannelx2);
	pcm86io_setopt(0x00);
}

/**
 * Bind
 */
void boardpx2_bind(void)
{
	opna_bind(&g_opna[0]);
	opna_bind(&g_opna[1]);
	opna_bind(&g_opna[2]);
	opna_bind(&g_opna[3]);
	opna_bind(&g_opna[4]);
	pcm86io_bind();
	cbuscore_attachsndex(0x188, spb_o, spb_i);
	cbuscore_attachsndex(0x588, spr_o, spr_i);
	cbuscore_attachsndex(0x088, spb_o2, spb_i2);
	cbuscore_attachsndex(0x488, spr_o2, spr_i2);
	cbuscore_attachsndex(0x288, p86_o3, p86_i3);
}

#endif	// defined(SUPPORT_PX)

