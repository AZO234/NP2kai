/* === screen management for wx port === */

#include <compiler.h>
#include "np2.h"
#include "scrnmng.h"
#include "mousemng.h"
#include <pccore.h>
#include <vram/scrndraw.h>
#include <embed/vramhdl.h>

#if defined(SUPPORT_WAB)
#include <wab/wab.h>
#endif

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

SCRNMNG scrnmng;

static SCRNSURF scrnsurf;
static pthread_mutex_t scrnmutex = PTHREAD_MUTEX_INITIALIZER;

/* forward declarations for UI notification (implemented in np2frame.cpp) */
extern "C" void np2frame_requestRedraw(void);

/* ---- utilities ---- */

static BRESULT calcdrawrect(int *srcpos, int *dstpos, int *w, int *h, const RECT_T *rt)
{
	*srcpos = 0;
	*dstpos = 0;
	*w = scrnmng.width;
	*h = scrnmng.height;

	if (rt) {
		int pos;
		pos = MAX(rt->left, 0);
		*srcpos += pos;
		*dstpos += pos * (scrnmng.bpp / 8);
		*w = MIN(rt->right, *w) - pos;

		pos = MAX(rt->top, 0);
		*srcpos += pos * scrnmng.width;
		*dstpos += pos * scrnmng.width * (scrnmng.bpp / 8);
		*h = MIN(rt->bottom, *h) - pos;
	}
	return ((*w > 0) && (*h > 0)) ? SUCCESS : FAILURE;
}

/* ---- public interface ---- */

void scrnmng_initialize(void)
{
	memset(&scrnmng, 0, sizeof(scrnmng));
	scrnmng.width  = 640;
	scrnmng.height = 400;
	scrnmng.bpp    = 16;
}

BRESULT scrnmng_create(UINT8 mode)
{
	size_t bufsz;

	scrnmng.bpp = draw32bit ? 32 : 16;

	if (mode & SCRNMODE_FULLSCREEN) {
		scrnmng.flag |= SCRNFLAG_FULLSCREEN;
	} else {
		scrnmng.flag &= ~SCRNFLAG_FULLSCREEN;
	}

	bufsz = (size_t)scrnmng.width * scrnmng.height * (scrnmng.bpp / 8);
	free(scrnmng.pixbuf);
	scrnmng.pixbuf = (UINT8 *)malloc(bufsz);
	if (!scrnmng.pixbuf) {
		return FAILURE;
	}
	memset(scrnmng.pixbuf, 0, bufsz);

	scrnmng.vram = vram_create(scrnmng.width, scrnmng.height, FALSE, scrnmng.bpp);
	if (!scrnmng.vram) {
		free(scrnmng.pixbuf);
		scrnmng.pixbuf = NULL;
		return FAILURE;
	}

	scrnmng.flag |= SCRNFLAG_ENABLE;
	scrnmng.enable = TRUE;
	return SUCCESS;
}

void scrnmng_destroy(void)
{
	scrnmng.enable = FALSE;
	scrnmng.flag   = 0;
	if (scrnmng.vram) {
		vram_destroy(scrnmng.vram);
		scrnmng.vram = NULL;
	}
	free(scrnmng.pixbuf);
	scrnmng.pixbuf = NULL;
}

void scrnmng_getsize(int *pw, int *ph)
{
	*pw = scrnmng.width;
	*ph = scrnmng.height;
}

void scrnmng_setwidth(int posx, int width)
{
	(void)posx;
	if (width != scrnmng.width) {
		scrnmng.width = width;
	}
}

void scrnmng_setheight(int posy, int height)
{
	(void)posy;
	if (height != scrnmng.height) {
		scrnmng.height = height;
	}
}

const SCRNSURF *scrnmng_surflock(void)
{
	if (!scrnmng.enable || !scrnmng.pixbuf) {
		return NULL;
	}
	pthread_mutex_lock(&scrnmutex);
	scrnsurf.ptr    = scrnmng.pixbuf;
	scrnsurf.xalign = scrnmng.bpp / 8;
	scrnsurf.yalign = scrnmng.width * scrnsurf.xalign;
	scrnsurf.width  = scrnmng.width;
	scrnsurf.height = scrnmng.height;
	scrnsurf.bpp    = scrnmng.bpp;
	scrnsurf.extend = 0;
	return &scrnsurf;
}

void scrnmng_surfunlock(const SCRNSURF *surf)
{
	(void)surf;
	pthread_mutex_unlock(&scrnmutex);
	scrnmng_update();
}

RGB16 scrnmng_makepal16(RGB32 pal32)
{
	return (RGB16)(
		((pal32.p.r >> 3) << 11) |
		((pal32.p.g >> 2) <<  5) |
		((pal32.p.b >> 3)      )
	);
}

void scrnmng_update(void)
{
	/* post redraw request to the UI thread */
	np2frame_requestRedraw();
}

void scrnmng_updatecursor(void)
{
	/* nothing needed for wx port */
}

void scrnmng_blthdc(void)   { /* stub */ }
void scrnmng_bltwab(void)   { /* stub */ }
void scrnmng_updatefsres(void) { /* stub */ }

/* ---- called from wx panel ---- */

const UINT8 *scrnmng_getpixbuf(int *width, int *height, int *bpp)
{
	*width  = scrnmng.width;
	*height = scrnmng.height;
	*bpp    = scrnmng.bpp;
	return scrnmng.pixbuf;
}

void scrnmng_lockbuf(void)
{
	pthread_mutex_lock(&scrnmutex);
}

void scrnmng_unlockbuf(void)
{
	pthread_mutex_unlock(&scrnmutex);
}
