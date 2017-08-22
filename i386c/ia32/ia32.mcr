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

#ifndef	IA32_CPU_IA32_MCR__
#define	IA32_CPU_IA32_MCR__

/*
 * misc
 */
#define	__CBW(src)	((UINT16)((SINT8)(src)))
#define	__CBD(src)	((UINT32)((SINT8)(src)))
#define	__CWDE(src)	((SINT16)(src))

#ifndef	PTR_TO_UINT32
#define	PTR_TO_UINT32(p)	((UINT32)((unsigned long)(p)))
#endif
#ifndef	UINT32_TO_PTR
#define	UINT32_TO_PTR(v)	((void *)((unsigned long)(UINT32)(v)))
#endif

#define	SWAP_BYTE(p, q) \
do { \
	UINT8 __tmp = (p); \
	(p) = (q); \
	(q) = __tmp; \
} while (/*CONSTCOND*/ 0)

#define	SWAP_WORD(p, q) \
do { \
	UINT16 __tmp = (p); \
	(p) = (q); \
	(q) = __tmp; \
} while (/*CONSTCOND*/ 0)

#define	SWAP_DWORD(p, q) \
do { \
	UINT32 __tmp = (p); \
	(p) = (q); \
	(q) = __tmp; \
} while (/*CONSTCOND*/ 0)


/*
 * clock
 */
#define	CPU_WORKCLOCK(clock) \
do { \
	CPU_REMCLOCK -= (clock); \
} while (/*CONSTCOND*/ 0)

#define	CPU_HALT() \
do { \
	CPU_REMCLOCK = -1; \
} while (/*CONSTCOND*/ 0)

#define	IRQCHECKTERM() \
do { \
	if (CPU_REMCLOCK > 0) { \
		CPU_BASECLOCK -= CPU_REMCLOCK; \
		CPU_REMCLOCK = 0; \
	} \
} while (/*CONSTCOND*/ 0)


/*
 * instruction pointer
 */
/* コードフェッチに使用するので、OpSize の影響を受けてはいけない */
#define	_ADD_EIP(v) \
do { \
	UINT32 __tmp_ip = CPU_EIP + (v); \
	if (!CPU_STATSAVE.cpu_inst_default.op_32) { \
		__tmp_ip &= 0xffff; \
	} \
	CPU_EIP = __tmp_ip; \
} while (/*CONSTCOND*/ 0)

#define	GET_PCBYTE(v) \
do { \
	(v) = cpu_codefetch(CPU_EIP); \
	_ADD_EIP(1); \
} while (/*CONSTCOND*/ 0)

#define	GET_PCBYTES(v) \
do { \
	(v) = __CBW(cpu_codefetch(CPU_EIP)); \
	_ADD_EIP(1); \
} while (/*CONSTCOND*/ 0)

#define	GET_PCBYTESD(v) \
do { \
	(v) = __CBD(cpu_codefetch(CPU_EIP)); \
	_ADD_EIP(1); \
} while (/*CONSTCOND*/ 0)

#define	GET_PCWORD(v) \
do { \
	(v) = cpu_codefetch_w(CPU_EIP); \
	_ADD_EIP(2); \
} while (/*CONSTCOND*/ 0)

#define	GET_PCWORDS(v) \
do { \
	(v) = __CWDE(cpu_codefetch_w(CPU_EIP)); \
	_ADD_EIP(2); \
} while (/*CONSTCOND*/ 0)

#define	GET_PCDWORD(v) \
do { \
	(v) = cpu_codefetch_d(CPU_EIP); \
	_ADD_EIP(4); \
} while (/*CONSTCOND*/ 0)

#define	PREPART_EA_REG8(b, d_s) \
do { \
	GET_PCBYTE((b)); \
	(d_s) = *(reg8_b53[(b)]); \
} while (/*CONSTCOND*/ 0)

#define	PREPART_EA_REG8P(b, d_s) \
do { \
	GET_PCBYTE((b)); \
	(d_s) = reg8_b53[(b)]; \
} while (/*CONSTCOND*/ 0)

#define	PREPART_EA_REG16(b, d_s) \
do { \
	GET_PCBYTE((b)); \
	(d_s) = *(reg16_b53[(b)]); \
} while (/*CONSTCOND*/ 0)

#define	PREPART_EA_REG16P(b, d_s) \
do { \
	GET_PCBYTE((b)); \
	(d_s) = reg16_b53[(b)]; \
} while (/*CONSTCOND*/ 0)

#define	PREPART_EA_REG32(b, d_s) \
do { \
	GET_PCBYTE((b)); \
	(d_s) = *(reg32_b53[(b)]); \
} while (/*CONSTCOND*/ 0)

#define	PREPART_EA_REG32P(b, d_s) \
do { \
	GET_PCBYTE((b)); \
	(d_s) = reg32_b53[(b)]; \
} while (/*CONSTCOND*/ 0)

#define	PREPART_REG8_EA(b, s, d, regclk, memclk) \
do { \
	GET_PCBYTE((b)); \
	if ((b) >= 0xc0) { \
		CPU_WORKCLOCK(regclk); \
		(s) = *(reg8_b20[(b)]); \
	} else { \
		UINT32 __t; \
		CPU_WORKCLOCK(memclk); \
		__t = calc_ea_dst((b)); \
		(s) = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, __t); \
	} \
	(d) = reg8_b53[(b)]; \
} while (/*CONSTCOND*/ 0)

#define	PREPART_REG16_EA(b, s, d, regclk, memclk) \
do { \
	GET_PCBYTE((b)); \
	if ((b) >= 0xc0) { \
		CPU_WORKCLOCK(regclk); \
		(s) = *(reg16_b20[(b)]); \
	} else { \
		UINT32 __t; \
		CPU_WORKCLOCK(memclk); \
		__t = calc_ea_dst((b)); \
		(s) = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, __t); \
	} \
	(d) = reg16_b53[(b)]; \
} while (/*CONSTCOND*/ 0)

#define	PREPART_REG16_EA8(b, s, d, regclk, memclk) \
do { \
	GET_PCBYTE((b)); \
	if ((b) >= 0xc0) { \
		CPU_WORKCLOCK(regclk); \
		(s) = *(reg8_b20[(b)]); \
	} else { \
		UINT32 __t; \
		CPU_WORKCLOCK(memclk); \
		__t = calc_ea_dst((b)); \
		(s) = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, __t); \
	} \
	(d) = reg16_b53[(b)]; \
} while (/*CONSTCOND*/ 0)

#define	PREPART_REG32_EA(b, s, d, regclk, memclk) \
do { \
	GET_PCBYTE((b)); \
	if ((b) >= 0xc0) { \
		CPU_WORKCLOCK(regclk); \
		(s) = *(reg32_b20[(b)]); \
	} else { \
		UINT32 __t; \
		CPU_WORKCLOCK(memclk); \
		__t = calc_ea_dst((b)); \
		(s) = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, __t); \
	} \
	(d) = reg32_b53[(b)]; \
} while (/*CONSTCOND*/ 0)

#define	PREPART_REG32_EA8(b, s, d, regclk, memclk) \
do { \
	GET_PCBYTE((b)); \
	if ((b) >= 0xc0) { \
		CPU_WORKCLOCK(regclk); \
		(s) = *(reg8_b20[(b)]); \
	} else { \
		UINT32 __t; \
		CPU_WORKCLOCK(memclk); \
		__t = calc_ea_dst((b)); \
		(s) = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, __t); \
	} \
	(d) = reg32_b53[(b)]; \
} while (/*CONSTCOND*/ 0)

#define	PREPART_REG32_EA16(b, s, d, regclk, memclk) \
do { \
	GET_PCBYTE((b)); \
	if ((b) >= 0xc0) { \
		CPU_WORKCLOCK(regclk); \
		(s) = *(reg16_b20[(b)]); \
	} else { \
		UINT32 __t; \
		CPU_WORKCLOCK(memclk); \
		__t = calc_ea_dst((b)); \
		(s) = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, __t); \
	} \
	(d) = reg32_b53[(b)]; \
} while (/*CONSTCOND*/ 0)


/*
 * arith
 */
#define	_ADD_BYTE(r, d, s) \
do { \
	(r) = (s) + (d); \
	CPU_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x80; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpcflag[(r) & 0x1ff]; \
} while (/*CONSTCOND*/ 0)

#define	_ADD_WORD(r, d, s) \
do { \
	(r) = (s) + (d); \
	CPU_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x8000; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	if ((r) & 0xffff0000) { \
		(r) &= 0x0000ffff; \
		CPU_FLAGL |= C_FLAG; \
	} \
	CPU_FLAGL |= szpflag_w[(UINT16)(r)]; \
} while (/*CONSTCOND*/ 0)

#define	_ADD_DWORD(r, d, s) \
do { \
	(r) = (s) + (d); \
	CPU_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x80000000; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	if ((r) < (s)) { \
		CPU_FLAGL |= C_FLAG; \
	} \
	if ((r) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if ((r) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
	CPU_FLAGL |= szpcflag[(UINT8)(r)] & P_FLAG; \
} while (/*CONSTCOND*/ 0)

#define	_OR_BYTE(d, s) \
do { \
	(d) |= (s); \
	CPU_OV = 0; \
	CPU_FLAGL = szpcflag[(UINT8)(d)]; \
} while (/*CONSTCOND*/ 0)

#define	_OR_WORD(d, s) \
do { \
	(d) |= (s); \
	CPU_OV = 0; \
	CPU_FLAGL = szpflag_w[(UINT16)(d)]; \
} while (/*CONSTCOND*/ 0)

#define	_OR_DWORD(d, s) \
do { \
	(d) |= (s); \
	CPU_OV = 0; \
	CPU_FLAGL = (UINT8)(szpcflag[(UINT8)(d)] & P_FLAG); \
	if ((d) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if ((d) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

/* flag no check */
#define	_ADC_BYTE(r, d, s) \
do { \
	(r) = (CPU_FLAGL & C_FLAG) + (s) + (d); \
	CPU_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x80; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpcflag[(r) & 0x1ff]; \
} while (/*CONSTCOND*/ 0)

#define	_ADC_WORD(r, d, s) \
do { \
	(r) = (CPU_FLAGL & C_FLAG) + (s) + (d); \
	CPU_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x8000; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	if ((r) & 0xffff0000) { \
		(r) &= 0x0000ffff; \
		CPU_FLAGL |= C_FLAG; \
	} \
	CPU_FLAGL |= szpflag_w[(UINT16)(r)]; \
} while (/*CONSTCOND*/ 0)

#define	_ADC_DWORD(r, d, s) \
do { \
	UINT32 __c = (CPU_FLAGL & C_FLAG); \
	(r) = (s) + (d) + __c; \
	CPU_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x80000000; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	if ((!__c && (r) < (s)) || (__c && (r) <= (s))) { \
		CPU_FLAGL |= C_FLAG; \
	} \
	if ((r) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if ((r) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
	CPU_FLAGL |= szpcflag[(UINT8)(r)] & P_FLAG; \
} while (/*CONSTCOND*/ 0)

/* flag no check */
#define	_BYTE_SBB(r, d, s) \
do { \
	(r) = (d) - (s) - (CPU_FLAGL & C_FLAG); \
	CPU_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x80; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpcflag[(r) & 0x1ff]; \
} while (/*CONSTCOND*/ 0)

#define	_WORD_SBB(r, d, s) \
do { \
	(r) = (d) - (s) - (CPU_FLAGL & C_FLAG); \
	CPU_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x8000; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	if ((r) & 0xffff0000) { \
		(r) &= 0x0000ffff; \
		CPU_FLAGL |= C_FLAG; \
	} \
	CPU_FLAGL |= szpflag_w[(UINT16)(r)]; \
} while (/*CONSTCOND*/ 0)

#define	_DWORD_SBB(r, d, s) \
do { \
	UINT32 __c = (CPU_FLAGL & C_FLAG); \
	(r) = (d) - (s) - __c; \
	CPU_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x80000000; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	if ((!__c && (d) < (s)) || (__c && (d) <= (s))) { \
		CPU_FLAGL |= C_FLAG; \
	} \
	if ((r) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if ((r) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
	CPU_FLAGL |= szpcflag[(UINT8)(r)] & P_FLAG; \
} while (/*CONSTCOND*/ 0)

#define	_AND_BYTE(d, s) \
do { \
	(d) &= (s); \
	CPU_OV = 0; \
	CPU_FLAGL = szpcflag[(UINT8)(d)]; \
} while (/*CONSTCOND*/ 0)

#define	_AND_WORD(d, s) \
do { \
	(d) &= (s); \
	CPU_OV = 0; \
	CPU_FLAGL = szpflag_w[(UINT16)(d)]; \
} while (/*CONSTCOND*/ 0)

#define	_AND_DWORD(d, s) \
do { \
	(d) &= (s); \
	CPU_OV = 0; \
	CPU_FLAGL = (UINT8)(szpcflag[(UINT8)(d)] & P_FLAG); \
	if ((d) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if ((d) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

#define	_BYTE_SUB(r, d, s) \
do { \
	(r) = (d) - (s); \
	CPU_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x80; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpcflag[(r) & 0x1ff]; \
} while (/*CONSTCOND*/ 0)

#define	_WORD_SUB(r, d, s) \
do { \
	(r) = (d) - (s); \
	CPU_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x8000; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	if ((r) & 0xffff0000) { \
		(r) &= 0x0000ffff; \
		CPU_FLAGL |= C_FLAG; \
	} \
	CPU_FLAGL |= szpflag_w[(UINT16)(r)]; \
} while (/*CONSTCOND*/ 0)

#define	_DWORD_SUB(r, d, s) \
do { \
	(r) = (d) - (s); \
	CPU_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x80000000; \
	CPU_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG); \
	if ((d) < (s)) { \
		CPU_FLAGL |= C_FLAG; \
	} \
	if ((r) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if ((r) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
	CPU_FLAGL |= szpcflag[(UINT8)(r)] & P_FLAG; \
} while (/*CONSTCOND*/ 0)

#define	_BYTE_XOR(d, s) \
do { \
	(d) ^= s; \
	CPU_OV = 0; \
	CPU_FLAGL = szpcflag[(UINT8)(d)]; \
} while (/*CONSTCOND*/ 0)

#define	_WORD_XOR(d, s) \
do { \
	(d) ^= (s); \
	CPU_OV = 0; \
	CPU_FLAGL = szpflag_w[(UINT16)(d)]; \
} while (/*CONSTCOND*/ 0)

#define	_DWORD_XOR(d, s) \
do { \
	(d) ^= (s); \
	CPU_OV = 0; \
	CPU_FLAGL = (UINT8)(szpcflag[(UINT8)(d)] & P_FLAG); \
	if ((d) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if ((d) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

#define	_BYTE_NEG(d, s) \
do { \
	(d) = 0 - (s); \
	CPU_OV = ((d) & (s)) & 0x80; \
	CPU_FLAGL = (UINT8)(((d) ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpcflag[(d) & 0x1ff]; \
} while (/*CONSTCOND*/ 0)

#define	_WORD_NEG(d, s) \
do { \
	(d) = 0 - (s); \
	CPU_OV = ((d) & (s)) & 0x8000; \
	CPU_FLAGL = (UINT8)(((d) ^ (s)) & A_FLAG); \
	if ((d) & 0xffff0000) { \
		(d) &= 0x0000ffff; \
		CPU_FLAGL |= C_FLAG; \
	} \
	CPU_FLAGL |= szpflag_w[(UINT16)(d)]; \
} while (/*CONSTCOND*/ 0)

#define	_DWORD_NEG(d, s) \
do { \
	(d) = 0 - (s); \
	CPU_OV = ((d) & (s)) & 0x80000000; \
	CPU_FLAGL = (UINT8)(((d) ^ (s)) & A_FLAG); \
	if ((d) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} else { \
		CPU_FLAGL |= C_FLAG; \
	} \
	if ((d) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
	CPU_FLAGL |= szpcflag[(UINT8)(d)] & P_FLAG; \
} while (/*CONSTCOND*/ 0)

#define	_BYTE_MUL(r, d, s) \
do { \
	CPU_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG); \
	(r) = (UINT8)(d) * (UINT8)(s); \
	CPU_OV = (r) >> 8; \
	if (CPU_OV) { \
		CPU_FLAGL |= C_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

#define	_WORD_MUL(r, d, s) \
do { \
	CPU_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG); \
	(r) = (UINT16)(d) * (UINT16)(s); \
	CPU_OV = (r) >> 16; \
	if (CPU_OV) { \
		CPU_FLAGL |= C_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

#define	_DWORD_MUL(r, d, s) \
do { \
	UINT64 __v; \
	CPU_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG); \
	__v = (UINT64)(d) * (UINT64)(s); \
	(r) = (UINT32)__v; \
	CPU_OV = (UINT32)(__v >> 32); \
	if (CPU_OV) { \
		CPU_FLAGL |= C_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

#define	_BYTE_IMUL(r, d, s) \
do { \
	CPU_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG); \
	(r) = (SINT8)(d) * (SINT8)(s); \
	CPU_OV = ((r) + 0x80) & 0xffffff00; \
	if (CPU_OV) { \
		CPU_FLAGL |= C_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

#define	_WORD_IMUL(r, d, s) \
do { \
	CPU_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG); \
	(r) = (SINT16)(d) * (SINT16)(s); \
	CPU_OV = ((r) + 0x8000) & 0xffff0000; \
	if (CPU_OV) { \
		CPU_FLAGL |= C_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

#define	_DWORD_IMUL(r, d, s) \
do { \
	CPU_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG); \
	(r) = (SINT64)(d) * (SINT64)(s); \
	CPU_OV = (UINT32)(((r) + QWORD_CONST(0x80000000)) >> 32); \
	if (CPU_OV) { \
		CPU_FLAGL |= C_FLAG; \
	} \
} while (/*CONSTCOND*/ 0)

/* flag no check */
#define	_BYTE_INC(s) \
do { \
	UINT8 __b = (s); \
	__b++; \
	CPU_OV = __b & (__b ^ (s)) & 0x80; \
	CPU_FLAGL &= C_FLAG; \
	CPU_FLAGL |= (UINT8)((__b ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpcflag[__b]; \
	(s) = __b; \
} while (/*CONSTCOND*/ 0)

#define	_WORD_INC(s) \
do { \
	UINT16 __b = (s); \
	__b++; \
	CPU_OV = __b & (__b ^ (s)) & 0x8000; \
	CPU_FLAGL &= C_FLAG; \
	CPU_FLAGL |= (UINT8)((__b ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpflag_w[__b]; \
	(s) = __b; \
} while (/*CONSTCOND*/ 0)

#define	_DWORD_INC(s) \
do { \
	UINT32 __b = (s); \
	__b++; \
	CPU_OV = __b & (__b ^ (s)) & 0x80000000; \
	CPU_FLAGL &= C_FLAG; \
	CPU_FLAGL |= (UINT8)((__b ^ (s)) & A_FLAG); \
	if (__b == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if (__b & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
	CPU_FLAGL |= szpcflag[(UINT8)(__b)] & P_FLAG; \
	(s) = __b; \
} while (/*CONSTCOND*/ 0)

/* flag no check */
#define	_BYTE_DEC(s) \
do { \
	UINT8 __b = (s); \
	__b--; \
	CPU_OV = (s) & (__b ^ (s)) & 0x80; \
	CPU_FLAGL &= C_FLAG; \
	CPU_FLAGL |= (UINT8)((__b ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpcflag[__b]; \
	(s) = __b; \
} while (/*CONSTCOND*/ 0)

#define	_WORD_DEC(s) \
do { \
	UINT16 __b = (s); \
	__b--; \
	CPU_OV = (s) & (__b ^ (s)) & 0x8000; \
	CPU_FLAGL &= C_FLAG; \
	CPU_FLAGL |= (UINT8)((__b ^ (s)) & A_FLAG); \
	CPU_FLAGL |= szpflag_w[__b]; \
	(s) = __b; \
} while (/*CONSTCOND*/ 0)

#define	_DWORD_DEC(s) \
do { \
	UINT32 __b = (s); \
	__b--; \
	CPU_OV = (s) & (__b ^ (s)) & 0x80000000; \
	CPU_FLAGL &= C_FLAG; \
	CPU_FLAGL |= (UINT8)((__b ^ (s)) & A_FLAG); \
	if ((__b) == 0) { \
		CPU_FLAGL |= Z_FLAG; \
	} \
	if ((__b) & 0x80000000) { \
		CPU_FLAGL |= S_FLAG; \
	} \
	CPU_FLAGL |= szpcflag[(UINT8)(__b)] & P_FLAG; \
	(s) = __b; \
} while (/*CONSTCOND*/ 0)

#define	BYTE_NOT(s) \
do { \
	(s) ^= 0xff; \
} while (/*CONSTCOND*/ 0)

#define	WORD_NOT(s) \
do { \
	(s) ^= 0xffff; \
} while (/*CONSTCOND*/ 0)

#define	DWORD_NOT(s) \
do { \
	(s) ^= 0xffffffff; \
} while (/*CONSTCOND*/ 0)


/*
 * stack
 */
#define	REGPUSH(reg, clock) \
do { \
	UINT16 __new_sp = CPU_SP - 2; \
	CPU_WORKCLOCK(clock); \
	cpu_vmemorywrite_w(CPU_SS_INDEX, __new_sp, reg); \
	CPU_SP = __new_sp; \
} while (/*CONSTCOND*/ 0)

#define	REGPUSH_32(reg, clock) \
do { \
	UINT32 __new_esp = CPU_ESP - 4; \
	CPU_WORKCLOCK(clock); \
	cpu_vmemorywrite_d(CPU_SS_INDEX, __new_esp, reg); \
	CPU_ESP = __new_esp; \
} while (/*CONSTCOND*/ 0)

#define	REGPUSH0(reg) \
do { \
	UINT16 __new_sp = CPU_SP - 2; \
	cpu_vmemorywrite_w(CPU_SS_INDEX, __new_sp, (UINT16)reg); \
	CPU_SP = __new_sp; \
} while (/*CONSTCOND*/ 0)

/* Operand Size == 16 && Stack Size == 32 */
#define	REGPUSH0_16_32(reg) \
do { \
	UINT32 __new_esp = CPU_ESP - 2; \
	cpu_vmemorywrite_w(CPU_SS_INDEX, __new_esp, (UINT16)reg); \
	CPU_ESP = __new_esp; \
} while (/*CONSTCOND*/ 0)

/* Operand Size == 32 && Stack Size == 16 */
#define	REGPUSH0_32_16(reg) \
do { \
	UINT16 __new_sp = CPU_SP - 4; \
	cpu_vmemorywrite_d(CPU_SS_INDEX, __new_sp, reg); \
	CPU_SP = __new_sp; \
} while (/*CONSTCOND*/ 0)

#define	REGPUSH0_32(reg) \
do { \
	UINT32 __new_esp = CPU_ESP - 4; \
	cpu_vmemorywrite_d(CPU_SS_INDEX, __new_esp, reg); \
	CPU_ESP = __new_esp; \
} while (/*CONSTCOND*/ 0)

#define	PUSH0_16(reg) \
do { \
	if (!CPU_STAT_SS32) { \
		REGPUSH0(reg); \
	} else { \
		REGPUSH0_16_32(reg); \
	} \
} while (/*CONSTCOND*/ 0)

#define	PUSH0_32(reg) \
do { \
	if (CPU_STAT_SS32) { \
		REGPUSH0_32(reg); \
	} else { \
		REGPUSH0_32_16(reg); \
	} \
} while (/*CONSTCOND*/ 0)

#define	XPUSH0(reg) \
do { \
	if (!CPU_INST_OP32) { \
		PUSH0_16(reg); \
	} else { \
		PUSH0_32(reg); \
	} \
} while (/*CONSTCOND*/ 0)

#define	REGPOP(reg, clock) \
do { \
	CPU_WORKCLOCK(clock); \
	(reg) = cpu_vmemoryread_w(CPU_SS_INDEX, CPU_SP); \
	CPU_SP += 2; \
} while (/*CONSTCOND*/ 0)

#define	REGPOP_32(reg, clock) \
do { \
	CPU_WORKCLOCK(clock); \
	(reg) = cpu_vmemoryread_d(CPU_SS_INDEX, CPU_ESP); \
	CPU_ESP += 4; \
} while (/*CONSTCOND*/ 0)

#define	REGPOP0(reg) \
do { \
	(reg) = cpu_vmemoryread_w(CPU_SS_INDEX, CPU_SP); \
	CPU_SP += 2; \
} while (/*CONSTCOND*/ 0)

#define	REGPOP0_16_32(reg) \
do { \
	(reg) = cpu_vmemoryread_w(CPU_SS_INDEX, CPU_ESP); \
	CPU_ESP += 2; \
} while (/*CONSTCOND*/ 0)

#define	REGPOP0_32_16(reg) \
do { \
	(reg) = cpu_vmemoryread_d(CPU_SS_INDEX, CPU_SP); \
	CPU_SP += 4; \
} while (/*CONSTCOND*/ 0)

#define	REGPOP0_32(reg) \
do { \
	(reg) = cpu_vmemoryread_d(CPU_SS_INDEX, CPU_ESP); \
	CPU_ESP += 4; \
} while (/*CONSTCOND*/ 0)

#define	POP0_16(reg) \
do { \
	if (!CPU_STAT_SS32) { \
		REGPOP0(reg); \
	} else { \
		REGPOP0_16_32(reg); \
	} \
} while (/*CONSTCOND*/ 0)

#define	POP0_32(reg) \
do { \
	if (CPU_STAT_SS32) { \
		REGPOP0_32(reg); \
	} else { \
		REGPOP0_32_16(reg); \
	} \
} while (/*CONSTCOND*/ 0)

/*
 * stack pointer
 */
#define	SP_PUSH_16(reg) \
do { \
	UINT16 __sp = CPU_SP; \
	if (!CPU_STAT_SS32) { \
		REGPUSH0(__sp); \
	} else { \
		REGPUSH0_16_32(__sp); \
	} \
} while (/*CONSTCOND*/ 0)

#define	ESP_PUSH_32(reg) \
do { \
	UINT32 __esp = CPU_ESP; \
	if (!CPU_STAT_SS32) { \
		REGPUSH0_32_16(__esp); \
	} else { \
		REGPUSH0_32(__esp); \
	} \
} while (/*CONSTCOND*/ 0)

#define	SP_POP_16(reg) \
do { \
	UINT32 __sp; \
	if (!CPU_STAT_SS32) { \
		__sp = CPU_SP; \
	} else { \
		__sp = CPU_ESP; \
	} \
	CPU_SP = cpu_vmemoryread_w(CPU_SS_INDEX, __sp); \
} while (/*CONSTCOND*/ 0)

#define	ESP_POP_32(reg) \
do { \
	UINT32 __esp; \
	if (!CPU_STAT_SS32) { \
		__esp = CPU_SP; \
	} else { \
		__esp = CPU_ESP; \
	} \
	CPU_ESP = cpu_vmemoryread_d(CPU_SS_INDEX, __esp); \
} while (/*CONSTCOND*/ 0)


/*
 * jump
 */
#define	JMPSHORT(clock) \
do { \
	UINT32 __new_ip; \
	UINT32 __dest; \
	CPU_WORKCLOCK(clock); \
	GET_PCBYTESD(__dest); \
	__new_ip = CPU_EIP + __dest; \
	if (!CPU_INST_OP32) { \
		__new_ip &= 0xffff; \
	} \
	if (__new_ip > CPU_STAT_CS_LIMIT) { \
		EXCEPTION(GP_EXCEPTION, 0); \
	} \
	CPU_EIP = __new_ip; \
} while (/*CONSTCOND*/ 0)

#define	JMPNEAR(clock) \
do { \
	UINT16 __new_ip; \
	SINT16 __dest; \
	CPU_WORKCLOCK(clock); \
	GET_PCWORDS(__dest); \
	__new_ip = CPU_IP + __dest; \
	if (__new_ip > CPU_STAT_CS_LIMIT) { \
		EXCEPTION(GP_EXCEPTION, 0); \
	} \
	CPU_EIP = __new_ip; \
} while (/*CONSTCOND*/ 0)

#define	JMPNEAR32(clock) \
do { \
	UINT32 __new_ip; \
	UINT32 __dest; \
	CPU_WORKCLOCK(clock); \
	GET_PCDWORD(__dest); \
	__new_ip = CPU_EIP + __dest; \
	if (__new_ip > CPU_STAT_CS_LIMIT) { \
		EXCEPTION(GP_EXCEPTION, 0); \
	} \
	CPU_EIP = __new_ip; \
} while (/*CONSTCOND*/ 0)

#define	JMPNOP(clock, d) \
do { \
	CPU_WORKCLOCK(clock); \
	_ADD_EIP((d)); \
} while (/*CONSTCOND*/ 0)


/*
 * conditions
 */
#define	CC_O	(CPU_OV)
#define	CC_NO	(!CPU_OV)
#define	CC_C	(CPU_FLAGL & C_FLAG)
#define	CC_NC	(!(CPU_FLAGL & C_FLAG))
#define	CC_Z	(CPU_FLAGL & Z_FLAG)
#define	CC_NZ	(!(CPU_FLAGL & Z_FLAG))
#define	CC_NA	(CPU_FLAGL & (Z_FLAG | C_FLAG))
#define	CC_A	(!(CPU_FLAGL & (Z_FLAG | C_FLAG)))
#define	CC_S	(CPU_FLAGL & S_FLAG)
#define	CC_NS	(!(CPU_FLAGL & S_FLAG))
#define	CC_P	(CPU_FLAGL & P_FLAG)
#define	CC_NP	(!(CPU_FLAGL & P_FLAG))
#define	CC_L	(((CPU_FLAGL & S_FLAG) == 0) != (CPU_OV == 0))
#define	CC_NL	(((CPU_FLAGL & S_FLAG) == 0) == (CPU_OV == 0))
#define	CC_LE	((CPU_FLAGL & Z_FLAG) || \
				(((CPU_FLAGL & S_FLAG) == 0) != (CPU_OV == 0)))
#define	CC_NLE	((!(CPU_FLAGL & Z_FLAG)) && \
				(((CPU_FLAGL & S_FLAG) == 0) == (CPU_OV == 0)))


/*
 * instruction check
 */
#include "ia32xc.mcr"

#endif	/* IA32_CPU_IA32_MCR__ */
