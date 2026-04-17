/* === keyboard manager for wx port === */

#include <compiler.h>
#include "kbdmng.h"
#include "np2.h"
#include <keystat.h>
#include <pccore.h>
#include <io/iocore.h>

static const UINT8 kbdmng_f12keys[] = {
	0x61,	/* Copy */
	0x60,	/* Stop */
	0x4f,	/* tenkey [,] */
	0x4d,	/* tenkey [=] */
	0x76,	/* User1 */
	0x77,	/* User2 */
	0x3f,	/* Help */
};

void kbdmng_initialize(void)
{
	/* PC-98 keyboard state reset */
	keystat_initialize();
}

UINT8 kbdmng_getf12key(void)
{
	int key;

	key = np2oscfg.F12KEY - 1; /* 0 is Mouse mode */
	if (key >= 0 && key < (int)NELEMENTS(kbdmng_f12keys))
		return kbdmng_f12keys[key];
	return KEYBOARD_KC_NC;
}

void kbdmng_resetf12(void)
{
	int i;

	for (i = 0; i < (int)NELEMENTS(kbdmng_f12keys); i++) {
		keystat_forcerelease(kbdmng_f12keys[i]);
	}
}
