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

#include "np2.h"
#include "palettes.h"
#include "scrndraw.h"

#include "scrnmng.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_drawmng.h"


DRAWMNG_HDL
drawmng_create(void *parent, int width, int height)
{
	GTKDRAWMNG_HDL hdl = NULL;
	GtkWidget *parent_window;
	GdkVisual *visual;
	pixmap_format_t fmt;
	int bytes_per_pixel;
	int padding;

	if (parent == NULL)
		return NULL;
	parent_window = GTK_WIDGET(parent);
	gtk_widget_realize(parent_window);

	hdl = (GTKDRAWMNG_HDL)_MALLOC(sizeof(_GTKDRAWMNG_HDL), "drawmng hdl");
	if (hdl == NULL)
		return NULL;
	memset(hdl, 0, sizeof(_GTKDRAWMNG_HDL));

	hdl->d.width = width;
	hdl->d.height = height;
	hdl->d.drawing = FALSE;

	hdl->drawarea = gtk_drawing_area_new();
	gtk_widget_set_size_request(GTK_WIDGET(hdl->drawarea), width, height);

	visual = gtk_widget_get_visual(hdl->drawarea);
	if (!gtkdrawmng_getformat(hdl->drawarea, parent_window, &fmt))
		goto destroy;

	switch (fmt.bits_per_pixel) {
#if defined(SUPPORT_32BPP)
	case 32:
		break;
#endif
#if defined(SUPPORT_24BPP)
	case 24:
		break;
#endif
#if defined(SUPPORT_16BPP)
	case 16:
	case 15:
		drawmng_make16mask(&hdl->d.pal16mask, visual->blue_mask,
		    visual->red_mask, visual->green_mask);
		break;
#endif
#if defined(SUPPORT_8BPP)
	case 8:
		break;
#endif

	default:
		goto destroy;
	}
	bytes_per_pixel = fmt.bits_per_pixel / 8;

	hdl->d.dest.x = hdl->d.dest.y = 0;
	hdl->d.src.left = hdl->d.src.top = 0;
	hdl->d.src.right = width;
	hdl->d.src.bottom = height;
	hdl->d.lpitch = hdl->d.src.right * bytes_per_pixel;
	padding = hdl->d.lpitch % (fmt.scanline_pad / 8);
	if (padding > 0) {
		hdl->d.src.right += padding / bytes_per_pixel;
		hdl->d.lpitch = hdl->d.src.right * bytes_per_pixel;
	}

	/* image */
	hdl->surface = gdk_image_new(GDK_IMAGE_FASTEST, visual,
	    hdl->d.src.right, hdl->d.src.bottom);
	if (hdl->surface == NULL)
		goto destroy;

	hdl->d.vram.width = hdl->d.src.right;
	hdl->d.vram.height = hdl->d.src.bottom;
	hdl->d.vram.xalign = bytes_per_pixel;
	hdl->d.vram.yalign = hdl->d.lpitch;
	hdl->d.vram.bpp = fmt.bits_per_pixel;

	/* pixmap */
	hdl->backsurf = gdk_pixmap_new(parent_window->window,
	    hdl->d.vram.width, hdl->d.vram.height, visual->depth);
	if (hdl->backsurf == NULL)
		goto destroy;

	return (DRAWMNG_HDL)hdl;

destroy:
	if (hdl) {
		GtkWidget *da = hdl->drawarea;
		drawmng_release((DRAWMNG_HDL)hdl);
		if (da) {
			g_object_unref(da);
		}
	}
	return NULL;
}

void
drawmng_release(DRAWMNG_HDL dhdl)
{
	GTKDRAWMNG_HDL hdl = (GTKDRAWMNG_HDL)dhdl;

	if (hdl) {
		while (hdl->d.drawing)
			usleep(1);
		if (hdl->backsurf) {
			g_object_unref(hdl->backsurf);
		}
		if (hdl->surface) {
			g_object_unref(hdl->surface);
		}
		_MFREE(hdl);
	}
}

CMNVRAM *
drawmng_surflock(DRAWMNG_HDL dhdl)
{
	GTKDRAWMNG_HDL hdl = (GTKDRAWMNG_HDL)dhdl;

	if (hdl) {
		hdl->d.vram.ptr = (UINT8 *)hdl->surface->mem;
		if (hdl->d.vram.ptr) {
			hdl->d.drawing = TRUE;
			return &hdl->d.vram;
		}
	}
	return NULL;
}

void
drawmng_surfunlock(DRAWMNG_HDL dhdl)
{
	GTKDRAWMNG_HDL hdl = (GTKDRAWMNG_HDL)dhdl;
	GdkGC *gc;

	if (hdl) {
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
		gc = hdl->drawarea->style->fg_gc[gtk_widget_get_state(hdl->drawarea)];
#else
		gc = hdl->drawarea->style->fg_gc[GTK_WIDGET_STATE(hdl->drawarea)];
#endif
		gdk_draw_image(hdl->backsurf, gc, hdl->surface,
		    0, 0, 0, 0, hdl->d.width, hdl->d.height);
		hdl->d.drawing = FALSE;
	}
}

void
drawmng_blt(DRAWMNG_HDL dhdl, RECT_T *sr, POINT_T *dp)
{
	GTKDRAWMNG_HDL hdl = (GTKDRAWMNG_HDL)dhdl;
	RECT_T r;
	POINT_T p;
	GdkGC *gc;
	int width, height;

	if (hdl) {
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
		gc = hdl->drawarea->style->fg_gc[gtk_widget_get_state(hdl->drawarea)];
#else
		gc = hdl->drawarea->style->fg_gc[GTK_WIDGET_STATE(hdl->drawarea)];
#endif
		if (sr || dp) {

			if (sr) {
				r = *sr;
			} else {
				r.left = r.top = 0;
				r.right = hdl->d.width;
				r.bottom = hdl->d.height;
			}
			if (dp) {
				p = *dp;
			} else {
				p.x = p.y = 0;
			}
			width = r.right - p.x;
			height = r.bottom - p.y;

			gdk_draw_drawable(hdl->drawarea->window, gc,
			    hdl->backsurf,
			    r.left, r.top, p.x, p.y, width, height);
		} else {
			gdk_draw_drawable(hdl->drawarea->window, gc,
			    hdl->backsurf,
			    0, 0, 0, 0, hdl->d.width, hdl->d.height);
		}
	}
}

void
drawmng_set_size(DRAWMNG_HDL dhdl, int width, int height)
{
	GTKDRAWMNG_HDL hdl = (GTKDRAWMNG_HDL)dhdl;

	hdl->d.width = width;
	hdl->d.height = height;
	gtk_widget_set_size_request(hdl->drawarea, width, height);
}

void
drawmng_invalidate(DRAWMNG_HDL dhdl, RECT_T *r)
{
	GTKDRAWMNG_HDL hdl = (GTKDRAWMNG_HDL)dhdl;
	gint x, y, w, h;

	if (r == NULL) {
		gtk_widget_queue_draw(hdl->drawarea);
	} else {
		x = r->left;
		y = r->top;
		w = r->right - r->left;
		h = r->bottom - r->top;
		gtk_widget_queue_draw_area(hdl->drawarea, x, y, w, h);
	}
}

void *
drawmng_get_widget_handle(DRAWMNG_HDL dhdl)
{
	GTKDRAWMNG_HDL hdl = (GTKDRAWMNG_HDL)dhdl;

	return hdl->drawarea;
}

BOOL
gtkdrawmng_getformat(GtkWidget *w, GtkWidget *pw, pixmap_format_t *fmtp)
{
	GdkVisual *visual;

	visual = gtk_widget_get_visual(w);
	switch (visual->type) {
	case GDK_VISUAL_TRUE_COLOR:
	case GDK_VISUAL_PSEUDO_COLOR:
	case GDK_VISUAL_DIRECT_COLOR:
		break;

	default:
		g_printerr("No support visual class.\n");
		return FALSE;
	}

	switch (visual->depth) {
#if defined(SUPPORT_32BPP)
	case 32:
#endif
#if defined(SUPPORT_24BPP)
	case 24:
#endif
#if defined(SUPPORT_16BPP)
	case 16:
	case 15:
#endif
#if defined(SUPPORT_8BPP)
	case 8:
#endif
		break;

	default:
		if (visual->depth < 8) {
			g_printerr("Too few allocable color.\n");
		}
		g_printerr("No support depth.\n");
		return FALSE;
	}

	return gdk_window_get_pixmap_format(pw->window, visual, fmtp);
}
