#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"


void artic_callback(void) {

	SINT32	mul;
	SINT32	leng;

	mul = pccore.multiple;
	if (pccore.cpumode & CPUMODE_8MHZ) {
		mul *= 13;
	}
	else {
		mul *= 16;
	}
	leng = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	leng *= 2;
	leng -= artic.lastclk2;
	if (leng > 0) {
		leng /= mul;
		artic.counter += leng;
		artic.lastclk2 += leng * mul;
	}
}

static UINT32 artic_getcnt(void) {

	SINT32	mul;
	SINT32	leng;

	mul = pccore.multiple;
	if (pccore.cpumode & CPUMODE_8MHZ) {
		mul *= 13;
	}
	else {
		mul *= 16;
	}
	leng = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	leng *= 2;
	leng -= artic.lastclk2;
	if (leng > 0) {
		leng /= mul;
		return(artic.counter + leng);
	}
	return(artic.counter);
}


// ---- I/O

static void IOOUTCALL artic_o5f(UINT port, REG8 dat) {

	(void)port;
	(void)dat;
	CPU_REMCLOCK -= 20;
}

static REG8 IOINPCALL artic_i5c(UINT port) {

	(void)port;
	return((UINT8)artic_getcnt());
}

static REG8 IOINPCALL artic_i5d(UINT port) {

	(void)port;
	return((UINT8)(artic_getcnt() >> 8));
}

static REG8 IOINPCALL artic_i5f(UINT port) {

	(void)port;
	return((UINT8)(artic_getcnt() >> 16));
}


// ---- I/F

void artic_reset(const NP2CFG *pConfig) {

	ZeroMemory(&artic, sizeof(artic));

	(void)pConfig;
}

void artic_bind(void) {

	iocore_attachout(0x005f, artic_o5f);
	iocore_attachinp(0x005c, artic_i5c);
	iocore_attachinp(0x005d, artic_i5d);
	iocore_attachinp(0x005e, artic_i5d);
	iocore_attachinp(0x005f, artic_i5f);
}

REG16 IOINPCALL artic_r16(UINT port) {

	UINT32	cnt;

	cnt = artic_getcnt();
	if (port & 2) {
		cnt >>= 8;
	}
	return((UINT16)cnt);
}

