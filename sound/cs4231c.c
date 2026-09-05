/**
 * @file	cs4231c.c
 * @brief	Implementation of the CS4231
 */

#include <compiler.h>
#include <sound/cs4231.h>
#include <io/iocore.h>
#include <sound/fmboard.h>
#include <io/dmac.h>
#include <sound/sndmtcs.h>

SNDMTCS_DECL(cs4231);
#include <cpucore.h>

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef CPU_STAT_PM
#define CPU_STAT_PM	0
#endif
#define CS4231_BUFREADSMP	128

#if 0
#undef  TRACEOUT
static void trace_fmt_ex(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "¥n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define TRACEOUT(s) trace_fmt_ex s
#endif

	CS4231CFG	cs4231cfg;

	int cs4231_bufdelaycounter = 0;
	
	static int playcountsmp_Ictl = CS4231_BUFREADSMP; // 積分制御で無理やり一定サンプルずつ読むようにする･･･
	
// 1サンプルあたりのバイト数（モノラル, ステレオの順）
static const SINT32 cs4231_playcountshift[16] = {
			1  ,		// 0: 8bit PCM
			1*2,
			1  ,		// 1: u-Law
			1  ,
			1*2,		// 2: 16bit PCM(little endian)?
			1*4,
			1  ,		// 3: A-Law
			1  ,
			1  ,		// 4:
			1  ,
			1  ,		// 5: ADPCM
			1  ,
			1*2,		// 6: 16bit PCM
			1*4,
			1  ,		// 7: ADPCM
			1  };

// Indirect Mapped Registers
enum {
	CS4231REG_LINPUT	= 0x00, // Left ADC Input Control (I0)
	CS4231REG_RINPUT	= 0x01, // Right ADC Input Control (I1)
	CS4231REG_AUX1L		= 0x02, // Left Auxiliary #1 Input Control (I2)
	CS4231REG_AUX1R		= 0x03, // Right Auxiliary #1 Input Control (I3)
	CS4231REG_AUX2L		= 0x04, // Left Auxiliary #2 Input Control (I4)
	CS4231REG_AUX2R		= 0x05, // Right Auxiliary #2 Input Control (I5)
	CS4231REG_LOUTPUT	= 0x06, // Left DAC Output Control (I6)
	CS4231REG_ROUTPUT	= 0x07, // Right DAC Output Control (I7)
	CS4231REG_PLAYFMT	= 0x08, // Fs and Playback Data Format (I8)
	CS4231REG_INTERFACE	= 0x09, // Interface Configuration (I9)
	CS4231REG_PINCTRL	= 0x0a, // Pin Control (I10)
	CS4231REG_TESTINIT	= 0x0b, // Error Status and Initialization (I11, Read Only)
	CS4231REG_MISCINFO	= 0x0c, // MODE and ID (I12)
	CS4231REG_LOOPBACK	= 0x0d, // Loopback Control (I13)
	CS4231REG_PLAYCNTM	= 0x0e, // Playback Upper Base (I14)
	CS4231REG_PLAYCNTL	= 0x0f, // Playback Lower Base (I15)

	CS4231REG_FEATURE1	= 0x10, // Alternate Feature Enable I (I16)
	CS4231REG_FEATURE2	= 0x11, // Alternate Feature Enable II (I17)
	CS4231REG_LLINEIN	= 0x12, // Left Line Input Control (I18)
	CS4231REG_RLINEIN	= 0x13, // Right Line Input Control (I19)
	CS4231REG_TIMERL	= 0x14, // Timer Lower Base (I20)
	CS4231REG_TIMERH	= 0x15, // Timer Upper Base (I21)
	CS4231REG_RESERVED1 = 0x16, // RESERVED (I22)
	CS4231REG_RESERVED2 = 0x17, // Alternate Feature Enable III (I23)
	CS4231REG_IRQSTAT	= 0x18, // Alternate Feature Status (I24)
	CS4231REG_VERSION	= 0x19, // Version / ID (I25)
	CS4231REG_MONOCTRL	= 0x1a, // Mono Input & Output Control (I26)
	CS4231REG_RESERVED3	= 0x1b, // RESERVED (I27)
	CS4231REG_RECFMT	= 0x1c, // Capture Data Format (I28)
	CS4231REG_PLAYFREQ	= 0x1d, // RESERVED (I29)
	CS4231REG_RECCNTM	= 0x1e, // Capture Upper Base (I30)
	CS4231REG_RECCNTL	= 0x1f  // Capture Lower Base (I31)
};


UINT dmac_getdata_(DMACH dmach, UINT8 *buf, UINT offset, UINT size);
static UINT dmac_putcapture_(DMACH dmach, UINT size);
static const UINT32 cs4231xtal64[2] = {24576000/64, 16934400/64};

static const UINT8 cs4231cnt64[8] = {
				3072/64,	//  8000/ 5510
				1536/64,	// 16000/11025
				 896/64,	// 27420/18900
				 768/64,	// 32000/22050
				 448/64,	// 54840/37800
				 384/64,	// 64000/44100
				 512/64,	// 48000/33075
				2560/64};	//  9600/ 6620

//    640:441

#if defined(SUPPORT_MULTITHREAD)
void cs4231cs_enter_criticalsection(void) { SNDMTCS_ENTER(cs4231); }
void cs4231cs_leave_criticalsection(void) { SNDMTCS_LEAVE(cs4231); }
void cs4231cs_initialize(void)
{
    /* クリティカルセクション準備 */
    if (!cs4231_cs_initialized)
    {
	memset(&cs4231_cs, 0, sizeof(cs4231_cs));
	SNDMTCS_INIT(cs4231);
        cs4231_cs_initialized = 1;
    }
}
void cs4231cs_shutdown(void)
{
    /* クリティカルセクション破棄 */
    if (cs4231_cs_initialized)
    {
        SNDMTCS_TERM(cs4231);
        memset(&cs4231_cs, 0, sizeof(cs4231_cs));
	cs4231_cs_initialized = 0;
    }
}
#endif

void cs4231_initialize(UINT rate) {

	cs4231cfg.rate = rate;
}

void cs4231_reset(void) {

	ZeroMemory(&cs4231, sizeof(cs4231));
	cs4231.bufsize = CS4231_BUFFERS;
//	cs4231.proc = cs4231_nodecode;
	cs4231.dmach = 0xff;
	cs4231.dmairq = 0xff;
	cs4231.totalsample = 0;
	FillMemory(cs4231.port, sizeof(cs4231.port), 0xff);
}

void cs4231_update(void) {
}

// 廃止：cs4231g.cで調整
void cs4231_setvol(UINT vol) {

	(void)vol;
}

// CS4231 DMA処理
void cs4231_dma(NEVENTITEM item) {

	DMACH	dmach;
	UINT	rem;
	UINT	pos;
	UINT	size;
	UINT	r = 0;
	BOOL	playback_dma;
	BOOL	capture_dma;

	if (item->flag & NEVENT_SETEVENT) {
		if (cs4231.dmach != 0xff) {
			dmach = dmac.dmach + cs4231.dmach;
			playback_dma = ((cs4231.reg.iface & (PEN | PPIO)) == PEN);
			capture_dma = ((cs4231.reg.iface & (CEN | CPIO)) == CEN);

			if (playback_dma) {
				// サウンド再生用バッファに送る(cs4231g.c)
				sound_sync();

#if defined(SUPPORT_MULTITHREAD)
				cs4231cs_enter_criticalsection();
#endif
				if (cs4231.bufsize * cs4231_playcountshift[cs4231.reg.datafmt >> 4] / 4 - 4 > cs4231.bufdatas) {
					rem = min(cs4231.bufsize - 4 - cs4231.bufdatas, CS4231_MAXDMAREADBYTES);
					pos = cs4231.bufwpos & CS4231_BUFMASK;
					size = min(rem, (UINT)dmach->startcount + 1);
					r = dmac_getdata_(dmach, cs4231.buffer, pos, size);
					cs4231.bufwpos = (cs4231.bufwpos + r) & CS4231_BUFMASK;
					cs4231.bufdatas += r;
				}
#if defined(SUPPORT_MULTITHREAD)
				cs4231cs_leave_criticalsection();
#endif
			}
			else if (capture_dma) {
				size = min((UINT)dmach->startcount + 1, (UINT)CS4231_MAXDMAREADBYTES);
				r = dmac_putcapture_(dmach, size);
			}

			// NEVENTをセット。TC/停止後はdmac.maskまたはreadyで再登録されない。
			if (cs4231cfg.rate &&
				!(dmac.mask & (1 << cs4231.dmach)) && dmach->ready &&
				(playback_dma || capture_dma)) {
				playcountsmp_Ictl += ((CS4231_BUFREADSMP - (int)r) / cs4231_playcountshift[cs4231.reg.datafmt >> 4])/2;
				if(playcountsmp_Ictl < 1)
					playcountsmp_Ictl = 1;
				if(playcountsmp_Ictl > CS4231_MAXDMAREADBYTES)
					playcountsmp_Ictl = CS4231_MAXDMAREADBYTES;
				nevent_set(NEVENT_CS4231, pccore.realclock / cs4231cfg.rate * playcountsmp_Ictl, cs4231_dma, NEVENT_RELATIVE);
			}
		}
	}
	(void)item;
}

// PIO再生用
void cs4231_datasend(REG8 dat) {
	UINT	pos;
	if (cs4231.reg.iface & PPIO) {		// PIO play enable
		if (cs4231.bufdatas > 0)
		{
			sound_sync();
		}
#if defined(SUPPORT_MULTITHREAD)
		cs4231cs_enter_criticalsection();
#endif
		if (cs4231.bufsize > cs4231.bufdatas) {
			pos = (cs4231.bufwpos) & CS4231_BUFMASK;
			cs4231.buffer[pos] = dat;
			cs4231.bufdatas++;
			cs4231.bufwpos = (cs4231.bufwpos + 1) & CS4231_BUFMASK;
		}
		if (cs4231.bufdatas > CS4231_PIOBUFFERS)
		{
			cs4231.intflag &= ~PRDY;
		}
		else
		{
			cs4231.intflag |= PRDY;
		}

		if (cs4231.reg.iface & PEN) {
			UINT32 bytespercount;
			UINT32 playbytes;
			UINT32 basecount;
			REG8 format;

			format = cs4231.reg.datafmt;
			if ((format & 0xe0) == 0xa0) {
				bytespercount = 4;	/* ADPCM: count is based on 4-byte words */
			}
			else {
				bytespercount = 1;
				if ((format & 0xe0) == 0x40 || (format & 0xe0) == 0xc0) {
					bytespercount <<= 1;	/* 16-bit sample */
				}
				if (format & 0x10) {
					bytespercount <<= 1;	/* stereo sample frame */
				}
			}

			basecount = ((UINT32)cs4231.reg.playcount[0] << 8) |
						(UINT32)cs4231.reg.playcount[1];
			playbytes = (basecount + 1) * bytespercount;
			cs4231.totalsample++;

			if ((UINT32)cs4231.totalsample >= playbytes) {
				cs4231.totalsample = 0;	/* reload current count */
				cs4231.reg.featurestatus |= PI;
				cs4231.intflag |= INt;
				if ((cs4231.reg.pinctrl & IEN) && (cs4231.dmairq != 0xff)) {
					pic_setirq(cs4231.dmairq);
				}
			}
		}
#if defined(SUPPORT_MULTITHREAD)
		cs4231cs_leave_criticalsection();
#endif
	}
}

// DMA再生開始・終了・中断時に呼ばれる（つもり）
REG8 DMACCALL cs4231dmafunc(REG8 func) {
	DMACH	dmach;
	SINT32	cnt;
	switch(func) {
		case DMAEXT_START:
			if (cs4231cfg.rate) {
#if defined(SUPPORT_MULTITHREAD)
				cs4231cs_enter_criticalsection();
#endif
				// DMA読み取り数カウンタを初期化
				cs4231.totalsample = 0;

				// ディレイ再生カウンタもリセット
				cs4231_bufdelaycounter = 0;

				// 再生位置も戻す
				cs4231.bufpos = cs4231.bufwpos;
				cs4231.bufdatas = 0;
				cs4231.pos12 = 0;

#if defined(SUPPORT_MULTITHREAD)
				cs4231cs_leave_criticalsection();
#endif
				// DMA読み取り処理開始(NEVENTセット)
				playcountsmp_Ictl = CS4231_BUFREADSMP;
				cnt = (pccore.realclock / cs4231cfg.rate * playcountsmp_Ictl) / 2;
				if (cnt < 1) cnt = 1;
				nevent_set(NEVENT_CS4231, cnt, cs4231_dma, NEVENT_ABSOLUTE);
			}
			break;
		case DMAEXT_END:
			// ここでの割り込みは要らない？
			//if ((cs4231.reg.pinctrl & IEN) && (cs4231.dmairq != 0xff)) {
			//	cs4231.intflag |= INt;
			//	cs4231.reg.featurestatus |= PI;
			//	pic_setirq(cs4231.dmairq);
			//}

			break;

		case DMAEXT_BREAK:
			// DMA読み取り処理終了(NEVENT解除)
			nevent_reset(NEVENT_CS4231);

			break;

	}
	return(0);
}

// バッファ位置のズレ修正用（雑音化防止）
static void setdataalign(void) {

	UINT	step;
	
	// バッファ位置がズレていたら修正（4byte単位に）
	step = (0 - cs4231.bufpos) & 3;
	if (step) {
		cs4231.bufpos += step;
		cs4231.bufdatas -= MIN(step, cs4231.bufdatas);
	}
	cs4231.bufdatas &= ~3;
	step = (0 - cs4231.bufwpos) & 3;
	if (step) {
		cs4231.bufwpos += step;
	}
}

// CS4231 Indexed Data registerのWRITE処理
void cs4231_control(UINT idx, REG8 dat) {
	UINT8	modify;
	DMACH	dmach;
	switch(idx){
	case 0x2: // Left Auxiliary #1 Input Control
	case 0x3: // Right Auxiliary #1 Input Control
		break;
	case 0xd:
		break;
	case 0xc:
		// MODE and ID (I12)
		dat &= 0x40;
		dat |= 0x8a;
		break;
	case 0xb://ErrorStatus 
	case 0x19://Version ID
		return;
	case CS4231REG_IRQSTAT:
		// バッファオーバーラン・アンダーランや割り込みの状態を表すレジスタ　Alternate Feature Status (I24)
		// 0をセットしたビットだけ消す（1の場合はそのまま）
		modify = ((UINT8 *)&cs4231.reg)[idx] & (~(dat|0x0f));
		((UINT8 *)&cs4231.reg)[idx] &= dat|0x0f;
		if (modify & (PI|TI|CI)) {
			// PI,TI,CIビットが全て消去されていたら割り込み解除
			if(((((UINT8 *)&cs4231.reg)[idx]) & (PI|TI|CI)) == 0){
				pic_resetirq(cs4231.dmairq);
				cs4231.intflag &= ~INt;
			}
		}
        return; // 他とは処理が違うので抜ける
	default:
		break;

	}
	dmach = dmac.dmach + cs4231.dmach;
	modify = ((UINT8 *)&cs4231.reg)[idx] ^ dat; // 変更されたビットを取得
	((UINT8 *)&cs4231.reg)[idx] = dat; // レジスタ値を新しい値に変更
	switch(idx) {
	case CS4231REG_PLAYFMT:
		// 再生フォーマット設定とか　Fs and Playback Data Format (I8)
		if (modify & 0xf0) {
			//dmach->adrs.d = dmach->startaddr;
			cs4231.bufpos = cs4231.bufwpos;
			cs4231.bufdatas = 0;
			setdataalign();
		}
		if (cs4231cfg.rate) {
			UINT32 r;
			r = cs4231xtal64[dat & 1] / cs4231cnt64[(dat >> 1) & 7];
			TRACEOUT(("samprate = %d", r));
			r <<= 12;
			r /= cs4231cfg.rate;
			cs4231.step12 = r;
			TRACEOUT(("step12 = %d", r));
		}
		else {
			cs4231.step12 = 0;
		}
		// DMA読み取り位置を戻す
		dmach->adrs.d = dmach->startaddr;
		break;
	case CS4231REG_INTERFACE:
		// 再生録音の有効無効とかDMAとかの設定　Interface Configuration (I9)
		if ((modify & (PEN | CEN)) && (dat & (PEN | CEN))) {
			/* Starting playback/capture reloads the codec-side sample counter. */
			cs4231.totalsample = 0;
		}
		if (modify & (PEN | CEN | PPIO | CPIO)) {
			if (cs4231.dmach != 0xff) {
				BOOL playback_dma;
				BOOL capture_dma;

				dmach = dmac.dmach + cs4231.dmach;
				playback_dma = ((dat & (PEN | PPIO)) == PEN);
				capture_dma = ((dat & (CEN | CPIO)) == CEN);

				dmach->ready = (playback_dma || capture_dma) ? 1 : 0;
				dmac_check();
			}	
			if (!(dat & PEN)) {		// playback stop
				cs4231.pos12 = 0;
			}
		}
		break;
	}
}

// CS4231 DMAデータ読み取り
UINT dmac_getdata_(DMACH dmach, UINT8 *buf, UINT offset, UINT size) {
	UINT	leng;						// 読み取り数
	UINT	lengsum;					// 合計読み取り数
	UINT	remain;						// DMA Terminal Countまでの残り転送数
	UINT	channel;
	REG8	channelbit;
	UINT32	addr;
	UINT	i;
	SINT32	sampleirq = 0;			// 割り込みまでに必要なデータ転送数(byte)
	BOOL	autoinit;

	lengsum = 0;
	channel = (UINT)(dmach - dmac.dmach);
	channelbit = (channel < 4) ? (REG8)(1 << channel) : 0;
	autoinit = (dmach->mode & 0x10) ? TRUE : FALSE;

	while(size > 0) {
		// Current Countは残り転送数-1
		if (!autoinit && channelbit &&
			(dmac.stat & channelbit) && dmach->leng.w == 0xffff) {
			break;
		}

		remain = (UINT)dmach->leng.w + 1;
		leng = min(remain, size);
		if (leng) {
			int playcount = ((cs4231.reg.playcount[1]|(cs4231.reg.playcount[0] << 8))) * cs4231_playcountshift[cs4231.reg.datafmt >> 4]+4; // PI割り込みを発生させるサンプル数(Playback Base register) * サンプルあたりのバイト数
			if(cs4231.totalsample + (SINT32)leng > playcount){
				// DMA再生サンプル数カウンタ(Playback DMA count register)がPI割り込みを発生させるサンプル数(Playback Base register)を超えないように調整
				leng = playcount - cs4231.totalsample;
			}
			if (!leng) {
				break;
			}

			addr = dmach->adrs.d; // 現在のメモリ読み取り位置
			if (!(dmach->mode & 0x20)) {			// dir +
				// +方向にDMA転送
				for (i=0; i<leng ; i++) {
					buf[offset] = MEMP_READ8(addr); // DMA MEM -> CS4231 BUFFER
					addr++;
					offset = (offset+1) & CS4231_BUFMASK;
				}
			}
			else {									// dir -
				// -方向にDMA転送
				for (i=0; i<leng; i++) {
					buf[offset] = MEMP_READ8(addr); // DMA MEM -> CS4231 BUFFER
					addr--;
					offset = (offset+1) & CS4231_BUFMASK;
				}
			}
			dmach->adrs.d = addr;

			if (leng >= remain) {
				dmach->leng.w = 0xffff;
				if (channelbit) {
					dmac.stat |= channelbit;		// Terminal Count status
				}
				dmach->proc.extproc(DMAEXT_END);


				if (autoinit) {
					// Auto-initialize: Base Address/Countを再ロードして継続
					dmach->adrs.d = dmach->startaddr;
					dmach->leng.w = dmach->startcount;
				}
				else if (channelbit) {
					// Auto-initialize無効ならTCでチャネルmask＆DMA停止
					dmac.mask |= channelbit;
					dmac_check();
				}
			}
			else {
				dmach->leng.w -= (UINT16)leng;
			}

			// 読み取り数と残り数更新
			lengsum += leng;
			size -= leng;

			// 読み取り数カウント
			cs4231.totalsample += leng;

			// DMA再生バイト数カウンタ(Playback DMA count register)がPI割り込みを発生させるバイト数になったらPI割り込みを発生させる
			if(cs4231.totalsample >= playcount){
				cs4231.totalsample -= playcount;
				// 割り込みが有効な場合割り込みを発生させる
				if ((cs4231.reg.pinctrl & IEN) && (cs4231.dmairq != 0xff)) {
					cs4231.intflag |= INt; // 割り込み中(Interrupt Status)ビットをセット
					cs4231.reg.featurestatus |= PI; // PI(Playback Interrupt)ビットをセット
					pic_setirq(cs4231.dmairq); // 割り込みを発生させる
				}
				break;
			}

			// Auto-initialize無効でTCなら終了
			if (!autoinit && dmach->leng.w == 0xffff && channelbit && (dmac.stat & channelbit)) {
				// 割り込みが有効な場合割り込みを発生させる
				if ((cs4231.reg.pinctrl & IEN) && (cs4231.dmairq != 0xff)) {
					cs4231.intflag |= INt; // 割り込み中(Interrupt Status)ビットをセット
					cs4231.reg.featurestatus |= PI; // PI(Playback Interrupt)ビットをセット
					pic_setirq(cs4231.dmairq); // 割り込みを発生させる
				}
				break;
			}
		}
		else{
			break;
		}
	}

	return(lengsum);
}


// 録音仮　とりあえず無音
static UINT dmac_putcapture_(DMACH dmach, UINT size) {
	UINT	leng;
	UINT	remain;
	UINT	channel;
	REG8	channelbit;
	UINT32	addr;
	UINT	i;
	REG8	silence;
	BOOL	autoinit;

	channel = (UINT)(dmach - dmac.dmach);
	channelbit = (channel < 4) ? (REG8)(1 << channel) : 0;
	autoinit = (dmach->mode & 0x10) ? TRUE : FALSE;

	if (!autoinit && channelbit &&
		(dmac.stat & channelbit) && dmach->leng.w == 0xffff) {
		return 0;
	}

	remain = (UINT)dmach->leng.w + 1;
	leng = min(remain, size);
	if (!leng) {
		return 0;
	}

	/* 8-bit linear PCM is unsigned.  Other formats are harmlessly zeroed. */
	silence = ((cs4231.reg.cap_datafmt & 0xe0) == 0x00) ? 0x80 : 0x00;
	addr = dmach->adrs.d;
	if (!(dmach->mode & 0x20)) {
		for (i = 0; i < leng; i++) {
			MEMP_WRITE8(addr, silence);
			addr++;
		}
	}
	else {
		for (i = 0; i < leng; i++) {
			MEMP_WRITE8(addr, silence);
			addr--;
		}
	}
	dmach->adrs.d = addr;

	if (leng >= remain) {
		dmach->leng.w = 0xffff;
		if (channelbit) {
			dmac.stat |= channelbit;
		}
		dmach->proc.extproc(DMAEXT_END);


		if (autoinit) {
			dmach->adrs.d = dmach->startaddr;
			dmach->leng.w = dmach->startcount;
		}
		else if (channelbit) {
			dmac.mask |= channelbit;
			dmac_check();
		}
	}
	else {
		dmach->leng.w -= (UINT16)leng;
	}

	return leng;
}

