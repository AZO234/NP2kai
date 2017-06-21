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

/**
 * フォント選択
 * @param[in] hWnd 親ウィンドウ
 */
void dialog_font(HWND hWnd)
{
	std::tstring rExt(LoadTString(IDS_FONTEXT));
	std::tstring rFilter(LoadTString(IDS_FONTFILTER));
	std::tstring rTitle(LoadTString(IDS_FONTTITLE));

	CFileDlg dlg(TRUE, rExt.c_str(), np2cfg.fontfile, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, rFilter.c_str(), hWnd);
	dlg.m_ofn.lpstrTitle = rTitle.c_str();
	dlg.m_ofn.nFilterIndex = 3;
	if (!dlg.DoModal())
	{
		return;
	}

	LPCTSTR lpFilename = dlg.GetPathName();
	if (font_load(lpFilename, FALSE))
	{
		gdcs.textdisp |= GDCSCRN_ALLDRAW2;
		milstr_ncpy(np2cfg.fontfile, lpFilename, _countof(np2cfg.fontfile));
		sysmng_update(SYS_UPDATECFG);
	}
}
