#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"


// ---- I/O

static void IOOUTCALL nmiio_o50(UINT port, REG8 dat) {

	nmiio.enable = 0;
	(void)port;
	(void)dat;
}

static void IOOUTCALL nmiio_o52(UINT port, REG8 dat) {

	nmiio.enable = 1;
	(void)port;
	(void)dat;
}


// ---- I/F

static const IOOUT nmiioo50[2] = {
					nmiio_o50,	nmiio_o52};

void nmiio_reset(const NP2CFG *pConfig) {

	ZeroMemory(&nmiio, sizeof(nmiio));

	(void)pConfig;
}

void nmiio_bind(void) {

	iocore_attachsysoutex(0x0050, 0x0cf1, nmiioo50, 2);
//	iocore_attachinp(0x98f0, nmiio_i98f0);
}

