#include	"compiler.h"
#include	"cpucore.h"
#include	"i286x.h"
#include	"i286xadr.h"
#include	"i286xs.h"
#include	"i286x.mcr"
#include	"i286xea.mcr"


// ----- reg8

I286 add_r8_i(void) {

		__asm {
				add		[ebp], dl
				FLAG_STORE_OF
				ret
		}
}

I286 or_r8_i(void) {

		__asm {
				or		[ebp], dl
				FLAG_STORE0
				ret
		}
}

I286 adc_r8_i(void) {

		__asm {
				CFLAG_LOAD
				adc		[ebp], dl
				FLAG_STORE_OF
				ret
		}
}

I286 sbb_r8_i(void) {

		__asm {
				CFLAG_LOAD
				sbb		[ebp], dl
				FLAG_STORE_OF
				ret
		}
}

I286 and_r8_i(void) {

		__asm {
				and		[ebp], dl
				FLAG_STORE0
				ret
		}
}

I286 sub_r8_i(void) {

		__asm {
				sub		[ebp], dl
				FLAG_STORE_OF
				ret
		}
}

I286 xor_r8_i(void) {

		__asm {
				xor		[ebp], dl
				FLAG_STORE0
				ret
		}
}

I286 cmp_r8_i(void) {

		__asm {
				cmp		[ebp], dl
				FLAG_STORE_OF
				ret
		}
}

// ----- ext8

I286 add_ext8_i(void) {

		__asm {
				add		dl, bl
				FLAG_STORE_OF
				jmp		i286_memorywrite
		}
}

I286 or_ext8_i(void) {

		__asm {
				or		dl, bl
				FLAG_STORE0
				jmp		i286_memorywrite
		}
}

I286 adc_ext8_i(void) {

		__asm {
				CFLAG_LOAD
				adc		dl, bl
				FLAG_STORE_OF
				jmp		i286_memorywrite
		}
}

I286 sbb_ext8_i(void) {

		__asm {
				CFLAG_LOAD
				sbb		dl, bl
				FLAG_STORE_OF
				jmp		i286_memorywrite
		}
}

I286 and_ext8_i(void) {

		__asm {
				and		dl, bl
				FLAG_STORE0
				jmp		i286_memorywrite
		}
}

I286 sub_ext8_i(void) {

		__asm {
				sub		dl, bl
				FLAG_STORE_OF
				jmp		i286_memorywrite
		}
}

I286 xor_ext8_i(void) {

		__asm {
				xor		dl, bl
				FLAG_STORE0
				jmp		i286_memorywrite
		}
}

I286 cmp_ext8_i(void) {

		__asm {
				I286CLOCKDEC
				cmp		dl, bl
				FLAG_STORE_OF
				ret
		}
}


									// dest: I286_REG[eax] src: dl
const I286TBL op8xreg8_xtable[8] = {
		add_r8_i,		or_r8_i,		adc_r8_i,		sbb_r8_i,
		and_r8_i,		sub_r8_i,		xor_r8_i,		cmp_r8_i};

									// dest: ecx  src: dl
const I286TBL op8xext8_xtable[8] = {
		add_ext8_i,		or_ext8_i,		adc_ext8_i,		sbb_ext8_i,
		and_ext8_i,		sub_ext8_i,		xor_ext8_i,		cmp_ext8_i};


// -------------------------------------------------------------------------

// ----- reg16

I286 add_r16_i(void) {

		__asm {
				add		[ebp], dx
				FLAG_STORE_OF
				ret
		}
}

I286 or_r16_i(void) {

		__asm {
				or		[ebp], dx
				FLAG_STORE0
				ret
		}
}

I286 adc_r16_i(void) {

		__asm {
				CFLAG_LOAD
				adc		[ebp], dx
				FLAG_STORE_OF
				ret
		}
}

I286 sbb_r16_i(void) {

		__asm {
				CFLAG_LOAD
				sbb		[ebp], dx
				FLAG_STORE_OF
				ret
		}
}

I286 and_r16_i(void) {

		__asm {
				and		[ebp], dx
				FLAG_STORE0
				ret
		}
}

I286 sub_r16_i(void) {

		__asm {
				sub		[ebp], dx
				FLAG_STORE_OF
				ret
		}
}

I286 xor_r16_i(void) {

		__asm {
				xor		[ebp], dx
				FLAG_STORE0
				ret
		}
}

I286 cmp_r16_i(void) {

		__asm {
				cmp		[ebp], dx
				FLAG_STORE_OF
				ret
		}
}

// ----- ext16

I286 add_ext16_i(void) {

		__asm {
				add		dx, bx
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 or_ext16_i(void) {

		__asm {
				or		dx, bx
				FLAG_STORE0
				jmp		i286_memorywrite_w
		}
}

I286 adc_ext16_i(void) {

		__asm {
				CFLAG_LOAD
				adc		dx, bx
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 sbb_ext16_i(void) {

		__asm {
				CFLAG_LOAD
				sbb		dx, bx
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 and_ext16_i(void) {

		__asm {
				and		dx, bx
				FLAG_STORE0
				jmp		i286_memorywrite_w
		}
}

I286 sub_ext16_i(void) {

		__asm {
				sub		dx, bx
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 xor_ext16_i(void) {

		__asm {
				xor		dx, bx
				FLAG_STORE0
				jmp		i286_memorywrite_w
		}
}

I286 cmp_ext16_i(void) {

		__asm {
				I286CLOCKDEC
				cmp		dx, bx
				FLAG_STORE_OF
				ret
		}
}


									// dest: ebp src: bx
const I286TBL op8xreg16_xtable[8] = {
		add_r16_i,		or_r16_i,		adc_r16_i,		sbb_r16_i,
		and_r16_i,		sub_r16_i,		xor_r16_i,		cmp_r16_i};

									// dest: [ecx]=dx  src: bx
const I286TBL op8xext16_xtable[8] = {
		add_ext16_i,	or_ext16_i,		adc_ext16_i,	sbb_ext16_i,
		and_ext16_i,	sub_ext16_i,	xor_ext16_i,	cmp_ext16_i};


// ----- ext16 dst=ax

I286 add_ext16a_i(void) {

		__asm {
				add		dx, ax
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 or_ext16a_i(void) {

		__asm {
				or		dx, ax
				FLAG_STORE0
				jmp		i286_memorywrite_w
		}
}

I286 adc_ext16a_i(void) {

		__asm {
				CFLAG_LOAD
				adc		dx, ax
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 sbb_ext16a_i(void) {

		__asm {
				CFLAG_LOAD
				sbb		dx, ax
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 and_ext16a_i(void) {

		__asm {
				and		dx, ax
				FLAG_STORE0
				jmp		i286_memorywrite_w
		}
}

I286 sub_ext16a_i(void) {

		__asm {
				sub		dx, ax
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 xor_ext16a_i(void) {

		__asm {
				xor		dx, ax
				FLAG_STORE0
				jmp		i286_memorywrite_w
		}
}

I286 cmp_ext16a_i(void) {

		__asm {
				I286CLOCKDEC
				cmp		dx, ax
				FLAG_STORE_OF
				ret
		}
}

									// dest: [ecx]=dx  src: ax
const I286TBL op8xext16_atable[8] = {
		add_ext16a_i,	or_ext16a_i,	adc_ext16a_i,	sbb_ext16a_i,
		and_ext16a_i,	sub_ext16a_i,	xor_ext16a_i,	cmp_ext16a_i};


// ------------------------------------------------------------ sft byte, 1

// ----- reg8

I286 rol_r8_1(void) {

		__asm {
				rol		byte ptr I286_REG[edx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 ror_r8_1(void) {

		__asm {
				ror		byte ptr I286_REG[edx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 rcl_r8_1(void) {

		__asm {
				CFLAG_LOAD
				rcl		byte ptr I286_REG[edx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 rcr_r8_1(void) {

		__asm {
				CFLAG_LOAD
				rcr		byte ptr I286_REG[edx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 shl_r8_1(void) {

		__asm {
				shl		byte ptr I286_REG[edx], 1
				FLAG_STORE_OF
				ret
		}
}

I286 shr_r8_1(void) {

		__asm {
				shr		byte ptr I286_REG[edx], 1
				FLAG_STORE_OF
				ret
		}
}

I286 scl_r8_1(void) {

		__asm {
				shl		byte ptr I286_REG[edx], 1
				mov		cl, I286_FLAGL
				FLAG_STORE_OF
				and		cl, 1
				or		byte ptr I286_REG[edx], cl
				ret
		}
}

I286 sar_r8_1(void) {

		__asm {
				sar		byte ptr I286_REG[edx], 1
				FLAG_STORE0
				ret
		}
}

// ----- mem8

I286 rol_mem8_1(void) {

		__asm {
				rol		byte ptr I286_MEM[ecx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 ror_mem8_1(void) {

		__asm {
				ror		byte ptr I286_MEM[ecx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 rcl_mem8_1(void) {

		__asm {
				CFLAG_LOAD
				rcl		byte ptr I286_MEM[ecx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 rcr_mem8_1(void) {

		__asm {
				CFLAG_LOAD
				rcr		byte ptr I286_MEM[ecx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 shl_mem8_1(void) {

		__asm {
				shl		byte ptr I286_MEM[ecx], 1
				FLAG_STORE_OF
				ret
		}
}

I286 shr_mem8_1(void) {

		__asm {
				shr		byte ptr I286_MEM[ecx], 1
				FLAG_STORE_OF
				ret
		}
}

I286 scl_mem8_1(void) {

		__asm {
				shl		byte ptr I286_MEM[ecx], 1
				mov		dl, I286_FLAGL
				FLAG_STORE_OF
				and		dl, 1
				or		byte ptr I286_REG[ecx], dl
				ret
		}
}

I286 sar_mem8_1(void) {

		__asm {
				sar		byte ptr I286_MEM[ecx], 1
				FLAG_STORE0
				ret
		}
}

// ----- ext8

I286 rol_ext8_1(void) {

		__asm {
				rol		dl, 1
				FLAG_STORE_OC
				jmp		i286_memorywrite
		}
}

I286 ror_ext8_1(void) {

		__asm {
				ror		dl, 1
				FLAG_STORE_OC
				jmp		i286_memorywrite
		}
}

I286 rcl_ext8_1(void) {

		__asm {
				CFLAG_LOAD
				rcl		dl, 1
				FLAG_STORE_OC
				jmp		i286_memorywrite
		}
}

I286 rcr_ext8_1(void) {

		__asm {
				CFLAG_LOAD
				rcr		dl, 1
				FLAG_STORE_OC
				jmp		i286_memorywrite
		}
}

I286 shl_ext8_1(void) {

		__asm {
				shl		dl, 1
				FLAG_STORE_OF
				jmp		i286_memorywrite
		}
}

I286 shr_ext8_1(void) {

		__asm {
				shr		dl, 1
				FLAG_STORE_OF
				jmp		i286_memorywrite
		}
}

I286 scl_ext8_1(void) {

		__asm {
				shl		dl, 1
				mov		al, I286_FLAGL
				FLAG_STORE_OF
				and		al, 1
				or		dl, al
				jmp		i286_memorywrite
		}
}

I286 sar_ext8_1(void) {

		__asm {
				sar		dl, 1
				FLAG_STORE0
				jmp		i286_memorywrite
		}
}


const I286TBL sftreg8_xtable[8] = {
		rol_r8_1,		ror_r8_1,		rcl_r8_1,		rcr_r8_1,
		shl_r8_1,		shr_r8_1,		shl_r8_1,		sar_r8_1};

const I286TBL sftmem8_xtable[8] = {
		rol_mem8_1,		ror_mem8_1,		rcl_mem8_1,		rcr_mem8_1,
		shl_mem8_1,		shr_mem8_1,		shl_mem8_1,		sar_mem8_1};

const I286TBL sftext8_xtable[8] = {
		rol_ext8_1,		ror_ext8_1,		rcl_ext8_1,		rcr_ext8_1,
		shl_ext8_1,		shr_ext8_1,		shl_ext8_1,		sar_ext8_1};


// ------------------------------------------------------------ sft word, 1

// ----- reg16

I286 rol_r16_1(void) {

		__asm {
				rol		word ptr I286_REG[edx*2], 1
				FLAG_STORE_OC
				ret
		}
}

I286 ror_r16_1(void) {

		__asm {
				ror		word ptr I286_REG[edx*2], 1
				FLAG_STORE_OC
				ret
		}
}

I286 rcl_r16_1(void) {

		__asm {
				CFLAG_LOAD
				rcl		word ptr I286_REG[edx*2], 1
				FLAG_STORE_OC
				ret
		}
}

I286 rcr_r16_1(void) {

		__asm {
				CFLAG_LOAD
				rcr		word ptr I286_REG[edx*2], 1
				FLAG_STORE_OC
				ret
		}
}

I286 shl_r16_1(void) {

		__asm {
				shl		word ptr I286_REG[edx*2], 1
				FLAG_STORE_OF
				ret
		}
}

I286 shr_r16_1(void) {

		__asm {
				shr		word ptr I286_REG[edx*2], 1
				FLAG_STORE_OF
				ret
		}
}

I286 scl_r16_1(void) {

		__asm {
				shl		word ptr I286_REG[edx*2], 1
				mov		cl, I286_FLAGL
				FLAG_STORE_OF
				and		cl, 1
				or		byte ptr I286_REG[edx*2], cl
				ret
		}
}

I286 sar_r16_1(void) {

		__asm {
				sar		word ptr I286_REG[edx*2], 1
				FLAG_STORE0
				ret
		}
}

// ----- mem16

I286 rol_mem16_1(void) {

		__asm {
				rol		word ptr I286_MEM[ecx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 ror_mem16_1(void) {

		__asm {
				ror		word ptr I286_MEM[ecx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 rcl_mem16_1(void) {

		__asm {
				CFLAG_LOAD
				rcl		word ptr I286_MEM[ecx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 rcr_mem16_1(void) {

		__asm {
				CFLAG_LOAD
				rcr		word ptr I286_MEM[ecx], 1
				FLAG_STORE_OC
				ret
		}
}

I286 shl_mem16_1(void) {

		__asm {
				shl		word ptr I286_MEM[ecx], 1
				FLAG_STORE_OF
				ret
		}
}

I286 shr_mem16_1(void) {

		__asm {
				shr		word ptr I286_MEM[ecx], 1
				FLAG_STORE_OF
				ret
		}
}

I286 scl_mem16_1(void) {

		__asm {
				shl		word ptr I286_MEM[ecx], 1
				mov		dl, I286_FLAGL
				FLAG_STORE_OF
				and		dl, 1
				or		byte ptr I286_REG[eax], dl
				ret
		}
}

I286 sar_mem16_1(void) {

		__asm {
				sar		word ptr I286_MEM[ecx], 1
				FLAG_STORE0
				ret
		}
}

// ----- ext16

I286 rol_ext16_1(void) {

		__asm {
				rol		dx, 1
				FLAG_STORE_OC
				jmp		i286_memorywrite_w
		}
}

I286 ror_ext16_1(void) {

		__asm {
				ror		dx, 1
				FLAG_STORE_OC
				jmp		i286_memorywrite_w
		}
}

I286 rcl_ext16_1(void) {

		__asm {
				CFLAG_LOAD
				rcl		dx, 1
				FLAG_STORE_OC
				jmp		i286_memorywrite_w
		}
}

I286 rcr_ext16_1(void) {

		__asm {
				CFLAG_LOAD
				rcr		dx, 1
				FLAG_STORE_OC
				jmp		i286_memorywrite_w
		}
}

I286 shl_ext16_1(void) {

		__asm {
				shl		dx, 1
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 shr_ext16_1(void) {

		__asm {
				shr		dx, 1
				FLAG_STORE_OF
				jmp		i286_memorywrite_w
		}
}

I286 scl_ext16_1(void) {

		__asm {
				shl		dx, 1
				mov		al, I286_FLAGL
				FLAG_STORE_OF
				and		al, 1
				or		dl, al
				jmp		i286_memorywrite_w
		}
}

I286 sar_ext16_1(void) {

		__asm {
				sar		dx, 1
				FLAG_STORE0
				jmp		i286_memorywrite_w
		}
}


const I286TBL sftreg16_xtable[8] = {
		rol_r16_1,		ror_r16_1,		rcl_r16_1,		rcr_r16_1,
		shl_r16_1,		shr_r16_1,		shl_r16_1,		sar_r16_1};

const I286TBL sftmem16_xtable[8] = {
		rol_mem16_1,	ror_mem16_1,	rcl_mem16_1,	rcr_mem16_1,
		shl_mem16_1,	shr_mem16_1,	shl_mem16_1,	sar_mem16_1};

const I286TBL sftext16_xtable[8] = {
		rol_ext16_1,	ror_ext16_1,	rcl_ext16_1,	rcr_ext16_1,
		shl_ext16_1,	shr_ext16_1,	shl_ext16_1,	sar_ext16_1};


// ------------------------------------------------------------ sft byte, cl

// ----- reg8

I286 rol_r8_cl(void) {

		__asm {
				rol		byte ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 ror_r8_cl(void) {

		__asm {
				ror		byte ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 rcl_r8_cl(void) {

		__asm {
				CFLAG_LOAD
				rcl		byte ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 rcr_r8_cl(void) {

		__asm {
				CFLAG_LOAD
				rcr		byte ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 shl_r8_cl(void) {

		__asm {
				shl		byte ptr [edx], cl
				FLAG_STORE_OF
				ret
		}
}

I286 shr_r8_cl(void) {

		__asm {
				shr		byte ptr [edx], cl
				FLAG_STORE_OF
				ret
		}
}

I286 scl_r8_cl(void) {

		__asm {
				shl		byte ptr [edx], cl
				mov		cl, I286_FLAGL
				FLAG_STORE_OF
				and		cl, 1
				or		[edx], cl
				ret
		}
}

I286 sar_r8_cl(void) {

		__asm {
				sar		byte ptr [edx], cl
				FLAG_STORE0
				ret
		}
}

// ----- ext8

I286 rol_ext8_cl(void) {

		__asm {
				rol		dl, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 ror_ext8_cl(void) {

		__asm {
				ror		dl, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 rcl_ext8_cl(void) {

		__asm {
				CFLAG_LOAD
				rcl		dl, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 rcr_ext8_cl(void) {

		__asm {
				CFLAG_LOAD
				rcr		dl, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 shl_ext8_cl(void) {

		__asm {
				shl		dl, cl
				FLAG_STORE_OF
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 shr_ext8_cl(void) {

		__asm {
				shr		dl, cl
				FLAG_STORE_OF
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 scl_ext8_cl(void) {

		__asm {
				shl		dl, cl
				mov		cl, I286_FLAGL
				FLAG_STORE_OF
				and		cl, 1
				or		dl, cl
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}

I286 sar_ext8_cl(void) {

		__asm {
				sar		dl, cl
				FLAG_STORE0
				mov		ecx, ebp
				jmp		i286_memorywrite
		}
}


const I286TBL sftreg8cl_xtable[8] = {
		rol_r8_cl,		ror_r8_cl,		rcl_r8_cl,		rcr_r8_cl,
		shl_r8_cl,		shr_r8_cl,		shl_r8_cl,		sar_r8_cl};

const I286TBL sftext8cl_xtable[8] = {
		rol_ext8_cl,	ror_ext8_cl,	rcl_ext8_cl,	rcr_ext8_cl,
		shl_ext8_cl,	shr_ext8_cl,	shl_ext8_cl,	sar_ext8_cl};


// ------------------------------------------------------------ sft word, cl

// ----- reg16

I286 rol_r16_cl(void) {

		__asm {
				rol		word ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 ror_r16_cl(void) {

		__asm {
				ror		word ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 rcl_r16_cl(void) {

		__asm {
				CFLAG_LOAD
				rcl		word ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 rcr_r16_cl(void) {

		__asm {
				CFLAG_LOAD
				rcr		word ptr [edx], cl
				FLAG_STORE_OC
				ret
		}
}

I286 shl_r16_cl(void) {

		__asm {
				shl		word ptr [edx], cl
				FLAG_STORE_OF
				ret
		}
}

I286 shr_r16_cl(void) {

		__asm {
				shr		word ptr [edx], cl
				FLAG_STORE_OF
				ret
		}
}

I286 scl_r16_cl(void) {

		__asm {
				shl		word ptr [edx], cl
				mov		cl, I286_FLAGL
				FLAG_STORE_OF
				and		cl, 1
				or		[edx], cl
				ret
		}
}

I286 sar_r16_cl(void) {

		__asm {
				sar		word ptr [edx], cl
				FLAG_STORE0
				ret
		}
}

// ----- ext16

I286 rol_ext16_cl(void) {

		__asm {
				rol		dx, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 ror_ext16_cl(void) {

		__asm {
				ror		dx, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 rcl_ext16_cl(void) {

		__asm {
				CFLAG_LOAD
				rcl		dx, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 rcr_ext16_cl(void) {

		__asm {
				CFLAG_LOAD
				rcr		dx, cl
				FLAG_STORE_OC
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 shl_ext16_cl(void) {

		__asm {
				shl		dx, cl
				FLAG_STORE_OF
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 shr_ext16_cl(void) {

		__asm {
				shr		dx, cl
				FLAG_STORE_OF
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 scl_ext16_cl(void) {

		__asm {
				shl		dx, cl
				mov		cl, I286_FLAGL
				FLAG_STORE_OF
				and		cl, 1
				or		dl, cl
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}

I286 sar_ext16_cl(void) {

		__asm {
				sar		dx, cl
				FLAG_STORE0
				mov		ecx, ebp
				jmp		i286_memorywrite_w
		}
}


const I286TBL sftreg16cl_xtable[8] = {
		rol_r16_cl,		ror_r16_cl,		rcl_r16_cl,		rcr_r16_cl,
		shl_r16_cl,		shr_r16_cl,		shl_r16_cl,		sar_r16_cl};

const I286TBL sftext16cl_xtable[8] = {
		rol_ext16_cl,	ror_ext16_cl,	rcl_ext16_cl,	rcr_ext16_cl,
		shl_ext16_cl,	shr_ext16_cl,	shl_ext16_cl,	sar_ext16_cl};


// ------------------------------------------------------------ opecode 0xf6,7

I286 test_ea8_data8(void) {

		__asm {
				PREPART_EA8(2)
					mov		ecx, ebx
					shr		ecx, 8
					test	byte ptr I286_REG[eax], ch
					FLAG_STORE0
					GET_NEXTPRE3
					ret
				MEMORY_EA8(6)
					test	byte ptr I286_MEM[ecx], bl
					FLAG_STORE0
					GET_NEXTPRE1
					ret
				EXTMEM_EA8
					test	al, bl
					FLAG_STORE0
					GET_NEXTPRE1
					ret
		}
}

I286 not_ea8(void) {

		__asm {
				PREPART_EA8(2)
					not		byte ptr I286_REG[eax]
					GET_NEXTPRE2
					ret
				MEMORY_EA8(7)
					not		byte ptr I286_MEM[ecx]
					ret
				EXTMEM_EA8
					mov		dl, al
					not		dl
					jmp		i286_memorywrite
		}
}

I286 neg_ea8(void) {

		__asm {
				PREPART_EA8(2)
					neg		byte ptr I286_REG[eax]
					FLAG_STORE_OF
					GET_NEXTPRE2
					ret
				MEMORY_EA8(7)
					neg		byte ptr I286_MEM[ecx]
					FLAG_STORE_OF
					ret
				EXTMEM_EA8
					mov		edx, eax
					neg		dl
					FLAG_STORE_OF
					jmp		i286_memorywrite
		}
}

I286 mul_ea8(void) {

		__asm {
				PREPART_EA8(13)
					mov		al, byte ptr I286_REG[eax]
					mul		I286_AL
					mov		I286_AX, ax
					FLAG_STORE_OF
					GET_NEXTPRE2
					ret
				MEMORY_EA8(16)
					mov		al, byte ptr I286_MEM[ecx]
					mul		I286_AL
					mov		I286_AX, ax
					FLAG_STORE_OF
					ret
				EXTMEM_EA8
					mul		I286_AL
					mov		I286_AX, ax
					FLAG_STORE_OF
					ret
		}
}

I286 imul_ea8(void) {

		__asm {
				PREPART_EA8(13)
					mov		al, byte ptr I286_REG[eax]
					imul	I286_AL
					mov		I286_AX, ax
					FLAG_STORE_OF
					GET_NEXTPRE2
					ret
				MEMORY_EA8(16)
					mov		al, byte ptr I286_MEM[ecx]
					imul	I286_AL
					mov		I286_AX, ax
					FLAG_STORE_OF
					ret
				EXTMEM_EA8
					imul	I286_AL
					mov		I286_AX, ax
					FLAG_STORE_OF
					ret
		}
}

I286 div_ea8(void) {

		__asm {
				push	esi
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
				pop		eax
				ret

				align	4
	divovf:		pop		esi
				INT_NUM(0)
		}
}

I286 idiv_ea8(void) {

		__asm {
				push	esi
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
				cmp		ax, 0x8000
				je		idivovf
				cwd
				idiv	bp
				mov		I286_AL, al
				mov		I286_AH, dl
				mov		dx, ax
				FLAG_STORE_OF
				bt		dx, 7
				adc		dh, 0
				jne		idivovf
				pop		eax
				ret

				align	4
	idivovf:	pop		esi
				INT_NUM(0)
		}
}

I286 test_ea16_data16(void) {

		__asm {
				PREPART_EA16(2)
					mov		ecx, ebx
					shr		ecx, 16
					test	word ptr I286_REG[eax*2], cx
					FLAG_STORE0
					GET_NEXTPRE4
					ret
				MEMORY_EA16(6)
					test	word ptr I286_MEM[ecx], bx
					FLAG_STORE0
					GET_NEXTPRE2
					ret
				EXTMEM_EA16
					test	ax, bx
					FLAG_STORE0
					GET_NEXTPRE2
					ret
		}
}

I286 not_ea16(void) {

		__asm {
				PREPART_EA16(2)
					not		word ptr I286_REG[eax*2]
					GET_NEXTPRE2
					ret
				MEMORY_EA16(7)
					not		word ptr I286_MEM[ecx]
					ret
				EXTMEM_EA16
					mov		edx, eax
					not		dx
					jmp		i286_memorywrite_w
		}
}

I286 neg_ea16(void) {

		__asm {
				PREPART_EA16(2)
					neg		word ptr I286_REG[eax*2]
					FLAG_STORE_OF
					GET_NEXTPRE2
					ret
				MEMORY_EA16(7)
					neg		word ptr I286_MEM[ecx]
					FLAG_STORE_OF
					ret
				EXTMEM_EA16
					mov		dx, ax
					neg		dx
					FLAG_STORE_OF
					jmp		i286_memorywrite_w
		}
}

I286 mul_ea16(void) {

		__asm {
				PREPART_EA16(21)
					mov		ax, word ptr I286_REG[eax*2]
					mul		I286_AX
					mov		I286_AX, ax
					mov		I286_DX, dx
					FLAG_STORE_OF
					GET_NEXTPRE2
					ret
				MEMORY_EA16(24)
					mov		ax, word ptr I286_MEM[ecx]
					mul		I286_AX
					mov		I286_AX, ax
					mov		I286_DX, dx
					FLAG_STORE_OF
					ret
				EXTMEM_EA16
					mul		I286_AX
					mov		I286_AX, ax
					mov		I286_DX, dx
					FLAG_STORE_OF
					ret
		}
}

I286 imul_ea16(void) {

		__asm {
				PREPART_EA16(21)
					mov		ax, word ptr I286_REG[eax*2]
					imul	I286_AX
					mov		I286_AX, ax
					mov		I286_DX, dx
					FLAG_STORE_OF
					GET_NEXTPRE2
					ret
				MEMORY_EA16(24)
					mov		ax, word ptr I286_MEM[ecx]
					imul	I286_AX
					mov		I286_AX, ax
					mov		I286_DX, dx
					FLAG_STORE_OF
					ret
				EXTMEM_EA16
					imul	I286_AX
					mov		I286_AX, ax
					mov		I286_DX, dx
					FLAG_STORE_OF
					ret
		}
}

I286 div_ea16(void) {

		__asm {
				push	esi
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
				pop		eax
				ret

				align	4
	divovf:		pop		esi
				INT_NUM(0)
		}
}

I286 idiv_ea16(void) {

		__asm {
				push	esi
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
				movzx	edx, I286_DX
				movzx	eax, I286_AX
				shl		edx, 16
				or		eax, edx
				cmp		eax, 0x80000000
				je		idivovf
				cdq
				idiv	ebp
				mov		I286_AX, ax
				mov		I286_DX, dx
				mov		edx, eax
				FLAG_STORE_OF
				shr		edx, 16
				adc		dx, 0
				jne		idivovf
				pop		eax
				ret

				align	4
	idivovf:	pop		esi
				INT_NUM(0)
		}
}

const I286TBL ope0xf6_xtable[8] = {
			test_ea8_data8,		test_ea8_data8,
			not_ea8,			neg_ea8,
			mul_ea8,			imul_ea8,
			div_ea8,			idiv_ea8};

const I286TBL ope0xf7_xtable[8] = {
			test_ea16_data16,	test_ea16_data16,
			not_ea16,			neg_ea16,
			mul_ea16,			imul_ea16,
			div_ea16,			idiv_ea16};


// ------------------------------------------------------------ opecode 0xfe,f

I286 inc_ea8(void) {

		__asm {
				PREPART_EA8(2)
					inc		byte ptr I286_REG[eax]
					FLAG_STORE_NC
					GET_NEXTPRE2
					ret
				MEMORY_EA8(7)
					inc		byte ptr I286_MEM[ecx]
					FLAG_STORE_NC
					ret
				EXTMEM_EA8
					mov		edx, eax
					inc		dl
					FLAG_STORE_NC
					jmp		i286_memorywrite
		}
}

I286 dec_ea8(void) {

		__asm {
				PREPART_EA8(2)
					dec		byte ptr I286_REG[eax]
					FLAG_STORE_NC
					GET_NEXTPRE2
					ret
				MEMORY_EA8(7)
					dec		byte ptr I286_MEM[ecx]
					FLAG_STORE_NC
					ret
				EXTMEM_EA8
					mov		edx, eax
					dec		dl
					FLAG_STORE_NC
					jmp		i286_memorywrite
		}
}

I286 inc_ea16(void) {

		__asm {
				PREPART_EA16(2)
					inc		word ptr I286_REG[eax*2]
					FLAG_STORE_NC
					GET_NEXTPRE2
					ret
				MEMORY_EA16(7)
					inc		word ptr I286_MEM[ecx]
					FLAG_STORE_NC
					ret
				EXTMEM_EA16
					mov		edx, eax
					inc		dx
					FLAG_STORE_NC
					jmp		i286_memorywrite_w
		}
}

I286 dec_ea16(void) {

		__asm {
				PREPART_EA16(2)
					dec		word ptr I286_REG[eax*2]
					FLAG_STORE_NC
					GET_NEXTPRE2
					ret
				MEMORY_EA16(7)
					dec		word ptr I286_MEM[ecx]
					FLAG_STORE_NC
					ret
				EXTMEM_EA16
					mov		edx, eax
					dec		dx
					FLAG_STORE_NC
					jmp		i286_memorywrite_w
		}
}

I286 call_ea16(void) {

		__asm {
				sub		I286_SP, 2
				PREPART_EA16(7)
					add		si, 2
					mov		dx, word ptr I286_REG[eax*2]
					xchg	dx, si
					RESET_XPREFETCH
					movzx	ecx, I286_SP
					add		ecx, SS_BASE
					jmp		i286_memorywrite_w
				MEMORY_EA16(11)
					mov		dx, word ptr I286_MEM[ecx]
					xchg	dx, si
					RESET_XPREFETCH
					movzx	ecx, I286_SP
					add		ecx, SS_BASE
					jmp		i286_memorywrite_w
				EXTMEM_EA16
					mov		dx, si
					mov		si, ax
					RESET_XPREFETCH
					movzx	ecx, I286_SP
					add		ecx, SS_BASE
					jmp		i286_memorywrite_w
		}
}

I286 call_far_ea16(void) {

		__asm {
				cmp		al, 0c0h
				jnc		register_eareg16
				I286CLOCK(16)
				call	p_get_ea[eax*4]
				push	edi
				mov		edi, SS_BASE
				movzx	ebx, I286_SP
				mov		dx, I286_CS
				sub		bx, 2
				lea		ecx, [edi + ebx]
				call	i286_memorywrite_w
				mov		dx, si
				sub		bx, 2
				lea		ecx, [edi + ebx]
				call	i286_memorywrite_w
				mov		I286_SP, bx
				pop		edi
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		si, ax
				add		bp, 2
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		I286_CS, ax
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short call_far_pe
				shl		eax, 4					// make segreg
call_far_base:	mov		CS_BASE, eax
				RESET_XPREFETCH
				ret

call_far_pe:	push	offset call_far_base
				jmp		i286x_selector

register_eareg16:
				INT_NUM(6)
		}
}

I286 jmp_ea16(void) {

		__asm {
				PREPART_EA16(7)
					mov		ax, word ptr I286_REG[eax*2]
					mov		si, ax
					RESET_XPREFETCH
					ret
				MEMORY_EA16(11)
					mov		ax, word ptr I286_MEM[ecx]
					mov		si, ax
					RESET_XPREFETCH
					ret
				EXTMEM_EA16
					mov		si, ax
					RESET_XPREFETCH
					ret
		}
}

I286 jmp_far_ea16(void) {

		__asm {
				cmp		al, 0c0h
				jnc		register_eareg16
				I286CLOCK(11)
				call	p_get_ea[eax*4]
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		si, ax
				add		bp, 2
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		I286_CS, ax
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short jmp_far_pe
				shl		eax, 4					// make segreg
jmp_far_base:	mov		CS_BASE, eax
				RESET_XPREFETCH
				ret

jmp_far_pe:		push	offset jmp_far_base
				jmp		i286x_selector

register_eareg16:
				INT_NUM(6)
		}
}

I286 push_ea16(void) {

		__asm {
				sub		I286_SP, 2
				PREPART_EA16(3)
					mov		dx, word ptr I286_REG[eax*2]
					GET_NEXTPRE2
					movzx	ecx, I286_SP
					add		ecx, SS_BASE
					jmp		i286_memorywrite_w
				MEMORY_EA16(5)
					mov		dx, word ptr I286_MEM[ecx]
					movzx	ecx, I286_SP
					add		ecx, SS_BASE
					jmp		i286_memorywrite_w
				EXTMEM_EA16
					mov		dx, ax
					movzx	ecx, I286_SP
					add		ecx, SS_BASE
					jmp		i286_memorywrite_w
		}
}

I286 pop_ea16(void) {

		__asm {
				I286CLOCK(5)
				push	eax
				movzx	ecx, I286_SP
				add		I286_SP, 2
				add		ecx, SS_BASE
				call	i286_memoryread_w
				mov		edx, eax
				pop		eax
				cmp		al, 0c0h
				jnc		src_register
				call	p_ea_dst[eax*4]
				jmp		i286_memorywrite_w
				align	4
		src_register:
				and		eax, 7
				mov		word ptr I286_REG[eax*2], dx
				GET_NEXTPRE2
				ret
		}
}


const I286TBL ope0xfe_xtable[2] = {
			inc_ea8,			dec_ea8};

const I286TBL ope0xff_xtable[8] = {
			inc_ea16,			dec_ea16,
			call_ea16,			call_far_ea16,
			jmp_ea16,			jmp_far_ea16,
			push_ea16,			pop_ea16};

