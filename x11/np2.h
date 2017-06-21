#ifndef	NP2_X11_NP2_H__
#define	NP2_X11_NP2_H__

#include <signal.h>

#include "joymng.h"

G_BEGIN_DECLS

typedef struct {
	UINT8	port;
	UINT8	def_en;
	UINT8	param;
	UINT32	speed;
	char	mout[MAX_PATH];
	char	min[MAX_PATH];
	char	mdl[64];
	char	def[MAX_PATH];
} COMCFG;

typedef struct {
	char	titles[256];

	UINT	paddingx;
	UINT	paddingy;

	UINT8	NOWAIT;
	UINT8	DRAW_SKIP;

	UINT8	DISPCLK;

	UINT8	KEYBOARD;
	UINT8	F12KEY;

	UINT8	MOUSE_SW;
	UINT8	JOYPAD1;
	UINT8	JOYPAD2;
	UINT8	JOY1BTN[JOY_NBUTTON];
	UINT8	JOYAXISMAP[2][JOY_NAXIS];
	UINT8	JOYBTNMAP[2][JOY_NBUTTON];
	char	JOYDEV[2][MAX_PATH];

	COMCFG	mpu;
	COMCFG	com[3];

	UINT8	confirm;

	UINT8	resume;						// ver0.30

	UINT8	statsave;
	UINT8	toolwin;
	UINT8	keydisp;
	UINT8	softkbd;
	UINT8	hostdrv_write;
	UINT8	jastsnd;
	UINT8	I286SAVE;

	UINT8	snddrv;
	char	MIDIDEV[2][MAX_PATH];
	UINT32	MIDIWAIT;

	UINT8	mouse_move_ratio;

	UINT8	disablemmx;
	UINT8	drawinterp;
	UINT8	F11KEY;

	UINT8	cfgreadonly;
} NP2OSCFG;


enum {
	SCREEN_WBASE		= 80,
	SCREEN_HBASE		= 50,
	SCREEN_DEFMUL		= 8,
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 480
};

enum {
	MMXFLAG_DISABLE		= 1,
	MMXFLAG_NOTSUPPORT	= 2
};

enum {
	INTERP_NEAREST		= 0,
	INTERP_TILES		= 1,
	INTERP_BILINEAR		= 2,
	INTERP_HYPER		= 3
};

/* np2.c */
extern volatile sig_atomic_t np2running;
extern NP2OSCFG np2oscfg;
extern UINT8 scrnmode;
extern int ignore_fullscreen_mode;

extern UINT framecnt;
extern UINT waitcnt;
extern UINT framemax;

extern BOOL s98logging;
extern int s98log_count;

extern int verbose;

extern char hddfolder[MAX_PATH];
extern char fddfolder[MAX_PATH];
extern char bmpfilefolder[MAX_PATH];
extern char modulefile[MAX_PATH];
extern char statpath[MAX_PATH];
extern char fontname[1024];

extern const char np2flagext[];
extern const char np2resumeext[];

int flagload(const char* ext, const char* title, BOOL force);
int flagsave(const char* ext);
void flagdelete(const char* ext);

void changescreen(UINT8 newmode);
void framereset(UINT cnt);
void processwait(UINT cnt);
int mainloop(void *);

extern int mmxflag;
int havemmx(void);

G_END_DECLS

#endif	/* NP2_X11_NP2_H__ */
