#include "compiler.h"

#if defined(SUPPORT_KEYDISP)

#include "np2.h"
#include "dosio.h"
#include "ini.h"

#include "kdispwin.h"

KDISPCFG kdispcfg;


BRESULT
kdispwin_initialize(void)
{

	keydisp_initialize();

	return SUCCESS;
}


// ---- ini

static const char ini_title[] = "NP2 keydisp";

static INITBL iniitem[] = {
	{"WindposX", INITYPE_SINT32,	&kdispcfg.posx,		0},
	{"WindposY", INITYPE_SINT32,	&kdispcfg.posy,		0},
	{"keydmode", INITYPE_UINT8,	&kdispcfg.mode,		0},
	{"windtype", INITYPE_BOOL,	&kdispcfg.type,		0},
};
#define	INIITEMS	(sizeof(iniitem) / sizeof(iniitem[0]))

void
kdispwin_readini(void)
{
	char path[MAX_PATH];

	memset(&kdispcfg, 0, sizeof(kdispcfg));
	kdispcfg.posx = 0;
	kdispcfg.posy = 0;
	file_cpyname(path, modulefile, sizeof(path));
	ini_read(path, ini_title, iniitem, INIITEMS);
}

void
kdispwin_writeini(void)
{
	char path[MAX_PATH];

	file_cpyname(path, modulefile, sizeof(path));
	ini_write(path, ini_title, iniitem, INIITEMS, FALSE);
}

#endif
