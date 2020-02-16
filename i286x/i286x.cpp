#include	"compiler.h"
#include	"cpucore.h"
#include	"i286x.h"
#include	"i286xadr.h"
#include	"i286xs.h"
#include	"i286xrep.h"
#include	"i286xcts.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"i286x.mcr"
#include	"i286xea.mcr"
#include	"v30patch.h"
#include	"bios/bios.h"
#include	"dmap.h"
#if defined(ENABLE_TRAP)
#include "trap/inttrap.h"
#include "trap/steptrap.h"
#endif


	I286CORE	i286core;

const UINT8 iflags[256] = {					// Z_FLAG, S_FLAG, P_FLAG
			0x44, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84};


void i286x_initialize(void) {

	i286xadr_init();
	if (CPU_TYPE == CPUTYPE_V30) {
		v30xinit();
	}
}

void i286x_deinitialize(void) {

	if (CPU_EXTMEM) {
		_MFREE(CPU_EXTMEM);
		CPU_EXTMEM = NULL;
		CPU_EXTMEMSIZE = 0;
	}
}

static void i286x_initreg(void) {
									// V30はV30PATCH.CPPに別途用意
	I286_CS = 0xf000;
	CS_BASE = 0xf0000;
	I286_IP = 0xfff0;
	i286core.s.adrsmask = 0xfffff;
	i286x_resetprefetch();
}

#if defined(VAEG_FIX)
void i286x_reset(void) {
	ZeroMemory(&i286core.s, sizeof(i286core.s));
	if (CPU_TYPE == CPUTYPE_V30) {
		v30x_initreg();
	}
	else {
		i286x_initreg();
	}
}
#else
void i286x_reset(void) {
	ZeroMemory(&i286core.s, sizeof(i286core.s));
	i286x_initreg();
}
#endif

void i286x_shut(void) {

	ZeroMemory(&i286core.s, offsetof(I286STAT, cpu_type));
	i286x_initreg();
}

void i286x_setextsize(UINT32 size) {

	if (CPU_EXTMEMSIZE != size) {
		if (CPU_EXTMEM) {
			_MFREE(CPU_EXTMEM);
			CPU_EXTMEM = NULL;
		}
		if (size) {
			CPU_EXTMEM = (UINT8 *)_MALLOC(size + 16, "EXTMEM");
			if (CPU_EXTMEM == NULL) {
				size = 0;
			}
		}
		CPU_EXTMEMSIZE = size;
	}
	i286core.e.ems[0] = mem + 0xc0000;
	i286core.e.ems[1] = mem + 0xc4000;
	i286core.e.ems[2] = mem + 0xc8000;
	i286core.e.ems[3] = mem + 0xcc000;
}

void i286x_setemm(UINT frame, UINT32 addr) {

	UINT8	*ptr;

	frame &= 3;
	if (addr < USE_HIMEM) {
		ptr = mem + addr;
	}
	else if ((addr - 0x100000 + 0x4000) <= CPU_EXTMEMSIZE) {
		ptr = CPU_EXTMEM + (addr - 0x100000);
	}
	else {
		ptr = mem + 0xc0000 + (frame << 14);
	}
	i286core.e.ems[frame] = ptr;
}


LABEL void i286x_resetprefetch(void) {

	__asm {
				pushad
				movzx	esi, I286_IP
				RESET_XPREFETCH
				mov		dword ptr (i286core.s.prefetchque), ebx
				popad
				ret
	}
}

LABEL void __fastcall i286x_interrupt(UINT8 vect) {

	__asm {
				pushad
				push	ecx

				movzx	ebx, I286_SP
				sub		bx, 2

				// hlt..
				cmp		byte ptr (i286core.s.prefetchque), 0f4h		// hlt
				jne		short nonhlt
				inc		I286_IP
nonhlt:			mov		edi, SS_BASE
				lea		ecx, [edi + ebx]
				sub		bx, 2
				mov		dx, I286_FLAG
				and		dh, 0fh
				and		I286_FLAG, not (T_FLAG or I_FLAG)
				mov		I286_TRAP, 0
				call	i286_memorywrite_w
				lea		ecx, [edi + ebx]
				sub		bx, 2
				mov		dx, I286_CS
				call	i286_memorywrite_w
				mov		I286_SP, bx
				lea		ecx, [edi + ebx]
				mov		dx, I286_IP
				call	i286_memorywrite_w
				pop		ecx

				xor		eax, eax
				mov		al, cl
				mov		eax, dword ptr I286_MEM[eax*4]

				mov		I286_IP, ax
				movzx	esi, ax
				shr		eax, 16
				mov		I286_CS, ax
				shl		eax, 4					// make segreg
				mov		CS_BASE, eax
				RESET_XPREFETCH
				mov		dword ptr (i286core.s.prefetchque), ebx

				popad
				ret
		}
}


// I286xルーチンのローカルからの割り込み
LABEL void __fastcall i286x_localint(void) {

	__asm {
				push	ecx
				movzx	ebx, I286_SP
				sub		bx, 2

				mov		edi, SS_BASE
				lea		ecx, [edi + ebx]
				sub		bx, 2
				mov		dx, I286_FLAG
				and		dh, 0fh
				and		I286_FLAG, not (T_FLAG or I_FLAG)
				mov		I286_TRAP, 0
				call	i286_memorywrite_w
				lea		ecx, [edi + ebx]
				sub		bx, 2
				mov		dx, I286_CS
				call	i286_memorywrite_w
				mov		I286_SP, bx
				lea		ecx, [edi + ebx]
				mov		dx, si							// I286_IP
				call	i286_memorywrite_w
				pop		ecx
				mov		eax, dword ptr I286_MEM[ecx*4]

				mov		si, ax
				shr		eax, 16
				mov		I286_CS, ax
				shl		eax, 4					// make segreg
				mov		CS_BASE, eax

				RESET_XPREFETCH
				ret
		}
}


// プロテクトモードのセレクタ(in ax / ret eax)
LABEL void __fastcall i286x_selector(void) {

	__asm {
				mov		ecx, dword ptr (I286_GDTR.base)
				test	eax, 4
				je		short ixsl_1
				mov		ecx, dword ptr (I286_LDTRC.base)
ixsl_1:			and		eax, not 7
				and		ecx, 0ffffffh
				lea		ecx, [ecx + eax + 2]
				call	i286_memoryread_w
				push	eax
				add		ecx, 2
				call	i286_memoryread
				pop		ecx
				and		eax, 0ffh
				movzx	ecx, cx
				shl		eax, 16
				add		eax, ecx
				ret
		}
}


LABEL void i286x(void) {

	__asm {
				pushad
				mov		ebx, dword ptr (i286core.s.prefetchque)
				movzx	esi, I286_IP

				cmp		I286_TRAP, 0
				jne		short i286_trapping
				cmp		dmac.working, 0
				jne		short i286_dma_mnlp

i286_mnlp:
#if defined(ENABLE_TRAP)
				mov		edx, esi
				movzx	ecx, I286_CS
				call	steptrap
#endif
				movzx	eax, bl
				call	i286op[eax*4]
				cmp		I286_REMCLOCK, 0
				jg		i286_mnlp
				mov		dword ptr (i286core.s.prefetchque), ebx
				mov		I286_IP, si
				popad
				ret

				align	16
i286_dma_mnlp:
#if defined(ENABLE_TRAP)
				mov		edx, esi
				movzx	ecx, I286_CS
				call	steptrap
#endif
				movzx	eax, bl
				call	i286op[eax*4]
				call	dmap_i286
				cmp		I286_REMCLOCK, 0
				jg		i286_dma_mnlp
				mov		dword ptr (i286core.s.prefetchque), ebx
				mov		I286_IP, si
				popad
				ret

				align	16
i286_trapping:
#if defined(ENABLE_TRAP)
				mov		edx, esi
				movzx	ecx, I286_CS
				call	steptrap
#endif
				movzx	eax, bl
				call	i286op[eax*4]
				cmp		I286_TRAP, 0
				je		i286notrap
				mov		ecx, 1
				call	i286x_localint
i286notrap:		mov		dword ptr (i286core.s.prefetchque), ebx
				mov		I286_IP, si
				popad
				ret
	}
}



LABEL void i286x_step(void) {

	__asm {
				pushad
				mov		ebx, dword ptr (i286core.s.prefetchque)
				movzx	esi, I286_IP

#if defined(ENABLE_TRAP)
				mov		edx, esi
				movzx	ecx, I286_CS
				call	steptrap
#endif

				movzx	eax, bl
				call	i286op[eax*4]

				cmp		I286_TRAP, 0
				je		short nexts
				mov		ecx, 1
				call	i286x_localint
nexts:
				mov		dword ptr (i286core.s.prefetchque), ebx
				mov		I286_IP, si

				call	dmap_i286
				popad
				ret
		}
}




LABEL void removeprefix(void) {

		__asm {
				mov		i286core.s.prefix, 0
				mov		eax, DS_BASE
				mov		DS_FIX, eax
				mov		eax, SS_BASE
				mov		SS_FIX, eax
				ret
		}
}


I286 _reserved(void) {

		__asm {
//				inc		si						// 01/08/31
				INT_NUM(6)
		}
}

// ----

I286 add_ea_r8(void) {							// 00: add EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					add		byte ptr I286_REG[eax], dl
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(7)
				MEMORY_EA_REG8_X(7, 16)
					add		byte ptr I286_MEM[ecx], dl
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG8
					add		al, byte ptr I286_REG[ebp]
					mov		dl, al
					FLAG_STORE_OF
					jmp		i286_memorywrite
		}
}

I286 add_ea_r16(void) {							// 01: add EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					add		word ptr I286_REG[eax*2], dx
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(7)
				MEMORY_EA_REG16_X(7, 16)
					add		word ptr I286_MEM[ecx], dx
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG16
					add		ax, word ptr I286_REG[ebp]
					mov		dx, ax
					FLAG_STORE_OF
					jmp		i286_memorywrite_w
		}
}

I286 add_r8_ea(void) {							// 02: add REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 7)
				PREPART_REG8_EA_X(2, 7,  2, 16)
				add		I286_REG[ebp], al
				FLAG_STORE_OF
				ret
		}
}

I286 add_r16_ea(void) {							// 03: add REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 7)
				PREPART_REG16_EA_X(2, 7,  2, 16)
				add		I286_REG[ebp], ax
				FLAG_STORE_OF
				ret
		}
}

I286 add_al_data8(void) {						// 04: add al, DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				add		I286_AL, bh
				FLAG_STORE_OF
				GET_NEXTPRE2
				ret
		}
}

I286 add_ax_data16(void) {						// 05: add ax, DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE3a
				add		I286_AX, bx
				FLAG_STORE_OF
				GET_NEXTPRE3b
				ret
		}
}

I286 push_es(void) {							// 06: push es

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_ES)
		}
}

I286 pop_es(void) {								// 07: pop es

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_ES)
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short pop_es_pe
				shl		eax, 4					// make segreg
pop_es_base:	mov		ES_BASE, eax
				GET_NEXTPRE1
				ret

pop_es_pe:		push	offset pop_es_base
				jmp		i286x_selector
		}
}

I286 or_ea_r8(void) {							// 08: or EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					or		byte ptr I286_REG[eax], dl
					FLAG_STORE0
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(7)
				MEMORY_EA_REG8_X(7, 16)
					or		byte ptr I286_MEM[ecx], dl
					FLAG_STORE0
					ret
				EXTMEM_EA_REG8
					or		al, byte ptr I286_REG[ebp]
					mov		dl, al
					FLAG_STORE0
					jmp		i286_memorywrite
		}
}

I286 or_ea_r16(void) {							// 09: or EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					or		word ptr I286_REG[eax*2], dx
					FLAG_STORE0
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(7)
				MEMORY_EA_REG16_X(7, 16)
					or		word ptr I286_MEM[ecx], dx
					FLAG_STORE0
					ret
				EXTMEM_EA_REG16
					or		ax, word ptr I286_REG[ebp]
					mov		dx, ax
					FLAG_STORE0
					jmp		i286_memorywrite_w
	}
}

I286 or_r8_ea(void) {							// 0A: or REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 7)
				PREPART_REG8_EA_X(2, 7,  2, 11)
				or		I286_REG[ebp], al
				FLAG_STORE0
				ret
		}
}

I286 or_r16_ea(void) {							// 0B: or REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 7)
				PREPART_REG16_EA_X(2, 7,  2, 11)
				or		I286_REG[ebp], ax
				FLAG_STORE0
				ret
		}
}

I286 or_al_data8(void) {						// 0C: or al, DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				or		I286_AL, bh
				FLAG_STORE0
				GET_NEXTPRE2
				ret
		}
}

I286 or_ax_data16(void) {						// 0D: or ax, DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE3a
				or		I286_AX, bx
				FLAG_STORE0
				GET_NEXTPRE3b
				ret
		}
}

I286 push_cs(void) {							// 0E: push es

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_CS)
		}
}


I286 adc_ea_r8(void) {							// 10: adc EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					CFLAG_LOAD
					adc		byte ptr I286_REG[eax], dl
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(7)
				MEMORY_EA_REG8_X(7, 16)
					CFLAG_LOAD
					adc		byte ptr I286_MEM[ecx], dl
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG8
					CFLAG_LOAD
					adc		al, byte ptr I286_REG[ebp]
					mov		dl, al
					FLAG_STORE_OF
					jmp		i286_memorywrite
		}
}

I286 adc_ea_r16(void) {							// 11: adc EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					CFLAG_LOAD
					adc		word ptr I286_REG[eax*2], dx
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(7)
				MEMORY_EA_REG16_X(7, 16)
					CFLAG_LOAD
					adc		word ptr I286_MEM[ecx], dx
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG16
					CFLAG_LOAD
					adc		ax, word ptr I286_REG[ebp]
					mov		dx, ax
					FLAG_STORE_OF
					jmp		i286_memorywrite_w
		}
}

I286 adc_r8_ea(void) {							// 12: adc REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 7)
				PREPART_REG8_EA_X(2, 7,  2, 11)
				CFLAG_LOAD
				adc		I286_REG[ebp], al
				FLAG_STORE_OF
				ret
		}
}

I286 adc_r16_ea(void) {							// 13: adc REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 7)
				PREPART_REG16_EA_X(2, 7,  2, 11)
				CFLAG_LOAD
				adc		I286_REG[ebp], ax
				FLAG_STORE_OF
				ret
		}
}

I286 adc_al_data8(void) {						// 14: adc al, DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				CFLAG_LOAD
				adc		I286_AL, bh
				FLAG_STORE_OF
				GET_NEXTPRE2
				ret
		}
}

I286 adc_ax_data16(void) {						// 15: adc ax, DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE3a
				CFLAG_LOAD
				adc		I286_AX, bx
				FLAG_STORE_OF
				GET_NEXTPRE3b
				ret
		}
}

I286 push_ss(void) {							// 16: push ss

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_SS)
		}
}

I286 pop_ss(void) {								// 17: pop ss

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_SS)
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short pop_ss_pe
				shl		eax, 4					// make segreg
pop_ss_base:	mov		SS_BASE, eax
				mov		SS_FIX, eax
				cmp		i286core.s.prefix, 0		// 00/06/24
				jne		prefix_exist
		noprefix:
				movzx	ebp, bh
				GET_NEXTPRE1
				jmp		i286op[ebp*4]

pop_ss_pe:		push	offset pop_ss_base
				jmp		i286x_selector

prefix_exist:	pop		eax						// eax<-offset removeprefix
				call	eax
				jmp		noprefix
		}
}

I286 sbb_ea_r8(void) {							// 18: sbb EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					CFLAG_LOAD
					sbb		byte ptr I286_REG[eax], dl
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(7)
				MEMORY_EA_REG8_X(7, 16)
					CFLAG_LOAD
					sbb		byte ptr I286_MEM[ecx], dl
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG8
					CFLAG_LOAD
					sbb		al, byte ptr I286_REG[ebp]
					mov		dl, al
					FLAG_STORE_OF
					jmp		i286_memorywrite
		}
}

I286 sbb_ea_r16(void) {							// 19: sbb EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					CFLAG_LOAD
					sbb		word ptr I286_REG[eax*2], dx
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(7)
				MEMORY_EA_REG16_X(7, 16)
					CFLAG_LOAD
					sbb		word ptr I286_MEM[ecx], dx
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG16
					CFLAG_LOAD
					sbb		ax, word ptr I286_REG[ebp]
					mov		dx, ax
					FLAG_STORE_OF
					jmp		i286_memorywrite_w
		}
}

I286 sbb_r8_ea(void) {							// 1A: sbb REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 7)
				PREPART_REG8_EA_X(2, 7,  2, 11)
				CFLAG_LOAD
				sbb		I286_REG[ebp], al
				FLAG_STORE_OF
				ret
		}
}

I286 sbb_r16_ea(void) {							// 1B: sbb REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 7)
				PREPART_REG16_EA_X(2, 7,  2, 11)
				CFLAG_LOAD
				sbb		I286_REG[ebp], ax
				FLAG_STORE_OF
				ret
		}
}

I286 sbb_al_data8(void) {						// 1C: sbb al, DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				CFLAG_LOAD
				sbb		I286_AL, bh
				FLAG_STORE_OF
				GET_NEXTPRE2
				ret
		}
}

I286 sbb_ax_data16(void) {						// 1D: sbb ax, DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE3a
				CFLAG_LOAD
				sbb		I286_AX, bx
				FLAG_STORE_OF
				GET_NEXTPRE3b
				ret
		}
}

I286 push_ds(void) {							// 1E: push ds

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_DS)
		}
}

I286 pop_ds(void) {								// 1F: pop ds

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_DS)
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short pop_ds_pe
				shl		eax, 4					// make segreg
pop_ds_base:	mov		DS_BASE, eax
				mov		DS_FIX, eax
				GET_NEXTPRE1
				ret

pop_ds_pe:		push	offset pop_ds_base
				jmp		i286x_selector
		}
}

I286 and_ea_r8(void) {							// 20: and EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					and		byte ptr I286_REG[eax], dl
					FLAG_STORE0
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(7)
				MEMORY_EA_REG8_X(7, 16)
					and		byte ptr I286_MEM[ecx], dl
					FLAG_STORE0
					ret
				EXTMEM_EA_REG8
					and		al, byte ptr I286_REG[ebp]
					mov		dl, al
					FLAG_STORE0
					jmp		i286_memorywrite
		}
}

I286 and_ea_r16(void) {							// 21: and EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					and		word ptr I286_REG[eax*2], dx
					FLAG_STORE0
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(7)
				MEMORY_EA_REG16_X(7, 16)
					and		word ptr I286_MEM[ecx], dx
					FLAG_STORE0
					ret
				EXTMEM_EA_REG16
					and		ax, word ptr I286_REG[ebp]
					mov		dx, ax
					FLAG_STORE0
					jmp		i286_memorywrite_w
		}
}

I286 and_r8_ea(void) {							// 22: and REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 7)
				PREPART_REG8_EA_X(2, 7,  2, 11)
				and		I286_REG[ebp], al
				FLAG_STORE0
				ret
		}
}

I286 and_r16_ea(void) {							// 23: and REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 7)
				PREPART_REG16_EA_X(2, 7,  2, 11)
				and		I286_REG[ebp], ax
				FLAG_STORE0
				ret
		}
}

I286 and_al_data8(void) {						// 24: and al, DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				and		I286_AL, bh
				FLAG_STORE0
				GET_NEXTPRE2
				ret
		}
}

I286 and_ax_data16(void) {						// 25: and ax, DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE3a
				and		I286_AX, bx
				FLAG_STORE0
				GET_NEXTPRE3b
				ret
		}
}

I286 segprefix_es(void) {						// 26: es:

		__asm {
				mov		eax, ES_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				I286PREFIX(i286op)
		}
}

I286 _daa(void) {								// 27: daa

		__asm {
				I286CLOCK(3)
				FLAG_LOAD
				mov		ax, I286_AX
				daa
				mov		I286_AX, ax
				FLAG_STORE
				GET_NEXTPRE1
				ret
		}
}

I286 sub_ea_r8(void) {							// 28: sub EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					sub		byte ptr I286_REG[eax], dl
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(7)
				MEMORY_EA_REG8_X(7, 16)
					sub		byte ptr I286_MEM[ecx], dl
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG8
					sub		al, byte ptr I286_REG[ebp]
					mov		dl, al
					FLAG_STORE_OF
					jmp		i286_memorywrite
		}
}

I286 sub_ea_r16(void) {							// 29: sub EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					sub		word ptr I286_REG[eax*2], dx
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(7)
				MEMORY_EA_REG16_X(7, 16)
					sub		word ptr I286_MEM[ecx], dx
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG16
					sub		ax, word ptr I286_REG[ebp]
					mov		dx, ax
					FLAG_STORE_OF
					jmp		i286_memorywrite_w
		}
}

I286 sub_r8_ea(void) {							// 2A: sub REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 7)
				PREPART_REG8_EA_X(2, 7,  2, 11)
				sub		I286_REG[ebp], al
				FLAG_STORE_OF
				ret
		}
}

I286 sub_r16_ea(void) {							// 2B: sub REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 7)
				PREPART_REG16_EA_X(2, 7,  2, 11)
				sub		I286_REG[ebp], ax
				FLAG_STORE_OF
				ret
		}
}

I286 sub_al_data8(void) {						// 2C: sub al, DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				sub		I286_AL, bh
				FLAG_STORE_OF
				GET_NEXTPRE2
				ret
		}
}

I286 sub_ax_data16(void) {						// 2D: sub ax, DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE3a
				sub		I286_AX, bx
				FLAG_STORE_OF
				GET_NEXTPRE3b
				ret
		}
}

I286 segprefix_cs(void) {						// 2E: cs:

		__asm {
				mov		eax, CS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				I286PREFIX(i286op)
		}
}

I286 _das(void) {								// 2F: das

		__asm {
				I286CLOCK(3)
				FLAG_LOAD
				mov		ax, I286_AX
				das
				mov		I286_AX, ax
				FLAG_STORE
				GET_NEXTPRE1
				ret
		}
}


I286 xor_ea_r8(void) {							// 30: xor EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					xor		byte ptr I286_REG[eax], dl
					FLAG_STORE0
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(7)
				MEMORY_EA_REG8_X(7, 16)
					xor		byte ptr I286_MEM[ecx], dl
					FLAG_STORE0
					ret
				EXTMEM_EA_REG8
					xor		al, byte ptr I286_REG[ebp]
					mov		dl, al
					FLAG_STORE0
					jmp		i286_memorywrite
		}
}

I286 xor_ea_r16(void) {							// 31: xor EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					xor		word ptr I286_REG[eax*2], dx
					FLAG_STORE0
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(7)
				MEMORY_EA_REG16_X(7, 16)
					xor		word ptr I286_MEM[ecx], dx
					FLAG_STORE0
					ret
				EXTMEM_EA_REG16
					xor		ax, word ptr I286_REG[ebp]
					mov		dx, ax
					FLAG_STORE0
					jmp		i286_memorywrite_w
		}
}

I286 xor_r8_ea(void) {							// 32: xor REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 7)
				PREPART_REG8_EA_X(2, 7,  2, 11)
				xor		byte ptr I286_REG[ebp], al
				FLAG_STORE0
				ret
		}
}

I286 xor_r16_ea(void) {							// 33: xor REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 7)
				PREPART_REG16_EA_X(2, 7,  2, 11)
				xor		word ptr I286_REG[ebp], ax
				FLAG_STORE0
				ret
		}
}

I286 xor_al_data8(void) {						// 34: xor al, DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				xor		I286_AL, bh
				FLAG_STORE0
				GET_NEXTPRE2
				ret
		}
}

I286 xor_ax_data16(void) {						// 35: xor ax, DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE3a
				xor		I286_AX, bx
				FLAG_STORE0
				GET_NEXTPRE3b
				ret
		}
}

I286 segprefix_ss(void) {						// 36: ss:

		__asm {
				mov		eax, SS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				I286PREFIX(i286op)
		}
}

I286 _aaa(void) {								// 37: aaa

		__asm {
				I286CLOCK(3)
				FLAG_LOAD
				mov		ax, I286_AX
				aaa
				mov		I286_AX, ax
				FLAG_STORE
				GET_NEXTPRE1
				ret
		}
}

I286 cmp_ea_r8(void) {							// 38: cmp EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					cmp		byte ptr I286_REG[eax], dl
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(6)
				MEMORY_EA_REG8_X(6, 11)
					cmp		byte ptr I286_MEM[ecx], dl
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG8
					cmp		al, byte ptr I286_REG[ebp]
					FLAG_STORE_OF
					ret
		}
}

I286 cmp_ea_r16(void) {							// 39: cmp EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					cmp		I286_REG[eax*2], dx
					FLAG_STORE_OF
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(6)
				MEMORY_EA_REG16_X(6, 11)
					cmp		word ptr I286_MEM[ecx], dx
					FLAG_STORE_OF
					ret
				EXTMEM_EA_REG16
					cmp		ax, I286_REG[ebp]
					FLAG_STORE_OF
					ret
		}
}

I286 cmp_r8_ea(void) {							// 3A: cmp REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 6)
				PREPART_REG8_EA_X(2, 6,  2, 11)
				cmp		I286_REG[ebp], al
				FLAG_STORE_OF
				ret
		}
}

I286 cmp_r16_ea(void) {							// 3B: cmp REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 6)
				PREPART_REG16_EA_X(2, 6,  2, 11)
				cmp		I286_REG[ebp], ax
				FLAG_STORE_OF
				ret
		}
}

I286 cmp_al_data8(void) {						// 3C: cmp al, DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				cmp		I286_AL, bh
				FLAG_STORE_OF
				GET_NEXTPRE2
				ret
		}
}

I286 cmp_ax_data16(void) {						// 3D: cmp ax, DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE3a
				cmp		I286_AX, bx
				FLAG_STORE_OF
				GET_NEXTPRE3b
				ret
		}
}

I286 segprefix_ds(void) {						// 3E: ds:

		__asm {
				mov		eax, DS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				I286PREFIX(i286op)
		}
}

I286 _aas(void) {								// 3F: aas

		__asm {
				I286CLOCK(3)
				FLAG_LOAD
				mov		ax, I286_AX
				aas
				mov		I286_AX, ax
				FLAG_STORE
				GET_NEXTPRE1
				ret
		}
}


I286 inc_ax(void) {								// 40: inc ax

		__asm {
				I286CLOCK(2)
				inc		I286_AX
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 inc_cx(void) {								// 41: inc cx

		__asm {
				I286CLOCK(2)
				inc		I286_CX
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 inc_dx(void) {								// 42: inc dx

		__asm {
				I286CLOCK(2)
				inc		I286_DX
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 inc_bx(void) {								// 43: inc bx

		__asm {
				I286CLOCK(2)
				inc		I286_BX
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 inc_sp(void) {								// 44: inc sp

		__asm {
				I286CLOCK(2)
				inc		I286_SP
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 inc_bp(void) {								// 45: inc bp

		__asm {
				I286CLOCK(2)
				inc		I286_BP
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 inc_si(void) {								// 46: inc si

		__asm {
				I286CLOCK(2)
				inc		I286_SI
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 inc_di(void) {								// 47: inc di

		__asm {
				I286CLOCK(2)
				inc		I286_DI
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 dec_ax(void) {								// 48: dec ax

		__asm {
				I286CLOCK(2)
				dec		I286_AX
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 dec_cx(void) {								// 49: dec cx

		__asm {
				I286CLOCK(2)
				dec		I286_CX
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 dec_dx(void) {								// 4a: dec dx

		__asm {
				I286CLOCK(2)
				dec		I286_DX
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 dec_bx(void) {								// 4b: dec bx

		__asm {
				I286CLOCK(2)
				dec		I286_BX
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 dec_sp(void) {								// 4c: dec sp

		__asm {
				I286CLOCK(2)
				dec		I286_SP
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 dec_bp(void) {								// 4d: dec bp

		__asm {
				I286CLOCK(2)
				dec		I286_BP
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 dec_si(void) {								// 4e: dec si

		__asm {
				I286CLOCK(2)
				dec		I286_SI
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}

I286 dec_di(void) {								// 4f: dec di

		__asm {
				I286CLOCK(2)
				dec		I286_DI
				FLAG_STORE_NC
				GET_NEXTPRE1
				ret
		}
}


I286 push_ax(void) {							// 50: push ax

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_AX)
		}
}

I286 push_cx(void) {							// 51: push cx

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_CX)
		}
}

I286 push_dx(void) {							// 52: push dx

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_DX)
		}
}

I286 push_bx(void) {							// 53: push bx

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_BX)
		}
}

I286 push_sp(void) {							// 54: push sp

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_SP)
		}
}

I286 push_bp(void) {							// 55: push bp

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_BP)
		}
}

I286 push_si(void) {							// 56: push si

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_SI)
		}
}

I286 push_di(void) {							// 57: push di

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				REGPUSH1(I286_DI)
		}
}

I286 pop_ax(void) {								// 58: pop ax

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_AX)
				GET_NEXTPRE1
				ret
		}
}

I286 pop_cx(void) {								// 59: pop cx

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_CX)
				GET_NEXTPRE1
				ret
		}
}

I286 pop_dx(void) {								// 5A: pop dx

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_DX)
				GET_NEXTPRE1
				ret
		}
}

I286 pop_bx(void) {								// 5B: pop bx

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_BX)
				GET_NEXTPRE1
				ret
		}
}

I286 pop_sp(void) {								// 5C: pop sp

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				movzx	ecx, I286_SP
				add		ecx, SS_BASE
				call	i286_memoryread_w
				mov		I286_SP, ax
				GET_NEXTPRE1
				ret
		}
}

I286 pop_bp(void) {								// 5D: pop bp

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_BP)
				GET_NEXTPRE1
				ret
		}
}

I286 pop_si(void) {								// 5E: pop si

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_SI)
				GET_NEXTPRE1
				ret
		}
}

I286 pop_di(void) {								// 5F: pop di

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				REGPOP(I286_DI)
				GET_NEXTPRE1
				ret
		}
}


I286 _pusha(void) {								// 60: pusha

		__asm {
				movzx	ebp, I286_SP
				sub		bp, 2

				GET_NEXTPRE1
				//I286CLOCK(17)
				I286CLOCK_X(a, 17, 35)
				mov		edi, SS_BASE

				lea		ecx, [edi + ebp]
				sub		bp, 2
				mov		dx, I286_AX
				call	i286_memorywrite_w

				lea		ecx, [edi + ebp]
				sub		bp, 2
				mov		dx, I286_CX
				call	i286_memorywrite_w

				lea		ecx, [edi + ebp]
				sub		bp, 2
				mov		dx, I286_DX
				call	i286_memorywrite_w

				lea		ecx, [edi + ebp]
				sub		bp, 2
				mov		dx, I286_BX
				call	i286_memorywrite_w

				lea		ecx, [edi + ebp]
				sub		bp, 2
				mov		dx, I286_SP
				call	i286_memorywrite_w

				lea		ecx, [edi + ebp]
				sub		bp, 2
				mov		dx, I286_BP
				call	i286_memorywrite_w

				lea		ecx, [edi + ebp]
				sub		bp, 2
				mov		dx, I286_SI
				call	i286_memorywrite_w

				mov		I286_SP, bp
				lea		ecx, [edi + ebp]
				mov		dx, I286_DI
				jmp		i286_memorywrite_w
		}
}

I286 _popa(void) {								// 61: popa

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(19)
				I286CLOCK_X(a, 19, 43)
				mov		edi, SS_BASE
				movzx	ebp, I286_SP

				lea		ecx, [edi + ebp]
				add		bp, 2
				call	i286_memoryread_w
				mov		I286_DI, ax

				lea		ecx, [edi + ebp]
				add		bp, 2
				call	i286_memoryread_w
				mov		I286_SI, ax

				lea		ecx, [edi + ebp]
				add		bp, 4
				call	i286_memoryread_w
				mov		I286_BP, ax

				lea		ecx, [edi + ebp]
				add		bp, 2
				call	i286_memoryread_w
				mov		I286_BX, ax

				lea		ecx, [edi + ebp]
				add		bp, 2
				call	i286_memoryread_w
				mov		I286_DX, ax

				lea		ecx, [edi + ebp]
				add		bp, 2
				call	i286_memoryread_w
				mov		I286_CX, ax

				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		I286_AX, ax
				add		I286_SP, 16
				ret
		}
}

I286 _bound(void) {

		__asm {
				movzx	eax, bh
				mov		ebp, 6
				cmp		al, 0c0h
				jnc		bountint
				mov		ebp, eax
				shr		ebp, 3-1
				and		ebp, 7*2
				I286CLOCK(13)
				call	p_ea_dst[eax*4]
				call	i286_memoryread_w
				cmp		I286_REG[ebp], ax
				jb		bounttrue
				add		ecx, 2
				call	i286_memoryread_w
				cmp		I286_REG[ebp], ax
				ja		bounttrue
				ret

				align	16
bounttrue:		mov		ebp, 5
bountint:		INT_NUM(ebp)
		}
}

I286 _arpl(void) {

		__asm {
				xor		eax, eax
				cmp		bh, 0c0h
				setc	al
		//		add		si, ax
				add		eax, 10
				I286CLOCK(eax)
				INT_NUM(6)
		}
}

I286 push_data16(void) {						// 68: push DATA16

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 7)
				GET_NEXTPRE3a
				REGPUSH(bx)
				GET_NEXTPRE3b
				ret
		}
}

I286 imul_reg_ea_data16(void) {					// 69: imul REG, EA, DATA16

		__asm {
				//PREPART_REG16_EA(21, 24)
				PREPART_REG16_EA_X(21, 24,  45, 49)
				imul	bx
				mov		I286_REG[ebp], ax

				mov		al, ah
				lahf
				and		ah, 0feh
				and		I286_FLAGL, 0feh
				or		I286_FLAGL, ah
				rcl		al, 1
				adc		dx, 0
#if defined(VAEG_FIX)
				jz		imulnooverflow
#else
				jne		imulnooverflow
#endif
				or		I286_FLAG, (O_FLAG or C_FLAG)

imulnooverflow:	GET_NEXTPRE2
				ret
		}
}

I286 push_data8(void) {							// 6A: push DATA8

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				mov		al, bh
				cbw
				REGPUSH(ax)

				GET_NEXTPRE2
				ret
		}
}

I286 imul_reg_ea_data8(void) {					// 6B: imul REG, EA, DATA8

		__asm {
				PREPART_REG16_EA(21, 24)
				movsx	edx, bl
				imul	dx
				mov		I286_REG[ebp], ax

				mov		al, ah
				lahf
				and		ah, 0feh
				and		I286_FLAGL, 0feh
				or		I286_FLAGL, ah
				rcl		al, 1
				adc		dx, 0
#if defined(VAEG_FIX)
				jz		imulnooverflow
#else
				jne		imulnooverflow
#endif
				or		I286_FLAG, (O_FLAG or C_FLAG)

imulnooverflow:	GET_NEXTPRE1
				ret
		}
}

I286 _insb(void) {								// 6C: insb

		__asm {
				GET_NEXTPRE1
				I286CLOCK(5)
				movzx	ecx, I286_DX
				call	iocore_inp8
				mov		dl, al
				movzx	ecx, I286_ES
				shl		ecx, 4
				movzx	eax, I286_DI
				add		ecx, eax
				STRING_DIR
				add		I286_DI, ax
				jmp		i286_memorywrite
		}
}

I286 _insw(void) {								// 6D: insw

		__asm {
				GET_NEXTPRE1
				I286CLOCK(5)
				movzx	ecx, I286_DX
				call	iocore_inp16
				mov		dx, ax
				movzx	ecx, I286_ES
				shl		ecx, 4
				movzx	eax, I286_DI
				add		ecx, eax
				STRING_DIRx2
				add		I286_DI, ax
				jmp		i286_memorywrite_w
		}
}

I286 _outsb(void) {								// 6E: outsb

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				call	i286_memoryread
				mov		dl, al
				STRING_DIR
				add		I286_SI, ax
				movzx	ecx, I286_DX
				jmp		iocore_out8
		}
}

I286 _outsw(void) {								// 6F: outsw

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				call	i286_memoryread_w
				mov		dx, ax
				STRING_DIRx2
				add		I286_SI, ax
				movzx	ecx, I286_DX
				jmp		iocore_out16
		}
}


I286 jo_short(void) {							// 70: jo short

		__asm {
				test	I286_FLAG, O_FLAG
				jne		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jno_short(void) {							// 71: jno short

		__asm {
				test	I286_FLAG, O_FLAG
				je		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jc_short(void) {							// 72: jnae/jb/jc short

		__asm {
				test	I286_FLAG, C_FLAG
				jne		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jnc_short(void) {							// 73: jae/jnb/jnc short

		__asm {
				test	I286_FLAG, C_FLAG
				je		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jz_short(void) {							// 74: je/jz short

		__asm {
				test	I286_FLAG, Z_FLAG
				jne		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jnz_short(void) {							// 75: jne/jnz short

		__asm {
				test	I286_FLAG, Z_FLAG
				jne		flagnonjump

				//I286CLOCK(7)					// ジャンプする事が多いと思う
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret

flagnonjump:	//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret
		}
}

I286 jna_short(void) {							// 76: jna/jbe short

		__asm {
				test	I286_FLAG, (Z_FLAG or C_FLAG)
				jne		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 ja_short(void) {							// 77: ja/jnbe short

		__asm {
				test	I286_FLAG, (Z_FLAG or C_FLAG)
				je		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 js_short(void) {							// 78: js short

		__asm {
				test	I286_FLAG, S_FLAG
				jne		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jns_short(void) {							// 79: jns short

		__asm {
				test	I286_FLAG, S_FLAG
				je		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jp_short(void) {							// 7A: jp/jpe short

		__asm {
				test	I286_FLAG, P_FLAG
				jne		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jnp_short(void) {							// 7B: jnp/jpo short

		__asm {
				test	I286_FLAG, P_FLAG
				je		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jl_short(void) {							// 7C: jl/jnge short

		__asm {
				mov		dx, I286_FLAG
				shl		dh, 4
				xor		dl, dh
				js		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jnl_short(void) {							// 7D: jnl/jge short

		__asm {
				mov		dx, I286_FLAG
				shl		dh, 4
				xor		dl, dh
				jns		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jle_short(void) {							// 7E: jle/jng short

		__asm {
				mov		dx, I286_FLAG
				test	dx, Z_FLAG
				jne		flagjump
				shl		dh, 4
				xor		dl, dh
				js		flagjump
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret

				align	16
flagjump:		//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 jnle_short(void) {							// 7F: jg/jnle short

		__asm {
				mov		dx, I286_FLAG
				test	dx, Z_FLAG
				jne		notjump
				shl		dh, 4
				xor		dl, dh
				js		notjump
				//I286CLOCK(7)
				I286CLOCK_X(b, 7, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret

				align	16
notjump:		//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				GET_NEXTPRE2
				ret
		}
}


I286 calc_ea8_i8(void) {							// 80,82: op EA8, DATA8

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		al, 0c0h
				jc		memory_eareg8
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				bt		ax, 2
				rcl		eax, 1
				and		eax, 7
				lea		ebp, I286_REG[eax]
				mov		edx, ebx
				shr		edx, 16
				GET_NEXTPRE3
				jmp		op8xreg8_xtable[edi]
				align	16
		memory_eareg8:
				//I286CLOCK(7)
				I286CLOCK_X(b, 7, 18)
				call	p_ea_dst[eax*4]
				cmp		ecx, I286_MEMWRITEMAX
				jnc		extmem_eareg8
				lea		ebp, I286_MEM[ecx]
				mov		edx, ebx
				GET_NEXTPRE1
				jmp		op8xreg8_xtable[edi]
				align	16
		extmem_eareg8:
				call	i286_memoryread
				mov		dl, al
				call	op8xext8_xtable[edi]
				GET_NEXTPRE1
				ret
	}
}

I286 calc_ea16_i16(void) {							// 81: op EA16, DATA16

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		al, 0c0h
				jc		memory_eareg8
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				and		eax, 7
				lea		ebp, word ptr I286_REG[eax*2]
				mov		edx, ebx
				shr		edx, 16
				GET_NEXTPRE4
				jmp		op8xreg16_xtable[edi]
				align	16
		memory_eareg8:
				//I286CLOCK(7)
				I286CLOCK_X(b, 7, 18)
				call	p_ea_dst[eax*4]
				cmp		ecx, I286_MEMWRITEMAX
				jnc		extmem_eareg8
				lea		ebp, word ptr I286_MEM[ecx]
				mov		edx, ebx
				GET_NEXTPRE2
				jmp		op8xreg16_xtable[edi]
				align	16
		extmem_eareg8:
				call	i286_memoryread_w
				mov		edx, eax
				call	op8xext16_xtable[edi]
				GET_NEXTPRE2
				ret
	}
}

I286 calc_ea16_i8(void) {							// 83: op EA16, DATA8

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		al, 0c0h
				jc		memory_eareg8
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				and		eax, 7
				lea		ebp, I286_REG[eax*2]
				GET_NEXTPRE2
				movsx	edx, bl
				GET_NEXTPRE1
				jmp		op8xreg16_xtable[edi]
				align	16
		memory_eareg8:
				//I286CLOCK(7)
				I286CLOCK_X(b, 7, 18)
				call	p_ea_dst[eax*4]
				cmp		ecx, (I286_MEMWRITEMAX-1)
				jnc		extmem_eareg8
				lea		ebp, I286_MEM[ecx]
				movsx	edx, bl
				GET_NEXTPRE1
				jmp		op8xreg16_xtable[edi]
				align	16
		extmem_eareg8:
				call	i286_memoryread_w
				mov		edx, eax
				movsx	eax, bl
				call	op8xext16_atable[edi]
				GET_NEXTPRE1
				ret
	}
}

I286 test_ea_r8(void) {								// 84: test EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					test	I286_REG[eax], dl
					FLAG_STORE0
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG8(7)
				MEMORY_EA_REG8_X(7, 10)
					test	I286_MEM[ecx], dl
					FLAG_STORE0
					ret
				EXTMEM_EA_REG8
					test	al, I286_REG[ebp]
					FLAG_STORE0
					ret
		}
}

I286 test_ea_r16(void) {							// 85: test EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					test	I286_REG[eax*2], dx
					FLAG_STORE0
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
				//MEMORY_EA_REG16(6)
				MEMORY_EA_REG16_X(6, 10)
					test	word ptr I286_MEM[ecx], dx
					FLAG_STORE0
					ret
				EXTMEM_EA_REG16
					test	ax, I286_REG[ebp]
					FLAG_STORE0
					ret
		}
}

I286 xchg_ea_r8(void) {								// 86: xchg EA, REG8

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3
				bt		di, 2
				rcl		edi, 1
				and		edi, 7
				cmp		al, 0c0h
				jc		memory_eareg8
				bt		ax, 2
				rcl		eax, 1
				and		eax, 7
				I286CLOCK(3)
				mov		dl, I286_REG[edi]
				xchg	dl, I286_REG[eax]
				mov		I286_REG[edi], dl
				GET_NEXTPRE2
				ret
				align	16
		memory_eareg8:
				//I286CLOCK(5)
				I286CLOCK_X(b, 5, 16)
				call	p_ea_dst[eax*4]
				cmp		ecx, I286_MEMREADMAX
				jae		extmem_eareg8
				mov		dl, I286_REG[edi]
				xchg	I286_MEM[ecx], dl
				mov		I286_REG[edi], dl
				ret
				align	16
		extmem_eareg8:
				call	i286_memoryread
				xchg	al, I286_REG[edi]
				mov		dl, al
				jmp		i286_memorywrite
		}
}

I286 xchg_ea_r16(void) {							// 87: xchg EA, REG16

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-1
				and		edi, 7*2
				cmp		al, 0c0h
				jc		memory_eareg16
				and		eax, 7
				I286CLOCK(3)
				mov		dx, I286_REG[edi]
				xchg	dx, I286_REG[eax*2]
				mov		I286_REG[edi], dx
				GET_NEXTPRE2
				ret
				align	16
		memory_eareg16:
				//I286CLOCK(5)
				I286CLOCK_X(b, 5, 16)
				call	p_ea_dst[eax*4]
				cmp		ecx, (I286_MEMREADMAX-1)
				jae		extmem_eareg16
				mov		dx, word ptr I286_MEM[ecx]
				xchg	dx, I286_REG[edi]
				mov		word ptr I286_MEM[ecx], dx
				ret
				align	16
		extmem_eareg16:
				call	i286_memoryread_w
				xchg	ax, I286_REG[edi]
				mov		dx, ax
				jmp		i286_memorywrite_w
		}
}

I286 mov_ea_r8(void) {							// 88: mov EA, REG8

		__asm {
				PREPART_EA_REG8(2)
					mov		I286_REG[eax], dl
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
					align	16
			memory_eareg8:
					//I286CLOCK(3)
					I286CLOCK_X(b, 3, 9)
					call	p_ea_dst[eax*4]
					mov		dl, I286_REG[ebp]
					jmp		i286_memorywrite
		}
}

I286 mov_ea_r16(void) {							// 89: mov EA, REG16

		__asm {
				PREPART_EA_REG16(2)
					mov		I286_REG[eax*2], dx
					GET_NEXTPRE2					// ea_regの regregだけ
					ret
			memory_eareg16:
					//I286CLOCK(3)
					I286CLOCK_X(b, 3, 9)
					call	p_ea_dst[eax*4]
					mov		dx, I286_REG[ebp]
					jmp		i286_memorywrite_w
		}
}

I286 mov_r8_ea(void) {							// 8A: mov REG8, EA

		__asm {
				//PREPART_REG8_EA(2, 5)
				PREPART_REG8_EA_X(2, 5,   2, 11)
				mov		I286_REG[ebp], al
				ret
		}
}

I286 mov_r16_ea(void) {							// 8B: add REG16, EA

		__asm {
				//PREPART_REG16_EA(2, 5)
				PREPART_REG16_EA_X(2, 5,   2, 11)
				mov		I286_REG[ebp], ax
				ret
		}
}

I286 mov_ea_seg(void) {							// 8C: mov EA, segreg

		__asm {
				movzx	eax, bh
				mov		ebp, eax
				shr		ebp, 3-1
				and		ebp, 3*2
				mov		dx, word ptr I286_SEGREG[ebp]
				cmp		al, 0c0h
				jc		memory_eareg16
				I286CLOCK(2)
				and		eax, 7
				mov		word ptr I286_REG[eax*2], dx
				GET_NEXTPRE2
				ret
				align	16
		memory_eareg16:
				//I286CLOCK(3)
				I286CLOCK_X(b, 3, 10)
				call	p_ea_dst[eax*4]
				jmp		i286_memorywrite_w
		}
}

I286 lea_r16_ea(void) {							// 8D: lea REG16, EA

		__asm {
				cmp		bh, 0c0h
				jnc		src_register
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 4)
				movzx	eax, bh
				mov		ebp, eax
				shr		ebp, 3-1
				and		ebp, 7*2
				call	p_lea[eax*4]
				mov		word ptr I286_REG[ebp], ax
				ret
				align	16
		src_register:
				INT_NUM(6)
		}
}

I286 mov_seg_ea(void) {							// 8E: mov segrem, EA

		__asm {
				movzx	eax, bh
				mov		ebp, eax
				shr		ebp, 3-1
				and		ebp, 3*2
#if 1
				cmp		ebp, 1*2
				je		fixcs
#endif
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
				//I286CLOCK(5)
				I286CLOCK_X(b, 5, 11)
				call	p_ea_dst[eax*4]
				call	i286_memoryread_w
		segset:
				mov		word ptr I286_SEGREG[ebp], ax
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short mov_seg_pe
				shl		eax, 4					// make segreg
mov_seg_base:	mov		SEG_BASE[ebp*2], eax
				sub		ebp, 2*2
				jc		short segsetr
				mov		SS_FIX[ebp*2], eax
				je		short setss
		segsetr:ret

		setss:	cmp		i286core.s.prefix, 0	// 00/05/13
				je		noprefix
				pop		eax
				call	eax						// eax<-offset removeprefix
		noprefix:
				movzx	eax, bl
				jmp		i286op[eax*4]

mov_seg_pe:		push	offset mov_seg_base
				jmp		i286x_selector

		fixcs:	INT_NUM(6)
		}
}

I286 pop_ea(void) {								// 8F: pop EA

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 17)
				movzx	ecx, I286_SP
				add		ecx, SS_BASE
				call	i286_memoryread_w
				add		I286_SP, 2
				mov		dx, ax
				movzx	eax, bh
				cmp		al, 0c0h
				jnc		src_register
				call	p_ea_dst[eax*4]
				jmp		i286_memorywrite_w
				align	16
		src_register:
				and		eax, 7
				mov		word ptr I286_REG[eax*2], dx
				GET_NEXTPRE2
				ret
		}
}


I286 _nop(void) {								// 90: nop / bios func

		__asm {
				I286CLOCK(3)
				GET_NEXTPRE1
#if 1
				lea		ecx, [esi - 1]
				add		ecx, CS_BASE
				cmp		ecx, 0f8000h
				jc		biosend
				cmp		ecx, 100000h
				jnc		biosend
				mov		I286_IP, si
				call	biosfunc
				cmp		al, ah
				je		biosend
				movzx	esi, I286_IP
				movzx	eax, I286_ES
				shl		eax, 4
				mov		ES_BASE, eax
				movzx	eax, I286_CS
				shl		eax, 4
				mov		CS_BASE, eax
				movzx	eax, I286_SS
				shl		eax, 4
				mov		SS_BASE, eax
				mov		SS_FIX, eax
				movzx	eax, I286_DS
				shl		eax, 4
				mov		DS_BASE, eax
				mov		DS_FIX, eax
				RESET_XPREFETCH
		biosend:
#endif
				ret
		}
}

I286 xchg_ax_cx(void) {							// 91: xchg ax, cx

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				mov		ax, I286_AX
				xchg	ax, I286_CX
				mov		I286_AX, ax
				ret
		}
}

I286 xchg_ax_dx(void) {							// 92: xchg ax, dx

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				mov		ax, I286_AX
				xchg	ax, I286_DX
				mov		I286_AX, ax
				ret
		}
}

I286 xchg_ax_bx(void) {							// 93: xchg ax, bx

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				mov		ax, I286_AX
				xchg	ax, I286_BX
				mov		I286_AX, ax
				ret
		}
}

I286 xchg_ax_sp(void) {							// 94: xchg ax, sp

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				mov		ax, I286_AX
				xchg	ax, I286_SP
				mov		I286_AX, ax
				ret
		}
}

I286 xchg_ax_bp(void) {							// 95: xchg ax, bp

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				mov		ax, I286_AX
				xchg	ax, I286_BP
				mov		I286_AX, ax
				ret
		}
}

I286 xchg_ax_si(void) {							// 96: xchg ax, si

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				mov		ax, I286_AX
				xchg	ax, I286_SI
				mov		I286_AX, ax
				ret
		}
}

I286 xchg_ax_di(void) {							// 97: xchg ax, di

		__asm {
				GET_NEXTPRE1
				I286CLOCK(3)
				mov		ax, I286_AX
				xchg	ax, I286_DI
				mov		I286_AX, ax
				ret
		}
}

I286 _cbw(void) {								// 98: cbw

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				test	I286_AL, 80h
				setz	I286_AH
				dec		I286_AH
				ret
		}
}

I286 _cwd(void) {								// 99: cwd

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				mov		ax, 80h
				test	I286_AH, al
				setz	al
				dec		ax
				mov		I286_DX, ax
				ret
		}
}

I286 call_far(void) {							// 9A: call far

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(13)
				I286CLOCK_X(a, 13, (21+4))
				mov		edi, SS_BASE
				movzx	ebp, I286_SP
				mov		dx, I286_CS
				sub		bp, 2
				lea		ecx, [edi + ebp]
				call	i286_memorywrite_w
				sub		bp, 2
				lea		ecx, [edi + ebp]
				lea		edx, [esi + 4]
				call	i286_memorywrite_w
				mov		I286_SP, bp
				mov		si, bx
				shr		ebx, 16
				mov		I286_CS, bx
				test	byte ptr (I286_MSW), MSW_PE
				jne		short call_far_pe
				shl		ebx, 4
				mov		CS_BASE, ebx
call_far_base:	RESET_XPREFETCH
				ret

call_far_pe:	mov		eax, ebx
				call	i286x_selector
				mov		CS_BASE, eax
				jmp		short call_far_base
		}
}

I286 _wait(void) {								// 9B: wait

		__asm {
				GET_NEXTPRE1
				ret
		}
}

I286 _pushf(void) {								// 9C: pushf

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				mov		dx, I286_FLAG
				and		dx, 0fffh	// 286
				sub		I286_SP, 2
				movzx	ecx, I286_SP
				add		ecx, SS_BASE
				jmp		i286_memorywrite_w
		}
}

I286 _popf(void) {								// 9D: popf

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				movzx	ecx, I286_SP
				add		ecx, SS_BASE
				call	i286_memoryread_w
				add		I286_SP, 2
				and		ax, 0fffh	// 286
				mov		I286_FLAG, ax
#if defined(VAEG_FIX)
				and		ah, 1
				cmp		ah, 1
#else
				and		ah, 3
				cmp		ah, 3
#endif
				sete	I286_TRAP

				je		irqcheck				// fast_intr
				test	ah, 2
				je		nextop
				mov		al, pic.pi[0 * (type _PICITEM)].imr
				mov		ah, pic.pi[1 * (type _PICITEM)].imr
				not		ax
				test	al, pic.pi[0 * (type _PICITEM)].irr
				jne		irqcheck
				test	ah, pic.pi[1 * (type _PICITEM)].irr
				jne		irqcheck
nextop:			ret

irqcheck:		I286IRQCHECKTERM
		}
}

I286 _sahf(void) {								// 9E: sahf

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 3)
				mov		al, I286_AH
				and		ax, 0fffh	// 286
				mov		I286_FLAGL, al
				ret
		}
}

I286 _lahf(void) {								// 9F: lahf

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				mov		al, I286_FLAGL
				and		ax, 0fffh	// 286
				mov		I286_AH, al
				ret
		}
}


I286 mov_al_m8(void) {							// A0: mov al, m8

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 10)
				mov		ecx, ebx
				shr		ecx, 8
				and		ecx, 0ffffh
				add		ecx, DS_FIX
				call	i286_memoryread
				mov		I286_AL, al
				GET_NEXTPRE3
				ret
		}
}

I286 mov_ax_m16(void) {							// A1: mov ax, m16

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 10)
				mov		ecx, ebx
				shr		ecx, 8
				and		ecx, 0ffffh
				add		ecx, DS_FIX
				call	i286_memoryread_w
				mov		I286_AX, ax
				GET_NEXTPRE3
				ret

		}
}

I286 mov_m8_al(void) {							// A2: mov m8, al

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 9)
				mov		edi, ebx
				shr		edi, 8
				and		edi, 0ffffh
				add		edi, DS_FIX
				GET_NEXTPRE3
				mov		ecx, edi
				mov		dl, I286_AL
				jmp		i286_memorywrite
		}
}

I286 mov_m16_ax(void) {							// A3: mov m16, ax

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 9)
				mov		edi, ebx
				shr		edi, 8
				and		edi, 0ffffh
				add		edi, DS_FIX
				GET_NEXTPRE3
				mov		ecx, edi
				mov		dx, I286_AX
				jmp		i286_memorywrite_w
		}
}

I286 _movsb(void) {								// A4: movsb

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 11)
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				call	i286_memoryread
				mov		dl, al
				movzx	ecx, I286_DI
				add		ecx, ES_BASE
				STRING_DIR
				add		I286_SI, ax
				add		I286_DI, ax
				jmp		i286_memorywrite
		}
}

I286 _movsw(void) {								// A5: movsw

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 11)
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				call	i286_memoryread_w
				mov		dx, ax
				movzx	ecx, I286_DI
				add		ecx, ES_BASE
				STRING_DIRx2
				add		I286_SI, ax
				add		I286_DI, ax
				jmp		i286_memorywrite_w
		}
}

I286 _cmpsb(void) {								// A6: cmpsb

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(8)
				I286CLOCK_X(a, 8, 13)
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				call	i286_memoryread
				mov		dl, al
				movzx	ecx, I286_DI
				add		ecx, ES_BASE
				call	i286_memoryread
				cmp		dl, al
				FLAG_STORE_OF
				STRING_DIR
				add		I286_SI, ax
				add		I286_DI, ax
				ret
		}
}

I286 _cmpsw(void) {								// A7: cmpsw

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(8)
				I286CLOCK_X(a, 8, 13)
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				call	i286_memoryread_w
				mov		edx, eax
				movzx	ecx, I286_DI
				add		ecx, ES_BASE
				call	i286_memoryread_w
				cmp		dx, ax
				FLAG_STORE_OF
				STRING_DIRx2
				add		I286_SI, ax
				add		I286_DI, ax
				ret
		}
}

I286 test_al_data8(void) {						// A8: test al, DATA8

		__asm {
				I286CLOCK(3)
				test	I286_AL, bh
				FLAG_STORE0
				GET_NEXTPRE2
				ret
		}
}

I286 test_ax_data16(void) {						// A9: test ax, DATA16

		__asm {
				I286CLOCK(3)
				GET_NEXTPRE3a
				test	I286_AX, bx
				FLAG_STORE0
				GET_NEXTPRE3b
				ret
		}
}

I286 _stosb(void) {								// AA: stosb

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 7)
				movzx	ecx, I286_DI
				add		ecx, ES_BASE
				STRING_DIR
				add		I286_DI, ax
				mov		dl, I286_AL
				jmp		i286_memorywrite
		}
}

I286 _stosw(void) {								// AB: stosw

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 7)
				movzx	ecx, I286_DI
				add		ecx, ES_BASE
				STRING_DIRx2
				add		I286_DI, ax
				mov		dx, I286_AX
				jmp		i286_memorywrite_w
		}
}

I286 _lodsb(void) {								// AC: lodsb

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 7)
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				call	i286_memoryread
				mov		I286_AL, al
				STRING_DIR
				add		I286_SI, ax
				ret
		}
}

I286 _lodsw(void) {								// AD: lodsw

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 7)
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				call	i286_memoryread_w
				mov		I286_AX, ax
				STRING_DIRx2
				add		I286_SI, ax
				ret
		}
}

I286 _scasb(void) {								// AE: scasb

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(7)
				I286CLOCK_X(a, 7, 7)
				movzx	ecx, I286_DI
				add		ecx, ES_BASE
				call	i286_memoryread
				cmp		I286_AL, al
				FLAG_STORE_OF
				STRING_DIR
				add		I286_DI, ax
				ret
		}
}

I286 _scasw(void) {								// AF: scasw

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(7)
				I286CLOCK_X(a, 7, 7)
				movzx	ecx, I286_DI
				add		ecx, ES_BASE
				call	i286_memoryread_w
				cmp		I286_AX, ax
				FLAG_STORE_OF
				STRING_DIRx2
				add		I286_DI, ax
				ret
		}
}


I286 mov_al_imm(void) {							// B0: mov al, imm8

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				mov		I286_AL, bh
				GET_NEXTPRE2
				ret
		}
}

I286 mov_cl_imm(void) {							// B1: mov cl, imm8

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				mov		I286_CL, bh
				GET_NEXTPRE2
				ret
		}
}

I286 mov_dl_imm(void) {							// B2: mov dl, imm8

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				mov		I286_DL, bh
				GET_NEXTPRE2
				ret
		}
}

I286 mov_bl_imm(void) {							// B3: mov bl, imm8

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				mov		I286_BL, bh
				GET_NEXTPRE2
				ret
		}
}

I286 mov_ah_imm(void) {							// B4: mov ah, imm8

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				mov		I286_AH, bh
				GET_NEXTPRE2
				ret
		}
}

I286 mov_ch_imm(void) {							// B5: mov ch, imm8

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				mov		I286_CH, bh
				GET_NEXTPRE2
				ret
		}
}

I286 mov_dh_imm(void) {							// B6: mov dh, imm8

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				mov		I286_DH, bh
				GET_NEXTPRE2
				ret
		}
}

I286 mov_bh_imm(void) {							// B7: mov bh, imm8

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				mov		I286_BH, bh
				GET_NEXTPRE2
				ret
		}
}

I286 mov_ax_imm(void) {							// B8: mov ax, imm16

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				GET_NEXTPRE3a
				mov		I286_AX, bx
				GET_NEXTPRE3b
				ret
		}
}

I286 mov_cx_imm(void) {							// B9: mov cx, imm16

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				GET_NEXTPRE3a
				mov		I286_CX, bx
				GET_NEXTPRE3b
				ret
		}
}

I286 mov_dx_imm(void) {							// BA: mov dx, imm16

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				GET_NEXTPRE3a
				mov		I286_DX, bx
				GET_NEXTPRE3b
				ret
		}
}

I286 mov_bx_imm(void) {							// BB: mov bx, imm16

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				GET_NEXTPRE3a
				mov		I286_BX, bx
				GET_NEXTPRE3b
				ret
		}
}

I286 mov_sp_imm(void) {							// BC: mov sp, imm16

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				GET_NEXTPRE3a
				mov		I286_SP, bx
				GET_NEXTPRE3b
				ret
		}
}

I286 mov_bp_imm(void) {							// BD: mov bp, imm16

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				GET_NEXTPRE3a
				mov		I286_BP, bx
				GET_NEXTPRE3b
				ret
		}
}

I286 mov_si_imm(void) {							// BE: mov si, imm16

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				GET_NEXTPRE3a
				mov		I286_SI, bx
				GET_NEXTPRE3b
				ret
		}
}

I286 mov_di_imm(void) {							// BF: mov di, imm16

		__asm {
				//I286CLOCK(2)
				I286CLOCK_X(a, 2, 4)
				GET_NEXTPRE3a
				mov		I286_DI, bx
				GET_NEXTPRE3b
				ret
		}
}

I286 shift_ea8_data8(void) {					// C0: shift EA8, DATA8

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
				and		ecx, 31
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE3
				pop		ecx
				jmp		sftreg8cl_xtable[edi]
				align	16
		memory_eareg8:
				I286CLOCK(8)
				call	p_ea_dst[eax*4]
				cmp		ecx, I286_MEMWRITEMAX
				jnc		extmem_eareg8
				lea		edx, I286_MEM[ecx]
				mov		ecx, ebx
				and		ecx, 31
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE1
				pop		ecx
				jmp		sftreg8cl_xtable[edi]
				align	16
		extmem_eareg8:
				call	i286_memoryread
				mov		ebp, ecx
				mov		edx, eax
				mov		ecx, ebx
				and		ecx, 31
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE1
				pop		ecx
				jmp		sftext8cl_xtable[edi]
	}
}

I286 shift_ea16_data8(void) {					// C1: shift EA16, DATA8

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
				and		ecx, 31
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE3
				pop		ecx
				jmp		sftreg16cl_xtable[edi]
				align	16
		memory_eareg16:
				I286CLOCK(8)
				call	p_ea_dst[eax*4]
				cmp		ecx, (I286_MEMWRITEMAX-1)
				jnc		extmem_eareg16
				lea		edx, I286_MEM[ecx]
				mov		ecx, ebx
				and		ecx, 31
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE1
				pop		ecx
				jmp		sftreg16cl_xtable[edi]
				align	16
		extmem_eareg16:
				call	i286_memoryread_w
				mov		ebp, ecx
				mov		edx, eax
				mov		ecx, ebx
				and		ecx, 31
				I286CLOCK(ecx)
				push	ecx
				GET_NEXTPRE1
				pop		ecx
				jmp		sftext16cl_xtable[edi]
	}
}

I286 ret_near_data16(void) {					// C2: ret near DATA16

		__asm {
				//I286CLOCK(11)
				I286CLOCK_X(a, 11, (20+4))
				shr		ebx, 8
				add		ebx, 2
				movzx	ecx, I286_SP
				add		ecx, SS_BASE
				add		I286_SP, bx
				call	i286_memoryread_w
				mov		si, ax
				RESET_XPREFETCH
				ret
		}
}

I286 ret_near(void) {							// C3: ret near

		__asm {
				//I286CLOCK(11)
				I286CLOCK_X(a, 11, (15+4))
				REGPOP(si)
				RESET_XPREFETCH
				ret
		}
}

I286 les_r16_ea(void) {							// C4: les REG16, EA

		__asm {
				cmp		bh, 0c0h
				jnc		src_register
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 18)
				movzx	eax, bh
				push	eax
				call	p_get_ea[eax*4]
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				add		bp, 2
				pop		edx
				shr		edx, 3-1
				and		edx, 7*2
				mov		word ptr I286_REG[edx], ax
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		I286_ES, ax
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short les_pe
				shl		eax, 4					// make segreg
les_base:		mov		ES_BASE, eax
				ret

les_pe:			push	offset les_base
				jmp		i286x_selector

src_register:	INT_NUM(6)
		}
}

I286 lds_r16_ea(void) {							// C5: lds REG16, EA

		__asm {
				cmp		bh, 0c0h
				jnc		src_register
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 18)
				movzx	eax, bh
				push	eax
				call	p_get_ea[eax*4]
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				add		bp, 2
				pop		edx
				shr		edx, 3-1
				and		edx, 7*2
				mov		word ptr I286_REG[edx], ax
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		I286_DS, ax
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short lds_pe
				shl		eax, 4					// make segreg
lds_base:		mov		DS_BASE, eax
				mov		DS_FIX, eax
				ret

lds_pe:			push	offset lds_base
				jmp		i286x_selector

src_register:	INT_NUM(6)

		}
}

I286 mov_ea8_data8(void) {						// C6: mov EA8, DATA8

		__asm {
				cmp		bh, 0c0h
				jc		memory_eareg8
				I286CLOCK(2)
				mov		edx, ebx
				shr		edx, 8
				mov		bp, dx
				bt		bp, 2
				rcl		ebp, 1
				and		ebp, 7
				GET_NEXTPRE2
				mov		byte ptr I286_REG[ebp], dh
				ret
				align	16
		memory_eareg8:
				//I286CLOCK(3)
				I286CLOCK_X(b, 3, 11)
				movzx	eax, bh
				call	p_get_ea[eax*4]
				mov		dl, bl
				GET_NEXTPRE1
				lea		ecx, [edi + ebp]
				jmp		i286_memorywrite
		}
}

I286 mov_ea16_data16(void) {					// C7: mov EA16, DATA16

		__asm {
				cmp		bh, 0c0h
				jc		memory_eareg8
				I286CLOCK(2)
				movzx	ebp, bh
				and		ebp, 7
				mov		edx, ebx
				shr		edx, 16
				GET_NEXTPRE4
				mov		word ptr I286_REG[ebp*2], dx
				ret
				align	16
		memory_eareg8:
				//I286CLOCK(3)
				I286CLOCK_X(b, 3, 11)
				movzx	eax, bh
				call	p_get_ea[eax*4]
				mov		dx, bx
				GET_NEXTPRE2
				lea		ecx, [edi + ebp]
				jmp		i286_memorywrite_w
		}
}

I286 _enter(void) {								// C8: enter DATA16, DATA8

		__asm {
				mov		edi, SS_BASE
				movzx	ebp, I286_SP
				sub		bp, 2
				lea		ecx, [edi + ebp]
				mov		dx, I286_BP
				call	i286_memorywrite_w

				shld	eax, ebx, 8
				shr		ebx, 8
				and		eax, 1fh
				je		enter0
				dec		eax
				je		enter1
				lea		ecx, [eax*4 + 12]
				I286CLOCK(ecx)						// ToDo: V30
				push	ebx
				movzx	ebx, I286_BP
				mov		I286_BP, bp
		enter_lp:
				push	eax
				sub		bx, 2
				lea		ecx, [edi + ebx]
				call	i286_memoryread_w
				mov		edx, eax
				sub		bp, 2
				lea		ecx, [edi + ebp]
				call	i286_memorywrite_w
				pop		eax
				dec		eax
				jne		enter_lp
				pop		ebx
				sub		bp, 2
				lea		ecx, [edi + ebp]
				mov		dx, I286_BP
				sub		bp, bx
				mov		I286_SP, bp
				push	ecx
				GET_NEXTPRE4
				pop		ecx
				jmp		i286_memorywrite_w

				align	16
		enter1:
				I286CLOCK(15)
				mov		dx, bp
				sub		bp, 2
				lea		ecx, [edi + ebp]
				mov		I286_BP, dx
				sub		bp, bx
				mov		I286_SP, bp
				push	ecx
				GET_NEXTPRE4
				pop		ecx
				jmp		i286_memorywrite_w

				align	16
		enter0:
				I286CLOCK(11)
				mov		I286_BP, bp
				sub		bp, bx
				mov		I286_SP, bp
				GET_NEXTPRE4
				ret
		}
}

I286 leave(void) {								// C9: leave

		__asm {
				GET_NEXTPRE1
				I286CLOCK(5)					// ToDo: V30
				mov		ax, I286_BP
				mov		I286_SP, ax
				REGPOP(I286_BP)
				ret
		}
}

I286 ret_far_data16(void) {						// CA: ret far DATA16

		__asm {
				//I286CLOCK(15)
				I286CLOCK_X(a, 15, (24+4))
				shr		ebx, 8
				mov		edi, SS_BASE
				movzx	ebp, I286_SP
				add		I286_SP, bx
				lea		ecx, [edi + ebp]
				add		bp, 2
				call	i286_memoryread_w
				mov		si, ax
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		I286_CS, ax
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short ret_far16_pe
				shl		eax, 4					// make segreg
ret_far16_base:	mov		CS_BASE, eax
				add		I286_SP, 4
				RESET_XPREFETCH
				ret

ret_far16_pe:	push	offset ret_far16_base
				jmp		i286x_selector
		}
}

I286 ret_far(void) {							// CB: ret far

		__asm {
				//I286CLOCK(15)
				I286CLOCK_X(a, 15, (21+4))
				mov		edi, SS_BASE
				movzx	ebx, I286_SP
				lea		ecx, [edi + ebx]
				add		bx, 2
				call	i286_memoryread_w
				mov		si, ax
				lea		ecx, [edi + ebx]
				add		bx, 2
				call	i286_memoryread_w
				mov		I286_CS, ax
				movzx	eax, ax
				test	byte ptr (I286_MSW), MSW_PE
				jne		short ret_far_pe
				shl		eax, 4					// make segreg
ret_far_base:	mov		CS_BASE, eax
				mov		ebp, eax
				mov		I286_SP, bx
				RESET_XPREFETCH
				ret

ret_far_pe:		push	offset ret_far_base
				jmp		i286x_selector
		}
}

I286 int_03(void) {								// CC: int 3

		__asm {
				//I286CLOCK(23)
				I286CLOCK_X(a, 23, (38+4))
				inc		si
				INT_NUM(3)
		}
}


I286 int_data8(void) {							// CD: int DATA8

		__asm {
				//I286CLOCK(23)
				I286CLOCK_X(a, 23, (38+4))
				mov		edi, SS_BASE
				movzx	ebp, I286_SP
				sub		bp, 2
				mov		dx, I286_FLAG
				and		I286_FLAG, not (T_FLAG or I_FLAG)
				mov		I286_TRAP, 0
				lea		ecx, [edi + ebp]
				call	i286_memorywrite_w
				sub		bp, 2
				lea		ecx, [edi + ebp]
				mov		dx, I286_CS
				call	i286_memorywrite_w
#if defined(ENABLE_TRAP)
				movzx	eax, bh
				push	eax
				lea		edx, [esi - 1]
				movzx	ecx, I286_CS
				call	softinttrap
#endif
				movzx	eax, bh
				sub		bp, 2
				mov		I286_SP, bp
				lea		edx, [esi + 2]
				mov		eax, dword ptr I286_MEM[eax*4]
				mov		si, ax
				shr		eax, 16
				mov		I286_CS, ax
				shl		eax, 4					// make segreg
				mov		CS_BASE, eax
				RESET_XPREFETCH
				lea		ecx, [edi + ebp]
				jmp		i286_memorywrite_w
		}
}

I286 _into(void) {								// CE: into

		__asm {
				I286CLOCK(4)
				test	I286_FLAG, O_FLAG
				jne		intovf
				GET_NEXTPRE1
				ret

		intovf:	inc		si											// ver0.80
				INT_NUM(4)
		}
}

I286 _iret(void) {								// CF: iret
												//		V30用はV30PATCH.CPPに別途作成(Shinra)
		__asm {
				I286CLOCK(31)
				mov		edi, SS_BASE
				movzx	ebx, I286_SP
				lea		ecx, [edi + ebx]
				add		bx, 2
				call	i286_memoryread_w
				mov		si, ax
				lea		ecx, [edi + ebx]
				add		bx, 2
				call	i286_memoryread_w
				mov		I286_CS, ax
				and		eax, 0000ffffh
				shl		eax, 4					// make segreg
				mov		CS_BASE, eax
				lea		ecx, [edi + ebx]
				add		bx, 2
				call	i286_memoryread_w
				mov		I286_SP, bx
				and		ah, 0fh
				mov		I286_FLAG, ax
#if defined(VAEG_FIX)
				and		ah, 1
				cmp		ah, 1
#else
				and		ah, 3
				cmp		ah, 3
#endif
				sete	I286_TRAP
				RESET_XPREFETCH

				cmp		I286_TRAP, 0			// fast_intr
				jne		irqcheck
				test	I286_FLAG, I_FLAG
				je		nextop
				mov		al, pic.pi[0 * (type _PICITEM)].imr
				mov		ah, pic.pi[1 * (type _PICITEM)].imr
				not		ax
				test	al, pic.pi[0 * (type _PICITEM)].irr
				jne		irqcheck
				test	ah, pic.pi[1 * (type _PICITEM)].irr
				jne		irqcheck
nextop:			ret

irqcheck:		I286IRQCHECKTERM
		}
}


I286 shift_ea8_1(void) {						// D0: shift EA8, 1

		__asm {
				movzx	edx, bh
				mov		edi, edx
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		bh, 0c0h
				jc		memory_eareg8
				bt		dx, 2
				rcl		edx, 1
				and		edx, 7
				I286CLOCK(2)
				GET_NEXTPRE2
				jmp		sftreg8_xtable[edi]
				align	16
		memory_eareg8:
				I286CLOCK(7)
				call	p_ea_dst[edx*4]
				cmp		ecx, I286_MEMWRITEMAX
				jnc		extmem_eareg8
				jmp		sftmem8_xtable[edi]
				align	16
		extmem_eareg8:
				call	i286_memoryread
				mov		edx, eax
				jmp		sftext8_xtable[edi]
	}
}

I286 shift_ea16_1(void) {						// D1: shift EA16, 1

		__asm {
				movzx	edx, bh
				mov		edi, edx
				shr		edi, 3-2
				and		edi, 7*4						// opcode
				cmp		bh, 0c0h
				jc		memory_eareg16
				I286CLOCK(2)
				and		edx, 7
				GET_NEXTPRE2
				jmp		sftreg16_xtable[edi]
				align	16
		memory_eareg16:
				I286CLOCK(7)
				call	p_ea_dst[edx*4]
				cmp		ecx, (I286_MEMWRITEMAX-1)
				jnc		extmem_eareg16
				jmp		sftmem16_xtable[edi]
				align	16
		extmem_eareg16:
				call	i286_memoryread_w
				mov		edx, eax
				jmp		sftext16_xtable[edi]
	}
}

I286 shift_ea8_cl(void) {						// D2: shift EA8, cl

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
				and		ecx, 31
				I286CLOCK(ecx)
				jmp		sftreg8cl_xtable[edi]
				align	16
		memory_eareg8:
				I286CLOCK(8)
				call	p_ea_dst[eax*4]
				cmp		ecx, I286_MEMWRITEMAX
				jnc		extmem_eareg8
				lea		edx, I286_MEM[ecx]
				movzx	ecx, I286_CL
				and		ecx, 31
				I286CLOCK(ecx)
				jmp		sftreg8cl_xtable[edi]
				align	16
		extmem_eareg8:
				call	i286_memoryread
				mov		edx, eax
				mov		ebp, ecx
				movzx	ecx, I286_CL
				and		ecx, 31
				I286CLOCK(ecx)
				jmp		sftext8cl_xtable[edi]
	}
}

I286 shift_ea16_cl(void) {						// D3: shift EA16, cl

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
				and		ecx, 31
				I286CLOCK(ecx)
				jmp		sftreg16cl_xtable[edi]
				align	16
		memory_eareg16:
				I286CLOCK(8)
				call	p_ea_dst[eax*4]
				cmp		ecx, (I286_MEMWRITEMAX-1)
				jnc		extmem_eareg16
				lea		edx, I286_MEM[ecx]
				movzx	ecx, I286_CL
				and		ecx, 31
				I286CLOCK(ecx)
				jmp		sftreg16cl_xtable[edi]
				align	16
		extmem_eareg16:
				call	i286_memoryread_w
				mov		edx, eax
				mov		ebp, ecx
				movzx	ecx, I286_CL
				and		ecx, 31
				I286CLOCK(ecx)
				jmp		sftext16cl_xtable[edi]
	}
}

I286 _aam(void) {								// D4: AAM

		__asm {									// todo: flag !!!
				cmp		bh, 0
				je		div0
				I286CLOCK(16)
				movzx	eax, I286_AL
				div		bh
				xchg	al, ah
				mov		I286_AX, ax
				and		I286_FLAGL, not (S_FLAG or Z_FLAG or P_FLAG)
				mov		dh, ah
				and		dh, S_FLAG
				test	ax, ax
				jne		short nzflagsed
				or		dh, Z_FLAG
nzflagsed:		xor		al, ah
				and		eax, 00ffh
				mov		al, iflags[eax]
				and		al, P_FLAG
				or		al, dh
				or		I286_FLAGL, al
				GET_NEXTPRE2
				ret

		div0:	INT_NUM(0)
		}
}

I286 _aad(void) {								// D5: AAD

		__asm {									// todo: flag !!!
				I286CLOCK(14)
				mov		al, I286_AH
				mul		bh
				add		al, I286_AL
				mov		ah, 0
				mov		I286_AX, ax
				and		I286_FLAGL, not (S_FLAG or Z_FLAG or P_FLAG)
				and		eax, 00ffh
				mov		al, iflags[eax]
				and		al, P_FLAG or Z_FLAG
				or		I286_FLAGL, al
				GET_NEXTPRE2
				ret
		}
}

I286 _setalc(void) {							// D6: setalc(80286)

		__asm {
				setnc	I286_AL
				dec		I286_AL
				GET_NEXTPRE1
				ret
		}
}

I286 _xlat(void) {								// D7: xlat

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

I286 _esc(void) {								// D8-DF: esc

		__asm {
				cmp		bh, 0c0h
				jnc		ea_reg8
				movzx	eax, bh
				jmp		p_lea[eax*4]
				align	16
		ea_reg8:
				GET_NEXTPRE2
				ret
		}
}


I286 _loopnz(void) {							// E0: loopnz

		__asm {
				dec		I286_CX
				je		loopend
				test	I286_FLAG, Z_FLAG
				jne		loopend
				//I286CLOCK(8)
				I286CLOCK_X(a, 8, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
				align	16
loopend:		//I286CLOCK(4)
				I286CLOCK_X(b, 4, 5)
				GET_NEXTPRE2
				ret
		}
}

I286 _loopz(void) {								// E1: loopz

		__asm {
				dec		I286_CX
				je		loopend
				test	I286_FLAG, Z_FLAG
				je		loopend
				//I286CLOCK(8)
				I286CLOCK_X(a, 8, (14+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
				align	16
loopend:		I286CLOCK(4)
				GET_NEXTPRE2
				ret
		}
}

I286 _loop(void) {								// E2: loop

		__asm {
				dec		I286_CX
				je		loopend
				//I286CLOCK(8)
				I286CLOCK_X(a, 8, (13 + 4))		// V30は13clock+4clock(命令読み込み時間分)
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
				align	16
loopend:		//I286CLOCK(4)
				I286CLOCK_X(b, 4, 5)
				GET_NEXTPRE2
				ret
		}
}

I286 _jcxz(void) {								// E3: jcxz

		__asm {
				cmp		I286_CX, 0
				jne		loopend
				//I286CLOCK(8)
				I286CLOCK_X(a, 8, (13+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
				align	16
loopend:		//I286CLOCK(4)
				I286CLOCK_X(b, 4, 5)
				GET_NEXTPRE2
				ret
		}
}

I286 in_al_data8(void) {						// E4: in al, DATA8

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 9)
				lea		eax, [esi + 2]
				add		eax, CS_BASE
				mov		I286_INPADRS, eax
				movzx	ecx, bh
				call	iocore_inp8
				mov		I286_AL, al
				mov		I286_INPADRS, 0
				GET_NEXTPRE2
				ret
		}
}

I286 in_ax_data8(void) {						// E5: in ax, DATA8

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 9)
				lea		eax, [esi + 2]
				add		eax, CS_BASE
				movzx	ecx, bh
				call	iocore_inp16
				mov		I286_AX, ax
				GET_NEXTPRE2
				ret
		}
}

I286 out_data8_al(void) {						// E6: out DATA8, al

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				movzx	ecx, bh
				push	ecx
				GET_NEXTPRE2
				pop		ecx
				mov		dl, I286_AL
				jmp		iocore_out8
		}
}

I286 out_data8_ax(void) {						// E7: out DATA8, ax

		__asm {
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				movzx	ecx, bh
				push	ecx
				GET_NEXTPRE2
				pop		ecx
				mov		dx, I286_AX
				jmp		iocore_out16
		}
}

I286 call_near(void) {							// E8: call near

		__asm {
				//I286CLOCK(7)
				I286CLOCK_X(a, 7, (16+4))
				shr		ebx, 8
				add		si, 3
				mov		dx, si
				add		si, bx
				RESET_XPREFETCH
				sub		I286_SP, 2
				movzx	ecx, I286_SP
				add		ecx, SS_BASE
				jmp		i286_memorywrite_w
		}
}

I286 jmp_near(void) {							// E9: jmp near

		__asm {
				//I286CLOCK(7)
				I286CLOCK_X(a, 7, (13+4))
				shr		ebx, 8
				add		si, 3
				add		si, bx
				RESET_XPREFETCH
				ret
		}
}

I286 jmp_far(void) {							// EA: jmp far

		__asm {
				//I286CLOCK(11)
				I286CLOCK_X(a, 11, (15+4))
				GET_NEXTPRE1
				mov		si, bx
				shr		ebx, 16
				mov		I286_CS, bx
				test	byte ptr (I286_MSW), MSW_PE
				jne		short jmp_far_pe
				shl		ebx, 4					// make segreg
				mov		CS_BASE, ebx
jmp_far_base:	RESET_XPREFETCH
				ret

jmp_far_pe:		mov		eax, ebx
				call	i286x_selector
				mov		CS_BASE, eax
				jmp		short jmp_far_base
		}
}

I286 jmp_short(void) {							// EB: jmp short

		__asm {
				//I286CLOCK(7)
				I286CLOCK_X(a, 7, (12+4))
				movsx	eax, bh
				add		si, ax
				add		si, 2
				RESET_XPREFETCH
				ret
		}
}

I286 in_al_dx(void) {							// EC: in al, dx

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				movzx	ecx, I286_DX
				call	iocore_inp8
				mov		I286_AL, al
				GET_NEXTPRE1
				ret
		}
}

I286 in_ax_dx(void) {							// ED: in ax, dx

		__asm {
				//I286CLOCK(5)
				I286CLOCK_X(a, 5, 8)
				movzx	ecx, I286_DX
				call	iocore_inp16
				mov		I286_AX, ax
				GET_NEXTPRE1
				ret
		}
}

I286 out_dx_al(void) {							// EE: out dx, al

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				movzx	ecx, I286_DX
				mov		dl, I286_AL
				jmp		iocore_out8
			}
}

I286 out_dx_ax(void) {							// EF: out dx, ax

		__asm {
				GET_NEXTPRE1
				//I286CLOCK(3)
				I286CLOCK_X(a, 3, 8)
				movzx	ecx, I286_DX
				mov		dx, I286_AX
				jmp		iocore_out16
		}
}

I286 _lock(void) {								// F0: lock

		__asm {
				GET_NEXTPRE1
				ret
		}
}

I286 _repne(void) {								// F2: repne

		__asm {
				I286PREFIX(i286op_repne)
		}
}

I286 _repe(void) {								// F3: repe

		__asm {
				I286PREFIX(i286op_repe)
		}
}

I286 _hlt(void) {								// F4: hlt

		__asm {
				mov		I286_REMCLOCK, -1
				ret
		}
}

I286 _cmc(void) {								// F5: cmc

		__asm {
				I286CLOCK(2)
				xor		I286_FLAG, C_FLAG
				GET_NEXTPRE1
				ret
		}
}

I286 _ope0xf6(void) {							// F6: 

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4
				jmp		ope0xf6_xtable[edi]
		}
}

I286 _ope0xf7(void) {							// F7: 

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4
				jmp		ope0xf7_xtable[edi]
		}
}

I286 _clc(void) {								// F8: clc

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				and		I286_FLAG, not C_FLAG
				ret
		}
}

I286 _stc(void) {								// F9: stc

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				or		I286_FLAG, C_FLAG
				ret
		}
}

I286 _cli(void) {								// FA: cli

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				and		I286_FLAG, not I_FLAG
#if defined(VAEG_FIX)
				// シングルステップ割り込みは割り込み許可フラグの影響を受けない
#else
				mov		I286_TRAP, 0
#endif
				ret
		}
}

I286 _sti(void) {								// FB: sti

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				cmp		i286core.s.prefix, 0	// ver0.26 00/10/08
				jne		prefix_exist			// 前方分岐ジャンプなので。
		noprefix:
				movzx	ebp, bl
				bts		I286_FLAG, 9
				jne		jmp_nextop
#if defined(VAEG_FIX)
				// シングルステップ割り込みは割り込み許可フラグの影響を受けない
#else
				test	I286_FLAG, T_FLAG
				setne	I286_TRAP
#endif

				jne		nextopandexit			// fast_intr
				mov		al, pic.pi[0 * (type _PICITEM)].imr
				mov		ah, pic.pi[1 * (type _PICITEM)].imr
				not		ax
				test	al, pic.pi[0 * (type _PICITEM)].irr
				jne		nextopandexit
				test	ah, pic.pi[1 * (type _PICITEM)].irr
				jne		nextopandexit
jmp_nextop:		jmp		i286op[ebp*4]

nextopandexit:	call	i286op[ebp*4]
				I286IRQCHECKTERM

prefix_exist:	pop		eax						// eax<-offset removeprefix
				call	eax
				jmp		noprefix
		}
}

I286 _cld(void) {								// FC: cld

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				and		I286_FLAG, not D_FLAG
				ret
		}
}

I286 _std(void) {								// FD: std

		__asm {
				GET_NEXTPRE1
				I286CLOCK(2)
				or		I286_FLAG, D_FLAG
				ret
		}
}

I286 _ope0xfe(void) {							// FE: 

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 1*4
				jmp		ope0xfe_xtable[edi]
		}
}

I286 _ope0xff(void) {							// FF: 

		__asm {
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4
				jmp		ope0xff_xtable[edi]
		}
}


// -------------------------------------------------------------------------

const I286TBL i286op[256] = {
			add_ea_r8,						// 00:	add		EA, REG8
			add_ea_r16,						// 01:	add		EA, REG16
			add_r8_ea,						// 02:	add		REG8, EA
			add_r16_ea,						// 03:	add		REG16, EA
			add_al_data8,					// 04:	add		al, DATA8
			add_ax_data16,					// 05:	add		ax, DATA16
			push_es,						// 06:	push	es
			pop_es,							// 07:	pop		es
			or_ea_r8,						// 08:	or		EA, REGF8
			or_ea_r16,						// 09:	or		EA, REG16
			or_r8_ea,						// 0A:	or		REG8, EA
			or_r16_ea,						// 0B:	or		REG16, EA
			or_al_data8,					// 0C:	or		al, DATA8
			or_ax_data16,					// 0D:	or		ax, DATA16
			push_cs,						// 0E:	push	cs
			_xcts,							// 0F:	i286 upper opcode

			adc_ea_r8,						// 10:	adc		EA, REG8
			adc_ea_r16,						// 11:	adc		EA, REG16
			adc_r8_ea,						// 12:	adc		REG8, EA
			adc_r16_ea,						// 13:	adc		REG16, EA
			adc_al_data8,					// 14:	adc		al, DATA8
			adc_ax_data16,					// 15:	adc		ax, DATA16
			push_ss,						// 16:	push	ss
			pop_ss,							// 17:	pop		ss
			sbb_ea_r8,						// 18:	sbb		EA, REG8
			sbb_ea_r16,						// 19:	sbb		EA, REG16
			sbb_r8_ea,						// 1A:	sbb		REG8, EA
			sbb_r16_ea,						// 1B:	sbb		REG16, EA
			sbb_al_data8,					// 1C:	sbb		al, DATA8
			sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			push_ds,						// 1E:	push	ds
			pop_ds,							// 1F:	pop		ds

			and_ea_r8,						// 20:	and		EA, REG8
			and_ea_r16,						// 21:	and		EA, REG16
			and_r8_ea,						// 22:	and		REG8, EA
			and_r16_ea,						// 23:	and		REG16, EA
			and_al_data8,					// 24:	and		al, DATA8
			and_ax_data16,					// 25:	and		ax, DATA16
			segprefix_es,					// 26:	es:
			_daa,							// 27:	daa
			sub_ea_r8,						// 28:	sub		EA, REG8
			sub_ea_r16,						// 29:	sub		EA, REG16
			sub_r8_ea,						// 2A:	sub		REG8, EA
			sub_r16_ea,						// 2B:	sub		REG16, EA
			sub_al_data8,					// 2C:	sub		al, DATA8
			sub_ax_data16,					// 2D:	sub		ax, DATA16
			segprefix_cs,					// 2E:	cs:
			_das,							// 2F:	das

			xor_ea_r8,						// 30:	xor		EA, REG8
			xor_ea_r16,						// 31:	xor		EA, REG16
			xor_r8_ea,						// 32:	xor		REG8, EA
			xor_r16_ea,						// 33:	xor		REG16, EA
			xor_al_data8,					// 34:	xor		al, DATA8
			xor_ax_data16,					// 35:	xor		ax, DATA16
			segprefix_ss,					// 36:	ss:
			_aaa,							// 37:	aaa
			cmp_ea_r8,						// 38:	cmp		EA, REG8
			cmp_ea_r16,						// 39:	cmp		EA, REG16
			cmp_r8_ea,						// 3A:	cmp		REG8, EA
			cmp_r16_ea,						// 3B:	cmp		REG16, EA
			cmp_al_data8,					// 3C:	cmp		al, DATA8
			cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			segprefix_ds,					// 3E:	ds:
			_aas,							// 3F:	aas

			inc_ax,							// 40:	inc		ax
			inc_cx,							// 41:	inc		cx
			inc_dx,							// 42:	inc		dx
			inc_bx,							// 43:	inc		bx
			inc_sp,							// 44:	inc		sp
			inc_bp,							// 45:	inc		bp
			inc_si,							// 46:	inc		si
			inc_di,							// 47:	inc		di
			dec_ax,							// 48:	dec		ax
			dec_cx,							// 49:	dec		cx
			dec_dx,							// 4A:	dec		dx
			dec_bx,							// 4B:	dec		bx
			dec_sp,							// 4C:	dec		sp
			dec_bp,							// 4D:	dec		bp
			dec_si,							// 4E:	dec		si
			dec_di,							// 4F:	dec		di

			push_ax,						// 50:	push	ax
			push_cx,						// 51:	push	cx
			push_dx,						// 52:	push	dx
			push_bx,						// 53:	push	bx
			push_sp,						// 54:	push	sp
			push_bp,						// 55:	push	bp
			push_si,						// 56:	push	si
			push_di,						// 57:	push	di
			pop_ax,							// 58:	pop		ax
			pop_cx,							// 59:	pop		cx
			pop_dx,							// 5A:	pop		dx
			pop_bx,							// 5B:	pop		bx
			pop_sp,							// 5C:	pop		sp
			pop_bp,							// 5D:	pop		bp
			pop_si,							// 5E:	pop		si
			pop_di,							// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_arpl,							// 63:	arpl
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			push_data16,					// 68:	push	DATA16
			imul_reg_ea_data16,				// 69:	imul	REG, EA, DATA16
			push_data8,						// 6A:	push	DATA8
			imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			_insb,							// 6C:	insb
			_insw,							// 6D:	insw
			_outsb,							// 6E:	outsb
			_outsw,							// 6F:	outsw

			jo_short,						// 70:	jo short
			jno_short,						// 71:	jno short
			jc_short,						// 72:	jnae/jb/jc short
			jnc_short,						// 73:	jae/jnb/jnc short
			jz_short,						// 74:	je/jz short
			jnz_short,						// 75:	jne/jnz short
			jna_short,						// 76:	jna/jbe short
			ja_short,						// 77:	ja/jnbe short
			js_short,						// 78:	js short
			jns_short,						// 79:	jns short
			jp_short,						// 7A:	jp/jpe short
			jnp_short,						// 7B:	jnp/jpo short
			jl_short,						// 7C:	jl/jnge short
			jnl_short,						// 7D:	jnl/jge short
			jle_short,						// 7E:	jle/jng short
			jnle_short,						// 7F:	jg/jnle short

			calc_ea8_i8,					// 80:	op		EA8, DATA8
			calc_ea16_i16,					// 81:	op		EA16, DATA16
			calc_ea8_i8,					// 82:	op		EA8, DATA8
			calc_ea16_i8,					// 83:	op		EA16, DATA8
			test_ea_r8,						// 84:	test	EA, REG8
			test_ea_r16,					// 85:	test	EA, REG16
			xchg_ea_r8,						// 86:	xchg	EA, REG8
			xchg_ea_r16,					// 87:	xchg	EA, REG16
			mov_ea_r8,						// 88:	mov		EA, REG8
			mov_ea_r16,						// 89:	mov		EA, REG16
			mov_r8_ea,						// 8A:	mov		REG8, EA
			mov_r16_ea,						// 8B:	add		REG16, EA
			mov_ea_seg,						// 8C:	mov		EA, segreg
			lea_r16_ea,						// 8D:	lea		REG16, EA
			mov_seg_ea,						// 8E:	mov		segrem, EA
			pop_ea,							// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			xchg_ax_cx,						// 91:	xchg	ax, cx
			xchg_ax_dx,						// 92:	xchg	ax, dx
			xchg_ax_bx,						// 93:	xchg	ax, bx
			xchg_ax_sp,						// 94:	xchg	ax, sp
			xchg_ax_bp,						// 95:	xchg	ax, bp
			xchg_ax_si,						// 96:	xchg	ax, si
			xchg_ax_di,						// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			mov_al_m8,						// A0:	mov		al, m8
			mov_ax_m16,						// A1:	mov		ax, m16
			mov_m8_al,						// A2:	mov		m8, al
			mov_m16_ax,						// A3:	mov		m16, ax
			_movsb,							// A4:	movsb
			_movsw,							// A5:	movsw
			_cmpsb,							// A6:	cmpsb
			_cmpsw,							// A7:	cmpsw
			test_al_data8,					// A8:	test	al, DATA8
			test_ax_data16,					// A9:	test	ax, DATA16
			_stosb,							// AA:	stosw
			_stosw,							// AB:	stosw
			_lodsb,							// AC:	lodsb
			_lodsw,							// AD:	lodsw
			_scasb,							// AE:	scasb
			_scasw,							// AF:	scasw

			mov_al_imm,						// B0:	mov		al, imm8
			mov_cl_imm,						// B1:	mov		cl, imm8
			mov_dl_imm,						// B2:	mov		dl, imm8
			mov_bl_imm,						// B3:	mov		bl, imm8
			mov_ah_imm,						// B4:	mov		ah, imm8
			mov_ch_imm,						// B5:	mov		ch, imm8
			mov_dh_imm,						// B6:	mov		dh, imm8
			mov_bh_imm,						// B7:	mov		bh, imm8
			mov_ax_imm,						// B8:	mov		ax, imm16
			mov_cx_imm,						// B9:	mov		cx, imm16
			mov_dx_imm,						// BA:	mov		dx, imm16
			mov_bx_imm,						// BB:	mov		bx, imm16
			mov_sp_imm,						// BC:	mov		sp, imm16
			mov_bp_imm,						// BD:	mov		bp, imm16
			mov_si_imm,						// BE:	mov		si, imm16
			mov_di_imm,						// BF:	mov		di, imm16

			shift_ea8_data8,				// C0:	shift	EA8, DATA8
			shift_ea16_data8,				// C1:	shift	EA16, DATA8
			ret_near_data16,				// C2:	ret near DATA16
			ret_near,						// C3:	ret near
			les_r16_ea,						// C4:	les		REG16, EA
			lds_r16_ea,						// C5:	lds		REG16, EA
			mov_ea8_data8,					// C6:	mov		EA8, DATA8
			mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			leave,							// C9:	leave
			ret_far_data16,					// CA:	ret far	DATA16
			ret_far,						// CB:	ret far
			int_03,							// CC:	int		3
			int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			shift_ea8_1,					// D0:	shift EA8, 1
			shift_ea16_1,					// D1:	shift EA16, 1
			shift_ea8_cl,					// D2:	shift EA8, cl
			shift_ea16_cl,					// D3:	shift EA16, cl
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
			in_al_data8,					// E4:	in		al, DATA8
			in_ax_data8,					// E5:	in		ax, DATA8
			out_data8_al,					// E6:	out		DATA8, al
			out_data8_ax,					// E7:	out		DATA8, ax
			call_near,						// E8:	call near
			jmp_near,						// E9:	jmp near
			jmp_far,						// EA:	jmp far
			jmp_short,						// EB:	jmp short
			in_al_dx,						// EC:	in		al, dx
			in_ax_dx,						// ED:	in		ax, dx
			out_dx_al,						// EE:	out		dx, al
			out_dx_ax,						// EF:	out		dx, ax

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

I286 repe_segprefix_es(void) {

		__asm {
				mov		eax, ES_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		i286op_repe[eax*4]
		}
}

I286 repe_segprefix_cs(void) {

		__asm {
				mov		eax, CS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		i286op_repe[eax*4]
		}
}

I286 repe_segprefix_ss(void) {

		__asm {
				mov		eax, SS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		i286op_repe[eax*4]
		}
}

I286 repe_segprefix_ds(void) {

		__asm {
				mov		eax, DS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		i286op_repe[eax*4]
		}
}

const I286TBL i286op_repe[256] = {
			add_ea_r8,						// 00:	add		EA, REG8
			add_ea_r16,						// 01:	add		EA, REG16
			add_r8_ea,						// 02:	add		REG8, EA
			add_r16_ea,						// 03:	add		REG16, EA
			add_al_data8,					// 04:	add		al, DATA8
			add_ax_data16,					// 05:	add		ax, DATA16
			push_es,						// 06:	push	es
			pop_es,							// 07:	pop		es
			or_ea_r8,						// 08:	or		EA, REGF8
			or_ea_r16,						// 09:	or		EA, REG16
			or_r8_ea,						// 0A:	or		REG8, EA
			or_r16_ea,						// 0B:	or		REG16, EA
			or_al_data8,					// 0C:	or		al, DATA8
			or_ax_data16,					// 0D:	or		ax, DATA16
			push_cs,						// 0E:	push	cs
			_xcts,							// 0F:	i286 upper opcode

			adc_ea_r8,						// 10:	adc		EA, REG8
			adc_ea_r16,						// 11:	adc		EA, REG16
			adc_r8_ea,						// 12:	adc		REG8, EA
			adc_r16_ea,						// 13:	adc		REG16, EA
			adc_al_data8,					// 14:	adc		al, DATA8
			adc_ax_data16,					// 15:	adc		ax, DATA16
			push_ss,						// 16:	push	ss
			pop_ss,							// 17:	pop		ss
			sbb_ea_r8,						// 18:	sbb		EA, REG8
			sbb_ea_r16,						// 19:	sbb		EA, REG16
			sbb_r8_ea,						// 1A:	sbb		REG8, EA
			sbb_r16_ea,						// 1B:	sbb		REG16, EA
			sbb_al_data8,					// 1C:	sbb		al, DATA8
			sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			push_ds,						// 1E:	push	ds
			pop_ds,							// 1F:	pop		ds

			and_ea_r8,						// 20:	and		EA, REG8
			and_ea_r16,						// 21:	and		EA, REG16
			and_r8_ea,						// 22:	and		REG8, EA
			and_r16_ea,						// 23:	and		REG16, EA
			and_al_data8,					// 24:	and		al, DATA8
			and_ax_data16,					// 25:	and		ax, DATA16
			repe_segprefix_es,				// 26:	repe es:
			_daa,							// 27:	daa
			sub_ea_r8,						// 28:	sub		EA, REG8
			sub_ea_r16,						// 29:	sub		EA, REG16
			sub_r8_ea,						// 2A:	sub		REG8, EA
			sub_r16_ea,						// 2B:	sub		REG16, EA
			sub_al_data8,					// 2C:	sub		al, DATA8
			sub_ax_data16,					// 2D:	sub		ax, DATA16
			repe_segprefix_cs,				// 2E:	repe cs:
			_das,							// 2F:	das

			xor_ea_r8,						// 30:	xor		EA, REG8
			xor_ea_r16,						// 31:	xor		EA, REG16
			xor_r8_ea,						// 32:	xor		REG8, EA
			xor_r16_ea,						// 33:	xor		REG16, EA
			xor_al_data8,					// 34:	xor		al, DATA8
			xor_ax_data16,					// 35:	xor		ax, DATA16
			repe_segprefix_ss,				// 36:	repe ss:
			_aaa,							// 37:	aaa
			cmp_ea_r8,						// 38:	cmp		EA, REG8
			cmp_ea_r16,						// 39:	cmp		EA, REG16
			cmp_r8_ea,						// 3A:	cmp		REG8, EA
			cmp_r16_ea,						// 3B:	cmp		REG16, EA
			cmp_al_data8,					// 3C:	cmp		al, DATA8
			cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			repe_segprefix_ds,				// 3E:	repe ds:
			_aas,							// 3F:	aas

			inc_ax,							// 40:	inc		ax
			inc_cx,							// 41:	inc		cx
			inc_dx,							// 42:	inc		dx
			inc_bx,							// 43:	inc		bx
			inc_sp,							// 44:	inc		sp
			inc_bp,							// 45:	inc		bp
			inc_si,							// 46:	inc		si
			inc_di,							// 47:	inc		di
			dec_ax,							// 48:	dec		ax
			dec_cx,							// 49:	dec		cx
			dec_dx,							// 4A:	dec		dx
			dec_bx,							// 4B:	dec		bx
			dec_sp,							// 4C:	dec		sp
			dec_bp,							// 4D:	dec		bp
			dec_si,							// 4E:	dec		si
			dec_di,							// 4F:	dec		di

			push_ax,						// 50:	push	ax
			push_cx,						// 51:	push	cx
			push_dx,						// 52:	push	dx
			push_bx,						// 53:	push	bx
			push_sp,						// 54:	push	sp
			push_bp,						// 55:	push	bp
			push_si,						// 56:	push	si
			push_di,						// 57:	push	di
			pop_ax,							// 58:	pop		ax
			pop_cx,							// 59:	pop		cx
			pop_dx,							// 5A:	pop		dx
			pop_bx,							// 5B:	pop		bx
			pop_sp,							// 5C:	pop		sp
			pop_bp,							// 5D:	pop		bp
			pop_si,							// 5E:	pop		si
			pop_di,							// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_arpl,							// 63:	arpl
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			push_data16,					// 68:	push	DATA16
			imul_reg_ea_data16,				// 69:	imul	REG, EA, DATA16
			push_data8,						// 6A:	push	DATA8
			imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			rep_xinsb,						// 6C:	rep insb
			rep_xinsw,						// 6D:	rep insw
			rep_xoutsb,						// 6E:	rep outsb
			rep_xoutsw,						// 6F:	rep outsw

			jo_short,						// 70:	jo short
			jno_short,						// 71:	jno short
			jc_short,						// 72:	jnae/jb/jc short
			jnc_short,						// 73:	jae/jnb/jnc short
			jz_short,						// 74:	je/jz short
			jnz_short,						// 75:	jne/jnz short
			jna_short,						// 76:	jna/jbe short
			ja_short,						// 77:	ja/jnbe short
			js_short,						// 78:	js short
			jns_short,						// 79:	jns short
			jp_short,						// 7A:	jp/jpe short
			jnp_short,						// 7B:	jnp/jpo short
			jl_short,						// 7C:	jl/jnge short
			jnl_short,						// 7D:	jnl/jge short
			jle_short,						// 7E:	jle/jng short
			jnle_short,						// 7F:	jg/jnle short

			calc_ea8_i8,					// 80:	op		EA8, DATA8
			calc_ea16_i16,					// 81:	op		EA16, DATA16
			calc_ea8_i8,					// 82:	op		EA8, DATA8
			calc_ea16_i8,					// 83:	op		EA16, DATA8
			test_ea_r8,						// 84:	test	EA, REG8
			test_ea_r16,					// 85:	test	EA, REG16
			xchg_ea_r8,						// 86:	xchg	EA, REG8
			xchg_ea_r16,					// 87:	xchg	EA, REG16
			mov_ea_r8,						// 88:	mov		EA, REG8
			mov_ea_r16,						// 89:	mov		EA, REG16
			mov_r8_ea,						// 8A:	mov		REG8, EA
			mov_r16_ea,						// 8B:	add		REG16, EA
			mov_ea_seg,						// 8C:	mov		EA, segreg
			lea_r16_ea,						// 8D:	lea		REG16, EA
			mov_seg_ea,						// 8E:	mov		segrem, EA
			pop_ea,							// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			xchg_ax_cx,						// 91:	xchg	ax, cx
			xchg_ax_dx,						// 92:	xchg	ax, dx
			xchg_ax_bx,						// 93:	xchg	ax, bx
			xchg_ax_sp,						// 94:	xchg	ax, sp
			xchg_ax_bp,						// 95:	xchg	ax, bp
			xchg_ax_si,						// 96:	xchg	ax, si
			xchg_ax_di,						// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			mov_al_m8,						// A0:	mov		al, m8
			mov_ax_m16,						// A1:	mov		ax, m16
			mov_m8_al,						// A2:	mov		m8, al
			mov_m16_ax,						// A3:	mov		m16, ax
			rep_xmovsb,						// A4:	rep movsb
			rep_xmovsw,						// A5:	rep movsw
			repe_xcmpsb,					// A6:	repe cmpsb
			repe_xcmpsw,					// A7:	repe cmpsw
			test_al_data8,					// A8:	test	al, DATA8
			test_ax_data16,					// A9:	test	ax, DATA16
			rep_xstosb,						// AA:	rep stosw
			rep_xstosw,						// AB:	rep stosw
			rep_xlodsb,						// AC:	rep lodsb
			rep_xlodsw,						// AD:	rep lodsw
			repe_xscasb,					// AE:	repe scasb
			repe_xscasw,					// AF:	repe scasw

			mov_al_imm,						// B0:	mov		al, imm8
			mov_cl_imm,						// B1:	mov		cl, imm8
			mov_dl_imm,						// B2:	mov		dl, imm8
			mov_bl_imm,						// B3:	mov		bl, imm8
			mov_ah_imm,						// B4:	mov		ah, imm8
			mov_ch_imm,						// B5:	mov		ch, imm8
			mov_dh_imm,						// B6:	mov		dh, imm8
			mov_bh_imm,						// B7:	mov		bh, imm8
			mov_ax_imm,						// B8:	mov		ax, imm16
			mov_cx_imm,						// B9:	mov		cx, imm16
			mov_dx_imm,						// BA:	mov		dx, imm16
			mov_bx_imm,						// BB:	mov		bx, imm16
			mov_sp_imm,						// BC:	mov		sp, imm16
			mov_bp_imm,						// BD:	mov		bp, imm16
			mov_si_imm,						// BE:	mov		si, imm16
			mov_di_imm,						// BF:	mov		di, imm16

			shift_ea8_data8,				// C0:	shift	EA8, DATA8
			shift_ea16_data8,				// C1:	shift	EA16, DATA8
			ret_near_data16,				// C2:	ret near DATA16
			ret_near,						// C3:	ret near
			les_r16_ea,						// C4:	les		REG16, EA
			lds_r16_ea,						// C5:	lds		REG16, EA
			mov_ea8_data8,					// C6:	mov		EA8, DATA8
			mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			leave,							// C9:	leave
			ret_far_data16,					// CA:	ret far	DATA16
			ret_far,						// CB:	ret far
			int_03,							// CC:	int		3
			int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			shift_ea8_1,					// D0:	shift EA8, 1
			shift_ea16_1,					// D1:	shift EA16, 1
			shift_ea8_cl,					// D2:	shift EA8, cl
			shift_ea16_cl,					// D3:	shift EA16, cl
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
			in_al_data8,					// E4:	in		al, DATA8
			in_ax_data8,					// E5:	in		ax, DATA8
			out_data8_al,					// E6:	out		DATA8, al
			out_data8_ax,					// E7:	out		DATA8, ax
			call_near,						// E8:	call near
			jmp_near,						// E9:	jmp near
			jmp_far,						// EA:	jmp far
			jmp_short,						// EB:	jmp short
			in_al_dx,						// EC:	in		al, dx
			in_ax_dx,						// ED:	in		ax, dx
			out_dx_al,						// EE:	out		dx, al
			out_dx_ax,						// EF:	out		dx, ax

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

I286 repne_segprefix_es(void) {

		__asm {
				mov		eax, ES_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		i286op_repne[eax*4]
		}
}

I286 repne_segprefix_cs(void) {

		__asm {
				mov		eax, CS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		i286op_repne[eax*4]
		}
}

I286 repne_segprefix_ss(void) {

		__asm {
				mov		eax, SS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		i286op_repne[eax*4]
		}
}

I286 repne_segprefix_ds(void) {

		__asm {
				mov		eax, DS_BASE
				mov		DS_FIX, eax
				mov		SS_FIX, eax
				GET_NEXTPRE1
				movzx	eax, bl
				jmp		i286op_repne[eax*4]
		}
}

const I286TBL i286op_repne[256] = {
			add_ea_r8,						// 00:	add		EA, REG8
			add_ea_r16,						// 01:	add		EA, REG16
			add_r8_ea,						// 02:	add		REG8, EA
			add_r16_ea,						// 03:	add		REG16, EA
			add_al_data8,					// 04:	add		al, DATA8
			add_ax_data16,					// 05:	add		ax, DATA16
			push_es,						// 06:	push	es
			pop_es,							// 07:	pop		es
			or_ea_r8,						// 08:	or		EA, REGF8
			or_ea_r16,						// 09:	or		EA, REG16
			or_r8_ea,						// 0A:	or		REG8, EA
			or_r16_ea,						// 0B:	or		REG16, EA
			or_al_data8,					// 0C:	or		al, DATA8
			or_ax_data16,					// 0D:	or		ax, DATA16
			push_cs,						// 0E:	push	cs
			_xcts,							// 0F:	i286 upper opcode

			adc_ea_r8,						// 10:	adc		EA, REG8
			adc_ea_r16,						// 11:	adc		EA, REG16
			adc_r8_ea,						// 12:	adc		REG8, EA
			adc_r16_ea,						// 13:	adc		REG16, EA
			adc_al_data8,					// 14:	adc		al, DATA8
			adc_ax_data16,					// 15:	adc		ax, DATA16
			push_ss,						// 16:	push	ss
			pop_ss,							// 17:	pop		ss
			sbb_ea_r8,						// 18:	sbb		EA, REG8
			sbb_ea_r16,						// 19:	sbb		EA, REG16
			sbb_r8_ea,						// 1A:	sbb		REG8, EA
			sbb_r16_ea,						// 1B:	sbb		REG16, EA
			sbb_al_data8,					// 1C:	sbb		al, DATA8
			sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			push_ds,						// 1E:	push	ds
			pop_ds,							// 1F:	pop		ds

			and_ea_r8,						// 20:	and		EA, REG8
			and_ea_r16,						// 21:	and		EA, REG16
			and_r8_ea,						// 22:	and		REG8, EA
			and_r16_ea,						// 23:	and		REG16, EA
			and_al_data8,					// 24:	and		al, DATA8
			and_ax_data16,					// 25:	and		ax, DATA16
			repne_segprefix_es,				// 26:	repne es:
			_daa,							// 27:	daa
			sub_ea_r8,						// 28:	sub		EA, REG8
			sub_ea_r16,						// 29:	sub		EA, REG16
			sub_r8_ea,						// 2A:	sub		REG8, EA
			sub_r16_ea,						// 2B:	sub		REG16, EA
			sub_al_data8,					// 2C:	sub		al, DATA8
			sub_ax_data16,					// 2D:	sub		ax, DATA16
			repne_segprefix_cs,				// 2E:	repne cs:
			_das,							// 2F:	das

			xor_ea_r8,						// 30:	xor		EA, REG8
			xor_ea_r16,						// 31:	xor		EA, REG16
			xor_r8_ea,						// 32:	xor		REG8, EA
			xor_r16_ea,						// 33:	xor		REG16, EA
			xor_al_data8,					// 34:	xor		al, DATA8
			xor_ax_data16,					// 35:	xor		ax, DATA16
			repne_segprefix_ss,				// 36:	repne ss:
			_aaa,							// 37:	aaa
			cmp_ea_r8,						// 38:	cmp		EA, REG8
			cmp_ea_r16,						// 39:	cmp		EA, REG16
			cmp_r8_ea,						// 3A:	cmp		REG8, EA
			cmp_r16_ea,						// 3B:	cmp		REG16, EA
			cmp_al_data8,					// 3C:	cmp		al, DATA8
			cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			repne_segprefix_ds,				// 3E:	repne ds:
			_aas,							// 3F:	aas

			inc_ax,							// 40:	inc		ax
			inc_cx,							// 41:	inc		cx
			inc_dx,							// 42:	inc		dx
			inc_bx,							// 43:	inc		bx
			inc_sp,							// 44:	inc		sp
			inc_bp,							// 45:	inc		bp
			inc_si,							// 46:	inc		si
			inc_di,							// 47:	inc		di
			dec_ax,							// 48:	dec		ax
			dec_cx,							// 49:	dec		cx
			dec_dx,							// 4A:	dec		dx
			dec_bx,							// 4B:	dec		bx
			dec_sp,							// 4C:	dec		sp
			dec_bp,							// 4D:	dec		bp
			dec_si,							// 4E:	dec		si
			dec_di,							// 4F:	dec		di

			push_ax,						// 50:	push	ax
			push_cx,						// 51:	push	cx
			push_dx,						// 52:	push	dx
			push_bx,						// 53:	push	bx
			push_sp,						// 54:	push	sp
			push_bp,						// 55:	push	bp
			push_si,						// 56:	push	si
			push_di,						// 57:	push	di
			pop_ax,							// 58:	pop		ax
			pop_cx,							// 59:	pop		cx
			pop_dx,							// 5A:	pop		dx
			pop_bx,							// 5B:	pop		bx
			pop_sp,							// 5C:	pop		sp
			pop_bp,							// 5D:	pop		bp
			pop_si,							// 5E:	pop		si
			pop_di,							// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_arpl,							// 63:	arpl
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			push_data16,					// 68:	push	DATA16
			imul_reg_ea_data16,				// 69:	imul	REG, EA, DATA16
			push_data8,						// 6A:	push	DATA8
			imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			rep_xinsb,						// 6C:	rep insb
			rep_xinsw,						// 6D:	rep insw
			rep_xoutsb,						// 6E:	rep outsb
			rep_xoutsw,						// 6F:	rep outsw

			jo_short,						// 70:	jo short
			jno_short,						// 71:	jno short
			jc_short,						// 72:	jnae/jb/jc short
			jnc_short,						// 73:	jae/jnb/jnc short
			jz_short,						// 74:	je/jz short
			jnz_short,						// 75:	jne/jnz short
			jna_short,						// 76:	jna/jbe short
			ja_short,						// 77:	ja/jnbe short
			js_short,						// 78:	js short
			jns_short,						// 79:	jns short
			jp_short,						// 7A:	jp/jpe short
			jnp_short,						// 7B:	jnp/jpo short
			jl_short,						// 7C:	jl/jnge short
			jnl_short,						// 7D:	jnl/jge short
			jle_short,						// 7E:	jle/jng short
			jnle_short,						// 7F:	jg/jnle short

			calc_ea8_i8,					// 80:	op		EA8, DATA8
			calc_ea16_i16,					// 81:	op		EA16, DATA16
			calc_ea8_i8,					// 82:	op		EA8, DATA8
			calc_ea16_i8,					// 83:	op		EA16, DATA8
			test_ea_r8,						// 84:	test	EA, REG8
			test_ea_r16,					// 85:	test	EA, REG16
			xchg_ea_r8,						// 86:	xchg	EA, REG8
			xchg_ea_r16,					// 87:	xchg	EA, REG16
			mov_ea_r8,						// 88:	mov		EA, REG8
			mov_ea_r16,						// 89:	mov		EA, REG16
			mov_r8_ea,						// 8A:	mov		REG8, EA
			mov_r16_ea,						// 8B:	add		REG16, EA
			mov_ea_seg,						// 8C:	mov		EA, segreg
			lea_r16_ea,						// 8D:	lea		REG16, EA
			mov_seg_ea,						// 8E:	mov		segrem, EA
			pop_ea,							// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			xchg_ax_cx,						// 91:	xchg	ax, cx
			xchg_ax_dx,						// 92:	xchg	ax, dx
			xchg_ax_bx,						// 93:	xchg	ax, bx
			xchg_ax_sp,						// 94:	xchg	ax, sp
			xchg_ax_bp,						// 95:	xchg	ax, bp
			xchg_ax_si,						// 96:	xchg	ax, si
			xchg_ax_di,						// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			mov_al_m8,						// A0:	mov		al, m8
			mov_ax_m16,						// A1:	mov		ax, m16
			mov_m8_al,						// A2:	mov		m8, al
			mov_m16_ax,						// A3:	mov		m16, ax
			rep_xmovsb,						// A4:	rep movsb
			rep_xmovsw,						// A5:	rep movsw
			repne_xcmpsb,					// A6:	repne cmpsb
			repne_xcmpsw,					// A7:	repne cmpsw
			test_al_data8,					// A8:	test	al, DATA8
			test_ax_data16,					// A9:	test	ax, DATA16
			rep_xstosb,						// AA:	rep stosw
			rep_xstosw,						// AB:	rep stosw
			rep_xlodsb,						// AC:	rep lodsb
			rep_xlodsw,						// AD:	rep lodsw
			repne_xscasb,					// AE:	repne scasb
			repne_xscasw,					// AF:	repne scasw

			mov_al_imm,						// B0:	mov		al, imm8
			mov_cl_imm,						// B1:	mov		cl, imm8
			mov_dl_imm,						// B2:	mov		dl, imm8
			mov_bl_imm,						// B3:	mov		bl, imm8
			mov_ah_imm,						// B4:	mov		ah, imm8
			mov_ch_imm,						// B5:	mov		ch, imm8
			mov_dh_imm,						// B6:	mov		dh, imm8
			mov_bh_imm,						// B7:	mov		bh, imm8
			mov_ax_imm,						// B8:	mov		ax, imm16
			mov_cx_imm,						// B9:	mov		cx, imm16
			mov_dx_imm,						// BA:	mov		dx, imm16
			mov_bx_imm,						// BB:	mov		bx, imm16
			mov_sp_imm,						// BC:	mov		sp, imm16
			mov_bp_imm,						// BD:	mov		bp, imm16
			mov_si_imm,						// BE:	mov		si, imm16
			mov_di_imm,						// BF:	mov		di, imm16

			shift_ea8_data8,				// C0:	shift	EA8, DATA8
			shift_ea16_data8,				// C1:	shift	EA16, DATA8
			ret_near_data16,				// C2:	ret near DATA16
			ret_near,						// C3:	ret near
			les_r16_ea,						// C4:	les		REG16, EA
			lds_r16_ea,						// C5:	lds		REG16, EA
			mov_ea8_data8,					// C6:	mov		EA8, DATA8
			mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			leave,							// C9:	leave
			ret_far_data16,					// CA:	ret far	DATA16
			ret_far,						// CB:	ret far
			int_03,							// CC:	int		3
			int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			shift_ea8_1,					// D0:	shift EA8, 1
			shift_ea16_1,					// D1:	shift EA16, 1
			shift_ea8_cl,					// D2:	shift EA8, cl
			shift_ea16_cl,					// D3:	shift EA16, cl
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
			in_al_data8,					// E4:	in		al, DATA8
			in_ax_data8,					// E5:	in		ax, DATA8
			out_data8_al,					// E6:	out		DATA8, al
			out_data8_ax,					// E7:	out		DATA8, ax
			call_near,						// E8:	call near
			jmp_near,						// E9:	jmp near
			jmp_far,						// EA:	jmp far
			jmp_short,						// EB:	jmp short
			in_al_dx,						// EC:	in		al, dx
			in_ax_dx,						// ED:	in		ax, dx
			out_dx_al,						// EE:	out		dx, al
			out_dx_ax,						// EF:	out		dx, ax

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

