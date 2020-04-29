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

#if !defined(__LIBRETRO__)

#include "compiler.h"

#include "pccore.h"
#include "np2.h"
#include "commng.h"
#include "codecnv/codecnv.h"
#if defined(SUPPORT_NP2_TICKCOUNT)
#include <np2_tickcount.h>
#endif

#if defined(_WINDOWS)
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


typedef struct {
#if defined(_WINDOWS)
	HANDLE	hdl;
#else
	int		hdl;
	struct termios	tio;
#endif
} _CMSER, *CMSER;

const UINT32 cmserial_speed[10] = {
	110, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
};


static UINT
serialread(COMMNG self, UINT8 *data)
{
	CMSER serial = (CMSER)(self + 1);
#if defined(_WINDOWS)
	COMSTAT status;
	DWORD errors;
	BOOLEAN rv;
	DWORD size;
#else
	size_t size;
	int bytes;
	int rv;
#endif

#if defined(_WINDOWS)
	rv = ClearCommError(serial->hdl, &errors, &status);
	if (rv && status.cbInQue > 0) {
#else
	rv = ioctl(serial->hdl, FIONREAD, &bytes);
	if (rv == 0 && bytes > 0) {
#endif
		VERBOSE(("serialread: bytes = %d", bytes));
#if defined(_WINDOWS)
		ReadFile(serial->hdl, data, 1, &size, NULL);
#else
		size = read(serial->hdl, data, 1);
#endif
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
#if defined(_WINDOWS)
	DWORD size;
#else
	size_t size;
#endif

#if defined(_WINDOWS)
	size = WriteFile(serial->hdl, &data, 1, &size, NULL);
#else
	size = write(serial->hdl, &data, 1);
#endif
	self->lastdata = data;
#if defined(SUPPORT_NP2_TICKCOUNT)
	self->lastdatatime = (UINT)NP2_TickCount_GetCount();
#else
	self->lastdatatime = 0;
#endif
	if (size == 1) {
		self->lastdatafail = 0;
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
#if defined(_WIN32)
	DWORD size;
#else
	size_t size;
#endif

	if(self->lastdatafail) {
#if defined(_WIN32)
		size = WriteFile(serial->hdl, &self->lastdata, 1, &size, NULL);
#else
		size = write(serial->hdl, &self->lastdata, 1);
#endif
		if (size == 0) {
			VERBOSE(("serialwriteretry: write failure (%s)", strerror(errno)));
			return 0;
		}
		self->lastdatafail = 0;
		VERBOSE(("serialwriteretry"));
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
#if defined(_WINDOWS)
	DCB status;
	BOOLEAN rv;
#else
	int status;
	int rv;
#endif

#if defined(_WINDOWS)
	rv = GetCommState(serial->hdl, &status);
	if (!rv) {
#else
	rv = ioctl(serial->hdl, TIOCMGET, &status);
	if (rv < 0) {
#endif
		VERBOSE(("serialgetstat: ioctl: %s", strerror(errno)));
		return 0x20;
	}
#if defined(_WINDOWS)
	if (!(status.fOutxDsrFlow)) {
#else
	if (!(status & TIOCM_DSR)) {
#endif
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

#if defined(_WINDOWS)
	CloseHandle(serial->hdl);
#else
	tcsetattr(serial->hdl, TCSANOW, &serial->tio);
	close(serial->hdl);
#endif
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

void convert_np2tocm(UINT port, UINT8* param, UINT32* speed) {
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
		if(!(np2cfg.pc9861sw[0] & 0x2)) {	// Sync
			*speed = cmserial_pc98_ch23_speed[7 - ((np2cfg.pc9861sw[0] >> 2) & 0x7) + 1];
		} else {	// Async
			*speed = cmserial_pc98_ch23_speed[8 - (((np2cfg.pc9861sw[0] >> 2) & 0xF) - 4)];
		}
		break;
	case 2:
		if(!(np2cfg.pc9861sw[2] & 0x2)) {	// Sync
			*speed = cmserial_pc98_ch23_speed[7 - ((np2cfg.pc9861sw[2] >> 2) & 0x7) + 1];
		} else {	// Async
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
#if defined(_WINDOWS)
	static const int cmserial_cflag[] = {
		0/*CBR_B75*/, CBR_110, CBR_300, CBR_600, CBR_1200, CBR_2400, CBR_4800,
		CBR_9600, CBR_19200, CBR_38400, CBR_57600, CBR_115200
	};
#else
	static const int cmserial_cflag[] = {
		B75, B110, B300, B600, B1200, B2400, B4800,
		B9600, B19200, B38400, B57600, B115200
	};
	static const int csize[] = { CS5, CS6, CS7, CS8 };
	struct termios options, origopt;
#endif
	COMMNG ret;
	CMSER serial;
#if defined(_WINDOWS)
	HANDLE hdl;
	DCB dcb = {0};
	wchar_t wmout[MAX_PATH];
#else
	int hdl;
#endif
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

#if defined(_WINDOWS)
	codecnv_utf8toucs2(wmout, MAX_PATH, np2oscfg.com[port].mout, -1);
	hdl = CreateFileW(wmout, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hdl == INVALID_HANDLE_VALUE) {
#else
	hdl = open(np2oscfg.com[port].mout, O_RDWR | O_NOCTTY | O_NDELAY);
	if (hdl == -1) {
#endif
		VERBOSE(("cmserial_create: open failure %s, %s", np2oscfg.com[port].mout, strerror(errno)));
		goto cscre_failure;
	}

#if !defined(_WINDOWS)
	if (!isatty(hdl)) {
		VERBOSE(("cmserial_create: not terminal file descriptor (%s)", strerror(errno)));
		goto cscre_close;
	}

	/* get current options for the port */
	tcgetattr(hdl, &options);
	origopt = options;
#endif

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
#if defined(_WINDOWS)
	dcb.BaudRate = cmserial_cflag[i];
#else
	cfsetispeed(&options, cmserial_cflag[i]);
	cfsetospeed(&options, cmserial_cflag[i]);
#endif

	/* character size bits */
#if defined(_WINDOWS)
	dcb.ByteSize = ((param >> 2) & 3) + 5;
	VERBOSE(("cmserial_create: charactor size = %d", ((param >> 2) & 3) + 5));
#else
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= csize[(param >> 2) & 3];
	VERBOSE(("cmserial_create: charactor size = %d", csize[(param >> 2) & 3]));
#endif

	/* parity check */
	switch (param & 0x30) {
	case 0x10:
		VERBOSE(("cmserial_create: odd parity"));
#if defined(_WINDOWS)
		dcb.fParity = 1;
		dcb.Parity = ODDPARITY;
#else
		options.c_cflag |= PARENB | PARODD;
		options.c_iflag |= INPCK | ISTRIP;
#endif
		break;

	case 0x30:
		VERBOSE(("cmserial_create: even parity"));
#if defined(_WINDOWS)
		dcb.fParity = 1;
		dcb.Parity = EVENPARITY;
#else
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK | ISTRIP;
#endif
		break;

	default:
		VERBOSE(("cmserial_create: non parity"));
#if defined(_WINDOWS)
		dcb.fParity = 0;
#else
		options.c_cflag &= ~PARENB;
		options.c_iflag &= ~(INPCK | ISTRIP);
#endif
		break;
	}

	/* stop bits */
	switch (param & 0xc0) {
	case 0x80:
		VERBOSE(("cmserial_create: stop bits: 1.5"));
#if defined(_WINDOWS)
		dcb.StopBits = ONE5STOPBITS;
#endif
		break;

	case 0xc0:
		VERBOSE(("cmserial_create: stop bits: 2"));
#if defined(_WINDOWS)
		dcb.StopBits = TWOSTOPBITS;
#else
		options.c_cflag |= CSTOPB;
#endif
		break;

	default:
		VERBOSE(("cmserial_create: stop bits: 1"));
#if defined(_WINDOWS)
		dcb.StopBits = ONESTOPBIT;
#else
		options.c_cflag &= ~CSTOPB;
#endif
		break;
	}

#if !defined(_WINDOWS)
	/* set misc flag */
	cfmakeraw(&options);
	options.c_cflag |= CLOCAL | CREAD;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
#endif

#if defined(SERIAL_DEBUG)
	print_status(&options);
#endif

	ret = (COMMNG)_MALLOC(sizeof(_COMMNG) + sizeof(_CMSER), "SERIAL");
	if (ret == NULL) {
		VERBOSE(("cmserial_create: memory alloc failure"));
		goto cscre_close;
	}

	/* set the new options for the port */
#if defined(_WINDOWS)
	SetCommState(hdl, &dcb);
#else
	tcsetattr(hdl, TCSANOW, &options);
#endif

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
#if !defined(_WINDOWS)
	serial->tio = origopt;
#endif
	return ret;

cscre_close:
#if defined(_WINDOWS)
	CloseHandle(hdl);
#else
	close(hdl);
#endif
cscre_failure:
	return NULL;
}

#endif	/* __LIBRETRO__ */
