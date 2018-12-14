
// PC-9821 PCI-CBusブリッジ

#include	"compiler.h"

#if defined(SUPPORT_PC9821)

#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"

#if defined(SUPPORT_PCI)

#include	"98graphbridge.h"

#define GETCFGREG_B(reg, ofs)			PCI_GETCFGREG_B(reg, ofs)
#define GETCFGREG_W(reg, ofs)			PCI_GETCFGREG_W(reg, ofs)
#define GETCFGREG_D(reg, ofs)			PCI_GETCFGREG_D(reg, ofs)

#define SETCFGREG_B(reg, ofs, value)	PCI_SETCFGREG_B(reg, ofs, value)
#define SETCFGREG_W(reg, ofs, value)	PCI_SETCFGREG_W(reg, ofs, value)
#define SETCFGREG_D(reg, ofs, value)	PCI_SETCFGREG_D(reg, ofs, value)

#define SETCFGREG_B_MASK(reg, ofs, value, mask)	PCI_SETCFGREG_B_MASK(reg, ofs, value, mask)
#define SETCFGREG_W_MASK(reg, ofs, value, mask)	PCI_SETCFGREG_W_MASK(reg, ofs, value, mask)
#define SETCFGREG_D_MASK(reg, ofs, value, mask)	PCI_SETCFGREG_D_MASK(reg, ofs, value, mask)

int pcidev_98graphbridge_deviceid = 7;

void pcidev_98graphbridge_cfgreg_w(UINT32 devNumber, UINT8 funcNumber, UINT8 cfgregOffset, UINT8 sizeinbytes, UINT32 value){

}

void pcidev_98graphbridge_reset(const NP2CFG *pConfig) {

	(void)pConfig;
}

void pcidev_98graphbridge_bind(void) {
	
	int devid = pcidev_98graphbridge_deviceid;

	// 98グラフィックバスブリッジ
	ZeroMemory(pcidev.devices+devid, sizeof(_PCIDEVICE));
	pcidev.devices[devid].enable = 1;
	pcidev.devices[devid].skipirqtbl = 1;
	pcidev.devices[devid].regwfn = &pcidev_98graphbridge_cfgreg_w;
	pcidev.devices[devid].header.vendorID = 0x1033;
	pcidev.devices[devid].header.deviceID = 0x0009;
	pcidev.devices[devid].header.command = 0x0003;//0x0006;//0x0003;
	pcidev.devices[devid].header.status = 0x0280;//0x0000;//0x0280;
	pcidev.devices[devid].header.revisionID = 0x01;
	pcidev.devices[devid].header.classcode[0] = 0x00; // レジスタレベルプログラミングインタフェース
	pcidev.devices[devid].header.classcode[1] = 0x80; // サブクラスコード
	pcidev.devices[devid].header.classcode[2] = 0x03; // ベースクラスコード
	pcidev.devices[devid].header.cachelinesize = 0;
	pcidev.devices[devid].header.latencytimer = 0x0;
	pcidev.devices[devid].header.headertype = 0;
	pcidev.devices[devid].header.BIST = 0x00;
	pcidev.devices[devid].header.interruptline = 0x00;
	pcidev.devices[devid].header.interruptpin = 0x00;
	
	// ROM領域設定
	pcidev.devices[devid].headerrom.vendorID = 0xffff;
	pcidev.devices[devid].headerrom.deviceID = 0xffff;
	pcidev.devices[devid].headerrom.status = 0xffff;
	pcidev.devices[devid].headerrom.revisionID = 0xff;
	pcidev.devices[devid].headerrom.classcode[0] = 0xff;
	pcidev.devices[devid].headerrom.classcode[1] = 0xff;
	pcidev.devices[devid].headerrom.classcode[2] = 0xff;
	pcidev.devices[devid].headerrom.baseaddrregs[0] = 0xffffffff;
	pcidev.devices[devid].headerrom.baseaddrregs[1] = 0xffffffff;
	pcidev.devices[devid].headerrom.baseaddrregs[2] = 0xffffffff;
	pcidev.devices[devid].headerrom.baseaddrregs[3] = 0xffffffff;
	pcidev.devices[devid].headerrom.baseaddrregs[4] = 0xffffffff;
	pcidev.devices[devid].headerrom.baseaddrregs[5] = 0xffffffff;
	pcidev.devices[devid].headerrom.expROMbaseaddr = 0xffffffff;
	memset(&(pcidev.devices[devid].headerrom), 0xff, sizeof(_PCICSH));
}

#endif
#endif

