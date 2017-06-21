#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"beep.h"


// ---- I/O

static void IOOUTCALL sysp_o35(UINT port, REG8 dat) {

	if ((sysport.c ^ dat) & 0x04) {					// ver0.29
		rs232c.send = 1;
	}
	sysport.c = dat;
	beep_oneventset();
	(void)port;
}

static void IOOUTCALL sysp_o37(UINT port, REG8 dat) {

	REG8	bit;

	if (!(dat & 0xf0)) {
		bit = 1 << (dat >> 1);
		if (dat & 1) {
			sysport.c |= bit;
		}
		else {
			sysport.c &= ~bit;
		}
		if (bit == 0x04) {									// ver0.29
			rs232c.send = 1;
		}
		else if (bit == 0x08) {
			beep_oneventset();
		}
	}
	(void)port;
}

static REG8 IOINPCALL sysp_i31(UINT port) {

	(void)port;
	return(pccore.dipsw[1]);
}

static REG8 IOINPCALL sysp_i33(UINT port) {

	REG8	ret;

	ret = ((~pccore.dipsw[0]) & 1) << 3;
	ret |= rs232c_stat();
	ret |= uPD4990.cdat;
	(void)port;
	return(ret);
}

static REG8 IOINPCALL sysp_i35(UINT port) {

	(void)port;
	return(sysport.c);
}


// ---- I/F

static const IOOUT syspo31[4] = {
					NULL,		NULL,		sysp_o35,	sysp_o37};

static const IOINP syspi31[4] = {
					sysp_i31,	sysp_i33,	sysp_i35,	NULL};

void systemport_reset(const NP2CFG *pConfig) {

	sysport.c = 0xf9;
	beep_oneventset();

	(void)pConfig;
}

void systemport_bind(void) {

	iocore_attachsysoutex(0x0031, 0xcf1, syspo31, 4);
	iocore_attachsysinpex(0x0031, 0xcf1, syspi31, 4);
}

