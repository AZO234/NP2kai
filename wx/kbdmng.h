/* === keyboard manager for wx port === */

#ifndef NP2_WX_KBDMNG_H
#define NP2_WX_KBDMNG_H

#include <keystat.h>

enum {
	KEY_KEY106 = 0,
	KEY_KEY101,
	KEY_TYPEMAX
};

enum {
	KEYBOARD_KC_NC = 0xff
};

#ifdef __cplusplus
extern "C" {
#endif

void kbdmng_initialize(void);
UINT8 kbdmng_getf12key(void);
void kbdmng_resetf12(void);

#ifdef __cplusplus
}
#endif

#endif /* NP2_WX_KBDMNG_H */
