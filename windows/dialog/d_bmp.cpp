/**
 * @file	d_bmp.cpp
 * @brief	bmp dialog
 */

#include <compiler.h>
#include "resource.h"
#include "dialog.h"
#include <dosio.h>
#include <np2.h>
#include <sysmng.h>
#include "misc/DlgProc.h"
#include <pccore.h>
#include <io/iocore.h>
#include <common/strres.h>
#include <common/bmpdata.h>
#include <vram/scrnsave.h>
#ifdef SUPPORT_WAB
#include <wab/wab.h>
#include <wab/wabbmpsave.h>
#endif
#include "dialog/winfiledlg.h"

/** フィルター */
static const UINT s_nFilter[4] =
{
	IDS_BMPFILTER1,
	IDS_BMPFILTER4,
	IDS_BMPFILTER8,
	IDS_BMPFILTER24
};

/**
 * デフォルト ファイルを得る
 * @param[in] lpExt 拡張子
 * @param[out] lpFilename ファイル名
 * @param[in] cchFilename ファイル名長
 */
static void GetDefaultFilename(LPCTSTR lpExt, LPTSTR lpFilename, UINT cchFilename)
{
	for (UINT i = 0; i < 10000; i++)
	{
		TCHAR szFilename[MAX_PATH];
		wsprintf(szFilename, TEXT("NP2_%04d.%s"), i, lpExt);

		file_cpyname(lpFilename, bmpfilefolder, cchFilename);
		file_cutname(lpFilename);
		file_catname(lpFilename, szFilename, cchFilename);

		if (file_attr(lpFilename) == -1)
		{
			break;
		}
	}
}


/**
 * BMP 出力
 * @param[in] hWnd 親ウィンドウ
 */
void dialog_writebmp(HWND hWnd)
{
	SCRNSAVE ss = scrnsave_create();
	if (ss == NULL)
	{
		return;
	}
	int nType = scrnsave_gettype(ss);
#ifdef SUPPORT_WAB
	if(np2wab.relay){
		nType = 3;
	}
#endif

	std::tstring rExt(LoadTString(IDS_BMPEXT));
	std::tstring rFilter(LoadTString(s_nFilter[nType]));
	std::tstring rTitle(LoadTString(IDS_BMPTITLE));

	TCHAR szPath[MAX_PATH];
	TCHAR szName[MAX_PATH];
	GetDefaultFilename(rExt.c_str(), szPath, _countof(szPath));

	OPENFILENAMEW ofnw;
	if (WinFileDialogW(hWnd, &ofnw, WINFILEDIALOGW_MODE_SET, szPath, szName, rExt.c_str(), rTitle.c_str(), rFilter.c_str(), 1))
	{
		LPCTSTR lpFilename = szPath;
		file_cpyname(bmpfilefolder, lpFilename, _countof(bmpfilefolder));
		sysmng_update(SYS_UPDATEOSCFG);

		LPCTSTR lpExt = file_getext(szPath);
		if ((nType <= SCRNSAVE_8BIT) && (!file_cmpname(lpExt, TEXT("gif"))))
		{
			scrnsave_writegif(ss, lpFilename, SCRNSAVE_AUTO);
		}
		else if (!file_cmpname(lpExt, str_bmp))
		{
#ifdef SUPPORT_WAB
			if(np2wab.relay){
				np2wab_writebmp(lpFilename);
			}else{
#endif
				scrnsave_writebmp(ss, lpFilename, SCRNSAVE_AUTO);
#ifdef SUPPORT_WAB
			}
#endif
		}
	}
	scrnsave_destroy(ss);
}
