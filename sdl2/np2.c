#include "compiler.h"
#if defined(__LIBRETRO__)
#include	"file_stream.h"
#endif
#include	"strres.h"
#include	"np2.h"
#if defined(CPUCORE_IA32)
#include	"cpu.h"
#endif
#include	"dosio.h"
#include	"commng.h"
#include	"codecnv/codecnv.h"
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
#if defined(SUPPORT_IA32_HAXM)
#include	"i386hax/haxfunc.h"
#include	"i386hax/haxcore.h"
#endif
#include	"np2_tickcount.h"

static const char appname[] =
#if defined(CPUCORE_IA32)
    "np21kai"
#else
    "np2kai"
#endif
;

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
	1,			/* xrollkey */

	SNDDRV_SDL,		/* snddrv */
	{ "", "" }, 		/* MIDIDEV */
#if defined(SUPPORT_SMPU98)
	{ "", "" }, 		/* MIDIDEVA */
	{ "", "" }, 		/* MIDIDEVB */
#endif
	0,			/* MIDIWAIT */

	{ TRUE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", "" },	/* mpu */
#if defined(SUPPORT_SMPU98)
	{ TRUE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", "" },	/* s-mpu */
	{ TRUE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", "" },	/* s-mpu */
#endif
#if !defined(__LIBRETRO__)
	{
		{ TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com1 */
		{ TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com2 */
		{ TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com3 */
	},
#endif	/* __LIBRETRO__ */

	0,			/* readonly */
};
static	UINT		framecnt;
static	UINT		waitcnt;
static	UINT		framemax = 1;
static  UINT		lateframecount; // フレーム遅れ数

BOOL s98logging = FALSE;
int s98log_count = 0;

char hddfolder[MAX_PATH];
char fddfolder[MAX_PATH];
char bmpfilefolder[MAX_PATH];
UINT bmpfilenumber;
char modulefile[MAX_PATH];
char draw32bit;

unsigned int np2_main_disk_images_count = 0;
static unsigned int np2_main_cd_images_count = 0;
OEMCHAR np2_main_disk_images_paths[50][MAX_PATH] = {0};
static OEMCHAR np2_main_cd_images_paths[5][MAX_PATH] = {0};
static unsigned int np2_main_cd_drv[5] = {0xF, 0xF, 0xF, 0xF, 0xF};

UINT8 scrnmode = 0;
UINT8 changescreeninit = 0;

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]        : print this message\n");
	printf("\t--config [-c] <file> : specify config file\n");
}


// ---- resume

static void getstatfilename(OEMCHAR *path, const OEMCHAR *ext, int size)
{
	OEMCHAR filename[32];
	OEMSNPRINTF(filename, sizeof(filename), OEMTEXT("np2.%s"), ext);

	file_cpyname(path, file_getcd(filename), size);
}

int flagsave(const OEMCHAR *ext) {

	int		ret;
	OEMCHAR	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	ret = statsave_save(path);
	if (ret) {
		file_delete(path);
	}
	return(ret);
}

void flagdelete(const OEMCHAR *ext) {

	OEMCHAR	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	file_delete(path);
}

int flagload(const OEMCHAR *ext, const OEMCHAR *title, BOOL force) {

	int		ret;
	int		id;
	OEMCHAR	path[MAX_PATH];
	OEMCHAR	buf[1024];
	OEMCHAR	buf2[1024 + 256];

	getstatfilename(path, ext, sizeof(path));
	id = DID_YES;
	ret = statsave_check(path, buf, sizeof(buf));
	if (ret & (~STATFLAG_DISKCHG)) {
		menumbox("Couldn't restart", title, MBOX_OK | MBOX_ICONSTOP);
		id = DID_NO;
	}
	else if ((!force) && (ret & STATFLAG_DISKCHG)) {
		OEMSNPRINTF(buf2, sizeof(buf2), OEMTEXT("Conflict!\n\n%s\nContinue?"), buf);
		id = menumbox(buf2, title, MBOX_YESNOCAN | MBOX_ICONQUESTION);
	}
	if (id == DID_YES) {
		statsave_load(path);
	}
	return(id);
}

void
changescreen(UINT8 newmode)
{
	UINT8 change;
	UINT8 renewal;
	UINT8 res;

	change = scrnmode ^ newmode;
	renewal = (change & SCRNMODE_FULLSCREEN);
	if (newmode & SCRNMODE_FULLSCREEN) {
		renewal |= (change & SCRNMODE_HIGHCOLOR);
	} else {
		renewal |= (change & SCRNMODE_ROTATEMASK);
	}
	if (renewal) {
		if(menuvram) {
			menubase_close();
		}
		changescreeninit = 1;
		soundmng_stop();
		scrnmng_destroy();
		sysmenu_destroy();

		if(scrnmng_create(newmode) == SUCCESS) {
			scrnmode = newmode;
		} else {
			if(scrnmng_create(scrnmode) != SUCCESS) {
				return;
			}
		}
		sysmenu_create();
		changescreeninit = 0;
		scrndraw_redraw();
		soundmng_play();
	} else {
		scrnmode = newmode;
	}
}

// ---- proc

#define	framereset(cnt)		framecnt = 0

static void processwait(UINT cnt) {

	if (timing_getcount() >= cnt) {
#if defined(SUPPORT_IA32_HAXM)
		if (np2hax.enable) {
			np2haxcore.hltflag = 0;
			if(lateframecount > 0 && np2haxcore.I_ratio < 254){
				np2haxcore.I_ratio++;
			}else if(np2haxcore.I_ratio > 1){
				//np2haxcore.I_ratio--;
			}
			lateframecount = 0;
		}
#endif
		timing_setcount(0);
		framereset(cnt);
	}
	else {
#if defined(SUPPORT_IA32_HAXM)
		if (np2hax.enable) {
			if(np2haxcore.I_ratio > 1){
				np2haxcore.I_ratio--;
			}
		}
#endif
		taskmng_sleep(1);
	}
}

int is_absolute(const char *path)
{
  if (path[0] == '/' || path[1] == ':')
    return 1;
  if (path[0] == '\\' && path[1] == '\\')
    return 1;
#if defined (_3DS) || defined (PSP) || defined (VITA)
  char *pcolon = strchr(path, ':');
  if (pcolon != NULL && pcolon[1] == '/')
    return 1;
#endif
  return 0;
}

void np2_main_getfullpath(char* fullpath, const char* file, const char* base_dir) {
  int len;

  milstr_ncpy(fullpath, file, MAX_PATH);
  len = OEMSTRLEN(fullpath);
  if(len > 1) {
    if(!is_absolute(fullpath)) {
      milstr_ncpy(fullpath, base_dir, MAX_PATH);
      file_setseparator(fullpath, MAX_PATH);
      milstr_ncat(fullpath, file, MAX_PATH);
    }
  }
}

char np2_isfdimage(const char *file, const int len) {
  char fd = 0;
  char* ext;

  if(len > 4) {
    ext = file + len - 4;
    if      (milstr_cmp(ext, ".d88") == 0) fd = 1;
    else if (milstr_cmp(ext, ".d98") == 0) fd = 1;
    else if (milstr_cmp(ext, ".fdi") == 0) fd = 1;
    else if (milstr_cmp(ext, ".hdm") == 0) fd = 1;
    else if (milstr_cmp(ext, ".xdf") == 0) fd = 1;
    else if (milstr_cmp(ext, ".dup") == 0) fd = 1;
    else if (milstr_cmp(ext, ".2hd") == 0) fd = 1;
    else if (milstr_cmp(ext, ".nfd") == 0) fd = 1;
    else if (milstr_cmp(ext, ".fdd") == 0) fd = 1;
    else if (milstr_cmp(ext, ".hd4") == 0) fd = 1;
    else if (milstr_cmp(ext, ".hd5") == 0) fd = 1;
    else if (milstr_cmp(ext, ".hd9") == 0) fd = 1;
    else if (milstr_cmp(ext, ".h01") == 0) fd = 1;
    else if (milstr_cmp(ext, ".hdb") == 0) fd = 1;
    else if (milstr_cmp(ext, ".ddb") == 0) fd = 1;
    else if (milstr_cmp(ext, ".dd6") == 0) fd = 1;
    else if (milstr_cmp(ext, ".dd9") == 0) fd = 1;
    else if (milstr_cmp(ext, ".dcp") == 0) fd = 1;
    else if (milstr_cmp(ext, ".dcu") == 0) fd = 1;
    else if (milstr_cmp(ext, ".flp") == 0) fd = 1;
    else if (milstr_cmp(ext, ".bin") == 0) fd = 1;
    else if (milstr_cmp(ext, ".tfd") == 0) fd = 1;
    else if (milstr_cmp(ext, ".fim") == 0) fd = 1;
    else if (milstr_cmp(ext, ".img") == 0) fd = 1;
    else if (milstr_cmp(ext, ".ima") == 0) fd = 1;
  }

  return fd;
}

char np2_main_read_m3u(const char *file)
{
  char res = 0;
  char line[MAX_PATH];
  char name[MAX_PATH];
  char base_dir[MAX_PATH];
#if defined(__LIBRETRO__)
  RFILE *f;
#else
  FILE *f;
#endif

#if defined(__LIBRETRO__)
  f = filestream_open(file, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
#elif defined(_WINDOWS)
	wchar_t	wfile[MAX_PATH];
	codecnv_utf8toucs2(wfile, MAX_PATH, file, -1);
  f = _wfopen(wfile, "r");
#else
  f = fopen(file, "r");
#endif
  if (!f) {
    res = 1;
    return res;
  }

  milstr_ncpy(base_dir, file, MAX_PATH);
  file_cutname(base_dir);

#if defined(__LIBRETRO__)
  while (filestream_gets(f, line, sizeof(line)) && np2_main_disk_images_count < sizeof(np2_main_disk_images_paths) / MAX_PATH)
#else
  while (fgets(f, line, sizeof(line)) && np2_main_disk_images_count < sizeof(np2_main_disk_images_paths) / MAX_PATH)
#endif
  {
    if (line[0] == '#')
      continue;

    char *carriage_return = strchr(line, '\r');
    if (carriage_return)
      *carriage_return = '\0';

    char *newline = strchr(line, '\n');
    if (newline)
      *newline = '\0';

    // Remove any beginning and ending quotes as these can cause issues when feeding the paths into command line later
    if (line[0] == '"')
       memmove(line, line + 1, OEMSTRLEN(line));

    if (line[OEMSTRLEN(line) - 1] == '"')
       line[OEMSTRLEN(line) - 1]  = '\0';

    if (OEMSTRLEN(line) > 4)
    {
      milstr_ncpy(name, line, MAX_PATH);
      if(line[0] != '/' && line[1] != ':' && (line[0] != '\\' && line[1] != '\\')) {
        milstr_ncpy(name, base_dir, MAX_PATH);
        milstr_ncat(name, line, MAX_PATH);
      }
      if(np2_isfdimage(name, OEMSTRLEN(name))) {
        milstr_ncpy(np2_main_disk_images_paths[np2_main_disk_images_count], name, MAX_PATH);
        np2_main_disk_images_count++;
      }
    }
  }

#if defined(__LIBRETRO__)
  filestream_close(f);
#else
  fclose(f);
#endif
  return res;
}

#if defined(__LIBRETRO__)
extern char lr_game_base_dir[MAX_PATH];
extern void attach_disk_swap_interface(void);
extern int lr_uselasthddmount;
#endif

int np2_main(int argc, char *argv[]) {

	int		pos;
	char	*p;
	int		id;
	int		i, j, imagetype, drvfdd, setmedia, drvhddSCSI, HDCount, CDCount, CDDrv[4], CDArgv[4];
	OEMCHAR	*ext;
	OEMCHAR	base_dir[MAX_PATH];
	OEMCHAR	fullpath[MAX_PATH];
  FILEH fcheck;

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

#if defined(__LIBRETRO__)
	milstr_ncpy(base_dir, lr_game_base_dir, MAX_PATH);
#else
	char *config_home = getenv("XDG_CONFIG_HOME");
	char *home = getenv("HOME");
	if (config_home && config_home[0] == '/') {
		/* base dir */
		milstr_ncpy(np2cfg.biospath, config_home, sizeof(np2cfg.biospath));
		milstr_ncat(np2cfg.biospath, "/", sizeof(np2cfg.biospath));
		milstr_ncat(np2cfg.biospath, appname, sizeof(np2cfg.biospath));
		milstr_ncat(np2cfg.biospath, "/", sizeof(np2cfg.biospath));
	} else if (home) {
		/* base dir */
		milstr_ncpy(np2cfg.biospath, home, sizeof(np2cfg.biospath));
		milstr_ncat(np2cfg.biospath, "/.config/", sizeof(np2cfg.biospath));
		milstr_ncat(np2cfg.biospath, appname, sizeof(np2cfg.biospath));
		milstr_ncat(np2cfg.biospath, "/", sizeof(np2cfg.biospath));
	} else {
		printf("$HOME isn't defined.\n");
		goto np2main_err1;
	}
	file_setcd(np2cfg.biospath);
	milstr_ncpy(base_dir, np2cfg.biospath, MAX_PATH);
#endif	/* __LIBRETRO__ */

	initload();
#if defined(SUPPORT_WAB)
	wabwin_readini();
#endif	// defined(SUPPORT_WAB)

	mmxflag = havemmx() ? 0 : MMXFLAG_NOTSUPPORT;

#if defined(SUPPORT_CL_GD5430)
	draw32bit = np2cfg.usegd5430;
#endif

	OEMSNPRINTF(fullpath, sizeof(fullpath), OEMTEXT("%sdefault.ttf"), np2cfg.biospath);
  fcheck = file_open_rb(fullpath);
	if (fcheck != NULL) {
		file_close(fcheck);
		fontmng_setdeffontname(fullpath);
	}
	
	setmedia = drvhddSCSI = HDCount = CDCount = 0;
#if defined(__LIBRETRO__)
	if(!lr_uselasthddmount) {
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI)
		for(i = 0; i < 4; i++) {
			milstr_ncpy(np2cfg.sasihdd[i], "", MAX_PATH);
		}
#endif
#if defined(SUPPORT_SCSI)
 		for(i = 0; i < 4; i++) {
			milstr_ncpy(np2cfg.scsihdd[i], "", MAX_PATH);
		}
#endif
	}
#endif	/* __LIBRETRO__ */
	for (i = 1; i < argc; i++) {
		if (OEMSTRLEN(argv[i]) < 5) {
			continue;
		}

		if(argv[i][0] == '"' && argv[i][OEMSTRLEN(argv[i]) - 1] == '"') {
		  argv[i]++;
		  argv[i][OEMSTRLEN(argv[i]) - 1] = '\0';
		}

		imagetype = IMAGETYPE_UNKNOWN;
		np2_main_getfullpath(fullpath, argv[i], base_dir);
		ext = fullpath + (OEMSTRLEN(fullpath) - 4);

		if(milstr_cmp(ext, ".m3u") == 0) {
			imagetype = IMAGETYPE_OTHER;
			np2_main_read_m3u(fullpath);
		}

		if(np2_isfdimage(fullpath, OEMSTRLEN(fullpath))) {
			imagetype = IMAGETYPE_FDD;
			milstr_ncpy(np2_main_disk_images_paths[np2_main_disk_images_count], fullpath, MAX_PATH);
			np2_main_disk_images_count++;
		}
#if defined(__LIBRETRO__)
		attach_disk_swap_interface();
#endif

#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI) || defined(SUPPORT_SCSI)
		if(imagetype == IMAGETYPE_UNKNOWN) {
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI)
			if      (milstr_cmp(ext, ".hdi") == 0) imagetype = IMAGETYPE_SASI_IDE;  // SASI/IDE
			else if (milstr_cmp(ext, ".thd") == 0) imagetype = IMAGETYPE_SASI_IDE;
			else if (milstr_cmp(ext, ".nhd") == 0) imagetype = IMAGETYPE_SASI_IDE;
			else if (milstr_cmp(ext, ".vhd") == 0) imagetype = IMAGETYPE_SASI_IDE;
			else if (milstr_cmp(ext, ".sln") == 0) imagetype = IMAGETYPE_SASI_IDE;
			else if (milstr_cmp(ext, ".iso") == 0) imagetype = IMAGETYPE_SASI_IDE_CD;  // SASI/IDE CD
			else if (milstr_cmp(ext, ".cue") == 0) imagetype = IMAGETYPE_SASI_IDE_CD;
			else if (milstr_cmp(ext, ".ccd") == 0) imagetype = IMAGETYPE_SASI_IDE_CD;
			else if (milstr_cmp(ext, ".mds") == 0) imagetype = IMAGETYPE_SASI_IDE_CD;
			else if (milstr_cmp(ext, ".nrg") == 0) imagetype = IMAGETYPE_SASI_IDE_CD;
#endif
#if defined(SUPPORT_SCSI)
			if      (milstr_cmp(ext, ".hdd") == 0) imagetype = IMAGETYPE_SCSI;  // SCSI
			else if (milstr_cmp(ext, ".hdn") == 0) imagetype = IMAGETYPE_SCSI;
#endif
			switch (imagetype) {
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI)
			case IMAGETYPE_SASI_IDE:
				for(j = 0; j < 4; j++) {
					if(np2cfg.idetype[j] == SXSIDEV_HDD) {
						if(!(setmedia & (1 << j))) {
							milstr_ncpy(np2cfg.sasihdd[j], fullpath, MAX_PATH);
							setmedia |= 1 << j;
							HDCount++;
							break;
						}
					}
				}
				break;
			case IMAGETYPE_SASI_IDE_CD:
				if(np2_main_cd_images_count < sizeof(np2_main_cd_images_paths) / MAX_PATH) {
					milstr_ncpy(np2_main_cd_images_paths[np2_main_cd_images_count], fullpath, MAX_PATH);
					for(j = 0; j < 4; j++) {
						if(np2cfg.idetype[j] == SXSIDEV_CDROM) {
							if(!(setmedia & (1 << j))) {
								np2_main_cd_drv[np2_main_cd_images_count] = j;
								setmedia |= 1 << j;
							}
							break;
						}
					}
					np2_main_cd_images_count++;
				}
				break;
#endif
#if defined(SUPPORT_SCSI)
			case IMAGETYPE_SCSI:
				if(drvhddSCSI < 4) {
					milstr_ncpy(np2cfg.scsihdd[drvhddSCSI], fullpath, MAX_PATH);
					drvhddSCSI++;
				}
				break;
#endif
			}
		}
#endif
	}

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
	if (scrnmng_create(0) != SUCCESS) {
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

	for (i = 0; i < np2_main_cd_images_count; i++) {
		if (i < 1) {
			sxsi_devopen(np2_main_cd_drv[i], np2_main_cd_images_paths[i]);
		}
	}

	drvfdd = 0;
	for (i = 0; i < np2_main_disk_images_count; i++) {
		if (i < 2) {
			diskdrv_setfdd(i, np2_main_disk_images_paths[i], 0);
		}
	}

//printf("bd:%s\n",base_dir);
//printf("fdin:%d\n",drvfdd);
//printf("hdin:%d\n",HDCount);
//printf("cdin:%d\n",CDCount);

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
	np2wabcfg.readonly = np2oscfg.readonly;
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

#if defined(GCC_CPU_ARCH_AMD64) || defined(__ANDROID__)
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

