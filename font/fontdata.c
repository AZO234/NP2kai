#include	"compiler.h"
#include	"cpucore.h"
#include	"font.h"
#include	"fontdata.h"
#include	"fontdata.res"


const OEMCHAR pc88ankname[]		= OEMTEXT("PC88.FNT");
const OEMCHAR pc88knj1name[]	= OEMTEXT("KANJI1.ROM");
const OEMCHAR pc88knj2name[]	= OEMTEXT("KANJI2.ROM");
const OEMCHAR pc98fontname[]	= OEMTEXT("FONT.BMP");
const OEMCHAR v98fontname[]		= OEMTEXT("FONT.ROM");
const OEMCHAR fm7ankname[]		= OEMTEXT("SUBSYS_C.ROM");
const OEMCHAR fm7knjname[]		= OEMTEXT("KANJI.ROM");
const OEMCHAR x1ank1name[]		= OEMTEXT("FNT0808.X1");
const OEMCHAR x1ank2name[]		= OEMTEXT("FNT0816.X1");
const OEMCHAR x1knjname[]		= OEMTEXT("FNT1616.X1");
const OEMCHAR x68kfontname[]	= OEMTEXT("CGROM.DAT");


static void patch29(UINT jish, const UINT8 *src) {

	UINT	i;
	UINT8	*p;

	p = fontrom + 0x21000 + (jish << 4);
	for (i=0x21; i<0x7f; i++) {
		CopyMemory(p, src, 16);
		p += 0x1000;
		src += 16;
	}
}

static void patch2c(void) {

	UINT	i;
	UINT	j;
const UINT8	*p;
	UINT8	*q;

	p = fontdata_2c;
	q = fontrom + 0x240c0;
	for (i=0x24; i<0x70; i++) {
		for (j=0; j<16; j++) {
			q[j + 0x800] = p[0];
			q[j + 0x000] = p[1];
			p += 2;
		}
		q += 0x1000;
	}
}


// ----

void fontdata_ank8store(const UINT8 *ptr, UINT pos, UINT cnt) {

	UINT8	*dat;

	dat = fontrom + 0x82000 + (pos * 16);
	while(cnt--) {
		CopyMemory(dat, ptr, 8);
		dat += 16;
		ptr += 8;
	}
}

void fontdata_patch16a(void) {

	CopyMemory(fontrom + 0x80000, fontdata_16 + 0*32*16, 32*16);
}

void fontdata_patch16b(void) {

	CopyMemory(fontrom + 0x80800, fontdata_16 + 1*32*16, 32*16);
	CopyMemory(fontrom + 0x80e00, fontdata_16 + 2*32*16, 32*16);
}

void fontdata_patchjis(void) {

	patch29(0x09, fontdata_29);
	patch29(0x0a, fontdata_2a);
	patch29(0x0b, fontdata_2b);
	patch2c();
}

