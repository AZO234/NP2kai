/**
 * @file	boardmo.c
 * @brief	Implementation of SNE Multimedia Orchestra
 */

#include	<compiler.h>
#include	<pccore.h>
#include	"boardmo.h"
#include	<io/iocore.h>
#include	<cbus/cbuscore.h>
#include	<sound/fmboard.h>
#include	<sound/sound.h>
#include	<sound/soundrom.h>
#include	<sound/ymzadpcm.h>
#include	<sound/s98.h>

/**
 * SNE Multimedia Orchestra
 * YM2203C(OPN) + YMF262-M(OPL3) + YMZ263B(MMA)
 *
 * np2sより移植 修正BSDライセンス
 * YMZ263B(MMA)のI/Oポートは恐らく次の通り。
 * ・D0D0h YMZ263Bステータス
 * ・D4D0h YMZ263Bレジスタアドレス
 * ・D0D2h YMZ263Bレジスタデータ
 * とりあえず使われていそうなADPCMのみ暫定実装。OPAN ADPCMに宿借り状態のため、正式な実装時にはYMZ263B(MMA)を分離独立すべき。
 */

#define G_OPL3_INDEX	2	/* fmboard.c: 0=118, 1=SB16, 2=Sound Orchestra */
#define BOARDMO_OPL3_CLOCK	14400000

#ifdef USE_MAME
#ifdef USE_MAME_BSD
#if _MSC_VER < 1900
#include "sound/mamebsdsub/np2interop.h"
#else
#include "sound/mamebsd/np2interop.h"
#endif
#else
#include "sound/mame/np2interop.h"
#endif
static int s_samplerate;

#if defined(USE_MAME_BSD)
#define boardmo_YMF262ResetChip(chip, rate)	YMF262ResetChip((chip), (rate))
#else
#define boardmo_YMF262ResetChip(chip, rate)	YMF262ResetChip((chip))
#endif
#endif	/* USE_MAME */

MMOSTAT mmostat;

static void boardmo_setopnpan(void)
{
	psggen_setpan(&g_opna[0].psg, 0, 2);
	psggen_setpan(&g_opna[0].psg, 1, 2);
	psggen_setpan(&g_opna[0].psg, 2, 2);

	g_opna[0].s.reg[0xb4] = 1 << 6;
	g_opna[0].s.reg[0xb5] = 1 << 6;
	g_opna[0].s.reg[0xb6] = 1 << 6;
	opngen_setreg(&g_opna[0].opngen, 0, 0xb4, 1 << 6);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb5, 1 << 6);
	opngen_setreg(&g_opna[0].opngen, 0, 0xb6, 1 << 6);
}

/* ---- OPN */

static void IOOUTCALL opn_o188(UINT port, REG8 dat)
{
	g_opna[0].s.addrl = dat;
	g_opna[0].s.data = dat;
	(void)port;
}

static void IOOUTCALL opn_o18a(UINT port, REG8 dat)
{
	UINT nAddress;

	nAddress = g_opna[0].s.addrl;

	if ((nAddress & 0xb4) == 0xb4)
	{
		return;
	}

	g_opna[0].s.data = dat;
	opna_writeRegister(&g_opna[0], nAddress, dat);

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

/* ---- OPL3 */

static void IOOUTCALL opl_o288(UINT port, REG8 dat)
{
	g_opl3[G_OPL3_INDEX].s.addrl = dat;
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		YMF262Write(g_mame_opl3[G_OPL3_INDEX], 0, dat);
	}
#endif
	(void)port;
}

static void IOOUTCALL opl_o28a(UINT port, REG8 dat)
{
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		YMF262Write(g_mame_opl3[G_OPL3_INDEX], 1, dat);
	}
#endif
	S98_put(NORMAL2608_2, g_opl3[G_OPL3_INDEX].s.addrl, dat);
	opl3_writeRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrl, dat);
	(void)port;
}

static void IOOUTCALL opl_o28c(UINT port, REG8 dat)
{
	g_opl3[G_OPL3_INDEX].s.addrh = dat;
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		YMF262Write(g_mame_opl3[G_OPL3_INDEX], 2, dat);
	}
#endif
	(void)port;
}

static void IOOUTCALL opl_o28e(UINT port, REG8 dat)
{
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		YMF262Write(g_mame_opl3[G_OPL3_INDEX], 3, dat);
	}
#endif
	S98_put(EXTEND2608_2, g_opl3[G_OPL3_INDEX].s.addrh, dat);
	opl3_writeExtendedRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrh, dat);
	(void)port;
}

static REG8 IOINPCALL opl_i288(UINT port)
{
	(void)port;
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 0);
	}
#endif
	return opl3_readStatus(&g_opl3[G_OPL3_INDEX]);
}

static REG8 IOINPCALL opl_i28a(UINT port)
{
	(void)port;
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 1);
	}
#endif
	return opl3_readRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrl);
}

static REG8 IOINPCALL opl_i28c(UINT port)
{
	(void)port;
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 2);
	}
#endif
	return opl3_readStatus(&g_opl3[G_OPL3_INDEX]);
}

static REG8 IOINPCALL opl_i28e(UINT port)
{
	(void)port;
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 3);
	}
#endif
	return opl3_readExtendedRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrh);
}

#ifdef USE_MAME
#define OPL3_SAMPLE_BUFFER	4096
static INT16 s_opl3_s1l[OPL3_SAMPLE_BUFFER];
static INT16 s_opl3_s1r[OPL3_SAMPLE_BUFFER];
static INT16 s_opl3_s2l[OPL3_SAMPLE_BUFFER];
static INT16 s_opl3_s2r[OPL3_SAMPLE_BUFFER];

static void SOUNDCALL boardmo_opl3_getpcm(void *opl3, SINT32 *pcm, UINT count)
{
	INT16 *buf[4];
	SINT32 *out;
	SINT32 volume;

	buf[0] = s_opl3_s1l;
	buf[1] = s_opl3_s1r;
	buf[2] = s_opl3_s2l;
	buf[3] = s_opl3_s2r;
	out = pcm;
	volume = np2cfg.vol_fm * np2cfg.vol_master / 100;

	while (count > 0)
	{
		UINT i;
		UINT cc = (count < OPL3_SAMPLE_BUFFER) ? count : OPL3_SAMPLE_BUFFER;
		YMF262UpdateOne(opl3, buf, cc);
		for (i = 0; i < cc; i++)
		{
			out[0] += ((SINT32)s_opl3_s1l[i] * volume) >> 7;
			out[1] += ((SINT32)s_opl3_s1r[i] * volume) >> 7;
			out += 2;
		}
		count -= cc;
	}
}

static void SOUNDCALL boardmo_opl3_getpcm_dummy(void *opl3, SINT32 *pcm, UINT count)
{
	INT16 *buf[4];

	buf[0] = s_opl3_s1l;
	buf[1] = s_opl3_s1r;
	buf[2] = s_opl3_s2l;
	buf[3] = s_opl3_s2r;
	(void)pcm;

	while (count > 0)
	{
		UINT cc = (count < OPL3_SAMPLE_BUFFER) ? count : OPL3_SAMPLE_BUFFER;
		YMF262UpdateOne(opl3, buf, cc);
		count -= cc;
	}
}

static void boardmo_mame_reset(void)
{
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		if (s_samplerate != soundcfg.rate)
		{
			YMF262Shutdown(g_mame_opl3[G_OPL3_INDEX]);
			g_mame_opl3[G_OPL3_INDEX] = YMF262Init(BOARDMO_OPL3_CLOCK, soundcfg.rate);
			s_samplerate = soundcfg.rate;
		}
		else
		{
			boardmo_YMF262ResetChip(g_mame_opl3[G_OPL3_INDEX], soundcfg.rate);
		}
	}
}

static void boardmo_mame_bind(void)
{
	if (!g_mame_opl3[G_OPL3_INDEX])
	{
		g_mame_opl3[G_OPL3_INDEX] = YMF262Init(BOARDMO_OPL3_CLOCK, np2cfg.samplingrate);
		s_samplerate = np2cfg.samplingrate;
	}

	if (g_opl3[G_OPL3_INDEX].userdata)
	{
		sound_streamregist(g_mame_opl3[G_OPL3_INDEX], (SOUNDCB)boardmo_opl3_getpcm_dummy);
	}
	else
	{
		sound_streamregist(g_mame_opl3[G_OPL3_INDEX], (SOUNDCB)boardmo_opl3_getpcm);
	}
}
#endif	/* USE_MAME */

/* ---- */

static const IOOUT opn_o[4] =
{
	opn_o188,	opn_o18a,	opl_o288,	opl_o28a
};

static const IOINP opn_i[4] =
{
	opn_i188,	opn_i18a,	opl_i288,	opl_i28a
};

static const IOOUT opl_o[4] =
{
	opl_o288,	opl_o28a,	opl_o28c,	opl_o28e
};

static const IOINP opl_i[4] =
{
	opl_i288,	opl_i28a,	opl_i28c,	opl_i28e
};

static UINT mmo_rate_table[4] = {
	22050,	/* FS=0 */
	11025,	/* FS=1 */
	7350,	/* FS=2 */
	5513	/* FS=3 */
};

void mmo_timer(NEVENTITEM item)
{
	UINT rate = mmo_rate_table[(mmostat.reg[0x09] >> 3) & 3];
	UINT samples = (((mmostat.reg[0x0c] >> 2) & 7) + 1) << 4;
	if (samples == 0) samples = 64;

	pic_setirq(3); // 暫定で割り込みは3固定とする
	if (mmostat.playing) {
		nevent_set(NEVENT_FMTIMER2A, (SINT32)((UINT64)pccore.realclock * samples / rate), mmo_timer, NEVENT_RELATIVE); // 未使用のNEVENT_FMTIMER2Aを借りる
	}
}

static void IOOUTCALL mmo_od0d0(UINT port, REG8 dat)
{
	(void)port;
}
static REG8 IOINPCALL mmo_id0d0(UINT port)
{
	mmostat.irqflag = 0;
	pic_resetirq(3);
	return(1);
}
static void IOOUTCALL mmo_od4d0(UINT port, REG8 dat)
{
	mmostat.current_reg = dat;
}
static void IOOUTCALL mmo_od0d2(UINT port, REG8 dat)
{
	// YMZ263B レジスタ操作
	mmostat.reg[mmostat.current_reg & 0xf] = dat;
	switch (mmostat.current_reg) {
	case 0x09:
		if ((dat & 0x83) == 0x03) {
			// 暫定 YMZの値をOPNA delta_n換算
			UINT rate;
			UINT32 base;
			UINT32 delta_n;
			UINT samples = (((mmostat.reg[0x0c] >> 2) & 7) + 1) << 4;
			if (samples == 0) samples = 64;
			rate = mmo_rate_table[(dat >> 3) & 3];
			base = (OPNA_CLOCK / 72);
			if (base == 0) {
				delta_n = 0x0100;
			}
			else {
				delta_n = (UINT32)(((UINT64)rate * 0x10000 + (base / 2)) / base);
				if (delta_n < 0x0100) {
					delta_n = 0x0100;
				}
				else if (delta_n > 0xffff) {
					delta_n = 0xffff;
				}
			}

			ymzadpcm_setreg(&(g_opna[0].adpcm), 0, 0x81);
			ymzadpcm_setreg(&(g_opna[0].adpcm), 0x0b, 0xff);
			ymzadpcm_setreg(&(g_opna[0].adpcm), 0x01, 0xc0);
			ymzadpcm_setreg(&(g_opna[0].adpcm), 0x09, (REG8)(delta_n & 0xff));
			ymzadpcm_setreg(&(g_opna[0].adpcm), 0x0a, (REG8)(delta_n >> 8));
			ymzadpcm_setreg(&(g_opna[0].adpcm), 0, 0x80);
			mmostat.playing = 1;
			mmostat.stoppending = 0;
			pic_resetirq(3);
			mmostat.irqflag = 0;
			nevent_set(NEVENT_FMTIMER2A, (SINT32)((UINT64)pccore.realclock * samples / rate), mmo_timer, NEVENT_ABSOLUTE); // 未使用のNEVENT_FMTIMER2Aを借りる
		}
		break;

	case 0x0A:
		if (dat == 0x00) {
			mmostat.playing = 0;
		}
		break;

	case 0x0B:
		// ADPCMデータ本体
		ymzadpcm_setreg(&(g_opna[0].adpcm), 0x08, dat);
		break;

	case 0x0C:
		//ymzadpcm_setreg(&(g_opna[0].adpcm), 0, 0x81);
		break;
	}
}

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void boardmo_reset(const NP2CFG *pConfig)
{
	opna_reset(&g_opna[0], OPNA_MODE_2203 | OPNA_HAS_TIMER | OPNA_S98);
	opna_timer(&g_opna[0], (pConfig->snd26opt & 0xc0) | 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	opngen_setcfg(&g_opna[0].opngen, 3, OPN_STEREO | 0x007);
	soundrom_loadex(pConfig->snd26opt & 7, OEMTEXT("MO"));
	g_opna[0].s.base = (pConfig->snd26opt & 0x10) ? 0x000 : 0x100;
	boardmo_setopnpan();

	opl3_reset(&g_opl3[G_OPL3_INDEX], OPL3_MODE_262);
#ifdef USE_MAME
	boardmo_mame_reset();
#endif
	ymzadpcm_reset(&g_opna[0].adpcm);

	memset(&mmostat, 0, sizeof(MMOSTAT));
}

/**
 * Bind
 */
void boardmo_bind(void)
{
	boardmo_setopnpan();
	opna_bind(&g_opna[0]);
	boardmo_setopnpan();

	opl3_bind(&g_opl3[G_OPL3_INDEX]);
#ifdef USE_MAME
	boardmo_mame_bind();
#endif

	cbuscore_attachsndex(0x188 - g_opna[0].s.base, opn_o, opn_i);
	cbuscore_attachsndex(0x288, opl_o, opl_i);

	iocore_attachinp(0xd0d0, mmo_id0d0);
	iocore_attachout(0xd0d0, mmo_od0d0);
	iocore_attachout(0xd4d0, mmo_od4d0);
	iocore_attachout(0xd0d2, mmo_od0d2);

	sound_streamregist(&g_opna[0].adpcm, (SOUNDCB)ymzadpcm_getpcm);
}

/**
 * Unbind
 */
void boardmo_unbind(void)
{
	cbuscore_detachsndex(0x188 - g_opna[0].s.base);
	cbuscore_detachsndex(0x288);
}

/**
 * Finalize
 */
void boardmo_finalize(void)
{
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX])
	{
		YMF262Shutdown(g_mame_opl3[G_OPL3_INDEX]);
		g_mame_opl3[G_OPL3_INDEX] = NULL;
	}
#endif
}
