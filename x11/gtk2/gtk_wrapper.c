/*
 * Copyright (c) 2002-2013 NONAKA Kimihiro
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "gtk2/xnp2.h"

#include <gdk/gdkx.h>

extern int verbose;
extern volatile sig_atomic_t np2running;

#ifdef DEBUG
#ifndef	VERBOSE
#define	VERBOSE(s)	if (verbose) g_printerr s
#endif	/* !VERBOSE */
#else	/* !DEBUG */
#define	VERBOSE(s)
#endif	/* DEBUG */

void
gtk_scale_set_default_values(GtkScale *scale)
{

	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_scale_set_digits(scale, 1);
	gtk_scale_set_value_pos(scale, GTK_POS_RIGHT);
	gtk_scale_set_draw_value(scale, TRUE);
}

void
gdk_window_set_pointer(GdkWindow *w, gint x, gint y)
{ 
	Display *xdisplay;
	Window xwindow;

	g_return_if_fail(w != NULL);

	xdisplay = GDK_WINDOW_XDISPLAY(w);
	xwindow = GDK_WINDOW_XWINDOW(w);
	XWarpPointer(xdisplay, None, xwindow, 0, 0, 0, 0, x, y);
}

gboolean
gdk_window_get_pixmap_format(GdkWindow *w, GdkVisual *visual, pixmap_format_t *fmtp)
{
	Display *xdisplay;
	XPixmapFormatValues *format;
	int count;
	int i;

	g_return_val_if_fail(w != NULL, FALSE);
	g_return_val_if_fail(visual != NULL, FALSE);
	g_return_val_if_fail(fmtp != NULL, FALSE);

	xdisplay = GDK_WINDOW_XDISPLAY(w);
	format = XListPixmapFormats(xdisplay, &count);
	if (format) {
		for (i = 0; i < count; i++) {
			if (visual->depth == format[i].depth) {
				fmtp->depth = format[i].depth;
				fmtp->bits_per_pixel = format[i].bits_per_pixel;
				fmtp->scanline_pad = format[i].scanline_pad;
				XFree(format);
				return TRUE;
			}
		}
		XFree(format);
	}
	return FALSE;
}

/*
 * Full screen support.
 */
extern int ignore_fullscreen_mode;
static int use_xvid;
static int use_netwm;
static int is_fullscreen;

#ifdef HAVE_XF86VIDMODE
#include <X11/extensions/xf86vmode.h>

static XF86VidModeModeInfo **modes = NULL;
static int modeidx = -1;
static XF86VidModeModeInfo *saved_modes;
static XF86VidModeModeInfo orig_mode;
static Display *fs_xdisplay;
static int fs_xscreen;
static int view_x, view_y;
static gint orig_x, orig_y;

static inline Bool
XF86VidModeGetModeInfo(Display *d, int s, XF86VidModeModeInfo *info)
{
	XF86VidModeModeLine line;
	int dotclock;

	memset(info, 0, sizeof(*info));

	Bool ret = XF86VidModeGetModeLine(d, s, &dotclock, &line);
	if (ret) {
		info->dotclock = dotclock;
		info->hdisplay = line.hdisplay;
		info->hsyncstart = line.hsyncstart;
		info->hsyncend = line.hsyncend;
		info->htotal = line.htotal;
		info->vdisplay = line.vdisplay;
		info->vsyncstart = line.vsyncstart;
		info->vsyncend = line.vsyncend;
		info->vtotal = line.vtotal;
		info->flags = line.flags;
		info->privsize = line.privsize;
		info->private = line.private;
	}
	return ret;
}

static int
check_xvid(GtkWidget *widget)
{
	gboolean ret = FALSE;
	GdkWindow *w;
	Display *xdisplay;
	int xscreen;
	XF86VidModeModeInfo mode;
	int event_base, error_base;
	int major_ver, minor_ver;
	int nmodes;
	int i;
	Bool rv;

	g_return_val_if_fail(widget != NULL, FALSE);

	w = widget->window;
	xdisplay = GDK_WINDOW_XDISPLAY(w);
	xscreen = XDefaultScreen(xdisplay);

	XLockDisplay(xdisplay);

	rv = XF86VidModeQueryExtension(xdisplay, &event_base, &error_base);
	if (!rv) {
		goto out;
	}

	rv = XF86VidModeQueryVersion(xdisplay, &major_ver, &minor_ver);
	if (!rv) {
		goto out;
	}
	VERBOSE(("XF86VidMode Extension: ver.%d.%d detected\n",
	    major_ver, minor_ver));

	rv = XF86VidModeGetModeInfo(xdisplay, xscreen, &mode);
	if (rv) {
		if ((mode.hdisplay == 640) && (mode.vdisplay == 480)) {
			orig_mode = mode;
			saved_modes = &orig_mode;
			modes = &saved_modes;
			modeidx = 0;
			ret = TRUE;
			goto out;
		}
	}

	rv = XF86VidModeGetAllModeLines(xdisplay, xscreen, &nmodes, &modes);
	if (!rv) {
		goto out;
	}
	VERBOSE(("XF86VidMode Extension: %d modes\n", nmodes));

	for (i = 0; i < nmodes; i++) {
		VERBOSE(("XF86VidMode Extension: %d: %dx%d\n", i,
		    modes[i]->hdisplay, modes[i]->vdisplay));

		if ((modes[i]->hdisplay == 640)
		 && (modes[i]->vdisplay == 480)) {
			rv = XF86VidModeGetModeInfo(xdisplay, xscreen,
			    &orig_mode);
			if (rv) {
				VERBOSE(("found\n"));
				modeidx = i;
				ret = TRUE;
				break;
			}
		}
	}

	if (ret) {
		fs_xdisplay = xdisplay;
		fs_xscreen = xscreen;
	} else {
		XFree(modes);
		modes = NULL;
	}

out:
	XUnlockDisplay(xdisplay);

	return ret;
}
#endif	/* HAVE_XF86VIDMODE */

static int
check_netwm(GtkWidget *widget)
{
	Display *xdisplay;
	Window root_window;
	Atom _NET_SUPPORTED;
	Atom _NET_WM_STATE_FULLSCREEN;
	Atom type;
	int format;
	unsigned long nitems;
	unsigned long remain;
	unsigned char *prop;
	guint32 *data;
	int rv;
	unsigned long i;

	g_return_val_if_fail(widget != NULL, 0);

	xdisplay = GDK_WINDOW_XDISPLAY(widget->window);
	root_window = DefaultRootWindow(xdisplay);

	_NET_SUPPORTED = XInternAtom(xdisplay, "_NET_SUPPORTED", False);
	_NET_WM_STATE_FULLSCREEN = XInternAtom(xdisplay,
	    "_NET_WM_STATE_FULLSCREEN", False);

	rv = XGetWindowProperty(xdisplay, root_window, _NET_SUPPORTED,
	    0, 65536 / sizeof(guint32), False, AnyPropertyType,
	    &type, &format, &nitems, &remain, &prop);
	if (rv != Success) {
		return 0;
	}
	if (type != XA_ATOM) {
		return 0;
	}
	if (format != 32) {
		return 0;
	}

	rv = 0;
	data = (guint32 *)prop;
	for (i = 0; i < nitems; i++) {
		if (data[i] == _NET_WM_STATE_FULLSCREEN) {
			VERBOSE(("Support _NET_WM_STATE_FULLSCREEN\n"));
			rv = 1;
			break;
		}
	}
	XFree(prop);

	return rv;
}

int
gtk_window_init_fullscreen(GtkWidget *widget)
{

#ifdef HAVE_XF86VIDMODE
	use_xvid = check_xvid(widget);
#endif
	use_netwm = check_netwm(widget);

	if (use_xvid && (ignore_fullscreen_mode & 1)) {
		VERBOSE(("Support XF86VidMode extension, but disabled\n"));
		use_xvid = 0;
	}
	if (use_netwm && (ignore_fullscreen_mode & 2)) {
		VERBOSE(("Support _NET_WM_STATE_FULLSCREEN, but disabled\n"));
		use_netwm = 0;
	}

	if (verbose) {
		if (use_xvid) {
			VERBOSE(("Using XF86VidMode extension\n"));
		} else if (use_netwm) {
			VERBOSE(("Using _NET_WM_STATE_FULLSCREEN\n"));
		} else {
			VERBOSE(("not supported\n"));
		}
	}

	return use_xvid;
}

void
gtk_window_fullscreen_mode(GtkWidget *widget)
{

	g_return_if_fail(widget != NULL);
	g_return_if_fail(widget->window != NULL);

#ifdef HAVE_XF86VIDMODE
	if (use_xvid && modes != NULL && modeidx >= 0) {
		GtkWindow *window = GTK_WINDOW(widget);

		XLockDisplay(fs_xdisplay);

		XF86VidModeLockModeSwitch(fs_xdisplay, fs_xscreen, True);
		XF86VidModeGetViewPort(fs_xdisplay,fs_xscreen,&view_x,&view_y);
		gdk_window_get_origin(widget->window, &orig_x, &orig_y);
		if (window)
			gtk_window_move(window, 0, 0);
		XF86VidModeSwitchToMode(fs_xdisplay,fs_xscreen,modes[modeidx]);

		XUnlockDisplay(fs_xdisplay);
	}
#endif	/* HAVE_XF86VIDMODE */
	if (use_netwm) {
		gtk_window_fullscreen(GTK_WINDOW(widget));
	}
	is_fullscreen = 1;
}

void
gtk_window_restore_mode(GtkWidget *widget)
{

	g_return_if_fail(widget != NULL);

	if (!is_fullscreen)
		return;
	is_fullscreen = 0;

#ifdef HAVE_XF86VIDMODE
	if (use_xvid) {
		XF86VidModeModeInfo mode;
		int rv;

		if ((orig_mode.hdisplay == 0) || (orig_mode.vdisplay == 0))
			return;

		XLockDisplay(fs_xdisplay);

		rv = XF86VidModeGetModeInfo(fs_xdisplay, fs_xscreen, &mode);
		if (rv) {
			if ((orig_mode.hdisplay != mode.hdisplay)
			 || (orig_mode.vdisplay != mode.vdisplay)) {
				XF86VidModeSwitchToMode(fs_xdisplay, fs_xscreen,
				    &orig_mode);
				XF86VidModeLockModeSwitch(fs_xdisplay,
				    fs_xscreen, False);
			}
			if ((view_x != 0) || (view_y != 0)) {
				XF86VidModeSetViewPort(fs_xdisplay, fs_xscreen,
				    view_x, view_y);
			}
		}

		if (np2running && GTK_IS_WINDOW(widget)) {
			gtk_window_move(GTK_WINDOW(widget), orig_x, orig_y);
		}

		XUnlockDisplay(fs_xdisplay);
	}
#endif	/* HAVE_XF86VIDMODE */
	if (use_netwm && GTK_IS_WINDOW(widget)) {
		gtk_window_unfullscreen(GTK_WINDOW(widget));
	}
}
