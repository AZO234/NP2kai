/* === screen management for wx port === */

#ifndef NP2_WX_SCRNMNG_H
#define NP2_WX_SCRNMNG_H

#include <embed/vramhdl.h>
#include <vram/scrndraw.h>

enum {
	RGB24_B = 2,
	RGB24_G = 1,
	RGB24_R = 0
};

typedef struct {
	UINT8	*ptr;
	int		xalign;
	int		yalign;
	int		width;
	int		height;
	UINT	bpp;
	int		extend;
} SCRNSURF;

typedef struct {
	BOOL		enable;
	int			width;
	int			height;
	int			bpp;
	int			flag;
	UINT8		*pixbuf;      /* raw pixel buffer (RGB16 or RGB32) */
	UINT8		*pixbuf_disp; /* display pixel buffer (scaled) */
	VRAMHDL		vram;
} SCRNMNG;

enum {
	SCRNMODE_FULLSCREEN		= 0x01,
	SCRNMODE_HIGHCOLOR		= 0x02,
	SCRNMODE_ROTATE			= 0x10,
	SCRNMODE_ROTATEDIR		= 0x20,
	SCRNMODE_ROTATELEFT		= (SCRNMODE_ROTATE + 0),
	SCRNMODE_ROTATERIGHT	= (SCRNMODE_ROTATE + SCRNMODE_ROTATEDIR),
	SCRNMODE_ROTATEMASK		= 0x30,
};

enum {
	SCRNFLAG_FULLSCREEN		= 0x01,
	SCRNFLAG_HAVEEXTEND		= 0x02,
	SCRNFLAG_ENABLE			= 0x80
};

#ifdef __cplusplus
extern "C" {
#endif

extern SCRNMNG scrnmng;

/* called from core */
void scrnmng_getsize(int *pw, int *ph);
void scrnmng_setwidth(int posx, int width);
void scrnmng_setheight(int posy, int height);
const SCRNSURF *scrnmng_surflock(void);
void scrnmng_surfunlock(const SCRNSURF *surf);

#define scrnmng_setextend(e)
#define scrnmng_isfullscreen()  (scrnmng.flag & SCRNFLAG_FULLSCREEN)
#define scrnmng_haveextend()    (0)
#define scrnmng_getbpp()        (16)
#define scrnmng_allflash()
#define scrnmng_palchanged()

RGB16 scrnmng_makepal16(RGB32 pal32);

/* lifecycle */
void    scrnmng_initialize(void);
BRESULT scrnmng_create(UINT8 mode);
void    scrnmng_destroy(void);

/* redraw notification - called from emulation, posts to UI */
void scrnmng_update(void);
void scrnmng_updatecursor(void);

/* WAB / HDD blit stubs */
void scrnmng_blthdc(void);
void scrnmng_bltwab(void);
void scrnmng_updatefsres(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* Called from the wx panel to get the current pixel buffer for painting */
const UINT8 *scrnmng_getpixbuf(int *width, int *height, int *bpp);
void         scrnmng_lockbuf(void);
void         scrnmng_unlockbuf(void);
#endif

#endif /* NP2_WX_SCRNMNG_H */
