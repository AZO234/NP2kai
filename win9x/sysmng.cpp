#include	"compiler.h"
#include	"np2.h"
#include	"dosio.h"
#include	"sysmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"diskimage/fddfile.h"
#include	"fdd/diskdrv.h"
#if defined(SUPPORT_IDEIO)||defined(SUPPORT_SCSI)
#include	"fdd/sxsi.h"
#include	"resource.h"
#include	"win9x/dialog/np2class.h"
#include	"win9x/menu.h"
#endif

	UINT	sys_updates;

	SYSMNGMISCINFO	sys_miscinfo = {0};


// ----

static	OEMCHAR	title[2048] = {0};
static	OEMCHAR	clock[256] = {0};
static	OEMCHAR	misc[256] = {0};

static struct {
	UINT32	tick;
	UINT32	clock;
	UINT32	draws;
	SINT32	fps;
	SINT32	khz;
} workclock;

void sysmng_workclockreset(void) {

	workclock.tick = GETTICK();
	workclock.clock = CPU_CLOCK;
	workclock.draws = drawcount;
}

BOOL sysmng_workclockrenewal(void) {

	SINT32	tick;

	tick = GETTICK() - workclock.tick;
	if (tick < 2000) {
		return(FALSE);
	}
	workclock.tick += tick;
	workclock.fps = ((drawcount - workclock.draws) * 10000) / tick;
	workclock.draws = drawcount;
	workclock.khz = (CPU_CLOCK - workclock.clock) / tick;
	workclock.clock = CPU_CLOCK;
	return(TRUE);
}

OEMCHAR* DOSIOCALL sysmng_file_getname(OEMCHAR* lpPathName){
	if(_tcsnicmp(lpPathName, OEMTEXT("\\\\.\\"), 4)==0){
		return lpPathName;
	}else{
		return file_getname(lpPathName);
	}
}

void sysmng_updatecaption(UINT8 flag) {
	
#if defined(SUPPORT_IDEIO)||defined(SUPPORT_SCSI)
	int i, cddrvnum = 1;
#endif
	static OEMCHAR hddimgmenustrorg[4][MAX_PATH] = {0};
	static OEMCHAR hddimgmenustr[4][MAX_PATH] = {0};
#if defined(SUPPORT_SCSI)
	static OEMCHAR scsiimgmenustrorg[4][MAX_PATH] = {0};
	static OEMCHAR scsiimgmenustr[4][MAX_PATH] = {0};
#endif
	OEMCHAR	work[2048] = {0};
	OEMCHAR	fddtext[16] = {0};
	
	if (flag & 1) {
		title[0] = '\0';
		for(i=0;i<4;i++){
			OEMSPRINTF(fddtext, OEMTEXT("  FDD%d:"), i+1);
			if (fdd_diskready(i)) {
				milstr_ncat(title, fddtext, NELEMENTS(title));
				milstr_ncat(title, file_getname(fdd_diskname(i)), NELEMENTS(title));
			}
		}
#ifdef SUPPORT_IDEIO
		for(i=0;i<4;i++){
			if(sxsi_getdevtype(i)==SXSIDEV_CDROM){
				OEMSPRINTF(work, OEMTEXT("  CD%d:"), cddrvnum);
				if (sxsi_getdevtype(i)==SXSIDEV_CDROM && *(np2cfg.idecd[i])) {
					milstr_ncat(title, work, NELEMENTS(title));
					milstr_ncat(title, sysmng_file_getname(np2cfg.idecd[i]), NELEMENTS(title));
				}
				cddrvnum++;
			}
			if(g_hWndMain){
				OEMCHAR newtext[MAX_PATH*2+100];
				OEMCHAR *fname;
				OEMCHAR *fnamenext;
				OEMCHAR *fnametmp;
				OEMCHAR *fnamenexttmp;
				HMENU hMenu = np2class_gethmenu(g_hWndMain);
				HMENU hMenuTgt;
				int hMenuTgtPos;
				MENUITEMINFO mii = {0};
				menu_searchmenu(hMenu, IDM_IDE0STATE+i, &hMenuTgt, &hMenuTgtPos);
				if(hMenu){
					mii.cbSize = sizeof(MENUITEMINFO);
					if(!hddimgmenustrorg[i][0]){
						GetMenuString(hMenuTgt, IDM_IDE0STATE+i, hddimgmenustrorg[i], NELEMENTS(hddimgmenustrorg[0]), MF_BYCOMMAND);
					}
					if(np2cfg.idetype[i]==SXSIDEV_NC){
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[disabled]"));
					}else{
						fname = sxsi_getfilename(i);
						if(np2cfg.idetype[i]==SXSIDEV_CDROM){
							fnamenext = np2cfg.idecd[i];
						}else{
							fnamenext = (OEMCHAR*)diskdrv_getsxsi(i);
						}
						if(fname && *fname && fnamenext && *fnamenext && (fnametmp = sysmng_file_getname(fname))!=NULL && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
							_tcscpy(newtext, hddimgmenustrorg[i]);
							_tcscat(newtext, fnametmp);
							if(_tcscmp(fname, fnamenext)){
								_tcscat(newtext, OEMTEXT(" -> "));
								_tcscat(newtext, fnamenexttmp);
							}
						}else if(fnamenext && *fnamenext && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
							_tcscpy(newtext, hddimgmenustrorg[i]);
							_tcscat(newtext, OEMTEXT("[none] -> "));
							_tcscat(newtext, fnamenexttmp);
						}else if(fname && *fname && (fnametmp = sysmng_file_getname(fname))!=NULL){
							_tcscpy(newtext, hddimgmenustrorg[i]);
							_tcscat(newtext, fnametmp);
							_tcscat(newtext, OEMTEXT(" -> [none]"));
						}else{
							_tcscpy(newtext, hddimgmenustrorg[i]);
							_tcscat(newtext, OEMTEXT("[none]"));
						}
					}
					if(_tcscmp(newtext, hddimgmenustr[i])){
						_tcscpy(hddimgmenustr[i], newtext);
						mii.fMask = MIIM_TYPE;
						mii.fType = MFT_STRING;
						mii.dwTypeData = hddimgmenustr[i];
						mii.cch = (UINT)_tcslen(hddimgmenustr[i]);
						SetMenuItemInfo(hMenuTgt, IDM_IDE0STATE+i, MF_BYCOMMAND, &mii);
					}
				}
			}
		}
#else
		for(i=0;i<2;i++){
			if(g_hWndMain){
				OEMCHAR newtext[MAX_PATH*2+100];
				OEMCHAR *fname;
				OEMCHAR *fnamenext;
				OEMCHAR *fnametmp;
				OEMCHAR *fnamenexttmp;
				HMENU hMenu = np2class_gethmenu(g_hWndMain);
				HMENU hMenuTgt;
				int hMenuTgtPos;
				MENUITEMINFO mii = {0};
				menu_searchmenu(hMenu, IDM_IDE0STATE+i, &hMenuTgt, &hMenuTgtPos);
				if(hMenu){
					mii.cbSize = sizeof(MENUITEMINFO);
					if(!hddimgmenustrorg[i][0]){
						GetMenuString(hMenuTgt, IDM_IDE0STATE+i, hddimgmenustrorg[i], NELEMENTS(hddimgmenustrorg[0]), MF_BYCOMMAND);
					}
					fname = sxsi_getfilename(i);
					fnamenext = (OEMCHAR*)diskdrv_getsxsi(i);
					if(fname && *fname && fnamenext && *fnamenext && (fnametmp = sysmng_file_getname(fname))!=NULL && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, fnametmp);
						if(_tcscmp(fname, fnamenext)){
							_tcscat(newtext, OEMTEXT(" -> "));
							_tcscat(newtext, fnamenexttmp);
						}
					}else if(fnamenext && *fnamenext && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[none] -> "));
						_tcscat(newtext, fnamenexttmp);
					}else if(fname && *fname && (fnametmp = sysmng_file_getname(fname))!=NULL){
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, fnametmp);
						_tcscat(newtext, OEMTEXT(" -> [none]"));
					}else{
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[none]"));
					}
					if(_tcscmp(newtext, hddimgmenustr[i])){
						_tcscpy(hddimgmenustr[i], newtext);
						mii.fMask = MIIM_TYPE;
						mii.fType = MFT_STRING;
						mii.dwTypeData = hddimgmenustr[i];
						mii.cch = (UINT)_tcslen(hddimgmenustr[i]);
						SetMenuItemInfo(hMenuTgt, IDM_IDE0STATE+i, MF_BYCOMMAND, &mii);
					}
				}
			}
		}
#endif
#ifdef SUPPORT_SCSI
		for(i=0;i<4;i++){
			if(g_hWndMain){
				OEMCHAR newtext[MAX_PATH*2+100];
				OEMCHAR *fname;
				OEMCHAR *fnamenext;
				OEMCHAR *fnametmp;
				OEMCHAR *fnamenexttmp;
				HMENU hMenu = np2class_gethmenu(g_hWndMain);
				HMENU hMenuTgt;
				int hMenuTgtPos;
				MENUITEMINFO mii = {0};
				menu_searchmenu(hMenu, IDM_SCSI0STATE+i, &hMenuTgt, &hMenuTgtPos);
				if(hMenu){
					mii.cbSize = sizeof(MENUITEMINFO);
					if(!scsiimgmenustrorg[i][0]){
						GetMenuString(hMenuTgt, IDM_SCSI0STATE+i, scsiimgmenustrorg[i], NELEMENTS(scsiimgmenustrorg[0]), MF_BYCOMMAND);
					}
					fname = sxsi_getfilename(i+0x20);
					fnamenext = (OEMCHAR*)diskdrv_getsxsi(i+0x20);
					if(fname && *fname && fnamenext && *fnamenext && (fnametmp = sysmng_file_getname(fname))!=NULL && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
						_tcscpy(newtext, scsiimgmenustrorg[i]);
						_tcscat(newtext, fnametmp);
						if(_tcscmp(fname, fnamenext)){
							_tcscat(newtext, OEMTEXT(" -> "));
							_tcscat(newtext, fnamenexttmp);
						}
					}else if(fnamenext && *fnamenext && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
						_tcscpy(newtext, scsiimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[none] -> "));
						_tcscat(newtext, fnamenexttmp);
					}else if(fname && *fname && (fnametmp = sysmng_file_getname(fname))!=NULL){
						_tcscpy(newtext, scsiimgmenustrorg[i]);
						_tcscat(newtext, fnametmp);
						_tcscat(newtext, OEMTEXT(" -> [none]"));
					}else{
						_tcscpy(newtext, scsiimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[none]"));
					}
					if(_tcscmp(newtext, scsiimgmenustr[i])){
						_tcscpy(scsiimgmenustr[i], newtext);
						mii.fMask = MIIM_TYPE;
						mii.fType = MFT_STRING;
						mii.dwTypeData = scsiimgmenustr[i];
						mii.cch = (UINT)_tcslen(scsiimgmenustr[i]);
						SetMenuItemInfo(hMenuTgt, IDM_SCSI0STATE+i, MF_BYCOMMAND, &mii);
					}
				}
			}
		}
#endif
	}
	if (flag & 2) {
		clock[0] = '\0';
		if (np2oscfg.DISPCLK & 2) {
			if (workclock.fps) {
				OEMSPRINTF(clock, OEMTEXT(" - %u.%1uFPS"),
									workclock.fps / 10, workclock.fps % 10);
			}
			else {
				milstr_ncpy(clock, OEMTEXT(" - 0FPS"), NELEMENTS(clock));
			}
		}
		if (np2oscfg.DISPCLK & 1) {
			OEMSPRINTF(work, OEMTEXT(" %2u.%03uMHz"),
								workclock.khz / 1000, workclock.khz % 1000);
			if (clock[0] == '\0') {
				milstr_ncpy(clock, OEMTEXT(" -"), NELEMENTS(clock));
			}
			milstr_ncat(clock, work, NELEMENTS(clock));
#if 0
			OEMSPRINTF(work, OEMTEXT(" (debug: OPN %d / PSG %s)"),
							opngen.playing,
							(g_psg1.mixer & 0x3f)?OEMTEXT("ON"):OEMTEXT("OFF"));
			milstr_ncat(clock, work, NELEMENTS(clock));
#endif
		}
	}
	
	if (flag & 4) {
		misc[0] = '\0';
		if(sys_miscinfo.showvolume && sys_miscinfo.showmousespeed){
			OEMSPRINTF(misc, OEMTEXT(" (Volume: %d%%, Mouse speed: %d%%)"), np2cfg.vol_master, 100 * np2oscfg.mousemul/np2oscfg.mousediv);
		}else if(sys_miscinfo.showvolume){
			OEMSPRINTF(misc, OEMTEXT(" (Volume: %d%%)"), np2cfg.vol_master);
		}else if(sys_miscinfo.showmousespeed){
			OEMSPRINTF(misc, OEMTEXT(" (Mouse speed: %d%%)"), 100 * np2oscfg.mousemul/np2oscfg.mousediv);
		}
	}

	milstr_ncpy(work, np2oscfg.titles, NELEMENTS(work));
	milstr_ncat(work, misc, NELEMENTS(work));
	milstr_ncat(work, title, NELEMENTS(work));
	milstr_ncat(work, clock, NELEMENTS(work));
	SetWindowText(g_hWndMain, work);
}

