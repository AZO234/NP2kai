#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"boardso.h"
#include	"sound.h"
#include	"ct1741io.h"
#include	"ct1745io.h"
#include	"fmboard.h"
#include	"mpu98ii.h"
#include	"smpu98.h"
#include	"joymng.h"
#include	"cpucore.h"
//#include	"s98.h"

#include	<math.h>

#if 0
#undef	TRACEOUT
#define USE_TRACEOUT_VS
#ifdef USE_TRACEOUT_VS
static void trace_fmt_ex(const char *fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "¥n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
#else
#define	TRACEOUT(s)	(void)(s)
#endif
#endif	/* 1 */

#ifdef SUPPORT_SOUND_SB16

/**
 * Creative Sound Blaster 16(98)
 * YMF262-M(OPL3) + CT1741(PCM) + CT1745(MIXER) + YM2203(OPN - option)
 *
 * デフォルト仕様 IO:D2 DMA:3 INT:5 
 */

#define G_OPL3_INDEX	1

static const UINT8 sb16base[] = {0xd2,0xd4,0xd6,0xd8,0xda,0xdc,0xde};
static int samplerate;

static UINT8 seq[] = {0x60, 0x80, 0xff, 0x21}; // XXX: Win2kドライバのチェックを通すためだけの暫定シーケンス
static int forceopl3mode = 0;
static int seqpos = 0;

#ifdef USE_MAME
void *YMF262Init(INT clock, INT rate);
void YMF262ResetChip(void *chip);
void YMF262Shutdown(void *chip);
INT YMF262Write(void *chip, INT a, INT v);
UINT8 YMF262Read(void *chip, INT a);
void YMF262UpdateOne(void *chip, INT16 **buffer, INT length);
#endif

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
	//if(g_sb16.dsp_info.uartmode){
		if(mpu98.enable){
			mpu98.mode = 1; // force set UART mode
			mpu98ii_o0(port, dat);
		}else if(smpu98.enable){
			smpu98.mode = 1; // force set UART mode
			smpu98_o0(port, dat);
		}
	//}
	port = dat;
}
static void IOOUTCALL sb16_o8100(UINT port, REG8 dat) {
	/* MIDI Stat Port */
//	TRACEOUT(("SB16-midi status: %.2x", dat));
	//if(g_sb16.dsp_info.uartmode){
		if(mpu98.enable){
			mpu98.mode = 1; // force set UART mode
			mpu98ii_o2(port, dat);
		}else if(smpu98.enable){
			smpu98.mode = 1; // force set UART mode
			smpu98_o2(port, dat);
		}
	//}
	TRACEOUT(("MPU PORT=0x%04x, DATA=0x%02x", port, dat));
	port = dat;
}

static void IOOUTCALL sb16_o2000(UINT port, REG8 dat) {
	(void)port;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 0, dat);
	g_opl3[G_OPL3_INDEX].s.addrl = dat; // Key Display用
#endif
	//TRACEOUT(("OPL3 PORT=0x%04x, DATA=0x%02x", port, dat));
}

static void IOOUTCALL sb16_o2100(UINT port, REG8 dat) {
	(void)port;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 1, dat);
	opl3_writeRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrl, dat); // Key Display用
#endif
	if(g_opl3[G_OPL3_INDEX].s.addrl==2 || g_opl3[G_OPL3_INDEX].s.addrl==4){
		if(seqpos < sizeof(seq) && seq[seqpos]==dat){
			seqpos++;
		}else if(seq[0]==dat){
			seqpos = 1;
		}else{
			seqpos = 0;
		}
	}
	//TRACEOUT(("OPL3 PORT=0x%04x, DATA=0x%02x", port, dat));
}
static void IOOUTCALL sb16_o2200(UINT port, REG8 dat) {
	(void)port;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 2, dat);
	g_opl3[G_OPL3_INDEX].s.addrh = dat; // Key Display用
#endif
	//TRACEOUT(("OPL3 PORT=0x%04x, DATA=0x%02x", port, dat));
}

static void IOOUTCALL sb16_o2300(UINT port, REG8 dat) {
	(void)port;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 3, dat);
	opl3_writeExtendedRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrh, dat); // Key Display用
#endif
	//TRACEOUT(("OPL3 PORT=0x%04x, DATA=0x%02x", port, dat));
}

static void IOOUTCALL sb16_o2800(UINT port, REG8 dat) {
	/**
	 * いわゆるPC/ATで言うところのAdlib互換ポート
	 * UltimaUnderWorldではこちらを叩く
	 */
	port = dat;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 0, dat);
#endif
}
static void IOOUTCALL sb16_o2900(UINT port, REG8 dat) {
	port = dat;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 1, dat);
#endif
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
	//if(g_sb16.dsp_info.uartmode){
		if(mpu98.enable){
			mpu98.mode = 1; // force set UART mode
			return mpu98ii_i0(port);
		}else if(smpu98.enable){
			smpu98.mode = 1; // force set UART mode
			return smpu98_i0(port);
		}
	//}
	return 0;
}
static REG8 IOINPCALL sb16_i8100(UINT port) {
	/* Midi Port */
	//if(g_sb16.dsp_info.uartmode){
	TRACEOUT(("MPU PORT=0x%04x", port));
		if(mpu98.enable){
			mpu98.mode = 1; // force set UART mode
			return mpu98ii_i2(port) & ~0x40; // 強制Ready
		}else if(smpu98.enable){
			smpu98.mode = 1; // force set UART mode
			return smpu98_i2(port) & ~0x40; // 強制Ready
		}
	//}
	return 0x00;
}


static REG8 IOINPCALL sb16_i2000(UINT port) {
	(void)port;
	//TRACEOUT(("OPL3 PORT=0x%04x", port));
#ifdef USE_MAME
	if(!forceopl3mode && seqpos == sizeof(seq)){
		return 0xC0;
	}else{
		return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 0);
	}
#else
	return 0;
#endif
}

static REG8 IOINPCALL sb16_i2200(UINT port) {
	(void)port;
	//TRACEOUT(("OPL3 PORT=0x%04x", port));
#ifdef USE_MAME
	return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 1);
#else
	return 0;
#endif
}

static REG8 IOINPCALL sb16_i2800(UINT port) {
	(void)port;
	//TRACEOUT(("OPL3 PORT=0x%04x", port));
#ifdef USE_MAME
	return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 0);
#else
	return 0;
#endif
}

#if defined(SUPPORT_GAMEPORT)
/*********** Sound Blaster 16 Gameport I/O ***********/
#define GAMEPORT_JOYCOUNTER_MGN2	(gameport_clkmax/100)
//#define GAMEPORT_JOYCOUNTER_MGN	(gameport_clkmax/100)
#define GAMEPORT_JOYCOUNTER_TMPCLK	10000000
#if defined(SUPPORT_IA32_HAXM)
static LARGE_INTEGER gameport_qpf;
static int gameport_useqpc = 0;
#endif
static UINT64 gameport_tsc;
static UINT32 gameport_clkmax;
static REG8 gameport_joyflag_base = 0x00;
static REG8 gameport_joyflag = 0x00;
static UINT32 gameport_threshold_x = 0;
static UINT32 gameport_threshold_y = 0;
//UINT32 gameport_timeoutcounter = 0;
//UINT32 gameport_timeoutinterval = 0;
// joyflag	bit:0		up
// 			bit:1		down
// 			bit:2		left
// 			bit:3		right
// 			bit:4		trigger1 (rapid)
// 			bit:5		trigger2 (rapid)
// 			bit:6		trigger1
// 			bit:7		trigger2
//void gameport_timeoutproc(NEVENTITEM item);
static void IOOUTCALL gameport_o4d2(UINT port, REG8 dat)
{
	REG8 joyflag = joymng_getstat();
	gameport_joyflag_base = joyflag;
	gameport_joyflag = ((joyflag >> 2) & 0x30)  | ((joyflag << 2) & 0xc0) | 0x0f;
#if defined(SUPPORT_IA32_HAXM)
	{
		LARGE_INTEGER li = {0};
		if (QueryPerformanceFrequency(&gameport_qpf)) {
			QueryPerformanceCounter(&li);
			li.QuadPart = li.QuadPart * GAMEPORT_JOYCOUNTER_TMPCLK / gameport_qpf.QuadPart;
			gameport_tsc = li.QuadPart;
			gameport_useqpc = 1;
		}else{
			gameport_tsc = CPU_MSR_TSC;
			gameport_useqpc = 0;
		}
	}
#else
#if defined(USE_TSC)
	if(CPU_REMCLOCK > 0){
		gameport_tsc = CPU_MSR_TSC - CPU_REMCLOCK * pccore.maxmultiple / pccore.multiple;
	}else{
		gameport_tsc = CPU_MSR_TSC;
	}
#else
	gameport_tsc = 0;
#endif
	//gameport_clkmax = pccore.baseclock * pccore.maxmultiple / 1000; // とりあえず1msで･･･
	//gameport_timeoutcounter = 400;
	//gameport_timeoutinterval = gameport_clkmax * 2 / gameport_timeoutcounter;
	//nevent_set(NEVENT_CDWAIT, gameport_timeoutinterval, gameport_timeoutproc, NEVENT_ABSOLUTE);
#endif
	(void)port;
}
//void gameport_timeoutproc(NEVENTITEM item) {
//	if(gameport_timeoutcounter > 0){
//		gameport_timeoutcounter--;
//		nevent_set(NEVENT_CDWAIT, gameport_timeoutinterval, gameport_timeoutproc, NEVENT_ABSOLUTE);
//	}
//}
static REG8 IOINPCALL gameport_i4d2(UINT port)
{
	UINT64 clockdiff;
#if defined(SUPPORT_IA32_HAXM)
	if(gameport_useqpc){
		LARGE_INTEGER li = {0};
		QueryPerformanceCounter(&li);
		li.QuadPart = li.QuadPart * GAMEPORT_JOYCOUNTER_TMPCLK / gameport_qpf.QuadPart;
		clockdiff = (unsigned long long)li.QuadPart - gameport_tsc;
		gameport_clkmax = GAMEPORT_JOYCOUNTER_TMPCLK/2000; // とりあえず0.5msで･･･
	}else{
		clockdiff = CPU_MSR_TSC - gameport_tsc;
		gameport_clkmax = pccore.realclock/2000; // とりあえず0.5msで･･･
	}
#else
#if defined(USE_TSC)
	if(CPU_REMCLOCK > 0){
		clockdiff = CPU_MSR_TSC - CPU_REMCLOCK * pccore.maxmultiple / pccore.multiple - gameport_tsc;
	}else{
		clockdiff = CPU_MSR_TSC - gameport_tsc;
	}
	gameport_clkmax = pccore.baseclock * pccore.maxmultiple / 2000; // とりあえず0.5msで･･･
#else
	gameport_clkmax = 32;
	clockdiff = gameport_tsc;
	gameport_tsc++;
#endif
#endif
	gameport_threshold_x = gameport_clkmax / 2;
	gameport_threshold_y = gameport_clkmax / 2;
	if(~gameport_joyflag_base & 0x1){
		gameport_threshold_y = GAMEPORT_JOYCOUNTER_MGN2;
	}
	if(~gameport_joyflag_base & 0x2){
		gameport_threshold_y = GAMEPORT_JOYCOUNTER_MGN2 + gameport_clkmax;
	}
	if(~gameport_joyflag_base & 0x4){
		gameport_threshold_x = GAMEPORT_JOYCOUNTER_MGN2;
	}
	if(~gameport_joyflag_base & 0x8){
		gameport_threshold_x = GAMEPORT_JOYCOUNTER_MGN2 + gameport_clkmax;
	}
	if(clockdiff >= (UINT64)gameport_threshold_x){
		gameport_joyflag &= ~0x01;
	}
	if(clockdiff >= (UINT64)gameport_threshold_y){
		gameport_joyflag &= ~0x02;
	}
	return gameport_joyflag;
}
#endif

// ----

static void SOUNDCALL opl3gen_getpcm(void* opl3, SINT32 *pcm, UINT count) {
	UINT i;
	INT16 *buf[4];
	INT16 s1l,s1r,s2l,s2r;
	SINT32 *outbuf = pcm;
	SINT32 oplfm_volume;
	SINT32 midivolL = g_sb16.mixregexp[MIXER_MIDI_LEFT];
	SINT32 midivolR = g_sb16.mixregexp[MIXER_MIDI_RIGHT];
	oplfm_volume = np2cfg.vol_fm;
	buf[0] = &s1l;
	buf[1] = &s1r;
	buf[2] = &s2l;
	buf[3] = &s2r;
	for (i=0; i < count; i++) {
		s1l = s1r = s2l = s2r = 0;
#ifdef USE_MAME
		YMF262UpdateOne(opl3, buf, 1);
#endif
		outbuf[0] += (SINT32)(((s1l << 1) * oplfm_volume * midivolL / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_LEFT] / 255) >> 6);
		outbuf[1] += (SINT32)(((s1r << 1) * oplfm_volume * midivolR / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_RIGHT] / 255) >> 6);
		outbuf += 2;
	}
}

void boardsb16_reset(const NP2CFG *pConfig) {
	DSP_INFO olddsp;
	if (g_mame_opl3[G_OPL3_INDEX]) {
		if (samplerate != pConfig->samplingrate) {
#ifdef USE_MAME
			YMF262Shutdown(g_mame_opl3[G_OPL3_INDEX]);
			g_mame_opl3[G_OPL3_INDEX] = YMF262Init(14400000, pConfig->samplingrate);
#endif
			samplerate = pConfig->samplingrate;
		} else {
#ifdef USE_MAME
			YMF262ResetChip(g_mame_opl3[G_OPL3_INDEX]);
#endif
		}
	}
	olddsp = g_sb16.dsp_info; // dsp_infoだけ初期化しない
	ZeroMemory(&g_sb16, sizeof(g_sb16));
	g_sb16.dsp_info = olddsp;
	// ボードデフォルト IO:D2 DMA:3 IRQ:5(INT1) 
	g_sb16.base = np2cfg.sndsb16io; //0xd2;
	g_sb16.dmach = np2cfg.sndsb16dma; //0x3;
	g_sb16.dmairq = np2cfg.sndsb16irq; //0x5;
	ct1745io_reset();
	ct1741io_reset();
	
	forceopl3mode = 0;
	seqpos = 0;
}

void boardsb16_bind(void) {
	opl3_reset(&g_opl3[G_OPL3_INDEX], OPL3_HAS_OPL3L|OPL3_HAS_OPL3);

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
	
#if defined(SUPPORT_GAMEPORT)
	iocore_attachout(0x0400 + g_sb16.base, gameport_o4d2);	/* GAME Port */
	iocore_attachout(0x0500 + g_sb16.base, gameport_o4d2);	/* GAME Port */
	iocore_attachout(0x0600 + g_sb16.base, gameport_o4d2);	/* GAME Port */
	iocore_attachout(0x0700 + g_sb16.base, gameport_o4d2);	/* GAME Port */
	iocore_attachinp(0x0400 + g_sb16.base, gameport_i4d2);	/* GAME Port */
	iocore_attachinp(0x0500 + g_sb16.base, gameport_i4d2);	/* GAME Port */
	iocore_attachinp(0x0600 + g_sb16.base, gameport_i4d2);	/* GAME Port */
	iocore_attachinp(0x0700 + g_sb16.base, gameport_i4d2);	/* GAME Port */
#else
	iocore_attachout(0x0400 + g_sb16.base, sb16_o0400);	/* GAME Port */
	iocore_attachout(0x0500 + g_sb16.base, sb16_o0400);	/* GAME Port */
	iocore_attachout(0x0600 + g_sb16.base, sb16_o0400);	/* GAME Port */
	iocore_attachout(0x0700 + g_sb16.base, sb16_o0400);	/* GAME Port */
	iocore_attachinp(0x0400 + g_sb16.base, sb16_i0400);	/* GAME Port */
	iocore_attachinp(0x0500 + g_sb16.base, sb16_i0400);	/* GAME Port */
	iocore_attachinp(0x0600 + g_sb16.base, sb16_i0400);	/* GAME Port */
	iocore_attachinp(0x0700 + g_sb16.base, sb16_i0400);	/* GAME Port */
#endif
	
	iocore_attachout(0x8000 + g_sb16.base, sb16_o8000);	/* MIDI Port */
	//iocore_attachout(0x8001 + g_sb16.base, sb16_o8100);	/* MIDI Port 暫定 */
	iocore_attachout(0x8100 + g_sb16.base, sb16_o8100);	/* MIDI Port */
	iocore_attachinp(0x8000 + g_sb16.base, sb16_i8000);	/* MIDI Port */
	//iocore_attachinp(0x8001 + g_sb16.base, sb16_i8100);	/* MIDI Port 暫定 */
	iocore_attachinp(0x8100 + g_sb16.base, sb16_i8100);	/* MIDI Port */
	
	iocore_attachout(0xC800 + g_sb16.base, sb16_o2000);	/* FM Music Register Address Port */
	iocore_attachinp(0xC800 + g_sb16.base, sb16_i2000);	/* FM Music Status Port */
	iocore_attachout(0xC900 + g_sb16.base, sb16_o2100);	/* FM Music Data Port */
	iocore_attachout(0xCA00 + g_sb16.base, sb16_o2200);	/* Advanced FM Music Register Address Port */
	iocore_attachinp(0xCA00 + g_sb16.base, sb16_i2200);	/* Advanced FM Music Status Port */
	iocore_attachout(0xCB00 + g_sb16.base, sb16_o2300);	/* Advanced FM Music Data Port */
	
	// PC/AT互換機テスト
	if(np2cfg.sndsb16at){
		iocore_attachout(0x388, sb16_o2000);	/* FM Music Register Address Port */
		iocore_attachinp(0x388, sb16_i2000);	/* FM Music Status Port */
		iocore_attachout(0x389, sb16_o2100);	/* FM Music Data Port */
		iocore_attachout(0x38a, sb16_o2200);	/* Advanced FM Music Register Address Port */
		iocore_attachinp(0x38a, sb16_i2200);	/* Advanced FM Music Status Port */
		iocore_attachout(0x38b, sb16_o2300);	/* Advanced FM Music Data Port */
	}

	if (!g_mame_opl3[G_OPL3_INDEX]) {
#ifdef USE_MAME
		g_mame_opl3[G_OPL3_INDEX] = YMF262Init(14400000, np2cfg.samplingrate);
#endif
		samplerate = np2cfg.samplingrate;
	}

#if defined(SUPPORT_GAMEPORT)
	// ゲームポート割り当て 4d2h
	if(np2cfg.gameport){
		iocore_attachout(0x0400 + g_sb16.base, gameport_o4d2);
		iocore_attachinp(0x0400 + g_sb16.base, gameport_i4d2);
	}
#endif

	if (!g_mame_opl3[G_OPL3_INDEX]) {
#ifdef USE_MAME
		g_mame_opl3[G_OPL3_INDEX] = YMF262Init(14400000, np2cfg.samplingrate);
#endif
		samplerate = np2cfg.samplingrate;
	}
	sound_streamregist(g_mame_opl3[G_OPL3_INDEX], (SOUNDCB)opl3gen_getpcm);
	opl3_bind(&g_opl3[G_OPL3_INDEX]); // MAME使用の場合Key Display用
}
void boardsb16_unbind(void) {
	ct1745io_unbind();
	ct1741io_unbind();

	iocore_detachout(0x2000 + g_sb16.base);	/* FM Music Register Address Port */
	iocore_detachout(0x2100 + g_sb16.base);	/* FM Music Data Port */
	iocore_detachout(0x2200 + g_sb16.base);	/* Advanced FM Music Register Address Port */
	iocore_detachout(0x2300 + g_sb16.base);	/* Advanced FM Music Data Port */
	iocore_detachout(0x2800 + g_sb16.base);	/* FM Music Register Port */
	iocore_detachout(0x2900 + g_sb16.base);	/* FM Music Data Port */

	iocore_detachinp(0x2000 + g_sb16.base);	/* FM Music Status Port */
	iocore_detachinp(0x2200 + g_sb16.base);	/* Advanced FM Music Status Port */
	iocore_detachinp(0x2800 + g_sb16.base);	/* FM Music Status Port */

	iocore_detachout(0x0400 + g_sb16.base);	/* GAME Port */
	iocore_detachout(0x0500 + g_sb16.base);	/* GAME Port */
	iocore_detachout(0x0600 + g_sb16.base);	/* GAME Port */
	iocore_detachout(0x0700 + g_sb16.base);	/* GAME Port */
	iocore_detachinp(0x0400 + g_sb16.base);	/* GAME Port */
	iocore_detachinp(0x0500 + g_sb16.base);	/* GAME Port */
	iocore_detachinp(0x0600 + g_sb16.base);	/* GAME Port */
	iocore_detachinp(0x0700 + g_sb16.base);	/* GAME Port */
	
	iocore_detachout(0x8000 + g_sb16.base);	/* MIDI Port */
	//iocore_detachout(0x8001 + g_sb16.base);	/* MIDI Port 暫定 */
	iocore_detachout(0x8100 + g_sb16.base);	/* MIDI Port */
	iocore_detachinp(0x8000 + g_sb16.base);	/* MIDI Port */
	//iocore_detachinp(0x8001 + g_sb16.base);	/* MIDI Port 暫定 */
	iocore_detachinp(0x8100 + g_sb16.base);	/* MIDI Port */
	
	iocore_detachout(0xC800 + g_sb16.base);	/* FM Music Register Address Port */
	iocore_detachinp(0xC800 + g_sb16.base);	/* FM Music Status Port */
	iocore_detachout(0xC900 + g_sb16.base);	/* FM Music Data Port */
	iocore_detachout(0xCA00 + g_sb16.base);	/* Advanced FM Music Register Address Port */
	iocore_detachinp(0xCA00 + g_sb16.base);	/* Advanced FM Music Status Port */
	iocore_detachout(0xCB00 + g_sb16.base);	/* Advanced FM Music Data Port */
	
	// PC/AT互換機テスト
	if(np2cfg.sndsb16at){
		iocore_detachout(0x388);	/* FM Music Register Address Port */
		iocore_detachinp(0x388);	/* FM Music Status Port */
		iocore_detachout(0x389);	/* FM Music Data Port */
		iocore_detachout(0x38a);	/* Advanced FM Music Register Address Port */
		iocore_detachinp(0x38a);	/* Advanced FM Music Status Port */
		iocore_detachout(0x38b);	/* Advanced FM Music Data Port */
	}

#if defined(SUPPORT_GAMEPORT)
	// ゲームポート割り当て 4d2h
	if(np2cfg.gameport){
		iocore_detachout(0x0400 + g_sb16.base);
		iocore_detachinp(0x0400 + g_sb16.base);
	}
#endif
}
void boardsb16_finalize(void)
{
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX]) {
		YMF262Shutdown(g_mame_opl3[G_OPL3_INDEX]);
		g_mame_opl3[G_OPL3_INDEX] = NULL;
	}
#endif
}

#endif
