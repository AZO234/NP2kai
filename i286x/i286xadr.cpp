#include	"compiler.h"
#include	"cpucore.h"
#include	"i286x.h"
#include	"i286x.mcr"
#include	"i286xea.mcr"


LABEL static void p2ea_nop(void) {

	__asm {
				ret
	 }
}

LABEL static void p2ea_bx_si(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ecx, I286_BX
				add		cx, I286_SI
				add		ecx, DS_FIX
				ret
	}
}

LABEL static void p2ea_bx_si_disp8(void) {

	__asm {
				mov		eax, ebx
				shr		eax, 16
				cbw
				add		ax, I286_BX
				add		ax, I286_SI
				add		eax, DS_FIX
				push	eax
				GET_NEXTPRE3
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bx_si_disp16(void) {

	__asm {
				shr		ebx, 16
				add		bx, I286_BX
				add		bx, I286_SI
				add		ebx, DS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bx_di(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ecx, I286_BX
				add		cx, I286_DI
				add		ecx, DS_FIX
				ret
	}
}

LABEL static void p2ea_bx_di_disp8(void) {

	__asm {
				mov		eax, ebx
				shr		eax, 16
				cbw
				add		ax, I286_BX
				add		ax, I286_DI
				add		eax, DS_FIX
				push	eax
				GET_NEXTPRE3
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bx_di_disp16(void) {

	__asm {
				shr		ebx, 16
				add		bx, I286_BX
				add		bx, I286_DI
				add		ebx, DS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bp_si(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ecx, I286_BP
				add		cx, I286_SI
				add		ecx, SS_FIX
				ret
	}
}

LABEL static void p2ea_bp_si_disp8(void) {

	__asm {
				mov		eax, ebx
				shr		eax, 16
				cbw
				add		ax, I286_BP
				add		ax, I286_SI
				add		eax, SS_FIX
				push	eax
				GET_NEXTPRE3
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bp_si_disp16(void) {

	__asm {
				shr		ebx, 16
				add		bx, I286_BP
				add		bx, I286_SI
				add		ebx, SS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bp_di(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ecx, I286_BP
				add		cx, I286_DI
				add		ecx, SS_FIX
				ret
	}
}

LABEL static void p2ea_bp_di_disp8(void) {

	__asm {
				mov		eax, ebx
				shr		eax, 16
				cbw
				add		ax, I286_BP
				add		ax, I286_DI
				add		eax, SS_FIX
				push	eax
				GET_NEXTPRE3
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bp_di_disp16(void) {

	__asm {
				shr		ebx, 16
				add		bx, I286_BP
				add		bx, I286_DI
				add		ebx, SS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

LABEL static void p2ea_si(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ecx, I286_SI
				add		ecx, DS_FIX
				ret
	}
}

LABEL static void p2ea_si_disp8(void) {

	__asm {
				mov		eax, ebx
				shr		eax, 16
				cbw
				add		ax, I286_SI
				add		eax, DS_FIX
				push	eax
				GET_NEXTPRE3
				pop		ecx
				ret
	}
}

LABEL static void p2ea_si_disp16(void) {

	__asm {
				shr		ebx, 16
				add		bx, I286_SI
				add		ebx, DS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

LABEL static void p2ea_di(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ecx, I286_DI
				add		ecx, DS_FIX
				ret
	}
}

LABEL static void p2ea_di_disp8(void) {

	__asm {
				mov		eax, ebx
				shr		eax, 16
				cbw
				add		ax, I286_DI
				add		eax, DS_FIX
				push	eax
				GET_NEXTPRE3
				pop		ecx
				ret
	}
}

LABEL static void p2ea_di_disp16(void) {

	__asm {
				shr		ebx, 16
				add		bx, I286_DI
				add		ebx, DS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

LABEL static void p2ea_disp16(void) {

	__asm {
				shr		ebx, 16
				add		ebx, DS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bp_disp8(void) {

	__asm {
				mov		eax, ebx
				shr		eax, 16
				cbw
				add		ax, I286_BP
				add		eax, SS_FIX
				push	eax
				GET_NEXTPRE3
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bp_disp16(void) {

	__asm {
				shr		ebx, 16
				add		bx, I286_BP
				add		ebx, SS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bx(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ecx, I286_BX
				add		ecx, DS_FIX
				ret
	}
}

LABEL static void p2ea_bx_disp8(void) {

	__asm {
				mov		eax, ebx
				shr		eax, 16
				cbw
				add		ax, I286_BX
				add		eax, DS_FIX
				push	eax
				GET_NEXTPRE3
				pop		ecx
				ret
	}
}

LABEL static void p2ea_bx_disp16(void) {

	__asm {
				shr		ebx, 16
				add		bx, I286_BX
				add		ebx, DS_FIX
				push	ebx
				GET_NEXTPRE4
				pop		ecx
				ret
	}
}

static const I286TBL peadst_tbl[] = {
				p2ea_bx_si,			p2ea_bx_di,
				p2ea_bp_si,			p2ea_bp_di,
				p2ea_si,			p2ea_di,
				p2ea_disp16,		p2ea_bx,

				p2ea_bx_si_disp8,	p2ea_bx_di_disp8,
				p2ea_bp_si_disp8,	p2ea_bp_di_disp8,
				p2ea_si_disp8,		p2ea_di_disp8,
				p2ea_bp_disp8,		p2ea_bx_disp8,

				p2ea_bx_si_disp16,	p2ea_bx_di_disp16,
				p2ea_bp_si_disp16,	p2ea_bp_di_disp16,
				p2ea_si_disp16,		p2ea_di_disp16,
				p2ea_bp_disp16,		p2ea_bx_disp16};


// --------------------------------------------------------------------------

LABEL static void p2lea_bx_si(void) {

	__asm {
				GET_NEXTPRE2
				mov		ax, I286_BX
				add		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_bx_si_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				cbw
				add		ax, I286_BX
				add		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_bx_si_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				add		ax, I286_BX
				add		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_bx_di(void) {

	__asm {
				GET_NEXTPRE2
				mov		ax, I286_BX
				add		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_bx_di_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				cbw
				add		ax, I286_BX
				add		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_bx_di_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				add		ax, I286_BX
				add		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_bp_si(void) {

	__asm {
				GET_NEXTPRE2
				mov		ax, I286_BP
				add		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_bp_si_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				cbw
				add		ax, I286_BP
				add		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_bp_si_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				add		ax, I286_BP
				add		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_bp_di(void) {

	__asm {
				GET_NEXTPRE2
				mov		ax, I286_BP
				add		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_bp_di_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				cbw
				add		ax, I286_BP
				add		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_bp_di_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				add		ax, I286_BP
				add		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_si(void) {

	__asm {
				GET_NEXTPRE2
				mov		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_si_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				cbw
				add		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_si_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				add		ax, I286_SI
				ret
	}
}

LABEL static void p2lea_di(void) {

	__asm {
				GET_NEXTPRE2
				mov		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_di_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				cbw
				add		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_di_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				add		ax, I286_DI
				ret
	}
}

LABEL static void p2lea_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				ret
	}
}

LABEL static void p2lea_bp_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				cbw
				add		ax, I286_BP
				ret
	}
}

LABEL static void p2lea_bp_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				add		ax, I286_BP
				ret
	}
}

LABEL static void p2lea_bx(void) {

	__asm {
				GET_NEXTPRE2
				mov		ax, I286_BX
				ret
	}
}

LABEL static void p2lea_bx_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				cbw
				add		ax, I286_BX
				ret
	}
}

LABEL static void p2lea_bx_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				add		ax, I286_BX
				ret
	}
}

static const I286TBL plea_tbl[] = {
				p2lea_bx_si,		p2lea_bx_di,
				p2lea_bp_si,		p2lea_bp_di,
				p2lea_si,			p2lea_di,
				p2lea_disp16,		p2lea_bx,

				p2lea_bx_si_disp8,	p2lea_bx_di_disp8,
				p2lea_bp_si_disp8,	p2lea_bp_di_disp8,
				p2lea_si_disp8,		p2lea_di_disp8,
				p2lea_bp_disp8,		p2lea_bx_disp8,

				p2lea_bx_si_disp16,	p2lea_bx_di_disp16,
				p2lea_bp_si_disp16,	p2lea_bp_di_disp16,
				p2lea_si_disp16,	p2lea_di_disp16,
				p2lea_bp_disp16,	p2lea_bx_disp16};


// --------------------------------------------------------------------------

LABEL static void a_bx_si(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ebp, I286_BX
				add		bp, I286_SI
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bx_si_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BX
				add		bp, I286_SI
				cbw
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bx_si_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BX
				add		bp, I286_SI
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bx_di(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ebp, I286_BX
				add		bp, I286_DI
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bx_di_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BX
				add		bp, I286_DI
				cbw
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bx_di_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BX
				add		bp, I286_DI
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bp_si(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ebp, I286_BP
				add		bp, I286_SI
				GET_PREFIX_SS
				ret
	}
}

LABEL static void a_bp_si_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BP
				add		bp, I286_SI
				cbw
				add		bp, ax
				GET_PREFIX_SS
				ret
	}
}

LABEL static void a_bp_si_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BP
				add		bp, I286_SI
				add		bp, ax
				GET_PREFIX_SS
				ret
	}
}

LABEL static void a_bp_di(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ebp, I286_BP
				add		bp, I286_DI
				GET_PREFIX_SS
				ret
	}
}

LABEL static void a_bp_di_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BP
				add		bp, I286_DI
				cbw
				add		bp, ax
				GET_PREFIX_SS
				ret
	}
}

LABEL static void a_bp_di_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BP
				add		bp, I286_DI
				add		bp, ax
				GET_PREFIX_SS
				ret
	}
}

LABEL static void a_si(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ebp, I286_SI
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_si_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_SI
				cbw
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_si_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_SI
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_di(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ebp, I286_DI
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_di_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_DI
				cbw
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_di_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_DI
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		ebp
				shr		ebp, 16
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bp_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BP
				cbw
				add		bp, ax
				GET_PREFIX_SS
				ret
	}
}

LABEL static void a_bp_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BP
				add		bp, ax
				GET_PREFIX_SS
				ret
	}
}

LABEL static void a_bx(void) {

	__asm {
				GET_NEXTPRE2
				movzx	ebp, I286_BX
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bx_disp8(void) {

	__asm {
				push	ebx
				GET_NEXTPRE3
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BX
				cbw
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

LABEL static void a_bx_disp16(void) {

	__asm {
				push	ebx
				GET_NEXTPRE4
				pop		eax
				shr		eax, 16
				movzx	ebp, I286_BX
				add		bp, ax
				GET_PREFIX_DS
				ret
	}
}

static const I286TBL pgetea_tbl[] = {
				a_bx_si,			a_bx_di,
				a_bp_si,			a_bp_di,
				a_si,				a_di,
				a_disp16,			a_bx,

				a_bx_si_disp8,		a_bx_di_disp8,
				a_bp_si_disp8,		a_bp_di_disp8,
				a_si_disp8,			a_di_disp8,
				a_bp_disp8,			a_bx_disp8,

				a_bx_si_disp16,		a_bx_di_disp16,
				a_bp_si_disp16,		a_bp_di_disp16,
				a_si_disp16,		a_di_disp16,
				a_bp_disp16,		a_bx_disp16};


// --------------------------------------------------------------------------

	I286TBL	p_ea_dst[256];
	I286TBL	p_lea[256];
	I286TBL	p_get_ea[256];


void i286xadr_init(void) {

	int		i;
	int		j;

	for	(i=0; i<0x100; i++) {
		if (i < 0xc0) {
			j =	((i >> 3) & 0x18) | (i & 7);
			p_ea_dst[i] = peadst_tbl[j];
			p_lea[i] = plea_tbl[j];
			p_get_ea[i] = pgetea_tbl[j];
		}
		else {
			// óàÇÈéñÇÕÇ»Ç¢î§ÇæÇ™Åc
			p_ea_dst[i] = p2ea_nop;
			p_lea[i] = p2ea_nop;
			p_get_ea[i] = p2ea_nop;
		}
	}
}

