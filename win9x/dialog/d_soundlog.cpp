/**
 * @file	d_soundlog.cpp
 * @brief	soundlog dialog
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "np2.h"
#include "dosio.h"
#include "sysmng.h"
#if defined(SUPPORT_RECVIDEO)
#include "recvideo.h"
#endif	// defined(SUPPORT_RECVIDEO)
#include "misc/DlgProc.h"
#if defined(SUPPORT_WAVEREC)
#include "sound/sound.h"
#endif	// defined(SUPPORT_WAVEREC)
#if defined(SUPPORT_S98)
#include "sound/s98.h"
#endif	// defined(SUPPORT_S98)

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
 * サウンド ログ
 * @param[in] hWnd 親ウィンドウ
 */
void dialog_soundlog(HWND hWnd)
{
	UINT nExtId = 0;
	std::tstring rFilter;

#if defined(SUPPORT_S98)
	S98_close();
	if (nExtId == 0)
	{
		nExtId = IDS_S98EXT;
	}
	rFilter += LoadTString(IDS_S98FILTER);
#endif	// defined(SUPPORT_S98)

#if defined(SUPPORT_WAVEREC)
	sound_recstop();

	if (nExtId == 0)
	{
		nExtId = IDS_WAVEEXT;
	}
	rFilter += LoadTString(IDS_WAVEFILTER);
#endif	// defined(SUPPORT_WAVEREC)

#if defined(SUPPORT_RECVIDEO)
	recvideo_close();
#endif	// defined(SUPPORT_RECVIDEO)

	if (nExtId == 0)
	{
		return;
	}

	std::tstring rExt(LoadTString(nExtId));
	std::tstring rTitle(LoadTString(IDS_WAVETITLE));

	TCHAR szPath[MAX_PATH];
	GetDefaultFilename(rExt.c_str(), szPath, _countof(szPath));

	CFileDlg dlg(FALSE, rExt.c_str(), szPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, rFilter.c_str(), hWnd);
	dlg.m_ofn.lpstrTitle = rTitle.c_str();
	dlg.m_ofn.nFilterIndex = 1;
	if (!dlg.DoModal())
	{
		return;
	}

	LPCTSTR lpFilename = dlg.GetPathName();
	file_cpyname(bmpfilefolder, lpFilename, _countof(bmpfilefolder));
	sysmng_update(SYS_UPDATEOSCFG);

	LPCTSTR lpExt = file_getext(lpFilename);

#if defined(SUPPORT_S98)
	if (file_cmpname(lpExt, TEXT("s98")) == 0)
	{
		S98_open(lpFilename);
		return;
	}
#endif	// defined(SUPPORT_S98)

#if defined(SUPPORT_WAVEREC)
	if (file_cmpname(lpExt, TEXT("wav")) == 0)
	{
		sound_recstart(lpFilename);
		return;
	}
#endif	// defined(SUPPORT_WAVEREC)

#if defined(SUPPORT_RECVIDEO)
	if (file_cmpname(lpExt, TEXT("avi")) == 0)
	{
		if (recvideo_open(hWnd, lpFilename))
		{
			TCHAR szWaveFilename[MAX_PATH];
			file_cpyname(szWaveFilename, lpFilename, _countof(szWaveFilename));
			file_cutext(szWaveFilename);
			file_catname(szWaveFilename, _T(".wav"), _countof(szWaveFilename));
			sound_recstart(szWaveFilename);
		}
	}
#endif	// defined(SUPPORT_RECVIDEO)
}
