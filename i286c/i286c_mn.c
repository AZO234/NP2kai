#include	"compiler.h"
#include	"cpucore.h"
#include	"i286c.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios/bios.h"
#include	"i286c.mcr"
#if defined(ENABLE_TRAP)
#include "trap/inttrap.h"
#endif


#define	MAX_PREFIX		8


#define	NEXT_OPCODE												\
		if (I286_REMCLOCK < 1) {								\
			I286_BASECLOCK += (1 - I286_REMCLOCK);				\
			I286_REMCLOCK = 1;									\
		}

#define	REMAIN_ADJUST(c)										\
		if (I286_REMCLOCK != (c)) {								\
			I286_BASECLOCK += ((c) - I286_REMCLOCK);			\
			I286_REMCLOCK = (c);								\
		}


// ----

I286FN _reserved(void) {

	INT_NUM(6, I286_IP - 1);
}

I286FN _add_ea_r8(void) {						// 00: add EA, REG8

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			dst = i286_memoryread(madr);
			ADDBYTE(res, dst, src);
			i286_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ADDBYTE(res, dst, src);
	*out = (UINT8)res;
}

I286FN _add_ea_r16(void) {						// 01: add EA, REG16

	UINT16	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = i286_memoryread_w(madr);
			ADDWORD(res, dst, src);
			i286_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ADDWORD(res, dst, src);
	*out = (UINT16)res;
}

I286FN _add_r8_ea(void) {						// 02: add REG8, EA

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ADDBYTE(res, dst, src);
	*out = (UINT8)res;
}

I286FN _add_r16_ea(void) {						// 03: add REG16, EA

	UINT16	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ADDWORD(res, dst, src);
	*out = (UINT16)res;
}

I286FN _add_al_data8(void) {					// 04: add al, DATA8

	UINT	src;
	UINT	res;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src);
	ADDBYTE(res, I286_AL, src);
	I286_AL = (UINT8)res;
}

I286FN _add_ax_data16(void) {					// 05: add ax, DATA16

	UINT	src;
	UINT	res;

	I286_WORKCLOCK(3);
	GET_PCWORD(src);
	ADDWORD(res, I286_AX, src);
	I286_AX = (UINT16)res;
}

I286FN _push_es(void) {							// 06: push es

	REGPUSH(I286_ES, 3);
}

I286FN _pop_es(void) {							// 07: pop es

	UINT	tmp;

	REGPOP(tmp, 5)
	I286_ES = tmp;
	ES_BASE = SEGSELECT(tmp);
}

I286FN _or_ea_r8(void) {						// 08: or EA, REG8

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			dst = i286_memoryread(madr);
			ORBYTE(dst, src);
			i286_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ORBYTE(dst, src);
	*out = (UINT8)dst;
}

I286FN _or_ea_r16(void) {							// 09: or EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = i286_memoryread_w(madr);
			ORWORD(dst, src);
			i286_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ORWORD(dst, src);
	*out = (UINT16)dst;
}

I286FN _or_r8_ea(void) {						// 0a: or REG8, EA

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ORBYTE(dst, src);
	*out = (UINT8)dst;
}

I286FN _or_r16_ea(void) {						// 0b: or REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ORWORD(dst, src);
	*out = (UINT16)dst;
}

I286FN _or_al_data8(void) {						// 0c: or al, DATA8

	UINT	src;
	UINT	dst;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = I286_AL;
	ORBYTE(dst, src);
	I286_AL = (UINT8)dst;
}

I286FN _or_ax_data16(void) {					// 0d: or ax, DATA16

	UINT32	src;
	UINT32	dst;

	I286_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = I286_AX;
	ORWORD(dst, src);
	I286_AX = (UINT16)dst;
}

I286FN _push_cs(void) {							// 0e: push cs

	REGPUSH(I286_CS, 3);
}

I286FN _adc_ea_r8(void) {						// 10: adc EA, REG8

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			dst = i286_memoryread(madr);
			ADCBYTE(res, dst, src);
			i286_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ADCBYTE(res, dst, src);
	*out = (UINT8)res;
}

I286FN _adc_ea_r16(void) {						// 11: adc EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = i286_memoryread_w(madr);
			ADCWORD(res, dst, src);
			i286_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ADCWORD(res, dst, src);
	*out = (UINT16)res;
}

I286FN _adc_r8_ea(void) {						// 12: adc REG8, EA

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ADCBYTE(res, dst, src);
	*out = (UINT8)res;
}

I286FN _adc_r16_ea(void) {						// 13: adc REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ADCWORD(res, dst, src);
	*out = (UINT16)res;
}

I286FN _adc_al_data8(void) {					// 14: adc al, DATA8

	UINT	src;
	UINT	res;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src);
	ADCBYTE(res, I286_AL, src);
	I286_AL = (UINT8)res;
}

I286FN _adc_ax_data16(void) {					// 15: adc ax, DATA16

	UINT32	src;
	UINT32	res;

	I286_WORKCLOCK(3);
	GET_PCWORD(src);
	ADCWORD(res, I286_AX, src);
	I286_AX = (UINT16)res;
}

I286FN _push_ss(void) {							// 16: push ss

	REGPUSH(I286_SS, 3);
}

I286FN _pop_ss(void) {							// 17: pop ss

	UINT	tmp;

	REGPOP(tmp, 5)
	I286_SS = tmp;
	SS_BASE = SEGSELECT(tmp);
	SS_FIX = SS_BASE;
	NEXT_OPCODE
}

I286FN _sbb_ea_r8(void) {						// 18: sbb EA, REG8

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			dst = i286_memoryread(madr);
			SBBBYTE(res, dst, src);
			i286_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	SBBBYTE(res, dst, src);
	*out = (UINT8)res;
}

I286FN _sbb_ea_r16(void) {						// 19: sbb EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = i286_memoryread_w(madr);
			SBBWORD(res, dst, src);
			i286_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	SBBWORD(res, dst, src);
	*out = (UINT16)res;
}

I286FN _sbb_r8_ea(void) {						// 1a: sbb REG8, EA

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT32	res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	SBBBYTE(res, dst, src);
	*out = (UINT8)res;
}

I286FN _sbb_r16_ea(void) {						// 1b: sbb REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	SBBWORD(res, dst, src);
	*out = (UINT16)res;
}

I286FN _sbb_al_data8(void) {					// 1c: adc al, DATA8

	UINT	src;
	UINT	res;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src);
	SBBBYTE(res, I286_AL, src);
	I286_AL = (UINT8)res;
}

I286FN _sbb_ax_data16(void) {					// 1d: adc ax, DATA16

	UINT32	src;
	UINT32	res;

	I286_WORKCLOCK(3);
	GET_PCWORD(src);
	SBBWORD(res, I286_AX, src);
	I286_AX = (UINT16)res;
}

I286FN _push_ds(void) {							// 1e: push ds

	REGPUSH(I286_DS, 3);
}

I286FN _pop_ds(void) {							// 1f: pop ds

	UINT	tmp;

	REGPOP(tmp, 5)
	I286_DS = tmp;
	DS_BASE = SEGSELECT(tmp);
	DS_FIX = DS_BASE;
}

I286FN _and_ea_r8(void) {						// 20: and EA, REG8

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			dst = i286_memoryread(madr);
			ANDBYTE(dst, src);
			i286_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ANDBYTE(dst, src);
	*out = (UINT8)dst;
}

I286FN _and_ea_r16(void) {						// 21: and EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = i286_memoryread_w(madr);
			ANDWORD(dst, src);
			i286_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ANDWORD(dst, src);
	*out = (UINT16)dst;
}

I286FN _and_r8_ea(void) {						// 22: and REG8, EA

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ANDBYTE(dst, src);
	*out = (UINT8)dst;
}

I286FN _and_r16_ea(void) {						// 23: and REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ANDWORD(dst, src);
	*out = (UINT16)dst;
}

I286FN _and_al_data8(void) {					// 24: and al, DATA8

	UINT	src;
	UINT	dst;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = I286_AL;
	ANDBYTE(dst, src);
	I286_AL = (UINT8)dst;
}

I286FN _and_ax_data16(void) {					// 25: and ax, DATA16

	UINT32	src;
	UINT32	dst;

	I286_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = I286_AX;
	ANDWORD(dst, src);
	I286_AX = (UINT16)dst;
}

I286FN _segprefix_es(void) {					// 26: es:

	SS_FIX = ES_BASE;
	DS_FIX = ES_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _daa(void) {								// 27: daa

	I286_WORKCLOCK(3);
	I286_OV = ((I286_AL < 0x80) && 
				((I286_AL >= 0x7a) ||
				((I286_AL >= 0x1a) && (I286_FLAGL & C_FLAG))));
	if ((I286_FLAGL & A_FLAG) || ((I286_AL & 0x0f) > 9)) {
		I286_FLAGL |= A_FLAG;
		I286_FLAGL |= (UINT8)((I286_AL + 6) >> 8);
		I286_AL += 6;
	}
	if ((I286_FLAGL & C_FLAG) || (I286_AL > 0x9f)) {
		I286_FLAGL |= C_FLAG;
		I286_AL += 0x60;
	}
	I286_FLAGL &= A_FLAG | C_FLAG;
	I286_FLAGL |= BYTESZPF(I286_AL);
}

I286FN _sub_ea_r8(void) {						// 28: sub EA, REG8

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			dst = i286_memoryread(madr);
			SUBBYTE(res, dst, src);
			i286_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	SUBBYTE(res, dst, src);
	*out = (UINT8)res;
}

I286FN _sub_ea_r16(void) {						// 29: sub EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = i286_memoryread_w(madr);
			SUBWORD(res, dst, src);
			i286_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	SUBWORD(res, dst, src);
	*out = (UINT16)res;
}

I286FN _sub_r8_ea(void) {						// 2a: sub REG8, EA

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	SUBBYTE(res, dst, src);
	*out = (UINT8)res;
}

I286FN _sub_r16_ea(void) {						// 2b: sub REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	SUBWORD(res, dst, src);
	*out = (UINT16)res;
}

I286FN _sub_al_data8(void) {					// 2c: sub al, DATA8

	UINT	src;
	UINT	res;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src);
	SUBBYTE(res, I286_AL, src);
	I286_AL = (UINT8)res;
}

I286FN _sub_ax_data16(void) {					// 2d: sub ax, DATA16

	UINT32	src;
	UINT32	res;

	I286_WORKCLOCK(3);
	GET_PCWORD(src);
	SUBWORD(res, I286_AX, src);
	I286_AX = (UINT16)res;
}

I286FN _segprefix_cs(void) {					// 2e: cs:

	SS_FIX = CS_BASE;
	DS_FIX = CS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _das(void) {								// 2f: das

	I286_WORKCLOCK(3);
	if ((I286_FLAGL & C_FLAG) || (I286_AL > 0x99)) {
		I286_FLAGL |= C_FLAG;
		I286_AL -= 0x60;
	}
	if ((I286_FLAGL & A_FLAG) || ((I286_AL & 0x0f) > 9)) {
		I286_FLAGL |= A_FLAG;
		I286_FLAGL |= ((I286_AL - 6) >> 8) & 1;
		I286_AL -= 6;
	}
	I286_FLAGL &= A_FLAG | C_FLAG;
	I286_FLAGL |= BYTESZPF(I286_AL);
}

I286FN _xor_ea_r8(void) {						// 30: xor EA, REG8

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			dst = i286_memoryread(madr);
			XORBYTE(dst, src);
			i286_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	XORBYTE(dst, src);
	*out = (UINT8)dst;
}

I286FN _xor_ea_r16(void) {						// 31: xor EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = i286_memoryread_w(madr);
			XORWORD(dst, src);
			i286_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	XORWORD(dst, src);
	*out = (UINT16)dst;
}

I286FN _xor_r8_ea(void) {						// 32: xor REG8, EA

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	XORBYTE(dst, src);
	*out = (UINT8)dst;
}

I286FN _xor_r16_ea(void) {						// 33: or REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	XORWORD(dst, src);
	*out = (UINT16)dst;
}

I286FN _xor_al_data8(void) {					// 34: or al, DATA8

	UINT	src;
	UINT	dst;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = I286_AL;
	XORBYTE(dst, src);
	I286_AL = (UINT8)dst;
}

I286FN _xor_ax_data16(void) {					// 35: or ax, DATA16

	UINT32	src;
	UINT32	dst;

	I286_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = I286_AX;
	XORWORD(dst, src);
	I286_AX = (UINT16)dst;
}

I286FN _segprefix_ss(void) {					// 36: ss:

	SS_FIX = SS_BASE;
	DS_FIX = SS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _aaa(void) {								// 37: aaa

	I286_WORKCLOCK(3);
	if ((I286_FLAGL & A_FLAG) || ((I286_AL & 0xf) > 9)) {
		I286_FLAGL |= A_FLAG | C_FLAG;
		I286_AX += 6;
		I286_AH++;
	}
	else {
		I286_FLAGL &= ~(A_FLAG | C_FLAG);
	}
	I286_AL &= 0x0f;
}

I286FN _cmp_ea_r8(void) {						// 38: cmp EA, REG8

	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		dst = *(REG8_B20(op));
		SUBBYTE(res, dst, src);
	}
	else {
		I286_WORKCLOCK(6);
		dst = i286_memoryread(CALC_EA(op));
		SUBBYTE(res, dst, src);
	}
}

I286FN _cmp_ea_r16(void) {						// 39: cmp EA, REG16

	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		dst = *(REG16_B20(op));
		SUBWORD(res, dst, src);
	}
	else {
		I286_WORKCLOCK(6);
		dst = i286_memoryread_w(CALC_EA(op));
		SUBWORD(res, dst, src);
	}
}

I286FN _cmp_r8_ea(void) {						// 3a: cmp REG8, EA

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG8_EA(op, src, out, 2, 6);
	dst = *out;
	SUBBYTE(res, dst, src);
}

I286FN _cmp_r16_ea(void) {						// 3b: cmp REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_REG16_EA(op, src, out, 2, 6);
	dst = *out;
	SUBWORD(res, dst, src);
}

I286FN _cmp_al_data8(void) {					// 3c: cmp al, DATA8

	UINT	src;
	UINT	res;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src);
	SUBBYTE(res, I286_AL, src);
}

I286FN _cmp_ax_data16(void) {					// 3d: cmp ax, DATA16

	UINT32	src;
	UINT32	res;

	I286_WORKCLOCK(3);
	GET_PCWORD(src);
	SUBWORD(res, I286_AX, src);
}

I286FN _segprefix_ds(void) {					// 3e: ds:

	SS_FIX = DS_BASE;
	DS_FIX = DS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _aas(void) {								// 3f: aas

	I286_WORKCLOCK(3);
	if ((I286_FLAGL & A_FLAG) || ((I286_AL & 0xf) > 9)) {
		I286_FLAGL |= A_FLAG | C_FLAG;
		I286_AX -= 6;
		I286_AH--;
	}
	else {
		I286_FLAGL &= ~(A_FLAG | C_FLAG);
	}
}

I286FN _inc_ax(void) INCWORD2(I286_AX, 2) 	// 40:	inc		ax
I286FN _inc_cx(void) INCWORD2(I286_CX, 2)	// 41:	inc		cx
I286FN _inc_dx(void) INCWORD2(I286_DX, 2)	// 42:	inc		dx
I286FN _inc_bx(void) INCWORD2(I286_BX, 2)	// 43:	inc		bx
I286FN _inc_sp(void) INCWORD2(I286_SP, 2)	// 44:	inc		sp
I286FN _inc_bp(void) INCWORD2(I286_BP, 2)	// 45:	inc		bp
I286FN _inc_si(void) INCWORD2(I286_SI, 2)	// 46:	inc		si
I286FN _inc_di(void) INCWORD2(I286_DI, 2)	// 47:	inc		di
I286FN _dec_ax(void) DECWORD2(I286_AX, 2)	// 48:	dec		ax
I286FN _dec_cx(void) DECWORD2(I286_CX, 2)	// 49:	dec		cx
I286FN _dec_dx(void) DECWORD2(I286_DX, 2)	// 4a:	dec		dx
I286FN _dec_bx(void) DECWORD2(I286_BX, 2)	// 4b:	dec		bx
I286FN _dec_sp(void) DECWORD2(I286_SP, 2)	// 4c:	dec		sp
I286FN _dec_bp(void) DECWORD2(I286_BP, 2)	// 4d:	dec		bp
I286FN _dec_si(void) DECWORD2(I286_SI, 2)	// 4e:	dec		si
I286FN _dec_di(void) DECWORD2(I286_DI, 2)	// 4f:	dec		di

I286FN _push_ax(void) REGPUSH(I286_AX, 3)	// 50:	push	ax
I286FN _push_cx(void) REGPUSH(I286_CX, 3)	// 51:	push	cx
I286FN _push_dx(void) REGPUSH(I286_DX, 3)	// 52:	push	dx
I286FN _push_bx(void) REGPUSH(I286_BX, 3)	// 53:	push	bx
I286FN _push_sp(void) SP_PUSH(I286_SP, 3)	// 54:	push	sp
I286FN _push_bp(void) REGPUSH(I286_BP, 3)	// 55:	push	bp
I286FN _push_si(void) REGPUSH(I286_SI, 3)	// 56:	push	si
I286FN _push_di(void) REGPUSH(I286_DI, 3)	// 57:	push	di
I286FN _pop_ax(void) REGPOP(I286_AX, 5)		// 58:	pop		ax
I286FN _pop_cx(void) REGPOP(I286_CX, 5)		// 59:	pop		cx
I286FN _pop_dx(void) REGPOP(I286_DX, 5)		// 5A:	pop		dx
I286FN _pop_bx(void) REGPOP(I286_BX, 5)		// 5B:	pop		bx
I286FN _pop_sp(void) SP_POP(I286_SP, 5)		// 5C:	pop		sp
I286FN _pop_bp(void) REGPOP(I286_BP, 5)		// 5D:	pop		bp
I286FN _pop_si(void) REGPOP(I286_SI, 5)		// 5E:	pop		si
I286FN _pop_di(void) REGPOP(I286_DI, 5)		// 5F:	pop		di

#if (defined(ARM) || defined(X11)) && defined(BYTESEX_LITTLE)

I286FN _pusha(void) {						// 60:	pusha

	REG16	tmp;
	UINT32	addr;

	I286_WORKCLOCK(17);
	tmp = I286_SP;
	addr = tmp + SS_BASE;
	if ((tmp < 16) || (INHIBIT_WORDP(addr))) {
		REGPUSH0(I286_AX)
		REGPUSH0(I286_CX)
		REGPUSH0(I286_DX)
		REGPUSH0(I286_BX)
	    REGPUSH0(tmp)
		REGPUSH0(I286_BP)
		REGPUSH0(I286_SI)
		REGPUSH0(I286_DI)
	}
	else {
		*(UINT16 *)(mem + addr - 2) = I286_AX;
		*(UINT16 *)(mem + addr - 4) = I286_CX;
		*(UINT16 *)(mem + addr - 6) = I286_DX;
		*(UINT16 *)(mem + addr - 8) = I286_BX;
		*(UINT16 *)(mem + addr - 10) = tmp;
		*(UINT16 *)(mem + addr - 12) = I286_BP;
		*(UINT16 *)(mem + addr - 14) = I286_SI;
		*(UINT16 *)(mem + addr - 16) = I286_DI;
		I286_SP -= 16;
	}
}

I286FN _popa(void) {						// 61:	popa

	UINT	tmp;
	UINT32	addr;

	I286_WORKCLOCK(19);
	tmp = I286_SP + 16;
	addr = tmp + SS_BASE;
	if ((tmp >= 0x10000) || (INHIBIT_WORDP(addr))) {
		REGPOP0(I286_DI);
		REGPOP0(I286_SI);
		REGPOP0(I286_BP);
		I286_SP += 2;
		REGPOP0(I286_BX);
		REGPOP0(I286_DX);
		REGPOP0(I286_CX);
		REGPOP0(I286_AX);
	}
	else {
		I286_DI = *(UINT16 *)(mem + addr - 16);
		I286_SI = *(UINT16 *)(mem + addr - 14);
		I286_BP = *(UINT16 *)(mem + addr - 12);
		I286_BX = *(UINT16 *)(mem + addr - 8);
		I286_DX = *(UINT16 *)(mem + addr - 6);
		I286_CX = *(UINT16 *)(mem + addr - 4);
		I286_AX = *(UINT16 *)(mem + addr - 2);
		I286_SP = tmp;
	}
}

#else

I286FN _pusha(void) {						// 60:	pusha

	REG16	tmp;

	tmp = I286_SP;
	REGPUSH0(I286_AX)
	REGPUSH0(I286_CX)
	REGPUSH0(I286_DX)
	REGPUSH0(I286_BX)
    REGPUSH0(tmp)
	REGPUSH0(I286_BP)
	REGPUSH0(I286_SI)
	REGPUSH0(I286_DI)
	I286_WORKCLOCK(17);
}

I286FN _popa(void) {						// 61:	popa

	REGPOP0(I286_DI);
	REGPOP0(I286_SI);
	REGPOP0(I286_BP);
	I286_SP += 2;
	REGPOP0(I286_BX);
	REGPOP0(I286_DX);
	REGPOP0(I286_CX);
	REGPOP0(I286_AX);
	I286_WORKCLOCK(19);
}

#endif

I286FN _bound(void) {						// 62:	bound

	UINT	vect = 0;
	UINT	op;
	UINT32	madr;
	REG16	reg;

	I286_WORKCLOCK(13);										// ToDo
	GET_PCBYTE(op);
	if (op < 0xc0) {
		reg = *(REG16_B53(op));
		madr = CALC_EA(op);
		if (reg >= i286_memoryread_w(madr)) {
			madr += 2;										// ToDo
			if (reg <= i286_memoryread_w(madr)) {
				return;
			}
		}
		vect = 5;
	}
	else {
		vect = 6;
	}
	INT_NUM(vect, I286_IP);
}

I286FN _arpl(void) {						// 63:	arpl

	UINT	op;
	UINT	tmp;

	GET_PCBYTE(op)
	tmp = ((op < 0xc0)?1:0);
	I286_IP += (UINT8)tmp;
	I286_WORKCLOCK(tmp + 10);
	INT_NUM(6, I286_IP);
}

I286FN _push_data16(void) {				// 68:	push	DATA16

	UINT16	tmp;

	GET_PCWORD(tmp)
	REGPUSH(tmp, 3)
}

I286FN _imul_reg_ea_data16(void) {		// 69:	imul	REG, EA, DATA16

	UINT16	*out;
	UINT	op;
	SINT16	src;
	SINT16	dst;
	SINT32	res;

	PREPART_REG16_EA(op, src, out, 21, 24)
	GET_PCWORD(dst)
	WORD_IMUL(res, dst, src)
	*out = (UINT16)res;
}

I286FN _push_data8(void) {				// 6A:	push	DATA8

	UINT16	tmp;

	GET_PCBYTES(tmp)
	REGPUSH(tmp, 3)
}

I286FN _imul_reg_ea_data8(void) {		// 6B:	imul	REG, EA, DATA8

	UINT16	*out;
	UINT	op;
	SINT16	src;
	SINT16	dst;
	SINT32	res;

	PREPART_REG16_EA(op, src, out, 21, 24)
	GET_PCBYTES(dst)
	WORD_IMUL(res, dst, src)
	*out = (UINT16)res;
}

I286FN _insb(void) {						// 6C:	insb

	REG8	dat;

	I286_WORKCLOCK(5);
	dat = iocore_inp8(I286_DX);
	i286_memorywrite(I286_DI + ES_BASE, dat);
	I286_DI += STRING_DIR;
}

I286FN _insw(void) {						// 6D:	insw

	REG16	dat;

	I286_WORKCLOCK(5);
	dat = iocore_inp16(I286_DX);
	i286_memorywrite_w(I286_DI + ES_BASE, dat);
	I286_DI += STRING_DIRx2;
}

I286FN _outsb(void) {						// 6E:	outsb

	REG8	dat;

	I286_WORKCLOCK(3);
	dat = i286_memoryread(I286_SI + DS_FIX);
	I286_SI += STRING_DIR;
	iocore_out8(I286_DX, (UINT8)dat);
}

I286FN _outsw(void) {						// 6F:	outsw

	REG16	dat;

	I286_WORKCLOCK(3);
	dat = i286_memoryread_w(I286_SI + DS_FIX);
	I286_SI += STRING_DIRx2;
	iocore_out16(I286_DX, (UINT16)dat);
}

I286FN _jo_short(void) {					// 70:	jo short

	if (!I286_OV) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jno_short(void) {					// 71:	jno short

	if (I286_OV) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jc_short(void) {					// 72:	jnae/jb/jc short

	if (!(I286_FLAGL & C_FLAG)) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jnc_short(void) {					// 73:	jae/jnb/jnc short

	if (I286_FLAGL & C_FLAG) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jz_short(void) {					// 74:	je/jz short

	if (!(I286_FLAGL & Z_FLAG)) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jnz_short(void) {					// 75:	jne/jnz short

	if (I286_FLAGL & Z_FLAG) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jna_short(void) {					// 76:	jna/jbe short

	if (!(I286_FLAGL & (Z_FLAG | C_FLAG))) JMPNOP(3) else JMPSHORT(7)
}

I286FN _ja_short(void) {					// 77:	ja/jnbe short
	if (I286_FLAGL & (Z_FLAG | C_FLAG)) JMPNOP(3) else JMPSHORT(7)
}

I286FN _js_short(void) {					// 78:	js short

	if (!(I286_FLAGL & S_FLAG)) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jns_short(void) {					// 79:	jns short

	if (I286_FLAGL & S_FLAG) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jp_short(void) {					// 7A:	jp/jpe short

	if (!(I286_FLAGL & P_FLAG)) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jnp_short(void) {					// 7B:	jnp/jpo short

	if (I286_FLAGL & P_FLAG) JMPNOP(3) else JMPSHORT(7)
}

I286FN _jl_short(void) {					// 7C:	jl/jnge short

	if (((I286_FLAGL & S_FLAG) == 0) == (I286_OV == 0))
												JMPNOP(3) else JMPSHORT(7)
}

I286FN _jnl_short(void) {					// 7D:	jnl/jge short

	if (((I286_FLAGL & S_FLAG) == 0) != (I286_OV == 0))
												JMPNOP(3) else JMPSHORT(7)
}

I286FN _jle_short(void) {					// 7E:	jle/jng short

	if ((!(I286_FLAGL & Z_FLAG)) &&
		(((I286_FLAGL & S_FLAG) == 0) == (I286_OV == 0)))
												JMPNOP(3) else JMPSHORT(7)
}

I286FN _jnle_short(void) {					// 7F:	jg/jnle short

	if ((I286_FLAGL & Z_FLAG) ||
		(((I286_FLAGL & S_FLAG) == 0) != (I286_OV == 0)))
												JMPNOP(3) else JMPSHORT(7)
}

I286FN _calc_ea8_i8(void) {					// 80:	op		EA8, DATA8
											// 82:	op		EA8, DATA8
	UINT8	*out;
	UINT	op;
	UINT32	madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(3);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			c_op8xext8_table[(op >> 3) & 7](madr);
			return;
		}
		out = mem + madr;
	}
	c_op8xreg8_table[(op >> 3) & 7](out);
}

I286FN _calc_ea16_i16(void) {				// 81:	op		EA16, DATA16

	UINT16	*out;
	UINT	op;
	UINT32	madr;
	UINT32	src;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(3);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			GET_PCWORD(src);
			c_op8xext16_table[(op >> 3) & 7](madr, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCWORD(src);
	c_op8xreg16_table[(op >> 3) & 7](out, src);
}

I286FN _calc_ea16_i8(void) {				// 83:	op		EA16, DATA8

	UINT16	*out;
	UINT	op;
	UINT32	madr;
	UINT32	src;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(3);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			GET_PCBYTES(src);
			c_op8xext16_table[(op >> 3) & 7](madr, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCBYTES(src);
	c_op8xreg16_table[(op >> 3) & 7](out, src);
}

I286FN _test_ea_r8(void) {					// 84:	test	EA, REG8

	UINT8	*out;
	UINT	op;
	UINT	src;
	UINT	tmp;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(6);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			tmp = i286_memoryread(madr);
			ANDBYTE(tmp, src);
			return;
		}
		out = mem + madr;
	}
	tmp = *out;
	ANDBYTE(tmp, src);
}

I286FN _test_ea_r16(void) {					// 85:	test	EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	tmp;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(6);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			tmp = i286_memoryread_w(madr);
			ANDWORD(tmp, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	tmp = *out;
	ANDWORD(tmp, src);
}

I286FN _xchg_ea_r8(void) {					// 86:	xchg	EA, REG8

	UINT8	*out;
	UINT8	*src;
	UINT	op;
	UINT32	madr;

	PREPART_EA_REG8P(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(3);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(5);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			UINT8 tmp = i286_memoryread(madr);
			i286_memorywrite(madr, *src);
			*src = tmp;
			return;
		}
		out = mem + madr;
	}
	SWAPBYTE(*out, *src);
}

I286FN _xchg_ea_r16(void) {					// 87:	xchg	EA, REG16

	UINT16	*out;
	UINT16	*src;
	UINT	op;
	UINT32	madr;

	PREPART_EA_REG16P(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(3);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(5);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			UINT16 tmp = i286_memoryread_w(madr);
			i286_memorywrite_w(madr, *src);
			*src = tmp;
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	SWAPWORD(*out, *src);
}

I286FN _mov_ea_r8(void) {					// 88:	mov		EA, REG8

	UINT8	src;
	UINT	op;
	UINT32	madr;

	PREPART_EA_REG8(op, src)
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		*(REG8_B20(op)) = src;
	}
	else {
		I286_WORKCLOCK(3);
		madr = CALC_EA(op);
		i286_memorywrite(madr, src);
	}
}

I286FN _mov_ea_r16(void) {					// 89:	mov		EA, REG16

	UINT16	src;
	UINT	op;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		*(REG16_B20(op)) = src;
	}
	else {
		I286_WORKCLOCK(3);
		i286_memorywrite_w(CALC_EA(op), src);
	}
}

I286FN _mov_r8_ea(void) {					// 8A:	mov		REG8, EA

	UINT8	*out;
	UINT8	src;
	UINT	op;

	PREPART_REG8_EA(op, src, out, 2, 5);
	*out = src;
}

I286FN _mov_r16_ea(void) {					// 8B:	mov		REG16, EA

	UINT16	*out;
	UINT16	src;
	UINT	op;

	PREPART_REG16_EA(op, src, out, 2, 5);
	*out = src;
}

I286FN _mov_ea_seg(void) {					// 8C:	mov		EA, segreg

	UINT	op;
	UINT16	tmp;

	GET_PCBYTE(op);
	tmp = *SEGMENTPTR((op >> 3) & 3);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		*(REG16_B20(op)) = tmp;
	}
	else {
		I286_WORKCLOCK(3);
		i286_memorywrite_w(CALC_EA(op), tmp);
	}
}

I286FN _lea_r16_ea(void) {					// 8D:	lea		REG16, EA

	UINT	op;

	I286_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		*(REG16_B53(op)) = CALC_LEA(op);
	}
	else {
		INT_NUM(6, I286_SP - 2);
	}
}

I286FN _mov_seg_ea(void) {					// 8E:	mov		segrem, EA

	UINT	op;
	UINT	tmp;
	UINT32	base;
	UINT16	ipbak;

	ipbak = I286_IP;
	GET_PCBYTE(op);
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		tmp = *(REG16_B20(op));
	}
	else {
		I286_WORKCLOCK(5);
		tmp = i286_memoryread_w(CALC_EA(op));
	}
	base = SEGSELECT(tmp);
	switch(op & 0x18) {
		case 0x00:			// es
			I286_ES = (UINT16)tmp;
			ES_BASE = base;
			break;

		case 0x10:			// ss
			I286_SS = (UINT16)tmp;
			SS_BASE = base;
			SS_FIX = base;
			NEXT_OPCODE
			break;

		case 0x18:			// ds
			I286_DS = (UINT16)tmp;
			DS_BASE = base;
			DS_FIX = base;
			break;

		default:			// cs
			INT_NUM(6, ipbak - 1);
			break;
	}
}

I286FN _pop_ea(void) {						// 8F:	pop		EA

	UINT	op;
	UINT16	tmp;

	I286_WORKCLOCK(5);
	REGPOP0(tmp)

	GET_PCBYTE(op)
	if (op < 0xc0) {
		i286_memorywrite_w(CALC_EA(op), tmp);
	}
	else {
		*(REG16_B20(op)) = tmp;
	}
}

I286FN _nop(void) {							// 90: nop / bios func

#if 1										// call BIOS
	UINT32	adrs;

	adrs = LOW16(I286_IP - 1) + CS_BASE;
	if ((adrs >= 0xf8000) && (adrs < 0x100000)) {
		biosfunc(adrs);
		ES_BASE = I286_ES << 4;
		CS_BASE = I286_CS << 4;
		SS_BASE = I286_SS << 4;
		SS_FIX = SS_BASE;
		DS_BASE = I286_DS << 4;
		DS_FIX = DS_BASE;
	}
#endif
	I286_WORKCLOCK(3);
}

I286FN _xchg_ax_cx(void) { 					// 91:	xchg	ax, cx

	I286_WORKCLOCK(3);
	SWAPWORD(I286_AX, I286_CX);
}

I286FN _xchg_ax_dx(void) { 					// 92:	xchg	ax, dx

	I286_WORKCLOCK(3);
	SWAPWORD(I286_AX, I286_DX);
}

I286FN _xchg_ax_bx(void) { 					// 93:	xchg	ax, bx

	I286_WORKCLOCK(3);
	SWAPWORD(I286_AX, I286_BX);
}

I286FN _xchg_ax_sp(void) { 					// 94:	xchg	ax, sp

	I286_WORKCLOCK(3);
	SWAPWORD(I286_AX, I286_SP);
}

I286FN _xchg_ax_bp(void) { 					// 95:	xchg	ax, bp

	I286_WORKCLOCK(3);
	SWAPWORD(I286_AX, I286_BP);
}

I286FN _xchg_ax_si(void) { 					// 96:	xchg	ax, si

	I286_WORKCLOCK(3);
	SWAPWORD(I286_AX, I286_SI);
}

I286FN _xchg_ax_di(void) { 					// 97:	xchg	ax, di

	I286_WORKCLOCK(3);
	SWAPWORD(I286_AX, I286_DI);
}

I286FN _cbw(void) {							// 98:	cbw

	I286_WORKCLOCK(2);
	I286_AX = __CBW(I286_AL);
}

I286FN _cwd(void) {							// 99:	cwd

	I286_WORKCLOCK(2);
	I286_DX = ((I286_AH & 0x80)?0xffff:0x0000);
}

I286FN _call_far(void) {					// 9A:	call far

	UINT16	newip;

	I286_WORKCLOCK(13);
	REGPUSH0(I286_CS)
	GET_PCWORD(newip)
	GET_PCWORD(I286_CS)
	CS_BASE = SEGSELECT(I286_CS);
	REGPUSH0(I286_IP)
	I286_IP = newip;
}

I286FN _wait(void) {						// 9B:	wait

	I286_WORKCLOCK(2);
}

I286FN _pushf(void) {						// 9C:	pushf

	REGPUSH(REAL_FLAGREG, 3)
}

I286FN _popf(void) {						// 9D:	popf

	UINT	flag;

	REGPOP0(flag)
	I286_OV = flag & O_FLAG;
	I286_FLAG = flag & (0xfff ^ O_FLAG);
	I286_TRAP = ((flag & 0x300) == 0x300);
	I286_WORKCLOCK(5);
#if defined(INTR_FAST)
	if ((I286_TRAP) || ((flag & I_FLAG) && (PICEXISTINTR))) {
		I286IRQCHECKTERM
	}
#else
	I286IRQCHECKTERM
#endif
}

I286FN _sahf(void) {						// 9E:	sahf

	I286_WORKCLOCK(2);
	I286_FLAGL = I286_AH;
}

I286FN _lahf(void) {						// 9F:	lahf

	I286_WORKCLOCK(2);
	I286_AH = I286_FLAGL;
}

I286FN _mov_al_m8(void) {					// A0:	mov		al, m8

	UINT	op;

	I286_WORKCLOCK(5);
	GET_PCWORD(op)
	I286_AL = i286_memoryread(DS_FIX + op);
}

I286FN _mov_ax_m16(void) {					// A1:	mov		ax, m16

	UINT	op;

	I286_WORKCLOCK(5);
	GET_PCWORD(op)
	I286_AX = i286_memoryread_w(DS_FIX + op);
}

I286FN _mov_m8_al(void) {					// A2:	mov		m8, al

	UINT	op;

	I286_WORKCLOCK(3);
	GET_PCWORD(op)
	i286_memorywrite(DS_FIX + op, I286_AL);
}

I286FN _mov_m16_ax(void) {					// A3:	mov		m16, ax

	UINT	op;

	I286_WORKCLOCK(3);
	GET_PCWORD(op);
	i286_memorywrite_w(DS_FIX + op, I286_AX);
}

I286FN _movsb(void) {						// A4:	movsb

	UINT8	tmp;

	I286_WORKCLOCK(5);
	tmp = i286_memoryread(I286_SI + DS_FIX);
	i286_memorywrite(I286_DI + ES_BASE, tmp);
	I286_SI += STRING_DIR;
	I286_DI += STRING_DIR;
}

I286FN _movsw(void) {						// A5:	movsw

	UINT16	tmp;

	I286_WORKCLOCK(5);
	tmp = i286_memoryread_w(I286_SI + DS_FIX);
	i286_memorywrite_w(I286_DI + ES_BASE, tmp);
	I286_SI += STRING_DIRx2;
	I286_DI += STRING_DIRx2;
}

I286FN _cmpsb(void) {						// A6:	cmpsb

	UINT	src;
	UINT	dst;
	UINT	res;

	I286_WORKCLOCK(8);
	dst = i286_memoryread(I286_SI + DS_FIX);
	src = i286_memoryread(I286_DI + ES_BASE);
	SUBBYTE(res, dst, src)
	I286_SI += STRING_DIR;
	I286_DI += STRING_DIR;
}

I286FN _cmpsw(void) {						// A7:	cmpsw

	UINT32	src;
	UINT32	dst;
	UINT32	res;

	I286_WORKCLOCK(8);
	dst = i286_memoryread_w(I286_SI + DS_FIX);
	src = i286_memoryread_w(I286_DI + ES_BASE);
	SUBWORD(res, dst, src)
	I286_SI += STRING_DIRx2;
	I286_DI += STRING_DIRx2;
}

I286FN _test_al_data8(void) {				// A8:	test	al, DATA8

	UINT	src;
	UINT	dst;

	I286_WORKCLOCK(3);
	GET_PCBYTE(src)
	dst = I286_AL;
	ANDBYTE(dst, src)
}

I286FN _test_ax_data16(void) {				// A9:	test	ax, DATA16

	UINT32	src;
	UINT32	dst;

	I286_WORKCLOCK(3);
	GET_PCWORD(src)
	dst = I286_AX;
	ANDWORD(dst, src)
}

I286FN _stosb(void) {						// AA:	stosw

	I286_WORKCLOCK(3);
	i286_memorywrite(I286_DI + ES_BASE, I286_AL);
	I286_DI += STRING_DIR;
}

I286FN _stosw(void) {						// AB:	stosw

	I286_WORKCLOCK(3);
	i286_memorywrite_w(I286_DI + ES_BASE, I286_AX);
	I286_DI += STRING_DIRx2;
}

I286FN _lodsb(void) {						// AC:	lodsb

	I286_WORKCLOCK(5);
	I286_AL = i286_memoryread(I286_SI + DS_FIX);
	I286_SI += STRING_DIR;
}

I286FN _lodsw(void) {						// AD:	lodsw

	I286_WORKCLOCK(5);
	I286_AX = i286_memoryread_w(I286_SI + DS_FIX);
	I286_SI += STRING_DIRx2;
}

I286FN _scasb(void) {						// AE:	scasb

	UINT	src;
	UINT	dst;
	UINT	res;

	I286_WORKCLOCK(7);
	src = i286_memoryread(I286_DI + ES_BASE);
	dst = I286_AL;
	SUBBYTE(res, dst, src)
	I286_DI += STRING_DIR;
}

I286FN _scasw(void) {						// AF:	scasw

	UINT32	src;
	UINT32	dst;
	UINT32	res;

	I286_WORKCLOCK(7);
	src = i286_memoryread_w(I286_DI + ES_BASE);
	dst = I286_AX;
	SUBWORD(res, dst, src)
	I286_DI += STRING_DIRx2;
}

I286FN _mov_al_imm(void) MOVIMM8(I286_AL)	// B0:	mov		al, imm8
I286FN _mov_cl_imm(void) MOVIMM8(I286_CL)	// B1:	mov		cl, imm8
I286FN _mov_dl_imm(void) MOVIMM8(I286_DL)	// B2:	mov		dl, imm8
I286FN _mov_bl_imm(void) MOVIMM8(I286_BL)	// B3:	mov		bl, imm8
I286FN _mov_ah_imm(void) MOVIMM8(I286_AH)	// B4:	mov		ah, imm8
I286FN _mov_ch_imm(void) MOVIMM8(I286_CH)	// B5:	mov		ch, imm8
I286FN _mov_dh_imm(void) MOVIMM8(I286_DH)	// B6:	mov		dh, imm8
I286FN _mov_bh_imm(void) MOVIMM8(I286_BH)	// B7:	mov		bh, imm8
I286FN _mov_ax_imm(void) MOVIMM16(I286_AX)	// B8:	mov		ax, imm16
I286FN _mov_cx_imm(void) MOVIMM16(I286_CX)	// B9:	mov		cx, imm16
I286FN _mov_dx_imm(void) MOVIMM16(I286_DX)	// BA:	mov		dx, imm16
I286FN _mov_bx_imm(void) MOVIMM16(I286_BX)	// BB:	mov		bx, imm16
I286FN _mov_sp_imm(void) MOVIMM16(I286_SP)	// BC:	mov		sp, imm16
I286FN _mov_bp_imm(void) MOVIMM16(I286_BP)	// BD:	mov		bp, imm16
I286FN _mov_si_imm(void) MOVIMM16(I286_SI)	// BE:	mov		si, imm16
I286FN _mov_di_imm(void) MOVIMM16(I286_DI)	// BF:	mov		di, imm16

I286FN _shift_ea8_data8(void) {				// C0:	shift	EA8, DATA8

	UINT8	*out;
	UINT	op;
	UINT32	madr;
	UINT8	cl;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(5);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			GET_PCBYTE(cl)
			I286_WORKCLOCK(cl);
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	GET_PCBYTE(cl)
	I286_WORKCLOCK(cl);
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

I286FN _shift_ea16_data8(void) {			// C1:	shift	EA16, DATA8

	UINT16	*out;
	UINT	op;
	UINT32	madr;
	UINT8	cl;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(5);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			GET_PCBYTE(cl);
			I286_WORKCLOCK(cl);
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCBYTE(cl);
	I286_WORKCLOCK(cl);
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

I286FN _ret_near_data16(void) {				// C2:	ret near DATA16

	UINT16	ad;

	I286_WORKCLOCK(11);
	GET_PCWORD(ad)
	REGPOP0(I286_IP)
	I286_SP += ad;
}

I286FN _ret_near(void) {					// C3:	ret near

	I286_WORKCLOCK(11);
	REGPOP0(I286_IP)
}

I286FN _les_r16_ea(void) {					// C4:	les		REG16, EA

	UINT	op;
	UINT32	seg;
	UINT	ad;

	I286_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		ad = GET_EA(op, &seg);
		*(REG16_B53(op)) = i286_memoryread_w(seg + ad);
		I286_ES = i286_memoryread_w(seg + LOW16(ad + 2));
		ES_BASE = SEGSELECT(I286_ES);
	}
	else {
		INT_NUM(6, I286_IP - 2);
	}
}

I286FN _lds_r16_ea(void) {					// C5:	lds		REG16, EA

	UINT	op;
	UINT32	seg;
	UINT	ad;

	I286_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		ad = GET_EA(op, &seg);
		*(REG16_B53(op)) = i286_memoryread_w(seg + ad);
		I286_DS = i286_memoryread_w(seg + LOW16(ad + 2));
		DS_BASE = SEGSELECT(I286_DS);
		DS_FIX = DS_BASE;
	}
	else {
		INT_NUM(6, I286_IP - 2);
	}
}

I286FN _mov_ea8_data8(void) {				// C6:	mov		EA8, DATA8

	UINT	op;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		GET_PCBYTE(*(REG8_B53(op)))
	}
	else {				// 03/11/23
		UINT32 ad;
		UINT8 val;
		I286_WORKCLOCK(3);
		ad = CALC_EA(op);
		GET_PCBYTE(val)
		i286_memorywrite(ad, val);
	}
}

I286FN _mov_ea16_data16(void) {				// C7:	mov		EA16, DATA16

	UINT	op;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		GET_PCWORD(*(REG16_B53(op)))
	}
	else {				// 03/11/23
		UINT32	ad;
		UINT16	val;
		I286_WORKCLOCK(3);
		ad = CALC_EA(op);
		GET_PCWORD(val)
		i286_memorywrite_w(ad, val);
	}
}

I286FN _enter(void) {						// C8:	enter	DATA16, DATA8

	UINT16	dimsize;
	UINT8	level;

	GET_PCWORD(dimsize)
	GET_PCBYTE(level)
	REGPUSH0(I286_BP)
	level &= 0x1f;
	if (!level) {								// enter level=0
		I286_WORKCLOCK(11);
		I286_BP = I286_SP;
		I286_SP -= dimsize;
	}
	else {
		level--;
		if (!level) {							// enter level=1
			UINT16 tmp;
			I286_WORKCLOCK(15);
			tmp = I286_SP;
			REGPUSH0(tmp)
			I286_BP = tmp;
			I286_SP -= dimsize;
		}
		else {									// enter level=2-31
			UINT16 bp;
			I286_WORKCLOCK(12 + 4 + level*4);
			bp = I286_BP;
			I286_BP = I286_SP;
			while(level--) {
#if 1											// ‚È‚É‚â‚Á‚Ä‚ñ‚¾ƒ’ƒŒ
				REG16 val;
				bp -= 2;
				I286_SP -= 2;
				val = i286_memoryread_w(bp + SS_BASE);
				i286_memorywrite_w(I286_SP + SS_BASE, val);
#else
				UINT16 val = i286_memoryread_w(bp + SS_BASE);
				i286_memorywrite_w(I286_SP + SS_BASE, val);
				bp -= 2;
				I286_SP -= 2;
#endif
			}
			REGPUSH0(I286_BP)
			I286_SP -= dimsize;
		}
	}
}

I286FN fleave(void) {						// C9:	leave

	I286_WORKCLOCK(5);
	I286_SP = I286_BP;
	REGPOP0(I286_BP)
}

I286FN _ret_far_data16(void) {				// CA:	ret far	DATA16

	UINT16	ad;

	I286_WORKCLOCK(15);
	GET_PCWORD(ad)
	REGPOP0(I286_IP)
	REGPOP0(I286_CS)
	I286_SP += ad;
	CS_BASE = SEGSELECT(I286_CS);
}

I286FN _ret_far(void) {						// CB:	ret far

	I286_WORKCLOCK(15);
	REGPOP0(I286_IP)
	REGPOP0(I286_CS)
	CS_BASE = SEGSELECT(I286_CS);
}

I286FN _int_03(void) {						// CC:	int		3

	I286_WORKCLOCK(3);
	INT_NUM(3, I286_IP);
}

I286FN _int_data8(void) {					// CD:	int		DATA8

	UINT	vect;

	I286_WORKCLOCK(3);
	GET_PCBYTE(vect)
#if defined(ENABLE_TRAP)
	softinttrap(CPU_CS, CPU_IP - 2, vect);
#endif
	INT_NUM(vect, I286_IP);
}

I286FN _into(void) {						// CE:	into

	I286_WORKCLOCK(4);
	if (I286_OV) {
		INT_NUM(4, I286_IP);
	}
}

I286FN _iret(void) {						// CF:	iret

	UINT	flag;

	REGPOP0(I286_IP)
	REGPOP0(I286_CS)
	REGPOP0(flag)
	I286_OV = flag & O_FLAG;
	I286_FLAG = flag & (0xfff ^ O_FLAG);
	I286_TRAP = ((flag & 0x300) == 0x300);
	CS_BASE = I286_CS << 4;
//	CS_BASE = SEGSELECT(I286_CS);
	I286_WORKCLOCK(31);
#if defined(INTR_FAST)
	if ((I286_TRAP) || ((flag & I_FLAG) && (PICEXISTINTR))) {
		I286IRQCHECKTERM
	}
#else
	I286IRQCHECKTERM
#endif
}

I286FN _shift_ea8_1(void) {				// D0:	shift EA8, 1

	UINT8	*out;
	UINT	op;
	UINT32	madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			sft_e8_table[(op >> 3) & 7](madr);
			return;
		}
		out = mem + madr;
	}
	sft_r8_table[(op >> 3) & 7](out);
}

I286FN _shift_ea16_1(void) {			// D1:	shift EA16, 1

	UINT16	*out;
	UINT	op;
	UINT32	madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			sft_e16_table[(op >> 3) & 7](madr);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	sft_r16_table[(op >> 3) & 7](out);
}

I286FN _shift_ea8_cl(void) {			// D2:	shift EA8, cl

	UINT8	*out;
	UINT	op;
	UINT32	madr;
	REG8	cl;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(5);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			cl = I286_CL;
			I286_WORKCLOCK(cl);
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	cl = I286_CL;
	I286_WORKCLOCK(cl);
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

I286FN _shift_ea16_cl(void) {			// D3:	shift EA16, cl

	UINT16	*out;
	UINT	op;
	UINT32	madr;
	REG8	cl;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		I286_WORKCLOCK(5);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			cl = I286_CL;
			I286_WORKCLOCK(cl);
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	cl = I286_CL;
	I286_WORKCLOCK(cl);
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

I286FN _aam(void) {							// D4:	AAM

	UINT8	al;
	UINT8	div;

	I286_WORKCLOCK(16);
	GET_PCBYTE(div);
	if (div) {
		al = I286_AL;
		I286_AH = al / div;
		I286_AL = al % div;
		I286_FLAGL &= ~(S_FLAG | Z_FLAG | P_FLAG);
		I286_FLAGL |= WORDSZPF(I286_AX);
	}
	else {
		INT_NUM(0, I286_IP - 2);				// 80286
//		INT_NUM(0, I286_IP);					// V30
	}
}

I286FN _aad(void) {							// D5:	AAD

	UINT8	mul;

	I286_WORKCLOCK(14);
	GET_PCBYTE(mul);
	I286_AL += (UINT8)(I286_AH * mul);
	I286_AH = 0;
	I286_FLAGL &= ~(S_FLAG | Z_FLAG | P_FLAG);
	I286_FLAGL |= BYTESZPF(I286_AL);
}

I286FN _setalc(void) {						// D6:	setalc (80286)

	I286_AL = ((I286_FLAGL & C_FLAG)?0xff:0);
}

I286FN _xlat(void) {						// D7:	xlat

	I286_WORKCLOCK(5);
	I286_AL = i286_memoryread(LOW16(I286_AL + I286_BX) + DS_FIX);
}

I286FN _esc(void) {							// D8:	esc

	UINT	op;

	I286_WORKCLOCK(2);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		CALC_LEA(op);
	}
}

I286FN _loopnz(void) {						// E0:	loopnz

	I286_CX--;
	if ((!I286_CX) || (I286_FLAGL & Z_FLAG)) JMPNOP(4) else JMPSHORT(8)
}

I286FN _loopz(void) {						// E1:	loopz

	I286_CX--;
	if ((!I286_CX) || (!(I286_FLAGL & Z_FLAG))) JMPNOP(4) else JMPSHORT(8)
}

I286FN _loop(void) {						// E2:	loop

	I286_CX--;
	if (!I286_CX) JMPNOP(4) else JMPSHORT(8)
}

I286FN _jcxz(void) {						// E3:	jcxz

	if (I286_CX) JMPNOP(4) else JMPSHORT(8)
}

I286FN _in_al_data8(void) {					// E4:	in		al, DATA8

	UINT	port;

	I286_WORKCLOCK(5);
	GET_PCBYTE(port)
	I286_INPADRS = CS_BASE + I286_IP;
	I286_AL = iocore_inp8(port);
	I286_INPADRS = 0;
}

I286FN _in_ax_data8(void) {					// E5:	in		ax, DATA8

	UINT	port;

	I286_WORKCLOCK(5);
	GET_PCBYTE(port)
	I286_AX = iocore_inp16(port);
}

I286FN _out_data8_al(void) {				// E6:	out		DATA8, al

	UINT	port;

	I286_WORKCLOCK(3);
	GET_PCBYTE(port);
	iocore_out8(port, I286_AL);
}

I286FN _out_data8_ax(void) {				// E7:	out		DATA8, ax

	UINT	port;

	I286_WORKCLOCK(3);
	GET_PCBYTE(port);
	iocore_out16(port, I286_AX);
}

I286FN _call_near(void) {					// E8:	call near

	UINT16	ad;

	I286_WORKCLOCK(7);
	GET_PCWORD(ad)
	REGPUSH0(I286_IP)
	I286_IP += ad;
}

I286FN _jmp_near(void) {					// E9:	jmp near

	UINT16	ad;

	I286_WORKCLOCK(7);
	GET_PCWORD(ad)
	I286_IP += ad;
}

I286FN _jmp_far(void) {						// EA:	jmp far

	UINT16	ad;

	I286_WORKCLOCK(11);
	GET_PCWORD(ad);
	GET_PCWORD(I286_CS);
	I286_IP = ad;
	CS_BASE = SEGSELECT(I286_CS);
}

I286FN _jmp_short(void) {					// EB:	jmp short

	UINT16	ad;

	I286_WORKCLOCK(7);
	GET_PCBYTES(ad)
	I286_IP += ad;
}

I286FN _in_al_dx(void) {					// EC:	in		al, dx

	I286_WORKCLOCK(5);
	I286_AL = iocore_inp8(I286_DX);
}

I286FN _in_ax_dx(void) {					// ED:	in		ax, dx

	I286_WORKCLOCK(5);
	I286_AX = iocore_inp16(I286_DX);
}

I286FN _out_dx_al(void) {					// EE:	out		dx, al

	I286_WORKCLOCK(3);
	iocore_out8(I286_DX, I286_AL);
}

I286FN _out_dx_ax(void) {					// EF:	out		dx, ax

	I286_WORKCLOCK(3);
	iocore_out16(I286_DX, I286_AX);
}

I286FN _lock(void) {						// F0:	lock
											// F1:	lock
	I286_WORKCLOCK(2);
}

I286FN _repne(void) {						// F2:	repne

	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repne[op]();
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _repe(void) {						// F3:	repe

	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repe[op]();
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _hlt(void) {							// F4:	hlt

	I286_REMCLOCK = -1;
	I286_IP--;
}

I286FN _cmc(void) {							// F5:	cmc

	I286_WORKCLOCK(2);
	I286_FLAGL ^= C_FLAG;
}

I286FN _ope0xf6(void) {						// F6:	

	UINT	op;

	GET_PCBYTE(op);
	c_ope0xf6_table[(op >> 3) & 7](op);
}

I286FN _ope0xf7(void) {						// F7:	

	UINT	op;

	GET_PCBYTE(op);
	c_ope0xf7_table[(op >> 3) & 7](op);
}

I286FN _clc(void) {							// F8:	clc

	I286_WORKCLOCK(2);
	I286_FLAGL &= ~C_FLAG;
}

I286FN _stc(void) {							// F9:	stc

	I286_WORKCLOCK(2);
	I286_FLAGL |= C_FLAG;
}

I286FN _cli(void) {							// FA:	cli

	I286_WORKCLOCK(2);
	I286_FLAG &= ~I_FLAG;
	I286_TRAP = 0;
}

I286FN _sti(void) {							// FB:	sti

	I286_WORKCLOCK(2);
#if defined(INTR_FAST)
	if (I286_FLAG & I_FLAG) {
		NEXT_OPCODE;
		return;									// XV‚ÌˆÓ–¡‚È‚µ
	}
#endif
	I286_FLAG |= I_FLAG;
	I286_TRAP = (I286_FLAG & T_FLAG) >> 8;
#if defined(INTR_FAST)
	if ((I286_TRAP) || (PICEXISTINTR)) {
		REMAIN_ADJUST(1)
	}
	else {
		NEXT_OPCODE;
	}
#else
	REMAIN_ADJUST(1)
#endif
}

I286FN _cld(void) {							// FC:	cld

	I286_WORKCLOCK(2);
	I286_FLAG &= ~D_FLAG;
}

I286FN _std(void) {							// FD:	std

	I286_WORKCLOCK(2);
	I286_FLAG |= D_FLAG;
}

I286FN _ope0xfe(void) {						// FE:	

	UINT	op;

	GET_PCBYTE(op);
	c_ope0xfe_table[(op >> 3) & 1](op);
}

I286FN _ope0xff(void) {						// FF:	

	UINT	op;

	GET_PCBYTE(op);
	c_ope0xff_table[(op >> 3) & 7](op);
}

// -------------------------------------------------------------------------

const I286OP i286op[] = {
			_add_ea_r8,						// 00:	add		EA, REG8
			_add_ea_r16,					// 01:	add		EA, REG16
			_add_r8_ea,						// 02:	add		REG8, EA
			_add_r16_ea,					// 03:	add		REG16, EA
			_add_al_data8,					// 04:	add		al, DATA8
			_add_ax_data16,					// 05:	add		ax, DATA16
			_push_es,						// 06:	push	es
			_pop_es,						// 07:	pop		es
			_or_ea_r8,						// 08:	or		EA, REGF8
			_or_ea_r16,						// 09:	or		EA, REG16
			_or_r8_ea,						// 0A:	or		REG8, EA
			_or_r16_ea,						// 0B:	or		REG16, EA
			_or_al_data8,					// 0C:	or		al, DATA8
			_or_ax_data16,					// 0D:	or		ax, DATA16
			_push_cs,						// 0E:	push	cs
			i286c_cts,						// 0F:	i286 upper opcode

			_adc_ea_r8,						// 10:	adc		EA, REG8
			_adc_ea_r16,					// 11:	adc		EA, REG16
			_adc_r8_ea,						// 12:	adc		REG8, EA
			_adc_r16_ea,					// 13:	adc		REG16, EA
			_adc_al_data8,					// 14:	adc		al, DATA8
			_adc_ax_data16,					// 15:	adc		ax, DATA16
			_push_ss,						// 16:	push	ss
			_pop_ss,						// 17:	pop		ss
			_sbb_ea_r8,						// 18:	sbb		EA, REG8
			_sbb_ea_r16,					// 19:	sbb		EA, REG16
			_sbb_r8_ea,						// 1A:	sbb		REG8, EA
			_sbb_r16_ea,					// 1B:	sbb		REG16, EA
			_sbb_al_data8,					// 1C:	sbb		al, DATA8
			_sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			_push_ds,						// 1E:	push	ds
			_pop_ds,						// 1F:	pop		ds

			_and_ea_r8,						// 20:	and		EA, REG8
			_and_ea_r16,					// 21:	and		EA, REG16
			_and_r8_ea,						// 22:	and		REG8, EA
			_and_r16_ea,					// 23:	and		REG16, EA
			_and_al_data8,					// 24:	and		al, DATA8
			_and_ax_data16,					// 25:	and		ax, DATA16
			_segprefix_es,					// 26:	es:
			_daa,							// 27:	daa
			_sub_ea_r8,						// 28:	sub		EA, REG8
			_sub_ea_r16,					// 29:	sub		EA, REG16
			_sub_r8_ea,						// 2A:	sub		REG8, EA
			_sub_r16_ea,					// 2B:	sub		REG16, EA
			_sub_al_data8,					// 2C:	sub		al, DATA8
			_sub_ax_data16,					// 2D:	sub		ax, DATA16
			_segprefix_cs,					// 2E:	cs:
			_das,							// 2F:	das

			_xor_ea_r8,						// 30:	xor		EA, REG8
			_xor_ea_r16,					// 31:	xor		EA, REG16
			_xor_r8_ea,						// 32:	xor		REG8, EA
			_xor_r16_ea,					// 33:	xor		REG16, EA
			_xor_al_data8,					// 34:	xor		al, DATA8
			_xor_ax_data16,					// 35:	xor		ax, DATA16
			_segprefix_ss,					// 36:	ss:
			_aaa,							// 37:	aaa
			_cmp_ea_r8,						// 38:	cmp		EA, REG8
			_cmp_ea_r16,					// 39:	cmp		EA, REG16
			_cmp_r8_ea,						// 3A:	cmp		REG8, EA
			_cmp_r16_ea,					// 3B:	cmp		REG16, EA
			_cmp_al_data8,					// 3C:	cmp		al, DATA8
			_cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			_segprefix_ds,					// 3E:	ds:
			_aas,							// 3F:	aas

			_inc_ax,						// 40:	inc		ax
			_inc_cx,						// 41:	inc		cx
			_inc_dx,						// 42:	inc		dx
			_inc_bx,						// 43:	inc		bx
			_inc_sp,						// 44:	inc		sp
			_inc_bp,						// 45:	inc		bp
			_inc_si,						// 46:	inc		si
			_inc_di,						// 47:	inc		di
			_dec_ax,						// 48:	dec		ax
			_dec_cx,						// 49:	dec		cx
			_dec_dx,						// 4A:	dec		dx
			_dec_bx,						// 4B:	dec		bx
			_dec_sp,						// 4C:	dec		sp
			_dec_bp,						// 4D:	dec		bp
			_dec_si,						// 4E:	dec		si
			_dec_di,						// 4F:	dec		di

			_push_ax,						// 50:	push	ax
			_push_cx,						// 51:	push	cx
			_push_dx,						// 52:	push	dx
			_push_bx,						// 53:	push	bx
			_push_sp,						// 54:	push	sp
			_push_bp,						// 55:	push	bp
			_push_si,						// 56:	push	si
			_push_di,						// 57:	push	di
			_pop_ax,						// 58:	pop		ax
			_pop_cx,						// 59:	pop		cx
			_pop_dx,						// 5A:	pop		dx
			_pop_bx,						// 5B:	pop		bx
			_pop_sp,						// 5C:	pop		sp
			_pop_bp,						// 5D:	pop		bp
			_pop_si,						// 5E:	pop		si
			_pop_di,						// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_arpl,							// 63:	arpl
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			_push_data16,					// 68:	push	DATA16
			_imul_reg_ea_data16,			// 69:	imul	REG, EA, DATA16
			_push_data8,					// 6A:	push	DATA8
			_imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			_insb,							// 6C:	insb
			_insw,							// 6D:	insw
			_outsb,							// 6E:	outsb
			_outsw,							// 6F:	outsw

			_jo_short,						// 70:	jo short
			_jno_short,						// 71:	jno short
			_jc_short,						// 72:	jnae/jb/jc short
			_jnc_short,						// 73:	jae/jnb/jnc short
			_jz_short,						// 74:	je/jz short
			_jnz_short,						// 75:	jne/jnz short
			_jna_short,						// 76:	jna/jbe short
			_ja_short,						// 77:	ja/jnbe short
			_js_short,						// 78:	js short
			_jns_short,						// 79:	jns short
			_jp_short,						// 7A:	jp/jpe short
			_jnp_short,						// 7B:	jnp/jpo short
			_jl_short,						// 7C:	jl/jnge short
			_jnl_short,						// 7D:	jnl/jge short
			_jle_short,						// 7E:	jle/jng short
			_jnle_short,					// 7F:	jg/jnle short

			_calc_ea8_i8,					// 80:	op		EA8, DATA8
			_calc_ea16_i16,					// 81:	op		EA16, DATA16
			_calc_ea8_i8,					// 82:	op		EA8, DATA8
			_calc_ea16_i8,					// 83:	op		EA16, DATA8
			_test_ea_r8,					// 84:	test	EA, REG8
			_test_ea_r16,					// 85:	test	EA, REG16
			_xchg_ea_r8,					// 86:	xchg	EA, REG8
			_xchg_ea_r16,					// 87:	xchg	EA, REG16
			_mov_ea_r8,						// 88:	mov		EA, REG8
			_mov_ea_r16,					// 89:	mov		EA, REG16
			_mov_r8_ea,						// 8A:	mov		REG8, EA
			_mov_r16_ea,					// 8B:	mov		REG16, EA
			_mov_ea_seg,					// 8C:	mov		EA, segreg
			_lea_r16_ea,					// 8D:	lea		REG16, EA
			_mov_seg_ea,					// 8E:	mov		segrem, EA
			_pop_ea,						// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			_xchg_ax_cx,					// 91:	xchg	ax, cx
			_xchg_ax_dx,					// 92:	xchg	ax, dx
			_xchg_ax_bx,					// 93:	xchg	ax, bx
			_xchg_ax_sp,					// 94:	xchg	ax, sp
			_xchg_ax_bp,					// 95:	xchg	ax, bp
			_xchg_ax_si,					// 96:	xchg	ax, si
			_xchg_ax_di,					// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			_call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			_mov_al_m8,						// A0:	mov		al, m8
			_mov_ax_m16,					// A1:	mov		ax, m16
			_mov_m8_al,						// A2:	mov		m8, al
			_mov_m16_ax,					// A3:	mov		m16, ax
			_movsb,							// A4:	movsb
			_movsw,							// A5:	movsw
			_cmpsb,							// A6:	cmpsb
			_cmpsw,							// A7:	cmpsw
			_test_al_data8,					// A8:	test	al, DATA8
			_test_ax_data16,				// A9:	test	ax, DATA16
			_stosb,							// AA:	stosw
			_stosw,							// AB:	stosw
			_lodsb,							// AC:	lodsb
			_lodsw,							// AD:	lodsw
			_scasb,							// AE:	scasb
			_scasw,							// AF:	scasw

			_mov_al_imm,					// B0:	mov		al, imm8
			_mov_cl_imm,					// B1:	mov		cl, imm8
			_mov_dl_imm,					// B2:	mov		dl, imm8
			_mov_bl_imm,					// B3:	mov		bl, imm8
			_mov_ah_imm,					// B4:	mov		ah, imm8
			_mov_ch_imm,					// B5:	mov		ch, imm8
			_mov_dh_imm,					// B6:	mov		dh, imm8
			_mov_bh_imm,					// B7:	mov		bh, imm8
			_mov_ax_imm,					// B8:	mov		ax, imm16
			_mov_cx_imm,					// B9:	mov		cx, imm16
			_mov_dx_imm,					// BA:	mov		dx, imm16
			_mov_bx_imm,					// BB:	mov		bx, imm16
			_mov_sp_imm,					// BC:	mov		sp, imm16
			_mov_bp_imm,					// BD:	mov		bp, imm16
			_mov_si_imm,					// BE:	mov		si, imm16
			_mov_di_imm,					// BF:	mov		di, imm16

			_shift_ea8_data8,				// C0:	shift	EA8, DATA8
			_shift_ea16_data8,				// C1:	shift	EA16, DATA8
			_ret_near_data16,				// C2:	ret near DATA16
			_ret_near,						// C3:	ret near
			_les_r16_ea,					// C4:	les		REG16, EA
			_lds_r16_ea,					// C5:	lds		REG16, EA
			_mov_ea8_data8,					// C6:	mov		EA8, DATA8
			_mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			fleave,							// C9:	leave
			_ret_far_data16,				// CA:	ret far	DATA16
			_ret_far,						// CB:	ret far
			_int_03,						// CC:	int		3
			_int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			_shift_ea8_1,					// D0:	shift EA8, 1
			_shift_ea16_1,					// D1:	shift EA16, 1
			_shift_ea8_cl,					// D2:	shift EA8, cl
			_shift_ea16_cl,					// D3:	shift EA16, cl
			_aam,							// D4:	AAM
			_aad,							// D5:	AAD
			_setalc,						// D6:	setalc (80286)
			_xlat,							// D7:	xlat
			_esc,							// D8:	esc
			_esc,							// D9:	esc
			_esc,							// DA:	esc
			_esc,							// DB:	esc
			_esc,							// DC:	esc
			_esc,							// DD:	esc
			_esc,							// DE:	esc
			_esc,							// DF:	esc

			_loopnz,						// E0:	loopnz
			_loopz,							// E1:	loopz
			_loop,							// E2:	loop
			_jcxz,							// E3:	jcxz
			_in_al_data8,					// E4:	in		al, DATA8
			_in_ax_data8,					// E5:	in		ax, DATA8
			_out_data8_al,					// E6:	out		DATA8, al
			_out_data8_ax,					// E7:	out		DATA8, ax
			_call_near,						// E8:	call near
			_jmp_near,						// E9:	jmp near
			_jmp_far,						// EA:	jmp far
			_jmp_short,						// EB:	jmp short
			_in_al_dx,						// EC:	in		al, dx
			_in_ax_dx,						// ED:	in		ax, dx
			_out_dx_al,						// EE:	out		dx, al
			_out_dx_ax,						// EF:	out		dx, ax

			_lock,							// F0:	lock
			_lock,							// F1:	lock
			_repne,							// F2:	repne
			_repe,							// F3:	repe
			_hlt,							// F4:	hlt
			_cmc,							// F5:	cmc
			_ope0xf6,						// F6:	
			_ope0xf7,						// F7:	
			_clc,							// F8:	clc
			_stc,							// F9:	stc
			_cli,							// FA:	cli
			_sti,							// FB:	sti
			_cld,							// FC:	cld
			_std,							// FD:	std
			_ope0xfe,						// FE:	
			_ope0xff,						// FF:	
};



// ----------------------------------------------------------------- repe

I286FN _repe_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repe[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _repe_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repe[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _repe_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repe[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _repe_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repe[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

const I286OP i286op_repe[] = {
			_add_ea_r8,						// 00:	add		EA, REG8
			_add_ea_r16,					// 01:	add		EA, REG16
			_add_r8_ea,						// 02:	add		REG8, EA
			_add_r16_ea,					// 03:	add		REG16, EA
			_add_al_data8,					// 04:	add		al, DATA8
			_add_ax_data16,					// 05:	add		ax, DATA16
			_push_es,						// 06:	push	es
			_pop_es,						// 07:	pop		es
			_or_ea_r8,						// 08:	or		EA, REGF8
			_or_ea_r16,						// 09:	or		EA, REG16
			_or_r8_ea,						// 0A:	or		REG8, EA
			_or_r16_ea,						// 0B:	or		REG16, EA
			_or_al_data8,					// 0C:	or		al, DATA8
			_or_ax_data16,					// 0D:	or		ax, DATA16
			_push_cs,						// 0E:	push	cs
			i286c_cts,						// 0F:	i286 upper opcode

			_adc_ea_r8,						// 10:	adc		EA, REG8
			_adc_ea_r16,					// 11:	adc		EA, REG16
			_adc_r8_ea,						// 12:	adc		REG8, EA
			_adc_r16_ea,					// 13:	adc		REG16, EA
			_adc_al_data8,					// 14:	adc		al, DATA8
			_adc_ax_data16,					// 15:	adc		ax, DATA16
			_push_ss,						// 16:	push	ss
			_pop_ss,						// 17:	pop		ss
			_sbb_ea_r8,						// 18:	sbb		EA, REG8
			_sbb_ea_r16,					// 19:	sbb		EA, REG16
			_sbb_r8_ea,						// 1A:	sbb		REG8, EA
			_sbb_r16_ea,					// 1B:	sbb		REG16, EA
			_sbb_al_data8,					// 1C:	sbb		al, DATA8
			_sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			_push_ds,						// 1E:	push	ds
			_pop_ds,						// 1F:	pop		ds

			_and_ea_r8,						// 20:	and		EA, REG8
			_and_ea_r16,					// 21:	and		EA, REG16
			_and_r8_ea,						// 22:	and		REG8, EA
			_and_r16_ea,					// 23:	and		REG16, EA
			_and_al_data8,					// 24:	and		al, DATA8
			_and_ax_data16,					// 25:	and		ax, DATA16
			_repe_segprefix_es,				// 26:	repe es:
			_daa,							// 27:	daa
			_sub_ea_r8,						// 28:	sub		EA, REG8
			_sub_ea_r16,					// 29:	sub		EA, REG16
			_sub_r8_ea,						// 2A:	sub		REG8, EA
			_sub_r16_ea,					// 2B:	sub		REG16, EA
			_sub_al_data8,					// 2C:	sub		al, DATA8
			_sub_ax_data16,					// 2D:	sub		ax, DATA16
			_repe_segprefix_cs,				// 2E:	repe cs:
			_das,							// 2F:	das

			_xor_ea_r8,						// 30:	xor		EA, REG8
			_xor_ea_r16,					// 31:	xor		EA, REG16
			_xor_r8_ea,						// 32:	xor		REG8, EA
			_xor_r16_ea,					// 33:	xor		REG16, EA
			_xor_al_data8,					// 34:	xor		al, DATA8
			_xor_ax_data16,					// 35:	xor		ax, DATA16
			_repe_segprefix_ss,				// 36:	repe ss:
			_aaa,							// 37:	aaa
			_cmp_ea_r8,						// 38:	cmp		EA, REG8
			_cmp_ea_r16,					// 39:	cmp		EA, REG16
			_cmp_r8_ea,						// 3A:	cmp		REG8, EA
			_cmp_r16_ea,					// 3B:	cmp		REG16, EA
			_cmp_al_data8,					// 3C:	cmp		al, DATA8
			_cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			_repe_segprefix_ds,				// 3E:	repe ds:
			_aas,							// 3F:	aas

			_inc_ax,						// 40:	inc		ax
			_inc_cx,						// 41:	inc		cx
			_inc_dx,						// 42:	inc		dx
			_inc_bx,						// 43:	inc		bx
			_inc_sp,						// 44:	inc		sp
			_inc_bp,						// 45:	inc		bp
			_inc_si,						// 46:	inc		si
			_inc_di,						// 47:	inc		di
			_dec_ax,						// 48:	dec		ax
			_dec_cx,						// 49:	dec		cx
			_dec_dx,						// 4A:	dec		dx
			_dec_bx,						// 4B:	dec		bx
			_dec_sp,						// 4C:	dec		sp
			_dec_bp,						// 4D:	dec		bp
			_dec_si,						// 4E:	dec		si
			_dec_di,						// 4F:	dec		di

			_push_ax,						// 50:	push	ax
			_push_cx,						// 51:	push	cx
			_push_dx,						// 52:	push	dx
			_push_bx,						// 53:	push	bx
			_push_sp,						// 54:	push	sp
			_push_bp,						// 55:	push	bp
			_push_si,						// 56:	push	si
			_push_di,						// 57:	push	di
			_pop_ax,						// 58:	pop		ax
			_pop_cx,						// 59:	pop		cx
			_pop_dx,						// 5A:	pop		dx
			_pop_bx,						// 5B:	pop		bx
			_pop_sp,						// 5C:	pop		sp
			_pop_bp,						// 5D:	pop		bp
			_pop_si,						// 5E:	pop		si
			_pop_di,						// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_arpl,							// 63:	arpl
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			_push_data16,					// 68:	push	DATA16
			_imul_reg_ea_data16,			// 69:	imul	REG, EA, DATA16
			_push_data8,					// 6A:	push	DATA8
			_imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			i286c_rep_insb,					// 6C:	rep insb
			i286c_rep_insw,					// 6D:	rep insw
			i286c_rep_outsb,				// 6E:	rep outsb
			i286c_rep_outsw,				// 6F:	rep outsw

			_jo_short,						// 70:	jo short
			_jno_short,						// 71:	jno short
			_jc_short,						// 72:	jnae/jb/jc short
			_jnc_short,						// 73:	jae/jnb/jnc short
			_jz_short,						// 74:	je/jz short
			_jnz_short,						// 75:	jne/jnz short
			_jna_short,						// 76:	jna/jbe short
			_ja_short,						// 77:	ja/jnbe short
			_js_short,						// 78:	js short
			_jns_short,						// 79:	jns short
			_jp_short,						// 7A:	jp/jpe short
			_jnp_short,						// 7B:	jnp/jpo short
			_jl_short,						// 7C:	jl/jnge short
			_jnl_short,						// 7D:	jnl/jge short
			_jle_short,						// 7E:	jle/jng short
			_jnle_short,					// 7F:	jg/jnle short

			_calc_ea8_i8,					// 80:	op		EA8, DATA8
			_calc_ea16_i16,					// 81:	op		EA16, DATA16
			_calc_ea8_i8,					// 82:	op		EA8, DATA8
			_calc_ea16_i8,					// 83:	op		EA16, DATA8
			_test_ea_r8,					// 84:	test	EA, REG8
			_test_ea_r16,					// 85:	test	EA, REG16
			_xchg_ea_r8,					// 86:	xchg	EA, REG8
			_xchg_ea_r16,					// 87:	xchg	EA, REG16
			_mov_ea_r8,						// 88:	mov		EA, REG8
			_mov_ea_r16,					// 89:	mov		EA, REG16
			_mov_r8_ea,						// 8A:	mov		REG8, EA
			_mov_r16_ea,					// 8B:	add		REG16, EA
			_mov_ea_seg,					// 8C:	mov		EA, segreg
			_lea_r16_ea,					// 8D:	lea		REG16, EA
			_mov_seg_ea,					// 8E:	mov		segrem, EA
			_pop_ea,						// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			_xchg_ax_cx,					// 91:	xchg	ax, cx
			_xchg_ax_dx,					// 92:	xchg	ax, dx
			_xchg_ax_bx,					// 93:	xchg	ax, bx
			_xchg_ax_sp,					// 94:	xchg	ax, sp
			_xchg_ax_bp,					// 95:	xchg	ax, bp
			_xchg_ax_si,					// 96:	xchg	ax, si
			_xchg_ax_di,					// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			_call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			_mov_al_m8,						// A0:	mov		al, m8
			_mov_ax_m16,					// A1:	mov		ax, m16
			_mov_m8_al,						// A2:	mov		m8, al
			_mov_m16_ax,					// A3:	mov		m16, ax
			i286c_rep_movsb,				// A4:	rep movsb
			i286c_rep_movsw,				// A5:	rep movsw
			i286c_repe_cmpsb,				// A6:	repe cmpsb
			i286c_repe_cmpsw,				// A7:	repe cmpsw
			_test_al_data8,					// A8:	test	al, DATA8
			_test_ax_data16,				// A9:	test	ax, DATA16
			i286c_rep_stosb,				// AA:	rep stosb
			i286c_rep_stosw,				// AB:	rep stosw
			i286c_rep_lodsb,				// AC:	rep lodsb
			i286c_rep_lodsw,				// AD:	rep lodsw
			i286c_repe_scasb,				// AE:	repe scasb
			i286c_repe_scasw,				// AF:	repe scasw

			_mov_al_imm,					// B0:	mov		al, imm8
			_mov_cl_imm,					// B1:	mov		cl, imm8
			_mov_dl_imm,					// B2:	mov		dl, imm8
			_mov_bl_imm,					// B3:	mov		bl, imm8
			_mov_ah_imm,					// B4:	mov		ah, imm8
			_mov_ch_imm,					// B5:	mov		ch, imm8
			_mov_dh_imm,					// B6:	mov		dh, imm8
			_mov_bh_imm,					// B7:	mov		bh, imm8
			_mov_ax_imm,					// B8:	mov		ax, imm16
			_mov_cx_imm,					// B9:	mov		cx, imm16
			_mov_dx_imm,					// BA:	mov		dx, imm16
			_mov_bx_imm,					// BB:	mov		bx, imm16
			_mov_sp_imm,					// BC:	mov		sp, imm16
			_mov_bp_imm,					// BD:	mov		bp, imm16
			_mov_si_imm,					// BE:	mov		si, imm16
			_mov_di_imm,					// BF:	mov		di, imm16

			_shift_ea8_data8,				// C0:	shift	EA8, DATA8
			_shift_ea16_data8,				// C1:	shift	EA16, DATA8
			_ret_near_data16,				// C2:	ret near DATA16
			_ret_near,						// C3:	ret near
			_les_r16_ea,					// C4:	les		REG16, EA
			_lds_r16_ea,					// C5:	lds		REG16, EA
			_mov_ea8_data8,					// C6:	mov		EA8, DATA8
			_mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			fleave,							// C9:	leave
			_ret_far_data16,				// CA:	ret far	DATA16
			_ret_far,						// CB:	ret far
			_int_03,						// CC:	int		3
			_int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			_shift_ea8_1,					// D0:	shift EA8, 1
			_shift_ea16_1,					// D1:	shift EA16, 1
			_shift_ea8_cl,					// D2:	shift EA8, cl
			_shift_ea16_cl,					// D3:	shift EA16, cl
			_aam,							// D4:	AAM
			_aad,							// D5:	AAD
			_setalc,						// D6:	setalc (80286)
			_xlat,							// D7:	xlat
			_esc,							// D8:	esc
			_esc,							// D9:	esc
			_esc,							// DA:	esc
			_esc,							// DB:	esc
			_esc,							// DC:	esc
			_esc,							// DD:	esc
			_esc,							// DE:	esc
			_esc,							// DF:	esc

			_loopnz,						// E0:	loopnz
			_loopz,							// E1:	loopz
			_loop,							// E2:	loop
			_jcxz,							// E3:	jcxz
			_in_al_data8,					// E4:	in		al, DATA8
			_in_ax_data8,					// E5:	in		ax, DATA8
			_out_data8_al,					// E6:	out		DATA8, al
			_out_data8_ax,					// E7:	out		DATA8, ax
			_call_near,						// E8:	call near
			_jmp_near,						// E9:	jmp near
			_jmp_far,						// EA:	jmp far
			_jmp_short,						// EB:	jmp short
			_in_al_dx,						// EC:	in		al, dx
			_in_ax_dx,						// ED:	in		ax, dx
			_out_dx_al,						// EE:	out		dx, al
			_out_dx_ax,						// EF:	out		dx, ax

			_lock,							// F0:	lock
			_lock,							// F1:	lock
			_repne,							// F2:	repne
			_repe,							// F3:	repe
			_hlt,							// F4:	hlt
			_cmc,							// F5:	cmc
			_ope0xf6,						// F6:	
			_ope0xf7,						// F7:	
			_clc,							// F8:	clc
			_stc,							// F9:	stc
			_cli,							// FA:	cli
			_sti,							// FB:	sti
			_cld,							// FC:	cld
			_std,							// FD:	std
			_ope0xfe,						// FE:	
			_ope0xff,						// FF:	
};


// ----------------------------------------------------------------- repne

I286FN _repne_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repne[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _repne_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repne[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _repne_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repne[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN _repne_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		i286op_repne[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

const I286OP i286op_repne[] = {
			_add_ea_r8,						// 00:	add		EA, REG8
			_add_ea_r16,					// 01:	add		EA, REG16
			_add_r8_ea,						// 02:	add		REG8, EA
			_add_r16_ea,					// 03:	add		REG16, EA
			_add_al_data8,					// 04:	add		al, DATA8
			_add_ax_data16,					// 05:	add		ax, DATA16
			_push_es,						// 06:	push	es
			_pop_es,						// 07:	pop		es
			_or_ea_r8,						// 08:	or		EA, REGF8
			_or_ea_r16,						// 09:	or		EA, REG16
			_or_r8_ea,						// 0A:	or		REG8, EA
			_or_r16_ea,						// 0B:	or		REG16, EA
			_or_al_data8,					// 0C:	or		al, DATA8
			_or_ax_data16,					// 0D:	or		ax, DATA16
			_push_cs,						// 0E:	push	cs
			i286c_cts,						// 0F:	i286 upper opcode

			_adc_ea_r8,						// 10:	adc		EA, REG8
			_adc_ea_r16,					// 11:	adc		EA, REG16
			_adc_r8_ea,						// 12:	adc		REG8, EA
			_adc_r16_ea,					// 13:	adc		REG16, EA
			_adc_al_data8,					// 14:	adc		al, DATA8
			_adc_ax_data16,					// 15:	adc		ax, DATA16
			_push_ss,						// 16:	push	ss
			_pop_ss,						// 17:	pop		ss
			_sbb_ea_r8,						// 18:	sbb		EA, REG8
			_sbb_ea_r16,					// 19:	sbb		EA, REG16
			_sbb_r8_ea,						// 1A:	sbb		REG8, EA
			_sbb_r16_ea,					// 1B:	sbb		REG16, EA
			_sbb_al_data8,					// 1C:	sbb		al, DATA8
			_sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			_push_ds,						// 1E:	push	ds
			_pop_ds,						// 1F:	pop		ds

			_and_ea_r8,						// 20:	and		EA, REG8
			_and_ea_r16,					// 21:	and		EA, REG16
			_and_r8_ea,						// 22:	and		REG8, EA
			_and_r16_ea,					// 23:	and		REG16, EA
			_and_al_data8,					// 24:	and		al, DATA8
			_and_ax_data16,					// 25:	and		ax, DATA16
			_repne_segprefix_es,			// 26:	repne es:
			_daa,							// 27:	daa
			_sub_ea_r8,						// 28:	sub		EA, REG8
			_sub_ea_r16,					// 29:	sub		EA, REG16
			_sub_r8_ea,						// 2A:	sub		REG8, EA
			_sub_r16_ea,					// 2B:	sub		REG16, EA
			_sub_al_data8,					// 2C:	sub		al, DATA8
			_sub_ax_data16,					// 2D:	sub		ax, DATA16
			_repne_segprefix_cs,			// 2E:	repne cs:
			_das,							// 2F:	das

			_xor_ea_r8,						// 30:	xor		EA, REG8
			_xor_ea_r16,					// 31:	xor		EA, REG16
			_xor_r8_ea,						// 32:	xor		REG8, EA
			_xor_r16_ea,					// 33:	xor		REG16, EA
			_xor_al_data8,					// 34:	xor		al, DATA8
			_xor_ax_data16,					// 35:	xor		ax, DATA16
			_repne_segprefix_ss,			// 36:	repne ss:
			_aaa,							// 37:	aaa
			_cmp_ea_r8,						// 38:	cmp		EA, REG8
			_cmp_ea_r16,					// 39:	cmp		EA, REG16
			_cmp_r8_ea,						// 3A:	cmp		REG8, EA
			_cmp_r16_ea,					// 3B:	cmp		REG16, EA
			_cmp_al_data8,					// 3C:	cmp		al, DATA8
			_cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			_repne_segprefix_ds,			// 3E:	repne ds:
			_aas,							// 3F:	aas

			_inc_ax,						// 40:	inc		ax
			_inc_cx,						// 41:	inc		cx
			_inc_dx,						// 42:	inc		dx
			_inc_bx,						// 43:	inc		bx
			_inc_sp,						// 44:	inc		sp
			_inc_bp,						// 45:	inc		bp
			_inc_si,						// 46:	inc		si
			_inc_di,						// 47:	inc		di
			_dec_ax,						// 48:	dec		ax
			_dec_cx,						// 49:	dec		cx
			_dec_dx,						// 4A:	dec		dx
			_dec_bx,						// 4B:	dec		bx
			_dec_sp,						// 4C:	dec		sp
			_dec_bp,						// 4D:	dec		bp
			_dec_si,						// 4E:	dec		si
			_dec_di,						// 4F:	dec		di

			_push_ax,						// 50:	push	ax
			_push_cx,						// 51:	push	cx
			_push_dx,						// 52:	push	dx
			_push_bx,						// 53:	push	bx
			_push_sp,						// 54:	push	sp
			_push_bp,						// 55:	push	bp
			_push_si,						// 56:	push	si
			_push_di,						// 57:	push	di
			_pop_ax,						// 58:	pop		ax
			_pop_cx,						// 59:	pop		cx
			_pop_dx,						// 5A:	pop		dx
			_pop_bx,						// 5B:	pop		bx
			_pop_sp,						// 5C:	pop		sp
			_pop_bp,						// 5D:	pop		bp
			_pop_si,						// 5E:	pop		si
			_pop_di,						// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_arpl,							// 63:	arpl
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			_push_data16,					// 68:	push	DATA16
			_imul_reg_ea_data16,			// 69:	imul	REG, EA, DATA16
			_push_data8,					// 6A:	push	DATA8
			_imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			i286c_rep_insb,					// 6C:	rep insb
			i286c_rep_insw,					// 6D:	rep insw
			i286c_rep_outsb,				// 6E:	rep outsb
			i286c_rep_outsw,				// 6F:	rep outsw

			_jo_short,						// 70:	jo short
			_jno_short,						// 71:	jno short
			_jc_short,						// 72:	jnae/jb/jc short
			_jnc_short,						// 73:	jae/jnb/jnc short
			_jz_short,						// 74:	je/jz short
			_jnz_short,						// 75:	jne/jnz short
			_jna_short,						// 76:	jna/jbe short
			_ja_short,						// 77:	ja/jnbe short
			_js_short,						// 78:	js short
			_jns_short,						// 79:	jns short
			_jp_short,						// 7A:	jp/jpe short
			_jnp_short,						// 7B:	jnp/jpo short
			_jl_short,						// 7C:	jl/jnge short
			_jnl_short,						// 7D:	jnl/jge short
			_jle_short,						// 7E:	jle/jng short
			_jnle_short,					// 7F:	jg/jnle short

			_calc_ea8_i8,					// 80:	op		EA8, DATA8
			_calc_ea16_i16,					// 81:	op		EA16, DATA16
			_calc_ea8_i8,					// 82:	op		EA8, DATA8
			_calc_ea16_i8,					// 83:	op		EA16, DATA8
			_test_ea_r8,					// 84:	test	EA, REG8
			_test_ea_r16,					// 85:	test	EA, REG16
			_xchg_ea_r8,					// 86:	xchg	EA, REG8
			_xchg_ea_r16,					// 87:	xchg	EA, REG16
			_mov_ea_r8,						// 88:	mov		EA, REG8
			_mov_ea_r16,					// 89:	mov		EA, REG16
			_mov_r8_ea,						// 8A:	mov		REG8, EA
			_mov_r16_ea,					// 8B:	add		REG16, EA
			_mov_ea_seg,					// 8C:	mov		EA, segreg
			_lea_r16_ea,					// 8D:	lea		REG16, EA
			_mov_seg_ea,					// 8E:	mov		segrem, EA
			_pop_ea,						// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			_xchg_ax_cx,					// 91:	xchg	ax, cx
			_xchg_ax_dx,					// 92:	xchg	ax, dx
			_xchg_ax_bx,					// 93:	xchg	ax, bx
			_xchg_ax_sp,					// 94:	xchg	ax, sp
			_xchg_ax_bp,					// 95:	xchg	ax, bp
			_xchg_ax_si,					// 96:	xchg	ax, si
			_xchg_ax_di,					// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			_call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			_mov_al_m8,						// A0:	mov		al, m8
			_mov_ax_m16,					// A1:	mov		ax, m16
			_mov_m8_al,						// A2:	mov		m8, al
			_mov_m16_ax,					// A3:	mov		m16, ax
			i286c_rep_movsb,				// A4:	rep movsb
			i286c_rep_movsw,				// A5:	rep movsw
			i286c_repne_cmpsb,				// A6:	repne cmpsb
			i286c_repne_cmpsw,				// A7:	repne cmpsw
			_test_al_data8,					// A8:	test	al, DATA8
			_test_ax_data16,				// A9:	test	ax, DATA16
			i286c_rep_stosb,				// AA:	rep stosb
			i286c_rep_stosw,				// AB:	rep stosw
			i286c_rep_lodsb,				// AC:	rep lodsb
			i286c_rep_lodsw,				// AD:	rep lodsw
			i286c_repne_scasb,				// AE:	repne scasb
			i286c_repne_scasw,				// AF:	repne scasw

			_mov_al_imm,					// B0:	mov		al, imm8
			_mov_cl_imm,					// B1:	mov		cl, imm8
			_mov_dl_imm,					// B2:	mov		dl, imm8
			_mov_bl_imm,					// B3:	mov		bl, imm8
			_mov_ah_imm,					// B4:	mov		ah, imm8
			_mov_ch_imm,					// B5:	mov		ch, imm8
			_mov_dh_imm,					// B6:	mov		dh, imm8
			_mov_bh_imm,					// B7:	mov		bh, imm8
			_mov_ax_imm,					// B8:	mov		ax, imm16
			_mov_cx_imm,					// B9:	mov		cx, imm16
			_mov_dx_imm,					// BA:	mov		dx, imm16
			_mov_bx_imm,					// BB:	mov		bx, imm16
			_mov_sp_imm,					// BC:	mov		sp, imm16
			_mov_bp_imm,					// BD:	mov		bp, imm16
			_mov_si_imm,					// BE:	mov		si, imm16
			_mov_di_imm,					// BF:	mov		di, imm16

			_shift_ea8_data8,				// C0:	shift	EA8, DATA8
			_shift_ea16_data8,				// C1:	shift	EA16, DATA8
			_ret_near_data16,				// C2:	ret near DATA16
			_ret_near,						// C3:	ret near
			_les_r16_ea,					// C4:	les		REG16, EA
			_lds_r16_ea,					// C5:	lds		REG16, EA
			_mov_ea8_data8,					// C6:	mov		EA8, DATA8
			_mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			fleave,							// C9:	leave
			_ret_far_data16,				// CA:	ret far	DATA16
			_ret_far,						// CB:	ret far
			_int_03,						// CC:	int		3
			_int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			_shift_ea8_1,					// D0:	shift EA8, 1
			_shift_ea16_1,					// D1:	shift EA16, 1
			_shift_ea8_cl,					// D2:	shift EA8, cl
			_shift_ea16_cl,					// D3:	shift EA16, cl
			_aam,							// D4:	AAM
			_aad,							// D5:	AAD
			_setalc,						// D6:	setalc (80286)
			_xlat,							// D7:	xlat
			_esc,							// D8:	esc
			_esc,							// D9:	esc
			_esc,							// DA:	esc
			_esc,							// DB:	esc
			_esc,							// DC:	esc
			_esc,							// DD:	esc
			_esc,							// DE:	esc
			_esc,							// DF:	esc

			_loopnz,						// E0:	loopnz
			_loopz,							// E1:	loopz
			_loop,							// E2:	loop
			_jcxz,							// E3:	jcxz
			_in_al_data8,					// E4:	in		al, DATA8
			_in_ax_data8,					// E5:	in		ax, DATA8
			_out_data8_al,					// E6:	out		DATA8, al
			_out_data8_ax,					// E7:	out		DATA8, ax
			_call_near,						// E8:	call near
			_jmp_near,						// E9:	jmp near
			_jmp_far,						// EA:	jmp far
			_jmp_short,						// EB:	jmp short
			_in_al_dx,						// EC:	in		al, dx
			_in_ax_dx,						// ED:	in		ax, dx
			_out_dx_al,						// EE:	out		dx, al
			_out_dx_ax,						// EF:	out		dx, ax

			_lock,							// F0:	lock
			_lock,							// F1:	lock
			_repne,							// F2:	repne
			_repe,							// F3:	repe
			_hlt,							// F4:	hlt
			_cmc,							// F5:	cmc
			_ope0xf6,						// F6:	
			_ope0xf7,						// F7:	
			_clc,							// F8:	clc
			_stc,							// F9:	stc
			_cli,							// FA:	cli
			_sti,							// FB:	sti
			_cld,							// FC:	cld
			_std,							// FD:	std
			_ope0xfe,						// FE:	
			_ope0xff,						// FF:	
};

