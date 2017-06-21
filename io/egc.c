#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memegc.h"
#include	"vram.h"


// エンディアン修正しる。

static const UINT16 maskword[16][4] = {
				{0x0000,0x0000,0x0000,0x0000}, {0xffff,0x0000,0x0000,0x0000},
				{0x0000,0xffff,0x0000,0x0000}, {0xffff,0xffff,0x0000,0x0000},
				{0x0000,0x0000,0xffff,0x0000}, {0xffff,0x0000,0xffff,0x0000},
				{0x0000,0xffff,0xffff,0x0000}, {0xffff,0xffff,0xffff,0x0000},
				{0x0000,0x0000,0x0000,0xffff}, {0xffff,0x0000,0x0000,0xffff},
				{0x0000,0xffff,0x0000,0xffff}, {0xffff,0xffff,0x0000,0xffff},
				{0x0000,0x0000,0xffff,0xffff}, {0xffff,0x0000,0xffff,0xffff},
				{0x0000,0xffff,0xffff,0xffff}, {0xffff,0xffff,0xffff,0xffff}};


static void IOOUTCALL egc_o4a0(UINT port, REG8 value) {

	if (!(vramop.operate & (1 << VOPBIT_EGC))) {
		return;
	}

//	TRACEOUT(port, value);
	switch(port & 0x000f) {
		case 0x00:
			egc.access &= 0xff00;
			egc.access |= value;
			break;
		case 0x01:
			egc.access &= 0x00ff;
			egc.access |= value << 8;
			break;

		case 0x02:
			egc.fgbg &= 0xff00;
			egc.fgbg |= value;
			break;
		case 0x03:
			egc.fgbg &= 0x00ff;
			egc.fgbg |= value << 8;
			break;

		case 0x04:
			egc.ope &= 0xff00;
			egc.ope |= value;
			break;
		case 0x05:
			egc.ope &= 0x00ff;
			egc.ope |= value << 8;
			break;

		case 0x06:
			egc.fg &= 0xff00;
			egc.fg |= value;
			egc.fgc.d[0] = *(UINT32 *)(maskword[value & 15] + 0);
			egc.fgc.d[1] = *(UINT32 *)(maskword[value & 15] + 2);
			break;
		case 0x07:
			egc.fg &= 0x00ff;
			egc.fg |= value << 8;
			break;

		case 0x08:
			if (!(egc.fgbg & 0x6000)) {
				egc.mask._b[0] = value;
			}
			break;
		case 0x09:
			if (!(egc.fgbg & 0x6000)) {
				egc.mask._b[1] = value;
			}
			break;

		case 0x0a:
			egc.bg &= 0xff00;
			egc.bg |= value;
			egc.bgc.d[0] = *(UINT32 *)(maskword[value & 15] + 0);
			egc.bgc.d[1] = *(UINT32 *)(maskword[value & 15] + 2);
			break;
		case 0x0b:
			egc.bg &= 0x00ff;
			egc.bg |= value << 8;
			break;

		case 0x0c:
			egc.sft &= 0xff00;
			egc.sft |= value;
			egcshift();
			egc.srcmask.w = 0xffff;
			break;
		case 0x0d:
			egc.sft &= 0x00ff;
			egc.sft |= value << 8;
			egcshift();
			egc.srcmask.w = 0xffff;
			break;

		case 0x0e:
			egc.leng &= 0xff00;
			egc.leng |= value;
			egcshift();
			egc.srcmask.w = 0xffff;
			break;
		case 0x0f:
			egc.leng &= 0x00ff;
			egc.leng |= value << 8;
			egcshift();
			egc.srcmask.w = 0xffff;
			break;
	}
}

void egc_reset(const NP2CFG *pConfig) {

	ZeroMemory(&egc, sizeof(egc));
	egc.access = 0xfff0;
	egc.fgbg = 0x00ff;
	egc.mask.w = 0xffff;
	egc.leng = 0x000f;
	egcshift();
	egc.srcmask.w = 0xffff;

	(void)pConfig;
}

void egc_bind(void) {

	UINT	i;

	for (i=0; i<16; i++) {
		iocore_attachout(0x04a0 + i, egc_o4a0);
	}
}

void IOOUTCALL egc_w16(UINT port, REG16 value) {

	if (!(vramop.operate & (1 << VOPBIT_EGC))) {
		return;
	}

//	TRACEOUTW(port, value);
	switch(port & 0x000f) {
		case 0x00:
			egc.access = value;
			break;

		case 0x02:
			egc.fgbg = value;
			break;

		case 0x04:
			egc.ope = value;
			break;

		case 0x06:
			egc.fg = value;
			egc.fgc.d[0] = *(UINT32 *)(maskword[value & 15] + 0);
			egc.fgc.d[1] = *(UINT32 *)(maskword[value & 15] + 2);
			break;

		case 0x08:
			if (!(egc.fgbg & 0x6000)) {
				egc.mask.w = value;
			}
			break;

		case 0x0a:
			egc.bg = value;
			egc.bgc.d[0] = *(UINT32 *)(maskword[value & 15] + 0);
			egc.bgc.d[1] = *(UINT32 *)(maskword[value & 15] + 2);
			break;

		case 0x0c:
			egc.sft = value;
			egcshift();
			egc.srcmask.w = 0xffff;
			break;

		case 0x0e:
			egc.leng = value;
			egcshift();
			egc.srcmask.w = 0xffff;
			break;
	}
}

