#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memepp.h"


// ---- EPP-ROM

void MEMCALL memd000_wr8(UINT32 address, REG8 value) {

	if (CPU_RAM_D000 & (1 << ((address >> 12) & 15))) {
		mem[address] = (UINT8)value;
	}
}

void MEMCALL memd000_wr16(UINT32 address, REG16 value) {

	UINT8	*ptr;
	UINT16	bit;

	ptr = mem + address;
	bit = 1 << ((address >> 12) & 15);
	if ((address + 1) & 0xfff) {
		if (CPU_RAM_D000 & bit) {
			STOREINTELWORD(ptr, value);
		}
	}
	else {
		if (CPU_RAM_D000 & bit) {
			ptr[0] = (UINT8)value;
		}
		if (CPU_RAM_D000 & (bit << 1)) {
			ptr[1] = (UINT8)(value >> 8);
		}
	}
}


// ---- ITF

REG8 MEMCALL memf800_rd8(UINT32 address) {

	if (CPU_ITFBANK) {
		address += VRAM_STEP;
	}
	return(mem[address]);
}

REG16 MEMCALL memf800_rd16(UINT32 address) {

	if (CPU_ITFBANK) {
		address += VRAM_STEP;
	}
	return(LOADINTELWORD(mem + address));
}


// ---- EPSON ROM

void MEMCALL memepson_wr8(UINT32 address, REG8 value) {

	mem[address + 0x1c8000 - 0xe8000] = (UINT8)value;
}

void MEMCALL memepson_wr16(UINT32 address, REG16 value) {

	UINT8	*ptr;

	ptr = mem + (address + 0x1c8000 - 0xe8000);
	STOREINTELWORD(ptr, value);
}

