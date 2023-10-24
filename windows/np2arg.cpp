/**
 *	@file	np2arg.cpp
 *	@brief	引数情報クラスの動作の定義を行います
 */

#include <compiler.h>
#include "np2arg.h"
#include <dosio.h>

#define	MAXARG		32				//!< 最大引数エントリ数
#define	ARG_BASE	1				//!< win32 の lpszCmdLine の場合の開始エントリ

//! 唯一のインスタンスです
Np2Arg Np2Arg::sm_instance;

/**
 * コンストラクタ
 */
Np2Arg::Np2Arg()
{
	ZeroMemory(this, sizeof(*this));
}

/**
 * デストラクタ
 */
Np2Arg::~Np2Arg()
{
	if(m_lpArg){
		free(m_lpArg);
		m_lpArg = NULL;
	}
	if(m_lpIniFile){
		free(m_lpIniFile); // np21w ver0.86 rev8
		m_lpIniFile = NULL;
	}
}

/**
 * パース
 */
void Np2Arg::Parse()
{
	LPCTSTR lpIniFile = NULL;

	// 引数読み出し
	if(m_lpArg) {
		free(m_lpArg);
	}
	m_lpArg = _tcsdup(::GetCommandLine());

	LPTSTR argv[MAXARG];
	const int argc = ::milstr_getarg(m_lpArg, argv, _countof(argv));

	int nDrive = 0;
	int nCDrive = 0;

	for (int i = ARG_BASE; i < argc; i++)
	{
		LPCTSTR lpArg = argv[i];
		if ((lpArg[0] == TEXT('/')) || (lpArg[0] == TEXT('-')))
		{
			switch (_totlower(lpArg[1]))
			{
				case 'f':
					m_fFullscreen = true;
					break;

				case 'i':
					lpIniFile = &lpArg[2];
					break;
			}
		}
		else
		{
			LPCTSTR lpExt = ::file_getext(lpArg);
			if (::file_cmpname(lpExt, TEXT("ini")) == 0 || ::file_cmpname(lpExt, TEXT("npc")) == 0 || ::file_cmpname(lpExt, TEXT("npcfg")) == 0 || ::file_cmpname(lpExt, TEXT("np2cfg")) == 0 || ::file_cmpname(lpExt, TEXT("np21cfg")) == 0 || ::file_cmpname(lpExt, TEXT("np21wcfg")) == 0)
			{
				lpIniFile = lpArg;
			}
			else if (::file_cmpname(lpExt, TEXT("iso")) == 0 || ::file_cmpname(lpExt, TEXT("cue")) == 0 || ::file_cmpname(lpExt, TEXT("ccd")) == 0 || ::file_cmpname(lpExt, TEXT("cdm")) == 0 || ::file_cmpname(lpExt, TEXT("mds")) == 0 || ::file_cmpname(lpExt, TEXT("nrg")) == 0)
			{
				// CDぽい
				if (nCDrive < _countof(m_lpCDisk))
				{
					m_lpCDisk[nCDrive++] = lpArg;
				}
			}
			else 
			{
				// ディスク扱い
				if (nDrive < _countof(m_lpDisk))
				{
					m_lpDisk[nDrive++] = lpArg;
				}
			}
		}
	}
	if(lpIniFile){ // np21w ver0.86 rev8
		LPTSTR strbuf;
		strbuf = (LPTSTR)calloc(500, sizeof(TCHAR));
		if(!(_tcsstr(lpIniFile,_T(":"))!=NULL || (lpIniFile[0]=='¥¥'))){
			// ファイル名のみの指定っぽかったら現在のディレクトリを結合
			//getcwd(pathname, 300);
			GetCurrentDirectory(500, strbuf);
			if(strbuf[_tcslen(strbuf)-1]!='¥¥'){
				_tcscat(strbuf, _T("¥¥")); // XXX: Linuxとかだったらスラッシュじゃないと駄目だよね？ -> Win専用だから問題ない
			}
		}
		_tcscat(strbuf, lpIniFile);
		m_lpIniFile = strbuf;
	}
}

/**
 * ディスク情報をクリア
 */
void Np2Arg::ClearDisk()
{
	ZeroMemory(m_lpDisk, sizeof(m_lpDisk));
	ZeroMemory(m_lpCDisk, sizeof(m_lpCDisk));
}
