#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"ct1741io.h"
#include	"sound.h"
#include	"fmboard.h"

#ifdef SUPPORT_SOUND_SB16

/**
 * Creative SoundBlaster16 DSP CT1741
 *
 */

enum DSP_STATUS {
	DSP_STATUS_NORMAL,
	DSP_STATUS_RESET
};
typedef enum {
	DSP_DMA_NONE,
	DSP_DMA_2,DSP_DMA_3,DSP_DMA_4,DSP_DMA_8,
	DSP_DMA_16,DSP_DMA_16_ALIASED,
} DMA_MODES;
typedef enum {
	DSP_MODE_NONE,
	DSP_MODE_DAC,
	DSP_MODE_DMA,
	DSP_MODE_DMA_PAUSE,
	DSP_MODE_DMA_MASKED
} DSP_MODES;

#define DSP_NO_COMMAND 0
#define SB_SH	14

#define DMA_BUFSIZE 1024
#define DSP_BUFSIZE 64

typedef struct {
	BOOL stereo,sign,autoinit;
	DMA_MODES mode;
	UINT32 rate,mul;
	UINT32 total,left,min;
//	unsigned __int64 start;
	union {
		UINT8  b8[DMA_BUFSIZE];
		SINT16 b16[DMA_BUFSIZE];
	} buf;
//	UINT32 bits;
	DMACH	chan;
	UINT32 remain_size;
} DMA_INFO;

typedef struct {
	DMA_INFO dma;
	UINT8 state;
	UINT8 cmd;
	UINT8 cmd_len;
	UINT8 cmd_in_pos;
	UINT8 cmd_in[DSP_BUFSIZE];
	struct {
		UINT8 data[DSP_BUFSIZE];
		UINT32 pos,used;
	} in,out;
	UINT8 test_register;
	UINT32 write_busy;
	DSP_MODES mode;
	UINT32 freq;
	UINT8 dmairq;
	UINT8 dmach;
} DSP_INFO;

static DSP_INFO dsp_info;

static char * const copyright_string = "COPYRIGHT (C) CREATIVE TECHNOLOGY LTD, 1992.";

static const UINT8 ct1741_cmd_len[256] = {
  0,0,0,0, 0,2,0,0, 0,0,0,0, 0,0,0,0,  // 0x00
  1,0,0,0, 2,0,2,2, 0,0,0,0, 0,0,0,0,  // 0x10
  0,0,0,0, 2,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x20
  0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0,  // 0x30

  1,2,2,0, 0,0,0,0, 2,0,0,0, 0,0,0,0,  // 0x40
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x50
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x60
  0,0,0,0, 2,2,2,2, 0,0,0,0, 0,0,0,0,  // 0x70

  2,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x80
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x90
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0xa0
  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0xb0

  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0xc0
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0xd0
  1,0,1,0, 1,0,0,0, 0,0,0,0, 0,0,0,0,  // 0xe0
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0   // 0xf0
};

void ct1741_set_dma_irq(UINT8 irq) {
	dsp_info.dmairq = irq;
	switch(irq) {
		case 1:
			g_sb16.dmairq = 3;
			break;
		case 8:
			g_sb16.dmairq = 5;
			break;
		case 2:
			g_sb16.dmairq = 10;
			break;
		case 4:
			g_sb16.dmairq = 12;
			break;
	}
}

UINT8 ct1741_get_dma_irq() {
	return dsp_info.dmairq;
}

void ct1741_set_dma_ch(UINT8 dmach) {
	dsp_info.dmach = dmach;
	if (dmach & 0x01) g_sb16.dmach = 0;
	if (dmach & 0x02) g_sb16.dmach = 3;
}

UINT8 ct1741_get_dma_ch() {
	return dsp_info.dmach;
}

static void ct1741_change_mode(DSP_MODES mode) {
	if (dsp_info.mode == mode) return;
//	else dsp_info.chan->FillUp();
	dsp_info.mode = mode;
}

static void ct1741_reset(void)
{
	ct1741_change_mode(DSP_MODE_NONE);
	dsp_info.freq=22050;
	dsp_info.cmd_len=0;
	dsp_info.in.pos=0;
	dsp_info.write_busy = 0;
}

static void ct1741_flush_data(void)
{
	dsp_info.out.used=0;
	dsp_info.out.pos=0;
}

static void ct1741_dma_transfer(DMA_MODES mode, UINT32 freq, BOOL stereo) {
	dsp_info.mode = DSP_MODE_DMA_MASKED;
//	dsp_info.chan->FillUp();
	dsp_info.dma.left = dsp_info.dma.total;
	dsp_info.dma.mode = mode;
	dsp_info.dma.stereo = stereo;
//	sb.irq.pending_8bit = false;
//	sb.irq.pending_16bit = false;
	switch (mode) {
	case DSP_DMA_2:
		// "2-bits ADPCM";
		dsp_info.dma.mul=(1 << SB_SH)/4;
		break;
	case DSP_DMA_3:
		// "3-bits ADPCM";
		dsp_info.dma.mul=(1 << SB_SH)/3;
		break;
	case DSP_DMA_4:
		// "4-bits ADPCM";
		dsp_info.dma.mul=(1 << SB_SH)/2;
		break;
	case DSP_DMA_8:
		// "8-bits PCM";
		dsp_info.dma.mul=(1 << SB_SH);
		break;
	case DSP_DMA_16_ALIASED:
		//type="16-bits(aliased) PCM";
		dsp_info.dma.mul=(1 << SB_SH)*2;
		break;
	case DSP_DMA_16:
		// type="16-bits PCM";
		dsp_info.dma.mul=(1 << SB_SH);
		break;
	default:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Illegal transfer mode %d",mode);
		return;
	}
	if (dsp_info.dma.stereo) dsp_info.dma.mul *= 2;
	dsp_info.dma.rate = (dsp_info.freq * dsp_info.dma.mul) >> SB_SH;
	dsp_info.dma.min = (dsp_info.dma.rate * 3) / 1000;
//	dsp_info.chan->SetFreq(freq);
	dsp_info.dma.mode = mode;
//	PIC_RemoveEvents(END_DMA_Event);
//	dsp_info.dma.chan->Register_Callback(DSP_DMA_CallBack);
	dsp_info.dma.chan->ready = 1;
	dmac_check();
}

static void ct1741_prepare_dma_old(DMA_MODES mode, BOOL autoinit) {
	dsp_info.dma.autoinit = autoinit;
	if (!autoinit)
		dsp_info.dma.total = 1 + dsp_info.in.data[0] + (dsp_info.in.data[1] << 8);
	dsp_info.dma.chan = dmac.dmach + g_sb16.dmach;	// 8bit dma irq
	ct1741_dma_transfer(mode, dsp_info.freq / 1, FALSE);
//	ct1741_dma_transfer(mode, dsp_info.freq / (sb.mixer.stereo ? 2 : 1), sb.mixer.stereo);
}

static void ct1741_prepare_dma(DMA_MODES mode, UINT32 length, BOOL autoinit, BOOL stereo) {
	UINT32 freq = dsp_info.freq;
	dsp_info.dma.total = length;
	dsp_info.dma.autoinit = autoinit;
	if (mode==DSP_DMA_16) {
		if (g_sb16.dmairq != 0xff) {
			dsp_info.dma.chan = dmac.dmach + g_sb16.dmach;
//			dsp_info.dma.chan = GetDMAChannel(sb.hw.dma16);
		} else {
			dsp_info.dma.chan = dmac.dmach + g_sb16.dmach;
//			dsp_info.dma.chan = GetDMAChannel(sb.hw.dma8);
			mode = DSP_DMA_16_ALIASED;
			freq /= 2;
		}
	} else {
		dsp_info.dma.chan = dmac.dmach + g_sb16.dmach;
//		sb.dma.chan = GetDMAChannel(sb.hw.dma8);
	}
	ct1741_dma_transfer(mode, freq, stereo);
}

static void ct1741_add_data(UINT8 dat)
{
	if (dsp_info.out.used < DSP_BUFSIZE) {
		UINT32 start = dsp_info.out.used + dsp_info.out.pos;
		if (start >= DSP_BUFSIZE)
			start -= DSP_BUFSIZE;
		dsp_info.out.data[start] = dat;
		dsp_info.out.used++;
	}
}

static void ct1741_exec_command()
{
	switch (dsp_info.cmd) {
	case 0x04:	/* DSP Status SB 2.0/pro version */
		ct1741_flush_data();
		ct1741_add_data(0xff);			//Everthing enabled
		break;
	case 0x10:	/* Direct DAC */
		ct1741_change_mode(DSP_MODE_DAC);
//		if (sb.dac.used<DSP_DACSIZE) {
//			sb.dac.data[sb.dac.used++]=(Bit8s(sb.dsp.in.data[0] ^ 0x80)) << 8;
//			sb.dac.data[sb.dac.used++]=(Bit8s(sb.dsp.in.data[0] ^ 0x80)) << 8;
//		}
		break;
	case 0x24:	/* Singe Cycle 8-Bit DMA ADC */
		dsp_info.dma.left = dsp_info.dma.total= 1 + dsp_info.in.data[0] + (dsp_info.in.data[1] << 8);
//		LOG(LOG_SB,LOG_ERROR)("DSP:Faked ADC for %d bytes",sb.dma.total);
//		GetDMAChannel(sb.hw.dma8)->Register_Callback(DSP_ADC_CallBack);
		break;
	case 0x14:	/* Singe Cycle 8-Bit DMA DAC */
	case 0x91:	/* Singe Cycle 8-Bit DMA High speed DAC */
		/* Note: 0x91 is documented only for DSP ver.2.x and 3.x, not 4.x */
		ct1741_prepare_dma_old(DSP_DMA_8, FALSE);
		break;
	case 0x90:	/* Auto Init 8-bit DMA High Speed */
	case 0x1c:	/* Auto Init 8-bit DMA */
		ct1741_prepare_dma_old(DSP_DMA_8, TRUE);
		break;
	case 0x38:  /* Write to SB MIDI Output */
//		if (sb.midi == true) MIDI_RawOutByte(dsp_info.in.data[0]);
		break;
	case 0x40:	/* Set Timeconstant */
		dsp_info.freq=(1000000 / (256 - dsp_info.in.data[0]));
		/* Nasty kind of hack to allow runtime changing of frequency */
		if (dsp_info.dma.mode != DSP_DMA_NONE && dsp_info.dma.autoinit) {
			ct1741_prepare_dma_old(dsp_info.dma.mode, dsp_info.dma.autoinit);
		}
		break;
	case 0x41:	/* Set Output Samplerate */
	case 0x42:	/* Set Input Samplerate */
		dsp_info.freq=(dsp_info.in.data[0] << 8)  | dsp_info.in.data[1];
		break;
	case 0x48:	/* Set DMA Block Size */
		//TODO Maybe check limit for new irq?
		dsp_info.dma.total= 1 + dsp_info.in.data[0] + (dsp_info.in.data[1] << 8);
		break;
	case 0x75:	/* 075h : Single Cycle 4-bit ADPCM Reference */
//		sb.adpcm.haveref = true;
	case 0x74:	/* 074h : Single Cycle 4-bit ADPCM */	
		ct1741_prepare_dma_old(DSP_DMA_4, FALSE);
		break;
	case 0x77:	/* 077h : Single Cycle 3-bit(2.6bit) ADPCM Reference*/
//		sb.adpcm.haveref=true;
	case 0x76:  /* 074h : Single Cycle 3-bit(2.6bit) ADPCM */
		ct1741_prepare_dma_old(DSP_DMA_3, FALSE);
		break;
	case 0x17:	/* 017h : Single Cycle 2-bit ADPCM Reference*/
//		sb.adpcm.haveref=true;
	case 0x16:  /* 074h : Single Cycle 2-bit ADPCM */
		ct1741_prepare_dma_old(DSP_DMA_2, FALSE);
		break;
	case 0x80:	/* Silence DAC */
//		PIC_AddEvent(&DSP_RaiseIRQEvent,
//			(1000.0f*(1 + dsp_info.in.data[0] + (dsp_info.in.data[1] << 8))/sb.freq));
		break;
	case 0xb0:	case 0xb1:	case 0xb2:	case 0xb3:  case 0xb4:	case 0xb5:	case 0xb6:	case 0xb7:
	case 0xb8:	case 0xb9:	case 0xba:	case 0xbb:  case 0xbc:	case 0xbd:	case 0xbe:	case 0xbf:
	case 0xc0:	case 0xc1:	case 0xc2:	case 0xc3:  case 0xc4:	case 0xc5:	case 0xc6:	case 0xc7:
	case 0xc8:	case 0xc9:	case 0xca:	case 0xcb:  case 0xcc:	case 0xcd:	case 0xce:	case 0xcf:
		/* Generic 8/16 bit DMA */
		dsp_info.dma.sign=(dsp_info.in.data[0] & 0x10) > 0;
		ct1741_prepare_dma((dsp_info.cmd & 0x10) ? DSP_DMA_16 : DSP_DMA_8,
			1 + dsp_info.in.data[1] + (dsp_info.in.data[2] << 8),
			(dsp_info.cmd & 0x4)>0,
			(dsp_info.in.data[0] & 0x20) > 0
		);
		break;
	case 0xd5:	/* Halt 16-bit DMA */
	case 0xd0:	/* Halt 8-bit DMA */
		dsp_info.mode = DSP_MODE_DMA_PAUSE;
//		PIC_RemoveEvents(END_DMA_Event);
		break;
	case 0xd1:	/* Enable Speaker */
//		DSP_SetSpeaker(true);
		break;
	case 0xd3:	/* Disable Speaker */
//		DSP_SetSpeaker(false);
		break;
	case 0xd8:  /* Speaker status */
		ct1741_flush_data();
//		if (sb.speaker)
			ct1741_add_data(0xff);
//		else
//			ct1741_add_data(0x00);
		break;
	case 0xd6:	/* Continue DMA 16-bit */
	case 0xd4:	/* Continue DMA 8-bit*/
		if (dsp_info.mode == DSP_MODE_DMA_PAUSE) {
			dsp_info.mode = DSP_MODE_DMA_MASKED;
//			sb.dma.chan->Register_Callback(DSP_DMA_CallBack);
		}
		break;
	case 0xd9:  /* Exit Autoinitialize 16-bit */
	case 0xda:	/* Exit Autoinitialize 8-bit */
		/* Set mode to single transfer so it ends with current block */
		dsp_info.dma.autoinit = FALSE;		//Should stop itself
		break;
	case 0xe0:	/* DSP Identification - SB2.0+ */
		ct1741_flush_data();
		ct1741_add_data(~dsp_info.in.data[0]);
		break;
	case 0xe1:	/* Get DSP Version */
		ct1741_flush_data();
		ct1741_add_data(0x4);
		ct1741_add_data(0x5);
		break;
	case 0xe2:	/* Weird DMA identification write routine */
		{
//			LOG(LOG_SB,LOG_NORMAL)("DSP Function 0xe2");
//			for (Bitu i = 0; i < 8; i++)
//				if ((sb.dsp.in.data[0] >> i) & 0x01) sb.e2.value += E2_incr_table[sb.e2.count % 4][i];
//			 sb.e2.value += E2_incr_table[sb.e2.count % 4][8];
//			 sb.e2.count++;
//			 GetDMAChannel(sb.hw.dma8)->Register_Callback(DSP_E2_DMA_CallBack);
		}
		break;
	case 0xe3:	/* DSP Copyright */
		{
			UINT32 i;
			ct1741_flush_data();
			for (i=0; i <= strlen(copyright_string); i++) {
				ct1741_add_data(copyright_string[i]);
			}
		}
		break;
	case 0xe4:	/* Write Test Register */
		dsp_info.test_register = dsp_info.in.data[0];
		break;
	case 0xe8:	/* Read Test Register */
		ct1741_flush_data();
		ct1741_add_data(dsp_info.test_register);;
		break;
	case 0xf2:	/* Trigger 8bit IRQ */
//		SB_RaiseIRQ(SB_IRQ_8);
		break;
	case 0x30: case 0x31:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unimplemented MIDI I/O command %2X",sb.dsp.cmd);
		break;
	case 0x34: case 0x35: case 0x36: case 0x37:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unimplemented MIDI UART command %2X",sb.dsp.cmd);
		break;
	case 0x7d: case 0x7f: case 0x1f:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unimplemented auto-init DMA ADPCM command %2X",sb.dsp.cmd);
		break;
	case 0x20:
	case 0x2c:
	case 0x98: case 0x99: /* Documented only for DSP 2.x and 3.x */
	case 0xa0: case 0xa8: /* Documented only for DSP 3.x */
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unimplemented input command %2X",sb.dsp.cmd);
		break;
	default:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unhandled (undocumented) command %2X",sb.dsp.cmd);
		break;
	}
	dsp_info.cmd = DSP_NO_COMMAND;
	dsp_info.cmd_len = 0;
	dsp_info.in.pos = 0;
}

/* DSP reset */
static void IOOUTCALL ct1741_write_reset(UINT port, REG8 dat)
{
	if ((dat & 0x01)) {
		/* status reset */
		ct1741_reset();
		dsp_info.state = DSP_STATUS_RESET;
	} else {
		/* status normal */
		ct1741_flush_data();
		ct1741_add_data(0xaa);
		dsp_info.state = DSP_STATUS_NORMAL;
	}
}

/* DSP Write Command/Data */
static void IOOUTCALL ct1741_write_data(UINT port, REG8 dat) {
	TRACEOUT(("CT1741 DSP command: %.2x", dat));
	switch (dsp_info.cmd) {
	case DSP_NO_COMMAND:
		dsp_info.cmd = dat;
		dsp_info.cmd_len = ct1741_cmd_len[dat];
		dsp_info.in.pos=0;
		if (!dsp_info.cmd_len) ct1741_exec_command();
		break;
	default:
		dsp_info.in.data[dsp_info.in.pos] = dat;
		dsp_info.in.pos++;
		if (dsp_info.in.pos >= dsp_info.cmd_len) ct1741_exec_command();
	}
	port = dat;
}

static REG8 IOINPCALL ct1741_read_reset(UINT port)
{
	/* DSP reset */
	return 0xff;		//ok
}

/* DSP read data */
static REG8 IOINPCALL ct1741_read_data(UINT port)
{
	static REG8 data = 0;
	if (dsp_info.out.used) {
		data = dsp_info.out.data[dsp_info.out.pos];
		dsp_info.out.pos++;
		if (dsp_info.out.pos >= DSP_BUFSIZE)
			dsp_info.out.pos -= DSP_BUFSIZE;
		dsp_info.out.used--;
	}
	return data;
}

/* DSP read write status */
static REG8 IOINPCALL ct1741_read_wstatus(UINT port)
{
	switch(dsp_info.state) {
		case DSP_STATUS_NORMAL:
			dsp_info.write_busy++;
			if (dsp_info.write_busy & 8) return 0xff;
			return 0x7f;
		case DSP_STATUS_RESET:
			return 0xff;
			break;
	}
	return 0xff;
}

/* DSP read read status */
static REG8 IOINPCALL ct1741_read_rstatus(UINT port)
{	
	// bit7が0ならDSPバッファは空
	if (dsp_info.out.used)
		return 0xff;
	else
		return 0x7f;
}

void ct1741_dma(NEVENTITEM item)
{
	UINT	r;
	SINT32	cnt;
	UINT	rem;
	UINT	pos;
	UINT	size;

	if (item->flag & NEVENT_SETEVENT) {
		if (g_sb16.dmach != 0xff) {
			sound_sync();
//			if (dsp_info.mode == DSP_MODE_DMA) {
				// 転送～
				rem = cs4231.bufsize - cs4231.bufdatas;
				pos = (cs4231.bufpos + cs4231.bufdatas) & (DMA_BUFSIZE -1);
				size = min(rem, DMA_BUFSIZE - pos);
				r = dmac_getdatas(dsp_info.dma.chan, (UINT8*)(dsp_info.dma.buf.b16 + pos), size);
//			}
			if ((dsp_info.dma.chan->leng.w) && (dsp_info.freq)) {
				// 再度イベント設定
				cnt = pccore.realclock * 16 / dsp_info.freq;
				nevent_set(NEVENT_CT1741, cnt, ct1741_dma, NEVENT_RELATIVE);
			}
		}
	}
}

REG8 DMACCALL ct1741dmafunc(REG8 func)
{
	SINT32	cnt;

	switch(func) {
		case DMAEXT_START:
//			if (cs4231cfg.rate) {
			cnt = pccore.realclock * 16 / dsp_info.freq;
			nevent_set(NEVENT_CT1741, cnt, ct1741_dma, NEVENT_ABSOLUTE);
//			}
			break;

		case DMAEXT_END:
//			if ((cs4231.reg.pinctrl & 2) && (cs4231.dmairq != 0xff)) {
//				cs4231.intflag = 1;
			pic_setirq(g_sb16.dmairq);
//			}
			break;

		case DMAEXT_BREAK:
			nevent_reset(NEVENT_CT1741);
			break;
	}
	return(0);
}

void ct1741io_reset(void)
{
	ct1741_reset();
	dsp_info.state = DSP_STATUS_NORMAL;
	dmac_attach(DMADEV_CT1741, g_sb16.dmach);
}

void ct1741io_bind(void)
{
	iocore_attachout(0x2600 + g_sb16.base, ct1741_write_reset);	/* DSP Reset */
	iocore_attachout(0x2C00 + g_sb16.base, ct1741_write_data);	/* DSP Write Command/Data */

	iocore_attachinp(0x2600 + g_sb16.base, ct1741_read_reset);	/* DSP Reset */
	iocore_attachinp(0x2a00 + g_sb16.base, ct1741_read_data);		/* DSP Read Data Port */
	iocore_attachinp(0x2c00 + g_sb16.base, ct1741_read_wstatus);	/* DSP Write Buffer Status (Bit 7) */
	iocore_attachinp(0x2e00 + g_sb16.base, ct1741_read_rstatus);	/* DSP Read Buffer Status (Bit 7) */
}

#endif
