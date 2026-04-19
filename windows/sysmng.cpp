#include	"compiler.h"
#include	"np2.h"
#include	"dosio.h"
#include	"sysmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"diskimage/fddfile.h"
#include	"fdd/diskdrv.h"
#if defined(SUPPORT_IDEIO)||defined(SUPPORT_SCSI)
#include	"fdd/sxsi.h"
#include	"resource.h"
#include	"win9x/dialog/np2class.h"
#include	"win9x/menu.h"
#endif

#include	<vector>
#include	<algorithm>
#include	<string>
#include	<process.h>

extern UINT8	np2userpause;

extern "C" REG8 cdchange_drv;

	UINT	sys_updates;

	SYSMNGMISCINFO	sys_miscinfo = {0};


// ----

static	UINT8 requestupdate = 0;

static	OEMCHAR	title[2048] = {0};
static	OEMCHAR	clock[256] = {0};
static	OEMCHAR	misc[256] = {0};

// FDDメニューの表示リスト
static char np2_fddmenu_dirname_visible[MAX_FDDFILE] = { 0 };
static OEMCHAR np2_fddmenu[MAX_FDDFILE][FDDMENU_ITEMS_MAX][MAX_PATH] = { 0 };
static char np2_fddmenu_visible[MAX_FDDFILE][FDDMENU_ITEMS_MAX] = { 0 };
static OEMCHAR np2_fddmenu_base[MAX_FDDFILE][MAX_PATH] = { 0 };
static char np2_fddmenu_lastready[MAX_FDDFILE] = { 0 };
static CRITICAL_SECTION	sysmng_findfile_cs = { 0 };

static struct {
	UINT32	tick;
	UINT32	clock;
	UINT32	draws;
	SINT32	fps;
	SINT32	khz;
} workclock;

void sysmng_workclockreset(void) {

	workclock.tick = GETTICK();
	workclock.clock = CPU_CLOCK;
	workclock.draws = drawcount;
}

BOOL sysmng_workclockrenewal(void) {

	SINT32	tick;

	tick = GETTICK() - workclock.tick;
	if (tick < 2000) {
		return(FALSE);
	}
	workclock.tick += tick;
	workclock.fps = ((drawcount - workclock.draws) * 10000) / tick;
	workclock.draws = drawcount;
	workclock.khz = (CPU_CLOCK - workclock.clock) / tick;
	workclock.clock = CPU_CLOCK;
	return(TRUE);
}

OEMCHAR* DOSIOCALL sysmng_file_getname(OEMCHAR* lpPathName){
	if(_tcsnicmp(lpPathName, OEMTEXT("\\\\.\\"), 4)==0){
		return lpPathName;
	}else{
		return file_getname(lpPathName);
	}
}

static bool sortfilefunc(const std::wstring& a, const std::wstring& b) {
	const wchar_t* p1 = a.c_str(), * p2 = b.c_str();

	while (*p1 && *p2) {

		// 両方が数字 → 数値比較
		if (iswdigit(*p1) && iswdigit(*p2)) {
			// 数値部分を抽出
			const wchar_t* b1 = p1;
			const wchar_t* b2 = p2;

			unsigned long long v1 = 0, v2 = 0;

			while (iswdigit(*p1)) {
				v1 = v1 * 10 + (*p1 - L'0');
				p1++;
			}
			while (iswdigit(*p2)) {
				v2 = v2 * 10 + (*p2 - L'0');
				p2++;
			}

			if (v1 != v2)
				return (v1 < v2);

			// 数値が同じ場合 → 桁数比較
			size_t len1 = p1 - b1;
			size_t len2 = p2 - b2;

			if (len1 != len2)
				return (len1 < len2);

			// 数値も長さも同じ → 続行
		}
		else {
			// 数値以外 → 普通に比較
			wchar_t c1 = *p1;
			wchar_t c2 = *p2;

			// 大文字小文字無視
			c1 = towlower(c1);
			c2 = towlower(c2);

			if (c1 != c2) {
				// ピリオドは優先する
				if (c1 == '.') return true;
				if (c2 == '.') return false;

				return (c1 < c2);
			}

			p1++;
			p2++;
		}
	}

	if (*p1 == *p2)
		return false;

	return (*p1 == 0);
}
void sysmng_findfile_Initialize() {
	InitializeCriticalSection(&sysmng_findfile_cs);
}
void sysmng_findfile_Finalize() {
	DeleteCriticalSection(&sysmng_findfile_cs);
}
static void sysmng_findfile_EnterCriticalSection() {
	EnterCriticalSection(&sysmng_findfile_cs);
}
static void sysmng_findfile_LeaveCriticalSection() {
	LeaveCriticalSection(&sysmng_findfile_cs);
}
typedef struct  {
	HANDLE completedEvent;
	TCHAR pattern[MAX_PATH];
	bool isCancel;
	std::vector<std::wstring> files;
} SYSMNG_THREAD_FIND_FILE_ARGS;
static unsigned int __stdcall sysmng_ThreadFuncFindFile(LPVOID vdParam)
{
	SYSMNG_THREAD_FIND_FILE_ARGS* args = (SYSMNG_THREAD_FIND_FILE_ARGS*)vdParam;
	std::vector<std::wstring> files;
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(args->pattern, &fd);
	if (hFind == INVALID_HANDLE_VALUE) {
		goto findfinalize;
	}
	do {
		sysmng_findfile_EnterCriticalSection();
		if (args->isCancel) {
			FindClose(hFind);
			goto findfinalize_incs; // キャンセルの場合
		}
		sysmng_findfile_LeaveCriticalSection();

		if (_tcscmp(fd.cFileName, L".") == 0 || _tcscmp(fd.cFileName, L"..") == 0)
			continue;

		files.push_back(fd.cFileName);
	} while (FindNextFile(hFind, &fd));
	FindClose(hFind);

findfinalize:
	sysmng_findfile_EnterCriticalSection();
findfinalize_incs:
	if (args->isCancel) {
		delete args;
	}
	else {
		args->files.insert(args->files.end(), files.begin(), files.end());
		SetEvent(args->completedEvent);
	}
	sysmng_findfile_LeaveCriticalSection();
	return 0;
}
static void np2updatefddmenu(int drvNo) {
	HMENU hMenu = np2class_gethmenu(g_hWndMain);
	HMENU hMenuTgt;
	int hMenuTgtPos;
	MENUITEMINFO mii = { 0 };
	std::vector<std::wstring> files;
	TCHAR szDirBuf[MAX_PATH];
	TCHAR szFindPatternBuf[MAX_PATH];
	TCHAR* szDiskName;
	int currentItemIndex = 0;
	int beginIndex = 0;
	int i;
	int hasdisk = 0;

	// ドライブ自体がなければ何もしない
	if (!(np2cfg.fddequip & (1 << drvNo))) return;
	
	if (!fdd_diskready(drvNo)) {
		// readyでないなら最後に開いたFDパスを使用
		szDiskName = fddfolder;

		// ファイルパスが変わっていなかったらリスト更新不要なので抜ける
		if (_tcscmp(np2_fddmenu_base[drvNo], szDiskName) == 0) {
			// 前回挿入状態だった場合、チェックは全て外してreturn
			if (np2_fddmenu_lastready[drvNo]) {
				for (i = 0; i < FDDMENU_ITEMS_MAX; i++) {
					if (np2_fddmenu_visible[drvNo][i]) {
						CheckMenuItem(hMenu, IDM_FDD1_LIST_ID0 + FDDMENU_ITEMS_MAX * drvNo + i, MF_BYCOMMAND | MF_UNCHECKED);
					}
				}
			}
			np2_fddmenu_lastready[drvNo] = hasdisk;
			return;
		}
	}
	else {
		// readyならそのファイルのパスを使用
		szDiskName = fdd_diskname(drvNo);
		hasdisk = 1;

		// ファイルパスが変わっていなかったらリスト更新不要なので抜ける
		if (_tcscmp(np2_fddmenu_base[drvNo], szDiskName) == 0 && np2_fddmenu_lastready[drvNo]) {
			np2_fddmenu_lastready[drvNo] = hasdisk;
			return;
		}
	}

	// \以降を削除
	_tcscpy(szDirBuf, szDiskName);
	TCHAR* sepaChar = _tcsrchr(szDirBuf, '\\');
	if (sepaChar) {
		*sepaChar = '\0';
	}
	_tcscpy(szFindPatternBuf, szDirBuf);
	// 拡張子一致を検索条件とする
	sepaChar = _tcsrchr(szDiskName, '\\');
	TCHAR* extChar = _tcsrchr(szDiskName, '.');
	if (extChar && sepaChar < extChar) {
		_tcscat(szFindPatternBuf, _T("\\*"));
		_tcscat(szFindPatternBuf, extChar);
	}
	else {
		// 拡張子とれなかったので諦める
		return;
	}

	if (!menu_searchmenu(hMenu, IDM_FDD1EJECT + drvNo, &hMenuTgt, &hMenuTgtPos)) return;

	// メニューを一旦全部消す
	for (i = 0; i < FDDMENU_ITEMS_MAX; i++) {
		if (np2_fddmenu_visible[drvNo][i]) {
			DeleteMenu(hMenuTgt, IDM_FDD1_LIST_ID0 + FDDMENU_ITEMS_MAX * drvNo + i, MF_BYCOMMAND);
			np2_fddmenu_visible[drvNo][i] = 0;
		}
	}
	if (np2_fddmenu_dirname_visible[drvNo]) {
		DeleteMenu(hMenuTgt, IDM_FDD1_LIST_DIRNAME + drvNo, MF_BYCOMMAND);
		np2_fddmenu_dirname_visible[drvNo] = 0;
	}

	// ものすごく時間がかかってしまう場合のために短めのタイムアウトを設ける
	SYSMNG_THREAD_FIND_FILE_ARGS* args = new SYSMNG_THREAD_FIND_FILE_ARGS;
	args->completedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	args->isCancel = false;
	_tcscpy(args->pattern, szFindPatternBuf);
	unsigned int dwID;
	HANDLE hThreadFind = (HANDLE)_beginthreadex(NULL, 0, sysmng_ThreadFuncFindFile, args, 0, &dwID);
	if (WaitForSingleObject(args->completedEvent, 3000) != WAIT_OBJECT_0) {
		sysmng_findfile_EnterCriticalSection();
		if (WaitForSingleObject(args->completedEvent, 0) != WAIT_OBJECT_0) {
			// CS内で完了していないならキャンセルフラグを立ててスレッドに破棄を任せる
			CloseHandle(hThreadFind); // スレッドハンドルは閉じる（もう用事が無いので）
			args->isCancel = true;
			sysmng_findfile_LeaveCriticalSection();

			// タイムアウトしたときもリスト取得したファイルパスを記憶しておく（次回は見に行かない）
			_tcscpy(np2_fddmenu_base[drvNo], szDiskName);
			np2_fddmenu_lastready[drvNo] = hasdisk;

			return;
		}
		// 結果的に完了したので続行
		sysmng_findfile_LeaveCriticalSection();
	}
	files.insert(files.end(), args->files.begin(), args->files.end());
	sysmng_findfile_EnterCriticalSection();
	delete args;
	sysmng_findfile_LeaveCriticalSection();
	CloseHandle(hThreadFind); // スレッドハンドルは閉じる（もう用事が無いので）

	// sort
	std::sort(files.begin(), files.end(), sortfilefunc);

	// 現在のファイルの位置を探す
	auto it = std::find(files.begin(), files.end(), file_getname(szDiskName));
	if (it == files.end()) {
		currentItemIndex = -1;
		beginIndex = 0; // 見つからない場合
	}
	else {
		currentItemIndex = static_cast<int>(std::distance(files.begin(), it));
		beginIndex = currentItemIndex - FDDMENU_ITEMS_MAX / 2;
		if (beginIndex + FDDMENU_ITEMS_MAX > files.size()) {
			beginIndex = files.size() - FDDMENU_ITEMS_MAX;
		}
		if (beginIndex < 0) beginIndex = 0;
	}

	// ディレクトリ名表示
	sepaChar = _tcsrchr(szDirBuf, '\\');
	if (sepaChar == NULL) {
		sepaChar = szDirBuf;
	}
	TCHAR mnuDirText[MAX_PATH] = { 0 };
	_tcscpy(mnuDirText, _T(""));
	_tcscat(mnuDirText, sepaChar);
	InsertMenu(hMenuTgt, -1, MF_BYPOSITION, IDM_FDD1_LIST_DIRNAME + drvNo, mnuDirText);
	EnableMenuItem(hMenuTgt, IDM_FDD1_LIST_DIRNAME + drvNo, MF_GRAYED);
	np2_fddmenu_dirname_visible[drvNo] = 1;

	// メニューに登録 最大で20ファイルまで
	i = 0;
	for (size_t fi = beginIndex; fi < files.size(); fi++) {
		TCHAR mnuText[MAX_PATH] = { 0 };
		_tcscpy(np2_fddmenu[drvNo][i], szDirBuf);
		_tcscat(np2_fddmenu[drvNo][i], _T("\\"));
		_tcscat(np2_fddmenu[drvNo][i], files[fi].c_str());
		_tcscpy(mnuText, files[fi].c_str());
		InsertMenu(hMenuTgt, -1, MF_BYPOSITION, IDM_FDD1_LIST_ID0 + FDDMENU_ITEMS_MAX * drvNo + i, mnuText);
		if (hasdisk && fi == currentItemIndex) {
			CheckMenuItem(hMenu, IDM_FDD1_LIST_ID0 + FDDMENU_ITEMS_MAX * drvNo + i, MF_BYCOMMAND | MF_CHECKED);
		}
		np2_fddmenu_visible[drvNo][i] = 1;
		i++;
		if (i == FDDMENU_ITEMS_MAX) break;
	}

	// リスト取得したファイルパスを記憶しておく（次回省略のため）
	_tcscpy(np2_fddmenu_base[drvNo], szDiskName);

	// 現在のReady状態を保存
	np2_fddmenu_lastready[drvNo] = hasdisk;
}

void sysmng_updatecaption(UINT8 flag) {
	
#if defined(SUPPORT_IDEIO)||defined(SUPPORT_SCSI)
	int i, cddrvnum = 1;
#endif
	static OEMCHAR hddimgmenustrorg[4][MAX_PATH] = {0};
	static OEMCHAR hddimgmenustr[4][MAX_PATH] = {0};
#if defined(SUPPORT_SCSI)
	static OEMCHAR scsiimgmenustrorg[4][MAX_PATH] = {0};
	static OEMCHAR scsiimgmenustr[4][MAX_PATH] = {0};
#endif
	OEMCHAR	work[2048] = {0};
	OEMCHAR	fddtext[16] = {0};
	
	if (flag & 1) {
		title[0] = '\0';
		for(i=0;i<4;i++){
			OEMSPRINTF(fddtext, OEMTEXT("  FDD%d:"), i+1);
			if (fdd_diskready(i)) {
				milstr_ncat(title, fddtext, NELEMENTS(title));
				milstr_ncat(title, file_getname(fdd_diskname(i)), NELEMENTS(title));
			}
			if (np2oscfg.dirfdlst) {
				np2updatefddmenu(i);
			}
		}
#ifdef SUPPORT_IDEIO
		for(i=0;i<4;i++){
			if(sxsi_getdevtype(i)==SXSIDEV_CDROM){
				OEMSPRINTF(work, OEMTEXT("  CD%d:"), cddrvnum);
				if (sxsi_getdevtype(i)==SXSIDEV_CDROM){
					if(*(np2cfg.idecd[i])) {
						milstr_ncat(title, work, NELEMENTS(title));
						milstr_ncat(title, sysmng_file_getname(np2cfg.idecd[i]), NELEMENTS(title));
					}else if(i==cdchange_drv && g_nevent.item[NEVENT_CDWAIT].clock > 0){
						milstr_ncat(title, work, NELEMENTS(title));
						milstr_ncat(title, OEMTEXT("Now Loading..."), NELEMENTS(title));
					}
				}
				cddrvnum++;
			}
			if(g_hWndMain){
				OEMCHAR newtext[MAX_PATH*2+100];
				OEMCHAR *fname;
				OEMCHAR *fnamenext;
				OEMCHAR *fnametmp;
				OEMCHAR *fnamenexttmp;
				HMENU hMenu = np2class_gethmenu(g_hWndMain);
				HMENU hMenuTgt;
				int hMenuTgtPos;
				MENUITEMINFO mii = {0};
				menu_searchmenu(hMenu, IDM_IDE0STATE+i, &hMenuTgt, &hMenuTgtPos);
				if(hMenu){
					mii.cbSize = sizeof(MENUITEMINFO);
					if(!hddimgmenustrorg[i][0]){
						GetMenuString(hMenuTgt, IDM_IDE0STATE+i, hddimgmenustrorg[i], NELEMENTS(hddimgmenustrorg[0]), MF_BYCOMMAND);
					}
					if(np2cfg.idetype[i]==SXSIDEV_NC){
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[disabled]"));
					}else{
						fname = sxsi_getfilename(i);
						if(np2cfg.idetype[i]==SXSIDEV_CDROM){
							fnamenext = np2cfg.idecd[i];
						}else{
							fnamenext = (OEMCHAR*)diskdrv_getsxsi(i);
						}
						if(fname && *fname && fnamenext && *fnamenext && (fnametmp = sysmng_file_getname(fname))!=NULL && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
							_tcscpy(newtext, hddimgmenustrorg[i]);
							_tcscat(newtext, fnametmp);
							if(_tcscmp(fname, fnamenext)){
								_tcscat(newtext, OEMTEXT(" -> "));
								_tcscat(newtext, fnamenexttmp);
							}
						}else if(fnamenext && *fnamenext && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
							_tcscpy(newtext, hddimgmenustrorg[i]);
							_tcscat(newtext, OEMTEXT("[none] -> "));
							_tcscat(newtext, fnamenexttmp);
						}else if(fname && *fname && (fnametmp = sysmng_file_getname(fname))!=NULL){
							_tcscpy(newtext, hddimgmenustrorg[i]);
							_tcscat(newtext, fnametmp);
							_tcscat(newtext, OEMTEXT(" -> [none]"));
						}else{
							_tcscpy(newtext, hddimgmenustrorg[i]);
							_tcscat(newtext, OEMTEXT("[none]"));
						}
					}
					if(_tcscmp(newtext, hddimgmenustr[i])){
						_tcscpy(hddimgmenustr[i], newtext);
						mii.fMask = MIIM_TYPE;
						mii.fType = MFT_STRING;
						mii.dwTypeData = hddimgmenustr[i];
						mii.cch = (UINT)_tcslen(hddimgmenustr[i]);
						SetMenuItemInfo(hMenuTgt, IDM_IDE0STATE+i, MF_BYCOMMAND, &mii);
					}
				}
			}
		}
#else
		for(i=0;i<2;i++){
			if(g_hWndMain){
				OEMCHAR newtext[MAX_PATH*2+100];
				OEMCHAR *fname;
				OEMCHAR *fnamenext;
				OEMCHAR *fnametmp;
				OEMCHAR *fnamenexttmp;
				HMENU hMenu = np2class_gethmenu(g_hWndMain);
				HMENU hMenuTgt;
				int hMenuTgtPos;
				MENUITEMINFO mii = {0};
				menu_searchmenu(hMenu, IDM_IDE0STATE+i, &hMenuTgt, &hMenuTgtPos);
				if(hMenu){
					mii.cbSize = sizeof(MENUITEMINFO);
					if(!hddimgmenustrorg[i][0]){
						GetMenuString(hMenuTgt, IDM_IDE0STATE+i, hddimgmenustrorg[i], NELEMENTS(hddimgmenustrorg[0]), MF_BYCOMMAND);
					}
					fname = sxsi_getfilename(i);
					fnamenext = (OEMCHAR*)diskdrv_getsxsi(i);
					if(fname && *fname && fnamenext && *fnamenext && (fnametmp = sysmng_file_getname(fname))!=NULL && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, fnametmp);
						if(_tcscmp(fname, fnamenext)){
							_tcscat(newtext, OEMTEXT(" -> "));
							_tcscat(newtext, fnamenexttmp);
						}
					}else if(fnamenext && *fnamenext && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[none] -> "));
						_tcscat(newtext, fnamenexttmp);
					}else if(fname && *fname && (fnametmp = sysmng_file_getname(fname))!=NULL){
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, fnametmp);
						_tcscat(newtext, OEMTEXT(" -> [none]"));
					}else{
						_tcscpy(newtext, hddimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[none]"));
					}
					if(_tcscmp(newtext, hddimgmenustr[i])){
						_tcscpy(hddimgmenustr[i], newtext);
						mii.fMask = MIIM_TYPE;
						mii.fType = MFT_STRING;
						mii.dwTypeData = hddimgmenustr[i];
						mii.cch = (UINT)_tcslen(hddimgmenustr[i]);
						SetMenuItemInfo(hMenuTgt, IDM_IDE0STATE+i, MF_BYCOMMAND, &mii);
					}
				}
			}
		}
#endif
#ifdef SUPPORT_SCSI
		for(i=0;i<4;i++){
			if(g_hWndMain){
				OEMCHAR newtext[MAX_PATH*2+100];
				OEMCHAR *fname;
				OEMCHAR *fnamenext;
				OEMCHAR *fnametmp;
				OEMCHAR *fnamenexttmp;
				HMENU hMenu = np2class_gethmenu(g_hWndMain);
				HMENU hMenuTgt;
				int hMenuTgtPos;
				MENUITEMINFO mii = {0};
				menu_searchmenu(hMenu, IDM_SCSI0STATE+i, &hMenuTgt, &hMenuTgtPos);
				if(hMenu){
					mii.cbSize = sizeof(MENUITEMINFO);
					if(!scsiimgmenustrorg[i][0]){
						GetMenuString(hMenuTgt, IDM_SCSI0STATE+i, scsiimgmenustrorg[i], NELEMENTS(scsiimgmenustrorg[0]), MF_BYCOMMAND);
					}
					fname = sxsi_getfilename(i+0x20);
					fnamenext = (OEMCHAR*)diskdrv_getsxsi(i+0x20);
					if(fname && *fname && fnamenext && *fnamenext && (fnametmp = sysmng_file_getname(fname))!=NULL && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
						_tcscpy(newtext, scsiimgmenustrorg[i]);
						_tcscat(newtext, fnametmp);
						if(_tcscmp(fname, fnamenext)){
							_tcscat(newtext, OEMTEXT(" -> "));
							_tcscat(newtext, fnamenexttmp);
						}
					}else if(fnamenext && *fnamenext && (fnamenexttmp = sysmng_file_getname(fnamenext))!=NULL){
						_tcscpy(newtext, scsiimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[none] -> "));
						_tcscat(newtext, fnamenexttmp);
					}else if(fname && *fname && (fnametmp = sysmng_file_getname(fname))!=NULL){
						_tcscpy(newtext, scsiimgmenustrorg[i]);
						_tcscat(newtext, fnametmp);
						_tcscat(newtext, OEMTEXT(" -> [none]"));
					}else{
						_tcscpy(newtext, scsiimgmenustrorg[i]);
						_tcscat(newtext, OEMTEXT("[none]"));
					}
					if(_tcscmp(newtext, scsiimgmenustr[i])){
						_tcscpy(scsiimgmenustr[i], newtext);
						mii.fMask = MIIM_TYPE;
						mii.fType = MFT_STRING;
						mii.dwTypeData = scsiimgmenustr[i];
						mii.cch = (UINT)_tcslen(scsiimgmenustr[i]);
						SetMenuItemInfo(hMenuTgt, IDM_SCSI0STATE+i, MF_BYCOMMAND, &mii);
					}
				}
			}
		}
#endif
	}
	
	if (flag & 2) {
		clock[0] = '\0';
		if (np2oscfg.DISPCLK & 2) {
			if (workclock.fps) {
				OEMSPRINTF(clock, OEMTEXT(" - %u.%1uFPS"),
									workclock.fps / 10, workclock.fps % 10);
			}
			else {
				milstr_ncpy(clock, OEMTEXT(" - 0FPS"), NELEMENTS(clock));
			}
		}
		if (!np2userpause && (np2oscfg.DISPCLK & 1)) {
			OEMSPRINTF(work, OEMTEXT(" %2u.%03uMHz"),
								workclock.khz / 1000, workclock.khz % 1000);
			if (clock[0] == '\0') {
				milstr_ncpy(clock, OEMTEXT(" -"), NELEMENTS(clock));
			}
			milstr_ncat(clock, work, NELEMENTS(clock));
#if 0
			OEMSPRINTF(work, OEMTEXT(" (debug: OPN %d / PSG %s)"),
							opngen.playing,
							(g_psg1.mixer & 0x3f)?OEMTEXT("ON"):OEMTEXT("OFF"));
			milstr_ncat(clock, work, NELEMENTS(clock));
#endif
		}
	}
	
	if (flag & 4) {
		misc[0] = '\0';
		if(sys_miscinfo.showvolume && sys_miscinfo.showmousespeed){
			OEMSPRINTF(misc, OEMTEXT(" (Volume: %d%%, Mouse speed: %d%%)"), np2cfg.vol_master, 100 * np2oscfg.mousemul/np2oscfg.mousediv);
		}else if(sys_miscinfo.showvolume){
			OEMSPRINTF(misc, OEMTEXT(" (Volume: %d%%)"), np2cfg.vol_master);
		}else if(sys_miscinfo.showmousespeed){
			OEMSPRINTF(misc, OEMTEXT(" (Mouse speed: %d%%)"), 100 * np2oscfg.mousemul/np2oscfg.mousediv);
		}
	}

	milstr_ncpy(work, np2oscfg.titles, NELEMENTS(work));
	milstr_ncat(work, misc, NELEMENTS(work));
	if(np2userpause){
		milstr_ncat(work, OEMTEXT(" [PAUSED]"), NELEMENTS(work));
	}
	milstr_ncat(work, title, NELEMENTS(work));
	milstr_ncat(work, clock, NELEMENTS(work));
	SetWindowText(g_hWndMain, work);
}

void sysmng_requestupdatecaption(UINT8 flag) {
	requestupdate |= flag; // マルチスレッド呼び出し対策･･･
}
void sysmng_requestupdatecheck() {
	if(requestupdate != 0){
		sysmng_updatecaption(requestupdate);
		requestupdate = 0;	
	};
}

OEMCHAR* sysmng_getfddlistitem(int drv, int index){
	if (!np2_fddmenu_visible[drv][index]) return NULL;
	return np2_fddmenu[drv][index];
}
OEMCHAR* sysmng_getlastfddlistitem(int drv) {
	return np2_fddmenu_base[drv];
}