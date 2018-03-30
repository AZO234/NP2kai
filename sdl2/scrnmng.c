#include	"compiler.h"
// #include	<sys/time.h>
// #include	<signal.h>
// #include	<unistd.h>
#include	"mousemng.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"commng.h"

#if defined(__LIBRETRO__)
#include "libretro.h"
#include "libretro_exports.h"

extern retro_environment_t environ_cb;

SCRNSURF	scrnsurf;
static	VRAMHDL		vram;
#else	/* __LIBRETRO__ */
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

static const char app_name[] = "Neko Project II kai";

static	SCRNMNG		scrnmng;
static	SCRNSTAT	scrnstat;
static	SCRNSURF	scrnsurf;
#endif	/* __LIBRETRO__ */

typedef struct {
	int		xalign;
	int		yalign;
	int		width;
	int		height;
	int		srcpos;
	int		dstpos;
} DRAWRECT;

extern char temp_bppswitch;

#if defined(__LIBRETRO__)
static BRESULT calcdrawrect(DRAWRECT *dr, const RECT_T *rt) {
#else	/* __LIBRETRO__ */
static BRESULT calcdrawrect(SDL_Surface *surface,
								DRAWRECT *dr, VRAMHDL s, const RECT_T *rt) {
#endif	/* __LIBRETRO__ */

	int		pos;

#if defined(__LIBRETRO__)
	if(temp_bppswitch) {
		dr->xalign = 4;
		dr->yalign = scrnsurf.width*4;
	} else {
		dr->xalign = 2;
		dr->yalign = scrnsurf.width*2;
	}
#else	/* __LIBRETRO__ */
	dr->xalign = surface->format->BytesPerPixel;
	dr->yalign = surface->pitch;
#endif	/* __LIBRETRO__ */
	dr->srcpos = 0;
	dr->dstpos = 0;
#if defined(__LIBRETRO__)
	dr->width = scrnsurf.width;
	dr->height = scrnsurf.height;
#else	/* __LIBRETRO__ */
	dr->width = np2max(scrnmng.width, s->width);
	dr->height = np2max(scrnmng.height, s->height);
#endif	/* __LIBRETRO__ */
	if (rt) {
		pos = np2max(rt->left, 0);
		dr->srcpos += pos;
		dr->dstpos += pos * dr->xalign;
		dr->width = np2min(rt->right, dr->width) - pos;

		pos = np2max(rt->top, 0);
#if defined(__LIBRETRO__)
		dr->srcpos += pos * scrnsurf.width;
#else	/* __LIBRETRO__ */
		dr->srcpos += pos * s->width;
#endif	/* __LIBRETRO__ */
		dr->dstpos += pos * dr->yalign;
		dr->height = np2min(rt->bottom, dr->height) - pos;
	}
	if ((dr->width <= 0) || (dr->height <= 0)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

#if defined(__LIBRETRO__)
void draw(DRAWRECT dr){

	const UINT8		*p;
	const UINT8		*q;
	UINT8		*r;
	UINT8		*a;
	int			salign;
	int			dalign;
	int			x;

	if(temp_bppswitch) {
		p = vram->ptr + (dr.srcpos * 4);
	} else {
		p = vram->ptr + (dr.srcpos * 2);
	}
	q = (BYTE *)GuiBuffer + dr.dstpos;
	a = menuvram->alpha + dr.srcpos;
	salign = menuvram->width;
	dalign = dr.yalign - (dr.width * dr.xalign);
	do {
		x = 0;
		do {
			if (a[x] == 0) {
				if(temp_bppswitch) {
					*(UINT32 *)q = *(UINT32 *)(p + (x * 4));
				} else {
					*(UINT16 *)q = *(UINT16 *)(p + (x * 2));
				}
			}
			q += dr.xalign;
		} while(++x < dr.width);
		if(temp_bppswitch) {
			p += salign * 4;
		} else {
			p += salign * 2;
		}
		q += dalign;
		a += salign;
	} while(--dr.height);
}

void draw2(DRAWRECT dr){

	const UINT8		*p;
	const UINT8		*q;
	UINT8		*r;
	UINT8		*a;
	int			salign;
	int			dalign;
	int			x;

	if(temp_bppswitch) {
		p = vram->ptr + (dr.srcpos * 4);
		q = menuvram->ptr + (dr.srcpos * 4);
	} else {
		p = vram->ptr + (dr.srcpos * 2);
		q = menuvram->ptr + (dr.srcpos * 2);
	}
	r = (BYTE *)GuiBuffer + dr.dstpos;
	a = menuvram->alpha + dr.srcpos;
	salign = menuvram->width;
	dalign = dr.yalign - (dr.width * dr.xalign);
	do {
		x = 0;
		do {
			if (a[x]) {
				if (a[x] & 2) {
					if(temp_bppswitch) {
						*(UINT32 *)r = *(UINT32 *)(p + (x * 4));
					} else {
						*(UINT16 *)r = *(UINT16 *)(p + (x * 2));
					}
				}
				else {
					a[x] = 0;
					if(temp_bppswitch) {
						*(UINT32 *)r = *(UINT32 *)(p + (x * 4));
					} else {
						*(UINT16 *)r = *(UINT16 *)(p + (x * 2));
					}
				}
			}
			r += dr.xalign;
		} while(++x < dr.width);
		if(temp_bppswitch) {
			p += salign * 4;
			q += salign * 4;
		} else {
			p += salign * 2;
			q += salign * 2;
		}
		r += dalign;
		a += salign;
	} while(--dr.height);

}
#endif	/* __LIBRETRO__ */

void scrnmng_initialize(void) {

#if defined(__LIBRETRO__)
	scrnsurf.width = 640;
	scrnsurf.height = 400;
#else	/* __LIBRETRO__ */
	scrnstat.width = 640;
	scrnstat.height = 400;
#endif	/* __LIBRETRO__ */
}

BRESULT scrnmng_create(int width, int height) {

#if defined(__LIBRETRO__)
   scrnsurf.ptr    = (UINT8*)FrameBuffer;
   if(temp_bppswitch) {
      scrnsurf.xalign = 4;//bytes per pixel
      scrnsurf.yalign = width * 4;//bytes per line
   } else {
      scrnsurf.xalign = 2;//bytes per pixel
      scrnsurf.yalign = width * 2;//bytes per line
   }
   scrnsurf.width  = width;
   scrnsurf.height = 400;
   if(temp_bppswitch) {
	   scrnsurf.bpp    = 32;
   } else {
	   scrnsurf.bpp    = 16;
   }
   scrnsurf.extend = 0;//?
   return(SUCCESS);
#else	/* __LIBRETRO__ */
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
	if(temp_bppswitch) {
		s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC, width, 400);
		s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, 400, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	} else {
		s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, width, 400);
		s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, 400, 16, 0xf800, 0x07e0, 0x001f, 0);
	}

	surface = s_surface;
	r = FALSE;
	fmt = surface->format;
	switch(fmt->BitsPerPixel) {
#if defined(SUPPORT_8BPP)
	case 8:
#endif
#if defined(SUPPORT_24BPP)
	case 24:
#endif
#if defined(SUPPORT_32BPP)
	case 32:
#endif
		r = TRUE;
		break;
#if defined(SUPPORT_16BPP)
	case 16:
		if ((fmt->Rmask == 0xf800) && (fmt->Gmask == 0x07e0) && (fmt->Bmask == 0x001f)) {
			r = TRUE;
		}
		break;
#endif
	}
//#if defined(SCREEN_BPP)
//	if (fmt->BitsPerPixel != SCREEN_BPP) {
//		r = FALSE;
//	}
//#endif
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
#endif	/* __LIBRETRO__ */
}

void scrnmng_destroy(void) {

#if !defined(__LIBRETRO__)
	scrnmng.enable = FALSE;
#endif	/* __LIBRETRO__ */
}

RGB16 scrnmng_makepal16(RGB32 pal32) {

	RGB16	ret;

	ret = (pal32.p.r & 0xf8) << 8;
#if defined(NP2_SIZE_QVGA)
	ret += (pal32.p.g & 0xfc) << (3 + 16);
#else
	ret += (pal32.p.g & 0xfc) << 3;
#endif
	ret += pal32.p.b >> 3;
	return(ret);
}

void scrnmng_setwidth(int posx, int width) {

#if !defined(__LIBRETRO__)
	SDL_FreeSurface(s_surface);
	SDL_DestroyTexture(s_texture);
	SDL_RenderSetLogicalSize(s_renderer, width, scrnmng.height);
	if(temp_bppswitch) {
		s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC, width, scrnmng.height);
		s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, scrnmng.height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	} else {
		s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, width, scrnmng.height);
		s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, scrnmng.height, 16, 0xf800, 0x07e0, 0x001f, 0);
	}
	scrnstat.width = width;
#else	/* __LIBRETRO__ */
	struct retro_system_av_info info;
	retro_get_system_av_info(&info);
	info.geometry.base_width = info.geometry.max_width = scrnsurf.width = width;
	info.geometry.base_height = info.geometry.max_height = scrnsurf.height;
	info.geometry.aspect_ratio = (double)scrnsurf.width / scrnsurf.height;
	environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
#endif	/* __LIBRETRO__ */
}

void scrnmng_setheight(int posy, int height) {

#if !defined(__LIBRETRO__)
	SDL_FreeSurface(s_surface);
	SDL_DestroyTexture(s_texture);
	SDL_RenderSetLogicalSize(s_renderer, scrnstat.width, height);
	if(temp_bppswitch) {
		s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC, scrnstat.width, height);
		s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnstat.width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	} else {
		s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, scrnstat.width, height);
		s_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnstat.width, height, 16, 0xf800, 0x07e0, 0x001f, 0);
	}
	scrnstat.height = height;
#else	/* __LIBRETRO__ */
	struct retro_system_av_info info;
	retro_get_system_av_info(&info);
	info.geometry.base_width = info.geometry.max_width = scrnsurf.width;
	info.geometry.base_height = info.geometry.max_height = scrnsurf.height = height;
	info.geometry.aspect_ratio = (double)scrnsurf.width / scrnsurf.height;
	environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
#endif	/* __LIBRETRO__ */
}

const SCRNSURF *scrnmng_surflock(void) {

#if defined(__LIBRETRO__)
	scrnsurf.width = scrnsurf.width;
//	scrnsurf.height =  400;
	scrnsurf.xalign = 2;
	scrnsurf.yalign = scrnsurf.width*2;
	if(temp_bppswitch) {
		scrnsurf.bpp = 32;
	} else {
		scrnsurf.bpp = 16;
	}
	scrnsurf.extend = 0;

	if (vram == NULL)
		scrnsurf.ptr = (BYTE *)FrameBuffer;
	else 
		scrnsurf.ptr = vram->ptr;

	return(&scrnsurf);
#else	/* __LIBRETRO__ */
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
	scrnsurf.width = np2max(scrnstat.width, 640);
	scrnsurf.height = np2max(scrnstat.height, 400);
	scrnsurf.extend = 0;
	return(&scrnsurf);
#endif	/* __LIBRETRO__ */
}

static void draw_onmenu(void) {

#if defined(__LIBRETRO__)
	RECT_T		rt;
	DRAWRECT	dr;

	rt.left = 0;
	rt.top = 0;
	rt.right = np2min(scrnsurf.width, 640);
	rt.bottom = np2min(scrnsurf.height, 400);

	if (calcdrawrect( &dr, &rt) == SUCCESS)
		draw(dr);
#else	/* __LIBRETRO__ */
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
	rt.right = np2min(scrnstat.width, 640);
	rt.bottom = np2min(scrnstat.height, 400);
#if defined(NP2_SIZE_QVGA)
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
#endif	/* __LIBRETRO__ */
}

void scrnmng_surfunlock(const SCRNSURF *surf) {

#if defined(__LIBRETRO__)
	if (surf)
		if (vram == NULL);
		else{
			if (menuvram)
				draw_onmenu();			
		}
#else	/* __LIBRETRO__ */
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
#endif	/* __LIBRETRO__ */
}


// ----

BRESULT scrnmng_entermenu(SCRNMENU *smenu) {

	if (smenu == NULL) {
		goto smem_err;
	}
#if defined(__LIBRETRO__)
	vram_destroy(vram);
	vram = vram_create(scrnsurf.width,scrnsurf.height,FALSE,scrnsurf.bpp);

	if (vram == NULL) {
#else	/* __LIBRETRO__ */
	vram_destroy(scrnmng.vram);
	scrnmng.vram = vram_create(scrnmng.width, scrnmng.height, FALSE,
																scrnmng.bpp);
	if (scrnmng.vram == NULL) {
#endif	/* __LIBRETRO__ */
		goto smem_err;
	}
	scrndraw_redraw();
#if defined(__LIBRETRO__)
	smenu->width = scrnsurf.width;
	smenu->height = scrnsurf.height;
	smenu->bpp = scrnsurf.bpp;
#else	/* __LIBRETRO__ */
	smenu->width = scrnmng.width;
	smenu->height = scrnmng.height;
	if(temp_bppswitch) {
		smenu->bpp = 32;
	} else {
		smenu->bpp = 16;
	}
	mousemng_showcursor();
#endif	/* __LIBRETRO__ */
	return(SUCCESS);

smem_err:
	return(FAILURE);
}

void scrnmng_leavemenu(void) {

#if defined(__LIBRETRO__)
	VRAM_RELEASE(vram);
#else	/* __LIBRETRO__ */
	VRAM_RELEASE(scrnmng.vram);
	mousemng_hidecursor();
#endif	/* __LIBRETRO__ */
}

void scrnmng_updatecursor(void) {
#if !defined(__LIBRETRO__)
	SDL_Surface	*surface;
	surface = s_surface;
	if (surface == NULL) {
		return;
	}

	SDL_UpdateTexture(s_texture, NULL, surface->pixels, surface->pitch);
	SDL_RenderClear(s_renderer);
	SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
	SDL_RenderPresent(s_renderer);
#endif	/* __LIBRETRO__ */
}

void scrnmng_menudraw(const RECT_T *rct) {

#if defined(__LIBRETRO__)
	DRAWRECT	dr;

	if (calcdrawrect( &dr, rct) == SUCCESS) 
		draw2(dr);
#else	/* __LIBRETRO__ */
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
				q = menuvram->ptr + (dr.srcpos * 4);
				r = (UINT8 *)surface->pixels + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x]) {
							if (a[x] & 2) {
								((RGB32 *)r)->p.b = q[x*4+0];
								((RGB32 *)r)->p.g = q[x*4+1];
								((RGB32 *)r)->p.r = q[x*4+2];
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
					q += salign * 4;
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
#endif	/* __LIBRETRO__ */
}

