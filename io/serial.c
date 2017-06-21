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
	keybrd.xferclock = pccore.realclock / 1920;
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



// ---- RS-232C

	COMMNG	cm_rs232c;

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
	}
}

void rs232c_callback(void) {

	BOOL	intr;

	intr = FALSE;
	if ((cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c.data))) {
		rs232c.result |= 2;
		if (sysport.c & 1) {
			intr = TRUE;
		}
	}
	else {
		rs232c.result &= (UINT8)~2;
	}
	if (sysport.c & 4) {
		if (rs232c.send) {
			rs232c.send = 0;
			intr = TRUE;
		}
	}
	if (intr) {
		pic_setirq(4);
	}
}

UINT8 rs232c_stat(void) {

	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL);
	}
	return(cm_rs232c->getstat(cm_rs232c));
}

void rs232c_midipanic(void) {

	if (cm_rs232c) {
		cm_rs232c->msg(cm_rs232c, COMMSG_MIDIRESET, 0);
	}
}


// ----

static void IOOUTCALL rs232c_o30(UINT port, REG8 dat) {

	if (cm_rs232c) {
		cm_rs232c->write(cm_rs232c, (UINT8)dat);
	}
	if (sysport.c & 4) {
		rs232c.send = 0;
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
			rs232c.pos++;
			break;

		case 0x02:			// cmd
			sysport.c &= ~7;
			sysport.c |= (dat & 7);
			rs232c.pos++;
			break;
	}
	(void)port;
}

static REG8 IOINPCALL rs232c_i30(UINT port) {

	(void)port;
	return(rs232c.data);
}

static REG8 IOINPCALL rs232c_i32(UINT port) {

	if (!(rs232c_stat() & 0x20)) {
		return(rs232c.result | 0x80);
	}
	else {
		(void)port;
		return(rs232c.result);
	}
}


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

	(void)pConfig;
}

void rs232c_bind(void) {

	iocore_attachsysoutex(0x0030, 0x0cf1, rs232co30, 2);
	iocore_attachsysinpex(0x0030, 0x0cf1, rs232ci30, 2);
}

