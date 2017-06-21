#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios/bios.h"
#include	"lio.h"
#include	"vram.h"


typedef struct {
	UINT8	x1[2];
	UINT8	y1[2];
	UINT8	x2[2];
	UINT8	y2[2];
	UINT8	off[2];
	UINT8	seg[2];
	UINT8	leng[2];
} GGET;

typedef struct {
	UINT8	x[2];
	UINT8	y[2];
	UINT8	off[2];
	UINT8	seg[2];
	UINT8	leng[2];
	UINT8	mode;
	UINT8	colorsw;
	UINT8	fg;
	UINT8	bg;
} GPUT1;

typedef struct {
	UINT8	x[2];
	UINT8	y[2];
	UINT8	chr[2];
	UINT8	mode;
	UINT8	colorsw;
	UINT8	fg;
	UINT8	bg;
} GPUT2;

typedef struct {
	SINT16	x;
	SINT16	y;
	UINT16	width;
	UINT16	height;
	UINT16	off;
	UINT16	seg;
	UINT8	mode;
	UINT8	sw;
	UINT8	fg;
	UINT8	bg;
} LIOPUT;

typedef struct {
	UINT8	*baseptr;
	UINT	addr;
	UINT	sft;
	UINT	width;
	UINT8	mask;
} GETCNTX;

typedef struct {
	UINT8	*baseptr;
	UINT	addr;
	UINT	sft;
	UINT	width;
	UINT8	maskl;
	UINT8	maskr;
	UINT8	masklr;
	UINT8	mask;
	UINT8	pat[84];
} PUTCNTX;


static void getvram(const GETCNTX *gt, UINT8 *dst) {

	UINT8	*baseptr;
	UINT	addr;
	UINT	width;
	UINT	dat;
	UINT	sft;

	baseptr = gt->baseptr;
	addr = gt->addr;
	width = gt->width;
	sft = 8 - gt->sft;
	dat = baseptr[LOW15(addr)];
	addr++;
	while(width > 8) {
		width -= 8;
		dat = (dat << 8) + baseptr[LOW15(addr)];
		addr++;
		*dst = (UINT8)(dat >> sft);
		dst++;
	}
	dat = (dat << 8) + baseptr[LOW15(addr)];
	*dst = (UINT8)((dat >> sft) & gt->mask);
}

static void setdirty(UINT addr, UINT width, UINT height, REG8 bit) {

	UINT	r;

	gdcs.grphdisp |= bit;
	width = (width + 7) >> 3;
	while(height--) {
		r = 0;
		while(r < width) {
			vramupdate[LOW15(addr + r)] |= bit;
			r++;
		}
		addr += 80;
	}
}

static void putor(const PUTCNTX *pt) {

	UINT8	*baseptr;
	UINT	addr;
const UINT8	*src;
	UINT	width;
	UINT	dat;

	baseptr = pt->baseptr;
	addr = pt->addr;
	src = pt->pat;
	width = pt->width;
	dat = *src++;
	if ((pt->sft + width) < 8) {
		baseptr[LOW15(addr)] |= (UINT8)((dat >> pt->sft) & pt->masklr);
	}
	else {
		baseptr[LOW15(addr)] |= (UINT8)(dat >> pt->sft) & pt->maskl;
		addr++;
		width -= (8 - pt->sft);
		while(width > 8) {
			width -= 8;
			dat = (dat << 8) + (*src);
			src++;
			baseptr[LOW15(addr)] |= (UINT8)(dat >> pt->sft);
			addr++;
		}
		if (width) {
			dat = (dat << 8) + (*src);
			baseptr[LOW15(addr)] |= (UINT8)(dat >> pt->sft) & pt->maskr;
		}
	}
}

static void putorn(const PUTCNTX *pt) {

	UINT8	*baseptr;
	UINT	addr;
const UINT8	*src;
	UINT	width;
	UINT	dat;

	baseptr = pt->baseptr;
	addr = pt->addr;
	src = pt->pat;
	width = pt->width;
	dat = *src++;
	if ((pt->sft + width) < 8) {
		baseptr[LOW15(addr)] |= (UINT8)(((~dat) >> pt->sft) & pt->masklr);
	}
	else {
		baseptr[LOW15(addr)] |= (UINT8)(((~dat) >> pt->sft) & pt->maskl);
		addr++;
		width -= (8 - pt->sft);
		while(width > 8) {
			width -= 8;
			dat = (dat << 8) + (*src);
			src++;
			baseptr[LOW15(addr)] |= (UINT8)((~dat) >> pt->sft);
			addr++;
		}
		if (width) {
			dat = (dat << 8) + (*src);
			baseptr[LOW15(addr)] |= (UINT8)(((~dat) >> pt->sft) & pt->maskr);
		}
	}
}

static void putand(const PUTCNTX *pt) {

	UINT8	*baseptr;
	UINT	addr;
const UINT8	*src;
	UINT	width;
	UINT	dat;

	baseptr = pt->baseptr;
	addr = pt->addr;
	src = pt->pat;
	width = pt->width;
	dat = *src++;
	if ((pt->sft + width) < 8) {
		baseptr[LOW15(addr)] &= (UINT8)((dat >> pt->sft) | (~pt->masklr));
	}
	else {
		baseptr[LOW15(addr)] &= (UINT8)((dat >> pt->sft) | (~pt->maskl));
		addr++;
		width -= (8 - pt->sft);
		while(width > 8) {
			width -= 8;
			dat = (dat << 8) + (*src);
			src++;
			baseptr[LOW15(addr)] &= (UINT8)(dat >> pt->sft);
			addr++;
		}
		if (width) {
			dat = (dat << 8) + (*src);
			baseptr[LOW15(addr)] &= (UINT8)((dat >> pt->sft) | (~pt->maskr));
		}
	}
}

static void putandn(const PUTCNTX *pt) {

	UINT8	*baseptr;
	UINT	addr;
const UINT8	*src;
	UINT	width;
	UINT	dat;

	baseptr = pt->baseptr;
	addr = pt->addr;
	src = pt->pat;
	width = pt->width;
	dat = *src++;
	if ((pt->sft + width) < 8) {
		baseptr[LOW15(addr)] &= (UINT8)(~((dat >> pt->sft) & pt->masklr));
	}
	else {
		baseptr[LOW15(addr)] &= (UINT8)(~((dat >> pt->sft) & pt->maskl));
		addr++;
		width -= (8 - pt->sft);
		while(width > 8) {
			width -= 8;
			dat = (dat << 8) + (*src);
			src++;
			baseptr[LOW15(addr)] &= (UINT8)((~dat) >> pt->sft);
			addr++;
		}
		if (width) {
			dat = (dat << 8) + (*src);
			baseptr[LOW15(addr)] &= (UINT8)(~((dat >> pt->sft) & pt->maskr));
		}
	}
}

static void putxor(const PUTCNTX *pt) {

	UINT8	*baseptr;
	UINT	addr;
const UINT8	*src;
	UINT	width;
	UINT	dat;

	baseptr = pt->baseptr;
	addr = pt->addr;
	src = pt->pat;
	width = pt->width;
	dat = *src++;
	if ((pt->sft + width) < 8) {
		baseptr[LOW15(addr)] ^= (UINT8)((dat >> pt->sft) & pt->masklr);
	}
	else {
		baseptr[LOW15(addr)] ^= (UINT8)(dat >> pt->sft) & pt->maskl;
		addr++;
		width -= (8 - pt->sft);
		while(width > 8) {
			width -= 8;
			dat = (dat << 8) + (*src);
			src++;
			baseptr[LOW15(addr)] ^= (UINT8)(dat >> pt->sft);
			addr++;
		}
		if (width) {
			dat = (dat << 8) + (*src);
			baseptr[LOW15(addr)] ^= (UINT8)(dat >> pt->sft) & pt->maskr;
		}
	}
}

static void putxorn(const PUTCNTX *pt) {

	UINT8	*baseptr;
	UINT	addr;
const UINT8	*src;
	UINT	width;
	UINT	dat;

	baseptr = pt->baseptr;
	addr = pt->addr;
	src = pt->pat;
	width = pt->width;
	dat = *src++;
	if ((pt->sft + width) < 8) {
		baseptr[LOW15(addr)] ^= (UINT8)(((~dat) >> pt->sft) & pt->masklr);
	}
	else {
		baseptr[LOW15(addr)] ^= (UINT8)(((~dat) >> pt->sft) & pt->maskl);
		addr++;
		width -= (8 - pt->sft);
		while(width > 8) {
			width -= 8;
			dat = (dat << 8) + (*src);
			src++;
			baseptr[LOW15(addr)] ^= (UINT8)((~dat) >> pt->sft);
			addr++;
		}
		if (width) {
			dat = (dat << 8) + (*src);
			baseptr[LOW15(addr)] ^= (UINT8)(((~dat) >> pt->sft) & pt->maskr);
		}
	}
}


// ----

static REG8 putsub(GLIO lio, const LIOPUT *lput) {

	UINT	addr;
	PUTCNTX	pt;
	UINT	datacnt;
	UINT	off;
	UINT	height;
	UINT	flag;
	UINT	pl;
	UINT	writecnt;

	if ((lput->x < lio->draw.x1) ||
		(lput->y < lio->draw.y1) ||
		((lput->x + lput->width - 1) > lio->draw.x2) ||
		((lput->y + lput->height - 1) > lio->draw.y2)) {
		return(LIO_ILLEGALFUNC);
	}
	if ((lput->width <= 0) || (lput->height <= 0)) {
		return(LIO_SUCCESS);
	}

	addr = (lput->x >> 3) + (lput->y * 80);
	if (lio->draw.flag & LIODRAW_UPPER) {
		addr += 16000;
	}
	setdirty(addr, (lput->x & 7) + lput->width, lput->height, lio->draw.sbit);

	pt.addr = addr;
	pt.sft = lput->x & 7;
	pt.width = lput->width;
	pt.maskl = (UINT8)(0xff >> pt.sft);
	pt.maskr = (UINT8)((~0x7f) >> ((pt.width + pt.sft - 1) & 7));
	pt.masklr = (UINT8)(pt.maskl >> pt.sft);

	datacnt = (lput->width + 7) >> 3;
	off = lput->off;

	flag = (lio->draw.flag & LIODRAW_4BPP)?0x0f:0x07;
	flag |= (lput->fg & 15) << 4;
	flag |= (lput->bg & 15) << 8;

	// ‚³‚Ä•\Ž¦B
	writecnt = 0;
	height = lput->height;
	do {
		flag <<= 4;
		for (pl=0; pl<4; pl++) {
			flag >>= 1;
			if (flag & 8) {
				pt.baseptr = mem + lio->draw.base + lioplaneadrs[pl];
				MEMR_READS(lput->seg, off, pt.pat, datacnt);
				if (lput->sw) {
					off += datacnt;
				}
				switch(lput->mode) {
					case 0:		// PSET
						if (flag & (8 << 4)) {
							putor(&pt);
						}
						else {
							putandn(&pt);
						}
						if (flag & (8 << 8)) {
							putorn(&pt);
						}
						else {
							putand(&pt);
						}
						writecnt += 2;
						break;

					case 1:		// NOT
						if (!(flag & (8 << 4))) {
							putor(&pt);
						}
						else {
							putandn(&pt);
						}
						if (!(flag & (8 << 8))) {
							putorn(&pt);
						}
						else {
							putand(&pt);
						}
						writecnt += 2;
						break;

					case 2:		// OR
						if (flag & (8 << 4)) {
							putor(&pt);
							writecnt++;
						}
						if (flag & (8 << 8)) {
							putorn(&pt);
							writecnt++;
						}
						break;

					case 3:		// AND
						if (!(flag & (8 << 4))) {
							putandn(&pt);
							writecnt++;
						}
						if (!(flag & (8 << 8))) {
							putand(&pt);
							writecnt++;
						}
						break;

					case 4:		// XOR
						if (flag & (8 << 4)) {
							putxor(&pt);
							writecnt++;
						}
						if (flag & (8 << 8)) {
							putxorn(&pt);
							writecnt++;
						}
						break;
				}
			}
		}
		pt.addr += 80;
		if (!lput->sw) {
			off += datacnt;
		}
	} while(--height);
	lio->wait += writecnt * datacnt * (10 + 10 + 10);
	return(LIO_SUCCESS);
}


// ---- GGET

REG8 lio_gget(GLIO lio) {

	GGET	dat;
	SINT32	x;
	SINT32	y;
	int		x2;
	int		y2;
	UINT	off;
	UINT	seg;
	UINT32	leng;
	UINT32	size;
	UINT	datacnt;
	UINT	mask;
	GETCNTX	gt;
	UINT8	pat[84];
	UINT	pl;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	x = (SINT16)LOADINTELWORD(dat.x1);
	y = (SINT16)LOADINTELWORD(dat.y1);
	x2 = (SINT16)LOADINTELWORD(dat.x2);
	y2 = (SINT16)LOADINTELWORD(dat.y2);
	if ((x < lio->draw.x1) || (y < lio->draw.y1) ||
		(x2 > lio->draw.x2) || (y2 > lio->draw.y2)) {
		return(LIO_ILLEGALFUNC);
	}
	x2 = x2 - x + 1;
	y2 = y2 - y + 1;
	if ((x2 <= 0) || (y2 <= 0)) {
		return(LIO_ILLEGALFUNC);
	}
	off = LOADINTELWORD(dat.off);
	seg = (SINT16)LOADINTELWORD(dat.seg);

	datacnt = (x2 + 7) >> 3;
	size = datacnt * y2;
	leng = LOADINTELWORD(dat.leng);
	if (!(lio->draw.flag & LIODRAW_MONO)) {
		if (lio->draw.flag & LIODRAW_4BPP) {
			size *= 4;
			mask = 0x0f;
		}
		else {
			size *= 3;
			mask = 0x07;
		}
	}
	else {
		mask = 1 << (lio->draw.flag & LIODRAW_PMASK);
	}
	if (leng < (size + 4)) {
		return(LIO_ILLEGALFUNC);
	}
	MEMR_WRITE16(seg, off, (REG16)x2);
	MEMR_WRITE16(seg, off+2, (REG16)y2);
	off += 4;
	gt.addr = (x >> 3) + (y * 80);
	if (lio->draw.flag & LIODRAW_UPPER) {
		gt.addr += 16000;
	}
	gt.sft = x & 7;
	gt.width = x2;
	gt.mask = (UINT8)((~0x7f) >> ((x2 - 1) & 7));
	do {
		mask <<= 4;
		for (pl=0; pl<4; pl++) {
			mask >>= 1;
			if (mask & 8) {
				gt.baseptr = mem + lio->draw.base + lioplaneadrs[pl];
				getvram(&gt, pat);
				MEMR_WRITES(seg, off, pat, datacnt);
				off += datacnt;
			}
		}
		gt.addr += 80;
	} while(--y2);
	lio->wait = size * 12;
	return(LIO_SUCCESS);
}


// ---- GPUT1

REG8 lio_gput1(GLIO lio) {

	GPUT1	dat;
	LIOPUT	lput;
	UINT	leng;
	UINT	size;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	lput.x = (SINT16)LOADINTELWORD(dat.x);
	lput.y = (SINT16)LOADINTELWORD(dat.y);
	lput.off = (UINT16)(LOADINTELWORD(dat.off) + 4);
	lput.seg = LOADINTELWORD(dat.seg);
	lput.mode = dat.mode;
	leng = LOADINTELWORD(dat.leng);
	lput.width = MEMR_READ16(lput.seg, lput.off - 4);
	lput.height = MEMR_READ16(lput.seg, lput.off - 2);
	size = ((lput.width + 7) >> 3) * lput.height;
	if (leng < (size + 4)) {
		return(LIO_ILLEGALFUNC);
	}
	if (leng < ((size * 3) + 4)) {
		lput.sw = 0;
		if (dat.colorsw) {
			lput.fg = dat.fg;
			lput.bg = dat.bg;
		}
		else {
			lput.fg = lio->work.fgcolor;
			lput.bg = lio->work.bgcolor;
		}
	}
	else {
		if (dat.colorsw) {
			lput.sw = 0;
			lput.fg = dat.fg;
			lput.bg = dat.bg;
		}
		else {
			lput.sw = 1;
			lput.fg = 0x0f;
			lput.bg = 0;
		}
	}
	return(putsub(lio, &lput));
}


// ---- GPUT2

REG8 lio_gput2(GLIO lio) {

	GPUT2	dat;
	LIOPUT	lput;
	UINT16	jis;
	int		pat;
	REG16	size;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	lput.x = (SINT16)LOADINTELWORD(dat.x);
	lput.y = (SINT16)LOADINTELWORD(dat.y);
	lput.off = 0x104e;
	lput.seg = CPU_DS;
	jis = LOADINTELWORD(dat.chr);
	pat = 0;
	if (jis < 0x200) {
		if (jis < 0x80) {
			if (jis == 0x7c) {
				pat = 1;
			}
			else if (jis == 0x7e) {
				pat = 2;
			}
			else {
				jis += 0x2900;
			}
		}
		else if (jis < 0x100) {
			if ((jis - 0x20) & 0x40) {
				pat = (jis & 0x3f) + 3;
			}
			else {
				jis += 0x2980;
			}
		}
		else {
			jis &= 0xff;
		}
	}
	if (!pat) {
		size = bios0x18_14(lput.seg, 0x104c, jis);
	}
	else {
		MEMR_WRITES(lput.seg, lput.off, mem + (LIO_SEGMENT << 4) +
										LIO_FONT + ((pat - 1) << 4), 0x10);
		size = 0x0102;
	}
	lput.width = (size & 0xff00) >> (8 - 3);
	lput.height = (size & 0xff) << 3;
	lput.mode = dat.mode;
	lput.sw = 0;
	if (dat.colorsw) {
		lput.fg = dat.fg;
		lput.bg = dat.bg;
	}
	else {
		lput.fg = lio->work.fgcolor;
		lput.bg = lio->work.bgcolor;
	}
	return(putsub(lio, &lput));
}

