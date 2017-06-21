#include	"compiler.h"
#include	"commng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"


	COMMNG	cm_prt;


// ---- I/O

static void IOOUTCALL prt_o40(UINT port, REG8 dat) {

	COMMNG	prt;

	prt = cm_prt;
	if (prt == NULL) {
		prt = commng_create(COMCREATE_PRINTER);
		cm_prt = prt;
	}
	prt->write(prt, (UINT8)dat);
//	TRACEOUT(("prt - %.2x", dat));
	(void)port;
}

static REG8 IOINPCALL prt_i42(UINT port) {

	REG8	ret;

	ret = 0x84;
	if (pccore.cpumode & CPUMODE_8MHZ) {
		ret |= 0x20;
	}
	if (pccore.dipsw[0] & 4) {
		ret |= 0x10;
	}
	if (pccore.dipsw[0] & 0x80) {
		ret |= 0x08;
	}
	if (!(pccore.model & PCMODEL_EPSON)) {
		if (CPU_TYPE & CPUTYPE_V30) {
			ret |= 0x02;
		}
	}
	else {
		if (pccore.dipsw[2] & 0x80) {
			ret |= 0x02;
		}
	}
	(void)port;
	return(ret);
}


// ---- I/F

static const IOOUT prto40[4] = {
					prt_o40,	NULL,		NULL,		NULL};

static const IOINP prti40[4] = {
					NULL,		prt_i42,	NULL,		NULL};

void printif_reset(const NP2CFG *pConfig) {

	commng_destroy(cm_prt);
	cm_prt = NULL;

	(void)pConfig;
}

void printif_bind(void) {

	iocore_attachsysoutex(0x0040, 0x0cf1, prto40, 4);
	iocore_attachsysinpex(0x0040, 0x0cf1, prti40, 4);
}

