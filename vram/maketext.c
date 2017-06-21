#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"vram.h"
#include	"scrndraw.h"
#include	"dispsync.h"
#include	"maketext.h"
#include	"font/font.h"


		TRAM_T	tramflag;
static	UINT32	text_table[512];
static	UINT32	text_tblx2[512][2];


void maketext_initialize(void) {

	int		i;
	int		j;
	UINT8	bit;

	ZeroMemory(text_table, sizeof(text_table));
	for (i=0; i<8; i++) {
		for (j=0; j<16; j++) {
#if defined(BYTESEX_LITTLE)
			for (bit=1; bit<0x10; bit<<=1)
#elif defined(BYTESEX_BIG)
			for (bit=8; bit; bit>>=1)
#endif
			{
				text_table[i*16+j] <<= 8;
				text_table[i*16+j+128] <<= 8;
				if (j & bit) {
					text_table[i*16+j] |= (i+1) << 4;
				}
				else {
					text_table[i*16+j+128] |= (i+1) << 4;
				}
			}
		}
	}
	for (i=0; i<256; i++) {
		text_table[i+256] = text_table[i ^ 0x80];
	}
	for (i=0; i<512; i++) {
#if defined(BYTESEX_LITTLE)
		text_tblx2[i][0] = (text_table[i] & 0x000000ff);
		text_tblx2[i][0] |= (text_table[i] & 0x0000ffff) << 8;
		text_tblx2[i][0] |= (text_table[i] & 0x0000ff00) << 16;
		text_tblx2[i][1] = (text_table[i] & 0x00ff0000) >> 16;
		text_tblx2[i][1] |= (text_table[i] & 0xffff0000) >> 8;
		text_tblx2[i][1] |= (text_table[i] & 0xff000000);
#elif defined(BYTESEX_BIG)
		text_tblx2[i][0]  = (text_table[i] & 0xff000000);
		text_tblx2[i][0] |= (text_table[i] & 0xffff0000) >> 8;
		text_tblx2[i][0] |= (text_table[i] & 0x00ff0000) >> 16;
		text_tblx2[i][1]  = (text_table[i] & 0x0000ff00) << 16;
		text_tblx2[i][1] |= (text_table[i] & 0x0000ffff) << 8;
		text_tblx2[i][1] |= (text_table[i] & 0x000000ff);
#endif
	}
}

void maketext_reset(void) {

	ZeroMemory(&tramflag, sizeof(tramflag));
}

static UINT8 dirtyonblink(void) {

	UINT8	ret;
	int		i;

	ret = 0;
	for (i=0; i<0x1000; i++) {
		if (mem[0xa2000 + i*2] & TXTATR_BL) {
			ret = 1;
			tramupdate[i] |= 1;
		}
	}
	return(ret);
}

UINT8 maketext_curblink(void) {

	UINT8	ret;
	UINT16	csrw;

	ret = 0;
	if (tramflag.renewal & 1) {
		tramflag.curdisp = tramflag.count & 1;
		if (!(gdc.m.para[GDC_CSRFORM] & 0x80)) {
			tramflag.curdisp = 0;
		}
		else if (gdc.m.para[GDC_CSRFORM+1] & 0x20) {
			tramflag.curdisp = 1;
		}
		csrw = LOADINTELWORD(gdc.m.para + GDC_CSRW);
		if ((tramflag.curdisp != tramflag.curdisplast) ||
			(tramflag.curpos != csrw)) {
			if ((tramflag.curdisplast) && (tramflag.curpos < 0x1000)) {
				tramupdate[tramflag.curpos] |= 1;
			}
			tramflag.curdisplast = tramflag.curdisp;
			tramflag.curpos = csrw;
			if ((tramflag.curdisplast) && (tramflag.curpos < 0x1000)) {
				tramupdate[tramflag.curpos] |= 1;
			}
			ret = GDCSCRN_REDRAW;
		}
	}
	if (tramflag.renewal & 2) {
		tramflag.blinkdisp = ((tramflag.count & 3)?1:0);
		if (tramflag.blink) {
			tramflag.blink = dirtyonblink();
			if (tramflag.blink) {
				ret = GDCSCRN_REDRAW;
			}
		}
	}
	tramflag.renewal = 0;
	return(ret);
}

void maketext(int text_renewal) {

	UINT8	multiple;
	UINT8	TEXT_LR;
	int		TEXT_PL;
	int		TEXT_BL;
	int		TEXT_CL;
	int		TEXT_SUR;
	int		TEXT_SDR;
	int 	topline;
	int		lines;
	int		nowline;
	UINT8	wait2;
	UINT	pitch;
	UINT	csrw;
	UINT	esi;
	UINT	scroll;
	int		scrp;
	UINT8	wait1;
	UINT8	LRcnt;
	BOOL	reloadline;
	int		new_flag;
	int		cur_line;
	int		linecnt;
	UINT8	*q;
	UINT	y;
	UINT8	line_effect = 0;		// for gcc
	int		x;
	UINT32	bitmap[TEXTXMAX];
	UINT8	curx[TEXTXMAX+1];
	UINT16	color[TEXTXMAX];

	if (text_renewal) {
		tramflag.gaiji = 0;
	}

	multiple = ((!(gdc.mode1 & 8)) && (!(gdc.crt15khz & 1)))?0x20:0x00;
	TEXT_LR = gdc.m.para[GDC_CSRFORM] & 0x1f;
	TEXT_PL = crtc.reg.pl;
	TEXT_BL = crtc.reg.bl + 1;
	TEXT_CL = crtc.reg.cl;
	TEXT_SUR = crtc.reg.sur;
	TEXT_SDR = -1;
	if (TEXT_CL > 16) {
		TEXT_CL = 16;
	}
	if (TEXT_PL >= 16) {
		topline = TEXT_PL - 32;
		lines = TEXT_BL;
	}
	else {
		topline = TEXT_PL;
		lines = TEXT_BL - topline;
		if (lines <= 0) {
			lines += 32;											// 補正
		}
	}
	nowline = topline;

	wait2 = 0;
	if (!TEXT_SUR) {
		wait2 = crtc.reg.ssl;
		TEXT_SDR = crtc.reg.sdr + 1;
	}
	else {
		TEXT_SUR = 32 - TEXT_SUR;
	}

	pitch = gdc.m.para[GDC_PITCH] & 0xfe;
	csrw = LOADINTELWORD(gdc.m.para + GDC_CSRW);
	esi = LOW12(LOADINTELWORD(gdc.m.para + GDC_SCROLL));
	scroll = LOADINTELWORD(gdc.m.para + GDC_SCROLL + 2);
	scroll = LOW14(scroll) >> 4;
	scrp = 0;

	wait1 = 0;
	LRcnt = 0;
	reloadline = FALSE;
	new_flag = 0;
	cur_line = -1;
	linecnt = 0;
	q = np2_tram + dsync.textvad;
	for (y=dsync.text_vbp; y<dsync.textymax;) {
		if (!wait1) {
			if (LRcnt-- == 0) {
				LRcnt = TEXT_LR;
				reloadline = TRUE;
			}
		}
		else {
			wait1--;
		}
		if (reloadline) {
			reloadline = FALSE;
			new_flag = text_renewal;
			cur_line = -1;
			line_effect = 0;
			if (!new_flag) {
				UINT edi;
				edi = esi;
				for (x=0; x<TEXTXMAX; x++) {
					if (tramupdate[edi]) {
						new_flag = 1;
						break;
					}
					edi = LOW12(edi + 1);
				}
			}
			if (new_flag) {
				UINT edi;
				UINT32 gaiji1st;
				BOOL kanji2nd;
				UINT32 lastbitp;
				edi = esi;
				gaiji1st = 0;
				kanji2nd = FALSE;
				lastbitp = 0;
				for (x=0; x<TEXTXMAX; x++) {						// width80
					if (edi == csrw) {
						cur_line = x;
					}
					curx[x] = mem[0xa2000 + edi*2] & (~TEXTATR_RGB);
					line_effect |= curx[x];
					color[x] = (mem[0xa2000 + edi*2] & (TEXTATR_RGB)) >> 1;
					if (curx[x] & TXTATR_RV) {				// text reverse
						color[x] |= 0x80;
					}
					if (kanji2nd) {
						kanji2nd = FALSE;
						bitmap[x] = lastbitp + 0x800;
						curx[x-1] |= 0x80;
						curx[x] |= curx[x-1] & 0x20;
					}
					else if (!(mem[0xa0001 + edi*2] & gdc.bitac)) {
						gaiji1st = 0;
						if (gdc.mode1 & 8) {
							bitmap[x] = 0x80000 +
										(mem[0xa0000 + edi*2] << 4);
							if ((curx[x] & TXTATR_BG) && (gdc.mode1 & 1)) {
								bitmap[x] += 0x1000;
							}
						}
						else {
							bitmap[x] = 0x82000 + 
										(mem[0xa0000 + edi*2] << 4);
							curx[x] |= multiple;					// ver0.74
							if ((curx[x] & TXTATR_BG) && (gdc.mode1 & 1)) {
								bitmap[x] += 8;
							}
						}
					}
					else {
						UINT kc;
						kc = LOADINTELWORD(mem + 0xa0000 + edi*2);
						bitmap[x] = (kc & 0x7f7f) << 4;
						kc &= 0x7f;									// ver0.78
						if ((kc == 0x56) || (kc == 0x57)) {
							tramflag.gaiji = 1;
							if ((gaiji1st) &&
								(bitmap[x] == (lastbitp & (~15)))) {
								curx[x-1] |= 0x80;
							}
							bitmap[x] += gaiji1st;
							gaiji1st ^= 0x800;
						}
						else {
							gaiji1st = 0;
							if ((kc < 0x09) || (kc >= 0x0c)) {
								kanji2nd = TRUE;
							}
						}
						if ((curx[x] & TXTATR_BG) && (gdc.mode1 & 1)) {
							curx[x] |= 0x20;
							bitmap[x] += 8;
						}
						else if (!(gdc.mode1 & 8)) {
							curx[x] |= multiple;
						}
					}
					lastbitp = bitmap[x];
					if (!(curx[x] & TXTATR_ST)) {
						bitmap[x] = 0;
					}
					else if (curx[x] & TXTATR_BL) {
						tramflag.blink = 1;
						if (!tramflag.blinkdisp) {
							bitmap[x] = 0;
						}
					}
					edi = LOW12(edi + 1);
				}
				if (!tramflag.curdisp) {
					cur_line = -1;
				}
			}
			esi = LOW12(esi + pitch);
		}

		if ((!TEXT_SDR) && (nowline >= topline + crtc.reg.ssl)) {
			nowline = topline;
			TEXT_SDR--;
			wait1 = crtc.reg.ssl;
		}

		if (!wait2) {
			if (new_flag) {
				renewal_line[y] |= 4;
				if (cur_line >= 0) {
					if ((nowline >= (gdc.m.para[GDC_CSRFORM+1] & 0x1f)) &&
						(nowline <= (gdc.m.para[GDC_CSRFORM+2] >> 3))) {
						color[cur_line] |= 256;
						if (curx[cur_line] & 0x80) {
							color[cur_line+1] |= 256;
						}
					}
					else {
						color[cur_line] &= ~(256);
						if (curx[cur_line] & 0x80) {
							color[cur_line+1] &= ~(256);
						}
					}
				}
				if ((nowline >= 0) && (nowline < TEXT_CL)) {
					// width80
					for (x=0; x<TEXTXMAX; x++) {
						int fntline;
						UINT8 data;
						fntline = nowline;
						if (curx[x] & 0x20) {
							fntline >>= 1;
						}
						data = fontrom[bitmap[x] + (fntline & 0x0f)];
						*(UINT32 *)(q+0) = text_table[color[x] + (data >> 4)];
						*(UINT32 *)(q+4) = text_table[color[x] + (data & 15)];
						q += 8;
					}
				}
				else {
					// width80
					for (x=0; x<(TEXTXMAX); x++) {
						*(UINT32 *)(q+0) = text_table[color[x]];
						*(UINT32 *)(q+4) = text_table[color[x]];
						q += 8;
					}
				}
				if ((line_effect & TXTATR_UL) &&
					((nowline + 1) == lines)) {			// アンダーライン位置
					// width80
					q -= TEXTXMAX * 8;
					q += 4;
					for (x=0; x<(TEXTXMAX-1); x++) {
						if (curx[x] & TXTATR_UL) {
							*(UINT32 *)(q+0) = text_table[(color[x] & 0x70)
																	+ 0x0f];
							*(UINT32 *)(q+4) = text_table[(color[x+1] & 0x70)
																	+ 0x0f];
						}
						q += 8;
					}
					if (curx[TEXTXMAX-1] & TXTATR_UL) {
						*(UINT32 *)q = text_table[(color[TEXTXMAX-1] & 0x70)
																	+ 0x0f];
					}
					q += 4;
				}
				if ((line_effect & TXTATR_VL) && (!(gdc.mode1 & 1))) {
					// width80
					q -= TEXTXMAX * 8;
					for (x=0; x<TEXTXMAX; x++) {
						if (curx[x] & TXTATR_VL) {
							// text_table[] を 使ってないので注意
							*(q+4) |= (color[x] & 0x70) + 0x10;
						}
						q += 8;
					}
				}
				// *(q+4) |= (color[x] & 0x70) + 0x10; は…
				// *(DWORD *)(q+4) |= text_table[(color[x] & 0x70) + 8];
				// で等価になる筈・・・
			}
			else {
				q += TEXTXMAX * 8;
			}
			y++;
			if (!(--scroll)) {
				scrp = (scrp + 4) & 0x0c;
				esi = LOW12(LOADINTELWORD(gdc.m.para + GDC_SCROLL + scrp));
				scroll = LOADINTELWORD(gdc.m.para + GDC_SCROLL + scrp + 2);
				scroll = LOW14(scroll) >> 4;
				reloadline = TRUE;
			}
		}
		else {
			wait2--;
		}

		nowline++;
		if ((TEXT_SDR) && (nowline >= lines)) {
			nowline = topline;
			TEXT_SDR--;
			if (++linecnt == TEXT_SUR) {
				wait2 = crtc.reg.ssl;
				TEXT_SDR = crtc.reg.sdr + 1;
			}
		}
	}
	ZeroMemory(tramupdate, sizeof(tramupdate));
}

void maketext40(int text_renewal) {

	UINT8	multiple;
	UINT8	TEXT_LR;
	int		TEXT_PL;
	int		TEXT_BL;
	int		TEXT_CL;
	int		TEXT_SUR;
	int		TEXT_SDR;
	int		topline;
	int		lines;
	int		nowline;
	UINT8	wait2;
	UINT	pitch;
	UINT	csrw;
	UINT	esi;
	UINT	scroll;
	int		scrp;
	UINT8	wait1;
	UINT8	LRcnt;
	BOOL	reloadline;
	int		new_flag;
	int		cur_line;
	int		linecnt;
	UINT8	*q;
	UINT	y;
	UINT8	line_effect = 0;		// for gcc
	int		x;
	UINT32	bitmap[TEXTXMAX];
	UINT8	curx[TEXTXMAX+1];
	UINT16	color[TEXTXMAX];

	if (text_renewal) {
		tramflag.gaiji = 0;
	}

	multiple = ((!(gdc.mode1 & 8)) && (!(gdc.crt15khz & 1)))?0x20:0x00;
	TEXT_LR = gdc.m.para[GDC_CSRFORM] & 0x1f;
	TEXT_PL = crtc.reg.pl;
	TEXT_BL = crtc.reg.bl + 1;
	TEXT_CL = crtc.reg.cl;
	TEXT_SUR = crtc.reg.sur;
	TEXT_SDR = -1;
	if (TEXT_CL > 16) {
		TEXT_CL = 16;
	}
	if (TEXT_PL >= 16) {
		topline = TEXT_PL - 32;
		lines = TEXT_BL;
	}
	else {
		topline = TEXT_PL;
		lines = TEXT_BL - topline;
		if (lines <= 0) {
			lines += 32;											// 補正
		}
	}
	nowline = topline;

	wait2 = 0;
	if (!TEXT_SUR) {
		wait2 = crtc.reg.ssl;
		TEXT_SDR = crtc.reg.sdr + 1;
	}
	else {
		TEXT_SUR = 32 - TEXT_SUR;
	}

	pitch = gdc.m.para[GDC_PITCH] & 0xfe;
	csrw = LOADINTELWORD(gdc.m.para + GDC_CSRW);
	esi = LOW12(LOADINTELWORD(gdc.m.para + GDC_SCROLL));
	scroll = LOADINTELWORD(gdc.m.para + GDC_SCROLL + 2);
	scroll = LOW14(scroll) >> 4;
	scrp = 0;

	wait1 = 0;
	LRcnt = 0;
	reloadline = FALSE;
	new_flag = 0;
	cur_line = -1;
	linecnt = 0;
	q = np2_tram + dsync.textvad;
	for (y=dsync.text_vbp; y<dsync.textymax;) {
		if (!wait1) {
			if (LRcnt-- == 0) {
				LRcnt = TEXT_LR;
				reloadline = TRUE;
			}
		}
		else {
			wait1--;
		}
		if (reloadline) {
			reloadline = FALSE;
			new_flag = text_renewal;
			cur_line = -1;
			line_effect = 0;
			if (!new_flag) {
				UINT edi;
				edi = esi;
				for (x=0; x<TEXTXMAX; x++) {
					if (tramupdate[edi]) {
						new_flag = 1;
						break;
					}
					edi = LOW12(edi + 1);
				}
			}
			if (new_flag) {
				UINT edi;
				UINT32 gaiji1st;
				BOOL kanji2nd;
				UINT32 lastbitp;
				edi = esi;
				gaiji1st = 0;
				kanji2nd = FALSE;
				lastbitp = 0;
				for (x=0; x<(TEXTXMAX/2); x++) {					// width40
					if (edi == csrw) {
						cur_line = x;
					}
					curx[x] = mem[0xa2000 + edi*2] & (~TEXTATR_RGB);
					line_effect |= curx[x];
					color[x] = (mem[0xa2000 + edi*2] & (TEXTATR_RGB)) >> 1;
					if (curx[x] & TXTATR_RV) {				// text reverse
						color[x] |= 0x80;
					}
					if (kanji2nd) {
						kanji2nd = FALSE;
						bitmap[x] = lastbitp + 0x800;
						curx[x-1] |= 0x80;
						curx[x] |= curx[x-1] & 0x20;
					}
					else if (!(mem[0xa0001 + edi*2] & gdc.bitac)) {
						gaiji1st = 0;
						if (gdc.mode1 & 8) {
							bitmap[x] = 0x80000 +
										(mem[0xa0000 + edi*2] << 4);
							if ((curx[x] & TXTATR_BG) && (gdc.mode1 & 1)) {
								bitmap[x] += 0x1000;
							}
						}
						else {
							bitmap[x] = 0x82000 + 
										(mem[0xa0000 + edi*2] << 4);
							curx[x] |= multiple;					// ver0.74
							if ((curx[x] & TXTATR_BG) && (gdc.mode1 & 1)) {
								bitmap[x] += 8;
							}
						}
					}
					else {
						UINT kc;
						kc = LOADINTELWORD(mem + 0xa0000 + edi*2);
						bitmap[x] = (kc & 0x7f7f) << 4;
						kc &= 0x7f;									// ver0.78
						if ((kc == 0x56) || (kc == 0x57)) {
							tramflag.gaiji = 1;
							if ((gaiji1st) &&
								(bitmap[x] == (lastbitp & (~15)))) {
								curx[x-1] |= 0x80;
							}
							bitmap[x] += gaiji1st;
							gaiji1st ^= 0x800;
						}
						else {
							gaiji1st = 0;
							if ((kc < 0x09) || (kc >= 0x0c)) {
								kanji2nd = TRUE;
							}
						}
						if ((curx[x] & TXTATR_BG) && (gdc.mode1 & 1)) {
							curx[x] |= 0x20;
							bitmap[x] += 8;
						}
						else if (!(gdc.mode1 & 8)) {
							curx[x] |= multiple;
						}
					}
					lastbitp = bitmap[x];
					if (!(curx[x] & TXTATR_ST)) {
						bitmap[x] = 0;
					}
					else if (curx[x] & TXTATR_BL) {
						tramflag.blink = 1;
						if (!tramflag.blinkdisp) {
							bitmap[x] = 0;
						}
					}
					edi = LOW12(edi + 2);							// width40
				}
				if (!tramflag.curdisp) {
					cur_line = -1;
				}
			}
			esi = LOW12(esi + pitch);
		}

		if ((!TEXT_SDR) && (nowline >= topline + crtc.reg.ssl)) {
			nowline = topline;
			TEXT_SDR--;
			wait1 = crtc.reg.ssl;
		}

		if (!wait2) {
			if (new_flag) {
				renewal_line[y] |= 4;
				if (cur_line >= 0) {
					if ((nowline >= (gdc.m.para[GDC_CSRFORM+1] & 0x1f)) &&
						(nowline <= (gdc.m.para[GDC_CSRFORM+2] >> 3))) {
						color[cur_line] |= 256;
						if (curx[cur_line] & 0x80) {
							color[cur_line+1] |= 256;
						}
					}
					else {
						color[cur_line] &= ~(256);
						if (curx[cur_line] & 0x80) {
							color[cur_line+1] &= ~(256);
						}
					}
				}
				if ((nowline >= 0) && (nowline < TEXT_CL)) {
					// width40
					for (x=0; x<(TEXTXMAX/2); x++) {
						int fntline;
						UINT8 data;
						fntline = nowline;
						if (curx[x] & 0x20) {
							fntline >>= 1;
						}
						data = fontrom[bitmap[x] + (fntline & 0x0f)];
						*(UINT32 *)(q+ 0) = text_tblx2[color[x] +
															(data>>4)][0];
						*(UINT32 *)(q+ 4) = text_tblx2[color[x] +
															(data>>4)][1];
						*(UINT32 *)(q+ 8) = text_tblx2[color[x] +
															(data&0xf)][0];
						*(UINT32 *)(q+12) = text_tblx2[color[x] +
															(data&0xf)][1];
						q += 16;
					}
				}
				else {
					// width40
					for (x=0; x<(TEXTXMAX/2); x++) {
						*(UINT32 *)(q+ 0) = text_table[color[x]];
						*(UINT32 *)(q+ 4) = text_table[color[x]];
						*(UINT32 *)(q+ 8) = text_table[color[x]];
						*(UINT32 *)(q+12) = text_table[color[x]];
						q += 16;
					}
				}
				if ((line_effect & TXTATR_UL) &&
					((nowline + 1) == lines)) {			// アンダーライン位置
					// width40
					q -= TEXTXMAX * 8;
					q += 4;
					for (x=0; x<((TEXTXMAX/2)-1); x++) {
						if (curx[x] & 8) {
							*(UINT32 *)(q+ 0) = text_table[(color[x] & 0x70)
																	+ 0x0f];
							*(UINT32 *)(q+ 4) = text_table[(color[x] & 0x70)
																	+ 0x0f];
							*(UINT32 *)(q+ 8) = text_table[(color[x] & 0x70)
																	+ 0x0f];
							*(UINT32 *)(q+12) = text_table[(color[x+1] & 0x70)
																	+ 0x0f];
						}
						q += 16;
					}
					if (curx[(TEXTXMAX/2)-1] & TXTATR_UL) {
						*(UINT32 *)(q+0) = text_table[
										(color[TEXTXMAX-1] & 0x70) + 0x0f];
						*(UINT32 *)(q+4) = text_table[
										(color[TEXTXMAX-1] & 0x70) + 0x0f];
						*(UINT32 *)(q+8) = text_table[
										(color[TEXTXMAX-1] & 0x70) + 0x0f];
					}
					q += 12;
				}
				if ((line_effect & TXTATR_VL) && (!(gdc.mode1 & 1))) {
					// width40
					q -= TEXTXMAX * 8;
					for (x=0; x<(TEXTXMAX/2); x++) {
						if (curx[x] & TXTATR_VL) {
							// text_table[] を 使ってないので注意
							*(q+ 4) |= (color[x] & 0x70) + 0x10;
							*(q+12) |= (color[x] & 0x70) + 0x10;
						}
						q += 16;
					}
				}
			}
			else {
				q += TEXTXMAX * 8;
			}
			y++;
			if (!(--scroll)) {
				scrp = (scrp + 4) & 0x0c;
				esi = LOW12(LOADINTELWORD(gdc.m.para + GDC_SCROLL + scrp));
				scroll = LOADINTELWORD(gdc.m.para + GDC_SCROLL + scrp + 2);
				scroll = LOW14(scroll) >> 4;
				reloadline = TRUE;
			}
		}
		else {
			wait2--;
		}

		nowline++;
		if ((TEXT_SDR) && (nowline >= lines)) {
			nowline = topline;
			TEXT_SDR--;
			if (++linecnt == TEXT_SUR) {
				wait2 = crtc.reg.ssl;
				TEXT_SDR = crtc.reg.sdr + 1;
			}
		}
	}
	ZeroMemory(tramupdate, sizeof(tramupdate));
}

