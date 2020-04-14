#ifndef _NP2_H_
#define _NP2_H_

#include "compiler.h"
#include "commng.h"

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

extern char hddfolder[MAX_PATH];
extern char fddfolder[MAX_PATH];
extern char bmpfilefolder[MAX_PATH];
extern UINT bmpfilenumber;
extern char modulefile[MAX_PATH];
extern char draw32bit;
extern UINT8 scrnmode;
int flagsave(const OEMCHAR *ext);
void flagdelete(const OEMCHAR *ext);
int flagload(const OEMCHAR *ext, const OEMCHAR *title, BOOL force);
extern void changescreen(UINT8 newmode);

enum {
	IMAGETYPE_UNKNOWN	= 0,
	IMAGETYPE_FDD,
	IMAGETYPE_SASI_IDE,
	IMAGETYPE_SASI_IDE_CD,
	IMAGETYPE_SCSI,
	IMAGETYPE_OTHER
};

#if defined(__LIBRETRO__)
typedef struct {
	UINT8	NOWAIT;
	UINT8	DRAW_SKIP;

	UINT8	KEYBOARD;

	UINT16	lrjoybtn[12];

	UINT8	resume;
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

	COMCFG	mpu;
#if defined(SUPPORT_SMPU98)
	COMCFG	smpuA;
	COMCFG	smpuB;
#endif

	UINT8	readonly; // No save changed settings
} NP2OSCFG;

enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 480
};

extern	NP2OSCFG	np2oscfg;

extern int np2_main(int argc, char *argv[]);
extern int np2_end();

extern int mmxflag;
int havemmx(void);

#else	/* __LIBRETRO__ */

#include <signal.h>

#include "joymng.h"

typedef struct {
	UINT8	NOWAIT;
	UINT8	DRAW_SKIP;

	UINT8	KEYBOARD;

	UINT8	JOYPAD1;
	UINT8	JOYPAD2;
	UINT8	JOY1BTN[JOY_NBUTTON];
	UINT8	JOYAXISMAP[2][JOY_NAXIS];
	UINT8	JOYBTNMAP[2][JOY_NBUTTON];
	char	JOYDEV[2][MAX_PATH];

	UINT8	resume;
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

	COMCFG	mpu;
#if defined(SUPPORT_SMPU98)
	COMCFG	smpuA;
	COMCFG	smpuB;
#endif
	COMCFG	com[3];

	UINT8	readonly; // No save changed settings
} NP2OSCFG;

#if defined(NP2_SIZE_QVGA)
enum {
	FULLSCREEN_WIDTH	= 320,
	FULLSCREEN_HEIGHT	= 240
};
#else
enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 480
};
#endif

extern	NP2OSCFG	np2oscfg;

extern BOOL s98logging;
extern int s98log_count;

extern int np2_main(int argc, char *argv[]);
extern int np2_end();

extern int mmxflag;
int havemmx(void);

extern UINT8 changescreeninit;

#endif	/* __LIBRETRO__ */

#endif  // _NP2_H_

