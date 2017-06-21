#include	"compiler.h"
#include	"cpucore.h"
#include	"i286x.h"
#include	"i286xadr.h"
#include	"i286xs.h"
#include	"i286xrep.h"
#include	"i286xcts.h"
#include	"pccore.h"
#include	"bios/bios.h"
#include	"iocore.h"
#include	"i286x.mcr"
#include	"i286xea.mcr"
#include	"dmax86.h"
#if defined(ENABLE_TRAP)
#include "trap/steptrap.h"
#endif


typedef struct {
	UINT	opnum;
	I286TBL	v30opcode;
} V30PATCH;

static	I286TBL v30op[256];
static	I286TBL v30op_repne[256];
static	I286TBL	v30op_repe[256];
static	I286TBL	v30ope0xf6_xtable[8];
static	I286TBL v30ope0xf7_xtable[8];


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

static const UINT8 shiftbase[256] =
				{0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
				17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17};



I286 v30_reserved(void) {

		__asm {
				I286CLOCK(2)
				GET_NEXTPRE1
				ret
		}
}

I286 v30pop_ss(void) {							// 17: pop ss

		__asm {
				I286CLOCK(5)
				REGPOP(I286_SS)
				and		eax, 00ffffh
				shl		eax, 4					// make segreg
				mov		SS_BASE, eax
				mov		SS_FIX, eax
				cmp		i286core.s.prefix, 0	// 00/06/24
				je		noprefix
				call	removeprefix
				pop		eax
		noprefix:
				movzx	ebp, bh
				GET_NEXTPRE1
				jmp		v30op[ebp*4]
		}
}

I286 v30segprefix_es(void) {					// 26: es:

		__asm {
				mov		eax, ES_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				I286PREFIX(v30op)
		}
}

I286 v30segprefix_cs(void) {					// 2E: cs:

		__asm {
				mov		eax, CS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				I286PREFIX(v30op)
		}
}

I286 v30segprefix_ss(void) {					// 36: ss:

		__asm {
				mov		eax, SS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				I286PREFIX(v30op)
		}
}

I286 v30segprefix_ds(void) {					// 3E: ds:

		__asm {
				mov		eax, DS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				I286PREFIX(v30op)
		}
}

I286 v30push_sp(void) {							// 54: push sp

		__asm {
				I286CLOCK(3)
				GET_NEXTPRE1
				sub		I286_SP, 2
				movzx	ecx, I286_SP
				mov		edx, ecx
				add		ecx, SS_BASE
				jmp		i286_memorywrite_w
		}
}

I286 v30pop_sp(void) {							// 5C: pop sp

		__asm {
				I286CLOCK(5)
				REGPOP(I286_SP)
				GET_NEXTPRE1
				ret
		}
}

I286 v30mov_seg_ea(void) {						// 8E: mov segrem, EA

		__asm {
				movzx	eax, bh
				mov		ebp, eax
				shr		ebp, 3-1
				and		ebp, 3*2
				cmp		al, 0c0h
				jc		src_memory
				I286CLOCK(2)
				and		eax, 7
				mov		edi, eax
				GET_NEXTPRE2
				mov		ax, word ptr I286_REG[edi*2]
				jmp		segset
				align	4
		src_memory:
				I286CLOCK(5)
				call	p_ea_dst[eax*4]
				call	i286_memoryread_w
		segset:
#if 0
				cmp		ebp, 1*2				// prefixed cs?
				je		segsetr
#endif
				mov		word ptr I286_SEGREG[ebp], ax
				and		eax, 0000ffffh
				shl		eax, 4					// make segreg
				mov		SEG_BASE[ebp*2], eax
				sub		ebp, 2*2
				jc		segsetr
				mov		SS_FIX[ebp*2], eax
				je		setss
		segsetr:ret

				align	4
		setss:	cmp		i286core.s.prefix, 0	// 00/05/13
				je		noprefix
				pop		eax
				call	eax						// eax<-offset removeprefix
		noprefix:
				movzx	eax, bl
				jmp		v30op[eax*4]
		}
}

I286 v30_pushf(void) {							// 9C: pushf

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				mov		dx, I286_FLAG
				or		dx, 0f000h
				sub		I286_SP, 2
				movzx	ecx, I286_SP
				add		ecx, SS_BASE
				jmp		i286_memorywrite_w
		}
}

I286 v30_popf(void) {							// 9D: popf

		__asm {
				GET_NEXTPRE1
				I286CLOCK(5)
				movzx	ecx, I286_SP
				add		ecx, SS_BASE
				call	i286_memoryread_w
				add		I286_SP, 2
				or		ah, 0f0h
				mov		I286_FLAG, ax
				and		ah, 3
				cmp		ah, 3
				sete	I286_TRAP
				I286IRQCHECKTERM
		}
}


// ----- reg8

I286 rol_r8_v30(void) {

		__asm {
				mov		cl, rotatebase16[ecx]
				rol		byte ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 ror_r8_v30(void) {

		__asm {
				mov		cl, rotatebase16[ecx]
				ror		byte ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 rcl_r8_v30(void) {

		__asm {
				mov		cl, rotatebase09[ecx]
				CFLAG_LOAD
				rcl		byte ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 rcr_r8_v30(void) {

		__asm {
				mov		cl, rotatebase09[ecx]
				CFLAG_LOAD
				rcr		byte ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 shl_r8_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				shl		byte ptr [edx], cl
				FLAG_STORE_OF
				ret
		}
}

I286 shr_r8_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				shr		byte ptr [edx], cl
				FLAG_STORE_OF
				ret
		}
}

I286 sar_r8_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				sar		byte ptr [edx], cl
				FLAG_STORE0
				ret
		}
}

static void (*sftreg8v30_table[])(void) = {
		rol_r8_v30,		ror_r8_v30,		rcl_r8_v30,		rcr_r8_v30,
		shl_r8_v30,		shr_r8_v30,		shl_r8_v30,		sar_r8_v30};

I286 rol_ext8_v30(void) {

		__asm {
				mov		cl, rotatebase16[ecx]
				rol		dl, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 ror_ext8_v30(void) {

		__asm {
				mov		cl, rotatebase16[ecx]
				ror		dl, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 rcl_ext8_v30(void) {

		__asm {
				mov		cl, rotatebase09[ecx]
				CFLAG_LOAD
				rcl		dl, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 rcr_ext8_v30(void) {

		__asm {
				mov		cl, rotatebase09[ecx]
				CFLAG_LOAD
				rcr		dl, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 shl_ext8_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				shl		dl, cl
				FLAG_STORE_OF
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 shr_ext8_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				shr		dl, cl
				FLAG_STORE_OF
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 sar_ext8_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				sar		dl, cl
				FLAG_STORE0
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

static void (*sftext8v30_table[])(void) = {
		rol_ext8_v30,	ror_ext8_v30,	rcl_ext8_v30,	rcr_ext8_v30,
		shl_ext8_v30,	shr_ext8_v30,	shl_ext8_v30,	sar_ext8_v30};

I286 v30shift_ea8_data8(void) {					// C0: shift EA8, DATA8

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		al, 0c0h
				jc		memory_eareg8
				I286CLOCK(5)
				bt		ax, 2
				rcl		eax, 1
				and		eax, 7
				lea		edx, I286_REG[eax]
				mov		ecx, ebx
				shr		ecx, 16
				and		ecx, 255
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE3
				pop		ecx
				jmp		sftreg8v30_table[edi]
				align	4
		memory_eareg8:
				I286CLOCK(8)
				call	p_ea_dst[eax*4]
				cmp		ecx, I286_MEMWRITEMAX
				jnc		extmem_eareg8
				lea		edx, I286_MEM[ecx]
				movzx	ecx, bl
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE1
				pop		ecx
				jmp		sftreg8v30_table[edi]
				align	4
		extmem_eareg8:
				call	i286_memoryread
				mov		ebp, ecx
				mov		edx, eax
				movzx	ecx, bl
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE1
				pop		ecx
				jmp		sftext8v30_table[edi]
	}
}

I286 rol_r16_v30(void) {

		__asm {
				mov		cl, rotatebase16[ecx]
				rol		word ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 ror_r16_v30(void) {

		__asm {
				mov		cl, rotatebase16[ecx]
				ror		word ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 rcl_r16_v30(void) {

		__asm {
				mov		cl, rotatebase17[ecx]
				CFLAG_LOAD
				rcl		word ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 rcr_r16_v30(void) {

		__asm {
				mov		cl, rotatebase17[ecx]
				CFLAG_LOAD
				rcr		word ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 shl_r16_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				shl		word ptr [edx], cl
				FLAG_STORE_OF
				ret
		}
}

I286 shr_r16_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				shr		word ptr [edx], cl
				FLAG_STORE_OF
				ret
		}
}

I286 sar_r16_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				sar		word ptr [edx], cl
				FLAG_STORE0
				ret
		}
}

static void (*sftreg16v30_table[])(void) = {
		rol_r16_v30,	ror_r16_v30,	rcl_r16_v30,	rcr_r16_v30,
		shl_r16_v30,	shr_r16_v30,	shl_r16_v30,	sar_r16_v30};

I286 rol_ext16_v30(void) {

		__asm {
				mov		cl, rotatebase16[ecx]
				rol		dx, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 ror_ext16_v30(void) {

		__asm {
				mov		cl, rotatebase16[ecx]
				ror		dx, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 rcl_ext16_v30(void) {

		__asm {
				mov		cl, rotatebase17[ecx]
				CFLAG_LOAD
				rcl		dx, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 rcr_ext16_v30(void) {

		__asm {
				mov		cl, rotatebase17[ecx]
				CFLAG_LOAD
				rcr		dx, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 shl_ext16_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				shl		dx, cl
				FLAG_STORE_OF
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 shr_ext16_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				shr		dx, cl
				FLAG_STORE_OF
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 sar_ext16_v30(void) {

		__asm {
				mov		cl, shiftbase[ecx]
				sar		dx, cl
				FLAG_STORE0
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

static void (*sftext16v30_table[])(void) = {
		rol_ext16_v30,	ror_ext16_v30,	rcl_ext16_v30,	rcr_ext16_v30,
		shl_ext16_v30,	shr_ext16_v30,	shl_ext16_v30,	sar_ext16_v30};

I286 v30shift_ea16_data8(void) {				// C1: shift EA16, DATA8

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		al, 0c0h
				jc		memory_eareg16
				and		eax, 7
				I286CLOCK(5)
				lea		edx, I286_REG[eax*2]
				mov		ecx, ebx
				shr		ecx, 16
				and		ecx, 255
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE3
				pop		ecx
				jmp		sftreg16v30_table[edi]
				align	4
		memory_eareg16:
				I286CLOCK(8)
				call	p_ea_dst[eax*4]
				cmp		ecx, (I286_MEMWRITEMAX-1)
				jnc		extmem_eareg16
				lea		edx, I286_MEM[ecx]
				movzx	ecx, bl
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE1
				pop		ecx
				jmp		sftreg16v30_table[edi]
				align	4
		extmem_eareg16:
				call	i286_memoryread_w
				mov		ebp, ecx
				mov		edx, eax
				movzx	ecx, bl
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE1
				pop		ecx
				jmp		sftext16v30_table[edi]
	}
}

I286 v30shift_ea8_cl(void) {					// D2: shift EA8, cl

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		al, 0c0h
				jc		memory_eareg8
				I286CLOCK(5)
				bt		ax, 2
				rcl		eax, 1
				and		eax, 7
				lea		edx, I286_REG[eax]
				GET_NEXTPRE2
				movzx	ecx, I286_CL
				I286CLOCK(ecx)
				jmp		sftreg8v30_table[edi]
				align	4
		memory_eareg8:
				I286CLOCK(8)
				call	p_ea_dst[eax*4]
				cmp		ecx, I286_MEMWRITEMAX
				jnc		extmem_eareg8
				lea		edx, I286_MEM[ecx]
				movzx	ecx, I286_CL
				I286CLOCK(ecx)
				jmp		sftreg8v30_table[edi]
				align	4
		extmem_eareg8:
				call	i286_memoryread
				mov		edx, eax
				mov		ebp, ecx
				movzx	ecx, I286_CL
				I286CLOCK(ecx)
				jmp		sftext8v30_table[edi]
	}
}

I286 v30shift_ea16_cl(void) {					// D3: shift EA16, cl

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		al, 0c0h
				jc		memory_eareg16
				I286CLOCK(5)
				and		eax, 7
				lea		edx, I286_REG[eax*2]
				GET_NEXTPRE2
				movzx	ecx, I286_CL
				I286CLOCK(ecx)
				jmp		sftreg16v30_table[edi]
				align	4
		memory_eareg16:
				I286CLOCK(8)
				call	p_ea_dst[eax*4]
				cmp		ecx, (I286_MEMWRITEMAX-1)
				jnc		extmem_eareg16
				lea		edx, I286_MEM[ecx]
				movzx	ecx, I286_CL
				I286CLOCK(ecx)
				jmp		sftreg16v30_table[edi]
				align	4
		extmem_eareg16:
				call	i286_memoryread_w
				mov		edx, eax
				mov		ebp, ecx
				movzx	ecx, I286_CL
				I286CLOCK(ecx)
				jmp		sftext16v30_table[edi]
	}
}

I286 v30_aam(void) {							// D4: AAM

		__asm {
				I286CLOCK(16)
				mov		ax, I286_AX
				aam
				mov		I286_AX, ax
				FLAG_STORE
				GET_NEXTPRE2
				ret
		}
}

I286 v30_aad(void) {							// D5: AAD

		__asm {
				I286CLOCK(14)
				mov		ax, I286_AX
				aad
				mov		I286_AX, ax
				FLAG_STORE
				GET_NEXTPRE2
				ret
		}
}

I286 v30_xlat(void) {							// D6: XLAT

		__asm {
				I286CLOCK(5)
				movzx	ecx, I286_AL
				add		cx, I286_BX
				add		ecx, DS_FIX
				call	i286_memoryread
				mov		I286_AL, al
				GET_NEXTPRE1
				ret
		}
}

I286 v30_repne(void) {							// F2: repne

		__asm {
				I286PREFIX(v30op_repne)
		}
}

I286 v30_repe(void) {							// F3: repe

		__asm {
				I286PREFIX(v30op_repe)
		}
}

I286 v30div_ea8(void) {							// F6-6: div ea8

		__asm {
				PREPART_EA8(14)
					movzx	ebp, byte ptr I286_REG[eax]
					GET_NEXTPRE2
					jmp		divcheck
				MEMORY_EA8(17)
					movzx	ebp, byte ptr I286_MEM[ecx]
					jmp		divcheck
				EXTMEM_EA8
					movzx	ebp, al

				align	4
	divcheck:	test	ebp, ebp
				je		divovf
				mov		ax, I286_AX
				xor		dx, dx
				div		bp
				mov		I286_AL, al
				mov		I286_AH, dl
				mov		dx, ax
				FLAG_STORE_OF
				test	dh, dh
				jne		divovf
				ret

				align	4
	divovf:		INT_NUM(0)
		}
}

I286 v30idiv_ea8(void) {						// F6-7 idiv ea8

		__asm {
				PREPART_EA8(17)
					movsx	ebp, byte ptr I286_REG[eax]
					GET_NEXTPRE2
					jmp		idivcheck
				MEMORY_EA8(20)
					movsx	ebp, byte ptr I286_MEM[ecx]
					jmp		idivcheck
				EXTMEM_EA8
					movsx	ebp, al

				align	4
	idivcheck:	test	ebp, ebp
				je		idivovf
				mov		ax, I286_AX
				cwd
				idiv	bp
				mov		I286_AL, al
				mov		I286_AH, dl
				mov		dx, ax
				FLAG_STORE_OF
				bt		dx, 7
				adc		dh, 0
				jne		idivovf
				ret

				align	4
	idivovf:	INT_NUM(0)
		}
}

I286 v30_ope0xf6(void) {						// F6: 

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4
				jmp		v30ope0xf6_xtable[edi]
		}
}

I286 v30div_ea16(void) {						// F7-6: div ea16

		__asm {
				PREPART_EA16(22)
					movzx	ebp, word ptr I286_REG[eax*2]
					GET_NEXTPRE2
					jmp		divcheck
				MEMORY_EA16(25)
					movzx	ebp, word ptr I286_MEM[ecx]
					jmp		divcheck
				EXTMEM_EA16
					movzx	ebp, ax

				align	4
	divcheck:	test	ebp, ebp
				je		divovf
				movzx	eax, I286_DX
				shl		eax, 16
				mov		ax, I286_AX
				xor		edx, edx
				div		ebp
				mov		I286_AX, ax
				mov		I286_DX, dx
				FLAG_STORE_OF
				cmp		eax, 10000h
				jae		divovf
				ret

				align	4
	divovf:		INT_NUM(0)
		}
}

I286 v30idiv_ea16(void) {						// F7-7: idiv ea16

		__asm {
				PREPART_EA16(25)
					movsx	ebp, word ptr I286_REG[eax*2]
					GET_NEXTPRE2
					jmp		idivcheck
				MEMORY_EA16(28)
					movsx	ebp, word ptr I286_MEM[ecx]
					jmp		idivcheck
				EXTMEM_EA16
					cwde
					mov		ebp, eax

				align	4
	idivcheck:	test	ebp, ebp
				je		idivovf
				movzx	eax, I286_DX
				shl		eax, 16
				mov		ax, I286_AX
				cdq
				idiv	ebp
				mov		I286_AX, ax
				mov		I286_DX, dx
				mov		edx, eax
				FLAG_STORE_OF
				shr		edx, 16
				adc		dx, 0
				jne		idivovf
				ret

				align	4
	idivovf:	INT_NUM(0)
		}
}

I286 v30_ope0xf7(void) {						// F7: 

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4
				jmp		v30ope0xf7_xtable[edi]
		}
}


static const V30PATCH v30patch_op[] = {
			{0x17, v30pop_ss},				// 17:	pop		ss
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

I286 v30repe_segprefix_es(void) {

		__asm {
				mov		eax, ES_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		v30op_repe[eax*4]
		}
}

I286 v30repe_segprefix_cs(void) {

		__asm {
				mov		eax, CS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		v30op_repe[eax*4]
		}
}

I286 v30repe_segprefix_ss(void) {

		__asm {
				mov		eax, SS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		v30op_repe[eax*4]
		}
}

I286 v30repe_segprefix_ds(void) {

		__asm {
				mov		eax, DS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		v30op_repe[eax*4]
		}
}


static const V30PATCH v30patch_repe[] = {
			{0x17, v30pop_ss},				// 17:	pop		ss
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

I286 v30repne_segprefix_es(void) {

		__asm {
				mov		eax, ES_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		v30op_repne[eax*4]
		}
}

I286 v30repne_segprefix_cs(void) {

		__asm {
				mov		eax, CS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		v30op_repne[eax*4]
		}
}

I286 v30repne_segprefix_ss(void) {

		__asm {
				mov		eax, SS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		v30op_repne[eax*4]
		}
}

I286 v30repne_segprefix_ds(void) {

		__asm {
				mov		eax, DS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		v30op_repne[eax*4]
		}
}

static const V30PATCH v30patch_repne[] = {
			{0x17, v30pop_ss},				// 17:	pop		ss
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

static void v30patching(I286TBL *dst, const V30PATCH *patch, int length) {

	while(length--) {
		dst[patch->opnum] = patch->v30opcode;
		patch++;
	}
}

#define	V30PATCHING(a, b)	v30patching(a, b, sizeof(b)/sizeof(V30PATCH))

void v30xinit(void) {

	CopyMemory(v30op, i286op, sizeof(v30op));
	V30PATCHING(v30op, v30patch_op);
	CopyMemory(v30op_repne, i286op_repne, sizeof(v30op_repne));
	V30PATCHING(v30op_repne, v30patch_repne);
	CopyMemory(v30op_repe, i286op_repe, sizeof(v30op_repe));
	V30PATCHING(v30op_repe, v30patch_repe);
	CopyMemory(v30ope0xf6_xtable, ope0xf6_xtable, sizeof(v30ope0xf6_xtable));
	v30ope0xf6_xtable[6] = v30div_ea8;
	v30ope0xf6_xtable[7] = v30idiv_ea8;
	CopyMemory(v30ope0xf7_xtable, ope0xf7_xtable, sizeof(v30ope0xf7_xtable));
	v30ope0xf7_xtable[6] = v30div_ea16;
	v30ope0xf7_xtable[7] = v30idiv_ea16;
}

LABEL void v30x(void) {

	__asm {
				pushad
				mov		ebx, dword ptr (i286core.s.prefetchque)
				movzx	esi, I286_IP

				cmp		I286_TRAP, 0
				jne		short v30_trapping
				cmp		dmac.working, 0
				jne		short v30_dma_mnlp

				align	4
v30_mnlp:
#if defined(ENABLE_TRAP)
				mov		edx, esi
				movzx	ecx, I286_CS
				call	steptrap
#endif
				movzx	eax, bl
				call	v30op[eax*4]
				cmp		I286_REMCLOCK, 0
				jg		v30_mnlp
				mov		dword ptr (i286core.s.prefetchque), ebx
				mov		I286_IP, si
				popad
				ret

				align	4
v30_dma_mnlp:
#if defined(ENABLE_TRAP)
				mov		edx, esi
				movzx	ecx, I286_CS
				call	steptrap
#endif
				movzx	eax, bl
				call	v30op[eax*4]
				call	dmax86
				cmp		I286_REMCLOCK, 0
				jg		v30_dma_mnlp
				mov		dword ptr (i286core.s.prefetchque), ebx
				mov		I286_IP, si
				popad
				ret

				align	4
v30_trapping:
#if defined(ENABLE_TRAP)
				mov		edx, esi
				movzx	ecx, I286_CS
				call	steptrap
#endif
				movzx	eax, bl
				call	v30op[eax*4]
				cmp		I286_TRAP, 0
				je		v30notrap
				mov		ecx, 1
				call	i286x_localint
v30notrap:		mov		dword ptr (i286core.s.prefetchque), ebx
				mov		I286_IP, si
				popad
				ret
	}
}

LABEL void v30x_step(void) {

	__asm {
				pushad
				mov		ebx, dword ptr (i286core.s.prefetchque)
				movzx	esi, I286_IP

				movzx	eax, bl
				call	v30op[eax*4]

				cmp		I286_TRAP, 0
				je		short nexts
				mov		ecx, 1
				call	i286x_localint
nexts:
				mov		dword ptr (i286core.s.prefetchque), ebx
				mov		I286_IP, si

				call	dmax86
				popad
				ret
		}
}

