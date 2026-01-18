#include	"compiler.h"

#if defined(SUPPORT_HOSTDRVNT)

/*
	ゲストOS(WinNT系)からホストOS(Win)にアクセス
	HOSTDRVのWindowsNT対応バージョンです
*/

#include	<shlwapi.h>
#include	<process.h>

#include	"pccore.h"
#include	"iocore.h"
#include	"cpucore.h"
#if defined(SUPPORT_IA32_HAXM)
#include	"i386hax/haxfunc.h"
#include	"i386hax/haxcore.h"
#endif
#include	"dosio.h"

#include	"hostdrv.h"
#include	"hostdrvnt.h"
#include	"hostdrvntdef.h"

// 性能上最適化で優先しない方がいいコードなのでわざと別セグメントに置く
#pragma code_seg(".MISCCODE")
#if !defined(CPUCORE_IA32)
#define	cpu_kmemorywrite(a,v)	memp_write8(a,v)
#define	cpu_kmemorywrite_w(a,v)	memp_write16(a,v)
#define	cpu_kmemorywrite_d(a,v)	memp_write32(a,v)
#define	cpu_kmemoryread(a)		memp_read8(a)
#define	cpu_kmemoryread_w(a)	memp_read16(a)
#define	cpu_kmemoryread_d(a)	memp_read32(a)
#endif

#if 0
#undef	TRACEOUT
static void trace_fmt_ex(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "¥n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
static void trace_fmt_exw(const WCHAR* fmt, ...)
{
	WCHAR stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vswprintf(stmp, 2048, fmt, ap);
	wcscat(stmp, L"¥n");
	va_end(ap);
	OutputDebugStringW(stmp);
}
#define	TRACEOUTW(s)	trace_fmt_exw s
#else
#define	TRACEOUTW(s)	(void)0
#endif	/* 1 */



HOSTDRVNT hostdrvNT;

static WCHAR s_hdrvRoot[MAX_PATH] = { 0 };
static UINT8 s_hdrvAcc = 0;

// I/O待機用いろいろ
static UINT32 s_pendingListCount = 0;
static UINT32 s_pendingIrpListAddr = 0;
static UINT32 s_pendingAliveListAddr = 0;
static SINT32 s_pendingIndexOrCompleteCount = 0;

static HANDLE s_hThreadChangeFS = NULL;
static HANDLE s_hChangeFSStopEvent = NULL;
static int s_FSChanged = 0;

static int s_fsContextUserDataOffset = 0;

#define HOSTDRVNTOPTIONS_NONE				0x0
#define HOSTDRVNTOPTIONS_REMOVABLEDEVICE	0x1
#define HOSTDRVNTOPTIONS_USEREALCAPACITY	0x2
#define HOSTDRVNTOPTIONS_USECHECKNOTIFY		0x4
#define HOSTDRVNTOPTIONS_AUTOMOUNTDRIVE		0x8
#define HOSTDRVNTOPTIONS_DISKDEVICE			0x10

// FSCONTEXTで固有データが格納されている位置オフセット（ver4以降）
#define HOSTDRVNT_FSCONTEXT_USERDATA_OFFSET	40

// おぷしょん
static UINT32 s_hostdrvNTOptions = HOSTDRVNTOPTIONS_NONE;

static void hostdrvNT_notifyChange(WCHAR* changedHostFileName, UINT32 action, UINT32 forceRequestEnumDir);

// ---------- Host File System Monitor

static unsigned int __stdcall hostdrvNT_changeFSMonitorThread(LPVOID vdParam)
{
	HANDLE hChangeFSEvent = NULL;

	hChangeFSEvent = FindFirstChangeNotificationW(s_hdrvRoot, TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
	if (hChangeFSEvent != NULL && hChangeFSEvent != INVALID_HANDLE_VALUE)
	{
		HANDLE handles[] = { hChangeFSEvent, s_hChangeFSStopEvent };
		while (1)
		{
			DWORD dwWait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
			if (dwWait == WAIT_OBJECT_0)
			{
				s_FSChanged = 1;
				if (!FindNextChangeNotification(hChangeFSEvent))
				{
					break;
				}
			}
			else if (dwWait == WAIT_OBJECT_0 + 1)
			{
				break;
			}
		}

		FindCloseChangeNotification(hChangeFSEvent); // ファイルシステム監視停止
	}

	return 0;
}

void hostdrvNT_stopMonitorChangeFS()
{
	if (s_hChangeFSStopEvent != NULL && s_hChangeFSStopEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(s_hChangeFSStopEvent);
		if (WaitForSingleObject(s_hThreadChangeFS, 10000) == WAIT_TIMEOUT)
		{
			TerminateThread(s_hThreadChangeFS, 0); // ゾンビスレッド死すべし
		}
		CloseHandle(s_hChangeFSStopEvent); // 停止イベントを閉じる
		CloseHandle(s_hThreadChangeFS); // スレッドも閉じる

		s_hThreadChangeFS = NULL;
		s_hChangeFSStopEvent = NULL;
	}
}

void hostdrvNT_beginMonitorChangeFS()
{
	DWORD dwID = 0;

	hostdrvNT_stopMonitorChangeFS();

	if (s_hostdrvNTOptions & HOSTDRVNTOPTIONS_USECHECKNOTIFY)
	{
		s_hChangeFSStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		s_hThreadChangeFS = (HANDLE)_beginthreadex(NULL, 0, hostdrvNT_changeFSMonitorThread, NULL, 0, &dwID);
	}
}

void hostdrvNT_invokeMonitorChangeFS()
{
	if ((s_hostdrvNTOptions & HOSTDRVNTOPTIONS_USECHECKNOTIFY))
	{
		// 有効
		if (s_hChangeFSStopEvent == NULL || s_hChangeFSStopEvent == INVALID_HANDLE_VALUE)
		{
			hostdrvNT_beginMonitorChangeFS();
		}
		if (s_FSChanged)
		{
			s_FSChanged = 0;
			hostdrvNT_notifyChange(NULL, NP2_FILE_ACTION_ADDED, 1);
		}
	}
	else
	{
		// 無効
		if (s_hChangeFSStopEvent != NULL && s_hChangeFSStopEvent != INVALID_HANDLE_VALUE)
		{
			hostdrvNT_stopMonitorChangeFS();
		}
	}
}


// ---------- Utility Functions

/// <summary>
/// ホスト共有ドライブのルートパスを取得してUnicode文字列として記憶。最後の区切り文字（¥）がある場合は削除する。
/// </summary>
void hostdrvNT_updateHDrvRoot(void)
{
	int slen;

	// パス長さが制限オーバーならエラー
	if (_tcslen(np2cfg.hdrvroot) >= MAX_PATH)
	{
		s_hdrvRoot[0] = '¥0';
		s_hdrvAcc = 0;
		return;
	}

	// パス長さが制限オーバーならエラー
#ifdef UNICODE
	// 変換不要
	wcscpy(s_hdrvRoot, np2cfg.hdrvroot);
#else
	// Unicodeへ変換
	int lengthUnicode = MultiByteToWideChar(CP_ACP, 0, np2cfg.hdrvroot, strlen(np2cfg.hdrvroot) + 1, NULL, 0);
	if (lengthUnicode < 0 || lengthUnicode > MAX_PATH)
	{
		s_hdrvRoot[0] = '¥0';
		s_hdrvAcc = 0;
		return;
	}
	ZeroMemory(s_hdrvRoot, sizeof(s_hdrvRoot));
	MultiByteToWideChar(CP_UTF8, 0, np2cfg.hdrvroot, strlen(np2cfg.hdrvroot) + 1, s_hdrvRoot, lengthUnicode);
#endif

	// 最後の文字が¥なら除去
	slen = wcslen(s_hdrvRoot);
	if (slen > 0 && s_hdrvRoot[slen - 1] == '¥¥')
	{
		s_hdrvRoot[slen - 1] = '¥0';
	}
	s_hdrvAcc = np2cfg.hdrvacc;

	// モニター対象更新
	hostdrvNT_beginMonitorChangeFS();

	// 更新する
	s_FSChanged = 1;
}

static int hostdrvNT_getEmptyFile()
{
	int i;
	for (i = 1; i < NP2HOSTDRVNT_FILES_MAX; i++)
	{ // 0は使わないことにする
		if (!hostdrvNT.files[i].fileName) return i;
	}
	return -1;
}
static void hostdrvNT_preCloseFile(int index)
{
	if (hostdrvNT.files[index].hFile && hostdrvNT.files[index].hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hostdrvNT.files[index].hFile);
		hostdrvNT.files[index].hFile = NULL;
	}
}
static void hostdrvNT_closeFile(int index)
{
	if (hostdrvNT.files[index].hFindFile && hostdrvNT.files[index].hFindFile != INVALID_HANDLE_VALUE)
	{
		FindClose(hostdrvNT.files[index].hFindFile);
		hostdrvNT.files[index].hFindFile = NULL;
	}
	if (hostdrvNT.files[index].hFile && hostdrvNT.files[index].hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hostdrvNT.files[index].hFile);
		hostdrvNT.files[index].hFile = NULL;
	}
	if (hostdrvNT.files[index].deleteOnClose)
	{
		// 削除権限があれば削除
		if (s_hdrvAcc & HDFMODE_DELETE)
		{
			if (hostdrvNT.files[index].isDirectory)
			{
				RemoveDirectoryW(hostdrvNT.files[index].hostFileName);
			}
			else
			{
				DeleteFileW(hostdrvNT.files[index].hostFileName);
			}
			hostdrvNT_notifyChange(hostdrvNT.files[index].hostFileName, NP2_FILE_ACTION_REMOVED, 0);
		}
		hostdrvNT.files[index].deleteOnClose = 0;
	}
	if (hostdrvNT.files[index].fileName != NULL)
	{
		free(hostdrvNT.files[index].fileName);
		hostdrvNT.files[index].fileName = NULL;
	}
	if (hostdrvNT.files[index].hostFileName != NULL)
	{
		free(hostdrvNT.files[index].hostFileName);
		hostdrvNT.files[index].hostFileName = NULL;
	}
}
static void hostdrvNT_closeAllFiles()
{
	int i;
	// 0は使わないことにする
	for (i = 1; i < NP2HOSTDRVNT_FILES_MAX; i++)
	{
		hostdrvNT_closeFile(i);
	}
}
static int hostdrvNT_reopenFile(int index)
{
	HANDLE fh;
	NP2HOSTDRVNT_FILEINFO *fi;

	if (index < 0 || index >= NP2HOSTDRVNT_FILES_MAX) return 0;

	fi = &hostdrvNT.files[index];
	fh = fi->hFile;
	if (!fh || fh == INVALID_HANDLE_VALUE)
	{
		TRACEOUTW((L"REPOEN: %d %s", index, fi->hostFileName));
		if (fi->isDirectory)
		{
			// ディレクトリの場合の特例（実質的にディレクトリ日付変更専用）
			if ((fi->hFile = CreateFileW(fi->hostFileName, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL)) == INVALID_HANDLE_VALUE)
			{
				DWORD error = GetLastError();
				TRACEOUTW((L"OPEN FILE ERROR (code %d): %d %s", error, index, fi->hostFileName));
				fi->hFile = NULL;
				return 0;
			}
		}
		else
		{
			// ファイルの場合
			if ((fi->hFile = CreateFileW(fi->hostFileName, fi->hostdrvWinAPIDesiredAccess, fi->hostdrvShareAccess, NULL, fi->hostdrvWinAPICreateDisposition, fi->hostdrvFileAttributes, NULL)) == INVALID_HANDLE_VALUE)
			{
				DWORD error = GetLastError();
				TRACEOUTW((L"OPEN FILE ERROR (code %d): %d %s", error, index, fi->hostFileName));
				fi->hFile = NULL;
				return 0;
			}
		}
	}
	return 1;
}

static int hostdrvNT_getHostPath(WCHAR* virPath, WCHAR* hostPath, UINT8* isRoot, int getTargetDir)
{
	WCHAR hdrvPath[MAX_PATH];
	WCHAR pathTmp[MAX_PATH];
	UINT32 hdrvPathLen = 0;

	wcscpy(hdrvPath, s_hdrvRoot);
	hdrvPathLen = wcslen(hdrvPath);

	// ホストのパスと結合
	if (virPath[0] == '¥¥') virPath++;
	hostPath[0] = '¥0';
	if (!PathCombineW(pathTmp, hdrvPath, virPath))
	{
		return 1;
	}
	// .と..を消す
	if (!PathCanonicalizeW(hostPath, pathTmp))
	{
		return 1;
	}

	// ホストのパス部分が無くなっていたら拒否
	if (wcsncmp(hdrvPath, hostPath, hdrvPathLen) != 0) return 1;

	// ルートディレクトリ判定
	if (wcslen(hostPath) > hdrvPathLen + 1)
	{
		UINT32 vlen = wcslen(hostPath + hdrvPathLen + 1);
		*isRoot = (vlen == 0 || (vlen == 1 && *(hostPath + hdrvPathLen + 1) == '¥¥'));
	}
	else
	{
		*isRoot = 1;
	}

	// 必要ならファイル名からそのファイルのディレクトリパスへ変換
	if (getTargetDir)
	{
		if (*isRoot)
		{
			// ルートならホストのパスと同じ
			wcscpy(hostPath, hdrvPath);
		}
		else
		{
			// 最後の区切り文字以降を削除
			WCHAR* sepaPos = wcsrchr(hostPath, '¥¥');
			if (sepaPos != NULL)
			{
				*sepaPos = '¥0';
			}
		}
	}

	return 0;
}

static int hostdrvNT_hasInvalidWildcard(WCHAR* name)
{
	int i;
	int hasWildcard = 0;
	for (i = 0; name[i] != '¥0'; i++)
	{
		WCHAR c = name[i];
		if (!hasWildcard)
		{
			if ((c == L'*' || c == L'?'))
			{
				hasWildcard = 1;
			}
		}
		else
		{
			if ((c == L'¥¥'))
			{
				return 1;
			}
		}
	}
	return 0;
}

static void hostdrvNT_memread(UINT32 vaddr, void* buffer, UINT32 size)
{
	UINT32 readaddr = vaddr;
	UINT32 readsize = size;
	UINT8* readptr = (UINT8*)buffer;
	while (readsize >= 4)
	{
		*((UINT32*)readptr) = cpu_kmemoryread_d(readaddr);
		readsize -= 4;
		readptr += 4;
		readaddr += 4;
	}
	while (readsize > 0)
	{
		*readptr = cpu_kmemoryread(readaddr);
		readsize--;
		readptr++;
		readaddr++;
	}
}
static void hostdrvNT_memwrite(UINT32 vaddr, void* buffer, UINT32 size)
{
	UINT32 writeaddr = vaddr;
	UINT32 writesize = size;
	UINT8* writeptr = (UINT8*)buffer;
	while (writesize >= 4)
	{
		cpu_kmemorywrite_d(writeaddr, *((UINT32*)writeptr));
		writesize -= 4;
		writeptr += 4;
		writeaddr += 4;
	}
	while (writesize > 0)
	{
		cpu_kmemorywrite(writeaddr, *writeptr);
		writesize--;
		writeptr++;
		writeaddr++;
	}
}

static void hostdrvNT_readFileObject(HOSTDRVNT_INVOKEINFO* invokeInfo, NP2_FILE_OBJECT *lpFileObject)
{
	hostdrvNT_memread(invokeInfo->stack.fileObject, lpFileObject, sizeof(NP2_FILE_OBJECT));
}
static WCHAR* hostdrvNT_readUnicodeString(UINT32 vaddr, UINT32 length)
{
	WCHAR* unicodeString = (WCHAR*)malloc(length + sizeof(WCHAR));
	if (!unicodeString) return NULL;
	ZeroMemory(unicodeString, length + sizeof(WCHAR));
	hostdrvNT_memread(vaddr, unicodeString, length);
	return unicodeString;
}
static int hostdrvNT_writeQueryInformationData(HOSTDRVNT_INVOKEINFO* invokeInfo, void* srcBuffer, UINT32 srcLength, int allowOverflow)
{
	UINT32 length = invokeInfo->stack.parameters.queryFile.Length; // どのQueryInformationも最初のデータがバッファ長さなのでこれでよい
	if (length < srcLength)
	{
		if (!allowOverflow)
		{
			return 0;
		}
		srcLength = length;
	}

	// 書き込み
	hostdrvNT_memwrite(invokeInfo->outBufferAddr, srcBuffer, srcLength);

	return srcLength;
}
static void hostdrvNT_setQueryInformationResult(HOSTDRVNT_INVOKEINFO* invokeInfo, void* returnData, UINT32 dataLen, int allowOverflow)
{
	if (returnData)
	{
		UINT32 writeLen = 0;
		writeLen = hostdrvNT_writeQueryInformationData(invokeInfo, returnData, dataLen, allowOverflow);
		if (writeLen < dataLen)
		{
			if (allowOverflow)
			{
				TRACEOUTW((L"OVERFLOW"));
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_OVERFLOW);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, writeLen); // Information
			}
			else
			{
				TRACEOUTW((L"BUFFER_TOO_SMALL"));
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			}
		}
		else
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, writeLen); // Information
		}
	}
	else
	{
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
	}
}
static int hostdrvNT_readSetInformationData(HOSTDRVNT_INVOKEINFO* invokeInfo, void* dstBuffer, UINT32 dstLength)
{
	UINT32 length = invokeInfo->stack.parameters.queryFile.Length;
	if (length < dstLength)
	{
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return 0;
	}

	// 読み込み
	hostdrvNT_memread(invokeInfo->inBufferAddr, dstBuffer, dstLength);

	return dstLength;
}

static NP2HOSTDRVNT_FILEINFO* hostdrvNT_getFileInfo(NP2_FILE_OBJECT* fileObject)
{
	UINT32 fsContextFileIndex = cpu_kmemoryread_d(fileObject->FsContext + s_fsContextUserDataOffset);
	if (fsContextFileIndex == 0 || NP2HOSTDRVNT_FILES_MAX <= fsContextFileIndex || hostdrvNT.files[fsContextFileIndex].fileName == NULL)
	{
		return NULL;
	}
	TRACEOUTW((L"FILE #%d", fsContextFileIndex));
	return &hostdrvNT.files[fsContextFileIndex];
}

/// <summary>
/// IRP_MN_NOTIFY_CHANGE_DIRECTORYで監視中の変更を処理する
/// </summary>
/// <param name="changedHostFileName">変更されたファイル名。仮想パスではなくホストのファイル名で指定。NULLにすると無条件で更新通知。</param>
/// <param name="action">通知するアクション NP2_FILE_ACTION_〜を指定</param>
/// <param name="forceRequestEnumDir">強制更新させる場合はtrue。XXX: 1個で通知できない場合はこれを使う。</param>
static void hostdrvNT_notifyChange(WCHAR* changedHostFileName, UINT32 action, UINT32 forceRequestEnumDir)
{
	WCHAR changedHostDir[MAX_PATH]; // 変更されたファイルのあるディレクトリ名
	WCHAR changedHostFileNameTmp[MAX_PATH];
	NP2HOSTDRVNT_FILEINFO* fi;
	int i;
	WCHAR hdrvPath[MAX_PATH];
	int forceMatch = changedHostFileName == NULL;

	wcscpy(hdrvPath, s_hdrvRoot);

	for (i = 0; i < s_pendingListCount; i++)
	{
		UINT32 irpListAddr = s_pendingIrpListAddr + i * sizeof(UINT32);
		UINT32 fileIdxListAddr = s_pendingAliveListAddr + i * sizeof(UINT32);
		UINT32 irpAddr = cpu_kmemoryread_d(irpListAddr);
		UINT32 fileIdx = cpu_kmemoryread_d(fileIdxListAddr);
		if (irpAddr != 0 && fileIdx != 0)
		{
			DWORD attr; // ファイル属性処理用テンポラリ
			UINT32 irpStatusAddr = irpAddr + 4 + 4 + 4 + 4 + 4 + 4; // IRP構造体のStatusが書き込まれている位置のアドレス
			UINT32 irpInfoAddr = irpStatusAddr + 4; // IRP構造体のInformationが書き込まれている位置のアドレス
			UINT32 irpOutBufferAddr; // SystemBufferのアドレス
			UINT32 length; // SystemBufferの長さ
			UINT32 completionFilter = 0; // 監視対象の条件のフィルタ
			UINT32 irpstackFlags = 0; // IRPスタックのFlags
			UINT32 watchTree = 0; // 下にあるディレクトリツリー全体を監視
			UINT32 match = 0; // 今回の変更が条件にマッチしているかどうか

			// ファイルオブジェクトを取得
			fi = &hostdrvNT.files[fileIdx];
			if (fi->fileName == NULL)
			{
				// ファイルが閉じていたらもう監視できないのでエラー
				cpu_kmemorywrite_d(irpStatusAddr, NP2_STATUS_INVALID_PARAMETER);
				cpu_kmemorywrite_d(irpInfoAddr, 0); // Information
				cpu_kmemorywrite_d(fileIdxListAddr, 0); // イベント発生要求
				if (s_pendingIndexOrCompleteCount < 0) s_pendingIndexOrCompleteCount = 0;
				s_pendingIndexOrCompleteCount++;
				continue;
			}

			// 書き込み先メモリアドレスを取得
			irpOutBufferAddr = cpu_kmemoryread_d(irpAddr + 4 + 4 + 4); // IRP構造体のSystemBufferが書き込まれている位置
			if (irpOutBufferAddr == NULL)
			{
				// NULLはおかしいのでエラー
				cpu_kmemorywrite_d(irpStatusAddr, NP2_STATUS_INVALID_PARAMETER);
				cpu_kmemorywrite_d(irpInfoAddr, 0); // Information
				cpu_kmemorywrite_d(fileIdxListAddr, 0); // イベント発生要求
				if (s_pendingIndexOrCompleteCount < 0) s_pendingIndexOrCompleteCount = 0;
				s_pendingIndexOrCompleteCount++;
				continue;
			}

			// 書き込み先サイズを取得　XXX:記録する場所がないのでSystemBufferの最初の部分を借りている
			length = cpu_kmemoryread_d(irpOutBufferAddr);

			// フィルタ条件を取得　XXX:記録する場所がないのでSystemBufferの最初の部分を借りている
			completionFilter = cpu_kmemoryread_d(irpOutBufferAddr + 4);
			if (!((completionFilter & (NP2_FILE_NOTIFY_CHANGE_FILE_NAME | NP2_FILE_NOTIFY_CHANGE_DIR_NAME)) && (action == NP2_FILE_ACTION_ADDED || action == NP2_FILE_ACTION_REMOVED || action == NP2_FILE_ACTION_MODIFIED || action == NP2_FILE_ACTION_REMOVED_BY_DELETE || action == NP2_FILE_ACTION_RENAMED_OLD_NAME || action == NP2_FILE_ACTION_RENAMED_NEW_NAME)) &&
				!((completionFilter & NP2_FILE_NOTIFY_CHANGE_ATTRIBUTES) && (action == NP2_FILE_ACTION_MODIFIED)) &&
				!((completionFilter & NP2_FILE_NOTIFY_CHANGE_SIZE) && (action == NP2_FILE_ACTION_MODIFIED)) &&
				!((completionFilter & NP2_FILE_NOTIFY_CHANGE_LAST_WRITE) && (action == NP2_FILE_ACTION_MODIFIED)) &&
				!((completionFilter & NP2_FILE_NOTIFY_CHANGE_LAST_ACCESS) && (action == NP2_FILE_ACTION_MODIFIED)) &&
				!((completionFilter & NP2_FILE_NOTIFY_CHANGE_CREATION) && (action == NP2_FILE_ACTION_MODIFIED)) &&
				!((completionFilter & NP2_FILE_NOTIFY_CHANGE_SECURITY) && (action == NP2_FILE_ACTION_MODIFIED)))
			{
				// 条件に合わないのでスキップ
				continue;
			}

			// フラグを取得　XXX:記録する場所がないのでSystemBufferの最初の部分を借りている
			irpstackFlags = cpu_kmemoryread(irpOutBufferAddr + 8);
			watchTree = !!(irpstackFlags & NP2_SL_WATCH_TREE);

			if (forceMatch)
			{
				// ホストファイルパスがNULLなら無条件通知とする
				wcscpy(changedHostFileNameTmp, fi->hostFileName);
				changedHostFileName = changedHostFileNameTmp;
				match = 1;
			}
			else
			{
				// ファイル名を除いてディレクトリ部分を取得
				wcscpy(changedHostDir, changedHostFileName);
				attr = GetFileAttributesW(changedHostDir);
				if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
				{
					// ファイルなら最後のドライブ区切り文字以降をカット
					WCHAR* hostSepa;
					if (hostSepa = wcsrchr(changedHostDir, '¥¥'))
					{
						*hostSepa = '¥0';
					}
				}
				else
				{
					// ディレクトリなら最後の文字が¥の時カット
					UINT32 changedHostDirLen = wcslen(changedHostDir);
					if (changedHostDirLen >= 1 && changedHostDir[changedHostDirLen - 1] == '¥¥')
					{
						changedHostDir[changedHostDirLen - 1] = '¥0';
					}
				}

				// パスを比較して必要に応じて変更通知
				if (wcscmp(fi->hostFileName, changedHostDir) == 0)
				{
					// 完全一致パターン
					match = 1;
				}
				else if (watchTree)
				{
					// 下の階層まで見るモード
					UINT32 path1Len = 0;
					UINT32 path2Len = 0;
					path1Len = wcslen(fi->hostFileName);
					path2Len = wcslen(changedHostDir);
					if (path1Len < path2Len && wcsncmp(fi->hostFileName, changedHostDir, path1Len) == 0)
					{
						// 下層一致パターン
						match = 1;
					}
				}
			}

			// 条件に一致した
			if (match)
			{
				WCHAR* fileNamePart;
				NP2_FILE_NOTIFY_INFORMATION info = { 0 };

				// 監視しているディレクトリからの相対パスに変換
				if (wcslen(changedHostFileName) > wcslen(fi->hostFileName) + 1)
				{
					fileNamePart = changedHostFileName + wcslen(fi->hostFileName) + 1;
				}
				else
				{
					fileNamePart = changedHostFileName + wcslen(fi->hostFileName);
				}

				// 変更内容をセット
				info.Action = action;

				// ファイル名をセット
				info.FileNameLength = wcslen(fileNamePart) * sizeof(WCHAR);
				wcscpy(info.FileName, fileNamePart);

				// XXX: 次エントリは使わないことにする
				info.NextEntryOffset = 0;

				// 書き込み
				if (sizeof(info) <= length && !forceRequestEnumDir)
				{
					length = sizeof(info);
					cpu_kmemorywrite_d(irpStatusAddr, NP2_STATUS_SUCCESS);
				}
				else
				{
					cpu_kmemorywrite_d(irpStatusAddr, NP2_STATUS_NOTIFY_ENUM_DIR);
				}
				hostdrvNT_memwrite(irpOutBufferAddr, &info, length);

				TRACEOUTW((L"FILE CHANGED: %s", fileNamePart));
				cpu_kmemorywrite_d(irpInfoAddr, length); // Information（書き込んだデータサイズ）
				cpu_kmemorywrite_d(fileIdxListAddr, 0); // イベント発生要求
				if (s_pendingIndexOrCompleteCount < 0) s_pendingIndexOrCompleteCount = 0;
				s_pendingIndexOrCompleteCount++;
			}
		}
	}
}

static int hostdrvNT_getOneEntry(NP2HOSTDRVNT_FILEINFO* fi, NP2_FILE_BOTH_DIR_INFORMATION* dirInfo, WCHAR* pattern)
{
	WIN32_FIND_DATA findFileData;
	UINT32 bytesReturned;

	if (fi->hFindFile == NULL || fi->hFindFile == INVALID_HANDLE_VALUE)
	{
		WCHAR defaultPattern[] = L"*";
		WCHAR findPath[MAX_PATH * 2];
		UINT32 findPathLen = 0;
		if (pattern == NULL)
		{
			pattern = defaultPattern;
		}
		if (wcslen(fi->hostFileName) >= MAX_PATH || wcslen(pattern) >= MAX_PATH)
		{
			return 0;
		}
		wcscpy(findPath, fi->hostFileName);
		findPathLen = wcslen(findPath);
		if (fi->isDirectory)
		{
			if (findPath[findPathLen - 1] != '¥¥')
			{
				wcscat(findPath, L"¥¥");
			}
			wcscat(findPath, pattern);
		}
		fi->hFindFile = FindFirstFile(findPath, &findFileData);
		if (fi->hFindFile == INVALID_HANDLE_VALUE)
		{
			fi->hFindFile = NULL;
			return 0;
		}
	}
	else
	{
		if (!FindNextFile(fi->hFindFile, &findFileData))
		{
			return 0;
		}
	}
	// 長すぎるファイル名は拒否。ルートなら.と..は列挙除外
	while (1)
	{
		if ((!fi->isRoot || wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) && wcslen(findFileData.cFileName) < MAX_PATH)
		{
			break;
		}
		if (!FindNextFile(fi->hFindFile, &findFileData))
		{
			return 0;
		}
	}

	bytesReturned = sizeof(NP2_FILE_BOTH_DIR_INFORMATION);
	if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		// ディレクトリ
		//WCHAR longPath[MAX_PATH];
		//WCHAR shortPath[MAX_PATH];
		dirInfo->FileNameLength = wcslen(findFileData.cFileName) * sizeof(WCHAR);
		wcscpy(dirInfo->FileName, findFileData.cFileName);
		dirInfo->FileAttributes = findFileData.dwFileAttributes;
		dirInfo->CreationTime = *((UINT64*)&findFileData.ftCreationTime);
		dirInfo->LastAccessTime = *((UINT64*)&findFileData.ftLastAccessTime);
		dirInfo->LastWriteTime = *((UINT64*)&findFileData.ftLastWriteTime);
		dirInfo->ChangeTime = *((UINT64*)&findFileData.ftLastWriteTime);

		//PathCombineW(longPath, fi->hostFileName, findFileData.cFileName);
		//if (GetShortPathNameW(longPath, shortPath, MAX_PATH))
		//{
		//	WCHAR *shortFileName;
		//	UINT32 shortLen;
		//	// 最後の区切り文字以降を採用
		//	shortFileName = wcsrchr(shortPath, '¥¥');
		//	if (shortFileName == NULL)
		//	{
		//		shortFileName = shortPath;
		//	}
		//	else
		//	{
		//		shortFileName++; // ¥は消す
		//	}
		//	shortLen = wcslen(shortFileName);
		//	if (shortLen <= sizeof(dirInfo->ShortName) / sizeof(WCHAR))
		//	{
		//		memcpy(dirInfo->ShortName, shortFileName, shortLen * sizeof(WCHAR));
		//		dirInfo->ShortNameLength = shortLen * sizeof(WCHAR);
		//	}
		//}
	}
	else
	{
		// ファイル
		//WCHAR longPath[MAX_PATH];
		//WCHAR shortPath[MAX_PATH];
		dirInfo->CreationTime = *((UINT64*)&findFileData.ftCreationTime);
		dirInfo->LastAccessTime = *((UINT64*)&findFileData.ftLastAccessTime);
		dirInfo->LastWriteTime = *((UINT64*)&findFileData.ftLastWriteTime);
		dirInfo->ChangeTime = *((UINT64*)&findFileData.ftLastWriteTime);
		dirInfo->EndOfFile = ((UINT64)findFileData.nFileSizeHigh << 32) | findFileData.nFileSizeLow;
		dirInfo->AllocationSize = dirInfo->EndOfFile;
		dirInfo->FileAttributes = findFileData.dwFileAttributes;
		dirInfo->FileNameLength = wcslen(findFileData.cFileName) * sizeof(WCHAR);
		wcscpy(dirInfo->FileName, findFileData.cFileName);

		//PathCombineW(longPath, fi->hostFileName, findFileData.cFileName);
		//if (GetShortPathNameW(longPath, shortPath, MAX_PATH))
		//{
		//	WCHAR* shortFileName;
		//	UINT32 shortLen;
		//	// 最後の区切り文字以降を採用
		//	shortFileName = wcsrchr(shortPath, '¥¥');
		//	if (shortFileName == NULL)
		//	{
		//		shortFileName = shortPath;
		//	}
		//	else
		//	{
		//		shortFileName++; // ¥は消す
		//	}
		//	shortLen = wcslen(shortFileName);
		//	if (shortLen <= sizeof(dirInfo->ShortName) / sizeof(WCHAR))
		//	{
		//		memcpy(dirInfo->ShortName, shortFileName, shortLen * sizeof(WCHAR));
		//		dirInfo->ShortNameLength = shortLen * sizeof(WCHAR);
		//	}
		//}
	}
	TRACEOUTW((L"FIND: %s", findFileData.cFileName));

	return bytesReturned;
}

int hostdrvNT_dirHasFiles(LPCWSTR hostPath)
{
	int hasFile = 0;
	WCHAR searchPath[MAX_PATH];
	WIN32_FIND_DATAW findData;
	HANDLE hFind;

	if (!PathCombineW(searchPath, hostPath, L"*"))
	{
		return 1;
	}

	hFind = FindFirstFileW(searchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	do
	{
		if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0)
		{
			hasFile = 1;
			break;
		}
	} while (FindNextFileW(hFind, &findData));

	FindClose(hFind);

	return hasFile;
}

// ---------- Major Functions

static void hostdrvNT_IRP_MJ_CREATE(HOSTDRVNT_INVOKEINFO *invokeInfo)
{
	int fileIndex;
	UINT32 returnInformation = 0;

	UINT32 hostdrvDesiredAccess = 0;
	UINT32 hostdrvOptions = 0;
	UINT32 hostdrvFileAttributes = 0;
	UINT32 hostdrvShareAccess = 0;
	UINT32 hostdrvEALength = 0;
	UINT32 hostdrvDirectoryFile;
	UINT32 hostdrvNonDirectoryFile;
	UINT32 hostdrvSequentialOnly;
	UINT32 hostdrvNoIntermediateBuffering;
	UINT32 hostdrvNoEaKnowledge;
	UINT32 hostdrvDeleteOnClose;

	UINT32 hostdrvTemporaryFile;
	UINT8 hostdrvCreateDisposition;
	UINT32 hostdrvIsPagingFile;
	UINT32 hostdrvOpenTargetDirectory;
	UINT32 hostdrvCreateFileOrDir;
	UINT32 hostdrvOpenFileOrDir;
	UINT32 hostdrvCreateDirectory;
	UINT32 hostdrvOpenDirectory;
	UINT32 hostdrvCreateFile;
	UINT32 hostdrvOpenFile;
	UINT32 hostdrvWinAPICreateDisposition;
	UINT32 hostdrvWinAPIDesiredAccess;

	WCHAR* fileName = NULL; // 開こうとしているファイル名
	NP2_FILE_OBJECT fileObject = { 0 }; // ファイルオブジェクト（I/OマネージャがCREATE呼び出し単位でメモリ自動割り当てする）
	UINT32 fsContextFileIndex; // ホスト側のファイル管理番号

	if (!invokeInfo->stack.fileObject)
	{
		TRACEOUTW((L"ERROR: FileObject is null"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	// ファイルオブジェクトとFsContextのファイル管理番号を取得
	hostdrvNT_readFileObject(invokeInfo, &fileObject);
	if (!fileObject.FsContext)
	{
		TRACEOUTW((L"ERROR: FsContext is null"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}
	fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);

	// 対象ファイル名を取得
	fileName = hostdrvNT_readUnicodeString(fileObject.FileName.Buffer, fileObject.FileName.Length);
	if (!fileName)
	{
		TRACEOUTW((L"ERROR: read FileName Failed"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	// 指定した場所からの相対指定の場合
	if (fileObject.RelatedFileObject != NULL)
	{
		WCHAR* dirName;
		UINT32 dirNameLen;
		NP2_FILE_OBJECT relFileObject = { 0 }; // パスの基準にするファイルオブジェクト
		hostdrvNT_memread(fileObject.RelatedFileObject, &relFileObject, sizeof(NP2_FILE_OBJECT));
		dirName = hostdrvNT_readUnicodeString(relFileObject.FileName.Buffer, relFileObject.FileName.Length);
		if (!dirName)
		{
			TRACEOUTW((L"ERROR: read dirName Failed"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		dirNameLen = wcslen(dirName);
		if (dirNameLen > 0)
		{
			WCHAR *pathTmp;
			UINT32 combineLen;
			if (dirName[dirNameLen - 1] == '¥¥')
			{
				dirName[dirNameLen - 1] = '¥0';
			}
			combineLen = wcslen(dirName) + 1 + wcslen(fileName);
			if (combineLen >= MAX_PATH)
			{
				TRACEOUTW((L"ERROR: too long path"));
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			pathTmp = (WCHAR*)malloc((combineLen + 1) * sizeof(WCHAR));
			ZeroMemory(pathTmp, (combineLen + 1) * sizeof(WCHAR));
			if (!pathTmp)
			{
				TRACEOUTW((L"ERROR: read dirName alloc Failed"));
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			wcscpy(pathTmp, dirName);
			wcscat(pathTmp, L"¥¥");
			wcscat(pathTmp, fileName);
			free(fileName);
			fileName = pathTmp; // ディレクトリ付きに入れ替え
		}
		free(dirName);
	}

	// フラグ系を取得
	if (invokeInfo->stack.parameters.create.securityContext)
	{
		hostdrvDesiredAccess = cpu_kmemoryread_d(invokeInfo->stack.parameters.create.securityContext + 4 * 2); // DesiredAccessはポインタの先の3番目の変数
	}
	hostdrvOptions = invokeInfo->stack.parameters.create.options;
	hostdrvFileAttributes = (UCHAR)(invokeInfo->stack.parameters.create.fileAttributes & ‾FILE_ATTRIBUTE_NORMAL);
	hostdrvShareAccess = invokeInfo->stack.parameters.create.shareAccess;
	hostdrvEALength = invokeInfo->stack.parameters.create.eaLength;

	// フラグを制限
	hostdrvFileAttributes &= (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);

	// ビットフラグ系を判定しておく
	hostdrvDirectoryFile = !!(hostdrvOptions & NP2_FILE_DIRECTORY_FILE);
	hostdrvNonDirectoryFile = !!(hostdrvOptions & NP2_FILE_NON_DIRECTORY_FILE);
	hostdrvSequentialOnly = !!(hostdrvOptions & NP2_FILE_SEQUENTIAL_ONLY);
	hostdrvNoIntermediateBuffering = !!(hostdrvOptions & NP2_FILE_NO_INTERMEDIATE_BUFFERING);
	hostdrvNoEaKnowledge = !!(hostdrvOptions & NP2_FILE_NO_EA_KNOWLEDGE);
	hostdrvDeleteOnClose = !!(hostdrvOptions & NP2_FILE_DELETE_ON_CLOSE);

	hostdrvTemporaryFile = !!(invokeInfo->stack.parameters.create.fileAttributes & FILE_ATTRIBUTE_TEMPORARY);
	hostdrvCreateDisposition = (UINT8)((hostdrvOptions >> 24) & 0xff); // ファイルオープンモード 新規作成など
	hostdrvIsPagingFile = !!(invokeInfo->stack.flags & NP2_SL_OPEN_PAGING_FILE);
	hostdrvOpenTargetDirectory = !!(invokeInfo->stack.flags & NP2_SL_OPEN_TARGET_DIRECTORY);

	// CreateDispositionをユーザーモードWinAPIのものに変換
	hostdrvWinAPICreateDisposition = 0;
	switch (hostdrvCreateDisposition)
	{
	case NP2_FILE_SUPERSEDE:
		hostdrvWinAPICreateDisposition = CREATE_ALWAYS;
		TRACEOUTW((L"MODE FILE_SUPERSEDE"));
		returnInformation = NP2_FILE_SUPERSEDED;
		break;
	case NP2_FILE_OPEN:
		hostdrvWinAPICreateDisposition = OPEN_EXISTING;
		TRACEOUTW((L"MODE FILE_OPEN"));
		returnInformation = NP2_FILE_OPENED;
		break;
	case NP2_FILE_CREATE:
		hostdrvWinAPICreateDisposition = CREATE_NEW;
		TRACEOUTW((L"MODE FILE_CREATE"));
		returnInformation = NP2_FILE_CREATED;
		break;
	case NP2_FILE_OPEN_IF:
		hostdrvWinAPICreateDisposition = OPEN_ALWAYS;
		TRACEOUTW((L"MODE FILE_OPEN_IF"));
		returnInformation = NP2_FILE_OPENED;
		break;
	case NP2_FILE_OVERWRITE:
		hostdrvWinAPICreateDisposition = TRUNCATE_EXISTING;
		TRACEOUTW((L"MODE FILE_OVERWRITE"));
		returnInformation = NP2_FILE_OVERWRITTEN;
		break;
	case NP2_FILE_OVERWRITE_IF:
		hostdrvWinAPICreateDisposition = CREATE_ALWAYS;
		TRACEOUTW((L"MODE FILE_OVERWRITE_IF"));
		returnInformation = NP2_FILE_OVERWRITTEN;
		break;
	}

	// DesiredAccessをユーザーモードWinAPIのものに変換
	hostdrvWinAPIDesiredAccess = 0;
	if ((hostdrvDesiredAccess & FILE_ALL_ACCESS) == FILE_ALL_ACCESS) hostdrvWinAPIDesiredAccess |= GENERIC_READ | GENERIC_WRITE | DELETE;
	if (hostdrvDesiredAccess & FILE_ADD_FILE) hostdrvWinAPIDesiredAccess |= GENERIC_READ;
	if (hostdrvDesiredAccess & FILE_ADD_SUBDIRECTORY) hostdrvWinAPIDesiredAccess |= GENERIC_READ;
	if (hostdrvDesiredAccess & FILE_APPEND_DATA) hostdrvWinAPIDesiredAccess |= GENERIC_READ | GENERIC_WRITE;
	if (hostdrvDesiredAccess & FILE_DELETE_CHILD) hostdrvWinAPIDesiredAccess |= GENERIC_READ | GENERIC_WRITE;
	if (hostdrvWinAPIDesiredAccess == 0)
	{
		if (hostdrvDesiredAccess & FILE_EXECUTE) hostdrvWinAPIDesiredAccess |= GENERIC_EXECUTE;
		if (hostdrvDesiredAccess & FILE_READ_DATA) hostdrvWinAPIDesiredAccess |= GENERIC_READ;
		if (hostdrvDesiredAccess & FILE_LIST_DIRECTORY) hostdrvWinAPIDesiredAccess |= GENERIC_READ;
		if (hostdrvDesiredAccess & FILE_READ_ATTRIBUTES) hostdrvWinAPIDesiredAccess |= GENERIC_READ;
		if (hostdrvDesiredAccess & FILE_READ_EA) hostdrvWinAPIDesiredAccess |= GENERIC_READ;
		if (hostdrvDesiredAccess & FILE_TRAVERSE) hostdrvWinAPIDesiredAccess |= GENERIC_READ;
		if (hostdrvDesiredAccess & READ_CONTROL) hostdrvWinAPIDesiredAccess |= GENERIC_READ;
		//if (hostdrvDesiredAccess & FILE_WRITE_ATTRIBUTES) hostdrvWinAPIDesiredAccess |= GENERIC_WRITE; // 属性変更は書き込み権限要らない
		if (hostdrvDesiredAccess & FILE_WRITE_EA) hostdrvWinAPIDesiredAccess |= GENERIC_WRITE;
		if (hostdrvDesiredAccess & FILE_WRITE_DATA) hostdrvWinAPIDesiredAccess |= GENERIC_WRITE;
	}
	if (hostdrvDesiredAccess & DELETE) hostdrvWinAPIDesiredAccess |= DELETE;

	// リードオンリーならCeateやWriteフラグはエラー
	if (!(s_hdrvAcc & HDFMODE_WRITE) && 
		((hostdrvWinAPIDesiredAccess & GENERIC_WRITE) || hostdrvCreateDisposition != NP2_FILE_OPEN))
	{
		TRACEOUTW((L"ERROR: HOSTDRV is readonly mode."));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_MEDIA_WRITE_PROTECTED);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		free(fileName);
		return; // パスが変
	}


	// 区分をあらかじめ計算
	hostdrvCreateFileOrDir = (hostdrvCreateDisposition == NP2_FILE_CREATE) || (hostdrvCreateDisposition == NP2_FILE_OPEN_IF);
	hostdrvOpenFileOrDir = (hostdrvCreateDisposition == NP2_FILE_OPEN) || (hostdrvCreateDisposition == NP2_FILE_OPEN_IF);
	hostdrvCreateDirectory = (hostdrvDirectoryFile && ((hostdrvCreateDisposition == NP2_FILE_CREATE) || (hostdrvCreateDisposition == NP2_FILE_OPEN_IF)));
	hostdrvOpenDirectory = (hostdrvDirectoryFile && ((hostdrvCreateDisposition == NP2_FILE_OPEN) || (hostdrvCreateDisposition == NP2_FILE_OPEN_IF)));
	hostdrvCreateFile = (hostdrvNonDirectoryFile && ((hostdrvCreateDisposition == NP2_FILE_CREATE) || (hostdrvCreateDisposition == NP2_FILE_OPEN_IF)));
	hostdrvOpenFile = (hostdrvNonDirectoryFile && ((hostdrvCreateDisposition == NP2_FILE_OPEN) || (hostdrvCreateDisposition == NP2_FILE_OPEN_IF)));

	// ホスト側のファイル管理番号の空きを取得
	fileIndex = hostdrvNT_getEmptyFile();
	if (fileIndex >= 0)
	{
		NP2HOSTDRVNT_FILEINFO *fi = &hostdrvNT.files[fileIndex];
		WCHAR hostPath[MAX_PATH] = { 0 }; // パス領域確保
		UINT8 isRoot;
		UINT32 hostPathLength;
		DWORD attrs;

		// パスに無効な文字が含まれる場合はSTATUS_OBJECT_NAME_INVALID　ここでSTATUS_OBJECT_NAME_NOT_FOUNDを返すとワイルドカード付きcopyコマンドなどがうまく動かない
		if (wcschr(fileName, '?') || wcschr(fileName, '*') || wcschr(fileName, '¥"') || wcschr(fileName, '|') || wcschr(fileName, '<') || wcschr(fileName, '>'))
		{
			TRACEOUTW((L"INVALID PATH", fileName));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID); // Status STATUS_OBJECT_NAME_INVALID
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			free(fileName);
			return;
		}

		// バッファを溢れるくらいパスが長いときは無効
		if (wcslen(fileName) > MAX_PATH || wcslen(s_hdrvRoot) > MAX_PATH)
		{
			TRACEOUTW((L"TOO LONG PATH", fileName));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID); // Status STATUS_OBJECT_NAME_INVALID
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			free(fileName);
			return; // パスが変
		}
		if (hostdrvOpenTargetDirectory)
		{
			TRACEOUTW((L"OPEN TARGET DIR of %s", fileName));
		}
		if (hostdrvNT_getHostPath(fileName, hostPath, &isRoot, hostdrvOpenTargetDirectory))
		{
			TRACEOUTW((L"ERROR: invalid FileName"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID); // Status STATUS_OBJECT_NAME_INVALID
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			free(fileName);
			return; // パスが変
		}

		// パスの末尾が¥なら除去
		hostPathLength = wcslen(hostPath);
		if (hostPathLength > 0 && hostPath[hostPathLength - 1] == '¥¥')
		{
			hostPath[hostPathLength - 1] = '¥0';
		}

		// とりあえずオープン
		attrs = GetFileAttributesW(hostPath); // ディレクトリ情報を取得
		TRACEOUTW((L">>> OPEN: FILE %d %s", fileIndex, hostPath));
		if (attrs == INVALID_FILE_ATTRIBUTES)
		{
			// パスが存在しない、またはエラー
			if (hostdrvCreateDisposition == NP2_FILE_OPEN || hostdrvCreateDisposition == NP2_FILE_OVERWRITE)
			{
				// ないので開けない
				TRACEOUTW((L"OPEN ERROR: FILE %d %s", fileIndex, hostPath));
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_NOT_FOUND); // Status STATUS_OBJECT_NAME_NOT_FOUND
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, NP2_FILE_DOES_NOT_EXIST); // Information
				TRACEOUTW((L"returns STATUS_OBJECT_NAME_NOT_FOUND"));
				free(fileName);
				return;
			}
			else
			{
				// 新規作成
				if (hostdrvDirectoryFile)
				{
					// ディレクトリ作成
					TRACEOUTW((L"-> CREATE DIR: FILE %d %s", fileIndex, hostPath));
					if (!CreateDirectory(hostPath, NULL))
					{
						// 作成できなかった
						TRACEOUTW((L"OPEN CREATE ERROR: FILE %d %s", fileIndex, hostPath));
						cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID); // Status STATUS_OBJECT_NAME_INVALID
						cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
						TRACEOUTW((L"returns STATUS_OBJECT_NAME_INVALID"));
						free(fileName);
						return;
					}
					// 初期属性も設定　失敗しても気にしない
					SetFileAttributesW(hostPath, hostdrvFileAttributes);
				}
				else
				{
					// ファイル作成　再オープンのために作成条件を覚えておく。この際ファイルがある前提のフラグに書き換え
					TRACEOUTW((L"-> CREATE FILE: FILE %d %s", fileIndex, hostPath));
					fi->hostdrvWinAPIDesiredAccess = hostdrvWinAPIDesiredAccess;
					fi->hostdrvShareAccess = hostdrvShareAccess;
					fi->hostdrvWinAPICreateDisposition = hostdrvWinAPICreateDisposition;
					if (fi->hostdrvWinAPICreateDisposition == CREATE_NEW) fi->hostdrvWinAPICreateDisposition = OPEN_EXISTING;
					if (fi->hostdrvWinAPICreateDisposition == CREATE_ALWAYS) fi->hostdrvWinAPICreateDisposition = OPEN_EXISTING;
					if (fi->hostdrvWinAPICreateDisposition == TRUNCATE_EXISTING) fi->hostdrvWinAPICreateDisposition = OPEN_EXISTING;
					fi->hostdrvFileAttributes = hostdrvFileAttributes;
					if ((fi->hFile = CreateFileW(hostPath, hostdrvWinAPIDesiredAccess, hostdrvShareAccess, NULL, hostdrvWinAPICreateDisposition, hostdrvFileAttributes, NULL)) == INVALID_HANDLE_VALUE)
					{
						// 作成できなかった
						DWORD error = GetLastError();
						if (error == ERROR_PATH_NOT_FOUND)
						{
							TRACEOUTW((L"OPEN CREATE ERROR (ERROR_PATH_NOT_FOUND code %d): FILE %d %s", error, fileIndex, hostPath));
							cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_PATH_NOT_FOUND); // Status STATUS_OBJECT_PATH_NOT_FOUND
							cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
							TRACEOUTW((L"returns STATUS_OBJECT_PATH_NOT_FOUND"));
						}
						else
						{
							TRACEOUTW((L"OPEN CREATE ERROR (code %d): FILE %d %s", error, fileIndex, hostPath));
							cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID); // Status STATUS_OBJECT_NAME_INVALID
							cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
							TRACEOUTW((L"returns STATUS_OBJECT_NAME_INVALID"));
						}
						free(fileName);
						return;
					}
				}
				// あらためて属性を取得
				attrs = GetFileAttributesW(hostPath);
				returnInformation = NP2_FILE_CREATED;

				hostdrvNT_notifyChange(hostPath, NP2_FILE_ACTION_ADDED, 0);
			}
		}
		else
		{
			// パスが存在する
			if (hostdrvCreateDisposition == NP2_FILE_CREATE)
			{
				// 名前重複のため新規作成できない
				TRACEOUTW((L"OPRN ERROR: FILE %d %s", fileIndex, hostPath));
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_COLLISION); // Status STATUS_OBJECT_NAME_COLLISION
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				TRACEOUTW((L"returns STATUS_OBJECT_NAME_COLLISION"));
				free(fileName);
				return;
			}
			if (attrs & FILE_ATTRIBUTE_DIRECTORY)
			{
				// 対象がディレクトリ
				TRACEOUTW((L"OPEN DIR: FILE %d %s", fileIndex, hostPath));
				if (hostdrvNonDirectoryFile)
				{
					// ファイルとして開こうとしていたらエラー
					TRACEOUTW((L"IS NOT FILE: FILE %d %s", fileIndex, hostPath));
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_FILE_IS_A_DIRECTORY); // Status STATUS_FILE_IS_A_DIRECTORY
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					TRACEOUTW((L"returns STATUS_FILE_IS_A_DIRECTORY"));
					free(fileName);
					return;
				}
			}
			else
			{
				// 対象がファイル
				hostdrvFileAttributes = attrs;
				if (hostdrvDirectoryFile)
				{
					// ディレクトリとして開こうとしていたらエラー
					TRACEOUTW((L"IS NOT DIR: FILE %d %s", fileIndex, hostPath));
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_A_DIRECTORY); // Status STATUS_NOT_A_DIRECTORY
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					free(fileName);
					return;
				}

				// ファイルを開く　再オープンのためにオープン条件を覚えておく
				TRACEOUTW((L"OPEN FILE: FILE %d %s", fileIndex, hostPath));
				fi->hostdrvWinAPIDesiredAccess = hostdrvWinAPIDesiredAccess;
				fi->hostdrvShareAccess = hostdrvShareAccess;
				fi->hostdrvWinAPICreateDisposition = hostdrvWinAPICreateDisposition;
				if (fi->hostdrvWinAPICreateDisposition == CREATE_NEW) fi->hostdrvWinAPICreateDisposition = OPEN_EXISTING;
				if (fi->hostdrvWinAPICreateDisposition == CREATE_ALWAYS) fi->hostdrvWinAPICreateDisposition = OPEN_EXISTING;
				if (fi->hostdrvWinAPICreateDisposition == TRUNCATE_EXISTING) fi->hostdrvWinAPICreateDisposition = OPEN_EXISTING;
				fi->hostdrvFileAttributes = hostdrvFileAttributes;
				if ((fi->hFile = CreateFileW(hostPath, hostdrvWinAPIDesiredAccess, hostdrvShareAccess, NULL, hostdrvWinAPICreateDisposition, hostdrvFileAttributes, NULL)) == INVALID_HANDLE_VALUE)
				{
					// ファイルを開けなかった
					DWORD error = GetLastError();
					if (error == ERROR_SHARING_VIOLATION)
					{
						// ファイルが既に開かれてロックされている
						TRACEOUTW((L"OPEN FILE ERROR (ERROR_SHARING_VIOLATION code %d): FILE %d %s", error, fileIndex, hostPath));
						cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SHARING_VIOLATION);
						cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
						TRACEOUTW((L"returns STATUS_SHARING_VIOLATION"));
					}
					else if (error == ERROR_ACCESS_DENIED)
					{
						// 書き込み禁止状態など
						TRACEOUTW((L"OPEN FILE ERROR (NP2_STATUS_ACCESS_DENIED code %d): FILE %d %s", error, fileIndex, hostPath));
						cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_ACCESS_DENIED);
						cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
						TRACEOUTW((L"returns NP2_STATUS_ACCESS_DENIED"));
					}
					else
					{
						TRACEOUTW((L"OPEN FILE ERROR (code %d): FILE %d %s", error, fileIndex, hostPath));
						cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID);
						cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
						TRACEOUTW((L"returns STATUS_OBJECT_NAME_INVALID"));
					}
					free(fileName);
					return;
				}
			}
		}

		// フラグを記憶
		fi->isDirectory = !!(attrs & FILE_ATTRIBUTE_DIRECTORY);
		if (fi->isDirectory)
		{
			fi->isRoot = isRoot;
		}
		else
		{
			fi->isRoot = 0;
		}
		fi->allowDeleteChild = (hostdrvDesiredAccess & FILE_DELETE_CHILD) ? 1 : 0; // TODO: 効果が分かっていない。無視してもとりあえず動く
		if (hostdrvDeleteOnClose)
		{
			if (isRoot || (!(hostdrvDesiredAccess & DELETE) && !fi->isDirectory))
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_CANNOT_DELETE); // Status STATUS_CANNOT_DELETE
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				free(fileName);
				return;
			}
			else
			{
				if (!(s_hdrvAcc & HDFMODE_DELETE))
				{
					TRACEOUTW((L"ERROR: delete command is disabled by HOSTDRV."));
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_ACCESS_DENIED);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					free(fileName);
					return; 
				}
				if (fi->isDirectory && hostdrvNT_dirHasFiles(fi->hostFileName))
				{
					TRACEOUTW((L"ERROR: STATUS_DIRECTORY_NOT_EMPTY."));
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_DIRECTORY_NOT_EMPTY);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					free(fileName);
					return;
				}
				fi->deleteOnClose = 1;
			}
		}

		// 仮想マシン内とホストのファイル名を記憶
		fi->hostFileName = (WCHAR*)malloc((wcslen(hostPath) + 1) * sizeof(WCHAR));
		if (!fi->hostFileName)
		{
			TRACEOUTW((L"ERROR: cannot alloc hostFileName"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID); // Status STATUS_OBJECT_NAME_INVALID
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			free(fileName);
			return;
		}
		wcscpy(fi->hostFileName, hostPath);
		fi->fileName = fileName; // fileNameは使い回すのでfreeはしないこと

		// ホスト側のファイル管理番号をメモリに書き込み
		fsContextFileIndex = fileIndex;
		cpu_kmemorywrite_d(fileObject.FsContext + s_fsContextUserDataOffset, fsContextFileIndex);
		
		// OK
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS); // Status 0=STATUS_SUCCESS
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, returnInformation); // Information
	}
	else
	{
		// 同時オープン数超過
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_TOO_MANY_OPENED_FILES); // Status STATUS_TOO_MANY_OPENED_FILES
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		free(fileName);
	}
}

static void hostdrvNT_IRP_MJ_QUERY_VOLUME_INFORMATION(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	FS_INFORMATION_CLASS fsInfoClass = invokeInfo->stack.parameters.queryVolume.fsInformationClass;
	void* returnData = NULL;
	UINT32 dataLen = 0;
	UINT32 allowOverflow = 1;

	// ボリューム情報取得
	if (fsInfoClass == FileFsVolumeInformation)
	{
		WCHAR volumeLabel[] = NP2HOSTDRVNT_VOLUMELABEL;
		NP2_FILE_FS_VOLUME_INFORMATION info = { 0 };

		info.volumeCreationTime = 0x01C3F8D688000000ULL;
		info.volumeSerialNumber = 0x19822004;
		info.supportsObjects = 0;
		info.volumeLabelLength = sizeof(volumeLabel);
		CopyMemory(info.volumeLabel, volumeLabel, info.volumeLabelLength);

		dataLen = sizeof(info);
		returnData = &info;

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (fsInfoClass == FileFsAttributeInformation)
	{
		WCHAR fileSystem[] = NP2HOSTDRVNT_FILESYSTEM;
		NP2_FILE_FS_ATTRIBUTE_INFORMATION info = { 0 };

		info.fileSystemAttributes = FILE_CASE_PRESERVED_NAMES | FILE_CASE_SENSITIVE_SEARCH | FILE_UNICODE_ON_DISK;
		info.maximumComponentNameLength = 255;
		info.fileSystemNameLength = sizeof(fileSystem);
		CopyMemory(info.fileSystemName, fileSystem, info.fileSystemNameLength);

		dataLen = sizeof(info);
		returnData = &info;

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (fsInfoClass == FileFsSizeInformation)
	{
		MP2_FILE_FS_SIZE_INFORMATION info = { 0 };
		ULARGE_INTEGER freeBytesAvailable;
		ULARGE_INTEGER totalNumberOfBytes;
		ULARGE_INTEGER totalNumberOfFreeBytes;

		if ((s_hostdrvNTOptions & HOSTDRVNTOPTIONS_USEREALCAPACITY) && GetDiskFreeSpaceEx(s_hdrvRoot, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
		{
			// 実容量を返す
			info.SectorsPerAllocationUnit = 8;      // 8 セクタで 1 クラスタ
			info.BytesPerSector = 512;              // 512 バイト/セクタ
			info.TotalAllocationUnits = (UINT64)totalNumberOfBytes.QuadPart / (info.SectorsPerAllocationUnit * info.BytesPerSector);
			info.AvailableAllocationUnits = (UINT64)freeBytesAvailable.QuadPart / (info.SectorsPerAllocationUnit * info.BytesPerSector);
		}
		else
		{
			// ダミーを返す
			info.SectorsPerAllocationUnit = 8;      // 8 セクタで 1 クラスタ
			info.BytesPerSector = 512;              // 512 バイト/セクタ
			info.TotalAllocationUnits = (UINT64)2 * 1024 * 1024 * 1024 / (info.SectorsPerAllocationUnit * info.BytesPerSector);
			info.AvailableAllocationUnits = info.TotalAllocationUnits / 2;
		}

		dataLen = sizeof(info);
		returnData = &info;

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (fsInfoClass == FileFsFullSizeInformation)
	{
		MP2_FILE_FS_FULL_SIZE_INFORMATION info = { 0 };
		ULARGE_INTEGER freeBytesAvailable;
		ULARGE_INTEGER totalNumberOfBytes;
		ULARGE_INTEGER totalNumberOfFreeBytes;

		if ((s_hostdrvNTOptions & HOSTDRVNTOPTIONS_USEREALCAPACITY) && GetDiskFreeSpaceEx(s_hdrvRoot, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
		{
			// 実容量を返す
			info.SectorsPerAllocationUnit = 8;      // 8 セクタで 1 クラスタ
			info.BytesPerSector = 512;              // 512 バイト/セクタ
			info.TotalAllocationUnits = (UINT64)totalNumberOfBytes.QuadPart / (info.SectorsPerAllocationUnit * info.BytesPerSector);
			info.ActualAvailableAllocationUnits = (UINT64)totalNumberOfFreeBytes.QuadPart / (info.SectorsPerAllocationUnit * info.BytesPerSector);
			info.CallerAvailableAllocationUnits = (UINT64)freeBytesAvailable.QuadPart / (info.SectorsPerAllocationUnit * info.BytesPerSector);
		}
		else
		{
			// ダミーを返す
			info.SectorsPerAllocationUnit = 8;      // 8 セクタで 1 クラスタ
			info.BytesPerSector = 512;              // 512 バイト/セクタ
			info.TotalAllocationUnits = (UINT64)2 * 1024 * 1024 * 1024 / (info.SectorsPerAllocationUnit * info.BytesPerSector);
			info.ActualAvailableAllocationUnits = info.CallerAvailableAllocationUnits = info.TotalAllocationUnits / 2;
		}

		dataLen = sizeof(info);
		returnData = &info;

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (fsInfoClass == FileFsDeviceInformation)
	{
		NP2_FILE_FS_DEVICE_INFORMATION info = { 0 };

		info.DeviceType = NP2_FILE_DEVICE_DISK_FILE_SYSTEM; // XXX: FILE_DEVICE_NETWORK_FILE_SYSTEMを返すとCOPY CONとかがおかしくなる・・・
		if (s_hostdrvNTOptions & HOSTDRVNTOPTIONS_REMOVABLEDEVICE)
		{
			info.Characteristics = NP2_FILE_REMOVABLE_MEDIA | NP2_FILE_DEVICE_IS_MOUNTED;
		}
		else if (s_hostdrvNTOptions & HOSTDRVNTOPTIONS_DISKDEVICE)
		{
			info.Characteristics = NP2_FILE_DEVICE_IS_MOUNTED;
		}
		else
		{
			info.Characteristics = NP2_FILE_REMOTE_DEVICE | NP2_FILE_DEVICE_IS_MOUNTED;
		}

		dataLen = sizeof(info);
		returnData = &info;

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else
	{
		TRACEOUTW((L"Not implemented fsInfoClass %d (0x%02x)", fsInfoClass, fsInfoClass));

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
}

static void hostdrvNT_IRP_MJ_DIRECTORY_CONTROL(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	if (invokeInfo->stack.minorFunction == NP2_IRP_MN_QUERY_DIRECTORY)
	{
		NP2_FILE_OBJECT fileObject = { 0 };
		WCHAR filePattern[MAX_PATH] = L"*";
		NP2HOSTDRVNT_FILEINFO *fi;
		UINT8 restartScan;
		UINT8 returnSingleEntry;

		// 対象のファイルオブジェクトを取得
		if (invokeInfo->stack.fileObject == NULL)
		{
			TRACEOUTW((L"Invalid FileObject"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		hostdrvNT_memread(invokeInfo->stack.fileObject, &fileObject, sizeof(fileObject));
		fi = hostdrvNT_getFileInfo(&fileObject);
		if (!fi)
		{
			TRACEOUTW((L"Invalid FsContext"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ディレクトリかどうか確認
		if (!fi->isDirectory)
		{
			TRACEOUTW((L"It is not directory."));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ファイルパターンの読み取り
		if (invokeInfo->stack.parameters.queryDirectory.FileName)
		{
			NP2_UNICODE_STRING patternStr = { 0 };
			hostdrvNT_memread(invokeInfo->stack.parameters.queryDirectory.FileName, &patternStr, sizeof(NP2_UNICODE_STRING));
			if (patternStr.Length != 0)
			{
				if (0 < patternStr.Length && patternStr.Length < MAX_PATH * sizeof(WCHAR))
				{
					hostdrvNT_memread(patternStr.Buffer, &filePattern, patternStr.Length);
				}
				else
				{
					TRACEOUTW((L"Invalid Pattern FileName"));
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					return;
				}
			}
		}
		TRACEOUTW((L"FILE PATTERN: %s", filePattern));

		// スキャンを最初からやり直すかどうか確認
		restartScan = invokeInfo->stack.flags & NP2_SL_RESTART_SCAN;
		if (restartScan)
		{
			// 最初からやり直す場合、列挙用のハンドルを一旦閉じる
			if (fi->hFindFile != NULL)
			{
				FindClose(fi->hFindFile);
				fi->hFindFile = NULL;
			}
		}

		// フォルダスキャン実施
		returnSingleEntry = invokeInfo->stack.flags & NP2_SL_RETURN_SINGLE_ENTRY;
		if (returnSingleEntry)
		{
			// 1つだけ返すモード
			UINT32 length = invokeInfo->stack.parameters.read.length;
			FILE_INFORMATION_CLASS fileInfoClass = invokeInfo->stack.parameters.queryDirectory.FileInformationClass;
			TRACEOUTW((L"Single Entry Mode"));
			if (fileInfoClass == FileBothDirectoryInformation)
			{
				NP2_FILE_BOTH_DIR_INFORMATION dirInfo = { 0 };
				UINT32 bytesReturned = sizeof(dirInfo);

				// ディレクトリの読み取り
				if (!hostdrvNT_getOneEntry(fi, &dirInfo, filePattern))
				{
					if (fi->hFindFile == NULL)
					{
						// 該当が1個もない場合
						cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_NOT_FOUND);
						cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
						return;
					}
					else
					{
						// 該当が1個以上あるがもう返すものがない場合
						cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NO_MORE_FILES);
						cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
						return;
					}
				}

				// バッファが足りているか確認
				if (length < bytesReturned)
				{
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					return;
				}

				// 書き込み
				hostdrvNT_memwrite(invokeInfo->outBufferAddr, &dirInfo, bytesReturned);

				cpu_kmemorywrite_d(invokeInfo->statusAddr, 0); // Status STATUS_SUCCESS
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, bytesReturned); // Information
			}
			else
			{
				TRACEOUTW((L"Unsupported fileInfoClass: %d (0x%02x)", fileInfoClass, fileInfoClass));
				cpu_kmemorywrite_d(invokeInfo->statusAddr, 0xC0000010); // Status STATUS_INVALID_DEVICE_REQUEST
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			}
		}
		else
		{
			// 複数を返すモード
			UINT32 wLength = 0;
			UINT32 length = invokeInfo->stack.parameters.read.length;
			FILE_INFORMATION_CLASS fileInfoClass = invokeInfo->stack.parameters.queryDirectory.FileInformationClass;
			TRACEOUTW((L"Multi Entry Mode"));
			if (fileInfoClass == FileBothDirectoryInformation)
			{
				UINT32 lastWriteAddr = invokeInfo->outBufferAddr;
				UINT32 writeAddr = invokeInfo->outBufferAddr;
				NP2_FILE_BOTH_DIR_INFORMATION dirInfo = { 0 };
				UINT32 bytesReturnedOne = sizeof(NP2_FILE_BOTH_DIR_INFORMATION);

				if (length < bytesReturnedOne)
				{
					// バッファに1つも格納できないならSTATUS_BUFFER_TOO_SMALL
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					return;
				}
				while (wLength < length - bytesReturnedOne)
				{
					// ディレクトリの読み取り
					if (!hostdrvNT_getOneEntry(fi, &dirInfo, filePattern))
					{
						// もうファイルがない
						break;
					}

					// 次エントリへのオフセット
					dirInfo.NextEntryOffset = bytesReturnedOne;

					// 書き込み
					hostdrvNT_memwrite(writeAddr, &dirInfo, bytesReturnedOne);

					// 次のエントリにアドレスを進める
					wLength += bytesReturnedOne;
					lastWriteAddr = writeAddr;
					writeAddr += bytesReturnedOne;
				}

				if (wLength == 0)
				{
					// 今回1つも返すものがない
					if (fi->hFindFile == NULL)
					{
						// 該当が1個もない場合
						cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_NOT_FOUND);
						cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
						return;
					}
					else
					{
						// 該当が1個以上あるがもう返すものがない場合
						cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NO_MORE_FILES);
						cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
						return;
					}
				}
				else
				{
					// 少なくとも1つのデータを返した

					// 直前データの次データオフセットを消す
					dirInfo.NextEntryOffset = 0;
					hostdrvNT_memwrite(lastWriteAddr, &dirInfo, bytesReturnedOne);

					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, wLength); // Information
				}
			}
			else
			{
				TRACEOUTW((L"Not implemented fileInfoClass: %d (0x%02x)", fileInfoClass, fileInfoClass));
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_DEVICE_REQUEST);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			}
		}
	}
	else if (invokeInfo->stack.minorFunction == NP2_IRP_MN_NOTIFY_CHANGE_DIRECTORY)
	{
		NP2_FILE_OBJECT fileObject = { 0 };
		NP2HOSTDRVNT_FILEINFO* fi;
		UINT32 length;
		int i;

		// 非対応の場合抜ける
		if (s_pendingListCount == 0)
		{
			TRACEOUTW((L"Not implemented minorFunction: %d (0x%02x)", invokeInfo->stack.minorFunction, invokeInfo->stack.minorFunction));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_DEVICE_REQUEST);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 対象のファイルオブジェクトを取得
		if (invokeInfo->stack.fileObject == NULL)
		{
			TRACEOUTW((L"Invalid FileObject"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		hostdrvNT_memread(invokeInfo->stack.fileObject, &fileObject, sizeof(fileObject));
		fi = hostdrvNT_getFileInfo(&fileObject);
		if (!fi)
		{
			TRACEOUTW((L"Invalid FsContext"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 監視を開始する
		length = invokeInfo->stack.parameters.notifyDirectory.length;
		if (length < 9)
		{
			// 後で領域を借りるので、バッファに9byte格納できないならSTATUS_BUFFER_TOO_SMALL
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// あいているところへ放り込む
		for (i = 0; i < s_pendingListCount; i++)
		{
			UINT32 irpListAddr = s_pendingIrpListAddr + i * sizeof(UINT32);
			UINT32 fileIdxListAddr = s_pendingAliveListAddr + i * sizeof(UINT32);
			UINT32 irpAddr = cpu_kmemoryread_d(irpListAddr);
			UINT32 fileIdx = cpu_kmemoryread_d(fileIdxListAddr);
			if (irpAddr == 0 && fileIdx == 0)
			{
				// 監視開始
				TRACEOUTW((L"IRP_MN_NOTIFY_CHANGE_DIRECTORY: Start pending idx=%d", i));
				cpu_kmemorywrite_d(fileIdxListAddr, cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset)); // ファイルインデックスを記憶
				s_pendingIndexOrCompleteCount = i; // 待機開始対象のインデックスをセット
				cpu_kmemorywrite_d(invokeInfo->outBufferAddr, invokeInfo->stack.parameters.notifyDirectory.length); // XXX: 出力用バッファを借りてlengthを無理やり記憶
				cpu_kmemorywrite_d(invokeInfo->outBufferAddr + 4, invokeInfo->stack.parameters.notifyDirectory.completionFilter); // XXX: 出力用バッファを借りてcompletionFilterを無理やり記憶
				cpu_kmemorywrite(invokeInfo->outBufferAddr + 8, invokeInfo->stack.flags); // XXX: 出力用バッファを借りてflagsを無理やり記憶
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_PENDING);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
		}

		// あいてないのでエラー
		TRACEOUTW((L"IRP_MN_NOTIFY_CHANGE_DIRECTORY: Cannot pending because of too meny pending objects."));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INSUFFICIENT_RESOURCES);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
	}
	else
	{
		TRACEOUTW((L"Not implemented minorFunction: %d (0x%02x)", invokeInfo->stack.minorFunction, invokeInfo->stack.minorFunction));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_DEVICE_REQUEST);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
	}
}

static void hostdrvNT_IRP_MJ_QUERY_INFORMATION(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	UINT32 infoClass = invokeInfo->stack.parameters.queryFile.FileInformationClass;
	void* returnData = NULL;
	UINT32 dataLen = 0;
	NP2_FILE_OBJECT fileObject = { 0 };
	NP2HOSTDRVNT_FILEINFO* fi;
	UINT32 allowOverflow = 1;

	// 対象のファイルオブジェクトを取得
	if (invokeInfo->stack.fileObject == NULL)
	{
		TRACEOUTW((L"Invalid FileObject"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}
	hostdrvNT_memread(invokeInfo->stack.fileObject, &fileObject, sizeof(fileObject));
	fi = hostdrvNT_getFileInfo(&fileObject);
	if (!fi)
	{
		TRACEOUTW((L"Invalid FsContext"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	// ファイル or ディレクトリ情報取得
	if (infoClass == FileBasicInformation)
	{
		NP2_FILE_BASIC_INFORMATION basicInfo = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;

		if (GetFileAttributesEx(fi->hostFileName, GetFileExInfoStandard, &fileInfo))
		{
			basicInfo.CreationTime = *((UINT64*)&fileInfo.ftCreationTime);
			basicInfo.LastAccessTime = *((UINT64*)&fileInfo.ftLastAccessTime);
			basicInfo.LastWriteTime = *((UINT64*)&fileInfo.ftLastWriteTime);
			basicInfo.ChangeTime = *((UINT64*)&fileInfo.ftLastWriteTime);
			basicInfo.FileAttributes = fileInfo.dwFileAttributes;
			if (!fi->isDirectory && basicInfo.FileAttributes == 0)
			{
				basicInfo.FileAttributes |= FILE_ATTRIBUTE_NORMAL;
			}

			dataLen = sizeof(basicInfo);
			returnData = &basicInfo;
		}

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (infoClass == FileStandardInformation)
	{
		NP2_FILE_STANDARD_INFORMATION stdInfo = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;

		if (GetFileAttributesEx(fi->hostFileName, GetFileExInfoStandard, &fileInfo))
		{
			stdInfo.EndOfFile = ((UINT64)fileInfo.nFileSizeHigh << 32) | fileInfo.nFileSizeLow;
			stdInfo.AllocationSize = stdInfo.EndOfFile;
			stdInfo.NumberOfLinks = 1;
			stdInfo.DeletePending = fi->deleteOnClose ? 1 : 0;
			stdInfo.Directory = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;

			dataLen = sizeof(stdInfo);
			returnData = &stdInfo;
		}

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (infoClass == FileEaInformation)
	{
		NP2_FILE_EA_INFORMATION info = { 0 };

		info.EaSize = 0;

		dataLen = sizeof(info);
		returnData = &info;

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (infoClass == FileModeInformation)
	{
		NP2_FILE_MODE_INFORMATION info = { 0 };

		info.Mode = 0;

		dataLen = sizeof(info);
		returnData = &info;

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (infoClass == FileAllInformation)
	{
		NP2_FILE_ALL_INFORMATION allInfo = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;
		UINT32 fileNameBytes = wcslen(fi->fileName) * sizeof(WCHAR);

		if (fileNameBytes < sizeof(allInfo.NameInformation.FileName))
		{
			if (GetFileAttributesEx(fi->hostFileName, GetFileExInfoStandard, &fileInfo))
			{
				if (!fi->isDirectory && fileInfo.dwFileAttributes == 0)
				{
					fileInfo.dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
				}
				allInfo.BasicInformation.CreationTime = *((UINT64*)&fileInfo.ftCreationTime);
				allInfo.BasicInformation.LastAccessTime = *((UINT64*)&fileInfo.ftLastAccessTime);
				allInfo.BasicInformation.LastWriteTime = *((UINT64*)&fileInfo.ftLastWriteTime);
				allInfo.BasicInformation.ChangeTime = *((UINT64*)&fileInfo.ftLastWriteTime);
				allInfo.BasicInformation.FileAttributes = fileInfo.dwFileAttributes;
				allInfo.StandardInformation.EndOfFile = ((UINT64)fileInfo.nFileSizeHigh << 32) | fileInfo.nFileSizeLow;
				allInfo.StandardInformation.AllocationSize = allInfo.StandardInformation.EndOfFile;
				allInfo.StandardInformation.NumberOfLinks = 1;
				allInfo.StandardInformation.DeletePending = 0;
				allInfo.StandardInformation.Directory = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
				allInfo.InternalInformation.IndexNumber = (UINT64)fi->hFile; // XXX: 適当な番号を返す
				allInfo.EaInformation.EaSize = 0;
				allInfo.AccessInformation.AccessFlags = STANDARD_RIGHTS_ALL;
				allInfo.PositionInformation.CurrentByteOffset = fileObject.CurrentByteOffset;
				allInfo.ModeInformation.Mode = 0;
				allInfo.AlignmentInformation.AlignmentRequirement = 0;
				allInfo.NameInformation.FileNameLength = fileNameBytes;
				wcscpy(allInfo.NameInformation.FileName, fi->fileName);

				// ファイル名の実際の長さに基づいて計算
				dataLen = sizeof(allInfo) - sizeof(allInfo.NameInformation.FileName) + fileNameBytes;
				returnData = &allInfo;
			}
		}

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (infoClass == FileAttributeTagInformation)
	{
		NP2_FILE_ATTRIBUTE_TAG_INFORMATION info = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;

		if (GetFileAttributesEx(fi->hostFileName, GetFileExInfoStandard, &fileInfo))
		{
			if (!fi->isDirectory && fileInfo.dwFileAttributes == 0)
			{
				fileInfo.dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
			}
			info.FileAttributes = fileInfo.dwFileAttributes;
			info.ReparseTag = 0;

			dataLen = sizeof(info);
			returnData = &info;
		}

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (infoClass == FileNameInformation)
	{
		NP2_FILE_NAME_INFORMATION_FIXED info = { 0 };
		UINT32 fileNameBytes = wcslen(fi->fileName) * sizeof(WCHAR);

		if (fileNameBytes < sizeof(info.FileName))
		{
			info.FileNameLength = fileNameBytes;
			wcscpy(info.FileName, fi->fileName);

			// ファイル名の実際の長さに基づいて計算
			dataLen = sizeof(info) - sizeof(info.FileName) + fileNameBytes;
			returnData = &info;
		}

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
	else if (infoClass == FileNamesInformation)
	{
		UINT32 length;
		NP2_FILE_OBJECT fileObject = { 0 };
		UINT32 isOverflow = 0;
		UINT32 writeLength = 0;
		WIN32_FIND_DATA findFileData;
		WCHAR findPath[MAX_PATH * 2];
		UINT32 findPathLen;
		HANDLE hFindFile = NULL;

		// ディレクトリかどうか確認
		if (!fi->isDirectory)
		{
			TRACEOUTW((L"It is not directory."));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ディレクトリ内の全ファイル検索
		wcscpy(findPath, fi->hostFileName);
		findPathLen = wcslen(findPath);
		if (findPath[findPathLen - 1] != '¥¥')
		{
			wcscat(findPath, L"¥¥");
		}
		wcscat(findPath, L"*");

		length = invokeInfo->stack.parameters.queryFile.Length;
		while (writeLength + sizeof(NP2_FILE_NAMES_INFORMATION) <= length)
		{
			NP2_FILE_NAMES_INFORMATION data = { 0 };
			UINT32 cFileNameLength;

			if (hFindFile == NULL || hFindFile == INVALID_HANDLE_VALUE)
			{
				hFindFile = FindFirstFile(findPath, &findFileData);
				if (hFindFile == INVALID_HANDLE_VALUE)
				{
					hFindFile = NULL;
					break;
				}
			}
			else
			{
				if (!FindNextFile(hFindFile, &findFileData))
				{
					break;
				}
			}

			// 長すぎるファイル名は列挙除外
			cFileNameLength = wcslen(findFileData.cFileName);
			if (cFileNameLength >= MAX_PATH)
			{
				TRACEOUTW((L"Too long fileName: %s", findFileData.cFileName));
				continue;
			}

			// データセット
			data.NextEntryOffset = sizeof(FileNamesInformation);
			data.FileIndex = 0;
			data.FileNameLength = wcslen(findFileData.cFileName) * sizeof(WCHAR);
			wcscpy(data.FileName, findFileData.cFileName);

			// 書き込み
			hostdrvNT_memwrite(invokeInfo->outBufferAddr + writeLength, &data, sizeof(data));

			TRACEOUTW((L"FileNamesInformation FIND: %s", findFileData.cFileName));

			writeLength += sizeof(NP2_FILE_NAMES_INFORMATION);
		}

		// 1つも取得できなかった場合
		if (hFindFile == NULL || hFindFile == INVALID_HANDLE_VALUE)
		{
			TRACEOUTW((L"FAILED"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL); // Status 0=STATUS_BUFFER_TOO_SMALL
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// まだ残りのファイルがあるか確認
		if (FindNextFile(hFindFile, &findFileData))
		{
			isOverflow = 1;
		}
		FindClose(hFindFile);

		// 少なくとも1データが完全にかけていれば最後のエントリの次エントリオフセットを削除
		if (writeLength > sizeof(NP2_FILE_NAMES_INFORMATION))
		{
			cpu_kmemorywrite_d(invokeInfo->outBufferAddr + writeLength - sizeof(NP2_FILE_NAMES_INFORMATION), writeLength); // Information
		}

		// ステータスを返す
		if (isOverflow)
		{
			TRACEOUTW((L"OVERFLOW"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_OVERFLOW);
		}
		else
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
		}
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, writeLength); // Information

		return; // 既に結果まで返しているので抜ける
	}
	else
	{
		TRACEOUTW((L"Not implemented FileInformationClass %d (0x%02x)", infoClass, infoClass));

		// 結果をセット
		hostdrvNT_setQueryInformationResult(invokeInfo, returnData, dataLen, allowOverflow);
	}
}

static void hostdrvNT_IRP_MJ_SET_INFORMATION(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	UINT32 infoClass = invokeInfo->stack.parameters.queryFile.FileInformationClass;
	NP2_FILE_OBJECT fileObject = { 0 };
	NP2HOSTDRVNT_FILEINFO* fi;
	UINT32 allowOverflow = 1;
	UINT32 length = invokeInfo->stack.parameters.queryFile.Length;
	int reopenIndex = 0;

	// リードオンリーなら拒否
	if (!(s_hdrvAcc & HDFMODE_WRITE))
	{
		TRACEOUTW((L"ERROR: set command is disabled by HOSTDRV."));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_MEDIA_WRITE_PROTECTED);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	// 対象のファイルオブジェクトを取得
	if (invokeInfo->stack.fileObject == NULL)
	{
		TRACEOUTW((L"Invalid FileObject"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}
	hostdrvNT_memread(invokeInfo->stack.fileObject, &fileObject, sizeof(fileObject));
	fi = hostdrvNT_getFileInfo(&fileObject);
	if (!fi)
	{
		TRACEOUTW((L"Invalid FsContext"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	// ファイル or ディレクトリ情報設定
	if (infoClass == FileBasicInformation)
	{
		NP2_FILE_BASIC_INFORMATION basicInfo = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;
		UINT32 changeFileTime = 0;

		// 入力データサイズが期待通りか確認
		if (length < sizeof(basicInfo))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 読み込み
		hostdrvNT_memread(invokeInfo->inBufferAddr, &basicInfo, sizeof(basicInfo));

		if (!GetFileAttributesEx(fi->hostFileName, GetFileExInfoStandard, &fileInfo))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ファイル属性を設定
		if (basicInfo.CreationTime != 0xffffffffffffffffull && 
			(fileInfo.ftCreationTime.dwHighDateTime != (UINT32)(basicInfo.CreationTime >> 32) ||
			 fileInfo.ftCreationTime.dwLowDateTime != (UINT32)(basicInfo.CreationTime)))
		{
			fileInfo.ftCreationTime.dwHighDateTime = (UINT32)(basicInfo.CreationTime >> 32);
			fileInfo.ftCreationTime.dwLowDateTime = (UINT32)(basicInfo.CreationTime);
			changeFileTime = 1;
		}
		if (basicInfo.LastAccessTime != 0xffffffffffffffffull &&
			(fileInfo.ftLastAccessTime.dwHighDateTime != (UINT32)(basicInfo.LastAccessTime >> 32) ||
			 fileInfo.ftLastAccessTime.dwLowDateTime != (UINT32)(basicInfo.LastAccessTime)))
		{
			fileInfo.ftLastAccessTime.dwHighDateTime = (UINT32)(basicInfo.LastAccessTime >> 32);
			fileInfo.ftLastAccessTime.dwLowDateTime = (UINT32)(basicInfo.LastAccessTime);
			changeFileTime = 1;
		}
		if (basicInfo.LastWriteTime != 0xffffffffffffffffull &&
			(fileInfo.ftLastWriteTime.dwHighDateTime != (UINT32)(basicInfo.LastWriteTime >> 32) ||
			 fileInfo.ftLastWriteTime.dwLowDateTime != (UINT32)(basicInfo.LastWriteTime)))
		{
			fileInfo.ftLastWriteTime.dwHighDateTime = (UINT32)(basicInfo.LastWriteTime >> 32);
			fileInfo.ftLastWriteTime.dwLowDateTime = (UINT32)(basicInfo.LastWriteTime);
			changeFileTime = 1;
		}
		fileInfo.dwFileAttributes = basicInfo.FileAttributes;
		if (fi->isDirectory)
		{
			fileInfo.dwFileAttributes &= ‾FILE_ATTRIBUTE_DIRECTORY; // エラーになるので付けない
		}
		else if (fileInfo.dwFileAttributes == 0)
		{
			fileInfo.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		}
		if (!SetFileAttributesW(fi->hostFileName, fileInfo.dwFileAttributes))
		{
			DWORD error = GetLastError();
			TRACEOUTW((L"Error: SetFileAttributesW code: %d (0x%08x)", error, error));
		}
		if (changeFileTime)
		{
			HANDLE fh = fi->hFile;
			if (!fh || fh == INVALID_HANDLE_VALUE)
			{
				// ファイルハンドルが閉じていたら再オープン
				UINT32 fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);
				if (!hostdrvNT_reopenFile(fsContextFileIndex))
				{
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					return;
				}
				fh = fi->hFile;
				if (!fh || fh == INVALID_HANDLE_VALUE)
				{
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					return;
				}
				reopenIndex = fsContextFileIndex;
			}
			if (!SetFileTime(fh, &fileInfo.ftCreationTime, &fileInfo.ftLastAccessTime, &fileInfo.ftLastWriteTime))
			{
				DWORD error = GetLastError();
				TRACEOUTW((L"Error: SetFileTime code: %d (0x%08x)", error, error));
			}
		}

		hostdrvNT_notifyChange(fi->hostFileName, NP2_FILE_ACTION_MODIFIED, 0);
	}
	else if (infoClass == FileEndOfFileInformation)
	{
		NP2_FILE_END_OF_FILE_INFORMATION eofInfo = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;
		LARGE_INTEGER liAllocationSize;
		HANDLE fh;

		// 入力データサイズが期待通りか確認
		if (length < sizeof(eofInfo))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 読み込み
		hostdrvNT_memread(invokeInfo->inBufferAddr, &eofInfo, sizeof(eofInfo));

		// ファイル長さを設定
		fh = fi->hFile;
		if (!fh || fh == INVALID_HANDLE_VALUE)
		{
			// ファイルハンドルが閉じていたら再オープン
			UINT32 fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);
			if (!hostdrvNT_reopenFile(fsContextFileIndex))
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			fh = fi->hFile;
			if (!fh || fh == INVALID_HANDLE_VALUE)
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			reopenIndex = fsContextFileIndex;
		}
		if (eofInfo.EndOfFile > UINT_MAX)
		{
			// 安全策　UINT最大値より大きいものは異常とする
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		liAllocationSize.QuadPart = eofInfo.EndOfFile;
		SetFilePointerEx(fh, liAllocationSize, NULL, FILE_BEGIN);
		if (!SetEndOfFile(fh))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		hostdrvNT_notifyChange(fi->hostFileName, NP2_FILE_ACTION_MODIFIED, 0);
	}
	else if (infoClass == FileAllocationInformation)
	{
		NP2_FILE_ALLOCATION_INFORMATION allocInfo = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;
		LARGE_INTEGER currentFileSize;
		HANDLE fh;

		// 入力データサイズが期待通りか確認
		if (length < sizeof(allocInfo))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 読み込み
		hostdrvNT_memread(invokeInfo->inBufferAddr, &allocInfo, sizeof(allocInfo));

		// ファイル長さを設定
		fh = fi->hFile;
		if (!fh || fh == INVALID_HANDLE_VALUE)
		{
			// ファイルハンドルが閉じていたら再オープン
			UINT32 fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);
			if (!hostdrvNT_reopenFile(fsContextFileIndex))
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			fh = fi->hFile;
			if (!fh || fh == INVALID_HANDLE_VALUE)
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			reopenIndex = fsContextFileIndex;
		}
		if (allocInfo.AllocationSize > UINT_MAX)
		{
			// 安全策　UINT最大値より大きいものは異常とする
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		// 実際のサイズは縮小する場合のみ変える
		if (GetFileSizeEx(fh, &currentFileSize))
		{
			if (allocInfo.AllocationSize < currentFileSize.QuadPart)
			{
				LARGE_INTEGER liAllocationSize;
				liAllocationSize.QuadPart = allocInfo.AllocationSize;
				SetFilePointerEx(fh, liAllocationSize, NULL, FILE_BEGIN);
				if (!SetEndOfFile(fh))
				{
					cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
					cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
					return;
				}
			}
		}

		hostdrvNT_notifyChange(fi->hostFileName, NP2_FILE_ACTION_MODIFIED, 0);
	}
	else if (infoClass == FileDispositionInformation)
	{
		NP2_FILE_DISPOSITION_INFORMATION disposeInfo = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;

		// 入力データサイズが期待通りか確認
		if (length < sizeof(disposeInfo))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 削除不可なら拒否
		if (!(s_hdrvAcc & HDFMODE_DELETE))
		{
			TRACEOUTW((L"ERROR: delete is disabled by HOSTDRV."));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_ACCESS_DENIED);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ファイルオープン時にDELETE権限がなければ拒否（ディレクトリはOK）
		if (!(fi->hostdrvWinAPIDesiredAccess & DELETE) && !fi->isDirectory)
		{
			TRACEOUTW((L"ERROR: no DELETE flag."));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_ACCESS_DENIED);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ディレクトリの場合、空でないならエラー
		if (fi->isDirectory && hostdrvNT_dirHasFiles(fi->hostFileName))
		{
			TRACEOUTW((L"ERROR: STATUS_DIRECTORY_NOT_EMPTY."));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_DIRECTORY_NOT_EMPTY);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ファイル属性が読み取り専用なら拒否
		if (!GetFileAttributesEx(fi->hostFileName, GetFileExInfoStandard, &fileInfo))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		{
			TRACEOUTW((L"ERROR: file is readonly."));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_CANNOT_DELETE);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 読み込み
		hostdrvNT_memread(invokeInfo->inBufferAddr, &disposeInfo, sizeof(disposeInfo));

		// 削除フラグをセット
		if (disposeInfo.DeleteFileOnClose)
		{
			fi->deleteOnClose = 1;
		}
		else
		{
			fi->deleteOnClose = 0;
		}
	}
	else if (infoClass == FileRenameInformation)
	{
		NP2_FILE_RENAME_INFORMATION renameInfo = { 0 };
		WCHAR* newPath = NULL;
		WCHAR newHostPath[MAX_PATH * 2] = { 0 };
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;
		int pathLengthInBytes;
		UINT8 isRoot;
		UINT32 fsContextFileIndex;
		WCHAR* oldFileName;
		WCHAR* oldHostFileName;

		// 入力データサイズが期待通りか確認
		if (length < sizeof(renameInfo))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_BUFFER_TOO_SMALL);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 削除不可なら拒否
		if (!(s_hdrvAcc & HDFMODE_DELETE))
		{
			TRACEOUTW((L"ERROR: delete is disabled by HOSTDRV."));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_ACCESS_DENIED);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 読み込み
		hostdrvNT_memread(invokeInfo->inBufferAddr, &renameInfo, sizeof(renameInfo));

		// 変更後のパス長さが異常なら拒否
		if (renameInfo.FileNameLength == 0 || renameInfo.FileNameLength > MAX_PATH)
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// XXX: 相対パスにしようとしてきた場合、対応していないので違うデバイス扱いとしておく
		if (renameInfo.RootDirectory != 0)
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SAME_DEVICE);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 新ファイルパス文字列割り当て
		pathLengthInBytes = renameInfo.FileNameLength;
		newPath = (WCHAR*)malloc(renameInfo.FileNameLength + sizeof(WCHAR));
		if (!newPath)
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		memset(newPath, 0, renameInfo.FileNameLength + sizeof(WCHAR));

		// 新ファイルパス部分を読み取り
		hostdrvNT_memread(invokeInfo->inBufferAddr + 4 + 4 + 4, newPath, renameInfo.FileNameLength);

		// 新ファイルパスを¥??¥の形式で指定された場合、特例
		if (wcsnicmp(newPath, L"¥¥??¥¥", 4) == 0)
		{
			// ドライブ文字がないタイプならNG
			if (wcslen(newPath) < 5)
			{
				free(newPath);
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SAME_DEVICE);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}

			// ¥??¥Z:¥の形式のパスを前提に、ドライブ文字部分を適当にzに書き換え
			newPath[4] = 'z';

			// ¥??¥Z:¥の形式のパスが来た場合、特例で¥??¥Z:¥をカットして新ファイルパスとする
			if (wcsnicmp(newPath, L"¥¥??¥¥z:¥¥", 7) == 0)
			{
				WCHAR* pathTmp = newPath;
				while (*(pathTmp + 7))
				{
					*pathTmp = *(pathTmp + 7);
					pathTmp++;
				}
				*pathTmp = *(pathTmp + 7);
			}
			else
			{
				// その他の形式なら不可
				free(newPath);
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SAME_DEVICE);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
		}
		// 新ファイルパスを¥DosDevices¥z:¥の形式で指定された場合、特例
		if (wcslen(newPath) >= 15 && wcsnicmp(newPath, L"¥¥DosDevices¥¥", 12) == 0 && newPath[13]==':')
		{
			// ¥DosDevices¥Z:¥の形式のパスを前提に、ドライブ文字部分を適当にzに書き換え
			newPath[12] = 'z';

			// ¥DosDevices¥Z:¥の形式のパスが来た場合、特例で¥DosDevices¥Z:¥をカットして新ファイルパスとする
			if (wcsnicmp(newPath, L"¥¥DosDevices¥¥z:¥¥", 15) == 0)
			{
				WCHAR* pathTmp = newPath;
				while (*(pathTmp + 15))
				{
					*pathTmp = *(pathTmp + 15);
					pathTmp++;
				}
				*pathTmp = *(pathTmp + 15);
			}
			else
			{
				// その他の形式なら不可
				free(newPath);
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SAME_DEVICE);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
		}

		// 長すぎるファイル名は不可
		if (wcslen(newPath) >= MAX_PATH)
		{
			free(newPath);
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ホストのディレクトリパスへ変換
		if (hostdrvNT_getHostPath(newPath, newHostPath, &isRoot, 0))
		{
			// パスが変
			free(newPath);
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return; 
		}

		// ルートの移動は不可
		if (isRoot)
		{
			free(newPath);
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 一旦ファイルを閉じてから移動させる
		fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);
		hostdrvNT_preCloseFile(fsContextFileIndex);
		if (!MoveFileExW(fi->hostFileName, newHostPath, (renameInfo.ReplaceIfExists ? MOVEFILE_REPLACE_EXISTING : 0) | MOVEFILE_COPY_ALLOWED))
		{
			DWORD err = GetLastError();
			if (err == ERROR_FILE_NOT_FOUND)
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_NOT_FOUND);
			}
			else if (err == ERROR_ALREADY_EXISTS)
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_COLLISION);
			}
			else
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			}
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			free(newPath);
			return;
		}
		TRACEOUTW((L"MOVE %s -> %s", fi->hostFileName, newHostPath));

		// ファイルオブジェクトのパスを新パスに変えておく
		oldFileName = fi->fileName;
		oldHostFileName = fi->hostFileName;
		fi->fileName = newPath;
		fi->hostFileName = (WCHAR*)malloc((wcslen(newHostPath) + 1) * sizeof(WCHAR));
		wcscpy(fi->hostFileName, newHostPath);
		free(oldFileName);
		free(oldHostFileName);

		hostdrvNT_notifyChange(fi->hostFileName, NP2_FILE_ACTION_RENAMED_NEW_NAME, 1);
	}
	else
	{
		TRACEOUTW((L"Not implemented FileInformationClass %d (0x%02x)", infoClass, infoClass));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_IMPLEMENTED);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	if (reopenIndex)
		hostdrvNT_preCloseFile(reopenIndex);

	// 結果（成功）をセット
	cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
	cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
}

static void hostdrvNT_IRP_MJ_CLEANUP(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	// ファイルロックなどを解除する。また要求が来たら再オープンは出来るようにしておく
	NP2_FILE_OBJECT fileObject = { 0 };
	UINT32 fsContextFileIndex;
	hostdrvNT_memread(invokeInfo->stack.fileObject, &fileObject, sizeof(fileObject));
	fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);

	if (1 <= fsContextFileIndex && fsContextFileIndex < NP2HOSTDRVNT_FILES_MAX && hostdrvNT.files[fsContextFileIndex].fileName != NULL)
	{
		if (hostdrvNT.files[fsContextFileIndex].isDirectory)
		{
			TRACEOUTW((L"CLEANUP DIR: FILE %d %s", fsContextFileIndex, hostdrvNT.files[fsContextFileIndex].hostFileName));
		}
		else
		{
			TRACEOUTW((L"CLEANUP FILE: FILE %d %s", fsContextFileIndex, hostdrvNT.files[fsContextFileIndex].hostFileName));
		}
		hostdrvNT_preCloseFile(fsContextFileIndex);
		hostdrvNT.files[fsContextFileIndex].hostdrvShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE; // XXX: CLEANUP後の再オープン時にはALL許可にする
		//hostdrvNT.files[fsContextFileIndex].hostdrvWinAPIDesiredAccess = GENERIC_READ; // XXX: CLEANUP後の再オープン時にはGENERIC_READにする
	}

	cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
	cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
}

static void hostdrvNT_IRP_MJ_CLOSE(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	// ファイルを完全に閉じる
	NP2_FILE_OBJECT fileObject = { 0 };
	UINT32 fsContextFileIndex;
	hostdrvNT_memread(invokeInfo->stack.fileObject, &fileObject, sizeof(fileObject));
	fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);

	if (1 <= fsContextFileIndex && fsContextFileIndex < NP2HOSTDRVNT_FILES_MAX && hostdrvNT.files[fsContextFileIndex].fileName != NULL)
	{
		if (hostdrvNT.files[fsContextFileIndex].isDirectory)
		{
			TRACEOUTW((L"<<< CLOSE DIR: FILE #%d %s", fsContextFileIndex, hostdrvNT.files[fsContextFileIndex].hostFileName));
		}
		else
		{
			TRACEOUTW((L"<<< CLOSE FILE: FILE #%d %s", fsContextFileIndex, hostdrvNT.files[fsContextFileIndex].hostFileName));
		}
		hostdrvNT_closeFile(fsContextFileIndex);
	}

	cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
	cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
}

static void hostdrvNT_IRP_MJ_READ(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	NP2_FILE_OBJECT fileObject = { 0 };
	NP2HOSTDRVNT_FILEINFO* fi;
	UINT32 length = invokeInfo->stack.parameters.read.length;

	// 対象のファイルオブジェクトを取得
	if (invokeInfo->stack.fileObject == NULL)
	{
		TRACEOUTW((L"Invalid FileObject"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}
	hostdrvNT_memread(invokeInfo->stack.fileObject, &fileObject, sizeof(fileObject));
	fi = hostdrvNT_getFileInfo(&fileObject);
	if (!fi)
	{
		TRACEOUTW((L"Invalid FsContext"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	// ディレクトリかどうか判定
	if (fi->isDirectory)
	{
		// ディレクトリのリードは未サポート
		TRACEOUTW((L"INVALID!!!  READ DIR: %s", fi->fileName));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}
	else
	{
		// ファイルのリード
		HANDLE fh;
		UINT64 offset;
		LARGE_INTEGER offsetLI;
		UINT64 fileLength;
		UINT64 copySize;
		BYTE* fileReadBuffer;
		DWORD bytesRead;
		int reopenIndex = 0;

		TRACEOUTW((L"READ FILE: %s", fi->hostFileName));

		// 長さ0なら成功扱いにする
		if (length == 0)
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 読み出しオフセットを取得
		if (invokeInfo->stack.parameters.read.byteOffset != NP2_FILE_USE_FILE_POINTER_POSITION)
		{
			offset = invokeInfo->stack.parameters.read.byteOffset;
		}
		else
		{
			offset = fileObject.CurrentByteOffset;
		}

		// ファイル内容読み出し
		fh = fi->hFile;
		if (!fh || fh == INVALID_HANDLE_VALUE)
		{
			// ファイルハンドルが閉じていたら再オープン
			UINT32 fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);
			if (!hostdrvNT_reopenFile(fsContextFileIndex))
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			fh = fi->hFile;
			if (!fh || fh == INVALID_HANDLE_VALUE)
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			reopenIndex = fsContextFileIndex;
		}

		// ファイル終端ならSTATUS_END_OF_FILE
		offsetLI.QuadPart = offset;
		if (!SetFilePointerEx(fh, offsetLI, NULL, FILE_BEGIN))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_END_OF_FILE);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		fileLength = file_getsize(fh);
		if (offset >= fileLength)
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_END_OF_FILE);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		
		// データコピーサイズ計算
		copySize = fileLength - offset;
		if (length < copySize)
		{
			copySize = length;
		}

		// バッファを確保
		fileReadBuffer = (BYTE*)malloc(copySize);
		if (!fileReadBuffer)
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		bytesRead = 0;
		if (!ReadFile(fh, fileReadBuffer, copySize, &bytesRead, NULL))
		{
			free(fileReadBuffer);
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 書き込み
		if (invokeInfo->outBufferAddr != 0)
		{
			hostdrvNT_memwrite(invokeInfo->outBufferAddr, fileReadBuffer, copySize);
		}
		else
		{
			copySize = 0;
		}

		// もう要らないのでメモリ解放
		free(fileReadBuffer);

		// 同期アクセスならバイトオフセットを更新
		if (fileObject.Flags & NP2_FO_SYNCHRONOUS_IO)
		{
			fileObject.CurrentByteOffset = offset + copySize;
			cpu_kmemorywrite_d(invokeInfo->stack.fileObject + offsetof(NP2_FILE_OBJECT, CurrentByteOffset), (UINT32)fileObject.CurrentByteOffset);
			cpu_kmemorywrite_d(invokeInfo->stack.fileObject + offsetof(NP2_FILE_OBJECT, CurrentByteOffset) + 4, (UINT32)(fileObject.CurrentByteOffset >> 32));
		}

		if (reopenIndex)
			hostdrvNT_preCloseFile(reopenIndex);

		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, copySize); // Information
	}
}

static void hostdrvNT_IRP_MJ_WRITE(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	NP2_FILE_OBJECT fileObject = { 0 };
	NP2HOSTDRVNT_FILEINFO* fi;
	UINT32 length = invokeInfo->stack.parameters.write.length;

	// リードオンリーなら拒否
	if (!(s_hdrvAcc & HDFMODE_WRITE))
	{
		TRACEOUTW((L"ERROR: write command is disabled by HOSTDRV."));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_MEDIA_WRITE_PROTECTED);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	// 対象のファイルオブジェクトを取得
	if (invokeInfo->stack.fileObject == NULL)
	{
		TRACEOUTW((L"Invalid FileObject"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}
	hostdrvNT_memread(invokeInfo->stack.fileObject, &fileObject, sizeof(fileObject));
	fi = hostdrvNT_getFileInfo(&fileObject);
	if (!fi)
	{
		TRACEOUTW((L"Invalid FsContext"));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}

	// ディレクトリかどうか判定
	if (fi->isDirectory)
	{
		// ディレクトリへの書き込みは不可
		TRACEOUTW((L"INVALID!!!  WRITE DIR: %s", fi->fileName));
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		return;
	}
	else
	{
		HANDLE fh;
		UINT64 offset;
		LARGE_INTEGER offsetLI;
		UINT64 fileLength;
		UINT64 copySize;
		BYTE* fileWriteBuffer;
		DWORD bytesWrite;
		int reopenIndex = 0;

		TRACEOUTW((L"WRITE FILE: %s", fi->hostFileName));

		// 長さ0なら成功扱いにする
		if (length == 0)
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 書き込みオフセットを取得
		if (invokeInfo->stack.parameters.write.byteOffset != 0xfffffffe) // FILE_USE_FILE_POINTER_POSITION
		{
			offset = invokeInfo->stack.parameters.write.byteOffset;
		}
		else
		{
			offset = fileObject.CurrentByteOffset;
		}

		// ファイル内容書き込み
		fh = fi->hFile;
		if (!fh || fh == INVALID_HANDLE_VALUE)
		{
			// ファイルハンドルが閉じていたら再オープン
			UINT32 fsContextFileIndex = cpu_kmemoryread_d(fileObject.FsContext + s_fsContextUserDataOffset);
			if (!hostdrvNT_reopenFile(fsContextFileIndex))
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_OBJECT_NAME_INVALID);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			fh = fi->hFile;
			if (!fh || fh == INVALID_HANDLE_VALUE)
			{
				cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
				cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
				return;
			}
			reopenIndex = fsContextFileIndex;
		}

		// ファイル書き込み位置の設定
		offsetLI.QuadPart = offset;
		if (!SetFilePointerEx(fh, offsetLI, NULL, FILE_BEGIN))
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_END_OF_FILE);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}
		
		// 書き込みサイズ
		copySize = length;

		// バッファを確保
		fileWriteBuffer = (BYTE*)malloc(copySize);
		if (!fileWriteBuffer)
		{
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// 書き込みデータをメモリから読み取り
		if (invokeInfo->inBufferAddr != 0)
		{
			hostdrvNT_memread(invokeInfo->inBufferAddr, fileWriteBuffer, copySize);
		}
		else
		{
			free(fileWriteBuffer);
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ファイルに書き込む
		bytesWrite = 0;
		if (!WriteFile(fh, fileWriteBuffer, copySize, &bytesWrite, NULL))
		{
			free(fileWriteBuffer);
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_PARAMETER);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			return;
		}

		// ファイルを閉じる
		free(fileWriteBuffer);

		// 同期アクセスならバイトオフセットを更新
		if (fileObject.Flags & 0x00000002) // FO_SYNCHRONOUS_IO
		{
			fileObject.CurrentByteOffset = offset + copySize;
			cpu_kmemorywrite_d(invokeInfo->stack.fileObject + offsetof(NP2_FILE_OBJECT, CurrentByteOffset), (UINT32)fileObject.CurrentByteOffset);
			cpu_kmemorywrite_d(invokeInfo->stack.fileObject + offsetof(NP2_FILE_OBJECT, CurrentByteOffset) + 4, (UINT32)(fileObject.CurrentByteOffset >> 32));
		}

		if (reopenIndex)
			hostdrvNT_preCloseFile(reopenIndex);

		cpu_kmemorywrite_d(invokeInfo->statusAddr, 0); // Status STATUS_SUCCESS
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, copySize); // Information
	}
}

static void hostdrvNT_IRP_MJ_FILE_SYSTEM_CONTROL(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	if (invokeInfo->stack.minorFunction == NP2_IRP_MN_USER_FS_REQUEST)
	{
		TRACEOUTW((L" IRP_MN_USER_FS_REQUEST: %d(0x%02x)", invokeInfo->stack.parameters.fileSystemControl.FsControlCode, invokeInfo->stack.parameters.fileSystemControl.FsControlCode));
		if (invokeInfo->stack.parameters.fileSystemControl.FsControlCode == FSCTL_INVALIDATE_VOLUMES)
		{
			TRACEOUTW((L"FSCTL_INVALIDATE_VOLUMES"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_DEVICE_REQUEST);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
		else if (invokeInfo->stack.parameters.fileSystemControl.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1)
		{
			TRACEOUTW((L"FSCTL_REQUEST_OPLOCK_LEVEL_1"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SUPPORTED);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
		else if (invokeInfo->stack.parameters.fileSystemControl.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)
		{
			TRACEOUTW((L"FSCTL_REQUEST_OPLOCK_LEVEL_2"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SUPPORTED);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
		else if (invokeInfo->stack.parameters.fileSystemControl.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)
		{
			TRACEOUTW((L"FSCTL_REQUEST_FILTER_OPLOCK"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SUPPORTED);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
		else if (invokeInfo->stack.parameters.fileSystemControl.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)
		{
			TRACEOUTW((L"FSCTL_REQUEST_BATCH_OPLOCK"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SUPPORTED);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
		else if (invokeInfo->stack.parameters.fileSystemControl.FsControlCode == FSCTL_IS_VOLUME_MOUNTED)
		{
			TRACEOUTW((L"FSCTL_IS_VOLUME_MOUNTED"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SUPPORTED); // Status STATUS_NOT_SUPPORTED
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
			//cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
			//cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
		else if (invokeInfo->stack.parameters.fileSystemControl.FsControlCode == FSCTL_DISMOUNT_VOLUME)
		{
			TRACEOUTW((L"FSCTL_DISMOUNT_VOLUME"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_SUCCESS);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
		else if (invokeInfo->stack.parameters.fileSystemControl.FsControlCode == FSCTL_GET_COMPRESSION)
		{
			TRACEOUTW((L"FSCTL_GET_COMPRESSION"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_SUPPORTED);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
		else
		{
			TRACEOUTW((L"UNKNOWN"));
			cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_INVALID_DEVICE_REQUEST);
			cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
		}
	}
	else
	{
		cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_IMPLEMENTED);
		cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
	}
}

static void hostdrvNT_IRP_MJ_DEVICE_CONTROL(HOSTDRVNT_INVOKEINFO* invokeInfo)
{
	UINT32 ioControlCode = invokeInfo->stack.parameters.deviceIoControl.IoControlCode;
	TRACEOUTW((L"IoControlCode: %d(0x%08x)", ioControlCode, ioControlCode));
	cpu_kmemorywrite_d(invokeInfo->statusAddr, NP2_STATUS_NOT_IMPLEMENTED);
	cpu_kmemorywrite_d(invokeInfo->statusAddr + 4, 0); // Information
}

// ---------- Entry Function
static void hostdrvNT_invoke()
{
	HOSTDRVNT_INVOKEINFO invokeInfo;

	if ((s_hdrvRoot[0] == '¥0') || (!np2cfg.hdrvenable))
	{
		// 無効の場合何もせずに抜ける
		return;
	}

#if defined(SUPPORT_IA32_HAXM)
	// HAXMレジスタを読み取り
	i386haxfunc_vcpu_getREGs(&np2haxstat.state);
	i386haxfunc_vcpu_getFPU(&np2haxstat.fpustate);
	np2haxstat.update_regs = np2haxstat.update_fpu = 0;
	// HAXMレジスタ→猫レジスタにコピー
	ia32hax_copyregHAXtoNP2();
#endif

	// ドライバから渡されたメモリアドレスからデータを直接読み取り
	hostdrvNT_memread(cpu_kmemoryread_d(hostdrvNT.dataAddr), &invokeInfo.stack, sizeof(NP2_IO_STACK_LOCATION));
	invokeInfo.statusAddr = cpu_kmemoryread_d(hostdrvNT.dataAddr + 4);
	invokeInfo.inBufferAddr = cpu_kmemoryread_d(hostdrvNT.dataAddr + 8);
	invokeInfo.deviceFlags = cpu_kmemoryread_d(hostdrvNT.dataAddr + 12);
	invokeInfo.outBufferAddr = cpu_kmemoryread_d(hostdrvNT.dataAddr + 16);
	invokeInfo.sectionObjectPointerAddr = cpu_kmemoryread_d(hostdrvNT.dataAddr + 20);
	if (hostdrvNT.cmdBaseVersion >= 1)
	{
		hostdrvNT.version = cpu_kmemoryread_d(hostdrvNT.dataAddr + 24);
	}
	else
	{
		hostdrvNT.version = 0;
	}
	if (hostdrvNT.version >= 4)
	{
		s_fsContextUserDataOffset = HOSTDRVNT_FSCONTEXT_USERDATA_OFFSET;
	}
	else
	{
		s_fsContextUserDataOffset = 0;
	}
	if (hostdrvNT.version >= 2)
	{
		// IO待機用
		s_pendingListCount = cpu_kmemoryread_d(hostdrvNT.dataAddr + 28);
		s_pendingIrpListAddr = cpu_kmemoryread_d(hostdrvNT.dataAddr + 32);
		s_pendingAliveListAddr = cpu_kmemoryread_d(hostdrvNT.dataAddr + 36);
		s_pendingIndexOrCompleteCount = cpu_kmemoryread_d(hostdrvNT.dataAddr + 40);
	}
	else
	{
		// 対応していない旧ドライバ
		s_pendingListCount = 0;
	}
	if (hostdrvNT.version >= 3)
	{
		// おぷしょん
		s_hostdrvNTOptions = cpu_kmemoryread_d(hostdrvNT.dataAddr + 44);

		// ホストファイルシステム監視
		hostdrvNT_invokeMonitorChangeFS();
	}
	else
	{
		// 対応していない旧ドライバ
		s_hostdrvNTOptions = HOSTDRVNTOPTIONS_NONE;
	}

	switch (invokeInfo.stack.majorFunction)
	{

	case NP2_IRP_MJ_CREATE:
	{
		TRACEOUTW((L"IRP_MJ_CREATE %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		hostdrvNT_IRP_MJ_CREATE(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_QUERY_VOLUME_INFORMATION:
	{
		TRACEOUTW((L"IRP_MJ_QUERY_VOLUME_INFORMATION: %d (0x%02x) CLASS=%d(0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction, invokeInfo.stack.parameters.queryVolume.fsInformationClass, invokeInfo.stack.parameters.queryVolume.fsInformationClass));
		hostdrvNT_IRP_MJ_QUERY_VOLUME_INFORMATION(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_DIRECTORY_CONTROL:
	{
		TRACEOUTW((L"IRP_MJ_DIRECTORY_CONTROL: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		hostdrvNT_IRP_MJ_DIRECTORY_CONTROL(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_QUERY_INFORMATION:
	{
		TRACEOUTW((L"IRP_MJ_QUERY_INFORMATION: %d(0x%02x) CLASS=%d(0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction, invokeInfo.stack.parameters.queryFile.FileInformationClass, invokeInfo.stack.parameters.queryFile.FileInformationClass));
		hostdrvNT_IRP_MJ_QUERY_INFORMATION(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_SET_INFORMATION:
	{
		TRACEOUTW((L"IRP_MJ_SET_INFORMATION: %d(0x%02x) CLASS=%d(0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction, invokeInfo.stack.parameters.queryFile.FileInformationClass, invokeInfo.stack.parameters.queryFile.FileInformationClass));
		hostdrvNT_IRP_MJ_SET_INFORMATION(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_CLEANUP:
	{
		TRACEOUTW((L"IRP_MJ_CLEANUP: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		hostdrvNT_IRP_MJ_CLEANUP(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_CLOSE:
	{
		TRACEOUTW((L"IRP_MJ_CLOSE: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		hostdrvNT_IRP_MJ_CLOSE(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_READ:
	{
		TRACEOUTW((L"IRP_MJ_READ: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		hostdrvNT_IRP_MJ_READ(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_WRITE:
	{
		TRACEOUTW((L"IRP_MJ_WRITE: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		hostdrvNT_IRP_MJ_WRITE(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_FILE_SYSTEM_CONTROL:
	{
		TRACEOUTW((L"IRP_MJ_FILE_SYSTEM_CONTROL: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		hostdrvNT_IRP_MJ_FILE_SYSTEM_CONTROL(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_DEVICE_CONTROL:
	{
		TRACEOUTW((L"IRP_MJ_DEVICE_CONTROL: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		hostdrvNT_IRP_MJ_DEVICE_CONTROL(&invokeInfo);
		break;
	}
	case NP2_IRP_MJ_LOCK_CONTROL:
	{
		TRACEOUTW((L"IRP_MJ_LOCK_CONTROL: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		cpu_kmemorywrite_d(invokeInfo.statusAddr, NP2_STATUS_SUCCESS); // XXX: 成功したことにする 本当はファイルロックを真面目に作るべき
		cpu_kmemorywrite_d(invokeInfo.statusAddr + 4, 0); // Information
		break;
	}
	case NP2_IRP_MJ_FLUSH_BUFFERS:
	{
		TRACEOUTW((L"IRP_MJ_FLUSH_BUFFERS: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		cpu_kmemorywrite_d(invokeInfo.statusAddr, NP2_STATUS_SUCCESS);
		cpu_kmemorywrite_d(invokeInfo.statusAddr + 4, 0); // Information
		break;
	}
	default:
	{
		TRACEOUTW((L"UNKNOWN IRP_MJ: %d (0x%02x)", invokeInfo.stack.majorFunction, invokeInfo.stack.majorFunction));
		break;
	}

	}

	TRACEOUTW((L"  -> Return Status: 0x%08x", cpu_kmemoryread_d(invokeInfo.statusAddr)));

	if (hostdrvNT.version >= 2)
	{
		cpu_kmemorywrite_d(hostdrvNT.dataAddr + 40, s_pendingIndexOrCompleteCount);
	}
}

// ホストファイルシステム変更の監視専用簡略版
static void hostdrvNT_invokeNotify()
{
	HOSTDRVNT_INVOKEINFO invokeInfo;

	if ((s_hdrvRoot[0] == '¥0') || (!np2cfg.hdrvenable))
	{
		// 無効の場合何もせずに抜ける
		return;
	}

#if defined(SUPPORT_IA32_HAXM)
	// HAXMレジスタを読み取り
	i386haxfunc_vcpu_getREGs(&np2haxstat.state);
	i386haxfunc_vcpu_getFPU(&np2haxstat.fpustate);
	np2haxstat.update_regs = np2haxstat.update_fpu = 0;
	// HAXMレジスタ→猫レジスタにコピー
	ia32hax_copyregHAXtoNP2();
#endif

	// ドライバから渡されたメモリアドレスからデータを直接読み取り
	hostdrvNT.version = cpu_kmemoryread_d(hostdrvNT.dataAddr);
	s_pendingListCount = cpu_kmemoryread_d(hostdrvNT.dataAddr + 4);
	s_pendingIrpListAddr = cpu_kmemoryread_d(hostdrvNT.dataAddr + 8);
	s_pendingAliveListAddr = cpu_kmemoryread_d(hostdrvNT.dataAddr + 12);
	s_pendingIndexOrCompleteCount = cpu_kmemoryread_d(hostdrvNT.dataAddr + 16);

	if (hostdrvNT.version >= 4)
	{
		s_fsContextUserDataOffset = HOSTDRVNT_FSCONTEXT_USERDATA_OFFSET;
	}
	else
	{
		s_fsContextUserDataOffset = 0;
	}

	// ホストファイルシステム監視
	hostdrvNT_invokeMonitorChangeFS();

	cpu_kmemorywrite_d(hostdrvNT.dataAddr + 16, s_pendingIndexOrCompleteCount);
}

// ---------- IO Ports

static void IOOUTCALL hostdrvNT_o7ec(UINT port, REG8 dat)
{

	hostdrvNT.dataAddr = (dat << 24) | (hostdrvNT.dataAddr >> 8);
	(void)port;
}

static void IOOUTCALL hostdrvNT_o7ee(UINT port, REG8 dat)
{
	if (dat == 'H')
	{
		hostdrvNT.cmdInvokePos = 1;
	}
	else if (dat == 'D' && hostdrvNT.cmdInvokePos == 1)
	{
		hostdrvNT.cmdInvokePos++;
	}
	else if (dat == 'R' && hostdrvNT.cmdInvokePos == 2)
	{
		hostdrvNT.cmdInvokePos++;
	}
	else if (dat == '9' && hostdrvNT.cmdInvokePos == 3)
	{
		hostdrvNT.cmdInvokePos++;
	}
	else if (dat == '8' && hostdrvNT.cmdInvokePos == 4)
	{
		hostdrvNT.cmdInvokePos++;
	}
	else if (hostdrvNT.cmdInvokePos == 5)
	{
		if ('0' <= dat && dat <= '9')
		{
			hostdrvNT.cmdBaseVersion = (UINT32)(dat - '0') * 10;
			hostdrvNT.cmdInvokePos++;
		}
		else if (dat == 'M')
		{
			// ファイルシステム監視用呼び出し
			hostdrvNT_invokeNotify();
			hostdrvNT.cmdInvokePos = 0;
		}
	}
	else if ('0' <= dat && dat <= '9' && hostdrvNT.cmdInvokePos == 6)
	{
		hostdrvNT.cmdBaseVersion += (UINT32)(dat - '0');
		if (hostdrvNT.dataAddr)
		{
			// 呼び出し
			hostdrvNT_invoke();
		}
		else
		{
			// リセット
			hostdrvNT_reset();
		}
		hostdrvNT.cmdInvokePos = 0;
	}
	else
	{
		hostdrvNT.cmdInvokePos = 0;
	}
	(void)port;
}

static REG8 IOINPCALL hostdrvNT_i7ec(UINT port)
{
	return(98);
}

static REG8 IOINPCALL hostdrvNT_i7ee(UINT port)
{
	return(21);
}

// System Function

void hostdrvNT_initialize(void)
{
	ZeroMemory(&hostdrvNT, sizeof(hostdrvNT));

	hostdrvNT_updateHDrvRoot();

	TRACEOUT(("hostdrvNT_initialize"));
}

void hostdrvNT_deinitialize(void)
{
	hostdrvNT_stopMonitorChangeFS();
	hostdrvNT_closeAllFiles();

	TRACEOUT(("hostdrv_deinitialize"));
}

// リセットルーチンで呼ぶべし
void hostdrvNT_reset(void)
{
	hostdrvNT_deinitialize();
	hostdrvNT_initialize();
}

void hostdrvNT_bind(void)
{
	if (np2cfg.hdrvntenable)
	{
		iocore_attachout(0x07ec, hostdrvNT_o7ec);
		iocore_attachout(0x07ee, hostdrvNT_o7ee);
		iocore_attachinp(0x07ec, hostdrvNT_i7ec);
		iocore_attachinp(0x07ee, hostdrvNT_i7ee);
	}
}

// ---------- state save

int hostdrvNT_sfsave(STFLAGH sfh, const SFENTRY* tbl)
{
	int	sfVersion = 0;
	int validDataCount = 0;
	int i;
	int	ret;

	ret = statflag_write(sfh, &sfVersion, sizeof(int));
	if (ret != STATFLAG_SUCCESS) return ret;
	ret |= statflag_write(sfh, &hostdrvNT.cmdInvokePos, sizeof(hostdrvNT.cmdInvokePos));
	for (i = 1; i < NP2HOSTDRVNT_FILES_MAX; i++)
	{
		if (hostdrvNT.files[i].fileName != NULL)
		{
			validDataCount++;
		}
	}
	ret |= statflag_write(sfh, &validDataCount, sizeof(validDataCount));
	for (i = 1; i < NP2HOSTDRVNT_FILES_MAX; i++)
	{
		if (hostdrvNT.files[i].fileName != NULL)
		{
			UINT32 fileNameLength = 0;
			UINT32 hostFileNameLength = 0;
			NP2HOSTDRVNT_FILEINFO *fi = &hostdrvNT.files[i];

			statflag_write(sfh, &i, sizeof(i));
			fileNameLength = (wcslen(fi->fileName) + 1) * sizeof(WCHAR);
			statflag_write(sfh, &fileNameLength, sizeof(fileNameLength));
			statflag_write(sfh, fi->fileName, fileNameLength);
			if (fi->hostFileName)
			{
				hostFileNameLength = (wcslen(fi->hostFileName) + 1) * sizeof(WCHAR);
				statflag_write(sfh, &hostFileNameLength, sizeof(hostFileNameLength));
				statflag_write(sfh, fi->hostFileName, hostFileNameLength);
			}
			else
			{
				hostFileNameLength = 0;
				statflag_write(sfh, &hostFileNameLength, sizeof(hostFileNameLength));
			}
			statflag_write(sfh, &fi->isRoot, sizeof(fi->isRoot));
			statflag_write(sfh, &fi->isDirectory, sizeof(fi->isDirectory));
			statflag_write(sfh, &fi->hostdrvWinAPIDesiredAccess, sizeof(fi->hostdrvWinAPIDesiredAccess));
			statflag_write(sfh, &fi->hostdrvShareAccess, sizeof(fi->hostdrvShareAccess));
			statflag_write(sfh, &fi->hostdrvWinAPICreateDisposition, sizeof(fi->hostdrvWinAPICreateDisposition));
			statflag_write(sfh, &fi->hostdrvFileAttributes, sizeof(fi->hostdrvFileAttributes));
			statflag_write(sfh, &fi->deleteOnClose, sizeof(fi->deleteOnClose));
			statflag_write(sfh, &fi->allowDeleteChild, sizeof(fi->allowDeleteChild));
			fi->extendLength = 0;
			statflag_write(sfh, &fi->extendLength, sizeof(fi->extendLength));

			fi->deleteOnClose = 0; // XXX: ステートセーブ後の終了処理でファイル削除が行われないようにする。本当はレジュームではない普通のステートセーブの時はそのままにしなければならない。

			validDataCount++;
		}
	}
	(void)tbl;
	return(ret);
}

int hostdrvNT_sfload(STFLAGH sfh, const SFENTRY* tbl)
{
	int	sfVersion = 0;
	int validDataCount = 0;
	int k;
	int i;
	int	ret;

	hostdrvNT_closeAllFiles();

	ret = statflag_read(sfh, &sfVersion, sizeof(sfVersion));
	if (ret != STATFLAG_SUCCESS) return ret;
	if (sfVersion == 0)
	{
		statflag_read(sfh, &hostdrvNT.cmdInvokePos, sizeof(hostdrvNT.cmdInvokePos));
		statflag_read(sfh, &validDataCount, sizeof(validDataCount));
		for (k = 0; k < validDataCount; k++)
		{
			UINT32 fileNameLength = 0;
			UINT32 hostFileNameLength = 0;
			NP2HOSTDRVNT_FILEINFO* fi;

			statflag_read(sfh, &i, sizeof(i));
			if (i >= NP2HOSTDRVNT_FILES_MAX)
			{
				return STATFLAG_FAILURE;
			}
			fi = &hostdrvNT.files[i];

			statflag_read(sfh, &fileNameLength, sizeof(fileNameLength));
			fi->fileName = (WCHAR*)malloc(fileNameLength);
			statflag_read(sfh, fi->fileName, fileNameLength);

			statflag_read(sfh, &hostFileNameLength, sizeof(hostFileNameLength));
			if (hostFileNameLength > 0)
			{
				fi->hostFileName = (WCHAR*)malloc(hostFileNameLength);
				statflag_read(sfh, fi->hostFileName, hostFileNameLength);
			}

			statflag_read(sfh, &fi->isRoot, sizeof(fi->isRoot));
			statflag_read(sfh, &fi->isDirectory, sizeof(fi->isDirectory));
			statflag_read(sfh, &fi->hostdrvWinAPIDesiredAccess, sizeof(fi->hostdrvWinAPIDesiredAccess));
			statflag_read(sfh, &fi->hostdrvShareAccess, sizeof(fi->hostdrvShareAccess));
			statflag_read(sfh, &fi->hostdrvWinAPICreateDisposition, sizeof(fi->hostdrvWinAPICreateDisposition));
			statflag_read(sfh, &fi->hostdrvFileAttributes, sizeof(fi->hostdrvFileAttributes));
			statflag_read(sfh, &fi->deleteOnClose, sizeof(fi->deleteOnClose));
			statflag_read(sfh, &fi->allowDeleteChild, sizeof(fi->allowDeleteChild));
			statflag_read(sfh, &fi->extendLength, sizeof(fi->extendLength));
			if (fi->extendLength > 0)
			{
				// ダミーリード
				char* dummyBuffer = malloc(fi->extendLength);
				statflag_read(sfh, dummyBuffer, fi->extendLength);
				free(dummyBuffer);
			}
			// ファイルロックがかかると不味いのでここで再オープンはしない
		}
	}
	else
	{
		return(STATFLAG_FAILURE);
	}
	return(ret);
}
#pragma code_seg()

#endif