#ifndef _SCRNMNG_H_
#define _SCRNMNG_H_

enum {
	RGB24_B	= 0,
	RGB24_G	= 1,
	RGB24_R	= 2
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

enum {
	DRAWTYPE_DIRECTDRAW_HW	= 0x00,
	DRAWTYPE_DIRECTDRAW_SW	= 0x01,
	DRAWTYPE_DIRECT3D		= 0x02,
	DRAWTYPE_INVALID		= 0xff
};

enum {
	D3D_IMODE_NEAREST_NEIGHBOR	= 0x00,
	D3D_IMODE_BILINEAR	= 0x01,
	D3D_IMODE_PIXEL		= 0x02,
	D3D_IMODE_PIXEL2	= 0x03,
	D3D_IMODE_PIXEL3	= 0x04,
};

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

enum {
	FSCRNMOD_NORESIZE		= 0x00,
	FSCRNMOD_ASPECTFIX8		= 0x01,
	FSCRNMOD_ASPECTFIX		= 0x02,
	FSCRNMOD_LARGE			= 0x03,
	FSCRNMOD_ASPECTMASK		= 0x13,
	FSCRNMOD_SAMERES		= 0x04,
	FSCRNMOD_SAMEBPP		= 0x08,
	FSCRNMOD_INTMULTIPLE	= 0x10,
	FSCRNMOD_FORCE43		= 0x11
};

typedef struct {
	UINT8	flag;
	UINT8	bpp;
	UINT8	allflash;
	UINT8	palchanged;
	UINT8	forcereset;
} SCRNMNG;

typedef struct {
	int		width;
	int		height;
	int		extend;
	int		multiple;
} SCRNSTAT;

typedef struct {
	UINT8	hasfscfg;
	UINT8	fscrnmod;
	UINT8	scrn_mul;
	UINT8	d3d_imode;
} SCRNRESCFG;


#ifdef __cplusplus
extern "C" {
#endif

extern	SCRNMNG		scrnmng;			// マクロ用
extern	SCRNSTAT	scrnstat;
extern	SCRNRESCFG	scrnrescfg;

#define	FSCRNCFG_fscrnmod	(np2oscfg.fsrescfg && scrnrescfg.hasfscfg ? scrnrescfg.fscrnmod : np2oscfg.fscrnmod)
#define	FSCRNCFG_d3d_imode	(np2oscfg.fsrescfg && scrnrescfg.hasfscfg ? scrnrescfg.d3d_imode : np2oscfg.d3d_imode)

void scrnres_readini();
void scrnres_readini_res(int width, int height);
void scrnres_writeini();

extern UINT8 scrnmng_current_drawtype;

void scrnmng_setwindowsize(HWND hWnd, int width, int height);

void scrnmng_initialize(void);
BRESULT scrnmng_create(UINT8 scrnmode);
void scrnmng_destroy(void);
void scrnmng_shutdown(void);

void scrnmng_setwidth(int posx, int width);
void scrnmng_setextend(int extend);
void scrnmng_setheight(int posy, int height);
void scrnmng_setsize(int posx, int posy, int width, int height);
#define scrnmng_setbpp(commendablebpp)
const SCRNSURF *scrnmng_surflock(void);
void scrnmng_surfunlock(const SCRNSURF *surf);
void scrnmng_update(void);

#define	scrnmng_isfullscreen()	(scrnmng.flag & SCRNFLAG_FULLSCREEN)
#define	scrnmng_haveextend()	(scrnmng.flag & SCRNFLAG_HAVEEXTEND)
#define	scrnmng_getbpp()		(scrnmng.bpp)
#define	scrnmng_allflash()		scrnmng.allflash = TRUE
#define	scrnmng_palchanged()	scrnmng.palchanged = TRUE

RGB16 scrnmng_makepal16(RGB32 pal32);


// ---- for windows

void scrnmng_setmultiple(int multiple);
int scrnmng_getmultiple(void);
void scrnmng_querypalette(void);
void scrnmng_setdefaultres(void);
void scrnmng_setfullscreen(BOOL fullscreen);
void scrnmng_setrotatemode(UINT type);
void scrnmng_fullscrnmenu(int y);
void scrnmng_topwinui(void);
void scrnmng_clearwinui(void);

void scrnmng_entersizing(void);
void scrnmng_sizing(UINT side, RECT *rect);
void scrnmng_exitsizing(void);

void scrnmng_updatefsres(void);
void scrnmng_blthdc(HDC hdc);
void scrnmng_bltwab(void);

void scrnmng_getrect(RECT *lpRect);

void scrnmng_delaychangemode();
void scrnmng_UIThreadProc();

#if defined(SUPPORT_DCLOCK)
BOOL scrnmng_isdispclockclick(const POINT *pt);
void scrnmng_dispclock(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  // _SCRNMNG_H_
