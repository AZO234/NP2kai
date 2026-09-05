/**
 *	@file	dosio.cpp
 *	@brief	ファイル アクセス関数群の動作の定義を行います
 */

#include "compiler.h"
#include "dosio.h"

#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#endif

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
 * リードライト両方可能でファイルを開きます。リードオンリーの場合は失敗します。
 * @param[in] lpPathName ファイル名
 * @return ファイル ハンドル
 */
FILEH DOSIOCALL file_open_rw(const OEMCHAR* lpPathName)
{
	return ::CreateFile(lpPathName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

short DOSIOCALL file_sync(FILEH hFile)
{
	return ::FlushFileBuffers(hFile) ? 0 : -1;
}

short DOSIOCALL file_setsize(FILEH hFile, FILELEN length)
{
	FILEPOS current;

	if (length < 0)
		return -1;
	current = file_seek(hFile, 0, FSEEK_CUR);
	if (current < 0)
		return -1;
	if (file_seek(hFile, (FILEPOS)length, FSEEK_SET) != (FILEPOS)length)
		return -1;
	if (!::SetEndOfFile(hFile)) {
		file_seek(hFile, current, FSEEK_SET);
		return -1;
	}
	return (file_seek(hFile, current, FSEEK_SET) == current) ? 0 : -1;
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
 * ホストファイルシステムが持つ短いファイル名を取得する。ない場合はFAILUREを返す。
 * @param[in] lpPathName 変換元
 * @param[out] lpShortName 結果格納先
 * @param[in] cchShortName 結果格納先バッファサイズ
 * @retval SUCCESS 成功
 * @retval FAILURE 失敗
 */
BRESULT DOSIOCALL file_getshortname(const OEMCHAR* lpPathName, OEMCHAR* lpShortName, UINT cchShortName)
{
	OEMCHAR szShortPath[MAX_PATH];
	DWORD nLength;
	OEMCHAR* lpLeaf;

	if ((lpPathName == NULL) || (lpShortName == NULL) || (cchShortName == 0))
	{
		return FAILURE;
	}
	lpShortName[0] = '\0';
	nLength = ::GetShortPathName(lpPathName, szShortPath, NELEMENTS(szShortPath));
	if ((nLength == 0) || (nLength >= NELEMENTS(szShortPath)))
	{
		return FAILURE;
	}
	lpLeaf = file_getname(szShortPath);
	if ((lpLeaf[0] == '\0') || (OEMSTRLEN(lpLeaf) >= cchShortName))
	{
		return FAILURE;
	}
	file_cpyname(lpShortName, lpLeaf, cchShortName);
	return SUCCESS;
}

BOOL DOSIOCALL file_islink(const OEMCHAR* lpPathName)
{
	const DWORD attr = ::GetFileAttributes(lpPathName);
	return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_REPARSE_POINT)) ? TRUE : FALSE;
}

BOOL DOSIOCALL file_infoislink(const FLINFO* fli, const OEMCHAR* lpPathName)
{
	if ((fli != NULL) && (fli->caps & FLICAPS_ATTR))
	{
		return (fli->attr & FILE_ATTRIBUTE_REPARSE_POINT) ? TRUE : FALSE;
	}
	return file_islink(lpPathName);
}

/**
 * ファイルのタイム スタンプを設定
 * @param[in] hFile ファイル ハンドル
 * @param[in] dosdate DOS日付
 * @param[in] dostime DOS時刻
 * @retval 0 成功
 * @retval -1 失敗
 */
short DOSIOCALL file_setdatetime(FILEH hFile, const DOSDATE* dosdate, const DOSTIME* dostime)
{
	SYSTEMTIME st;
	FILETIME ftLocalTime;
	FILETIME ft;

	if ((dosdate == NULL) || (dostime == NULL))
	{
		return -1;
	}

	::ZeroMemory(&st, sizeof(st));
	st.wYear = dosdate->year;
	st.wMonth = dosdate->month;
	st.wDay = dosdate->day;
	st.wHour = dostime->hour;
	st.wMinute = dostime->minute;
	st.wSecond = dostime->second;

	if ((!::SystemTimeToFileTime(&st, &ftLocalTime)) ||
		(!::LocalFileTimeToFileTime(&ftLocalTime, &ft)) ||
		(!::SetFileTime(hFile, NULL, NULL, &ft)))
	{
		return -1;
	}
	return 0;
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
 * ファイルロックの確認
 * @param[in] lpPathName ファイル名
 * @retval 0 ロックされていない
 * @retval それ以外 ロックされている
 */
short DOSIOCALL file_islocked(const OEMCHAR* lpPathName)
{
	FILEH hFile = ::CreateFile(lpPathName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		const DWORD err = GetLastError();
		if (err == ERROR_SHARING_VIOLATION)
		{
			return -1; // ファイルロック
		}
		else {
			return 0; // ファイルロックではない
		}
	}
	CloseHandle(hFile);
	return 0; // 普通に開ける
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
#if !defined(_WIN32_WCE)
		file_cpyname(fli->shortpath, w32fd.cAlternateFileName, NELEMENTS(fli->shortpath));
#else
		fli->shortpath[0] = '\0';
#endif
	}
	return true;
}

/**
 * ファイル/ディレクトリ1件の情報を得る（ワイルドカードを付加しない）
 * @param[in] lpPathName パス
 * @param[out] fli 検索結果
 * @retval SUCCESS 成功
 * @retval FAILURE 失敗
 */
BRESULT DOSIOCALL file_getinfo(const OEMCHAR* lpPathName, FLINFO* fli)
{
	WIN32_FIND_DATA w32fd;
	HANDLE hFile;
	bool result;

	hFile = ::FindFirstFile(lpPathName, &w32fd);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FAILURE;
	}
	result = setFLInfo(w32fd, fli);
	::FindClose(hFile);
	return result ? SUCCESS : FAILURE;
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

#if defined(DOSIO_HAS_DIRMONITOR)
FDIRMONH DOSIOCALL file_dirmonitor_open(const OEMCHAR* lpPathName)
{
	if (lpPathName == NULL || lpPathName[0] == '\0')
	{
		return FDIRMONH_INVALID;
	}
	return ::FindFirstChangeNotification(lpPathName, FALSE,
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
}

BOOL DOSIOCALL file_dirmonitor_changed(FDIRMONH hMonitor)
{
	DWORD dwWait;

	if (hMonitor == NULL || hMonitor == FDIRMONH_INVALID)
	{
		return TRUE;
	}
	dwWait = ::WaitForSingleObject(hMonitor, 0);
	return (dwWait == WAIT_TIMEOUT) ? FALSE : TRUE;
}

void DOSIOCALL file_dirmonitor_close(FDIRMONH hMonitor)
{
	if (hMonitor != NULL && hMonitor != FDIRMONH_INVALID)
	{
		::FindCloseChangeNotification(hMonitor);
	}
}
#endif



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
