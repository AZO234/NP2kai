#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"pcm86io.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"cs4231io.h"


extern	PCM86CFG	pcm86cfg;

static const UINT8 pcm86bits[] = {1, 1, 1, 2, 0, 0, 0, 1};
static const SINT32 pcm86rescue[] = {PCM86_RESCUE * 32, PCM86_RESCUE * 24,
									 PCM86_RESCUE * 16, PCM86_RESCUE * 12,
									 PCM86_RESCUE *  8, PCM86_RESCUE *  6,
									 PCM86_RESCUE *  4, PCM86_RESCUE *  3};

static const UINT8 s_irqtable[8] = {0xff, 0xff, 0xff, 0xff, 0x03, 0x0a, 0x0d, 0x0c};

// for Q-Vision Wave Star (XXX: この辺のポートを操作中にResumeやStateSaveを使うとやばい)
static void pcm86_updateWaveStarPorts();
REG8 wavestar_a462_seq[] = {0xa6, 0xd3, 0x69, 0xb4, 0x5a};
REG8 wavestar_a462_seq_index = 0;
REG8 wavestar_a464_value = 0xff;

static void IOOUTCALL pcm86_oa460(UINT port, REG8 val)
{
//	TRACEOUT(("86pcm out %.4x %.2x", port, val));
	g_pcm86.soundflags = (g_pcm86.soundflags & 0xfe) | (val & 1);
	fmboard_extenable((REG8)(val & 1));
	(void)port;
}

static void IOOUTCALL pcm86_oa466(UINT port, REG8 val) {

//	TRACEOUT(("86pcm out %.4x %.2x", port, val));
	if ((val & 0xe0) == 0xa0) {
		sound_sync();
		g_pcm86.vol5 = (~val) & 15;
		g_pcm86.volume = pcm86cfg.vol * g_pcm86.vol5;
	}
	// WaveStar FM音量？
	if((g_nSoundID == SOUNDID_WAVESTAR && (val & 0xe0) == 0x00)){
		UINT i;
		cs4231.devvolume[0xff] = (~val) & 15;
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
		for (i = 0; i < NELEMENTS(g_opna); i++)
		{
			rhythm_update(&g_opna[i].rhythm);
		}
	}

	(void)port;
}

static void IOOUTCALL pcm86_oa468(UINT port, REG8 val) {

	REG8	xchgbit;
	
//	TRACEOUT(("86pcm out %.4x %.2x", port, val));
	sound_sync();
	xchgbit = g_pcm86.fifo ^ val;
	// バッファリセット判定
	if ((xchgbit & 8) && (val & 8)) {
		g_pcm86.readpos = 0;				// バッファリセット
		g_pcm86.wrtpos = 0;
		g_pcm86.realbuf = 0;
		g_pcm86.virbuf = 0;
		g_pcm86.lastclock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
		g_pcm86.lastclock <<= 6;
	}
	if ((xchgbit & 0x10) && (!(val & 0x10))) {
		g_pcm86.irqflag = 0;
//		g_pcm86.write = 0;
//		g_pcm86.reqirq = 0;
	}
	// サンプリングレート変更
	if (xchgbit & 7) {
		g_pcm86.rescue = pcm86rescue[val & 7] << g_pcm86.stepbit;
		pcm86_setpcmrate(val);
	}
#if 1	// これ重大なバグ....
	g_pcm86.fifo = val;
#else
	g_pcm86.fifo = val & (~0x10);
#endif
	if ((xchgbit & 0x80) && (val & 0x80)) {
		g_pcm86.lastclock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
		g_pcm86.lastclock <<= 6;
	}
	pcm86_setnextintr();
	(void)port;
}

static void IOOUTCALL pcm86_oa46a(UINT port, REG8 val) {
	
//	TRACEOUT(("86pcm out %.4x %.2x", port, val));
	sound_sync();
	if (g_pcm86.fifo & 0x20) {
#if 1
		if (val != 0xff) {
			g_pcm86.fifosize = (UINT16)((val + 1) << 7);
		}
		else {
			g_pcm86.fifosize = 0x7ffc;
		}
#else
		if (!val) {
			val++;
		}
		g_pcm86.fifosize = (WORD)(val) << 7;
#endif
	}
	else {
		g_pcm86.dactrl = val;
		g_pcm86.stepbit = pcm86bits[(val >> 4) & 7];
		g_pcm86.stepmask = (1 << g_pcm86.stepbit) - 1;
		g_pcm86.rescue = pcm86rescue[g_pcm86.fifo & 7] << g_pcm86.stepbit;
	}
	pcm86_setnextintr();
	(void)port;
}

static void IOOUTCALL pcm86_oa46c(UINT port, REG8 val) {
	
//	TRACEOUT(("86pcm out %.4x %.2x", port, val));
#if 1
	if (g_pcm86.virbuf < PCM86_LOGICALBUF) {
		g_pcm86.virbuf++;
	}
	g_pcm86.buffer[g_pcm86.wrtpos] = val;
	g_pcm86.wrtpos = (g_pcm86.wrtpos + 1) & PCM86_BUFMSK;
	g_pcm86.realbuf++;
	// バッファオーバーフローの監視
	if (g_pcm86.realbuf >= PCM86_REALBUFSIZE) {
#if 1
		g_pcm86.realbuf -= 4;
		g_pcm86.readpos = (g_pcm86.readpos + 4) & PCM86_BUFMSK;
#else
		g_pcm86.realbuf &= 3;				// align4決めウチ
		g_pcm86.realbuf += PCM86_REALBUFSIZE - 4;
#endif
	}
//	g_pcm86.write = 1;
	g_pcm86.reqirq = 1;
#else
	if (g_pcm86.virbuf < PCM86_LOGICALBUF) {
		g_pcm86.virbuf++;
		g_pcm86.buffer[g_pcm86.wrtpos] = val;
		g_pcm86.wrtpos = (g_pcm86.wrtpos + 1) & PCM86_BUFMSK;
		g_pcm86.realbuf++;
		// バッファオーバーフローの監視
		if (g_pcm86.realbuf >= PCM86_REALBUFSIZE) {
			g_pcm86.realbuf &= 3;				// align4決めウチ
			g_pcm86.realbuf += PCM86_REALBUFSIZE - 4;
		}
//		g_pcm86.write = 1;
		g_pcm86.reqirq = 1;
	}
#endif
	(void)port;
}

static REG8 IOINPCALL pcm86_ia460(UINT port)
{
	(void)port;
	return g_pcm86.soundflags;
}

static REG8 IOINPCALL pcm86_ia466(UINT port) {

	UINT32	past;
	UINT32	cnt;
	UINT32	stepclock;
	REG8	ret;
	
	past = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	past <<= 6;
	past -= g_pcm86.lastclock;
	stepclock = g_pcm86.stepclock;
	if (past >= stepclock) {
		cnt = past / stepclock;
		g_pcm86.lastclock += (cnt * stepclock);
		past -= cnt * stepclock;
		if (g_pcm86.fifo & 0x80) {
			sound_sync();
			RECALC_NOWCLKWAIT(cnt);
		}
	}
	ret = ((past << 1) >= stepclock)?1:0;
	if (g_pcm86.virbuf >= PCM86_LOGICALBUF) {			// バッファフル
		ret |= 0x80;
	}
	else if (!g_pcm86.virbuf) {						// バッファ０
		ret |= 0x40;								// ちと変…
	}
	(void)port;
//	TRACEOUT(("86pcm in %.4x %.2x", port, ret));
	return(ret);
}

static REG8 IOINPCALL pcm86_ia468(UINT port) {

	REG8	ret;
	
	ret = g_pcm86.fifo & (~0x10);
#if 1
	if (pcm86gen_intrq()) {
		ret |= 0x10;
	}
#elif 1		// むしろこう？
	if (g_pcm86.fifo & 0x20) {
		sound_sync();
		if (g_pcm86.virbuf <= g_pcm86.fifosize) {
			if (g_pcm86.write) {
				g_pcm86.write = 0;
			}
			else {
				ret |= 0x10;
			}
		}
	}
#else
	if ((g_pcm86.write) && (g_pcm86.fifo & 0x20)) {
//		g_pcm86.write = 0;
		sound_sync();
		if (g_pcm86.virbuf <= g_pcm86.fifosize) {
			g_pcm86.write = 0;
			ret |= 0x10;
		}
	}
#endif
	(void)port;
//	TRACEOUT(("86pcm in %.4x %.2x", port, ret));
	return(ret);
}

static REG8 IOINPCALL pcm86_ia46a(UINT port) {
	
	(void)port;
//	TRACEOUT(("86pcm in %.4x %.2x", port, g_pcm86.dactrl));
	return(g_pcm86.dactrl);
}

static REG8 IOINPCALL pcm86_inpdummy(UINT port) {

	(void)port;
	return(0);
}

// for Q-Vision Wave Star
static void IOOUTCALL pcm86_oa462(UINT port, REG8 val)
{
	if(wavestar_a462_seq_index < sizeof(wavestar_a462_seq) && val == wavestar_a462_seq[wavestar_a462_seq_index]){
		wavestar_a462_seq_index++;
		if(wavestar_a462_seq_index == sizeof(wavestar_a462_seq)){
			wavestar_a464_value = 0x0b;
		}
	}else if(val == wavestar_a462_seq[0]){
		wavestar_a462_seq_index = 1;
	}else{
		wavestar_a462_seq_index = 0;
	}
	(void)port;
}
static REG8 IOINPCALL pcm86_ia462(UINT port) {

	(void)port;

	return(0xff);
}
static void IOOUTCALL pcm86_oa464(UINT port, REG8 val)
{
	if(wavestar_a462_seq_index == sizeof(wavestar_a462_seq)){
		if(val == 0x04){
			cs4231.devvolume[0xfe] = 1; // XXX: 暫定でフラグ用に借りる（本来は変数を追加すべき）
			wavestar_a464_value = 0x0c;
		}else{
			cs4231.devvolume[0xfe] = 0; // XXX: 暫定でフラグ用に借りる（本来は変数を追加すべき）
			wavestar_a464_value = 0x08;
		}
		pcm86_updateWaveStarPorts();
	}
	if(val == 0x09) wavestar_a464_value = 0xff;
	(void)port;
}
static REG8 IOINPCALL pcm86_ia464(UINT port) {

	REG8 ret;
	(void)port;
	if(wavestar_a462_seq_index != sizeof(wavestar_a462_seq)){
		wavestar_a464_value = 0xff;
	}
	ret = wavestar_a464_value;
	if(wavestar_a464_value==0x00){
		wavestar_a464_value = 0xff;
	}else{
		wavestar_a464_value = 0x00;
	}
	return(ret);
}
static void pcm86_updateWaveStarPorts(){
	if(cs4231.devvolume[0xfe]){
		// I/OポートをWSSに変更
		iocore_detachout(0xa460);
		//iocore_attachout(0xa464, cs4231io0_w8_wavestar);
		iocore_attachout(0xa466, cs4231io0_w8_wavestar);
		iocore_attachout(0xa468, cs4231io0_w8_wavestar);
		iocore_attachout(0xa46a, cs4231io0_w8_wavestar);
		iocore_attachout(0xa46c, cs4231io0_w8_wavestar);
		iocore_detachinp(0xa460);
		iocore_attachinp(0xa464, cs4231io0_r8_wavestar);
		iocore_attachinp(0xa466, cs4231io0_r8_wavestar);
		iocore_attachinp(0xa468, cs4231io0_r8_wavestar);
		iocore_attachinp(0xa46a, cs4231io0_r8_wavestar);
		iocore_attachinp(0xa46c, cs4231io0_r8_wavestar);
		
		// OPNA割り込み無効
		g_pcm86.irq = 0xff;
		g_opna[0].s.irq = 0xff;
	}else{
		// I/Oポートを86互換に変更
		iocore_attachout(0xa460, pcm86_oa460);
		iocore_attachout(0xa462, pcm86_oa462);
		iocore_attachout(0xa464, pcm86_oa464);
		iocore_attachout(0xa466, pcm86_oa466);
		iocore_attachout(0xa468, pcm86_oa468);
		iocore_attachout(0xa46a, pcm86_oa46a);
		iocore_attachout(0xa46c, pcm86_oa46c);
		iocore_attachinp(0xa460, pcm86_ia460);
		iocore_attachinp(0xa462, pcm86_ia462);
		iocore_attachinp(0xa464, pcm86_ia464);
		iocore_attachinp(0xa466, pcm86_ia466);
		iocore_attachinp(0xa468, pcm86_ia468);
		iocore_attachinp(0xa46a, pcm86_ia46a);
		iocore_attachinp(0xa46c, pcm86_inpdummy);
		iocore_attachinp(0xa46e, pcm86_inpdummy);
		
		// OPNA割り込み有効
		g_pcm86.irq = cs4231.devvolume[0xfd];
		g_opna[0].s.irq = cs4231.devvolume[0xfc];
	}
}


// ----

/**
 * Reset
 * @param[in] cDipSw Dip switch
 */
void pcm86io_setopt(REG8 cDipSw)
{
	g_pcm86.soundflags = ((~cDipSw) >> 1) & 0x70;
	g_pcm86.irq = s_irqtable[(cDipSw >> 2) & 7];

	if(g_nSoundID==SOUNDID_WAVESTAR){
		g_pcm86.soundflags = 0x41;
		fmboard_extenable(1);
		cs4231.devvolume[0xfd] = g_pcm86.irq;
		cs4231.devvolume[0xfc] = g_opna[0].s.irq;
	}
}

void pcm86io_bind(void) {

	sound_streamregist(&g_pcm86, (SOUNDCB)pcm86gen_getpcm);

	iocore_attachout(0xa460, pcm86_oa460);
	if(g_nSoundID == SOUNDID_WAVESTAR){
		iocore_attachout(0xa462, pcm86_oa462);
		iocore_attachout(0xa464, pcm86_oa464);
		wavestar_a462_seq_index = 0;
		wavestar_a464_value = 0xff;
	}
	iocore_attachout(0xa466, pcm86_oa466);
	iocore_attachout(0xa468, pcm86_oa468);
	iocore_attachout(0xa46a, pcm86_oa46a);
	iocore_attachout(0xa46c, pcm86_oa46c);

	iocore_attachinp(0xa460, pcm86_ia460);
	if(g_nSoundID == SOUNDID_WAVESTAR){
		iocore_attachinp(0xa462, pcm86_ia462);
		iocore_attachinp(0xa464, pcm86_ia464);
	}else{
		iocore_attachinp(0xa462, pcm86_inpdummy);
		iocore_attachinp(0xa464, pcm86_inpdummy);
	}
	iocore_attachinp(0xa466, pcm86_ia466);
	iocore_attachinp(0xa468, pcm86_ia468);
	iocore_attachinp(0xa46a, pcm86_ia46a);
	iocore_attachinp(0xa46c, pcm86_inpdummy);
	iocore_attachinp(0xa46e, pcm86_inpdummy);
	
	if(g_nSoundID == SOUNDID_WAVESTAR){
		pcm86_updateWaveStarPorts();
	}
}
void pcm86io_unbind(void) {
	
	iocore_detachout(0xa460);
	if(g_nSoundID == SOUNDID_WAVESTAR){
		iocore_detachout(0xa462);
		iocore_detachout(0xa464);
	}
	iocore_detachout(0xa466);
	iocore_detachout(0xa468);
	iocore_detachout(0xa46a);
	iocore_detachout(0xa46c);

	iocore_detachinp(0xa460);
	iocore_detachinp(0xa462);
	iocore_detachinp(0xa464);
	iocore_detachinp(0xa466);
	iocore_detachinp(0xa468);
	iocore_detachinp(0xa46a);
	iocore_detachinp(0xa46c);
	iocore_detachinp(0xa46e);
}

