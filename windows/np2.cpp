/**
 * @file	np2.cpp
 * @brief	main window
 *
 * @author	$Author: yui $
 * @date	$Date: 2011/02/17 10:36:05 $
 */

#include "compiler.h"

// Win2000で動くようにする
#if defined(SUPPORT_WIN2000HOST)
#ifdef _WINDOWS
#ifndef _WIN64
#define WINVER2 0x0500
#include "commonfix.h"
#endif
#endif
#endif

#include <time.h>
#include <winsock.h>
#ifndef __GNUC__
#include <winnls32.h>
#endif
#include "resource.h"
#include "strres.h"
#include "parts.h"
#include "np2.h"
#include "np2mt.h"
#include "misc\WndProc.h"
#include "debuguty\viewer.h"
#include "np2arg.h"
#include "dosio.h"
#include "misc\tstring.h"
#include "commng.h"
#include "commng\cmmidiin32.h"
#if defined(SUPPORT_VSTi)
#include "commng\vsthost\vsteditwnd.h"
#endif	// defined(SUPPORT_VSTi)
#include "joymng.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "sysmng.h"
#include "winkbd.h"
#include "ini.h"
#include "menu.h"
#include "winloc.h"
#include "dialog\np2class.h"
#include "dialog\dialog.h"
#include "cpucore.h"
#include "pccore.h"
#include "statsave.h"
#include "iocore.h"
#include "pc9861k.h"
#include "mpu98ii.h"
#if defined(SUPPORT_SMPU98)
#include "smpu98.h"
#endif
#include "scrndraw.h"
#include "sound.h"
#include "beep.h"
#include "s98.h"
#include "fdd/diskdrv.h"
#include "diskimage/fddfile.h"
#include "timing.h"
#include "keystat.h"
#include "debugsub.h"
#include "subwnd/kdispwnd.h"
#include "subwnd/mdbgwnd.h"
#include "subwnd/skbdwnd.h"
#include "subwnd/subwnd.h"
#include "subwnd/toolwnd.h"
#include "bmpdata.h"
#include "vram/scrnsave.h"
#include "fdd/sxsi.h"
#if !defined(_WIN64)
#include "cputype.h"
#endif
#if defined(SUPPORT_DCLOCK)
#include "subwnd\dclock.h"
#endif
#include "recvideo.h"
#if defined(SUPPORT_IDEIO)
#include "ideio.h"
#endif
#if defined(SUPPORT_NET)
#include "network/net.h"
#endif
#if defined(SUPPORT_WAB)
#include "wab/wab.h"
#include "wab/wabbmpsave.h"
#endif
#if defined(SUPPORT_CL_GD5430)
#include "wab/cirrus_vga_extern.h"
#endif
#include "fmboard.h"
#include "pcm86.h"
#if defined(SUPPORT_PHYSICAL_CDDRV)
#include "Dbt.h"
#endif

#if defined(SUPPORT_IA32_HAXM)
#include	"i386hax/haxfunc.h"
#include	"i386hax/haxcore.h"
#endif
#include	<process.h>

extern bool scrnmng_create_pending; // グラフィックレンダラ生成保留中


#ifdef SUPPORT_WACOM_TABLET
void cmwacom_setNCControl(bool enable);
#endif

#ifdef BETA_RELEASE
#define		OPENING_WAIT		1500
#endif

static	TCHAR		szClassName[] = _T("NP2-MainWindow");
		HWND		g_hWndMain;
		HINSTANCE	g_hInstance;
#if !defined(_WIN64)
		int			mmxflag;
#endif
		UINT8		np2break = 0;									// ver0.30
		BOOL		winui_en;
		UINT8		g_scrnmode;

		NP2OSCFG	np2oscfg = {
						OEMTEXT(PROJECTNAME) OEMTEXT(PROJECTSUBNAME),
						OEMTEXT("NP2"),
						CW_USEDEFAULT, CW_USEDEFAULT, 1, 1, 0, 0, 0, 1, 0, 1,
						0, 1, KEY_UNKNOWN, 0, 0,
						0, 0, 0, {1, 2, 2, 1}, {1, 2, 2, 1}, 0, 1, 0, 0,
						{5, 0, 0x3e, 19200,
						 OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), 0, 1, OEMTEXT(""), 5000,
#if defined(SUPPORT_NAMED_PIPE)
						 OEMTEXT("NP2-NamedPipe"), OEMTEXT("."),
#endif
						 OEMTEXT(""), 5000, 0, 130, 1, 0, 0, 0, 100
						},
#if defined(SUPPORT_SMPU98)
						{5, 0, 0x3e, 19200,
						 OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), 0, 1, OEMTEXT(""), 5000,
#if defined(SUPPORT_NAMED_PIPE)
						 OEMTEXT("NP2-NamedPipe"), OEMTEXT("."),
#endif
						 OEMTEXT(""), 5000, 0, 130, 1, 0, 0, 0, 100
						},
						{5, 0, 0x3e, 19200,
						 OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), 0, 1, OEMTEXT(""), 5000,
#if defined(SUPPORT_NAMED_PIPE)
						 OEMTEXT("NP2-NamedPipe"), OEMTEXT("."),
#endif
						 OEMTEXT(""), 5000, 0, 130, 1, 0, 0, 0, 100
						},
#endif
						{0, 0, 0x3e, 19200,
						 OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), 0, 1, OEMTEXT(""), 5000,
#if defined(SUPPORT_NAMED_PIPE)
						 OEMTEXT("NP2-NamedPipe"), OEMTEXT("."),
#endif
						 OEMTEXT(""), 5000, 0, 130, 1, 0, 0, 0, 100
						},
						{0, 0, 0x3e, 19200,
						 OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), 0, 1, OEMTEXT(""), 5000,
#if defined(SUPPORT_NAMED_PIPE)
						 OEMTEXT("NP2-NamedPipe"), OEMTEXT("."),
#endif
						 OEMTEXT(""), 5000, 0, 130, 1, 0, 0, 0, 100
						},
						{0, 0, 0x3e, 19200,
						 OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), 0, 1, OEMTEXT(""), 5000,
#if defined(SUPPORT_NAMED_PIPE)
						 OEMTEXT("NP2-NamedPipe"), OEMTEXT("."),
#endif
						 OEMTEXT(""), 5000, 0, 130, 1, 0, 0, 0, 100
						},
						{0, 0, 0x3e, 19200,
						 OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), 0, 1, OEMTEXT(""), 5000,
#if defined(SUPPORT_NAMED_PIPE)
						 OEMTEXT("NP2-NamedPipeLPT"), OEMTEXT("."),
#endif
						 OEMTEXT(""), 5000, 1, 130, 1, 0, 0, 0, 100
						},
						0xffffff, 0xffbf6a, 0, 0,
						0, 1,
						0, 0,
#if !defined(_WIN64)
						0,
#endif
						0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, FSCRNMOD_SAMEBPP | FSCRNMOD_SAMERES | FSCRNMOD_ASPECTFIX8, 0,

#if defined(SUPPORT_SCRN_DIRECT3D)
						0, 0,
#endif

						CSoundMng::kDSound3, TEXT(""),

#if defined(SUPPORT_VSTi)
						TEXT("%ProgramFiles%\\Roland\\Sound Canvas VA\\SOUND Canvas VA.dll"),
#endif	// defined(SUPPORT_VSTi)
						0, 
						DRAWTYPE_DIRECTDRAW_HW, 
						0, 0, 1, 0, 1, 1, 
						0, 0, 
						0, 8, 
						1, 0, 0, 0, TCMODE_DEFAULT, 0, 100,
						0,
#if defined(SUPPORT_WACOM_TABLET)
						0,
#endif	// defined(SUPPORT_WACOM_TABLET)
#if defined(SUPPORT_MULTITHREAD)
						1,
#endif	// defined(SUPPORT_MULTITHREAD)
						0, 200,
						1, 0, 1, 0,
						OEMTEXT(""), OEMTEXT(""), 1,
						{2, 8, 16, 30, 42, 52, 62},
						{50, 75, 100, 150, 200, 400, 800}
					};

		OEMCHAR		fddfolder[MAX_PATH];
		OEMCHAR		hddfolder[MAX_PATH];
		OEMCHAR		cdfolder[MAX_PATH];
		OEMCHAR		bmpfilefolder[MAX_PATH];
		OEMCHAR		npcfgfilefolder[MAX_PATH];
		OEMCHAR		modulefile[MAX_PATH];


static	UINT		framecnt = 0;
static	UINT		framecntUI = 0;
static	UINT		waitcnt = 0;
static	UINT		framemax = 1;
static	UINT8		np2stopemulate = 0;
		UINT8		np2userpause = 0;
static	int			np2opening = 1;
static	int			np2quitmsg = 0;
static	WINLOCEX	smwlex;
static	HMODULE		s_hModResource;
static  UINT		lateframecount; // フレーム遅れ数
static  int			mousecapturemode = 0;
static  int			screenChanging = 0;

static void np2_SetUserPause(UINT8 pause){
	if(np2userpause && !pause){
		CSoundMng::GetInstance()->Enable(SNDPROC_USER);
	}else if(!np2userpause && pause){
		CSoundMng::GetInstance()->Disable(SNDPROC_USER);
	}
	np2userpause = pause;
}

static void np2_DynamicChangeClockMul(int newClockMul) {
	UINT8 oldclockmul = pccore.maxmultiple;
	UINT8 oldclockmult = pccore.multiple;

	pccore.multiple = newClockMul;
	pccore.maxmultiple = newClockMul;
	pccore.realclock = pccore.baseclock * pccore.multiple;
		
	pcm86_changeclock(oldclockmult);
	sound_changeclock();
	beep_changeclock();
	mpu98ii_changeclock();
#if defined(SUPPORT_SMPU98)
	smpu98_changeclock();
#endif
	keyboard_changeclock();
	mouseif_changeclock();
	gdc_updateclock();
}

static const OEMCHAR np2help[] = OEMTEXT("np2.chm");
static const OEMCHAR np2flagext[] = OEMTEXT("S%02d");
#if defined(_WIN64)
static const OEMCHAR szNp2ResDll[] = OEMTEXT("np2x64_%u.dll");
#else	// defined(_WIN64)
static const OEMCHAR szNp2ResDll[] = OEMTEXT("np2_%u.dll");
#endif	// defined(_WIN64)

// ASCII -> 98キーコード表(np21w ver0.86 rev22)
char vkeylist[256] = {0};
char shift_on[256] = {0};

// マルチスレッド用
#if defined(SUPPORT_MULTITHREAD)
static int np2_multithread_requestswitch = 0; // マルチスレッドモード切替要求フラグ
static int np2_multithread_enable = 0; // マルチスレッドモード有効フラグ
static BOOL np2_multithread_initialized = 0; // マルチスレッドモード初期化済みフラグ
static HANDLE	np2_multithread_hThread = NULL; // エミュレーション用スレッド
static CRITICAL_SECTION	np2_multithread_hThread_cs = {0}; // エミュレーション用スレッド　クリティカルセクション
static BOOL	np2_multithread_hThread_requestexit = FALSE; // エミュレーション用スレッド　終了要求フラグ
static bool np2_multithread_pauseemulation = false; // エミュレーション一時停止フラグ
static bool np2_multithread_pausing = false; // エミュレーション一時停止中フラグ
static void np2_multithread_Initialize(){
	if(!np2_multithread_initialized){
		InitializeCriticalSection(&np2_multithread_hThread_cs);
		np2_multithread_initialized = TRUE;
	}
}
static void np2_multithread_fakeWndProc(){
	// グラフィック周りがWndProcが処理される前提になっているので無理やり
	MSG msg;
	if(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
	{
		if(msg.message != WM_QUIT && msg.message != WM_CLOSE && msg.message != WM_DESTROY){
			GetMessage(&msg, NULL, 0, 0);
			if ((msg.hwnd != g_hWndMain) ||
				((msg.message != WM_SYSKEYDOWN) &&
				(msg.message != WM_SYSKEYUP))) {
				TranslateMessage(&msg);
			}
			DispatchMessage(&msg);
		}
	}
}
void np2_multithread_Suspend(){
	if(!np2_multithread_pauseemulation){
		np2_multithread_pauseemulation = true;
		if(np2_multithread_initialized){
			if(np2_multithread_enable && np2_multithread_hThread){
				int workaroundCounter = 0;
				Sleep(10);
				while(!np2_multithread_hThread_requestexit && np2_multithread_hThread && !np2_multithread_pausing){
					if(workaroundCounter >= 30){
						np2_multithread_fakeWndProc();
					}else{
						workaroundCounter++;
					}
					Sleep(10);
				}
			}
		}
	}
}
void np2_multithread_Resume(){
	if(np2_multithread_pauseemulation){
		np2_multithread_pauseemulation = false;
		if(np2_multithread_initialized){
			if(np2_multithread_enable && np2_multithread_hThread){
				int workaroundCounter = 0;
				Sleep(10);
				while(!np2_multithread_hThread_requestexit && np2_multithread_hThread && np2_multithread_pausing){
					if(workaroundCounter >= 30){
						np2_multithread_fakeWndProc();
					}else{
						workaroundCounter++;
					}
					Sleep(10);
				}
			}
		}
	}
}
static void np2_multithread_WaitForExitThread(){
	if(np2_multithread_initialized){
		if(np2_multithread_hThread){
			np2_multithread_Suspend();
			np2_multithread_hThread_requestexit = TRUE;
			if(WaitForSingleObject(np2_multithread_hThread, 20000) == WAIT_TIMEOUT){
				TerminateThread(np2_multithread_hThread, 0);
			}
			CloseHandle(np2_multithread_hThread);
			np2_multithread_hThread = NULL;
			np2_multithread_pauseemulation = false;
		}
	}
}
static void np2_multithread_Finalize(){
	if(np2_multithread_initialized){
		np2_multithread_WaitForExitThread();
		DeleteCriticalSection(&np2_multithread_hThread_cs);
		np2_multithread_initialized = FALSE;
	}
}
void np2_multithread_EnterCriticalSection(){
	if(np2_multithread_initialized && np2_multithread_enable){
		EnterCriticalSection(&np2_multithread_hThread_cs);
	}
}
void np2_multithread_LeaveCriticalSection(){
	if(np2_multithread_initialized && np2_multithread_enable){
		LeaveCriticalSection(&np2_multithread_hThread_cs);
	}
}
int np2_multithread_Enabled(){
	return np2_multithread_initialized && np2_multithread_enable;
}
#else
void np2_multithread_Suspend(){
	// nothing to do
}
void np2_multithread_Resume(){
	// nothing to do
}
void np2_multithread_EnterCriticalSection(){
	// nothing to do
}
void np2_multithread_LeaveCriticalSection(){
	// nothing to do
}
int np2_multithread_Enabled(){
	return 0;
}
#endif

// コピペ用(np21w ver0.86 rev22)
char *autokey_sendbuffer = NULL;
int autokey_sendbufferlen = 0;
int autokey_sendbufferpos = 0;
int autokey_lastkanastate = 0;

// オートラン抑制用
static int WM_QueryCancelAutoPlay;

// システムキーフック用
#ifdef HOOK_SYSKEY
static HANDLE	np2_hThreadKeyHook = NULL; // キーフック用スレッド
static int		np2_hThreadKeyHookexit = 0; // スレッド終了フラグ
static HWND		np2_hThreadKeyHookhWnd = 0;
LRESULT CALLBACK LowLevelKeyboardProc(INT nCode, WPARAM wParam, LPARAM lParam);
HHOOK hHook = NULL;
LRESULT CALLBACK np2_ThreadFuncKeyHook_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg){
	case WM_CLOSE:
		if(!np2_hThreadKeyHookexit) return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
static unsigned int __stdcall np2_ThreadFuncKeyHook(LPVOID vdParam) 
{
	MSG msg;
	LPCTSTR wndclassname = _T("NP2 Key Hook");

	WNDCLASSEX wcex ={sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, np2_ThreadFuncKeyHook_WndProc, 0, 0, g_hInstance, NULL, NULL, (HBRUSH)(COLOR_WINDOW), NULL, wndclassname, NULL};

	if(!RegisterClassEx(&wcex)) return 0;

	if(!(np2_hThreadKeyHookhWnd = CreateWindow(wndclassname, _T("NP2 Key Hook"), WS_POPUPWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, g_hInstance, NULL))) return 0;

	ShowWindow( np2_hThreadKeyHookhWnd, SW_HIDE ); // 念のため

	if(!hHook){
		hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, g_hInstance, 0);
	}
	// メイン メッセージ ループ
	while( GetMessage(&msg, NULL, 0, 0) > 0 ) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if(hHook){
		UnhookWindowsHookEx(hHook);
		hHook = NULL;
	}
	np2_hThreadKeyHookhWnd = NULL;
	np2_hThreadKeyHook = NULL;
	UnregisterClass(wndclassname, g_hInstance);
	return 0;
}
static void start_hook_systemkey()
{
	unsigned int dwID;
	if(!np2_hThreadKeyHook){
		np2_hThreadKeyHook = (HANDLE)_beginthreadex(NULL, 0, np2_ThreadFuncKeyHook, NULL, 0, &dwID);
	}
	//if(!hHook){
	//	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, g_hInstance, 0);
	//}
}
static void stop_hook_systemkey()
{
	if(np2_hThreadKeyHook && np2_hThreadKeyHookhWnd){
		np2_hThreadKeyHookexit = 1;
		SendMessage(np2_hThreadKeyHookhWnd , WM_CLOSE , 0 , 0);
		WaitForSingleObject(np2_hThreadKeyHook, INFINITE);
		CloseHandle(np2_hThreadKeyHook);
		np2_hThreadKeyHook = NULL;
		np2_hThreadKeyHookexit = 0;
	}
	//if(hHook){
	//	UnhookWindowsHookEx(hHook);
	//	hHook = NULL;
	//}
}
#endif

// タイトルバーの音量・マウス速度 自動非表示用
#define TMRSYSMNG_ID	9898 // 他と被らないようにすること
UINT_PTR tmrSysMngHide = 0;
VOID CALLBACK SysMngHideTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	sys_miscinfo.showvolume = 0;
	sys_miscinfo.showmousespeed = 0;
	sysmng_updatecaption(SYS_UPDATECAPTION_MISC);
	KillTimer(hwnd , tmrSysMngHide);
	tmrSysMngHide = 0;
}


// ----

static int messagebox(HWND hWnd, LPCTSTR lpcszText, UINT uType)
{
	LPCTSTR szCaption = np2oscfg.titles;

	std::tstring rText(LoadTString(lpcszText));
	return MessageBox(hWnd, rText.c_str(), szCaption, uType);
}

// ----

/**
 * リソース DLL をロード
 * @param[in] hInstance 元のインスタンス
 * @return インスタンス
 */
static HINSTANCE LoadExternalResource(HINSTANCE hInstance)
{
	OEMCHAR szDllName[32];
	OEMSPRINTF(szDllName, szNp2ResDll, GetOEMCP());

	TCHAR szPath[MAX_PATH];
	file_cpyname(szPath, modulefile, _countof(szPath));
	file_cutname(szPath);
	file_catname(szPath, szDllName, _countof(szPath));

	HMODULE hModule = LoadLibrary(szPath);
	s_hModResource = hModule;
	if (hModule != NULL)
	{
		hInstance = static_cast<HINSTANCE>(hModule);
	}
	return hInstance;
}

/**
 * リソースのアンロード
 */
static void UnloadExternalResource()
{
	HMODULE hModule = s_hModResource;
	s_hModResource = NULL;
	if (hModule)
	{
		FreeLibrary(hModule);
	}
}


// ----

static void winuienter(void) {

	winui_en = TRUE;
	if(!np2_multithread_Enabled()){
		CSoundMng::GetInstance()->Disable(SNDPROC_MAIN);
	}
	scrnmng_topwinui();
}

static void winuileave(void) {

	scrnmng_clearwinui();
	if(!np2_multithread_Enabled()){
		CSoundMng::GetInstance()->Enable(SNDPROC_MAIN);
	}
	winui_en = FALSE;
}

WINLOCEX np2_winlocexallwin(HWND base) {

	UINT	i;
	UINT	cnt;
	HWND	list[10];

	cnt = 0;
	list[cnt++] = g_hWndMain;
	list[cnt++] = toolwin_gethwnd();
	list[cnt++] = kdispwin_gethwnd();
	list[cnt++] = skbdwin_gethwnd();
	list[cnt++] = mdbgwin_gethwnd();
	if(FindWindow(OEMTEXT("Shell_TrayWnd"), NULL)){
		list[cnt++] = FindWindow(OEMTEXT("Shell_TrayWnd"), NULL);
	}
	for (i=0; i<cnt; i++) {
		if (list[i] == base) {
			list[i] = NULL;
		}
	}
	if (base != g_hWndMain) {		// hWndMainのみ全体移動
		base = NULL;
	}
	return(winlocex_create(base, list, cnt));
}

static void changescreen(UINT8 newmode) {

	UINT8		change;
	UINT8		renewal;
	WINLOCEX	wlex;

	screenChanging = 1;

	np2_multithread_Suspend();
	np2_multithread_EnterCriticalSection();

	change = g_scrnmode ^ newmode;
	renewal = (change & SCRNMODE_FULLSCREEN);
	wlex = NULL;
	if (newmode & SCRNMODE_FULLSCREEN) {
		renewal |= (change & SCRNMODE_HIGHCOLOR);
	}
	else {
		renewal |= (change & SCRNMODE_ROTATEMASK);
	}
	if (renewal) {
		if (renewal & SCRNMODE_FULLSCREEN) {
			toolwin_destroy();
			kdispwin_destroy();
			skbdwin_destroy();
			mdbgwin_destroy();
		}
		else if (renewal & SCRNMODE_ROTATEMASK) {
			wlex = np2_winlocexallwin(g_hWndMain);
			winlocex_setholdwnd(wlex, g_hWndMain);
		}
		soundmng_stop();
		mousemng_disable(MOUSEPROC_WINUI);
		scrnmng_destroy();
		if((newmode & SCRNMODE_FULLSCREEN)==0){
			DEVMODE devmode;
			if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode)) {
				while ((DWORD)((scrnstat.width * scrnstat.multiple) >> 3) >= devmode.dmPelsWidth-64 || (DWORD)((scrnstat.height * scrnstat.multiple) >> 3) >= devmode.dmPelsHeight-64){
					scrnstat.multiple--;
					if(scrnstat.multiple==1) break;
				}
			}
		}
		if (scrnmng_create(newmode) == SUCCESS) {
			g_scrnmode = newmode;
			if(np2oscfg.scrnmode != g_scrnmode){
				np2oscfg.scrnmode = g_scrnmode; // Screen状態保存
				sysmng_update(SYS_UPDATEOSCFG);
			}
		}
		else {
			if (scrnmng_create(g_scrnmode) != SUCCESS) {
				scrnmng_create_pending = true;
				//PostQuitMessage(0);
				np2_multithread_LeaveCriticalSection();
				np2_multithread_Resume();
				screenChanging = 0;
				return;
			}
		}
		scrndraw_redraw();
		if (renewal & SCRNMODE_FULLSCREEN) {
			if (!scrnmng_isfullscreen()) {
				if (np2oscfg.toolwin) {
					toolwin_create();
				}
				if (np2oscfg.keydisp) {
					kdispwin_create();
				}
			}
		}
		else if (renewal & SCRNMODE_ROTATEMASK) {
			winlocex_move(wlex);
			winlocex_destroy(wlex);
		}
		mousemng_enable(MOUSEPROC_WINUI);
		soundmng_play();
	}
	else {
		g_scrnmode = newmode;
	}
	
	np2_multithread_LeaveCriticalSection();
	np2_multithread_Resume();

	screenChanging = 0;
}

static void wincentering(HWND hWnd) {

	RECT	rc;
	int		width;
	int		height;

	GetWindowRect(hWnd, &rc);
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;
	np2oscfg.winx = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	np2oscfg.winy = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	if (np2oscfg.winx < 0) {
		np2oscfg.winx = 0;
	}
	if (np2oscfg.winy < 0) {
		np2oscfg.winy = 0;
	}
	sysmng_update(SYS_UPDATEOSCFG);
	MoveWindow(g_hWndMain, np2oscfg.winx, np2oscfg.winy, width, height, TRUE);
}

void np2active_renewal(void) {										// ver0.30

	if (np2break & (~NP2BREAK_MAIN)) {
		np2stopemulate = 2;
		CSoundMng::GetInstance()->Disable(SNDPROC_MASTER);
	}
	else if (np2break & NP2BREAK_MAIN) {
		if (np2oscfg.background & 1) {
			np2stopemulate = 1;
		}
		else {
			np2stopemulate = 0;
		}
		if (np2oscfg.background) {
			CSoundMng::GetInstance()->Disable(SNDPROC_MASTER);
		}
		else {
			CSoundMng::GetInstance()->Enable(SNDPROC_MASTER);
		}
	}
	else {
		np2stopemulate = 0;
		CSoundMng::GetInstance()->Enable(SNDPROC_MASTER);
	}
}


// ---- resume and statsave

#if defined(SUPPORT_RESUME) || defined(SUPPORT_STATSAVE)
static void getstatfilename(OEMCHAR *path, const OEMCHAR *ext, int size) {

	initgetfile(path, size);
	//file_cpyname(path, modulefile, size);
	file_cutext(path);
	file_catname(path, str_dot, size);
	file_catname(path, ext, size);
}

static int flagsave(const OEMCHAR *ext) {

	int		ret;
	OEMCHAR	path[MAX_PATH];
	
	np2_multithread_Suspend();
	getstatfilename(path, ext, NELEMENTS(path));
	soundmng_stop();
	ret = statsave_save(path);
	if (ret) {
		file_delete(path);
	}
	soundmng_play();
	np2_multithread_Resume();
	return(ret);
}

static void flagdelete(const OEMCHAR *ext) {

	OEMCHAR	path[MAX_PATH];

	getstatfilename(path, ext, NELEMENTS(path));
	file_delete(path);
}

static int flagload(HWND hWnd, const OEMCHAR *ext, LPCTSTR title, BOOL force)
{
	int		nRet;
	int		nID;
	OEMCHAR	szPath[MAX_PATH];
	OEMCHAR	szStat[1024];
	TCHAR	szMessage[1024 + 256];

	getstatfilename(szPath, ext, NELEMENTS(szPath));
	np2_multithread_Suspend();
	winuienter();
	nID = IDYES;
	nRet = statsave_check(szPath, szStat, NELEMENTS(szStat));
	if (nRet & (~STATFLAG_DISKCHG))
	{
		messagebox(hWnd, MAKEINTRESOURCE(IDS_ERROR_RESUME),
													MB_OK | MB_ICONSTOP);
		nID = IDNO;
	}
	else if ((!force) && (nRet & STATFLAG_DISKCHG))
	{
		std::tstring rFormat(LoadTString(IDS_CONFIRM_RESUME));
		wsprintf(szMessage, rFormat.c_str(), szStat);
		nID = messagebox(hWnd, szMessage, MB_YESNOCANCEL | MB_ICONQUESTION);
	}
	if (nID == IDYES)
	{
		statsave_load(szPath);
		toolwin_setfdd(0, fdd_diskname(0));
		toolwin_setfdd(1, fdd_diskname(1));
	}
	sysmng_workclockreset();
	sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
	winuileave();
	np2_multithread_Resume();
	return nID;
}
#endif

/**
 * サウンドデバイスの再オープン
 * @param[in] hWnd ウィンドウ ハンドル
 */
static void OpenSoundDevice(HWND hWnd)
{
	CSoundMng* pSoundMng = CSoundMng::GetInstance();
	if (pSoundMng->Open(static_cast<CSoundMng::DeviceType>(np2oscfg.cSoundDeviceType), np2oscfg.szSoundDeviceName, hWnd))
	{
		pSoundMng->LoadPCM(SOUND_PCMSEEK, TEXT("SEEKWAV"));
		pSoundMng->LoadPCM(SOUND_PCMSEEK1, TEXT("SEEK1WAV"));
		pSoundMng->LoadPCM(SOUND_RELAY1, TEXT("RELAY1WAV"));
		pSoundMng->SetPCMVolume(SOUND_PCMSEEK, np2cfg.MOTORVOL);
		pSoundMng->SetPCMVolume(SOUND_PCMSEEK1, np2cfg.MOTORVOL);
		pSoundMng->SetPCMVolume(SOUND_RELAY1, np2cfg.MOTORVOL);
		pSoundMng->SetMasterVolume(np2cfg.vol_master);
	}
}

// ---- proc

static void np2popup(HWND hWnd, LPARAM lp) {

	HMENU	mainmenu;
	HMENU	hMenu;
	HMENU	hMenuEdit;
	POINT	pt;

	mainmenu = (HMENU)GetWindowLongPtr(hWnd, NP2GWLP_HMENU);
	hMenu = CreatePopupMenu();
    hMenuEdit = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_COPYPASTEPOPUP));
#if defined(SUPPORT_WAB)
		EnableMenuItem(hMenuEdit, IDM_COPYPASTE_COPYWABMEM, 0);
#endif
	menu_addmenubar(hMenu, hMenuEdit);
	if (mainmenu) {
		InsertMenu(hMenu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
		menu_addmenubar(hMenu, mainmenu);
	}
	xmenu_update(hMenu);
	pt.x = LOWORD(lp);
	pt.y = HIWORD(lp);
	ClientToScreen(hWnd, &pt);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);
}

#ifdef SUPPORT_PHYSICAL_CDDRV
static void np2updateCDmenu() {
	static char drvMenuVisible[4][26] = {0}; // 実ドライブメニューの表示状態
	char drvAvailable[26] = {0}; // 使える実ドライブ
	
	REG8 drv;
	HMENU hMenu = np2class_gethmenu(g_hWndMain);
	HMENU hMenuTgt;
	int hMenuTgtPos;
	MENUITEMINFO mii = {0};

	DWORD dwDrive;
	int nDrive;
	TCHAR szBuff2[] = OEMTEXT("A:\\");

	// 有効なCDドライブのドライブ文字を調べる
	dwDrive = GetLogicalDrives();
	for ( nDrive = 0 ; nDrive < 26 ; nDrive++ ){
		if ( dwDrive & (1 << nDrive) ){
			szBuff2[0] = nDrive + 'A';
			if(GetDriveType(szBuff2)==DRIVE_CDROM){
				drvAvailable[nDrive] = 1;
			}
		}
	}
	szBuff2[2] = 0;
#if defined(SUPPORT_IDEIO)
	for (drv = 0x00; drv < 0x04; drv++)
	{
		int mnupos = 1;
		if(menu_searchmenu(hMenu, IDM_IDE0OPEN+drv, &hMenuTgt, &hMenuTgtPos)){
			// 一旦全部消す
			for ( nDrive = 0 ; nDrive < 26 ; nDrive++ ){
				if(drvMenuVisible[drv][nDrive]){
					DeleteMenu(hMenuTgt, IDM_IDE0PHYSICALDRV_ID0 + 26*drv + nDrive, MF_BYCOMMAND);
					drvMenuVisible[drv][nDrive] = 0;
				}
			}
			if(np2cfg.idetype[drv]==SXSIDEV_CDROM){
				// 再追加
				for ( nDrive = 0 ; nDrive < 26 ; nDrive++ ){
					if(drvAvailable[nDrive]){
						TCHAR mnuText[200] = {0};
						szBuff2[0] = nDrive + 'A';
						if(!LoadString(g_hInstance, IDS_PHYSICALDRIVE, mnuText, sizeof(mnuText)/sizeof(mnuText[0]) - sizeof(szBuff2)/sizeof(szBuff2[0]))){
							_tcscpy(mnuText, OEMTEXT("&Physical Drive "));
						}
						_tcscat(mnuText, szBuff2);
						InsertMenu(hMenuTgt, mnupos++, MF_BYPOSITION, IDM_IDE0PHYSICALDRV_ID0 + 26*drv + nDrive, mnuText); 
						drvMenuVisible[drv][nDrive] = 1;
					}
				}
			}
		}
		//if(np2cfg.idetype[drv]==SXSIDEV_CDROM)
		//{
		//	EnableMenuItem(hMenu, IDM_IDE0PHYSICALDRV+drv, MF_BYCOMMAND|MFS_ENABLED);
		//}
		//else
		//{
		//	EnableMenuItem(hMenu, IDM_IDE0PHYSICALDRV+drv, MF_BYCOMMAND|MFS_GRAYED);
		//}
	}	
#endif
}
#endif

static void OnCommand(HWND hWnd, WPARAM wParam)
{
	UINT		update;
	UINT		uID;
	BOOL		b;

	static UINT16 oldcpustabf = 90;

	update = 0;
	uID = LOWORD(wParam);
#if defined(SUPPORT_IDEIO)
#if defined(SUPPORT_PHYSICAL_CDDRV)
	if(IDM_IDE0PHYSICALDRV_ID0 <= uID && uID < IDM_IDE0PHYSICALDRV_ID0 + 26*4){
		TCHAR szBuff[] = OEMTEXT("\\\\.\\A:");
		int idedrv = (uID - IDM_IDE0PHYSICALDRV_ID0) / 26;
		int drvnum = (uID - IDM_IDE0PHYSICALDRV_ID0) % 26;
		szBuff[4] = drvnum + 'A';
		sysmng_update(SYS_UPDATEOSCFG);
		diskdrv_setsxsi(idedrv, szBuff);
	}
#endif
#endif
	if (IDM_FDD1_LIST_ID0 <= uID && uID <= IDM_FDD1_LIST_LAST) {
		int fdddrv = (uID - IDM_FDD1_LIST_ID0) / FDDMENU_ITEMS_MAX;
		int index = (uID - IDM_FDD1_LIST_ID0) % FDDMENU_ITEMS_MAX;
		OEMCHAR* lpImage = sysmng_getfddlistitem(fdddrv, index);
		if (lpImage) {
			file_cpyname(fddfolder, lpImage, _countof(fddfolder));
			sysmng_update(SYS_UPDATEOSCFG);
			np2_multithread_Suspend();
			diskdrv_setfdd(fdddrv, lpImage, false);
			toolwin_setfdd(fdddrv, lpImage);
			np2_multithread_Resume();
		}
	}
	switch(uID)
	{
		case IDM_RESET:
			b = FALSE;
			if (!np2oscfg.comfirm)
			{
				b = TRUE;
			}
			else
			{
				winuienter();
				if (messagebox(hWnd, MAKEINTRESOURCE(IDS_CONFIRM_RESET), MB_ICONQUESTION | MB_YESNO) == IDYES)
				{
					b = TRUE;
				}
				winuileave();
			}
			if (b)
			{
				if (sys_updates & SYS_UPDATESNDDEV)
				{
					sys_updates &= ~SYS_UPDATESNDDEV;
					OpenSoundDevice(hWnd);
				}
#ifdef HOOK_SYSKEY
				stop_hook_systemkey();
#endif
				np2_multithread_Suspend();
				pccore_cfgupdate();
				if(nevent_iswork(NEVENT_CDWAIT)){
					nevent_forceexecute(NEVENT_CDWAIT);
				}
				pccore_reset();
				np2_SetUserPause(0);
				np2_multithread_Resume();
				sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
#ifdef SUPPORT_PHYSICAL_CDDRV
				np2updateCDmenu();
#endif
#ifdef HOOK_SYSKEY
				start_hook_systemkey();
#endif
			}
			break;
		case IDM_PAUSE:
			np2_SetUserPause(!np2userpause);
			update |= SYS_UPDATECFG;
			break;
		case IDM_CONFIG:
			winuienter();
			dialog_configure(hWnd);
			if (!scrnmng_isfullscreen()) {
				UINT8 thick;
				thick = (GetWindowLong(hWnd, GWL_STYLE) & WS_THICKFRAME)?1:0;
				if (thick != np2oscfg.thickframe) {
					WINLOCEX wlex;
					wlex = np2_winlocexallwin(hWnd);
					winlocex_setholdwnd(wlex, hWnd);
					np2class_frametype(hWnd, np2oscfg.thickframe);
					winlocex_move(wlex);
					winlocex_destroy(wlex);
				}
			}
			winuileave();
#if defined(SUPPORT_MULTITHREAD)
			if (!!np2_multithread_enable != !!np2oscfg.multithread)
			{
				// マルチスレッドモード切替要求
				np2_multithread_requestswitch = 1;
			}
#endif
			break;

		case IDM_EMULSPEED_50:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[0];// 50;
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;
		case IDM_EMULSPEED_75:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[1];// 75;
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;
		case IDM_EMULSPEED_100:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[2];// 100;
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;
		case IDM_EMULSPEED_150:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[3];// 150;
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;
		case IDM_EMULSPEED_200:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[4];// 200;
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;
		case IDM_EMULSPEED_400:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[5];// 400;
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;
		case IDM_EMULSPEED_800:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[6];// 800;
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;
		case IDM_EMULSPEED_USER1:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[7];
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;
		case IDM_EMULSPEED_USER2:
			np2cfg.emuspeed = np2oscfg.cpuspdlst[8];
			timing_setspeed(np2cfg.emuspeed * 128 / 100);
			update |= SYS_UPDATECFG;
			break;

		case IDM_CHANGECLK_X2:
			np2_DynamicChangeClockMul(np2oscfg.cpumullst[0]);
			break;
		case IDM_CHANGECLK_X8:
			np2_DynamicChangeClockMul(np2oscfg.cpumullst[1]);
			break;
		case IDM_CHANGECLK_X16:
			np2_DynamicChangeClockMul(np2oscfg.cpumullst[2]);
			break;
		case IDM_CHANGECLK_X30:
			np2_DynamicChangeClockMul(np2oscfg.cpumullst[3]);
			break;
		case IDM_CHANGECLK_X42:
			np2_DynamicChangeClockMul(np2oscfg.cpumullst[4]);
			break;
		case IDM_CHANGECLK_X52:
			np2_DynamicChangeClockMul(np2oscfg.cpumullst[5]);
			break;
		case IDM_CHANGECLK_X62:
			np2_DynamicChangeClockMul(np2oscfg.cpumullst[6]);
			break;
		case IDM_CHANGECLK_USER1:
			np2_DynamicChangeClockMul(np2oscfg.cpumullst[7]);
			break;
		case IDM_CHANGECLK_RESTORE:
			np2_DynamicChangeClockMul(np2cfg.multiple);
			break;

		case IDM_NEWDISK:
			winuienter();
			dialog_newdisk(hWnd);
			winuileave();
			break;
			
		case IDM_NEWDISKFD:
			winuienter();
			dialog_newdisk_ex(hWnd, NEWDISKMODE_FD);
			winuileave();
			break;

		case IDM_NEWDISKHD:
			winuienter();
			dialog_newdisk_ex(hWnd, NEWDISKMODE_HD);
			winuileave();
			break;

		case IDM_CHANGEFONT:
			winuienter();
			dialog_font(hWnd);
			winuileave();
			break;
			
		case IDM_LOADVMCFG:
			winuienter();
			dialog_readnpcfg(hWnd);
			winuileave();

			break;

		case IDM_SAVEVMCFG:
			winuienter();
			dialog_writenpcfg(hWnd);
			winuileave();
			break;

		case IDM_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0L);
			break;

		case IDM_FDD1OPEN:
			winuienter();
			dialog_changefdd(hWnd, 0);
			winuileave();
			break;

		case IDM_FDD1EJECT:
			diskdrv_setfdd(0, NULL, 0);
			toolwin_setfdd(0, NULL);
			break;

		case IDM_FDD2OPEN:
			winuienter();
			dialog_changefdd(hWnd, 1);
			winuileave();
			break;

		case IDM_FDD2EJECT:
			diskdrv_setfdd(1, NULL, 0);
			toolwin_setfdd(1, NULL);
			break;

		case IDM_FDD3OPEN:
			winuienter();
			dialog_changefdd(hWnd, 2);
			winuileave();
			break;

		case IDM_FDD3EJECT:
			diskdrv_setfdd(2, NULL, 0);
			toolwin_setfdd(2, NULL);
			break;

		case IDM_FDD4OPEN:
			winuienter();
			dialog_changefdd(hWnd, 3);
			winuileave();
			break;

		case IDM_FDD4EJECT:
			diskdrv_setfdd(3, NULL, 0);
			toolwin_setfdd(3, NULL);
			break;

		//case IDM_FDD1_LIST_DIRNAME:
		//case IDM_FDD2_LIST_DIRNAME:
		//case IDM_FDD3_LIST_DIRNAME:
		//case IDM_FDD4_LIST_DIRNAME:
		//	{
		//		OEMCHAR* fname = fdd_diskname(uID - IDM_FDD1_LIST_DIRNAME); // 現在開いているファイルから取得
		//		if (!fname || !(*fname)) {
		//			fname = sysmng_getlastfddlistitem(uID - IDM_FDD1_LIST_DIRNAME); // だめならディレクトリパスに基づいて取得
		//		}
		//		if (fname && *fname) {
		//			TCHAR seltmp[500];
		//			_tcscpy(seltmp, OEMTEXT("/select,"));
		//			_tcscat(seltmp, fname);
		//			ShellExecute(NULL, NULL, OEMTEXT("explorer.exe"), seltmp, NULL, SW_SHOWNORMAL);
		//		}
		//	}
		//	break;

		case IDM_IDE0OPEN:
			winuienter();
			dialog_changehdd(hWnd, 0x00);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_IDE0EJECT:
			diskdrv_setsxsi(0x00, NULL);
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_IDE1OPEN:
			winuienter();
			dialog_changehdd(hWnd, 0x01);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_IDE1EJECT:
			diskdrv_setsxsi(0x01, NULL);
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

#if defined(SUPPORT_IDEIO)
		case IDM_IDE2OPEN:
			winuienter();
			dialog_changehdd(hWnd, 0x02);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_IDE2EJECT:
			diskdrv_setsxsi(0x02, NULL);
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;
			
		case IDM_IDE3OPEN:
			winuienter();
			dialog_changehdd(hWnd, 0x03);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_IDE3EJECT:
			diskdrv_setsxsi(0x03, NULL);
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;
			
		case IDM_IDEOPT:
			winuienter();
			dialog_ideopt(hWnd);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;
#endif
			
		case IDM_IDE0STATE:
		case IDM_IDE1STATE:
		case IDM_IDE2STATE:
		case IDM_IDE3STATE:
			{
				const OEMCHAR *fname;
				fname = sxsi_getfilename(uID - IDM_IDE0STATE);
				if(!fname || !(*fname)){
#if defined(SUPPORT_IDEIO)
					if(np2cfg.idetype[uID - IDM_IDE0STATE]==SXSIDEV_CDROM){
						fname = np2cfg.idecd[uID - IDM_IDE0STATE];
					}else{
#endif
						fname = diskdrv_getsxsi(uID - IDM_IDE0STATE);
#if defined(SUPPORT_IDEIO)
					}
#endif
				}
				if(_tcsnicmp(fname, OEMTEXT("\\\\.\\"), 4)==0){
					fname += 4;
				}
				if(fname && *fname){
					TCHAR seltmp[500];
					_tcscpy(seltmp, OEMTEXT("/select,"));
					_tcscat(seltmp, fname);
					ShellExecute(NULL, NULL, OEMTEXT("explorer.exe"), seltmp, NULL, SW_SHOWNORMAL);
				}
			}
			break;

#if defined(SUPPORT_SCSI)
		case IDM_SCSI0OPEN:
			winuienter();
			dialog_changehdd(hWnd, 0x20);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_SCSI0EJECT:
			diskdrv_setsxsi(0x20, NULL);
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_SCSI1OPEN:
			winuienter();
			dialog_changehdd(hWnd, 0x21);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_SCSI1EJECT:
			diskdrv_setsxsi(0x21, NULL);
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_SCSI2OPEN:
			winuienter();
			dialog_changehdd(hWnd, 0x22);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_SCSI2EJECT:
			diskdrv_setsxsi(0x22, NULL);
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_SCSI3OPEN:
			winuienter();
			dialog_changehdd(hWnd, 0x23);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;

		case IDM_SCSI3EJECT:
			diskdrv_setsxsi(0x23, NULL);
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;
			
		case IDM_SCSI0STATE:
		case IDM_SCSI1STATE:
		case IDM_SCSI2STATE:
		case IDM_SCSI3STATE:
			{
				const OEMCHAR *fname;
				fname = sxsi_getfilename(uID - IDM_SCSI0STATE + 0x20);
				if(!fname || !(*fname)){
					fname = diskdrv_getsxsi(uID - IDM_IDE0STATE + 0x20);
				}
				if(fname && *fname){
					TCHAR seltmp[500];
					_tcscpy(seltmp, OEMTEXT("/select,"));
					_tcscat(seltmp, fname);
					ShellExecute(NULL, NULL, OEMTEXT("explorer.exe"), seltmp, NULL, SW_SHOWNORMAL);
				}
			}
			break;
#endif

		case IDM_WINDOW:
			changescreen(g_scrnmode & (~SCRNMODE_FULLSCREEN));
			break;

		case IDM_FULLSCREEN:
			changescreen(g_scrnmode | SCRNMODE_FULLSCREEN);
			break;

		case IDM_ROLNORMAL:
			changescreen(g_scrnmode & (~SCRNMODE_ROTATEMASK));
			break;

		case IDM_ROLLEFT:
			changescreen((g_scrnmode & (~SCRNMODE_ROTATEMASK)) | SCRNMODE_ROTATELEFT);
			break;

		case IDM_ROLRIGHT:
			changescreen((g_scrnmode & (~SCRNMODE_ROTATEMASK)) | SCRNMODE_ROTATERIGHT);
			break;

		case IDM_DISPSYNC:
			np2cfg.DISPSYNC = !np2cfg.DISPSYNC;
			update |= SYS_UPDATECFG;
			break;

		case IDM_RASTER:
			np2cfg.RASTER = !np2cfg.RASTER;
			if (np2cfg.RASTER)
			{
				changescreen(g_scrnmode | SCRNMODE_HIGHCOLOR);
			}
			else
			{
				changescreen(g_scrnmode & (~SCRNMODE_HIGHCOLOR));
			}
			update |= SYS_UPDATECFG;
			break;

		case IDM_NOWAIT:
			np2oscfg.NOWAIT = !np2oscfg.NOWAIT;
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_CPUSTABILIZER:
			if(np2oscfg.cpustabf == 0){
				np2oscfg.cpustabf = oldcpustabf;
			}else{
				oldcpustabf = np2oscfg.cpustabf;
				np2oscfg.cpustabf = 0;
			}
			update |= SYS_UPDATECFG;
			break;
			
#if defined(SUPPORT_ASYNC_CPU)
		case IDM_ASYNCCPU:
			np2cfg.asynccpu = !np2cfg.asynccpu;
			update |= SYS_UPDATECFG;
			break;
		case IDM_ASYNCCPU_LEVEL_MAX:
			np2cfg.asynclvl = 100;
			pccore_asynccpu_updatesettings(np2cfg.asynclvl);
			update |= SYS_UPDATECFG;
			break;
		case IDM_ASYNCCPU_LEVEL_MIN:
			np2cfg.asynclvl = 0;
			pccore_asynccpu_updatesettings(np2cfg.asynclvl);
			update |= SYS_UPDATECFG;
			break;
#endif

		case IDM_AUTOFPS:
			np2oscfg.DRAW_SKIP = 0;
			update |= SYS_UPDATECFG;
			break;

		case IDM_60FPS:
			np2oscfg.DRAW_SKIP = 1;
			update |= SYS_UPDATECFG;
			break;

		case IDM_30FPS:
			np2oscfg.DRAW_SKIP = 2;
			update |= SYS_UPDATECFG;
			break;

		case IDM_20FPS:
			np2oscfg.DRAW_SKIP = 3;
			update |= SYS_UPDATECFG;
			break;

		case IDM_15FPS:
			np2oscfg.DRAW_SKIP = 4;
			update |= SYS_UPDATECFG;
			break;

		case IDM_SCREENOPT:
			winuienter();
			dialog_scropt(hWnd);
			winuileave();
			break;

		case IDM_KEY:
			np2cfg.KEY_MODE = 0;
			keystat_resetjoykey();
			update |= SYS_UPDATECFG;
			break;

		case IDM_JOY1:
			np2cfg.KEY_MODE = 1;
			keystat_resetjoykey();
			update |= SYS_UPDATECFG;
			break;

		case IDM_JOY2:
			np2cfg.KEY_MODE = 2;
			keystat_resetjoykey();
			update |= SYS_UPDATECFG;
			break;

		case IDM_XSHIFT:
			np2cfg.XSHIFT ^= 1;
			keystat_forcerelease(0x70);
			update |= SYS_UPDATECFG;
			break;

		case IDM_XCTRL:
			np2cfg.XSHIFT ^= 2;
			keystat_forcerelease(0x74);
			update |= SYS_UPDATECFG;
			break;

		case IDM_XGRPH:
			np2cfg.XSHIFT ^= 4;
			keystat_forcerelease(0x73);
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_SENDCAD:
			keystat_senddata(0x73);
			keystat_senddata(0x74);
			keystat_senddata(0x39);
			keystat_senddata(0x73 | 0x80);
			keystat_senddata(0x74 | 0x80);
			keystat_senddata(0x39 | 0x80);
			break;
			
		case IDM_USENUMLOCK:
			np2oscfg.USENUMLOCK = !np2oscfg.USENUMLOCK;
			update |= SYS_UPDATEOSCFG;
			break;
			
		case IDM_SWAPPAGEUPDOWN:
			np2oscfg.xrollkey = !np2oscfg.xrollkey;
			winkbd_roll(np2oscfg.KEYBOARD==KEY_PC98 ? np2oscfg.xrollkey : !np2oscfg.xrollkey);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_F12MOUSE:
			np2oscfg.F12COPY = 0;
			winkbd_resetf12();
			winkbd_setf12(0);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_F12COPY:
			np2oscfg.F12COPY = 1;
			winkbd_resetf12();
			winkbd_setf12(1);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_F12STOP:
			np2oscfg.F12COPY = 2;
			winkbd_resetf12();
			winkbd_setf12(2);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_F12EQU:
			np2oscfg.F12COPY = 3;
			winkbd_resetf12();
			winkbd_setf12(3);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_F12COMMA:
			np2oscfg.F12COPY = 4;
			winkbd_resetf12();
			winkbd_setf12(4);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_USERKEY1:
			np2oscfg.F12COPY = 5;
			winkbd_resetf12();
			winkbd_setf12(5);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_USERKEY2:
			np2oscfg.F12COPY = 6;
			winkbd_resetf12();
			winkbd_setf12(6);
			update |= SYS_UPDATEOSCFG;
			break;
			
		case IDM_F12NOWAIT:
			np2oscfg.F12COPY = 7;
			winkbd_resetf12();
			winkbd_setf12(7);
			update |= SYS_UPDATEOSCFG;
			break;
			
		case IDM_F12NOWAIT2:
			np2oscfg.F12COPY = 8;
			winkbd_resetf12();
			winkbd_setf12(8);
			update |= SYS_UPDATEOSCFG;
			break;
			
		case IDM_F12WABRELAY:
			np2oscfg.F12COPY = 9;
			winkbd_resetf12();
			winkbd_setf12(9);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_BEEPOFF:
			np2cfg.BEEP_VOL = 0;
			beep_setvol(0);
			update |= SYS_UPDATECFG;
			break;

		case IDM_BEEPLOW:
			np2cfg.BEEP_VOL = 1;
			beep_setvol(1);
			update |= SYS_UPDATECFG;
			break;

		case IDM_BEEPMID:
			np2cfg.BEEP_VOL = 2;
			beep_setvol(2);
			update |= SYS_UPDATECFG;
			break;

		case IDM_BEEPHIGH:
			np2cfg.BEEP_VOL = 3;
			beep_setvol(3);
			update |= SYS_UPDATECFG;
			break;

		case IDM_FIXBEEPOFFSET:
			np2cfg.nbeepofs = (np2cfg.nbeepofs == 0 ? 1 : 0);
			update |= SYS_UPDATECFG;
			break;

		case IDM_NOSOUND:
			np2cfg.SOUND_SW = 0x00;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_PC9801_14:
			np2cfg.SOUND_SW = 0x01;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_PC9801_26K:
			np2cfg.SOUND_SW = 0x02;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_PC9801_86:
			np2cfg.SOUND_SW = 0x04;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_PC9801_26_86:
			np2cfg.SOUND_SW = 0x06;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_PC9801_86_CB:
			np2cfg.SOUND_SW = 0x14;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_PC9801_118:
			np2cfg.SOUND_SW = 0x08;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_PC9801_86_WSS:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_WSS;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_PC9801_86_118:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_118;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_MATE_X_PCM:
			np2cfg.SOUND_SW = SOUNDID_MATE_X_PCM;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_SPEAKBOARD:
			np2cfg.SOUND_SW = SOUNDID_SPEAKBOARD;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_86SPEAKBOARD:
			np2cfg.SOUND_SW = SOUNDID_86_SPEAKBOARD;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_SPARKBOARD:
			np2cfg.SOUND_SW = 0x40;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
#if defined(SUPPORT_SOUND_SB16)
		case IDM_SB16:
			np2cfg.SOUND_SW = SOUNDID_SB16;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_PC9801_86_SB16:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_SB16;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_WSS_SB16:
			np2cfg.SOUND_SW = SOUNDID_WSS_SB16;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_PC9801_86_WSS_SB16:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_WSS_SB16;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_PC9801_118_SB16:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_118_SB16;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_PC9801_86_118_SB16:
			np2cfg.SOUND_SW = SOUNDID_PC_9801_86_118_SB16;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
#endif	// defined(SUPPORT_SOUND_SB16)

#if defined(SUPPORT_PX)
		case IDM_PX1:
			np2cfg.SOUND_SW = 0x30;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_PX2:
			np2cfg.SOUND_SW = 0x50;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
#endif	// defined(SUPPORT_PX)

		case IDM_SOUNDORCHESTRA:
			np2cfg.SOUND_SW = 0x32;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_SOUNDORCHESTRAV:
			np2cfg.SOUND_SW = 0x82;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_AMD98:
			np2cfg.SOUND_SW = 0x80;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;
			
		case IDM_WAVESTAR:
			np2cfg.SOUND_SW = SOUNDID_WAVESTAR;
			update |= SYS_UPDATECFG | SYS_UPDATESBOARD;
			break;

		case IDM_JASTSOUND:
			np2oscfg.jastsnd = !np2oscfg.jastsnd;
			update |= SYS_UPDATEOSCFG;
			break;
			
		case IDM_SEEKSND:
			np2cfg.MOTOR = !np2cfg.MOTOR;
			update |= SYS_UPDATECFG;
			break;

		case IDM_MEM640:
			np2cfg.EXTMEM = 0;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM16:
			np2cfg.EXTMEM = 1;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM36:
			np2cfg.EXTMEM = 3;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM76:
			np2cfg.EXTMEM = 7;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM116:
			np2cfg.EXTMEM = 11;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM136:
			np2cfg.EXTMEM = 13;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM166:
			np2cfg.EXTMEM = 16;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM326:
			np2cfg.EXTMEM = 32;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM646:
			np2cfg.EXTMEM = 64;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;

		case IDM_MEM1206:
			np2cfg.EXTMEM = 120;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;
			
		case IDM_MEM2306:
			np2cfg.EXTMEM = 230;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;
			
#if defined(SUPPORT_LARGE_MEMORY)
		case IDM_MEM5126:
			np2cfg.EXTMEM = 512;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;
			
		case IDM_MEM10246:
			np2cfg.EXTMEM = 1024;
			update |= SYS_UPDATECFG | SYS_UPDATEMEMORY;
			break;
#endif
			
		case IDM_FPU80:
			np2cfg.fpu_type = FPU_TYPE_SOFTFLOAT;
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_FPU64:
			np2cfg.fpu_type = FPU_TYPE_DOSBOX;
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_FPU64INT:
			np2cfg.fpu_type = FPU_TYPE_DOSBOX2;
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_MOUSE:
			mousemng_toggle(MOUSEPROC_SYSTEM);
			np2oscfg.MOUSE_SW = !np2oscfg.MOUSE_SW;
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_MOUSENC:
			np2oscfg.mouse_nc = !np2oscfg.mouse_nc;
			if(np2oscfg.mouse_nc){
				if (np2oscfg.wintype != 0) {
					// XXX: メニューが出せなくなって詰むのを回避（暫定）
					if (!scrnmng_isfullscreen()) {
						WINLOCEX	wlex;
						np2oscfg.wintype = 0;
						wlex = np2_winlocexallwin(hWnd);
						winlocex_setholdwnd(wlex, hWnd);
						np2class_windowtype(hWnd, np2oscfg.wintype);
						winlocex_move(wlex);
						winlocex_destroy(wlex);
						sysmng_update(SYS_UPDATEOSCFG);
					}
				}
			}
			if (np2oscfg.MOUSE_SW || np2oscfg.mouse_nc) {
				SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) & ~CS_DBLCLKS);
			}
			else {
				SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) | CS_DBLCLKS);
			}
#ifdef SUPPORT_WACOM_TABLET
			cmwacom_setNCControl(!!np2oscfg.mouse_nc);
#endif
			mousemng_updateautohidecursor();
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_MOUSEWHEELCTL:
			np2oscfg.usewheel = !np2oscfg.usewheel;
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_MOUSERAW:
			np2oscfg.rawmouse = !np2oscfg.rawmouse;
			mousemng_updateclip(); // キャプチャし直す
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_SLOWMOUSE:
			np2cfg.slowmous = !np2cfg.slowmous;
			update |= SYS_UPDATECFG;
			break;

		case IDM_MOUSE30X:
			np2oscfg.mousemul = 3;
			np2oscfg.mousediv = 1;
			mousemng_updatespeed();
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_MOUSE20X:
			np2oscfg.mousemul = 2;
			np2oscfg.mousediv = 1;
			mousemng_updatespeed();
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_MOUSE15X:
			np2oscfg.mousemul = 3;
			np2oscfg.mousediv = 2;
			mousemng_updatespeed();
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_MOUSE10X:
			np2oscfg.mousemul = 1;
			np2oscfg.mousediv = 1;
			mousemng_updatespeed();
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_MOUSED2X:
			np2oscfg.mousemul = 1;
			np2oscfg.mousediv = 2;
			mousemng_updatespeed();
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_MOUSED3X:
			np2oscfg.mousemul = 1;
			np2oscfg.mousediv = 3;
			mousemng_updatespeed();
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_MOUSED4X:
			np2oscfg.mousemul = 1;
			np2oscfg.mousediv = 4;
			mousemng_updatespeed();
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_SERIAL1:
			winuienter();
			dialog_serial(hWnd);
			winuileave();
			break;

		case IDM_PARALLEL_ENDPRINTJOB:
			printif_finishjob();
			break;
			
		case IDM_MPUPC98:
			winuienter();
			dialog_mpu98(hWnd);
			winuileave();
			break;

		case IDM_MIDIPANIC:
			rs232c_midipanic();
			mpu98ii_midipanic();
#if defined(SUPPORT_SMPU98)
			smpu98_midipanic();
#endif
			pc9861k_midipanic();
			break;

		case IDM_SNDOPT:
			winuienter();
			dialog_sndopt(hWnd);
			winuileave();
			break;

#if defined(SUPPORT_NET)
		case IDM_NETOPT:
			winuienter();
			dialog_netopt(hWnd);
			winuileave();
			break;
#endif
#if defined(SUPPORT_CL_GD5430)
		case IDM_WABOPT:
			winuienter();
			dialog_wabopt(hWnd);
			winuileave();
			break;
#endif
#if defined(SUPPORT_HOSTDRV)
		case IDM_HOSTDRVOPT:
			winuienter();
			dialog_hostdrvopt(hWnd);
			winuileave();
			sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
			break;
#endif
#if defined(SUPPORT_PCI)
		case IDM_PCIOPT:
			winuienter();
			dialog_pciopt(hWnd);
			winuileave();
			break;
#endif

		case IDM_BMPSAVE:
			winuienter();
			dialog_writebmp(hWnd);
			winuileave();
			break;
			
		case IDM_TXTSAVE:
			winuienter();
			dialog_writetxt(hWnd);
			winuileave();
			break;

		case IDM_S98LOGGING:
			winuienter();
			dialog_soundlog(hWnd);
			winuileave();
			break;

		case IDM_CALENDAR:
			winuienter();
			dialog_calendar(hWnd);
			winuileave();
			break;

		case IDM_ALTENTER:
			np2oscfg.shortcut ^= 1;
			update |= SYS_UPDATECFG;
			break;

		case IDM_ALTF4:
			np2oscfg.shortcut ^= 2;
			update |= SYS_UPDATECFG;
			break;
			
#ifdef HOOK_SYSKEY
		case IDM_SYSKHOOK:
			np2oscfg.syskhook = !np2oscfg.syskhook;
			if(np2oscfg.syskhook){
				start_hook_systemkey();
			}else{
				stop_hook_systemkey();
			}
			update |= SYS_UPDATECFG;
			break;
#endif

		case IDM_DISPCLOCK:
			np2oscfg.DISPCLK ^= 1;
			update |= SYS_UPDATECFG;
			sysmng_workclockrenewal();
			sysmng_updatecaption(SYS_UPDATECAPTION_CLK);
			break;

		case IDM_DISPFRAME:
			np2oscfg.DISPCLK ^= 2;
			update |= SYS_UPDATECFG;
			sysmng_workclockrenewal();
			sysmng_updatecaption(SYS_UPDATECAPTION_CLK);
			break;

		case IDM_JOYX:
			np2cfg.BTN_MODE = !np2cfg.BTN_MODE;
			update |= SYS_UPDATECFG;
			break;

		case IDM_RAPID:
			np2cfg.BTN_RAPID = !np2cfg.BTN_RAPID;
			update |= SYS_UPDATECFG;
			break;

		case IDM_MSRAPID:
			np2cfg.MOUSERAPID = !np2cfg.MOUSERAPID;
			update |= SYS_UPDATECFG;
			break;

		case IDM_CPUSAVE:
			debugsub_status();
			break;

		case IDM_HELP:
			ShellExecute(hWnd, NULL, file_getcd(np2help), NULL, NULL, SW_SHOWNORMAL);
			break;

		case IDM_ABOUT:
			np2_multithread_Suspend();
			winuienter();
			CSoundMng::GetInstance()->Disable(SNDPROC_MAIN);
			dialog_about(hWnd);
			CSoundMng::GetInstance()->Enable(SNDPROC_MAIN);
			winuileave();
			np2_multithread_Resume();
			break;

		case IDM_ITFWORK:
			np2cfg.ITF_WORK = !np2cfg.ITF_WORK;
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_TIMERFIX:
			np2cfg.timerfix= !np2cfg.timerfix;
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_SKIP16MEMCHK:
			if(np2cfg.memchkmx != 0){
				np2cfg.memchkmx = 0;
			}else{
				np2cfg.memchkmx = 15;
			}
			update |= SYS_UPDATECFG;
			break;
			
		case IDM_FASTMEMCHK:
#if defined(SUPPORT_FAST_MEMORYCHECK)
			if(np2cfg.memcheckspeed==1){
				np2cfg.memcheckspeed = 8;
			}else{
				np2cfg.memcheckspeed = 1;
			}
			update |= SYS_UPDATECFG;
#endif
			break;

		case IDM_ALLOWDRAGDROP:
			np2oscfg.dragdrop = !np2oscfg.dragdrop;
			if (np2oscfg.dragdrop)
				DragAcceptFiles(hWnd, TRUE);	//	イメージファイルのＤ＆Ｄに対応(Kai1)
			else
				DragAcceptFiles(hWnd, FALSE);
			update |= SYS_UPDATEOSCFG;
			break;

		case IDM_RESTOREBORDER:
			if(np2oscfg.wintype!=0){
				WINLOCEX	wlex;
				np2oscfg.wintype = 0;
				wlex = np2_winlocexallwin(hWnd);
				winlocex_setholdwnd(wlex, hWnd);
				np2class_windowtype(hWnd, np2oscfg.wintype);
				winlocex_move(wlex);
				winlocex_destroy(wlex);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case IDM_COPYPASTE_COPYTVRAM:
			{
				HGLOBAL hMem;
				OEMCHAR *lpMem;
				hMem = GlobalAlloc(GHND,0x4000); // アトリビュート分小さくなるので0x4000で十分
				lpMem = (OEMCHAR*)GlobalLock(hMem);
				dialog_getTVRAM(lpMem);
				GlobalUnlock(hMem);
				if(OpenClipboard(hWnd)){
					// クリップボード奪取成功
					EmptyClipboard();
					SetClipboardData(CF_TEXT, hMem);
					CloseClipboard();
				}else{
					// クリップボード奪取失敗･･･
					GlobalFree(hMem);
				}
				pcm86_setnextintr();
			}
			break;
			
		case IDM_COPYPASTE_COPYGVRAM:
		case IDM_COPYPASTE_COPYWABMEM:
#ifdef SUPPORT_WAB
			{
				BMPFILE bf;
				BMPINFO bi;
				UINT8 *lppal;
				UINT8 *lppixels;
				HDC hDC;
				DWORD dwHeaderSize;
				BITMAPINFO *lpbinfo;
				HBITMAP hBmp;
				SCRNSAVE ss = scrnsave_create();
				if(uID == IDM_COPYPASTE_COPYWABMEM){
					np2wab_getbmp(&bf, &bi, &lppal, &lppixels);
				}else{
					scrnsave_getbmp(ss, &bf, &bi, &lppal, &lppixels, SCRNSAVE_AUTO);
				}
				hDC = GetDC(NULL);
				dwHeaderSize = LOADINTELDWORD(bf.bfOffBits) - sizeof(BMPFILE);
				lpbinfo = (BITMAPINFO*)malloc(dwHeaderSize);
				CopyMemory(lpbinfo, &bi, sizeof(BMPINFO));
				if(LOADINTELWORD(bi.biBitCount) <= 8){
					CopyMemory(lpbinfo->bmiColors, lppal, 4 << LOADINTELWORD(bi.biBitCount));
					hBmp = CreateDIBitmap(hDC, &(lpbinfo->bmiHeader), CBM_INIT, lppixels, lpbinfo, DIB_RGB_COLORS);
				}else{
					hBmp = CreateDIBitmap(hDC, &(lpbinfo->bmiHeader), CBM_INIT, lppixels, lpbinfo, DIB_RGB_COLORS);
				}
				ReleaseDC(NULL, hDC);
				free(lppal);
				free(lppixels);
				free(lpbinfo);
				if(OpenClipboard(hWnd)){
					// クリップボード奪取成功
					EmptyClipboard();
					SetClipboardData(CF_BITMAP,hBmp);
					CloseClipboard();
				}else{
					// クリップボード奪取失敗･･･
					DeleteObject(hBmp);
				}
				scrnsave_destroy(ss);
			}
#endif
			break;
			
		case IDM_COPYPASTE_PASTE:
			{
				int txtlen;
				HGLOBAL hg;
				char *strClip;
				//char *strText;
				if(autokey_sendbuffer==NULL){
					if (OpenClipboard(hWnd)){
						if((hg = GetClipboardData(CF_TEXT))!=NULL) {
							txtlen = (int)GlobalSize(hg);
							autokey_sendbufferlen = 0;
							autokey_sendbuffer = (char*)malloc(txtlen + 10);
							memset(autokey_sendbuffer, 0, txtlen + 10);
							strClip = (char*)GlobalLock(hg);
							strcpy(autokey_sendbuffer , strClip);
							GlobalUnlock(hg);
							CloseClipboard();
							autokey_lastkanastate = keyctrl.kanaref;
							autokey_sendbufferlen = (int)strlen(autokey_sendbuffer);
							autokey_sendbufferpos = 0;
							keystat_senddata(0x80|0x70);
						}else{
							CloseClipboard();
						}
					}
				}else{
					// 強制終了
					autokey_sendbufferpos = autokey_sendbufferlen;
				}
			}
			break;

		default:
#if defined(SUPPORT_STATSAVE)
			if ((uID >= IDM_FLAGSAVE) && (uID < (IDM_FLAGSAVE + SUPPORT_STATSAVE)))
			{
				OEMCHAR ext[4];
				OEMSPRINTF(ext, np2flagext, uID - IDM_FLAGSAVE);
				flagsave(ext);
			}
			else if ((uID >= IDM_FLAGLOAD) && (uID < (IDM_FLAGLOAD + SUPPORT_STATSAVE)))
			{
				OEMCHAR ext[4];
				OEMSPRINTF(ext, np2flagext, uID - IDM_FLAGLOAD);
				flagload(hWnd, ext, _T("Status Load"), TRUE);
			}
#endif
			break;
	}
	sysmng_update(update);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	PAINTSTRUCT	ps;
	RECT		rc;
	HDC			hdc;
	BOOL		b;
	UINT		update;
	HWND		subwin;
	WINLOCEX	wlex;

	static int lastmx = -1;
	static int lastmy = -1;
	static int lastbtn = -1;

	switch (msg) {
		//	イメージファイルのＤ＆Ｄに対応(Kai1)
		case WM_DROPFILES:
			np2_multithread_EnterCriticalSection();
			if(np2oscfg.dragdrop){
				int		files;				//	Kai1追加
				OEMCHAR	fname[MAX_PATH];	//	Kai1追加
				const OEMCHAR	*ext;		//	Kai1追加
   				files = DragQueryFile((HDROP)wParam, (UINT)-1, NULL, 0);
				REG8	hddrv_IDE = 0x00;
				REG8	hddrv_IDECD = 0x00;
				REG8	hddrv_SCSI = 0x20;
				REG8	fddrv = 0x00;
				UINT8	i;
				
#if defined(SUPPORT_IDEIO)
				while(hddrv_IDE <= 0x03 && np2cfg.idetype[hddrv_IDE]!=0x01) hddrv_IDE++;
				while(hddrv_IDECD <= 0x03 && np2cfg.idetype[hddrv_IDECD]!=0x02) hddrv_IDECD++;
#endif	//	SUPPORT_IDEIO
				for (i = 0; i < files; i++) {
#if defined(OSLANG_UTF8)
					TCHAR tchr[MAX_PATH];
					DragQueryFile((HDROP)wParam, i, tchr, NELEMENTS(tchr));
					tchartooem(fname, NELEMENTS(fname), tchr, -1);
#else
					DragQueryFile((HDROP)wParam, i, fname, NELEMENTS(fname));
#endif
					ext = file_getext(fname);
#if defined(SUPPORT_IDEIO)
					//	CDイメージ？
					if ((!file_cmpname(ext, OEMTEXT("iso"))) ||
						(!file_cmpname(ext, OEMTEXT("cue"))) ||
						(!file_cmpname(ext, OEMTEXT("ccd"))) ||
						(!file_cmpname(ext, OEMTEXT("cdm"))) ||
						(!file_cmpname(ext, OEMTEXT("mds"))) ||
						(!file_cmpname(ext, OEMTEXT("nrg")))) {
						diskdrv_setsxsi(hddrv_IDECD, fname);
						while(hddrv_IDECD <= 0x03 && np2cfg.idetype[hddrv_IDECD]!=0x02) hddrv_IDECD++;
						continue;
					}
#endif	//	SUPPORT_IDEIO
					//	HDイメージ？
					if ((!file_cmpname(ext, str_hdi)) ||
						(!file_cmpname(ext, str_thd)) ||
						(!file_cmpname(ext, str_nhd))) {
#if defined(SUPPORT_IDEIO)
						if (hddrv_IDE <= 0x03) {
							diskdrv_setsxsi(hddrv_IDE, fname);
							while(hddrv_IDE <= 0x03 && np2cfg.idetype[hddrv_IDE]!=0x01) hddrv_IDE++;
						}
#else
						if (hddrv_IDE <= 0x01) {
							diskdrv_setsxsi(hddrv_IDE, fname);
							hddrv_IDE++;
						}
#endif
						continue;
					}
					if (!file_cmpname(ext, str_hdd)) {
						if (hddrv_SCSI <= 0x23) {
							diskdrv_setsxsi(hddrv_SCSI, fname);
							hddrv_SCSI++;
						}
						continue;
					}
					// VM設定ファイル（単独で入れた場合のみ有効）
					if ((!file_cmpname(ext, OEMTEXT("npcfg"))) ||
						(!file_cmpname(ext, OEMTEXT("np2cfg"))) ||
						(!file_cmpname(ext, OEMTEXT("np21cfg"))) ||
						(!file_cmpname(ext, OEMTEXT("np21wcfg")))) {
						if (files == 1) {
							LPCTSTR lpFilename = fname;
							file_cpyname(npcfgfilefolder, lpFilename, _countof(bmpfilefolder));
							sysmng_update(SYS_UPDATEOSCFG);
							BOOL b = FALSE;
							if (!np2oscfg.comfirm) {
								b = TRUE;
							}
							else
							{
								if (messagebox(hWnd, MAKEINTRESOURCE(IDS_CONFIRM_EXIT),
									MB_ICONQUESTION | MB_YESNO) == IDYES)
								{
									b = TRUE;
								}
							}
							if (b) {
								np2_multithread_Suspend();
								unloadNP2INI();
								loadNP2INI(lpFilename);
								np2_multithread_Resume();
							}
							break;
						}
						else {
							continue;
						}
					}
					//	FDイメージ…？
					if (fddrv <= 0x02) {
						file_cpyname(fddfolder, fname, _countof(fddfolder));
						diskdrv_setfdd(fddrv, fname, 0);
						sysmng_update(SYS_UPDATEOSCFG);
						toolwin_setfdd(fddrv, fname);
						fddrv++;
					}
				}
				DragFinish((HDROP)wParam);
				if (GetKeyState(VK_SHIFT) & 0x8000) {
					//	Shiftキーが押下されていればリセット
					pccore_cfgupdate();
#ifdef HOOK_SYSKEY
					stop_hook_systemkey();
#endif
					np2_multithread_Suspend();
					if(nevent_iswork(NEVENT_CDWAIT)){
						nevent_forceexecute(NEVENT_CDWAIT);
					}
					pccore_reset();
					np2_SetUserPause(0);
					np2_multithread_Resume();
#ifdef HOOK_SYSKEY
					start_hook_systemkey();
#endif
				}
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_CREATE:
			g_hWndMain = hWnd;
			np2class_wmcreate(hWnd);
			np2class_windowtype(hWnd, np2oscfg.wintype);
#ifndef __GNUC__
			WINNLSEnableIME(hWnd, FALSE);
#endif
			break;

		case WM_SYSCOMMAND:
			update = 0;
			switch(wParam) {
				case IDM_TOOLWIN:
					np2oscfg.toolwin = !np2oscfg.toolwin;
					if (np2oscfg.toolwin) {
						toolwin_create();
					}
					else {
						toolwin_destroy();
					}
					update |= SYS_UPDATEOSCFG;
					break;

#if defined(SUPPORT_KEYDISP)
				case IDM_KEYDISP:
					np2oscfg.keydisp = !np2oscfg.keydisp;
					if (np2oscfg.keydisp) {
						kdispwin_create();
					}
					else {
						kdispwin_destroy();
					}
					update |= SYS_UPDATEOSCFG;
					break;
#endif
#if defined(SUPPORT_SOFTKBD)
				case IDM_SOFTKBD:
					np2oscfg.skbdwin = !np2oscfg.skbdwin;
					if (np2oscfg.skbdwin) {
						skbdwin_create();
					}
					else {
						skbdwin_destroy();
					}
					update |= SYS_UPDATEOSCFG;
					break;
#endif
#if defined(CPUCORE_IA32) && defined(SUPPORT_MEMDBG32)
				case IDM_MEMDBG32:
					mdbgwin_create();
					break;
#endif
				case IDM_SCREENCENTER:
					if ((!scrnmng_isfullscreen()) &&
						(!(GetWindowLong(hWnd, GWL_STYLE) &
											(WS_MAXIMIZE | WS_MINIMIZE)))) {
						wlex = np2_winlocexallwin(hWnd);
						wincentering(hWnd);
						winlocex_move(wlex);
						winlocex_destroy(wlex);
					}
					break;

				case IDM_SNAPENABLE:
					np2oscfg.WINSNAP = !np2oscfg.WINSNAP;
					update |= SYS_UPDATEOSCFG;
					break;

				case IDM_BACKGROUND:
					np2oscfg.background ^= 1;
					update |= SYS_UPDATEOSCFG;
					break;

				case IDM_BGSOUND:
					np2oscfg.background ^= 2;
					update |= SYS_UPDATEOSCFG;
					break;

				case IDM_RESTOREBORDER:
					if(np2oscfg.wintype!=0){
						WINLOCEX	wlex;
						np2oscfg.wintype = 0;
						wlex = np2_winlocexallwin(hWnd);
						winlocex_setholdwnd(wlex, hWnd);
						np2class_windowtype(hWnd, np2oscfg.wintype);
						winlocex_move(wlex);
						winlocex_destroy(wlex);
						sysmng_update(SYS_UPDATEOSCFG);
					}
					break;

				case IDM_MEMORYDUMP:
					debugsub_memorydump();
					break;

				case IDM_DEBUGUTY:
					CDebugUtyView::New();
					break;

				case IDM_ALLOWRESIZE:
					np2oscfg.thickframe ^= 1;
					update |= SYS_UPDATEOSCFG;
					if (!scrnmng_isfullscreen())
					{
						UINT8 thick;
						thick = (GetWindowLong(hWnd, GWL_STYLE) & WS_THICKFRAME) ? 1 : 0;
						if (thick != np2oscfg.thickframe)
						{
							WINLOCEX wlex;
							wlex = np2_winlocexallwin(hWnd);
							winlocex_setholdwnd(wlex, hWnd);
							np2class_frametype(hWnd, np2oscfg.thickframe);
							winlocex_move(wlex);
							winlocex_destroy(wlex);
						}
					}
					break;

				case IDM_SAVEWINDOWSIZE:
					np2oscfg.svscrmul ^= 1;
					update |= SYS_UPDATEOSCFG;
					break;

				case SC_MINIMIZE:
					wlex = np2_winlocexallwin(hWnd);
					winlocex_close(wlex);
					winlocex_destroy(wlex);
					return(DefWindowProc(hWnd, msg, wParam, lParam));

				case SC_RESTORE:
					subwin = toolwin_gethwnd();
					if (subwin) {
						ShowWindow(subwin, SW_SHOWNOACTIVATE);
					}
					subwin = kdispwin_gethwnd();
					if (subwin) {
						ShowWindow(subwin, SW_SHOWNOACTIVATE);
					}
					subwin = skbdwin_gethwnd();
					if (subwin) {
						ShowWindow(subwin, SW_SHOWNOACTIVATE);
					}
					subwin = mdbgwin_gethwnd();
					if (subwin) {
						ShowWindow(subwin, SW_SHOWNOACTIVATE);
					}
					return(DefWindowProc(hWnd, msg, wParam, lParam));

				default:
					if (IDM_SCRNMUL < wParam && wParam <= IDM_SCRNMUL_END) {
						if ((!scrnmng_isfullscreen()) &&
							!(GetWindowLong(g_hWndMain, GWL_STYLE) & WS_MINIMIZE))
						{
							np2_multithread_EnterCriticalSection();
							scrnmng_setmultiple((int)(wParam - IDM_SCRNMUL));
							np2_multithread_LeaveCriticalSection();
						}
						break;
					}
					else {
						return(DefWindowProc(hWnd, msg, wParam, lParam));
					}
			}
			sysmng_update(update);
			break;

		case WM_COMMAND:
			np2_multithread_EnterCriticalSection();
			OnCommand(hWnd, wParam);
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_INACTIVE) {
				np2break &= ~NP2BREAK_MAIN;
				scrndraw_updateallline();
				scrndraw_redraw();
				if (np2stopemulate || np2userpause) {
					scrndraw_draw(1);
				}
				np2_multithread_EnterCriticalSection();
				keystat_allrelease();
				mousemng_enable(MOUSEPROC_BG);
				np2_multithread_LeaveCriticalSection();
				// キャプチャ外す
				mousemng_disable(MOUSEPROC_SYSTEM);
				np2oscfg.MOUSE_SW = 0;
				update |= SYS_UPDATECFG;
			}
			else {
				np2break |= NP2BREAK_MAIN;
				np2_multithread_EnterCriticalSection();
				mousemng_disable(MOUSEPROC_BG);
				np2_multithread_LeaveCriticalSection();
			}
			np2active_renewal();
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			if (np2opening) {
				RECT		rect;
				int			width;
				int			height;
				HBITMAP		hbmp;
				BITMAP		bmp;
				HDC			hmdc;
				HBRUSH		hbrush;
				GetClientRect(hWnd, &rect);
				width = rect.right - rect.left;
				height = rect.bottom - rect.top;
				HINSTANCE hInstance = CWndProc::FindResourceHandle(TEXT("NP2BMP"), RT_BITMAP);
				hbmp = LoadBitmap(hInstance, TEXT("NP2BMP"));
				GetObject(hbmp, sizeof(BITMAP), &bmp);
				hbrush = (HBRUSH)SelectObject(hdc,
												GetStockObject(BLACK_BRUSH));
				PatBlt(hdc, 0, 0, width, height, PATCOPY);
				SelectObject(hdc, hbrush);
				hmdc = CreateCompatibleDC(hdc);
				SelectObject(hmdc, hbmp);
				BitBlt(hdc, (width - bmp.bmWidth) / 2,
						(height - bmp.bmHeight) / 2,
							bmp.bmWidth, bmp.bmHeight, hmdc, 0, 0, SRCCOPY);
				DeleteDC(hmdc);
				DeleteObject(hbmp);
			}
			else {
//				scrnmng_update();
				scrndraw_redraw();
				if (np2stopemulate || np2userpause) {
					scrndraw_draw(1);
				}
			}
			EndPaint(hWnd, &ps);
			break;

		case WM_QUERYNEWPALETTE:
			np2_multithread_EnterCriticalSection();
			scrnmng_querypalette();
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_MOVE:
			if ((!scrnmng_isfullscreen()) &&
				(!(GetWindowLong(hWnd, GWL_STYLE) &
									(WS_MAXIMIZE | WS_MINIMIZE)))) {
				GetWindowRect(hWnd, &rc);
				np2oscfg.winx = rc.left;
				np2oscfg.winy = rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_SIZE:
			np2wab_forceupdate();
			break;

		case WM_ENTERMENULOOP:
			if(!np2_multithread_Enabled())
			{
				winuienter();
			}
			sysmenu_update(GetSystemMenu(hWnd, FALSE));
			xmenu_update(GetMenu(hWnd));
			if (scrnmng_isfullscreen()) {
				DrawMenuBar(hWnd);
			}
			break;

		case WM_EXITMENULOOP:
			if(!np2_multithread_Enabled())
			{
				winuileave();
			}
			break;

		case WM_ENTERSIZEMOVE:
			np2_multithread_EnterCriticalSection();
			if(!np2_multithread_Enabled())
			{
				CSoundMng::GetInstance()->Disable(SNDPROC_MAIN);
			}
			mousemng_disable(MOUSEPROC_WINUI);
			np2_multithread_LeaveCriticalSection();
			winlocex_destroy(smwlex);
			smwlex = np2_winlocexallwin(hWnd);
			scrnmng_entersizing();
			break;

		case WM_MOVING:
			if (np2oscfg.WINSNAP) {
				winlocex_moving(smwlex, (RECT *)lParam);
			}
			break;

		case WM_SIZING:
			scrnmng_sizing((UINT)wParam, (RECT *)lParam);
			break;

		case WM_EXITSIZEMOVE:
			scrnmng_exitsizing();
			winlocex_move(smwlex);
			winlocex_destroy(smwlex);
			smwlex = NULL;
			np2_multithread_EnterCriticalSection();
			mousemng_enable(MOUSEPROC_WINUI);
			if(!np2_multithread_Enabled())
			{
				CSoundMng::GetInstance()->Enable(SNDPROC_MAIN);
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_KEYDOWN:
			autokey_sendbufferpos = autokey_sendbufferlen; // コピペ強制終了 np21w ver0.86 rev22
			if (wParam == VK_F11) {
				np2class_enablemenu(g_hWndMain, TRUE);
				return(DefWindowProc(hWnd, WM_SYSKEYDOWN, VK_F10, lParam));
			}
			if ((wParam == VK_F12) && (!np2oscfg.F12COPY)) {
				mousemng_toggle(MOUSEPROC_SYSTEM);
				np2oscfg.MOUSE_SW = !np2oscfg.MOUSE_SW;
				if(np2oscfg.mouse_nc){
					if (np2oscfg.wintype != 0) {
						// XXX: メニューが出せなくなって詰むのを回避（暫定）
						if (!scrnmng_isfullscreen()) {
							WINLOCEX	wlex;
							np2oscfg.wintype = 0;
							wlex = np2_winlocexallwin(hWnd);
							winlocex_setholdwnd(wlex, hWnd);
							np2class_windowtype(hWnd, np2oscfg.wintype);
							winlocex_move(wlex);
							winlocex_destroy(wlex);
							sysmng_update(SYS_UPDATEOSCFG);
						}
					}
				}
				if (np2oscfg.MOUSE_SW || np2oscfg.mouse_nc) {
					SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) & ~CS_DBLCLKS);
				}
				else {
					SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) | CS_DBLCLKS);
				}
				sysmng_update(SYS_UPDATECFG);
			}
			else if ((wParam == VK_F12) && (np2oscfg.F12COPY==7)) {
				np2oscfg.NOWAIT = 1;
				update |= SYS_UPDATECFG;
			}
			else if ((wParam == VK_F12) && (np2oscfg.F12COPY==8)) {
				np2oscfg.NOWAIT = !np2oscfg.NOWAIT;
				update |= SYS_UPDATECFG;
			}
#if defined(SUPPORT_CL_GD5430) && defined(SUPPORT_WAB)
			else if ((wParam == VK_F12) && (np2oscfg.F12COPY==9)) {
				np2_multithread_EnterCriticalSection();
				if(np2clvga.enabled && cirrusvga_opaque){
					np2wab_setRelayState(np2wab.relay ? 0 : 1);
				}
				np2_multithread_LeaveCriticalSection();
			}
#endif
#ifdef HOOK_SYSKEY
			else if ((wParam == VK_SNAPSHOT) && (np2oscfg.syskhook)) {
				// nothing to do
			}
#endif
			else {
				if (!np2userpause) {
					np2_multithread_EnterCriticalSection();
					winkbd_keydown(wParam, lParam);
					np2_multithread_LeaveCriticalSection();
				}
			}
			break;

		case WM_KEYUP:
			if (wParam == VK_F11) {
				return(DefWindowProc(hWnd, WM_SYSKEYUP, VK_F10, lParam));
			}
			if ((wParam == VK_F12) && (np2oscfg.F12COPY==7)) {
				np2oscfg.NOWAIT = 0;
				update |= SYS_UPDATECFG;
			}
			else if ((wParam != VK_F12) || (np2oscfg.F12COPY && np2oscfg.F12COPY!=7)) {
				if (!np2userpause) {
					np2_multithread_EnterCriticalSection();
					winkbd_keyup(wParam, lParam);
					np2_multithread_LeaveCriticalSection();
				}
			}
			break;

		case WM_SYSKEYDOWN:
			autokey_sendbufferpos = autokey_sendbufferlen; // コピペ強制終了 np21w ver0.86 rev22
#ifdef HOOK_SYSKEY
			if (GetAsyncKeyState (VK_RMENU) >> ((sizeof(SHORT) * 8) - 1)) {	// np21w ver0.86 rev6	
#else
			if (lParam & 0x20000000) {								// ver0.30
#endif
				if ((np2oscfg.shortcut & 1) && (wParam == VK_RETURN)) {
					changescreen(g_scrnmode ^ SCRNMODE_FULLSCREEN);
					break;
				}
				if ((np2oscfg.shortcut & 2) && (wParam == VK_F4)) {
					SendMessage(hWnd, WM_CLOSE, 0, 0L);
					break;
				}
				if (np2oscfg.mouse_nc && np2oscfg.wintype != 0) {
					// XXX: メニューが出せなくなって詰むのを回避（暫定）
					if (!scrnmng_isfullscreen()) {
						np2oscfg.wintype = 0;
						wlex = np2_winlocexallwin(hWnd);
						winlocex_setholdwnd(wlex, hWnd);
						np2class_windowtype(hWnd, np2oscfg.wintype);
						winlocex_move(wlex);
						winlocex_destroy(wlex);
						sysmng_update(SYS_UPDATEOSCFG);
						break;
					}
				}
			}
			if (!np2userpause) {
				np2_multithread_EnterCriticalSection();
				winkbd_keydown(wParam, lParam);
				np2_multithread_LeaveCriticalSection();
			}
			break;

		case WM_SYSKEYUP:
			if (!np2userpause) {
				np2_multithread_EnterCriticalSection();
				winkbd_keyup(wParam, lParam);
				np2_multithread_LeaveCriticalSection();
			}
			break;

		case WM_MOUSEMOVE:
			np2_multithread_EnterCriticalSection();
			if (scrnmng_isfullscreen()) {
				POINT p;
				if (GetCursorPos(&p)) {
					scrnmng_fullscrnmenu(p.y);
				}
			}
			if(np2oscfg.mouse_nc){
				RECT rectClient;
				int xPos, yPos;
				int mouseon = 1;
				static int mousebufX = 0; // マウス移動バッファ(X)
				static int mousebufY = 0; // マウス移動バッファ(Y)
				int x = (short)LOWORD(lParam);
				int y = (short)HIWORD(lParam);

				SINT16 dx, dy;
				UINT8 btn;
				btn = mousemng_getstat(&dx, &dy, 0);
				if(lastmx == -1 || lastmy == -1){
					lastmx = x;
					lastmy = y;
				}
				if((x-lastmx) || (y-lastmy)){
					RECT r;
					int mouse_edge_sh_x = 100;
					int mouse_edge_sh_y = 100;
					int dxmul, dymul;
					GetClientRect(hWnd, &r);
					mouse_edge_sh_x = (r.right-r.left)/8;
					mouse_edge_sh_y = (r.bottom-r.top)/8;
					mousebufX += ((x-lastmx)*np2oscfg.mousemul);
					mousebufY += ((y-lastmy)*np2oscfg.mousemul);
					if(mousebufX >= np2oscfg.mousediv || mousebufX <= -np2oscfg.mousediv){
						dx += (SINT16)(mousebufX / np2oscfg.mousediv);
						mousebufX   = mousebufX % np2oscfg.mousediv;
					}
					if(mousebufY >= np2oscfg.mousediv || mousebufY <= -np2oscfg.mousediv){
						dy += (SINT16)(mousebufY / np2oscfg.mousediv);
						mousebufY   = mousebufY % np2oscfg.mousediv;
					}
					// XXX: 端実験
#define MOUSE_EDGE_ACM	4
					if(x<mouse_edge_sh_x && dx < 0){
						dxmul = 1+(mouse_edge_sh_x - x)*MOUSE_EDGE_ACM/mouse_edge_sh_x;
					}else if(r.right-mouse_edge_sh_x <= x && dx > 0){
						dxmul = 1+(mouse_edge_sh_x - (r.right-x))*MOUSE_EDGE_ACM/mouse_edge_sh_x;
					}else{
						dxmul = 1;
					}
					if(y<mouse_edge_sh_y && dy < 0){
						dymul = 1+(mouse_edge_sh_y - y)*MOUSE_EDGE_ACM/mouse_edge_sh_y;
					}else if(r.bottom-mouse_edge_sh_y <= y && dy > 0){
						dymul = 1+(mouse_edge_sh_y - (r.bottom-y))*MOUSE_EDGE_ACM/mouse_edge_sh_y;
					}else{
						dymul = 1;
					}
					dxmul = (int)dx * dxmul;
					dymul = (int)dy * dymul;
					if(dxmul < -128) dxmul = -128;
					if(dxmul > +127) dxmul = +127;
					if(dymul < -128) dymul = -128;
					if(dymul > +127) dymul = +127;
					dx = (SINT16)dxmul;
					dy = (SINT16)dymul;
					mousemng_setstat(dx, dy, btn);
					lastmx = x;
					lastmy = y;

					// 絶対座標マウス
					scrnmng_getrect(&rectClient);
					xPos = x - rectClient.left;
					yPos = y - rectClient.top;
					if (xPos < 0)
					{
						xPos = 0;
						mouseon = 0;
					}
					if (xPos > (rectClient.right - rectClient.left))
					{
						xPos = (rectClient.right - rectClient.left);
						mouseon = 0;
					}
					if (yPos < 0)
					{
						yPos = 0;
						mouseon = 0;
					}
					if (yPos > (rectClient.bottom - rectClient.top))
					{
						yPos = (rectClient.bottom - rectClient.top);
						mouseon = 0;
					}
					mousemng_updatemouseon(mouseon);
					if (rectClient.right - rectClient.left > 0 && rectClient.bottom - rectClient.top > 0)
					{
						xPos = xPos * 65535 / (rectClient.right - rectClient.left);
						yPos = yPos * 65535 / (rectClient.bottom - rectClient.top);
						mousemng_setabspos(xPos, yPos);
					}
				}
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_NCMOUSEMOVE:
			mousemng_updatemouseon(false);
			break;

		case WM_LBUTTONDOWN:
			np2_multithread_EnterCriticalSection();
			autokey_sendbufferpos = autokey_sendbufferlen; // コピペ強制終了 np21w ver0.86 rev22
			if (!mousemng_buttonevent(MOUSEMNG_LEFTDOWN)) {
				if (!scrnmng_isfullscreen()) {
					if (np2oscfg.wintype == 2) {
						np2_multithread_LeaveCriticalSection();
						return(SendMessage(hWnd, WM_NCLBUTTONDOWN,
															HTCAPTION, 0L));
					}
				}
#if defined(SUPPORT_DCLOCK)
				else {
					POINT p;
					if ((GetCursorPos(&p)) &&
						(scrnmng_isdispclockclick(&p))) {
						np2oscfg.clk_x++;
						sysmng_update(SYS_UPDATEOSCFG);
						DispClock::GetInstance()->Reset();
					}
				}
#endif
				np2_multithread_LeaveCriticalSection();
				return(DefWindowProc(hWnd, msg, wParam, lParam));
			}
			if (!mousecapturemode && !np2oscfg.MOUSE_SW && np2oscfg.mouse_nc)
			{
				SetCapture(hWnd);
				mousecapturemode = 1;
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_LBUTTONUP:
			np2_multithread_EnterCriticalSection();
			if (!mousemng_buttonevent(MOUSEMNG_LEFTUP)) {
				np2_multithread_LeaveCriticalSection();
				return(DefWindowProc(hWnd, msg, wParam, lParam));
			}
			if (mousecapturemode)
			{
				ReleaseCapture();
				mousecapturemode = 0;
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_MBUTTONDOWN:
			np2_multithread_EnterCriticalSection();
			autokey_sendbufferpos = autokey_sendbufferlen; // コピペ強制終了 np21w ver0.86 rev22
			mousemng_toggle(MOUSEPROC_SYSTEM);
			np2oscfg.MOUSE_SW = !np2oscfg.MOUSE_SW;
			sysmng_update(SYS_UPDATECFG);
			if(np2oscfg.mouse_nc){
				if (np2oscfg.wintype != 0) {
					// XXX: メニューが出せなくなって詰むのを回避（暫定）
					if (!scrnmng_isfullscreen()) {
						WINLOCEX	wlex;
						np2oscfg.wintype = 0;
						wlex = np2_winlocexallwin(hWnd);
						winlocex_setholdwnd(wlex, hWnd);
						np2class_windowtype(hWnd, np2oscfg.wintype);
						winlocex_move(wlex);
						winlocex_destroy(wlex);
						sysmng_update(SYS_UPDATEOSCFG);
					}
				}
			}
			if (np2oscfg.MOUSE_SW || np2oscfg.mouse_nc) {
				SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) & ~CS_DBLCLKS);
			}
			else {
				SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) | CS_DBLCLKS);
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_RBUTTONDOWN:
			np2_multithread_EnterCriticalSection();
			autokey_sendbufferpos = autokey_sendbufferlen; // コピペ強制終了 np21w ver0.86 rev22
			if (!mousemng_buttonevent(MOUSEMNG_RIGHTDOWN)) {
				if (!scrnmng_isfullscreen()) {
					np2popup(hWnd, lParam);
				}
#if defined(SUPPORT_DCLOCK)
				else {
					POINT p;
					if ((GetCursorPos(&p)) &&
						(scrnmng_isdispclockclick(&p)) &&
						(np2oscfg.clk_x)) {
						np2oscfg.clk_fnt++;
						sysmng_update(SYS_UPDATEOSCFG);
						DispClock::GetInstance()->Reset();
					}
				}
#endif
				np2_multithread_LeaveCriticalSection();
				return(DefWindowProc(hWnd, msg, wParam, lParam));
			}
			if (!mousecapturemode && !np2oscfg.MOUSE_SW && np2oscfg.mouse_nc)
			{
				SetCapture(hWnd);
				mousecapturemode = 1;
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_RBUTTONUP:
			np2_multithread_EnterCriticalSection();
			if (!mousemng_buttonevent(MOUSEMNG_RIGHTUP)) {
				np2_multithread_LeaveCriticalSection();
				return(DefWindowProc(hWnd, msg, wParam, lParam));
			}
			if (mousecapturemode)
			{
				ReleaseCapture();
				mousecapturemode = 0;
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case WM_LBUTTONDBLCLK:
			if(np2oscfg.mouse_nc){
				if (np2oscfg.wintype != 0) {
					// XXX: メニューが出せなくなって詰むのを回避（暫定）
					if (!scrnmng_isfullscreen()) {
						WINLOCEX	wlex;
						np2oscfg.wintype = 0;
						wlex = np2_winlocexallwin(hWnd);
						winlocex_setholdwnd(wlex, hWnd);
						np2class_windowtype(hWnd, np2oscfg.wintype);
						winlocex_move(wlex);
						winlocex_destroy(wlex);
						sysmng_update(SYS_UPDATEOSCFG);
					}
				}
			}
			else {
				if (!scrnmng_isfullscreen())
				{
					np2oscfg.wintype++;
					if (np2oscfg.wintype >= 3)
					{
						np2oscfg.wintype = 0;
					}
					wlex = np2_winlocexallwin(hWnd);
					winlocex_setholdwnd(wlex, hWnd);
					np2class_windowtype(hWnd, np2oscfg.wintype);
					winlocex_move(wlex);
					winlocex_destroy(wlex);
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
			if (np2oscfg.MOUSE_SW || np2oscfg.mouse_nc) {
				SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) & ~CS_DBLCLKS);
			}
			else {
				SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) | CS_DBLCLKS);
			}
			break;

		case WM_RBUTTONDBLCLK:
			if (!np2oscfg.MOUSE_SW) {
				if(np2oscfg.mouse_nc){
					if (np2oscfg.wintype != 0) {
						// XXX: メニューが出せなくなって詰むのを回避（暫定）
						if (!scrnmng_isfullscreen()) {
							WINLOCEX	wlex;
							np2oscfg.wintype = 0;
							wlex = np2_winlocexallwin(hWnd);
							winlocex_setholdwnd(wlex, hWnd);
							np2class_windowtype(hWnd, np2oscfg.wintype);
							winlocex_move(wlex);
							winlocex_destroy(wlex);
							sysmng_update(SYS_UPDATEOSCFG);
						}
					}
				}
			}
			if (np2oscfg.MOUSE_SW || np2oscfg.mouse_nc)
			{
				SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) & ~CS_DBLCLKS);
			}
			else
			{
				SetClassLong(g_hWndMain, GCL_STYLE, GetClassLong(g_hWndMain, GCL_STYLE) | CS_DBLCLKS);
			}
			break;

		case WM_MOUSEWHEEL:
			if(np2oscfg.usewheel){
				if ((wParam & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT)) {
					int mmul = np2oscfg.mousemul;
					int mdiv = np2oscfg.mousediv;
					// 面倒なので x/2にする
					if(mdiv == 1) {
						mdiv *= 2;
						mmul *= 2;
					}
					if(GET_WHEEL_DELTA_WPARAM(wParam) > 0){
						if(mdiv <= 2){
							mdiv = 2;
							mmul++;
						}else{
							mdiv--;
						}
					}else{
						if(mmul <= 2){
							mmul = 2;
							mdiv++;
						}else{
							mmul--;
						}
					}
					if(mmul > 8) mmul = 8;
					if(mdiv > 8) mdiv = 8;
					// 2で割れるなら割っておく
					if(mdiv == 2 && mmul%2 == 0) {
						mdiv /= 2;
						mmul /= 2;
					}
					np2oscfg.mousemul = mmul;
					np2oscfg.mousediv = mdiv;
					mousemng_updatespeed();
					sys_miscinfo.showmousespeed = 1;
					sysmng_updatecaption(SYS_UPDATECAPTION_MISC);
					tmrSysMngHide = SetTimer(hWnd, TMRSYSMNG_ID, 5000, SysMngHideTimerProc);
				}else{
					//if(np2oscfg.usemastervolume){
						int cMaster = np2cfg.vol_master;
						cMaster += GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * 2;
						if(cMaster < 0) cMaster = 0;
						if(cMaster > np2oscfg.mastervolumemax) cMaster = np2oscfg.mastervolumemax;
						if (np2cfg.vol_master != cMaster)
						{
							np2_multithread_EnterCriticalSection();
							np2cfg.vol_master = cMaster;
							soundmng_setvolume(cMaster);
							fmboard_updatevolume();
							np2_multithread_LeaveCriticalSection();
						}
						sys_miscinfo.showvolume = 1;
						sysmng_updatecaption(SYS_UPDATECAPTION_MISC);
						tmrSysMngHide = SetTimer(hWnd, TMRSYSMNG_ID, 5000, SysMngHideTimerProc);
					//}
				}
			}
			break;

		case WM_CAPTURECHANGED:
			mousecapturemode = 0;
			return 0;

			
#if defined(SUPPORT_SCRN_DIRECT3D)
		case WM_SETFOCUS:
			if (!screenChanging && scrnmng_isfullscreen() && scrnmng_current_drawtype==DRAWTYPE_DIRECT3D && !np2oscfg.d3d_exclusive && !winui_en) {
				ShowWindow( hWnd, SW_RESTORE );
			}
			break;

		case WM_KILLFOCUS:
			if (!screenChanging && scrnmng_isfullscreen() && scrnmng_current_drawtype==DRAWTYPE_DIRECT3D && !np2oscfg.d3d_exclusive && !winui_en) {
				ShowWindow( hWnd, SW_MINIMIZE );
			}
			break;
#endif

		case WM_CLOSE:
			b = FALSE;
			if (!np2oscfg.comfirm) {
				b = TRUE;
			}
			else
			{
				winuienter();
				if (messagebox(hWnd, MAKEINTRESOURCE(IDS_CONFIRM_EXIT),
									MB_ICONQUESTION | MB_YESNO) == IDYES)
				{
					b = TRUE;
				}
				winuileave();
			}
			if (b) {
				// 初期画面サイズに戻す
				np2_multithread_EnterCriticalSection();
				scrnmng_setsize(0, 0, 640, 400);
				np2_multithread_LeaveCriticalSection();
				if (np2oscfg.WINSNAP) {
					RECT currect;
					GetWindowRect(hWnd, &currect);
					winlocex_moving(smwlex, &currect);
				}

				CDebugUtyView::AllClose();
				CDebugUtyView::DisposeAllClosedWindow();
				DestroyWindow(hWnd);
			}
			break;

		case WM_DESTROY:
			np2class_wmdestroy(hWnd);
			PostQuitMessage(0);
			break;

		case WM_NP2CMD:
			np2_multithread_EnterCriticalSection();
			switch(LOWORD(lParam)) {
				case NP2CMD_EXIT:
					np2quitmsg = 1;
					PostQuitMessage(0);
					break;

				case NP2CMD_EXIT2:
					np2quitmsg = 2;
					PostQuitMessage(0);
					break;

				case NP2CMD_RESET:
					pccore_cfgupdate();
#ifdef HOOK_SYSKEY
					stop_hook_systemkey();
#endif
					np2_multithread_Suspend();
					if(nevent_iswork(NEVENT_CDWAIT)){
						nevent_forceexecute(NEVENT_CDWAIT);
					}
					pccore_reset();
					np2_SetUserPause(0);
					np2_multithread_Resume();
#ifdef HOOK_SYSKEY
					start_hook_systemkey();
#endif
					break;
			}
			np2_multithread_LeaveCriticalSection();
			break;

		case MM_MIM_DATA:
			CComMidiIn32::RecvData(reinterpret_cast<HMIDIIN>(wParam), static_cast<UINT>(lParam));
			break;

		case MM_MIM_LONGDATA:
			CComMidiIn32::RecvExcv(reinterpret_cast<HMIDIIN>(wParam), reinterpret_cast<MIDIHDR*>(lParam));
			break;
			
#if defined(SUPPORT_IDEIO)
#ifdef SUPPORT_PHYSICAL_CDDRV
		case WM_DEVICECHANGE:
			{
				PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;
				switch(wParam)
				{
				case DBT_DEVICEARRIVAL:
				case DBT_DEVICEREMOVECOMPLETE:
					// See if a CD-ROM or DVD was inserted into a drive.
					if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
					{
						PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;

						if (lpdbv -> dbcv_flags & DBTF_MEDIA)
						{
							int i;
							int drvlnum;
							int unitmask = lpdbv ->dbcv_unitmask;
							OEMCHAR *fname;
							OEMCHAR fnamebuf[MAX_PATH];
							OEMCHAR drvstr[] = OEMTEXT("x:");
							for (drvlnum = 0; drvlnum < 26; ++drvlnum)
							{
								if (unitmask & 0x1)
									break;
								unitmask = unitmask >> 1;
							}
							drvstr[0] = 'A' + drvlnum;
							np2_multithread_EnterCriticalSection();
							for(i=0;i<4;i++){
								if(sxsi_getdevtype(i)==SXSIDEV_CDROM){
									fname = np2cfg.idecd[i];
									if(_tcsnicmp(fname, OEMTEXT("\\\\.\\"), 4)==0){
										fname += 4;
										if(_tcsicmp(fname, drvstr)==0){
											_tcscpy(fnamebuf, np2cfg.idecd[i]);
											if(wParam == DBT_DEVICEARRIVAL){
												// CD挿入
												diskdrv_setsxsi(i, fnamebuf);
											}else{
												// CD取出 XXX: 中身が空でもマウントは継続
												diskdrv_setsxsi(i, NULL);
												_tcscpy(np2cfg.idecd[i], fnamebuf);
											}
											sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
										}
									}
								}
							}
							np2_multithread_LeaveCriticalSection();
						}
					}
					break;
				}
			}
			break;
#endif
#endif

		default:
#ifdef SUPPORT_IDEIO
			if(msg == WM_QueryCancelAutoPlay){
				int i;
				for(i=0;i<4;i++){
					if(sxsi_getdevtype(i)==SXSIDEV_CDROM){
						if ((np2cfg.idecd[i][0]==_T('\0') || np2cfg.idecd[i][0]==_T('\\')) && np2cfg.idecd[i][1]==_T('\\') && np2cfg.idecd[i][2]==_T('.') && np2cfg.idecd[i][3]==_T('\\')) {
							return 1;
						}
					}
				}
			}
#endif
			
			return(DefWindowProc(hWnd, msg, wParam, lParam));
	}
	return(0L);
}

// --- auto sendkey

static unsigned short sjis_to_jis(unsigned short sjis)
{
	int h = sjis >> 8;
	int l = sjis & 0xff;

	if (h <= 0x9f)
	{
		if (l < 0x9f)
			h = (h << 1) - 0xe1;
		else
			h = (h << 1) - 0xe0;
	}
	else
	{
		if (l < 0x9f)
			h = (h << 1) - 0x161;
		else
			h = (h << 1) - 0x160;
	}

	if (l < 0x7f)
		l -= 0x1f;
	else if (l < 0x9f)
		l -= 0x20;
	else
		l -= 0x7e;
	return h << 8 | l;
}

void autoSendKey(){
	static int shift = 0;
	static int kanjimode = 0;
	static DWORD lastsendtime = 0;
	int capslock = 0;
	//int i;
	DWORD curtime = 0;
	
	// 送るものなし
	if (autokey_sendbufferlen == 0)
	{
		return;
	}
	
	// 10文字だけ送る(入力速度制御付き)
	curtime = GetTickCount();
	if(curtime - lastsendtime > 256/pccore.multiple+8){
		int i;
		int maxkey = 1;//((int)pccore.multiple-20)/16;
		if(maxkey <= 0) maxkey = 1;
		for(i=0;i<maxkey;i++){
			if(keybrd.buffers < KB_BUF/2 && autokey_sendbufferpos < autokey_sendbufferlen){
				UINT8 sendchar = ((UINT8*)autokey_sendbuffer)[autokey_sendbufferpos];
				int isKanji = 0;
				if(sendchar){
					np2_multithread_EnterCriticalSection();
					if(sendchar <= 0x7f){
						// ASCII
						if (kanjimode)
						{
							keystat_senddata(0x00 | 0x74);
							keystat_senddata(0x00 | 0x35);
							keystat_senddata(0x80 | 0x35);
							keystat_senddata(0x80 | 0x74);
							kanjimode = 0;
						}
						if (keyctrl.kanaref != 0xff)
						{
							keystat_senddata(0x00 | 0x72);
							keystat_senddata(0x80 | 0x72);
							i++;
						}
						if (vkeylist[sendchar])
						{
							if ((shift_on[sendchar]) && !(capslock ^ shift))
							{
								keystat_senddata(0x00 | 0x70);
								shift = 1;
							}
							if ((!shift_on[sendchar]) && (capslock ^ shift))
							{
								keystat_senddata(0x80 | 0x70);
								shift = 0;
							}
							keystat_senddata(0x00 | vkeylist[sendchar]);
							keystat_senddata(0x80 | vkeylist[sendchar]);
						}
					}else if(0xA1 <= sendchar && sendchar <= 0xDF){
						// 半角ｶﾅ
						if (kanjimode)
						{
							keystat_senddata(0x00 | 0x74);
							keystat_senddata(0x00 | 0x35);
							keystat_senddata(0x80 | 0x35);
							keystat_senddata(0x80 | 0x74);
							kanjimode = 0;
							i+=2;
						}
						if (keyctrl.kanaref == 0xff)
						{
							keystat_senddata(0x00 | 0x72);
							keystat_senddata(0x80 | 0x72);
							i++;
						}
						if (vkeylist[sendchar])
						{
							if ((shift_on[sendchar]) && !(capslock ^ shift))
							{
								keystat_senddata(0x00 | 0x70);
								shift = 1;
							}
							if ((!shift_on[sendchar]) && (capslock ^ shift))
							{
								keystat_senddata(0x80 | 0x70);
								shift = 0;
							}
							keystat_senddata(0x00 | vkeylist[sendchar]);
							keystat_senddata(0x80 | vkeylist[sendchar]);
						}
					}else if(0x80 <= sendchar){
						// 多分2byte文字
						if (np2oscfg.knjpaste)
						{
							isKanji = 1;
							if ((capslock ^ shift))
							{
								keystat_senddata(0x80 | 0x70);
								shift = 0;
							}
							if (keyctrl.kanaref != 0xff)
							{
								keystat_senddata(0x00 | 0x72);
								keystat_senddata(0x80 | 0x72);
								i++;
							}
							if (!kanjimode)
							{
								keystat_senddata(0x00 | 0x74);
								keystat_senddata(0x00 | 0x35);
								keystat_senddata(0x80 | 0x35);
								keystat_senddata(0x80 | 0x74);
								kanjimode = 1;
								i += 2;
							}
							UINT8 sendchar2 = ((UINT8*)autokey_sendbuffer)[autokey_sendbufferpos + 1];
							unsigned short jiscode = sjis_to_jis(((unsigned short)sendchar << 8) | (unsigned short)sendchar2);
							UINT8 hexToAsc[] = { '0', '1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,'a' ,'b' ,'c' ,'d' ,'e' ,'f' };
							keystat_senddata(0x00 | vkeylist[hexToAsc[((jiscode >> 12) & 0xf)]]);
							keystat_senddata(0x80 | vkeylist[hexToAsc[((jiscode >> 12) & 0xf)]]);
							keystat_senddata(0x00 | vkeylist[hexToAsc[((jiscode >> 8) & 0xf)]]);
							keystat_senddata(0x80 | vkeylist[hexToAsc[((jiscode >> 8) & 0xf)]]);
							keystat_senddata(0x00 | vkeylist[hexToAsc[((jiscode >> 4) & 0xf)]]);
							keystat_senddata(0x80 | vkeylist[hexToAsc[((jiscode >> 4) & 0xf)]]);
							keystat_senddata(0x00 | vkeylist[hexToAsc[((jiscode) & 0xf)]]);
							keystat_senddata(0x80 | vkeylist[hexToAsc[((jiscode) & 0xf)]]);
							if (np2oscfg.knjpaste == 2)
							{
								// DOSは1文字毎に解除される
								kanjimode = 0;
							}
							i += 16;
						}
						autokey_sendbufferpos++;
					}
					else
					{
						i--;
					}
					np2_multithread_LeaveCriticalSection();
				}
				autokey_sendbufferpos++;
				//if (isKanji)
				//{
				//	// 漢字は慎重に送る
				//	break;
				//}
				if (keybrd.buffers > KB_BUF * 3 / 4)
				{
					// 溢れそうなので一旦逃げる
					break;
				}
			}
		}
		lastsendtime = curtime;
	}

	// 送信完了したら
	if(autokey_sendbufferpos >= autokey_sendbufferlen){
		if (kanjimode)
		{
			keystat_senddata(0x00 | 0x74);
			keystat_senddata(0x00 | 0x35);
			keystat_senddata(0x80 | 0x35);
			keystat_senddata(0x80 | 0x74);
			kanjimode = 0;
		}
		if (autokey_lastkanastate != 0xff && keyctrl.kanaref == 0xff || autokey_lastkanastate == 0xff && keyctrl.kanaref != 0xff)
		{
			keystat_senddata(0x00 | 0x72);
			keystat_senddata(0x80 | 0x72);
		}
		keystat_senddata(0x80|0x70);
		autokey_sendbufferlen = 0;
		autokey_sendbufferpos = 0;
		_MFREE(autokey_sendbuffer);
		autokey_sendbuffer = NULL;
		shift = 0;
	}
}

// キーコード表作成
void createAsciiTo98KeyCodeList(){
	int i;
	// キーコード表作成（暫定）
	UINT8 numkeys[] = {0,'!', '"','#','$','%','&','\'','(',')'};
	for(i='0';i<='9';i++){
		vkeylist[i] = i-'0';
		if(i=='0') vkeylist[i] = 0x0a;
		vkeylist[numkeys[i-'0']] = vkeylist[i];
		shift_on[numkeys[i-'0']] = 1;
	}
	char asckeycode[] = {0x1d,0x2d,0x2b,0x1f,0x12,0x20,0x21,0x22,0x17,0x23,0x24,0x25,0x2f,0x2e,0x18,0x19,0x10,0x13,0x1e,0x14,0x16,0x2c,0x11,0x2a,0x15,0x29};
	for(i='A';i<='Z';i++){
		vkeylist[i] = asckeycode[i-'A'];
		shift_on[i] = 1;
		vkeylist[i+0x20] = vkeylist[i];
	}
	UINT8 spkeyascii[] = { '-', '^','\\', '@', '[', ';', ':', ']', ',', '.', '/', '_'};
	UINT8 spshascii[]  = { '=', '`', '|', '~', '{', '+', '*', '}', '<', '>', '?', '_'};
	char spkeycode[]  = {0x0B,0x0C,0x0D,0x1A,0x1B,0x26,0x27,0x28,0x30,0x31,0x32,0x33};
	vkeylist[' '] = 0x34;
	vkeylist['\t'] = 0x0f;
	vkeylist['\n'] = 0x1c;
	for(i=0;i<NELEMENTS(spkeyascii);i++){
		vkeylist[spkeyascii[i]] = spkeycode[i];
		vkeylist[spshascii[i] ] = spkeycode[i];
		shift_on[spshascii[i] ] = 1;
	}
	char kanakeycode[] = { 0x31,0x1b,0x28,0x30,0x32,0x0a,0x03,0x12,0x04,0x05,0x06,0x07,0x08,0x09,0x29,0x0d,0x03,0x12,0x04,0x05,0x06,0x14,0x21,0x22,0x27,0x2d,0x2a,0x1f,0x13,0x19,0x2b,0x10,0x1d,0x29,0x11,0x1e,0x16,0x17,0x01,0x30,0x24,0x20,0x2c,0x02,0x0c,0x0b,0x23,0x2e,0x28,0x32,0x2f,0x07,0x08,0x09,0x18,0x25,0x31,0x26,0x33,0x0a,0x15,0x1a,0x1b };
	for (i = 0xa1; i <= 0xdf; i++)
	{
		vkeylist[i] = kanakeycode[i - 0xa1];
		if (i <= 0xaf)
		{
			shift_on[i] = 1;
		}
		else
		{
			shift_on[i] = 0;
		}
	}
}

#ifdef HOOK_SYSKEY
// システムショートカットキー
LRESULT CALLBACK LowLevelKeyboardProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if(np2oscfg.syskhook){
		// By returning a non-zero value from the hook procedure, the
		// message does not get passed to the target window
		KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *) lParam;
		BOOL bControlKeyDown = 0;
		BOOL bShiftKeyDown = 0;
		BOOL bAltKeyDown = 0;

		switch (nCode)
		{
			case HC_ACTION:
			{
				if(GetForegroundWindow()==g_hWndMain){
					KBDLLHOOKSTRUCT *kbstruct = (KBDLLHOOKSTRUCT*)lParam;
					// Check to see if the CTRL,DHIFT,ALT key is pressed
					bControlKeyDown = GetAsyncKeyState (VK_LCONTROL) >> ((sizeof(SHORT) * 8) - 1);
					bShiftKeyDown = GetAsyncKeyState (VK_LSHIFT) >> ((sizeof(SHORT) * 8) - 1);
					bAltKeyDown = GetAsyncKeyState (VK_LMENU) >> ((sizeof(SHORT) * 8) - 1);
            
					// Disable CTRL+ESC, ALT+TAB, ALT+ESC
					if (pkbhs->vkCode == VK_ESCAPE && bControlKeyDown
						|| pkbhs->vkCode == VK_TAB && bAltKeyDown
						|| pkbhs->vkCode == VK_ESCAPE && bAltKeyDown
						|| pkbhs->vkCode == VK_LWIN
						|| pkbhs->vkCode == VK_APPS){
							
						switch((int)wParam){
						case WM_KEYDOWN:
						case WM_SYSKEYDOWN:
							//np2_multithread_EnterCriticalSection();
							winkbd_keydown(kbstruct->vkCode, ((kbstruct->flags)<<24)|(kbstruct->scanCode<<16));
							//np2_multithread_LeaveCriticalSection();
							break;
						case WM_KEYUP:
						case WM_SYSKEYUP:
							//np2_multithread_EnterCriticalSection();
							winkbd_keyup(kbstruct->vkCode, ((kbstruct->flags)<<24)|(kbstruct->scanCode<<16));
							//np2_multithread_LeaveCriticalSection();
							break;
						}
						return 1;
					}
					if(pkbhs->vkCode == VK_SCROLL && bAltKeyDown && bControlKeyDown){
						// Ctrl+Alt+ScrollLock → Ctrl+Alt+Delete
						switch((int)wParam){
						case WM_KEYDOWN:
						case WM_SYSKEYDOWN:
							//np2_multithread_EnterCriticalSection();
							keystat_keydown(0x39);
							//np2_multithread_LeaveCriticalSection();
							break;
						case WM_KEYUP:
						case WM_SYSKEYUP:
							//np2_multithread_EnterCriticalSection();
							keystat_keyup(0x39);
							//np2_multithread_LeaveCriticalSection();
							break;
						}
						return 1;
					}
#ifdef HOOK_SYSKEY
					else if ((pkbhs->vkCode == VK_SNAPSHOT) && (np2oscfg.syskhook)) {
						// PrintScreen -> COPY
						switch((int)wParam){
						case WM_KEYDOWN:
						case WM_SYSKEYDOWN:
							//np2_multithread_EnterCriticalSection();
							keystat_keydown(0x61);
							//np2_multithread_LeaveCriticalSection();
							break;
						case WM_KEYUP:
						case WM_SYSKEYUP:
							////np2_multithread_EnterCriticalSection();
							keystat_keyup(0x61);
							//np2_multithread_LeaveCriticalSection();
							break;
						}
					}
#endif
				}
				break;
			}

			default:
				break;
		}
	}
    return CallNextHookEx (hHook, nCode, wParam, lParam);
}
#endif

/**
 * 1フレーム実行
 * @param[in] bDraw 描画フラグ
 */
static void ExecuteOneFrame_MT_EmulateThread(BOOL bDraw)
{
	if (recvideo_isEnabled())
	{
		bDraw = TRUE;
	}

	pccore_exec(bDraw);

	recvideo_write();
}
static void ExecuteOneFrame(BOOL bDraw)
{
	if (recvideo_isEnabled())
	{
		bDraw = TRUE;
	}

	joymng_sync();
	mousemng_sync();
	pccore_exec(bDraw);
	recvideo_write();
#if defined(SUPPORT_DCLOCK)
	DispClock::GetInstance()->Update();
#endif
#if defined(SUPPORT_VSTi)
	CVstEditWnd::OnIdle();
#endif	// defined(SUPPORT_VSTi)
}

static void framereset_MT_EmulateThread(UINT cnt) {
	
	framecnt = 0;
}
static void framereset_MT_UIThread(UINT cnt) {
	
	framecntUI = 0;
#if defined(SUPPORT_DCLOCK)
	scrnmng_dispclock();
#endif
	kdispwin_draw((UINT8)cnt);
	sysmng_requestupdatecheck();
	skbdwin_process();
	mdbgwin_process();
	toolwin_draw((UINT8)cnt);
	CDebugUtyView::AllUpdate(false);
	if (np2oscfg.DISPCLK & 3) {
		if (sysmng_workclockrenewal()) {
			sysmng_updatecaption(SYS_UPDATECAPTION_CLK);
		}
	}
}
static void framereset_ALL(UINT cnt) {
	framereset_MT_EmulateThread(cnt);
	framereset_MT_UIThread(cnt);
}

static void (*framereset)(UINT cnt) = framereset_ALL;

static void processasyncwait()
{
#if defined(SUPPORT_ASYNC_CPU)
	UINT32 rawTiming = timing_getcount_raw();
	pccore_asynccpustat.lastTimingValue = rawTiming;
#endif
}

static void processwait(UINT cnt) {

	static int frameSleep = 0;

	UINT count = timing_getcount();
	if (count+lateframecount >= cnt) {
		lateframecount = lateframecount + count - cnt;
#if defined(SUPPORT_IA32_HAXM)
		if (np2hax.enable) {
			np2haxcore.hurryup = 0;
			lateframecount = 0;
		}
#endif
		if(lateframecount > np2oscfg.cpustabf) lateframecount = np2oscfg.cpustabf;
		timing_setcount(0);
		framereset(cnt);
		frameSleep = 0;
	}
	else if (frameSleep == 0)
	{
		UINT32 rawTiming = timing_getcount_raw();
		int waitTime = (TIMING_MSSHIFT_VALUE - (rawTiming & TIMING_MSSHIFT_MASK)) / timing_getmsstep();
		waitTime--; // 少し減らす
		if (waitTime > 0)
		{
			if (waitTime > 1000) waitTime = 1000;
			Sleep(waitTime); // 休む
		}
		else if (waitTime == 0)
		{
			Sleep(0);
		}
		frameSleep = 1;
	}
}

void unloadNP2INI(){
	// 旧INI片付け

	// 画面表示倍率を保存
	np2oscfg.scrn_mul = scrnmng_getmultiple();
	toolwin_destroy();
	kdispwin_destroy();
	skbdwin_destroy();
	mdbgwin_destroy();

	pccore_cfgupdate();

	mousemng_disable(MOUSEPROC_WINUI);
	S98_trash();

#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume) {
		flagsave(str_sav);
	}
	else {
		flagdelete(str_sav);
	}
#endif

	np2_multithread_Suspend();

	sxsi_alltrash();
	pccore_term();

	CSoundMng::GetInstance()->Close();
	CSoundMng::Deinitialize();
	scrnmng_destroy();
	recvideo_close();

	mousemng_destroy();

	if (sys_updates	& (SYS_UPDATECFG | SYS_UPDATEOSCFG)) {
		initsave();
		toolwin_writeini();
		kdispwin_writeini();
		skbdwin_writeini();
		mdbgwin_writeini();
	}
#if defined(SUPPORT_HOSTDRV)
	hostdrv_writeini();
#endif	// defined(SUPPORT_HOSTDRV)
#if defined(SUPPORT_WAB)
	wabwin_writeini();
#endif	// defined(SUPPORT_WAB)
	skbdwin_deinitialize();

	np2opening = 1;
	
	InvalidateRect( g_hWndMain, NULL, FALSE );
	UpdateWindow( g_hWndMain );
}
void loadNP2INI(const OEMCHAR *fname){
	HINSTANCE hInstance;
	BOOL		xrollkey;
	//WNDCLASS	wc;
	HWND		hWnd;
	DWORD		style;
	int			i;
#ifdef OPENING_WAIT
	DWORD		tick;
#endif
	
#ifdef HOOK_SYSKEY
	stop_hook_systemkey();
#endif
	
	LPTSTR lpFilenameBuf = (LPTSTR)malloc((_tcslen(fname)+1)*sizeof(TCHAR));
	_tcscpy(lpFilenameBuf, fname);
	hInstance = g_hInstance;

	// 新INI読み込み
	Np2Arg::GetInstance()->setiniFilename(lpFilenameBuf);

	initload();
	toolwin_readini();
	kdispwin_readini();
	skbdwin_readini();
	mdbgwin_readini();
#if defined(SUPPORT_WAB)
	wabwin_readini();
	np2wabcfg.readonly = np2oscfg.readonly;
#endif	// defined(SUPPORT_WAB)
#if defined(SUPPORT_HOSTDRV)
	hostdrv_readini();
#endif	// defined(SUPPORT_HOSTDRV)
	toolwin_readini();

	rand_setseed((unsigned)time(NULL));

	szClassName[0] = (TCHAR)np2oscfg.winid[0];
	szClassName[1] = (TCHAR)np2oscfg.winid[1];
	szClassName[2] = (TCHAR)np2oscfg.winid[2];
	
#if !defined(_WIN64)
	mmxflag = (havemmx())?0:MMXFLAG_NOTSUPPORT;
	mmxflag += (np2oscfg.disablemmx)?MMXFLAG_DISABLE:0;
#endif
	TRACEINIT();

	xrollkey = (np2oscfg.xrollkey == 0);
	if (np2oscfg.KEYBOARD >= KEY_TYPEMAX) {
		int keytype = GetKeyboardType(1);
		if ((keytype & 0xff00) == 0x0d00) {
			np2oscfg.KEYBOARD = KEY_PC98;
			xrollkey = !xrollkey;
		}
		else if (!keytype) {
			np2oscfg.KEYBOARD = KEY_KEY101;
		}
		else {
			np2oscfg.KEYBOARD = KEY_KEY106;
		}
	}
	winkbd_roll(xrollkey);
	winkbd_setf12(np2oscfg.F12COPY);
	keystat_initialize();

	np2class_initialize(hInstance);
	kdispwin_initialize();
	skbdwin_initialize();
	mdbgwin_initialize();
	CDebugUtyView::Initialize(hInstance);

	style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
	if (np2oscfg.thickframe) {
		style |= WS_THICKFRAME;
	}
	hWnd = g_hWndMain;

	mousemng_initialize();

	scrnmng_initialize();

	if(np2oscfg.dragdrop)
		DragAcceptFiles(hWnd, TRUE);	//	イメージファイルのＤ＆Ｄに対応(Kai1)
	else
		DragAcceptFiles(hWnd, FALSE);

	UpdateWindow(hWnd);
	
#ifdef OPENING_WAIT
	tick = GetTickCount();
#endif
	
	HMENU hSysMenu = GetSystemMenu(hWnd, FALSE);
	//sysmenu_initialize(hSysMenu); // 対応面倒くさい
	
	HMENU hMenu = np2class_gethmenu(hWnd);
	//xmenu_initialize(hMenu); // 対応面倒くさい
	xmenu_iniupdate(hMenu);
	xmenu_update(hMenu);
	if (file_attr_c(np2help) == -1)								// ver0.30
	{
		EnableMenuItem(hMenu, IDM_HELP, MF_GRAYED);
	}
	DrawMenuBar(hWnd);
	
	if(np2oscfg.savescrn){
		g_scrnmode = np2oscfg.scrnmode;
	}else{
		g_scrnmode = np2oscfg.scrnmode = 0;
	}
	if (Np2Arg::GetInstance()->fullscreen())
	{
		g_scrnmode |= SCRNMODE_FULLSCREEN;
	}
	if (np2cfg.RASTER) {
		g_scrnmode |= SCRNMODE_HIGHCOLOR;
	}
	if (scrnmng_create(g_scrnmode) != SUCCESS) {
		g_scrnmode ^= SCRNMODE_FULLSCREEN;
		if (scrnmng_create(g_scrnmode) != SUCCESS) {
			messagebox(hWnd, MAKEINTRESOURCE(IDS_ERROR_DIRECTDRAW), MB_OK | MB_ICONSTOP);
			UnloadExternalResource();
			TRACETERM();
			dosio_term();
			SendMessage(hWnd, WM_CLOSE, 0, 0L);
			return;
		}
	}

	CSoundMng::Initialize();
	OpenSoundDevice(hWnd);

	if (CSoundMng::GetInstance()->Open(static_cast<CSoundMng::DeviceType>(np2oscfg.cSoundDeviceType), np2oscfg.szSoundDeviceName, hWnd))
	{
		CSoundMng::GetInstance()->LoadPCM(SOUND_PCMSEEK, TEXT("SEEKWAV"));
		CSoundMng::GetInstance()->LoadPCM(SOUND_PCMSEEK1, TEXT("SEEK1WAV"));
		CSoundMng::GetInstance()->LoadPCM(SOUND_RELAY1, TEXT("RELAY1WAV"));
		CSoundMng::GetInstance()->SetPCMVolume(SOUND_PCMSEEK, np2cfg.MOTORVOL);
		CSoundMng::GetInstance()->SetPCMVolume(SOUND_PCMSEEK1, np2cfg.MOTORVOL);
		CSoundMng::GetInstance()->SetPCMVolume(SOUND_RELAY1, np2cfg.MOTORVOL);
	}

	if (np2oscfg.MOUSE_SW) {										// ver0.30
		mousemng_enable(MOUSEPROC_SYSTEM);
	}

	commng_initialize();
	sysmng_initialize();

	joymng_initialize();
	pccore_init();
	S98_init();

#ifdef OPENING_WAIT
	while((GetTickCount() - tick) < OPENING_WAIT);
#endif

#ifdef SUPPORT_NET
	//np2net_init();
#endif
#ifdef SUPPORT_WAB
	//np2wab_init(g_hInstance, g_hWndMain);
#endif
#ifdef SUPPORT_CL_GD5430
	//pc98_cirrus_vga_init();
#endif
	
#ifdef SUPPORT_PHYSICAL_CDDRV
	np2updateCDmenu();
#endif
	
	SetTickCounterMode(np2oscfg.tickmode);
	pccore_reset();
	np2_SetUserPause(0);
	
	// スナップ位置の復元のため先に作成
	if (!(g_scrnmode & SCRNMODE_FULLSCREEN)) {
		if (np2oscfg.toolwin) {
			toolwin_create();
		}
		if (np2oscfg.keydisp) {
			kdispwin_create();
		}
		if (np2oscfg.skbdwin) {
			skbdwin_create();
		}
	}
	
	// れじうむ
#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume) {
		int		id;

		id = flagload(hWnd, str_sav, _T("Resume"), FALSE);
		if (id == IDYES)
		{
			Np2Arg::GetInstance()->ClearDisk();
		}
		else if (id == IDCANCEL) {
			DestroyWindow(hWnd);
			mousemng_disable(MOUSEPROC_WINUI);
			S98_trash();
			pccore_term();
			CSoundMng::GetInstance()->Close();
			CSoundMng::Deinitialize();
			scrnmng_destroy();
			UnloadExternalResource();
			TRACETERM();
			dosio_term();
			SendMessage(hWnd, WM_CLOSE, 0, 0L);
			return;
		}
	}
#endif
	soundmng_setvolume(np2cfg.vol_master);
	
	// 画面表示倍率を復元
	if(np2oscfg.svscrmul){
		scrnmng_setmultiple(np2oscfg.scrn_mul);
	}
//	リセットしてから… 
	// INIに記録されたディスクを挿入
	if(np2cfg.savefddfile){
		for (i = 0; i < 4; i++)
		{
			LPCTSTR lpDisk = np2cfg.fddfile[i];
			if (lpDisk)
			{
				diskdrv_readyfdd((REG8)i, lpDisk, 0);
				toolwin_setfdd((REG8)i, lpDisk);
			}
		}
	}
//#if defined(SUPPORT_IDEIO)
//	// INIに記録されたCDを挿入
//	if (np2cfg.savecdfile) {
//		for (i = 0; i < 4; i++)
//		{
//			if (np2cfg.idetype[i] == IDETYPE_CDROM) {
//				LPCTSTR lpDisk = np2cfg.idecd[i];
//				if (lpDisk)
//				{
//					diskdrv_setsxsi(i, lpDisk);
//				}
//			}
//		}
//	}
//#endif

	scrndraw_redraw();
	
	np2opening = 0;

	sysmng_workclockreset();
	sysmng_updatecaption(SYS_UPDATECAPTION_ALL);
	
#ifdef HOOK_SYSKEY
	start_hook_systemkey();
#endif

#if defined(SUPPORT_MULTITHREAD)
	np2_multithread_requestswitch = 1;
#endif

	np2_multithread_Resume();
}

#if defined(SUPPORT_MULTITHREAD)
static unsigned int __stdcall np2_multithread_EmulatorThreadMain(LPVOID vdParam){
	while (!np2_multithread_hThread_requestexit) {
		if (!np2stopemulate && !np2_multithread_pauseemulation && !np2userpause) {
			UINT32 drawskip = (np2oscfg.DRAW_SKIP == 0 ? 1 : np2oscfg.DRAW_SKIP);
#if defined(SUPPORT_ASYNC_CPU)
			pccore_asynccpustat.drawskip = drawskip;
			pccore_asynccpustat.nowait = np2oscfg.NOWAIT;
#endif
			np2_multithread_pausing = false;
			if (np2oscfg.NOWAIT) {
				ExecuteOneFrame_MT_EmulateThread(framecnt == 0);
				if (drawskip) {		// nowait frame skip
					framecnt++;
					if (framecnt >= drawskip) {
						processwait(0);
						soundmng_sync();
					}
				}
				else {							// nowait auto skip
					framecnt = 1;
					if (timing_getcount()) {
						processwait(0);
						soundmng_sync();
					}
				}
			}
			else if (drawskip) {		// frame skip
				if (framecnt < drawskip) {
					ExecuteOneFrame_MT_EmulateThread(framecnt == 0);
					if (framecnt == 0) processasyncwait();
					framecnt++;
				}
				else {
					processwait(drawskip);
					soundmng_sync();
				}
			}
			else {								// auto skip
				if (!waitcnt) {
					UINT cnt;
					ExecuteOneFrame_MT_EmulateThread(framecnt == 0);
					framecnt++;
					cnt = timing_getcount();
					if (framecnt > cnt) {
						waitcnt = framecnt;
						if (framemax > 1) {
							framemax--;
						}
					}
					else if (framecnt >= framemax) {
						if (framemax < 12) {
							framemax++;
						}
						if (cnt >= 12) {
							timing_reset();
						}
						else {
							timing_setcount(cnt - framecnt);
						}
						framereset_MT_EmulateThread(0);
						if (framecnt == 0) processasyncwait();
					}
				}
				else {
					processwait(waitcnt);
					soundmng_sync();
					waitcnt = framecnt;
				}
			}
			if(autokey_sendbufferlen > 0) 
				autoSendKey(); // 自動キー送信
		}
		else if (np2_multithread_pauseemulation == 1) {
			np2_multithread_pausing = true;
			Sleep(100);
		}
		else if (np2stopemulate == 1 || np2userpause) { // background sleep
			np2_multithread_pausing = false;
			Sleep(100);
		}else{
			np2_multithread_pausing = false;
		}
	}
	return 0;
}
void np2_multithread_StartThread(){
	if(np2_multithread_initialized){
		unsigned int dwID;
		np2_multithread_hThread_requestexit = FALSE;
		np2_multithread_hThread = (HANDLE)_beginthreadex(NULL, 0, np2_multithread_EmulatorThreadMain, NULL, 0, &dwID);
		SetThreadPriority(np2_multithread_hThread, THREAD_PRIORITY_ABOVE_NORMAL);
	}
}
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst,
										LPSTR lpszCmdLine, int nCmdShow) {
	WNDCLASS	wc;
	MSG			msg;
	HWND		hWnd;
	UINT		i;
	DWORD		style;
#ifdef OPENING_WAIT
	UINT32		tick;
#endif
	BOOL		xrollkey;
	int			winx, winy;
	int			terminateFlag = 0;
	
#ifdef _DEBUG
	// 使うときはstdlib.hとcrtdbg.hをインクルードする
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(499);
#endif

#if defined(SUPPORT_WIN2000HOST)
#ifdef _WINDOWS
#ifndef _WIN64
#define WINVER2 0x0500
	initialize_findacx();
#endif
#endif
#endif
	
	winloc_InitDwmFunc();

	WM_QueryCancelAutoPlay = RegisterWindowMessage(_T("QueryCancelAutoPlay"));

	createAsciiTo98KeyCodeList();
	
	_MEM_INIT();
	CWndProc::Initialize(hInstance);
	CSubWndBase::Initialize(hInstance);
#if defined(SUPPORT_VSTi)
	CVstEditWnd::Initialize(hInstance);
#endif	// defined(SUPPORT_VSTi)

	sysmng_findfile_Initialize();

	GetModuleFileName(NULL, modulefile, NELEMENTS(modulefile));
	dosio_init();
	file_setcd(modulefile);
	Np2Arg::GetInstance()->Parse();
	initload();
	toolwin_readini();
	kdispwin_readini();
	skbdwin_readini();
	mdbgwin_readini();
#if defined(SUPPORT_WAB)
	wabwin_readini();
	np2wabcfg.readonly = np2oscfg.readonly;
#endif	// defined(SUPPORT_WAB)
#if defined(SUPPORT_HOSTDRV)
	hostdrv_readini();
#endif	// defined(SUPPORT_HOSTDRV)

#if defined(SUPPORT_MULTITHREAD)
	np2_multithread_Initialize();
#endif

	rand_setseed((unsigned)time(NULL));

	szClassName[0] = (TCHAR)np2oscfg.winid[0];
	szClassName[1] = (TCHAR)np2oscfg.winid[1];
	szClassName[2] = (TCHAR)np2oscfg.winid[2];
	
#ifndef ALLOW_MULTIRUN
	if ((hWnd = FindWindow(szClassName, NULL)) != NULL) {
		ShowWindow(hWnd, SW_RESTORE);
		SetForegroundWindow(hWnd);
		dosio_term();
		return(FALSE);
	}
#else
	if ((hWnd = FindWindow(szClassName, NULL)) != NULL && np2oscfg.resume) {
		// レジュームの時は複数起動するとやばいので･･･
		ShowWindow(hWnd, SW_RESTORE);
		SetForegroundWindow(hWnd);
		dosio_term();
		return(FALSE);
	}
#endif

	g_hInstance = hInstance = LoadExternalResource(hInstance);
	CWndProc::SetResourceHandle(hInstance);

#if !defined(_WIN64)
	mmxflag = (havemmx())?0:MMXFLAG_NOTSUPPORT;
	mmxflag += (np2oscfg.disablemmx)?MMXFLAG_DISABLE:0;
#endif
	TRACEINIT();

	xrollkey = (np2oscfg.xrollkey == 0);
	if (np2oscfg.KEYBOARD >= KEY_TYPEMAX) {
		int keytype = GetKeyboardType(1);
		if ((keytype & 0xff00) == 0x0d00) {
			np2oscfg.KEYBOARD = KEY_PC98;
			xrollkey = !xrollkey;
		}
		else if (!keytype) {
			np2oscfg.KEYBOARD = KEY_KEY101;
		}
		else {
			np2oscfg.KEYBOARD = KEY_KEY106;
		}
	}
	winkbd_roll(xrollkey);
	winkbd_setf12(np2oscfg.F12COPY);
	keystat_initialize();

	np2class_initialize(hInstance);
	if (!hPrevInst) {
		if(np2oscfg.MOUSE_SW || np2oscfg.mouse_nc){
			wc.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW;
		}else{
			wc.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		}
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = NP2GWLP_SIZE;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN);
		wc.lpszClassName = szClassName;
		if (!RegisterClass(&wc)) {
			UnloadExternalResource();
			TRACETERM();
			dosio_term();
			return(FALSE);
		}

		kdispwin_initialize();
		skbdwin_initialize();
		mdbgwin_initialize();
		CDebugUtyView::Initialize(hInstance);
	}

	style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
	if (np2oscfg.thickframe) {
		style |= WS_THICKFRAME;
	}
	
	winx = np2oscfg.winx;
	winy = np2oscfg.winy;
	hWnd = CreateWindowEx(0, szClassName, np2oscfg.titles, style,
						winx, winy, 640, 400,
						NULL, NULL, hInstance, NULL);
	winloc_DisableCornerRound(g_hWndMain);
	g_hWndMain = hWnd;

	//{
	//	HMODULE modUsedr32 = LoadLibraryA("User32.dll");
	//	if(modUsedr32){
	//		BOOL (WINAPI *pfnRegisterTouchWindow)(__in HWND hwnd, __in ULONG ulFlags);
	//		pfnRegisterTouchWindow = (BOOL (WINAPI *)(__in HWND hwnd, __in ULONG ulFlags))GetProcAddress(modUsedr32, "RegisterTouchWindow");
	//		if(pfnRegisterTouchWindow){
	//			(*pfnRegisterTouchWindow)(g_hWndMain, 0x00000001);
	//		}
	//		FreeLibrary(modUsedr32);
	//	}
	//}
	
	mousemng_initialize(); // 場所移動 np21w ver0.96 rev13

	scrnmng_initialize();

	if(np2oscfg.dragdrop)
		DragAcceptFiles(hWnd, TRUE);	//	イメージファイルのＤ＆Ｄに対応(Kai1)
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	
	SetWindowPos(hWnd, NULL, winx, winy, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED); // Win10環境でウィンドウ位置がずれる問題の対策
	
#ifdef OPENING_WAIT
	tick = GetTickCount();
#endif

	sysmenu_initialize(GetSystemMenu(hWnd, FALSE));

	HMENU hMenu = np2class_gethmenu(hWnd);
	xmenu_initialize(hMenu);
	xmenu_iniupdate(hMenu);
	xmenu_update(hMenu);
	if (file_attr_c(np2help) == -1)								// ver0.30
	{
		EnableMenuItem(hMenu, IDM_HELP, MF_GRAYED);
	}
	DrawMenuBar(hWnd);

	if(np2oscfg.savescrn){
		g_scrnmode = np2oscfg.scrnmode;
	}else{
		g_scrnmode = np2oscfg.scrnmode = 0;
	}
	if (Np2Arg::GetInstance()->fullscreen())
	{
		g_scrnmode |= SCRNMODE_FULLSCREEN;
	}
	if (np2cfg.RASTER) {
		g_scrnmode |= SCRNMODE_HIGHCOLOR;
	}
	if (scrnmng_create(g_scrnmode) != SUCCESS) {
		g_scrnmode ^= SCRNMODE_FULLSCREEN;
		if (scrnmng_create(g_scrnmode) != SUCCESS) {
			messagebox(hWnd, MAKEINTRESOURCE(IDS_ERROR_DIRECTDRAW), MB_OK | MB_ICONSTOP);
			UnloadExternalResource();
			TRACETERM();
			dosio_term();
			return(FALSE);
		}
	}
	/*
	// XXX: Direct3D絡みのエラー対策
	{
		MSG msg;
		while(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0)) {
				break;
			}
			if ((msg.hwnd != hWnd) ||
				((msg.message != WM_SYSKEYDOWN) &&
				(msg.message != WM_SYSKEYUP))) {
				TranslateMessage(&msg);
			}
			DispatchMessage(&msg);
		}
	}*/

	CSoundMng::Initialize();
	OpenSoundDevice(hWnd);

	if (CSoundMng::GetInstance()->Open(static_cast<CSoundMng::DeviceType>(np2oscfg.cSoundDeviceType), np2oscfg.szSoundDeviceName, hWnd))
	{
		CSoundMng::GetInstance()->LoadPCM(SOUND_PCMSEEK, TEXT("SEEKWAV"));
		CSoundMng::GetInstance()->LoadPCM(SOUND_PCMSEEK1, TEXT("SEEK1WAV"));
		CSoundMng::GetInstance()->LoadPCM(SOUND_RELAY1, TEXT("RELAY1WAV"));
		CSoundMng::GetInstance()->SetPCMVolume(SOUND_PCMSEEK, np2cfg.MOTORVOL);
		CSoundMng::GetInstance()->SetPCMVolume(SOUND_PCMSEEK1, np2cfg.MOTORVOL);
		CSoundMng::GetInstance()->SetPCMVolume(SOUND_RELAY1, np2cfg.MOTORVOL);
	}

	if (np2oscfg.MOUSE_SW) {										// ver0.30
		if(GetForegroundWindow() == hWnd){
			mousemng_enable(MOUSEPROC_SYSTEM);
		}else{
			np2oscfg.MOUSE_SW = 0;
		}
	}

	commng_initialize();
	sysmng_initialize();

	joymng_initialize();
	pccore_init();
	S98_init();

#ifdef SUPPORT_NET
	np2net_init();
#endif
#ifdef SUPPORT_WAB
	np2wab_init(g_hInstance, g_hWndMain);
#endif
#ifdef SUPPORT_CL_GD5430
	pc98_cirrus_vga_init();
#endif
	
#ifdef SUPPORT_PHYSICAL_CDDRV
	np2updateCDmenu();
#endif

	SetTickCounterMode(np2oscfg.tickmode);
	pccore_reset();
	np2_SetUserPause(0);
	
	// スナップ位置の復元のため先に作成
	if (!(g_scrnmode & SCRNMODE_FULLSCREEN)) {
		if (np2oscfg.toolwin) {
			toolwin_create();
		}
		if (np2oscfg.keydisp) {
			kdispwin_create();
		}
		if (np2oscfg.skbdwin) {
			skbdwin_create();
		}
	}

	// れじうむ
#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume) {
		int		id;

		id = flagload(hWnd, str_sav, _T("Resume"), FALSE);
		if (id == IDYES)
		{
			Np2Arg::GetInstance()->ClearDisk();
		}
		else if (id == IDCANCEL) {
			DestroyWindow(hWnd);
			mousemng_disable(MOUSEPROC_WINUI);
			S98_trash();
			pccore_term();
			CSoundMng::GetInstance()->Close();
			CSoundMng::Deinitialize();
			scrnmng_destroy();
			scrnmng_shutdown();
			UnloadExternalResource();
			TRACETERM();
			dosio_term();
			return(FALSE);
		}
	}
#endif
	soundmng_setvolume(np2cfg.vol_master);
	
	// 画面表示倍率を復元
	if(np2oscfg.svscrmul){
		scrnmng_setmultiple(np2oscfg.scrn_mul);
	}
//	リセットしてから… 
	// INIに記録されたディスクを挿入
	if(np2cfg.savefddfile){
		for (i = 0; i < 4; i++)
		{
			LPCTSTR lpDisk = np2cfg.fddfile[i];
			if (lpDisk)
			{
				diskdrv_readyfdd((REG8)i, lpDisk, 0);
				toolwin_setfdd((REG8)i, lpDisk);
			}
		}
	}
	// コマンドラインのディスク挿入。
	for (i = 0; i < 4; i++)
	{
		LPCTSTR lpDisk = Np2Arg::GetInstance()->disk(i);
		if (lpDisk)
		{
			diskdrv_readyfdd((REG8)i, lpDisk, 0);
			toolwin_setfdd((REG8)i, lpDisk);
		}
	}
	// コマンドラインのディスク挿入。
	for (i = 0; i < 4; i++)
	{
		LPCTSTR lpDisk = Np2Arg::GetInstance()->disk(i);
		if (lpDisk)
		{
			diskdrv_readyfdd((REG8)i, lpDisk, 0);
		}
	}
#ifdef SUPPORT_IDEIO
	if (Np2Arg::GetInstance()->cdisk(0))
	{
		int cdiskidx = 0;
		for (i = 0; i < 4; i++)
		{
			if (np2cfg.idetype[i] == IDETYPE_CDROM)
			{
				for (; cdiskidx < 4; cdiskidx++)
				{
					LPCTSTR lpDisk = Np2Arg::GetInstance()->cdisk(cdiskidx);
					if (lpDisk)
					{
						diskdrv_setsxsi(i, NULL);
						diskdrv_setsxsi(i, lpDisk);
						break;
					}
				}
			}
		}
	}
#endif

#ifdef OPENING_WAIT
	while((GetTickCount() - tick) < OPENING_WAIT);
#endif
	np2opening = 0;

	scrndraw_redraw();
	
	sysmng_workclockreset();
	sysmng_updatecaption(SYS_UPDATECAPTION_ALL);
	
#ifdef HOOK_SYSKEY
	start_hook_systemkey();
#endif

	timeBeginPeriod(1);
	
#if defined(SUPPORT_MULTITHREAD)
	do
	{
		np2_multithread_requestswitch = 0;
		np2_multithread_enable = np2oscfg.multithread;
		if (np2_multithread_enable)
		{
			UINT_PTR tmrID;
			const int tmrInterval = 50;
			// マルチスレッドモード
			framereset = framereset_MT_EmulateThread;
			np2_multithread_StartThread();
			tmrID = SetTimer(hWnd, 23545, tmrInterval, NULL);
			while (1)
			{
				if (!GetMessage(&msg, NULL, 0, 0))
				{
					terminateFlag = 1;
					break;
				}
				if ((msg.hwnd != hWnd) ||
					((msg.message != WM_SYSKEYDOWN) &&
						(msg.message != WM_SYSKEYUP)))
				{
					if (msg.message == WM_TIMER && msg.wParam == tmrID)
					{
						framereset_MT_UIThread(1);
#if defined(SUPPORT_DCLOCK)
						DispClock::GetInstance()->Update();
#endif
#if defined(SUPPORT_VSTi)
						CVstEditWnd::OnIdle();
#endif	// defined(SUPPORT_VSTi)
					}
					TranslateMessage(&msg);
				}
				DispatchMessage(&msg);
				joymng_sync();
				mousemng_sync();
				scrnmng_delaychangemode();
				mousemng_UIThreadSync();
				scrnmng_UIThreadProc();
				if (np2_multithread_requestswitch) break;
			}
			KillTimer(hWnd, tmrID);
		}
		else
#endif
		{
			framereset = framereset_ALL;
			lateframecount = 0;
			while (1)
			{
				if (!np2stopemulate && !np2userpause)
				{
					if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
					{
						if (!GetMessage(&msg, NULL, 0, 0))
						{
							terminateFlag = 1;
							break;
						}
						if ((msg.hwnd != hWnd) ||
							((msg.message != WM_SYSKEYDOWN) &&
								(msg.message != WM_SYSKEYUP)))
						{
							TranslateMessage(&msg);
						}
						DispatchMessage(&msg);
					}
					else
					{
						UINT32 drawskip = (np2oscfg.DRAW_SKIP == 0 ? 1 : np2oscfg.DRAW_SKIP);
#if defined(SUPPORT_ASYNC_CPU)
						pccore_asynccpustat.drawskip = drawskip;
						pccore_asynccpustat.nowait = np2oscfg.NOWAIT;
#endif
						if (np2oscfg.NOWAIT)
						{
							ExecuteOneFrame(framecnt == 0);
							if (drawskip)
							{		// nowait frame skip
								framecnt++;
								if (framecnt >= drawskip)
								{
									processwait(0);
									soundmng_sync();
								}
							}
							else
							{							// nowait auto skip
								framecnt = 1;
								if (timing_getcount())
								{
									processwait(0);
									soundmng_sync();
								}
							}
						}
						else if (drawskip)
						{		// frame skip
							if (framecnt < drawskip)
							{
								ExecuteOneFrame(framecnt == 0);
								framecnt++;
							}
							else
							{
								processwait(drawskip);
								soundmng_sync();
							}
						}
						else
						{								// auto skip
							if (!waitcnt)
							{
								UINT cnt;
								ExecuteOneFrame(framecnt == 0);
								framecnt++;
								cnt = timing_getcount();
								if (framecnt > cnt)
								{
									waitcnt = framecnt;
									if (framemax > 1)
									{
										framemax--;
									}
								}
								else if (framecnt >= framemax)
								{
									if (framemax < 12)
									{
										framemax++;
									}
									if (cnt >= 12)
									{
										timing_reset();
									}
									else
									{
										timing_setcount(cnt - framecnt);
									}
									framereset(0);
								}
							}
							else
							{
								processwait(waitcnt);
								soundmng_sync();
								waitcnt = framecnt;
							}
						}
						if (autokey_sendbufferlen > 0)
							autoSendKey(); // 自動キー送信
					}
					scrnmng_delaychangemode();
					mousemng_UIThreadSync();
					scrnmng_UIThreadProc();
				}
				else if ((np2stopemulate == 1 || np2userpause) ||				// background sleep
					(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)))
				{
					if (np2stopemulate == 1)
					{
						lateframecount = 0;
						timing_setcount(0);
					}
					if (!GetMessage(&msg, NULL, 0, 0))
					{
						terminateFlag = 1;
						break;
					}
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
#if defined(SUPPORT_MULTITHREAD)
				if (np2_multithread_requestswitch) break;
#endif
			}
		}
#if defined(SUPPORT_MULTITHREAD)
		np2_multithread_WaitForExitThread();
	} while (np2_multithread_requestswitch && !terminateFlag);
#endif

	timeEndPeriod(1);
	
#ifdef HOOK_SYSKEY
	stop_hook_systemkey();
#endif

	// 画面表示倍率を保存
	np2oscfg.scrn_mul = scrnmng_getmultiple();

	toolwin_destroy();
	kdispwin_destroy();
	skbdwin_destroy();
	mdbgwin_destroy();

	pccore_cfgupdate();

	mousemng_disable(MOUSEPROC_WINUI);
	S98_trash();

#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume) {
		flagsave(str_sav);
	}
	else {
		flagdelete(str_sav);
	}
#endif

#ifdef SUPPORT_CL_GD5430
	pc98_cirrus_vga_shutdown();
#endif

	pccore_term();

#ifdef SUPPORT_WAB
	np2wab_shutdown();
#endif
#ifdef SUPPORT_NET
	np2net_shutdown();
#endif

	CSoundMng::GetInstance()->Close();
	CSoundMng::Deinitialize();
	scrnmng_destroy();
	scrnmng_shutdown();
	commng_finalize();
	recvideo_close();
	mousemng_destroy();
	
	if (sys_updates	& (SYS_UPDATECFG | SYS_UPDATEOSCFG)) {
		initsave();
		toolwin_writeini();
		kdispwin_writeini();
		skbdwin_writeini();
		mdbgwin_writeini();
	}
#if defined(SUPPORT_HOSTDRV)
	hostdrv_writeini();
#endif	// defined(SUPPORT_HOSTDRV)
#if defined(SUPPORT_WAB)
	wabwin_writeini();
#endif	// defined(SUPPORT_WAB)
	skbdwin_deinitialize();
	
	CSubWndBase::Deinitialize();
	CWndProc::Deinitialize();
	
	winloc_DisposeDwmFunc();

	UnloadExternalResource();

	TRACETERM();
	_MEM_USED(TEXT("report.txt"));
	dosio_term();

	Np2Arg::Release();
	
#if defined(SUPPORT_MULTITHREAD)
	np2_multithread_Finalize();
#endif

	sysmng_findfile_Finalize();

	//_CrtDumpMemoryLeaks();

	return((int)msg.wParam);
}
