/**
 * @file	s98.cpp
 * @brief	Interface of logging PC-98 sound
 *			for S98amp S98 Input plugin for Winamp Version 1.3.1+ by Mamiya
 */

#include <compiler.h>

#if defined(SUPPORT_S98)

#include <sound/s98.h>
#include <pccore.h>
#include <nevent.h>
#include <dosio.h>
#include <sound/fmboard.h>

#define S98LOG_BUFSIZE (32 * 1024)

typedef struct {
	UINT8	magic[3];
	UINT8	formatversion;
	UINT8	timerinfo[4];
	UINT8	timerinfo2[4];
	UINT8	compressing[4];
	UINT8	offset[4];
	UINT8	dumpdata[4];
	UINT8	looppoint[4];
	UINT8	headerreserved[0x24];
	UINT8	devicecount[4];
	UINT8	device1type[4];
	UINT8	device1clock[4];
	UINT8	device1pan[4];
	UINT8	device1reserved[4];
	UINT8	device2type[4];
	UINT8	device2clock[4];
	UINT8	device2pan[4];
	UINT8	device2reserved[4];
	UINT8	device3type[4];
	UINT8	device3clock[4];
	UINT8	device3pan[4];
	UINT8	device3reserved[4];
	UINT8	title[0x40];
} S98HDR;

static struct {
	FILEH	fh;
	UINT32	intcount;
	SINT32	clock;
	UINT	p;
	UINT8	buf[S98LOG_BUFSIZE];
} s98log;


static void s98timer(NEVENTITEM item);

static void sets98event(NEVENTPOSITION absolute) {

	s98log.intcount++;
	nevent_set(NEVENT_S98TIMER, s98log.clock, s98timer, NEVENT_RELATIVE);
	(void)absolute;
}

static void s98timer(NEVENTITEM item) {

	if (s98log.fh != FILEH_INVALID) {
		sets98event(NEVENT_RELATIVE);
	}
	(void)item;
}

static void S98_flush(void) {

	if (s98log.p) {
		file_write(s98log.fh, s98log.buf, s98log.p);
		s98log.p = 0;
	}
}

static void S98_putc(REG8 data) {

	s98log.buf[s98log.p++] = data;
	if (s98log.p == S98LOG_BUFSIZE) {
		S98_flush();
	}
}

static void S98_putint(void) {

	if (s98log.intcount) {
		if (s98log.intcount == 1) {
			S98_putc(0xFF);					/* SYNC(1) */
		}
		else if (s98log.intcount == 2) {
			S98_putc(0xFF);					/* SYNC(1) */
			S98_putc(0xFF);					/* SYNC(1) */
		}
		else {
			S98_putc(0xFE);					/* SYNC(n) */
			s98log.intcount -= 2;
			while (s98log.intcount > 0x7f) {
				S98_putc((REG8)(0x80 | (s98log.intcount & 0x7f)));
				s98log.intcount >>= 7;
			}
			S98_putc((REG8)(s98log.intcount & 0x7f));
		}
		s98log.intcount = 0;
	}
}


// ----

void S98_init(void) {

	s98log.fh = FILEH_INVALID;
}

void S98_trash(void) {

	S98_close();
}

BRESULT S98_open(const OEMCHAR *filename) {

	UINT	i;
	S98HDR	hdr;

	// ファイルのオープン
	s98log.fh = file_create(filename);
	if (s98log.fh == FILEH_INVALID) {
		return(FAILURE);
	}

	// 初期化
	s98log.clock = pccore.realclock / 1000;
	s98log.p = 0;

	// ヘッダの保存
	ZeroMemory(&hdr, sizeof(hdr));
	hdr.magic[0] = 'S';
	hdr.magic[1] = '9';
	hdr.magic[2] = '8';
	hdr.formatversion = '1';
	STOREINTELDWORD(hdr.timerinfo, 1);
	STOREINTELDWORD(hdr.offset, offsetof(S98HDR, title));
	STOREINTELDWORD(hdr.dumpdata, sizeof(S98HDR));
	switch(g_nSoundID) {
	  case 0x02:
	  case 0x22:
		hdr.formatversion = '3';
		STOREINTELDWORD(hdr.devicecount, 1);
		STOREINTELDWORD(hdr.device1type, 2);
		STOREINTELDWORD(hdr.device1clock, OPNA_CLOCK);
		break;
	  case 0x06:
		hdr.formatversion = '3';
		STOREINTELDWORD(hdr.devicecount, 2);
		STOREINTELDWORD(hdr.device1type, 4);
		STOREINTELDWORD(hdr.device2type, 2);
		STOREINTELDWORD(hdr.device1clock, OPNA_CLOCK*2);
		STOREINTELDWORD(hdr.device2clock, OPNA_CLOCK);
		break;
	  case 0x40:
		hdr.formatversion = '3';
		STOREINTELDWORD(hdr.devicecount, 2);
		STOREINTELDWORD(hdr.device1type, 4);
		STOREINTELDWORD(hdr.device2type, 3);
		STOREINTELDWORD(hdr.device1clock, OPNA_CLOCK*2);
		STOREINTELDWORD(hdr.device2clock, OPNA_CLOCK*2);
		break;
	  case 0x80:
		hdr.formatversion = '3';
		STOREINTELDWORD(hdr.devicecount, 3);
		STOREINTELDWORD(hdr.device1type, 0xf);
		STOREINTELDWORD(hdr.device2type, 0xf);
		STOREINTELDWORD(hdr.device3type, 0xf);
		STOREINTELDWORD(hdr.device1clock, 2000000);
		STOREINTELDWORD(hdr.device2clock, 2000000);
		STOREINTELDWORD(hdr.device3clock, 2000000);
		STOREINTELDWORD(hdr.device1pan, 0x21);
		STOREINTELDWORD(hdr.device2pan, 0x21);
		STOREINTELDWORD(hdr.device3pan, 0x21);
		break;
	  case 0x32:
		hdr.formatversion = '3';
		STOREINTELDWORD(hdr.devicecount, 2);
		STOREINTELDWORD(hdr.device1type, 2);
		STOREINTELDWORD(hdr.device2type, 8);
		STOREINTELDWORD(hdr.device1clock, OPNA_CLOCK);
		STOREINTELDWORD(hdr.device2clock, OPNA_CLOCK);
		STOREINTELDWORD(hdr.device1pan, 0x40);
		STOREINTELDWORD(hdr.device2pan, 0x02);
		break;
	  case 0x41:
		hdr.formatversion = '3';
		STOREINTELDWORD(hdr.devicecount, 1);
		STOREINTELDWORD(hdr.device1type, 9);
		STOREINTELDWORD(hdr.device1clock, 14400000);
		break;
	  case 0x82:
		hdr.formatversion = '3';
		STOREINTELDWORD(hdr.devicecount, 2);
		STOREINTELDWORD(hdr.device1type, 2);
		STOREINTELDWORD(hdr.device2type, 10);
		STOREINTELDWORD(hdr.device1clock, OPNA_CLOCK);
		STOREINTELDWORD(hdr.device2clock, OPNA_CLOCK);
		STOREINTELDWORD(hdr.device1pan, 0x40);
		STOREINTELDWORD(hdr.device2pan, 0x02);
		break;
	  default:
		hdr.formatversion = '1';
		STOREINTELDWORD(hdr.offset, offsetof(S98HDR, title));
		break;
	}
	for (i=0; i<sizeof(hdr); i++) {
		S98_putc(*(((UINT8 *)&hdr) + i));
	}

#if 1
	// FM
	for (i = 0x30; i < 0xb8; i++)
	{
		if ((i & 3) != 3)
		{
			S98_putc(NORMAL2608);
			S98_putc((REG8)i);
			S98_putc(g_opna[0].s.reg[i]);

			S98_putc(EXTEND2608);
			S98_putc((REG8)i);
			S98_putc(g_opna[0].s.reg[i+0x100]);
		}
	}
	// PSG
	for (i = 0x00; i < 0x0e; i++)
	{
		S98_putc(NORMAL2608);
		S98_putc((REG8)i);
		S98_putc(g_opna[0].s.reg[i]);
	}
#endif

	// 一応パディング
	s98log.intcount = 10;

	sets98event(NEVENT_ABSOLUTE);
	return(SUCCESS);
}

void S98_close(void) {

	if (s98log.fh != FILEH_INVALID) {
		S98_putint();
		S98_putc(0xFD);				/* END MARK */
		S98_flush();
		nevent_reset(NEVENT_S98TIMER);
		file_close(s98log.fh);
		s98log.fh = FILEH_INVALID;
	}
}

void S98_put(REG8 module, UINT addr, REG8 data) {

	if (s98log.fh != FILEH_INVALID) {
		S98_putint();
		S98_putc(module);
		S98_putc((UINT8)addr);
		S98_putc(data);
	}
}

void S98_sync(void) {
}

BOOL S98_isopened(void)
{
	return (s98log.fh != FILEH_INVALID) ? TRUE : FALSE;
}

#endif
