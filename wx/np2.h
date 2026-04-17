/* === NP2kai wx port main header === */

#ifndef NP2_WX_NP2_H
#define NP2_WX_NP2_H

#include <compiler.h>
#include <commng.h>
#include <joymng.h>

typedef struct {
	UINT8	direct;
	UINT8	port;
	UINT8	def_en;
	UINT8	param;
	UINT32	speed;
	char	mout[MAX_PATH];
	char	min[MAX_PATH];
	char	mdl[64];
	char	def[MAX_PATH];
} COMCFG;

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
#if defined(SUPPORT_SMPU98)
	COMCFG	smpuA;
	COMCFG	smpuB;
#endif
	COMCFG	com[3];

	UINT8	confirm;

	UINT8	resume;

	UINT8	statsave;
	UINT8	toolwin;
	UINT8	keydisp;
	UINT8	softkbd;
	UINT8	hostdrv_write;
	UINT8	jastsnd;
	UINT8	I286SAVE;
	UINT8	xrollkey;

	UINT8	snddrv;
	char	MIDIDEV[2][MAX_PATH];
#if defined(SUPPORT_SMPU98)
	char	MIDIDEVA[2][MAX_PATH];
	char	MIDIDEVB[2][MAX_PATH];
#endif
	UINT32	MIDIWAIT;

	UINT8	mouse_move_ratio;

	UINT8	disablemmx;
	UINT8	drawinterp;
	UINT8	F11KEY;

	UINT8	readonly;

	/* wx-specific: window position/size */
	int		winx;
	int		winy;
	UINT	winwidth;
	UINT	winheight;
} NP2OSCFG;

enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 400
};

extern NP2OSCFG	np2oscfg;

extern BOOL s98logging;
extern int  s98log_count;

extern char hddfolder[MAX_PATH];
extern char fddfolder[MAX_PATH];
extern char bmpfilefolder[MAX_PATH];
extern UINT bmpfilenumber;
extern char modulefile[MAX_PATH];
extern char draw32bit;
extern UINT8 scrnmode;
extern UINT8 changescreeninit;

int findCdromDrive(void);

#ifdef __cplusplus
extern "C" {
#endif

int  flagsave(const OEMCHAR *ext);
void flagdelete(const OEMCHAR *ext);
int  flagload(const OEMCHAR *ext, const OEMCHAR *title, BOOL force);
void changescreen(UINT8 newmode);

extern int  mmxflag;
int  havemmx(void);

BRESULT np2_initialize(const char *argv0);
void    np2_terminate(void);
void    np2_exec(void);

/* clipboard text paste: sjis_text must be Shift_JIS encoded */
void autokey_start(const char *sjis_text);

#ifdef __cplusplus
}
#endif

/* state slot (0-9) */
extern int np2_stateslotnow;

#endif  /* NP2_WX_NP2_H */
