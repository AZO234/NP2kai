/* === input management for wx port === */

#include <compiler.h>
#include "inputmng.h"
#include <string.h>

#define INPUTMNG_MAX 256
static UINT keybinds[INPUTMNG_MAX];

void inputmng_init(void)
{
	memset(keybinds, 0, sizeof(keybinds));
}

void inputmng_keybind(short key, UINT bit)
{
	if ((unsigned short)key < INPUTMNG_MAX)
		keybinds[(unsigned short)key] = bit;
}

UINT inputmng_getkey(short key)
{
	if ((unsigned short)key < INPUTMNG_MAX)
		return keybinds[(unsigned short)key];
	return 0;
}
