#include "compiler.h"
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
#include	"sxsi.h"
#include	"fdd/diskdrv.h"
#include	"timing.h"
#include	"keystat.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"
#if defined(SUPPORT_NET)
#include	"net.h"
#endif
#if defined(SUPPORT_WAB)
#include	"wab.h"
#include	"wabbmpsave.h"
#endif
#if defined(SUPPORT_CL_GD5430)
#include	"cirrus_vga_extern.h"
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
#else	/* __LIBRETRO__ */
	{ 273, 274, 276, 275, 120, 122, 32, 306, 8, 303, 27, 13 },	/* lrjoybtn */
#endif	/* __LIBRETRO__ */

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
char draw32bit;

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]        : print this message\n");
	printf("\t--config [-c] <file> : specify config file\n");
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
	int		i, j, imagetype, drvfdd, setmedia, drvhddSCSI, CDCount, CDDrv[4], CDArgv[4];
	char	*ext;
	char	tmppath[MAX_PATH];
	FILE	*fcheck;

	pos = 1;
	while(pos < argc) {
		p = argv[pos++];
		if ((!milstr_cmp(p, "-h")) || (!milstr_cmp(p, "--help"))) {
			usage(argv[0]);
			goto np2main_err1;
		} else if ((!milstr_cmp(p, "-c")) || (!milstr_cmp(p, "--config"))) {
			if(pos < argc) {
				milstr_ncpy(modulefile, argv[pos++], sizeof(modulefile));
			} else {
				printf("Invalid option.\n");
				goto np2main_err1;
			}
		}/*
		else {
			printf("error command: %s\n", p);
			goto np2main_err1;
		}*/
	}

#if !defined(__LIBRETRO__)
	char *config_home = getenv("XDG_CONFIG_HOME");
	char *home = getenv("HOME");
	if (config_home && config_home[0] == '/') {
		/* base dir */
		milstr_ncpy(np2cfg.biospath, config_home, sizeof(np2cfg.biospath));
		milstr_ncat(np2cfg.biospath, "/np2kai/", sizeof(np2cfg.biospath));
	} else if (home) {
		/* base dir */
		milstr_ncpy(np2cfg.biospath, home, sizeof(np2cfg.biospath));
		milstr_ncat(np2cfg.biospath, "/.config/np2kai/", sizeof(np2cfg.biospath));
	} else {
		printf("$HOME isn't defined.\n");
		goto np2main_err1;
	}
	file_setcd(np2cfg.biospath);
#endif	/* __LIBRETRO__ */

	initload();
#if defined(SUPPORT_WAB)
	wabwin_readini();
#endif	// defined(SUPPORT_WAB)

	mmxflag = havemmx() ? 0 : MMXFLAG_NOTSUPPORT;

#if defined(SUPPORT_CL_GD5430)
	draw32bit = np2cfg.usegd5430;
#endif

	sprintf(tmppath, "%sdefault.ttf", np2cfg.biospath);
	fcheck = fopen(tmppath, "rb");
	if (fcheck != NULL) {
		fclose(fcheck);
		fontmng_setdeffontname(tmppath);
	}
	
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI) || defined(SUPPORT_SCSI)
	setmedia = drvhddSCSI = CDCount = 0;
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
		else if (0 == milstr_cmp(ext, ".iso"))	imagetype = IMAGETYPE_SASI_IDE_CD; // SASI/IDE CD
		else if (0 == milstr_cmp(ext, ".cue"))	imagetype = IMAGETYPE_SASI_IDE_CD;
		else if (0 == milstr_cmp(ext, ".ccd"))	imagetype = IMAGETYPE_SASI_IDE_CD;
		else if (0 == milstr_cmp(ext, ".mds"))	imagetype = IMAGETYPE_SASI_IDE_CD;
		else if (0 == milstr_cmp(ext, ".nrg"))	imagetype = IMAGETYPE_SASI_IDE_CD;
		else if (0 == milstr_cmp(ext, ".hdd"))	imagetype = IMAGETYPE_SCSI; // SCSI
		else if (0 == milstr_cmp(ext, ".hdn"))	imagetype = IMAGETYPE_SCSI;
		
		switch (imagetype) {
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI)
		case IMAGETYPE_SASI_IDE:
			for(j = 0; j < 4; j++) {
				if (np2cfg.idetype[j] == SXSIDEV_HDD) {
					if(!(setmedia & (1 << j))) {
						milstr_ncpy(np2cfg.sasihdd[j], argv[i], MAX_PATH);
						setmedia |= 1 << j;
						break;
					}
				}
			}
			break;
		case IMAGETYPE_SASI_IDE_CD:
			for(j = 0; j < 4; j++) {
				if (np2cfg.idetype[j] == SXSIDEV_CDROM) {
					if(!(setmedia & (1 << j))) {
						CDDrv[CDCount] = j;
						CDArgv[CDCount] = i;
						CDCount++;
						setmedia |= 1 << j;
						break;
					}
				}
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
	if (scrnmng_create(FULLSCREEN_WIDTH, 400) != SUCCESS) {
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
#ifdef SUPPORT_WAB
	np2wab_init();
#endif
#ifdef SUPPORT_CL_GD5430
	pc98_cirrus_vga_init();
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

	for(i = 0; i < CDCount; i++) {
		sxsi_devopen(CDDrv[i], argv[CDArgv[i]]);
	}

	drvfdd = 0;
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
		else if (0 == milstr_cmp(ext, ".img")) imagetype = IMAGETYPE_FDD;
		else if (0 == milstr_cmp(ext, ".ima")) imagetype = IMAGETYPE_FDD;
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
#ifdef SUPPORT_CL_GD5430
	pc98_cirrus_vga_shutdown();
#endif
#ifdef SUPPORT_WAB
	np2wab_shutdown();
#endif
	pccore_term();
	S98_trash();
	soundmng_deinitialize();

	sysmng_deinitialize();

	scrnmng_destroy();
	sysmenu_destroy();
#if defined(SUPPORT_WAB)
	wabwin_writeini();
#endif	// defined(SUPPORT_WAB)
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

int mmxflag;

int
havemmx(void)
{
#if !defined(GCC_CPU_ARCH_IA32)
	return 0;
#else	/* GCC_CPU_ARCH_IA32 */
	int rv;

#if defined(GCC_CPU_ARCH_AMD64)
	rv = 1;
#else	/* !GCC_CPU_ARCH_AMD64 */
	asm volatile (
		"pushf;"
		"popl	%%eax;"
		"movl	%%eax, %%edx;"
		"xorl	$0x00200000, %%eax;"
		"pushl	%%eax;"
		"popf;"
		"pushf;"
		"popl	%%eax;"
		"subl	%%edx, %%eax;"
		"je	.nocpuid;"
		"xorl	%%eax, %%eax;"
		"incl	%%eax;"
		"pushl	%%ebx;"
		"cpuid;"
		"popl	%%ebx;"
		"movl	%%edx, %0;"
		"andl	$0x00800000, %0;"
	".nocpuid:"
		: "=a" (rv));
#endif /* GCC_CPU_ARCH_AMD64 */
	return rv;
#endif /* GCC_CPU_ARCH_IA32 */
}
