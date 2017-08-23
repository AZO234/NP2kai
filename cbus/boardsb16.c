#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"boardso.h"
#include	"sound.h"
#include	"ct1741io.h"
#include	"ct1745io.h"
#include	"fmboard.h"
//#include	"s98.h"

#ifdef SUPPORT_SOUND_SB16

/**
 * Creative Sound Blaster 16(98)
 * YMF262-M(OPL3) + CT1741(PCM) + CT1745(MIXER) + YM2203(OPN - option)
 *
 * 現状は以下の固定仕様で動く IO:D2 DMA:3 INT:5 
 */

static void *opl3;
static const UINT8 sb16base[] = {0xd2,0xd4,0xd6,0xd8,0xda,0xdc,0xde};
static int samplerate;

void *YMF262Init(INT clock, INT rate);
void YMF262ResetChip(void *chip);
void YMF262Shutdown(void *chip);
INT YMF262Write(void *chip, INT a, INT v);
UINT8 YMF262Read(void *chip, INT a);
void YMF262UpdateOne(void *chip, INT16 **buffer, INT length);

static void IOOUTCALL sb16_o0400(UINT port, REG8 dat) {
	port = dat;
}
static void IOOUTCALL sb16_o0500(UINT port, REG8 dat) {
	port = dat;
}
static void IOOUTCALL sb16_o0600(UINT port, REG8 dat) {
	port = dat;
}
static void IOOUTCALL sb16_o0700(UINT port, REG8 dat) {
	port = dat;
}
static void IOOUTCALL sb16_o8000(UINT port, REG8 dat) {
	/* MIDI Data Port */
//	TRACEOUT(("SB16-midi commands: %.2x", dat));
	port = dat;
}
static void IOOUTCALL sb16_o8100(UINT port, REG8 dat) {
	/* MIDI Stat Port */
//	TRACEOUT(("SB16-midi status: %.2x", dat));
	port = dat;
}

static void IOOUTCALL sb16_o2000(UINT port, REG8 dat) {
	(void)port;
	g_opl.addr = dat;
	YMF262Write(opl3, 0, dat);
}

static void IOOUTCALL sb16_o2100(UINT port, REG8 dat) {
	(void)port;
	g_opl.reg[g_opl.addr] = dat;
	//S98_put(NORMAL2608, g_opl.addr, dat);
	YMF262Write(opl3, 1, dat);
}
static void IOOUTCALL sb16_o2200(UINT port, REG8 dat) {
	(void)port;
	g_opl.addr2 = dat;
	YMF262Write(opl3, 2, dat);
}

static void IOOUTCALL sb16_o2300(UINT port, REG8 dat) {
	(void)port;
	g_opl.reg[g_opl.addr2 + 0x100] = dat;
	//S98_put(EXTEND2608, opl.addr2, dat);
	YMF262Write(opl3, 3, dat);
}

static void IOOUTCALL sb16_o2800(UINT port, REG8 dat) {
	/**
	 * いわゆるPC/ATで言うところのAdlib互換ポート
	 * UltimaUnderWorldではこちらを叩く
	 */
	port = dat;
	YMF262Write(opl3, 0, dat);
}
static void IOOUTCALL sb16_o2900(UINT port, REG8 dat) {
	port = dat;
	YMF262Write(opl3, 1, dat);
}

static REG8 IOINPCALL sb16_i0400(UINT port) {
	return 0xff;
}
static REG8 IOINPCALL sb16_i0500(UINT port) {
	return 0;
}
static REG8 IOINPCALL sb16_i0600(UINT port) {
	return 0;
}
static REG8 IOINPCALL sb16_i0700(UINT port) {
	return 0;
}
static REG8 IOINPCALL sb16_i8000(UINT port) {
	/* Midi Port */
	return 0;
}
static REG8 IOINPCALL sb16_i8100(UINT port) {
	/* Midi Port */
	return 0;
}


static REG8 IOINPCALL sb16_i2000(UINT port) {
	(void)port;
	return YMF262Read(opl3, 0);
}

static REG8 IOINPCALL sb16_i2200(UINT port) {
	(void)port;
	return YMF262Read(opl3, 1);
}

static REG8 IOINPCALL sb16_i2800(UINT port) {
	(void)port;
	return YMF262Read(opl3, 0);
}

// ----

void SOUNDCALL opl3gen_getpcm(void* opl3, SINT32 *pcm, UINT count) {
	UINT i;
	INT16 *buf[4];
	INT16 s1l,s1r,s2l,s2r;
	SINT32 *outbuf = pcm;
	buf[0] = &s1l;
	buf[1] = &s1r;
	buf[2] = &s2l;
	buf[3] = &s2r;
	for (i=0; i < count; i++) {
		s1l = s1r = s2l = s2r = 0;
		YMF262UpdateOne(opl3, buf, 1);
		outbuf[0] += s1l << 1;
		outbuf[1] += s1r << 1;
		outbuf += 2;
	}
}

void boardsb16_reset(const NP2CFG *pConfig) {
	if (opl3) {
		if (samplerate != pConfig->samplingrate) {
			YMF262Shutdown(opl3);
			opl3 = YMF262Init(14400000, pConfig->samplingrate);
			samplerate = pConfig->samplingrate;
		} else {
			YMF262ResetChip(opl3);
		}
	}
	ZeroMemory(&g_sb16, sizeof(g_sb16));
	ZeroMemory(&g_opl, sizeof(g_opl));
	// ボードデフォルト IO:D2 DMA:3 INT:5 
	g_sb16.base = 0xd2;
	g_sb16.dmach = 0x3;
	g_sb16.dmairq = 0x5;
	ct1745io_reset();
	ct1741io_reset();
}

void boardsb16_bind(void) {
	ct1745io_bind();
	ct1741io_bind();

	iocore_attachout(0x2000 + g_sb16.base, sb16_o2000);	/* FM Music Register Address Port */
	iocore_attachout(0x2100 + g_sb16.base, sb16_o2100);	/* FM Music Data Port */
	iocore_attachout(0x2200 + g_sb16.base, sb16_o2200);	/* Advanced FM Music Register Address Port */
	iocore_attachout(0x2300 + g_sb16.base, sb16_o2300);	/* Advanced FM Music Data Port */
	iocore_attachout(0x2800 + g_sb16.base, sb16_o2800);	/* FM Music Register Port */
	iocore_attachout(0x2900 + g_sb16.base, sb16_o2900);	/* FM Music Data Port */

	iocore_attachinp(0x2000 + g_sb16.base, sb16_i2000);	/* FM Music Status Port */
	iocore_attachinp(0x2200 + g_sb16.base, sb16_i2200);	/* Advanced FM Music Status Port */
	iocore_attachinp(0x2800 + g_sb16.base, sb16_i2800);	/* FM Music Status Port */

	iocore_attachout(0x0400 + g_sb16.base, sb16_o0400);	/* GAME Port */
	iocore_attachout(0x0500 + g_sb16.base, sb16_o0500);	/* GAME Port */
	iocore_attachout(0x0600 + g_sb16.base, sb16_o0600);	/* GAME Port */
	iocore_attachout(0x0700 + g_sb16.base, sb16_o0700);	/* GAME Port */
	iocore_attachinp(0x0400 + g_sb16.base, sb16_i0400);	/* GAME Port */
	iocore_attachinp(0x0500 + g_sb16.base, sb16_i0500);	/* GAME Port */
	iocore_attachinp(0x0600 + g_sb16.base, sb16_i0600);	/* GAME Port */
	iocore_attachinp(0x0700 + g_sb16.base, sb16_i0700);	/* GAME Port */

	iocore_attachout(0x8000 + g_sb16.base, sb16_o8000);	/* MIDI Port */
	iocore_attachout(0x8100 + g_sb16.base, sb16_o8100);	/* MIDI Port */
	iocore_attachinp(0x8000 + g_sb16.base, sb16_i8000);	/* MIDI Port */
	iocore_attachinp(0x8100 + g_sb16.base, sb16_i8100);	/* MIDI Port */

	if (!opl3) {
		opl3 = YMF262Init(14400000, np2cfg.samplingrate);
		samplerate = np2cfg.samplingrate;
	}
	sound_streamregist(opl3, (SOUNDCB)opl3gen_getpcm);
}

#endif