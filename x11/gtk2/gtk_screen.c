/*
 * Copyright (c) 2003 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"

#include <math.h>

#include "np2.h"
#include "palettes.h"
#include "scrndraw.h"

#include "toolkit.h"

#include "scrnmng.h"
#include "mousemng.h"
#include "soundmng.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_drawmng.h"
#include "gtk2/gtk_menu.h"


typedef struct {
	UINT8		scrnmode;
	volatile int	drawing;
	int		width;		/* drawarea の width */
	int		height;		/* drawarea の height */
	int		extend;
	int		clipping;

	PAL16MASK	pal16mask;

	RECT_T		scrn;		/* drawarea 内の描画領域位置 */
	RECT_T		rect;		/* drawarea に描画するサイズ */

	/* toolkit depend */
	GdkPixbuf	*drawsurf;
	GdkPixbuf	*backsurf;
	GdkPixbuf	*surface;
	double		ratio_w, ratio_h;
	int		interp;

	GdkColor	pal[NP2PAL_MAX];
} DRAWMNG;

typedef struct {
	int	width;
	int	height;
	int	extend;
	int	multiple;
} SCRNSTAT;

static SCRNMNG scrnmng;
static DRAWMNG drawmng;
static SCRNSTAT scrnstat = { 640, 400, 1, SCREEN_DEFMUL };
static SCRNSURF scrnsurf;
static int real_fullscreen;

SCRNMNG *scrnmngp = &scrnmng;

GtkWidget *main_window;
GtkWidget *drawarea;

#define	BITS_PER_PIXEL	24
#define	BYTES_PER_PIXEL	3

/*
 * drawarea のアスペクト比を 4:3 (640x480) にする。
 */
static void
adapt_aspect(int width, int height, int scrnwidth, int scrnheight)
{
	double ratio;
	int w, h;

	ratio = (double)scrnwidth / width;
	h = floor((height * ratio) + 0.5);
	if (h < scrnheight) {
		drawmng.rect.right = scrnwidth;
		drawmng.rect.bottom = h;
	} else {
		ratio = (double)scrnheight / height;
		w = floor((width * ratio) + 0.5);
		if (w < scrnwidth) {
			drawmng.rect.right = w;
			drawmng.rect.bottom = scrnheight;
		}
	}
}

static void
set_window_size(int width, int height)
{

	gtk_widget_set_size_request(drawarea,
	    width + np2oscfg.paddingx * 2, height + np2oscfg.paddingy * 2);
}

static void
renewal_client_size(void)
{
	int width;
	int height;
	int extend;
	int scrnwidth;
	int scrnheight;
	int multiple;

	width = min(scrnstat.width, drawmng.width);
	height = min(scrnstat.height, drawmng.height);
	extend = 0;

	if (drawmng.scrnmode & SCRNMODE_FULLSCREEN) {
		scrnwidth = drawmng.width;
		scrnheight = drawmng.height;

		drawmng.rect.right = width;
		drawmng.rect.bottom = height;
		if (!real_fullscreen) {
			adapt_aspect(width, height, scrnwidth, scrnheight);
		}

		drawmng.ratio_w = (double)drawmng.rect.right / width;
		drawmng.ratio_h = (double)drawmng.rect.bottom / height;

		drawmng.scrn.left = (scrnwidth - drawmng.rect.right) / 2;
		drawmng.scrn.top = (scrnheight - drawmng.rect.bottom) / 2;
		drawmng.scrn.right = drawmng.scrn.left + drawmng.rect.right;
		drawmng.scrn.bottom = drawmng.scrn.top + drawmng.rect.bottom;

		gtk_widget_set_size_request(drawarea, scrnwidth, scrnheight);
	} else {
		multiple = scrnstat.multiple;
		if (!(drawmng.scrnmode & SCRNMODE_ROTATE)) {
			if ((np2oscfg.paddingx > 0) && (multiple == SCREEN_DEFMUL)) {
				extend = min(scrnstat.extend, drawmng.extend);
			}
			scrnwidth = (width * multiple) / SCREEN_DEFMUL;
			scrnheight = (height * multiple) / SCREEN_DEFMUL;

			drawmng.rect.right = scrnwidth + extend;
			drawmng.rect.bottom = scrnheight;

			drawmng.ratio_w = (double)drawmng.rect.right / width;
			drawmng.ratio_h = (double)drawmng.rect.bottom / height;

			drawmng.scrn.left = np2oscfg.paddingx - extend;
			drawmng.scrn.top = np2oscfg.paddingy;
		} else {
			if ((np2oscfg.paddingy > 0) && (multiple == SCREEN_DEFMUL)) {
				extend = min(scrnstat.extend, drawmng.extend);
			}
			scrnwidth = (height * multiple) / SCREEN_DEFMUL;
			scrnheight = (width * multiple) / SCREEN_DEFMUL;

			drawmng.rect.right = scrnwidth;
			drawmng.rect.bottom = scrnheight + extend;

			drawmng.ratio_w = (double)drawmng.rect.right / height;
			drawmng.ratio_h = (double)drawmng.rect.bottom / width;

			drawmng.scrn.left = np2oscfg.paddingx;
			drawmng.scrn.top = np2oscfg.paddingy - extend;
		}
		drawmng.scrn.right = np2oscfg.paddingx + scrnwidth;
		drawmng.scrn.bottom = np2oscfg.paddingy + scrnheight;

		set_window_size(scrnwidth, scrnheight);
	}

	scrnsurf.width = width;
	scrnsurf.height = height;
	scrnsurf.extend = extend;
}

static void
clear_out_of_rect(const RECT_T *target, const RECT_T *base)
{
	GdkDrawable *d = drawarea->window;
	GdkGC *gc = drawarea->style->black_gc;
	RECT_T rect;

	rect.left = base->left;
	rect.right = base->right;
	rect.top = base->top;
	rect.bottom = target->top;
	if (rect.top < rect.bottom) {
		gdk_draw_rectangle(d, gc, TRUE,
		    rect.left, rect.top, rect.right, rect.bottom);
	}
	rect.top = target->bottom;
	rect.bottom = base->bottom;
	if (rect.top < rect.bottom) {
		gdk_draw_rectangle(d, gc, TRUE,
		    rect.left, rect.top, rect.right, rect.bottom);
	}

	rect.top = max(base->top, target->top);
	rect.bottom = min(base->bottom, target->bottom);
	if (rect.top < rect.bottom) {
		rect.left = base->left;
		rect.right = target->left;
		if (rect.left < rect.right) {
			gdk_draw_rectangle(d, gc, TRUE,
			    rect.left, rect.top, rect.right, rect.bottom);
		}
		rect.left = target->right;
		rect.right = base->right;
		if (rect.left < rect.right) {
			gdk_draw_rectangle(d, gc, TRUE,
			    rect.left, rect.top, rect.right, rect.bottom);
		}
	}
}

static void
clear_outscreen(void)
{
	RECT_T base;

	base.left = base.top = 0;
	base.right = drawarea->allocation.width;
	base.bottom = drawarea->allocation.height;
	clear_out_of_rect(&drawmng.scrn, &base);
}

static void
palette_init(void)
{
	GdkColormap *cmap;
	gboolean success;
	int i;

	cmap = gdk_colormap_get_system();
	for (i = 0; i < 8; i++) {
		drawmng.pal[NP2PAL_TEXT + i + 1].pixel =
		    (np2_pal32[NP2PAL_TEXT + i + 1].p.r << 0) |
		    (np2_pal32[NP2PAL_TEXT + i + 1].p.g << 8) |
		    (np2_pal32[NP2PAL_TEXT + i + 1].p.b << 16);
		drawmng.pal[NP2PAL_TEXT + i + 1].red =
		    np2_pal32[NP2PAL_TEXT + i + 1].p.r << 8;
		drawmng.pal[NP2PAL_TEXT + i + 1].green =
		    np2_pal32[NP2PAL_TEXT + i + 1].p.g << 8;
		drawmng.pal[NP2PAL_TEXT + i + 1].blue =
		    np2_pal32[NP2PAL_TEXT + i + 1].p.b << 8;
	}
	gdk_colormap_alloc_colors(cmap, &drawmng.pal[NP2PAL_TEXT + 1], 8,
	    TRUE, FALSE, &success);
}

static void
palette_set(void)
{
	static int first = 1;
	GdkColormap *cmap;
	gboolean success;
	int i;

	cmap = gdk_colormap_get_system();

	if (!first) {
		gdk_colormap_free_colors(cmap, &drawmng.pal[NP2PAL_GRPH],
		    NP2PALS_GRPH);
	}
	first = 0;

	for (i = 0; i < NP2PALS_GRPH; i++) {
		drawmng.pal[NP2PAL_GRPH + i].pixel =
		    (np2_pal32[NP2PAL_GRPH + i].p.r << 0) |
		    (np2_pal32[NP2PAL_GRPH + i].p.g << 8) |
		    (np2_pal32[NP2PAL_GRPH + i].p.b << 16);
		drawmng.pal[NP2PAL_GRPH + i].red =
		    np2_pal32[NP2PAL_GRPH + i].p.r << 8;
		drawmng.pal[NP2PAL_GRPH + i].green =
		    np2_pal32[NP2PAL_GRPH + i].p.g << 8;
		drawmng.pal[NP2PAL_GRPH + i].blue =
		    np2_pal32[NP2PAL_GRPH + i].p.b << 8;
	}
	gdk_colormap_alloc_colors(cmap, &drawmng.pal[NP2PAL_GRPH], NP2PALS_GRPH,
	    TRUE, FALSE, &success);
}

void
scrnmng_initialize(void)
{

	drawmng.drawing = FALSE;
	scrnstat.width = 640;
	scrnstat.height = 400;
	scrnstat.extend = 1;
	scrnstat.multiple = SCREEN_DEFMUL;
	set_window_size(scrnstat.width, scrnstat.height);

	real_fullscreen = gtk_window_init_fullscreen(main_window);

	switch (np2oscfg.drawinterp) {
	case INTERP_NEAREST:
		drawmng.interp = GDK_INTERP_NEAREST;
		break;

	case INTERP_TILES:
		drawmng.interp = GDK_INTERP_TILES;
		break;

	case INTERP_HYPER:
		drawmng.interp = GDK_INTERP_HYPER;
		break;

	case INTERP_BILINEAR:
	default:
		drawmng.interp = GDK_INTERP_BILINEAR;
		break;
	}
}

BRESULT
scrnmng_create(UINT8 mode)
{
	GdkScreen *screen;
	GdkVisual *visual;
	RECT_T rect;
	pixmap_format_t fmt;

	while (drawmng.drawing)
		gtk_main_iteration_do(FALSE);
	drawmng.drawing = TRUE;

	visual = gtk_widget_get_visual(drawarea);
	if (!gtkdrawmng_getformat(drawarea, main_window, &fmt))
		return FAILURE;

	switch (fmt.bits_per_pixel) {
	case 16:
		drawmng_make16mask(&drawmng.pal16mask, visual->blue_mask,
		    visual->red_mask, visual->green_mask);
		break;

	case 8:
		palette_init();
		break;
	}

	if (mode & SCRNMODE_FULLSCREEN) {
		mode &= ~SCRNMODE_ROTATEMASK;
		scrnmng.flag = 0;
		drawmng.extend = 0;
		if (real_fullscreen) {
			drawmng.width = FULLSCREEN_WIDTH;
			drawmng.height = FULLSCREEN_HEIGHT;
		} else {
			screen = gdk_screen_get_default();
			drawmng.width = gdk_screen_get_width(screen);
			drawmng.height = gdk_screen_get_height(screen);
		}
	} else {
		scrnmng.flag = SCRNFLAG_HAVEEXTEND;
		drawmng.extend = 1;
		drawmng.width = 640;
		drawmng.height = 480;
	}

	if (!(mode & SCRNMODE_ROTATE)) {
		rect.right = 640 + drawmng.extend;
		rect.bottom = 480;
	} else {
		rect.right = 480;
		rect.bottom = 640 + drawmng.extend;
	}

	scrnmng.bpp = BITS_PER_PIXEL;
	scrnsurf.bpp = BITS_PER_PIXEL;
	drawmng.scrnmode = mode;
	drawmng.clipping = 0;
	renewal_client_size();

	drawmng.backsurf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	    rect.right, rect.bottom);
	if (drawmng.backsurf == NULL) {
		drawmng.drawing = FALSE;
		g_message("can't create backsurf.");
		return FAILURE;
	}
	gdk_pixbuf_fill(drawmng.backsurf, 0);

	drawmng.surface = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	    drawmng.rect.right, drawmng.rect.bottom);
	if (drawmng.surface == NULL) {
		drawmng.drawing = FALSE;
		g_message("can't create surface.");
		return FAILURE;
	}
	gdk_pixbuf_fill(drawmng.surface, 0);

	if (mode & SCRNMODE_FULLSCREEN) {
		drawmng.drawsurf =
		    real_fullscreen ? drawmng.backsurf : drawmng.surface;
		xmenu_hide();
		gtk_window_fullscreen_mode(main_window);
	} else {
		drawmng.drawsurf = (scrnstat.multiple == SCREEN_DEFMUL)
		    ? drawmng.backsurf : drawmng.surface;
		gtk_window_restore_mode(main_window);
		xmenu_show();
	}

	drawmng.drawing = FALSE;

	return SUCCESS;
}

void
scrnmng_destroy(void)
{

	if (drawmng.backsurf) {
		g_object_unref(drawmng.backsurf);
		drawmng.backsurf = NULL;
	}
	if (drawmng.surface) {
		g_object_unref(drawmng.surface);
		drawmng.surface = NULL;
	}
}

void
scrnmng_fullscreen(int onoff)
{

	if (onoff) {
		gtk_window_fullscreen_mode(main_window);
	} else {
		gtk_window_restore_mode(main_window);
	}
}

RGB16
scrnmng_makepal16(RGB32 pal32)
{

	return drawmng_makepal16(&drawmng.pal16mask, pal32);
}

void
scrnmng_setwidth(int posx, int width)
{

	scrnstat.width = width;
	renewal_client_size();
}

void
scrnmng_setheight(int posy, int height)
{

	scrnstat.height = height;
	renewal_client_size();
}

void
scrnmng_setextend(int extend)
{

	scrnstat.extend = extend;
	scrnmng.allflash = TRUE;
	renewal_client_size();
}

void
scrnmng_setmultiple(int multiple)
{

	if (scrnstat.multiple != multiple) {
		scrnstat.multiple = multiple;
		if (!(drawmng.scrnmode & SCRNMODE_FULLSCREEN)) {
			soundmng_stop();
			mouse_running(MOUSE_STOP);
			scrnmng_destroy();
			if (scrnmng_create(scrnmode) != SUCCESS) {
				toolkit_widget_quit();
				return;
			}
			renewal_client_size();
			scrndraw_redraw();
			mouse_running(MOUSE_CONT);
			soundmng_play();
		}
	}
}

const SCRNSURF *
scrnmng_surflock(void)
{
	const int lpitch = gdk_pixbuf_get_rowstride(drawmng.backsurf);

	scrnsurf.ptr = (UINT8 *)gdk_pixbuf_get_pixels(drawmng.backsurf);
	if (!(drawmng.scrnmode & SCRNMODE_ROTATE)) {
		scrnsurf.xalign = BYTES_PER_PIXEL;
		scrnsurf.yalign = lpitch;
	} else if (!(drawmng.scrnmode & SCRNMODE_ROTATEDIR)) {
		/* rotate left */
		scrnsurf.ptr += (scrnsurf.width + scrnsurf.extend - 1) * lpitch;
		scrnsurf.xalign = -lpitch;
		scrnsurf.yalign = BYTES_PER_PIXEL;
	} else {
		/* rotate right */
		scrnsurf.ptr += (scrnsurf.height - 1) * BYTES_PER_PIXEL;
		scrnsurf.xalign = lpitch;
		scrnsurf.yalign = -BYTES_PER_PIXEL;
	}
	return &scrnsurf;
}

void
scrnmng_surfunlock(const SCRNSURF *surf)
{

	if (drawmng.drawsurf == drawmng.surface) {
		gdk_pixbuf_scale(drawmng.backsurf, drawmng.surface,
		    0, 0, drawmng.rect.right, drawmng.rect.bottom,
		    0, 0, drawmng.ratio_w, drawmng.ratio_h,
		    drawmng.interp);
	}

	scrnmng_update();
}

void
scrnmng_update(void)
{
	GdkDrawable *d = drawarea->window;
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	GdkGC *gc = drawarea->style->fg_gc[gtk_widget_get_state(drawarea)];
#else
	GdkGC *gc = drawarea->style->fg_gc[GTK_WIDGET_STATE(drawarea)];
#endif

	if (scrnmng.palchanged) {
		scrnmng.palchanged = FALSE;
		palette_set();
	}

	if (drawmng.drawing)
		return;

	drawmng.drawing = TRUE;

	if (drawmng.scrnmode & SCRNMODE_FULLSCREEN) {
		if (scrnmng.allflash) {
			scrnmng.allflash = 0;
			clear_outscreen();
		}
	} else {
		if (scrnmng.allflash) {
			scrnmng.allflash = 0;
			if (np2oscfg.paddingx || np2oscfg.paddingy) {
				clear_outscreen();
			}
		}
	}

	gdk_draw_pixbuf(d, gc, drawmng.drawsurf,
	    0, 0,
	    drawmng.scrn.left, drawmng.scrn.top,
	    drawmng.rect.right, drawmng.rect.bottom,
	    GDK_RGB_DITHER_NORMAL, 0, 0);

	drawmng.drawing = FALSE;
}
