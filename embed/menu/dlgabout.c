#include	<compiler.h>
#include	<common/strres.h>
#include	<np2ver.h>
#include	<pccore.h>
#include	<embed/vramhdl.h>
#include	<embed/menubase/menubase.h>
#include	<embed/menu/menustr.h>
#include	"sysmenu.res"
#include	<embed/menu/dlgabout.h>


enum {
	DID_ICON	= DID_USER,
	DID_VER,
	DID_VER2,
	DID_VER3
};

#if defined(NP2_SIZE_QVGA)
static const MENUPRM res_about[] = {
			{DLGTYPE_ICON,		DID_ICON,		0,
				(void *)MICON_NP2,						  7,   7,  24,  24},
			{DLGTYPE_LTEXT,		DID_VER,		0,
				NULL,									 40,  7, 128,  11},
			{DLGTYPE_LTEXT,		DID_VER2,		0,
				NULL,									 40,  14, 128,  11},
			{DLGTYPE_LTEXT,		DID_VER3,		0,
				NULL,									 40,  21, 128,  11},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				mstr_ok,								176,   8,  48,  15}};
#else
static const MENUPRM res_about[] = {
			{DLGTYPE_ICON,		DID_ICON,		0,
				(void *)MICON_NP2,						 14,  12,  32,  32},
			{DLGTYPE_LTEXT,		DID_VER,		0,
				NULL,									 64,  12, 180,  13},
			{DLGTYPE_LTEXT,		DID_VER2,		0,
				NULL,									 64,  24, 180,  13},
			{DLGTYPE_LTEXT,		DID_VER3,		0,
				NULL,									 64,  36, 180,  13},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				mstr_ok,								258,  12,  70,  21}};
#endif


// ----

static void dlginit(void) {

	OEMCHAR	work[128];
	OEMCHAR	work2[128];
	OEMCHAR	work3[128];

	menudlg_appends(res_about, NELEMENTS(res_about));
	milstr_ncpy(work, str_np2, NELEMENTS(work));
	milstr_ncat(work, str_space, NELEMENTS(work));
	milstr_ncat(work, NP2KAI_GIT_TAG, NELEMENTS(work));
	milstr_ncpy(work2, NP2KAI_GIT_HASH, NELEMENTS(work2));
	milstr_ncpy(work3, NP2KAI_BUILDER, NELEMENTS(work2));
	menudlg_settext(DID_VER, work);
	menudlg_settext(DID_VER2, work2);
	menudlg_settext(DID_VER3, work3);
}

int dlgabout_cmd(int msg, MENUID id, long param) {

	switch(msg) {
		case DLGMSG_CREATE:
			dlginit();
			break;

		case DLGMSG_COMMAND:
			switch(id) {
				case DID_OK:
					menubase_close();
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

