/* === NP2 file dialog for Windows === (c) 2020 AZO */

#include "winfiledlg.h"
#include "codecnv.h"

#define WINFILEDLG_GET_FLAG (OFN_FILEMUSTEXIST | OFN_HIDEREADONLY)
#define WINFILEDLG_SET1_FLAG (OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHAREAWARE)
#define WINFILEDLG_SET2_FLAG (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY)

BOOL WinFileDialogW_MM(
  HWND hwnd,
  const WinFileDialogW_Mode_t mode,
  char* path,
  char* name,
  const char* title,
  const char* filter
) {
  BOOL res;
  wchar_t wpath[MAX_PATH];
  wchar_t wname[MAX_PATH];

  res = WinFileDialogW_MW(hwnd, mode, wpath, wname, title, filter);

  codecnv_ucs2toutf8(path, MAX_PATH, wpath, MAX_PATH);
  codecnv_ucs2toutf8(name, MAX_PATH, wname, MAX_PATH);

  return res;
}

BOOL WinFileDialogW_MW(
  HWND hwnd,
  const WinFileDialogW_Mode_t mode,
  wchar_t* path,
  wchar_t* name,
  const char* title,
  const char* filter
) {
  wchar_t wtitle[256];
  wchar_t wfilter[MAX_PATH];

  codecnv_utf8toucs2(wtitle, 256, title, 256);
  codecnv_utf8toucs2(wfilter, MAX_PATH, filter, MAX_PATH);

  return WinFileDialogW_WW(hwnd, mode, wpath, wname, title, filter);
}

BOOL WinFileDialogW_WM(
  HWND hwnd,
  const WinFileDialogW_Mode_t mode,
  char* path,
  char* name,
  const wchar_t* title,
  const wchar_t* filter
) {
  BOOL res;
  wchar_t wpath[MAX_PATH];
  wchar_t wname[MAX_PATH];

  res = WinFileDialogW_WW(hwnd, mode, wpath, wname, title, filter);

  codecnv_ucs2toutf8(path, MAX_PATH, wpath, MAX_PATH);
  codecnv_ucs2toutf8(name, MAX_PATH, wname, MAX_PATH);

  return res;
}

BOOL WinFileDialogW_WW(
  HWND hwnd,
  const WinFileDialogW_Mode_t mode,
  wchar_t* path,
  wchar_t* name,
  const wchar_t* title,
  const wchar_t* filter
) {
  BOOL res = TRUE;
  OPENFILENAMEW ofnw;

  path[0] = L'\0';
  name[0] = L'\0';

  memset(&ofnw, 0, sizeof(OPENFILENAMEW));

  ofnw.lStructSize = sizeof(OPENFILENAMEW);
  ofnw.hwndOwner = hwnd;
  ofnw.lpstrFile = path;
  ofnw.nMaxFile = MAX_PATH;
  ofnw.lpstrFileTitle = name;
  ofnw.nMaxFileTitle = MAX_PATH;
  ofnw.lpstrFilter = filter;
  ofnw.lpstrTitle = title;

  switch(mode) {
  case WINFILEDIALOGW_MODE_GET:
    ofnw.Flags = WINFILEDLG_GET_FLAG;
    if(GetOpenFileNameW(&ofnw) == FALSE) 
      res = FALSE;
    break;
  case WINFILEDIALOGW_MODE_SET1:
    ofnw.Flags = WINFILEDLG_SET1_FLAG;
    if(GetSaveFileNameW(&ofnw) == FALSE) 
      res = FALSE;
    break;
  case WINFILEDIALOGW_MODE_SET2:
    ofnw.Flags = WINFILEDLG_SET2_FLAG;
    if(GetSaveFileNameW(&ofnw) == FALSE) 
      res = FALSE;
    break;
  default:
    res = FALSE;
    break;
  }
 
  return res;
}

