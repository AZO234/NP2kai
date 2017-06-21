#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"


// EPSON専用ポート 0c00～

static void bankselect(void) {

	if (epsonio.bankioen & 0x02) {
		CPU_ITFBANK = 1;
		TRACEOUT(("EPSON ITF - Enable"));
	}
	else {
		CPU_ITFBANK = 0;
		TRACEOUT(("EPSON ITF - Disable"));
	}
}

// ---- I/O

static void IOOUTCALL epsonio_o043d(UINT port, REG8 dat) {

	switch(dat) {
		case 0x00:
			if (epsonio.bankioen & 0x01) {
				epsonio.bankioen &= ~0x02;
				bankselect();
			}
			break;

		case 0x02:
			if (epsonio.bankioen & 0x01) {
				epsonio.bankioen |= 0x02;
				bankselect();
			}
			break;

		case 0x10:
			CPU_ITFBANK = 1;
			break;

		case 0x12:
			CPU_ITFBANK = 0;
			break;
	}
	(void)port;
}

static void IOOUTCALL epsonio_o043f(UINT port, REG8 dat) {

	switch(dat) {
		case 0x40:
			epsonio.bankioen &= ~0x01;
			break;

		case 0x42:
			epsonio.bankioen |= 0x01;
			break;
	}
	(void)port;
}

static void IOOUTCALL epsonio_oc07(UINT port, REG8 dat) {

	TRACEOUT(("EPSON ROM MODE - %.2x", dat));

	switch(dat) {
		case 0x2a:	// 0010|1010
		case 0x2b:
			CopyMemory(mem + 0x1e8000, mem + 0x1c8000, 0x18000);
			break;

		case 0x2c:	// 0010|1100
		case 0x2d:
			CopyMemory(mem + 0x1e8000, mem + 0x0e8000, 0x10000);
			CopyMemory(mem + 0x1f8000, mem + 0x1c0000, 0x08000);
			break;

		case 0xa6:	// 1010|0110
			CopyMemory(mem + 0x1c8000, mem + 0x0e8000, 0x10000);
			CopyMemory(mem + 0x1d8000, mem + 0x1c0000, 0x08000);
			CopyMemory(mem + 0x1e8000, mem + 0x0e8000, 0x10000);
			CopyMemory(mem + 0x1f8000, mem + 0x1c0000, 0x08000);
			break;

		case 0xe6:	// 1110|0110
			CopyMemory(mem + 0x1d8000, mem + 0x1c0000, 0x08000);
			CopyMemory(mem + 0x1f8000, mem + 0x1c0000, 0x08000);
			break;
	}
	(void)port;
}

static REG8 IOINPCALL epsonio_ic03(UINT port) {

	(void)port;
	return(epsonio.cpumode);
}

static REG8 IOINPCALL epsonio_ic13(UINT port) {

	(void)port;
	return(0x00);
}


// ---- I/F

void epsonio_reset(const NP2CFG *pConfig) {

	epsonio.cpumode = 'R';

	(void)pConfig;
}

void epsonio_bind(void) {

	if (pccore.model & PCMODEL_EPSON) {
		iocore_attachout(0x043d, epsonio_o043d);
		iocore_attachout(0x043f, epsonio_o043f);
		iocore_attachout(0x0c07, epsonio_oc07);
		iocore_attachinp(0x0c03, epsonio_ic03);
		iocore_attachinp(0x0c13, epsonio_ic13);
		iocore_attachinp(0x0c14, epsonio_ic13);
	}
}

