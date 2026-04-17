/* === compiler header for wxWidgets+SDL3 port === (c) 2024 AZO */

/* This file lives at wx/compiler.h in the NP2kai source tree.
 * wxWidgets' own internal headers also include "wx/compiler.h" to get
 * wxCHECK_GCC_VERSION etc.  When the NP2kai root is in the -I path,
 * wxWidgets finds THIS file instead of its own.  We use #include_next to
 * pull in the real wxWidgets wx/compiler.h first (before our port guard),
 * so that wxCHECK_GCC_VERSION is defined when wx/defs.h needs it. */
#ifndef _WX_COMPILER_H_
#include_next "wx/compiler.h"
#endif

#ifndef NP2_WX_COMPILER_H
#define NP2_WX_COMPILER_H

/* wx port marker - must be before compiler_base.h */
#define NP2_WX

/* Use SDL3 for audio / threading infrastructure */
#define USE_SDL 3

#include "compiler_base.h"

#include <pthread.h>

/* wxWidgets */
#ifdef __cplusplus
#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/notebook.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/timer.h>
#include <wx/bitmap.h>
#include <wx/rawbmp.h>
#endif /* __cplusplus */

/* Message box via wxWidgets */
#ifdef __cplusplus
#define msgbox(title, msg) \
    wxMessageBox(wxString::FromUTF8(msg), wxString::FromUTF8(title), wxOK | wxICON_INFORMATION)
#else
#define msgbox(title, msg)
#endif

#define __ASSERT(s)

#define RESOURCE_US

#define NP2_SIZE_VGA

/* Tick via SDL */
#define GETTICK() SDL_GetTicks()

/* Feature flags */
#define SUPPORT_KEYDISP  1
#define SUPPORT_SOFTKBD  0
#define SUPPORT_SCREENSIZE
#define USE_SDL_JOYSTICK
#define SUPPORT_WAVEREC

/* path for config */
#define NP2_WX_CONFIGDIR ".config"
#if defined(CPUCORE_IA32)
#define NP2_WX_APPNAME  "wxnp21kai"
#else
#define NP2_WX_APPNAME  "wxnp2kai"
#endif

#include <common/milstr.h>
#include <trace.h>

#endif  /* NP2_WX_COMPILER_H */
