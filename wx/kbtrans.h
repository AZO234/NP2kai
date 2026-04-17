/* === keyboard translation for wx port === */

#ifndef NP2_WX_KBTRANS_H
#define NP2_WX_KBTRANS_H

#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

void wxkbd_initialize(void);
void wxkbd_reset(void);
/* keycode: wxWidgets key code (wxKeyCode / raw key code) */
void wxkbd_keydown(int keycode, int rawcode);
void wxkbd_keyup(int keycode, int rawcode);

#ifdef __cplusplus
}
#endif

#endif /* NP2_WX_KBTRANS_H */
