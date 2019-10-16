
#ifdef __cplusplus
extern "C" {
#endif

BRESULT scrnmngDD_create(UINT8 scrnmode);
void scrnmngDD_destroy(void);
void scrnmngDD_shutdown(void);

void scrnmngDD_setwidth(int posx, int width);
void scrnmngDD_setextend(int extend);
void scrnmngDD_setheight(int posy, int height);
void scrnmngDD_setsize(int posx, int posy, int width, int height);
#define scrnmngDD_setbpp(commendablebpp)
const SCRNSURF *scrnmngDD_surflock(void);
void scrnmngDD_surfunlock(const SCRNSURF *surf);
void scrnmngDD_update(void);

RGB16 scrnmngDD_makepal16(RGB32 pal32);


// ---- for windows

void scrnmngDD_setmultiple(int multiple);
int scrnmngDD_getmultiple(void);
void scrnmngDD_querypalette(void);
void scrnmngDD_setdefaultres(void);
void scrnmngDD_setfullscreen(BOOL fullscreen);
void scrnmngDD_setrotatemode(UINT type);
void scrnmngDD_fullscrnmenu(int y);
void scrnmngDD_topwinui(void);
void scrnmngDD_clearwinui(void);

void scrnmngDD_entersizing(void);
void scrnmngDD_sizing(UINT side, RECT *rect);
void scrnmngDD_exitsizing(void);

void scrnmngDD_updatefsres(void);
void scrnmngDD_blthdc(HDC hdc);
void scrnmngDD_bltwab(void);

void scrnmngDD_getrect(RECT *lpRect);

#if defined(SUPPORT_DCLOCK)
BOOL scrnmngDD_isdispclockclick(const POINT *pt);
void scrnmngDD_dispclock(void);
#endif

#ifdef __cplusplus
}
#endif

