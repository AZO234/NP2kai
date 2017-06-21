#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"biosmem.h"


void bios0x09_init(void) {

	iocore_out8(0x43, 0x3a);		// keyboard reset-high
	iocore_out8(0x43, 0x32);		// keyboard reset-low
	iocore_out8(0x43, 0x16);		// error reset
	ZeroMemory(mem + 0x00502, 0x20);
	ZeroMemory(mem + 0x00528, 0x13);
	SETBIOSMEM16(MEMW_KB_SHIFT_TBL, 0x0e00);
	SETBIOSMEM16(MEMW_KB_BUF_HEAD, 0x0502);
	SETBIOSMEM16(MEMW_KB_BUF_TAIL, 0x0502);
	SETBIOSMEM16(MEMW_KB_CODE_OFF, 0x0e00);
	SETBIOSMEM16(MEMW_KB_CODE_SEG, 0xfd80);
}

static void updateshiftkey(void) {

	UINT8	shiftsts;
	UINT	base;

	shiftsts = mem[MEMB_SHIFT_STS];
	mem[MEMB_MSW6] &= 0x3f;							// KEYBOARD LED
	mem[MEMB_MSW6] |= (UINT8)(shiftsts << 5);
	if (shiftsts & 0x10) {
		base = 7;
	}
	else if (shiftsts & 0x08) {
		base = 6;
	}
	else {
		base = shiftsts & 7;
		if (base >= 6) {
			base -= 2;
		}
	}
	base = 0x0e00 + (base * 0x60);
	SETBIOSMEM16(MEMW_KB_SHIFT_TBL, base);
}

void bios0x09(void) {

	UINT8	key;
	UINT	pos;
	UINT8	bit;
	UINT16	code;
	UINT32	base;
	UINT	kbbuftail;

	key = CPU_AL;
	pos = (key & 0x7f) >> 3;
	bit = 1 << (key & 7);
	if (!(key & 0x80)) {
		mem[MEMX_KB_KY_STS + pos] |= bit;
		code = 0xffff;
		base = GETBIOSMEM16(MEMW_KB_SHIFT_TBL);
		base += 0xfd800;
		if (key <= 0x51) {
			if ((key == 0x51) || (key == 0x35) || (key == 0x3e)) {
				code = mem[base + key] << 8;
				if (code == 0xff00) {
					code = 0xffff;
				}
			}
			else {
				code = mem[base + key];
				if (code != 0xff) {
					code += key << 8;
				}
				else {
					code = 0xffff;
				}
			}
		}
		else if (key < 0x60) {
			if (key == 0x5e) {								// home
				code = 0xae00;
			}
		}
		else {
			if (key == 0x60) {
//				CPU_INTERRUPT(6, -1);
			}
			else if (key == 0x61) {
//				CPU_INTERRUPT(5, -1);
			}
			else if (key < 0x70) {
				code = mem[base + key - 0x0c] << 8;
				if (code == 0xff00) {
					code = 0xffff;
				}
			}
			else if (key < 0x75) {
				mem[MEMB_SHIFT_STS] |= bit;
				updateshiftkey();
			}
		}
		if (code != 0xffff) {
			if (mem[MEMB_KB_COUNT] < 0x10) {
				mem[MEMB_KB_COUNT]++;
				kbbuftail = GETBIOSMEM16(MEMW_KB_BUF_TAIL);
				SETBIOSMEM16(kbbuftail, code);
				kbbuftail += 2;
				if (kbbuftail >= 0x522) {
					kbbuftail = 0x502;
				}
				SETBIOSMEM16(MEMW_KB_BUF_TAIL, kbbuftail);
			}
		}
	}
	else {
		mem[MEMX_KB_KY_STS + pos] &= ~bit;
		if ((key >= 0xf0) && (key < 0xf5)) {
			mem[MEMB_SHIFT_STS] &= ~bit;
			updateshiftkey();
		}
	}
}

