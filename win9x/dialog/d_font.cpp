/**
 * @file	d_font.cpp
 * @brief	font dialog
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "dosio.h"
#include "np2.h"
#include "sysmng.h"
#include "misc/DlgProc.h"
#include "pccore.h"
#include "iocore.h"
#include "font/font.h"
#include "dialog/winfiledlg.h"

/**
 * フォント選択
 * @param[in] hWnd 親ウィンドウ
 */
void dialog_font(HWND hWnd)
{
	TCHAR szPath[MAX_PATH];
	TCHAR szName[MAX_PATH];
	std::tstring rExt(LoadTString(IDS_FONTEXT));
	std::tstring rFilter(LoadTString(IDS_FONTFILTER));
	std::tstring rTitle(LoadTString(IDS_FONTTITLE));

	OPENFILENAMEW ofnw;
	if (WinFileDialogW(hWnd, &ofnw, WINFILEDIALOGW_MODE_GET1, szPath, szName, rExt.c_str(), rTitle.c_str(), rFilter.c_str(), 3))
	{
		return;
	}

	LPCTSTR lpFilename = szPath;
	if (font_load(lpFilename, FALSE))
	{
		gdcs.textdisp |= GDCSCRN_ALLDRAW2;
		milstr_ncpy(np2cfg.fontfile, lpFilename, _countof(np2cfg.fontfile));
		sysmng_update(SYS_UPDATECFG);
	}
}
