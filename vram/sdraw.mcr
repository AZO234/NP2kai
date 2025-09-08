
// ---- plasma display

// vram off
static void SCRNCALL SDSYM(p_0)(SDRAW sdraw, int maxy) {

	UINT8	*p;
	int		y;
	int		x;

	p = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(p, NP2PAL_TEXT2);
				p += sdraw->xalign;
			}
			p -= sdraw->xbytes;
		}
		p += sdraw->yalign;
	} while(++y < maxy);

	sdraw->dst = p;
	sdraw->y = y;
}

// text or grph 1プレーン
static void SCRNCALL SDSYM(p_1)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph
static void SCRNCALL SDSYM(p_2)(SDRAW sdraw, int maxy) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

// text + (grph:interleave)
static void SCRNCALL SDSYM(p_ti)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, (p[x] >> 4) + NP2PAL_TEXT);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// grph:interleave
static void SCRNCALL SDSYM(p_gi)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, NP2PAL_TEXT);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph:interleave
static void SCRNCALL SDSYM(p_2i)(SDRAW sdraw, int maxy) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		q += SURFACE_WIDTH;
		r += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(r, (q[x] >> 4) + NP2PAL_TEXT);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		p += (SURFACE_WIDTH * 2);
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

//	grph:interleave ex
static void SCRNCALL SDSYM(p_gie)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			sdraw->dirty[y+1] |= 0xff;
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_SKIP);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += (SURFACE_WIDTH * 2);
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

//	text + grph:interleave ex
static void SCRNCALL SDSYM(p_2ie)(SDRAW sdraw, int maxy) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;
	UINT8	c;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			sdraw->dirty[y+1] |= 0xff;
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		q += SURFACE_WIDTH;
		r += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				c = q[x] >> 4;
				if (!c) {
					c = p[x] + NP2PALS_TXT;
				}
				SDSETPIXEL(r, c + NP2PAL_TEXT);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		p += (SURFACE_WIDTH * 2);
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

#if defined(SUPPORT_CRT15KHZ)
// text or grph 1プレーン(15kHz)
static void SCRNCALL SDSYM(p_1d)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		a;
	int		y;
	int		x;
	int		c;

	p = sdraw->src;
	q = sdraw->dst;
	a = sdraw->yalign;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				c = p[x] + NP2PAL_GRPH;
				SDSETPIXEL(q, c);
				SDSETPIXEL((q + a), c);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += a * 2;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph (15kHz)
static void SCRNCALL SDSYM(p_2d)(SDRAW sdraw, int maxy) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		a;
	int		y;
	int		x;
	int		c;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	a = sdraw->yalign;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				c = p[x] + q[x] + NP2PAL_GRPH;
				SDSETPIXEL(r, c);
				SDSETPIXEL((r + a), c);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += a * 2;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}
#endif

static const SDRAWFN SDSYM(p)[] = {
		SDSYM(p_0),		SDSYM(p_1),		SDSYM(p_1),		SDSYM(p_2),
		SDSYM(p_0),		SDSYM(p_ti),	SDSYM(p_gi),	SDSYM(p_2i),
		SDSYM(p_0),		SDSYM(p_ti),	SDSYM(p_gie),	SDSYM(p_2ie),
#if defined(SUPPORT_CRT15KHZ)
		SDSYM(p_0),		SDSYM(p_1d),	SDSYM(p_1d),	SDSYM(p_2d),
#endif
	};

// ---- normal display

#ifdef SUPPORT_NORMALDISP

// vram off
static void SCRNCALL SDSYM(n_0)(SDRAW sdraw, int maxy) {

	UINT8	*p;
	int		y;
	int		x;

	p = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			SDSETPIXEL(p, NP2PAL_TEXT3);
			for (x=0; x<sdraw->width; x++) {
				p += sdraw->xalign;
				SDSETPIXEL(p, NP2PAL_TEXT2);
			}
			p -= sdraw->xbytes;
		}
		p += sdraw->yalign;
	} while(++y < maxy);

	sdraw->dst = p;
	sdraw->y = y;
}

// text 1プレーン
static void SCRNCALL SDSYM(n_t)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			SDSETPIXEL(q, (p[0] >> 4) + NP2PAL_TEXT3);
			q += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			SDSETPIXEL(q, NP2PAL_TEXT2);
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// grph 1プレーン
static void SCRNCALL SDSYM(n_g)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			SDSETPIXEL(q, NP2PAL_TEXT3);
			for (x=0; x<sdraw->width; x++) {
				q += sdraw->xalign;
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph
static void SCRNCALL SDSYM(n_2)(SDRAW sdraw, int maxy) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			SDSETPIXEL(r, (q[0] >> 4) + NP2PAL_TEXT3);
			r += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x-1] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			SDSETPIXEL(r, p[x-1] + NP2PAL_GRPH);
			r -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

// text + (grph:interleave)
static void SCRNCALL SDSYM(n_ti)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			SDSETPIXEL(q, (p[0] >> 4) + NP2PAL_TEXT3);
			q += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			SDSETPIXEL(q, NP2PAL_GRPH);
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			SDSETPIXEL(q, (p[0] >> 4) + NP2PAL_TEXT3);
			q += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				SDSETPIXEL(q, (p[x] >> 4) + NP2PAL_TEXT);
				q += sdraw->xalign;
			}
			SDSETPIXEL(q, NP2PAL_TEXT);
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// grph:interleave
static void SCRNCALL SDSYM(n_gi)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			SDSETPIXEL(q, NP2PAL_TEXT3);
			for (x=0; x<sdraw->width; x++) {
				q += sdraw->xalign;
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			SDSETPIXEL(q, NP2PAL_TEXT3);
			for (x=0; x<sdraw->width; x++) {
				q += sdraw->xalign;
				SDSETPIXEL(q, NP2PAL_TEXT);
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph:interleave
static void SCRNCALL SDSYM(n_2i)(SDRAW sdraw, int maxy) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			SDSETPIXEL(r, (q[0] >> 4) + NP2PAL_TEXT3);
			r += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x-1] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			SDSETPIXEL(r, p[x-1] + NP2PAL_GRPH);
			r -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			SDSETPIXEL(r, (q[0] >> 4) + NP2PAL_TEXT3);
			r += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				SDSETPIXEL(r, (q[x] >> 4) + NP2PAL_TEXT);
				r += sdraw->xalign;
			}
			SDSETPIXEL(r, NP2PAL_TEXT);
			r -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

//	grph:interleave ex
static void SCRNCALL SDSYM(n_gie)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			sdraw->dirty[y+1] |= 0xff;
			SDSETPIXEL(q, NP2PAL_TEXT3);
			for (x=0; x<sdraw->width; x++) {
				q += sdraw->xalign;
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
			}
			q -= sdraw->xbytes;
		}
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			SDSETPIXEL(q, NP2PAL_TEXT3);
			for (x=0; x<sdraw->width; x++) {
				q += sdraw->xalign;
				SDSETPIXEL(q, p[x] + NP2PAL_SKIP);
			}
			q -= sdraw->xbytes;
		}
		p += (SURFACE_WIDTH * 2);
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

//	text + grph:interleave ex
static void SCRNCALL SDSYM(n_2ie)(SDRAW sdraw, int maxy) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;
	UINT8	c;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			sdraw->dirty[y+1] |= 0xff;
			SDSETPIXEL(r, (q[0] >> 4) + NP2PAL_TEXT3);
			r += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x-1] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			SDSETPIXEL(r, p[x-1] + NP2PAL_GRPH);
			r -= sdraw->xbytes;
		}
		q += SURFACE_WIDTH;
		r += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			SDSETPIXEL(r, (q[0] >> 4) + NP2PAL_TEXT3);
			r += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				c = q[x] >> 4;
				if (!c) {
					c = p[x-1] + NP2PALS_TXT;
				}
				SDSETPIXEL(r, c + NP2PAL_TEXT);
				r += sdraw->xalign;
			}
			SDSETPIXEL(r, p[x-1] + NP2PAL_SKIP);
			r -= sdraw->xbytes;
		}
		p += (SURFACE_WIDTH * 2);
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

#if defined(SUPPORT_CRT15KHZ)
// text 1プレーン (15kHz)
static void SCRNCALL SDSYM(n_td)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		a;
	int		y;
	int		x;
	int		c;

	p = sdraw->src;
	q = sdraw->dst;
	a = sdraw->yalign;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			c = (p[0] >> 4) + NP2PAL_TEXT3;
			SDSETPIXEL(q, c);
			SDSETPIXEL((q + a), c);
			q += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				c = p[x] + NP2PAL_GRPH;
				SDSETPIXEL(q, c);
				SDSETPIXEL((q + a), c);
				q += sdraw->xalign;
			}
			SDSETPIXEL(q, NP2PAL_TEXT2);
			SDSETPIXEL((q + a), NP2PAL_TEXT2);
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += a * 2;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// grph 1プレーン (15kHz)
static void SCRNCALL SDSYM(n_gd)(SDRAW sdraw, int maxy) {

const UINT8	*p;
	UINT8	*q;
	int		a;
	int		y;
	int		x;
	int		c;

	p = sdraw->src;
	q = sdraw->dst;
	a = sdraw->yalign;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			SDSETPIXEL(q, NP2PAL_TEXT3);
			SDSETPIXEL((q + a), NP2PAL_TEXT3);
			for (x=0; x<sdraw->width; x++) {
				q += sdraw->xalign;
				c = p[x] + NP2PAL_GRPH;
				SDSETPIXEL(q, c);
				SDSETPIXEL((q + a), c);
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += a * 2;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph (15kHz)
static void SCRNCALL SDSYM(n_2d)(SDRAW sdraw, int maxy) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		a;
	int		y;
	int		x;
	int		c;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	a = sdraw->yalign;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			c = (q[0] >> 4) + NP2PAL_TEXT3;
			SDSETPIXEL(r, c);
			SDSETPIXEL((r + a), c);
			r += sdraw->xalign;
			for (x=1; x<sdraw->width; x++) {
				c = p[x-1] + q[x] + NP2PAL_GRPH;
				SDSETPIXEL(r, c);
				SDSETPIXEL((r + a), c);
				r += sdraw->xalign;
			}
			c = p[x-1] + NP2PAL_GRPH;
			SDSETPIXEL(r, c);
			SDSETPIXEL((r + a), c);
			r -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += a * 2;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}
#endif

static const SDRAWFN SDSYM(n)[] = {
		SDSYM(n_0),		SDSYM(n_t),		SDSYM(n_g),		SDSYM(n_2),
		SDSYM(n_0),		SDSYM(n_ti),	SDSYM(n_gi),	SDSYM(n_2i),
		SDSYM(n_0),		SDSYM(n_ti),	SDSYM(n_gie),	SDSYM(n_2ie),
#if defined(SUPPORT_CRT15KHZ)
		SDSYM(n_0),		SDSYM(n_td),	SDSYM(n_gd),	SDSYM(n_2d),
#endif
	};
#endif

