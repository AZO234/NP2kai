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
#include "joymng.h"
#include "cpucore.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/soundrom.h"
#include "mpu98ii.h"


static int opna_idx = 0;
static int a460_soundid = 0x80;

/*********** for OPL (MAME) ***********/

#define G_OPL3_INDEX	0

#ifdef USE_MAME
static int samplerate;
void *YMF262Init(INT clock, INT rate);
void YMF262ResetChip(void *chip);
void YMF262Shutdown(void *chip);
INT YMF262Write(void *chip, INT a, INT v);
UINT8 YMF262Read(void *chip, INT a);
void YMF262UpdateOne(void *chip, INT16 **buffer, INT length);

static void IOOUTCALL sb16_o20d2(UINT port, REG8 dat) {
	(void)port;
	g_opl3[G_OPL3_INDEX].s.addrl = dat; // Key Display用
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 0, dat);
}

static void IOOUTCALL sb16_o21d2(UINT port, REG8 dat) {
	(void)port;
	opl3_writeRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrl, dat); // Key Display用
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 1, dat);
}
static void IOOUTCALL sb16_o22d2(UINT port, REG8 dat) {
	(void)port;
	g_opl3[G_OPL3_INDEX].s.addrh = dat; // Key Display用
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 2, dat);
}

static void IOOUTCALL sb16_o23d2(UINT port, REG8 dat) {
	(void)port;
	opl3_writeExtendedRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrh, dat); // Key Display用
	//S98_put(EXTEND2608, opl.addr2, dat);
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 3, dat);
}

static void IOOUTCALL sb16_o28d2(UINT port, REG8 dat) {
	/**
	 * いわゆるPC/ATで言うところのAdlib互換ポート
	 * UltimaUnderWorldではこちらを叩く
	 */
	port = dat;
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 0, dat);
}
static void IOOUTCALL sb16_o29d2(UINT port, REG8 dat) {
	port = dat;
	YMF262Write(g_mame_opl3[G_OPL3_INDEX], 1, dat);
}

static REG8 IOINPCALL sb16_i20d2(UINT port) {
	
	REG8 ret;
	ret = YMF262Read(g_mame_opl3[G_OPL3_INDEX], 0);
	////if(g_opl.reg[0x4] == 1) return 0x02;
	////if(g_opl.reg[0x4] == 1){
	//	if ((cs4231.reg.pinctrl & IEN) && (cs4231.dmairq != 0xff)) {
	//		if(cs4231.intflag & INt){
	//		}
	//	}
	////}
	return ret;
}

static REG8 IOINPCALL sb16_i22d2(UINT port) {
	(void)port;
	return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 1);
}

static REG8 IOINPCALL sb16_i28d2(UINT port) {
	(void)port;
	return YMF262Read(g_mame_opl3[G_OPL3_INDEX], 0);
}
#endif

/*********** for OPNA ***********/

static void IOOUTCALL ymf_o188(UINT port, REG8 dat)
{
	g_opna[opna_idx].s.addrl = dat;
	g_opna[opna_idx].s.addrh = 0;
	g_opna[opna_idx].s.data = dat;
	(void)port;
}

static void IOOUTCALL ymf_o18a(UINT port, REG8 dat)
{
	g_opna[opna_idx].s.data = dat;
	if (g_opna[opna_idx].s.addrh != 0) {
		return;
	}

	if (g_opna[opna_idx].s.addrl == 0x27) {
		/* OPL3-LにCSMモードは無い */
		dat &= ~0x80;
		g_opna[opna_idx].opngen.opnch[2].extop = dat & 0xc0;
	}

	opna_writeRegister(&g_opna[opna_idx], g_opna[opna_idx].s.addrl, dat);

	(void)port;
}


static void IOOUTCALL ymf_o18c(UINT port, REG8 dat)
{
	if (g_opna[opna_idx].s.extend)
	{
		g_opna[opna_idx].s.addrl = dat;
		g_opna[opna_idx].s.addrh = 1;
		g_opna[opna_idx].s.data = dat;
	}
	(void)port;
}

static void IOOUTCALL ymf_o18e(UINT port, REG8 dat)
{
	if (!g_opna[opna_idx].s.extend)
	{
		return;
	}
	g_opna[opna_idx].s.data = dat;

	if (g_opna[opna_idx].s.addrh != 1)
	{
		return;
	}

	opna_writeExtendedRegister(&g_opna[opna_idx], g_opna[opna_idx].s.addrl, dat);

	(void)port;
}

static REG8 IOINPCALL ymf_i188(UINT port)
{
	REG8 ret;
	ret = g_opna[opna_idx].s.status;
	if((cs4231.reg.pinctrl & IEN) && (cs4231.reg.featurestatus & (PI|TI|CI))) {
		ret |= 0x02;
	}
	return ret;
}

static REG8 IOINPCALL ymf_i18a(UINT port)
{
	UINT nAddress;

	if (g_opna[opna_idx].s.addrh == 0)
	{
		nAddress = g_opna[opna_idx].s.addrl;
		if (nAddress == 0x0e)
		{
			return fmboard_getjoy(&g_opna[opna_idx]);
		}
		else if (nAddress < 0x10)
		{
			return opna_readRegister(&g_opna[opna_idx], nAddress);
		}
		else if (nAddress == 0xff)
		{
			return 1;
		}
	}

	(void)port;
	return g_opna[opna_idx].s.data;
}

static REG8 IOINPCALL ymf_i18c(UINT port)
{
	if (g_opna[opna_idx].s.extend)
	{
		return (g_opna[opna_idx].s.status & 3);
	}

	(void)port;
	return 0xff;
}

static void extendchannel(REG8 enable)
{
	g_opna[opna_idx].s.extend = enable;
	if (enable)
	{
		opngen_setcfg(&g_opna[opna_idx].opngen, 6, OPN_STEREO | 0x007);
	}
	else
	{
		opngen_setcfg(&g_opna[opna_idx].opngen, 3, OPN_MONORAL | 0x007);
		rhythm_setreg(&g_opna[opna_idx].rhythm, 0x10, 0xff);
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
	return (a460_soundid | (cs4231.extfunc & 1));
	(void)port;
}

/*********** SRN-F I/O ***********/

char srnf;
static void IOOUTCALL srnf_oa460(UINT port, REG8 dat)
{
	srnf = dat;
	(void)port;
}

static REG8 IOINPCALL srnf_ia460(UINT port)
{
	(void)port;
	return (srnf);
}

/*********** WaveStar I/O ***********/

REG8 wavestar_4d2 = 0xff;
static void IOOUTCALL wavestar_o4d2(UINT port, REG8 dat)
{
	wavestar_4d2 = dat;
	(void)port;
}

static REG8 IOINPCALL wavestar_i4d2(UINT port)
{
	(void)port;
	return (wavestar_4d2);
}

/*********** ソフトウェアディップスイッチ I/O ***********/

static REG8 IOINPCALL wss_i881e(UINT port)
{
	if(g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16){
		int ret = 0x64;
		ret |= (cs4231.dmairq-1) << 3;
		if((cs4231.dmairq-1)==0x1 || (cs4231.dmairq-1)==0x2){
			ret |= 0x80; // 奇数パリティ
		}
		return (ret);
	}else{
		return (0xDC);
	}
}

/*********** YMF-701/715 I/O ? ***********/

REG8 ymf701;
static void IOOUTCALL wss_o548e(UINT port, REG8 dat)
{
	ymf701 = dat;
}
static REG8 IOINPCALL wss_i548e(UINT port)
{
	return (ymf701); 
}
static REG8 IOINPCALL wss_i548f(UINT port)
{
	if(ymf701 == 0) return 0xe8;
	else if(ymf701 == 0x1) return 0xfe;
	else if(ymf701 == 0x2) return 0x40;
	else if(ymf701 == 0x3) return 0x30;
	else if(ymf701 == 0x4) return 0xff;
	else if(ymf701 == 0x20) return 0x04;
	else if(ymf701 == 0x40) return 0x20;
	else return 0;// from PC-9821Nr166
}

#if defined(SUPPORT_GAMEPORT)
/*********** PC-9801-118 Gameport I/O ***********/
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
static void IOOUTCALL gameport_o1480(UINT port, REG8 dat)
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
static REG8 IOINPCALL gameport_i1480(UINT port)
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


/*********** for OPL (NP2) ***********/

static void IOOUTCALL ym_o1488(UINT port, REG8 dat) //FM Music Register Address Port
{
	g_opl3[G_OPL3_INDEX].s.addrl = dat;
	(void)port;
}
REG8 opl_data;
static void IOOUTCALL ym_o1489(UINT port, REG8 dat) //FM Music Data Port
{
	opl3_writeRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrl, dat);
	opl_data = dat;
	(void)port;
}


static void IOOUTCALL ym_o148a(UINT port, REG8 dat) // Advanced FM Music Register Address	 Port
{
	g_opl3[G_OPL3_INDEX].s.addrh = dat;
	(void)port;
}
static void IOOUTCALL ym_o148b(UINT port, REG8 dat) //Advanced FM Music Data Port
{
	opl3_writeExtendedRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrh, dat);
	(void)port;
}

static REG8 IOINPCALL ym_i1488(UINT port) //FM Music Status Port
{
	TRACEOUT(("%x read",port));
//	if (opl_data == 0x80) return 0xe0;
	if (opl_data == 0x21) return 0xc0;
	return 0;
}

static REG8 IOINPCALL ym_i1489(UINT port) //  ???
{
	TRACEOUT(("%x read",port));
	return opl3_readRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrl);
}
static REG8 IOINPCALL ym_i148a(UINT port) //Advanced FM Music Status Port
{
	TRACEOUT(("%x read",port));
	return opl3_readStatus(&g_opl3[G_OPL3_INDEX]);
}

static REG8 IOINPCALL ym_i148b(UINT port) //  ???
{
	TRACEOUT(("%x read",port));
	return opl3_readExtendedRegister(&g_opl3[G_OPL3_INDEX], g_opl3[G_OPL3_INDEX].s.addrh);
}

/*********** PC-9801-118 config I/O ? ***********/

REG8 sound118 = 0x05;
static void IOOUTCALL csctrl_o148e(UINT port, REG8 dat) {
	TRACEOUT(("write %x %x",port,dat));
	sound118 = dat;
}

static REG8 IOINPCALL csctrl_i148e(UINT port) {
	return(sound118);
	TRACEOUT(("%x read",port));
}
REG8 control118;
static REG8 IOINPCALL csctrl_i148f(UINT port) {
	TRACEOUT(("%x read",port));
	(void)port;
	//if(sound118 == 0)	return(0xf0);//PC-9801-118は3だけどYMFは0xff 2000はこれだけじゃまだダメ
	if(sound118 == 0)	return(0xf3);//PC-9801-118は3だけどYMFは0xff 2000はこれだけじゃまだダメ
	if(sound118 == 0x05){
		if(control118==4)return 4;
		if(control118==0x0c) return 4;
		if(control118==0)return 0;
	}
	if(sound118 == 0x04) return (0x00);
	if(sound118 == 0x21){
		switch(10/*mpu98.irqnum2*/){//MIDI割り込み 00:IRQ10　01:IRQ6 02:IRQ5 03:IRQ3
			case 10:return 0;
			case 6: return 1;
			case 5: return 2;
			case 3: return 3;
			default: return 0;
		}
	}
	if(sound118 == 0xff) return (0x05);//bit0 MIDI割り込みあり bit1:Cb Na7 bit2:Mate-X bit3:A-Mate Ce2
	else
	return(0xff);
}

static void IOOUTCALL csctrl_o148f(UINT port, REG8 dat) {
	TRACEOUT(("write %x %x",port,dat));

	//if (sound118 == 0x21) switch(dat){
	//				case 0:mpu98.irqnum2 = 10;break;
	//				case 1:mpu98.irqnum2 = 6;break;
	//				case 2:mpu98.irqnum2 = 5;break;
	//				case 3:mpu98.irqnum2 = 3;break;
	//				default: break;
	//}
	control118 = dat;
}

static REG8 IOINPCALL csctrl_i486(UINT port) {
	return(0);
}

// ---- MAME使用の場合のPCM再生
#ifdef USE_MAME
static SINT32 oplfm_softvolume_L = 0;
static SINT32 oplfm_softvolume_R = 0;
static SINT32 oplfm_softvolumereg_L = 0xff;
static SINT32 oplfm_softvolumereg_R = 0xff;
static void SOUNDCALL opl3gen_getpcm2(void* opl3, SINT32 *pcm, UINT count) {
	UINT i;
	INT16 *buf[4];
	INT16 s1l,s1r,s2l,s2r;
	SINT32 oplfm_volume;
	SINT32 *outbuf = pcm;
	buf[0] = &s1l;
	buf[1] = &s1r;
	buf[2] = &s2l;
	buf[3] = &s2r;

	// NP2グローバルFMボリューム(0～127)
	oplfm_volume = np2cfg.vol_fm; 

	// Canbe/ValueStarミキサー FMボリューム(0～31) bit7:1=mute, bit6-5:reserved, bit4-0:volume(00000b max, 11111b min)
	if(oplfm_softvolumereg_L != cs4231.devvolume[0x30]){
		oplfm_softvolumereg_L = cs4231.devvolume[0x30];
		if(oplfm_softvolumereg_L & 0x80){ // FM L Mute
			oplfm_softvolume_L = 0;
		}else{
			oplfm_softvolume_L = ((~oplfm_softvolumereg_L) & 0x1f); // FM L Volume
		}
	}
	if(oplfm_softvolumereg_R != cs4231.devvolume[0x31]){
		oplfm_softvolumereg_R = cs4231.devvolume[0x31];
		if(oplfm_softvolumereg_R & 0x80){ // FM R Mute
			oplfm_softvolume_R = 0;
		}else{
			oplfm_softvolume_R = ((~oplfm_softvolumereg_R) & 0x1f); // FM R Volume
		}
	}

	// PCMサウンドバッファに送る
	for (i=0; i < count; i++) {
		s1l = s1r = s2l = s2r = 0;
		YMF262UpdateOne(opl3, buf, 1);
		outbuf[0] += ((((SINT32)s1l << 1) * oplfm_volume * oplfm_softvolume_L) >> 10);
		outbuf[1] += ((((SINT32)s1r << 1) * oplfm_volume * oplfm_softvolume_R) >> 10);
		outbuf += 2;
	}
}
#endif

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

	// 86音源と共存させる場合、使用するNP2 OPNA番号を変える
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_PC_9801_86_118 || g_nSoundID==SOUNDID_WAVESTAR || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_118_SB16){
		opna_idx = 1;
	}else{
		opna_idx = 0;
	}
	
	// OPNAリセット
	opna_reset(&g_opna[opna_idx], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_S98);
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16){
		// OPNAタイマーをセットしない
		//opna_timer(&g_opna[opna_idx], 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	}else{
		// OPNAタイマーをセット
		UINT irqval = 0x00;
		UINT8 irqf = np2cfg.snd118irqf;
		if(g_nSoundID==SOUNDID_PC_9801_86_118 || g_nSoundID==SOUNDID_PC_9801_86_118_SB16){
			UINT8 irq86table[4] = {0x03, 0x0d, 0x0a, 0x0c};
			UINT8 nIrq86 = (np2cfg.snd86opt & 0x10) | ((np2cfg.snd86opt & 0x4) << 5) | ((np2cfg.snd86opt & 0x8) << 3);
			UINT8 irq86 = irq86table[nIrq86 >> 6];
			irqf = np2cfg.snd118irqp;
			if(irqf == irq86){
				if(irq86!=3){
					irqf = 0x3;
				}else{
					irqf = 0xC;
				}
			}
		}
		switch(irqf){
		case 3:
			irqval = 0x10|(0 << 6);
			break;
		case 10:
			irqval = 0x10|(2 << 6);
			break;
		case 12:
			irqval = 0x10|(3 << 6);
			break;
		case 13:
			irqval = 0x10|(1 << 6);
			break;
		}
		if(opna_idx == 1){
			opna_timer(&g_opna[opna_idx], irqval, NEVENT_FMTIMER2A, NEVENT_FMTIMER2B);
		}else{
			opna_timer(&g_opna[opna_idx], irqval, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
		}

		// OPLリセット
		opl3_reset(&g_opl3[G_OPL3_INDEX], OPL3_HAS_OPL3L|OPL3_HAS_OPL3);
		opngen_setcfg(&g_opna[opna_idx].opngen, 3, OPN_STEREO | 0x038);
	}
	
	// CS4231リセット
	cs4231io_reset();

	// 86+118の場合、被らないように修正
	if(g_nSoundID==SOUNDID_PC_9801_86_118 || g_nSoundID==SOUNDID_PC_9801_86_118_SB16){
		UINT16 snd86iobase = (pConfig->snd86opt & 0x01) ? 0x000 : 0x100;
		if(np2cfg.snd118io == 0x188 + snd86iobase){
			cs4231.port[4] += 0x100;
		}
	}
	
	// 86音源+118音源の場合、118側をはじめからFM6音にする
	if(g_nSoundID==SOUNDID_PC_9801_86_118 || g_nSoundID==SOUNDID_PC_9801_86_118_SB16){
		cs4231.extfunc |= 1;
		extendchannel(1);
	}
	
	// 色々設定
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16){
	}else{
		if(pConfig->snd118rom && g_nSoundID!=SOUNDID_PC_9801_86_118 && g_nSoundID!=SOUNDID_PC_9801_86_118_SB16){
			soundrom_load(0xcc000, OEMTEXT("118"));
		}
		fmboard_extreg(extendchannel);
#ifdef SUPPORT_SOUND_SB16
#ifdef USE_MAME
		if (g_mame_opl3[G_OPL3_INDEX]) {
			if (samplerate != pConfig->samplingrate) {
				YMF262Shutdown(g_mame_opl3[G_OPL3_INDEX]);
				g_mame_opl3[G_OPL3_INDEX] = YMF262Init(14400000, pConfig->samplingrate);
				samplerate = pConfig->samplingrate;
			} else {
				YMF262ResetChip(g_mame_opl3[G_OPL3_INDEX]);
			}
		}
		//ZeroMemory(&g_sb16, sizeof(g_sb16));
		//// ボードデフォルト IO:D2 DMA:3 INT:5 
		//g_sb16.base = 0xd2;
		//g_sb16.dmach = 0x3;
		//g_sb16.dmairq = 0x5;
#endif
#endif
		//cs4231.extfunc = 0x01;
		//extendchannel((REG8)(cs4231.extfunc & 1));
	}

	// WaveStarの場合、ボリューム初期化
	if(g_nSoundID==SOUNDID_WAVESTAR){
		// FM音量
		cs4231.devvolume[0xff] = 0xf;
		opngen_setvol(np2cfg.vol_fm * cs4231.devvolume[0xff] / 15);
		psggen_setvol(np2cfg.vol_ssg * cs4231.devvolume[0xff] / 15);
		rhythm_setvol(np2cfg.vol_rhythm * cs4231.devvolume[0xff] / 15);
#if defined(SUPPORT_FMGEN)
		if(np2cfg.usefmgen) {
			opna_fmgen_setallvolumeFM_linear(np2cfg.vol_fm * cs4231.devvolume[0xff] / 15);
			opna_fmgen_setallvolumePSG_linear(np2cfg.vol_ssg * cs4231.devvolume[0xff] / 15);
			opna_fmgen_setallvolumeRhythmTotal_linear(np2cfg.vol_rhythm * cs4231.devvolume[0xff] / 15);
		}
#endif
	}
	(void)pConfig;
}

/**
 * Bind
 */
void board118_bind(void)
{
	int i;

	// CS4231バインド（I/Oポート割り当てとか）
	cs4231io_bind();
	
	// 86音源と共存させる場合、使用するNP2 OPNA番号を変える
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_PC_9801_86_118 || g_nSoundID==SOUNDID_WAVESTAR || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_118_SB16){
		opna_idx = 1;
	}else{
		opna_idx = 0;
	}

	if(g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16){
		a460_soundid = np2cfg.sndwssid;//0x70;
	}else if(g_nSoundID==SOUNDID_WAVESTAR){
		a460_soundid = 0x41;
	}else{
		a460_soundid = np2cfg.snd118id;//0x80;
	}

	if(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_WAVESTAR || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16){
		// Mate-X PCMの場合、CS4231だけ
		if(g_nSoundID!=SOUNDID_WAVESTAR){
			iocore_attachout(cs4231.port[1], ymf_oa460);
			iocore_attachinp(cs4231.port[1], ymf_ia460);
			iocore_attachinp(0x881e, wss_i881e);
		}
	}else{
		// 118音源の場合、色々割り当て

		// OPNA割り当て
		if(cs4231.port[4]){
			opna_bind(&g_opna[opna_idx]);
			cbuscore_attachsndex(cs4231.port[4],ymf_o, ymf_i);
		}

#if defined(SUPPORT_GAMEPORT)
		// ゲームポート割り当て 1480h～1487hどこでも良いらしい
		if(np2cfg.gameport){
			for(i=0;i<=7;i++){
				iocore_attachout(0x1480+i, gameport_o1480);
				iocore_attachinp(0x1480+i, gameport_i1480);
			}
		}
#endif
		
		// OPL割り当て
#ifdef USE_MAME
		iocore_attachout(cs4231.port[9], sb16_o20d2);
		iocore_attachinp(cs4231.port[9], sb16_i20d2);
		iocore_attachout(cs4231.port[9]+1, sb16_o21d2);
		iocore_attachout(cs4231.port[9]+2, sb16_o22d2);
		iocore_attachinp(cs4231.port[9]+2, sb16_i22d2);
		iocore_attachout(cs4231.port[9]+3, sb16_o23d2);

		if (!g_mame_opl3[G_OPL3_INDEX]) {
			g_mame_opl3[G_OPL3_INDEX] = YMF262Init(14400000, np2cfg.samplingrate);
			samplerate = np2cfg.samplingrate;
		}
		sound_streamregist(g_mame_opl3[G_OPL3_INDEX], (SOUNDCB)opl3gen_getpcm2);
#else
		iocore_attachout(cs4231.port[9], ym_o1488);
		iocore_attachinp(cs4231.port[9], ym_i1488);
		iocore_attachout(cs4231.port[9]+1, ym_o1489);
		iocore_attachout(cs4231.port[9]+2, ym_o148a);
		iocore_attachout(cs4231.port[9]+3, ym_o148b);
#endif
		opl3_bind(&g_opl3[G_OPL3_INDEX]); // MAME使用の場合Key Display用
		
		// Sound ID I/O port割り当て
		iocore_attachout(cs4231.port[1], ymf_oa460);
		iocore_attachinp(cs4231.port[1], ymf_ia460);

		// SRN-F寄生 I/O port割り当て
		//iocore_attachout(cs4231.port[15],srnf_oa460);//SRN-Fは必要なときだけ使う
		//iocore_attachinp(cs4231.port[15],srnf_ia460);
		//srnf = 0x81;
		
		// WaveStar I/O port割り当て
		if(g_nSoundID==SOUNDID_WAVESTAR){
			iocore_attachout(0x4d2 ,wavestar_o4d2);
			iocore_attachinp(0x4d2 ,wavestar_i4d2);
		}
		
		// PC-9801-118 config I/O port割り当て
		iocore_attachout(cs4231.port[14],csctrl_o148e);
		iocore_attachinp(cs4231.port[14],csctrl_i148e);
		iocore_attachout(cs4231.port[14]+1,csctrl_o148f);
		iocore_attachinp(cs4231.port[14]+1,csctrl_i148f);
		
		// YMF-701 I/O port割り当て
		iocore_attachout(cs4231.port[6], wss_o548e);// YMF-701
		iocore_attachinp(cs4231.port[6], wss_i548e);// YMF-701
		iocore_attachinp(cs4231.port[6]+1, wss_i548f);// YMF-701
		
		// PC-9801-118 control? I/O port割り当て
		iocore_attachinp(cs4231.port[11]+6,csctrl_i486);
		iocore_attachinp(0x881e, wss_i881e);
		
		// MIDI I/O port割り当て mpu98ii.c側で調整
		//iocore_attachout(cs4231.port[10], mpu98ii_o0);
		//iocore_attachinp(cs4231.port[10], mpu98ii_i0);
		//iocore_attachout(cs4231.port[10]+1, mpu98ii_o2);
		//iocore_attachinp(cs4231.port[10]+1, mpu98ii_i2);
		//mpu98.irqnum = mpu98.irqnum2;

	}
}
void board118_unbind(void)
{
	cs4231io_unbind();
	
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_WAVESTAR || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16){
		// Mate-X PCMの場合、CS4231だけ
		iocore_detachout(cs4231.port[1]);
		iocore_detachinp(cs4231.port[1]);
		iocore_detachinp(0x881e);
	}else{
		// 118音源の場合、色々割り当て

		// OPNA割り当て
		if(cs4231.port[4]){
			cbuscore_detachsndex(cs4231.port[4]);
		}
		
#if defined(SUPPORT_GAMEPORT)
		// ゲームポート割り当て 1480h～1487hどこでも良いらしい
		if(np2cfg.gameport){
			int i;
			for(i=0;i<=7;i++){
				iocore_detachout(0x1480+i);
				iocore_detachinp(0x1480+i);
			}
		}
#endif
		
		// OPL割り当て
		iocore_detachout(cs4231.port[9]);
		iocore_detachinp(cs4231.port[9]);
		iocore_detachout(cs4231.port[9]+1);
		iocore_detachout(cs4231.port[9]+2);
		iocore_detachout(cs4231.port[9]+3);
		
		// Sound ID I/O port割り当て
		iocore_detachout(cs4231.port[1]);
		iocore_detachinp(cs4231.port[1]);

		// SRN-F寄生 I/O port割り当て
		//iocore_detachout(cs4231.port[15]);//SRN-Fは必要なときだけ使う
		//iocore_detachinp(cs4231.port[15]);
		
		// PC-9801-118 config I/O port割り当て
		iocore_detachout(cs4231.port[14]);
		iocore_detachinp(cs4231.port[14]);
		iocore_detachout(cs4231.port[14]+1);
		iocore_detachinp(cs4231.port[14]+1);
		
		// YMF-701 I/O port割り当て
		iocore_detachout(cs4231.port[6]);// YMF-701
		iocore_detachinp(cs4231.port[6]);// YMF-701
		iocore_detachinp(cs4231.port[6]+1);// YMF-701
		
		// PC-9801-118 control? I/O port割り当て
		iocore_detachinp(cs4231.port[11]+6);
		iocore_detachinp(0x881e);
		
		// MIDI I/O port割り当て mpu98ii.c側で調整
		//iocore_detachout(cs4231.port[10]);
		//iocore_detachinp(cs4231.port[10]);
		//iocore_detachout(cs4231.port[10]+1);
		//iocore_detachinp(cs4231.port[10]+1);

	}
}
void board118_finalize(void)
{
#ifdef USE_MAME
	if (g_mame_opl3[G_OPL3_INDEX]) {
		YMF262Shutdown(g_mame_opl3[G_OPL3_INDEX]);
		g_mame_opl3[G_OPL3_INDEX] = NULL;
	}
#endif
}
