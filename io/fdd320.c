#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"


// ‚±‚Á‚¿‚Åˆ—‚·‚é‚©@ƒVƒŠƒ“ƒ_”{ˆÚ“®‚ÅŒë–‚‰»‚·‚©”Y‚Ü‚µ‚¢‚Æ‚±‚ë

	UINT8	fdd320_stat;


static REG8 IOINPCALL fdd320_i51(UINT port) {

	(void)port;
	return(0x00);
}

static REG8 IOINPCALL fdd320_i55(UINT port) {

	fdd320_stat ^= 0xff;
	(void)port;
	return(fdd320_stat);
}


// ----

static const IOINP fdd320i51[4] = {
					fdd320_i51,	NULL,		fdd320_i55,	NULL};

void fdd320_reset(const NP2CFG *pConfig) {

	fdd320_stat = 0xff;
}

void fdd320_bind(void) {

	iocore_attachcmninpex(0x0051, 0x00f9, fdd320i51, 4);
}

