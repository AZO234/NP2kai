#include	"compiler.h"
#include	"cpucore.h"
#include	"i286x.h"
#include	"i286xadr.h"
#include	"i286xcts.h"
#include	"i286x.mcr"
#include	"i286xea.mcr"


typedef void (*I286OP_0F)(void);


// ---- 0f 00

I286 _sldt(void) {

		__asm {
				PREPART_EA16(2)
					mov		dx, I286_LDTR
					mov		word ptr I286_REG[eax*2], dx
					GET_NEXTPRE2
					ret
				MEMORY_EA16(3)
					mov		ax, I286_LDTR
					mov		word ptr I286_MEM[ecx], ax
					ret
				extmem_eareg16:
					mov		dx, I286_LDTR
					jmp		i286_memorywrite_w
		}
}

I286 _str(void) {

		__asm {
				PREPART_EA16(3)
					mov		dx, I286_TR
					mov		word ptr I286_REG[eax*2], dx
					GET_NEXTPRE2
					ret
				MEMORY_EA16(6)
					mov		ax, I286_TR
					mov		word ptr I286_MEM[ecx], ax
					ret
				extmem_eareg16:
					mov		dx, I286_TR
					jmp		i286_memorywrite_w
		}
}

I286 _lldt(void) {

		__asm {
				PREPART_EA16(17)
					mov		ax, word ptr I286_REG[eax*2]
					call	lldt_sub
					GET_NEXTPRE2
					ret
				MEMORY_EA16(19)
					mov		ax, word ptr I286_MEM[ecx]
					jmp		short lldt_sub
				EXTMEM_EA16

lldt_sub:			mov		word ptr I286_LDTR, ax
					call	i286x_selector
					mov		ecx, eax
					call	i286_memoryread_w
					mov		I286_LDTRC.limit, ax
					add		ecx, 2
					call	i286_memoryread_w
					mov		I286_LDTRC.base, ax
					add		ecx, 2
					call	i286_memoryread
					mov		I286_LDTRC.base24, al
					ret
		}
}

I286 _ltr(void) {

		__asm {
				PREPART_EA16(17)
					mov		ax, word ptr I286_REG[eax*2]
					call	ltr_sub
					GET_NEXTPRE2
					ret
				MEMORY_EA16(19)
					mov		ax, word ptr I286_MEM[ecx]
					jmp		short ltr_sub
				EXTMEM_EA16

ltr_sub:			mov		word ptr I286_TR, ax
					call	i286x_selector
					mov		ecx, eax
					call	i286_memoryread_w
					mov		I286_TRC.limit, ax
					add		ecx, 2
					call	i286_memoryread_w
					mov		I286_TRC.base, ax
					add		ecx, 2
					call	i286_memoryread
					mov		I286_TRC.base24, al
					ret
		}
}

I286 _verr(void) {

		__asm {
				PREPART_EA16(14)
					mov		ax, word ptr I286_REG[eax*2]
					GET_NEXTPRE2
					ret
				MEMORY_EA16(16)
					mov		ax, word ptr I286_MEM[ecx]
					ret
				EXTMEM_EA16
					ret
		}
}

I286 _verw(void) {

		__asm {
				PREPART_EA16(14)
					mov		ax, word ptr I286_REG[eax*2]
					GET_NEXTPRE2
					ret
				MEMORY_EA16(16)
					mov		ax, word ptr I286_MEM[ecx]
					ret
				EXTMEM_EA16
					ret
		}
}

static const I286OP_0F cts0x_table[] = {
			_sldt,	_str,	_lldt,	_ltr,
			_verr,	_verw,	_verr,	_verw};


// ---- 0f 01

I286 _sgdt(void) {

		__asm {
				cmp		al, 0c0h
				jnc		register_eareg16
				I286CLOCK(11)
				call	p_ea_dst[eax*4]
				mov		dx, word ptr I286_GDTR
				call	i286_memorywrite_w
				add		ecx, 2
				mov		dx, word ptr (I286_GDTR + 2)
				call	i286_memorywrite_w
				add		ecx, 2
				mov		dl, byte ptr (I286_GDTR + 4)
				mov		dh, -1
				jmp		i286_memorywrite_w
				align	4
		register_eareg16:
				INT_NUM(6)
		}
}

I286 _sidt(void) {

		__asm {
				cmp		al, 0c0h
				jnc		register_eareg16
				I286CLOCK(12)
				call	p_ea_dst[eax*4]
				mov		dx, word ptr I286_IDTR
				call	i286_memorywrite_w
				add		ecx, 2
				mov		dx, word ptr (I286_IDTR + 2)
				call	i286_memorywrite_w
				add		ecx, 2
				mov		dl, byte ptr (I286_IDTR + 4)
				mov		dh, -1
				jmp		i286_memorywrite_w
				align	4
		register_eareg16:
				INT_NUM(6)
		}
}

I286 _lgdt(void) {

		__asm {
				cmp		al, 0c0h
				jnc		register_eareg16
				I286CLOCK(11)
				call	p_ea_dst[eax*4]
				call	i286_memoryread_w
				mov		word ptr I286_GDTR, ax
				add		ecx, 2
				call	i286_memoryread_w
				mov		word ptr (I286_GDTR + 2), ax
				add		ecx, 2
				call	i286_memoryread
				mov		byte ptr (I286_GDTR + 4), al
				ret
				align	4
		register_eareg16:
				INT_NUM(6)
		}
}

I286 _lidt(void) {

		__asm {
				cmp		al, 0c0h
				jnc		register_eareg16
				I286CLOCK(12)
				call	p_ea_dst[eax*4]
				call	i286_memoryread_w
				mov		word ptr I286_IDTR, ax
				add		ecx, 2
				call	i286_memoryread_w
				mov		word ptr (I286_IDTR + 2), ax
				add		ecx, 2
				call	i286_memoryread
				mov		byte ptr (I286_IDTR + 4), al
				ret
				align	4
		register_eareg16:
				INT_NUM(6)
		}
}

I286 _smsw(void) {

		__asm {
				PREPART_EA16(3)
					mov		dx, I286_MSW
					mov		word ptr I286_REG[eax*2], dx
					GET_NEXTPRE2
					ret
				MEMORY_EA16(6)
					mov		ax, I286_MSW
					mov		word ptr I286_MEM[ecx], ax
					ret
				extmem_eareg16:
					mov		dx, I286_MSW
					jmp		i286_memorywrite_w
		}
}

I286 _lmsw(void) {

		__asm {
				and		I286_MSW, MSW_PE
				PREPART_EA16(2)
					mov		ax, word ptr I286_REG[eax*2]
					or		I286_MSW, ax
					GET_NEXTPRE2
					ret
				MEMORY_EA16(3)
					mov		ax, word ptr I286_MEM[ecx]
					or		I286_MSW, ax
					ret
				EXTMEM_EA16
					or		I286_MSW, ax
					ret
		}
}

static const I286OP_0F cts1x_table[] = {
			_sgdt,	_sidt,	_lgdt,	_lidt,
			_smsw,	_smsw,	_lmsw,	_lmsw};


// ----

I286EXT _xcts(void) {

		__asm {
				mov		edi, esi
				GET_NEXTPRE1
				test	bl, bl
				je		short i286_cts0
				dec		bl
				je		short i286_cts1
				cmp		bl, (5 - 1)
				je		short loadall286
				jmp		expint6

				align	4
i286_cts0:		test	I286_MSW, MSW_PE
				je		expint6
				movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4
				jmp		cts0x_table[edi]

				align	4
i286_cts1:		movzx	eax, bh
				mov		edi, eax
				shr		edi, 3-2
				and		edi, 7*4
				jmp		cts1x_table[edi]

				align	4
loadall286:		I286CLOCK(195)
				mov		ax, word ptr mem[0x0804]		// MSW
				mov		I286_MSW, ax
				mov		ax, word ptr mem[0x0816]		// TR
				mov		I286_TR, ax
				mov		ax, word ptr mem[0x0818]		// flag
				mov		I286_FLAG, ax
				and		ah, 3
				cmp		ah, 3
				sete	I286_TRAP
				mov		si, word ptr mem[0x081a]		// ip
				mov		ax, word ptr mem[0x081c]		// LDTR
				mov		I286_LDTR, ax
				mov		ax, word ptr mem[0x081e]		// ds
				mov		I286_DS, ax
				mov		ax, word ptr mem[0x0820]		// ss
				mov		I286_SS, ax
				mov		ax, word ptr mem[0x0822]		// cs
				mov		I286_CS, ax
				mov		ax, word ptr mem[0x0824]		// es
				mov		I286_ES, ax
				mov		ax, word ptr mem[0x0826]		// di
				mov		I286_DI, ax
				mov		ax, word ptr mem[0x0828]		// si
				mov		I286_SI, ax
				mov		ax, word ptr mem[0x082a]		// bp
				mov		I286_BP, ax
				mov		ax, word ptr mem[0x082c]		// sp
				mov		I286_SP, ax
				mov		ax, word ptr mem[0x082e]		// bx
				mov		I286_BX, ax
				mov		ax, word ptr mem[0x0830]		// dx
				mov		I286_DX, ax
				mov		ax, word ptr mem[0x0832]		// cx
				mov		I286_CX, ax
				mov		ax, word ptr mem[0x0834]		// ax
				mov		I286_AX, ax
				mov		eax, dword ptr mem[0x0836]		// es_desc
				and		eax, 00ffffffh
				mov		ES_BASE, eax
				mov		eax, dword ptr mem[0x083c]		// cs_desc
				and		eax, 00ffffffh
				mov		CS_BASE, eax
				mov		eax, dword ptr mem[0x0842]		// ss_desc
				and		eax, 00ffffffh
				mov		SS_BASE, eax
				mov		SS_FIX, eax
				mov		eax, dword ptr mem[0x0848]		// ds_desc
				and		eax, 00ffffffh
				mov		DS_BASE, eax
				mov		DS_FIX, eax

				mov		eax, dword ptr mem[0x084e]		// GDTR
				mov		dword ptr (I286_GDTR.base), eax
				mov		ax, word ptr mem[0x0852]
				mov		I286_GDTR.limit, ax

				mov		eax, dword ptr mem[0x0854]		// LDTRC
				mov		dword ptr (I286_LDTRC.base), eax
				mov		ax, word ptr mem[0x0858]
				mov		I286_LDTRC.limit, ax

				mov		eax, dword ptr mem[0x085a]		// IDTR
				mov		dword ptr (I286_IDTR.base), eax
				mov		ax, word ptr mem[0x085e]
				mov		I286_IDTR.limit, ax

				mov		eax, dword ptr mem[0x0860]		// TRC
				mov		dword ptr (I286_TRC.base), eax
				mov		ax, word ptr mem[0x0864]
				mov		I286_TRC.limit, ax

				RESET_XPREFETCH
				I286IRQCHECKTERM

				align	4
expint6:		mov		si, di					// ver0.27 このタイプ・・・
				I286CLOCK(20)					// 全部修正しなきゃ(汗
				INT_NUM(6)						// i286とi386で挙動が違うから
		}										// いやらしいね…
}

