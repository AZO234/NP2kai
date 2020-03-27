#include	"compiler.h"

#if defined(SUPPORT_WAB) && defined(SUPPORT_CL_GD5430)

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
#include	"dlgwab.h"
#include	"wab.h"
#include	"cirrus_vga_extern.h"

enum {
	DID_TAB		= DID_USER,
	DID_WAB_ASW,
	DID_WAB_MT,
	DID_CLGD_EN,
	DID_CLGD_TYPE,
	DID_CLGD_TYPESTR,
	DID_CLGD_FC
};

static const OEMCHAR str_wab[] = OEMTEXT("General");
static const OEMCHAR str_wab_asw[] = OEMTEXT("Use Analog Switch IC (No relay sound)");
static const OEMCHAR str_wab_mt[] = OEMTEXT("Multi Thread Mode");

static const OEMCHAR str_clgd[] = OEMTEXT("CL-GD54xx");
static const OEMCHAR str_clgd_en[] = OEMTEXT("Enabled (restart)");
static const OEMCHAR str_clgd_type[] = OEMTEXT("Type");
static const OEMCHAR str_clgd_fc[] = OEMTEXT("Use Fake Hardware Cursor");

static const OEMCHAR *str_cl_gd54xx_type[] = {
	OEMTEXT("PC-9821Bp,Bs,Be,Bf built-in"),
	OEMTEXT("PC-9821Xe built-in"),
	OEMTEXT("PC-9821Cb built-in"),
	OEMTEXT("PC-9821Cf built-in"),
	OEMTEXT("PC-9821Xe10,Xa7e,Xb10 built-in"),
	OEMTEXT("PC-9821Cb2 built-in"),
	OEMTEXT("PC-9821Cx2 built-in"),
#ifdef SUPPORT_PCI
	OEMTEXT("PC-9821 PCI CL-GD5446 built-in"),
#endif
	OEMTEXT("MELCO WAB-S"),
	OEMTEXT("MELCO WSN-A2F"),
	OEMTEXT("MELCO WSN-A4F"),
	OEMTEXT("I-O DATA GA-98NBI/C"),
	OEMTEXT("I-O DATA GA-98NBII"),
	OEMTEXT("I-O DATA GA-98NBIV"),
	OEMTEXT("PC-9801-96(PC-9801B3-E02)"),
#ifdef SUPPORT_PCI
	OEMTEXT("Auto Select(Xe10, GA-98NBI/C), PCI"),
	OEMTEXT("Auto Select(Xe10, GA-98NBII), PCI"),
	OEMTEXT("Auto Select(Xe10, GA-98NBIV), PCI"),
	OEMTEXT("Auto Select(Xe10, WAB-S), PCI"),
	OEMTEXT("Auto Select(Xe10, WSN-A2F), PCI"),
	OEMTEXT("Auto Select(Xe10, WSN-A4F), PCI"),
#endif
	OEMTEXT("Auto Select(Xe10, WAB-S)"),
	OEMTEXT("Auto Select(Xe10, WSN-A2F)"),
	OEMTEXT("Auto Select(Xe10, WSN-A4F)"),
};

static const MENUPRM res_wab0[] = {
			{DLGTYPE_TABLIST,	DID_TAB,		0,
				NULL,									  7,   6, 379, 196},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				mstr_ok,								204, 208,  88,  21},
			{DLGTYPE_BUTTON,	DID_CANCEL,		MENU_TABSTOP,
				mstr_cancel,							299, 208,  88,  21}};

static const MENUPRM res_wab1[] = {
			{DLGTYPE_CHECK,		DID_WAB_ASW,		MENU_TABSTOP,
				str_wab_asw,								 25,  40, 358,  13},
			{DLGTYPE_CHECK,		DID_WAB_MT,		MENU_TABSTOP,
				str_wab_mt,								 25,  64, 358,  13}};

static const MENUPRM res_wab2[] = {
			{DLGTYPE_CHECK,		DID_CLGD_EN,		MENU_TABSTOP,
				str_clgd_en,								 25,  40, 358,  13},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_clgd_type,								 25,  64, 358,  13},
			{DLGTYPE_SLIDER,	DID_CLGD_TYPE,	MSS_BOTH | MENU_TABSTOP,
				(void *)SLIDERPOS(0, 14),						 86,  66, 130,  11},
			{DLGTYPE_RTEXT,		DID_CLGD_TYPESTR,	0,
				NULL,									 86,  88, 250,  13},
			{DLGTYPE_CHECK,		DID_CLGD_FC,		MENU_TABSTOP,
				str_clgd_fc,								 25, 112, 358,  13}};

typedef struct {
	const OEMCHAR	*tab;
	const MENUPRM	*prm;
	UINT		count;
} TABLISTS;

static const TABLISTS tablist[] = {
		{str_wab,	res_wab1, NELEMENTS(res_wab1)},
		{str_clgd,	res_wab2, NELEMENTS(res_wab2)},
};

static void setpage(UINT page) {

	UINT	i;

	for (i=0; i<NELEMENTS(tablist); i++) {
		menudlg_disppagehidden((MENUID)(i + 1), (i != page));
	}
}

static void setintstr(MENUID id, int val) {

	OEMCHAR	buf[64];

	OEMSNPRINTF(buf, sizeof(buf), str_d, val);
	menudlg_settext(id, buf);
}

static void dlginit(void) {

	UINT		i;
const TABLISTS	*tl;
	UINT type;

	switch (np2cfg.gd5430type) {
	case CIRRUS_98ID_Be:
		type = 0;
		break;
	case CIRRUS_98ID_Xe:
		type = 1;
		break;
	case CIRRUS_98ID_Cb:
		type = 2;
		break;
	case CIRRUS_98ID_Cf:
		type = 3;
		break;
	case CIRRUS_98ID_Xe10:
		type = 4;
		break;
	case CIRRUS_98ID_Cb2:
		type = 5;
		break;
	case CIRRUS_98ID_Cx2:
		type = 6;
		break;
#ifdef SUPPORT_PCI
	case CIRRUS_98ID_PCI:
		type = 7;
		break;
	case CIRRUS_98ID_WAB:
		type = 8;
		break;
	case CIRRUS_98ID_WSN_A2F:
		type = 9;
		break;
	case CIRRUS_98ID_WSN:
		type = 10;
		break;
	case CIRRUS_98ID_GA98NBIC:
		type = 11;
		break;
	case CIRRUS_98ID_GA98NBII:
		type = 12;
		break;
	case CIRRUS_98ID_GA98NBIV:
		type = 13;
		break;
	case CIRRUS_98ID_96:
		type = 14;
		break;
	case CIRRUS_98ID_AUTO_XE_G1_PCI:
		type = 15;
		break;
	case CIRRUS_98ID_AUTO_XE_G2_PCI:
		type = 16;
		break;
	case CIRRUS_98ID_AUTO_XE_G4_PCI:
		type = 17;
		break;
	case CIRRUS_98ID_AUTO_XE_WA_PCI:
		type = 18;
		break;
	case CIRRUS_98ID_AUTO_XE_WS_PCI:
		type = 19;
		break;
	case CIRRUS_98ID_AUTO_XE_W4_PCI:
		type = 20;
		break;
	case CIRRUS_98ID_AUTO_XE10_WABS:
		type = 21;
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN2:
		type = 22;
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN4:
		type = 23;
		break;
#else
	case CIRRUS_98ID_WAB:
		type = 7;
		break;
	case CIRRUS_98ID_WSN_A2F:
		type = 8;
		break;
	case CIRRUS_98ID_WSN:
		type = 9;
		break;
	case CIRRUS_98ID_GA98NBIC:
		type = 10;
		break;
	case CIRRUS_98ID_GA98NBII:
		type = 11;
		break;
	case CIRRUS_98ID_GA98NBIV:
		type = 12;
		break;
	case CIRRUS_98ID_96:
		type = 13;
		break;
	case CIRRUS_98ID_AUTO_XE10_WABS:
		type = 14;
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN2:
		type = 15;
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN4:
		type = 16;
		break;
#endif
	}

	menudlg_appends(res_wab0, NELEMENTS(res_wab0));
	tl = tablist;
	for (i=0; i<NELEMENTS(tablist); i++, tl++) {
		menudlg_setpage((MENUID)(i + 1));
		menudlg_itemappend(DID_TAB, (OEMCHAR *)tl->tab);
		menudlg_appends(tl->prm, tl->count);
	}

	menudlg_setval(DID_WAB_ASW, np2cfg.wabasw & 1);
	menudlg_setval(DID_WAB_MT, np2wabcfg.multithread & 1);
menudlg_setenable(DID_WAB_MT, 0);

	menudlg_setval(DID_CLGD_EN, np2cfg.usegd5430 & 1);
	menudlg_setval(DID_CLGD_TYPE, type);
	menudlg_settext(DID_CLGD_TYPESTR, str_cl_gd54xx_type[type]);
	menudlg_setval(DID_CLGD_FC, np2cfg.gd5430fakecur & 1);

	menudlg_setval(DID_TAB, 0);
	setpage(0);
}

static void dlgupdate(void) {

	UINT	update;
	BOOL	renewal;
	UINT	val;
	UINT	type;

	update = 0;
	renewal = FALSE;
	val = menudlg_getval(DID_WAB_ASW);
	if (np2cfg.wabasw != (UINT8)val) {
		np2cfg.wabasw = (UINT8)val;
		renewal = TRUE;
	}
	val = menudlg_getval(DID_WAB_MT);
	if (np2wabcfg.multithread != (UINT8)val) {
		np2wabcfg.multithread = (UINT8)val;
		renewal = TRUE;
	}
	val = menudlg_getval(DID_CLGD_EN);
	if (np2cfg.usegd5430 != (UINT8)val) {
		np2cfg.usegd5430 = (UINT8)val;
		renewal = TRUE;
	}
	val = menudlg_getval(DID_CLGD_TYPE);
	if (val > 14) {
		val = 14;
	}
	switch(val) {
	case 0:
		type = CIRRUS_98ID_Be;
		break;
	case 1:
		type = CIRRUS_98ID_Xe;
		break;
	case 2:
		type = CIRRUS_98ID_Cb;
		break;
	case 3:
		type = CIRRUS_98ID_Cf;
		break;
	case 4:
		type = CIRRUS_98ID_Xe10;
		break;
	case 5:
		type = CIRRUS_98ID_Cb2;
		break;
	case 6:
		type = CIRRUS_98ID_Cx2;
		break;
#ifdef SUPPORT_PCI
	case 7:
		type = CIRRUS_98ID_PCI;
		break;
	case 8:
		type = CIRRUS_98ID_WAB;
		break;
	case 9:
		type = CIRRUS_98ID_WSN_A2F;
		break;
	case 10:
		type = CIRRUS_98ID_WSN;
		break;
	case 11:
		type = CIRRUS_98ID_GA98NBIC;
		break;
	case 12:
		type = CIRRUS_98ID_GA98NBII;
		break;
	case 13:
		type = CIRRUS_98ID_GA98NBIV;
		break;
	case 14:
		type = CIRRUS_98ID_96;
		break;
	case 15:
		type = CIRRUS_98ID_AUTO_XE_G1_PCI;
		break;
	case 16:
		type = CIRRUS_98ID_AUTO_XE_G2_PCI;
		break;
	case 17:
		type = CIRRUS_98ID_AUTO_XE_G4_PCI;
		break;
	case 18:
		type = CIRRUS_98ID_AUTO_XE_WA_PCI;
		break;
	case 19:
		type = CIRRUS_98ID_AUTO_XE_WS_PCI;
		break;
	case 20:
		type = CIRRUS_98ID_AUTO_XE_W4_PCI;
		break;
	case 21:
		type = CIRRUS_98ID_AUTO_XE10_WABS;
		break;
	case 22:
		type = CIRRUS_98ID_AUTO_XE10_WSN2;
		break;
	case 23:
		type = CIRRUS_98ID_AUTO_XE10_WSN4;
		break;
#else
	case 7:
		type = CIRRUS_98ID_WAB;
		break;
	case 8:
		type = CIRRUS_98ID_WSN_A2F;
		break;
	case 9:
		type = CIRRUS_98ID_WSN;
		break;
	case 10:
		type = CIRRUS_98ID_GA98NBIC;
		break;
	case 11:
		type = CIRRUS_98ID_GA98NBII;
		break;
	case 12:
		type = CIRRUS_98ID_GA98NBIV;
		break;
	case 13:
		type = CIRRUS_98ID_96;
		break;
	case 14:
		type = CIRRUS_98ID_AUTO_XE10_WABS;
		break;
	case 15:
		type = CIRRUS_98ID_AUTO_XE10_WSN2;
		break;
	case 16:
		type = CIRRUS_98ID_AUTO_XE10_WSN4;
		break;
#endif
	}
	if (np2cfg.gd5430type != (UINT16)type) {
		np2cfg.gd5430type = (UINT16)type;
		renewal = TRUE;
	}
	val = menudlg_getval(DID_CLGD_FC);
	if (np2cfg.gd5430fakecur != (UINT8)val) {
		np2cfg.gd5430fakecur = (UINT8)val;
		renewal = TRUE;
	}
	if (renewal) {
		scrndraw_redraw();
		update |= SYS_UPDATECFG;
	}

	sysmng_update(update);
}

int dlgwab_cmd(int msg, MENUID id, long param) {

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

				case DID_CLGD_TYPE:
					menudlg_settext(DID_CLGD_TYPESTR, str_cl_gd54xx_type[menudlg_getval(DID_CLGD_TYPE)]);
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

#endif
