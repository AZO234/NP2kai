#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"vram.h"
#include	"scrndraw.h"
#include	"dispsync.h"
#include	"makegrph.h"
#include	"makegrph.mcr"


typedef struct {
	UINT32	*vm;
	UINT	liney;
//	UINT	pitch;
	UINT	lr;
} _MKGRPH, *MKGRPH;


#if (!defined(MEMOPTIMIZE)) || (MEMOPTIMIZE == 0)

	UINT32	grph_table[4*256*2];

void makegrph_initialize(void) {

	int		i, j;
	UINT8	bit;
	UINT32	tmp;

	tmp = 0;
	for (i=0; i<256; i++) {
#if defined(BYTESEX_LITTLE)
		for (bit=1; bit<0x10; bit<<=1)
#else
		for (bit=8; bit; bit>>=1)
#endif
		{
			tmp <<= 8;
			if (i & bit) {
				tmp++;
			}
		}
		for (j=0; j<4; j++, tmp<<=1) {
			grph_table[j*512+i*2+1] = tmp;
		}
#if defined(BYTESEX_LITTLE)
		for (; bit; bit<<=1)
#else
		for (bit=0x80; bit & 0xf0; bit>>=1)
#endif
		{
			tmp <<= 8;
			if (i & bit) {
				tmp++;
			}
		}
		for (j=0; j<4; j++, tmp<<=1) {
			grph_table[j*512+i*2+0] = tmp;
		}
	}
}

#elif (MEMOPTIMIZE == 1)

	UINT32	grph_table1[256*2];

void makegrph_initialize(void) {

	int		i;
	UINT8	bit;
	UINT32	tmp;

	tmp = 0;
	for (i=0; i<256; i++) {
#if defined(BYTESEX_LITTLE)
		for (bit=1; bit<0x10; bit<<=1)
#else
		for (bit=8; bit; bit>>=1)
#endif
		{
			tmp <<= 8;
			if (i & bit) {
				tmp++;
			}
		}
		grph_table1[i*2+1] = tmp;
#if defined(BYTESEX_LITTLE)
		for (; bit; bit<<=1)
#else
		for (bit=0x80; bit & 0xf0; bit>>=1)
#endif
		{
			tmp <<= 8;
			if (i & bit) {
				tmp++;
			}
		}
		grph_table1[i*2+0] = tmp;
	}
}

#else

	UINT32	grph_table0[16];

void makegrph_initialize(void) {

	int		i;
	UINT8	bit;
	UINT32	tmp;

	tmp = 0;
	for (i=0; i<16; i++) {
#if defined(BYTESEX_LITTLE)
		for (bit=1; bit<0x10; bit<<=1)
#else
		for (bit=8; bit; bit>>=1)
#endif
		{
			tmp <<= 8;
			if (i & bit) {
				tmp++;
			}
		}
		grph_table0[i] = tmp;
	}
}
#endif


static BOOL grphput_indirty0(MKGRPH mkgrph, int gpos) {

	_MKGRPH	mg;
	UINT	vad;
	UINT	remain;
	UINT	pitch;
	UINT	mul;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrph;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW15(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	pitch = gdc.s.para[GDC_PITCH];
	if (!(remain & (1 << 14))) {
		pitch <<= 1;
	}
	pitch &= 0xfe;
	remain = LOW14(remain) >> 4;
	mul = mg.lr;
	while(1) {
		if ((!(mg.liney & 1)) || (!(gdc.mode1 & 0x10))) {
			vc = vad;
			p = mg.vm;
			pterm = p + (80 * 2);
			do {
				if (vramupdate[vc] & 1) {
					renewal_line[mg.liney] |= 1;
					GRPHDATASET(p, vc);
				}
				vc = LOW15(vc + 1);
				p += 2;
			} while(p < pterm);
		}
		mg.liney++;
		if (mg.liney >= dsync.grphymax) {
			return(TRUE);
		}
		mg.vm += 80*2;
		remain--;
		if (!remain) {
			break;
		}
		mul--;
		if (!mul) {
			mul = mg.lr;
			vad = LOW15(vad + pitch);
		}
	}
	mkgrph->vm = mg.vm;
	mkgrph->liney = mg.liney;
	return(FALSE);
}

static BOOL grphput_indirty1(MKGRPH mkgrph, int gpos) {

	_MKGRPH	mg;
	UINT	vad;
	UINT	remain;
	UINT	pitch;
	UINT	mul;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrph;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW15(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	pitch = gdc.s.para[GDC_PITCH];
	if (!(remain & (1 << 14))) {
		pitch <<= 1;
	}
	pitch &= 0xfe;
	remain = LOW14(remain) >> 4;
	mul = mg.lr;
	while(1) {
		if ((!(mg.liney & 1)) || (!(gdc.mode1 & 0x10))) {
			vc = vad;
			p = mg.vm;
			pterm = p + (80 * 2);
			do {
				if (vramupdate[vc] & 2) {
					renewal_line[mg.liney] |= 2;
					GRPHDATASET(p, vc + VRAM_STEP);
				}
				vc = LOW15(vc + 1);
				p += 2;
			} while(p < pterm);
		}
		mg.liney++;
		if (mg.liney >= dsync.grphymax) {
			return(TRUE);
		}
		mg.vm += 80*2;
		remain--;
		if (!remain) {
			break;
		}
		mul--;
		if (!mul) {
			mul = mg.lr;
			vad = LOW15(vad + pitch);
		}
	}
	mkgrph->vm = mg.vm;
	mkgrph->liney = mg.liney;
	return(FALSE);
}

static BOOL grphput_all0(MKGRPH mkgrph, int gpos) {

	_MKGRPH	mg;
	UINT	vad;
	UINT	remain;
	UINT	pitch;
	UINT	mul;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrph;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW15(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	pitch = gdc.s.para[GDC_PITCH];
	if (!(remain & (1 << 14))) {
		pitch <<= 1;
	}
	pitch &= 0xfe;
	remain = LOW14(remain) >> 4;
	mul = mg.lr;
	while(1) {
		if ((!(mg.liney & 1)) || (!(gdc.mode1 & 0x10))) {
			vc = vad;
			p = mg.vm;
			pterm = p + (80 * 2);
			do {
				GRPHDATASET(p, vc);
				vc = LOW15(vc + 1);
				p += 2;
			} while(p < pterm);
		}
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
		mul--;
		if (!mul) {
			mul = mg.lr;
			vad = LOW15(vad + pitch);
		}
	}
	mkgrph->vm = mg.vm;
	mkgrph->liney = mg.liney;
	return(FALSE);
}

static BOOL grphput_all1(MKGRPH mkgrph, int gpos) {

	_MKGRPH	mg;
	UINT	vad;
	UINT	remain;
	UINT	pitch;
	UINT	mul;
	UINT	vc;
	UINT32	*p;
	UINT32	*pterm;

	mg = *mkgrph;
	vad = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 0);
	vad = LOW15(vad << 1);
	remain = LOADINTELWORD(gdc.s.para + GDC_SCROLL + gpos + 2);
	pitch = gdc.s.para[GDC_PITCH];
	if (!(remain & (1 << 14))) {
		pitch <<= 1;
	}
	pitch &= 0xfe;
	remain = LOW14(remain) >> 4;
	mul = mg.lr;
	while(1) {
		if ((!(mg.liney & 1)) || (!(gdc.mode1 & 0x10))) {
			vc = vad;
			p = mg.vm;
			pterm = p + (80 * 2);
			do {
				GRPHDATASET(p, vc + VRAM_STEP);
				vc = LOW15(vc + 1);
				p += 2;
			} while(p < pterm);
		}
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
		mul--;
		if (!mul) {
			mul = mg.lr;
			vad = LOW15(vad + pitch);
		}
	}
	mkgrph->vm = mg.vm;
	mkgrph->liney = mg.liney;
	return(FALSE);
}

void VRAMCALL makegrph(int page, int alldraw) {

	_MKGRPH	mg;
	int		i;

//	mg.pitch = gdc.s.para[GDC_PITCH];
//	if (!(gdc.clock & 0x80)) {
//		mg.pitch <<= 1;
//	}
//	mg.pitch &= 0xfe;
	mg.lr = (gdc.s.para[GDC_CSRFORM] & 0x1f) + 1;
	mg.liney = dsync.grph_vbp;

	if (!page) {
		mg.vm = (UINT32 *)(np2_vram[0] + dsync.grphvad);
		if (!alldraw) {
			while(1) {
				if (grphput_indirty0(&mg, 0)) {
					break;
				}
				if (grphput_indirty0(&mg, 4)) {
					break;
				}
				if (np2cfg.uPD72020) {
					continue;
				}
				if (grphput_indirty0(&mg, 8)) {
					break;
				}
				if (grphput_indirty0(&mg, 12)) {
					break;
				}
			}
		}
		else {
			while(1) {
				if (grphput_all0(&mg, 0)) {
					break;
				}
				if (grphput_all0(&mg, 4)) {
					break;
				}
				if (np2cfg.uPD72020) {
					continue;
				}
				if (grphput_all0(&mg, 8)) {
					break;
				}
				if (grphput_all0(&mg, 12)) {
					break;
				}
			}
		}
		for (i=0; i<0x8000; i+=4) {
			*(UINT32 *)(vramupdate + i) &= ~0x01010101;
		}
	}
	else {
		mg.vm = (UINT32 *)(np2_vram[1] + dsync.grphvad);
		if (!alldraw) {
			while(1) {
				if (grphput_indirty1(&mg, 0)) {
					break;
				}
				if (grphput_indirty1(&mg, 4)) {
					break;
				}
				if (np2cfg.uPD72020) {
					continue;
				}
				if (grphput_indirty1(&mg, 8)) {
					break;
				}
				if (grphput_indirty1(&mg, 12)) {
					break;
				}
			}
		}
		else {
			while(1) {
				if (grphput_all1(&mg, 0)) {
					break;
				}
				if (grphput_all1(&mg, 4)) {
					break;
				}
				if (np2cfg.uPD72020) {
					continue;
				}
				if (grphput_all1(&mg, 8)) {
					break;
				}
				if (grphput_all1(&mg, 12)) {
					break;
				}
			}
		}
		for (i=0; i<0x8000; i+=4) {
			*(UINT32 *)(vramupdate + i) &= ~0x02020202;
		}
	}
}

