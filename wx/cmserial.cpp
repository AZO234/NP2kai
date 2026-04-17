/*
 * Copyright (c) 2004 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Copied from x/cmserial.c for wx port.
 * g_printerr() replaced with fprintf(stderr, ...) to remove GLib dependency.
 */

#include <compiler.h>

#include <pccore.h>
#include <np2.h>
#include <commng.h>
#if defined(SUPPORT_NP2_TICKCOUNT)
#include <np2_tickcount.h>
#endif

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>


typedef struct {
	int		hdl;

	struct termios	tio;
} _CMSER, *CMSER;

const UINT32 cmserial_speed[10] = {
	110, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
};


static UINT
serialread(COMMNG self, UINT8 *data)
{
	CMSER serial = (CMSER)(self + 1);
	size_t size;
	int bytes;
	int rv;

	rv = ioctl(serial->hdl, FIONREAD, &bytes);
	if (rv == 0 && bytes > 0) {
		VERBOSE(("serialread: bytes = %d", bytes));
		size = read(serial->hdl, data, 1);
		if (size == 1) {
			VERBOSE(("serialread: data = %02x", *data));
			return 1;
		}
		VERBOSE(("serialread: read failure (%s)", strerror(errno)));
	}
	return 0;
}

static UINT
serialwrite(COMMNG self, UINT8 data)
{
	CMSER serial = (CMSER)(self + 1);
	size_t size;

	size = write(serial->hdl, &data, 1);
	self->lastdata = data;
#if defined(SUPPORT_NP2_TICKCOUNT)
	self->lastdatatime = (UINT)NP2_TickCount_GetCount();
#else
	self->lastdatatime = 0;
#endif
	if (size == 1) {
		VERBOSE(("serialwrite: data = %02x", data));
		return 1;
	}
	self->lastdatafail = 1;
	VERBOSE(("serialwrite: write failure (%s)", strerror(errno)));
	return 0;
}

static UINT
serialwriteretry(COMMNG self)
{
	CMSER serial = (CMSER)(self + 1);
	size_t size;

	if(self->lastdatafail) {
		size = write(serial->hdl, &self->lastdata, 1);
		if (size == 0) {
			VERBOSE(("serialwriteretry: write failure (%s)", strerror(errno)));
			return 0;
		}
		self->lastdatafail = 0;
		VERBOSE(("serialwriteretry: data = %02x", self->lastdata));
	}
	return 1;
}

static UINT
seriallastwritesuccess(COMMNG self)
{
	return self->lastdatafail;
}

static UINT8
serialgetstat(COMMNG self)
{
	CMSER serial = (CMSER)(self + 1);
	int status;
	int rv;
	UINT8 ret = 0;

	rv = ioctl(serial->hdl, TIOCMGET, &status);
	if (rv < 0) {
		VERBOSE(("serialgetstat: ioctl: %s", strerror(errno)));
		return 0x01;
	}
	if (!(status & TIOCM_DSR)) {
		VERBOSE(("serialgetstat: DSR is disable"));
		ret |= 0x01;
	} else {
		VERBOSE(("serialgetstat: DSR is enable"));
	}
	if (!(status & TIOCM_CTS)) {
		ret |= 0x40;
	}
	if (!(status & TIOCM_RNG)) {
		ret |= 0x80;
	}
	if (!(status & TIOCM_CAR)) {
		ret |= 0x20;
	}
	return ret;
}

static INTPTR
serialmsg(COMMNG self, UINT msg, INTPTR param)
{
	(void)self;
	(void)msg;
	(void)param;

	return 0;
}

static void
serialrelease(COMMNG self)
{
	CMSER serial = (CMSER)(self + 1);

	tcsetattr(serial->hdl, TCSANOW, &serial->tio);
	close(serial->hdl);
	_MFREE(self);
}


/* ---- interface */
#if defined(SERIAL_DEBUG)
static void
print_status(const struct termios *tio)
{
	char *csstr;
	int cs;
	speed_t ispeed = cfgetispeed(tio);
	speed_t ospeed = cfgetospeed(tio);

	fprintf(stderr, " ispeed %d", ispeed);
	fprintf(stderr, " ospeed %d", ospeed);
	fprintf(stderr, "%s", "\r\n");
	fprintf(stderr, " %cIGNBRK", (tio->c_iflag & IGNBRK) ? '+' : '-');
	fprintf(stderr, " %cBRKINT", (tio->c_iflag & BRKINT) ? '+' : '-');
	fprintf(stderr, " %cIGNPAR", (tio->c_iflag & IGNPAR) ? '+' : '-');
	fprintf(stderr, " %cPARMRK", (tio->c_iflag & PARMRK) ? '+' : '-');
	fprintf(stderr, " %cINPCK",  (tio->c_iflag & INPCK)  ? '+' : '-');
	fprintf(stderr, " %cISTRIP", (tio->c_iflag & ISTRIP) ? '+' : '-');
	fprintf(stderr, " %cINLCR",  (tio->c_iflag & INLCR)  ? '+' : '-');
	fprintf(stderr, " %cIGNCR",  (tio->c_iflag & IGNCR)  ? '+' : '-');
	fprintf(stderr, " %cICRNL",  (tio->c_iflag & ICRNL)  ? '+' : '-');
	fprintf(stderr, " %cIXON",   (tio->c_iflag & IXON)   ? '+' : '-');
	fprintf(stderr, " %cIXOFF",  (tio->c_iflag & IXOFF)  ? '+' : '-');
	fprintf(stderr, " %cIXANY",  (tio->c_iflag & IXANY)  ? '+' : '-');
	fprintf(stderr, " %cIMAXBEL",(tio->c_iflag & IMAXBEL)? '+' : '-');
	fprintf(stderr, "%s", "\r\n");
	fprintf(stderr, " %cOPOST",  (tio->c_oflag & OPOST)  ? '+' : '-');
	fprintf(stderr, " %cONLCR",  (tio->c_oflag & ONLCR)  ? '+' : '-');
#ifdef OXTABS
	fprintf(stderr, " %cOXTABS", (tio->c_oflag & OXTABS) ? '+' : '-');
#endif
#ifdef TABDLY
	fprintf(stderr, " %cTABDLY", (tio->c_oflag & TABDLY) == XTABS ? '+' : '-');
#endif
#ifdef ONOEOT
	fprintf(stderr, " %cONOEOT", (tio->c_oflag & ONOEOT) ? '+' : '-');
#endif
	fprintf(stderr, "%s", "\r\n");

	cs = tio->c_cflag & CSIZE;
	switch (cs) {
	case CS5: csstr = "5"; break;
	case CS6: csstr = "6"; break;
	case CS7: csstr = "7"; break;
	case CS8: csstr = "8"; break;
	default:  csstr = "?"; break;
	}
	fprintf(stderr, " cs%s",     csstr);
	fprintf(stderr, " %cCSTOPB", (tio->c_cflag & CSTOPB) ? '+' : '-');
	fprintf(stderr, " %cCREAD",  (tio->c_cflag & CREAD)  ? '+' : '-');
	fprintf(stderr, " %cPARENB", (tio->c_cflag & PARENB) ? '+' : '-');
	fprintf(stderr, " %cPARODD", (tio->c_cflag & PARODD) ? '+' : '-');
	fprintf(stderr, " %cHUPCL",  (tio->c_cflag & HUPCL)  ? '+' : '-');
	fprintf(stderr, " %cCLOCAL", (tio->c_cflag & CLOCAL) ? '+' : '-');
#ifdef CCTS_OFLOW
	fprintf(stderr, " %cCCTS_OFLOW", (tio->c_cflag & CCTS_OFLOW) ? '+' : '-');
#endif
	fprintf(stderr, " %cCRTSCTS",(tio->c_cflag & CRTSCTS)? '+' : '-');
#ifdef CRTS_IFLOW
	fprintf(stderr, " %cCRTS_IFLOW", (tio->c_cflag & CRTS_IFLOW) ? '+' : '-');
#endif
#ifdef MDMBUF
	fprintf(stderr, " %cMDMBUF", (tio->c_cflag & MDMBUF) ? '+' : '-');
#endif
	fprintf(stderr, " %cECHOKE", (tio->c_lflag & ECHOKE) ? '+' : '-');
	fprintf(stderr, " %cECHOE",  (tio->c_lflag & ECHOE)  ? '+' : '-');
	fprintf(stderr, " %cECHO",   (tio->c_lflag & ECHO)   ? '+' : '-');
	fprintf(stderr, " %cECHONL", (tio->c_lflag & ECHONL) ? '+' : '-');
	fprintf(stderr, " %cECHOPRT",(tio->c_lflag & ECHOPRT)? '+' : '-');
	fprintf(stderr, " %cECHOCTL",(tio->c_lflag & ECHOCTL)? '+' : '-');
	fprintf(stderr, " %cISIG",   (tio->c_lflag & ISIG)   ? '+' : '-');
	fprintf(stderr, " %cICANON", (tio->c_lflag & ICANON) ? '+' : '-');
#ifdef ALTWERASE
	fprintf(stderr, " %cALTWERASE", (tio->c_lflag & ALTWERASE) ? '+' : '-');
#endif
	fprintf(stderr, " %cIEXTEN", (tio->c_lflag & IEXTEN) ? '+' : '-');
	fprintf(stderr, "%s", "\r\n");
#ifdef EXTPROC
	fprintf(stderr, " %cEXTPROC",(tio->c_lflag & EXTPROC)? '+' : '-');
#endif
	fprintf(stderr, " %cTOSTOP", (tio->c_lflag & TOSTOP) ? '+' : '-');
	fprintf(stderr, " %cFLUSHO", (tio->c_lflag & FLUSHO) ? '+' : '-');
#ifdef NOKERNINFO
	fprintf(stderr, " %cNOKERNINFO", (tio->c_lflag & NOKERNINFO) ? '+' : '-');
#endif
	fprintf(stderr, " %cPENDIN", (tio->c_lflag & PENDIN) ? '+' : '-');
	fprintf(stderr, " %cNOFLSH", (tio->c_lflag & NOFLSH) ? '+' : '-');
	fprintf(stderr, "%s", "\r\n");
}
#endif

void
convert_np2tocm(UINT port, UINT8* param, UINT32* speed) {
	static const int cmserial_pc98_ch1_speed[] = {
		0, 75, 150, 300, 600, 1200, 2400, 4800, 9600
	};
	static const int cmserial_pc98_ch23_speed[] = {
		75, 150, 300, 600, 1200, 2400, 4800, 9600, 19200
	};

	switch(port) {
	case 0:
		*speed = cmserial_pc98_ch1_speed[np2cfg.memsw[1] & 0xF];
		break;
	case 1:
		if(!(np2cfg.pc9861sw[0] & 0x2)) {	/* Sync */
			*speed = cmserial_pc98_ch23_speed[7 - ((np2cfg.pc9861sw[0] >> 2) & 0x7) + 1];
		} else {	/* Async */
			*speed = cmserial_pc98_ch23_speed[8 - (((np2cfg.pc9861sw[0] >> 2) & 0xF) - 4)];
		}
		break;
	case 2:
		if(!(np2cfg.pc9861sw[2] & 0x2)) {	/* Sync */
			*speed = cmserial_pc98_ch23_speed[7 - ((np2cfg.pc9861sw[2] >> 2) & 0x7) + 1];
		} else {	/* Async */
			*speed = cmserial_pc98_ch23_speed[8 - (((np2cfg.pc9861sw[2] >> 2) & 0xF) - 4)];
		}
		break;
	}

	*param = 0;
	switch(port) {
	case 0:
		*param |= np2cfg.memsw[0] & 0xC;
		switch((np2cfg.memsw[0] & 0x30) >> 4) {
		case 1:
			*param |= 0x10;
			break;
		case 3:
			*param |= 0x30;
			break;
		default:
			break;
		}
		switch((np2cfg.memsw[0] & 0xC0) >> 6) {
		case 2:
			*param |= 0x80;
			break;
		case 3:
			*param |= 0xC0;
			break;
		default:
			break;
		}
		break;
	case 1:
	case 2:
		*param |= 0xC;
		break;
	}
}

COMMNG
cmserial_create(UINT port, UINT8 param, UINT32 speed)
{
	static const int cmserial_cflag[] = {
		B75, B110, B300, B600, B1200, B2400, B4800,
		B9600, B19200, B38400, B57600, B115200
	};
	static const int csize[] = { CS5, CS6, CS7, CS8 };
	struct termios options, origopt;
	COMMNG ret;
	CMSER serial;
	int hdl;
	UINT i;

	if(np2oscfg.com[port].direct) {
		convert_np2tocm(port, &param, &speed);
	}

	VERBOSE(("cmserial_create: port = %d, param = %02x, speed = %d", port, param, speed));

	if (port == 0 || port > MAX_SERIAL_PORT_NUM) {
		VERBOSE(("cmserial_create: port is invalid"));
		goto cscre_failure;
	}

	port--;
	if (np2oscfg.com[port].mout[0] == '\0') {
		VERBOSE(("cmserial_create: com device file is disable"));
		goto cscre_failure;
	}

	hdl = open(np2oscfg.com[port].mout, O_RDWR | O_NOCTTY | O_NDELAY);
	if (hdl == -1) {
		VERBOSE(("cmserial_create: open failure %s, %s", np2oscfg.com[port].mout, strerror(errno)));
		goto cscre_failure;
	}

	if (!isatty(hdl)) {
		VERBOSE(("cmserial_create: not terminal file descriptor (%s)", strerror(errno)));
		goto cscre_close;
	}

	/* get current options for the port */
	tcgetattr(hdl, &options);
	origopt = options;

	/* baud rates */
	for (i = 0; i < NELEMENTS(cmserial_speed); i++) {
		if (cmserial_speed[i] >= speed) {
			VERBOSE(("cmserial_create: spped = %d", cmserial_speed[i]));
			break;
		}
	}
	if (i >= NELEMENTS(cmserial_speed)) {
		VERBOSE(("cmserial_create: speed is invaild"));
		goto cscre_close;
	}
	cfsetispeed(&options, cmserial_cflag[i]);
	cfsetospeed(&options, cmserial_cflag[i]);

	/* character size bits */
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= csize[(param >> 2) & 3];
	VERBOSE(("cmserial_create: charactor size = %d", csize[(param >> 2) & 3]));

	/* parity check */
	switch (param & 0x30) {
	case 0x10:
		VERBOSE(("cmserial_create: odd parity"));
		options.c_cflag |= PARENB | PARODD;
		options.c_iflag |= INPCK | ISTRIP;
		break;

	case 0x30:
		VERBOSE(("cmserial_create: even parity"));
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK | ISTRIP;
		break;

	default:
		VERBOSE(("cmserial_create: non parity"));
		options.c_cflag &= ~PARENB;
		options.c_iflag &= ~(INPCK | ISTRIP);
		break;
	}

	/* stop bits */
	switch (param & 0xc0) {
	case 0x80:
		VERBOSE(("cmserial_create: stop bits: 1.5"));
		break;

	case 0xc0:
		VERBOSE(("cmserial_create: stop bits: 2"));
		options.c_cflag |= CSTOPB;
		break;

	default:
		VERBOSE(("cmserial_create: stop bits: 1"));
		options.c_cflag &= ~CSTOPB;
		break;
	}

	/* set misc flag */
	cfmakeraw(&options);
	options.c_cflag |= CLOCAL | CREAD;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

#if defined(SERIAL_DEBUG)
	print_status(&options);
#endif

	ret = (COMMNG)_MALLOC(sizeof(_COMMNG) + sizeof(_CMSER), "SERIAL");
	if (ret == NULL) {
		VERBOSE(("cmserial_create: memory alloc failure"));
		goto cscre_close;
	}

	/* set the new options for the port */
	tcsetattr(hdl, TCSANOW, &options);

#if 1
	ret->connect = COMCONNECT_MIDI;
#else
	ret->connect = COMCONNECT_SERIAL;
#endif
	ret->read = serialread;
	ret->write = serialwrite;
	ret->writeretry = serialwriteretry;
	ret->lastwritesuccess = seriallastwritesuccess;
	ret->getstat = serialgetstat;
	ret->msg = serialmsg;
	ret->release = serialrelease;
	serial = (CMSER)(ret + 1);
	serial->hdl = hdl;
	serial->tio = origopt;
	return ret;

cscre_close:
	close(hdl);
cscre_failure:
	return NULL;
}
