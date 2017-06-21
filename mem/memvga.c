#include	"compiler.h"

#if defined(SUPPORT_PC9821)

#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memvga.h"
#include	"vram.h"


// ---- macros

#define	VGARD8(p, a) {													\
	UINT32	addr;														\
	addr = (vramop.mio1[(p) * 2] & 15) << 15;							\
	addr += (a);														\
	addr -= (0xa8000 + ((p) * 0x8000));									\
	return(vramex[addr]);												\
}

#define	VGAWR8(p, a, v) {												\
	UINT32	addr;														\
	UINT8	bit;														\
	addr = (vramop.mio1[(p) * 2] & 15) << 15;							\
	addr += (a);														\
	addr -= (0xa8000 + ((p) * 0x8000));									\
	vramex[addr] = (v);													\
	bit = (addr & 0x40000)?2:1;											\
	vramupdate[LOW15(addr >> 3)] |= bit;								\
	gdcs.grphdisp |= bit;												\
}

#define	VGARD16(p, a) {													\
	UINT32	addr;														\
	addr = (vramop.mio1[(p) * 2] & 15) << 15;							\
	addr += (a);														\
	addr -= (0xa8000 + ((p) * 0x8000));									\
	return(LOADINTELWORD(vramex + addr));								\
}

#define	VGAWR16(p, a, v) {												\
	UINT32	addr;														\
	UINT8	bit;														\
	addr = (vramop.mio1[(p) * 2] & 15) << 15;							\
	addr += (a);														\
	addr -= (0xa8000 + ((p) * 0x8000));									\
	STOREINTELWORD(vramex + addr, (v));									\
	bit = (addr & 0x40000)?2:1;											\
	vramupdate[LOW15((addr + 0) >> 3)] |= bit;							\
	vramupdate[LOW15((addr + 1) >> 3)] |= bit;							\
	gdcs.grphdisp |= bit;												\
}


// ---- flat

REG8 MEMCALL memvgaf_rd8(UINT32 address) {

	return(vramex[address & 0x7ffff]);
}

void MEMCALL memvgaf_wr8(UINT32 address, REG8 value) {

	UINT8	bit;

	address = address & 0x7ffff;
	vramex[address] = value;
	bit = (address & 0x40000)?2:1;
	vramupdate[LOW15(address >> 3)] |= bit;
	gdcs.grphdisp |= bit;
}

REG16 MEMCALL memvgaf_rd16(UINT32 address) {

	address = address & 0x7ffff;
	return(LOADINTELWORD(vramex + address));
}

void MEMCALL memvgaf_wr16(UINT32 address, REG16 value) {

	UINT8	bit;

	address = address & 0x7ffff;
	STOREINTELWORD(vramex + address, value);
	bit = (address & 0x40000)?2:1;
	vramupdate[LOW15((address + 0) >> 3)] |= bit;
	vramupdate[LOW15((address + 1) >> 3)] |= bit;
	gdcs.grphdisp |= bit;
}


// ---- 8086 bank memory

REG8 MEMCALL memvga0_rd8(UINT32 address)	VGARD8(0, address)
REG8 MEMCALL memvga1_rd8(UINT32 address)	VGARD8(1, address)
void MEMCALL memvga0_wr8(UINT32 address, REG8 value)
											VGAWR8(0, address, value)
void MEMCALL memvga1_wr8(UINT32 address, REG8 value)
											VGAWR8(1, address, value)
REG16 MEMCALL memvga0_rd16(UINT32 address)	VGARD16(0, address)
REG16 MEMCALL memvga1_rd16(UINT32 address)	VGARD16(1, address)
void MEMCALL memvga0_wr16(UINT32 address, REG16 value)
											VGAWR16(0, address, value)
void MEMCALL memvga1_wr16(UINT32 address, REG16 value)
											VGAWR16(1, address, value)


// ---- 8086 bank I/O

REG8 MEMCALL memvgaio_rd8(UINT32 address) {

	UINT	pos;

	address -= 0xe0000;
	pos = address - 0x0004;
	if (pos < 4) {
		return(vramop.mio1[pos]);
	}
	pos = address - 0x0100;
	if (pos < 0x40) {
		return(vramop.mio2[pos]);
	}
	return(0x00);
}

void MEMCALL memvgaio_wr8(UINT32 address, REG8 value) {

	UINT	pos;

	address -= 0xe0000;
	pos = address - 0x0004;
	if (pos < 4) {
		vramop.mio1[pos] = value;
		return;
	}
	pos = address - 0x0100;
	if (pos < 0x40) {
		vramop.mio2[pos] = value;
		return;
	}
}

REG16 MEMCALL memvgaio_rd16(UINT32 address) {

	REG16	ret;

	ret = memvgaio_rd8(address);
	ret |= memvgaio_rd8(address + 1) << 8;
	return(ret);
}

void MEMCALL memvgaio_wr16(UINT32 address, REG16 value) {

	memvgaio_wr8(address + 0, (REG8)value);
	memvgaio_wr8(address + 1, (REG8)(value >> 8));
}

#endif

