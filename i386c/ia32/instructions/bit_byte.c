/*
 * Copyright (c) 2002-2003 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "bit_byte.h"

#define	BIT_OFFSET16(v)		(2 * (((SINT16)(v)) >> 4))
#define	BIT_INDEX16(v)		((v) & 0xf)
#define	BIT_MAKEBIT16(v)	(1 << BIT_INDEX16(v))

#define	BIT_OFFSET32(v)		(4 * (((SINT32)(v)) >> 5))
#define	BIT_INDEX32(v)		((v) & 0x1f)
#define	BIT_MAKEBIT32(v)	(1 << BIT_INDEX32(v))


/*
 * BT
 */
void
BT_EwGw(void)
{
	UINT32 op, src, dst, madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		dst = *(reg16_b20[op]);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		madr += BIT_OFFSET16(src);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
	}
	CPU_FLAGL &= ~C_FLAG;
	CPU_FLAGL |= (dst >> BIT_INDEX16(src)) & 1;
}

void
BT_EdGd(void)
{
	UINT32 op, src, dst, madr;

	PREPART_EA_REG32(op, src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		dst = *(reg32_b20[op]);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		madr += BIT_OFFSET32(src);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}
	CPU_FLAGL &= ~C_FLAG;
	CPU_FLAGL |= (dst >> BIT_INDEX32(src)) & 1;
}

void CPUCALL
BT_EwIb(UINT32 op)
{
	UINT32 src, dst, madr;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		GET_PCBYTE(src);
		dst = *(reg16_b20[op]);
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
	}
	CPU_FLAGL &= ~C_FLAG;
	CPU_FLAGL |= (dst >> BIT_INDEX16(src)) & 1;
}

void CPUCALL
BT_EdIb(UINT32 op)
{
	UINT32 src, dst, madr;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		GET_PCBYTE(src);
		dst = *(reg32_b20[op]);
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}
	CPU_FLAGL &= ~C_FLAG;
	CPU_FLAGL |= (dst >> BIT_INDEX32(src)) & 1;
}

/*
 * BTS
 */
void
BTS_EwGw(void)
{
	UINT16 *out;
	UINT32 op, src, dst, res, madr;
	UINT16 bit;

	PREPART_EA_REG16(op, src);
	bit = BIT_MAKEBIT16(src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg16_b20[op];
		dst = *out;
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
			res = dst | bit;
			*out = (UINT16)res;
		}
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		madr += BIT_OFFSET16(src);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst | bit;
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, (UINT16)res);
	}
}

void
BTS_EdGd(void)
{
	UINT32 *out;
	UINT32 op, src, dst, res, madr;
	UINT32 bit;

	PREPART_EA_REG32(op, src);
	bit = BIT_MAKEBIT32(src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg32_b20[op];
		dst = *out;
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
			res = dst | bit;
			*out = res;
		}
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		madr += BIT_OFFSET32(src);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst | bit;
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, res);
	}
}

void CPUCALL
BTS_EwIb(UINT32 op)
{
	UINT16 *out;
	UINT32 src, dst, res, madr;
	UINT16 bit;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		GET_PCBYTE(src);
		out = reg16_b20[op];
		dst = *out;
		bit = BIT_MAKEBIT16(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
			res = dst | bit;
			*out = (UINT16)res;
		}
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		bit = BIT_MAKEBIT16(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst | bit;
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, (UINT16)res);
	}
}

void CPUCALL
BTS_EdIb(UINT32 op)
{
	UINT32 *out;
	UINT32 src, dst, res, madr;
	UINT32 bit;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		GET_PCBYTE(src);
		out = reg32_b20[op];
		dst = *out;
		bit = BIT_MAKEBIT32(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
			res = dst | bit;
			*out = res;
		}
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		bit = BIT_MAKEBIT32(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst | bit;
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, res);
	}
}

/*
 * BTR
 */
void
BTR_EwGw(void)
{
	UINT16 *out;
	UINT32 op, src, dst, res, madr;
	UINT16 bit;

	PREPART_EA_REG16(op, src);
	bit = BIT_MAKEBIT16(src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg16_b20[op];
		dst = *out;
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
			res = dst & ~bit;
			*out = (UINT16)res;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		madr += BIT_OFFSET16(src);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst & ~bit;
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, (UINT16)res);
	}
}

void
BTR_EdGd(void)
{
	UINT32 *out;
	UINT32 op, src, dst, res, madr;
	UINT32 bit;

	PREPART_EA_REG32(op, src);
	bit = BIT_MAKEBIT32(src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg32_b20[op];
		dst = *out;
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
			res = dst & ~bit;
			*out = res;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		madr += BIT_OFFSET32(src);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst & ~bit;
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, res);
	}
}

void CPUCALL
BTR_EwIb(UINT32 op)
{
	UINT16 *out;
	UINT32 src, dst, res, madr;
	UINT16 bit;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		GET_PCBYTE(src);
		out = reg16_b20[op];
		dst = *out;
		bit = BIT_MAKEBIT16(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
			res = dst & ~bit;
			*out = (UINT16)res;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		bit = BIT_MAKEBIT16(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst & ~bit;
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, (UINT16)res);
	}
}

void CPUCALL
BTR_EdIb(UINT32 op)
{
	UINT32 *out;
	UINT32 src, dst, res, madr;
	UINT32 bit;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		GET_PCBYTE(src);
		out = reg32_b20[op];
		dst = *out;
		bit = BIT_MAKEBIT32(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
			res = dst & ~bit;
			*out = res;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		bit = BIT_MAKEBIT32(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst & ~bit;
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, res);
	}
}

/*
 * BTC
 */
void
BTC_EwGw(void)
{
	UINT16 *out;
	UINT32 op, src, dst, res, madr;
	UINT16 bit;

	PREPART_EA_REG16(op, src);
	bit = BIT_MAKEBIT16(src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg16_b20[op];
		dst = *out;
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst ^ bit;
		*out = (UINT16)res;
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		madr += BIT_OFFSET16(src);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst ^ bit;
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, (UINT16)res);
	}
}

void
BTC_EdGd(void)
{
	UINT32 *out;
	UINT32 op, src, dst, res, madr;
	UINT32 bit;

	PREPART_EA_REG32(op, src);
	bit = BIT_MAKEBIT32(src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg32_b20[op];
		dst = *out;
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst ^ bit;
		*out = res;
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		madr += BIT_OFFSET32(src);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst ^ bit;
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, res);
	}
}

void CPUCALL
BTC_EwIb(UINT32 op)
{
	UINT16 *out;
	UINT32 src, dst, res, madr;
	UINT16 bit;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		GET_PCBYTE(src);
		out = reg16_b20[op];
		dst = *out;
		bit = BIT_MAKEBIT16(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst ^ bit;
		*out = (UINT16)res;
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		bit = BIT_MAKEBIT16(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst ^ bit;
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, (UINT16)res);
	}
}

void CPUCALL
BTC_EdIb(UINT32 op)
{
	UINT32 *out;
	UINT32 src, dst, res, madr;
	UINT32 bit;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		GET_PCBYTE(src);
		out = reg32_b20[op];
		dst = *out;
		bit = BIT_MAKEBIT32(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst ^ bit;
		*out = res;
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		GET_PCBYTE(src);
		dst = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		bit = BIT_MAKEBIT32(src);
		if (dst & bit) {
			CPU_FLAGL |= C_FLAG;
		} else {
			CPU_FLAGL &= ~C_FLAG;
		}
		res = dst ^ bit;
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr, res);
	}
}

/*
 * BSF
 */
void
BSF_GwEw(void)
{
	UINT16 *out;
	UINT32 op, src;
	int bit;

	PREPART_REG16_EA(op, src, out, 2, 7);
	if (src == 0) {
		CPU_FLAGL |= Z_FLAG;
		/* dest reg is undefined */
	} else {
		CPU_FLAGL &= ~Z_FLAG;
		for (bit = 0; bit < 15; bit++) {
			if (src & (1 << bit))
				break;
		}
		*out = (UINT16)bit;
	}
}

void
BSF_GdEd(void)
{
	UINT32 *out;
	UINT32 op, src;
	int bit;

	PREPART_REG32_EA(op, src, out, 2, 7);
	if (src == 0) {
		CPU_FLAGL |= Z_FLAG;
		/* dest reg is undefined */
	} else {
		CPU_FLAGL &= ~Z_FLAG;
		for (bit = 0; bit < 31; bit++) {
			if (src & (1 << bit))
				break;
		}
		*out = (UINT32)bit;
	}
}

/*
 * BSR
 */
void
BSR_GwEw(void)
{
	UINT16 *out;
	UINT32 op, src;
	int bit;

	PREPART_REG16_EA(op, src, out, 2, 7);
	if (src == 0) {
		CPU_FLAGL |= Z_FLAG;
		/* dest reg is undefined */
	} else {
		CPU_FLAGL &= ~Z_FLAG;
		for (bit = 15; bit > 0; bit--) {
			if (src & (1 << bit))
				break;
		}
		*out = (UINT16)bit;
	}
}

void
BSR_GdEd(void)
{
	UINT32 *out;
	UINT32 op, src;
	int bit;

	PREPART_REG32_EA(op, src, out, 2, 7);
	if (src == 0) {
		CPU_FLAGL |= Z_FLAG;
		/* dest reg is undefined */
	} else {
		CPU_FLAGL &= ~Z_FLAG;
		for (bit = 31; bit > 0; bit--) {
			if (src & (1 << bit))
				break;
		}
		*out = (UINT32)bit;
	}
}

/*
 * SETcc
 */
void
SETO_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_O?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETNO_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_NO?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETC_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_C?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETNC_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_NC?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETZ_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_Z?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETNZ_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_NZ?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETA_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_A?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETNA_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_NA?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETS_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_S?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETNS_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_NS?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETP_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_P?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETNP_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_NP?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETL_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_L?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETNL_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_NL?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETLE_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_LE?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

void
SETNLE_Eb(void)
{
	UINT32 op, madr;
	UINT8 v = CC_NLE?1:0;

	GET_PCBYTE(op);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		*(reg8_b20[op]) = v;
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite(CPU_INST_SEGREG_INDEX, madr, v);
	}
}

/*
 * TEST
 */
void
TEST_EbGb(void)
{
	UINT32 op, src, tmp, madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		tmp = *(reg8_b20[op]);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		tmp = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, madr);
	}
	BYTE_AND(tmp, src);
}

void
TEST_EwGw(void)
{
	UINT32 op, src, tmp, madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		tmp = *(reg16_b20[op]);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		tmp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
	}
	WORD_AND(tmp, src);
}

void
TEST_EdGd(void)
{
	UINT32 op, src, tmp, madr;

	PREPART_EA_REG32(op, src);
	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		tmp = *(reg32_b20[op]);
	} else {
		CPU_WORKCLOCK(7);
		madr = calc_ea_dst(op);
		tmp = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}
	DWORD_AND(tmp, src);
}

void
TEST_ALIb(void)
{
	UINT32 src, tmp;

	CPU_WORKCLOCK(3);
	tmp = CPU_AL;
	GET_PCBYTE(src);
	BYTE_AND(tmp, src);
}

void
TEST_AXIw(void)
{
	UINT32 src, tmp;

	CPU_WORKCLOCK(3);
	tmp = CPU_AX;
	GET_PCWORD(src);
	WORD_AND(tmp, src);
}

void
TEST_EAXId(void)
{
	UINT32 src, tmp;

	CPU_WORKCLOCK(3);
	tmp = CPU_EAX;
	GET_PCDWORD(src);
	DWORD_AND(tmp, src);
}

void CPUCALL
TEST_EbIb(UINT32 op)
{
	UINT32 src, tmp, madr;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		tmp = *(reg8_b20[op]);
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		tmp = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, madr);
	}
	GET_PCBYTE(src);
	BYTE_AND(tmp, src);
}

void CPUCALL
TEST_EwIw(UINT32 op)
{
	UINT32 src, tmp, madr;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		tmp = *(reg16_b20[op]);
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		tmp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
	}
	GET_PCWORD(src);
	WORD_AND(tmp, src);
}

void CPUCALL
TEST_EdId(UINT32 op)
{
	UINT32 src, tmp, madr;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		tmp = *(reg32_b20[op]);
	} else {
		CPU_WORKCLOCK(6);
		madr = calc_ea_dst(op);
		tmp = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}
	GET_PCDWORD(src);
	DWORD_AND(tmp, src);
}
