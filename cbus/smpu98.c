
// emulation Super MPU

// 参考資料
//   ・Super MPU Developer's Kit for Windows プログラマーズ・ガイド
//   ・https://gist.github.com/o-p-a/8b26f12c36073a3f1f6829d3ed974dd1#file-smpumsg-txt-L93

#include	"compiler.h"

#if defined(SUPPORT_SMPU98)

#include	"commng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"mpu98ii.h"
#include	"smpu98.h"
#include	"fmboard.h"

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

enum {
	SMPUMODE_401		= 0,
	SMPUMODE_UART		= 1,
	SMPUMODE_NATIVE		= 2,
};

// [Native Mode] status のビット
enum {
	SMPU_STATUS_ISR		= 0x0001, // Imm data set ready
	SMPU_STATUS_SSR		= 0x0002, // Seq data set ready
	SMPU_STATUS_MSR		= 0x0004, // Mon data set ready
	SMPU_STATUS_IRR		= 0x0008, // Imm data read ready
	SMPU_STATUS_SRR		= 0x0010, // Seq data read ready
	SMPU_STATUS_RSS		= 0x0020, // Request sequence set
	SMPU_STATUS_RIR		= 0x0040, // Request IMM receive
	SMPU_STATUS_RSR		= 0x0080, // Request SEQ receive
	SMPU_STATUS_RMR		= 0x0100, // Request MON receive
	SMPU_STATUS_IM0		= 0x0200, // Int mask monitor 0001
	SMPU_STATUS_IM1		= 0x0400, // Int mask monitor 0002
	SMPU_STATUS_IM2		= 0x0800, // Int mask monitor 0004
	SMPU_STATUS_IM3		= 0x1000, // Int mask monitor 0008
	SMPU_STATUS_IM4		= 0x2000, // Int mask monitor 0010
};

// [Native Mode] Int mask のビット  0で禁止 1で許可
enum {
	SMPU_INTMASK_RSS		= 0x0001, // RSS 割り込み
	SMPU_INTMASK_RIR		= 0x0002, // RIR 割り込み
	SMPU_INTMASK_RSR		= 0x0004, // RSR 割り込み
	SMPU_INTMASK_RMR		= 0x0008, // RMR 割り込み
	SMPU_INTMASK_401		= 0x0010, // 401 割り込み
	
	SMPU_INTMASK_ALL		= 0x001f,
};

#define SMPU_PORTS	2

	_SMPU98		smpu98;
	COMMNG		cm_smpu98[SMPU_PORTS]; // [0]=A, [1]=B


static const UINT8 mpuirqnum[4] = {3, 5, 6, 12};

static const UINT8 shortmsgleng[0x10] = {
		0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 2, 2, 3, 1};

static const UINT8 hclk_step1[4][4] = {{0, 0, 0, 0}, {1, 0, 0, 0},
									{1, 0, 1, 0}, {1, 1, 1, 0}};


static void makeintclock(void) {

	UINT32	l;
	UINT	curtempo;

	l = smpu98.tempo * 2 * smpu98.reltempo / 0x40;
	if (l < 5 * 2) {
		l = 5 * 2;
	}
	curtempo = l >> 1;
	if (curtempo > 250) {
		curtempo = 250;
	}
	smpu98.curtempo = curtempo;
	if (!(smpu98.flag2 & MPUFLAG2_FSKRESO)) {
		l *= smpu98.inttimebase;							//	*12
	}
	smpu98.stepclock = (pccore.realclock * 5 / l);		//	/12
}

static void sethclk(REG8 data) {

	REG8	quarter;
	int		i;

	quarter = data >> 2;
	if (!quarter) {
		quarter = 64;
	}
	for (i=0; i<4; i++) {
		smpu98.hclk_step[i] = quarter + hclk_step1[data & 3][i];
	}
	smpu98.hclk_rem = 0;
}

static void setdefaultcondition(void) {

	smpu98.recvevent = 0;
	smpu98.remainstep = 0;
	smpu98.intphase = 0;
	smpu98.intreq = 0;

	ZeroMemory(&smpu98.cmd, sizeof(smpu98.cmd));
	ZeroMemory(smpu98.tr, sizeof(smpu98.tr));
	ZeroMemory(&smpu98.cond, sizeof(smpu98.cond));

	smpu98.syncmode = MPUSYNCMODE_INT;
	smpu98.metromode = MPUMETROMODE_OFF;
	smpu98.flag1 = MPUFLAG1_THRU | MPUFLAG1_SENDME;
	smpu98.flag2 = MPUFLAG2_RTAFF;

	smpu98.inttimebase = 120 / 24;
	smpu98.tempo = 100;
	smpu98.reltempo = 0x40;
	makeintclock();
	smpu98.midipermetero = 12;
	smpu98.meteropermeas = 8;
	sethclk(240);
	smpu98.acttr = 0x00;
	smpu98.sendplaycnt = 0;
	smpu98.accch = 0xffff;

	// for S-MPU
	smpu98.native_status = SMPU_STATUS_ISR|SMPU_STATUS_SSR|SMPU_STATUS_MSR;
	smpu98.native_intmask = SMPU_INTMASK_ALL;
	smpu98.mode = SMPUMODE_NATIVE;
}

static void setrecvdata(REG8 data) {

	MPURECV	*r;

	r = &smpu98.r;
	if (r->cnt < MPU98_RECVBUFS) {
		r->buf[(r->pos + r->cnt) & (MPU98_RECVBUFS - 1)] = data;
		r->cnt++;
	}
}

// -------   for S-MPU Native Mode
static void smpu98_writeimm_r(UINT16 dat) {
	if(smpu98.native_immbuf_r_len < SMPU_IMMBUF_R_LEN){
		// IMM READバッファに書き込み処理
		smpu98.native_immbuf_r[smpu98.native_immbuf_r_len] = dat;
		smpu98.native_immbuf_r_len++;
		if(smpu98.native_immbuf_r_len >= SMPU_IMMBUF_R_LEN){
			//smpu98.native_status |= SMPU_STATUS_IRR; // XXX: これで良いのか分からない
		}
		if(smpu98.native_status & SMPU_STATUS_ISR){
			smpu98.native_status &= ~SMPU_STATUS_ISR;
			smpu98.native_status |= SMPU_STATUS_RIR;
		}
	}
}

static void sendmpushortmsg(const UINT8 *dat, UINT count) {

	UINT	i;

#if 0
	if (!(smpu98.flag1 & MPUFLAG1_BENDERTOHOST)) {
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
	if (smpu98.mode == SMPUMODE_NATIVE) {
		for (i=0; i<count; i++) {
			cm_smpu98[smpu98.native_portnum]->write(cm_smpu98[smpu98.native_portnum], dat[i]);
		}
	}else{
		for (i=0; i<count; i++) {
			cm_smpu98[0]->write(cm_smpu98[0], dat[i]);
		}
		if(smpu98.portBready && !smpu98.muteB) {
			for (i=0; i<count; i++) {
				cm_smpu98[1]->write(cm_smpu98[1], dat[i]);
			}
		}
	}
}

static void sendmpulongmsg(const UINT8 *dat, UINT count) {

	UINT	i;
	
	if (smpu98.mode == SMPUMODE_NATIVE) {
		for (i=0; i<count; i++) {
			cm_smpu98[smpu98.native_portnum]->write(cm_smpu98[smpu98.native_portnum], dat[i]);
		}
	}else{
		for (i=0; i<count; i++) {
			cm_smpu98[0]->write(cm_smpu98[0], dat[i]);
		}
		if(smpu98.portBready && !smpu98.muteB) {
			for (i=0; i<count; i++) {
				cm_smpu98[1]->write(cm_smpu98[1], dat[i]);
			}
		}
	}
}

static void sendmpureset(void) {

	UINT	i;
	UINT8	sMessage[3];
	
	if (smpu98.mode == SMPUMODE_NATIVE) {
		UINT8 oldlinenum = smpu98.native_portnum;
		for (i=0; i<0x10; i++) {
			sMessage[0] = (UINT8)(0xb0 + i);
			sMessage[1] = 0x7b;
			sMessage[2] = 0x00;
			smpu98.native_portnum = 0;
			sendmpushortmsg(sMessage, 3);
			smpu98.native_portnum = 1;
			sendmpushortmsg(sMessage, 3);
		}
		smpu98.native_portnum = oldlinenum;
	}else{
		for (i=0; i<0x10; i++) {
			sMessage[0] = (UINT8)(0xb0 + i);
			sMessage[1] = 0x7b;
			sMessage[2] = 0x00;
			sendmpushortmsg(sMessage, 3);
		}
	}
}

static void smpu98_int(void) {

	TRACEOUT(("int!"));
	pic_setirq(smpu98.irqnum);
	
	//if(!mpu98.enable){
	//	// Sound Blaster 16
	//	if(g_nSoundID == SOUNDID_SB16 || g_nSoundID == SOUNDID_PC_9801_86_SB16 || g_nSoundID == SOUNDID_WSS_SB16 || g_nSoundID == SOUNDID_PC_9801_86_WSS_SB16 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
	//		pic_setirq(g_sb16.dmairq);
	//	}
	//	// PC-9801-118
	//	if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
	//		pic_setirq(10);
	//	}
	//	// WaveStar
	//	if(g_nSoundID == SOUNDID_WAVESTAR){
	//		pic_setirq(10);
	//	}
	//}
}

static void tr_step(void) {

	int		i;
	REG8	bit;

	if (smpu98.flag1 & MPUFLAG1_CONDUCTOR) {
		if (smpu98.cond.step) {
			smpu98.cond.step--;
		}
	}
	for (i=0, bit=1; i<8; bit<<=1, i++) {
		if (smpu98.acttr & bit) {
			if (smpu98.tr[i].step) {
				smpu98.tr[i].step--;
			}
		}
	}
}

static BOOL tr_nextsearch(void) {

	int		i;
	REG8	bit;

tr_nextsearch_more:
	if (smpu98.intphase == 1) {
		if (smpu98.flag1 & MPUFLAG1_CONDUCTOR) {
			if (!smpu98.cond.step) {
				smpu98.intreq = MPUMSG_REQCOND;
				smpu98.cond.phase |= MPUCMDP_STEP | MPUCMDP_CMD;
				smpu98_int();
				return(TRUE);
			}
		}
		smpu98.intphase = 2;
	}
	if (smpu98.intphase) {
		bit = 1 << (smpu98.intphase - 2);
		do {
			if (smpu98.acttr & bit) {
				MPUTR *tr;
				tr = smpu98.tr + (smpu98.intphase - 2);
				if (!tr->step) {
					if ((tr->datas) && (tr->remain == 0)) {
						if (cm_smpu98[0] == NULL) {
							cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
						}
						if (cm_smpu98[1] == NULL) {
							cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
							smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
						}
						if (tr->data[0] == MIDI_STOP) {
							tr->datas = 0;
							cm_smpu98[0]->write(cm_smpu98[0], MIDI_STOP);
							if(smpu98.portBready && !smpu98.muteB) cm_smpu98[1]->write(cm_smpu98[1], MIDI_STOP);
							setrecvdata(MIDI_STOP);
							smpu98_int();
							return(TRUE);
						}
						for (i=0; i<tr->datas; i++) {
							cm_smpu98[0]->write(cm_smpu98[0], tr->data[i]);
							if(smpu98.portBready && !smpu98.muteB) cm_smpu98[1]->write(cm_smpu98[1], tr->data[i]);
						}
						tr->datas = 0;
					}
					smpu98.intreq = 0xf0 + (smpu98.intphase - 2);
					smpu98.recvevent |= MIDIE_STEP;
					smpu98_int();
					return(TRUE);
				}
			}
			bit <<= 1;
			smpu98.intphase++;
		} while(smpu98.intphase < 10);
		smpu98.intphase = 0;
	}
	smpu98.remainstep--;
	if (smpu98.remainstep) {
		tr_step();
		smpu98.intphase = 1;
		goto tr_nextsearch_more;
	}
	return(FALSE);
}

void smpu98_midiint(NEVENTITEM item) {

	nevent_set(NEVENT_MIDIINT, smpu98.stepclock, smpu98_midiint, NEVENT_RELATIVE);

	if (smpu98.flag2 & MPUFLAG2_CLKTOHOST) {
		if (!smpu98.hclk_rem) {
			smpu98.hclk_rem = smpu98.hclk_step[smpu98.hclk_cnt & 3];
			smpu98.hclk_cnt++;
		}
		smpu98.hclk_rem--;
		if (!smpu98.hclk_rem) {
			setrecvdata(MPUMSG_HCLK);
			smpu98_int();
		}
	}
	if (smpu98.flag1 & MPUFLAG1_PLAY) {
		if (!smpu98.remainstep++) {
			tr_step();
			smpu98.intphase = 1;
			tr_nextsearch();
		}
	}
	(void)item;
}

void smpu98_midiwaitout(NEVENTITEM item) {

//	TRACE_("midi ready", 0);
	smpu98.status &= ~MIDIOUT_BUSY;
	(void)item;
}

static void midiwait(SINT32 waitclock) {

	if (!nevent_iswork(NEVENT_MIDIWAIT)) {
		smpu98.status |= MIDIOUT_BUSY;
		nevent_set(NEVENT_MIDIWAIT, waitclock, smpu98_midiwaitout, NEVENT_ABSOLUTE);
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
			smpu98.flag1 &= ~MPUFLAG1_PLAY;
			smpu98.recvevent = 0;
			smpu98.intphase = 0;
			smpu98.intreq = 0;
			ZeroMemory(smpu98.tr, sizeof(smpu98.tr));
			ZeroMemory(&smpu98.cond, sizeof(smpu98.cond));
			if (!(smpu98.flag2 & MPUFLAG2_CLKTOHOST)) {
				nevent_reset(NEVENT_MIDIINT);
			}
			break;

		case 2:								// Start Play
			smpu98.flag1 |= MPUFLAG1_PLAY;
			smpu98.remainstep = 0;
			if (!nevent_iswork(NEVENT_MIDIINT)) {
				nevent_set(NEVENT_MIDIINT, smpu98.stepclock,
											smpu98_midiint, NEVENT_ABSOLUTE);
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

	smpu98.mode = SMPUMODE_UART;
	sendmpureset();
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_sr(REG8 cmd) {			// 40-7F: Set ch of Ref table

	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_sm(REG8 cmd) {			// 80-82: Clock Sync/Mode

	smpu98.syncmode = cmd - 0x80;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_mm(REG8 cmd) {			// 83-85: Metronome

	smpu98.metromode = cmd - 0x83;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_8x(REG8 cmd) {			// 86-8F: Flag1

	REG8	bit;

	bit = 1 << ((cmd >> 1) & 7);
	if (cmd & 1) {
		smpu98.flag1 |= bit;
	}
	else {
		smpu98.flag1 &= ~bit;
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
		smpu98.flag2 |= bit;
	}
	else {
		smpu98.flag2 &= ~bit;
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
			if (!(smpu98.flag1 & MPUFLAG1_PLAY)) {
				nevent_reset(NEVENT_MIDIINT);
			}
			break;

		case 0x05:							// 95: CLK to Host / on
			if (!nevent_iswork(NEVENT_MIDIINT)) {
				nevent_set(NEVENT_MIDIINT, smpu98.stepclock,
											smpu98_midiint, NEVENT_ABSOLUTE);
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

	smpu98.reltempo = 0x40;
	makeintclock();
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_b8(REG8 cmd) {			// B8: Clear PC

	int		i;

	for (i=0; i<8; i++) {
		smpu98.tr[i].step = 0;
	}
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_tb(REG8 cmd) {			// C2-C8: .INT Time Base

	smpu98.inttimebase = cmd & 0x0f;
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

static REG8 mpucmd_fe(REG8 cmd) {			// FE: Switch to Native Mode

	smpu98.mode = SMPUMODE_NATIVE;
	sendmpureset();
	(void)cmd;
	return(MPUCMDP_IDLE);
}

static REG8 mpucmd_ff(REG8 cmd) {			// FF: Reset

	sendmpureset();
	nevent_reset(NEVENT_MIDIINT);
	setdefaultcondition();
	smpu98.mode = SMPUMODE_401;
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
	mpucmd_xx,		mpucmd_xx,		mpucmd_fe,		mpucmd_ff};


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
			setrecvdata(smpu98.tr[cmd - 0xa0].step);
			break;

		case 0xab:						// AB: Read & Clear RC
			setrecvdata(0);
			break;

		case 0xac:						// AC: Req Major Version
			setrecvdata(0x20);
			break;

		case 0xad:						// AD: Req Minor Version
			setrecvdata(0);
			break;

		case 0xaf:						// AF: Req Tempo
			setrecvdata(smpu98.curtempo);
			break;
	}
}

static void setmpucmdgroupe(REG8 cmd, REG8 data) {

	switch(cmd) {
		case 0xe0:				// Set Tempo
			smpu98.tempo = data;
			smpu98.reltempo = 0x40;
			makeintclock();
			break;

		case 0xe1:				// Rel Tempo
			smpu98.reltempo = data;
			makeintclock();
			break;

		case 0xe2:				// Graduation
			break;

		case 0xe4:				// MIDI/Metro
			smpu98.midipermetero = data;
			break;

		case 0xe6:				// Metro/Meas
			smpu98.meteropermeas = data;
			break;

		case 0xe7:				// INTx4/H.CLK
			sethclk(data);
			break;

		case 0xec:				// Act Tr
			smpu98.acttr = data;
			break;

		case 0xed:				// Send Play CNT
			smpu98.sendplaycnt = data;
			break;

		case 0xee:				// Acc CH 1-8
			smpu98.accch = (smpu98.accch & 0xff00) | data;
			break;

		case 0xef:				// Acc CH 9-16
			smpu98.accch = (smpu98.accch & 0x00ff) | (data << 8);
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
//		if (smpu98.intreq == 0xf9) {
//			smpu98.intreq = 0;
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
				cm_smpu98[0]->write(cm_smpu98[0], MIDI_STOP);
				if(smpu98.portBready && !smpu98.muteB) cm_smpu98[1]->write(cm_smpu98[1], MIDI_STOP);
				setrecvdata(MIDI_STOP);
				smpu98_int();
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

	if (smpu98.cmd.phase) {
		sendmpucmd(&smpu98.cmd, data);
		return;
	}

	if (smpu98.recvevent & MIDIE_STEP) {
		MPUTR *tr;
		smpu98.recvevent ^= MIDIE_STEP;
		tr = smpu98.tr + (smpu98.intphase - 2);
		tr->datas = 0;
		if (data < 0xf0) {
			smpu98.recvevent ^= MIDIE_EVENT;
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
	if (smpu98.recvevent & MIDIE_EVENT) {
		MPUTR *tr;
		smpu98.recvevent ^= MIDIE_EVENT;
		smpu98.recvevent |= MIDIE_DATA;
		tr = smpu98.tr + (smpu98.intphase - 2);
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
	if (smpu98.recvevent & MIDIE_DATA) {
		MPUTR *tr;
		tr = smpu98.tr + (smpu98.intphase - 2);
		if (tr->remain) {
			tr->data[tr->datas] = data;
			tr->datas++;
			tr->remain--;
		}
		if (!tr->remain) {
			smpu98.recvevent ^= MIDIE_DATA;
			tr_nextsearch();
		}
		return;
	}

#if 1
	if (smpu98.cond.phase)
#else
	if (smpu98.cond.phase & (MPUCMDP_CMD | MPUCMDP_FOLLOWBYTE))
#endif
	{
		sendmpucond(&smpu98.cond, data);
		return;
	}
}

// [401 Mode] Data
void IOOUTCALL smpu98_o0(UINT port, REG8 dat) {

	UINT	sent;

TRACEOUT(("smpu98 out %.4x %.2x", port, dat));
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF) {
		if (smpu98.mode == SMPUMODE_UART) {
			sent = cm_smpu98[0]->write(cm_smpu98[0], (UINT8)dat);
			if(smpu98.portBready && !smpu98.muteB) cm_smpu98[1]->write(cm_smpu98[1], (UINT8)dat);
		}
		else if(smpu98.mode == SMPUMODE_NATIVE){
			sent = 0;
		}
		else {
//			TRACEOUT(("send data->%.2x", dat));
			sendmpudata(dat);
			sent = 1;
		}
		if (sent) {
			midiwait(smpu98.xferclock * sent);
		}
	}
	(void)port;
}

// [401 Mode] Command
void IOOUTCALL smpu98_o2(UINT port, REG8 dat) {

TRACEOUT(("smpu98 out %.4x %.2x", port, dat));
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF) {
		if (smpu98.mode != SMPUMODE_UART) {
			REG8 phase;
//			TRACEOUT(("send cmd->%.2x", dat));
			smpu98.cmd.cmd = dat;
			phase = (*mpucmds[dat])(dat);
			setrecvdata(MPUMSG_ACK);
			smpu98_int();
			if (phase & MPUCMDP_REQ) {
				phase &= ~MPUCMDP_REQ;
				reqmpucmdgroupd(dat);
			}
			smpu98.cmd.phase = phase;
		}
		else {
			if (dat == 0xff) {
				smpu98.mode = SMPUMODE_401;
				setrecvdata(MPUMSG_ACK);
			}
		}
		midiwait(pccore.realclock / 10000);
	}
	(void)port;
}

// [401 Mode] Data
REG8 IOINPCALL smpu98_i0(UINT port) {
	
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF) {
		if (smpu98.r.cnt) {
			smpu98.r.cnt--;
#if 0
			if (smpu98.r.cnt) {
				smpu98_int();
			}
			else {
				pic_resetirq(smpu98.irqnum);
			}
#endif
			smpu98.data = smpu98.r.buf[smpu98.r.pos];
			smpu98.r.pos = (smpu98.r.pos + 1) & (MPU98_RECVBUFS - 1);
		}
		else if (smpu98.intreq) {
			smpu98.data = smpu98.intreq;
			smpu98.intreq = 0;
		}
		if ((smpu98.r.cnt) || (smpu98.intreq)) {
			smpu98_int();
		}
		else {
			pic_resetirq(smpu98.irqnum);
		}

//		TRACEOUT(("recv data->%.2x", smpu98.data));
TRACEOUT(("smpu98 inp %.4x %.2x", port, smpu98.data));
		return(smpu98.data);
	}
	(void)port;
	return(0xff);
}

// [401 Mode] Status
REG8 IOINPCALL smpu98_i2(UINT port) {

	REG8	ret;
	
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if ((cm_smpu98[0]->connect != COMCONNECT_OFF) || g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16) {

		ret = smpu98.status;
		if ((smpu98.r.cnt == 0) && (smpu98.intreq == 0)) {
			ret |= MIDIIN_AVAIL;
		}
// TRACEOUT(("smpu98 inp %.4x %.2x", port, ret));
		return(ret);
	}
	(void)port;
	return(0xff);
}

// -------   for S-MPU Native Mode

// [Native Mode] Immediate Message/Function
void IOOUTCALL smpu98_o4(UINT port, UINT16 dat) {

TRACEOUT(("smpu98 out %.4x %.2x", port, dat));
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF && cm_smpu98[1]->connect != COMCONNECT_OFF) {
		if((smpu98.native_status & SMPU_STATUS_IRR) == 0){
			if(smpu98.native_immbuf_w_len < SMPU_IMMBUF_W_LEN){
				// IMM書き込み処理
				smpu98.native_immbuf_w[smpu98.native_immbuf_w_len] = dat;
				smpu98.native_immbuf_w_len++;
				if(smpu98.native_immbuf_w_len >= SMPU_IMMBUF_W_LEN){
					//smpu98.native_status |= SMPU_STATUS_IRR; // XXX: これで良いのか分からない
				}
			}
		}
	}
	(void)port;
}

// [Native Mode] Sequence Message/Function
void IOOUTCALL smpu98_o6(UINT port, UINT16 dat) {

TRACEOUT(("smpu98 out %.4x %.2x", port, dat));
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF && cm_smpu98[1]->connect != COMCONNECT_OFF) {
		if((smpu98.native_status & SMPU_STATUS_SRR) == 0){
			if(smpu98.native_seqbuf_w_len < SMPU_SEQBUF_W_LEN){
				// SEQ書き込み処理
				smpu98.native_seqbuf_w[smpu98.native_seqbuf_w_len] = dat;
				smpu98.native_seqbuf_w_len++;
				if(smpu98.native_seqbuf_w_len >= SMPU_SEQBUF_W_LEN){
					//smpu98.native_status |= SMPU_STATUS_SRR; // XXX: これで良いのか分からない
				}
			}
		}
	}
	(void)port;
}

// [Native Mode] Interrupt to Super MPU
void IOOUTCALL smpu98_o8(UINT port, UINT16 dat) {

	UINT16 sent = 0;

TRACEOUT(("smpu98 out %.4x %.2x", port, dat));
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF && cm_smpu98[1]->connect != COMCONNECT_OFF) {
		UINT8 *buf = smpu98.native_tmpbuf;
		switch(dat){
		case 1:
			// IMM処理
			while(smpu98.native_immbuf_w_pos < smpu98.native_immbuf_w_len){
				UINT16 w1 = smpu98.native_immbuf_w[smpu98.native_immbuf_w_pos];
				UINT8 msg = ((w1 >> 8) & 0xff);
				if(msg & 0x80){
					smpu98.native_lastmsg = msg;
					if(msg==0xf7){
						// F7(End of Exclusive)だけ送る
						sent += cm_smpu98[smpu98.native_portnum]->write(cm_smpu98[smpu98.native_portnum], (w1 >> 8) & 0xff);
						smpu98.native_lastmsg = smpu98.native_runningmsg;
					}else if(msg==0xef){
						// Continue Exclusive
						smpu98.native_linenum = (w1 & 0xff);
						smpu98.native_portnum = (smpu98.native_linenum > 0xf ? 1 : 0);
						smpu98.native_lastmsg = 0xf0;
					}else if(0xf0 <= msg && msg <= 0xff){
						// ラインナンバーを削って送る
						smpu98.native_linenum = (w1 & 0xff);
						smpu98.native_portnum = (smpu98.native_linenum > 0xf ? 1 : 0);
						sent += cm_smpu98[smpu98.native_portnum]->write(cm_smpu98[smpu98.native_portnum], (w1 >> 8) & 0xff);
					}else if(msg & 0x0f){
						// とりあえず無視
						if(smpu98.native_lastmsg==0xf0){
							// タイムスタンプとかを間に挟めるらしい。Continue Exclusiveが送られてきたらExclusive送信再開
						}
					}else{
						// chを入れて送る
						smpu98.native_linenum = (w1 & 0xff);
						smpu98.native_portnum = (smpu98.native_linenum > 0xf ? 1 : 0);
						sent += cm_smpu98[smpu98.native_portnum]->write(cm_smpu98[smpu98.native_portnum], (w1 >> 8) & 0xf0 | (smpu98.native_linenum & 0xf));
						smpu98.native_runningmsg = msg;
					}
				}else{
					msg = smpu98.native_lastmsg; // 直近のメッセージの続き
					if(!(msg==0xf7) && !(0xf0 <= msg && msg <= 0xff || msg==0xef) && (msg & 0x0f)){
						// とりあえず無視
					}else{
						switch(msg){
						case 0xc0:
						case 0xd0:
							// 1byte目だけ送る
							sent += cm_smpu98[smpu98.native_portnum]->write(cm_smpu98[smpu98.native_portnum], (w1 >> 8) & 0xff);
							break;
						default:
							// そのまま送る
							sent += cm_smpu98[smpu98.native_portnum]->write(cm_smpu98[smpu98.native_portnum], (w1 >> 8) & 0xff);
							sent += cm_smpu98[smpu98.native_portnum]->write(cm_smpu98[smpu98.native_portnum], (w1     ) & 0xff);
							if((w1 & 0xff) == 0xf7){
								smpu98.native_lastmsg = smpu98.native_runningmsg;
							}
						}
					}
				}
				smpu98.native_immbuf_w_pos++;
			}
			smpu98.native_immbuf_w_pos = smpu98.native_immbuf_w_len = 0;
			break;
		case 2:
			// TODO: SEQ処理
			smpu98.native_seqbuf_w_pos = smpu98.native_seqbuf_w_len = 0;
			break;
		}
		if (sent) {
			//midiwait(smpu98.xferclock * sent);
		}
	}
	(void)port;
}

// [Native Mode] Interrupt Mask
void IOOUTCALL smpu98_oa(UINT port, UINT16 dat) {

TRACEOUT(("smpu98 out %.4x %.2x", port, dat));
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF && cm_smpu98[1]->connect != COMCONNECT_OFF) {
		smpu98.native_intmask = dat;
	}
	(void)port;
}

// [Native Mode] Immediate Message/Function
UINT16 IOINPCALL smpu98_i4(UINT port) {

	UINT16	ret = 0xffff;
	
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF && cm_smpu98[1]->connect != COMCONNECT_OFF) {
		if((smpu98.native_status & SMPU_STATUS_ISR) == 0){
			ret = smpu98.native_immbuf_r[smpu98.native_immbuf_r_pos];
			smpu98.native_immbuf_r_pos++;
			if(smpu98.native_immbuf_r_pos >= smpu98.native_immbuf_r_len){
				smpu98.native_status |= SMPU_STATUS_ISR;
				smpu98.native_immbuf_r_pos = smpu98.native_immbuf_r_len = 0;
				//smpu98.native_status &= ~SMPU_STATUS_IRR; // XXX: これで良いのか分からない
			}
			smpu98.native_status &= ~SMPU_STATUS_RIR;
		}
// TRACEOUT(("smpu98 inp %.4x %.2x", port, ret));
		return(ret);
	}
	(void)port;
	return(0xffff);
}

// [Native Mode] Sequence Message/Function
UINT16 IOINPCALL smpu98_i6(UINT port) {

	UINT16	ret = 0xffff;
	
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF && cm_smpu98[1]->connect != COMCONNECT_OFF) {
		if((smpu98.native_status & SMPU_STATUS_SSR) == 0){
			ret = smpu98.native_seqbuf_r[smpu98.native_seqbuf_r_pos];
			smpu98.native_seqbuf_r_pos++;
			if(smpu98.native_seqbuf_r_pos >= smpu98.native_seqbuf_r_len){
				smpu98.native_status |= SMPU_STATUS_SSR;
				smpu98.native_seqbuf_r_pos = smpu98.native_seqbuf_r_len = 0;
				//smpu98.native_status &= ~SMPU_STATUS_SRR; // XXX: これで良いのか分からない
			}
			smpu98.native_status &= ~SMPU_STATUS_RSR;
		}
// TRACEOUT(("smpu98 inp %.4x %.2x", port, ret));
		return(ret);
	}
	(void)port;
	return(0xffff);
}

// [Native Mode] Monitor
UINT16 IOINPCALL smpu98_i8(UINT port) {

	UINT16	ret = 0xffff;
	
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF && cm_smpu98[1]->connect != COMCONNECT_OFF) {
		if((smpu98.native_status & SMPU_STATUS_MSR) == 0){
			ret = smpu98.native_monbuf[smpu98.native_monbuf_pos];
			smpu98.native_monbuf_pos++;
			if(smpu98.native_monbuf_pos >= smpu98.native_monbuf_len){
				smpu98.native_status |= SMPU_STATUS_MSR;
				smpu98.native_monbuf_pos = smpu98.native_monbuf_len = 0;
			}
			smpu98.native_status &= ~SMPU_STATUS_RMR;
		}

// TRACEOUT(("smpu98 inp %.4x %.2x", port, ret));
		return(ret);
	}
	(void)port;
	return(0xffff);
}

// [Native Mode] Status
UINT16 IOINPCALL smpu98_ia(UINT port) {

	UINT16	ret;
	
	if (cm_smpu98[0] == NULL) {
		cm_smpu98[0] = commng_create(COMCREATE_SMPU98_A);
	}
	if (cm_smpu98[1] == NULL) {
		cm_smpu98[1] = commng_create(COMCREATE_SMPU98_B);
		smpu98.portBready = (cm_smpu98[1]->connect != COMCONNECT_OFF);
	}
	if (cm_smpu98[0]->connect != COMCONNECT_OFF && cm_smpu98[1]->connect != COMCONNECT_OFF) {
		ret = smpu98.native_status & ~(SMPU_INTMASK_ALL << 9) | (smpu98.native_intmask << 9);
		//if ((smpu98.r.cnt == 0) && (smpu98.intreq == 0)) {
		//	ret |= MIDIIN_AVAIL;
		//}
// TRACEOUT(("smpu98 inp %.4x %.2x", port, ret));
		return(ret);
	}
	(void)port;
	return(0xffff);
}

// 16-bit I/O func
void (IOOUTCALL *smpu98_io16outfunc[])(UINT port, UINT16 dat) = {
	NULL,			NULL, NULL,			NULL, 
	smpu98_o4,		NULL, smpu98_o6,	NULL, 
	smpu98_o8,		NULL, smpu98_oa,	NULL,
	NULL,			NULL, NULL,			NULL, 
};
UINT16 (IOINPCALL *smpu98_io16inpfunc[])(UINT port) = {
	NULL,			NULL, NULL,			NULL, 
	smpu98_i4,		NULL, smpu98_i6,	NULL, 
	smpu98_i8,		NULL, smpu98_ia,	 NULL,
	NULL,			NULL, NULL,			NULL, 
};

// ---- I/F

void smpu98_construct(void) {

	cm_smpu98[0] = NULL;
	cm_smpu98[1] = NULL;
	smpu98.portBready = 0;
}

void smpu98_destruct(void) {

	commng_destroy(cm_smpu98[0]);
	commng_destroy(cm_smpu98[1]);
	cm_smpu98[0] = NULL;
	cm_smpu98[1] = NULL;
	smpu98.portBready = 0;
}

void smpu98_reset(const NP2CFG *pConfig) {

	commng_destroy(cm_smpu98[0]);
	commng_destroy(cm_smpu98[1]);
	cm_smpu98[0] = NULL;
	cm_smpu98[1] = NULL;

	ZeroMemory(&smpu98, sizeof(smpu98));
	smpu98.portBready = 0;
	smpu98.enable = (pConfig->smpuenable ? 1 : 0);
	smpu98.muteB = (pConfig->smpumuteB ? 1 : 0);
	smpu98.data = MPUMSG_ACK;
	smpu98.port = 0xc0d0 | ((pConfig->smpuopt & 0xf0) << 6);
	smpu98.irqnum = mpuirqnum[pConfig->smpuopt & 3];
	setdefaultcondition();
//	pic_registext(smpu98.irqnum);

	(void)pConfig;
}

void smpu98_bind(void) {

	UINT	port;
	
	if(smpu98.enable){
		smpu98_changeclock();

		port = smpu98.port;
		iocore_attachout(port, smpu98_o0);
		iocore_attachinp(port, smpu98_i0);
		port += 2;
		iocore_attachout(port, smpu98_o2);
		iocore_attachinp(port, smpu98_i2);

		// for S-MPU Native Mode (16-bit I/Oポートなのでiocore.cで処理)
		//port += 2;
		//iocore_attachout(port, smpu98_o4);
		//iocore_attachinp(port, smpu98_i4);
		//port += 2;
		//iocore_attachout(port, smpu98_o6);
		//iocore_attachinp(port, smpu98_i6);
		//port += 2;
		//iocore_attachout(port, smpu98_o8);
		//iocore_attachinp(port, smpu98_i8);
		//port += 2;
		//iocore_attachout(port, smpu98_oa);
		//iocore_attachinp(port, smpu98_ia);

		if(!mpu98.enable){
			// PC/AT MPU-401
			if(np2cfg.mpu_at){
				iocore_attachout(0x330, smpu98_o0);
				iocore_attachinp(0x330, smpu98_i0);
				iocore_attachout(0x331, smpu98_o2);
				iocore_attachinp(0x331, smpu98_i2);
			}
			// PC-9801-118
			if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
				iocore_attachout(cs4231.port[10], smpu98_o0);
				iocore_attachinp(cs4231.port[10], smpu98_i0);
				iocore_attachout(cs4231.port[10]+1, smpu98_o2);
				iocore_attachinp(cs4231.port[10]+1, smpu98_i2);
			}
		}
	}else if(!mpu98.enable){
		// MPU-PC98IIもS-MPUも無効の時

		// PC-9801-118
		if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
			iocore_attachout(cs4231.port[10], smpu98_o0);
			iocore_attachinp(cs4231.port[10], smpu98_i0);
			iocore_attachout(cs4231.port[10]+1, smpu98_o2);
			iocore_attachinp(cs4231.port[10]+1, smpu98_i2);
			// NULLで作っておく
			cm_smpu98[0] = commng_create(COMCREATE_NULL);
			cm_smpu98[1] = commng_create(COMCREATE_NULL);
		}
	}
}

static void smpu98_callback_port(UINT8 dat, UINT8 port) {
	if(smpu98.native_immread_phase[port] == 0){
		// 1byte目
		if(dat & 0x80){
			smpu98.native_immread_lastmsg[port] = dat;
			smpu98.native_immread_portbuf[port] = ((UINT16)dat) << 8;
			if(dat==0xf7){
				// F7(End of Exclusive)
				smpu98_writeimm_r(smpu98.native_immread_portbuf[port]);
			}else if(0xf0 <= dat && dat <= 0xff){
				// ラインナンバーを入れる
				smpu98.native_immread_portbuf[port] |= (port==0 ? 0x00 : 0x10);
				smpu98_writeimm_r(smpu98.native_immread_portbuf[port]);
			}else{
				// chをラインナンバーに変換
				smpu98.native_immread_portbuf[port] &= 0xf000; // chを消す
				smpu98.native_immread_portbuf[port] |= (port==0 ? 0x00 : 0x10) | (dat & 0x0f); // ch→ラインナンバー変換
				smpu98_writeimm_r(smpu98.native_immread_portbuf[port]);
			}
		}else{
			UINT8 oldmsg = smpu98.native_immread_lastmsg[port]; // 直近のメッセージの続き
			smpu98.native_immread_portbuf[port] = ((UINT16)dat) << 8;
			switch(oldmsg){
			case 0xc0:
			case 0xd0:
				// 1byteだけ（2byte目は0で）
				smpu98_writeimm_r(smpu98.native_immread_portbuf[port]);
				break;
			default:
				// 2byte目を待つ
				smpu98.native_immread_phase[port] = 1;
			}
		}
	}else{
		// 2byte目
		smpu98.native_immread_portbuf[port] |= ((UINT16)dat);
		smpu98_writeimm_r(smpu98.native_immread_portbuf[port]);
		smpu98.native_immread_phase[port] = 0;
	}
}

void smpu98_callback(void) {

	UINT8	data;
	
	if (smpu98.mode == SMPUMODE_NATIVE) {
		if (cm_smpu98[0]) {
			while((SMPU_IMMBUF_R_LEN < smpu98.native_immbuf_r_len) && (cm_smpu98[0]->read(cm_smpu98[0], &data))) {
				smpu98_callback_port(data, 0);
				// XXX: 401にも送っておく
				if (!smpu98.r.cnt) {
					smpu98_int();
				}
				setrecvdata(data);
			}
		}
		if (cm_smpu98[1]) {
			while((SMPU_IMMBUF_R_LEN < smpu98.native_immbuf_r_len) && (cm_smpu98[1]->read(cm_smpu98[1], &data))) {
				smpu98_callback_port(data, 0);
			}
		}
	}else{
		if (cm_smpu98[0]) {
			while((smpu98.r.cnt < MPU98_RECVBUFS) &&
				(cm_smpu98[0]->read(cm_smpu98[0], &data))) {
				if (!smpu98.r.cnt) {
					smpu98_int();
				}
				setrecvdata(data);
			}
		}
	}
}

void smpu98_midipanic(void) {

	if (cm_smpu98[0]) {
		cm_smpu98[0]->msg(cm_smpu98[0], COMMSG_MIDIRESET, 0);
	}
	if (cm_smpu98[1] && smpu98.portBready) {
		cm_smpu98[1]->msg(cm_smpu98[1], COMMSG_MIDIRESET, 0);
	}
}

void smpu98_changeclock(void) {
	
	smpu98.xferclock = pccore.realclock / 3125;
	makeintclock();
}

#endif

