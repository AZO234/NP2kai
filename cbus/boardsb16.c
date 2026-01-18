#include	<compiler.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<cbus/cbuscore.h>
#include	<cbus/boardso.h>
#include	<sound/sound.h>
#include	<cbus/ct1741io.h>
#include	<cbus/ct1745io.h>
#include	<sound/fmboard.h>
#include	<cbus/mpu98ii.h>
#include	<cbus/smpu98.h>
#include	<joymng.h>
#include	<cpucore.h>
//#include	<sound/s98.h>

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

static UINT8 seq[] = {0x60, 0x80, 0xff, 0x21}; // XXX: Win2kドライバのチェックを通すためだけの暫定シーケンス
static int forceopl3mode = 0;
static int seqpos = 0;

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
static int samplerate;
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
#endif
	g_opl3[G_OPL3_INDEX].s.addrl = dat; // Key Display用
	//TRACEOUT(("OPL3 PORT=0x%04x, DATA=0x%02x", port, dat));
}

static void IOOUTCALL sb16_o2100(UINT port, REG8 dat) {
	(void)port;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 1, dat);
#endif
	opl3_writeRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrl, dat); // Key Display用
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
#endif
	g_opl3[G_OPL3_INDEX].s.addrh = dat; // Key Display用
	//TRACEOUT(("OPL3 PORT=0x%04x, DATA=0x%02x", port, dat));
}

static void IOOUTCALL sb16_o2300(UINT port, REG8 dat) {
	(void)port;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 3, dat);
#endif
	opl3_writeExtendedRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrh, dat); // Key Display用
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
	g_opl3[G_OPL3_INDEX].s.addrl = dat; // Key Display用
}
static void IOOUTCALL sb16_o2900(UINT port, REG8 dat) {
	port = dat;
#ifdef USE_MAME
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 1, dat);
#endif
	opl3_writeRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrl, dat); // Key Display用
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
#define GAMEPORT_JOYCOUNTER_TMPCLK	100000000
#if defined(SUPPORT_IA32_HAXM)
static LARGE_INTEGER gameport_qpf;
static int gameport_useqpc = 0;
#endif
extern int gameport_tsccounter;
static UINT64 gameport_tsc;
static REG8 gameport_joyflag_base = 0x00;
static REG8 gameport_joyflag = 0x00;
#if defined(SUPPORT_IA32_HAXM)
static UINT64 gameport_clkmax;
static UINT64 gameport_threshold_x = 0;
static UINT64 gameport_threshold_y = 0;
#else
static UINT32 gameport_clkmax;
static UINT32 gameport_threshold_x = 0;
static UINT32 gameport_threshold_y = 0;
#endif
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
	REG8 joyflag;
	joyflag = joymng_getstat();
	if(!joymng_available()){
		return;
	}
	gameport_joyflag_base = joyflag;
	gameport_joyflag = ((joyflag >> 2) & 0x30)  | ((joyflag << 2) & 0xc0) | 0x0f;
#if defined(SUPPORT_IA32_HAXM)
	{
		LARGE_INTEGER li = {0};
		if (QueryPerformanceFrequency(&gameport_qpf)) {
			QueryPerformanceCounter(&li);
			//li.QuadPart = li.QuadPart;
			gameport_tsc = (UINT64)li.QuadPart;
			gameport_useqpc = 1;
		}else{
			gameport_tsc = CPU_MSR_TSC;
			gameport_useqpc = 0;
		}
	}
#else
#if defined(USE_TSC)
	if(CPU_REMCLOCK > 0){
		gameport_tsc = CPU_MSR_TSC - (UINT64)CPU_REMCLOCK * pccore.maxmultiple / pccore.multiple;
	}else{
		gameport_tsc = CPU_MSR_TSC;
	}
	gameport_tsccounter = 0;
#else
	gameport_tsc = 0;
#endif
#endif
	(void)port;
}
static REG8 IOINPCALL gameport_i4d2(UINT port)
{
	REG8 retval = 0xff;
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
	UINT32 joyAnalogX;
	UINT32 joyAnalogY;
#endif
	UINT64 clockdiff;
#if defined(SUPPORT_IA32_HAXM)
	LARGE_INTEGER li = { 0 };
	QueryPerformanceCounter(&li);
#endif
	{
		REG8 joyflag = joymng_getstat();
		gameport_joyflag = ((joyflag >> 2) & 0x30) | ((joyflag << 2) & 0xc0) | (gameport_joyflag & 0x0f);
	}
	if (!joymng_available())
	{
		return 0xff;
	}
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
	joyAnalogX = joymng_getAnalogX();
	joyAnalogY = joymng_getAnalogY();
#endif
#if defined(SUPPORT_IA32_HAXM)
	if(gameport_useqpc){
		clockdiff = (unsigned long long)((UINT64)li.QuadPart - gameport_tsc);
		gameport_clkmax = (UINT64)gameport_qpf.QuadPart / 1300; // とりあえず0.7msで･･･
	}else{
		clockdiff = CPU_MSR_TSC - gameport_tsc;
		gameport_clkmax = pccore.realclock / 1430; // とりあえず0.7msで･･･
	}
#else
#if defined(USE_TSC)
	if(CPU_REMCLOCK > 0){
		clockdiff = CPU_MSR_TSC - (UINT64)CPU_REMCLOCK * pccore.maxmultiple / pccore.multiple - gameport_tsc;
	}else{
		clockdiff = CPU_MSR_TSC - gameport_tsc;
	}
	gameport_clkmax = pccore.baseclock * pccore.maxmultiple / 1430; // とりあえず0.7msで･･･
#if !defined(SUPPORT_IA32_HAXM)
	if(gameport_tsccounter == 0 || !np2cfg.consttsc){
		gameport_clkmax = gameport_clkmax * pccore.maxmultiple / pccore.multiple; // CPUクロック依存の場合のfix
	}
#endif
#else
	gameport_clkmax = 32;
	clockdiff = gameport_tsc;
	gameport_tsc++;
#endif
#endif
	if (np2cfg.analogjoy)
	{
		// アナログ入力タイプ
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
		gameport_threshold_x = GAMEPORT_JOYCOUNTER_MGN2 + (UINT32)((UINT64)(gameport_clkmax - GAMEPORT_JOYCOUNTER_MGN2 * 2) * joyAnalogX / 65535);
		gameport_threshold_y = GAMEPORT_JOYCOUNTER_MGN2 + (UINT32)((UINT64)(gameport_clkmax - GAMEPORT_JOYCOUNTER_MGN2 * 2) * joyAnalogY / 65535);
#else
		gameport_threshold_x = GAMEPORT_JOYCOUNTER_MGN2;
		gameport_threshold_y = GAMEPORT_JOYCOUNTER_MGN2;
#endif
	}
	else
	{
		// ON/OFFタイプ
		gameport_threshold_x = gameport_clkmax / 2;
		gameport_threshold_y = gameport_clkmax / 2;
		if(~gameport_joyflag_base & 0x1){
			gameport_threshold_y = GAMEPORT_JOYCOUNTER_MGN2;
		}
		if(~gameport_joyflag_base & 0x2){
			gameport_threshold_y = gameport_clkmax - GAMEPORT_JOYCOUNTER_MGN2;
		}
		if(~gameport_joyflag_base & 0x4){
			gameport_threshold_x = GAMEPORT_JOYCOUNTER_MGN2;
		}
		if(~gameport_joyflag_base & 0x8){
			gameport_threshold_x = gameport_clkmax - GAMEPORT_JOYCOUNTER_MGN2;
		}
	}
	retval = gameport_joyflag;
	if(clockdiff >= (UINT64)gameport_threshold_x){
		gameport_joyflag &= ~0x01;
	}
	if(clockdiff >= (UINT64)gameport_threshold_y){
		gameport_joyflag &= ~0x02;
	}
	return retval;
}
#endif

// ----

#ifdef USE_MAME
#define OPL3_SAMPLE_BUFFER	4096	
static INT16 oplfm_s1ls[OPL3_SAMPLE_BUFFER] = { 0 };
static INT16 oplfm_s1rs[OPL3_SAMPLE_BUFFER] = { 0 };
static INT16 oplfm_s2ls[OPL3_SAMPLE_BUFFER] = { 0 };
static INT16 oplfm_s2rs[OPL3_SAMPLE_BUFFER] = { 0 };
static void SOUNDCALL opl3gen_getpcm(void* opl3, SINT32 *pcm, UINT count) {
	UINT i;
	INT16 *buf[4];
	SINT32 *outbuf = pcm;
	SINT32 oplfm_volume;
	SINT32 midivolL = g_sb16.mixregexp[MIXER_MIDI_LEFT];
	SINT32 midivolR = g_sb16.mixregexp[MIXER_MIDI_RIGHT];
	SINT32 volL, volR;
	oplfm_volume = np2cfg.vol_fm * np2cfg.vol_master / 100;
	buf[0] = oplfm_s1ls;
	buf[1] = oplfm_s1rs;
	buf[2] = oplfm_s2ls;
	buf[3] = oplfm_s2rs;

	// PCMサウンドバッファに送る
	volL = oplfm_volume * midivolL * 2;
	volR = oplfm_volume * midivolR * 2;
	while (count > 0) {
		int cc = MIN(count, OPL3_SAMPLE_BUFFER);
		YMF262UpdateOne(opl3, buf, cc);
		for (i = 0; i < cc; i++) {
			outbuf[0] += (SINT32)((oplfm_s1ls[i] * volL / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_LEFT] / 255) >> 6);
			outbuf[1] += (SINT32)((oplfm_s1rs[i] * volR / 255 * (SINT32)g_sb16.mixregexp[MIXER_MASTER_RIGHT] / 255) >> 6);
			outbuf += 2;
		}
		count -= cc;
	}
}

static void SOUNDCALL opl3gen_getpcm_dummy(void* opl3, SINT32* pcm, UINT count) {
	UINT i;
	INT16* buf[4];
	INT16 s1l, s1r, s2l, s2r;
	buf[0] = oplfm_s1ls;
	buf[1] = oplfm_s1rs;
	buf[2] = oplfm_s2ls;
	buf[3] = oplfm_s2rs;
	while (count > 0) {
		int cc = MIN(count, OPL3_SAMPLE_BUFFER);
		YMF262UpdateOne(opl3, buf, cc);
		count -= cc;
	}
}
#endif

void boardsb16_reset(const NP2CFG *pConfig) {
	DSP_INFO olddsp;
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX]) {
		if (samplerate != soundcfg.rate) {
			YMF262Shutdown(g_mame_opl3[G_OPL3_INDEX]);
			g_mame_opl3[G_OPL3_INDEX] = YMF262Init(14400000, soundcfg.rate);
			samplerate = soundcfg.rate;
		} else {
			YMF262ResetChip(g_mame_opl3[G_OPL3_INDEX], samplerate);
		}
	}
#endif
	olddsp = g_sb16.dsp_info; // dsp_infoだけ初期化しない
	ZeroMemory(&g_sb16, sizeof(g_sb16));
	g_sb16.dsp_info = olddsp;
	// ボードデフォルト IO:D2 DMA:3 IRQ:5(INT1) 
	g_sb16.base = np2cfg.sndsb16io; //0xd2;
	g_sb16.dmachnum = np2cfg.sndsb16dma; //0x3;
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

#ifdef USE_MAME
	if (!g_mame_opl3[G_OPL3_INDEX]) {
		g_mame_opl3[G_OPL3_INDEX] = YMF262Init(14400000, np2cfg.samplingrate);
		samplerate = np2cfg.samplingrate;
	}
#endif
	opl3_bind(&g_opl3[G_OPL3_INDEX]); // MAME使用の場合Key Display用
#ifdef USE_MAME
	if (g_opl3[G_OPL3_INDEX].userdata) {
		// 外部音源を使用する場合 ダミー登録
		sound_streamregist(g_mame_opl3[G_OPL3_INDEX], (SOUNDCB)opl3gen_getpcm_dummy);
	}
	else {
		// 外部音源を使用しない場合 PCM登録
		sound_streamregist(g_mame_opl3[G_OPL3_INDEX], (SOUNDCB)opl3gen_getpcm);
	}
#endif
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
