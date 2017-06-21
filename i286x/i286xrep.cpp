#include	"compiler.h"
#include	"cpucore.h"
#include	"i286x.h"
#include	"i286xs.h"
#include	"i286xrep.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"i286x.mcr"
#include	"i286xea.mcr"


#define	LOAD_DS_SI												\
				__asm {	mov		edi, DS_FIX					}	\
				__asm {	movzx	ebp, I286_SI				}

#define	REPLOOP(a)												\
				__asm {	cmp		I286_REMCLOCK, 0			}	\
				__asm {	jg		(a)							}	\
				__asm {	mov		esi, I286_REPPOSBAK			}


// ---------------------------------------------------------------------- ins

I286EXT rep_xinsb(void) {

		__asm {
				cmp		I286_CX, 0
				je		insb_ed
				mov		edi, ES_BASE
				movzx	ebp, I286_DI
				align	4
insb_lp:		I286CLOCK(4)
#if 1
				movzx	ecx, I286_DX
				call	iocore_inp8
#else
				mov		cx, I286_DX
				call	i286_in
#endif
				lea		ecx, [edi + ebp]
				mov		dl, al
				call	i286_memorywrite
				STRING_DIR
				add		bp, ax
				dec		I286_CX
				je		insb_cnt
				REPLOOP(insb_lp)
				mov		I286_DI, bp
				RESET_XPREFETCH
				ret
insb_cnt:		mov		I286_DI, bp
insb_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT rep_xinsw(void) {

		__asm {
				cmp		I286_CX, 0
				je		insw_ed
				mov		edi, ES_BASE
				movzx	ebp, I286_DI
				align	4
insw_lp:		I286CLOCK(4)
				movzx	ecx, I286_DX
				call	iocore_inp16
				lea		ecx, [edi + ebp]
				mov		dx, ax
				call	i286_memorywrite_w
				STRING_DIRx2
				add		bp, ax
				dec		I286_CX
				je		insw_cnt
				REPLOOP(insw_lp)
				mov		I286_DI, bp
				RESET_XPREFETCH
				ret
insw_cnt:		mov		I286_DI, bp
insw_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

// ---------------------------------------------------------------------- outs

I286EXT rep_xoutsb(void) {

		__asm {
				cmp		I286_CX, 0
				je		outsb_ed
				LOAD_DS_SI
				align	4
outsb_lp:		I286CLOCK(4)
				lea		ecx, [edi + ebp]
				call	i286_memoryread
#if 1
				movzx	ecx, I286_DX
				mov		dl, al
				call	iocore_out8
#else
				mov		cx, I286_DX
				mov		dl, al
				call	i286_out
#endif
				STRING_DIR
				add		bp, ax
				dec		I286_CX
				je		outsb_cnt
				REPLOOP(outsb_lp)
				mov		I286_SI, bp
				RESET_XPREFETCH
				ret
outsb_cnt:		mov		I286_SI, bp
outsb_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT rep_xoutsw(void) {

		__asm {
				cmp		I286_CX, 0
				je		outsw_ed
				LOAD_DS_SI
				align	4
outsw_lp:		I286CLOCK(4)
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				movzx	ecx, I286_DX
				mov		dx, ax
				call	iocore_out16
				STRING_DIRx2
				add		bp, ax
				dec		I286_CX
				je		outsw_cnt
				REPLOOP(outsw_lp)
				mov		I286_SI, bp
				RESET_XPREFETCH
				ret
outsw_cnt:		mov		I286_SI, bp
outsw_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

// ---------------------------------------------------------------------- movs

I286EXT rep_xmovsb(void) {

		__asm {
				cmp		I286_CX, 0
				je		movsb_ed
				push	esi
				push	ebx
				LOAD_DS_SI
				mov		esi, ES_BASE
				movzx	ebx, I286_DI
				align	4
movsb_lp:		I286CLOCK(4)
				lea		ecx, [edi + ebp]
				call	i286_memoryread
				mov		edx, eax
				lea		ecx, [esi + ebx]
				call	i286_memorywrite
				STRING_DIR
				add		bp, ax
				add		bx, ax
				dec		I286_CX
				je		movsb_cnt
				REPLOOP(movsb_lp)
				mov		I286_SI, bp
				mov		I286_DI, bx
				pop		ebx
				pop		ecx
				RESET_XPREFETCH
				ret
movsb_cnt:		mov		I286_SI, bp
				mov		I286_DI, bx
				pop		ebx
				pop		esi
movsb_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT rep_xmovsw(void) {

		__asm {
				cmp		I286_CX, 0
				je		movsw_ed
				push	esi
				push	ebx
				LOAD_DS_SI
				mov		esi, ES_BASE
				movzx	ebx, I286_DI
				align	4
movsw_lp:		I286CLOCK(4)
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		edx, eax
				lea		ecx, [esi + ebx]
				call	i286_memorywrite_w
				STRING_DIRx2
				add		bp, ax
				add		bx, ax
				dec		I286_CX
				je		movsw_cnt
				REPLOOP(movsw_lp)
				mov		I286_SI, bp
				mov		I286_DI, bx
				pop		ebx
				pop		ecx
				RESET_XPREFETCH
				ret
movsw_cnt:		mov		I286_SI, bp
				mov		I286_DI, bx
				pop		ebx
				pop		esi
movsw_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

// ---------------------------------------------------------------------- lods

I286EXT rep_xlodsb(void) {

		__asm {
				cmp		I286_CX, 0
				je		lodsb_ed
				LOAD_DS_SI
				align	4
lodsb_lp:		I286CLOCK(4)
				lea		ecx, [edi + ebp]
				call	i286_memoryread
				mov		I286_AL, al
				STRING_DIR
				add		bp, ax
				dec		I286_CX
				je		lodsb_cnt
				REPLOOP(lodsb_lp)
				mov		I286_SI, bp
				RESET_XPREFETCH
				ret
lodsb_cnt:		mov		I286_SI, bp
lodsb_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT rep_xlodsw(void) {

		__asm {
				cmp		I286_CX, 0
				je		lodsw_ed
				LOAD_DS_SI
				align	4
lodsw_lp:		I286CLOCK(4)
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		I286_AX, ax
				STRING_DIRx2
				add		bp, ax
				dec		I286_CX
				je		lodsw_cnt
				REPLOOP(lodsw_lp)
				mov		I286_SI, bp
				RESET_XPREFETCH
				ret
lodsw_cnt:		mov		I286_SI, bp
lodsw_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

// ---------------------------------------------------------------------- stos

I286EXT rep_xstosb(void) {

		__asm {
				cmp		I286_CX, 0
				je		stosb_ed
				mov		edi, ES_BASE
				movzx	ebp, I286_DI
				align	4
stosb_lp:		I286CLOCK(3)
				lea		ecx, [edi + ebp]
				mov		dl, I286_AL
				call	i286_memorywrite
				STRING_DIR
				add		bp, ax
				dec		I286_CX
				je		stosb_cnt
				REPLOOP(stosb_lp)
				mov		I286_DI, bp
				RESET_XPREFETCH
				ret
stosb_cnt:		mov		I286_DI, bp
stosb_ed:		I286CLOCK(4)
				GET_NEXTPRE1
				ret
		}
}

I286EXT rep_xstosw(void) {

		__asm {
				cmp		I286_CX, 0
				je		stosw_ed
				mov		edi, ES_BASE
				movzx	ebp, I286_DI
				align	4
stosw_lp:		I286CLOCK(3)
				lea		ecx, [edi + ebp]
				mov		dx, I286_AX
				call	i286_memorywrite_w
				STRING_DIRx2
				add		bp, ax
				dec		I286_CX
				je		stosw_cnt
				REPLOOP(stosw_lp)
				mov		I286_DI, bp
				RESET_XPREFETCH
				ret
stosw_cnt:		mov		I286_DI, bp
stosw_ed:		I286CLOCK(4)
				GET_NEXTPRE1
				ret
		}
}

// ---------------------------------------------------------------------- cmps

I286EXT repe_xcmpsb(void) {

		__asm {
				cmp		I286_CX, 0
				je		cmpsb_ed
				push	esi
				push	ebx
				LOAD_DS_SI
				mov		esi, ES_BASE
				movzx	ebx, I286_DI
zcmpsb_lp:		I286CLOCK(9)
				lea		ecx, [edi + ebp]
				call	i286_memoryread
				mov		dl, al
				lea		ecx, [esi + ebx]
				call	i286_memoryread
				cmp		dl, al
				FLAG_STORE_OF
				STRING_DIR
				add		bp, ax
				add		bx, ax
				dec		I286_CX
				je		zcmpsb_ed
				test	I286_FLAG, Z_FLAG
				jne		zcmpsb_lp
zcmpsb_ed:		mov		I286_SI, bp
				mov		I286_DI, bx
				pop		ebx
				pop		esi
cmpsb_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT repne_xcmpsb(void) {

		__asm {
				cmp		I286_CX, 0
				je		cmpsb_ed
				push	esi
				push	ebx
				LOAD_DS_SI
				mov		esi, ES_BASE
				movzx	ebx, I286_DI
nzcmpsb_lp:		I286CLOCK(9)
				lea		ecx, [edi + ebp]
				call	i286_memoryread
				mov		dl, al
				lea		ecx, [esi + ebx]
				call	i286_memoryread
				cmp		dl, al
				FLAG_STORE_OF
				STRING_DIR
				add		bp, ax
				add		bx, ax
				dec		I286_CX
				je		nzcmpsb_ed
				test	I286_FLAG, Z_FLAG
				je		nzcmpsb_lp
nzcmpsb_ed:		mov		I286_SI, bp
				mov		I286_DI, bx
				pop		ebx
				pop		esi
cmpsb_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT repe_xcmpsw(void) {

		__asm {
				cmp		I286_CX, 0
				je		cmpsw_ed
				push	esi
				push	ebx
				LOAD_DS_SI
				mov		esi, ES_BASE
				movzx	ebx, I286_DI
zcmpsw_lp:		I286CLOCK(9)
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		dx, ax
				lea		ecx, [esi + ebx]
				call	i286_memoryread_w
				cmp		dx, ax
				FLAG_STORE_OF
				STRING_DIRx2
				add		bp, ax
				add		bx, ax
				dec		I286_CX
				je		zcmpsw_ed
				test	I286_FLAG, Z_FLAG
				jne		zcmpsw_lp
zcmpsw_ed:		mov		I286_SI, bp
				mov		I286_DI, bx
				pop		ebx
				pop		esi
cmpsw_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT repne_xcmpsw(void) {

		__asm {
				cmp		I286_CX, 0
				je		cmpsw_ed
				push	esi
				push	ebx
				LOAD_DS_SI
				mov		esi, ES_BASE
				movzx	ebx, I286_DI
nzcmpsw_lp:		I286CLOCK(9)
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				mov		dx, ax
				lea		ecx, [esi + ebx]
				call	i286_memoryread_w
				cmp		dx, ax
				FLAG_STORE_OF
				STRING_DIRx2
				add		bp, ax
				add		bx, ax
				dec		I286_CX
				je		nzcmpsw_ed
				test	I286_FLAG, Z_FLAG
				je		nzcmpsw_lp
nzcmpsw_ed:		mov		I286_SI, bp
				mov		I286_DI, bx
				pop		ebx
				pop		esi
cmpsw_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

// ---------------------------------------------------------------------- scas

I286EXT repe_xscasb(void) {

		__asm {
				cmp		I286_CX, 0
				je		scasb_ed
				mov		edi, ES_BASE
				movzx	ebp, I286_DI
zscasb_lp:		I286CLOCK(8)
				lea		ecx, [edi + ebp]
				call	i286_memoryread
				cmp		I286_AL, al
				FLAG_STORE_OF
				STRING_DIR
				add		bp, ax
				dec		I286_CX
				je		zscasb_ed
				test	I286_FLAG, Z_FLAG
				jne		zscasb_lp
zscasb_ed:		mov		I286_DI, bp
scasb_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT repne_xscasb(void) {

		__asm {
				cmp		I286_CX, 0
				je		scasb_ed
				mov		edi, ES_BASE
				movzx	ebp, I286_DI
nzscasb_lp:		I286CLOCK(8)
				lea		ecx, [edi + ebp]
				call	i286_memoryread
				cmp		I286_AL, al
				FLAG_STORE_OF
				STRING_DIR
				add		bp, ax
				dec		I286_CX
				je		nzscasb_ed
				test	I286_FLAG, Z_FLAG
				je		nzscasb_lp
nzscasb_ed:		mov		I286_DI, bp
scasb_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT repe_xscasw(void) {

		__asm {
				cmp		I286_CX, 0
				je		scasw_ed
				mov		edi, ES_BASE
				movzx	ebp, I286_DI
zscasw_lp:		I286CLOCK(8)
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				cmp		I286_AX, ax
				FLAG_STORE_OF
				STRING_DIRx2
				add		bp, ax
				dec		I286_CX
				je		zscasw_ed
				test	I286_FLAG, Z_FLAG
				jne		zscasw_lp
zscasw_ed:		mov		I286_DI, bp
scasw_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

I286EXT repne_xscasw(void) {

		__asm {
				cmp		I286_CX, 0
				je		scasw_ed
				mov		edi, ES_BASE
				movzx	ebp, I286_DI
nzscasw_lp:		I286CLOCK(8)
				lea		ecx, [edi + ebp]
				call	i286_memoryread_w
				cmp		I286_AX, ax
				FLAG_STORE_OF
				STRING_DIRx2
				add		bp, ax
				dec		I286_CX
				je		nzscasw_ed
				test	I286_FLAG, Z_FLAG
				je		nzscasw_lp
nzscasw_ed:		mov		I286_DI, bp
scasw_ed:		I286CLOCK(5)
				GET_NEXTPRE1
				ret
		}
}

