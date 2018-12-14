
// PC-9821 PCI-CBusブリッジ

#include	"compiler.h"

#if defined(SUPPORT_PC9821)

#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"

#if defined(SUPPORT_PCI)

#include	"cbusbridge.h"

#define GETCFGREG_B(reg, ofs)			PCI_GETCFGREG_B(reg, ofs)
#define GETCFGREG_W(reg, ofs)			PCI_GETCFGREG_W(reg, ofs)
#define GETCFGREG_D(reg, ofs)			PCI_GETCFGREG_D(reg, ofs)

#define SETCFGREG_B(reg, ofs, value)	PCI_SETCFGREG_B(reg, ofs, value)
#define SETCFGREG_W(reg, ofs, value)	PCI_SETCFGREG_W(reg, ofs, value)
#define SETCFGREG_D(reg, ofs, value)	PCI_SETCFGREG_D(reg, ofs, value)

#define SETCFGREG_B_MASK(reg, ofs, value, mask)	PCI_SETCFGREG_B_MASK(reg, ofs, value, mask)
#define SETCFGREG_W_MASK(reg, ofs, value, mask)	PCI_SETCFGREG_W_MASK(reg, ofs, value, mask)
#define SETCFGREG_D_MASK(reg, ofs, value, mask)	PCI_SETCFGREG_D_MASK(reg, ofs, value, mask)

int pcidev_cbusbridge_deviceid = 6;

void pcidev_cbusbridge_cfgreg_w(UINT32 devNumber, UINT8 funcNumber, UINT8 cfgregOffset, UINT8 sizeinbytes, UINT32 value){

}

void pcidev_cbusbridge_reset(const NP2CFG *pConfig) {

	(void)pConfig;
}

void pcidev_cbusbridge_bind(void) {
	int devid = pcidev_cbusbridge_deviceid;

	// PCI-Cバスブリッジ
	ZeroMemory(pcidev.devices+devid, sizeof(_PCIDEVICE));
	pcidev.devices[devid].enable = 1;
	pcidev.devices[devid].skipirqtbl = 1;
	pcidev.devices[devid].regwfn = &pcidev_cbusbridge_cfgreg_w;
	pcidev.devices[devid].header.vendorID = 0x1033;
	pcidev.devices[devid].header.deviceID = 0x0001;
	pcidev.devices[devid].header.command = 0x010f;//0x010f;//0x0107;
	pcidev.devices[devid].header.status = 0x0200;//0x0200;//0x0400;
	pcidev.devices[devid].header.revisionID = 0x01;//0x03;
	pcidev.devices[devid].header.classcode[0] = 0x00; // レジスタレベルプログラミングインタフェース
	pcidev.devices[devid].header.classcode[1] = 0x80; // サブクラスコード
	pcidev.devices[devid].header.classcode[2] = 0x06; // ベースクラスコード
	pcidev.devices[devid].header.cachelinesize = 0;
	pcidev.devices[devid].header.latencytimer = 0x0;
	pcidev.devices[devid].header.headertype = 0;
	pcidev.devices[devid].header.BIST = 0x00;
	pcidev.devices[devid].header.interruptline = 0x00;
	pcidev.devices[devid].header.interruptpin = 0x01;
	pcidev.devices[devid].cfgreg8[0x40] = 0x10;
	pcidev.devices[devid].cfgreg8[0x41] = 0x00;
	pcidev.devices[devid].cfgreg8[0x42] = 0xEF;
	pcidev.devices[devid].cfgreg8[0x43] = 0x00;
	pcidev.devices[devid].cfgreg8[0x44] = 0xFA;
	pcidev.devices[devid].cfgreg8[0x45] = 0xff;
	pcidev.devices[devid].cfgreg8[0x46] = 0xfb;
	pcidev.devices[devid].cfgreg8[0x47] = 0xff;
	pcidev.devices[devid].cfgreg8[0x48] = 0xFE;
	pcidev.devices[devid].cfgreg8[0x49] = 0xFF;
	pcidev.devices[devid].cfgreg8[0x4A] = 0xFE;
	pcidev.devices[devid].cfgreg8[0x4B] = 0xFF;
	pcidev.devices[devid].cfgreg8[0x4C] = 0xFF;
	pcidev.devices[devid].cfgreg8[0x4D] = 0xFF;
	pcidev.devices[devid].cfgreg8[0x4E] = 0x00;
	pcidev.devices[devid].cfgreg8[0x4F] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x40] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x41] = 0x10;
	//pcidev.devices[devid].cfgreg8[0x42] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x43] = 0xff;
	//pcidev.devices[devid].cfgreg8[0x44] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x45] = 0xff;
	//pcidev.devices[devid].cfgreg8[0x46] = 0xff;
	//pcidev.devices[devid].cfgreg8[0x47] = 0xff;
	//pcidev.devices[devid].cfgreg8[0x48] = 0x3f;
	//pcidev.devices[devid].cfgreg8[0x4C] = 0x3e;
	//pcidev.devices[devid].cfgreg8[0x4E] = 0x03;
	pcidev.devices[devid].cfgreg8[0x59] = 9;
	pcidev.devices[devid].cfgreg8[0x5a] = 10;
	pcidev.devices[devid].cfgreg8[0x5b] = 11;
	pcidev.devices[devid].cfgreg8[0x5d] = 13;
	//SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x50, 0x00EF0010);
	//SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x54, 0xFFFBFFFA);
	//SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x58, 0xFFFEFFFE);
	//SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x5C, 0x0000FFFF);
	pcidev.devices[devid].cfgreg8[0x60] = 0x03;//0x80;//0x03;
	pcidev.devices[devid].cfgreg8[0x61] = 0x05;//0x80;//0x05;
	pcidev.devices[devid].cfgreg8[0x62] = 0x06;//0x80;//0x06;
	pcidev.devices[devid].cfgreg8[0x63] = 0x0c;//0x80;//0x0c;

	//// PCI-Cバスブリッジ（違うバージョン）
	//ZeroMemory(pcidev.devices+devid, sizeof(_PCIDEVICE));
	//pcidev.devices[devid].enable = 1;
	//pcidev.devices[devid].regwfn = &pcidev_cbusbridge_cfgreg_w;
	//pcidev.devices[devid].header.vendorID = 0x1033;
	//pcidev.devices[devid].header.deviceID = 0x0001;
	//pcidev.devices[devid].header.command = 0x010f;//0x0107;
	//pcidev.devices[devid].header.status = 0x0200;//0x0400;
	//pcidev.devices[devid].header.revisionID = 0x00;//0x03;
	//pcidev.devices[devid].header.classcode[0] = 0x00; // レジスタレベルプログラミングインタフェース
	//pcidev.devices[devid].header.classcode[1] = 0x80; // サブクラスコード
	//pcidev.devices[devid].header.classcode[2] = 0x06; // ベースクラスコード
	//pcidev.devices[devid].header.cachelinesize = 0;
	//pcidev.devices[devid].header.latencytimer = 0x0;
	//pcidev.devices[devid].header.headertype = 0;
	//pcidev.devices[devid].header.BIST = 0x00;
	//pcidev.devices[devid].header.interruptline = 0x00;
	//pcidev.devices[devid].header.interruptpin = 0x00;
	//pcidev.devices[devid].cfgreg8[0x40] = 0x10;
	//pcidev.devices[devid].cfgreg8[0x41] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x42] = 0xEF;
	//pcidev.devices[devid].cfgreg8[0x43] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x44] = 0xFA;
	//pcidev.devices[devid].cfgreg8[0x45] = 0xff;
	//pcidev.devices[devid].cfgreg8[0x46] = 0xfb;
	//pcidev.devices[devid].cfgreg8[0x47] = 0xff;
	//pcidev.devices[devid].cfgreg8[0x48] = 0xFE;
	//pcidev.devices[devid].cfgreg8[0x49] = 0xFF;
	//pcidev.devices[devid].cfgreg8[0x4A] = 0xFE;
	//pcidev.devices[devid].cfgreg8[0x4B] = 0xFF;
	//pcidev.devices[devid].cfgreg8[0x4C] = 0xFF;
	//pcidev.devices[devid].cfgreg8[0x4D] = 0xFF;
	//pcidev.devices[devid].cfgreg8[0x4E] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x4F] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x50] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x51] = 0x01;
	//pcidev.devices[devid].cfgreg8[0x52] = 0x02;
	//pcidev.devices[devid].cfgreg8[0x53] = 0x0F;
	//pcidev.devices[devid].cfgreg8[0x54] = 0x04;
	//pcidev.devices[devid].cfgreg8[0x55] = 0x05;
	//pcidev.devices[devid].cfgreg8[0x56] = 0x86;
	//pcidev.devices[devid].cfgreg8[0x57] = 0x07;
	//pcidev.devices[devid].cfgreg8[0x58] = 0x08;
	//pcidev.devices[devid].cfgreg8[0x59] = 0x09;
	//pcidev.devices[devid].cfgreg8[0x5A] = 0x0A;
	//pcidev.devices[devid].cfgreg8[0x5B] = 0x0B;
	//pcidev.devices[devid].cfgreg8[0x5C] = 0x8C;
	//pcidev.devices[devid].cfgreg8[0x5D] = 0x0D;
	//pcidev.devices[devid].cfgreg8[0x5E] = 0x0E;
	//pcidev.devices[devid].cfgreg8[0x5F] = 0x8F;
	////pcidev.devices[devid].cfgreg8[0x59] = 9;
	////pcidev.devices[devid].cfgreg8[0x5a] = 10;
	////pcidev.devices[devid].cfgreg8[0x5b] = 11;
	////pcidev.devices[devid].cfgreg8[0x5d] = 13;
	////SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x50, 0x00EF0010);
	////SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x54, 0xFFFBFFFA);
	////SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x58, 0xFFFEFFFE);
	////SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x5C, 0x0000FFFF);
	////pcidev.devices[devid].cfgreg8[0x60] = 0x03;//0x80;//0x03;
	////pcidev.devices[devid].cfgreg8[0x61] = 0x05;//0x80;//0x05;
	////pcidev.devices[devid].cfgreg8[0x62] = 0x06;//0x80;//0x06;
	////pcidev.devices[devid].cfgreg8[0x63] = 0x80;//0x0c;//0x80;//0x0c;
	//pcidev.devices[devid].cfgreg8[0x60] = 0x0A;//0x80;//0x03;
	//pcidev.devices[devid].cfgreg8[0x61] = 0x0A;//0x80;//0x05;
	//pcidev.devices[devid].cfgreg8[0x62] = 0x0A;//0x80;//0x06;
	//pcidev.devices[devid].cfgreg8[0x63] = 0x0A;//0x0c;//0x80;//0x0c;
	//pcidev.devices[devid].cfgreg8[0x64] = 0x10;
	//pcidev.devices[devid].cfgreg8[0x65] = 0x3F;
	//pcidev.devices[devid].cfgreg8[0x66] = 0x07;
	//pcidev.devices[devid].cfgreg8[0x67] = 0x18;
	//pcidev.devices[devid].cfgreg8[0x68] = 0x32;
	//pcidev.devices[devid].cfgreg8[0x69] = 0x01;
	//pcidev.devices[devid].cfgreg8[0x6A] = 0x00;
	//pcidev.devices[devid].cfgreg8[0x6B] = 0x0D;

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

