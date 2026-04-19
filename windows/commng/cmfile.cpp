/**
 * @file	cmfile.cpp
 * @brief	ファイルダンプ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmfile.h"
#include "np2.h"

#include <process.h>

static unsigned int __stdcall cComFile_TimeoutThread(LPVOID vdParam)
{
    CComFile* t = (CComFile*)vdParam;

    while (WaitForSingleObject(t->m_hThreadExitEvent, 500) == WAIT_TIMEOUT) {
        EnterCriticalSection(&t->m_csPrint);
        if (GetTickCounter() - t->m_lastSendTime >= t->m_pageTimeout) {
            t->CCCloseFile();
            t->m_hThreadTimeout = NULL;
            LeaveCriticalSection(&t->m_csPrint);
            break;
        }
        LeaveCriticalSection(&t->m_csPrint);
    }
    return 0;
}


static BOOL DirectoryExists(const TCHAR* path)
{
	DWORD attr = GetFileAttributes(path);
	if (attr == INVALID_FILE_ATTRIBUTES) return FALSE;
	return (attr & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
}

static BOOL FileExists(const TCHAR* path)
{
	DWORD attr = GetFileAttributes(path);
	return (attr != INVALID_FILE_ATTRIBUTES);
}

// dirpath: 保存先ディレクトリ
// 戻り値: 成功時は有効なファイルハンドル、失敗時はINVALID_HANDLE_VALUE
HANDLE OpenUniqueDumpFile(const TCHAR* dirpath)
{
    if (!dirpath)
    {
        return INVALID_HANDLE_VALUE;
    }

    if (!DirectoryExists(dirpath))
    {
        return INVALID_HANDLE_VALUE;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);

    // ファイル名のベース部分をつくる
    TCHAR base[MAX_PATH] = { 0 };
    _stprintf(base, _T("PORTDUMP_%04u%02u%02u_%02u%02u%02u"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // dirpathの末尾に\が無ければ付ける
    TCHAR dirWithSep[MAX_PATH];
    _tcscpy(dirWithSep, dirpath);
    size_t len = _tcslen(dirWithSep);
    if (len > 0 && dirWithSep[len - 1] != _T('\\'))
    {
        dirWithSep[len] = '\\';
        dirWithSep[len + 1] = '\0';
        len++;
    }

    // 衝突しない名前を探す
    // suffix=0 -> PORTDUMP_yyyyMMdd_hhmmss.bin
    // suffix>0 -> PORTDUMP_...._n.bin
    for (DWORD suffix = 0; suffix < 100000; ++suffix)
    {
        TCHAR fullpath[MAX_PATH] = { 0 };

        if (suffix == 0)
        {
            _stprintf(fullpath, _T("%s%s.bin"), dirWithSep, base);
        }
        else
        {
            _stprintf(fullpath, _T("%s%s%lu.bin"), dirWithSep, base, suffix);
        }

        HANDLE hFile = CreateFile(
            fullpath,
            GENERIC_WRITE,
            0,                      // 共有なし
            NULL,
            CREATE_NEW,             // 既存なら失敗
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        DWORD lastError = GetLastError();
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // 新規作成成功
            return hFile;
        }

        // 既に存在する場合は次の番号へ
        if (lastError != ERROR_FILE_EXISTS && lastError != ERROR_ALREADY_EXISTS)
        {
            // それ以外のエラーは失敗
            return INVALID_HANDLE_VALUE;
        }
    }

    // 全然だめ
    return INVALID_HANDLE_VALUE;
}

/**
 * インスタンス作成
 * @param[in] dirpath 保存先ディレクトリ
 * @return インスタンス
 */
CComFile* CComFile::CreateInstance(LPCTSTR dirpath, int pageTimeout)
{
	CComFile* pFile = new CComFile;
	if (!pFile->Initialize(dirpath, pageTimeout))
	{
		delete pFile;
		pFile = NULL;
	}
	return pFile;
}

/**
 * コンストラクタ
 */
CComFile::CComFile()
	: CComBase(COMCONNECT_FILE)
	, m_dirpath()
	, m_hFile(INVALID_HANDLE_VALUE)
    , m_pageTimeout(5000)
    , m_lastSendTime(0)
    , m_hThreadTimeout(NULL)
    , m_hThreadExitEvent(NULL)
    , m_csPrint()
{
    InitializeCriticalSection(&m_csPrint);
    m_hThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

/**
 * デストラクタ
 */
CComFile::~CComFile()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
        CCEndThread();
        CCCloseFile();
	}
    CloseHandle(m_hThreadExitEvent);
    DeleteCriticalSection(&m_csPrint);
}

/**
 * 初期化
 * @param[in] dirpath 保存先ディレクトリ
 * @retval true 成功
 * @retval false 失敗
 */
bool CComFile::Initialize(LPCTSTR dirpath, int pageTimeout)
{
    m_pageTimeout = pageTimeout;

    DWORD ret = GetFullPathName(dirpath, NELEMENTS(m_dirpath), m_dirpath, NULL);
    if (ret == 0 || ret >= NELEMENTS(m_dirpath)) {
        return 0;
    }

	DWORD attr = GetFileAttributes(m_dirpath);
	if (attr == INVALID_FILE_ATTRIBUTES)
		return 0;

	return (attr & FILE_ATTRIBUTE_DIRECTORY);
}

/**
 * タイムアウト監視スレッド開始
 */
void CComFile::CCStartThread()
{
    EnterCriticalSection(&m_csPrint);

    // タイムアウト監視開始
    if (m_pageTimeout > 0 && !m_hThreadTimeout) {
        unsigned int dwID;
        m_hThreadTimeout = (HANDLE)_beginthreadex(NULL, 0, cComFile_TimeoutThread, this, 0, &dwID);
    }

    m_lastSendTime = GetTickCounter();
finalize:
    LeaveCriticalSection(&m_csPrint);
}
/**
 * タイムアウト監視スレッド終了
 */
void CComFile::CCEndThread()
{
    // タイムアウト監視終了
    if (m_hThreadTimeout) {
        SetEvent(m_hThreadExitEvent);
        if (WaitForSingleObject(m_hThreadTimeout, 10000) == WAIT_TIMEOUT)
        {
            TerminateThread(m_hThreadTimeout, 0); // ゾンビスレッド死すべし
        }
        CloseHandle(m_hThreadTimeout);
        m_hThreadTimeout = NULL;
    }
}
/**
 * ファイルを閉じる
 */
void CComFile::CCCloseFile()
{
    EnterCriticalSection(&m_csPrint);
    if (m_hFile) {
        ::CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
    LeaveCriticalSection(&m_csPrint);
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComFile::Read(UINT8* pData)
{
	return 0;
}

/**
 * 書き込み
 * @param[out] cData データ
 * @return サイズ
 */
UINT CComFile::Write(UINT8 cData)
{
    UINT ret = 0;
    DWORD dwWrittenSize;

    EnterCriticalSection(&m_csPrint);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
        // いきなりEOTは無視
        if (cData == 0x04) {
            ret = 1; // 成功扱い
            goto finalize;
        }

        m_hFile = OpenUniqueDumpFile(m_dirpath);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            goto finalize;
        }
        CCStartThread();
	}
    m_lastSendTime = GetTickCounter();
    ret = (::WriteFile(m_hFile, &cData, 1, &dwWrittenSize, NULL)) ? 1 : 0;
finalize:
    LeaveCriticalSection(&m_csPrint);
    return ret;
}

/**
 * ステータスを得る
 * bit 7: ~CI (RI, RING)
 * bit 6: ~CS (CTS)
 * bit 5: ~CD (DCD, RLSD)
 * bit 4: reserved
 * bit 3: reserved
 * bit 2: reserved
 * bit 1: reserved
 * bit 0: ~DSR (DR)
 * @return ステータス
 */
UINT8 CComFile::GetStat()
{
	return 0xa0;
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComFile::Message(UINT nMessage, INTPTR nParam)
{
    switch (nMessage)
    {
    case COMMSG_REOPEN:
            CCEndThread();

            EnterCriticalSection(&m_csPrint);

            CCCloseFile();

            if (nParam) {
                COMCFG* cfg = (COMCFG*)nParam;
                m_pageTimeout = cfg->fileTimeout;
                GetFullPathName(cfg->dirpath, NELEMENTS(m_dirpath), m_dirpath, NULL);
            }

            LeaveCriticalSection(&m_csPrint);
        break;

    default:
        break;
    }
    return 0;
}
