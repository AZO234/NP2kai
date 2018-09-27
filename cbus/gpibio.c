/**
 * @file	gpib.c
 * @brief	Implementation of PC-9801-06/19/29/29K/29N GP-IB(IEEE-488.1) Interface
 */

// 注意：まだ何も実装してないので使えません（GP-IBボードもそれで動く機器も持ってないし･･･）

#include	"compiler.h"

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios/biosmem.h"
#include	"gpibio.h"

#if defined(SUPPORT_GPIB)


	_GPIB		gpib;

UINT8 irq2idx(UINT8 irq){
	switch(irq){
	case 3:
		return 0x0;
	case 10:
		return 0x1;
	case 12:
		return 0x2;
	case 13:
		return 0x3;
	}
	return 0x0;
}
UINT8 idx2irq(UINT8 idx){
	switch(idx){
	case 0x0:
		return 3;
	case 0x1:
		return 10;
	case 0x2:
		return 12;
	case 0x3:
		return 13;
	}
	return 3;
}

// ----

//static const IOOUT gpib_o[] = {
//					gpib_o0, gpib_o1, gpib_o2, gpib_o3, gpib_o4, gpib_o5, gpib_o6, gpib_o7, gpib_o8, gpib_o9, gpib_oa, gpib_ob, gpib_oc, gpib_od, gpib_oe, gpib_of};
//
//static const IOINP gpib_i[] = {
//					gpib_i0, gpib_i1, gpib_i2, gpib_i3, gpib_i4, gpib_i5, gpib_i6, gpib_i7, gpib_i8, gpib_i9, gpib_ia, gpib_ib, gpib_ic, gpib_id, gpib_ie, gpib_if};

void gpibio_reset(const NP2CFG *pConfig) {
	
	OEMCHAR	path[MAX_PATH];
	FILEH	fh;
	OEMCHAR tmpbiosname[16];
	//int i;
	UINT16 iobase = 0x00D0;
	
	_tcscpy(tmpbiosname, OEMTEXT("gpib.rom"));
	getbiospath(path, tmpbiosname, NELEMENTS(path));
	fh = file_open_rb(path);

	// GP-IB BIOS 拡張ROM(D4000h - D5FFFh) 有効?
	if((np2cfg.memsw[3] & 0x20) == 0){
		gpib.enable = 0;
		return;
	}
	gpib.enable = 1;
	gpib.irq = np2cfg.gpibirq;
	gpib.mode = np2cfg.gpibmode;
	gpib.gpibaddr = np2cfg.gpibaddr;
	
	if (fh != FILEH_INVALID) {
		// GP-IB BIOS
		if (file_read(fh, mem + 0x0d4000, 0x2000) == 0x2000) {
			TRACEOUT(("load gpib.rom"));
		}else{
			//CopyMemory(mem + 0x0d4000, gpibbios, sizeof(gpibbios));
			//TRACEOUT(("use simulate gpib.rom"));
		}
		file_close(fh);
	}else{
		//CopyMemory(mem + 0x0d4000, gpibbios, sizeof(gpibbios));
		//TRACEOUT(("use simulate gpib.rom"));
	}

	//mem[0xD5400+0] = 0x01;
	//mem[0xD5400+4] = 0xD1;
	//mem[0xD5400+6] = 0x08;
	//mem[0xD5400+8] = 0xfb; // ???
	//mem[0xD5400+9] = 0x1e; // ???
	//mem[0xD5400+10] = 0x50; // ???
	//mem[0xD5400+11] = 0x33; // ???

	////// GP-IB BIOS 拡張ROM(D4000h - D5FFFh) 有効
	////np2cfg.memsw[3] |= 0x20;
	////mem[MEMB_MSW4] |= 0x20;
	//// GP-IB BIOS 拡張ROM(D4000h - D5FFFh) 無効
	//np2cfg.memsw[3] &= ~0x20;
	//mem[MEMB_MSW4] &= ~0x20;

	// AZI-4301P
	//for(i=0;i<16;i++){
	//	if(gpib_o[i]){
	//		iocore_attachout(iobase + i, gpib_o[i]);
	//	}
	//	if(gpib_i[i]){
	//		iocore_attachinp(iobase + i, gpib_i[i]);
	//	}
	//}
	////iocore_attachout(0x00d9, gpib_o9);
	////iocore_attachinp(0x00d9, gpib_i9);
	////iocore_attachout(0x00db, gpib_ob);
	////iocore_attachinp(0x00db, gpib_ib);

	(void)pConfig;
}

void gpibio_bind(void) {

	// GP-IB 有効?
	if(!gpib.enable){
		return;
	}

}
#endif

