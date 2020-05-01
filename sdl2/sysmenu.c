#include	"compiler.h"
#include	"strres.h"
#include	"np2.h"
#include	"fontmng.h"
#include	"scrnmng.h"
#include	"sysmng.h"
#include	"taskmng.h"
#include	"kbtrans.h"
#include	"kbdmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"dosio.h"
#include	"pc9861k.h"
#include	"mpu98ii.h"
#if defined(SUPPORT_SMPU98)
#include	"smpu98.h"
#endif
#include	"sound.h"
#include	"beep.h"
#include	"fdd/diskdrv.h"
#include	"keystat.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"menustr.h"
#include	"sysmenu.h"
#include	"sysmenu.res"
#include	"sysmenu.str"
#include	"filesel.h"
#include	"dlgcfg.h"
#include	"dlgscr.h"
#include	"dlgwab.h"
#include	"dlgabout.h"
#include	"vram/scrnsave.h"
#include	"ini.h"
#ifdef SUPPORT_WAB
#include	"wab/wab.h"
#include	"wab/wabbmpsave.h"
#endif
#include	"font/font.h"

static UINT bmpno = 0;

/* Forward declarations */
extern void changescreen(UINT8 newmode);

static void sys_cmd(MENUID id) {

	UINT	update;

	update = 0;
	switch(id) {
		case MID_RESET:
#if defined(__LIBRETRO__)
			reset_lrkey();
#elif defined(NP2_SDL2)
			sdlkbd_reset();
#endif
			pccore_cfgupdate();
			pccore_reset();
			break;

		case MID_CONFIG:
			menudlg_create(DLGCFG_WIDTH, DLGCFG_HEIGHT,
											(char *)mstr_cfg, dlgcfg_cmd);
			break;

#if defined(SUPPORT_STATSAVE)
		case MID_SAVESTAT0:
			flagsave("s00");
			break;

		case MID_SAVESTAT1:
			flagsave("s01");
			break;

		case MID_SAVESTAT2:
			flagsave("s02");
			break;

		case MID_SAVESTAT3:
			flagsave("s03");
			break;

		case MID_SAVESTAT4:
			flagsave("s04");
			break;

		case MID_SAVESTAT5:
			flagsave("s05");
			break;

		case MID_SAVESTAT6:
			flagsave("s06");
			break;

		case MID_SAVESTAT7:
			flagsave("s07");
			break;

		case MID_SAVESTAT8:
			flagsave("s08");
			break;

		case MID_SAVESTAT9:
			flagsave("s09");
			break;

		case MID_LOADSTAT0:
			flagload("s00", "Status Load", TRUE);
			break;

		case MID_LOADSTAT1:
			flagload("s01", "Status Load", TRUE);
			break;

		case MID_LOADSTAT2:
			flagload("s02", "Status Load", TRUE);
			break;

		case MID_LOADSTAT3:
			flagload("s03", "Status Load", TRUE);
			break;

		case MID_LOADSTAT4:
			flagload("s04", "Status Load", TRUE);
			break;

		case MID_LOADSTAT5:
			flagload("s05", "Status Load", TRUE);
			break;

		case MID_LOADSTAT6:
			flagload("s06", "Status Load", TRUE);
			break;

		case MID_LOADSTAT7:
			flagload("s07", "Status Load", TRUE);
			break;

		case MID_LOADSTAT8:
			flagload("s08", "Status Load", TRUE);
			break;

		case MID_LOADSTAT9:
			flagload("s09", "Status Load", TRUE);
			break;
#endif	/* SUPPORT_STATSAVE */

		case MID_FDD1OPEN:
			filesel_fdd(0);
			break;

		case MID_FDD1EJECT:
			diskdrv_setfdd(0, NULL, 0);
			break;

		case MID_FDD2OPEN:
			filesel_fdd(1);
			break;

		case MID_FDD2EJECT:
			diskdrv_setfdd(1, NULL, 0);
			break;

		case MID_FDD3OPEN:
			filesel_fdd(2);
			break;

		case MID_FDD3EJECT:
			diskdrv_setfdd(2, NULL, 0);
			break;

		case MID_FDD4OPEN:
			filesel_fdd(3);
			break;

		case MID_FDD4EJECT:
			diskdrv_setfdd(3, NULL, 0);
			break;

#if defined(SUPPORT_IDEIO)
		case MID_IDE1OPEN:
			filesel_hdd(0x00);
			break;

		case MID_IDE1EJECT:
			diskdrv_setsxsi(0x00, NULL);
			break;

		case MID_IDE2OPEN:
			filesel_hdd(0x01);
			break;

		case MID_IDE2EJECT:
			diskdrv_setsxsi(0x01, NULL);
			break;

		case MID_IDECDOPEN:
			filesel_hdd(0x02);
			break;

		case MID_IDECDEJECT:
			diskdrv_setsxsi(0x02, NULL);
			break;
#else
		case MID_SASI1OPEN:
			filesel_hdd(0x00);
			break;

		case MID_SASI1EJECT:
			diskdrv_setsxsi(0x00, NULL);
			break;

		case MID_SASI2OPEN:
			filesel_hdd(0x01);
			break;

		case MID_SASI2EJECT:
			diskdrv_setsxsi(0x01, NULL);
			break;
#endif
#if defined(SUPPORT_SCSI)
		case MID_SCSI0OPEN:
			filesel_hdd(0x20);
			break;

		case MID_SCSI0EJECT:
			diskdrv_setsxsi(0x20, NULL);
			break;

		case MID_SCSI1OPEN:
			filesel_hdd(0x21);
			break;

		case MID_SCSI1EJECT:
			diskdrv_setsxsi(0x21, NULL);
			break;

		case MID_SCSI2OPEN:
			filesel_hdd(0x22);
			break;

		case MID_SCSI2EJECT:
			diskdrv_setsxsi(0x22, NULL);
			break;

		case MID_SCSI3OPEN:
			filesel_hdd(0x23);
			break;

		case MID_SCSI3EJECT:
			diskdrv_setsxsi(0x23, NULL);
			break;
#endif
		case MID_ROLNORMAL:
			changescreen((scrnmode & ~SCRNMODE_ROTATEMASK) | 0);
			break;

		case MID_ROLLEFT:
			changescreen((scrnmode & ~SCRNMODE_ROTATEMASK) | SCRNMODE_ROTATELEFT);
			break;

		case MID_ROLRIGHT:
			changescreen((scrnmode & ~SCRNMODE_ROTATEMASK) | SCRNMODE_ROTATERIGHT);
			break;

		case MID_DISPSYNC:
			np2cfg.DISPSYNC ^= 1;
			update |= SYS_UPDATECFG;
			break;

		case MID_RASTER:
			np2cfg.RASTER ^= 1;
			update |= SYS_UPDATECFG;
			break;

		case MID_NOWAIT:
			np2oscfg.NOWAIT ^= 1;
			update |= SYS_UPDATECFG;
			break;

#if defined(SUPPORT_ASYNC_CPU)
		case MID_ASYNCCPU:
			np2cfg.asynccpu ^= 1;
			update |= SYS_UPDATECFG;
			break;
#endif

		case MID_AUTOFPS:
			np2oscfg.DRAW_SKIP = 0;
			update |= SYS_UPDATECFG;
			break;

		case MID_60FPS:
			np2oscfg.DRAW_SKIP = 1;
			update |= SYS_UPDATECFG;
			break;

		case MID_30FPS:
			np2oscfg.DRAW_SKIP = 2;
			update |= SYS_UPDATECFG;
			break;

		case MID_20FPS:
			np2oscfg.DRAW_SKIP = 3;
			update |= SYS_UPDATECFG;
			break;

		case MID_15FPS:
			np2oscfg.DRAW_SKIP = 4;
			update |= SYS_UPDATECFG;
			break;

		case MID_SCREENOPT:
			menudlg_create(DLGSCR_WIDTH, DLGSCR_HEIGHT,
											(char *)mstr_scropt, dlgscr_cmd);
			break;

#if defined(SUPPORT_WAB) && defined(SUPPORT_CL_GD5430)
		case MID_WABOPT:
			menudlg_create(DLGWAB_WIDTH, DLGWAB_HEIGHT,
											(char *)mstr_wabopt, dlgwab_cmd);
			break;
#endif

		case MID_KEY:
			np2cfg.KEY_MODE = 0;
			keystat_resetjoykey();
			update |= SYS_UPDATECFG;
			break;

		case MID_JOY1:
			np2cfg.KEY_MODE = 1;
			keystat_resetjoykey();
			update |= SYS_UPDATECFG;
			break;

		case MID_JOY2:
			np2cfg.KEY_MODE = 2;
			keystat_resetjoykey();
			update |= SYS_UPDATECFG;
			break;

		case MID_MOUSEKEY:
			np2cfg.KEY_MODE = 3;
			keystat_resetjoykey();
			update |= SYS_UPDATECFG;
			break;

		case MID_XSHIFT:
			np2cfg.XSHIFT ^= 1;
			keystat_forcerelease(0x70);
			update |= SYS_UPDATECFG;
			break;

		case MID_XCTRL:
			np2cfg.XSHIFT ^= 2;
			keystat_forcerelease(0x74);
			update |= SYS_UPDATECFG;
			break;

		case MID_XGRPH:
			np2cfg.XSHIFT ^= 4;
			keystat_forcerelease(0x73);
			update |= SYS_UPDATECFG;
			break;

		case MID_XROLL:
			np2oscfg.xrollkey ^= 1;
			keystat_forcerelease(0x36);
			keystat_forcerelease(0x37);
			update |= SYS_UPDATEOSCFG;
			break;

		case MID_KEYBOARD_106:
			np2oscfg.KEYBOARD = KEY_KEY106;
			update |= SYS_UPDATEOSCFG;
			break;

		case MID_KEYBOARD_101:
			np2oscfg.KEYBOARD = KEY_KEY101;
			update |= SYS_UPDATEOSCFG;
			break;

		case MID_KEY_COPY:
			keystat_senddata(0x61);
			keystat_senddata(0x61 | 0x80);
			break;

		case MID_KEY_KANA:
			keystat_senddata(0x72);
			keystat_senddata(0x72 | 0x80);
			break;

		case MID_KEY_YEN:
			keystat_senddata(0x0d);
			keystat_senddata(0x0d | 0x80);
			break;

		case MID_KEY_SYEN:
			keystat_senddata(0x70);
			keystat_senddata(0x0d);
			keystat_senddata(0x0d | 0x80);
			keystat_senddata(0x70 | 0x80);
			break;

		case MID_KEY_AT:
			keystat_senddata(0x1a);
			keystat_senddata(0x1a | 0x80);
			break;

		case MID_KEY_SAT:
			keystat_senddata(0x70);
			keystat_senddata(0x1a);
			keystat_senddata(0x1a | 0x80);
			keystat_senddata(0x70 | 0x80);
			break;

		case MID_KEY_UB:
			keystat_senddata(0x33);
			keystat_senddata(0x33 | 0x80);
			break;

		case MID_KEY_SUB:
			keystat_senddata(0x70);
			keystat_senddata(0x33);
			keystat_senddata(0x33 | 0x80);
			keystat_senddata(0x70 | 0x80);
			break;

		case MID_KEY_KPEQUALS:
			keystat_senddata(0x4d);
			keystat_senddata(0x4d | 0x80);
			break;

		case MID_KEY_KPCOMMA:
			keystat_senddata(0x4f);
			keystat_senddata(0x4f | 0x80);
			break;

		case MID_SNDCAD:
			keystat_senddata(0x73);
			keystat_senddata(0x74);
			keystat_senddata(0x39);
			keystat_senddata(0x73 | 0x80);
			keystat_senddata(0x74 | 0x80);
			keystat_senddata(0x39 | 0x80);
			break;

		case MID_BEEPOFF:
			np2cfg.BEEP_VOL = 0;
			beep_setvol(0);
			update |= SYS_UPDATECFG;
			break;

		case MID_BEEPLOW:
			np2cfg.BEEP_VOL = 1;
			beep_setvol(1);
			update |= SYS_UPDATECFG;
			break;

		case MID_BEEPMID:
			np2cfg.BEEP_VOL = 2;
			beep_setvol(2);
			update |= SYS_UPDATECFG;
			break;

		case MID_BEEPHIGH:
			np2cfg.BEEP_VOL = 3;
			beep_setvol(3);
			update |= SYS_UPDATECFG;
			break;

		case MID_NOSOUND:
			np2cfg.SOUND_SW = SOUNDID_NONE;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_14:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_14;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_26K:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_26K;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_86:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_26_86:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_26K;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_86_CB:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_ADPCM;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_118:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_118;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_86_MX:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_WSS;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_86_118:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_118;
			update |= SYS_UPDATECFG;
			break;

		case MID_PC9801_MX:
			np2cfg.SOUND_SW = SOUNDID_MATE_X_PCM;
			update |= SYS_UPDATECFG;
			break;

		case MID_SPEAKBOARD:
			np2cfg.SOUND_SW = SOUNDID_SPEAKBOARD;
			update |= SYS_UPDATECFG;
			break;

		case MID_SPEAKBOARD86:
			np2cfg.SOUND_SW = SOUNDID_86_SPEAKBOARD;
			update |= SYS_UPDATECFG;
			break;

		case MID_SPARKBOARD:
			np2cfg.SOUND_SW = SOUNDID_SPARKBOARD;
			update |= SYS_UPDATECFG;
			break;

		case MID_SOUNDORCHESTRA:
			np2cfg.SOUND_SW = SOUNDID_SOUNDORCHESTRA;
			update |= SYS_UPDATECFG;
			break;

		case MID_SOUNDORCHESTRAV:
			np2cfg.SOUND_SW = SOUNDID_SOUNDORCHESTRAV;
			update |= SYS_UPDATECFG;
			break;

		case MID_LITTLEORCHESTRAL:
			np2cfg.SOUND_SW = SOUNDID_LITTLEORCHESTRAL;
			update |= SYS_UPDATECFG;
			break;

		case MID_MMORCHESTRA:
			np2cfg.SOUND_SW = SOUNDID_MMORCHESTRA;
			update |= SYS_UPDATECFG;
			break;

#if defined(SUPPORT_SOUND_SB16)
		case MID_SB16:
			np2cfg.SOUND_SW = SOUNDID_SB16;
			update |= SYS_UPDATECFG;
			break;

		case MID_86_SB16:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_SB16;
			update |= SYS_UPDATECFG;
			break;

		case MID_MX_SB16:
			np2cfg.SOUND_SW = SOUNDID_WSS_SB16;
			update |= SYS_UPDATECFG;
			break;

		case MID_118_SB16:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_118_SB16;
			update |= SYS_UPDATECFG;
			break;

		case MID_86MXSB16:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_WSS_SB16;
			update |= SYS_UPDATECFG;
			break;

		case MID_86118SB16:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_118_SB16;
			update |= SYS_UPDATECFG;
			break;
#endif

		case MID_AMD98:
			np2cfg.SOUND_SW = SOUNDID_AMD98;
			update |= SYS_UPDATECFG;
			break;

		case MID_WAVESTAR:
			np2cfg.SOUND_SW = SOUNDID_WAVESTAR;
			update |= SYS_UPDATECFG;
			break;

#if defined(SUPPORT_PX)
		case MID_PX1:
			np2cfg.SOUND_SW = SOUNDID_PX1;
			update |= SYS_UPDATECFG;
			break;

		case MID_PX2:
			np2cfg.SOUND_SW = SOUNDID_PX2;
			update |= SYS_UPDATECFG;
			break;
#endif

		case MID_PC9801_118_ROM:
			np2cfg.snd118rom ^= 1;
			update |= SYS_UPDATECFG;
			break;

#if defined(SUPPORT_FMGEN)
		case MID_FMGEN:
			np2cfg.usefmgen ^= 1;
			update |= SYS_UPDATECFG;
			break;
#endif

		case MID_JASTSND:
			np2oscfg.jastsnd ^= 1;
			update |= SYS_UPDATEOSCFG;
			break;

		case MID_SEEKSND:
			np2cfg.MOTOR ^= 1;
			update |= SYS_UPDATECFG;
			break;
#if 0
		case IDM_SNDOPT:
			winuienter();
			dialog_sndopt(hWnd);
			winuileave();
			break;
#endif
		case MID_MEM640:
			np2cfg.EXTMEM = 0;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM16:
			np2cfg.EXTMEM = 1;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM36:
			np2cfg.EXTMEM = 3;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM76:
			np2cfg.EXTMEM = 7;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM96:
			np2cfg.EXTMEM = 9;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM136:
			np2cfg.EXTMEM = 13;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM166:
			np2cfg.EXTMEM = 16;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM326:
			np2cfg.EXTMEM = 32;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM646:
			np2cfg.EXTMEM = 64;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM1206:
			np2cfg.EXTMEM = 120;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM2306:
			np2cfg.EXTMEM = 230;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM5126:
			np2cfg.EXTMEM = 512;
			update |= SYS_UPDATECFG;
			break;

		case MID_MEM10246:
			np2cfg.EXTMEM = 1024;
			update |= SYS_UPDATECFG;
			break;
#if 0
		case IDM_SERIAL1:
			winuienter();
			dialog_serial(hWnd);
			winuileave();
			break;

		case IDM_MPUPC98:
			winuienter();
			DialogBox(hInst, MAKEINTRESOURCE(IDD_MPUPC98),
									hWnd, (DLGPROC)MidiDialogProc);
			winuileave();
			break;
#endif
		case MID_MIDIPANIC:
			rs232c_midipanic();
			mpu98ii_midipanic();
#if defined(SUPPORT_SMPU98)
			smpu98_midipanic();
#endif
			pc9861k_midipanic();
			break;

		case MID_BMPSAVE:
			{
				SCRNSAVE bmp = NULL;
				char path[MAX_PATH];

#ifdef SUPPORT_WAB
			if(np2wab.relay){
				sprintf(path, "%s%06d.bmp", bmpfilefolder, bmpfilenumber);
				np2wab_writebmp(path);
			}else{
#endif
				bmp = scrnsave_create();
				if (bmp == NULL)
					break;
				sprintf(path, "%s%06d.bmp", bmpfilefolder, bmpfilenumber);
				scrnsave_writebmp(bmp, path, SCRNSAVE_AUTO);
				scrnsave_destroy(bmp);
#ifdef SUPPORT_WAB
			}
#endif

				bmpfilenumber++;
				if(bmpfilenumber >= 1000000) {
					bmpfilenumber = 0;
				}
				initsave();
			}
			break;

		case MID_HF_ENABLE:
			hf_enable ^= 1;
			if(hf_enable) {
				hook_fontrom_defenable();
			} else {
				hook_fontrom_defdisable();
			}
			break;

#if 0
		case IDM_S98LOGGING:
			winuienter();
			dialog_s98(hWnd);
			winuileave();
			break;

		case IDM_DISPCLOCK:
			xmenu_setdispclk(np2oscfg.DISPCLK ^ 1);
			update |= SYS_UPDATECFG;
			break;

		case IDM_DISPFRAME:
			xmenu_setdispclk(np2oscfg.DISPCLK ^ 2);
			update |= SYS_UPDATECFG;
			break;

		case IDM_CALENDAR:
			winuienter();
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CALENDAR),
									hWnd, (DLGPROC)ClndDialogProc);
			winuileave();
			break;

		case IDM_ALTENTER:
			xmenu_setshortcut(np2oscfg.shortcut ^ 1);
			update |= SYS_UPDATECFG;
			break;

		case IDM_ALTF4:
			xmenu_setshortcut(np2oscfg.shortcut ^ 2);
			update |= SYS_UPDATECFG;
			break;
#endif
		case MID_JOYX:
			np2cfg.BTN_MODE ^= 1;
			update |= SYS_UPDATECFG;
			break;

		case MID_RAPID:
			np2cfg.BTN_RAPID ^= 1;
			update |= SYS_UPDATECFG;
			break;

		case MID_MSRAPID:
			np2cfg.MOUSERAPID ^= 1;
			update |= SYS_UPDATECFG;
			break;

		case MID_ITFWORK:
			np2cfg.ITF_WORK ^= 1;
			update |= SYS_UPDATECFG;
			break;

		case MID_FIXMMTIMER:
			np2cfg.timerfix ^= 1;
			update |= SYS_UPDATECFG;
			break;
			
		case MID_WINNTIDEFIX:
 			np2cfg.winntfix ^= 1;
 			update |= SYS_UPDATECFG;
 			break;
			
		case MID_SKIP16MBMEMCHK:
			if(np2cfg.memchkmx == 0)
				np2cfg.memchkmx = 15;
			else
				np2cfg.memchkmx = 0;
			update |= SYS_UPDATECFG;
			break;

#if defined(SUPPORT_FAST_MEMORYCHECK)
		case MID_FASTMEMCHK:
			if(np2cfg.memcheckspeed == 1)
				np2cfg.memcheckspeed = 8;
			else
				np2cfg.memcheckspeed = 1;
			update |= SYS_UPDATECFG;
			break;
#endif

		case MID_ABOUT:
			menudlg_create(DLGABOUT_WIDTH, DLGABOUT_HEIGHT,
											(char *)mstr_about, dlgabout_cmd);
			break;

#if defined(MENU_TASKMINIMIZE)
		case SID_MINIMIZE:
			taskmng_minimize();
			break;
#endif
		case MID_EXIT:
		case SID_CLOSE:
			taskmng_exit();
			break;
	}
	sysmng_update(update);
}


// ----

BRESULT sysmenu_create(void) {

	if (menubase_create() != SUCCESS) {
		goto smcre_err;
	}
	menuicon_regist(MICON_NP2, &np2icon);
#if defined(SUPPORT_STATSAVE)
	if(np2cfg.statsave) {
		s_main[1].child = s_statee;
	}
#endif	/* SUPPORT_STATSAVE */
	if(np2cfg.fddrive3) {
		if(np2cfg.fddrive4) {
			s_main[2].child = s_fddf;
		} else {
			s_main[2].child = s_fdd123;
		}
	} else {
		if(np2cfg.fddrive4) {
			s_main[2].child = s_fdd124;
		}
	}
	if (menusys_create(s_main, sys_cmd, MICON_NP2, str_np2)) {
		goto smcre_err;
	}
	return(SUCCESS);

smcre_err:
	return(FAILURE);
}

void sysmenu_destroy(void) {

	menubase_close();
	menubase_destroy();
	menusys_destroy();
}

BRESULT sysmenu_menuopen(UINT menutype, int x, int y) {

	UINT8	b;

	menusys_setcheck(MID_DISPSYNC, (np2cfg.DISPSYNC & 1));
	menusys_setcheck(MID_RASTER, (np2cfg.RASTER & 1));
	menusys_setcheck(MID_NOWAIT, (np2oscfg.NOWAIT & 1));
#if defined(SUPPORT_ASYNC_CPU)
	menusys_setcheck(MID_ASYNCCPU, (np2cfg.asynccpu & 1));
#endif
	menusys_setcheck(MID_ROLNORMAL, ((scrnmode & SCRNMODE_ROTATEMASK) == 0));
	menusys_setcheck(MID_ROLLEFT,   ((scrnmode & SCRNMODE_ROTATEMASK) == SCRNMODE_ROTATELEFT));
	menusys_setcheck(MID_ROLRIGHT,  ((scrnmode & SCRNMODE_ROTATEMASK) == SCRNMODE_ROTATERIGHT));
	b = np2oscfg.DRAW_SKIP;
	menusys_setcheck(MID_AUTOFPS, (b == 0));
	menusys_setcheck(MID_60FPS, (b == 1));
	menusys_setcheck(MID_30FPS, (b == 2));
	menusys_setcheck(MID_20FPS, (b == 3));
	menusys_setcheck(MID_15FPS, (b == 4));
	b = np2oscfg.KEYBOARD;
	menusys_setcheck(MID_KEYBOARD_106, (b == KEY_KEY106));
	menusys_setcheck(MID_KEYBOARD_101, (b == KEY_KEY101));
	b = np2cfg.KEY_MODE;
	menusys_setcheck(MID_KEY, (b == 0));
	menusys_setcheck(MID_JOY1, (b == 1));
	menusys_setcheck(MID_JOY2, (b == 2));
	menusys_setcheck(MID_MOUSEKEY, (b == 3));
	b = np2cfg.XSHIFT;
	menusys_setcheck(MID_XSHIFT, (b & 1));
	menusys_setcheck(MID_XCTRL, (b & 2));
	menusys_setcheck(MID_XGRPH, (b & 4));
	menusys_setcheck(MID_XROLL, (np2oscfg.xrollkey & 1));
	b = np2cfg.BEEP_VOL & 3;
	menusys_setcheck(MID_BEEPOFF, (b == 0));
	menusys_setcheck(MID_BEEPLOW, (b == 1));
	menusys_setcheck(MID_BEEPMID, (b == 2));
	menusys_setcheck(MID_BEEPHIGH, (b == 3));
	b = np2cfg.SOUND_SW;
	menusys_setcheck(MID_NOSOUND, (b == 0x00));
	menusys_setcheck(MID_PC9801_14, (b == 0x01));
	menusys_setcheck(MID_PC9801_26K, (b == 0x02));
	menusys_setcheck(MID_PC9801_86, (b == 0x04));
	menusys_setcheck(MID_PC9801_26_86, (b == 0x06));
	menusys_setcheck(MID_PC9801_86_CB, (b == 0x14));
	menusys_setcheck(MID_PC9801_118, (b == 0x08));
	menusys_setcheck(MID_SPEAKBOARD, (b == 0x20));
	menusys_setcheck(MID_SPEAKBOARD86, (b == 0x24));
	menusys_setcheck(MID_SPARKBOARD, (b == 0x40));
	menusys_setcheck(MID_SOUNDORCHESTRA, (b == 0x32));
	menusys_setcheck(MID_SOUNDORCHESTRAV, (b == 0x82));
	menusys_setcheck(MID_AMD98, (b == 0x80));
	menusys_setcheck(MID_WAVESTAR, (b == 0x70));
#if defined(SUPPORT_SOUND_SB16)
	menusys_setcheck(MID_SB16, (b == 0x41));
#endif	/* SUPPORT_SOUND_SB16 */
#if defined(SUPPORT_PX)
	menusys_setcheck(MID_PX1, (b == 0x30));
	menusys_setcheck(MID_PX2, (b == 0x50));
#endif	/* defined(SUPPORT_PX) */
	menusys_setcheck(MID_PC9801_118_ROM, (np2cfg.snd118rom & 1));
#if defined(SUPPORT_FMGEN)
	menusys_setcheck(MID_FMGEN, (np2cfg.usefmgen & 1));
#endif	/* SUPPORT_FMGEN */
	menusys_setcheck(MID_JASTSND, (np2oscfg.jastsnd & 1));
	menusys_setcheck(MID_SEEKSND, (np2cfg.MOTOR & 1));
	b = np2cfg.EXTMEM;
	menusys_setcheck(MID_MEM640, (b == 0));
	menusys_setcheck(MID_MEM16, (b == 1));
	menusys_setcheck(MID_MEM36, (b == 3));
	menusys_setcheck(MID_MEM76, (b == 7));
	menusys_setcheck(MID_MEM96, (b == 9));
	menusys_setcheck(MID_MEM136, (b == 13));
	menusys_setcheck(MID_MEM166, (b == 16));
	menusys_setcheck(MID_MEM326, (b == 32));
	menusys_setcheck(MID_MEM646, (b == 64));
	menusys_setcheck(MID_MEM1206, (b == 120));
	menusys_setcheck(MID_MEM2306, (b == 230));
	menusys_setcheck(MID_MEM5126, (b == 512));
	menusys_setcheck(MID_MEM10246, (b == 1024));
	menusys_setcheck(MID_JOYX, (np2cfg.BTN_MODE & 1));
	menusys_setcheck(MID_RAPID, (np2cfg.BTN_RAPID & 1));
	menusys_setcheck(MID_MSRAPID, (np2cfg.MOUSERAPID & 1));
	menusys_setcheck(MID_ITFWORK, (np2cfg.ITF_WORK & 1));
	menusys_setcheck(MID_FIXMMTIMER, (np2cfg.timerfix & 1));
	menusys_setcheck(MID_WINNTIDEFIX, (np2cfg.winntfix & 1));
	menusys_setcheck(MID_SKIP16MBMEMCHK, (np2cfg.memchkmx != 0));
#if defined(SUPPORT_FAST_MEMORYCHECK)
	menusys_setcheck(MID_FASTMEMCHK, (np2cfg.memcheckspeed > 1));
#endif
	menusys_setcheck(MID_HF_ENABLE, (hf_enable == 1));
	return(menusys_open(x, y));
}

