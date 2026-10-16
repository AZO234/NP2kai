/* === UI hooks called from the toolkit-independent part of the wx port ===
 * Implemented by each frontend that reuses that code (wx: np2frame.cpp).
 * Kept free of toolkit headers so sysmng.cpp and scrnmng.cpp stay neutral.
 */

#ifndef NP2_WX_NP2UI_H
#define NP2_WX_NP2UI_H

#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

void np2frame_requestRedraw(void);
void np2frame_updateCaption(UINT8 flag);
void np2frame_setTitle(const char *title);

#ifdef __cplusplus
}
#endif

#endif /* NP2_WX_NP2UI_H */
