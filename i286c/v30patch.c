#include	"compiler.h"
#include	"cpucore.h"
#include	"i286c.h"
#include	"v30patch.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios/bios.h"
#include	"dmav30.h"
#include	"i286c.mcr"
#if defined(ENABLE_TRAP)
#include "trap/steptrap.h"
#endif


// victory30 patch

#define	MAX_PREFIX		8

#define	NEXT_OPCODE												\
		if (I286_REMCLOCK < 1) {								\
			I286_BASECLOCK += (1 - I286_REMCLOCK);				\
			I286_REMCLOCK = 1;									\
		}

#define REAL_V30FLAG	(UINT16)((I286_FLAG & 0x7ff) + \
											(I286_OV?O_FLAG:0) + 0xf000)

typedef struct {
	UINT	opnum;
	I286OP	v30opcode;
} V30PATCH;

static	I286OP		v30op[256];
static	I286OP		v30op_repne[256];
static	I286OP		v30op_repe[256];
static	I286OPF6	v30ope0xf6_table[8];
static	I286OPF6	v30ope0xf7_table[8];


static const UINT8 rotatebase16[256] =
				{0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15};

static const UINT8 rotatebase09[256] =
				{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6,
				 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4,
				 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2,
				 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9,
				 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7,
				 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5,
				 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3,
				 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1,
				 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8,
				 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6,
				 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4,
				 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2,
				 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9,
				 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7,
				 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5,
				 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3};

static const UINT8 rotatebase17[256] =
				{0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
				15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,
				14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
				13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
				12,13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,
				11,12,13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,
				10,11,12,13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8,
				 9,10,11,12,13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7,
				 8, 9,10,11,12,13,14,15,16,17, 1, 2, 3, 4, 5, 6,
				 7, 8, 9,10,11,12,13,14,15,16,17, 1, 2, 3, 4, 5,
				 6, 7, 8, 9,10,11,12,13,14,15,16,17, 1, 2, 3, 4,
				 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17, 1, 2, 3,
				 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17, 1, 2,
				 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17, 1,
				 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17};


I286FN v30_reserved(void) {

	I286_WORKCLOCK(2);
}

I286FN v30segprefix_es(void) {				// 26: es:

	SS_FIX = ES_BASE;
	DS_FIX = ES_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30segprefix_cs(void) {				// 2e: cs:

	SS_FIX = CS_BASE;
	DS_FIX = CS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30segprefix_ss(void) {				// 36: ss:

	SS_FIX = SS_BASE;
	DS_FIX = SS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30segprefix_ds(void) {				// 3e: ds:

	SS_FIX = DS_BASE;
	DS_FIX = DS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30push_sp(void) REGPUSH(I286_SP, 3)	// 54: push sp
I286FN v30pop_sp(void) REGPOP(I286_SP, 5)	// 5C: pop sp

I286FN v30mov_seg_ea(void) {				// 8E:	mov		segrem, EA

	UINT	op;
	UINT	tmp;
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
	switch(op & 0x18) {
		case 0x00:			// es
			I286_ES = (UINT16)tmp;
			ES_BASE = tmp << 4;
			break;

		case 0x08:			// cs
			I286_CS = (UINT16)tmp;
			CS_BASE = tmp << 4;
			break;

		case 0x10:			// ss
			I286_SS = (UINT16)tmp;
			SS_BASE = tmp << 4;
			SS_FIX = SS_BASE;
			NEXT_OPCODE
			break;

		case 0x18:			// ds
			I286_DS = (UINT16)tmp;
			DS_BASE = tmp << 4;
			DS_FIX = DS_BASE;
			break;
	}
}

I286FN v30_pushf(void) {					// 9C:	pushf

	REGPUSH(REAL_V30FLAG, 3)
}

I286FN v30_popf(void) {						// 9D:	popf

	I286_WORKCLOCK(5);
	REGPOP0(I286_FLAG)
	I286_FLAG |= 0xf000;
	I286_OV = I286_FLAG & O_FLAG;
	I286_FLAG &= (0xfff ^ O_FLAG);
	I286_TRAP = ((I286_FLAG & 0x300) == 0x300);
	I286IRQCHECKTERM
}

I286FN v30shift_ea8_data8(void) {			// C0:	shift	EA8, DATA8

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
			GET_PCBYTE(cl)
			I286_WORKCLOCK(cl);
			if (!(op & 0x20)) {					// rotate
				if (!(op & 0x10)) {
					cl = rotatebase16[cl];
				}
				else {							// rotate with carry
					cl = rotatebase09[cl];
				}
			}
			else {
				cl = max(cl, 9);
			}
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	GET_PCBYTE(cl)
	I286_WORKCLOCK(cl);
	if (!(op & 0x20)) {					// rotate
		if (!(op & 0x10)) {
			cl = rotatebase16[cl];
		}
		else {							// rotate with carry
			cl = rotatebase09[cl];
		}
	}
	else {
		cl = max(cl, 9);
	}
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

I286FN v30shift_ea16_data8(void) {			// C1:	shift	EA16, DATA8

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
			GET_PCBYTE(cl);
			I286_WORKCLOCK(cl);
			if (!(op & 0x20)) {					// rotate
				if (!(op & 0x10)) {
					cl = rotatebase16[cl];
				}
				else {							// with carry
					cl = rotatebase17[cl];
				}
			}
			else {								// shift
				cl = max(cl, 17);
			}
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCBYTE(cl);
	I286_WORKCLOCK(cl);
	if (!(op & 0x20)) {					// rotate
		if (!(op & 0x10)) {
			cl = rotatebase16[cl];
		}
		else {							// with carry
			cl = rotatebase17[cl];
		}
	}
	else {								// shift
		cl = max(cl, 17);
	}
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

I286FN v30shift_ea8_cl(void) {				// D2:	shift EA8, cl

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
			if (!(op & 0x20)) {					// rotate
				if (!(op & 0x10)) {
					cl = rotatebase16[cl];
				}
				else {							// rotate with carry
					cl = rotatebase09[cl];
				}
			}
			else {
				cl = max(cl, 9);
			}
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	cl = I286_CL;
	I286_WORKCLOCK(cl);
	if (!(op & 0x20)) {					// rotate
		if (!(op & 0x10)) {
			cl = rotatebase16[cl];
		}
		else {							// rotate with carry
			cl = rotatebase09[cl];
		}
	}
	else {
		cl = max(cl, 9);
	}
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

I286FN v30shift_ea16_cl(void) {				// D3:	shift EA16, cl

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
			if (!(op & 0x20)) {					// rotate
				if (!(op & 0x10)) {
					cl = rotatebase16[cl];
				}
				else {							// with carry
					cl = rotatebase17[cl];
				}
			}
			else {								// shift
				cl = max(cl, 17);
			}
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	cl = I286_CL;
	I286_WORKCLOCK(cl);
	if (!(op & 0x20)) {					// rotate
		if (!(op & 0x10)) {
			cl = rotatebase16[cl];
		}
		else {							// with carry
			cl = rotatebase17[cl];
		}
	}
	else {								// shift
		cl = max(cl, 17);
	}
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

I286FN v30_aam(void) {						// D4:	AAM

	UINT8	al;

	I286_WORKCLOCK(16);
	I286_IP++;								// is 10
	al = I286_AL;
	I286_AH = al / 10;
	I286_AL = al % 10;
	I286_FLAGL &= ~(S_FLAG | Z_FLAG | P_FLAG);
	I286_FLAGL |= WORDSZPF(I286_AX);
}

I286FN v30_aad(void) {						// D5:	AAD

	I286_WORKCLOCK(14);
	I286_IP++;								// is 10
	I286_AL += (UINT8)(I286_AH * 10);
	I286_AH = 0;
	I286_FLAGL &= ~(S_FLAG | Z_FLAG | P_FLAG);
	I286_FLAGL |= BYTESZPF(I286_AL);
}

I286FN v30_xlat(void) {						// D6:	xlat

	I286_WORKCLOCK(5);
	I286_AL = i286_memoryread(LOW16(I286_AL + I286_BX) + DS_FIX);
}

I286FN v30_repne(void) {					// F2:	repne

	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30_repe(void) {						// F3:	repe

	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286_F6 v30_div_ea8(UINT op) {

	UINT16	tmp;
	UINT8	src;

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
		INT_NUM(0, I286_IP);									// V30
	}
}

I286_F6 v30_idiv_ea8(UINT op) {

	SINT16	tmp;
	SINT16	r;
	SINT8	src;

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
	INT_NUM(0, I286_IP);										// V30
}

I286FN v30_ope0xf6(void) {					// F6:	

	UINT	op;

	GET_PCBYTE(op);
	v30ope0xf6_table[(op >> 3) & 7](op);
}

I286_F6 v30_div_ea16(UINT op) {

	UINT32	tmp;
	UINT32	src;

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
		INT_NUM(0, I286_IP);									// V30
	}
}

I286_F6 v30_idiv_ea16(UINT op) {

	SINT32	tmp;
	SINT32	r;
	SINT16	src;

	if (op >= 0xc0) {
		I286_WORKCLOCK(25);
		src = *(REG16_B20(op));
	}
	else {
		I286_WORKCLOCK(28);
		src = i286_memoryread_w(CALC_EA(op));
	}
	tmp = (SINT32)((I286_DX << 16) + I286_AX);
	if (src) {
		r = tmp / src;
		if (!((r + 0x8000) & 0xffff0000)) {
			I286_AX = (SINT16)r;
			I286_DX = tmp % src;
			return;
		}
	}
	INT_NUM(0, I286_IP);										// V30
}

I286FN v30_ope0xf7(void) {					// F7:	

	UINT	op;

	GET_PCBYTE(op);
	v30ope0xf7_table[(op >> 3) & 7](op);
}

static const V30PATCH v30patch_op[] = {
			{0x26, v30segprefix_es},		// 26:	es:
			{0x2e, v30segprefix_cs},		// 2E:	cs:
			{0x36, v30segprefix_ss},		// 36:	ss:
			{0x3e, v30segprefix_ds},		// 3E:	ds:
			{0x54, v30push_sp},				// 54:	push	sp
			{0x5c, v30pop_sp},				// 5C:	pop		sp
			{0x63, v30_reserved},			// 63:	reserved
			{0x64, v30_reserved},			// 64:	reserved
			{0x65, v30_reserved},			// 65:	reserved
			{0x66, v30_reserved},			// 66:	reserved
			{0x67, v30_reserved},			// 67:	reserved
			{0x8e, v30mov_seg_ea},			// 8E:	mov		segrem, EA
			{0x9c, v30_pushf},				// 9C:	pushf
			{0x9d, v30_popf},				// 9D:	popf
			{0xc0, v30shift_ea8_data8},		// C0:	shift	EA8, DATA8
			{0xc1, v30shift_ea16_data8},	// C1:	shift	EA16, DATA8
			{0xd2, v30shift_ea8_cl},		// D2:	shift EA8, cl
			{0xd3, v30shift_ea16_cl},		// D3:	shift EA16, cl
			{0xd4, v30_aam},				// D4:	AAM
			{0xd5, v30_aad},				// D5:	AAD
			{0xd6, v30_xlat},				// D6:	xlat (8086/V30)
			{0xf2, v30_repne},				// F2:	repne
			{0xf3, v30_repe},				// F3:	repe
			{0xf6, v30_ope0xf6},			// F6:
			{0xf7, v30_ope0xf7}};			// F7:


// ----------------------------------------------------------------- repe

I286FN v30repe_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30repe_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30repe_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30repe_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

static const V30PATCH v30patch_repe[] = {
			{0x26, v30repe_segprefix_es},	// 26:	repe es:
			{0x2e, v30repe_segprefix_cs},	// 2E:	repe cs:
			{0x36, v30repe_segprefix_ss},	// 36:	repe ss:
			{0x3e, v30repe_segprefix_ds},	// 3E:	repe ds:
			{0x54, v30push_sp},				// 54:	push	sp
			{0x5c, v30pop_sp},				// 5C:	pop		sp
			{0x63, v30_reserved},			// 63:	reserved
			{0x64, v30_reserved},			// 64:	reserved
			{0x65, v30_reserved},			// 65:	reserved
			{0x66, v30_reserved},			// 66:	reserved
			{0x67, v30_reserved},			// 67:	reserved
			{0x8e, v30mov_seg_ea},			// 8E:	mov		segrem, EA
			{0x9c, v30_pushf},				// 9C:	pushf
			{0x9d, v30_popf},				// 9D:	popf
			{0xc0, v30shift_ea8_data8},		// C0:	shift	EA8, DATA8
			{0xc1, v30shift_ea16_data8},	// C1:	shift	EA16, DATA8
			{0xd2, v30shift_ea8_cl},		// D2:	shift EA8, cl
			{0xd3, v30shift_ea16_cl},		// D3:	shift EA16, cl
			{0xd4, v30_aam},				// D4:	AAM
			{0xd5, v30_aad},				// D5:	AAD
			{0xd6, v30_xlat},				// D6:	xlat (8086/V30)
			{0xf2, v30_repne},				// F2:	repne
			{0xf3, v30_repe},				// F3:	repe
			{0xf6, v30_ope0xf6},			// F6:
			{0xf7, v30_ope0xf7}};			// F7:


// ----------------------------------------------------------------- repne

I286FN v30repne_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30repne_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30repne_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

I286FN v30repne_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	I286_PREFIX++;
	if (I286_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		REMOVE_PREFIX
		I286_PREFIX = 0;
	}
	else {
		INT_NUM(6, I286_IP);
	}
}

static const V30PATCH v30patch_repne[] = {
			{0x26, v30repne_segprefix_es},	// 26:	repne es:
			{0x2e, v30repne_segprefix_cs},	// 2E:	repne cs:
			{0x36, v30repne_segprefix_ss},	// 36:	repne ss:
			{0x3e, v30repne_segprefix_ds},	// 3E:	repne ds:
			{0x54, v30push_sp},				// 54:	push	sp
			{0x5c, v30pop_sp},				// 5C:	pop		sp
			{0x63, v30_reserved},			// 63:	reserved
			{0x64, v30_reserved},			// 64:	reserved
			{0x65, v30_reserved},			// 65:	reserved
			{0x66, v30_reserved},			// 66:	reserved
			{0x67, v30_reserved},			// 67:	reserved
			{0x8e, v30mov_seg_ea},			// 8E:	mov		segrem, EA
			{0x9c, v30_pushf},				// 9C:	pushf
			{0x9d, v30_popf},				// 9D:	popf
			{0xc0, v30shift_ea8_data8},		// C0:	shift	EA8, DATA8
			{0xc1, v30shift_ea16_data8},	// C1:	shift	EA16, DATA8
			{0xd2, v30shift_ea8_cl},		// D2:	shift EA8, cl
			{0xd3, v30shift_ea16_cl},		// D3:	shift EA16, cl
			{0xd4, v30_aam},				// D4:	AAM
			{0xd5, v30_aad},				// D5:	AAD
			{0xd6, v30_xlat},				// D6:	xlat (8086/V30)
			{0xf2, v30_repne},				// F2:	repne
			{0xf3, v30_repe},				// F3:	repe
			{0xf6, v30_ope0xf6},			// F6:
			{0xf7, v30_ope0xf7}};			// F7:


// ---------------------------------------------------------------------------

static void v30patching(I286OP *op, const V30PATCH *patch, int cnt) {

	do {
		op[patch->opnum] = patch->v30opcode;
		patch++;
	} while(--cnt);
}

#define	V30PATCHING(a, b)	v30patching(a, b, sizeof(b)/sizeof(V30PATCH))

void v30cinit(void) {

	CopyMemory(v30op, i286op, sizeof(v30op));
	V30PATCHING(v30op, v30patch_op);
	CopyMemory(v30op_repne, i286op_repne, sizeof(v30op_repne));
	V30PATCHING(v30op_repne, v30patch_repne);
	CopyMemory(v30op_repe, i286op_repe, sizeof(v30op_repe));
	V30PATCHING(v30op_repe, v30patch_repe);
	CopyMemory(v30ope0xf6_table, c_ope0xf6_table, sizeof(v30ope0xf6_table));
	v30ope0xf6_table[6] = v30_div_ea8;
	v30ope0xf6_table[7] = v30_idiv_ea8;
	CopyMemory(v30ope0xf7_table, c_ope0xf7_table, sizeof(v30ope0xf7_table));
	v30ope0xf7_table[6] = v30_div_ea16;
	v30ope0xf7_table[7] = v30_idiv_ea16;
}

void v30c(void) {

	UINT	opcode;

	if (I286_TRAP) {
		do {
#if defined(ENABLE_TRAP)
			steptrap(CPU_CS, CPU_IP);
#endif
			GET_PCBYTE(opcode);
			v30op[opcode]();
			if (I286_TRAP) {
				i286c_interrupt(1);
			}
			dmav30();
		} while(I286_REMCLOCK > 0);
	}
	else if (dmac.working) {
		do {
#if defined(ENABLE_TRAP)
			steptrap(CPU_CS, CPU_IP);
#endif
			GET_PCBYTE(opcode);
			v30op[opcode]();
			dmav30();
		} while(I286_REMCLOCK > 0);
	}
	else {
		do {
#if defined(ENABLE_TRAP)
			steptrap(CPU_CS, CPU_IP);
#endif
			GET_PCBYTE(opcode);
			v30op[opcode]();
		} while(I286_REMCLOCK > 0);
	}
}

void v30c_step(void) {

	UINT	opcode;

	I286_OV = I286_FLAG & O_FLAG;
	I286_FLAG &= ~(O_FLAG);

	GET_PCBYTE(opcode);
	v30op[opcode]();

	I286_FLAG &= ~(O_FLAG);
	if (I286_OV) {
		I286_FLAG |= (O_FLAG);
	}
	dmav30();
}

