#include	"compiler.h"
// #include	<sys/time.h>
// #include	<signal.h>
// #include	<unistd.h>
#include	"mousemng.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"vramhdl.h"
#include	"menubase.h"

static SDL_Window *s_sdlWindow;
static SDL_Renderer *s_renderer;
static SDL_Texture *s_texture;
static SDL_Surface *s_surface;

typedef struct {
	BOOL		enable;
	int			width;
	int			height;
	int			bpp;
	SDL_Surface	*surface;
	VRAMHDL		vram;
} SCRNMNG;

typedef struct {
	int		width;
	int		height;
} SCRNSTAT;

static const char app_name[] = "Neko Project II";

static	SCRNMNG		scrnmng;
static	SCRNSTAT	scrnstat;
static	SCRNSURF	scrnsurf;

typedef struct {
	int		xalign;
	int		yalign;
	int		width;
	int		height;
	int		srcpos;
	int		dstpos;
} DRAWRECT;

static BRESULT calcdrawrect(SDL_Surface *surface,
								DRAWRECT *dr, VRAMHDL s, const RECT_T *rt) {

	int		pos;

	dr->xalign = surface->format->BytesPerPixel;
	dr->yalign = surface->pitch;
	dr->srcpos = 0;
	dr->dstpos = 0;
	dr->width = max(scrnmng.width, s->width);
	dr->height = max(scrnmng.height, s->height);
	if (rt) {
		pos = max(rt->left, 0);
		dr->srcpos += pos;
		dr->dstpos += pos * dr->xalign;
		dr->width = min(rt->right, dr->width) - pos;

		pos = max(rt->top, 0);
		dr->srcpos += pos * s->width;
		dr->dstpos += pos * dr->yalign;
		dr->height = min(rt->bottom, dr->height) - pos;
	}
	if ((dr->width <= 0) || (dr->height <= 0)) {
		return(FAILURE);
	}
	return(SUCCESS);
}


void scrnmng_initialize(void) {

	scrnstat.width = 640;
	scrnstat.height = 400;
}

BRESULT scrnmng_create(int width, int height) {

	SDL_Surface		*surface;
	SDL_PixelFormat	*fmt;
	BOOL			r;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
		return(FAILURE);
	}
	s_sdlWindow = SDL_CreateWindow(app_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
	s_renderer = SDL_CreateRenderer(s_sdlWindow, -1, 0);
	SDL_RenderSetLogicalSize(s_renderer, width, 400);
	s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, width, 400);
	s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, 400, 16, 0xf800, 0x07e0, 0x001f, 0);

	surface = s_surface;
	r = FALSE;
	fmt = surface->format;
#if defined(SUPPORT_8BPP)
	if (fmt->BitsPerPixel == 8) {
		r = TRUE;
	}
#endif
#if defined(SUPPORT_16BPP)
	if ((fmt->BitsPerPixel == 16) && (fmt->Rmask == 0xf800) &&
		(fmt->Gmask == 0x07e0) && (fmt->Bmask == 0x001f)) {
		r = TRUE;
	}
#endif
#if defined(SUPPORT_24BPP)
	if (fmt->BitsPerPixel == 24) {
		r = TRUE;
	}
#endif
#if defined(SUPPORT_32BPP)
	if (fmt->BitsPerPixel == 32) {
		r = TRUE;
	}
#endif
#if defined(SCREEN_BPP)
	if (fmt->BitsPerPixel != SCREEN_BPP) {
		r = FALSE;
	}
#endif
	if (r) {
		scrnmng.enable = TRUE;
		scrnmng.width = width;
		scrnmng.height = height;
		scrnmng.bpp = fmt->BitsPerPixel;
		return(SUCCESS);
	}
	else {
		fprintf(stderr, "Error: Bad screen mode");
		return(FAILURE);
	}
}

void scrnmng_destroy(void) {

	scrnmng.enable = FALSE;
}

RGB16 scrnmng_makepal16(RGB32 pal32) {

	RGB16	ret;

	ret = (pal32.p.r & 0xf8) << 8;
#if defined(SIZE_QVGA)
	ret += (pal32.p.g & 0xfc) << (3 + 16);
#else
	ret += (pal32.p.g & 0xfc) << 3;
#endif
	ret += pal32.p.b >> 3;
	return(ret);
}

void scrnmng_setwidth(int posx, int width) {

	SDL_FreeSurface(s_surface);
	SDL_DestroyTexture(s_texture);
	SDL_RenderSetLogicalSize(s_renderer, width, scrnmng.height);
	s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, width, scrnmng.height);
	s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, scrnmng.height, 16, 0xf800, 0x07e0, 0x001f, 0);
	scrnstat.width = width;
}

void scrnmng_setheight(int posy, int height) {

	SDL_FreeSurface(s_surface);
	SDL_DestroyTexture(s_texture);
	SDL_RenderSetLogicalSize(s_renderer, scrnstat.width, height);
	s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, scrnstat.width, height);
	s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnstat.width, height, 16, 0xf800, 0x07e0, 0x001f, 0);
	scrnstat.height = height;
}

const SCRNSURF *scrnmng_surflock(void) {

	SDL_Surface	*surface;

	if (scrnmng.vram == NULL) {
		surface = s_surface;
		if (surface == NULL) {
			return(NULL);
		}
		SDL_LockSurface(surface);
		scrnmng.surface = surface;
		scrnsurf.ptr = (UINT8 *)surface->pixels;
		scrnsurf.xalign = surface->format->BytesPerPixel;
		scrnsurf.yalign = surface->pitch;
		scrnsurf.bpp = surface->format->BitsPerPixel;
	}
	else {
		scrnsurf.ptr = scrnmng.vram->ptr;
		scrnsurf.xalign = scrnmng.vram->xalign;
		scrnsurf.yalign = scrnmng.vram->yalign;
		scrnsurf.bpp = scrnmng.vram->bpp;
	}
	scrnsurf.width = max(scrnstat.width, 640);
	scrnsurf.height = max(scrnstat.height, 400);
	scrnsurf.extend = 0;
	return(&scrnsurf);
}

static void draw_onmenu(void) {

	RECT_T		rt;
	SDL_Surface	*surface;
	DRAWRECT	dr;
const UINT8		*p;
	UINT8		*q;
const UINT8		*a;
	int			salign;
	int			dalign;
	int			x;

	rt.left = 0;
	rt.top = 0;
	rt.right = min(scrnstat.width, 640);
	rt.bottom = min(scrnstat.height, 400);
#if defined(SIZE_QVGA)
	rt.right >>= 1;
	rt.bottom >>= 1;
#endif

	surface = s_surface;
	if (surface == NULL) {
		return;
	}
	mousemng_hidecursor();
	SDL_LockSurface(surface);
	if (calcdrawrect(surface, &dr, menuvram, &rt) == SUCCESS) {
		switch(scrnmng.bpp) {
#if defined(SUPPORT_16BPP)
			case 16:
				p = scrnmng.vram->ptr + (dr.srcpos * 2);
				q = (UINT8 *)surface->pixels + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x] == 0) {
							*(UINT16 *)q = *(UINT16 *)(p + (x * 2));
						}
						q += dr.xalign;
					} while(++x < dr.width);
					p += salign * 2;
					q += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
#if defined(SUPPORT_24BPP)
			case 24:
				p = scrnmng.vram->ptr + (dr.srcpos * 3);
				q = (UINT8 *)surface->pixels + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x] == 0) {
							q[0] = p[x*3+0];
							q[1] = p[x*3+1];
							q[2] = p[x*3+2];
						}
						q += dr.xalign;
					} while(++x < dr.width);
					p += salign * 3;
					q += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
#if defined(SUPPORT_32BPP)
			case 32:
				p = scrnmng.vram->ptr + (dr.srcpos * 4);
				q = (UINT8 *)surface->pixels + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x] == 0) {
							*(UINT32 *)q = *(UINT32 *)(p + (x * 4));
						}
						q += dr.xalign;
					} while(++x < dr.width);
					p += salign * 4;
					q += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
		}
	}
	SDL_UnlockSurface(surface);
	mousemng_showcursor();

	SDL_UpdateTexture(s_texture, NULL, surface->pixels, surface->pitch);
	SDL_RenderClear(s_renderer);
	SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
	SDL_RenderPresent(s_renderer);
}

void scrnmng_surfunlock(const SCRNSURF *surf) {

	SDL_Surface	*surface;

	if (surf) {
		if (scrnmng.vram == NULL) {
			if (scrnmng.surface != NULL) {
				surface = scrnmng.surface;
				scrnmng.surface = NULL;
				SDL_UnlockSurface(surface);

				SDL_UpdateTexture(s_texture, NULL, surface->pixels, surface->pitch);
				SDL_RenderClear(s_renderer);
				SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
				SDL_RenderPresent(s_renderer);
			}
		}
		else {
			if (menuvram) {
				draw_onmenu();
			}
		}
	}
}


// ----

BRESULT scrnmng_entermenu(SCRNMENU *smenu) {

	if (smenu == NULL) {
		goto smem_err;
	}
	vram_destroy(scrnmng.vram);
	scrnmng.vram = vram_create(scrnmng.width, scrnmng.height, FALSE,
																scrnmng.bpp);
	if (scrnmng.vram == NULL) {
		goto smem_err;
	}
	scrndraw_redraw();
	smenu->width = scrnmng.width;
	smenu->height = scrnmng.height;
	smenu->bpp = (scrnmng.bpp == 32)?24:scrnmng.bpp;
	mousemng_showcursor();
	return(SUCCESS);

smem_err:
	return(FAILURE);
}

void scrnmng_leavemenu(void) {

	VRAM_RELEASE(scrnmng.vram);
	mousemng_hidecursor();
}

void scrnmng_updatecursor(void) {
	SDL_Surface	*surface;
	surface = s_surface;
	if (surface == NULL) {
		return;
	}

	SDL_UpdateTexture(s_texture, NULL, surface->pixels, surface->pitch);
	SDL_RenderClear(s_renderer);
	SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
	SDL_RenderPresent(s_renderer);
}

void scrnmng_menudraw(const RECT_T *rct) {

	SDL_Surface	*surface;
	DRAWRECT	dr;
const UINT8		*p;
const UINT8		*q;
	UINT8		*r;
	UINT8		*a;
	int			salign;
	int			dalign;
	int			x;

	if ((!scrnmng.enable) && (menuvram == NULL)) {
		return;
	}
	surface = s_surface;
	if (surface == NULL) {
		return;
	}
	mousemng_hidecursor();
	SDL_LockSurface(surface);
	if (calcdrawrect(surface, &dr, menuvram, rct) == SUCCESS) {
		switch(scrnmng.bpp) {
#if defined(SUPPORT_16BPP)
			case 16:
				p = scrnmng.vram->ptr + (dr.srcpos * 2);
				q = menuvram->ptr + (dr.srcpos * 2);
				r = (UINT8 *)surface->pixels + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x]) {
							if (a[x] & 2) {
								*(UINT16 *)r = *(UINT16 *)(q + (x * 2));
							}
							else {
								a[x] = 0;
								*(UINT16 *)r = *(UINT16 *)(p + (x * 2));
							}
						}
						r += dr.xalign;
					} while(++x < dr.width);
					p += salign * 2;
					q += salign * 2;
					r += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
#if defined(SUPPORT_24BPP)
			case 24:
				p = scrnmng.vram->ptr + (dr.srcpos * 3);
				q = menuvram->ptr + (dr.srcpos * 3);
				r = (UINT8 *)surface->pixels + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x]) {
							if (a[x] & 2) {
								r[RGB24_B] = q[x*3+0];
								r[RGB24_G] = q[x*3+1];
								r[RGB24_R] = q[x*3+2];
							}
							else {
								a[x] = 0;
								r[0] = p[x*3+0];
								r[1] = p[x*3+1];
								r[2] = p[x*3+2];
							}
						}
						r += dr.xalign;
					} while(++x < dr.width);
					p += salign * 3;
					q += salign * 3;
					r += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
#if defined(SUPPORT_32BPP)
			case 32:
				p = scrnmng.vram->ptr + (dr.srcpos * 4);
				q = menuvram->ptr + (dr.srcpos * 3);
				r = (UINT8 *)surface->pixels + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x]) {
							if (a[x] & 2) {
								((RGB32 *)r)->p.b = q[x*3+0];
								((RGB32 *)r)->p.g = q[x*3+1];
								((RGB32 *)r)->p.r = q[x*3+2];
							//	((RGB32 *)r)->p.e = 0;
							}
							else {
								a[x] = 0;
								*(UINT32 *)r = *(UINT32 *)(p + (x * 4));
							}
						}
						r += dr.xalign;
					} while(++x < dr.width);
					p += salign * 4;
					q += salign * 3;
					r += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
		}
	}
	SDL_UnlockSurface(surface);
	mousemng_showcursor();

	SDL_UpdateTexture(s_texture, NULL, surface->pixels, surface->pitch);
	SDL_RenderClear(s_renderer);
	SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
	SDL_RenderPresent(s_renderer);
}

