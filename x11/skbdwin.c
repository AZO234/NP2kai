#include "compiler.h"

#if defined(SUPPORT_SOFTKBD)

#include "np2.h"
#include "dosio.h"
#include "ini.h"

#include "skbdwin.h"

SKBDCFG skbdcfg;


BRESULT
skbdwin_initialize(void)
{

	softkbd_initialize();

	return SUCCESS;
}

void
skbdwin_deinitialize(void)
{

	softkbd_deinitialize();
}


/* ---- ini */

static const char ini_title[] = "NP2 software keyboard";

static INITBL iniitem[] = {
	{"WindposX", INITYPE_SINT32,	&skbdcfg.posx,		0},
	{"WindposY", INITYPE_SINT32,	&skbdcfg.posy,		0},
};
#define	INIITEMS	(sizeof(iniitem) / sizeof(iniitem[0]))

void
skbdwin_readini(void)
{
	char path[MAX_PATH];

	memset(&skbdcfg, 0, sizeof(skbdcfg));
	file_cpyname(path, modulefile, sizeof(path));
	ini_read(path, ini_title, iniitem, INIITEMS);
}

void
skbdwin_writeini(void)
{
	char path[MAX_PATH];

	file_cpyname(path, modulefile, sizeof(path));
	ini_write(path, ini_title, iniitem, INIITEMS, FALSE);
}
#endif
