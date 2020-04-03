/**
 * @file	d_cfgload.cpp
 * @brief	load VM configuration dialog
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "dosio.h"
#include "np2.h"
#include "sysmng.h"
#include "misc/DlgProc.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "common/strres.h"
#include "np2arg.h"
#include "profile.h"
#include "ini.h"
#include "subwnd/toolwnd.h"
#include "subwnd/kdispwnd.h"
#include "subwnd/skbdwnd.h"
#include "subwnd/mdbgwnd.h"
#if defined(SUPPORT_WAB)
#include "wab/wab.h"
#endif
#include "dialog/winfiledlg.h"

/** フィルター */
static const UINT s_nFilter[1] =
{
	IDS_CFGFILTER_L
};

static int messagebox(HWND hWnd, LPCTSTR lpcszText, UINT uType)
{
	LPCTSTR szCaption = np2oscfg.titles;

	std::tstring rText(LoadTString(lpcszText));
	return MessageBox(hWnd, rText.c_str(), szCaption, uType);
}

/**
 * VM configuration 読み込み
 * @param[in] hWnd 親ウィンドウ
 */
int dialog_readnpcfg(HWND hWnd)
{
	std::tstring rExt(LoadTString(IDS_CFGEXT));
	std::tstring rFilter(LoadTString(s_nFilter[0]));
	std::tstring rTitle(LoadTString(IDS_CFGTITLE_L));

	TCHAR szPath[MAX_PATH] = {0};
	TCHAR szName[MAX_PATH] = {0};
	file_cpyname(szPath, npcfgfilefolder, _countof(szPath));
	
	CFileDlg dlg(TRUE, rExt.c_str(), szPath, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, rFilter.c_str(), hWnd);
	dlg.m_ofn.lpstrTitle = rTitle.c_str();
	dlg.m_ofn.nFilterIndex = 1;
	OPENFILENAMEW ofnw;
	if (WinFileDialogW(hWnd, &ofnw, WINFILEDIALOGW_MODE_GET1, szPath, szName, rExt.c_str(), rTitle.c_str(), rFilter.c_str(), 1))
	{
		LPCTSTR lpFilename = dlg.GetPathName();
		file_cpyname(npcfgfilefolder, lpFilename, _countof(bmpfilefolder));
		sysmng_update(SYS_UPDATEOSCFG);
		BOOL b = FALSE;
		if (!np2oscfg.comfirm) {
			b = TRUE;
		}
		else
		{
			if (messagebox(hWnd, MAKEINTRESOURCE(IDS_CONFIRM_EXIT),
								MB_ICONQUESTION | MB_YESNO) == IDYES)
			{
				b = TRUE;
			}
		}
		if (b) {
			unloadNP2INI();
			loadNP2INI(lpFilename);
			return 1;
		}
	}
	return 0;
}
