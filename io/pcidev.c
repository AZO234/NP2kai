#include	"compiler.h"

#if defined(SUPPORT_PC9821)

#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"


// ‚Æ‚è‚ ‚¦‚¸ config #1 type0ŒÅ’è‚Åc

static void pcidevset10(UINT32 addr, REG8 dat) {

	UINT32	work;

	switch(addr) {
		case 0x000064 + 3:
			pcidev.membankd0 = dat;
			work = CPU_RAM_D000 & 0x03ff;
			if (dat & 0x10) {
				work |= 0x0400;
			}
			if (dat & 0x20) {
				work |= 0x0800;
			}
			if (dat & 0x80) {
				work |= 0xf000;
			}
			CPU_RAM_D000 = (UINT16)work;
			break;
	}
}

static REG8 pcidevget10(UINT32 addr) {

	switch(addr) {
		case 0x000064 + 3:
			return(pcidev.membankd0);
	}
	return(0xff);
}


// ----

static void IOOUTCALL pci_o04(UINT port, REG8 dat) {

	UINT32	addr;

	if (pcidev.base & 0x80000000) {
		addr = pcidev.base & 0x00fffffc;
		addr += port & 3;
		pcidevset10(addr, dat);
	}
}

static REG8 IOINPCALL pci_i04(UINT port) {

	UINT32	addr;

	if (pcidev.base & 0x80000000) {
		addr = pcidev.base & 0x00fffffc;
		addr += port & 3;
		return(pcidevget10(addr));
	}
	else {
		return(0xff);
	}
}

void IOOUTCALL pcidev_w32(UINT port, UINT32 value) {

	UINT32	addr;

	if (!(port & 4)) {
		pcidev.base = value;
	}
	else {
		if (pcidev.base & 0x80000000) {
			addr = pcidev.base & 0x00fffffc;
			pcidevset10(addr + 0, (UINT8)(value >> 0));
			pcidevset10(addr + 1, (UINT8)(value >> 8));
			pcidevset10(addr + 2, (UINT8)(value >> 16));
			pcidevset10(addr + 3, (UINT8)(value >> 24));
		}
	}
}

UINT32 IOOUTCALL pcidev_r32(UINT port) {

	UINT32	ret;
	UINT32	addr;

	ret = (UINT32)-1;
	if (!(port & 4)) {
		ret = pcidev.base;
	}
	else {
		if (pcidev.base & 0x80000000) {
			addr = pcidev.base & 0x00fffffc;
			ret = pcidevget10(addr + 0);
			ret |= (pcidevget10(addr + 1) << 8);
			ret |= (pcidevget10(addr + 2) << 16);
			ret |= (pcidevget10(addr + 3) << 24);
		}
	}
	return(ret);
}

void pcidev_reset(const NP2CFG *pConfig) {

	ZeroMemory(&pcidev, sizeof(pcidev));

	(void)pConfig;
}

void pcidev_bind(void) {

	UINT	i;

	for (i=0x0cfc; i<0x0d00; i++) {
		iocore_attachout(i, pci_o04);
		iocore_attachinp(i, pci_i04);
	}
}
#endif

