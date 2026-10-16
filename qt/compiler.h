/* === compiler header for the Qt6+SDL3 port ===
 *
 * The Qt port reuses the toolkit-independent half of the wx port straight
 * from wx/ (np2.cpp, ini.cpp, scrnmng.cpp, soundmng.cpp ...). qt/ comes before
 * wx/ in the include path, so this header replaces wx/compiler.h for every
 * translation unit, while all other headers still come from wx/. Never add a
 * header to qt/ that shares its name with one in wx/.
 */

#ifndef NP2_QT_COMPILER_H
#define NP2_QT_COMPILER_H

/* Port marker - must be before compiler_base.h */
#define NP2_QT

/* Use SDL3 for audio / threading infrastructure */
#define USE_SDL 3

#include "compiler_base.h"

#include <pthread.h>

/* Message box: safe to call from the emulator thread, never blocks it */
#ifdef __cplusplus
extern "C" {
#endif
void np2qt_msgbox(const char *title, const char *msg);
#ifdef __cplusplus
}
#endif
#define msgbox(title, msg)  np2qt_msgbox((title), (msg))

#define __ASSERT(s)

#define RESOURCE_US

#define NP2_SIZE_VGA

/* Tick via SDL */
#define GETTICK() SDL_GetTicks()

/* Feature flags (same as the wx port, whose sources are shared) */
#define SUPPORT_KEYDISP  1
#define SUPPORT_SOFTKBD  0
#define SUPPORT_SCREENSIZE
#define USE_SDL_JOYSTICK
#define SUPPORT_WAVEREC

/* Config directory and file names; the wx sources build them from these */
#define NP2_WX_CONFIGDIR ".config"
#if defined(CPUCORE_IA32)
#define NP2_WX_APPNAME  "qtnp21kai"
#define NP2_WX_TITLE    "NP21kai (Qt)"
#else
#define NP2_WX_APPNAME  "qtnp2kai"
#define NP2_WX_TITLE    "NP2kai (Qt)"
#endif

#include <common/milstr.h>
#include <trace.h>

#endif  /* NP2_QT_COMPILER_H */
