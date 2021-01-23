
#pragma once

void winloc_setclientsize(HWND hwnd, int width, int height);

void winloc_getDWMmargin(HWND hwnd, int *ofsx, int *ofsy);


// ----

typedef struct {
	UINT	flag;
	int		gx;
	int		gy;
	int		tx;
	int		ty;
} WINLOC;

void winloc_movingstart(WINLOC *wl);
void winloc_movingproc(WINLOC *wl, RECT *rect);


// ----

typedef struct {
	HWND	hwnd;
	RECT	rect;
	UINT8	connect;
	UINT8	padding;
	UINT16	parent;
} WLEXWND;

typedef struct {
	UINT	flagx;
	UINT	flagy;
	int		gx;
	int		gy;
	int		tx;
	int		ty;
	HWND	base;
	RECT	rect;
	HWND	hold;
	UINT	holdflag;
	UINT	count;
} _WINLOCEX, *WINLOCEX;

WINLOCEX winlocex_create(HWND base, const HWND *child, UINT count);
void winlocex_destroy(WINLOCEX wle);
void winlocex_setholdwnd(WINLOCEX wle, HWND hold);
void winlocex_moving(WINLOCEX wle, RECT *rect);
void winlocex_move(WINLOCEX wle);
void winlocex_close(WINLOCEX wle);

BOOL winloc_InitDwmFunc();
void winloc_DisposeDwmFunc();

