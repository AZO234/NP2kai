#include	"compiler.h"
#include	"scrnmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"palettes.h"

		RGB32		np2_pal32[NP2PAL_MAX];
#if defined(SUPPORT_16BPP)
		RGB16		np2_pal16[NP2PAL_MAX];
#endif

		PALEVENT	palevent;
static	RGB32		degpal1[8];
static	RGB32		degpal2[8];
static	UINT8		anapal1[16];
static	UINT8		anapal2[16];

static	RGB32		lcdpal[15];
static	UINT8		lcdtbl[0x1000];
		UINT8		pal_monotable[16] = {0, 0, 0, 0, 1, 1, 1, 1,
											0, 0, 0, 0, 1, 1, 1, 1};

static const UINT8 lcdpal_a[27] = {	0, 1, 2, 3, 5, 2, 4, 4, 6,
									7, 9, 2,11,13, 2, 4, 4, 6,
									8, 8,10, 8, 8,10,12,12,14};
static const UINT8 deftbl[4] = {0x04, 0x15, 0x26, 0x37};


void pal_makegrad(RGB32 *pal, int pals, UINT32 bg, UINT32 fg) {

	int		i;

	if (pals >= 2) {
		pals--;
		for (i=0; i<=pals; i++) {
			pal[i].p.b = (UINT8)
				((((fg >> 0) & 0x0000ff) * i + 
				((bg >> 0) & 0x0000ff) * (pals-i)) / pals);
			pal[i].p.g = (UINT8)
				((((fg >> 8) & 0x0000ff) * i + 
				((bg >> 8) & 0x0000ff) * (pals-i)) / pals);
			pal[i].p.r = (UINT8)
				((((fg >> 16) & 0x0000ff) * i + 
				((bg >> 16) & 0x0000ff) * (pals-i)) / pals);
			pal[i].p.e = 0;
		}
	}
}


// ----

void pal_initlcdtable(void) {

	UINT	i;
	int		j;

	for (i=0; i<0x1000; i++) {
		j = 0;
		if ((i & 0x00f) >= 0x004) {					// b
			j++;
			if ((i & 0x00f) >= 0x00b) {
				j++;
			}
		}
		if ((i & 0x0f0) >= 0x040) {					// r
			j += 3;
			if ((i & 0x0f0) >= 0x0b0) {
				j += 3;
			}
		}
		if ((i & 0xf00) >= 0x400) {					// g
			j += 9;
			if ((i & 0xf00) >= 0xb00) {
				j += 9;
			}
		}
		lcdtbl[i] = lcdpal_a[j];
	}
}

void pal_makelcdpal(void) {

	if (!(np2cfg.LCD_MODE & 2)) {
		pal_makegrad(lcdpal, 15, np2cfg.BG_COLOR, np2cfg.FG_COLOR);
	}
	else {
		pal_makegrad(lcdpal, 15, np2cfg.FG_COLOR, np2cfg.BG_COLOR);
	}
}

void pal_makeskiptable(void) {

	int		i;
	RGB32	pal;
	UINT8	ana;

	for (i=0; i<8; i++) {
		pal.p.b = (UINT8)(i & 1);
		pal.p.r = (UINT8)((i >> 1) & 1);
		pal.p.g = (UINT8)((i >> 2) & 1);
		pal.p.e = 0;
		degpal1[i].d = pal.d * 255;
		degpal2[i].d = pal.d * np2cfg.skiplight;
	}
	for (i=0; i<16; i++) {
		ana = (UINT8)(i * 0x11);
		anapal1[i] = ana;
		anapal2[i] = (UINT8)((np2cfg.skiplight * anapal1[i]) / 255);
	}
}


// ---------------------------------------------------------------------------

void pal_makeanalog(RGB32 *pal, UINT16 bit) {

	UINT	i;

	for (i=0; i<NP2PALS_GRPH; i++, pal++) {
		if (bit & (1 << i)) {
			np2_pal32[i+NP2PAL_GRPH].p.b = anapal1[pal->p.b & 15];
			np2_pal32[i+NP2PAL_GRPH].p.g = anapal1[pal->p.g & 15];
			np2_pal32[i+NP2PAL_GRPH].p.r = anapal1[pal->p.r & 15];
			if (np2cfg.skipline) {
				np2_pal32[i+NP2PAL_SKIP].p.b = anapal2[pal->p.b & 15];
				np2_pal32[i+NP2PAL_SKIP].p.g = anapal2[pal->p.g & 15];
				np2_pal32[i+NP2PAL_SKIP].p.r = anapal2[pal->p.r & 15];
			}
		}
	}
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		for (i=0; i<NP2PALS_GRPH; i++) {
			if (bit & (1 << i)) {
				np2_pal16[i+NP2PAL_GRPH] =
								scrnmng_makepal16(np2_pal32[i+NP2PAL_GRPH]);
				np2_pal16[i+NP2PAL_SKIP] =
								scrnmng_makepal16(np2_pal32[i+NP2PAL_SKIP]);
			}
		}
	}
#endif
}

static void pal_makedegital(const UINT8 *paltbl) {

	UINT	i;

	for (i=0; i<4; i++) {
		np2_pal32[i+NP2PAL_GRPH+ 0].d =
		np2_pal32[i+NP2PAL_GRPH+ 8].d =
									degpal1[(paltbl[i] >> 4) & 7].d;
		np2_pal32[i+NP2PAL_GRPH+ 4].d =
		np2_pal32[i+NP2PAL_GRPH+12].d =
									degpal1[paltbl[i] & 7].d;
		if (np2cfg.skipline) {
			np2_pal32[i+NP2PAL_SKIP+ 0].d =
			np2_pal32[i+NP2PAL_SKIP+ 8].d =
									degpal2[(paltbl[i] >> 4) & 7].d;
			np2_pal32[i+NP2PAL_SKIP+ 4].d =
			np2_pal32[i+NP2PAL_SKIP+12].d =
									degpal2[paltbl[i] & 7].d;
		}
	}
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		for (i=0; i<4; i++) {
			np2_pal16[i+NP2PAL_GRPH+ 0] =
			np2_pal16[i+NP2PAL_GRPH+ 8] =
								scrnmng_makepal16(np2_pal32[i+NP2PAL_GRPH+0]);
			np2_pal16[i+NP2PAL_GRPH+ 4] =
			np2_pal16[i+NP2PAL_GRPH+12] =
								scrnmng_makepal16(np2_pal32[i+NP2PAL_GRPH+4]);
		}
		if (np2cfg.skipline) {
			for (i=0; i<4; i++) {
				np2_pal16[i+NP2PAL_SKIP+ 0] =
				np2_pal16[i+NP2PAL_SKIP+ 8] =
								scrnmng_makepal16(np2_pal32[i+NP2PAL_SKIP+0]);
				np2_pal16[i+NP2PAL_SKIP+ 4] =
				np2_pal16[i+NP2PAL_SKIP+12] =
								scrnmng_makepal16(np2_pal32[i+NP2PAL_SKIP+4]);
			}
		}
	}
#endif
}

void pal_makeanalog_lcd(RGB32 *pal, UINT16 bit) {

	UINT	i;
	UINT	j;

	for (i=0; i<NP2PALS_GRPH; i++, pal++) {
		if (bit & (1 << i)) {
			j = (pal->p.b & 15);
			j |= (pal->p.r & 15) << 4;
			j |= (pal->p.g & 15) << 8;
			np2_pal32[i+NP2PAL_SKIP].d =
			np2_pal32[i+NP2PAL_GRPH].d = lcdpal[lcdtbl[j]].d;
		}
	}
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		for (i=0; i<NP2PALS_GRPH; i++) {
			if (bit & (1 << i)) {
				np2_pal16[i+NP2PAL_GRPH] =
				np2_pal16[i+NP2PAL_SKIP] =
								scrnmng_makepal16(np2_pal32[i+NP2PAL_GRPH]);
			}
		}
	}
#endif
}

static void pal_makedegital_lcd(const UINT8 *paltbl) {

	UINT	i;
	UINT32	pal32;
#if defined(SUPPORT_16BPP)
	RGB16	pal16;
#endif

	for (i=0; i<4; i++) {
		pal32 = lcdpal[(paltbl[i] >> 3) & 14].d;
		np2_pal32[i+NP2PAL_GRPH+ 0].d = pal32;
		np2_pal32[i+NP2PAL_GRPH+ 8].d = pal32;
		pal32 = lcdpal[(paltbl[i] << 1) & 14].d;
		np2_pal32[i+NP2PAL_GRPH+ 4].d = pal32;
		np2_pal32[i+NP2PAL_GRPH+12].d = pal32;
		if (np2cfg.skipline) {
			pal32 = np2_pal32[i+NP2PAL_GRPH+ 0].d;
			np2_pal32[i+NP2PAL_SKIP+ 0].d = pal32;
			np2_pal32[i+NP2PAL_SKIP+ 8].d = pal32;
			pal32 = np2_pal32[i+NP2PAL_GRPH+ 4].d;
			np2_pal32[i+NP2PAL_SKIP+ 4].d = pal32;
			np2_pal32[i+NP2PAL_SKIP+12].d = pal32;
		}
	}
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		for (i=0; i<4; i++) {
			pal16 = scrnmng_makepal16(np2_pal32[i+NP2PAL_GRPH+0]);
			np2_pal16[i+NP2PAL_GRPH+ 0] = pal16;
			np2_pal16[i+NP2PAL_GRPH+ 8] = pal16;
			pal16 = scrnmng_makepal16(np2_pal32[i+NP2PAL_GRPH+4]);
			np2_pal16[i+NP2PAL_GRPH+ 4] = pal16;
			np2_pal16[i+NP2PAL_GRPH+12] = pal16;
		}
		if (np2cfg.skipline) {
			for (i=0; i<4; i++) {
				pal16 = np2_pal16[i+NP2PAL_GRPH+ 0];
				np2_pal16[i+NP2PAL_SKIP+ 0] = pal16;
				np2_pal16[i+NP2PAL_SKIP+ 8] = pal16;
				pal16 = np2_pal16[i+NP2PAL_GRPH+ 4];
				np2_pal16[i+NP2PAL_SKIP+ 4] = pal16;
				np2_pal16[i+NP2PAL_SKIP+12] = pal16;
			}
		}
	}
#endif
}

static void pal_maketext(void) {

	UINT	i;
	UINT	j;
	UINT	k;
#if defined(SUPPORT_16BPP)
	RGB16	pal16;
#endif

	k = NP2PAL_TEXT2;
	for (i=0; i<8; i++) {
		np2_pal32[i+1+NP2PAL_TEXT].d = degpal1[i].d;
		np2_pal32[i+1+NP2PAL_TEXT3].d = degpal1[i].d;
		for (j=0; j<NP2PALS_GRPH; j++, k++) {
			np2_pal32[k].d = degpal1[i].d;
		}
	}
	np2_pal32[NP2PAL_TEXT3] = np2_pal32[NP2PAL_TEXT3 + 1];
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		k = NP2PAL_TEXT2;
		for (i=0; i<8; i++) {
			pal16 = scrnmng_makepal16(degpal1[i]);
			np2_pal16[i+1+NP2PAL_TEXT] = pal16;
			np2_pal16[i+1+NP2PAL_TEXT3] = pal16;
			for (j=0; j<NP2PALS_GRPH; j++, k++) {
				np2_pal16[k] = pal16;
			}
		}
		np2_pal16[NP2PAL_TEXT3] = np2_pal16[NP2PAL_TEXT3 + 1];
	}
#endif
}

static void pal_maketext_lcd(void) {

	UINT	i;
	UINT	j;
	UINT	k;
#if defined(SUPPORT_16BPP)
	RGB16	pal16;
#endif

	k = NP2PAL_TEXT2;
	for (i=0; i<8; i++) {
		np2_pal32[i+1+NP2PAL_TEXT].d = lcdpal[i*2].d;
		np2_pal32[i+1+NP2PAL_TEXT3].d = lcdpal[i*2].d;
		for (j=0; j<NP2PALS_GRPH; j++, k++) {
			np2_pal32[k].d = lcdpal[i*2].d;
		}
	}
	np2_pal32[NP2PAL_TEXT3] = np2_pal32[NP2PAL_TEXT3 + 1];
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		k = NP2PAL_TEXT2;
		for (i=0; i<8; i++) {
			pal16 = scrnmng_makepal16(lcdpal[i*2]);
			np2_pal16[i+1+NP2PAL_TEXT] = pal16;
			np2_pal16[i+1+NP2PAL_TEXT3] = pal16;
			for (j=0; j<NP2PALS_GRPH; j++, k++) {
				np2_pal16[k] = pal16;
			}
		}
		np2_pal16[NP2PAL_TEXT3] = np2_pal16[NP2PAL_TEXT3 + 1];
	}
#endif
}

#if defined(SUPPORT_PC9821)
static void pal_maketext256(void) {

	UINT	i;
#if defined(SUPPORT_16BPP)
	RGB16	pal16;
#endif

	for (i=0; i<8; i++) {
		np2_pal32[i+1+NP2PAL_TEXTEX].d = degpal1[i].d;
		np2_pal32[i+1+NP2PAL_TEXTEX3].d = degpal1[i].d;
	}
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		for (i=0; i<8; i++) {
			pal16 = scrnmng_makepal16(degpal1[i]);
			np2_pal16[i+1+NP2PAL_TEXTEX] = pal16;
			np2_pal16[i+1+NP2PAL_TEXTEX3] = pal16;
		}
	}
#endif
}
#endif

static void pal_makeingmono(void) {							// ver0.28/pr4

	int		i;

	if (gdc.analog) {
		for (i=0; i<16; i++) {
			pal_monotable[i] = gdc.anapal[i].p.g & 8;
		}
	}
	else {
		for (i=0; i<4; i++) {
			pal_monotable[i+0] = gdc.degpal[i] & 0x40;
			pal_monotable[i+8] = gdc.degpal[i] & 0x40;
			pal_monotable[i+4] = gdc.degpal[i] & 0x4;
			pal_monotable[i+12] = gdc.degpal[i] & 0x4;
		}
	}
}


#if defined(SUPPORT_PC9821)
static void pal_make9821(const UINT8 *pal) {

	int		i;
	RGB32	*p;

	p = np2_pal32 + NP2PAL_GRPHEX;
	for (i=0; i<256; i++) {
		p[i].p.g = pal[i*4+0];
		p[i].p.r = pal[i*4+1];
		p[i].p.b = pal[i*4+2];
	}
	np2_pal32[NP2PAL_TEXTEX3].d = np2_pal32[NP2PAL_GRPHEX].d;
#if defined(SUPPORT_16BPP)
	if (scrnmng_getbpp() == 16) {
		for (i=0; i<256; i++) {
			np2_pal16[i + NP2PAL_GRPHEX] =
							scrnmng_makepal16(np2_pal32[i + NP2PAL_GRPHEX]);
		}
		np2_pal16[NP2PAL_TEXTEX3] = np2_pal16[NP2PAL_GRPHEX];
	}
#endif
}
#endif

void pal_change(UINT8 textpalset) {

	if (textpalset) {
		if (!(np2cfg.LCD_MODE & 1)) {
			pal_maketext();
		}
		else {
			pal_maketext_lcd();
		}
		np2_pal32[NP2PAL_TEXT].d = np2_pal32[NP2PAL_TEXT2].d;
#if defined(SUPPORT_16BPP)
		np2_pal16[NP2PAL_TEXT] = np2_pal16[NP2PAL_TEXT2];
#endif
#if defined(SUPPORT_PC9821)
		pal_maketext256();
#endif
	}
#if defined(SUPPORT_PC9821)
	if (gdc.analog & 2) {
		pal_make9821(gdc.anareg + (16 * 3));
		scrndraw_changepalette();
		return;
	}
#endif
	if (!(np2cfg.LCD_MODE & 1)) {
		if (gdc.mode1 & 2) {
			pal_makedegital(deftbl);
			pal_makeingmono();
		}
		else {
			if (gdc.analog) {
				pal_makeanalog(gdc.anapal, 0xffff);
			}
			else {
				pal_makedegital(gdc.degpal);
			}
		}
	}
	else {
		if (gdc.mode1 & 2) {
			pal_makedegital_lcd(deftbl);
			pal_makeingmono();
		}
		else {
			if (gdc.analog) {
				pal_makeanalog_lcd(gdc.anapal, 0xffff);
			}
			else {
				pal_makedegital_lcd(gdc.degpal);
			}
		}
	}
	if (np2cfg.skipline) {
		np2_pal32[NP2PAL_TEXT].d = np2_pal32[NP2PAL_SKIP].d;
#if defined(SUPPORT_16BPP)
		np2_pal16[NP2PAL_TEXT] = np2_pal16[NP2PAL_SKIP];
#endif
	}
	scrndraw_changepalette();
}

void pal_eventclear(void) {

	palevent.anabit = 0;
	palevent.events = 0;
	if ((!pcstat.drawframe) || (!np2cfg.RASTER) || (scrnmng_getbpp() == 8)) {
		palevent.events--;					// 0xffffffff ‚É‚·‚é...
	}
	else {
		CopyMemory(palevent.pal, gdc.anapal, sizeof(gdc.anapal));
		palevent.vsyncpal = 0;
	}
}

