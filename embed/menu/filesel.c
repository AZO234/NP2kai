#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"soundmng.h"
#include	"pccore.h"
#include	"fdd/diskdrv.h"
#include	"diskimage/fddfile.h"
#include	"filesel.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"menustr.h"

#ifdef SUPPORT_NVL_IMAGES
BOOL nvl_check();
#endif

enum {
	DID_FOLDER	= DID_USER,
	DID_PARENT,
	DID_FLIST,
	DID_FILE,
	DID_FILTER
};

#if defined(OSLANG_SJIS) && !defined(RESOURCE_US)
static const OEMCHAR str_dirname[] =			// ファイルの場所
			"\203\164\203\100\203\103\203\213\202\314\217\352\217\212";
static const OEMCHAR str_filename[] =			// ファイル名
			"\203\164\203\100\203\103\203\213\226\274";
static const OEMCHAR str_filetype[] =			// ファイルの種類
			"\203\164\203\100\203\103\203\213\202\314\216\355\227\336";
static const OEMCHAR str_open[] =				// 開く
			"\212\112\202\255";
#elif defined(OSLANG_EUC) && !defined(RESOURCE_US)
static const OEMCHAR str_dirname[] =			// ファイルの場所
			"\245\325\245\241\245\244\245\353\244\316\276\354\275\352";
static const OEMCHAR str_filename[] =			// ファイル名
			"\245\325\245\241\245\244\245\353\314\276";
static const OEMCHAR str_filetype[] =			// ファイルの種類
			"\245\325\245\241\245\244\245\353\244\316\274\357\316\340";
static const OEMCHAR str_open[] =				// 開く
			"\263\253\244\257";
#elif defined(OSLANG_UTF8) && !defined(RESOURCE_US)
static const OEMCHAR str_dirname[] =			// ファイルの場所
			"\343\203\225\343\202\241\343\202\244\343\203\253\343\201\256" \
			"\345\240\264\346\211\200";
static const OEMCHAR str_filename[] =			// ファイル名
			"\343\203\225\343\202\241\343\202\244\343\203\253\345\220\215";
static const OEMCHAR str_filetype[] =			// ファイルの種類
			"\343\203\225\343\202\241\343\202\244\343\203\253\343\201\256" \
			"\347\250\256\351\241\236";
static const OEMCHAR str_open[] =				// 開く
			"\351\226\213\343\201\217";
#else
static const OEMCHAR str_dirname[] = OEMTEXT("Look in");
static const OEMCHAR str_filename[] = OEMTEXT("File name");
static const OEMCHAR str_filetype[] = OEMTEXT("Files of type");
static const OEMCHAR str_open[] = OEMTEXT("Open");
#endif

#if defined(NP2_SIZE_QVGA)
enum {
	DLGFS_WIDTH		= 294,
	DLGFS_HEIGHT	= 187
};
static const MENUPRM res_fs[] = {
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_dirname,							  6,   9,  68,  11},
			{DLGTYPE_EDIT,		DID_FOLDER,		0,
				NULL,									 74,   6, 192,  16},
			{DLGTYPE_BUTTON,	DID_PARENT,		MENU_TABSTOP,
				NULL,									272,   6,  16,  16},
			{DLGTYPE_LIST,		DID_FLIST,		MENU_TABSTOP,
				NULL,									  5,  28, 284, 115},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_filename,							  6, 150,  68,  11},
			{DLGTYPE_EDIT,		DID_FILE,		0,
				NULL,									 74, 147, 159,  16},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_filetype,							  6, 169,  68,  11},
			{DLGTYPE_EDIT,		DID_FILTER,		0,
				NULL,									 74, 166, 159,  16},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				str_open,								237, 147,  52,  15},
			{DLGTYPE_BUTTON,	DID_CANCEL,		MENU_TABSTOP,
				mstr_cancel,							237, 166,  52,  15}};
#else
enum {
	DLGFS_WIDTH		= 499,
	DLGFS_HEIGHT	= 227
};
static const MENUPRM res_fs[] = {
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_dirname,							 12,  10, 102,  13},
			{DLGTYPE_EDIT,		DID_FOLDER,		0,
				NULL,									114,   7, 219,  18},
			{DLGTYPE_BUTTON,	DID_PARENT,		MENU_TABSTOP,
				NULL,									348,   7,  18,  18},
			{DLGTYPE_LIST,		DID_FLIST,		MENU_TABSTOP,
				NULL,									  7,  30, 481, 128},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_filename,							 12, 168, 104,  13},
			{DLGTYPE_EDIT,		DID_FILE,		0,
				NULL,									116, 165, 268,  18},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_filetype,							 12, 192, 104,  13},
			{DLGTYPE_EDIT,		DID_FILTER,		0,
				NULL,									116, 189, 268,  18},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				str_open,								397, 165,  88,  23},
			{DLGTYPE_BUTTON,	DID_CANCEL,		MENU_TABSTOP,
				mstr_cancel,							397, 192,  88,  23}};
#endif

struct _flist;
typedef struct _flist	 _FLIST;
typedef struct _flist	 *FLIST;

struct _flist {
	FLIST	next;
	UINT	isdir;
	OEMCHAR	name[MAX_PATH];
};

typedef struct {
const OEMCHAR	*title;
const OEMCHAR	*filter;
const OEMCHAR	*ext;
#if defined(__LIBRETRO__)
int drv;
#endif
} FSELPRM;

typedef struct {
	BOOL		result;
	LISTARRAY	flist;
	FLIST		fbase;
const OEMCHAR	*filter;
const OEMCHAR	*ext;
	OEMCHAR		path[MAX_PATH];
#if defined(__LIBRETRO__)
int drv;
#endif
} FILESEL;

static	FILESEL		filesel;


// ----

static FLIST getflist(int pos) {

	FLIST	ret;

	ret = NULL;
	if (pos >= 0) {
		ret = filesel.fbase;
		while((pos > 0) && (ret)) {
			pos--;
			ret = ret->next;
		}
	}
	return(ret);
}

static BRESULT fappend(LISTARRAY flist, FLINFO *fli) {

	FLIST	fl;
	FLIST	*st;
	FLIST	cur;

	fl = (FLIST)listarray_append(flist, NULL);
	if (fl == NULL) {
		return(FAILURE);
	}
	fl->isdir = (fli->attr & 0x10)?1:0;
	file_cpyname(fl->name, fli->path, NELEMENTS(fl->name));
	st = &filesel.fbase;
	while(1) {
		cur = *st;
		if (cur == NULL) {
			break;
		}
		if (fl->isdir > cur->isdir) {
			break;
		}
		if ((fl->isdir == cur->isdir) &&
			(file_cmpname(fl->name, cur->name) < 0)) {
			break;
		}
		st = &cur->next;
	}
	fl->next = *st;
	*st = fl;
	return(SUCCESS);
}

static BOOL checkext(OEMCHAR *path, const OEMCHAR *ext) {

const OEMCHAR	*p;

	if (ext == NULL) {
		return(TRUE);
	}
	p = file_getext(path);
	while(*ext) {
		if (!file_cmpname(p, ext)) {
			return(TRUE);
		}
		ext += OEMSTRLEN(ext) + 1;
	}
	return(FALSE);
}

static void dlgsetlist(void) {

	LISTARRAY	flist;
	FLISTH		flh;
	FLINFO		fli;
	BOOL		append;
	FLIST		fl;
	ITEMEXPRM	prm;

	menudlg_itemreset(DID_FLIST);
	menudlg_settext(DID_FOLDER, file_getname(filesel.path));
	listarray_destroy(filesel.flist);
	flist = listarray_new(sizeof(_FLIST), 64);
	filesel.flist = flist;
	filesel.fbase = NULL;
	flh = file_list1st(filesel.path, &fli);
	if (flh != FLISTH_INVALID) {
		do {
			append = FALSE;
			if(!(strcmp(fli.path, ".") == 0 || strcmp(fli.path, "..") == 0)) {
				if (fli.attr & 0x10) {
					append = TRUE;
				}
				else if (!(fli.attr & 0x08)) {
					append = checkext(fli.path, filesel.ext);
				}
			}
			if (append) {
				if (fappend(flist, &fli) != SUCCESS) {
					break;
				}
			}
		} while(file_listnext(flh, &fli) == SUCCESS);
		file_listclose(flh);
	}
	prm.pos = 0;
	fl = filesel.fbase;
	while(fl) {
		menudlg_itemappend(DID_FLIST, NULL);
		prm.icon = (fl->isdir)?MICON_FOLDER:MICON_FILE;
		prm.str = fl->name;
		menudlg_itemsetex(DID_FLIST, &prm);
		fl = fl->next;
		prm.pos++;
	}
}

#if defined(_WINDOWS)
static void dlgsetdrvlist(void) {

	LISTARRAY	flist;
	FLISTH		flh;
	FLINFO		fli;
	BOOL		append;
	FLIST		fl;
	ITEMEXPRM	prm;
	UINT32		drives;
	UINT		d;

	menudlg_itemreset(DID_FLIST);
	menudlg_settext(DID_FOLDER, "Drives");
	listarray_destroy(filesel.flist);
	flist = listarray_new(sizeof(_FLIST), 64);
	filesel.flist = flist;
	filesel.fbase = NULL;
	drives = GetLogicalDrives();
	for(d = 0; d < 26; d++) {
		append = FALSE;
		if ((drives & (1 << d)) != 0) {
			append = TRUE;
		}
		if (append) {
			sprintf(fli.path, "%c:", 'A' + d);
			fli.attr = 0x10;
			if (fappend(flist, &fli) != SUCCESS) {
				break;
			}
		}
	}
	prm.pos = 0;
	fl = filesel.fbase;
	while(fl) {
		menudlg_itemappend(DID_FLIST, NULL);
		prm.icon = (fl->isdir)?MICON_FOLDER:MICON_FILE;
		prm.str = fl->name;
		menudlg_itemsetex(DID_FLIST, &prm);
		fl = fl->next;
		prm.pos++;
	}
}
#endif

static void dlginit(void) {

	menudlg_appends(res_fs, NELEMENTS(res_fs));
	menudlg_seticon(DID_PARENT, MICON_FOLDERPARENT);
	menudlg_settext(DID_FILE, file_getname(filesel.path));
	menudlg_settext(DID_FILTER, filesel.filter);
	file_cutname(filesel.path);
	file_cutseparator(filesel.path);
	dlgsetlist();
}

static BOOL dlgupdate(void) {

	FLIST	fl;

	fl = getflist(menudlg_getval(DID_FLIST));
	if (fl == NULL) {
		return(FALSE);
	}
	file_setseparator(filesel.path, NELEMENTS(filesel.path));
	file_catname(filesel.path, fl->name, NELEMENTS(filesel.path));
	if (fl->isdir) {
		dlgsetlist();
		menudlg_settext(DID_FILE, NULL);
		return(FALSE);
	}
	else {
		filesel.result = TRUE;
		return(TRUE);
	}
}

static void dlgflist(void) {

	FLIST	fl;

	fl = getflist(menudlg_getval(DID_FLIST));
	if ((fl != NULL) && (!fl->isdir)) {
		menudlg_settext(DID_FILE, fl->name);
	}
}

static int dlgcmd(int msg, MENUID id, long param) {

	switch(msg) {
		case DLGMSG_CREATE:
			dlginit();
			break;

		case DLGMSG_COMMAND:
			switch(id) {
				case DID_OK:
					if (dlgupdate()) {
#if defined(__LIBRETRO__)
						if(filesel.drv>=0xff)diskdrv_setsxsi(filesel.drv-0xff,filesel.path);
						else diskdrv_setfdd(filesel.drv, filesel.path, 0);
#endif
						menubase_close();
					}
					break;

				case DID_CANCEL:
					menubase_close();
					break;

				case DID_PARENT:
					file_cutname(filesel.path);
					file_cutseparator(filesel.path);
#if defined(_WINDOWS)
					if(filesel.path[0] == '\0')
						dlgsetdrvlist();
					else
						dlgsetlist();
#else
					dlgsetlist();
#endif
					menudlg_settext(DID_FILE, NULL);
					break;

				case DID_FLIST:
					if (param) {
						return(dlgcmd(DLGMSG_COMMAND, DID_OK, 0));
					}
					else {
						dlgflist();
					}
					break;
			}
			break;

		case DLGMSG_CLOSE:
			menubase_close();
			break;

		case DLGMSG_DESTROY:
			listarray_destroy(filesel.flist);
			filesel.flist = NULL;
			break;
	}
	(void)param;
	return(0);
}

#if defined(__LIBRETRO__)
static BOOL selectfile(const FSELPRM *prm, OEMCHAR *path, int size, 
														const OEMCHAR *def,int drv) {
#else
static BOOL selectfile(const FSELPRM *prm, OEMCHAR *path, int size, 
														const OEMCHAR *def) {
#endif

const OEMCHAR	*title;

	soundmng_stop();
	ZeroMemory(&filesel, sizeof(filesel));
	if ((def) && (def[0])) {
		file_cpyname(filesel.path, def, NELEMENTS(filesel.path));
	}
	else {
		file_cpyname(filesel.path, file_getcd(str_null),
													NELEMENTS(filesel.path));
		file_cutname(filesel.path);
	}
	title = NULL;
	if (prm) {
		title = prm->title;
		filesel.filter = prm->filter;
		filesel.ext = prm->ext;
#if defined(__LIBRETRO__)
		filesel.drv = drv;
#endif
	}
	menudlg_create(DLGFS_WIDTH, DLGFS_HEIGHT, title, dlgcmd);
#if !defined(__LIBRETRO__)
	menubase_modalproc();
#endif
	soundmng_play();
	if (filesel.result) {
		file_cpyname(path, filesel.path, size);
		return(TRUE);
	}
	else {
		return(FALSE);
	}
}


// ----

static const OEMCHAR diskfilter[] = OEMTEXT("All supported files");
static const OEMCHAR fddtitle[] = OEMTEXT("Select floppy image");
static const OEMCHAR fddext[] = OEMTEXT("d88\0") OEMTEXT("88d\0") OEMTEXT("d98\0") OEMTEXT("98d\0") OEMTEXT("fdi\0") OEMTEXT("xdf\0") OEMTEXT("hdm\0") OEMTEXT("dup\0") OEMTEXT("2hd\0") OEMTEXT("tfd\0") OEMTEXT("nfd\0") OEMTEXT("hd4\0") OEMTEXT("hd5\0") OEMTEXT("hd9\0") OEMTEXT("fdd\0") OEMTEXT("h01\0") OEMTEXT("hdb\0") OEMTEXT("ddb\0") OEMTEXT("dd6\0") OEMTEXT("dcp\0") OEMTEXT("dcu\0") OEMTEXT("flp\0") OEMTEXT("bin\0") OEMTEXT("fim\0") OEMTEXT("img\0") OEMTEXT("ima\0");
static const OEMCHAR hddtitle[] = OEMTEXT("Select HDD image");
static OEMCHAR sasiext[1000] = OEMTEXT("thd\0") OEMTEXT("nhd\0") OEMTEXT("hdi\0") OEMTEXT("vhd\0") OEMTEXT("slh\0") OEMTEXT("hdn\0");
#if defined(SUPPORT_IDEIO)
static const OEMCHAR cdtitle[] = OEMTEXT("Select CD-ROM image");
static const OEMCHAR cdext[] = OEMTEXT("iso\0") OEMTEXT("cue\0") OEMTEXT("ccd\0") OEMTEXT("cdm\0") OEMTEXT("mds\0") OEMTEXT("nrg\0");
#endif

static const FSELPRM fddprm = {fddtitle, diskfilter, fddext};
static const FSELPRM sasiprm = {hddtitle, diskfilter, sasiext};
#if defined(SUPPORT_IDEIO)
static const FSELPRM cdprm = {cdtitle, diskfilter, cdext};
#endif

#if defined(SUPPORT_SCSI)
static const OEMCHAR scsiext[] = OEMTEXT("hdd\0") OEMTEXT("hdn\0");
static const FSELPRM scsiprm = {hddtitle, diskfilter, scsiext};
#endif


void filesel_fdd(REG8 drv) {

	OEMCHAR	path[MAX_PATH];

	if (drv < 4) {
#if defined(__LIBRETRO__)
		if (selectfile(&fddprm, path, NELEMENTS(path), fdd_diskname(drv),drv)) {
#else
		if (selectfile(&fddprm, path, NELEMENTS(path), fdd_diskname(drv))) {
#endif
			diskdrv_setfdd(drv, path, 0);
		}
	}
}

void filesel_hdd(REG8 drv) {

	UINT		num;
	OEMCHAR		*p;
const FSELPRM	*prm;
	OEMCHAR		path[MAX_PATH];

	num = drv & 0x0f;
	p = NULL;
	prm = NULL;
	if (!(drv & 0x20)) {		// SASI/IDE
		if (num < 2) {
			p = np2cfg.sasihdd[num];
#ifdef SUPPORT_NVL_IMAGES
		if(nvl_check()) {
			strcat(sasiext, OEMTEXT("vmdk\0") OEMTEXT("dsk\0") OEMTEXT("vmdx\0") OEMTEXT("vdi\0") OEMTEXT("qcow\0") OEMTEXT("qcow2\0") OEMTEXT("hdd\0"));
		}
#endif	/* SUPPORT_NVL_IMAGES */

			prm = &sasiprm;
		}
#if defined(SUPPORT_IDEIO)
		if (num == 2) {
			p = np2cfg.sasihdd[num];
			prm = &cdprm;
		}
#endif
	}
#if defined(SUPPORT_SCSI)
	else {						// SCSI
		if (num < 4) {
			p = np2cfg.scsihdd[num];
			prm = &scsiprm;
		}
	}
#endif
#if defined(__LIBRETRO__)
	if ((prm) && (selectfile(prm, path, NELEMENTS(path), p,drv+0xff))) {
#else
	if ((prm) && (selectfile(prm, path, NELEMENTS(path), p))) {
#endif
		diskdrv_setsxsi(drv, path);
	}
}

