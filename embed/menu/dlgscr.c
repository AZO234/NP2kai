#include	"compiler.h"
#include	"strres.h"
#include	"scrnmng.h"
#include	"sysmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"palettes.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"menustr.h"
#include	"sysmenu.res"
#include	"dlgscr.h"


enum {
	DID_TAB		= DID_USER,
	DID_LCD,
	DID_LCDX,
	DID_SKIPLINE,
	DID_SKIPLIGHT,
	DID_LIGHTSTR,
	DID_GDC7220,
	DID_GDC72020,
	DID_GRCGNON,
	DID_GRCG,
	DID_GRCG2,
	DID_EGC,
	DID_PC980124,
	DID_TRAMWAIT,
	DID_TRAMSTR,
	DID_VRAMWAIT,
	DID_VRAMSTR,
	DID_GRCGWAIT,
	DID_GRCGSTR,
	DID_REALPAL,
	DID_REALPALSTR
};

static const OEMCHAR str_video[] = OEMTEXT("Video");
static const OEMCHAR str_lcd[] = OEMTEXT("Liquid Crystal Display");
static const OEMCHAR str_lcdx[] = OEMTEXT("Reverse");
static const OEMCHAR str_skipline[] = OEMTEXT("Use skipline revisions");
static const OEMCHAR str_skiplght[] = OEMTEXT("Ratio");

static const OEMCHAR str_chip[] = OEMTEXT("Chip");
static const OEMCHAR str_gdc[] = OEMTEXT("GDC");
static const OEMCHAR str_gdc0[] = OEMTEXT("uPD7220");
static const OEMCHAR str_gdc1[] = OEMTEXT("uPD72020");
static const OEMCHAR str_grcg[] = OEMTEXT("Graphic Charger");
static const OEMCHAR str_grcg0[] = OEMTEXT("None");
static const OEMCHAR str_grcg1[] = OEMTEXT("GRCG");
static const OEMCHAR str_grcg2[] = OEMTEXT("GRCG+");
static const OEMCHAR str_grcg3[] = OEMTEXT("EGC");
static const OEMCHAR str_pc980124[] = OEMTEXT("Enable 16color (PC-9801-24)");

static const OEMCHAR str_timing[] = OEMTEXT("Timing");
static const OEMCHAR str_tram[] = OEMTEXT("T-RAM");
static const OEMCHAR str_vram[] = OEMTEXT("V-RAM");
static const OEMCHAR str_clock[] = OEMTEXT("clock");
static const OEMCHAR str_realpal[] = OEMTEXT("RealPalettes Adjust");


#if defined(SIZE_QVGA)
static const MENUPRM res_scr0[] = {
			{DLGTYPE_TABLIST,	DID_TAB,		0,
				NULL,									  5,   3, 270, 144},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				mstr_ok,								152, 150,  58,  15},
			{DLGTYPE_BUTTON,	DID_CANCEL,		MENU_TABSTOP,
				mstr_cancel,							214, 150,  58,  15}};

static const MENUPRM res_scr1[] = {
			{DLGTYPE_CHECK,		DID_LCD,		MENU_TABSTOP,
				str_lcd,								 18,  28, 240,  11},
			{DLGTYPE_CHECK,		DID_LCDX,		MENU_TABSTOP,
				str_lcdx,								 32,  43, 226,  11},
			{DLGTYPE_CHECK,		DID_SKIPLINE,	MENU_GRAY | MENU_TABSTOP,
				str_skipline,							 18,  62, 240,  11},
			{DLGTYPE_LTEXT,		DID_STATIC,		MENU_GRAY,
				str_skiplght,							 32,  77,  32,  11},
			{DLGTYPE_SLIDER,	DID_SKIPLIGHT,	MENU_GRAY |
													MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 255),				 66,  77, 112,  11},
			{DLGTYPE_RTEXT,		DID_LIGHTSTR,	MENU_GRAY,
				NULL,									186,  77,  20,  11}};

static const MENUPRM res_scr2[] = {
			{DLGTYPE_FRAME,		DID_STATIC,		0,
				str_gdc,								 18,  26, 153,  32},
			{DLGTYPE_RADIO,		DID_GDC7220,	MENU_TABSTOP,
				str_gdc0,								 32,  40,  64,  11},
			{DLGTYPE_RADIO,		DID_GDC72020,	0,
				str_gdc1,								 96,  40,  64,  11},
			{DLGTYPE_FRAME,		DID_STATIC,		0,
				str_grcg,								 18,  64, 196,  32},
			{DLGTYPE_RADIO,		DID_GRCGNON,	MENU_TABSTOP,
				str_grcg0,								 32,  78,  44,  11},
			{DLGTYPE_RADIO,		DID_GRCG,		0,
				str_grcg1,								 76,  78,  44,  11},
			{DLGTYPE_RADIO,		DID_GRCG2,		0,
				str_grcg2,								120,  78,  48,  11},
			{DLGTYPE_RADIO,		DID_EGC,		0,
				str_grcg3,								168,  78,  40,  11},
			{DLGTYPE_CHECK,		DID_PC980124,	MENU_TABSTOP,
				str_pc980124,							 28, 104, 224,  11}};

static const MENUPRM res_scr3[] = {
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_tram,								 18,  28,  42,  11},
			{DLGTYPE_SLIDER,	DID_TRAMWAIT,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 48),				 60,  28,  96,  11},
			{DLGTYPE_RTEXT,		DID_TRAMSTR,	0,
				NULL,									168,  28,  22,  11},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_clock,								192,  28,  32,  11},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_vram,								 18,  46,  42,  11},
			{DLGTYPE_SLIDER,	DID_VRAMWAIT,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 48),				 60,  46,  96,  11},
			{DLGTYPE_RTEXT,		DID_VRAMSTR,	0,
				NULL,									168,  46,  22,  11},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_clock,								192,  46,  32,  11},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_grcg1,								 18,  64,  42,  11},
			{DLGTYPE_SLIDER,	DID_GRCGWAIT,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 48),				 60,  64,  96,  11},
			{DLGTYPE_RTEXT,		DID_GRCGSTR,	0,
				NULL,									168,  64,  22,  11},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_clock,								192,  64,  32,  11},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_realpal,							 18,  84, 192,  11},
			{DLGTYPE_SLIDER,	DID_REALPAL,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 64),				 60,  98,  96,  11},
			{DLGTYPE_RTEXT,		DID_REALPALSTR,	0,
				NULL,									168,  98,  22,  11}};
#else
static const MENUPRM res_scr0[] = {
			{DLGTYPE_TABLIST,	DID_TAB,		0,
				NULL,									  7,   6, 379, 196},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				mstr_ok,								204, 208,  88,  21},
			{DLGTYPE_BUTTON,	DID_CANCEL,		MENU_TABSTOP,
				mstr_cancel,							299, 208,  88,  21}};

static const MENUPRM res_scr1[] = {
			{DLGTYPE_CHECK,		DID_LCD,		MENU_TABSTOP,
				str_lcd,								 25,  40, 358,  13},
			{DLGTYPE_CHECK,		DID_LCDX,		MENU_TABSTOP,
				str_lcdx,								 46,  64, 337,  13},
			{DLGTYPE_CHECK,		DID_SKIPLINE,	MENU_TABSTOP,
				str_skipline,							 25,  88, 358,  13},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_skiplght,							 46, 109,  40,  13},
			{DLGTYPE_SLIDER,	DID_SKIPLIGHT,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 255),				 92, 110, 160,  11},
			{DLGTYPE_RTEXT,		DID_LIGHTSTR,	0,
				NULL,									264, 109,  26,  13}};

static const MENUPRM res_scr2[] = {
			{DLGTYPE_FRAME,		DID_STATIC,		0,
				str_gdc,								 25,  39, 214,  42},
			{DLGTYPE_RADIO,		DID_GDC7220,	MENU_TABSTOP,
				str_gdc0,								 47,  58,  90,  13},
			{DLGTYPE_RADIO,		DID_GDC72020,	0,
				str_gdc1,								138,  58,  90,  13},
			{DLGTYPE_FRAME,		DID_STATIC,		0,
				str_grcg,								 25,  90, 273,  42},
			{DLGTYPE_RADIO,		DID_GRCGNON,	MENU_TABSTOP,
				str_grcg0,								 47, 109,  60,  13},
			{DLGTYPE_RADIO,		DID_GRCG,		0,
				str_grcg1,								107, 109,  60,  13},
			{DLGTYPE_RADIO,		DID_GRCG2,		0,
				str_grcg2,								167, 109,  69,  13},
			{DLGTYPE_RADIO,		DID_EGC,		0,
				str_grcg3,								236, 109,  60,  13},
			{DLGTYPE_CHECK,		DID_PC980124,	MENU_TABSTOP,
				str_pc980124,							 39, 148, 224,  13}};

static const MENUPRM res_scr3[] = {
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_tram,								 24,  41,  60,  13},
			{DLGTYPE_SLIDER,	DID_TRAMWAIT,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 48),				 86,  42, 130,  11},
			{DLGTYPE_RTEXT,		DID_TRAMSTR,	0,
				NULL,									234,  41,  28,  13},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_clock,								268,  41,  48,  13},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_vram,								 24,  65,  60,  13},
			{DLGTYPE_SLIDER,	DID_VRAMWAIT,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 48),				 86,  66, 130,  11},
			{DLGTYPE_RTEXT,		DID_VRAMSTR,	0,
				NULL,									234,  65,  28,  13},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_clock,								268,  65,  48,  13},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_grcg1,								 24,  89,  60,  13},
			{DLGTYPE_SLIDER,	DID_GRCGWAIT,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 48),				 86,  90, 130,  11},
			{DLGTYPE_RTEXT,		DID_GRCGSTR,	0,
				NULL,									234,  89,  28,  13},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_clock,								268,  89,  48,  13},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_realpal,							 24, 116, 224,  13},
			{DLGTYPE_SLIDER,	DID_REALPAL,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 64),				 86, 137, 130,  11},
			{DLGTYPE_RTEXT,		DID_REALPALSTR,	0,
				NULL,									234, 136,  28,  13}};
#endif

typedef struct {
const OEMCHAR	*tab;
const MENUPRM	*prm;
	UINT		count;
} TABLISTS;

static const MENUID gdcchip[4] = {DID_GRCGNON, DID_GRCG, DID_GRCG2, DID_EGC};

static const TABLISTS tablist[] = {
		{str_video,	res_scr1, NELEMENTS(res_scr1)},
		{str_chip,	res_scr2, NELEMENTS(res_scr2)},
		{str_timing,res_scr3, NELEMENTS(res_scr3)},
};

static void setpage(UINT page) {

	UINT	i;

	for (i=0; i<NELEMENTS(tablist); i++) {
		menudlg_disppagehidden((MENUID)(i + 1), (i != page));
	}
}

static void setintstr(MENUID id, int val) {

	OEMCHAR	buf[16];

	OEMSPRINTF(buf, str_d, val);
	menudlg_settext(id, buf);
}

static void dlginit(void) {

	UINT		i;
const TABLISTS	*tl;

	menudlg_appends(res_scr0, NELEMENTS(res_scr0));
	tl = tablist;
	for (i=0; i<NELEMENTS(tablist); i++, tl++) {
		menudlg_setpage((MENUID)(i + 1));
		menudlg_itemappend(DID_TAB, (OEMCHAR *)tl->tab);
		menudlg_appends(tl->prm, tl->count);
	}

	menudlg_setval(DID_LCD, np2cfg.LCD_MODE & 1);
	menudlg_setenable(DID_LCDX, np2cfg.LCD_MODE & 1);
	menudlg_setval(DID_LCDX, np2cfg.LCD_MODE & 2);
	menudlg_setval(DID_SKIPLINE, np2cfg.skipline);
	menudlg_setval(DID_SKIPLIGHT, np2cfg.skiplight);
	setintstr(DID_LIGHTSTR, np2cfg.skiplight);

	if (!np2cfg.uPD72020) {
		menudlg_setval(DID_GDC7220, TRUE);
	}
	else {
		menudlg_setval(DID_GDC72020, TRUE);
	}
	menudlg_setval(gdcchip[np2cfg.grcg & 3], TRUE);
	menudlg_setval(DID_PC980124, np2cfg.color16);

	menudlg_setval(DID_TRAMWAIT, np2cfg.wait[0]);
	setintstr(DID_TRAMSTR, np2cfg.wait[0]);
	menudlg_setval(DID_VRAMWAIT, np2cfg.wait[2]);
	setintstr(DID_VRAMSTR, np2cfg.wait[2]);
	menudlg_setval(DID_GRCGWAIT, np2cfg.wait[4]);
	setintstr(DID_GRCGSTR, np2cfg.wait[4]);
	menudlg_setval(DID_REALPAL, np2cfg.realpal);
	setintstr(DID_REALPALSTR, np2cfg.realpal - 32);

	menudlg_setval(DID_TAB, 0);
	setpage(0);
}

static void dlgupdate(void) {

	UINT	update;
	BOOL	renewal;
	UINT	val;
	UINT8	value[6];

	update = 0;
	renewal = FALSE;
	val = menudlg_getval(DID_SKIPLINE);
	if (np2cfg.skipline != (UINT8)val) {
		np2cfg.skipline = (UINT8)val;
		renewal = TRUE;
	}
	val = menudlg_getval(DID_SKIPLIGHT);
	if (val > 255) {
		val = 255;
	}
	if (np2cfg.skiplight != (UINT16)val) {
		np2cfg.skiplight = (UINT16)val;
		renewal = TRUE;
	}
	if (renewal) {
		pal_makeskiptable();
	}
	val = menudlg_getval(DID_LCD) + (menudlg_getval(DID_LCDX) << 1);
	if (np2cfg.LCD_MODE != (UINT8)val) {
		np2cfg.LCD_MODE = (UINT8)val;
		pal_makelcdpal();
		renewal = TRUE;
	}
	if (renewal) {
		scrndraw_redraw();
		update |= SYS_UPDATECFG;
	}

	val = menudlg_getval(DID_GDC72020);
	if (np2cfg.uPD72020 != (UINT8)val) {
		np2cfg.uPD72020 = (UINT8)val;
		update |= SYS_UPDATECFG;
		gdc_restorekacmode();
		gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
	}
	for (val=0; (val<3) && (!menudlg_getval(gdcchip[val])); val++) { }
	if (np2cfg.grcg != (UINT8)val) {
		np2cfg.grcg = (UINT8)val;
		update |= SYS_UPDATECFG;
		gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
	}
	val = menudlg_getval(DID_PC980124);
	if (np2cfg.color16 != (UINT8)val) {
		np2cfg.color16 = (UINT8)val;
		update |= SYS_UPDATECFG;
	}

	ZeroMemory(value, sizeof(value));
	value[0] = (UINT8)menudlg_getval(DID_TRAMWAIT);
	if (value[0]) {
		value[1] = 1;
	}
	value[2] = (UINT8)menudlg_getval(DID_VRAMWAIT);
	if (value[2]) {
		value[3] = 1;
	}
	value[4] = (UINT8)menudlg_getval(DID_GRCGWAIT);
	if (value[4]) {
		value[5] = 1;
	}
	for (val=0; val<6; val++) {
		if (np2cfg.wait[val] != value[val]) {
			np2cfg.wait[val] = value[val];
			update |= SYS_UPDATECFG;
		}
	}
	val = menudlg_getval(DID_REALPAL);
	if (np2cfg.realpal != (UINT8)val) {
		np2cfg.realpal = (UINT8)val;
		update |= SYS_UPDATECFG;
	}
	sysmng_update(update);
}

int dlgscr_cmd(int msg, MENUID id, long param) {

	switch(msg) {
		case DLGMSG_CREATE:
			dlginit();
			break;

		case DLGMSG_COMMAND:
			switch(id) {
				case DID_OK:
					dlgupdate();
					menubase_close();
					break;

				case DID_CANCEL:
					menubase_close();
					break;

				case DID_TAB:
					setpage(menudlg_getval(DID_TAB));
					break;

				case DID_LCD:
					menudlg_setenable(DID_LCDX, menudlg_getval(DID_LCD));
					break;

				case DID_SKIPLIGHT:
					setintstr(DID_LIGHTSTR, menudlg_getval(DID_SKIPLIGHT));
					break;

				case DID_TRAMWAIT:
					setintstr(DID_TRAMSTR, menudlg_getval(DID_TRAMWAIT));
					break;

				case DID_VRAMWAIT:
					setintstr(DID_VRAMSTR, menudlg_getval(DID_VRAMWAIT));
					break;

				case DID_GRCGWAIT:
					setintstr(DID_GRCGSTR, menudlg_getval(DID_GRCGWAIT));
					break;

				case DID_REALPAL:
					setintstr(DID_REALPALSTR,
											menudlg_getval(DID_REALPAL) - 32);
					break;
			}
			break;

		case DLGMSG_CLOSE:
			menubase_close();
			break;
	}
	(void)param;
	return(0);
}

