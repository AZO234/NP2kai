#include	<compiler.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>


// ---- I/O

static void IOOUTCALL necio_o0439(UINT port, REG8 dat) {

	necio.port0439 = dat;
	(void)port;
}

static void IOOUTCALL necio_o043b(UINT port, REG8 dat) {

	necio.port043b = dat & 0x04;
	(void)port;
}
static REG8 IOOUTCALL necio_i043b(UINT port) {
	
	return(necio.port043b);
}

static void IOOUTCALL necio_o043d(UINT port, REG8 dat) {

	switch(dat) {
		case 0x10:
			CPU_ITFBANK = 1;
			break;

		case 0x12:
			CPU_ITFBANK = 0;
			break;
	}
	(void)port;
}


// ---- I/F

void necio_reset(const NP2CFG *pConfig) {

	necio.port0439 = 0xff;

	(void)pConfig;
}

void necio_bind(void) {

	iocore_attachout(0x0439, necio_o0439);

	if (!(pccore.model & PCMODEL_EPSON)) {
		iocore_attachout(0x043b, necio_o043b);
		iocore_attachinp(0x043b, necio_i043b);
		iocore_attachout(0x043d, necio_o043d);
	}
}

