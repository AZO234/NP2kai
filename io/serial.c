#include	"compiler.h"
#include	"cpucore.h"
#include	"commng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"keystat.h"


// ---- Keyboard

void keyboard_callback(NEVENTITEM item) {

	if (item->flag & NEVENT_SETEVENT) {
		if ((keybrd.ctrls) || (keybrd.buffers)) {
			if (!(keybrd.status & 2)) {
				keybrd.status |= 2;
				if (keybrd.ctrls) {
					keybrd.ctrls--;
					keybrd.data = keybrd.ctr[keybrd.ctrpos];
					keybrd.ctrpos = (keybrd.ctrpos + 1) & KB_CTRMASK;
				}
				else if (keybrd.buffers) {
					keybrd.buffers--;
					keybrd.data = keybrd.buf[keybrd.bufpos];
					keybrd.bufpos = (keybrd.bufpos + 1) & KB_BUFMASK;
				}
//				TRACEOUT(("recv -> %02x", keybrd.data));
			}
			pic_setirq(1);
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_RELATIVE);
		}
	}
}

static void IOOUTCALL keyboard_o41(UINT port, REG8 dat) {

	if (keybrd.cmd & 1) {
//		TRACEOUT(("send -> %02x", dat));
		keystat_ctrlsend(dat);
	}
	else {
		keybrd.mode = dat;
	}
	(void)port;
}

static void IOOUTCALL keyboard_o43(UINT port, REG8 dat) {

//	TRACEOUT(("out43 -> %02x %.4x:%.8x", dat, CPU_CS, CPU_EIP));
	if ((!(dat & 0x08)) && (keybrd.cmd & 0x08)) {
		keyboard_resetsignal();
	}
	if (dat & 0x10) {
		keybrd.status &= ~(0x38);
	}
	keybrd.cmd = dat;
	(void)port;
}

static REG8 IOINPCALL keyboard_i41(UINT port) {

	(void)port;
	keybrd.status &= ~2;
	pic_resetirq(1);
//	TRACEOUT(("in41 -> %02x %.4x:%.8x", keybrd.data, CPU_CS, CPU_EIP));
	return(keybrd.data);
}

static REG8 IOINPCALL keyboard_i43(UINT port) {

	(void)port;
//	TRACEOUT(("in43 -> %02x %.4x:%.8x", keybrd.status, CPU_CS, CPU_EIP));
	return(keybrd.status | 0x85);
}


// ----

static const IOOUT keybrdo41[2] = {
					keyboard_o41,	keyboard_o43};

static const IOINP keybrdi41[2] = {
					keyboard_i41,	keyboard_i43};


void keyboard_reset(const NP2CFG *pConfig) {

	ZeroMemory(&keybrd, sizeof(keybrd));
	keybrd.data = 0xff;
	keybrd.mode = 0x5e;

	(void)pConfig;
}

void keyboard_bind(void) {

	keystat_ctrlreset();
	keyboard_changeclock();
	iocore_attachsysoutex(0x0041, 0x0cf1, keybrdo41, 2);
	iocore_attachsysinpex(0x0041, 0x0cf1, keybrdi41, 2);
}

void keyboard_resetsignal(void) {

	nevent_reset(NEVENT_KEYBOARD);
	keybrd.cmd = 0;
	keybrd.status = 0;
	keybrd.ctrls = 0;
	keybrd.buffers = 0;
	keystat_ctrlreset();
	keystat_resendstat();
}

void keyboard_ctrl(REG8 data) {

	if ((data == 0xfa) || (data == 0xfc)) {
		keybrd.ctrls = 0;
	}
	if (keybrd.ctrls < KB_CTR) {
		keybrd.ctr[(keybrd.ctrpos + keybrd.ctrls) & KB_CTRMASK] = data;
		keybrd.ctrls++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_ABSOLUTE);
		}
	}
}

void keyboard_send(REG8 data) {

	if (keybrd.buffers < KB_BUF) {
		keybrd.buf[(keybrd.bufpos + keybrd.buffers) & KB_BUFMASK] = data;
		keybrd.buffers++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_ABSOLUTE);
		}
	}
	else {
		keybrd.status |= 0x10;
	}
}

void keyboard_changeclock(void) {

	keybrd.xferclock = pccore.realclock / 1920;
}



// ---- RS-232C

	COMMNG	cm_rs232c;
	
#define RS232C_BUFFER		(1 << 6)
#define RS232C_BUFFER_MASK	(RS232C_BUFFER - 1)
#define RS232C_BUFFER_CLRC	16
static UINT8 rs232c_buf[RS232C_BUFFER];
static UINT8 rs232c_buf_rpos = 0;
static UINT8 rs232c_buf_wpos = 0;
static int rs232c_removecounter = 0;

static void rs232c_writeretry() {

	int ret;
	if((rs232c.result & 0x1) != 0) return;
	if (cm_rs232c) {
		cm_rs232c->writeretry(cm_rs232c);
		ret = cm_rs232c->lastwritesuccess(cm_rs232c);
		if(ret==0){
			return; // 書き込み無視
		}
		rs232c.result |= 0x5;
	}
	if (sysport.c & 4) {
		rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
		rs232cfifo.irqflag = 1;
#endif
		pic_setirq(4);
	}
	else {
		rs232c.send = 1;
	}
}

void rs232c_construct(void) {

	cm_rs232c = NULL;
}

void rs232c_destruct(void) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
}

void rs232c_open(void) {

	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL);
#if defined(VAEG_FIX)
		cm_rs232c->msg(cm_rs232c, COMMSG_SETRSFLAG, rs232c.cmd & 0x22); /* RTS, DTR */
#endif
	}
}

void rs232c_callback(void) {

	BOOL	intr;
	
	int bufused = (rs232c_buf_wpos - rs232c_buf_rpos) & RS232C_BUFFER_MASK;
	if(bufused == 0){
		rs232c_removecounter = 0;
	}

	rs232c_writeretry();

#if defined(SUPPORT_RS232C_FIFO)
	if(rs232cfifo.port138 & 0x1){
		rs232c_removecounter = 0; // FIFOモードでは消さない
		if(bufused == RS232C_BUFFER-1){
			return; // バッファがいっぱいなら待機
		}
		if(rs232cfifo.irqflag){
			return; // 割り込み原因フラグが立っていれば待機
		}
	}
#endif
	//if(rs232c.result & 2) {
	//	return;
	//}

	intr = FALSE;
	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL);
	}
	rs232c_removecounter = (rs232c_removecounter + 1) % RS232C_BUFFER_CLRC;
	if (bufused > 0 && rs232c_removecounter==0 || bufused == RS232C_BUFFER-1){
		rs232c_buf_rpos = (rs232c_buf_rpos+1) & RS232C_BUFFER_MASK; // 一番古いものを捨てる
	}
	if ((cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c_buf[rs232c_buf_wpos]))) {
		rs232c_buf_wpos = (rs232c_buf_wpos+1) & RS232C_BUFFER_MASK;
	}
	if (rs232c_buf_rpos != rs232c_buf_wpos) {
		rs232c.data = rs232c_buf[rs232c_buf_rpos]; // データを1つ取り出し
		//if(!(rs232c.result & 2) || bufused == RS232C_BUFFER-1) {
			rs232c.result |= 2;
#if defined(SUPPORT_RS232C_FIFO)
			if(rs232cfifo.port138 & 0x1){
				rs232cfifo.irqflag = 2;
				//OutputDebugString(_T("READ INT!¥n"));
				intr = TRUE;
			}else
#endif
			if (sysport.c & 1) {
				intr = TRUE;
			}
		//}
	}
	else {
		//rs232c.result &= (UINT8)~2;
	}
	if (sysport.c & 4) {
		if (rs232c.send) {
			rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
			rs232cfifo.irqflag = 1;
#endif
			intr = TRUE;
		}
	}
	if (intr) {
		pic_setirq(4);
	}
}

UINT8 rs232c_stat(void) {

#if !defined(NP2_X11) && !defined(NP2_SDL2) && !defined(__LIBRETRO__)
	if (cm_rs232c == NULL) {
#if defined(VAEG_FIX)
		rs232c_open();
#else
		cm_rs232c = commng_create(COMCREATE_SERIAL);
#endif
		return(cm_rs232c->getstat(cm_rs232c));
	}
#endif

	return 0;
}

void rs232c_midipanic(void) {

	if (cm_rs232c) {
		cm_rs232c->msg(cm_rs232c, COMMSG_MIDIRESET, 0);
	}
}


// ----

static void IOOUTCALL rs232c_o30(UINT port, REG8 dat) {

	int ret;
	if (cm_rs232c) {
		rs232c_writeretry();
		cm_rs232c->write(cm_rs232c, (UINT8)dat);
#if !defined(__LIBRETRO__)
		ret = cm_rs232c->lastwritesuccess(cm_rs232c);

		if(!ret){
			rs232c.result &= ~0x5;
			return; // まだ書き込めないので待つ
		}
#endif
		rs232c.result |= 0x5;
	}
	if (sysport.c & 4) {
		rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
		rs232cfifo.irqflag = 1;
#endif
		pic_setirq(4);
	}
	else {
		rs232c.send = 1;
	}
	(void)port;
}

static void IOOUTCALL rs232c_o32(UINT port, REG8 dat) {

	if (!(dat & 0xfd)) {
		rs232c.dummyinst++;
	}
	else {
		if ((rs232c.dummyinst >= 3) && (dat == 0x40)) {
			rs232c.pos = 0;
		}
		rs232c.dummyinst = 0;
	}
	switch(rs232c.pos) {
		case 0x00:			// reset
			rs232c.pos++;
			break;

		case 0x01:			// mode
			rs232c.rawmode = dat;
			if (!(dat & 0x03)) {
				rs232c.mul = 10 * 16;
			}
			else {
				rs232c.mul = ((dat >> 1) & 6) + 10;
				if (dat & 0x10) {
					rs232c.mul += 2;
				}
				switch(dat & 0xc0) {
					case 0x80:
						rs232c.mul += 3;
						break;
					case 0xc0:
						rs232c.mul += 4;
						break;
					default:
						rs232c.mul += 2;
						break;
				}
				switch(dat & 0x03) {
					case 0x01:
						rs232c.mul >>= 1;
						break;
					case 0x03:
						rs232c.mul *= 32;
						break;
					default:
						rs232c.mul *= 8;
						break;
				}
			}
			pit_setrs232cspeed((pit.ch + 2)->value);
			rs232c.pos++;
			break;

		case 0x02:			// cmd
#if defined(VAEG_FIX)
			rs232c.cmd = dat;
			if (cm_rs232c) {
				cm_rs232c->msg(cm_rs232c, COMMSG_SETRSFLAG, dat & 0x22); /* RTS, DTR */
			}
#else
			sysport.c &= ~7;
			sysport.c |= (dat & 7);
			rs232c.pos++;
#endif
			break;
	}
	(void)port;
}

static REG8 IOINPCALL rs232c_i30(UINT port) {

	UINT8 ret = rs232c.data;
	
	rs232c_writeretry();

#if defined(SUPPORT_RS232C_FIFO)
	if(port==0x130){
		if (rs232c_buf_rpos == rs232c_buf_wpos) {
			// 無理矢理読む
			if ((cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c_buf[rs232c_buf_wpos]))) {
				rs232c_buf_wpos = (rs232c_buf_wpos+1) & RS232C_BUFFER_MASK;
				rs232c.data = rs232c_buf[rs232c_buf_rpos]; // データを1つ取り出し
			}
		}
	}
#endif
	if (rs232c_buf_rpos != rs232c_buf_wpos) {
		rs232c_buf_rpos = (rs232c_buf_rpos+1) & RS232C_BUFFER_MASK; // バッファ読み取り位置を1進める
	}
#if defined(SUPPORT_RS232C_FIFO)
	if(port==0x130){
		if (rs232c_buf_rpos != rs232c_buf_wpos) { // 送信すべきデータがあるか確認
			rs232c.data = rs232c_buf[rs232c_buf_rpos]; // 次のデータを取り出し
			//if (sysport.c & 1) {
			//	rs232cfifo.irqflag = 2;
			//	pic_setirq(4);
			//}
			//OutputDebugString(_T("READ!¥n"));
		}else{
			rs232c.result &= ~0x2;
			rs232cfifo.irqflag = 3;
			pic_setirq(4);
			//rs232c.data = 0xff;
			//pic_resetirq(4);
			//OutputDebugString(_T("READ END!¥n"));
		}
	}else
#endif
	{
		rs232c.result &= ~0x2;
	}
	rs232c_removecounter = 0;
	return(ret);
}

static REG8 IOINPCALL rs232c_i32(UINT port) {

	UINT8 ret;

	rs232c_writeretry();

	ret = rs232c.result;
	if (!(rs232c_stat() & 0x20)) {
		return(ret | 0x80);
	}
	else {
		(void)port;
		return(ret);
	}
}

static REG8 IOINPCALL rs232c_i132(UINT port) {

	UINT8 ret;
	
	rs232c_writeretry();

	ret = rs232c.result;
	ret = (ret & ~0xf7) | ((rs232c.result << 1) & 0x6) | ((rs232c.result >> 2) & 0x1);

	if (!(rs232c_stat() & 0x20)) {
		return(ret | 0x80);
	}
	else {
		(void)port;
		return(ret);
	}
}

// FIFOモード

#if defined(SUPPORT_RS232C_FIFO)
static REG8 IOINPCALL rs232c_i134(UINT port) {

	return(0x00);
}

static REG8 IOINPCALL rs232c_i136(UINT port) {

	rs232cfifo.port136 ^= 0x40;
	
	if(rs232cfifo.irqflag){
		rs232cfifo.port136 &= ~0xf;
		if(rs232cfifo.irqflag == 3){
			rs232cfifo.port136 |= 0x6;
			rs232cfifo.irqflag = 0;
			//OutputDebugString(_T("CHECK READ END INT!¥n"));
		}else if(rs232cfifo.irqflag == 2){
			rs232cfifo.port136 |= 0x4;
			//OutputDebugString(_T("CHECK READ INT!¥n"));
		}else if(rs232cfifo.irqflag == 1){
			rs232cfifo.port136 |= 0x2;
			rs232cfifo.irqflag = 0;
			//OutputDebugString(_T("CHECK WRITE INT!¥n"));
		}
	}else{
		rs232cfifo.port136 |= 0x1;
		pic_resetirq(4);
		//OutputDebugString(_T("NULL INT!¥n"));
	}

	return(rs232cfifo.port136);
}

static void IOOUTCALL rs232c_o138(UINT port, REG8 dat) {

	if(dat & 0x2){
		//int i;
		//// 受信FIFOリセット
		//rs232c_buf_rpos = rs232c_buf_wpos;
		//if(rs232cfifo.irqflag==2) rs232cfifo.irqflag = 0;
		//if(cm_rs232c){
		//	for(i=0;i<256;i++){
		//		cm_rs232c->read(cm_rs232c, &rs232c_buf[rs232c_buf_wpos]);
		//	}
		//}
		//pic_resetirq(4);
	}
	rs232cfifo.port138 = dat;
	(void)port;
}

static REG8 IOINPCALL rs232c_i138(UINT port) {

	UINT8 ret = rs232cfifo.port138;
	
	return(ret);
}

void rs232c_vfast_setrs232cspeed(UINT8 value) {
	if(value == 0) return;
	if(!(rs232cfifo.vfast & 0x80)) return; // V FASTモードでない場合はなにもしない
	if (cm_rs232c) {
		int speedtbl[16] = {
			0, 115200, 57600, 38400,
			28800, 0, 19200, 0,
			14400, 0, 0, 0,
			9600, 0, 0, 0,
		};
		int newspeed;
		newspeed = speedtbl[rs232cfifo.vfast & 0xf];
		if(newspeed != 0){
			cm_rs232c->msg(cm_rs232c, COMMSG_CHANGESPEED, (INTPTR)&newspeed);
		}
	}
}

static void IOOUTCALL rs232c_o13a(UINT port, REG8 dat) {

	rs232cfifo.vfast = dat;
	rs232c_vfast_setrs232cspeed(rs232cfifo.vfast);
	(void)port;
}

static REG8 IOINPCALL rs232c_i13a(UINT port) {

	UINT8 ret = rs232cfifo.vfast;
	return(ret);
}
#endif



// ----

static const IOOUT rs232co30[2] = {
					rs232c_o30,	rs232c_o32};

static const IOINP rs232ci30[2] = {
					rs232c_i30,	rs232c_i32};

void rs232c_reset(const NP2CFG *pConfig) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
	rs232c.result = 0x05;
	rs232c.data = 0xff;
	rs232c.send = 1;
	rs232c.pos = 0;
	rs232c.dummyinst = 0;
	rs232c.mul = 10 * 16;
	rs232c.rawmode = 0;
#if defined(VAEG_FIX)
	rs232c.cmd = 0;
#endif

#if defined(SUPPORT_RS232C_FIFO)
	ZeroMemory(&rs232cfifo, sizeof(rs232cfifo));
#endif

	(void)pConfig;
}

void rs232c_bind(void) {

	iocore_attachsysoutex(0x0030, 0x0cf1, rs232co30, 2);
	iocore_attachsysinpex(0x0030, 0x0cf1, rs232ci30, 2);
	
#if defined(SUPPORT_RS232C_FIFO)
	iocore_attachout(0x130, rs232c_o30);
	iocore_attachinp(0x130, rs232c_i30);
	iocore_attachout(0x132, rs232c_o32);
	iocore_attachinp(0x132, rs232c_i132);
	//iocore_attachout(0x134, rs232c_o134);
	iocore_attachinp(0x134, rs232c_i134);
	//iocore_attachout(0x136, rs232c_o136);
	iocore_attachinp(0x136, rs232c_i136);
	iocore_attachout(0x138, rs232c_o138);
	iocore_attachinp(0x138, rs232c_i138);
	iocore_attachout(0x13a, rs232c_o13a);
	iocore_attachinp(0x13a, rs232c_i13a);
#endif
}

