/**
 * @file	pcm86c.c
 * @brief	Implementation of the 86-PCM
 */

#include <compiler.h>
#include <sound/pcm86.h>
#include <pccore.h>
#include <cpucore.h>
#include <io/iocore.h>
#include <sound/fmboard.h>

#if 0
#undef	TRACEOUT
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
#endif	/* 1 */

/* サンプリングレートに8掛けた物 */
const UINT pcm86rate8[] = {352800, 264600, 176400, 132300,
							88200,  66150,  44010,  33075};

/* 32,24,16,12, 8, 6, 4, 3 - 最少公倍数: 96 */
/*  3, 4, 6, 8,12,16,24,32 */

static const UINT clk25_128[] = {
					0x00001bde, 0x00002527, 0x000037bb, 0x00004a4e,
					0x00006f75, 0x0000949c, 0x0000df5f, 0x00012938};
static const UINT clk20_128[] = {
					0x000016a4, 0x00001e30, 0x00002d48, 0x00003c60,
					0x00005a8f, 0x000078bf, 0x0000b57d, 0x0000f17d};


	PCM86CFG	pcm86cfg;

	UINT64 datawriteirqwait = 0;
	static SINT32 bufunferflag = 0;
	static SINT32 vbufunferflag = 0;

#if defined(SUPPORT_MULTITHREAD)
	static int pcm86_cs_initialized = 0;
	static CRITICAL_SECTION pcm86_cs;

	void pcm86cs_enter_criticalsection(void)
	{
		if (!pcm86_cs_initialized) return;
		EnterCriticalSection(&pcm86_cs);
	}
	void pcm86cs_leave_criticalsection(void)
	{
		if (!pcm86_cs_initialized) return;
		LeaveCriticalSection(&pcm86_cs);
	}

	void pcm86cs_initialize(void)
	{
		/* クリティカルセクション準備 */
		if (!pcm86_cs_initialized)
		{
			memset(&pcm86_cs, 0, sizeof(pcm86_cs));
			InitializeCriticalSection(&pcm86_cs);
			pcm86_cs_initialized = 1;
		}
	}
	void pcm86cs_shutdown(void)
	{
		/* クリティカルセクション破棄 */
		if (pcm86_cs_initialized)
		{
			memset(&pcm86_cs, 0, sizeof(pcm86_cs));
			DeleteCriticalSection(&pcm86_cs);
			pcm86_cs_initialized = 0;
		}
	}
#endif

void pcm86gen_initialize(UINT rate)
{
	pcm86cfg.rate = rate;
}

void pcm86gen_setvol(UINT vol)
{
	pcm86cfg.vol = vol;
	pcm86gen_update();
}

void pcm86_reset(void)
{
	PCM86 pcm86 = &g_pcm86;

	memset(pcm86, 0, sizeof(*pcm86));
	pcm86->fifosize = 0x80;
	pcm86->dactrl = 0x32;
	pcm86->stepmask = (1 << 2) - 1;
	pcm86->stepbit = 2;
	pcm86->stepclock = ((UINT64)pccore.baseclock << 6);
	pcm86->stepclock /= 44100;
	pcm86->stepclock *= pccore.multiple;
	pcm86->rescue = (PCM86_RESCUE * 32) << 2;
	pcm86->irq = 0xff;	
	pcm86_setpcmrate(pcm86->fifo); // デフォルト値をセット
}

void pcm86gen_update(void)
{
	PCM86 pcm86 = &g_pcm86;

	pcm86->volume = pcm86cfg.vol * pcm86->vol5;
	pcm86_setpcmrate(pcm86->fifo);
}

void pcm86_setwaittime()
{
	// WORKAROUND: データ書き込みから割り込み発生までの時間が短すぎると不具合が起こる場合があるのでわざと遅延 値に根拠はなし
	datawriteirqwait = 20000 * pccore.multiple;
}

void pcm86_setpcmrate(REG8 val)
{
	PCM86 pcm86 = &g_pcm86;
	SINT32	rate;

	pcm86->rateval = rate = pcm86rate8[val & 7];
	pcm86->stepclock = ((UINT64)pccore.baseclock << 6);
	pcm86->stepclock /= rate;
	pcm86->stepclock *= (pccore.multiple << 3);
	if (pcm86cfg.rate)
	{
		pcm86->div = (rate << (PCM86_DIVBIT - 3)) / pcm86cfg.rate;
		pcm86->div2 = (pcm86cfg.rate << (PCM86_DIVBIT + 3)) / rate;
	}

	pcm86_setwaittime();
}

void pcm86_cb(NEVENTITEM item)
{
	PCM86 pcm86 = &g_pcm86;
	SINT32 adjustbuf;

	if (pcm86->reqirq)
	{
		sound_sync();
		//		RECALC_NOWCLKP;

		adjustbuf = (SINT32)(((SINT64)pcm86->virbuf * 4 + pcm86->realbuf) / 5);
		if (pcm86->virbuf <= pcm86->fifosize || pcm86->realbuf > pcm86->stepmask && adjustbuf <= pcm86->fifosize)
		{
			pcm86->reqirq = 0;
			pcm86->irqflag = 1;
			if (pcm86->irq != 0xff)
			{
				pic_setirq(pcm86->irq);
			}
		}
		else
		{
			pcm86_setnextintr();
		}
	}
	else
	{
		pcm86_setnextintr();
	}

	(void)item;
}

void pcm86_setnextintr(void) {

	PCM86 pcm86 = &g_pcm86;
	SINT32	cntv;
	SINT32	cntr;
	SINT32	cnt;
	SINT32	clk;

	if (pcm86->fifo & 0x80)
	{
		cntv = pcm86->virbuf - pcm86->fifosize;
		cntr = pcm86->realbuf - pcm86->fifosize;
		if (pcm86->realbuf > pcm86->stepmask)
		{
			if (cntr < cntv)
			{
				//cnt = cntr;
				if (bufunferflag > 32000)
				{
					cnt = (SINT32)(((SINT64)cntv * 9 + cntr) / 10);
					TRACEOUT(("Buf Under2", bufunferflag));
				}
				else if (bufunferflag > 4000)
				{
					cnt = (SINT32)(((SINT64)cntv * 99 + cntr) / 100);
					TRACEOUT(("Buf Under", bufunferflag));
				}
				else
				{
					cnt = cntv;
				}
			}
			else
			{
				if (vbufunferflag > 64000)
				{
					cnt = (SINT32)(((SINT64)cntv * 9 + cntr) / 10);
					TRACEOUT(("VBuf Under3", vbufunferflag));
				}
				else if (vbufunferflag > 16000)
				{
					cnt = (SINT32)(((SINT64)cntv * 49 + cntr) / 50);
					TRACEOUT(("VBuf Under2", vbufunferflag));
				}
				else if (vbufunferflag > 4000)
				{
					cnt = (SINT32)(((SINT64)cntv * 99 + cntr) / 100);
					TRACEOUT(("VBuf Under", vbufunferflag));
				}
				else
				{
					cnt = cntv;
				}
			}
		}
		else
		{
			cnt = cntv;
		}
		//cnt = (SINT32)(((SINT64)cntv + cntr) / 2);
		if (cnt > 0)
		{
			cnt += pcm86->stepmask;
			cnt >>= pcm86->stepbit;
			//cnt += 4;
			/* ここで clk = pccore.realclock * cnt / 86pcm_rate */
			/* clk = ((pccore.baseclock / 86pcm_rate) * cnt) * pccore.multiple */
			if (pccore.cpumode & CPUMODE_8MHZ) {
				clk = clk20_128[pcm86->fifo & 7];
			}
			else {
				clk = clk25_128[pcm86->fifo & 7];
			}
			/* cntは最大 8000h で 32bitで収まるように… */
			clk *= cnt;
			clk >>= 7;
//			clk++;						/* roundup */
			//clk--;						/* roundup */
			//if (clk > 1) clk--;
			clk *= pccore.multiple;
			nevent_set(NEVENT_86PCM, clk, pcm86_cb, NEVENT_ABSOLUTE);
			TRACEOUT(("%d,%d", pcm86->virbuf, pcm86->realbuf));
		}
		else
		{
			if (pcm86->reqirq)
			{
				// 即時
				nevent_set(NEVENT_86PCM, 1, pcm86_cb, NEVENT_ABSOLUTE);
			}
			else
			{
				// WORKAROUND: 前回の割り込みがうまくいっていない。適当な時間間隔で再度割り込みを送る。早すぎるとNT4等が割り込み無限ループするので注意
				pcm86->reqirq = 1;
				nevent_set(NEVENT_86PCM, 100 * pccore.multiple, pcm86_cb, NEVENT_ABSOLUTE);
			}
		}
	}
}

void RECALC_NOWCLKWAIT(UINT64 cnt)
{
	SINT64 decvalue = (SINT64)(cnt << g_pcm86.stepbit);
#if defined(SUPPORT_MULTITHREAD)
	pcm86cs_enter_criticalsection();
#endif
	if (g_pcm86.virbuf - decvalue < g_pcm86.virbuf)
	{
		g_pcm86.virbuf -= decvalue;
	}
	if (g_pcm86.virbuf < 0)
	{
		g_pcm86.virbuf &= g_pcm86.stepmask;
	}
#if defined(SUPPORT_MULTITHREAD)
	pcm86cs_leave_criticalsection();
#endif
}

void pcm86_changeclock(UINT oldmultiple)
{
	PCM86 pcm86 = &g_pcm86;
	if(pcm86){
		if(pcm86->rateval){
			UINT64	cur;
			UINT64	past;
			UINT64  pastCycle;
			UINT64	newstepclock;
			newstepclock = ((UINT64)pccore.baseclock << 6);
			newstepclock /= pcm86->rateval;
			newstepclock *= ((UINT64)pccore.multiple << 3);
			pastCycle = (UINT64)UINT_MAX << 6;
			cur = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
			cur <<= 6;
			past = (cur + pastCycle - pcm86->lastclock) % pastCycle;
			if (past > pastCycle / 2)
			{
				// 負の値になってしまっているとき
				if (past < pastCycle - pcm86->stepclock * 4)
				{
					// かなり小さいならリセットをかける
					past = 1;
					pcm86->lastclock = cur - 1;
				}
				else
				{
					// 小さいなら様子見で0扱いとする
					past = 0;
				}
			}
			if (past >= pcm86->stepclock)
			{
				//SINT32 latvirbuf = pcm86->virbuf;
				past = past / pcm86->stepclock;
				pcm86->lastclock = (pcm86->lastclock + past * pcm86->stepclock) % pastCycle;
				RECALC_NOWCLKWAIT(past);
				//TRACEOUT(("%d %d %d", latvirbuf, pcm86->virbuf, past));
			}
			past = (cur + pastCycle - pcm86->lastclock) % pastCycle;
			pcm86->lastclock = (cur + pastCycle - (past * newstepclock + pcm86->stepclock / 2) / pcm86->stepclock) % pastCycle; // 補正
			pcm86->stepclock = newstepclock;

			pcm86_setwaittime();
		}else{
			//pcm86->stepclock = ((UINT64)pccore.baseclock << 6);
			//pcm86->stepclock /= 44100;
			//pcm86->stepclock *= pccore.multiple;
		}
	}
}

void SOUNDCALL pcm86gen_checkbuf(PCM86 pcm86, UINT nCount)
{
	long	bufs;
	UINT64	cur;
	UINT64	past;
	UINT64  pastCycle;
	UINT64	curClock;
	SINT32	flagStep;
	static UINT32	lastClock = 0;
	curClock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	if (lastClock == 0)
	{
		flagStep = 0;
	}
	else
	{
		flagStep = (SINT32)((SINT64)(curClock - lastClock) * 1000 / pccore.realclock);
	}
	//TRACEOUT(("FS %d", flagStep));

	pastCycle = (UINT64)UINT_MAX << 6;
	cur = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	cur <<= 6;
	past = (cur + pastCycle - pcm86->lastclock) % pastCycle;
	if (past > pastCycle / 2)
	{
		// 負の値になってしまっているとき
		if (past < pastCycle - pcm86->stepclock * 4)
		{
			// かなり小さいならリセットをかける
			past = 1;
			pcm86->lastclock = cur - 1;
		}
		else
		{
			// 小さいなら様子見で0扱いとする
			past = 0;
		}
	}
	if (past >= pcm86->stepclock)
	{
		past = past / pcm86->stepclock;
		pcm86->lastclock = (pcm86->lastclock + past * pcm86->stepclock) % pastCycle;
		RECALC_NOWCLKWAIT(past);
	}

	//pcm86->virbuf = pcm86->realbuf;
	bufs = pcm86->realbuf - pcm86->virbuf;
	if (bufs < 0)
	{
		// 処理落ちてるかもしれない
		if (bufs <= -pcm86->fifosize && pcm86->virbuf < pcm86->fifosize)
		{
			// 駄目そうな場合
			//TRACEOUT(("CRITCAL buf: real %d, vir %d, (FIFOSIZE: %d) FORCE IRQ", pcm86->realbuf, pcm86->virbuf, pcm86->fifosize));
			bufs &= ~3;
			pcm86->virbuf += bufs;
			// nevent_setでデッドロックする可能性があるので割り込みはしない
		}
		//TRACEOUT(("buf: real %d, vir %d, (FIFOSIZE: %d) FORCE IRQ", pcm86->realbuf, pcm86->virbuf, pcm86->fifosize));
		//pcm86->lastclock += pcm86->stepclock / 50;
		if (bufunferflag < INT_MAX - flagStep)
		{
			if (lastClock != 0)
			{
				bufunferflag += flagStep;
			}
			//TRACEOUT(("%d", bufunferflag));
		}
	//}
	//else if (bufs < pcm86->fifosize)
	//{
	//	if (bufunferflag < 8000)
	//	{
	//		if (lastClock != 0)
	//		{
	//			bufunferflag += flagStep;
	//		}
	//		//TRACEOUT(("%d", bufunferflag));
	//	}
	}
	else
	{
		// 余裕あり
		//bufs -= PCM86_EXTBUF;
		//if (bufs > 0)
		//{
		//	bufs &= ~3;
		//	pcm86->realbuf -= bufs;
		//	pcm86->readpos += bufs;
		//}

		//TRACEOUT(("%d,%d", pcm86->virbuf, pcm86->realbuf));
		if (pcm86->virbuf > pcm86->fifosize && pcm86->realbuf > pcm86->stepmask && pcm86->realbuf > pcm86->virbuf + pcm86->fifosize * 3)
		{
			//if (vbufunferflag < INT_MAX - flagStep)
			//{
			//	if (lastClock != 0)
			//	{
			//		vbufunferflag += flagStep;
			//	}
			//	//TRACEOUT(("v %d", vbufunferflag));
			//}
			pcm86->virbuf += (pcm86->realbuf - (pcm86->virbuf - pcm86->fifosize * 3)) / 8;
			TRACEOUT(("ADJUST!"));
		}
		//else
		//{
		//	//vbufunferflag = 0;
		//	if (lastClock != 0)
		//	{
		//		vbufunferflag -= flagStep;
		//	}
		//	if (vbufunferflag < 0) vbufunferflag = 0;
		//}
		bufunferflag = 0;
	}

	lastClock = curClock;
}

BOOL pcm86gen_intrq(int fromFMTimer)
{
	PCM86 pcm86 = &g_pcm86;
	UINT64 curclk = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	// WORKAROUND: データ書き込みから割り込み発生までの時間が短すぎると不具合が起こる場合があるのでわざと遅延
	// XXX: 本当は循環しているのでg_pcm86.lastclockforwaitセットから時間が経ちすぎると不味い
	// しかし仮に500MHzとしたときUINT64が1周するのは40万日くらいなので事実上問題ない
	if (!(pcm86->fifo & 0x20) || !(curclk - g_pcm86.lastclockforwait >= datawriteirqwait))
	{
		return FALSE;
	}
	if (pcm86->irqflag)
	{
		return TRUE;
	}
	if (!nevent_iswork(NEVENT_86PCM)) {
		sound_sync();
		if (!(pcm86->irqflag) && (pcm86->virbuf <= pcm86->fifosize || pcm86->realbuf > pcm86->stepmask && pcm86->realbuf <= pcm86->fifosize))
		{
			//pcm86->reqirq = 0;
			pcm86->irqflag = 1;
			return TRUE;
		}
	}
	return FALSE;
}