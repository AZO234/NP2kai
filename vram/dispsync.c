#include	"compiler.h"
#include	"scrnmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"dispsync.h"


	DSYNC	dsync;


void dispsync_initialize(void) {

	ZeroMemory(&dsync, sizeof(dsync));
	dsync.textymax = 400;
	dsync.grphymax = 400;

	dsync.scrnxmax = 640;
	dsync.scrnxextend = 0;
	dsync.scrnymax = 400;

//	scrnmng_setwidth(0, 640);
//	scrnmng_setextend(0);
//	scrnmng_setheight(0, 400);
}

BOOL dispsync_renewalmode(void) {

	UINT	disp;

	if (!scrnmng_haveextend()) {
		return(FALSE);
	}
	disp = 0;
	if ((!(np2cfg.LCD_MODE & 1)) && ((gdc.display & 7) < 3)) {
		disp = 1;
	}
	if (dsync.scrnxextend != disp) {
		dsync.scrnxextend = disp;
		scrnmng_setextend(disp);
		return(TRUE);
	}
	return(FALSE);
}

BOOL dispsync_renewalhorizontal(void) {

	UINT	hbp;
	UINT	cr;
	UINT	scrnxpos;
	UINT	scrnxmax;

	hbp = gdc.m.para[GDC_SYNC + 4] & 0x1f;
	cr = gdc.m.para[GDC_SYNC + 1];

	scrnxpos = 0;
	if (hbp >= 7) {
		scrnxpos = hbp - 7;
	}
	scrnxmax = cr + 2;
	if ((scrnxpos + scrnxmax) > 80) {
		scrnxmax = min(scrnxmax, 80);
		scrnxpos = 80 - scrnxmax;
	}
	scrnxpos <<= 3;
	scrnxmax <<= 3;
	if ((dsync.scrnxpos != scrnxpos) || (dsync.scrnxmax != scrnxmax)) {
		dsync.scrnxpos = scrnxpos;
		dsync.scrnxmax = scrnxmax;
		scrnmng_setwidth(scrnxpos, scrnxmax);
		return(TRUE);
	}
	else {
		return(FALSE);
	}
}

BOOL dispsync_renewalvertical(void) {

	UINT	text_vbp;
	UINT	grph_vbp;
	UINT	textymax;
	UINT	grphymax;
	UINT	scrnymax;

	text_vbp = gdc.m.para[GDC_SYNC + 7] >> 2;
	grph_vbp = gdc.s.para[GDC_SYNC + 7] >> 2;
	if (text_vbp >= grph_vbp) {
		text_vbp -= grph_vbp;
		grph_vbp = 0;
	}
	else {
		grph_vbp -= text_vbp;
		text_vbp = 0;
	}

	textymax = LOADINTELWORD(gdc.m.para + GDC_SYNC + 6);
	textymax = ((textymax - 1) & 0x3ff) + 1;
	textymax += text_vbp;

	grphymax = LOADINTELWORD(gdc.s.para + GDC_SYNC + 6);
	grphymax = ((grphymax - 1) & 0x3ff) + 1;
	grphymax += grph_vbp;

#if defined(SUPPORT_CRT15KHZ)
	if (gdc.crt15khz & 2) {
		textymax *= 2;
		grphymax *= 2;
	}
#endif
	if (textymax > SURFACE_HEIGHT) {
		textymax = SURFACE_HEIGHT;
	}
	if (grphymax > SURFACE_HEIGHT) {
		grphymax = SURFACE_HEIGHT;
	}
	if ((dsync.text_vbp == text_vbp) && (dsync.grph_vbp == grph_vbp) &&
		(dsync.textymax == textymax) && (dsync.grphymax == grphymax)) {
		return(FALSE);
	}
	dsync.text_vbp = text_vbp;
	dsync.grph_vbp = grph_vbp;
	dsync.textymax = textymax;
	dsync.grphymax = grphymax;

	scrnymax = max(grphymax, textymax);
	scrnymax = (scrnymax + 7) & (~7);
	if (dsync.scrnymax != scrnymax) {
		dsync.scrnymax = scrnymax;
		scrnmng_setheight(0, scrnymax);
	}

	dsync.textvad = text_vbp * 640;
	dsync.grphvad = grph_vbp * 640;
	if (text_vbp) {
		ZeroMemory(np2_tram, text_vbp * 640);
	}
	if (scrnymax - textymax) {
		ZeroMemory(np2_tram + textymax * 640, (scrnymax - textymax) * 640);
	}
	if (grph_vbp) {
		ZeroMemory(np2_vram[0], grph_vbp * 640);
		ZeroMemory(np2_vram[1], grph_vbp * 640);
	}
	if (scrnymax - grphymax) {
		ZeroMemory(np2_vram[0] + grphymax * 640, (scrnymax - grphymax) * 640);
		ZeroMemory(np2_vram[1] + grphymax * 640, (scrnymax - grphymax) * 640);
	}
	return(TRUE);
}

