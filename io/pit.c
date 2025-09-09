// 
// μPD8253C タイマLSI
// 

#include	<compiler.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<sound/sound.h>
#include	<sound/beep.h>
#include	<cbus/board14.h>
#include	<commng.h>

extern	COMMNG	cm_rs232c;

#define	BEEPCOUNTEREX					// BEEPアイドル時のカウンタをα倍に
#if defined(CPUCORE_IA32)
#define	uPD71054
#endif


// --- Interval timer

static void setsystimerevent(UINT32 cnt, NEVENTPOSITION absolute) {

	if (cnt > 8) {									// 根拠なし
		cnt *= pccore.multiple;
	}
	else {
		cnt = pccore.multiple << 16;
	}
	nevent_set(NEVENT_ITIMER, cnt, systimer, absolute);
}

void systimer(NEVENTITEM item) {

	PITCH	pitch;

	if (item->flag & NEVENT_SETEVENT) {
		pitch = pit.ch + 0;
		if (pitch->flag & PIT_FLAG_I) {
			pitch->flag &= ~PIT_FLAG_I;
			pic_setirq(0);
		}
		if ((pitch->ctrl & 0x0c) == 0x04) {
			// レートジェネレータ
			pitch->flag |= PIT_FLAG_I;
			setsystimerevent(pitch->value, NEVENT_RELATIVE);
		}
		else {
			setsystimerevent(0, NEVENT_RELATIVE);
		}
	}
}


// --- Beep

#if defined(BEEPCOUNTEREX)
static void setbeepeventex(UINT32 cnt, NEVENTPOSITION absolute) {

	if (cnt > 2) {
		cnt *= pccore.multiple;
	}
	else {
		cnt = pccore.multiple << 16;
	}
	while(cnt < 0x100000) {
		cnt <<= 1;
	}
	nevent_set(NEVENT_BEEP, (SINT32)cnt, beeponeshot, absolute);
}
#endif

static void setbeepevent(UINT32 cnt, NEVENTPOSITION absolute) {

	if (cnt > 2) {
		cnt *= pccore.multiple;
	}
	else {
		cnt = pccore.multiple << 16;
	}
	nevent_set(NEVENT_BEEP, cnt, beeponeshot, absolute);
}

void beeponeshot(NEVENTITEM item) {

	PITCH	pitch;

	if (item->flag & NEVENT_SETEVENT) {
		pitch = pit.ch + 1;
		if (!(pitch->ctrl & 0x0c)) {
			beep_lheventset(0);
		}
#if defined(uPD71054)
		if ((pitch->ctrl & 0x06) == 0x02)
#else
		if (pitch->ctrl & 0x02)
#endif
		{
#if defined(BEEPCOUNTEREX)
			setbeepeventex(pitch->value, NEVENT_RELATIVE);
#else
			setbeepevent(pitch->value, NEVENT_RELATIVE);
#endif
		}
	}
}


// --- RS-232C

static void setrs232cevent(UINT32 cnt, NEVENTPOSITION absolute) {

	if (cnt > 1) {
		cnt *= pccore.multiple;
	}
	else {
		cnt = pccore.multiple << 16;
	}
	cnt *= rs232c.mul;
	nevent_set(NEVENT_RS232C, cnt, rs232ctimer, absolute);
}

void rs232ctimer(NEVENTITEM item) {

	PITCH	pitch;

	if (item->flag & NEVENT_SETEVENT) {
		pitch = pit.ch + 2;
		if (pitch->flag & PIT_FLAG_I) {
			//pitch->flag &= ‾PIT_FLAG_I;
			rs232c_callback();
		}
#if defined(SUPPORT_RS232C_FIFO)
		if (rs232cfifo.vfast & 0x80) {
			// V FASTモード
			int speedtbl[16] = {
				0, 115200, 57600, 38400,
				28800, 0, 19200, 0,
				14400, 0, 0, 0,
				9600, 0, 0, 0,
			};
			int speed;
			speed = speedtbl[rs232cfifo.vfast & 0xf];
			if(speed != 0){
				nevent_set(NEVENT_RS232C, pccore.realclock * 8 / speed, rs232ctimer, NEVENT_RELATIVE);
			}else{
				nevent_set(NEVENT_RS232C, pccore.realclock * 8 / 9600, rs232ctimer, NEVENT_RELATIVE);
			}
		}else
#endif
		if ((pitch->ctrl & 0x0c) == 0x04) {
			// レートジェネレータ
			setrs232cevent(pitch->value, NEVENT_RELATIVE);
		}
		else {
			setrs232cevent(0, NEVENT_RELATIVE);
		}
	}
}


// ---------------------------------------------------------------------------

static UINT getcount(const _PITCH *pitch) {

	SINT32	clk;

	switch(pitch->ch) {
		case 0:
			clk = nevent_getremain(NEVENT_ITIMER);
			break;

		case 1:
			switch(pitch->ctrl & 0x06) {
#ifdef uPD71054				// ?
//				case 0x00:
#endif
				case 0x04:
					return(pitch->value);
#ifdef uPD71054
				case 0x06:
					return(pitch->value & (~1));
#endif
			}
			clk = nevent_getremain(NEVENT_BEEP);
#if defined(BEEPCOUNTEREX)
			if (clk >= 0) {
				clk /= pccore.multiple;
				if (pitch->value > 2) {
					clk %= pitch->value;
				}
				else {
					clk = LOW16(clk);
				}
				return(clk);
			}
#else
			break;
#endif

		case 2:
			clk = nevent_getremain(NEVENT_RS232C);
			break;

#if !defined(DISABLE_SOUND)
		case 3:
			return(board14_pitcount());
#endif

		default:
			clk = 0;
			break;
	}
	if (clk > 0) {
		return(clk / pccore.multiple);
	}
	return(0);
}

static void latchcmd(PITCH pitch, REG8 ctrl) {

	UINT8	flag;

	flag = pitch->flag;
	if (!(ctrl & PIT_LATCH_S)) {
		flag |= PIT_FLAG_S;
		pitch->stat = pitch->ctrl;
	}
	if (!(ctrl & PIT_LATCH_C)) {
		flag |= PIT_FLAG_C;
		flag &= ~PIT_FLAG_L;
		pitch->latch = getcount(pitch);
	}
	pitch->flag = flag;
}


// ----

void pit_setflag(PITCH pitch, REG8 value) {

	if (value & PIT_CTRL_RL) {
		pitch->ctrl = (UINT8)((value & 0x3f) | PIT_STAT_CMD);
		pitch->flag &= ~(PIT_FLAG_R | PIT_FLAG_W | PIT_FLAG_L |
													PIT_FLAG_S | PIT_FLAG_C);
	}
	else {														// latch
		latchcmd(pitch, ~PIT_LATCH_C);
	}
}

// RS-232C通信速度設定　関連: serial.c rs232c_vfast_setrs232cspeed
void pit_setrs232cspeed(UINT16 value) {
	if (cm_rs232c) {
#if defined(SUPPORT_RS232C_FIFO)
		if(!(rs232cfifo.vfast & 0x80)) // V FASTモードでは通信速度変更しない
#endif
		{
			if(value > 0){
				if ((pccore.dipsw[0] & 0x30)==0x30) { // とりあえず調歩同期だけ
					int newvalue;
					int mul[] = {1, 1, 16, 64};
					if (pccore.cpumode & CPUMODE_8MHZ) {
						newvalue = 9600 * 208 / mul[rs232c.rawmode & 0x3] / value;
					}else{
						newvalue = 9600 * 256 / mul[rs232c.rawmode & 0x3] / value;
					}
					if(newvalue <= 38400){ // XXX: 大きすぎるのは無視
						cm_rs232c->msg(cm_rs232c, COMMSG_CHANGESPEED, (INTPTR)&newvalue);
					}
				}
			}
		}
		cm_rs232c->msg(cm_rs232c, COMMSG_CHANGEMODE, (INTPTR)&rs232c.rawmode);
	}
}

BOOL pit_setcount(PITCH pitch, REG8 value) {

	UINT8	flag;

	switch(pitch->ctrl & PIT_CTRL_RL) {
		case PIT_RL_L:		// access low
			pitch->value = value;
			break;

		case PIT_RL_H:		// access high
			pitch->value = value << 8;
			break;

		case PIT_RL_ALL:	// access word
			flag = pitch->flag;
			pitch->flag = flag ^ PIT_FLAG_W;
			if (!(flag & PIT_FLAG_W)) {
				pitch->value &= 0xff00;
				pitch->value += value;
				return(TRUE);
			}
			pitch->value &= 0x00ff;
			pitch->value += value << 8;
			break;
	}
	pitch->ctrl &= ~PIT_STAT_CMD;
	if (((pitch->ctrl & 0x06) == 0x02) && (pitch->flag & PIT_FLAG_I)) {
		return(TRUE);
	}
	return(FALSE);
}

REG8 pit_getstat(PITCH pitch) {

	UINT8	flag;
	UINT8	rl;
	UINT16	w;
	REG8	ret;

	flag = pitch->flag;
#if defined(uPD71054)
	if (flag & PIT_FLAG_S) {
		flag &= ~PIT_FLAG_S;
		ret = pitch->stat;
	}
	else
#endif
	{
		rl = pitch->ctrl & PIT_CTRL_RL;
		if (flag & (PIT_FLAG_C | PIT_FLAG_L)) {
			flag &= ~PIT_FLAG_C;
			if (rl == PIT_RL_ALL) {
				flag ^= PIT_FLAG_L;
			}
			w = pitch->latch;
		}
		else {
			w = getcount(pitch);
		}
		switch(rl) {
			case PIT_RL_L:						// access low
				ret = (UINT8)w;
				break;

			case PIT_RL_H:
				ret = (UINT8)(w >> 8);
				break;

			case PIT_RL_ALL:
			default:
				if (!(flag & PIT_FLAG_R)) {
					ret = (UINT8)w;
				}
				else {
					ret = (UINT8)(w >> 8);
				}
				flag ^= PIT_FLAG_R;
				break;
		}
	}
	pitch->flag = flag;
	return(ret);
}


// ---- I/O

// system timer
static void IOOUTCALL pit_o71(UINT port, REG8 dat) {

	PITCH	pitch;

	pitch = pit.ch + 0;
	if (pit_setcount(pitch, dat)) {
		return;
	}
	pic.pi[0].irr &= (~1);
	pitch->flag |= PIT_FLAG_I;
	setsystimerevent(pitch->value, NEVENT_ABSOLUTE);
	(void)port;
}

static int beep_mode_bit = 0;
static int beep_mode_bit_c = 0;

// beep
static void IOOUTCALL pit_o73(UINT port, REG8 dat) {

	PITCH	pitch;

	if(g_beep.mode == 0) {
		switch(beep_mode_bit) {
		case 0:
			beep_data[g_beep.beep_data_load_loc] = dat;
			break;
		case 1:
			beep_data[g_beep.beep_data_load_loc] = dat << 8;
			break;
		case 2:
			if(beep_mode_bit_c == 0)
				beep_data[g_beep.beep_data_load_loc] = dat;
			else
				beep_data[g_beep.beep_data_load_loc] += dat << 8;
			break;
		}
		beep_time[g_beep.beep_data_load_loc] = CPU_CLOCK;
		if(!(beep_mode_bit == 2 && beep_mode_bit_c == 0)) {
			g_beep.beep_data_load_loc++;
			if(g_beep.beep_data_load_loc >= BEEPDATACOUNT)
				g_beep.beep_data_load_loc = 0;
			g_beep.beep_laskclk = CPU_CLOCK;
		}
		beep_mode_bit_c ^= 1;
	}

	pitch = pit.ch + 1;
	if (pit_setcount(pitch, dat)) {
		return;
	}
	setbeepevent(pitch->value, NEVENT_ABSOLUTE);
	beep_lheventset(1);												// ver0.79
	if (pitch->ctrl & 0x0c) {
		beep_hzset(pitch->value);
	}
}

// rs-232c
static void IOOUTCALL pit_o75(UINT port, REG8 dat) {

	PITCH	pitch;
	UINT16	oldvalue;

	pitch = pit.ch + 2;
	oldvalue = pitch->value;
	if (pit_setcount(pitch, dat)) {
		if(pitch->value != oldvalue){
			pit_setrs232cspeed(pitch->value);
		}
		return;
	}
	if(pitch->value != oldvalue){
		pit_setrs232cspeed(pitch->value);
	}
	pitch->flag |= PIT_FLAG_I;
	rs232c_open();
	setrs232cevent(pitch->value, NEVENT_ABSOLUTE);
	(void)port;
}

// ctrl
static void IOOUTCALL pit_o77(UINT port, REG8 dat) {

	UINT	chnum;
	PITCH	pitch;

	if((dat & 0xC0) == 0x40) {
		if(CPU_CLOCK - g_beep.beep_laskclk >= 20000000) {
			g_beep.beep_data_load_loc = 0;
			g_beep.beep_data_curr_loc = 0;
		}
		beep_mode_bit = ((dat >> 4) & 3) - 1;
		beep_mode_bit_c = 0;
	}

	chnum = (dat >> 6) & 3;
	if (chnum != 3) {
		pitch = pit.ch + chnum;
		pit_setflag(pitch, dat);
		if (chnum == 0) {		// 書込みで itimerのirrがリセットされる…
			pic.pi[0].irr &= (~1);
			if (dat & 0x30) {	// 一応ラッチ時は割り込みをセットしない
				pitch->flag |= PIT_FLAG_I;
			}
		}
		if (chnum == 1) {
			beep_modeset();
		}
	}
#if defined(uPD71054)
	else {
		TRACEOUT(("multiple latch commands - %x", dat));
		for (chnum=0; chnum<3; chnum++) {
			if (dat & (2 << chnum)) {
				latchcmd(pit.ch + chnum, dat);
			}
		}
	}
#endif
	(void)port;
}

static REG8 IOINPCALL pit_i71(UINT port) {

	return(pit_getstat(pit.ch + ((port >> 1) & 3)));
}


// ---- I/F

static const IOOUT pito71[4] = {
					pit_o71,	pit_o73,	pit_o75,	pit_o77};

static const IOINP piti71[4] = {
					pit_i71,	pit_i71,	pit_i71,	NULL};

void itimer_reset(const NP2CFG *pConfig) {

	UINT16	beepcnt;

	ZeroMemory(&pit, sizeof(pit));
	beepcnt = (pccore.cpumode & CPUMODE_8MHZ)?998:1229;
	pit.ch[0].ctrl = 0x30;
	pit.ch[0].ch = 0;
	pit.ch[0].flag = PIT_FLAG_I;
	pit.ch[0].ctrl = 0x56 & 0x3f;
	pit.ch[1].ch = 1;
	pit.ch[1].value = beepcnt;
	pit.ch[2].ctrl = 0xb6 & 0x3f;
	pit.ch[2].ch = 2;
#if !defined(DISABLE_SOUND)
	pit.ch[3].ctrl = 0x36;
	pit.ch[3].ch = 3;
	pit.ch[4].ctrl = 0x36;
	pit.ch[4].ch = 4;
#endif
	setsystimerevent(0, NEVENT_ABSOLUTE);
	beep_lheventset(1);												// ver0.79
	beep_hzset(beepcnt);
//	setrs232cevent(0, NEVENT_ABSOLUTE);

	(void)pConfig;
}

void itimer_bind(void) {

	iocore_attachsysoutex(0x0071, 0x0cf1, pito71, 4);
	iocore_attachsysinpex(0x0071, 0x0cf1, piti71, 4);
	iocore_attachout(0x3fd9, pit_o71);
	iocore_attachout(0x3fdb, pit_o73);
	iocore_attachout(0x3fdd, pit_o75);
	iocore_attachout(0x3fdf, pit_o77);
	iocore_attachinp(0x3fd9, pit_i71);
	iocore_attachinp(0x3fdb, pit_i71);
	iocore_attachinp(0x3fdd, pit_i71);
}

