#define DIRECTINPUT_VERSION 0x0800

#include	<compiler.h>
#include	<np2.h>
#include	<mousemng.h>
#include    <scrnmng.h>
#include	<np2mt.h>

#include	<dinput.h>
//#pragma comment(lib, "dinput8.lib")

#define DIDFT_OPTIONAL	0x80000000

#ifdef SUPPORT_WACOM_TABLET
bool cmwacom_skipMouseEvent(void);
void cmwacom_setExclusiveMode(bool enable);
#endif

#define	MOUSEMNG_RANGE		128


typedef struct {
	SINT16	x;
	SINT16	y;
	UINT8	btn;
	UINT	flag;
} MOUSEMNG;

static	MOUSEMNG	mousemng;
static  int mousecaptureflg = 0;

static  int mouseMul = 1; // マウススピード倍率（分子）
static  int mouseDiv = 1; // マウススピード倍率（分母）

static  int mousebufX = 0; // マウス移動バッファ(X)
static  int mousebufY = 0; // マウス移動バッファ(Y)

// RAWマウス入力対応 np21w ver0.86 rev13
static  LPDIRECTINPUT8 dinput = NULL; 
static  LPDIRECTINPUTDEVICE8 diRawMouse = NULL; 
static  int mouseRawDeltaX = 0;
static  int mouseRawDeltaY = 0;

typedef HRESULT (WINAPI *FN_DIRECTINPUT8CREATE)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);

static  HMODULE hModuleDI8 = NULL;
static  FN_DIRECTINPUT8CREATE fndi8create = NULL;

static DWORD mousemng_UIthreadID = 0;
static bool mousemng_requestCreateInput = false;
static bool mousemng_requestAcquire = false;
static CRITICAL_SECTION mousemng_multithread_deviceinit_cs = { 0 };

BRESULT mousemng_checkdinput8(){
	if (mousemng_UIthreadID != GetCurrentThreadId()) return FAILURE; // 別のスレッドからのアクセスは不可

	// DirectInput8が使用できるかチェック
	LPDIRECTINPUT8 test_dinput = NULL; 
	LPDIRECTINPUTDEVICE8 test_didevice = NULL;

	if(fndi8create) return(SUCCESS);
	
	hModuleDI8 = LoadLibrary(_T("dinput8.dll"));
	if(!hModuleDI8){
		goto scre_err;
	}
	
	fndi8create = (FN_DIRECTINPUT8CREATE)GetProcAddress(hModuleDI8, "DirectInput8Create");
	if(!fndi8create){
		goto scre_err2;
	}
	if(FAILED(fndi8create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&test_dinput, NULL))){
		goto scre_err2;
	}
	if(FAILED(test_dinput->CreateDevice(GUID_SysMouse, &test_didevice, NULL))){
		goto scre_err3;
	}

	// デバイス作成まで出来そうならOKとする
	test_didevice->Release();
	test_dinput->Release();

	return(SUCCESS);
scre_err3:
	test_dinput->Release();
scre_err2:
	FreeLibrary(hModuleDI8);
scre_err:
	hModuleDI8 = NULL;
	fndi8create = NULL;
	return(FAILURE);
}

UINT8 mousemng_getstat(SINT16 *x, SINT16 *y, int clear) {
#ifdef SUPPORT_WACOM_TABLET
	if(cmwacom_skipMouseEvent()){
		mousemng.x = 0;
		mousemng.y = 0;
	}
#endif
	*x = mousemng.x;
	*y = mousemng.y;
	if (clear) {
		mousemng.x = 0;
		mousemng.y = 0;
	}
	return(mousemng.btn);
}
void mousemng_setstat(SINT16 x, SINT16 y, UINT8 btn) {
	if (mousemng.flag){
		mousemng.x = x;
		mousemng.y = y;
		mousemng.btn = btn;
	}
}

UINT8 mousemng_supportrawinput() {
	return(dinput && diRawMouse);
}

void mousemng_updatespeed() {
	np2_multithread_EnterCriticalSection();
	mouseMul = np2oscfg.mousemul != 0 ? np2oscfg.mousemul : 1;
	mouseDiv = np2oscfg.mousediv != 0 ? np2oscfg.mousediv : 1;
	np2_multithread_LeaveCriticalSection();
}

// ----

static void getmaincenter(POINT *cp) {

	RECT	rct;

	GetWindowRect(g_hWndMain, &rct);
	cp->x = (rct.right + rct.left) / 2;
	cp->y = (rct.bottom + rct.top) / 2;
}

static void initDirectInput(){
	if (mousemng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可
	
	HRESULT		hr;
	DIOBJECTDATAFORMAT obj[7];
	DIDATAFORMAT dimouse_format;
    dimouse_format.dwSize       = sizeof(DIDATAFORMAT);
    dimouse_format.dwObjSize    = sizeof(DIOBJECTDATAFORMAT);
    dimouse_format.dwFlags      = DIDF_RELAXIS;
    dimouse_format.dwDataSize   = 16;
    dimouse_format.dwNumObjs    = 7;
    dimouse_format.rgodf        = obj;
    obj[0].dwOfs        = 0;
    obj[0].pguid        = &GUID_XAxis;
    obj[0].dwType       = DIDFT_ANYINSTANCE | DIDFT_AXIS;
    obj[0].dwFlags      = 0;
    obj[1].dwOfs        = 4;
    obj[1].pguid        = &GUID_YAxis;
    obj[1].dwType       = DIDFT_ANYINSTANCE | DIDFT_AXIS;
    obj[1].dwFlags      = 0;
    obj[2].dwOfs        = 8;
    obj[2].pguid        = &GUID_ZAxis;
    obj[2].dwType       = DIDFT_ANYINSTANCE | DIDFT_OPTIONAL | DIDFT_AXIS;
    obj[2].dwFlags      = 0;
    obj[3].dwOfs        = 12;
    obj[3].pguid        = NULL;
    obj[3].dwType       = DIDFT_ANYINSTANCE | DIDFT_BUTTON;
    obj[3].dwFlags      = 0;
    obj[4].dwOfs        = 13;
    obj[4].pguid        = NULL;
    obj[4].dwType       = DIDFT_ANYINSTANCE | DIDFT_BUTTON;
    obj[4].dwFlags      = 0;
    obj[5].dwOfs        = 14;
    obj[5].pguid        = NULL;
    obj[5].dwType       = DIDFT_ANYINSTANCE | DIDFT_OPTIONAL | DIDFT_BUTTON;
    obj[5].dwFlags      = 0;
    obj[6].dwOfs        = 15;
    obj[6].pguid        = NULL;
    obj[6].dwType       = DIDFT_ANYINSTANCE | DIDFT_OPTIONAL | DIDFT_BUTTON;
    obj[6].dwFlags      = 0;

	EnterCriticalSection(&mousemng_multithread_deviceinit_cs);
	if(fndi8create && !dinput){
		//hr = DirectInputCreateEx(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput7, (void**)&dinput, NULL);
		//hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&dinput, NULL); // 関数名変えてやがった( ﾟдﾟ)
		hr = fndi8create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&dinput, NULL);
		if (!FAILED(hr)){
			hr = dinput->CreateDevice(GUID_SysMouse, &diRawMouse, NULL);
			if (!FAILED(hr)){
				// データフォーマット設定
				hr = diRawMouse->SetDataFormat(&dimouse_format);
				if (!FAILED(hr)){
					// 協調レベル設定
					hr = diRawMouse->SetCooperativeLevel(g_hWndMain, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
				}
				if (!FAILED(hr)){
					// デバイス設定
					DIPROPDWORD		diprop;
					diprop.diph.dwSize = sizeof(diprop);
					diprop.diph.dwHeaderSize = sizeof(diprop.diph);
					diprop.diph.dwObj = 0;
					diprop.diph.dwHow = DIPH_DEVICE;
					diprop.dwData = DIPROPAXISMODE_REL;	// 相対値モード
					hr = diRawMouse->SetProperty(DIPROP_AXISMODE, &diprop.diph);
				}
				if (!FAILED(hr)) {
					// 入力開始
					hr = diRawMouse->Acquire();
				}else{
					// 失敗･･･
					diRawMouse->Release();
					diRawMouse = NULL;
					dinput->Release();
					dinput = NULL;
				}
			}else{
				// 失敗･･･
				diRawMouse = NULL;
				dinput->Release();
				dinput = NULL;
			}
		}else{
			// 失敗･･･
			dinput = NULL;
		}
	}else{
		if (diRawMouse)
		{
			hr = diRawMouse->Acquire();
		}
	}
	LeaveCriticalSection(&mousemng_multithread_deviceinit_cs);
}
static void destroyDirectInput(){
	if (mousemng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

	EnterCriticalSection(&mousemng_multithread_deviceinit_cs);
	if(diRawMouse){
		diRawMouse->Release();
		diRawMouse = NULL;
	}
	if(dinput){
		dinput->Release();
		dinput = NULL;
	}
	if(hModuleDI8){
		FreeLibrary(hModuleDI8);
		hModuleDI8 = NULL;
		fndi8create = NULL;
	}
	LeaveCriticalSection(&mousemng_multithread_deviceinit_cs);
}

static void mousecapture(BOOL capture) {
	if (mousemng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

	LONG	style;
	POINT	cp;
	RECT	rct;

#ifdef SUPPORT_WACOM_TABLET
	cmwacom_setExclusiveMode(capture ? true : false);
#endif

	if(np2oscfg.rawmouse){
		if(mousemng_checkdinput8()!=SUCCESS){
			np2_multithread_LeaveCriticalSection();
			np2oscfg.rawmouse = 0;
			MessageBox(g_hWndMain, _T("Failed to initialize DirectInput8."), _T("DirectInput Error"), MB_OK|MB_ICONEXCLAMATION);
			return;
		}
	}

	mousemng_updatespeed();

	style = GetClassLong(g_hWndMain, GCL_STYLE);
	if (capture) {
		ShowCursor(FALSE);
		getmaincenter(&cp);
		rct.left = cp.x - MOUSEMNG_RANGE;
		rct.right = cp.x + MOUSEMNG_RANGE;
		rct.top = cp.y - MOUSEMNG_RANGE;
		rct.bottom = cp.y + MOUSEMNG_RANGE;
		SetCursorPos(cp.x, cp.y);
		ClipCursor(&rct);
		style &= ~(CS_DBLCLKS);
		mousecaptureflg = 1;
		if(np2oscfg.rawmouse && fndi8create){
			initDirectInput();
		}
	}
	else {
		ShowCursor(TRUE);
		ClipCursor(NULL);
		style |= CS_DBLCLKS;
		mousecaptureflg = 0;
		EnterCriticalSection(&mousemng_multithread_deviceinit_cs);
		if(np2oscfg.rawmouse && fndi8create && diRawMouse){
			diRawMouse->Unacquire();
			//destroyDirectInput();
		}
		LeaveCriticalSection(&mousemng_multithread_deviceinit_cs);
	}
	SetClassLong(g_hWndMain, GCL_STYLE, style);
}

void mousemng_initialize(void) {

	InitializeCriticalSection(&mousemng_multithread_deviceinit_cs);

	mousemng_UIthreadID = GetCurrentThreadId();

	np2_multithread_EnterCriticalSection();
	ZeroMemory(&mousemng, sizeof(mousemng));
	mousemng.btn = uPD8255A_LEFTBIT | uPD8255A_RIGHTBIT;
	mousemng.flag = (1 << MOUSEPROC_SYSTEM);
	np2_multithread_LeaveCriticalSection();
	
	mousemng_updatespeed();
}

void mousemng_destroy(void) {
	if (mousemng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

	destroyDirectInput();

	DeleteCriticalSection(&mousemng_multithread_deviceinit_cs);
}

void mousemng_UIThreadSync()
{
	if (mousemng_requestCreateInput)
	{
		mousemng_requestCreateInput = false;
		destroyDirectInput();
		initDirectInput();
	}
	if (mousemng_requestAcquire)
	{
		mousemng_requestAcquire = false;
		EnterCriticalSection(&mousemng_multithread_deviceinit_cs);
		if (diRawMouse)
		{
			diRawMouse->Acquire();
		}
		LeaveCriticalSection(&mousemng_multithread_deviceinit_cs);
	}
}

void mousemng_sync(void) {
	POINT	p;
	POINT	cp;

	if ((!mousemng.flag) && (GetCursorPos(&p))) {
		getmaincenter(&cp);
		if (np2oscfg.rawmouse && fndi8create && dinput == NULL)
		{
			mousemng_requestCreateInput = true;
			PostMessage(g_hWndMain, WM_NULL, 0, 0);
		}
		EnterCriticalSection(&mousemng_multithread_deviceinit_cs);
		if(np2oscfg.rawmouse && fndi8create && mousemng_supportrawinput()){
			DIMOUSESTATE diMouseState = {0};
			HRESULT hr;
			hr = diRawMouse->GetDeviceState(sizeof(DIMOUSESTATE), &diMouseState);
			if (hr != DI_OK){
				switch(hr){
				case E_PENDING:
					break;
				case DIERR_INPUTLOST:
				case DIERR_NOTACQUIRED:
					mousemng_requestAcquire = true;
					PostMessage(g_hWndMain, WM_NULL, 0, 0);
					break;
				case DIERR_NOTINITIALIZED:
				case DIERR_INVALIDPARAM:
				default:
					mousemng_requestCreateInput = true;
					PostMessage(g_hWndMain, WM_NULL, 0, 0);
					break;
				}
			}else{
				mousebufX += (diMouseState.lX*mouseMul);
				mousebufY += (diMouseState.lY*mouseMul);
			}
		}else{
			mousebufX += (p.x - cp.x)*mouseMul;
			mousebufY += (p.y - cp.y)*mouseMul;
		}
		LeaveCriticalSection(&mousemng_multithread_deviceinit_cs);
		if(mousebufX >= mouseDiv || mousebufX <= -mouseDiv){
			mousemng.x += (SINT16)(mousebufX / mouseDiv);
			mousebufX   = mousebufX % mouseDiv;
		}
		if(mousebufY >= mouseDiv || mousebufY <= -mouseDiv){
			mousemng.y += (SINT16)(mousebufY / mouseDiv);
			mousebufY   = mousebufY % mouseDiv;
		}
		//mousemng.x += (SINT16)((p.x - cp.x));// / 2);
		//mousemng.y += (SINT16)((p.y - cp.y));// / 2);
		SetCursorPos(cp.x, cp.y);
	}
}

BOOL mousemng_buttonevent(UINT event) {
	
	np2_multithread_EnterCriticalSection();
	if (!mousemng.flag || (np2oscfg.mouse_nc/* && !scrnmng_isfullscreen()*/ && mousemng.flag)) {
		switch(event) {
			case MOUSEMNG_LEFTDOWN:
				mousemng.btn &= ~(uPD8255A_LEFTBIT);
				break;

			case MOUSEMNG_LEFTUP:
				mousemng.btn |= uPD8255A_LEFTBIT;
				break;

			case MOUSEMNG_RIGHTDOWN:
				mousemng.btn &= ~(uPD8255A_RIGHTBIT);
				break;

			case MOUSEMNG_RIGHTUP:
				mousemng.btn |= uPD8255A_RIGHTBIT;
				break;
		}
		np2_multithread_LeaveCriticalSection();
		return(TRUE);
	}
	else {
		np2_multithread_LeaveCriticalSection();
		return(FALSE);
	}
}

void mousemng_enable(UINT proc) {

	UINT	bit;
	
	np2_multithread_EnterCriticalSection();
	bit = 1 << proc;
	if (mousemng.flag & bit) {
		mousemng.flag &= ~bit;
		if (!mousemng.flag) {
			mousecapture(TRUE);
		}
	}
	np2_multithread_LeaveCriticalSection();
}

void mousemng_disable(UINT proc) {
	
	np2_multithread_EnterCriticalSection();
	if (!mousemng.flag) {
		mousecapture(FALSE);
	}
	mousemng.flag |= (1 << proc);
	np2_multithread_LeaveCriticalSection();
}

void mousemng_toggle(UINT proc) {
	
	np2_multithread_EnterCriticalSection();
	if (!mousemng.flag) {
		mousecapture(FALSE);
	}
	mousemng.flag ^= (1 << proc);
	if (!mousemng.flag) {
		mousecapture(TRUE);
	}
	np2_multithread_LeaveCriticalSection();
}

void mousemng_updateclip(){
	//np2_multithread_EnterCriticalSection();
	if(mousecaptureflg){
		mousecapture(FALSE);
		mousecapture(TRUE); // キャプチャし直し
	}
	//np2_multithread_LeaveCriticalSection();
}
