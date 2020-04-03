/* === NP2 file dialog for Windows === (c) 2020 AZO */

#ifndef _WINFILEDLG_H_
#define _WINFILEDLG_H_

#include "compiler.h"

#define WinFileDialogW(h, m, p, n, t, f) WinFileDialogW_MM(h, m, p, n, t, f)

typedef enum {
  WINFILEDIALOGW_MODE_GET = 0,
  WINFILEDIALOGW_MODE_SET1,
  WINFILEDIALOGW_MODE_SET2,
} WinFileDialogW_Mode_t;

#ifdef __cplusplus
extern "C" {
#endif

// [I] mode : Get(Exist file) or Set1(Overwrite file) or New(New or Overwrite)
// [O] path : Full path of file
// [O] name : Filename
// [I] title : Title of dialog
// [I] name : filename

BOOL WinFileDialogW_MM(
  HWND hwnd,
  const WinFileDialogW_Mode_t mode,
  char* path,
  char* name,
  const char* title,
  const char* filter
);
BOOL WinFileDialogW_MW(
  HWND hwnd,
  const WinFileDialogW_Mode_t mode,
  wchar_t* path,
  wchar_t* name,
  const char* title,
  const char* filter
);
BOOL WinFileDialogW_WM(
  HWND hwnd,
  const WinFileDialogW_Mode_t mode,
  char* path,
  char* name,
  const wchar_t* title,
  const wchar_t* filter
);
BOOL WinFileDialogW_WW(
  HWND hwnd,
  const WinFileDialogW_Mode_t mode,
  wchar_t* path,
  wchar_t* name,
  const wchar_t* title,
  const wchar_t* filter
);

#ifdef __cplusplus
}
#endif

#endif  // _WINFILEDLG_H_

