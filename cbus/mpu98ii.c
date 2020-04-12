
// emulation MPU-401 CPU Version 1.3 ('84/07/07)

#include	"compiler.h"
#include	"commng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"mpu98ii.h"
#include	"fmboard.h"

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
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
#else
#define	TRACEOUT(s)	(void)(s)
#endif
#endif	/* 1 */

enum {
	MIDI_STOP			= 0xfc,

	MIDIIN_AVAIL		= 0x80,
	MIDIOUT_BUSY		= 0x40,

	MPUMSG_TRACKDATAREQ	= 0xf0,
	MPUMSG_OVERFLOW		= 0xf8,
	MPUMSG_REQCOND		= 0xf9,
	MPUMSG_DATAEND		= 0xfc,
	MPUMSG_HCLK			= 0xfd,
	MPUMSG_ACK         	= 0xfe,
	MPUMSG_SYS			= 0xff,

	MIDITIMEOUTCLOCK	= 3000,
	MIDITIMEOUTCLOCK2	= 300
};

enum {
	MIDIE_STEP			= 0x01,
	MIDIE_EVENT			= 0x02,
	MIDIE_DATA			= 0x04
};

enum {
	MPUCMDP_IDLE		= 0x00,
	MPUCMDP_STEP		= 0x01,
	MPUCMDP_CMD			= 0x02,
	MPUCMDP_REQ			= 0x04,
	MPUCMDP_FOLLOWBYTE	= 0x08,
	MPUCMDP_SHORT		= 0x10,
	MPUCMDP_LONG		= 0x20,
	MPUCMDP_INIT		= 0x80
};

enum {
	MPUSYNCMODE_INT			= 0x00,
	MPUSYNCMODE_FSK			= 0x01,
	MPUSYNCMODE_MIDI		= 0x02,

	MPUMETROMODE_ACC		= 0x00,
	MPUMETROMODE_OFF		= 0x01,
	MPUMETROMODE_ON			= 0x02,

	MPUFLAG1_PLAY			= 0x01,
	MPUFLAG1_BENDERTOHOST	= 0x08,
	MPUFLAG1_THRU			= 0x10,
	MPUFLAG1_DATAINSTOP		= 0x20,
	MPUFLAG1_SENDME			= 0x40,
	MPUFLAG1_CONDUCTOR		= 0x80,

	MPUFLAG2_RTAFF			= 0x01,
	MPUFLAG2_FSKRESO		= 0x02,
	MPUFLAG2_CLKTOHOST		= 0x04,
	MPUFLAG2_EXECLUTOHOST	= 0x08,
	MPUFLAG2_REFA			= 0x10,
	MPUFLAG2_REFB			= 0x20,
	MPUFLAG2_REFC			= 0x40,
	MPUFLAG2_REFD			= 0x80
};


	_MPU98II	mpu98;
	COMMNG		cm_mpu98;


static const UINT8 mpuirqnum[4] = {3, 5, 6, 12};

static const UINT8 shortmsgleng[0x10] = {
		0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 2, 2, 3, 1};

static const UINT8 hclk_step1[4][4] = {{0, 0, 0, 0}, {1, 0, 0, 0},
									{1, 0, 1, 0}, {1, 1, 1, 0}};


static void makeintclock(void) {

	UINT32	l;
	UINT	curtempo;

	l = mpu98.tempo * 2 * mpu98.reltempo / 0x40;
	if (l < 5 * 2) {
		l = 5 * 2;
	}
	curtempo = l >> 1;
	if (curtempo > 250) {
		curtempo = 250;
	}
	mpu98.curtempo = curtempo;
	if (!(mpu98.flag2 & MPUFLAG2_FSKRESO)) {
		l *= mpu98.inttimebase;							//	*12
	}
	mpu98.stepclock = (pccore.realclock * 5 / l);		//	/12
}

static void sethclk(REG8 data) {

	REG8	quarter;
	int		i;

	quarter = data >> 2;
	if (!quarter) {
		quarter = 64;
	}
	for (i=0; i<4; i++) {
		mpu98.hclk_step[i] = quarter + hclk_step1[data & 3][i];
	}
	mpu98.hclk_rem = 0;
}

static void setdefaultcondition(void) {

	mpu98.recvevent = 0;
	mpu98.remainstep = 0;
	mpu98.intphase = 0;
	mpu98.intreq = 0;

	ZeroMemory(&mpu98.cmd, sizeof(mpu98.cmd));
	ZeroMemory(mpu98.tr, sizeof(mpu98.tr));
	ZeroMemory(&mpu98.cond, sizeof(mpu98.cond));

	mpu98.syncmode = MPUSYNCMODE_INT;
	mpu98.metromode = MPUMETROMODE_OFF;
	mpu98.flag1 = MPUFLAG1_THRU | MPUFLAG1_SENDME;
	mpu98.flag2 = MPUFLAG2_RTAFF;

	mpu98.inttimebase = 120 / 24;
	mpu98.tempo = 100;
	mpu98.reltempo = 0x40;
	makeintclock();
	mpu98.midipermetero = 12;
	mpu98.meteropermeas = 8;
	sethclk(240);
	mpu98.acttr = 0x00;
	mpu98.sendplaycnt = 0;
	mpu98.accch = 0xffff;
}

static void setrecvdata(REG8 data) {

	MPURECV	*r;

	r = &mpu98.r;
	if (r->cnt < MPU98_RECVBUFS) {
		r->buf[(r->pos + r->cnt) & (MPU98_RECVBUFS - 1)] = data;
		r->cnt++;
	}
}

static void sendmpushortmsg(const UINT8 *dat, UINT count) {

	UINT	i;
	COMMNG	cm;

#if 0
	if (!(mpu98.flag1 & MPUFLAG1_BENDERTOHOST)) {
		switch(dat[0] >> 4) {
			case 0x0a:
			case 0x0d:
			case 0x0e:
				return;

			case 0x0b:
				if (dat[1] < 0x40) {
					return;
				}
				break;
		}
	}
#endif
	cm = cm_mpu98;
	for (i=0; i<count; i++) {
		cm->write(cm, dat[i]);
	}
}

static void sendmpulongmsg(const UINT8 *dat, UINT count) {

	UINT	i;
	COMMNG	cm;

	cm = cm_mpu98;
	for (i=0; i<count; i++) {
		cm->write(cm, dat[i]);
	}
}

static void sendmpureset(void) {

	UINT	i;
	UINT8	sMessage[3];

	for (i=0; i<0x10; i++) {
		sMessage[0] = (UINT8)(0xb0 + i);
		sMessage[1] = 0x7b;
		sMessage[2] = 0x00;
		sendmpushortmsg(sMessage, 3);
	}
}

static void mpu98ii_int(void) {

	TRACEOUT(("int!"));
	pic_setirq(mpu98.irqnum);
	//// Sound Blaster 16
	//if(g_nSoundID == SOUNDID_SB16 || g_nSoundID == SOUNDID_PC_9801_86_SB16 || g_nSoundID == SOUNDID_WSS_SB16 || g_nSoundID == SOUNDID_PC_9801_86_WSS_SB16 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
	//	pic_setirq(g_sb16.dmairq);
	//}
	//// PC-9801-118
	//if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
	//	pic_setirq(10);
	//}
	//// WaveStar
	//if(g_nSoundID == SOUNDID_WAVESTAR){
	//	pic_setirq(10);
	//}
}

static void tr_step(void) {

	int		i;
	REG8	bit;

	if (mpu98.flag1 & MPUFLAG1_CONDUCTOR) {
		if (mpu98.cond.step) {
			mpu98.cond.step--;
		}
	}
	for (i=0, bit=1; i<8; bit<<=1, i++) {
		if (mpu98.acttr & bit) {
			if (mpu98.tr[i].step) {
				mpu98.tr[i].step--;
			}
		}
	}
}

static BOOL tr_nextsearch(void) {

	int		i;
	REG8	bit;

tr_nextsearch_more:
	if (mpu98.intphase == 1) {
		if (mpu98.flag1 & MPUFLAG1_CONDUCTOR) {
			if (!mpu98.cond.step) {
				mpu98.intreq = MPUMSG_REQCOND;
				mpu98.cond.phase |= MPUCMDP_STEP | MPUCMDP_CMD;
				mpu98ii_int();
				return(TRUE);
			}
		}
		mpu98.intphase = 2;
	}
	if (mpu98.intphase) {
		bit = 1 << (mpu98.intphase - 2);
		do {
			if (mpu98.acttr & bit) {
				MPUTR *tr;
				tr = mpu98.tr + (mpu98.intphase - 2);
				if (!tr->step) {
					if ((tr->datas) && (tr->remain == 0)) {
						if (cm_mpu98 == NULL) {
							cm_mpu98 = commng_create(COMCREATE_MPU98II);
						}
						if (tr->data[0] == MIDI_STOP) {
							tr->datas = 0;
							cm_mpu98->write(cm_mpu98, MIDI_STOP);
							setrecvdata(MIDI_STOP);
							mpu98ii_int();
							return(TRUE);
						}
						for (i=0; i<tr->datas; i++) {
							cm_mpu98->write(cm_mpu98, tr->data[i]);
						}
						tr->datas = 0;
					}
					mpu98.intreq = 0xf0 + (mpu98.intphase - 2);
					mpu98.recvevent |= MIDIE_STEP;
					mpu98ii_int();
					return(TRUE);
				}
			}
			bit <<= 1;
			mpu98.intphase++;
		} while(mpu98.intphase < 10);
		mpu98.intphase = 0;
	}
	mpu98.remainstep--;
	if (mpu98.remainstep) {
		tr_step();
		mpu98.intphase = 1;
		goto tr_nextsearch_more;
	}
	return(FALSE);
}

void midiint(NEVENTITEM item) {

	nevent_set(NEVENT_MIDIINT, mpu98.stepclock, midiint, NEVENT_RELATIVE);

	if (mpu98.flag2 & MPUFLAG2_CLKTOHOST) {
		if (!mpu98.hclk_rem) {
			mpu98.hclk_rem = mpu98.hclk_step[mpu98.hclk_cnt & 3];
			mpu98.hclk_cnt++;
		}
		mpu98.hclk_rem--;
		if (!mpu98.hclk_rem) {
			setrecvdata(MPUMSG_HCLK);
			mpu98ii_int();
		}
	}
	if (mpu98.flag1 & MPUFLAG1_PLAY) {
		if (!mpu98.remainstep++) {
			tr_step();
			mpu98.intphase = 1;
			tr_nextsearch();
		}
	}
	(void)item;
}

void midiwaitout(NEVENTITEM item) {

//	TRACE_("midi ready", 0);
	mpu98.status &= ~MIDIOUT_BUSY;
	(void)item;
}

static void midiwait(SINT32 waitclock) {

	if (!nevent_iswork(NEVENT_MIDIWAIT)) {
		mpu98.status |= MIDIOUT_BUSY;
		nevent_set(NEVENT_MIDIWAIT, waitclock, midiwaitout, NEVENT_ABSOLUTE);
	}
}


// ----

typedef REG8 (*MPUCMDFN)(REG8 cmd);

static REG8 mpucmd_xx(REG8 cmd) {

//	TRACEOUT(("unknown MPU commands: %.2x", cmd));
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_md(REG8 cmd) {			// 00-2F: Mode

	TRACEOUT(("mpucmd_md %.2x", cmd));
#if 0
	switch((cmd >> 0) & 3) {
		case 1:								// MIDI Stop
		case 2:								// MIDI Start
		case 3:								// MIDI Cont
			break;
	}
#endif
	switch((cmd >> 2) & 3) {
		case 1:								// Stop Play
			mpu98.flag1 &= ~MPUFLAG1_PLAY;
			mpu98.recvevent = 0;
			mpu98.intphase = 0;
			mpu98.intreq = 0;
			ZeroMemory(mpu98.tr, sizeof(mpu98.tr));
			ZeroMemory(&mpu98.cond, sizeof(mpu98.cond));
			if (!(mpu98.flag2 & MPUFLAG2_CLKTOHOST)) {
				nevent_reset(NEVENT_MIDIINT);
			}
			break;

		case 2:								// Start Play
			mpu98.flag1 |= MPUFLAG1_PLAY;
			mpu98.remainstep = 0;
			if (!nevent_iswork(NEVENT_MIDIINT)) {
				nevent_set(NEVENT_MIDIINT, mpu98.stepclock,
											midiint, NEVENT_ABSOLUTE);
			}
			break;
	}
#if 0
	switch((cmd >> 4) & 3) {
		case 1:								// Stop Rec
		case 2:								// Rec
			break;
	}
#endif
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_3f(REG8 cmd) {			// 3F: UART

	mpu98.mode = 1;
	sendmpureset();
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_sr(REG8 cmd) {			// 40-7F: Set ch of Ref table

	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_sm(REG8 cmd) {			// 80-82: Clock Sync/Mode

	mpu98.syncmode = cmd - 0x80;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_mm(REG8 cmd) {			// 83-85: Metronome

	mpu98.metromode = cmd - 0x83;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_8x(REG8 cmd) {			// 86-8F: Flag1

	REG8	bit;

	bit = 1 << ((cmd >> 1) & 7);
	if (cmd & 1) {
		mpu98.flag1 |= bit;
	}
	else {
		mpu98.flag1 &= ~bit;
	}
#if 0
	switch(cmd & 0x0f) {
		case 0x06:							// 86: Bender to Host / off
		case 0x07:							// 87: Bender to Host / on
		case 0x08:							// 88: THRU / off
		case 0x09:							// 89: THRU / on
		case 0x0a:							// 8A: Data in Stop / off
		case 0x0b:							// 8B: Data in Stop / on
		case 0x0c:							// 8C: Send Me / off
		case 0x0d:							// 8D: Send Me / on
		case 0x0e:							// 8E: Conductor / off
		case 0x0f:							// 8F: Conductor / on
			break;
	}
#endif
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_9x(REG8 cmd) {			// 90-9F: Flag2

	REG8	bit;

	bit = 1 << ((cmd >> 1) & 7);
	if (cmd & 1) {
		mpu98.flag2 |= bit;
	}
	else {
		mpu98.flag2 &= ~bit;
	}
	switch(cmd & 0x0f) {
#if 0
		case 0x00:							// 90: RT Aff / off
		case 0x01:							// 91: RT Aff / on
		case 0x02:							// 92: FSK Reso / INT
		case 0x03:							// 93: FSK Reso / 24
			break;
#endif

		case 0x04:							// 94: CLK to Host / off
			if (!(mpu98.flag1 & MPUFLAG1_PLAY)) {
				nevent_reset(NEVENT_MIDIINT);
			}
			break;

		case 0x05:							// 95: CLK to Host / on
			if (!nevent_iswork(NEVENT_MIDIINT)) {
				nevent_set(NEVENT_MIDIINT, mpu98.stepclock,
											midiint, NEVENT_ABSOLUTE);
			}
			break;

#if 0
		case 0x06:							// 96: Exclu to Host / off
		case 0x07:							// 97: Exclu to Host / on
		case 0x08:							// 98: Ref A / off
		case 0x09:							// 99: Ref A / on
		case 0x0a:							// 9A: Ref B / off
		case 0x0b:							// 9B: Ref B / on
		case 0x0c:							// 9C: Ref C / off
		case 0x0d:							// 9D: Ref C / on
		case 0x0e:							// 9E: Ref D / off
		case 0x0f:							// 9F: Ref D / on
			break;
#endif
	}
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_rq(REG8 cmd) {			// A0-AF: Req

	(void)cmd;
	return(MPUCMDP_REQ);
}

static REG8 mpucmd_b1(REG8 cmd) {			// B1: Clear Rel

	mpu98.reltempo = 0x40;
	makeintclock();
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_b8(REG8 cmd) {			// B8: Clear PC

	int		i;

	for (i=0; i<8; i++) {
		mpu98.tr[i].step = 0;
	}
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_tb(REG8 cmd) {			// C2-C8: .INT Time Base

	mpu98.inttimebase = cmd & 0x0f;
	makeintclock();
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_ws(REG8 cmd) {			// D0-D7: Want to Send Data

	(void)cmd;
	return(MPUCMDP_SHORT | MPUCMDP_INIT);
}

static REG8 mpucmd_df(REG8 cmd) {			// DF: WSD System

	(void)cmd;
	return(MPUCMDP_LONG | MPUCMDP_INIT);
}

static REG8 mpucmd_fo(REG8 cmd) {			// E0-EF: Following Byte

	(void)cmd;
	return(MPUCMDP_FOLLOWBYTE);
}

static REG8 mpucmd_ff(REG8 cmd) {			// FF: Reset

	sendmpureset();
	nevent_reset(NEVENT_MIDIINT);
	setdefaultcondition();
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static const MPUCMDFN mpucmds[0x100] = {
	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,		// 00
	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,
	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,

	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,		// 10
	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,
	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,

	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,		// 20
	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,
	mpucmd_xx,		mpucmd_md,		mpucmd_md,		mpucmd_md,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,

	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		// 30
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_3f,

	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		// 40
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,

	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		// 50
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,

	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		// 60
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,

	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		// 70
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,
	mpucmd_sr,		mpucmd_sr,		mpucmd_sr,		mpucmd_sr,

	mpucmd_sm,		mpucmd_sm,		mpucmd_sm,		mpucmd_mm,		// 80
	mpucmd_mm,		mpucmd_mm,		mpucmd_8x,		mpucmd_8x,
	mpucmd_8x,		mpucmd_8x,		mpucmd_8x,		mpucmd_8x,
	mpucmd_8x,		mpucmd_8x,		mpucmd_8x,		mpucmd_8x,

	mpucmd_9x,		mpucmd_9x,		mpucmd_9x,		mpucmd_9x,		// 90
	mpucmd_9x,		mpucmd_9x,		mpucmd_9x,		mpucmd_9x,
	mpucmd_9x,		mpucmd_9x,		mpucmd_9x,		mpucmd_9x,
	mpucmd_9x,		mpucmd_9x,		mpucmd_9x,		mpucmd_9x,

	mpucmd_rq,		mpucmd_rq,		mpucmd_rq,		mpucmd_rq,		// a0
	mpucmd_rq,		mpucmd_rq,		mpucmd_rq,		mpucmd_rq,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_rq,
	mpucmd_rq,		mpucmd_rq,		mpucmd_xx,		mpucmd_rq,

	mpucmd_xx,		mpucmd_b1,		mpucmd_xx,		mpucmd_xx,		// b0
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_b8,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,

	mpucmd_xx,		mpucmd_xx,		mpucmd_tb,		mpucmd_tb,		// c0
	mpucmd_tb,		mpucmd_tb,		mpucmd_tb,		mpucmd_tb,
	mpucmd_tb,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,

	mpucmd_ws,		mpucmd_ws,		mpucmd_ws,		mpucmd_ws,		// d0
	mpucmd_ws,		mpucmd_ws,		mpucmd_ws,		mpucmd_ws,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_df,

	mpucmd_fo,		mpucmd_fo,		mpucmd_fo,		mpucmd_xx,		// e0
	mpucmd_fo,		mpucmd_xx,		mpucmd_fo,		mpucmd_fo,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_fo,		mpucmd_fo,		mpucmd_fo,		mpucmd_fo,

	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		// f0
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_xx,
	mpucmd_xx,		mpucmd_xx,		mpucmd_xx,		mpucmd_ff};


static void reqmpucmdgroupd(REG8 cmd) {

	switch(cmd) {
		case 0xa0:						// A0: Req Play Cnt Tr1
		case 0xa1:						// A1: Req Play Cnt Tr2
		case 0xa2:						// A2: Req Play Cnt Tr3
		case 0xa3:						// A3: Req Play Cnt Tr4
		case 0xa4:						// A4: Req Play Cnt Tr5
		case 0xa5:						// A5: Req Play Cnt Tr6
		case 0xa6:						// A6: Req Play Cnt Tr7
		case 0xa7:						// A7: Req Play Cnt Tr8
			setrecvdata(mpu98.tr[cmd - 0xa0].step);
			break;

		case 0xab:						// AB: Read & Clear RC
			setrecvdata(0);
			break;

		case 0xac:						// AC: Req Major Version
			setrecvdata(1);
			break;

		case 0xad:						// AD: Req Minor Version
			setrecvdata(0);
			break;

		case 0xaf:						// AF: Req Tempo
			setrecvdata(mpu98.curtempo);
			break;
	}
}

static void setmpucmdgroupe(REG8 cmd, REG8 data) {

	switch(cmd) {
		case 0xe0:				// Set Tempo
			mpu98.tempo = data;
			mpu98.reltempo = 0x40;
			makeintclock();
			break;

		case 0xe1:				// Rel Tempo
			mpu98.reltempo = data;
			makeintclock();
			break;

		case 0xe2:				// Graduation
			break;

		case 0xe4:				// MIDI/Metro
			mpu98.midipermetero = data;
			break;

		case 0xe6:				// Metro/Meas
			mpu98.meteropermeas = data;
			break;

		case 0xe7:				// INTx4/H.CLK
			sethclk(data);
			break;

		case 0xec:				// Act Tr
			mpu98.acttr = data;
			break;

		case 0xed:				// Send Play CNT
			mpu98.sendplaycnt = data;
			break;

		case 0xee:				// Acc CH 1-8
			mpu98.accch = (mpu98.accch & 0xff00) | data;
			break;

		case 0xef:				// Acc CH 9-16
			mpu98.accch = (mpu98.accch & 0x00ff) | (data << 8);
			break;
	}
}

static BOOL sendmpumsg(MPUCMDS *cmd, REG8 data) {

	if (cmd->phase & MPUCMDP_SHORT) {
		if (cmd->phase & MPUCMDP_INIT) {
			cmd->phase &= ~MPUCMDP_INIT;
			if (!(data & 0x80)) {
				cmd->data[0] = cmd->rstat;
				cmd->datapos = 1;
				cmd->datacnt = shortmsgleng[cmd->rstat >> 4];
			}
			else {
				if ((data & 0xf0) != 0xf0) {
					cmd->rstat = data;
				}
				cmd->datapos = 0;
				cmd->datacnt = shortmsgleng[data >> 4];
			}
		}
		cmd->data[cmd->datapos] = data;
		cmd->datapos++;
		if (cmd->datapos >= cmd->datacnt) {
			sendmpushortmsg(cmd->data, cmd->datacnt);
			cmd->phase &= ~MPUCMDP_SHORT;
		}
		return(TRUE);
	}
	else if (cmd->phase & MPUCMDP_LONG) {
		if (cmd->phase & MPUCMDP_INIT) {
			cmd->phase &= ~MPUCMDP_INIT;
			cmd->datapos = 0;
			cmd->datacnt = sizeof(cmd->data);
		}
		if (cmd->datapos < cmd->datacnt) {
			cmd->data[cmd->datapos] = data;
			cmd->datapos++;
		}
		switch(cmd->data[0]) {
			case 0xf0:
				if (data == 0xf7) {
					cmd->phase &= ~MPUCMDP_LONG;
					sendmpulongmsg(cmd->data, cmd->datapos);
				}
				break;

			case 0xf2:
			case 0xf3:
				if (cmd->datapos >= 3) {
					cmd->phase &= ~MPUCMDP_LONG;
				}
				break;

			case 0xf6:
			default:
				cmd->phase &= ~MPUCMDP_LONG;
				break;
		}
		return(TRUE);
	}
	return(FALSE);
}

static BRESULT sendmpucmd(MPUCMDS *cmd, REG8 data) {

	if (cmd->phase & MPUCMDP_FOLLOWBYTE) {
		cmd->phase &= ~MPUCMDP_FOLLOWBYTE;
		setmpucmdgroupe(cmd->cmd, data);
		return(SUCCESS);
	}
	else if (cmd->phase & (MPUCMDP_SHORT | MPUCMDP_LONG)) {
		sendmpumsg(cmd, data);
		return(SUCCESS);
	}
	else {
		cmd->phase = 0;
		return(FAILURE);
	}
}

static BRESULT sendmpucond(MPUCMDS *cmd, REG8 data) {

	if (cmd->phase & (MPUCMDP_SHORT | MPUCMDP_LONG)) {
//		if (mpu98.intreq == 0xf9) {
//			mpu98.intreq = 0;
//		}
		sendmpumsg(cmd, data);
		return(SUCCESS);
	}
	else if (cmd->phase & MPUCMDP_STEP) {
		if (data < 0xf0) {
			cmd->step = data;
			cmd->phase &= ~MPUCMDP_STEP;
		}
		else {
			cmd->step = 0xf0;
			cmd->phase &= ~(MPUCMDP_STEP | MPUCMDP_CMD);
		}
		return(SUCCESS);
	}
	else if (cmd->phase & MPUCMDP_CMD) {
		cmd->phase &= ~MPUCMDP_CMD;
		cmd->cmd = data;
		if (data < 0xf0) {
			REG8 phase;
			phase = (*mpucmds[data])(data);
			if (phase & MPUCMDP_REQ) {
				phase &= ~MPUCMDP_REQ;
				reqmpucmdgroupd(data);
			}
			cmd->phase = phase;
			if (phase & MPUCMDP_FOLLOWBYTE) {
				return(SUCCESS);
			}
		}
		else {
			cmd->phase = 0;
			if (data == MIDI_STOP) {
				cm_mpu98->write(cm_mpu98, MIDI_STOP);
				setrecvdata(MIDI_STOP);
				mpu98ii_int();
			}
		}
	}
	else if (cmd->phase & MPUCMDP_FOLLOWBYTE) {
		cmd->phase &= ~MPUCMDP_FOLLOWBYTE;
		setmpucmdgroupe(cmd->cmd, data);
	}
	else {
		cmd->phase = 0;
		return(FAILURE);
	}

	tr_nextsearch();
	return(SUCCESS);
}

static void sendmpudata(REG8 data) {

	if (mpu98.cmd.phase) {
		sendmpucmd(&mpu98.cmd, data);
		return;
	}

	if (mpu98.recvevent & MIDIE_STEP) {
		MPUTR *tr;
		mpu98.recvevent ^= MIDIE_STEP;
		tr = mpu98.tr + (mpu98.intphase - 2);
		tr->datas = 0;
		if (data < 0xf0) {
			mpu98.recvevent ^= MIDIE_EVENT;
			tr->step = data;
		}
		else {
			tr->step = 0xf0;
			tr->remain = 0;
			tr->datas = 0;
			tr_nextsearch();
		}
		return;
	}
	if (mpu98.recvevent & MIDIE_EVENT) {
		MPUTR *tr;
		mpu98.recvevent ^= MIDIE_EVENT;
		mpu98.recvevent |= MIDIE_DATA;
		tr = mpu98.tr + (mpu98.intphase - 2);
		switch(data & 0xf0) {
			case 0xc0:
			case 0xd0:
				tr->remain = 2;
				tr->rstat = data;
				break;

			case 0x80:
			case 0x90:
			case 0xa0:
			case 0xb0:
			case 0xe0:
				tr->remain = 3;
				tr->rstat = data;
				break;

			case 0xf0:
				tr->remain = 1;
				break;

			default:
				tr->data[0] = tr->rstat;
				tr->datas = 1;
				tr->remain = 2;
				if ((tr->rstat & 0xe0) == 0xc0) {
					tr->remain--;
				}
				break;
		}
	}
	if (mpu98.recvevent & MIDIE_DATA) {
		MPUTR *tr;
		tr = mpu98.tr + (mpu98.intphase - 2);
		if (tr->remain) {
			tr->data[tr->datas] = data;
			tr->datas++;
			tr->remain--;
		}
		if (!tr->remain) {
			mpu98.recvevent ^= MIDIE_DATA;
			tr_nextsearch();
		}
		return;
	}

#if 1
	if (mpu98.cond.phase)
#else
	if (mpu98.cond.phase & (MPUCMDP_CMD | MPUCMDP_FOLLOWBYTE))
#endif
	{
		sendmpucond(&mpu98.cond, data);
		return;
	}
}

void IOOUTCALL mpu98ii_o0(UINT port, REG8 dat) {

	UINT	sent;

TRACEOUT(("mpu98ii out %.4x %.2x", port, dat));
	if (cm_mpu98 == NULL) {
		cm_mpu98 = commng_create(COMCREATE_MPU98II);
	}
	if (cm_mpu98->connect != COMCONNECT_OFF) {
		if (mpu98.mode) {
			sent = cm_mpu98->write(cm_mpu98, (UINT8)dat);
		}
		else {
//			TRACEOUT(("send data->%.2x", dat));
			sendmpudata(dat);
			sent = 1;
		}
		if (sent) {
			midiwait(mpu98.xferclock * sent);
		}
	}
	(void)port;
}

void IOOUTCALL mpu98ii_o2(UINT port, REG8 dat) {

TRACEOUT(("mpu98ii out %.4x %.2x", port, dat));
	if (cm_mpu98 == NULL) {
		cm_mpu98 = commng_create(COMCREATE_MPU98II);
	}
	if (cm_mpu98->connect != COMCONNECT_OFF) {
		if (!mpu98.mode) {
			REG8 phase;
//			TRACEOUT(("send cmd->%.2x", dat));
			mpu98.cmd.cmd = dat;
			phase = (*mpucmds[dat])(dat);
			setrecvdata(MPUMSG_ACK);
			mpu98ii_int();
			if (phase & MPUCMDP_REQ) {
				phase &= ~MPUCMDP_REQ;
				reqmpucmdgroupd(dat);
			}
			mpu98.cmd.phase = phase;
		}
		else {
			if (dat == 0xff) {
				mpu98.mode = 0;
				setrecvdata(MPUMSG_ACK);
			}
		}
		midiwait(pccore.realclock / 10000);
	}
	(void)port;
}

REG8 IOINPCALL mpu98ii_i0(UINT port) {
	
	if (cm_mpu98 == NULL) {
		cm_mpu98 = commng_create(COMCREATE_MPU98II);
	}
	if (cm_mpu98->connect != COMCONNECT_OFF) {
		if (mpu98.r.cnt) {
			mpu98.r.cnt--;
#if 0
			if (mpu98.r.cnt) {
				mpu98ii_int();
			}
			else {
				pic_resetirq(mpu98.irqnum);
			}
#endif
			mpu98.data = mpu98.r.buf[mpu98.r.pos];
			mpu98.r.pos = (mpu98.r.pos + 1) & (MPU98_RECVBUFS - 1);
		}
		else if (mpu98.intreq) {
			mpu98.data = mpu98.intreq;
			mpu98.intreq = 0;
		}
		if ((mpu98.r.cnt) || (mpu98.intreq)) {
			mpu98ii_int();
		}
		else {
			pic_resetirq(mpu98.irqnum);
		}

//		TRACEOUT(("recv data->%.2x", mpu98.data));
TRACEOUT(("mpu98ii inp %.4x %.2x", port, mpu98.data));
		return(mpu98.data);
	}
	(void)port;
	return(0xff);
}

REG8 IOINPCALL mpu98ii_i2(UINT port) {

	REG8	ret;
	
	if (cm_mpu98 == NULL) {
		cm_mpu98 = commng_create(COMCREATE_MPU98II);
	}
	if (cm_mpu98->connect != COMCONNECT_OFF || g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16) {
		ret = mpu98.status;
		if ((mpu98.r.cnt == 0) && (mpu98.intreq == 0)) {
			ret |= MIDIIN_AVAIL;
		}
// TRACEOUT(("mpu98ii inp %.4x %.2x", port, ret));
TRACEOUT(("mpu98ii inp %.4x %.2x", port, mpu98.data));
		return(ret);
	}
	(void)port;
	return(0xff);
}


// ---- I/F

void mpu98ii_construct(void) {

	cm_mpu98 = NULL;
}

void mpu98ii_destruct(void) {

	commng_destroy(cm_mpu98);
	cm_mpu98 = NULL;
}

void mpu98ii_reset(const NP2CFG *pConfig) {

	commng_destroy(cm_mpu98);
	cm_mpu98 = NULL;

	ZeroMemory(&mpu98, sizeof(mpu98));
	mpu98.enable = (pConfig->mpuenable ? 1 : 0);
	mpu98.data = MPUMSG_ACK;
	mpu98.port = 0xc0d0 | ((pConfig->mpuopt & 0xf0) << 6);
	mpu98.irqnum = mpuirqnum[pConfig->mpuopt & 3];
	setdefaultcondition();
//	pic_registext(mpu98.irqnum);

	(void)pConfig;
}

void mpu98ii_bind(void) {

	UINT	port;
	
	if(mpu98.enable){
		mpu98ii_changeclock();

		port = mpu98.port;
		iocore_attachout(port, mpu98ii_o0);
		iocore_attachinp(port, mpu98ii_i0);
		//iocore_attachout(port+1, mpu98ii_o2);
		//iocore_attachinp(port+1, mpu98ii_i2);
		//iocore_attachout(port+0x100, mpu98ii_o2);
		//iocore_attachinp(port+0x100, mpu98ii_i2);
		port |= 2;
		iocore_attachout(port, mpu98ii_o2);
		iocore_attachinp(port, mpu98ii_i2);
	
		// PC/AT MPU-401
		if(np2cfg.mpu_at){
			iocore_attachout(0x330, mpu98ii_o0);
			iocore_attachinp(0x330, mpu98ii_i0);
			iocore_attachout(0x331, mpu98ii_o2);
			iocore_attachinp(0x331, mpu98ii_i2);
		}
		// PC-9801-118
		if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
			iocore_attachout(cs4231.port[10], mpu98ii_o0);
			iocore_attachinp(cs4231.port[10], mpu98ii_i0);
			iocore_attachout(cs4231.port[10]+1, mpu98ii_o2);
			iocore_attachinp(cs4231.port[10]+1, mpu98ii_i2);
		}
	}
}

//#define MIDIIN_DEBUG

void mpu98ii_callback(void) {

	UINT8	data;
#ifdef MIDIIN_DEBUG
	static UINT8 testseq[] = {0x90, 0x3C, 0x7F};
	static int testseqpos = 0;
	static DWORD time = 0;
#endif

	if (cm_mpu98) {
		while((mpu98.r.cnt < MPU98_RECVBUFS) &&
			(cm_mpu98->read(cm_mpu98, &data)
#ifdef MIDIIN_DEBUG
			 || (np2cfg.asynccpu)
#endif
			)) {
			if (!mpu98.r.cnt) {
				mpu98ii_int();
			}
#ifdef MIDIIN_DEBUG
			data = testseq[testseqpos];
#endif
			setrecvdata(data);
#ifdef MIDIIN_DEBUG
			if(testseqpos == sizeof(testseq) - 1){
				time = GetTickCount();
				testseq[1] = rand() & 0x7f;
			}
			testseqpos = (testseqpos + 1) % sizeof(testseq);
#endif
		}
	}
}

void mpu98ii_midipanic(void) {

	if (cm_mpu98) {
		cm_mpu98->msg(cm_mpu98, COMMSG_MIDIRESET, 0);
	}
}

void mpu98ii_changeclock(void) {
	
	if(mpu98.enable){
		mpu98.xferclock = pccore.realclock / 3125;
		makeintclock();
	}
}


