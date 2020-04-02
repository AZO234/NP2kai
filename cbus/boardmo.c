#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"boardso.h"
#include	"sound.h"
#include	"soundrom.h"
#include	"fmboard.h"
#include	"s98.h"

#define G_OPL3_INDEX1	2
#define G_OPL3_INDEX2	3

/**
 * SNE Multimedia Orchestra
 * YM2203C(OPN) + YMF262-M(OPL3) + YMZ263B(MMA)
 *
 * 対応ソフト/マニュアルが無い為に現状MMAのポートが不明。
 *
 */
static void *opl3;
static int samplerate;

/*
void *YMF262Init(INT clock, INT rate);
void YMF262ResetChip(void *chip);
void YMF262Shutdown(void *chip);
INT YMF262Write(void *chip, INT a, INT v);
UINT8 YMF262Read(void *chip, INT a);
void YMF262UpdateOne(void *chip, INT16 **buffer, INT length);
*/

static void IOOUTCALL opn_o188(UINT port, REG8 dat) {

	g_opna[0].s.addrl = dat;
	g_opna[0].s.data = dat;
	(void)port;
}

static void IOOUTCALL opn_o18a(UINT port, REG8 dat) {

	UINT	addr;

	if ((g_opna[0].s.addrl & 0xb4) == 0xb4)
		return;

//	g_opna[0].s.data = dat;
	addr = g_opna[0].s.addrl;
	if (addr != 0xf)
		S98_put(NORMAL2608, addr, dat);
	opna_writeRegister(&g_opna[0], g_opna[0].s.addrl, dat);
/*
	if (addr < 0x10) {
		if (addr != 0x0e) {
			psggen_setreg(&psg1, addr, dat);
		}
	}
	else if (addr < 0x100) {
		if (addr < 0x30) {
			if (addr == 0x28) {
				if ((dat & 0x0f) < 3) {
					opngen_keyon(dat & 0x0f, dat);
				}
			}
			else {
				fmtimer_setreg(addr, dat);
				if (addr == 0x27) {
					opnch[2].extop = dat & 0xc0;
				}
			}
		}
		else if (addr < 0xc0) {
			opngen_setreg(0, addr, dat);
		}
		g_opna[0].s.reg[addr] = dat;
	}
*/
	(void)port;
}

static void IOOUTCALL opl_o288(UINT port, REG8 dat) {
	(void)port;
	g_opl3[G_OPL3_INDEX1].s.addrl = dat;
//	YMF262Write(opl3, 0, dat);
}

static void IOOUTCALL opl_o28a(UINT port, REG8 dat) {
	(void)port;
	S98_put(NORMAL2608_2, g_opl3[G_OPL3_INDEX1].s.addrl, dat);
	g_opl3[G_OPL3_INDEX1].s.reg[g_opl3[G_OPL3_INDEX1].s.addrl] = dat;
//	YMF262Write(opl3, 1, dat);
}

static void IOOUTCALL opl_o28c(UINT port, REG8 dat) {
	(void)port;
	g_opl3[G_OPL3_INDEX2].s.addrl = dat;
//	YMF262Write(opl3, 2, dat);
}

static void IOOUTCALL opl_o28e(UINT port, REG8 dat) {
	(void)port;
	S98_put(EXTEND2608_2, g_opl3[G_OPL3_INDEX2].s.addrl, dat);
	g_opl3[G_OPL3_INDEX2].s.reg[g_opl3[G_OPL3_INDEX2].s.addrl] = dat;
//	YMF262Write(opl3, 3, dat);
}

static REG8 IOINPCALL opn_i188(UINT port) {

	(void)port;
	return g_opna[0].s.status;
}

static REG8 IOINPCALL opn_i18a(UINT port) {

	UINT	addr;

	addr = g_opna[0].s.addrl;
	if (addr == 0x0e) {
		return(fmboard_getjoy(&g_opna[0]));
	}
	else if (addr < 0x10) {
		return(psggen_getreg(&g_opna[0], addr));
	}
	(void)port;
	return(g_opna[0].s.data);
}

static REG8 IOINPCALL opl_i288(UINT port) {
	(void)port;
	return opl3_readStatus(&g_opl3[G_OPL3_INDEX1]);
}

static REG8 IOINPCALL opl_i28a(UINT port) {
	(void)port;
	return opl3_readRegister(&g_opl3[G_OPL3_INDEX1], g_opl3[G_OPL3_INDEX1].s.addrl);
}
static REG8 IOINPCALL opl_i28c(UINT port) {
	(void)port;
	return opl3_readStatus(&g_opl3[G_OPL3_INDEX2]);
}

static REG8 IOINPCALL opl_i28e(UINT port) {
	(void)port;
	return opl3_readRegister(&g_opl3[G_OPL3_INDEX2], g_opl3[G_OPL3_INDEX2].s.addrl);
}
// ----

static const IOOUT opn_o[4] = {
			opn_o188,	opn_o18a,	opl_o288,		opl_o28a};

static const IOINP opn_i[4] = {
			opn_i188,	opn_i18a,	opl_i288,		opl_i28a};

static const IOOUT opl_o[4] = {
			opl_o288,	opl_o28a,	opl_o28c,		opl_o28e};

static const IOINP opl_i[4] = {
			opl_i288,	opl_i28a,	opl_i28c,		opl_i28e};

static void psgpanset(OPNA* psg) {
	// SSGはL固定
	psggen_setpan(psg, 0, 2);
	psggen_setpan(psg, 1, 2);
	psggen_setpan(psg, 2, 2);
}

extern void SOUNDCALL opl3gen_getpcm(void* opl3, SINT32 *pcm, UINT count);

void boardmo_reset(const NP2CFG *pConfig) {

	opngen_setcfg(&g_opna[0].opngen, 3, OPN_STEREO | 0x007);
	opna_timer(&g_opna[0], (pConfig->snd26opt & 0xc0) | 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	soundrom_loadex(pConfig->snd26opt & 7, OEMTEXT("MO"));
	g_opna[0].s.base = (pConfig->snd26opt & 0x10)?0x000:0x100;
	opngen_setreg(&g_opna[0].opngen, 0, 0xb4, 1 << 6);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb5, 1 << 6);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb6, 1 << 6);
/*
	ZeroMemory(&g_opl3[G_OPL3_INDEX1], sizeof(g_opl3[G_OPL3_INDEX1]));
	ZeroMemory(&g_opl3[G_OPL3_INDEX2], sizeof(g_opl3[G_OPL3_INDEX2]));
	if (opl3) {
		if (samplerate != pConfig->samplingrate) {
			YMF262Shutdown(opl3);
			opl3 = YMF262Init(14400000, pConfig->samplingrate);
			samplerate = pConfig->samplingrate;
		} else {
			YMF262ResetChip(opl3);
		}
	}
*/
	opl3_reset(&g_opl3[G_OPL3_INDEX1], (REG8)OPL3_MODE_3812);
	opl3_reset(&g_opl3[G_OPL3_INDEX2], (REG8)OPL3_MODE_3812);
}

void boardmo_bind(void) {
	psgpanset(&g_opna[0]);
//	fmboard_fmrestore(0, 0);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb4, 1 << 6);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb5, 1 << 6);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb6, 1 << 6);
	psggen_restore(&g_opna[0].opngen);
	sound_streamregist(&g_opna[0].opngen, (SOUNDCB)opngen_getpcm);
	sound_streamregist(&g_opna[0].opngen, (SOUNDCB)psggen_getpcm);
	cbuscore_attachsndex(0x188 - g_opna[0].s.base, opn_o, opn_i);
	cbuscore_attachsndex(0x288, opl_o, opl_i);
/*
	if (!opl3) {
		opl3 = YMF262Init(15974400, np2cfg.samplingrate);
		samplerate = np2cfg.samplingrate;
	}
	sound_streamregist(opl3, (SOUNDCB)opl3gen_getpcm);
*/
}
void boardmo_unbind(void)
{
	cbuscore_detachsndex(0x188 - g_opna[0].s.base);
}

