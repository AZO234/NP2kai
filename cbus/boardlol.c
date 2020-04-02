#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"boardlol.h"
#include	"sound.h"
#include	"soundrom.h"
#include	"fmboard.h"
#include	"s98.h"

/**
 * SNE Little Orchestra
 * YM2203(OPN) with panning
 *
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

	g_opna[0].s.data = dat;
	addr = g_opna[0].s.addrl;
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
		return opna_readRegister(&g_opna[0], addr);
	}
	(void)port;
	return(g_opna[0].s.data);
}

// ----

static const IOOUT opn_o[4] = {
			opn_o188,	opn_o18a,	NULL,		NULL};

static const IOINP opn_i[4] = {
			opn_i188,	opn_i18a,	NULL,		NULL};

static void psgpanset(OPNA* psg) {
	// SSG‚ÍRŒÅ’è
	psggen_setpan(psg, 0, 1);
	psggen_setpan(psg, 1, 1);
	psggen_setpan(psg, 2, 1);
}

void boardlol_reset(const NP2CFG *pConfig) {

	opngen_setcfg(&g_opna[0].opngen, 3, OPN_STEREO | 0x007);
	opna_timer(&g_opna[0], (pConfig->snd26opt & 0xc0) | 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	soundrom_loadex(pConfig->snd26opt & 7, OEMTEXT("26"));
	g_opna[0].s.base = (pConfig->snd26opt & 0x10)?0x000:0x100;
	opngen_setreg(&g_opna[0].opngen, 0, 0xb4, 1 << 6);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb5, 1 << 6);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb6, 1 << 6);
}

void boardlol_bind(void) {
	psgpanset(&g_opna[0]);
//	fmboard_fmrestore(0, 0);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb4, 1 << 7);		// OPN‚ÍLŒÅ’è
	opngen_setreg(&g_opna[0].opngen, 0, 0xb5, 1 << 7);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb6, 1 << 7);
	psggen_restore(&g_opna[0]);
	sound_streamregist(&g_opna[0].opngen, (SOUNDCB)opngen_getpcm);
	sound_streamregist(&g_opna[0].opngen, (SOUNDCB)psggen_getpcm);
	cbuscore_attachsndex(0x188 - g_opna[0].s.base, opn_o, opn_i);
}
void boardlol_unbind(void)
{
	cbuscore_detachsndex(0x188 - g_opna[0].s.base);
}

