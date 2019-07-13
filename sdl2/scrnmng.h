#ifndef NP2_SDL2_SCRNMNG_H
#define NP2_SDL2_SCRNMNG_H

#include	"vramhdl.h"
#include	"scrndraw.h"

enum {
	RGB24_B	= 2,
	RGB24_G	= 1,
	RGB24_R	= 0
};

typedef struct {
	UINT8	*ptr;
	int	xalign;
	int	yalign;
	int	width;
	int	height;
	UINT	bpp;
	int	extend;
} SCRNSURF;

enum {
	SCRNMODE_FULLSCREEN	= 0x01,
	SCRNMODE_HIGHCOLOR	= 0x02,
	SCRNMODE_ROTATE		= 0x10,
	SCRNMODE_ROTATEDIR	= 0x20,
	SCRNMODE_ROTATELEFT	= (SCRNMODE_ROTATE + 0),
	SCRNMODE_ROTATERIGHT	= (SCRNMODE_ROTATE + SCRNMODE_ROTATEDIR),
	SCRNMODE_ROTATEMASK	= 0x30,
};

enum {
	SCRNFLAG_FULLSCREEN	= 0x01,
	SCRNFLAG_HAVEEXTEND	= 0x02,
	SCRNFLAG_ENABLE		= 0x80
};

#ifdef __cplusplus
extern "C" {
#endif

void scrnmng_getsize(int* pw, int* ph);
void scrnmng_setwidth(int posx, int width);
#define scrnmng_setextend(e)
void scrnmng_setheight(int posy, int height);
const SCRNSURF *scrnmng_surflock(void);
void scrnmng_surfunlock(const SCRNSURF *surf);

#define	scrnmng_isfullscreen()	(0)
#define	scrnmng_haveextend()	(0)
#define	scrnmng_getbpp()		(16)
#define	scrnmng_allflash()		
#define	scrnmng_palchanged()	

RGB16 scrnmng_makepal16(RGB32 pal32);

#ifdef __cplusplus
}
#endif


// ---- for SDL

void scrnmng_initialize(void);
BRESULT scrnmng_create(UINT8 mode);
void scrnmng_destroy(void);


// ---- for menubase

typedef struct {
	int		width;
	int		height;
	int		bpp;
} SCRNMENU;

BRESULT scrnmng_entermenu(SCRNMENU *smenu);
void scrnmng_leavemenu(void);
void scrnmng_menudraw(const RECT_T *rct);
void scrnmng_update(void);
void scrnmng_updatecursor(void);

void scrnmng_updatefsres(void);
void scrnmng_blthdc(void);
void scrnmng_bltwab(void);

#endif	/* NP2_SDL2_SCRNMNG_H */

