/**
 * @file	d_cfgsave.cpp
 * @brief	save VM configuration dialog
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

/** フィルター */
static const UINT s_nFilter[1] =
{
	IDS_CFGFILTER
};

/**
 * デフォルト ファイルを得る
 * @param[in] lpExt 拡張子
 * @param[out] lpFilename ファイル名
 * @param[in] cchFilename ファイル名長
 */
static void GetDefaultFilename(LPCTSTR lpExt, LPTSTR lpFilename, UINT cchFilename)
{
	if(np2cfg.fddfile[0][0] || np2cfg.sasihdd[0][0]){
		TCHAR szFilename[MAX_PATH];
		TCHAR *fname, *extpos;
		if(np2cfg.fddfile[0][0]){
			fname = _tcsrchr(np2cfg.fddfile[0], '\\')+1;
		}else if(np2cfg.sasihdd[0][0]){
			fname = _tcsrchr(np2cfg.sasihdd[0], '\\')+1;
		}
		extpos = _tcsrchr(fname, '.');
		if(extpos){
			*extpos = '\0';
			wsprintf(szFilename, TEXT("%s.%s"), fname, lpExt);
			*extpos = '.';
		}else{
			wsprintf(szFilename, TEXT("%s.%s"), fname, lpExt);
		}
		file_cpyname(lpFilename, npcfgfilefolder, cchFilename);
		file_cutname(lpFilename);
		file_catname(lpFilename, szFilename, cchFilename);
	}else{
		for (UINT i = 0; i < 10000; i++)
		{
			TCHAR szFilename[MAX_PATH];
			wsprintf(szFilename, TEXT("NP2_%04d.%s"), i, lpExt);

			file_cpyname(lpFilename, npcfgfilefolder, cchFilename);
			file_cutname(lpFilename);
			file_catname(lpFilename, szFilename, cchFilename);

			if (file_attr(lpFilename) == -1)
			{
				break;
			}
		}
	}
}

/**
 * VM configuration 出力
 * @param[in] hWnd 親ウィンドウ
 */
void dialog_writenpcfg(HWND hWnd)
{
	std::tstring rExt(LoadTString(IDS_CFGEXT));
	std::tstring rFilter(LoadTString(s_nFilter[0]));
	std::tstring rTitle(LoadTString(IDS_CFGTITLE));

	TCHAR szPath[MAX_PATH];
	GetDefaultFilename(rExt.c_str(), szPath, _countof(szPath));

	CFileDlg dlg(FALSE, rExt.c_str(), szPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, rFilter.c_str(), hWnd);
	dlg.m_ofn.lpstrTitle = rTitle.c_str();
	dlg.m_ofn.nFilterIndex = 1;
	if (dlg.DoModal())
	{
		LPCTSTR lpFilename = dlg.GetPathName();
		LPCTSTR lpExt = file_getext(szPath);
		file_cpyname(npcfgfilefolder, lpFilename, _countof(bmpfilefolder));
		sysmng_update(SYS_UPDATEOSCFG);
		LPTSTR lpFilenameBuf = (LPTSTR)malloc((_tcslen(lpFilename)+1)*sizeof(TCHAR));
		_tcscpy(lpFilenameBuf, lpFilename);
		Np2Arg::GetInstance()->setiniFilename(lpFilenameBuf);
		initsave();
		toolwin_writeini();
		kdispwin_writeini();
		skbdwin_writeini();
		mdbgwin_writeini();
#if defined(SUPPORT_WAB)
		wabwin_writeini();
#endif	// defined(SUPPORT_WAB)
	}
}
