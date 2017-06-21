#include	"compiler.h"
#include	"sysmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"beep.h"


void IOOUTCALL dipsw_w8(UINT port, REG8 value) {

	UINT8	set;

	switch(port & 0x0f00) {
		case 0x0400:
			if (!(iflags[value] & 0x04)) {
				set = value & 0xfc;
				if ((pccore.dipsw[0] ^ set) & 0xfc) {
					pccore.dipsw[0] &= ~(0xfc);
					pccore.dipsw[0] |= set;
				}
			}
			break;

		case 0x0500:
			if (!(iflags[value] & 0x04)) {
				set = value & 0xef;
				if ((pccore.dipsw[1] ^ set) & 0xef) {
					pccore.dipsw[1] &= ~(0xef);
					pccore.dipsw[1] |= set;
				}
			}
			break;

		case 0x0600:
			if (!(iflags[value] & 0x04)) {
				set = value & 0x7f;
				if ((pccore.dipsw[2] ^ set) & 0x7f) {
					pccore.dipsw[2] &= ~(0x7f);
					pccore.dipsw[2] |= set;
				}
			}
			break;

		case 0x0700:
			if (!(iflags[value] & 0x04)) {
				set = (value & 0x20) >> 1;
				if ((pccore.dipsw[1] ^ set) & 0x10) {
					pccore.dipsw[1] ^= 0x10;
				}
				set = (value >> 2) & 0x03;
				if (np2cfg.BEEP_VOL != set) {
					np2cfg.BEEP_VOL = set;
					beep_setvol(set);
					sysmng_update(SYS_UPDATECFG);
				}
			}
			break;

		case 0x0e00:
			if (!(iflags[value] & 0x04)) {
				set = (value & 0x10) << 3;
				if ((pccore.dipsw[2] ^ set) & 0x80) {
					pccore.dipsw[2] ^= 0x80;
				}
			}
			break;
	}
}

REG8 IOINPCALL dipsw_r8(UINT port) {

	REG8	ret;

	ret = 0xff;
	switch(port & 0x0f00) {
		case 0x0400:
			ret = pccore.dipsw[0] & 0xfc;
			if (iflags[ret] & 0x04) {
				ret |= 0x01;
			}
			break;

		case 0x0500:
			ret = pccore.dipsw[1] & 0xef;
			if (iflags[ret] & 0x04) {
				ret |= 0x10;
			}
			break;

		case 0x0600:
			ret = pccore.dipsw[2] & 0x7f;
			if (iflags[ret] & 0x04) {
				ret |= 0x80;
			}
			break;

		case 0x0700:
			ret = ((pccore.dipsw[1] & 0x10) << 1) |
					((np2cfg.BEEP_VOL & 0x03) << 2);
			if (iflags[ret] & 0x04) {
				ret |= 0x80;
			}
			break;

		case 0x0e00:
			ret = (pccore.dipsw[2] & 0x80) >> 3;
			if (iflags[ret] & 0x04) {
				ret |= 0x80;
			}
			break;
	}
	return(ret);
}

