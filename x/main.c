/*
 * Copyright (c) 2003 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <compiler.h>

#include <sys/stat.h>
#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <libgen.h>

#if defined(SUPPORT_SDL_AUDIO) || defined(SUPPORT_SDL_MIXER)
#include <SDL.h>
#endif

#include <np2.h>
#include <fdd/diskdrv.h>
#include <dosio.h>
#include <ini.h>
#include <common/parts.h>
#include <pccore.h>
#include <sound/s98.h>
#include <vram/scrndraw.h>
#include <io/serial.h>
#include <timing.h>
#include <toolkit.h>

#include "kdispwin.h"
#include "toolwin.h"
#include <debug/viewer.h>
#include <debug/debugwin.h>
#include "skbdwin.h"

#include <commng.h>
#include <fontmng.h>
#include <joymng.h>
#include "kbdmng.h"
#include <mousemng.h>
#include <scrnmng.h>
#include <soundmng.h>
#include <sysmng.h>
#include <taskmng.h>
#if defined(SUPPORT_NET)
#include <network/net.h>
#endif
#if defined(SUPPORT_WAB)
#include <wab/wab.h>
#include <wab/wabbmpsave.h>
#endif
#if defined(SUPPORT_CL_GD5430)
#include <wab/cirrus_vga_extern.h>
#endif


static const char appname[] =
#if defined(CPUCORE_IA32)
    "xnp21kai"
#else
    "xnp2kai"
#endif
;

/*
 * failure signale handler
 */
typedef void sigfunc(int);

static sigfunc *
setup_signal(int signo, sigfunc *func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(signo, &act, &oact) < 0)
		return SIG_ERR;
	return oact.sa_handler;
}

static void
sighandler(int signo)
{

	toolkit_widget_quit();
}


/*
 * option
 */
static struct option longopts[] = {
	{ "config",		required_argument,	0,	'c' },
	{ "timidity-config",	required_argument,	0,	'C' },
	{ "help",		no_argument,		0,	'h' },
	{ 0,			0,			0,	0   },
};

static char* progname;

static void
usage(void)
{

	g_printerr("Usage: %s [options] [[FD1 image] [[FD2 image] [[FD3 image] [FD4 image]]]]\n\n", progname);
	g_printerr("options:\n");
	g_printerr("\t--help            [-h]        : print this message\n");
	g_printerr("\t--config          [-c] <file> : specify config file\n");
	g_printerr("\t--timidity-config [-C] <file> : specify timidity config file\n");
	exit(1);
}

void hostdrv_readini();
void hostdrv_writeini();


/*
 * main
 */
int
main(int argc, char *argv[])
{
	struct stat sb;
	BRESULT result;
	int rv = 1;
	int ch;
	int i, drvmax;
	char	fontfile[MAX_PATH];
  FILE *fcheck;
  int createini = 0;

	progname = argv[0];

	setlocale(LC_ALL, "");
	(void) bindtextdomain(appname, NP2LOCALEDIR);
	(void) bind_textdomain_codeset(appname, "UTF-8");
	(void) textdomain(appname);

	toolkit_initialize();
	toolkit_arginit(&argc, &argv);

	while ((ch = getopt_long(argc, argv, "c:C:t:vh", longopts, NULL)) != -1) {
		switch (ch) {
		case 'c':
			if (stat(optarg, &sb) < 0) {
				g_printerr("Can't access %s.\n", optarg);
				exit(1);
			} else {
				if (!S_ISREG(sb.st_mode)) {
					g_printerr("%s isn't regular file.\n", optarg);
					exit(1);
				}
			}
			milstr_ncpy(modulefile, optarg, sizeof(modulefile));
			break;

		case 'C':
			if (stat(optarg, &sb) < 0) {
				g_printerr("Can't access %s.\n", optarg);
				exit(1);
			} else {
				if (!S_ISREG(sb.st_mode)) {
					g_printerr("%s.isn't regular file.\n", optarg);
					exit(1);
				}
			}
			milstr_ncpy(timidity_cfgfile_path, optarg,
			    sizeof(timidity_cfgfile_path));
			break;

		case 'v':
			verbose = 1;
			break;

		case 'h':
		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (modulefile[0] == '\0') {
		char* locate;	/* Don't free() */

		/* same dir ()*/
		locate = dirname(argv[0]);

		/* default dir */
		if (modulefile[0] == '\0') {
			char *config_home = getenv("XDG_CONFIG_HOME");
			char *home = getenv("HOME");

			if (config_home) {
				/* XDG_CONFIG_HOME dir */
				g_snprintf(modulefile, sizeof(modulefile),
					  "%s/%s", config_home, appname);
			} else if (home) {
				/* HOME dir */
				g_snprintf(modulefile, sizeof(modulefile),
					  "%s/.config/%s", home, appname);
			} else {
				g_printerr("$XDG_CONFIG_HOME or $HOME isn't defined.\n");
				exit(1);
			}
			if (stat(modulefile, &sb) < 0) {
				if (mkdir(modulefile, 0700) < 0) {
					g_printerr("Can't mkdir. %s\n",
						  modulefile);
					exit(1);
				}
			} else {
				if (!S_ISDIR(sb.st_mode)) {
					g_printerr("%s isn't directory.\n",
						  modulefile);
					exit(1);
				}
				if (access(modulefile, R_OK | W_OK) < 0) {
					g_printerr("Can't RW access. %s\n",
						  modulefile);
					exit(1);
				}
			}
		}

		/* default config file */
		milstr_ncat(modulefile, "/", sizeof(modulefile));
		milstr_ncat(modulefile, appname, sizeof(modulefile));
		milstr_ncat(modulefile, "rc", sizeof(modulefile));
		if (stat(modulefile, &sb) >= 0) {
			if (!S_ISREG(sb.st_mode)) {
				g_printerr("%s isn't regular file.\n",
				    modulefile);
				exit(1);
			}
			if(access(modulefile, R_OK | W_OK) < 0) {
				g_printerr("Can't RW access. %s\n",
				    modulefile);
				exit(1);
			}
		}
	}
	if (modulefile[0] != '\0') {
		/* font file */
		file_cpyname(fontfile, modulefile, sizeof(fontfile));
		file_cutname(fontfile);
		file_setseparator(fontfile, sizeof(fontfile));
		file_catname(fontfile, "default.ttf", sizeof(fontfile));
		fcheck = file_open_rb(fontfile);
		if (fcheck != NULL) {
			file_close(fcheck);
			fontmng_setdeffontname(fontfile);
		}

		/* font bmp file */
		file_cpyname(np2cfg.fontfile, modulefile,
		    sizeof(np2cfg.fontfile));
		file_cutname(np2cfg.fontfile);
		file_setseparator(np2cfg.fontfile, sizeof(np2cfg.fontfile));
		file_catname(np2cfg.fontfile, "font.bmp",
		    sizeof(np2cfg.fontfile));

		/* resume/statsave dir */
		file_cpyname(statpath, modulefile, sizeof(statpath));
		file_cutname(statpath);
		file_catname(statpath, "/sav/", sizeof(statpath));
		if (stat(statpath, &sb) < 0) {
			if (mkdir(statpath, 0700) < 0) {
				perror(statpath);
				exit(1);
			}
		} else if (!S_ISDIR(sb.st_mode)) {
			g_printerr("%s isn't directory.\n",
			    statpath);
			exit(1);
		}
		file_catname(statpath, appname, sizeof(statpath));
	}
	if (timidity_cfgfile_path[0] == '\0') {
		file_cpyname(timidity_cfgfile_path, modulefile,
		    sizeof(timidity_cfgfile_path));
		file_cutname(timidity_cfgfile_path);
		file_catname(timidity_cfgfile_path, "timidity.cfg",
		    sizeof(timidity_cfgfile_path));
	}

	dosio_init();
	file_setcd(modulefile);
	initload();
	toolwin_readini();
	kdispwin_readini();
	skbdwin_readini();
#if defined(SUPPORT_WAB)
	wabwin_readini();
#endif	// defined(SUPPORT_WAB)
#if defined(SUPPORT_HOSTDRV)
	hostdrv_readini();
#endif	// defined(SUPPORT_HOSTDRV)
	fcheck = file_open_rb(modulefile);
	if (fcheck == NULL)	{
		createini = 1;
	} else {
		file_close(fcheck);
	}

	rand_setseed((SINT32)time(NULL));

	mmxflag = havemmx() ? 0 : MMXFLAG_NOTSUPPORT;
	mmxflag += np2oscfg.disablemmx ? MMXFLAG_DISABLE : 0;

	TRACEINIT();

#if defined(SUPPORT_SDL_AUDIO) || defined(SUPPORT_SDL_MIXER)
	SDL_Init(0);
#endif

	if (fontmng_init() != SUCCESS)
		goto fontmng_failure;

	kdispwin_initialize();
	viewer_init();
	skbdwin_initialize();

	toolkit_widget_create();
	scrnmng_initialize();
	kbdmng_init();
	keystat_initialize();

	scrnmode = 0;
	if (np2cfg.RASTER) {
		scrnmode |= SCRNMODE_HIGHCOLOR;
	}
	if (scrnmng_create(scrnmode) != SUCCESS)
		goto scrnmng_failure;

	if (soundmng_initialize() == SUCCESS) {
		result = soundmng_pcmload(SOUND_PCMSEEK, file_getcd("fddseek.wav"));
		if (result != SUCCESS) {
			result = soundmng_pcmload(SOUND_PCMSEEK, SYSRESPATH "/fddseek.wav");
		}
		if (result == SUCCESS) {
			soundmng_pcmvolume(SOUND_PCMSEEK, np2cfg.MOTORVOL);
		}

		result = soundmng_pcmload(SOUND_PCMSEEK1, file_getcd("fddseek1.wav"));
		if (result != SUCCESS) {
			result = soundmng_pcmload(SOUND_PCMSEEK1, SYSRESPATH "/fddseek1.wav");
		}
		if (result == SUCCESS) {
			soundmng_pcmvolume(SOUND_PCMSEEK1, np2cfg.MOTORVOL);
		}

		result = soundmng_pcmload(SOUND_RELAY1, file_getcd("relay1.wav"));
		if (result != SUCCESS) {
			result = soundmng_pcmload(SOUND_RELAY1, SYSRESPATH "/relay1.wav");
		}
		if (result == SUCCESS) {
			soundmng_pcmvolume(SOUND_RELAY1, np2cfg.MOTORVOL);
		}
	}

	joymng_initialize();
	mousemng_initialize();
	if (np2oscfg.MOUSE_SW) {
		mouse_running(MOUSE_ON);
	}

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
	toolkit_widget_show();
	scrndraw_redraw();

	pccore_reset();

	if (!(scrnmode & SCRNMODE_FULLSCREEN)) {
		if (np2oscfg.toolwin) {
			toolwin_create();
		}
		if (np2oscfg.keydisp) {
			kdispwin_create();
		}
		if (np2oscfg.softkbd) {
			skbdwin_create();
		}
	}

#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume) {
		flagload(np2resumeext, "Resume", FALSE);
	}
#endif
	sysmng_workclockreset();

	drvmax = (argc < 4) ? argc : 4;
	for (i = 0; i < drvmax; i++) {
		diskdrv_readyfdd(i, argv[i], 0);
	}

	setup_signal(SIGINT, sighandler);
	setup_signal(SIGTERM, sighandler);

	toolkit_widget_mainloop();
	rv = 0;

	kdispwin_destroy();
	toolwin_destroy();
	skbdwin_destroy();

	pccore_cfgupdate();

	mouse_running(MOUSE_OFF);
	joymng_deinitialize();
	S98_trash();

#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume) {
		flagsave(np2resumeext);
	} else {
		flagdelete(np2resumeext);
	}
#endif
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
	debugwin_destroy();

	soundmng_deinitialize();
	scrnmng_destroy();

scrnmng_failure:
fontmng_failure:
	if ((!np2oscfg.readonly
	 && (sys_updates & (SYS_UPDATECFG|SYS_UPDATEOSCFG))) || createini) {
		initsave();
		toolwin_writeini();
		kdispwin_writeini();
		skbdwin_writeini();
#if defined(SUPPORT_HOSTDRV)
	hostdrv_writeini();
#endif	// defined(SUPPORT_HOSTDRV)
#if defined(SUPPORT_WAB)
	wabwin_writeini();
	np2wabcfg.readonly = np2oscfg.readonly;
#endif	// defined(SUPPORT_WAB)
	}

	skbdwin_deinitialize();

#if defined(SUPPORT_SDL_AUDIO) || defined(SUPPORT_SDL_MIXER)
	SDL_Quit();
#endif

	TRACETERM();
	dosio_term();

	viewer_term();
	toolkit_terminate();

	return rv;
}
