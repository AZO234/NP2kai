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

#ifndef	NP2_X11_DRAWMNG_H__
#define	NP2_X11_DRAWMNG_H__

#include "compiler.h"

#include "cmndraw.h"

G_BEGIN_DECLS

typedef struct {
	RGB32	mask;
	UINT8	r16b;
	UINT8	l16r;
	UINT8	l16g;
} PAL16MASK;

typedef struct {
	CMNVRAM		vram;

	int		width;
	int		height;
	int		lpitch;

	RECT_T		src;
	POINT_T		dest;

	PAL16MASK	pal16mask;
	BOOL		drawing;
} _DRAWMNG_HDL, *DRAWMNG_HDL;

DRAWMNG_HDL drawmng_create(void *parent, int width, int height);
void drawmng_release(DRAWMNG_HDL hdl);
CMNVRAM *drawmng_surflock(DRAWMNG_HDL hdl);
void drawmng_surfunlock(DRAWMNG_HDL hdl);
void drawmng_blt(DRAWMNG_HDL hdl, RECT_T *sr, POINT_T *dp);
void drawmng_set_size(DRAWMNG_HDL hdl, int width, int height);
void drawmng_invalidate(DRAWMNG_HDL hdl, RECT_T *r);
void *drawmng_get_widget_handle(DRAWMNG_HDL hdl);

void drawmng_make16mask(PAL16MASK *pal16, UINT32 bmask, UINT32 rmask, UINT32 gmask);
RGB16 drawmng_makepal16(PAL16MASK *pal16, RGB32 pal32);

G_END_DECLS

#endif	/* NP2_X11_DRAWMNG_H__ */
