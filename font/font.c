/**
 * @file	font.c
 * @brief	CGROM and font loader
 *
 * @author	$Author: yui $
 * @date	$Date: 2011/02/23 10:11:44 $
 */

#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"font.h"
#include	"fontdata.h"
#include	"fontmake.h"

#ifndef FONTMEMORYBIND
	UINT8	__font[0x84000];
#endif

static const OEMCHAR fonttmpname[] = OEMTEXT("font.tmp");

/**
 * Initializes CGROM
 */
void font_initialize(void) {

	ZeroMemory(fontrom, 0x84000);
	font_setchargraph(FALSE);
}

/**
 * Builds charactor graphics
 * @param[in] epson If this parameter is FALSE, patched NEC charactor
 */
void font_setchargraph(BOOL epson) {

	UINT8	*p;
	UINT8	*q;
	UINT	i;
	UINT	j;
	UINT32	dbit;

	p = fontrom + 0x81000;
	q = fontrom + 0x82000;
	for (i=0; i<256; i++) {
		q += 8;
		for (j=0; j<4; j++) {
			dbit = 0;
			if (i & (0x01 << j)) {
				dbit |= 0xf0f0f0f0;
			}
			if (i & (0x10 << j)) {
				dbit |= 0x0f0f0f0f;
			}
			*(UINT32 *)p = dbit;
			p += 4;
			*(UINT16 *)q = (UINT16)dbit;
			q += 2;
		}
	}

	if (!epson) {
		*(UINT16 *)(fontrom + 0x81000 + (0xf2 * 16)) = 0;
		fontrom[0x82000 + (0xf2 * 8)] = 0;
	}
}

/**
 * Retrieves the font type of the specified file.
 * @param[in] fname The name of the font file
 * @return font type
 */
static UINT8 fonttypecheck(const OEMCHAR *fname) {

const OEMCHAR	*p;

	p = file_getext(fname);
	if (!file_cmpname(p, str_bmp)) {
		return(FONTTYPE_PC98);
	}
	p = file_getname(fname);
	if (!file_cmpname(p, v98fontname)) {
		return(FONTTYPE_V98);
	}
	if ((!file_cmpname(p, pc88ankname)) ||
		(!file_cmpname(p, pc88knj1name)) ||
		(!file_cmpname(p, pc88knj2name))) {
		return(FONTTYPE_PC88);
	}
	if ((!file_cmpname(p, fm7ankname)) ||
		(!file_cmpname(p, fm7knjname))) {
		return(FONTTYPE_FM7);
	}
	if ((!file_cmpname(p, x1ank1name)) ||
		(!file_cmpname(p, x1ank2name)) ||
		(!file_cmpname(p, x1knjname))) {
		return(FONTTYPE_X1);
	}
	if (!file_cmpname(p, x68kfontname)) {
		return(FONTTYPE_X68);
	}
	return(FONTTYPE_NONE);
}

/**
 * Loads font files
 * @param[in] filename The name of the font file
 * @param[in] force If this parameter is TRUE, load file always
 *                  If this parameter is FALSE, load when font is not ready
 * @return font type
 */
UINT8 font_load(const OEMCHAR *filename, BOOL force) {

	UINT	i;
const UINT8	*p;
	UINT8	*q;
	UINT	j;
	OEMCHAR	fname[MAX_PATH];
	UINT8	type;
	UINT8	loading;

	if (filename) {
		file_cpyname(fname, filename, NELEMENTS(fname));
	}
	else {
		fname[0] = '\0';
	}
	type = fonttypecheck(fname);
	if ((!type) && (!force)) {
		return(0);
	}

	// ŠOŽš: font[??560-??57f], font[??d60-??d7f] ‚Íí‚ç‚È‚¢‚æ‚¤‚Éc
	for (i=0; i<0x80; i++) {
		q = fontrom + (i << 12);
		ZeroMemory(q + 0x000, 0x0560 - 0x000);
		ZeroMemory(q + 0x580, 0x0d60 - 0x580);
		ZeroMemory(q + 0xd80, 0x1000 - 0xd80);
	}

	fontdata_ank8store(fontdata_8, 0, 256);
	p = fontdata_8;
	q = fontrom + 0x80000;
	for (i=0; i<256; i++) {
		for (j=0; j<8; j++) {
			q[0] = p[0];
			q[1] = p[0];
			p += 1;
			q += 2;
		}
	}

	loading = 0xff;
	switch(type) {
		case FONTTYPE_PC98:
			loading = fontpc98_read(fname, loading);
			break;

		case FONTTYPE_V98:
			loading = fontv98_read(fname, loading);
			break;

		case FONTTYPE_PC88:
			loading = fontpc88_read(fname, loading);
			break;

		case FONTTYPE_FM7:
			loading = fontfm7_read(fname, loading);
			break;

		case FONTTYPE_X1:
			loading = fontx1_read(fname, loading);
			break;

		case FONTTYPE_X68:
			loading = fontx68k_read(fname, loading);
			break;
	}
	loading = fontpc98_read(file_getcd(pc98fontname), loading);
	loading = fontv98_read(file_getcd(v98fontname), loading);
	loading = fontpc88_read(file_getcd(pc88ankname), loading);
	if (loading & FONTLOAD_16) {
		file_cpyname(fname, file_getcd(fonttmpname), NELEMENTS(fname));
		if (file_attr(fname) == -1) {
			makepc98bmp(fname);
		}
		loading = fontpc98_read(fname, loading);
	}
	return(type);
}
