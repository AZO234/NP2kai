/**
 * @file	ct1741io.c
 * @brief	Implementation of the Creative SoundBlaster16 CT1741 DSP I/O
 */

#ifdef SUPPORT_SOUND_SB16

#include	<compiler.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<cbus/ct1741io.h>
#include	<cbus/ct1745io.h>
#include	<sound/sound.h>
#include	<sound/fmboard.h>
#include	<cbus/mpu98ii.h>
#include	<cbus/smpu98.h>
#include	<sound/ct1741.h>

#if 0
#undef	TRACEOUT
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
#define	TRACEOUT(s)	trace_fmt_ex s
#endif	/* 1 */

#define CT1741_DSP_SAMPLES_DEFAULT	22050

// DSPコマンド
typedef enum {
	DSP_CMD_NONE = 0,
	DSP_CMD_GET_ASP_VERSION = 0x02,
	DSP_CMD_SET_ASP_MODE_REG = 0x04,
	DSP_CMD_SET_ASP_CODEC = 0x05,
	DSP_CMD_WRITE_ASP_REG = 0x0e,
	DSP_CMD_READ_ASP_REG = 0x0f,

	DSP_CMD_8BIT_PIO = 0x10,
	DSP_CMD_8BIT_DMA_SINGLE = 0x14,
	DSP_CMD_2BITADPCM_DMA_SINGLE = 0x16,
	DSP_CMD_2BITADPCM_DMA_SINGLE_WITH_REF = 0x17,
	DSP_CMD_8BIT_DMA_AUTOINIT = 0x1c,
	DSP_CMD_2BITADPCM_DMA_AUTOINIT_WITH_REF = 0x1f,

	DSP_CMD_8BIT_PIO_IN = 0x20,
	DSP_CMD_8BIT_DMA_SINGLE_IN = 0x24,

	DSP_CMD_POLLING_MIDI_IN = 0x30,
	DSP_CMD_INTERRUPT_MIDI_IN = 0x31,
	DSP_CMD_UART_POLLING_MIDI = 0x34,
	DSP_CMD_UART_INTERRUPT_MIDI = 0x35,
	DSP_CMD_UART_POLLING_MIDI_TIMESTAMP = 0x36,
	DSP_CMD_UART_INTERRUPT_MIDI_TIMESTAMP = 0x37,
	DSP_CMD_MIDI_OUT = 0x38,

	DSP_CMD_SET_DS_TRANSFER_TIMECONSTANT = 0x40,
	DSP_CMD_SET_DS_OUT_SAMPLINGRATE = 0x41,
	DSP_CMD_SET_DS_IN_SAMPLINGRATE = 0x42,
	DSP_CMD_SET_DSP_BLOCK_SIZE = 0x48,

	DSP_CMD_4BITADPCM_DMA_SINGLE = 0x74,
	DSP_CMD_4BITADPCM_DMA_SINGLE_WITH_REF = 0x75,
	DSP_CMD_3BITADPCM_DMA_SINGLE = 0x76,
	DSP_CMD_3BITADPCM_DMA_SINGLE_WITH_REF = 0x77,
	DSP_CMD_4BITADPCM_DMA_AUTOINIT_WITH_REF = 0x7d,
	DSP_CMD_3BITADPCM_DMA_AUTOINIT_WITH_REF = 0x7f,

	DSP_CMD_PAUSE_DAC_DURATION = 0x80,

	DSP_CMD_8BIT_HIGHSPEED_DMA_AUTOINIT = 0x90,
	DSP_CMD_8BIT_HIGHSPEED_DMA_SINGLE = 0x91,
	DSP_CMD_8BIT_HIGHSPEED_DMA_AUTOINIT_IN = 0x98,
	DSP_CMD_8BIT_HIGHSPEED_DMA_SINGLE_IN = 0x99,

	DSP_CMD_SET_MONO = 0xa0,
	DSP_CMD_SET_STEREO = 0xa8,

	DSP_CMD_PROGRAM_16BIT_DMA_BEGIN = 0xb0,
	DSP_CMD_PROGRAM_16BIT_DMA_END = 0xbf,

	DSP_CMD_PROGRAM_8BIT_DMA_BEGIN = 0xc0,
	DSP_CMD_PROGRAM_8BIT_DMA_END = 0xcf,

	DSP_CMD_PAUSE_8BIT_DMA = 0xd0,
	DSP_CMD_SPEAKER_ON = 0xd1,
	DSP_CMD_SPEAKER_OFF = 0xd3,
	DSP_CMD_CONTINUE_8BIT_DMA = 0xd4,
	DSP_CMD_PAUSE_16BIT_DMA = 0xd5,
	DSP_CMD_CONTINUE_16BIT_DMA = 0xd6,
	DSP_CMD_GET_SPEAKER_STATUS = 0xd8,
	DSP_CMD_EXIT_16BIT_DMA_AUTOINIT = 0xd9,
	DSP_CMD_EXIT_8BIT_DMA_AUTOINIT = 0xda,

	DSP_CMD_GET_DSP_ID = 0xe0,
	DSP_CMD_GET_DSP_VERSION = 0xe1,
	DSP_CMD_DSP_PROTECT = 0xe2,
	DSP_CMD_GET_CR = 0xe3,
	DSP_CMD_WRITE_TESTREG = 0xe4,
	DSP_CMD_READ_TESTREG = 0xe8,
	DSP_CMD_GEN_8BIT_IRQ = 0xf2,
	DSP_CMD_GEN_16BIT_IRQ = 0xf3,
} DSP_CMD;

// コマンド長さ（引数の数）テーブル
static UINT8 ct1741dsp_cmdlentbl[256] = { 0 };

// コマンド長さ（引数の数）の取得
static UINT8 ct1741dsp_getcmdlen(UINT8 cmd) {
	if (DSP_CMD_PROGRAM_16BIT_DMA_BEGIN <= cmd && cmd <= DSP_CMD_PROGRAM_16BIT_DMA_END) {
		return 3;
	}
	if (DSP_CMD_PROGRAM_8BIT_DMA_BEGIN <= cmd && cmd <= DSP_CMD_PROGRAM_8BIT_DMA_END) {
		return 3;
	}
	switch (cmd) {
	case DSP_CMD_GET_ASP_VERSION:
	case DSP_CMD_SET_ASP_MODE_REG:
	case DSP_CMD_READ_ASP_REG:
	case DSP_CMD_8BIT_PIO:
	case DSP_CMD_MIDI_OUT:
	case DSP_CMD_SET_DS_TRANSFER_TIMECONSTANT:
	case DSP_CMD_GET_DSP_ID:
	case DSP_CMD_DSP_PROTECT:
	case DSP_CMD_WRITE_TESTREG:
		return 1;
	case DSP_CMD_SET_ASP_CODEC:
	case DSP_CMD_WRITE_ASP_REG:
	case DSP_CMD_8BIT_DMA_SINGLE:
	case DSP_CMD_2BITADPCM_DMA_SINGLE:
	case DSP_CMD_2BITADPCM_DMA_SINGLE_WITH_REF:
	case DSP_CMD_8BIT_DMA_SINGLE_IN:
	case DSP_CMD_SET_DS_OUT_SAMPLINGRATE:
	case DSP_CMD_SET_DS_IN_SAMPLINGRATE:
	case DSP_CMD_SET_DSP_BLOCK_SIZE:
	case DSP_CMD_4BITADPCM_DMA_SINGLE:
	case DSP_CMD_4BITADPCM_DMA_SINGLE_WITH_REF:
	case DSP_CMD_3BITADPCM_DMA_SINGLE:
	case DSP_CMD_3BITADPCM_DMA_SINGLE_WITH_REF:
	case DSP_CMD_PAUSE_DAC_DURATION:
		return 2;
	default:
		return 0;
	}
}

// コマンド長さ（引数の数）テーブル作成
static void ct1741dsp_create_cmdlentbl() {
	int i;
	for (i = 0; i < NELEMENTS(ct1741dsp_cmdlentbl); i++) {
		ct1741dsp_cmdlentbl[i] = ct1741dsp_getcmdlen(i);
	}
}

// CPU→DSPのバッファをクリア
static void ct1741dsp_dspout_clear()
{
	g_sb16.dsp_info.dspout.datalen = 0;
}

// CPU→DSPへ引数の値をPush
static void ct1741dsp_dspout_push(UINT8 dat)
{
	if (g_sb16.dsp_info.dspout.datalen < CT1741_DSP_BUFSIZE) {
		g_sb16.dsp_info.dspout.data[(g_sb16.dsp_info.dspout.rpos + g_sb16.dsp_info.dspout.datalen) % CT1741_DSP_BUFSIZE] = dat;
		g_sb16.dsp_info.dspout.datalen++;
	}
}

// DSP→CPUへ返す値をPush
static void ct1741dsp_dspin_push(UINT8 dat)
{
	if (g_sb16.dsp_info.dspin.datalen < CT1741_DSP_BUFSIZE) {
		g_sb16.dsp_info.dspin.data[g_sb16.dsp_info.dspin.datalen] = dat;
		g_sb16.dsp_info.dspin.datalen++;
	}
}

// DMA転送開始する
static void ct1741dsp_setdma(CT1741_DMAMODE mode, BOOL autoinit, BOOL stereo, BOOL input)
{
	g_sb16.dsp_info.dma.dmach = dmac.dmach + g_sb16.dmachnum; // DMA割り当て
	if (g_sb16.dsp_info.mode != CT1741_DSPMODE_NONE)
	{
		// DMA開始後に設定を変えてくる場合があるので、その時は一旦止めて設定し直し
		g_sb16.dsp_info.dma.dmach->ready = 0;
		dmac_check();
	}
	g_sb16.dsp_info.mode = input ? CT1741_DSPMODE_DMA_IN : CT1741_DSPMODE_DMA;
	g_sb16.dsp_info.dma.lastautoinit = g_sb16.dsp_info.dma.autoinit = !!autoinit;
	g_sb16.dsp_info.dma.stereo = !!stereo;
	g_sb16.dsp_info.dma.mode = mode;

	// DMA開始
	g_sb16.dsp_info.dma.dmach->ready = 1;
	dmac_check();
}

// DMA転送を再設定する
static void ct1741dsp_updatedma()
{
	ct1741dsp_setdma(g_sb16.dsp_info.dma.mode, g_sb16.dsp_info.dma.autoinit, g_sb16.dsp_info.dma.stereo, g_sb16.dsp_info.mode == CT1741_DSPMODE_DMA_IN);
}

// DSPコマンドを実行
static void ct1741dsp_exec_command()
{
	UINT8 cmd = g_sb16.dsp_info.cmd;
	TRACEOUT(("CT1741 DSP CMD=0x%02x", cmd));
	if (DSP_CMD_PROGRAM_16BIT_DMA_BEGIN <= cmd && cmd <= DSP_CMD_PROGRAM_16BIT_DMA_END) {
		// Prigram 16bit DMA
		ct1741_playinfo.bufdatasrem = 0;
		g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[2] << 8) | g_sb16.dsp_info.dspin.data[1]) + 1;
		ct1741dsp_setdma(CT1741_DMAMODE_16, g_sb16.dsp_info.cmd & 0x4, g_sb16.dsp_info.dspin.data[0] & 0x20, g_sb16.dsp_info.cmd & 0x8);
		g_sb16.mixreg[0x82] |= (g_sb16.dsp_info.dmachnum & 0xe0) ? 0x2 : 0x1; // High-DMAが有効な場合はHigh-DMAを使用
		if (g_sb16.dsp_info.cmd & 0x8) {
			// 入力の場合、即座に割り込むことにする
			ct1741_setpicirq();
		}
	}
	else if (DSP_CMD_PROGRAM_8BIT_DMA_BEGIN <= cmd && cmd <= DSP_CMD_PROGRAM_8BIT_DMA_END) {
		// Prigram 8bit DMA
		ct1741_playinfo.bufdatasrem = 0;
		g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[2] << 8) | g_sb16.dsp_info.dspin.data[1]) + 1;
		ct1741dsp_setdma(CT1741_DMAMODE_8, g_sb16.dsp_info.cmd & 0x4, g_sb16.dsp_info.dspin.data[0] & 0x20, g_sb16.dsp_info.cmd & 0x8);
		g_sb16.mixreg[0x82] |= 0x1;
		if (g_sb16.dsp_info.cmd & 0x8) {
			// 入力の場合、即座に割り込むことにする
			ct1741_setpicirq();
		}
	}
	else {
		switch (g_sb16.dsp_info.cmd) {
		case DSP_CMD_GET_ASP_VERSION:
			break;
		case DSP_CMD_SET_ASP_MODE_REG:
			ct1741dsp_dspout_clear();
			ct1741dsp_dspout_push(0xff);
			break;
		case DSP_CMD_SET_ASP_CODEC:
			break;
		case DSP_CMD_WRITE_ASP_REG:
			break;
		case DSP_CMD_READ_ASP_REG:
			ct1741dsp_dspout_clear();
			ct1741dsp_dspout_push(0);
			break;

		case DSP_CMD_8BIT_PIO:
			g_sb16.dsp_info.mode = CT1741_DSPMODE_DAC;
			ct1741_playinfo.playwaitcounter = 0;
			// PIO再生のつもりだがノーチェック
#if defined(SUPPORT_MULTITHREAD)
			ct1741cs_enter_criticalsection();
#endif
			if (ct1741_playinfo.pio.bufdatas < CT1741_PIO_BUFSIZE) {
				ct1741_playinfo.pio.buffer[ct1741_playinfo.pio.bufwpos] = g_sb16.dsp_info.dspin.data[0];
				ct1741_playinfo.pio.bufwpos++;
				if (ct1741_playinfo.pio.bufwpos >= CT1741_PIO_BUFSIZE) ct1741_playinfo.pio.bufwpos -= CT1741_PIO_BUFSIZE;
				ct1741_playinfo.pio.bufdatas++;
			}
#if defined(SUPPORT_MULTITHREAD)
			ct1741cs_leave_criticalsection();
#endif
			if (ct1741_playinfo.pio.bufdatas > CT1741_PIO_BUFSIZE / 2) {
				if (ct1741_playinfo.pio.bufdatas > CT1741_PIO_BUFSIZE)
				{
					g_sb16.dsp_info.wbusy = 1;
				}
				else
				{
					g_sb16.dsp_info.wbusy = 0;
				}
			}
			else
			{
				g_sb16.dsp_info.wbusy = 0;
			}
			break;
		case DSP_CMD_8BIT_DMA_SINGLE:
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			ct1741_playinfo.bufdatasrem = 0;
			ct1741dsp_setdma(CT1741_DMAMODE_8, FALSE, FALSE, FALSE);
			break;
		case DSP_CMD_2BITADPCM_DMA_SINGLE:
		case DSP_CMD_2BITADPCM_DMA_SINGLE_WITH_REF:
			ct1741_playinfo.bufdatasrem = 0;
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			ct1741dsp_setdma(CT1741_DMAMODE_2, FALSE, FALSE, FALSE);
			break;
		case DSP_CMD_8BIT_DMA_AUTOINIT:
			ct1741_playinfo.bufdatasrem = 0;
			ct1741dsp_setdma(CT1741_DMAMODE_8, TRUE, FALSE, FALSE);
			break;
		case DSP_CMD_2BITADPCM_DMA_AUTOINIT_WITH_REF:
			ct1741_playinfo.bufdatasrem = 0;
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			ct1741dsp_setdma(CT1741_DMAMODE_2, TRUE, FALSE, FALSE);
			break;

		case DSP_CMD_8BIT_PIO_IN:
			// not implemented yet
			break;
		case DSP_CMD_8BIT_DMA_SINGLE_IN:
			ct1741_playinfo.bufdatasrem = 0;
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			ct1741dsp_setdma(CT1741_DMAMODE_8, FALSE, FALSE, TRUE);
			break;
		case DSP_CMD_POLLING_MIDI_IN:
		case DSP_CMD_INTERRUPT_MIDI_IN:
		case DSP_CMD_UART_POLLING_MIDI:
		case DSP_CMD_UART_INTERRUPT_MIDI:
		case DSP_CMD_UART_POLLING_MIDI_TIMESTAMP:
		case DSP_CMD_UART_INTERRUPT_MIDI_TIMESTAMP:
			// not implemented yet
			break;
		case DSP_CMD_MIDI_OUT:
			if (!g_sb16.dsp_info.uartmode) {
				if (mpu98.enable) {
					mpu98.mode = 1; // force set UART mode
					mpu98ii_o0(0x8000 + g_sb16.base, g_sb16.dsp_info.dspin.data[0]);
				}
				else if (smpu98.enable) {
					smpu98.mode = 1; // force set UART mode
					smpu98_o0(0x8000 + g_sb16.base, g_sb16.dsp_info.dspin.data[0]);
				}
			}
			break;

		case DSP_CMD_SET_DS_TRANSFER_TIMECONSTANT:
			g_sb16.dsp_info.freq = (1000000 / (256 - g_sb16.dsp_info.dspin.data[0]));
			if (g_sb16.dsp_info.dma.mode != CT1741_DMAMODE_NONE && g_sb16.dsp_info.dma.autoinit) {
				ct1741dsp_updatedma();
			}
			break;
		case DSP_CMD_SET_DS_OUT_SAMPLINGRATE:
		case DSP_CMD_SET_DS_IN_SAMPLINGRATE:
			g_sb16.dsp_info.freq = (g_sb16.dsp_info.dspin.data[0] << 8) | g_sb16.dsp_info.dspin.data[1];
			break;
		case DSP_CMD_SET_DSP_BLOCK_SIZE:
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			break;

		case DSP_CMD_4BITADPCM_DMA_SINGLE:
		case DSP_CMD_4BITADPCM_DMA_SINGLE_WITH_REF:
			ct1741_playinfo.bufdatasrem = 0;
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			ct1741dsp_setdma(CT1741_DMAMODE_4, FALSE, FALSE, FALSE);
			break;
		case DSP_CMD_3BITADPCM_DMA_SINGLE:
		case DSP_CMD_3BITADPCM_DMA_SINGLE_WITH_REF:
			ct1741_playinfo.bufdatasrem = 0;
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			ct1741dsp_setdma(CT1741_DMAMODE_3, FALSE, FALSE, FALSE);
			break;
		case DSP_CMD_4BITADPCM_DMA_AUTOINIT_WITH_REF:
			ct1741_playinfo.bufdatasrem = 0;
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			ct1741dsp_setdma(CT1741_DMAMODE_4, TRUE, FALSE, FALSE);
			break;
		case DSP_CMD_3BITADPCM_DMA_AUTOINIT_WITH_REF:
			ct1741_playinfo.bufdatasrem = 0;
			g_sb16.dsp_info.dma.total = ((g_sb16.dsp_info.dspin.data[1] << 8) | g_sb16.dsp_info.dspin.data[0]) + 1;
			ct1741dsp_setdma(CT1741_DMAMODE_3, TRUE, FALSE, FALSE);
			break;

		case DSP_CMD_PAUSE_DAC_DURATION:
			// not implemented yet
			break;

		case DSP_CMD_8BIT_HIGHSPEED_DMA_AUTOINIT:
			ct1741_playinfo.bufdatasrem = 0;
			ct1741dsp_setdma(CT1741_DMAMODE_8, TRUE, FALSE, FALSE);
			break;
		case DSP_CMD_8BIT_HIGHSPEED_DMA_SINGLE:
			ct1741_playinfo.bufdatasrem = 0;
			ct1741dsp_setdma(CT1741_DMAMODE_8, FALSE, FALSE, FALSE);
			break;
		case DSP_CMD_8BIT_HIGHSPEED_DMA_AUTOINIT_IN:
			ct1741_playinfo.bufdatasrem = 0;
			ct1741dsp_setdma(CT1741_DMAMODE_8, TRUE, FALSE, TRUE);
			break;
		case DSP_CMD_8BIT_HIGHSPEED_DMA_SINGLE_IN:
			ct1741_playinfo.bufdatasrem = 0;
			ct1741dsp_setdma(CT1741_DMAMODE_8, FALSE, FALSE, TRUE);
			break;

		case DSP_CMD_SET_MONO:
		case DSP_CMD_SET_STEREO:
			// not supported in DSP V4
			break;

		case DSP_CMD_PAUSE_8BIT_DMA:
		case DSP_CMD_PAUSE_16BIT_DMA:
			ct1741_playinfo.bufdatasrem = g_sb16.dsp_info.dma.bufdatas; // 停止した瞬間までに送られていた分は再生する
			g_sb16.dsp_info.mode = CT1741_DSPMODE_DMA_PAUSE;
			g_sb16.dsp_info.dma.mode = CT1741_DMAMODE_NONE;
			break;
		case DSP_CMD_SPEAKER_ON:
			g_sb16.dsp_info.speaker = 0xff;
			break;
		case DSP_CMD_SPEAKER_OFF:
			ct1741_playinfo.bufdatasrem = 0; // バッファ破棄
			g_sb16.dsp_info.speaker = 0x00;
			break;
		case DSP_CMD_CONTINUE_8BIT_DMA:
		case DSP_CMD_CONTINUE_16BIT_DMA:
			if (g_sb16.dsp_info.mode == CT1741_DSPMODE_DMA_PAUSE) {
				g_sb16.dsp_info.mode = CT1741_DSPMODE_DMA;
				ct1741_playinfo.bufdatasrem = 0;
			}
			break;
		case DSP_CMD_GET_SPEAKER_STATUS:
			ct1741dsp_dspout_clear();
			ct1741dsp_dspout_push(g_sb16.dsp_info.speaker);
			break;
		case DSP_CMD_EXIT_16BIT_DMA_AUTOINIT:
		case DSP_CMD_EXIT_8BIT_DMA_AUTOINIT:
		{
			// 停止した瞬間までに送られていた分は再生する
#if defined(SUPPORT_MULTITHREAD)
			ct1741cs_enter_criticalsection();
#endif
			if (g_sb16.dsp_info.dma.bufdatas > g_sb16.dsp_info.dma.total || g_sb16.dsp_info.dma.total == 65536) {
				// WORKAROUND: バッファを捨てて停止する。この処理をするとWin3.1で後ろが切れる。しないとWin9x DirectSoundで末尾にノイズ（ダブり）が入る
				g_sb16.dsp_info.dma.bufdatas = 0;
				g_sb16.dsp_info.dma.dmach->ready = 0;
			}
			g_sb16.dsp_info.dma.autoinit = FALSE; // 本来は次の割り込みタイミングで停止らしい
#if defined(SUPPORT_MULTITHREAD)
			ct1741cs_leave_criticalsection();
#endif
			break;
		}

		case DSP_CMD_GET_DSP_ID:
			ct1741dsp_dspout_clear();
			ct1741dsp_dspout_push(~g_sb16.dsp_info.dspin.data[0]);
			break;
		case DSP_CMD_GET_DSP_VERSION:
			ct1741dsp_dspout_clear();
			ct1741dsp_dspout_push(0x4);
			ct1741dsp_dspout_push(0xc);
			break;
		case DSP_CMD_DSP_PROTECT:
			// not implemented yet
			break;
		case DSP_CMD_GET_CR:
			{
				UINT32 i;
				char crstr[] = "NOT COPYRIGHT (C) CREATIVE TECHNOLOGY LTD, 1992.";
				ct1741dsp_dspout_clear();
				for (i = 4; i < sizeof(crstr); i++) {
					ct1741dsp_dspout_push(crstr[i]);
				}
			}
			break;
		case DSP_CMD_WRITE_TESTREG:
			g_sb16.dsp_info.testreg = g_sb16.dsp_info.dspin.data[0];
			break;
		case DSP_CMD_READ_TESTREG:
			ct1741dsp_dspout_clear();
			ct1741dsp_dspout_push(g_sb16.dsp_info.testreg);;
			break;
		case DSP_CMD_GEN_8BIT_IRQ:
			ct1741dsp_dspout_clear();
			ct1741dsp_dspout_push(0xaa);
			ct1741_setpicirq();
			g_sb16.mixreg[0x82] |= 1;
			break;
		case DSP_CMD_GEN_16BIT_IRQ:
			ct1741dsp_dspout_clear();
			ct1741dsp_dspout_push(0xaa);
			ct1741_setpicirq();
			g_sb16.mixreg[0x82] |= 2;
			break;
		default:
			break;
		}
	}
	g_sb16.dsp_info.dspin.datalen = 0;
	g_sb16.dsp_info.cmd = DSP_CMD_NONE;
	g_sb16.dsp_info.cmdlen = 0;
}

// DSPをリセット
static void ct1741dsp_reset(BOOL ioreset)
{
	g_sb16.dsp_info.mode = CT1741_DSPMODE_NONE;
	g_sb16.dsp_info.freq = CT1741_DSP_SAMPLES_DEFAULT;
	g_sb16.dsp_info.cmdlen = 0;
	g_sb16.dsp_info.dspin.datalen = 0;
	g_sb16.dsp_info.wbusy = 0;
	g_sb16.dsp_info.uartmode = 0;
	g_sb16.dsp_info.dma.autoinit = FALSE;
	g_sb16.dsp_info.dma.lastautoinit = FALSE;
	if (ioreset) {
		if (g_sb16.dsp_info.dma.dmach) {
			g_sb16.dsp_info.dma.dmach->ready = 0;
			dmac_check();
			if (g_sb16.dmachnum != 0xff) {
				dmac.stat |= (1 << g_sb16.dmachnum);
			}
			g_sb16.mixreg[0x82] &= ~3;
			ct1741_resetpicirq();
		}
	}
}

// ***** DSP I/O Ports *****

/* DSP reset */
static void IOOUTCALL ct1741dsp_write_reset(UINT port, REG8 dat)
{
	TRACEOUT(("CT1741 DSP Write Reset"));
	if ((dat & 0x01)) {
		/* status reset */
		ct1741dsp_reset(TRUE);
		g_sb16.dsp_info.resetout = CT1741_DSPRST_RESET;
	}
	else {
		/* status normal */
		ct1741dsp_dspout_clear();
		ct1741dsp_dspout_push(0xaa);
		g_sb16.dsp_info.resetout = CT1741_DSPRST_NORMAL;
	}

	if (dat == 0xc6) {
		ct1741dsp_dspout_clear();
		ct1741dsp_reset(TRUE);
		g_sb16.dsp_info.resetout = CT1741_DSPRST_SPECIAL;
	}
}

/* DSP write Command/Data */
static void IOOUTCALL ct1741dsp_write_data(UINT port, REG8 dat)
{
	TRACEOUT(("CT1741 DSP command: %.2x", dat));
	if (g_sb16.dsp_info.cmd == DSP_CMD_NONE) {
		// コマンド書き込み
		g_sb16.dsp_info.dspin.datalen = 0;
		g_sb16.dsp_info.cmd = g_sb16.dsp_info.cmd_o = dat;
		g_sb16.dsp_info.cmdlen = ct1741dsp_cmdlentbl[dat];
	}
	else {
		// パラメータ書き込み
		ct1741dsp_dspin_push(dat);
	}
	if (g_sb16.dsp_info.dspin.datalen >= g_sb16.dsp_info.cmdlen) {
		// パラメータが規定数に達したら実行
		ct1741dsp_exec_command();
	}
}

/* DSP reset read */
static REG8 IOINPCALL ct1741dsp_read_reset(UINT port)
{
	if ((port & 0x0d00) == 0x0d00)
		return 0x00; // アドレスが xDxxhの時は0x00を返す
	else 
		return 0xff; // アドレスが上記以外の時は0xffを返す
}

/* DSP read data */
static REG8 IOINPCALL ct1741dsp_read_data(UINT port)
{
	static REG8 data = 0;
	if (g_sb16.dsp_info.dspout.datalen) {
		if (g_sb16.dsp_info.dspout.rpos < sizeof(g_sb16.dsp_info.dspout.data) / sizeof(g_sb16.dsp_info.dspout.data[0])) {
			data = g_sb16.dsp_info.dspout.data[g_sb16.dsp_info.dspout.rpos];
			g_sb16.dsp_info.dspout.rpos++;
			if (g_sb16.dsp_info.dspout.rpos >= CT1741_DSP_BUFSIZE)
				g_sb16.dsp_info.dspout.rpos -= CT1741_DSP_BUFSIZE;
			g_sb16.dsp_info.dspout.datalen--;
		}
	}
	return data;
}

/* DSP read write status */
static REG8 IOINPCALL ct1741dsp_read_wstatus(UINT port)
{
	switch (g_sb16.dsp_info.resetout) {
	case CT1741_DSPRST_NORMAL:
		return g_sb16.dsp_info.wbusy ? 0x80 : 0x00;

	case CT1741_DSPRST_RESET:
		return 0xff;

	case CT1741_DSPRST_SPECIAL:
		return 0;

	default: // ????変な数値とるときがある　意味不明
		return 0;
	}
	return 0xff;
}

/* DSP read read status (8 bit) */
static REG8 IOINPCALL ct1741dsp_read_rstatus(UINT port)
{
	TRACEOUT(("CT1741 DSP read read status"));
	// 詳細不明
	if (g_sb16.dsp_info.cmd_o == 0xf2) {
		g_sb16.dsp_info.cmd_o = 0;
		return 0;
	}
	// 割り込みを解除。bit0は8-bit用
	if (g_sb16.mixreg[0x82] & 1) {
		g_sb16.mixreg[0x82] &= ~1;
		ct1741_resetpicirq();
	}

	if (g_sb16.dsp_info.dspout.datalen)
		return 0x80;
	else
		return 0x00;
}

/* DSP read read status (16 bit) */
static REG8 IOINPCALL ct1741dsp_read_rstatus16(UINT port)
{
	TRACEOUT(("CT1741 DSP read read status16"));
	// 割り込みを解除。bit1は16-bit用
	if (g_sb16.mixreg[0x82] & 2) { 
		g_sb16.mixreg[0x82] &= ~2;
		ct1741_resetpicirq();
	}

	return 0xff;
}

void ct1741io_reset(void)
{
	ct1741dsp_create_cmdlentbl();
	ct1741dsp_reset(FALSE);
	g_sb16.dsp_info.dmairq = ct1741_get_dma_irq();
	g_sb16.dsp_info.dmachnum = ct1741_get_dma_ch() & 0xf; // High DMAはDefaultで無効
	g_sb16.dsp_info.resetout = CT1741_DSPRST_NORMAL;
	if (g_sb16.dmachnum == 0 || g_sb16.dmachnum == 3) {
		g_sb16.dsp_info.dma.dmach = dmac.dmach + g_sb16.dmachnum; // DMA割り当て
		dmac_attach(DMADEV_CT1741, g_sb16.dmachnum);
	}
	g_sb16.dsp_info.dma.bufsize = CT1741_DMA_BUFSIZE;
	g_sb16.dsp_info.dma.rate2 = ct1741_playinfo.playrate;
	g_sb16.dsp_info.dma.bufdatas = 0;
	ct1741_playinfo.playwaitcounter = 0;
	ct1741_playinfo.bufdatasrem = 0;
}

void ct1741io_bind(void)
{
	sound_streamregist(&g_sb16.dsp_info.dma, (SOUNDCB)ct1741_getpcm); // CT1741用 オーディオ再生ストリーム

	iocore_attachout(0x2600 + g_sb16.base, ct1741dsp_write_reset);	/* DSP Reset */
	iocore_attachout(0x2C00 + g_sb16.base, ct1741dsp_write_data);	/* DSP Write Command/Data */

	iocore_attachinp(0x2600 + g_sb16.base, ct1741dsp_read_reset);	/* DSP Reset */
	iocore_attachinp(0x2a00 + g_sb16.base, ct1741dsp_read_data);		/* DSP Read Data Port */
	iocore_attachinp(0x2c00 + g_sb16.base, ct1741dsp_read_wstatus);	/* DSP Write Buffer Status (Bit 7) */
	iocore_attachinp(0x2d00 + g_sb16.base, ct1741dsp_read_reset);	/* DSP Reset */
	iocore_attachinp(0x2e00 + g_sb16.base, ct1741dsp_read_rstatus);	/* DSP Read Buffer Status (Bit 7) */
	iocore_attachinp(0x2f00 + g_sb16.base, ct1741dsp_read_rstatus16);	/* DSP Read Buffer Status (Bit 7) */
	
	// Canopus PowerWindow T64S/98 音源部テスト
	iocore_attachout(0x6600 + g_sb16.base, ct1741dsp_write_reset);	/* DSP Reset */
	iocore_attachout(0x6C00 + g_sb16.base, ct1741dsp_write_data);	/* DSP Write Command/Data */

	iocore_attachinp(0x6600 + g_sb16.base, ct1741dsp_read_reset);	/* DSP Reset */
	iocore_attachinp(0x6a00 + g_sb16.base, ct1741dsp_read_data);		/* DSP Read Data Port */
	iocore_attachinp(0x6c00 + g_sb16.base, ct1741dsp_read_wstatus);	/* DSP Write Buffer Status (Bit 7) */
	iocore_attachinp(0x6d00 + g_sb16.base, ct1741dsp_read_reset);	/* DSP Reset */
	iocore_attachinp(0x6e00 + g_sb16.base, ct1741dsp_read_rstatus);	/* DSP Read Buffer Status (Bit 7) */
	iocore_attachinp(0x6f00 + g_sb16.base, ct1741dsp_read_rstatus16);	/* DSP Read Buffer Status (Bit 7) */
	
	// PC/AT互換機テスト
	if(np2cfg.sndsb16at){
		iocore_attachout(0x226, ct1741dsp_write_reset);	/* DSP Reset */
		iocore_attachout(0x22C, ct1741dsp_write_data);		/* DSP Write Command/Data */

		iocore_attachinp(0x226, ct1741dsp_read_reset);		/* DSP Reset */
		iocore_attachinp(0x22a, ct1741dsp_read_data);		/* DSP Read Data Port */
		iocore_attachinp(0x22c, ct1741dsp_read_wstatus);	/* DSP Write Buffer Status (Bit 7) */
		iocore_attachinp(0x22d, ct1741dsp_read_reset);		/* DSP Reset */
		iocore_attachinp(0x22e, ct1741dsp_read_rstatus);	/* DSP Read Buffer Status (Bit 7) */
		iocore_attachinp(0x22f, ct1741dsp_read_rstatus16);	/* DSP Read Buffer Status (Bit 7) */
	}
}
void ct1741io_unbind(void)
{
	iocore_detachout(0x2600 + g_sb16.base);	/* DSP Reset */
	iocore_detachout(0x2C00 + g_sb16.base);	/* DSP Write Command/Data */

	iocore_detachinp(0x2600 + g_sb16.base);	/* DSP Reset */
	iocore_detachinp(0x2a00 + g_sb16.base);		/* DSP Read Data Port */
	iocore_detachinp(0x2c00 + g_sb16.base);	/* DSP Write Buffer Status (Bit 7) */
	iocore_detachinp(0x2e00 + g_sb16.base);	/* DSP Read Buffer Status (Bit 7) */
	iocore_detachinp(0x2f00 + g_sb16.base);	/* DSP Read Buffer Status (Bit 7) */

	// Canopus PowerWindow T64S/98 音源部テスト
	iocore_detachinp(0x6600 + g_sb16.base);	/* DSP Reset */
	iocore_detachinp(0x6C00 + g_sb16.base);	/* DSP Write Command/Data */

	iocore_detachinp(0x6600 + g_sb16.base);	/* DSP Reset */
	iocore_detachinp(0x6a00 + g_sb16.base);	/* DSP Read Data Port */
	iocore_detachinp(0x6c00 + g_sb16.base);	/* DSP Write Buffer Status (Bit 7) */
	iocore_detachinp(0x6d00 + g_sb16.base);	/* DSP Reset */
	iocore_detachinp(0x6e00 + g_sb16.base);	/* DSP Read Buffer Status (Bit 7) */
	iocore_detachinp(0x6f00 + g_sb16.base);	/* DSP Read Buffer Status (Bit 7) */

	// PC/AT互換機テスト
	if(np2cfg.sndsb16at){
		iocore_detachout(0x226);	/* DSP Reset */
		iocore_detachout(0x22C);	/* DSP Write Command/Data */

		iocore_detachinp(0x226);	/* DSP Reset */
		iocore_detachinp(0x22a);	/* DSP Read Data Port */
		iocore_detachinp(0x22c);	/* DSP Write Buffer Status (Bit 7) */
		iocore_detachinp(0x22d);	/* DSP Reset */
		iocore_detachinp(0x22e);	/* DSP Read Buffer Status (Bit 7) */
		iocore_detachinp(0x22f);	/* DSP Read Buffer Status (Bit 7) */
	}
}

#endif