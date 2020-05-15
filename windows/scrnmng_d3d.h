
#ifdef __cplusplus
extern "C" {
#endif
	
BRESULT scrnmngD3D_check(void);

BRESULT scrnmngD3D_create(UINT8 scrnmode);
void scrnmngD3D_destroy(void);
void scrnmngD3D_shutdown(void);

void scrnmngD3D_setwidth(int posx, int width);
void scrnmngD3D_setextend(int extend);
void scrnmngD3D_setheight(int posy, int height);
void scrnmngD3D_setsize(int posx, int posy, int width, int height);
#define scrnmngD3D_setbpp(commendablebpp)
const SCRNSURF *scrnmngD3D_surflock(void);
void scrnmngD3D_surfunlock(const SCRNSURF *surf);
void scrnmngD3D_update(void);

RGB16 scrnmngD3D_makepal16(RGB32 pal32);


// ---- for windows

void scrnmngD3D_setmultiple(int multiple);
int scrnmngD3D_getmultiple(void);
void scrnmngD3D_querypalette(void);
void scrnmngD3D_setdefaultres(void);
void scrnmngD3D_setfullscreen(BOOL fullscreen);
void scrnmngD3D_setrotatemode(UINT type);
void scrnmngD3D_fullscrnmenu(int y);
void scrnmngD3D_topwinui(void);
void scrnmngD3D_clearwinui(void);

void scrnmngD3D_entersizing(void);
void scrnmngD3D_sizing(UINT side, RECT *rect);
void scrnmngD3D_exitsizing(void);

void scrnmngD3D_updatefsres(void);
void scrnmngD3D_blthdc(HDC hdc);
void scrnmngD3D_bltwab(void);

void scrnmngD3D_getrect(RECT *lpRect);

#if defined(SUPPORT_DCLOCK)
BOOL scrnmngD3D_isdispclockclick(const POINT *pt);
void scrnmngD3D_dispclock(void);
#endif

#ifdef __cplusplus
}
#endif
