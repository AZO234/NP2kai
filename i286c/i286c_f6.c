#include	"compiler.h"
#include	"cpucore.h"
#include	"i286c.h"
#include <limits.h>
#include	"i286c.mcr"


// ------------------------------------------------------------ opecode 0xf6,7

I286_F6 _test_ea8_data8(UINT op) {

	UINT	src;
	UINT	dst;

	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		dst = *(REG8_B20(op));
	}
	else {
		I286_WORKCLOCK(6);
		dst = i286_memoryread(CALC_EA(op));
	}
	GET_PCBYTE(src)
	ANDBYTE(dst, src)
}

I286_F6 _not_ea8(UINT op) {

	UINT32	madr;

	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		*(REG8_B20(op)) ^= 0xff;
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			REG8 value = i286_memoryread(madr);
			value ^= 0xff;
			i286_memorywrite(madr, value);
			return;
		}
		*(mem + madr) ^= 0xff;
	}
}

I286_F6 _neg_ea8(UINT op) {

	UINT8	*out;
	UINT	src;
	UINT	dst;
	UINT32	madr;

	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= I286_MEMWRITEMAX) {
			src = i286_memoryread(madr);
			NEGBYTE(dst, src)
			i286_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	src = *out;
	NEGBYTE(dst, src)
	*out = (UINT8)dst;
}

I286_F6 _mul_ea8(UINT op) {

	UINT8	src;
	UINT	res;

	if (op >= 0xc0) {
		I286_WORKCLOCK(13);
		src = *(REG8_B20(op));
	}
	else {
		I286_WORKCLOCK(16);
		src = i286_memoryread(CALC_EA(op));
	}
	BYTE_MUL(res, I286_AL, src)
	I286_AX = (UINT16)res;
}

I286_F6 _imul_ea8(UINT op) {

	UINT8	src;
	SINT32	res;

	if (op >= 0xc0) {
		I286_WORKCLOCK(13);
		src = *(REG8_B20(op));
	}
	else {
		I286_WORKCLOCK(16);
		src = i286_memoryread(CALC_EA(op));
	}
	BYTE_IMUL(res, I286_AL, src)
	I286_AX = (UINT16)res;
}

I286_F6 _div_ea8(UINT op) {

	UINT16	tmp;
	UINT8	src;
	UINT16	ip;

	ip = I286_IP;
	if (op >= 0xc0) {
		I286_WORKCLOCK(14);
		src = *(REG8_B20(op));
	}
	else {
		I286_WORKCLOCK(17);
		src = i286_memoryread(CALC_EA(op));
	}
	tmp = I286_AX;
	if ((src) && (tmp < ((UINT16)src << 8))) {
		I286_AL = tmp / src;
		I286_AH = tmp % src;
	}
	else {
		INT_NUM(0, ip - 2);										// 80x86
	}
}

I286_F6 _idiv_ea8(UINT op) {

	SINT16	tmp, r;
	SINT8	src;
	UINT16	ip;

	ip = I286_IP;
	if (op >= 0xc0) {
		I286_WORKCLOCK(17);
		src = *(REG8_B20(op));
	}
	else {
		I286_WORKCLOCK(20);
		src = i286_memoryread(CALC_EA(op));
	}
	tmp = (SINT16)I286_AX;
	if (src) {
		r = tmp / src;
		if (!((r + 0x80) & 0xff00)) {
			I286_AL = (UINT8)r;
			I286_AH = tmp % src;
			return;
		}
	}
	INT_NUM(0, ip - 2);											// 80x86
}


I286_F6 _test_ea16_data16(UINT op) {

	UINT32	src;
	UINT32	dst;

	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		dst = *(REG16_B20(op));
	}
	else {
		I286_WORKCLOCK(6);
		dst = i286_memoryread_w(CALC_EA(op));
	}
	GET_PCWORD(src)
	ANDWORD(dst, src)
}

I286_F6 _not_ea16(UINT op) {

	UINT32	madr;

	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		*(REG16_B20(op)) ^= 0xffff;
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (!(INHIBIT_WORDP(madr))) {
			*(mem + madr) ^= 0xffff;
		}
		else {
			REG16 value = i286_memoryread_w(madr);
			value = ~value;
			i286_memorywrite_w(madr, value);
		}
	}
}

I286_F6 _neg_ea16(UINT op) {

	UINT16	*out;
	UINT32	src;
	UINT32	dst;
	UINT32	madr;

	if (op >= 0xc0) {
		I286_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		I286_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			src = i286_memoryread_w(madr);
			NEGWORD(dst, src)
			i286_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	src = *out;
	NEGWORD(dst, src)
	*out = (UINT16)dst;
}

I286_F6 _mul_ea16(UINT op) {

	UINT16	src;
	UINT32	res;

	if (op >= 0xc0) {
		I286_WORKCLOCK(21);
		src = *(REG16_B20(op));
	}
	else {
		I286_WORKCLOCK(24);
		src = i286_memoryread_w(CALC_EA(op));
	}
	WORD_MUL(res, I286_AX, src)
	I286_AX = (UINT16)res;
	I286_DX = (UINT16)(res >> 16);
}

I286_F6 _imul_ea16(UINT op) {

	SINT16	src;
	SINT32	res;

	if (op >= 0xc0) {
		I286_WORKCLOCK(21);
		src = *(REG16_B20(op));
	}
	else {
		I286_WORKCLOCK(24);
		src = i286_memoryread_w(CALC_EA(op));
	}
	WORD_IMUL(res, I286_AX, src)
	I286_AX = (UINT16)res;
	I286_DX = (UINT16)(res >> 16);
}

I286_F6 _div_ea16(UINT op) {

	UINT32	tmp;
	UINT32	src;
	UINT16	ip;

	ip = I286_IP;
	if (op >= 0xc0) {
		I286_WORKCLOCK(22);
		src = *(REG16_B20(op));
	}
	else {
		I286_WORKCLOCK(25);
		src = i286_memoryread_w(CALC_EA(op));
	}
	tmp = (I286_DX << 16) + I286_AX;
	if ((src) && (tmp < (src << 16))) {
		I286_AX = tmp / src;
		I286_DX = tmp % src;
	}
	else {
		INT_NUM(0, ip - 2);										// 80x86
	}
}

I286_F6 _idiv_ea16(UINT op) {

	SINT32	tmp;
	SINT32	r;
	SINT16	src;
	UINT16	ip;

	ip = I286_IP;
	if (op >= 0xc0) {
		I286_WORKCLOCK(25);
		src = *(REG16_B20(op));
	}
	else {
		I286_WORKCLOCK(28);
		src = i286_memoryread_w(CALC_EA(op));
	}
	tmp = (SINT32)((I286_DX << 16) + I286_AX);
	if ((src) && (tmp != INT_MIN)) {
		r = tmp / src;
		if (!((r + 0x8000) & 0xffff0000)) {
			I286_AX = (SINT16)r;
			I286_DX = tmp % src;
			return;
		}
	}
	INT_NUM(0, ip - 2);											// 80x86
}


const I286OPF6 c_ope0xf6_table[] = {
			_test_ea8_data8,	_test_ea8_data8,
			_not_ea8,			_neg_ea8,
			_mul_ea8,			_imul_ea8,
			_div_ea8,			_idiv_ea8};

const I286OPF6 c_ope0xf7_table[] = {
			_test_ea16_data16,	_test_ea16_data16,
			_not_ea16,			_neg_ea16,
			_mul_ea16,			_imul_ea16,
			_div_ea16,			_idiv_ea16};

