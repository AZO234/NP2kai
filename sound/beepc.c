#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"beep.h"


	_BEEP		g_beep;
	BEEPCFG		beepcfg;


// #define	BEEPLOG

#if defined(BEEPLOG)
static struct {
	FILEH	fh;
	UINT	events;
	UINT32	event[0x10000];
} bplog;

static void beeplogflash(void) {

	if ((bplog.fh != FILEH_INVALID) && (bplog.events)) {
		file_write(bplog.fh, bplog.event, bplog.events * sizeof(UINT32));
		bplog.events = 0;
	}
}
#endif


void beep_initialize(UINT rate) {

	beepcfg.rate = rate;
	beepcfg.vol = 2;
#if defined(BEEPLOG)
	bplog.fh = file_create("beeplog");
	bplog.events = 0;
#endif
}

void beep_deinitialize(void) {

#if defined(BEEPLOG)
	beeplogflash();
	if (bplog.fh != FILEH_INVALID) {
		file_close(bplog.fh);
		bplog.fh = FILEH_INVALID;
	}
#endif
}

void beep_setvol(UINT vol) {

	beepcfg.vol = vol & 3;
}

void beep_changeclock(void) {

	UINT32	hz;
	UINT	rate;

	hz = pccore.realclock / 25;
	rate = beepcfg.rate / 25;
	beepcfg.samplebase = (1 << 16) * rate / hz;
}

void beep_reset(void) {

	beep_changeclock();
	ZeroMemory(&g_beep, sizeof(g_beep));
	g_beep.mode = 1;
}

void beep_hzset(UINT16 cnt) {

	double	hz;

	sound_sync();
	g_beep.hz = 0;
	if ((cnt & 0xff80) && (beepcfg.rate)) {
		hz = 65536.0 / 4.0 * pccore.baseclock / beepcfg.rate / cnt;
		if (hz < 0x8000) {
			g_beep.hz = (UINT16)hz;
			return;
		}
	}
}

void beep_modeset(void) {

	UINT8	newmode;

	newmode = (pit.ch[1].ctrl >> 2) & 3;
	if (g_beep.mode != newmode) {
		sound_sync();
		g_beep.mode = newmode;
		beep_eventinit();
	}
}

static void beep_eventset(void) {

	BPEVENT	*evt;
	int		enable;
	SINT32	clk;

	enable = g_beep.low & g_beep.buz;
	if (g_beep.enable != enable) {
#if defined(BEEPLOG)
		UINT32	tmp;
		tmp = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
		if (enable) {
			tmp |= 0x80000000;
		}
		else {
			tmp &= ~0x80000000;
		}
		bplog.event[bplog.events++] = tmp;
		if (bplog.events >= NELEMENTS(bplog.event)) {
			beeplogflash();
		}
#endif
		if (g_beep.events >= (BEEPEVENT_MAX / 2)) {
			sound_sync();
		}
		g_beep.enable = enable;
		if (g_beep.events < BEEPEVENT_MAX) {
			clk = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
			evt = g_beep.event + g_beep.events;
			g_beep.events++;
			evt->clock = (clk - g_beep.clock) * beepcfg.samplebase;
			evt->enable = enable;
			g_beep.clock = clk;
		}
	}
}

void beep_eventinit(void) {

	g_beep.low = 0;
	g_beep.enable = 0;
	g_beep.lastenable = 0;
	g_beep.clock = soundcfg.lastclock;
	g_beep.events = 0;
}

void beep_eventreset(void) {

	g_beep.lastenable = g_beep.enable;
	g_beep.clock = soundcfg.lastclock;
	g_beep.events = 0;
}

void beep_lheventset(int low) {

	if (g_beep.low != low) {
		g_beep.low = low;
		beep_eventset();
	}
}

void beep_oneventset(void) {

	int		buz;

	buz = (sysport.c & 8)?0:1;
	if (g_beep.buz != buz) {
		g_beep.buz = buz;
		beep_eventset();
	}
}

