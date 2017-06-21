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

#ifndef	NP2_GTK2_XNP2_H__
#define	NP2_GTK2_XNP2_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

extern GtkWidget *main_window;
extern GtkWidget *drawarea;

typedef struct {
	int depth;
	int bits_per_pixel;
	int scanline_pad;
} pixmap_format_t;

void install_idle_process(void);
void uninstall_idle_process(void);

void gtk_scale_set_default_values(GtkScale *scale);
void gdk_window_set_pointer(GdkWindow *w, gint x, gint y);
gboolean gdk_window_get_pixmap_format(GdkWindow *w, GdkVisual *visual, pixmap_format_t *fmtp);
gboolean gtk_window_init_fullscreen(GtkWidget *widget);
void gtk_window_fullscreen_mode(GtkWidget *widget);
void gtk_window_restore_mode(GtkWidget *widget);

G_END_DECLS

#endif /* NP2_GTK2_XNP2_H__ */
