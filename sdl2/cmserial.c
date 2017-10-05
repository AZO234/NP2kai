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

#include "compiler.h"

#include "np2.h"
#include "commng.h"

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
	if (size == 1) {
		VERBOSE(("serialwrite: data = %02x", data));
		return 1;
	}
	VERBOSE(("serialwrite: write failure (%s)", strerror(errno)));
	return 0;
}

static UINT8
serialgetstat(COMMNG self)
{
	CMSER serial = (CMSER)(self + 1);
	int status;
	int rv;

	rv = ioctl(serial->hdl, TIOCMGET, &status);
	if (rv < 0) {
		VERBOSE(("serialgetstat: ioctl: %s", strerror(errno)));
		return 0x20;
	}
	if (!(status & TIOCM_DSR)) {
		VERBOSE(("serialgetstat: DSR is disable"));
		return 0x20;
	}
	VERBOSE(("serialgetstat: DSR is enable"));
	return 0x00;
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

	g_printerr(" ispeed %d", ispeed);
	g_printerr(" ospeed %d", ospeed);
	g_printerr("%s", "\r\n");
	g_printerr(" %cIGNBRK", (tio->c_iflag & IGNBRK) ? '+' : '-');
	g_printerr(" %cBRKINT", (tio->c_iflag & BRKINT) ? '+' : '-');
	g_printerr(" %cIGNPAR", (tio->c_iflag & IGNPAR) ? '+' : '-');
	g_printerr(" %cPARMRK", (tio->c_iflag & PARMRK) ? '+' : '-');
	g_printerr(" %cINPCK", (tio->c_iflag & INPCK) ? '+' : '-');
	g_printerr(" %cISTRIP", (tio->c_iflag & ISTRIP) ? '+' : '-');
	g_printerr(" %cINLCR", (tio->c_iflag & INLCR) ? '+' : '-');
	g_printerr(" %cIGNCR", (tio->c_iflag & IGNCR) ? '+' : '-');
	g_printerr(" %cICRNL", (tio->c_iflag & ICRNL) ? '+' : '-');
	g_printerr(" %cIXON", (tio->c_iflag & IXON) ? '+' : '-');
	g_printerr(" %cIXOFF", (tio->c_iflag & IXOFF) ? '+' : '-');
	g_printerr(" %cIXANY", (tio->c_iflag & IXANY) ? '+' : '-');
	g_printerr(" %cIMAXBEL", (tio->c_iflag & IMAXBEL) ? '+' : '-');
	g_printerr("%s", "\r\n");
	g_printerr(" %cOPOST", (tio->c_oflag & OPOST) ? '+' : '-');
	g_printerr(" %cONLCR", (tio->c_oflag & ONLCR) ? '+' : '-');
#ifdef OXTABS
	g_printerr(" %cOXTABS", (tio->c_oflag & OXTABS) ? '+' : '-');
#endif
#ifdef TABDLY
	g_printerr(" %cTABDLY", (tio->c_oflag & TABDLY) == XTABS ? '+' : '-');
#endif
#ifdef ONOEOT
	g_printerr(" %cONOEOT", (tio->c_oflag & ONOEOT) ? '+' : '-');
#endif
	g_printerr("%s", "\r\n");

	cs = tio->c_cflag & CSIZE;
	switch (cs) {
	case CS5:
		csstr = "5";
		break;

	case CS6:
		csstr = "6";
		break;

	case CS7:
		csstr = "7";
		break;

	case CS8:
		csstr = "8";
		break;

	default:
		csstr = "?";
		break;
	}
	g_printerr(" cs%s", csstr);
	g_printerr(" %cCSTOPB", (tio->c_cflag & CSTOPB) ? '+' : '-');
	g_printerr(" %cCREAD", (tio->c_cflag & CREAD) ? '+' : '-');
	g_printerr(" %cPARENB", (tio->c_cflag & PARENB) ? '+' : '-');
	g_printerr(" %cPARODD", (tio->c_cflag & PARODD) ? '+' : '-');
	g_printerr(" %cHUPCL", (tio->c_cflag & HUPCL) ? '+' : '-');
	g_printerr(" %cCLOCAL", (tio->c_cflag & CLOCAL) ? '+' : '-');
#ifdef CCTS_OFLOW
	g_printerr(" %cCCTS_OFLOW", (tio->c_cflag & CCTS_OFLOW) ? '+' : '-');
#endif
	g_printerr(" %cCRTSCTS", (tio->c_cflag & CRTSCTS) ? '+' : '-');
#ifdef CRTS_IFLOW
	g_printerr(" %cCRTS_IFLOW", (tio->c_cflag & CRTS_IFLOW) ? '+' : '-');
#endif
#ifdef MDMBUF
	g_printerr(" %cMDMBUF", (tio->c_cflag & MDMBUF) ? '+' : '-');
#endif
	g_printerr(" %cECHOKE", (tio->c_lflag & ECHOKE) ? '+' : '-');
	g_printerr(" %cECHOE", (tio->c_lflag & ECHOE) ? '+' : '-');
	g_printerr(" %cECHO", (tio->c_lflag & ECHO) ? '+' : '-');
	g_printerr(" %cECHONL", (tio->c_lflag & ECHONL) ? '+' : '-');
	g_printerr(" %cECHOPRT", (tio->c_lflag & ECHOPRT) ? '+' : '-');
	g_printerr(" %cECHOCTL", (tio->c_lflag & ECHOCTL) ? '+' : '-');
	g_printerr(" %cISIG", (tio->c_lflag & ISIG) ? '+' : '-');
	g_printerr(" %cICANON", (tio->c_lflag & ICANON) ? '+' : '-');
#ifdef ALTWERASE
	g_printerr(" %cALTWERASE", (tio->c_lflag & ALTWERASE) ? '+' : '-');
#endif
	g_printerr(" %cIEXTEN", (tio->c_lflag & IEXTEN) ? '+' : '-');
	g_printerr("%s", "\r\n");
#ifdef EXTPROC
	g_printerr(" %cEXTPROC", (tio->c_lflag & EXTPROC) ? '+' : '-');
#endif
	g_printerr(" %cTOSTOP", (tio->c_lflag & TOSTOP) ? '+' : '-');
	g_printerr(" %cFLUSHO", (tio->c_lflag & FLUSHO) ? '+' : '-');
#ifdef NOKERNINFO
	g_printerr(" %cNOKERNINFO", (tio->c_lflag & NOKERNINFO) ? '+' : '-');
#endif
	g_printerr(" %cPENDIN", (tio->c_lflag & PENDIN) ? '+' : '-');
	g_printerr(" %cNOFLSH", (tio->c_lflag & NOFLSH) ? '+' : '-');
	g_printerr("%s", "\r\n");
}
#endif

COMMNG
cmserial_create(UINT port, UINT8 param, UINT32 speed)
{
	static const int cmserial_cflag[10] = {
		B110, B300, B1200, B2400, B4800,
		B9600, B19200, B38400, B57600, B115200
	};
	static const int csize[] = { CS5, CS6, CS7, CS8 };
	struct termios options, origopt;
	COMMNG ret;
	CMSER serial;
	int hdl;
	UINT i;

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
