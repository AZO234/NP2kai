#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"vram.h"
#include	"scrndraw.h"
#include	"dispsync.h"
#include	"palettes.h"
#include	"maketext.h"
#include	"maketgrp.h"
#include	"makegrph.h"
#include	"font/font.h"
#include	"makegrph.mcr"


// extern	int		displaymoder;
#define	displaymoder	dsync.scrnxextend


void maketextgrph(int plane, int text_renewal, int grph_renewal) {

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
	UINT	m_pitch;
	UINT	esi;
	UINT	m_scr;
	int		m_scrp;
	UINT	s_pitch;
	UINT	ebp;
	UINT	s_scr;
	int		s_scrp;
	int		s_scrpmask;
	UINT8	GRPH_LR;
	UINT8	GRPH_LRcnt;
	UINT32	ppage;
	UINT32	gbit;
	UINT	ymax;
	UINT8	*q;
	UINT8	wait1;
	UINT8	TEXT_LRcnt;
	BOOL	reloadline;
	int		new_flag;
	int		linecnt;
	UINT	y;
	UINT	edi;
	UINT	x;
	int		i;
	UINT8	color[TEXTXMAX];
	UINT32	bit[160];

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
			lines += 32;
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

	m_pitch = gdc.m.para[GDC_PITCH] & 0xfe;
	esi = LOW12(LOADINTELWORD(gdc.m.para + GDC_SCROLL));
	m_scr = LOADINTELWORD(gdc.m.para + GDC_SCROLL + 2);
	m_scr = LOW14(m_scr) >> 4;
	m_scrp = 0;

	s_pitch = gdc.s.para[GDC_PITCH];
	if (!(gdc.clock & 0x80)) {
		s_pitch <<= 1;
	}
	s_pitch &= 0xfe;
	ebp = LOADINTELWORD(gdc.s.para + GDC_SCROLL);
	ebp = LOW15(ebp << 1);
	s_scr = LOADINTELWORD(gdc.s.para + GDC_SCROLL + 2);
	s_scr = LOW14(s_scr) >> 4;
	s_scrp = 0;
	s_scrpmask = (np2cfg.uPD72020)?0x4:0xc;

	GRPH_LR = gdc.s.para[GDC_CSRFORM] & 0x1f;
	GRPH_LRcnt = GRPH_LR;

	// グラフのほーが上…
	if (dsync.text_vbp > dsync.grph_vbp) {
		UINT remain;
		remain = dsync.text_vbp - dsync.grph_vbp;
		do {
			if (!GRPH_LRcnt) {
				GRPH_LRcnt = GRPH_LR;
				s_scr--;
				if (!s_scr) {
					s_scrp = (s_scrp + 4) & s_scrpmask;
					ebp = LOADINTELWORD(gdc.s.para + GDC_SCROLL + s_scrp);
					ebp = LOW15(ebp << 1);
					s_scr = LOADINTELWORD(gdc.s.para + GDC_SCROLL +
																s_scrp + 2);
					s_scr = LOW14(s_scr) >> 4;
				}
				else {
					ebp = LOW15(ebp + s_pitch);
				}
			}
			else {
				GRPH_LRcnt--;
			}
		} while(--remain);
	}

	ppage = (plane)?VRAM_STEP:0;
	gbit = 0x01010101 << plane;
	ymax = min(dsync.textymax, dsync.grphymax);
	q = np2_vram[plane] + dsync.textvad;
	wait1 = 0;
	TEXT_LRcnt = 0;
	reloadline = FALSE;
	new_flag = 0;
	linecnt = 0;
	for (y=dsync.text_vbp; y<ymax;) {
		if (!wait1) {
			if (TEXT_LRcnt-- == 0) {
				TEXT_LRcnt = TEXT_LR;
				reloadline = TRUE;
			}
		}
		else {
			wait1--;
		}
		if (reloadline) {
			reloadline = FALSE;
			new_flag = text_renewal;
			if (!new_flag) {
				edi = esi;
				for (x=0; x<TEXTXMAX; x++) {
					if (tramupdate[edi] & 2) {
						new_flag = 1;
						break;
					}
					edi = LOW12(edi + 1);
				}
			}
			edi = esi;
			for (x=0; x<TEXTXMAX; x++) {							// width80
				color[x] = (mem[0xa2000 + edi*2] & (TEXTATR_RGB)) >> 5;
				edi = LOW12(edi + 1);								// width80
			}
			esi = LOW12(esi + m_pitch);
		}

		if ((!TEXT_SDR) && (nowline >= topline + crtc.reg.ssl)) {
			nowline = topline;
			TEXT_SDR--;
			wait1 = crtc.reg.ssl;
		}

		if (!wait2) {
			int grph_new;
			grph_new = 0;
			if (y >= dsync.grph_vbp) {
				grph_new = new_flag | grph_renewal;
				if (!grph_new) {
					UINT vc = ebp;
					for (x=0; x<TEXTXMAX; x++) {
						if (vramupdate[vc] & (UINT8)gbit) {
							grph_new = 1;
							break;
						}
						vc = LOW15(vc + 1);
					}
				}
			}
			if (grph_new) {
				UINT32 vc = ebp + ppage;
				UINT32 *p;
				UINT8 *d;
				UINT xdot;
				p = bit;
				for (x=0; x<TEXTXMAX; x++) {
					GRPHDATASET(p, vc);
					p += 2;
					vc = VRAMADDRMASKEX(vc + 1);
				}
				d = (UINT8 *)bit;
				ZeroMemory(q, TEXTXMAX * 8);
				// width80
				xdot = 8 - displaymoder;
				for (x=0; x<TEXTXMAX; x++) {
					do {
						if (pal_monotable[*d++]) {
							*q = color[x];
						}
						q++;
					} while(--xdot);
					xdot = 8;
				}
				q += displaymoder;
				renewal_line[y] |= (UINT8)gbit;
			}
			else {
				q += TEXTXMAX * 8;
			}
			y++;
			m_scr--;
			if (!m_scr) {
				m_scrp = (m_scrp + 4) & 0x0c;
				esi = LOW12(LOADINTELWORD(gdc.m.para + GDC_SCROLL + m_scrp));
				m_scr = LOADINTELWORD(gdc.m.para + GDC_SCROLL + m_scrp + 2);
				m_scr = LOW14(m_scr) >> 4;
				reloadline = TRUE;
			}
			if (!GRPH_LRcnt) {
				GRPH_LRcnt = GRPH_LR;
				s_scr--;
				if (!s_scr) {
					s_scrp = (s_scrp + 4) & s_scrpmask;
					ebp = LOADINTELWORD(gdc.s.para + GDC_SCROLL + s_scrp);
					ebp = LOW15(ebp << 1);
					s_scr = LOADINTELWORD(gdc.s.para + GDC_SCROLL +
																s_scrp + 2);
					s_scr = LOW14(s_scr) >> 4;
				}
				else {
					ebp = LOW15(ebp + s_pitch);
				}
			}
			else {
				GRPH_LRcnt--;
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

	gbit = ~gbit;
	for (i=0; i<0x8000; i+=4) {
		*(UINT32 *)(vramupdate + i) &= gbit;
	}
}

void maketextgrph40(int plane, int text_renewal, int grph_renewal) {

	UINT8	TEXT_LR;
	int		TEXT_PL;
	int		TEXT_BL;
	int		TEXT_CL;
	int		TEXT_SUR;
	int		TEXT_SDR;
	int		topline;
	int		lines;
	int		nowline = 0;
	UINT8	wait2 = 0;
	UINT	m_pitch;
	UINT	esi;
	UINT	m_scr;
	int		m_scrp;
	UINT	s_pitch;
	UINT	ebp;
	UINT	s_scr;
	int		s_scrp;
	int		s_scrpmask;
	UINT8	GRPH_LR;
	UINT8	GRPH_LRcnt;
	UINT32	ppage;
	UINT32	gbit;
	UINT	ymax;
	UINT8	*q;
	UINT8	wait1;
	UINT8	TEXT_LRcnt;
	BOOL	reloadline;
	int		new_flag;
	int		linecnt;
	UINT	y;
	UINT	edi;
	UINT	x;
	int		i;
	UINT8	color[TEXTXMAX];
	UINT32	bit[160];

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
			lines += 32;
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

	m_pitch = gdc.m.para[GDC_PITCH] & 0xfe;
	esi = LOW12(LOADINTELWORD(gdc.m.para + GDC_SCROLL));
	m_scr = LOADINTELWORD(gdc.m.para + GDC_SCROLL + 2);
	m_scr = LOW14(m_scr) >> 4;
	m_scrp = 0;

	s_pitch = gdc.s.para[GDC_PITCH];
	if (!(gdc.clock & 0x80)) {
		s_pitch <<= 1;
	}
	s_pitch &= 0xfe;
	ebp = LOADINTELWORD(gdc.s.para + GDC_SCROLL);
	ebp = LOW15(ebp << 1);
	s_scr = LOADINTELWORD(gdc.s.para + GDC_SCROLL + 2);
	s_scr = LOW14(s_scr) >> 4;
	s_scrp = 0;
	s_scrpmask = (np2cfg.uPD72020)?0x4:0xc;

	GRPH_LR = gdc.s.para[GDC_CSRFORM] & 0x1f;
	GRPH_LRcnt = GRPH_LR;

	// グラフのほーが上…
	if (dsync.text_vbp > dsync.grph_vbp) {
		UINT remain;
		remain = dsync.text_vbp - dsync.grph_vbp;
		do {
			if (!GRPH_LRcnt) {
				GRPH_LRcnt = GRPH_LR;
				s_scr--;
				if (!s_scr) {
					s_scrp = (s_scrp + 4) & s_scrpmask;
					ebp = LOADINTELWORD(gdc.s.para + GDC_SCROLL + s_scrp);
					ebp = LOW15(ebp << 1);
					s_scr = LOADINTELWORD(gdc.s.para + GDC_SCROLL +
																s_scrp + 2);
					s_scr = LOW14(s_scr) >> 4;
				}
				else {
					ebp = LOW15(ebp + s_pitch);
				}
			}
			else {
				GRPH_LRcnt--;
			}
		} while(--remain);
	}

	ppage = (plane)?VRAM_STEP:0;
	gbit = 0x01010101 << plane;
	ymax = min(dsync.textymax, dsync.grphymax);
	q = np2_vram[plane] + dsync.textvad;
	wait1 = 0;
	TEXT_LRcnt = 0;
	reloadline = FALSE;
	new_flag = 0;
	linecnt = 0;
	for (y=dsync.text_vbp; y<ymax;) {
		if (!wait1) {
			if (TEXT_LRcnt-- == 0) {
				TEXT_LRcnt = TEXT_LR;
				reloadline = TRUE;
			}
		}
		else {
			wait1--;
		}
		if (reloadline) {
			reloadline = FALSE;
			new_flag = text_renewal;
			if (!new_flag) {
				edi = esi;
				for (x=0; x<TEXTXMAX; x++) {
					if (tramupdate[edi] & 2) {
						new_flag = 1;
						break;
					}
					edi = LOW12(edi + 1);
				}
			}
			edi = esi;
			for (x=0; x<TEXTXMAX/2; x++) {							// width40
				color[x] = (mem[0xa2000 + edi*2] & (TEXTATR_RGB)) >> 5;
				edi = LOW12(edi + 2);								// width40
			}
			esi = LOW12(esi + m_pitch);
		}

		if ((!TEXT_SDR) && (nowline >= topline + crtc.reg.ssl)) {
			nowline = topline;
			TEXT_SDR--;
			wait1 = crtc.reg.ssl;
		}

		if (!wait2) {
			int grph_new;
			grph_new = 0;
			if (y >= dsync.grph_vbp) {
				grph_new = new_flag | grph_renewal;
				if (!grph_new) {
					UINT vc = ebp;
					for (x=0; x<TEXTXMAX; x++) {
						if (vramupdate[vc] & (UINT8)gbit) {
							grph_new = 1;
							break;
						}
						vc = LOW15(vc + 1);
					}
				}
			}
			if (grph_new) {
				UINT32 vc = ebp + ppage;
				UINT32 *p;
				UINT8 *d;
				UINT xdot;
				p = bit;
				for (x=0; x<TEXTXMAX; x++) {
					GRPHDATASET(p, vc);
					p += 2;
					vc = VRAMADDRMASKEX(vc + 1);
				}
				d = (UINT8 *)bit;
				ZeroMemory(q, TEXTXMAX * 8);
				// width40
				xdot = 16 - displaymoder;
				for (x=0; x<TEXTXMAX/2; x++) {
					do {
						if (pal_monotable[*d++]) {
							*q = color[x];
						}
						q++;
					} while(--xdot);
					xdot = 16;
				}
				q += displaymoder;
				renewal_line[y] |= (UINT8)gbit;
			}
			else {
				q += TEXTXMAX * 8;
			}
			y++;
			m_scr--;
			if (!m_scr) {
				m_scrp = (m_scrp + 4) & 0x0c;
				esi = LOW12(LOADINTELWORD(gdc.m.para + GDC_SCROLL + m_scrp));
				m_scr = LOADINTELWORD(gdc.m.para + GDC_SCROLL + m_scrp + 2);
				m_scr = LOW14(m_scr) >> 4;
				reloadline = TRUE;
			}
			if (!GRPH_LRcnt) {
				GRPH_LRcnt = GRPH_LR;
				s_scr--;
				if (!s_scr) {
					s_scrp = (s_scrp + 4) & s_scrpmask;
					ebp = LOADINTELWORD(gdc.s.para + GDC_SCROLL + s_scrp);
					ebp = LOW15(ebp << 1);
					s_scr = LOADINTELWORD(gdc.s.para + GDC_SCROLL +
																s_scrp + 2);
					s_scr = LOW14(s_scr) >> 4;
				}
				else {
					ebp = LOW15(ebp + s_pitch);
				}
			}
			else {
				GRPH_LRcnt--;
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

	gbit = ~gbit;
	for (i=0; i<0x8000; i+=4) {
		*(UINT32 *)(vramupdate + i) &= gbit;
	}
}

