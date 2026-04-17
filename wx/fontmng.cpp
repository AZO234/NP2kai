/* === font management for wx port (SDL3_ttf) === */

#include <compiler.h>
#include "fontmng.h"
#include <dosio.h>

#include <SDL3_ttf/SDL_ttf.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	TTF_Font *ttf;
	int       size;
	/* static pixel buffer for fontmng_get() */
	UINT8    *pixbuf;
	int       pixbuf_w;
	int       pixbuf_h;
	int       pixbuf_pitch;
	_FNTDAT   dat;
} FNTMNG;

static char  s_deffont[MAX_PATH] = "";
static bool  s_ttf_inited = false;

static const char *s_candidates[] = {
	"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
	"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
	"/usr/share/fonts/opentype/ipafont-gothic/ipagp.ttf",
	"/usr/share/fonts/truetype/freefont/FreeSans.ttf",
	NULL
};

BRESULT fontmng_init(void)
{
	if (!s_ttf_inited) {
		if (!TTF_Init()) return FAILURE;
		s_ttf_inited = true;
	}
	if (s_deffont[0] == '\0') {
		for (int i = 0; s_candidates[i]; i++) {
			if (access(s_candidates[i], R_OK) == 0) {
				milstr_ncpy(s_deffont, s_candidates[i], MAX_PATH);
				break;
			}
		}
	}
	return SUCCESS;
}

void fontmng_term(void)
{
	if (s_ttf_inited) {
		TTF_Quit();
		s_ttf_inited = false;
	}
}

void fontmng_setdeffontname(const char *name)
{
	if (name) milstr_ncpy(s_deffont, name, MAX_PATH);
}

void *fontmng_create(int size, UINT type, const char *fontface)
{
	const char *path = (fontface && fontface[0]) ? fontface : s_deffont;
	if (!path || path[0] == '\0') return NULL;
	TTF_Font *ttf = TTF_OpenFont(path, size);
	if (!ttf) return NULL;
	FNTMNG *fnt = (FNTMNG *)calloc(1, sizeof(FNTMNG));
	if (!fnt) { TTF_CloseFont(ttf); return NULL; }
	fnt->ttf  = ttf;
	fnt->size = size;
	if (type & FDAT_BOLD) TTF_SetFontStyle(ttf, TTF_STYLE_BOLD);
	return fnt;
}

void fontmng_destroy(void *hdl)
{
	if (!hdl) return;
	FNTMNG *fnt = (FNTMNG *)hdl;
	if (fnt->ttf)    TTF_CloseFont(fnt->ttf);
	if (fnt->pixbuf) free(fnt->pixbuf);
	free(fnt);
}

BRESULT fontmng_getsize(void *hdl, const char *string, POINT_T *pt)
{
	if (!hdl || !string || !pt) return FAILURE;
	FNTMNG *fnt = (FNTMNG *)hdl;
	int w = 0, h = 0;
	if (!TTF_GetStringSize(fnt->ttf, string, 0, &w, &h)) return FAILURE;
	pt->x = w;
	pt->y = h;
	return SUCCESS;
}

BRESULT fontmng_getdrawsize(void *hdl, const char *string, POINT_T *pt)
{
	return fontmng_getsize(hdl, string, pt);
}

FNTDAT fontmng_get(void *hdl, const char *string)
{
	if (!hdl || !string) return NULL;
	FNTMNG *fnt = (FNTMNG *)hdl;

	SDL_Color white = {255, 255, 255, 255};
	SDL_Surface *surf = TTF_RenderText_Blended(fnt->ttf, string, 0, white);
	if (!surf) return NULL;

	int w     = surf->w;
	int h     = surf->h;
	int pitch = (w + 7) / 8;

	if (!fnt->pixbuf || fnt->pixbuf_w != w || fnt->pixbuf_h != h) {
		free(fnt->pixbuf);
		fnt->pixbuf = (UINT8 *)calloc(1, (size_t)(pitch * h));
		fnt->pixbuf_w     = w;
		fnt->pixbuf_h     = h;
		fnt->pixbuf_pitch = pitch;
	} else {
		memset(fnt->pixbuf, 0, (size_t)(pitch * h));
	}

	/* Convert ARGB surface to 1bpp mask */
	UINT32 *pixels = (UINT32 *)surf->pixels;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			UINT8 a = (UINT8)(pixels[y * (surf->pitch / 4) + x] >> 24);
			if (a > 128)
				fnt->pixbuf[y * pitch + (x / 8)] |= (UINT8)(0x80 >> (x & 7));
		}
	}
	SDL_DestroySurface(surf);

	fnt->dat.width  = w;
	fnt->dat.height = h;
	fnt->dat.pitch  = pitch;
	return &fnt->dat;
}
