#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"

#if defined(SUPPORT_PCI)
#include "ia32/cpu.h"
#include "ia32/instructions/data_trans.h"
#endif

// ---- CMT

void bios0x1a_cmt(void) {

	if (CPU_AH == 0x04) {
		CPU_AH = 0x02;
	}
	else {
		CPU_AH = 0x00;
	}
}


// ---- Printer

static void printerbios_11(void) {

	if (iocore_inp8(0x42) & 0x04) {				// busy?
		CPU_AH = 0x01;
		iocore_out8(0x40, CPU_AL);
#if 0
		iocore_out8(0x46, 0x0e);
		iocore_out8(0x46, 0x0f);
#endif
	}
	else {
		CPU_AH = 0x02;
	}
}

void bios0x1a_prt(void) {

	switch(CPU_AH & 0x0f) {
		case 0x00:
			if (CPU_AH == 0x30) {
				if (CPU_CX) {
					do {
						CPU_AL = MEMR_READ8(CPU_ES, CPU_BX);
						printerbios_11();
						if (CPU_AH & 0x02) {
							CPU_AH = 0x02;
							return;
						}
						CPU_BX++;
					} while(--CPU_CX);
					CPU_AH = 0x00;
				}
				else {
					CPU_AH = 0x02;
				}
			}
			else {
				iocore_out8(0x37, 0x0d);				// printer f/f
				iocore_out8(0x46, 0x82);				// reset
				iocore_out8(0x46, 0x0f);				// PSTB inactive
				iocore_out8(0x37, 0x0c);				// printer f/f
				CPU_AH = (iocore_inp8(0x42) >> 2) & 1;
			}
			break;

		case 0x01:
			printerbios_11();
			break;

		case 0x02:
			CPU_AH = (iocore_inp8(0x42) >> 2) & 1;
			break;

		default:
			CPU_AH = 0x00;
			break;
	}
}


#if defined(SUPPORT_PCI)

// ---- PCI

enum {
	PCIBIOS_STATUS_SUCCESSFUL			= 0x00,
	PCIBIOS_STATUS_UNSUPPORTED_FUNCTION	= 0x81,
	PCIBIOS_STATUS_BAD_VENDOR_ID		= 0x83,
	PCIBIOS_STATUS_DEVICE_NOT_FOUND		= 0x86,
	PCIBIOS_STATUS_BAD_PCI_REG_NUMBER	= 0x87,
	PCIBIOS_STATUS_SET_FAILED			= 0x88,
	PCIBIOS_STATUS_BUFFER_TOO_SMALL		= 0x89,
};

void bios0x1a_pci_part(int is32bit) {

	int i;
	int idx;
	int devnum;
	int funcnum;
	
	switch(CPU_AL & 0x7f) {
		case 0x01: // INSTALLATION CHECK
			CPU_AH = 0x00;
			CPU_FLAGL &= ~C_FLAG;
			CPU_EDX = 0x20494350; // " ICP"
			//CPU_EDI = 0; // XXX: physical address of protected-mode entry point
			CPU_AL = 0x01; // configuration space access mechanism 1 supported
			CPU_BH = 0x02; // PCI interface level major version (BCD)
			CPU_BL = 0x00; // PCI interface level minor version (BCD)
			CPU_CL = 0x0; // number of last PCI bus in system

			pcidev_updateBIOS32data();
			break;

		case 0x02: // FIND PCI DEVICE
			// デバイスを探す
			for(i=0;i<PCI_DEVICES_MAX;i++){
				if(pcidev.devices[i].enable){
					if(pcidev.devices[i].header.deviceID == CPU_CX && pcidev.devices[i].header.vendorID == CPU_DX){
						break;
					}
				}
			}
			if(i < PCI_DEVICES_MAX){
				// 発見
				CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
				CPU_FLAGL &= ~C_FLAG;
				CPU_BH = 0x0; // bus number
				CPU_BL = (i << 3)|(0); // device/function number (bits 7-3 device, bits 2-0 func)
			}else{
				// 発見できず
				CPU_AH = PCIBIOS_STATUS_DEVICE_NOT_FOUND;
				CPU_FLAGL |= C_FLAG;
			}
			break;

		case 0x03: // FIND PCI CLASS CODE
			// デバイスを探す
			idx = CPU_SI; // 同じクラスを持つデバイスのうち、idx番目に見つけたものを返す
			for(i=0;i<PCI_DEVICES_MAX;i++){
				if(pcidev.devices[i].enable){
					if((*((UINT32*)pcidev.devices[i].header.classcode) & 0xffffff) == (CPU_ECX & 0xffffff)){
						if(idx==0)
							break;
						idx--;
					}
				}
			}
			if(i < PCI_DEVICES_MAX){
				// 発見
				CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
				CPU_FLAGL &= ~C_FLAG;
				CPU_BH = 0x0; // bus number
				CPU_BL = (i << 3)|(0); // device/function number (bits 7-3 device, bits 2-0 func)
			}else{
				// 発見できず
				CPU_AH = PCIBIOS_STATUS_DEVICE_NOT_FOUND;
				CPU_FLAGL |= C_FLAG;
			}
			break;
			
		case 0x06: // PCI BUS-SPECIFIC OPERATIONS
			//CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
			//CPU_FLAGL &= ~C_FLAG;
			CPU_AH = PCIBIOS_STATUS_UNSUPPORTED_FUNCTION;
			CPU_FLAGL |= C_FLAG;
			break;
			
		case 0x08: // READ CONFIGURATION BYTE
			devnum = CPU_BL >> 3;
			funcnum = CPU_BL & 0x7;
			if(CPU_BH==0/* && funcnum==0*/ && pcidev.devices[devnum].enable){
				if(CPU_DI <= 0xff){
					CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
					CPU_FLAGL &= ~C_FLAG;
					CPU_CL = pcidev.devices[devnum].cfgreg8[CPU_DI];
				}else{
					CPU_AH = PCIBIOS_STATUS_BAD_PCI_REG_NUMBER;
					CPU_FLAGL |= C_FLAG;
					CPU_CL = 0xff;
				}
			}else{
				//CPU_AH = PCIBIOS_STATUS_DEVICE_NOT_FOUND;
				//CPU_FLAGL |= C_FLAG;
				CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
				CPU_FLAGL &= ~C_FLAG;
				CPU_CL = 0xff;
			}
			break;
		case 0x09: // READ CONFIGURATION WORD
			devnum = CPU_BL >> 3;
			funcnum = CPU_BL & 0x7;
			if(CPU_BH==0/* && funcnum==0*/ && pcidev.devices[devnum].enable){
				if(CPU_DI <= 0xff){
					CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
					CPU_FLAGL &= ~C_FLAG;
					CPU_CX = *((UINT16*)(pcidev.devices[devnum].cfgreg8 + CPU_DI)); // XXX: 2の倍数のレジスタ番号でなくても読めちゃうけどまあいいかー
				}else{
					CPU_AH = PCIBIOS_STATUS_BAD_PCI_REG_NUMBER;
					CPU_FLAGL |= C_FLAG;
					CPU_CX = 0xffff;
				}
			}else{
				//CPU_AH = PCIBIOS_STATUS_DEVICE_NOT_FOUND;
				//CPU_FLAGL |= C_FLAG;
				CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
				CPU_FLAGL &= ~C_FLAG;
				CPU_CX = 0xffff;
			}
			break;
		case 0x0A: // READ CONFIGURATION DWORD
			devnum = CPU_BL >> 3;
			funcnum = CPU_BL & 0x7;
			if(CPU_BH==0/* && funcnum==0*/ && pcidev.devices[devnum].enable){
				if(CPU_DI <= 0xff){
					CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
					CPU_FLAGL &= ~C_FLAG;
					CPU_ECX = *((UINT32*)(pcidev.devices[devnum].cfgreg8 + CPU_DI)); // XXX: 4の倍数のレジスタ番号でなくても読めちゃうけどまあいいかー
				}else{
					CPU_AH = PCIBIOS_STATUS_BAD_PCI_REG_NUMBER;
					CPU_FLAGL |= C_FLAG;
					CPU_ECX = 0xffffffff;
				}
			}else{
				//CPU_AH = PCIBIOS_STATUS_DEVICE_NOT_FOUND;
				//CPU_FLAGL |= C_FLAG;
				CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
				CPU_FLAGL &= ~C_FLAG;
				CPU_ECX = 0xffffffff;
			}
			break;
			
		case 0x0B: // WRITE CONFIGURATION BYTE
			devnum = CPU_BL >> 3;
			funcnum = CPU_BL & 0x7;
			if(CPU_BH==0/* && funcnum==0*/ && pcidev.devices[devnum].enable){
				if(CPU_DI <= 0xff){
					CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
					CPU_FLAGL &= ~C_FLAG;
					pcidev.devices[devnum].cfgreg8[CPU_DI] = CPU_CL;
				}else{
					CPU_AH = PCIBIOS_STATUS_BAD_PCI_REG_NUMBER;
					CPU_FLAGL |= C_FLAG;
				}
			}else{
				CPU_AH = PCIBIOS_STATUS_DEVICE_NOT_FOUND;
				CPU_FLAGL |= C_FLAG;
			}
			break;
		case 0x0C: // WRITE CONFIGURATION WORD
			devnum = CPU_BL >> 3;
			funcnum = CPU_BL & 0x7;
			if(CPU_BH==0/* && funcnum==0*/ && pcidev.devices[devnum].enable){
				if(CPU_DI <= 0xff){
					CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
					CPU_FLAGL &= ~C_FLAG;
					*((UINT16*)(pcidev.devices[devnum].cfgreg8 + CPU_DI)) = CPU_CX; // XXX: 2の倍数のレジスタ番号でなくても書けちゃうけどまあいいかー
				}else{
					CPU_AH = PCIBIOS_STATUS_BAD_PCI_REG_NUMBER;
					CPU_FLAGL |= C_FLAG;
				}
			}else{
				CPU_AH = PCIBIOS_STATUS_DEVICE_NOT_FOUND;
				CPU_FLAGL |= C_FLAG;
			}
			break;
		case 0x0D: // WRITE CONFIGURATION DWORD
			devnum = CPU_BL >> 3;
			funcnum = CPU_BL & 0x7;
			if(CPU_BH==0/* && funcnum==0*/ && pcidev.devices[devnum].enable){
				if(CPU_DI <= 0xff){
					CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
					CPU_FLAGL &= ~C_FLAG;
					*((UINT32*)(pcidev.devices[devnum].cfgreg8 + CPU_DI)) = CPU_ECX; // XXX: 4の倍数のレジスタ番号でなくても書けちゃうけどまあいいかー
				}else{
					CPU_AH = PCIBIOS_STATUS_BAD_PCI_REG_NUMBER;
					CPU_FLAGL |= C_FLAG;
				}
			}else{
				CPU_AH = PCIBIOS_STATUS_DEVICE_NOT_FOUND;
				CPU_FLAGL |= C_FLAG;
			}
			break;
			
		case 0x0E: // GET IRQ ROUTING INFORMATION
			if(CPU_BX == 0x0000){
				UINT16 dataSize = 0;
				UINT32 dataAddress = 0; // seg:ofs
				pcidev_updateBIOS32data();
				if(is32bit){
					// 32bit
					dataSize = MEMR_READ16(CPU_ES, CPU_EDI);
					dataAddress = (UINT32)MEMR_READ16(CPU_ES, CPU_EDI+2)|(((UINT32)MEMR_READ16(CPU_ES, CPU_EDI+4)) << 16);
					if(dataSize < pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY)){
						MEMR_WRITE16(CPU_ES, CPU_EDI, pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY));
						CPU_AH = PCIBIOS_STATUS_BUFFER_TOO_SMALL;
						CPU_FLAGL |= C_FLAG;
					}else{
						dataSize = pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY);
						MEMR_WRITE16(CPU_ES, CPU_EDI, dataSize);
						MEMR_WRITES((dataAddress >> 16) & 0xffff, dataAddress & 0xffff, pcidev.biosdata.data, dataSize);
						CPU_BX = pcidev.allirqbitmap;
						CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
						CPU_FLAGL &= ~C_FLAG;
					}
				}else{
					// 16bit
					dataSize = MEMR_READ16(CPU_ES, CPU_DI);
					dataAddress = (UINT32)MEMR_READ16(CPU_ES, CPU_DI+2)|(((UINT32)MEMR_READ16(CPU_ES, CPU_DI+4)) << 16);
					if(dataSize < pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY)){
						MEMR_WRITE16(CPU_ES, CPU_DI, pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY));
						CPU_AH = PCIBIOS_STATUS_BUFFER_TOO_SMALL;
						CPU_FLAGL |= C_FLAG;
					}else{
						dataSize = pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY);
						MEMR_WRITE16(CPU_ES, CPU_DI, dataSize);
						MEMR_WRITES((dataAddress >> 16) & 0xffff, dataAddress & 0xffff, pcidev.biosdata.data, dataSize);
						CPU_BX = pcidev.allirqbitmap;
						CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
						CPU_FLAGL &= ~C_FLAG;
					}
				}
			}else{
				CPU_AH = PCIBIOS_STATUS_UNSUPPORTED_FUNCTION;
				CPU_FLAGL |= C_FLAG;
			}
			break;
			
		case 0x0F: // SET PCI IRQ
			devnum = CPU_BL >> 3;
			funcnum = CPU_BL & 0x7;
			//if(CPU_BH==0 && 0x0A <= CPU_CL && CPU_CL <= 0x0D && (CPU_CH & 0xf0)==0 && pcidev.devices[devnum].enable){
			//	UINT8 intpinidx = CPU_CL - 0x0A;
			//	CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
			//	CPU_FLAGL &= ~C_FLAG;
			//}else{
				CPU_AH = PCIBIOS_STATUS_SET_FAILED;
				CPU_FLAGL |= C_FLAG;
			//}
			break;

		default:
			if(CPU_EAX==0x49435024){
				// Find BIOS32 Service Directory Entry Point by using $PCI signature
				CPU_EBX = (pcidev.bios32entrypoint & 0xff000);
				CPU_ECX = 1;
				CPU_EDX = (pcidev.bios32entrypoint & 0xfff);
				CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
				CPU_AL = 0;
				CPU_FLAGL &= ~C_FLAG;
			}else{
				CPU_AH = PCIBIOS_STATUS_UNSUPPORTED_FUNCTION;
				CPU_FLAGL |= C_FLAG;
			}
			break;
	}
}
void bios0x1a_pci(void) {

	UINT32 oldDX;
	oldDX = CPU_DX;
	
	if(pcidev.enable){
#if !defined(SUPPORT_IA32_HAXM)
		// XXX: np2 BIOSがDXレジスタをPUSH/POPしてしまうので、DXレジスタの内容をスタックから強引に拾ってくる
		CPU_DX = cpu_vmemoryread_w(CPU_SS_INDEX, CPU_SP + 2);
#endif

		// 16bit PCI BIOS処理
		bios0x1a_pci_part(0);
		
#if !defined(SUPPORT_IA32_HAXM)
		// XXX: np2 BIOSがDXレジスタをPUSH/POPしてしまうので、DXレジスタの内容をスタックに強引に書き込む
		cpu_vmemorywrite_w(CPU_SS_INDEX, CPU_SP + 2, (UINT16)CPU_DX);
	
		// DXレジスタの値を元に戻す
		CPU_DX = oldDX;
#endif
	}
}
void bios0x1a_pcipnp(void) {

	UINT32 oldDX;
	oldDX = CPU_DX;
	
	if(pcidev.enable){
#if !defined(SUPPORT_IA32_HAXM)
		// XXX: np2 BIOSがDXレジスタをPUSH/POPしてしまうので、DXレジスタの内容をスタックから強引に拾ってくる
		CPU_DX = cpu_vmemoryread_w(CPU_SS_INDEX, CPU_SP + 2);
#endif
		
		switch(CPU_AL & 0x7f) {
			case 0x00: // Intel Plug-and-Play AUTO-CONFIGURATION - INSTALLATION CHECK
				CPU_AX = 0x0000;
				CPU_FLAGL &= ~C_FLAG;
				CPU_EDX = 0x47464341; // "GFCA"
				CPU_BH = 0x02; // ACFG major version (02h)
				CPU_BL = 0x08; // ACFG minor version (08h)
				CPU_CX = 0x0002;
				CPU_SI = 0x001F;

				pcidev_updateBIOS32data();

				break;
			case 0x06: // Intel Plug-and-Play AUTO-CONFIGURATION - GET PCI IRQ ROUTING TABLE
				if(CPU_BX == 0x0000){
					UINT16 dataSize = 0;
					UINT32 dataAddress = 0;
					pcidev_updateBIOS32data();
					if(CPU_AL & 0x80){
						// 32bit
						dataSize = MEMR_READ16(CPU_ES, CPU_EDI);
						dataAddress = (UINT32)MEMR_READ16(CPU_ES, CPU_EDI+2)|(((UINT32)MEMR_READ16(CPU_ES, CPU_EDI+4)) << 16);
						if(dataSize < pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY)){
							MEMR_WRITE16(CPU_ES, CPU_EDI, pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY));
							CPU_AH = PCIBIOS_STATUS_BUFFER_TOO_SMALL;
							CPU_FLAGL |= C_FLAG;
						}else{
							dataSize = pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY);
							MEMR_WRITE16(CPU_ES, CPU_EDI, dataSize);
							MEMR_WRITES((dataAddress >> 16) & 0xffff, dataAddress & 0xffff, pcidev.biosdata.data, dataSize);
							CPU_BX = pcidev.allirqbitmap;
							CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
							CPU_FLAGL &= ~C_FLAG;
						}
					}else{
						// 16bit
						dataSize = MEMR_READ16(CPU_ES, CPU_DI);
						dataAddress = (UINT32)MEMR_READ16(CPU_ES, CPU_DI+2)|(((UINT32)MEMR_READ16(CPU_ES, CPU_DI+4)) << 16);
						if(dataSize < pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY)){
							MEMR_WRITE16(CPU_ES, CPU_DI, pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY));
							CPU_AH = PCIBIOS_STATUS_BUFFER_TOO_SMALL;
							CPU_FLAGL |= C_FLAG;
						}else{
							dataSize = pcidev.biosdata.datacount * sizeof(_PCIPNP_IRQTBL_ENTRY);
							MEMR_WRITE16(CPU_ES, CPU_DI, dataSize);
							MEMR_WRITES((dataAddress >> 16) & 0xffff, dataAddress & 0xffff, pcidev.biosdata.data, dataSize);
							CPU_BX = pcidev.allirqbitmap;
							CPU_AH = PCIBIOS_STATUS_SUCCESSFUL;
							CPU_FLAGL &= ~C_FLAG;
						}
					}
				}else{
					CPU_AH = PCIBIOS_STATUS_UNSUPPORTED_FUNCTION;
					CPU_FLAGL |= C_FLAG;
				}
				break;

			default:
				CPU_AH = PCIBIOS_STATUS_UNSUPPORTED_FUNCTION;
				CPU_FLAGL |= C_FLAG;
				break;
		}
		
#if !defined(SUPPORT_IA32_HAXM)
		// XXX: np2 BIOSがDXレジスタをPUSH/POPしてしまうので、DXレジスタの内容をスタックに強引に書き込む
		cpu_vmemorywrite_w(CPU_SS_INDEX, CPU_SP + 2, (UINT16)CPU_DX);
	
		// DXレジスタの値を元に戻す
		CPU_DX = oldDX;
#endif
	}
}

#endif
