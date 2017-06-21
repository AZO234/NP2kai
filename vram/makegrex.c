#include	"compiler.h"

#if defined(SUPPORT_PC9821)

#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"vram.h"
#include	"scrndraw.h"
#include	"dispsync.h"
#include	"makegrex.h"


typedef struct {
	UINT32	*vm;
	UINT	liney;
	UINT	pitch;
} _MKGREX, *MKGREX;


static BOOL grphput_indirty0(MKGREX mkgrex, int gpos) {

	_MKGREX	mg;
	UINT	vad;
	UINT	remain;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrex;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW15(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	remain = LOW14(remain) >> 4;
	while(1) {
		vc = vad;
		p = mg.vm;
		pterm = p + (80 * 2);
		do {
			if (vramupdate[vc] & 1) {
				renewal_line[mg.liney] |= 1;
				p[0] = *(UINT32 *)(vramex + (vc * 8) + 0);
				p[1] = *(UINT32 *)(vramex + (vc * 8) + 4);
			}
			vc = LOW15(vc + 1);
			p += 2;
		} while(p < pterm);
		mg.liney++;
		if (mg.liney >= dsync.grphymax) {
			return(TRUE);
		}
		mg.vm += 80*2;
		remain--;
		if (!remain) {
			break;
		}
		vad = LOW15(vad + mg.pitch);
	}
	mkgrex->vm = mg.vm;
	mkgrex->liney = mg.liney;
	return(FALSE);
}

static BOOL grphput_all0(MKGREX mkgrex, int gpos) {

	_MKGREX	mg;
	UINT	vad;
	UINT	remain;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrex;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW15(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	remain = LOW14(remain) >> 4;
	while(1) {
		vc = vad;
		p = mg.vm;
		pterm = p + (80 * 2);
		do {
			p[0] = *(UINT32 *)(vramex + (vc * 8) + 0);
			p[1] = *(UINT32 *)(vramex + (vc * 8) + 4);
			vc = LOW15(vc + 1);
			p += 2;
		} while(p < pterm);
		renewal_line[mg.liney] |= 1;
		mg.liney++;
		if (mg.liney >= dsync.grphymax) {
			return(TRUE);
		}
		mg.vm += 80*2;
		remain--;
		if (!remain) {
			break;
		}
		vad = LOW15(vad + mg.pitch);
	}
	mkgrex->vm = mg.vm;
	mkgrex->liney = mg.liney;
	return(FALSE);
}

static BOOL grphput_indirty1(MKGREX mkgrex, int gpos) {

	_MKGREX	mg;
	UINT	vad;
	UINT	remain;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrex;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW15(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	remain = LOW14(remain) >> 4;
	while(1) {
		vc = vad;
		p = mg.vm;
		pterm = p + (80 * 2);
		do {
			if (vramupdate[vc] & 2) {
				renewal_line[mg.liney] |= 2;
				p[0] = *(UINT32 *)(vramex + 0x40000 + (vc * 8) + 0);
				p[1] = *(UINT32 *)(vramex + 0x40000 + (vc * 8) + 4);
			}
			vc = LOW15(vc + 1);
			p += 2;
		} while(p < pterm);
		mg.liney++;
		if (mg.liney >= dsync.grphymax) {
			return(TRUE);
		}
		mg.vm += 80*2;
		remain--;
		if (!remain) {
			break;
		}
		vad = LOW15(vad + mg.pitch);
	}
	mkgrex->vm = mg.vm;
	mkgrex->liney = mg.liney;
	return(FALSE);
}

static BOOL grphput_all1(MKGREX mkgrex, int gpos) {

	_MKGREX	mg;
	UINT	vad;
	UINT	remain;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrex;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW15(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	remain = LOW14(remain) >> 4;
	while(1) {
		vc = vad;
		p = mg.vm;
		pterm = p + (80 * 2);
		do {
			p[0] = *(UINT32 *)(vramex + 0x40000 + (vc * 8) + 0);
			p[1] = *(UINT32 *)(vramex + 0x40000 + (vc * 8) + 4);
			vc = LOW15(vc + 1);
			p += 2;
		} while(p < pterm);
		renewal_line[mg.liney] |= 2;
		mg.liney++;
		if (mg.liney >= dsync.grphymax) {
			return(TRUE);
		}
		mg.vm += 80*2;
		remain--;
		if (!remain) {
			break;
		}
		vad = LOW15(vad + mg.pitch);
	}
	mkgrex->vm = mg.vm;
	mkgrex->liney = mg.liney;
	return(FALSE);
}


// ---- all

static BOOL grphput_indirty(MKGREX mkgrex, int gpos) {

	_MKGREX	mg;
	UINT	vad;
	UINT	remain;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrex;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW16(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	remain = LOW15(remain) >> 4;
	while(1) {
		vc = vad;
		p = mg.vm;
		pterm = p + (80 * 2);
		do {
			if (vramupdate[LOW15(vc)] & 3) {
				renewal_line[mg.liney] |= 3;
				p[0] = *(UINT32 *)(vramex + (vc * 8) + 0);
				p[1] = *(UINT32 *)(vramex + (vc * 8) + 4);
			}
			vc = LOW16(vc + 1);
			p += 2;
		} while(p < pterm);
		mg.liney++;
		if (mg.liney >= dsync.grphymax) {
			return(TRUE);
		}
		mg.vm += 80*2;
		remain--;
		if (!remain) {
			break;
		}
		vad = LOW16(vad + mg.pitch);
	}
	mkgrex->vm = mg.vm;
	mkgrex->liney = mg.liney;
	return(FALSE);
}

static BOOL grphput_all(MKGREX mkgrex, int gpos) {

	_MKGREX	mg;
	UINT	vad;
	UINT	remain;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrex;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW16(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	remain = LOW15(remain) >> 4;
	while(1) {
		vc = vad;
		p = mg.vm;
		pterm = p + (80 * 2);
		do {
			p[0] = *(UINT32 *)(vramex + (vc * 8) + 0);
			p[1] = *(UINT32 *)(vramex + (vc * 8) + 4);
			vc = LOW16(vc + 1);
			p += 2;
		} while(p < pterm);
		renewal_line[mg.liney] |= 3;
		mg.liney++;
		if (mg.liney >= dsync.grphymax) {
			return(TRUE);
		}
		mg.vm += 80*2;
		remain--;
		if (!remain) {
			break;
		}
		vad = LOW16(vad + mg.pitch);
	}
	mkgrex->vm = mg.vm;
	mkgrex->liney = mg.liney;
	return(FALSE);
}


// ----

void VRAMCALL makegrphex(int page, int alldraw) {

	_MKGREX	mg;
	int		i;
	BOOL	(*grphput)(MKGREX mkgrex, int gpos);
	UINT32	mask;

	mg.pitch = gdc.s.para[GDC_PITCH];
	if (!(gdc.clock & 0x80)) {
		mg.pitch <<= 1;
	}
	mg.pitch &= 0xfe;
	mg.liney = dsync.grph_vbp;

	if (gdc.analog & 4) {
		mg.vm = (UINT32 *)(np2_vram[0] + dsync.grphvad);
		grphput = (alldraw)?grphput_all:grphput_indirty;
		mask = ~0x03030303;
	}
	else if (!page) {
		mg.vm = (UINT32 *)(np2_vram[0] + dsync.grphvad);
		grphput = (alldraw)?grphput_all0:grphput_indirty0;
		mask = ~0x01010101;
	}
	else {
		mg.vm = (UINT32 *)(np2_vram[1] + dsync.grphvad);
		grphput = (alldraw)?grphput_all1:grphput_indirty1;
		mask = ~0x02020202;
	}
	while(1) {
		if ((*grphput)(&mg, 0)) {
			break;
		}
		if ((*grphput)(&mg, 4)) {
			break;
		}
	}
	for (i=0; i<0x8000; i+=4) {
		*(UINT32 *)(vramupdate + i) &= mask;
	}
}
#endif

