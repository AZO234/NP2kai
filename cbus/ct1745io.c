#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"ct1741io.h"
#include	"ct1745io.h"
#include	"sound.h"
#include	"fmboard.h"

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
 * Creative SoundBlaster16 mixer CT1745
 */

static void ct1745_mixer_reset() {
	ZeroMemory(g_sb16.mixreg, sizeof(g_sb16.mixreg));

	g_sb16.mixregexp[MIXER_MASTER_LEFT]  = g_sb16.mixreg[MIXER_MASTER_LEFT]  =
	g_sb16.mixregexp[MIXER_MASTER_RIGHT] = g_sb16.mixreg[MIXER_MASTER_RIGHT] =
	g_sb16.mixregexp[MIXER_VOC_LEFT]     = g_sb16.mixreg[MIXER_VOC_LEFT]     =
	g_sb16.mixregexp[MIXER_VOC_RIGHT]    = g_sb16.mixreg[MIXER_VOC_RIGHT]    =
	g_sb16.mixregexp[MIXER_MIDI_LEFT]    = g_sb16.mixreg[MIXER_MIDI_LEFT]    =
	g_sb16.mixregexp[MIXER_MIDI_RIGHT]   = g_sb16.mixreg[MIXER_MIDI_RIGHT]   = 0xff;
	g_sb16.mixregexp[MIXER_OUT_SW]       = g_sb16.mixreg[MIXER_OUT_SW]       = 0x1f;
	g_sb16.mixregexp[MIXER_IN_SW_LEFT]   = g_sb16.mixreg[MIXER_IN_SW_LEFT]   = 0x15;
	g_sb16.mixregexp[MIXER_IN_SW_RIGHT]  = g_sb16.mixreg[MIXER_IN_SW_RIGHT]  = 0x0b;

	g_sb16.mixregexp[MIXER_TREBLE_LEFT]  = g_sb16.mixreg[MIXER_TREBLE_LEFT]  =
	g_sb16.mixregexp[MIXER_TREBLE_RIGHT] = g_sb16.mixreg[MIXER_TREBLE_RIGHT] =
	g_sb16.mixregexp[MIXER_BASS_LEFT]    = g_sb16.mixreg[MIXER_BASS_LEFT]    =
	g_sb16.mixregexp[MIXER_BASS_RIGHT]   = g_sb16.mixreg[MIXER_BASS_RIGHT]   = 8;
	g_sb16.mixregexp[0x82] = g_sb16.mixreg[0x82] = 2<<5; 

}

static void IOOUTCALL sb16_o2400(UINT port, REG8 dat) {
	(void)port;
	g_sb16.mixsel = dat;
}
static void IOOUTCALL sb16_o2500(UINT port, REG8 dat) {
//printf("mixer port write %x %x¥n",dat,g_sb16.mixsel);
	if (g_sb16.mixsel >= MIXER_VOL_START &&
		g_sb16.mixsel <= MIXER_VOL_END) {
		g_sb16.mixreg[g_sb16.mixsel] = dat;
		g_sb16.mixregexp[g_sb16.mixsel] = (int)(pow(((dat >> 3) & 0x1f) / 32.0, 2) * 255);//(int)(pow(dat / 255.0, 4) * 255);
		TRACEOUT(("CT1745 MIXER ID=0x%02x, DATA=0x%02x", g_sb16.mixsel, dat));
		return;
	}

	switch (g_sb16.mixsel) {
		case MIXER_RESET:	// Mixer reset
			ct1745_mixer_reset();
			break;
		case 0x04:			// Voice volume(old)
			g_sb16.mixregexp[MIXER_VOC_LEFT] = g_sb16.mixreg[MIXER_VOC_LEFT]  = (dat & 0x0f) << 4;
			g_sb16.mixregexp[MIXER_VOC_RIGHT] = g_sb16.mixreg[MIXER_VOC_RIGHT] = (dat & 0xf0);
			break;
		case 0x0a:			// Mic volume(old)
			g_sb16.mixregexp[MIXER_MIC] = g_sb16.mixreg[MIXER_MIC] = dat & 0x7;
			break;
		case 0x22:			// Master volume(old)
			g_sb16.mixregexp[MIXER_MASTER_LEFT] = g_sb16.mixreg[MIXER_MASTER_LEFT]  = (dat & 0x0f) << 4;
			g_sb16.mixregexp[MIXER_MASTER_RIGHT] = g_sb16.mixreg[MIXER_MASTER_RIGHT] = (dat & 0xf0);
			break;
		case 0x26:			// MIDI volume(old)
			g_sb16.mixregexp[MIXER_MIDI_LEFT] = g_sb16.mixreg[MIXER_MIDI_LEFT]  = (dat & 0x0f) << 4;
			g_sb16.mixregexp[MIXER_MIDI_RIGHT] = g_sb16.mixreg[MIXER_MIDI_RIGHT] = (dat & 0xf0);
			break;
		case 0x28:			// CD volume(old)
			g_sb16.mixregexp[MIXER_CD_LEFT] = g_sb16.mixreg[MIXER_CD_LEFT]  = (dat & 0x0f) << 4;
			g_sb16.mixregexp[MIXER_CD_RIGHT] = g_sb16.mixreg[MIXER_CD_RIGHT] = (dat & 0xf0);
			break;
		case 0x2e:			// Line volume(old)
			g_sb16.mixregexp[MIXER_LINE_LEFT] = g_sb16.mixreg[MIXER_LINE_LEFT]  = (dat & 0x0f) << 4;
			g_sb16.mixregexp[MIXER_LINE_RIGHT] = g_sb16.mixreg[MIXER_LINE_RIGHT] = (dat & 0xff);
			
		case 0x80:			// Write irq num
			ct1741_set_dma_irq(dat);
			TRACEOUT(("CT1745 MIXER SET IRQ ID=0x%02x", dat));
			break;
		case 0x81:			// Write dma num
			ct1741_set_dma_ch(dat);
			TRACEOUT(("CT1745 MIXER SET DMA ID=0x%02x", dat));
			break;
		case 0x83:
		default:
			break;
	}

}
static void IOOUTCALL sb16_o2500_AT(UINT port, REG8 dat) {
	if(g_sb16.mixsel==0x80){
		switch (dat) {
		case 1: // IRQ2
			// 変換不能
			break;
		case 2: // IRQ5
			dat = 8;
			break;
		case 4: // IRQ7 (IRQ2)
			// 変換不能
			break;
		case 8: // IRQ10
			dat = 2;
			break;
		}
	}else if(g_sb16.mixsel==0x81){
		switch (dat) {
		case 1: // DMA0
			dat = 1;
			break;
		case 2: // DMA1
			// 変換不能
			break;
		case 4: // NONE
			// 変換不能
			break;
		case 8: // DMA3
			dat = 2;
			break;
		}
	}
	sb16_o2500(port, dat);
}

static REG8 IOINPCALL sb16_i2400(UINT port) {
	return g_sb16.mixsel;
}
static REG8 IOINPCALL sb16_i2500(UINT port) {
//printf("mixer port read %x %x¥n",g_sb16.mixreg[g_sb16.mixsel],g_sb16.mixsel);
	if (g_sb16.mixsel >= MIXER_VOL_START && g_sb16.mixsel <= MIXER_VOL_END) {
		return g_sb16.mixreg[g_sb16.mixsel];
	}

	switch (g_sb16.mixsel) {
		case 0x04:
			return  (g_sb16.mixreg[MIXER_VOC_LEFT] + g_sb16.mixreg[MIXER_VOC_RIGHT]) << 1;
		case 0x22:
			return  (g_sb16.mixreg[MIXER_MASTER_LEFT] + g_sb16.mixreg[MIXER_MASTER_RIGHT]) << 1;
		case 0x26:
			return  (g_sb16.mixreg[MIXER_MIDI_LEFT] + g_sb16.mixreg[MIXER_MIDI_RIGHT]) << 1;
		case 0x28:			// CD volume(old)
			return  (g_sb16.mixreg[MIXER_CD_LEFT] + g_sb16.mixreg[MIXER_CD_RIGHT]) << 1;
		case 0x2e:			// Line volume(old)
			return  (g_sb16.mixreg[MIXER_LINE_LEFT] + g_sb16.mixreg[MIXER_LINE_RIGHT]) << 1;
		case 0x0a:			// Mic volume(old)
			return g_sb16.mixreg[MIXER_MIC];
		case 0x80:			// Read irq num
			return ct1741_get_dma_irq();
		case 0x81:			// Read dma num
			return ct1741_get_dma_ch();
		case 0x82:			// Irq pending(98には不要)　diagnose用　他よくわからず
			if(g_sb16.mixreg[0x82] == 0x41)return 0x1;
			if(g_sb16.mixreg[0x82] == 0x42)return 0x2;
			if(g_sb16.mixreg[0x82] == 0x43)return 0x3;
			else return 0x40;
		default:
			break;
	}
	return 0;
}
static REG8 IOINPCALL sb16_i2500_AT(UINT port) {
	REG8 ret = sb16_i2500(port);
	if(g_sb16.mixsel==0x80){
		switch (ret) {
		case 1: // IRQ3
			// 変換不能
			break;
		case 2: // IRQ10
			ret = 8;
			break;
		case 4: // IRQ12
			// 変換不能
			break;
		case 8: // IRQ5
			ret = 2;
			break;
		}
	}else if(g_sb16.mixsel==0x81){
		switch (ret) {
		case 1: // DMA0
			ret = 1;
			break;
		case 2: // DMA3
			ret = 8;
			break;
		case 4: // NONE
			// 変換不能
			break;
		case 8: // NONE
			// 変換不能
			break;
		}
	}
	return ret;
}

void ct1745io_reset(void)
{
	ct1745_mixer_reset();
}

void ct1745io_bind(void)
{
	iocore_attachout(0x2400 + g_sb16.base, sb16_o2400);	/* Mixer Chip Register Address Port */
	iocore_attachout(0x2500 + g_sb16.base, sb16_o2500);	/* Mixer Chip Data Port */
	iocore_attachinp(0x2400 + g_sb16.base, sb16_i2400);	/* Mixer Chip Register Address Port */
	iocore_attachinp(0x2500 + g_sb16.base, sb16_i2500);	/* Mixer Chip Data Port */
	
	// PC/AT互換機テスト
	if(np2cfg.sndsb16at){
		iocore_attachout(0x224, sb16_o2400);	/* Mixer Chip Register Address Port */
		iocore_attachout(0x225, sb16_o2500_AT);	/* Mixer Chip Data Port */
		iocore_attachinp(0x224, sb16_i2400);	/* Mixer Chip Register Address Port */
		iocore_attachinp(0x225, sb16_i2500_AT);	/* Mixer Chip Data Port */
	}
}
void ct1745io_unbind(void)
{
	iocore_detachout(0x2400 + g_sb16.base);	/* Mixer Chip Register Address Port */
	iocore_detachout(0x2500 + g_sb16.base);	/* Mixer Chip Data Port */
	iocore_detachinp(0x2400 + g_sb16.base);	/* Mixer Chip Register Address Port */
	iocore_detachinp(0x2500 + g_sb16.base);	/* Mixer Chip Data Port */

	// PC/AT互換機テスト
	if(np2cfg.sndsb16at){
		iocore_detachout(0x224);	/* Mixer Chip Register Address Port */
		iocore_detachout(0x225);	/* Mixer Chip Data Port */
		iocore_detachinp(0x224);	/* Mixer Chip Register Address Port */
		iocore_detachinp(0x225);	/* Mixer Chip Data Port */
	}
}

#endif
