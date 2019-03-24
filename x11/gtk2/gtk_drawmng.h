/*
 * Copyright (c) 2003, 2004 NONAKA Kimihiro
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

#ifndef	NP2_X11_GTK2_GTKDRAWMNG_H__
#define	NP2_X11_GTK2_GTKDRAWMNG_H__

#include "compiler.h"

#include "drawmng.h"

#include "gtk2/xnp2.h"

G_BEGIN_DECLS

typedef struct {
	_DRAWMNG_HDL	d;

	GtkWidget	*drawarea;
	GdkImage	*surface;
	GdkPixmap	*backsurf;
} _GTKDRAWMNG_HDL, *GTKDRAWMNG_HDL;

BOOL gtkdrawmng_getformat(GtkWidget *w, GtkWidget *pw, pixmap_format_t *fmtp);

G_END_DECLS

#endif	/* NP2_X11_GTK2_GTKDRAWMNG_H__ */
