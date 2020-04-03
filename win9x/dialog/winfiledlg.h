/* === NP2 file dialog for Windows === (c) 2020 AZO */

#ifndef _WINFILEDLG_H_
#define _WINFILEDLG_H_

#include "compiler.h"

#define WinFileDialogW(h, o, m, p, n, d, t, f, i) WinFileDialogW_MM(h, o, m, p, n, d, t, f, i)

typedef enum {
  WINFILEDIALOGW_MODE_GET1 = 0,
  WINFILEDIALOGW_MODE_GET2,
  WINFILEDIALOGW_MODE_SET
} WinFileDialogW_Mode_t;

#ifdef __cplusplus
extern "C" {
#endif

// [I] mode : Get(Exist file) or Set1(Overwrite file) or New(New or Overwrite)
// [O] ofnw : OPENFILENAMEW
// [O] path : Full path of file
// [O] name : Filename
// [I] title : Title of dialog
// [I] name : Ext filter
// [I] name : Filter index

BOOL WinFileDialogW_MM(
  HWND hwnd,
  OPENFILENAMEW* pofnw,
  const WinFileDialogW_Mode_t mode,
  char* path,
  char* name,
  const char* defext,
  const char* title,
  const char* filter,
  DWORD nFilterIndex
);
BOOL WinFileDialogW_WW(
  HWND hwnd,
  OPENFILENAMEW* pofnw,
  const WinFileDialogW_Mode_t mode,
  wchar_t* path,
  wchar_t* name,
  const wchar_t* defext,
  const wchar_t* title,
  const wchar_t* filter,
  DWORD nFilterIndex
);

#ifdef __cplusplus
}
#endif

#endif  // _WINFILEDLG_H_

