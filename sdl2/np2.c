#include	"compiler.h"
#include	"strres.h"
#include	"np2.h"
#include	"dosio.h"
#include	"commng.h"
#include	"fontmng.h"
#include	"inputmng.h"
#include	"joymng.h"
#include	"kbdmng.h"
#include	"mousemng.h"
#include	"scrnmng.h"
#include	"soundmng.h"
#include	"sysmng.h"
#include	"taskmng.h"
#include	"kbtrans.h"
#include	"ini.h"
#include	"pccore.h"
#include	"statsave.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"s98.h"
#include	"fdd/diskdrv.h"
#include	"timing.h"
#include	"keystat.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"
#if defined(SUPPORT_NET)
#include	"net.h"
#endif


NP2OSCFG np2oscfg = {
	0,			/* NOWAIT */
	0,			/* DRAW_SKIP */

	KEY_KEY106,		/* KEYBOARD */

#if !defined(__LIBRETRO__)
	0,			/* JOYPAD1 */
	0,			/* JOYPAD2 */
	{ 1, 2, 5, 6 },		/* JOY1BTN */
	{
		{ 0, 1 },		/* JOYAXISMAP[0] */
		{ 0, 1 },		/* JOYAXISMAP[1] */
	},
	{
		{ 0, 1, 0xff, 0xff },	/* JOYBTNMAP[0] */
		{ 0, 1, 0xff, 0xff },	/* JOYBTNMAP[1] */
	},
	{ "", "" },		/* JOYDEV */
#endif	/* __LIBRETRO__ */

	{ COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", "" },	/* mpu */
	{
		{ COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com1 */
		{ COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com2 */
		{ COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com3 */
	},

	0,			/* resume */
	0,			/* jastsnd */
	0,			/* I286SAVE */

	SNDDRV_SDL,		/* snddrv */
	{ "", "" }, 		/* MIDIDEV */
	0,			/* MIDIWAIT */
};
static	UINT		framecnt;
static	UINT		waitcnt;
static	UINT		framemax = 1;

BOOL s98logging = FALSE;
int s98log_count = 0;

char hddfolder[MAX_PATH];
char fddfolder[MAX_PATH];
char bmpfilefolder[MAX_PATH];
UINT bmpfilenumber;
char modulefile[MAX_PATH];

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]       : print this message\n");
}


// ---- resume

static void getstatfilename(char *path, const char *ext, int size)
{
	char filename[32];
	sprintf(filename, "np2.%s", ext);

	file_cpyname(path, file_getcd(filename), size);
}

int flagsave(const char *ext) {

	int		ret;
	char	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	ret = statsave_save(path);
	if (ret) {
		file_delete(path);
	}
	return(ret);
}

void flagdelete(const char *ext) {

	char	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	file_delete(path);
}

int flagload(const char *ext, const char *title, BOOL force) {

	int		ret;
	int		id;
	char	path[MAX_PATH];
	char	buf[1024];
	char	buf2[1024 + 256];

	getstatfilename(path, ext, sizeof(path));
	id = DID_YES;
	ret = statsave_check(path, buf, sizeof(buf));
	if (ret & (~STATFLAG_DISKCHG)) {
		menumbox("Couldn't restart", title, MBOX_OK | MBOX_ICONSTOP);
		id = DID_NO;
	}
	else if ((!force) && (ret & STATFLAG_DISKCHG)) {
		SPRINTF(buf2, "Conflict!\n\n%s\nContinue?", buf);
		id = menumbox(buf2, title, MBOX_YESNOCAN | MBOX_ICONQUESTION);
	}
	if (id == DID_YES) {
		statsave_load(path);
	}
	return(id);
}


// ---- proc

#define	framereset(cnt)		framecnt = 0

static void processwait(UINT cnt) {

	if (timing_getcount() >= cnt) {
		timing_setcount(0);
		framereset(cnt);
	}
	else {
		taskmng_sleep(1);
	}
}

int np2_main(int argc, char *argv[]) {

	int		pos;
	char	*p;
	int		id;
	int		i, imagetype, drvfdd, drvhddSASI, drvhddSCSI;
	char	*ext;
	char	*tmppath;
	char	tmppath2[MAX_PATH];

	pos = 1;
	while(pos < argc) {
		p = argv[pos++];
		if ((!milstr_cmp(p, "-h")) || (!milstr_cmp(p, "--help"))) {
			usage(argv[0]);
			goto np2main_err1;
		}/*
		else {
			printf("error command: %s\n", p);
			goto np2main_err1;
		}*/
	}

	initload();

	if(np2cfg.biospath[0] == '\0') {
#if defined(_WIN32)
		GetModuleFileName(NULL, np2cfg.biospath, MAX_PATH);
#else	/* _WIN32 */
		readlink("/proc/self/exe", np2cfg.biospath, sizeof(np2cfg.biospath) - 1);
#endif	/* _WIN32 */
		file_setcd(np2cfg.biospath);
		tmppath = dirname(np2cfg.biospath);
		strcpy(np2cfg.biospath, tmppath);
	}
	sprintf(tmppath2, "%sdefault.ttf", np2cfg.biospath);
	fontmng_setdeffontname(tmppath2);
	
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SATA) || defined(SUPPORT_SCSI)
	drvhddSASI = drvhddSCSI = 0;
	for (i = 1; i < argc; i++) {
		if (OEMSTRLEN(argv[i]) < 5) {
			continue;
		}
		
		imagetype = IMAGETYPE_UNKNOWN;
		ext = argv[i] + OEMSTRLEN(argv[i]) - 4;
		if      (0 == milstr_cmp(ext, ".hdi"))	imagetype = IMAGETYPE_SASI_IDE; // SASI/IDE
		else if (0 == milstr_cmp(ext, ".thd"))	imagetype = IMAGETYPE_SASI_IDE;
		else if (0 == milstr_cmp(ext, ".nhd"))	imagetype = IMAGETYPE_SASI_IDE;
		else if (0 == milstr_cmp(ext, ".vhd"))	imagetype = IMAGETYPE_SASI_IDE;
		else if (0 == milstr_cmp(ext, ".sln"))	imagetype = IMAGETYPE_SASI_IDE;
		else if (0 == milstr_cmp(ext, ".hdd"))	imagetype = IMAGETYPE_SCSI; // SCSI
		
		switch (imagetype) {
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SATA)
		case IMAGETYPE_SASI_IDE:
			if (drvhddSASI < 2) {
				milstr_ncpy(np2cfg.sasihdd[drvhddSASI], argv[i], MAX_PATH);
				drvhddSASI++;
			}
			break;
#endif
#if defined(SUPPORT_SCSI)
		case IMAGETYPE_SCSI:
			if (drvhddSCSI < 4) {
				milstr_ncpy(np2cfg.scsihdd[drvhddSASI], argv[i], MAX_PATH);
				drvhddSCSI++;
			}
			break;
#endif
		}
	}
#endif

	TRACEINIT();

	if (fontmng_init() != SUCCESS) {
		goto np2main_err2;
	}
	inputmng_init();
	keystat_initialize();

	if (sysmenu_create() != SUCCESS) {
		goto np2main_err3;
	}

#if !defined(__LIBRETRO__)
	joymng_initialize();
#endif	/* __LIBRETRO__ */
	mousemng_initialize();

	scrnmng_initialize();
	if (scrnmng_create(FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT) != SUCCESS) {
		goto np2main_err4;
	}

	soundmng_initialize();
	commng_initialize();
	sysmng_initialize();
	taskmng_initialize();
	pccore_init();
	S98_init();

#if defined(SUPPORT_NET)
	np2net_init();
#endif
#if !defined(__LIBRETRO__)
	mousemng_hidecursor();
#endif	/* __LIBRETRO__ */
	scrndraw_redraw();
	pccore_reset();

#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume) {
		id = flagload(str_sav, str_resume, FALSE);
		if (id == DID_CANCEL) {
			goto np2main_err5;
		}
	}
#endif	/* defined(SUPPORT_RESUME) */

	drvfdd = drvhddSASI = drvhddSCSI = 0;
	for (i = 1; i < argc; i++) {
		if (OEMSTRLEN(argv[i]) < 5) {
			continue;
		}
		
		imagetype = IMAGETYPE_UNKNOWN;
		ext = argv[i] + OEMSTRLEN(argv[i]) - 4;
		if      (0 == milstr_cmp(ext, ".d88")) imagetype = IMAGETYPE_FDD; // FDD
		else if (0 == milstr_cmp(ext, ".d98")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".fdi")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".hdm")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".xdf")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".dup")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".2hd")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".nfd")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".fdd")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".hd4")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".hd5")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".hd9")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".h01")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".hdb")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".ddb")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".dd6")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".dd9")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".dcp")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".dcu")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".flp")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".bin")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".tfd")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".fim")) imagetype = IMAGETYPE_FDD;
		if (imagetype == IMAGETYPE_UNKNOWN) {
			continue;
		}
		
		if (drvfdd < 4) {
			diskdrv_readyfdd(drvfdd, argv[i], 0);
			drvfdd++;
		}
		
	}

#if defined(__LIBRETRO__)
	return(SUCCESS);

#if defined(SUPPORT_RESUME)
np2main_err5:
	pccore_term();
	S98_trash();
	soundmng_deinitialize();
#endif	/* defined(SUPPORT_RESUME) */

np2main_err4:
	scrnmng_destroy();

np2main_err3:
	sysmenu_destroy();

np2main_err2:
	TRACETERM();
	//SDL_Quit();

np2main_err1:
	return(FAILURE);
}

int np2_end(){
#else	/* __LIBRETRO__ */
	
	while(taskmng_isavail()) {
#ifdef SUPPORT_HRTIMER
		upd4990_timingpulse();
#endif	/* SUPPORT_HRTIMER */
		taskmng_rol();
		if (np2oscfg.NOWAIT) {
			joymng_sync();
			pccore_exec(framecnt == 0);
			if (np2oscfg.DRAW_SKIP) {			// nowait frame skip
				framecnt++;
				if (framecnt >= np2oscfg.DRAW_SKIP) {
					processwait(0);
				}
			}
			else {							// nowait auto skip
				framecnt = 1;
				if (timing_getcount()) {
					processwait(0);
				}
			}
		}
		else if (np2oscfg.DRAW_SKIP) {		// frame skip
			if (framecnt < np2oscfg.DRAW_SKIP) {
				joymng_sync();
				pccore_exec(framecnt == 0);
				framecnt++;
			}
			else {
				processwait(np2oscfg.DRAW_SKIP);
			}
		}
		else {								// auto skip
			if (!waitcnt) {
				UINT cnt;
				joymng_sync();
				pccore_exec(framecnt == 0);
				framecnt++;
				cnt = timing_getcount();
				if (framecnt > cnt) {
					waitcnt = framecnt;
					if (framemax > 1) {
						framemax--;
					}
				}
				else if (framecnt >= framemax) {
					if (framemax < 12) {
						framemax++;
					}
					if (cnt >= 12) {
						timing_reset();
					}
					else {
						timing_setcount(cnt - framecnt);
					}
					framereset(0);
				}
			}
			else {
				processwait(waitcnt);
				waitcnt = framecnt;
			}
		}
	}
#endif	/* __LIBRETRO__ */

	pccore_cfgupdate();
#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume) {
		flagsave(str_sav);
	}
	else {
		flagdelete(str_sav);
	}
#endif
#if !defined(__LIBRETRO__)
	joymng_deinitialize();
#endif	/* __LIBRETRO__ */
#if defined(SUPPORT_NET)
	np2net_shutdown();
#endif
	pccore_term();
	S98_trash();
	soundmng_deinitialize();

	sysmng_deinitialize();

	scrnmng_destroy();
	sysmenu_destroy();
	TRACETERM();
#if !defined(__LIBRETRO__)
	SDL_Quit();
#endif	/* __LIBRETRO__ */
	return(SUCCESS);

#if !defined(__LIBRETRO__)
#if defined(SUPPORT_RESUME)
np2main_err5:
	pccore_term();
	S98_trash();
	soundmng_deinitialize();
#endif	/* defined(SUPPORT_RESUME) */

np2main_err4:
	scrnmng_destroy();

np2main_err3:
	sysmenu_destroy();

np2main_err2:
	TRACETERM();
	SDL_Quit();

np2main_err1:
	return(FAILURE);
#endif	/* __LIBRETRO__ */
}

