/* === NP2 file dialog for Windows === (c) 2020 AZO */

#include "winfiledlg.h"
#include "codecnv/codecnv.h"

#define WINFILEDLG_GET1_FLAG (OFN_FILEMUSTEXIST | OFN_HIDEREADONLY)
#define WINFILEDLG_GET2_FLAG (OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHAREAWARE)
#define WINFILEDLG_SET_FLAG  (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY)

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
) {
  BOOL res;
  wchar_t wpath[MAX_PATH];
  wchar_t wname[MAX_PATH];
  wchar_t wdefext[MAX_PATH];
  wchar_t wtitle[MAX_PATH];
  wchar_t wfilter[MAX_PATH];

  codecnv_utf8toucs2(wpath, MAX_PATH, path, -1);
  codecnv_utf8toucs2(wname, MAX_PATH, name, -1);
  codecnv_utf8toucs2(wdefext, MAX_PATH, defext, -1);
  codecnv_utf8toucs2(wtitle, MAX_PATH, title, -1);
  codecnv_utf8toucs2(wfilter, MAX_PATH, filter, -1);

  res = WinFileDialogW_WW(hwnd, pofnw, mode, wpath, wname, wdefext, wtitle, wfilter, nFilterIndex);

  codecnv_ucs2toutf8(path, MAX_PATH, wpath, -1);
  codecnv_ucs2toutf8(name, MAX_PATH, wname, -1);

  return res;
}


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
) {
  BOOL res = TRUE;

  memset(pofnw, 0, sizeof(OPENFILENAMEW));
  pofnw->lStructSize = sizeof(OPENFILENAMEW);
  pofnw->hwndOwner = hwnd;
  pofnw->lpstrFile = path;
  pofnw->nMaxFile = MAX_PATH;
  pofnw->lpstrFileTitle = name;
  pofnw->lpstrDefExt = defext;
  pofnw->nMaxFileTitle = MAX_PATH;
  pofnw->lpstrFilter = filter;
  pofnw->lpstrTitle = title;
  pofnw->nFilterIndex = nFilterIndex;

  switch(mode) {
  case WINFILEDIALOGW_MODE_GET1:
    pofnw->Flags = WINFILEDLG_GET1_FLAG;
    if(GetOpenFileNameW(pofnw) == FALSE) 
      res = FALSE;
    break;
  case WINFILEDIALOGW_MODE_GET2:
    pofnw->Flags = WINFILEDLG_GET2_FLAG;
    if(GetOpenFileNameW(pofnw) == FALSE) 
      res = FALSE;
    break;
  case WINFILEDIALOGW_MODE_SET:
    pofnw->Flags = WINFILEDLG_SET_FLAG;
    if(GetSaveFileNameW(pofnw) == FALSE) 
      res = FALSE;
    break;
  default:
    res = FALSE;
    break;
  }
 
  return res;
}

