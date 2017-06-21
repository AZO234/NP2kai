/**
 *	@file	np2arg.cpp
 *	@brief	引数情報クラスの動作の定義を行います
 */

#include "compiler.h"
#include "np2arg.h"
#include "dosio.h"

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
	free(m_lpArg);
	if(m_lpIniFile) free((TCHAR*)m_lpIniFile); // np21w ver0.86 rev8
}

/**
 * パース
 */
void Np2Arg::Parse()
{
	// 引数読み出し
	free(m_lpArg);
	m_lpArg = _tcsdup(::GetCommandLine());

	LPTSTR argv[MAXARG];
	const int argc = ::milstr_getarg(m_lpArg, argv, _countof(argv));

	int nDrive = 0;

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
					m_lpIniFile = &lpArg[2];
					break;
			}
		}
		else
		{
			LPCTSTR lpExt = ::file_getext(lpArg);
			if (::file_cmpname(lpExt, TEXT("ini")) == 0 || ::file_cmpname(lpExt, TEXT("npc")) == 0 || ::file_cmpname(lpExt, TEXT("npcfg")) == 0 || ::file_cmpname(lpExt, TEXT("np2cfg")) == 0 || ::file_cmpname(lpExt, TEXT("np21cfg")) == 0 || ::file_cmpname(lpExt, TEXT("np21wcfg")) == 0)
			{
				m_lpIniFile = lpArg;
			}
			else if (nDrive < _countof(m_lpDisk))
			{
				m_lpDisk[nDrive++] = lpArg;
			}
		}
	}
	if(m_lpIniFile){ // np21w ver0.86 rev8
		LPTSTR strbuf;
		strbuf = (LPTSTR)calloc(500, sizeof(TCHAR));
		if(!(_tcsstr(m_lpIniFile,_T(":"))!=NULL || (m_lpIniFile[0]=='\\'))){
			// ファイル名のみの指定っぽかったら現在のディレクトリを結合
			//getcwd(pathname, 300);
			GetCurrentDirectory(500, strbuf);
			if(strbuf[_tcslen(strbuf)-1]!='\\'){
				_tcscat(strbuf, _T("\\")); // XXX: Linuxとかだったらスラッシュじゃないと駄目だよね？
			}
		}
		_tcscat(strbuf, m_lpIniFile);
		m_lpIniFile = strbuf;
	}
}

/**
 * ディスク情報をクリア
 */
void Np2Arg::ClearDisk()
{
	ZeroMemory(m_lpDisk, sizeof(m_lpDisk));
}
