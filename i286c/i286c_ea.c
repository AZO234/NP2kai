#include	"compiler.h"
#include	"cpucore.h"
#include	"i286c.h"
#include	"i286c.mcr"


enum {
	EA_BX_SI		= 0,
	EA_BX_DI,
	EA_BP_SI,
	EA_BP_DI,
	EA_SI,
	EA_DI,
	EA_DISP16,
	EA_BX,
	EA_BX_SI_DISP8,
	EA_BX_DI_DISP8,
	EA_BP_SI_DISP8,
	EA_BP_DI_DISP8,
	EA_SI_DISP8,
	EA_DI_DISP8,
	EA_BP_DISP8,
	EA_BX_DISP8,
	EA_BX_SI_DISP16,
	EA_BX_DI_DISP16,
	EA_BP_SI_DISP16,
	EA_BP_DI_DISP16,
	EA_SI_DISP16,
	EA_DI_DISP16,
	EA_BP_DISP16,
	EA_BX_DISP16
};


#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)

static UINT32 ea_bx_si(void) {

	return(LOW16(I286_BX + I286_SI) + DS_FIX);
}

static UINT32 ea_bx_si_disp8(void) {

	SINT32	adrs;

	GET_PCBYTESD(adrs);
	return(LOW16(adrs + I286_BX + I286_SI) + DS_FIX);
}

static UINT32 ea_bx_si_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BX + I286_SI) + DS_FIX);
}

static UINT32 ea_bx_di(void) {

	return(LOW16(I286_BX + I286_DI) + DS_FIX);
}

static UINT32 ea_bx_di_disp8(void) {

	SINT32	adrs;

	GET_PCBYTESD(adrs);
	return(LOW16(adrs + I286_BX + I286_DI) + DS_FIX);
}

static UINT32 ea_bx_di_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BX + I286_DI) + DS_FIX);
}

static UINT32 ea_bp_si(void) {

	return(LOW16(I286_BP + I286_SI) + SS_FIX);
}

static UINT32 ea_bp_si_disp8(void) {

	SINT32	adrs;

	GET_PCBYTESD(adrs);
	return(LOW16(adrs + I286_BP + I286_SI) + SS_FIX);
}

static UINT32 ea_bp_si_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BP + I286_SI) + SS_FIX);
}

static UINT32 ea_bp_di(void) {

	return(LOW16(I286_BP + I286_DI) + SS_FIX);
}

static UINT32 ea_bp_di_disp8(void) {

	SINT32	adrs;

	GET_PCBYTESD(adrs);
	return(LOW16(adrs + I286_BP + I286_DI) + SS_FIX);
}

static UINT32 ea_bp_di_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BP + I286_DI) + SS_FIX);
}

static UINT32 ea_si(void) {

	return(I286_SI + DS_FIX);
}

static UINT32 ea_si_disp8(void) {

	SINT32	adrs;

	GET_PCBYTESD(adrs);
	return(LOW16(adrs + I286_SI) + DS_FIX);
}

static UINT32 ea_si_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_SI) + DS_FIX);
}

static UINT32 ea_di(void) {

	return(I286_DI + DS_FIX);
}

static UINT32 ea_di_disp8(void) {

	SINT32	adrs;

	GET_PCBYTESD(adrs);
	return(LOW16(adrs + I286_DI) + DS_FIX);
}

static UINT32 ea_di_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_DI) + DS_FIX);
}

static UINT32 ea_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(adrs + DS_FIX);
}

static UINT32 ea_bp_disp8(void) {

	SINT32	adrs;

	GET_PCBYTESD(adrs);
	return(LOW16(adrs + I286_BP) + SS_FIX);
}

static UINT32 ea_bp_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BP) + SS_FIX);
}

static UINT32 ea_bx(void) {

	return(I286_BX + DS_FIX);
}

static UINT32 ea_bx_disp8(void) {

	SINT32	adrs;

	GET_PCBYTESD(adrs);
	return(LOW16(adrs + I286_BX) + DS_FIX);
}

static UINT32 ea_bx_disp16(void) {

	UINT32	adrs;

	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BX) + DS_FIX);
}

static const CALCEA i286c_ea_dst_tbl[] = {
			ea_bx_si,			ea_bx_di,
			ea_bp_si,			ea_bp_di,
			ea_si,				ea_di,
			ea_disp16,			ea_bx,
			ea_bx_si_disp8,		ea_bx_di_disp8,
			ea_bp_si_disp8,		ea_bp_di_disp8,
			ea_si_disp8,		ea_di_disp8,
			ea_bp_disp8,		ea_bx_disp8,
			ea_bx_si_disp16,	ea_bx_di_disp16,
			ea_bp_si_disp16,	ea_bp_di_disp16,
			ea_si_disp16,		ea_di_disp16,
			ea_bp_disp16,		ea_bx_disp16};


// ----

static UINT16 lea_bx_si(void) {

	return(I286_BX + I286_SI);
}

static UINT16 lea_bx_si_disp8(void) {

	UINT16	adrs;

	GET_PCBYTES(adrs);
	return(adrs + I286_BX + I286_SI);
}

static UINT16 lea_bx_si_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs + I286_BX + I286_SI);
}

static UINT16 lea_bx_di(void) {

	return(I286_BX + I286_DI);
}

static UINT16 lea_bx_di_disp8(void) {

	UINT16	adrs;

	GET_PCBYTES(adrs);
	return(adrs + I286_BX + I286_DI);
}

static UINT16 lea_bx_di_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs + I286_BX + I286_DI);
}

static UINT16 lea_bp_si(void) {

	return(I286_BP + I286_SI);
}

static UINT16 lea_bp_si_disp8(void) {

	UINT16	adrs;

	GET_PCBYTES(adrs);
	return(adrs + I286_BP + I286_SI);
}

static UINT16 lea_bp_si_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs + I286_BP + I286_SI);
}

static UINT16 lea_bp_di(void) {

	return(I286_BP + I286_DI);
}

static UINT16 lea_bp_di_disp8(void) {

	UINT16	adrs;

	GET_PCBYTES(adrs);
	return(adrs + I286_BP + I286_DI);
}

static UINT16 lea_bp_di_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs + I286_BP + I286_DI);
}

static UINT16 lea_si(void) {

	return(I286_SI);
}

static UINT16 lea_si_disp8(void) {

	UINT16	adrs;

	GET_PCBYTES(adrs);
	return(adrs + I286_SI);
}

static UINT16 lea_si_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs + I286_SI);
}

static UINT16 lea_di(void) {

	return(I286_DI);
}

static UINT16 lea_di_disp8(void) {

	UINT16	adrs;

	GET_PCBYTES(adrs);
	return(adrs + I286_DI);
}

static UINT16 lea_di_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs + I286_DI);
}

static UINT16 lea_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs);
}

static UINT16 lea_bp_disp8(void) {

	UINT16	adrs;

	GET_PCBYTES(adrs);
	return(adrs + I286_BP);
}

static UINT16 lea_bp_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs + I286_BP);
}

static UINT16 lea_bx(void) {

	return(I286_BX);
}

static UINT16 lea_bx_disp8(void) {

	UINT16	adrs;

	GET_PCBYTES(adrs);
	return(adrs + I286_BX);
}

static UINT16 lea_bx_disp16(void) {

	UINT16	adrs;

	GET_PCWORD(adrs);
	return(adrs + I286_BX);
}

static const CALCLEA i286c_lea_tbl[] = {
			lea_bx_si,			lea_bx_di,
			lea_bp_si,			lea_bp_di,
			lea_si,				lea_di,
			lea_disp16,			lea_bx,
			lea_bx_si_disp8,	lea_bx_di_disp8,
			lea_bp_si_disp8,	lea_bp_di_disp8,
			lea_si_disp8,		lea_di_disp8,
			lea_bp_disp8,		lea_bx_disp8,
			lea_bx_si_disp16,	lea_bx_di_disp16,
			lea_bp_si_disp16,	lea_bp_di_disp16,
			lea_si_disp16,		lea_di_disp16,
			lea_bp_disp16,		lea_bx_disp16};


// ----

static UINT a_bx_si(UINT32 *seg) {

	*seg = DS_FIX;
	return(LOW16(I286_BX + I286_SI));
}

static UINT a_bx_si_disp8(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCBYTES(adrs);
	return(LOW16(adrs + I286_BX + I286_SI));
}

static UINT a_bx_si_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BX + I286_SI));
}

static UINT a_bx_di(UINT32 *seg) {

	*seg = DS_FIX;
	return(LOW16(I286_BX + I286_DI));
}

static UINT a_bx_di_disp8(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCBYTES(adrs);
	return(LOW16(adrs + I286_BX + I286_DI));
}

static UINT a_bx_di_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BX + I286_DI));
}

static UINT a_bp_si(UINT32 *seg) {

	*seg = SS_FIX;
	return(LOW16(I286_BP + I286_SI));
}

static UINT a_bp_si_disp8(UINT32 *seg) {

	UINT	adrs;

	*seg = SS_FIX;
	GET_PCBYTES(adrs);
	return(LOW16(adrs + I286_BP + I286_SI));
}

static UINT a_bp_si_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = SS_FIX;
	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BP + I286_SI));
}

static UINT a_bp_di(UINT32 *seg) {

	*seg = SS_FIX;
	return(LOW16(I286_BP + I286_DI));
}

static UINT a_bp_di_disp8(UINT32 *seg) {

	UINT	adrs;

	*seg = SS_FIX;
	GET_PCBYTES(adrs);
	return(LOW16(adrs + I286_BP + I286_DI));
}

static UINT a_bp_di_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = SS_FIX;
	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BP + I286_DI));
}

static UINT a_si(UINT32 *seg) {

	*seg = DS_FIX;
	return(I286_SI);
}

static UINT a_si_disp8(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCBYTES(adrs);
	return(LOW16(adrs + I286_SI));
}

static UINT a_si_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_SI));
}

static UINT a_di(UINT32 *seg) {

	*seg = DS_FIX;
	return(I286_DI);
}

static UINT a_di_disp8(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCBYTES(adrs);
	return(LOW16(adrs + I286_DI));
}

static UINT a_di_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_DI));
}

static UINT a_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCWORD(adrs);
	return(adrs);
}

static UINT a_bp_disp8(UINT32 *seg) {

	UINT	adrs;

	*seg = SS_FIX;
	GET_PCBYTES(adrs);
	return(LOW16(adrs + I286_BP));
}

static UINT a_bp_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = SS_FIX;
	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BP));
}

static UINT a_bx(UINT32 *seg) {

	*seg = DS_FIX;
	return(I286_BX);
}

static UINT a_bx_disp8(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCBYTES(adrs);
	return(LOW16(adrs + I286_BX));
}

static UINT a_bx_disp16(UINT32 *seg) {

	UINT	adrs;

	*seg = DS_FIX;
	GET_PCWORD(adrs);
	return(LOW16(adrs + I286_BX));
}

static const GETLEA i286c_ea_tbl[] = {
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


// ----

	CALCEA	_calc_ea_dst[256];
	CALCLEA	_calc_lea[192];
	GETLEA	_get_ea[192];

static UINT32 ea_nop(void) {

	return(0);
}

void i286cea_initialize(void) {

	UINT	i;
	UINT	pos;

	for (i=0; i<0xc0; i++) {
		pos = ((i >> 3) & 0x18) + (i & 0x07);
		_calc_ea_dst[i] = i286c_ea_dst_tbl[pos];
		_calc_lea[i] = i286c_lea_tbl[pos];
		_get_ea[i] = i286c_ea_tbl[pos];
	}
	for (; i<0x100; i++) {
		_calc_ea_dst[i] = ea_nop;
	}
}

#else								// ARM‚¾‚Æswitch‚É‚µ‚½‚Ù[‚ª‘‚¢‚Í‚¸c

UINT32 calc_ea_dst(UINT op) {

	UINT32	adrs;

	switch(((op >> 3) & 0x18) + (op & 0x07)) {
		case EA_BX_SI:
			return(LOW16(I286_BX + I286_SI) + DS_FIX);

		case EA_BX_SI_DISP8:
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BX + I286_SI) + DS_FIX);

		case EA_BX_SI_DISP16:
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BX + I286_SI) + DS_FIX);

		case EA_BX_DI:
			return(LOW16(I286_BX + I286_DI) + DS_FIX);

		case EA_BX_DI_DISP8:
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BX + I286_DI) + DS_FIX);

		case EA_BX_DI_DISP16:
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BX + I286_DI) + DS_FIX);

		case EA_BP_SI:
			return(LOW16(I286_BP + I286_SI) + SS_FIX);

		case EA_BP_SI_DISP8:
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BP + I286_SI) + SS_FIX);

		case EA_BP_SI_DISP16:
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BP + I286_SI) + SS_FIX);

		case EA_BP_DI:
			return(LOW16(I286_BP + I286_DI) + SS_FIX);

		case EA_BP_DI_DISP8:
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BP + I286_DI) + SS_FIX);

		case EA_BP_DI_DISP16:
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BP + I286_DI) + SS_FIX);

		case EA_SI:
			return(I286_SI + DS_FIX);

		case EA_SI_DISP8:
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_SI) + DS_FIX);

		case EA_SI_DISP16:
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_SI) + DS_FIX);

		case EA_DI:
			return(I286_DI + DS_FIX);

		case EA_DI_DISP8:
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_DI) + DS_FIX);

		case EA_DI_DISP16:
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_DI) + DS_FIX);

		case EA_BX:
			return(I286_BX + DS_FIX);

		case EA_BX_DISP8:
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BX) + DS_FIX);

		case EA_BX_DISP16:
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BX) + DS_FIX);

		case EA_BP_DISP8:
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BP) + SS_FIX);

		case EA_BP_DISP16:
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BP) + SS_FIX);

		case EA_DISP16:
			GET_PCWORD(adrs);
			return(adrs + DS_FIX);

		default:
			return(0);
	}
}

UINT16 calc_lea(UINT op) {

	UINT16	adrs;

	switch(((op >> 3) & 0x18) + (op & 0x07)) {
		case EA_BX_SI:
			return(I286_BX + I286_SI);

		case EA_BX_SI_DISP8:
			GET_PCBYTESD(adrs);
			return(adrs + I286_BX + I286_SI);

		case EA_BX_SI_DISP16:
			GET_PCWORD(adrs);
			return(adrs + I286_BX + I286_SI);

		case EA_BX_DI:
			return(I286_BX + I286_DI);

		case EA_BX_DI_DISP8:
			GET_PCBYTESD(adrs);
			return(adrs + I286_BX + I286_DI);

		case EA_BX_DI_DISP16:
			GET_PCWORD(adrs);
			return(adrs + I286_BX + I286_DI);

		case EA_BP_SI:
			return(I286_BP + I286_SI);

		case EA_BP_SI_DISP8:
			GET_PCBYTESD(adrs);
			return(adrs + I286_BP + I286_SI);

		case EA_BP_SI_DISP16:
			GET_PCWORD(adrs);
			return(adrs + I286_BP + I286_SI);

		case EA_BP_DI:
			return(I286_BP + I286_DI);

		case EA_BP_DI_DISP8:
			GET_PCBYTESD(adrs);
			return(adrs + I286_BP + I286_DI);

		case EA_BP_DI_DISP16:
			GET_PCWORD(adrs);
			return(adrs + I286_BP + I286_DI);

		case EA_SI:
			return(I286_SI);

		case EA_SI_DISP8:
			GET_PCBYTESD(adrs);
			return(adrs + I286_SI);

		case EA_SI_DISP16:
			GET_PCWORD(adrs);
			return(adrs + I286_SI);

		case EA_DI:
			return(I286_DI);

		case EA_DI_DISP8:
			GET_PCBYTESD(adrs);
			return(adrs + I286_DI);

		case EA_DI_DISP16:
			GET_PCWORD(adrs);
			return(adrs + I286_DI);

		case EA_BX:
			return(I286_BX);

		case EA_BX_DISP8:
			GET_PCBYTESD(adrs);
			return(adrs + I286_BX);

		case EA_BX_DISP16:
			GET_PCWORD(adrs);
			return(adrs + I286_BX);

		case EA_BP_DISP8:
			GET_PCBYTESD(adrs);
			return(adrs + I286_BP);

		case EA_BP_DISP16:
			GET_PCWORD(adrs);
			return(adrs + I286_BP);

		case EA_DISP16:
			GET_PCWORD(adrs);
			return(adrs);

		default:
			return(0);
	}
}

UINT calc_a(UINT op, UINT32 *seg) {

	UINT	adrs;

	switch(((op >> 3) & 0x18) + (op & 0x07)) {
		case EA_BX_SI:
			*seg = DS_FIX;
			return(LOW16(I286_BX + I286_SI));

		case EA_BX_SI_DISP8:
			*seg = DS_FIX;
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BX + I286_SI));

		case EA_BX_SI_DISP16:
			*seg = DS_FIX;
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BX + I286_SI));

		case EA_BX_DI:
			*seg = DS_FIX;
			return(LOW16(I286_BX + I286_DI));

		case EA_BX_DI_DISP8:
			*seg = DS_FIX;
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BX + I286_DI));

		case EA_BX_DI_DISP16:
			*seg = DS_FIX;
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BX + I286_DI));

		case EA_BP_SI:
			*seg = SS_FIX;
			return(LOW16(I286_BP + I286_SI));

		case EA_BP_SI_DISP8:
			*seg = SS_FIX;
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BP + I286_SI));

		case EA_BP_SI_DISP16:
			*seg = SS_FIX;
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BP + I286_SI));

		case EA_BP_DI:
			*seg = SS_FIX;
			return(LOW16(I286_BP + I286_DI));

		case EA_BP_DI_DISP8:
			*seg = SS_FIX;
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BP + I286_DI));

		case EA_BP_DI_DISP16:
			*seg = SS_FIX;
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BP + I286_DI));

		case EA_SI:
			*seg = DS_FIX;
			return(I286_SI);

		case EA_SI_DISP8:
			*seg = DS_FIX;
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_SI));

		case EA_SI_DISP16:
			*seg = DS_FIX;
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_SI));

		case EA_DI:
			*seg = DS_FIX;
			return(I286_DI);

		case EA_DI_DISP8:
			*seg = DS_FIX;
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_DI));

		case EA_DI_DISP16:
			*seg = DS_FIX;
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_DI));

		case EA_BX:
			*seg = DS_FIX;
			return(I286_BX);

		case EA_BX_DISP8:
			*seg = DS_FIX;
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BX));

		case EA_BX_DISP16:
			*seg = DS_FIX;
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BX));

		case EA_BP_DISP8:
			*seg = SS_FIX;
			GET_PCBYTESD(adrs);
			return(LOW16(adrs + I286_BP));

		case EA_BP_DISP16:
			*seg = SS_FIX;
			GET_PCWORD(adrs);
			return(LOW16(adrs + I286_BP));

		case EA_DISP16:
			*seg = DS_FIX;
			GET_PCWORD(adrs);
			return(adrs);

		default:
			*seg = 0;
			return(0);
	}
}

#endif


UINT32 i286c_selector(UINT sel) {

	I286DTR	*dtr;
	UINT32	addr;
	UINT32	ret;

	dtr = (sel & 4)?&I286_LDTRC:&I286_GDTR;
	addr = (dtr->base24 << 16) + dtr->base + (sel & (~7));
	ret = i286_memoryread_w(addr+2);
	ret += i286_memoryread(addr+4) << 16;
	TRACEOUT(("ProtectMode: selector idx=%x %s rpl=%d - real addr = %.6x",
					(sel >> 3), (sel & 4)?"LDT":"GDT", sel & 3, ret));
	return(ret);
}

