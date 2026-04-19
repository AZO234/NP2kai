/**
 *	@file	dosio.cpp
 *	@brief	ファイル アクセス関数群の動作の定義を行います
 */

#include "compiler.h"
#include "dosio.h"

//! カレント パス バッファ
static OEMCHAR curpath[MAX_PATH];

//! ファイル名ポインタ
static OEMCHAR *curfilep = curpath;

/**
 * 初期化
 */
void dosio_init(void)
{
}

/**
 * 解放
 */
void dosio_term(void)
{
}

/**
 * ファイルを開きます
 * @param[in] lpPathName ファイル名
 * @return ファイル ハンドル
 */
FILEH DOSIOCALL file_open(const OEMCHAR* lpPathName)
{
	FILEH hFile = ::CreateFile(lpPathName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		hFile = ::CreateFile(lpPathName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	return hFile;
}

/**
 * リード オンリーでファイルを開きます
 * @param[in] lpPathName ファイル名
 * @return ファイル ハンドル
 */
FILEH DOSIOCALL file_open_rb(const OEMCHAR* lpPathName)
{
	return ::CreateFile(lpPathName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

/**
 * ファイルを作成します
 * @param[in] lpPathName ファイル名
 * @return ファイル ハンドル
 */
FILEH DOSIOCALL file_create(const OEMCHAR* lpPathName)
{
	return ::CreateFile(lpPathName, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

/**
 * ファイルのシーク
 * @param[in] hFile ファイル ハンドル
 * @param[in] pointer 移動すべきバイト数
 * @param[in] method 開始点
 * @return ファイルの位置
 */
FILEPOS DOSIOCALL file_seek(FILEH hFile, FILEPOS pointer, int method)
{
#ifdef SUPPORT_LARGE_HDD
	LARGE_INTEGER li, lires;
	li.QuadPart = pointer;
	::SetFilePointerEx(hFile, li, &lires, method);
	return lires.QuadPart;
#else
	return static_cast<long>(::SetFilePointer(hFile, pointer, 0, method));
#endif
}

/**
 * ファイル読み込み
 * @param[in] hFile ファイル ハンドル
 * @param[out] lpBuffer バッファ
 * @param[in] cbBuffer バッファ サイズ
 * @return 読み込みサイズ
 */
UINT DOSIOCALL file_read(FILEH hFile, void* lpBuffer, UINT cbBuffer)
{
	DWORD dwReadSize;
	if (::ReadFile(hFile, lpBuffer, cbBuffer, &dwReadSize, NULL))
	{
		return dwReadSize;
	}
	return 0;
}

/**
 * ファイル書き込み
 * @param[in] hFile ファイル ハンドル
 * @param[in] lpBuffer バッファ
 * @param[in] cbBuffer バッファ サイズ
 * @return 書き込みサイズ
 */
UINT DOSIOCALL file_write(FILEH hFile, const void* lpBuffer, UINT cbBuffer)
{
	if (cbBuffer != 0)
	{
		DWORD dwWrittenSize;
		if (::WriteFile(hFile, lpBuffer, cbBuffer, &dwWrittenSize, NULL))
		{
			return dwWrittenSize;
		}
	}
	else
	{
		::SetEndOfFile(hFile);
	}
	return 0;
}

/**
 * ファイル ハンドルを閉じる
 * @param[in] hFile ファイル ハンドル
 * @retval 0 成功
 */
short DOSIOCALL file_close(FILEH hFile)
{
	::CloseHandle(hFile);
	return 0;
}

/**
 * ファイル サイズを得る
 * @param[in] hFile ファイル ハンドル
 * @return ファイル サイズ
 */
FILELEN DOSIOCALL file_getsize(FILEH hFile)
{
#ifdef SUPPORT_LARGE_HDD
	LARGE_INTEGER lires;
	::GetFileSizeEx(hFile, &lires);
	return lires.QuadPart;
#else
	return ::GetFileSize(hFile, NULL);
#endif
}

/**
 * FILETIME を DOSDATE/DOSTIME に変換
 * @param[in] ft ファイル タイム
 * @param[out] dosdate DOSDATE
 * @param[out] dostime DOSTIME
 * @retval true 成功
 * @retval false 失敗
 */
static bool convertDateTime(const FILETIME& ft, DOSDATE* dosdate, DOSTIME* dostime)
{
	FILETIME ftLocalTime;
	if (!::FileTimeToLocalFileTime(&ft, &ftLocalTime))
	{
		return false;
	}

	SYSTEMTIME st;
	if (!::FileTimeToSystemTime(&ftLocalTime, &st))
	{
		return false;
	}

	if (dosdate)
	{
		dosdate->year = st.wYear;
		dosdate->month = static_cast<UINT8>(st.wMonth);
		dosdate->day = static_cast<UINT8>(st.wDay);
	}
	if (dostime)
	{
		dostime->hour = static_cast<UINT8>(st.wHour);
		dostime->minute = static_cast<UINT8>(st.wMinute);
		dostime->second = static_cast<UINT8>(st.wSecond);
	}
	return true;
}

/**
 * ファイルのタイム スタンプを得る
 * @param[in] hFile ファイル ハンドル
 * @param[out] dosdate DOSDATE
 * @param[out] dostime DOSTIME
 * @retval 0 成功
 * @retval -1 失敗
 */
short DOSIOCALL file_getdatetime(FILEH hFile, DOSDATE* dosdate, DOSTIME* dostime)
{
	FILETIME ft;
	if (!::GetFileTime(hFile, NULL, NULL, &ft))
	{
		return -1;
	}
	return (convertDateTime(ft, dosdate, dostime)) ? 0 : -1;
}

/**
 * ファイルの削除
 * @param[in] lpPathName ファイル名
 * @retval 0 成功
 * @retval -1 失敗
 */
short DOSIOCALL file_delete(const OEMCHAR* lpPathName)
{
	return (::DeleteFile(lpPathName)) ? 0 : -1;
}

/**
 * ファイルの属性を得る
 * @param[in] lpPathName ファイル名
 * @return ファイル属性
 */
short DOSIOCALL file_attr(const OEMCHAR* lpPathName)
{
	return static_cast<short>(::GetFileAttributes(lpPathName));
}

/**
 * ファイルの属性を設定
 * @param[in] lpPathName ファイル名
 * @param[in] attr ファイル属性
 * @retval 0 成功
 * @retval -1 失敗
 */
short DOSIOCALL file_setattr(const OEMCHAR* lpPathName, short attr)
{
	return (::SetFileAttributes(lpPathName, attr) ? 0 : -1);
}

/**
 * ファイルの移動
 * @param[in] lpExistFile ファイル名
 * @param[in] lpNewFile ファイル名
 * @retval 0 成功
 * @retval -1 失敗
 */
short DOSIOCALL file_rename(const OEMCHAR* lpExistFile, const OEMCHAR* lpNewFile)
{
	return (::MoveFile(lpExistFile, lpNewFile)) ? 0 : -1;
}

/**
 * ディレクトリ作成
 * @param[in] lpPathName パス
 * @retval 0 成功
 * @retval -1 失敗
 */
short DOSIOCALL file_dircreate(const OEMCHAR* lpPathName)
{
	return (::CreateDirectory(lpPathName, NULL)) ? 0 : -1;
}

/**
 * ディレクトリ削除
 * @param[in] lpPathName パス
 * @retval 0 成功
 * @retval -1 失敗
 */
short DOSIOCALL file_dirdelete(const OEMCHAR* lpPathName)
{
	return (::RemoveDirectory(lpPathName)) ? 0 : -1;
}



// ---- カレントファイル操作

/**
 * カレント パス設定
 * @param[in] lpPathName カレント ファイル名
 */
void DOSIOCALL file_setcd(const OEMCHAR* lpPathName)
{
	file_cpyname(curpath, lpPathName, NELEMENTS(curpath));
	curfilep = file_getname(curpath);
	*curfilep = '\0';
}

/**
 * カレント パス取得
 * @param[in] lpFilename ファイル名
 * @return パス
 */
OEMCHAR* DOSIOCALL file_getcd(const OEMCHAR* lpFilename)
{
	file_cpyname(curfilep, lpFilename, NELEMENTS(curpath) - (int)(curfilep - curpath));
	return curpath;
}

/**
 * カレント ファイルを開きます
 * @param[in] lpFilename ファイル名
 * @return ファイル ハンドル
 */
FILEH DOSIOCALL file_open_c(const OEMCHAR* lpFilename)
{
	return file_open(file_getcd(lpFilename));
}

/**
 * リード オンリーでカレント ファイルを開きます
 * @param[in] lpFilename ファイル名
 * @return ファイル ハンドル
 */

FILEH DOSIOCALL file_open_rb_c(const OEMCHAR* lpFilename)
{
	return file_open_rb(file_getcd(lpFilename));
}

/**
 * カレント ファイルを作成します
 * @param[in] lpFilename ファイル名
 * @return ファイル ハンドル
 */
FILEH DOSIOCALL file_create_c(const OEMCHAR* lpFilename)
{
	return file_create(file_getcd(lpFilename));
}

/**
 * カレント ファイルの削除
 * @param[in] lpFilename ファイル名
 * @retval 0 成功
 * @retval -1 失敗
 */
short DOSIOCALL file_delete_c(const OEMCHAR* lpFilename)
{
	return file_delete(file_getcd(lpFilename));
}

/**
 * カレント ファイルの属性を得る
 * @param[in] lpFilename ファイル名
 * @return ファイル属性
 */
short DOSIOCALL file_attr_c(const OEMCHAR* lpFilename)
{
	return file_attr(file_getcd(lpFilename));
}



// ---- ファイル検索

/**
 * WIN32_FIND_DATA を FLINFO に変換
 * @param[in] w32fd WIN32_FIND_DATA
 * @param[out] fli FLINFO
 * @retval true 成功
 * @retval false 失敗
 */
static bool DOSIOCALL setFLInfo(const WIN32_FIND_DATA& w32fd, FLINFO *fli)
{
#if !defined(_WIN32_WCE)
	if ((w32fd.dwFileAttributes & FILEATTR_DIRECTORY) && (w32fd.cFileName[0] == '.'))
	{
		return false;
	}
#endif	// !defined(_WIN32_WCE)

	if (fli)
	{
		fli->caps = FLICAPS_SIZE | FLICAPS_ATTR | FLICAPS_DATE | FLICAPS_TIME;
		fli->size = w32fd.nFileSizeLow;
		fli->attr = w32fd.dwFileAttributes;
		convertDateTime(w32fd.ftLastWriteTime, &fli->date, &fli->time);
		file_cpyname(fli->path, w32fd.cFileName, NELEMENTS(fli->path));
	}
	return true;
}

/**
 * ファイルの検索
 * @param[in] lpPathName パス
 * @param[out] fli 検索結果
 * @return ファイル検索ハンドル
 */
FLISTH DOSIOCALL file_list1st(const OEMCHAR* lpPathName, FLINFO* fli)
{
	static const OEMCHAR s_szWildCard[] = OEMTEXT("*.*");

	OEMCHAR szPath[MAX_PATH];
	file_cpyname(szPath, lpPathName, NELEMENTS(szPath));
	file_setseparator(szPath, NELEMENTS(szPath));
	file_catname(szPath, s_szWildCard, NELEMENTS(szPath));

	WIN32_FIND_DATA w32fd;
	HANDLE hFile = ::FindFirstFile(szPath, &w32fd);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (setFLInfo(w32fd, fli))
			{
				return hFile;
			}
		} while(::FindNextFile(hFile, &w32fd));
		::FindClose(hFile);
	}
	return FLISTH_INVALID;
}

/**
 * ファイルの検索
 * @param[in] hList ファイル検索ハンドル
 * @param[out] fli 検索結果
 * @retval SUCCESS 成功
 * @retval FAILURE 失敗
 */
BRESULT DOSIOCALL file_listnext(FLISTH hList, FLINFO* fli)
{
	WIN32_FIND_DATA w32fd;
	while (::FindNextFile(hList, &w32fd))
	{
		if (setFLInfo(w32fd, fli))
		{
			return SUCCESS;
		}
	}
	return FAILURE;
}

/**
 * ファイル検索ハンドルを閉じる
 * @param[in] hList ファイル検索ハンドル
 */
void DOSIOCALL file_listclose(FLISTH hList)
{
	::FindClose(hList);
}



// ---- ファイル名操作

/**
 * ファイル名のポインタを得る
 * @param[in] lpPathName パス
 * @return ポインタ
 */
OEMCHAR* DOSIOCALL file_getname(const OEMCHAR* lpPathName)
{
	const OEMCHAR* ret = lpPathName;
	while (1 /* EVER */)
	{
		const int cch = milstr_charsize(lpPathName);
		if (cch == 0)
		{
			break;
		}
		else if ((cch == 1) && ((*lpPathName == '\\') || (*lpPathName == '/') || (*lpPathName == ':')))
		{
			ret = lpPathName + 1;
		}
		lpPathName += cch;
	}
	return const_cast<OEMCHAR*>(ret);
}

/**
 * ファイル名を削除
 * @param[in,out] lpPathName パス
 */
void DOSIOCALL file_cutname(OEMCHAR* lpPathName)
{
	OEMCHAR* p = file_getname(lpPathName);
	p[0] = '\0';
}

/**
 * 拡張子のポインタを得る
 * @param[in] lpPathName パス
 * @return ポインタ
 */
OEMCHAR* DOSIOCALL file_getext(const OEMCHAR* lpPathName)
{
	const OEMCHAR* p = file_getname(lpPathName);
	const OEMCHAR* q = NULL;
	while (1 /* EVER */)
	{
		const int cch = milstr_charsize(p);
		if (cch == 0)
		{
			break;
		}
		else if ((cch == 1) && (*p == '.'))
		{
			q = p + 1;
		}
		p += cch;
	}
	if (q == NULL)
	{
		q = p;
	}
	return const_cast<OEMCHAR*>(q);
}

/**
 * 拡張子を削除
 * @param[in,out] lpPathName パス
 */
void DOSIOCALL file_cutext(OEMCHAR* lpPathName)
{
	OEMCHAR* p = file_getname(lpPathName);
	OEMCHAR* q = NULL;
	while (1 /* EVER */)
	{
		const int cch = milstr_charsize(p);
		if (cch == 0)
		{
			break;
		}
		else if ((cch == 1) && (*p == '.'))
		{
			q = p;
		}
		p += cch;
	}
	if (q)
	{
		*q = '\0';
	}
}

/**
 * パス セパレータを削除
 * @param[in,out] lpPathName パス
 */
void DOSIOCALL file_cutseparator(OEMCHAR* lpPathName)
{
	const int pos = OEMSTRLEN(lpPathName) - 1;
	if ((pos > 0) &&								// 2文字以上でー
		(lpPathName[pos] == '\\') &&				// ケツが \ でー
		(!milstr_kanji2nd(lpPathName, pos)) &&		// 漢字の2バイト目ぢゃなくてー
		((pos != 1) || (lpPathName[0] != '\\')) &&	// '\\' ではなくてー
		((pos != 2) || (lpPathName[1] != ':')))		// '?:\' ではなかったら
	{
		lpPathName[pos] = '\0';
	}
}

/**
 * パス セパレータを追加
 * @param[in,out] lpPathName パス
 * @param[in] cchPathName バッファ長
 */
void DOSIOCALL file_setseparator(OEMCHAR* lpPathName, int cchPathName)
{
	const int pos = OEMSTRLEN(lpPathName) - 1;
	if ((pos < 0) ||
		((pos == 1) && (lpPathName[1] == ':')) ||
		((lpPathName[pos] == '\\') && (!milstr_kanji2nd(lpPathName, pos))) ||
		((pos + 2) >= cchPathName))
	{
		return;
	}
	lpPathName[pos + 1] = '\\';
	lpPathName[pos + 2] = '\0';
}
