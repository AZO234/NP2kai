#include	"compiler.h"
#include	"mousemng.h"
#include	"scrnmng.h"
#include	"menubase.h"
#include	"commng.h"
#include	"pccore.h"
#include	"np2.h"

#if defined(SUPPORT_WAB)
#include "wab.h"
#endif

#if defined(__LIBRETRO__)
#include "libretro.h"
#include "libretro_exports.h"

extern retro_environment_t environ_cb;
#endif	/* __LIBRETRO__ */

typedef struct {
	BOOL	enable;
	int		width;
	int		height;
	int		bpp;
#if defined(__LIBRETRO__)
	void*	pc98surf;
	void*	dispsurf;
#else	/* __LIBRETRO__ */
	SDL_Surface* pc98surf;
	SDL_Surface* dispsurf;
#endif	/* __LIBRETRO__ */
	VRAMHDL vram;
} SCRNMNG;

static SCRNMNG scrnmng;
static SCRNSURF scrnsurf;

#if !defined(__LIBRETRO__)
#if SDL_MAJOR_VERSION == 1
static SDL_VideoInfo* s1_videoinfo;
#else
static SDL_Window* s_window;
static SDL_Renderer* s_renderer;
static SDL_Texture* s_texture;
#endif
#endif	/* __LIBRETRO__ */

static const char app_name[] =
#if defined(CPUCORE_IA32) && !defined(__LIBRETRO__)
    "Neko Project II kai + IA-32"
#else
    "Neko Project II kai"
#endif
;

typedef struct {
	int		xalign;
	int		yalign;
	int		width;
	int		height;
	int		srcpos;
	int		dstpos;
} DRAWRECT;

static BRESULT calcdrawrect(DRAWRECT *dr, const RECT_T *rt) {
	int		pos;

	dr->xalign = scrnmng.bpp / 8;
	dr->yalign = scrnmng.width * dr->xalign;
	dr->srcpos = 0;
	dr->dstpos = 0;
	dr->width = scrnmng.width;
	dr->height = scrnmng.height;

	if (rt) {
		pos = MAX(rt->left, 0);
		dr->srcpos += pos;
		dr->dstpos += pos * dr->xalign;
		dr->width = MIN(rt->right, dr->width) - pos;

		pos = MAX(rt->top, 0);
		dr->srcpos += pos * scrnmng.width;
		dr->dstpos += pos * dr->yalign;
		dr->height = MIN(rt->bottom, dr->height) - pos;
	}
	if ((dr->width <= 0) || (dr->height <= 0)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

void scrnmng_initialize(void) {
	scrnmng.width = 640;
	scrnmng.height = 400;
}

void scrnmng_getsize(int* pw, int* ph) {
	*pw = scrnmng.width;
	*ph = scrnmng.height;
}

BRESULT scrnmng_create(UINT8 mode) {
   if(draw32bit) {
      scrnmng.bpp = 32;
   } else {
      scrnmng.bpp = 16;
   }

#if defined(__LIBRETRO__)
   scrnmng.pc98surf = malloc(scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
   scrnmng.dispsurf = malloc(scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
#else	/* __LIBRETRO__ */
	if(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
		return(FAILURE);
	}

#if SDL_MAJOR_VERSION == 1
	s1_videoinfo = SDL_GetVideoInfo();
	scrnmng.dispsurf = SDL_SetVideoMode(scrnmng.width, scrnmng.height, scrnmng.bpp, SDL_HWSURFACE);
	scrnmng.pc98surf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, scrnmng.height, scrnmng.bpp, 0xf800, 0x07e0, 0x001f, 0);
#else
	if(mode & SCRNMODE_ROTATEMASK) {
		s_window = SDL_CreateWindow(app_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, scrnmng.height, scrnmng.width, 0);
	} else {
		s_window = SDL_CreateWindow(app_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, scrnmng.width, scrnmng.height, 0);
	}
	s_renderer = SDL_CreateRenderer(s_window, -1, 0);
#endif
#if SDL_MAJOR_VERSION != 1
#if defined(__OPENDINGUX__)
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
#endif
	if(mode & SCRNMODE_ROTATEMASK) {
		SDL_RenderSetLogicalSize(s_renderer, scrnmng.height, scrnmng.width);
	} else {
		SDL_RenderSetLogicalSize(s_renderer, scrnmng.width, scrnmng.height);
	}
	switch(scrnmng.bpp) {
	case 16:
		if(mode & SCRNMODE_ROTATEMASK) {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, scrnmng.height, scrnmng.width);
		} else {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, scrnmng.width, scrnmng.height);
		}
		scrnmng.pc98surf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, scrnmng.height, scrnmng.bpp, 0xf800, 0x07e0, 0x001f, 0);
		scrnmng.dispsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, scrnmng.height, scrnmng.bpp, 0xf800, 0x07e0, 0x001f, 0);
		break;
	case 32:
		if(mode & SCRNMODE_ROTATEMASK) {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, scrnmng.height, scrnmng.width);
		} else {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, scrnmng.width, scrnmng.height);
		}
		scrnmng.pc98surf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, scrnmng.height, scrnmng.bpp, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		scrnmng.dispsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, scrnmng.height, scrnmng.bpp, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		break;
	}
#endif
#endif	/* __LIBRETRO__ */

	scrnsurf.width = scrnmng.width;
	scrnsurf.height = scrnmng.height;
	scrnsurf.xalign = scrnmng.bpp / 8;									// bytes per pixel
	scrnsurf.yalign = scrnmng.width * scrnsurf.xalign;	// bytes per line
	scrnsurf.extend = 0;																// ?

	scrnmng.enable = TRUE;
	return(SUCCESS);
}

void scrnmng_destroy(void) {
	if(menuvram) {
		menubase_close();
	}
	scrnmng.enable = FALSE;

#if defined(__LIBRETRO__)
	free(scrnmng.pc98surf);
	scrnmng.pc98surf = NULL;
	free(scrnmng.dispsurf);
	scrnmng.dispsurf = NULL;
#else
	SDL_FreeSurface(scrnmng.pc98surf);
	SDL_FreeSurface(scrnmng.dispsurf);
#if SDL_MAJOR_VERSION != 1
	SDL_DestroyTexture(s_texture);
	SDL_DestroyRenderer(s_renderer);
	SDL_DestroyWindow(s_window);
#endif
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
	if(menuvram) {
		menubase_close();
	}

#if defined(__LIBRETRO__)
	struct retro_system_av_info info;

	free(scrnmng.pc98surf);
	scrnmng.pc98surf = malloc(width * scrnmng.height * scrnmng.bpp / 8);
	free(scrnmng.dispsurf);
	scrnmng.dispsurf = malloc(width * scrnmng.height * scrnmng.bpp / 8);

	retro_get_system_av_info(&info);
	info.geometry.base_width = info.geometry.max_width = scrnmng.width = width;
	info.geometry.base_height = info.geometry.max_height = scrnmng.height;
	info.geometry.aspect_ratio = (double)scrnmng.width / scrnmng.height;
	environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
#else	/* __LIBRETRO__ */
	SDL_FreeSurface(scrnmng.pc98surf);
	SDL_FreeSurface(scrnmng.dispsurf);
#if SDL_MAJOR_VERSION != 1
	SDL_DestroyTexture(s_texture);

	if(scrnmode & SCRNMODE_ROTATEMASK) {
		SDL_SetWindowSize(s_window, scrnmng.height, width);
	} else {
		SDL_SetWindowSize(s_window, width, scrnmng.height);
	}
	if(scrnmode & SCRNMODE_ROTATEMASK) {
		SDL_RenderSetLogicalSize(s_renderer, scrnmng.height, width);
	} else {
		SDL_RenderSetLogicalSize(s_renderer, width, scrnmng.height);
	}
#if defined(__OPENDINGUX__)
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
#endif
	switch(scrnmng.bpp) {
	case 16:
		if(scrnmode & SCRNMODE_ROTATEMASK) {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, scrnmng.height, width);
		} else {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, width, scrnmng.height);
		}
		scrnmng.pc98surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, scrnmng.height, scrnmng.bpp, 0xf800, 0x07e0, 0x001f, 0);
		scrnmng.dispsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, scrnmng.height, scrnmng.bpp, 0xf800, 0x07e0, 0x001f, 0);
		break;
	case 32:
		if(scrnmode & SCRNMODE_ROTATEMASK) {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, scrnmng.height, width);
		} else {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, scrnmng.height);
		}
		scrnmng.pc98surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, scrnmng.height, scrnmng.bpp, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		scrnmng.dispsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, scrnmng.height, scrnmng.bpp, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		break;
	}
#endif
#endif	/* __LIBRETRO__ */

	scrnmng.width = width;
	scrnsurf.width = width;
	menubase.width = width;
}

void scrnmng_setheight(int posy, int height) {
	if(menuvram) {
		menubase_close();
	}

#if defined(__LIBRETRO__)
	struct retro_system_av_info info;

	free(scrnmng.pc98surf);
	scrnmng.pc98surf = malloc(scrnmng.width * height * scrnmng.bpp / 8);
	free(scrnmng.dispsurf);
	scrnmng.dispsurf = malloc(scrnmng.width * height * scrnmng.bpp / 8);

	retro_get_system_av_info(&info);
	info.geometry.base_width = info.geometry.max_width = scrnmng.width;
	info.geometry.base_height = info.geometry.max_height = scrnmng.height = height;
	info.geometry.aspect_ratio = (double)scrnmng.width / scrnmng.height;
	environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
#else	/* __LIBRETRO__ */
	SDL_FreeSurface(scrnmng.pc98surf);
	SDL_FreeSurface(scrnmng.dispsurf);
#if SDL_MAJOR_VERSION != 1
	SDL_DestroyTexture(s_texture);

	if(scrnmode & SCRNMODE_ROTATEMASK) {
		SDL_SetWindowSize(s_window, height, scrnmng.width);
	} else {
		SDL_SetWindowSize(s_window, scrnmng.width, height);
	}
	if(scrnmode & SCRNMODE_ROTATEMASK) {
		SDL_RenderSetLogicalSize(s_renderer, height, scrnmng.width);
	} else {
		SDL_RenderSetLogicalSize(s_renderer, scrnmng.width, height);
	}
#if defined(__OPENDINGUX__)
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
#endif
	switch(scrnmng.bpp) {
	case 16:
		if(scrnmode & SCRNMODE_ROTATEMASK) {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, height, scrnmng.width);
		} else {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, scrnmng.width, height);
		}
		scrnmng.pc98surf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, height, scrnmng.bpp, 0xf800, 0x07e0, 0x001f, 0);
		scrnmng.dispsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, height, scrnmng.bpp, 0xf800, 0x07e0, 0x001f, 0);
		break;
	case 32:
		if(scrnmode & SCRNMODE_ROTATEMASK) {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, height, scrnmng.width);
		} else {
			s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, scrnmng.width, height);
		}
		scrnmng.pc98surf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, height, scrnmng.bpp, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		scrnmng.dispsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, scrnmng.width, height, scrnmng.bpp, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		break;
	}
	scrnmng.height = height;
#endif
#endif	/* __LIBRETRO__ */

	scrnmng.height = height;
	scrnsurf.height = height;
	menubase.height = height;
}

#if !defined(__LIBRETRO__)
SDL_Surface* scrnmng_makerotatesurface(char bLeft) {
	SDL_Surface* output = NULL;
	int x, y;

	switch(scrnmng.bpp) {
	case 16:
		output = SDL_CreateRGBSurface(0, scrnmng.height, scrnmng.width, scrnmng.bpp, 0xf800, 0x07e0, 0x001f, 0);
		if(output) {
			SDL_LockSurface(output);
			for(y = 0; y < scrnmng.height; y++) {
				for(x = 0; x < scrnmng.width; x++) {
					if(bLeft)
						((UINT16*)output->pixels)[scrnmng.height * ((scrnmng.width - 1) - x) + y] = ((UINT16*)scrnmng.dispsurf->pixels)[scrnmng.width * y + x];
					else
						((UINT16*)output->pixels)[scrnmng.height * x + (scrnmng.height - 1) - y] = ((UINT16*)scrnmng.dispsurf->pixels)[scrnmng.width * y + x];
				}
			}
			SDL_UnlockSurface(output);
		}
		break;
	case 32:
		output = SDL_CreateRGBSurface(0, scrnmng.height, scrnmng.width, scrnmng.bpp, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		if(output) {
			SDL_LockSurface(output);
			for(y = 0; y < scrnmng.height; y++) {
				for(x = 0; x < scrnmng.width; x++) {
					if(bLeft)
						((UINT32*)output->pixels)[scrnmng.height * ((scrnmng.width - 1) - x) + y] = ((UINT32*)scrnmng.dispsurf->pixels)[scrnmng.width * y + x];
					else
						((UINT32*)output->pixels)[scrnmng.height * x + (scrnmng.height - 1) - y] = ((UINT32*)scrnmng.dispsurf->pixels)[scrnmng.width * y + x];
				}
			}
			SDL_UnlockSurface(output);
		}
		break;
	}
	return output;
}
#endif	/* __LIBRETRO__ */

const SCRNSURF *scrnmng_surflock(void) {
	if(!scrnmng.enable) return NULL;

	if (scrnmng.pc98surf) {
		scrnsurf.width = scrnmng.width;
		scrnsurf.height = scrnmng.height;
		scrnsurf.bpp = scrnmng.bpp;
		scrnsurf.xalign = scrnmng.bpp / 8;
		scrnsurf.yalign = scrnsurf.width * scrnsurf.xalign;
		scrnsurf.extend = 0;
#if defined(__LIBRETRO__)
		scrnsurf.ptr = (BYTE*)(scrnmng.pc98surf);
#else	/* __LIBRETRO__ */
		SDL_LockSurface(scrnmng.pc98surf);
		scrnsurf.ptr = (BYTE*)(scrnmng.pc98surf->pixels);
#endif	/* __LIBRETRO__ */
	}
	return(&scrnsurf);
}

void scrnmng_surfunlock(const SCRNSURF *surf) {
	if(scrnmng.pc98surf) {
#if defined(__LIBRETRO__)
		memcpy(scrnmng.dispsurf, scrnmng.pc98surf, scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
#else
		SDL_UnlockSurface(scrnmng.pc98surf);
		SDL_LockSurface(scrnmng.dispsurf);
		memcpy(scrnmng.dispsurf->pixels, scrnmng.pc98surf->pixels, scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
		SDL_UnlockSurface(scrnmng.dispsurf);
#endif	/* __LIBRETRO__ */
		if(menuvram) {
			RECT_T rect = {0, 0, scrnmng.width, scrnmng.height};
			scrnmng_menudraw(&rect);
		}
	}
	scrnmng_update();
}

// ----

BRESULT scrnmng_entermenu(SCRNMENU *smenu) {
	if (smenu == NULL) {
		return(FAILURE);
	}
	smenu->width = scrnmng.width;
	smenu->height = scrnmng.height;
	smenu->bpp = scrnmng.bpp;
#if !defined(__LIBRETRO__)
	mousemng_showcursor();
#endif	/* __LIBRETRO__ */
	return(SUCCESS);
}

void scrnmng_leavemenu(void) {
#if !defined(__LIBRETRO__)
#if !defined(TARGET_OS_MAC)
/* once hide mouse cursor, can't show it on macOS */
	mousemng_hidecursor();
#endif
#endif	/* __LIBRETRO__ */
}

void scrnmng_updatecursor(void) {
}

void scrnmng_menudraw(const RECT_T *rct) {
	if(!menuvram) return;
	if(!scrnmng.enable) return;

	if(scrnmng.dispsurf) {
		void* pc98surf;
		void* dispsurf;
		const UINT8* p;
		const UINT8* q;
		UINT8* r;
		UINT8* a;
		int salign;
		int dalign;
		int x;
		DRAWRECT dr;

#if defined(__LIBRETRO__)
		pc98surf = scrnmng.pc98surf;
		dispsurf = scrnmng.dispsurf;
#else
		SDL_LockSurface(scrnmng.dispsurf);
		pc98surf = scrnmng.pc98surf->pixels;
		dispsurf = scrnmng.dispsurf->pixels;
#endif	/* __LIBRETRO__ */

		if(calcdrawrect(&dr, rct) == SUCCESS) {
			switch(scrnmng.bpp) {
#if defined(SUPPORT_16BPP)
			case 16:
				p = (UINT8*)pc98surf + (dr.srcpos * dr.xalign);
				q = menuvram->ptr + (dr.srcpos * dr.xalign);
				r = (UINT8*)dispsurf + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if(a[x]) {
							if(a[x] & 2) {
								*(UINT32*)r = *(UINT32*)(q + (x * 2));
							} else {
								a[x] = 0;
								*(UINT32*)r = *(UINT32*)(p + (x * 2));
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
#if defined(SUPPORT_32BPP)
			case 32:
				p = (UINT8*)pc98surf + (dr.srcpos * dr.xalign);
				q = menuvram->ptr + (dr.srcpos * dr.xalign);
				r = (UINT8*)dispsurf + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if(a[x]) {
							if(a[x] & 2) {
								*(UINT32*)r = *(UINT32*)(q + (x * 4));
							} else {
								a[x] = 0;
								*(UINT32*)r = *(UINT32*)(p + (x * 4));
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
#if !defined(__LIBRETRO__)
			SDL_UnlockSurface(scrnmng.dispsurf);
#endif	/* __LIBRETRO__ */
			scrnmng_update();
		}
	}
}

void
scrnmng_update(void)
{
	if(!scrnmng.enable) return;

#if defined(__LIBRETRO__)
	memcpy(FrameBuffer, scrnmng.dispsurf, scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
#else
	SDL_Surface	*usesurface;

#if SDL_MAJOR_VERSION == 1
	SDL_UpdateRect(scrnmng.dispsurf, 0, 0, 0, 0);
#else
	if((scrnmode & SCRNMODE_ROTATEMASK) == SCRNMODE_ROTATELEFT) {
		usesurface = scrnmng_makerotatesurface(1);
		SDL_UpdateTexture(s_texture, NULL, usesurface->pixels, usesurface->pitch);
		SDL_FreeSurface(usesurface);
	} else if((scrnmode & SCRNMODE_ROTATEMASK) == SCRNMODE_ROTATERIGHT) {
		usesurface = scrnmng_makerotatesurface(0);
		SDL_UpdateTexture(s_texture, NULL, usesurface->pixels, usesurface->pitch);
		SDL_FreeSurface(usesurface);
	} else {
		SDL_UpdateTexture(s_texture, NULL, scrnmng.dispsurf->pixels, scrnmng.dispsurf->pitch);
	}
	SDL_RenderClear(s_renderer);
	SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
	SDL_RenderPresent(s_renderer);
#endif
#endif
}

// fullscreen resolution
void scrnmng_updatefsres(void) {
}

// transmit WAB display
void scrnmng_blthdc(void) {
}

void scrnmng_bltwab(void) {
#if defined(SUPPORT_WAB)
	if(scrnmng.pc98surf) {
#if !defined(__LIBRETRO__)
		SDL_LockSurface(scrnmng.pc98surf);
		memcpy(scrnmng.pc98surf->pixels, np2wabwnd.pBuffer, scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
		SDL_UnlockSurface(scrnmng.pc98surf);
		SDL_LockSurface(scrnmng.dispsurf);
		memcpy(scrnmng.dispsurf->pixels, scrnmng.pc98surf->pixels, scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
		SDL_UnlockSurface(scrnmng.dispsurf);
#else
		memcpy(scrnmng.pc98surf, np2wabwnd.pBuffer, scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
		memcpy(scrnmng.dispsurf, scrnmng.pc98surf, scrnmng.width * scrnmng.height * scrnmng.bpp / 8);
#endif
		if(menuvram) {
			RECT_T rect = {0, 0, scrnmng.width, scrnmng.height};
			scrnmng_menudraw(&rect);
		}
		scrnmng_update();
	}
#endif
}
