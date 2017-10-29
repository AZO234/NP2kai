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

enum {
	IMAGETYPE_UNKNOWN	= 0,
	IMAGETYPE_FDD,
	IMAGETYPE_SASI_IDE,
	IMAGETYPE_SASI_IDE_CD,
	IMAGETYPE_SCSI,
};

#if defined(__LIBRETRO__)
typedef struct {
	UINT8	NOWAIT;
	UINT8	DRAW_SKIP;

	UINT8	KEYBOARD;

	COMCFG	mpu;
	COMCFG	com[3];

	UINT8	resume;
	UINT8	jastsnd;
	UINT8	I286SAVE;

	UINT8	snddrv;
	char	MIDIDEV[2][MAX_PATH];
	UINT32	MIDIWAIT;
} NP2OSCFG;

enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 480
};

extern	NP2OSCFG	np2oscfg;

extern int np2_main(int argc, char *argv[]);
extern int np2_end();

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

	COMCFG	mpu;
	COMCFG	com[3];

	UINT8	resume;
	UINT8	jastsnd;
	UINT8	I286SAVE;

	UINT8	snddrv;
	char	MIDIDEV[2][MAX_PATH];
	UINT32	MIDIWAIT;
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
#endif	/* __LIBRETRO__ */

