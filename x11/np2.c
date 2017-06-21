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

#include "compiler.h"

#include "np2.h"
#include "pccore.h"
#include "statsave.h"
#include "dosio.h"
#include "scrndraw.h"
#include "timing.h"
#include "toolkit.h"

#include "kdispwin.h"
#include "toolwin.h"
#include "viewer.h"
#include "debugwin.h"
#include "skbdwin.h"

#include "commng.h"
#include "joymng.h"
#include "kbdmng.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "sysmng.h"
#include "taskmng.h"


NP2OSCFG np2oscfg = {
#if !defined(CPUCORE_IA32)		/* titles */
	"Neko Project II",
#else
	"Neko Project II + IA32",
#endif

	0, 			/* paddingx */
	0,			/* paddingy */

	0,			/* NOWAIT */
	0,			/* DRAW_SKIP */

	0,			/* DISPCLK */

	KEY_KEY106,		/* KEYBOARD */
	0,			/* F12KEY */

	0,			/* MOUSE_SW */

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

	{ COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", "" },	/* mpu */
	{
		{ COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com1 */
		{ COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com2 */
		{ COMPORT_NONE, 0, 0x3e, 19200, "", "", "", "" },/* com3 */
	},

	0,			/* confirm */

	0,			/* resume */

	0,			/* statsave */
	0,			/* toolwin */
	0,			/* keydisp */
	0,			/* softkbd */
	0,			/* hostdrv_write */
	0,			/* jastsnd */
	0,			/* I286SAVE */

	SNDDRV_SDL,		/* snddrv */
	{ "", "" }, 		/* MIDIDEV */
	0,			/* MIDIWAIT */

	MOUSE_RATIO_100,	/* mouse_move_ratio */

	MMXFLAG_DISABLE,	/* disablemmx */
	INTERP_NEAREST,		/* drawinterp */
	0,			/* F11KEY */

	FALSE,			/* cfgreadonly */
};

volatile sig_atomic_t np2running = 0;
UINT8 scrnmode = 0;
int ignore_fullscreen_mode = 0;

UINT framecnt = 0;
UINT waitcnt = 0;
UINT framemax = 1;

BOOL s98logging = FALSE;
int s98log_count = 0;

char hddfolder[MAX_PATH];
char fddfolder[MAX_PATH];
char bmpfilefolder[MAX_PATH];
char modulefile[MAX_PATH];
char statpath[MAX_PATH];

const char np2flagext[] = "s%02d";
const char np2resumeext[] = "sav";

#ifndef FONTFACE
#define FONTFACE "-misc-fixed-%s-r-normal--%d-*-*-*-*-*-*-*"
#endif
char fontname[1024] = FONTFACE;

char timidity_cfgfile_path[MAX_PATH];

int verbose = 0;


UINT32
gettick(void)
{
	struct timeval tv;

	gettimeofday(&tv, 0);
	return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}

static void
getstatfilename(char* path, const char* ext, int size)
{

	/*
	 * default:
	 * e.g. resume:   "/home/user_name/.np2/sav/np2.sav"
	 *      statpath: "/home/user_name/.np2/sav/np2.s00"
	 *      config:   "/home/user_name/.np2/np2rc"
	 *
	 * --config option:
	 * e.g. resume:   "/config_file_path/sav/np2.sav"
	 *      statpath: "/config_file_path/sav/np2.s00"
	 *      config:   "/config_file_path/config_file_name"
	 */
	file_cpyname(path, statpath, size);
	file_catname(path, ".", size);
	file_catname(path, ext, size);
}

int
flagsave(const char* ext)
{
	char path[MAX_PATH];
	int ret;

	getstatfilename(path, ext, sizeof(path));
	soundmng_stop();
	ret = statsave_save(path);
	if (ret) {
		file_delete(path);
	}
	soundmng_play();

	return ret;
}

void
flagdelete(const char* ext)
{
	char path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	file_delete(path);
}

int
flagload(const char* ext, const char* title, BOOL force)
{
	char path[MAX_PATH];
	char buf[1024];
	int ret;
	int rv = 0;

	getstatfilename(path, ext, sizeof(path));
	ret = statsave_check(path, buf, sizeof(buf));
	if (ret & (~STATFLAG_DISKCHG)) {
		toolkit_msgbox(title, "Couldn't restart",
		    TK_MB_OK|TK_MB_ICON_ERROR);
		rv = 1;
	} else if ((!force) && (ret & STATFLAG_DISKCHG)) {
		ret = toolkit_msgbox(title, "Conflict!\nContinue?",
		    TK_MB_YESNO|TK_MB_ICON_QUESTION);
		if (ret != TK_MB_YES) {
			rv = 1;
		}
	}
	if (rv == 0) {
		statsave_load(path);
		toolwin_setfdd(0, fdd_diskname(0));
		toolwin_setfdd(1, fdd_diskname(1));
	}
	sysmng_workclockreset();
	sysmng_updatecaption(1);

	return rv;
}

void
changescreen(UINT8 newmode)
{
	UINT8 change;
	UINT8 renewal;

	change = scrnmode ^ newmode;
	renewal = (change & SCRNMODE_FULLSCREEN);
	if (newmode & SCRNMODE_FULLSCREEN) {
		renewal |= (change & SCRNMODE_HIGHCOLOR);
	} else {
		renewal |= (change & SCRNMODE_ROTATEMASK);
	}
	if (renewal) {
		if (renewal & SCRNMODE_FULLSCREEN) {
			toolwin_destroy();
			kdispwin_destroy();
		}
		soundmng_stop();
		mouse_running(MOUSE_STOP);
		scrnmng_destroy();
		if (scrnmng_create(newmode) == SUCCESS) {
			scrnmode = newmode;
		} else {
			if (scrnmng_create(scrnmode) != SUCCESS) {
				toolkit_widget_quit();
				return;
			}
		}
		scrndraw_redraw();
		if (renewal & SCRNMODE_FULLSCREEN) {
			if (!scrnmng_isfullscreen()) {
				if (np2oscfg.toolwin) {
					toolwin_create();
				}
				if (np2oscfg.keydisp) {
					kdispwin_create();
				}
			}
		}
		mouse_running(MOUSE_CONT);
		soundmng_play();
	} else {
		scrnmode = newmode;
	}
}

void
framereset(UINT cnt)
{

	framecnt = 0;
	scrnmng_dispclock();
	kdispwin_draw((UINT8)cnt);
	skbdwin_process();
	debugwin_process();
	toolwin_draw((UINT8)cnt);
	viewer_allreload(FALSE);
	if (np2oscfg.DISPCLK & 3) {
		if (sysmng_workclockrenewal()) {
			sysmng_updatecaption(3);
		}
	}
}

void
processwait(UINT cnt)
{

	if (timing_getcount() >= cnt) {
		timing_setcount(0);
		framereset(cnt);
	} else {
		taskmng_sleep(1);
	}
}

int
mainloop(void *p)
{

	if (np2oscfg.NOWAIT) {
		joymng_sync();
		mousemng_callback();
		pccore_exec(framecnt == 0);
		if (np2oscfg.DRAW_SKIP) {
			/* nowait frame skip */
			framecnt++;
			if (framecnt >= np2oscfg.DRAW_SKIP) {
				processwait(0);
			}
		} else {
			/* nowait auto skip */
			framecnt = 1;
			if (timing_getcount()) {
				processwait(0);
			}
		}
	} else if (np2oscfg.DRAW_SKIP) {
		/* frame skip */
		if (framecnt < np2oscfg.DRAW_SKIP) {
			joymng_sync();
			mousemng_callback();
			pccore_exec(framecnt == 0);
			framecnt++;
		} else {
			processwait(np2oscfg.DRAW_SKIP);
		}
	} else {
		/* auto skip */
		if (waitcnt == 0) {
			UINT cnt;
			joymng_sync();
			mousemng_callback();
			pccore_exec(framecnt == 0);
			framecnt++;
			cnt = timing_getcount();
			if (framecnt > cnt) {
				waitcnt = framecnt;
				if (framemax > 1) {
					framemax--;
				}
			} else if (framecnt >= framemax) {
				if (framemax < 12) {
					framemax++;
				}
				if (cnt >= 12) {
					timing_reset();
				} else {
					timing_setcount(cnt - framecnt);
				}
				framereset(0);
			}
		} else {
			processwait(waitcnt);
			waitcnt = framecnt;
		}
	}

	return TRUE;
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
