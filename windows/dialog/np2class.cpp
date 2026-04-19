#include	"compiler.h"
#include	<prsht.h>
#include	"resource.h"
#include	"winloc.h"
#include	"np2class.h"
#include	"np2.h"


const TCHAR np2dlgclass[] = _T("np2dialog");


void np2class_initialize(HINSTANCE hinst) {

	WNDCLASS	wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_SAVEBITS | CS_DBLCLKS;
	wc.lpfnWndProc = DefDlgProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hinst;
	wc.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = np2dlgclass;
	RegisterClass(&wc);
}

void np2class_deinitialize(HINSTANCE hinst) {

	UnregisterClass(np2dlgclass, hinst);
}


// ----

void np2class_move(HWND hWnd, int posx, int posy, int cx, int cy) {

	RECT	workrc;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrc, 0);
	// マルチモニタ暫定対応 ver0.86 rev30
	workrc.right = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	workrc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	if (workrc.right < (posx + cx)) {
		posx = workrc.right - cx;
	}
	if (workrc.left > posx) {
		posx = workrc.left;
	}
	if (workrc.bottom < (posy + cy)) {
		posy = workrc.bottom - cy;
	}
	if (workrc.top > posy) {
		posy = workrc.top;
	}
	MoveWindow(hWnd, posx, posy, cx, cy, TRUE);
}

// ----

int CALLBACK np2class_propetysheet(HWND hWndDlg, UINT uMsg, LPARAM lParam) {

	if (uMsg == PSCB_INITIALIZED) {
		SetWindowLong(hWndDlg, GWL_EXSTYLE,
				GetWindowLong(hWndDlg, GWL_EXSTYLE) & (~WS_EX_CONTEXTHELP));
	}
	return(0);
}


// ----

void np2class_wmcreate(HWND hWnd) {

	SetWindowLongPtr(hWnd, NP2GWLP_HMENU, 0);
}

void np2class_wmdestroy(HWND hWnd) {

	HMENU	hmenu;

	hmenu = (HMENU)GetWindowLongPtr(hWnd, NP2GWLP_HMENU);
	if (hmenu != NULL) {
		DestroyMenu(hmenu);
		SetWindowLongPtr(hWnd, NP2GWLP_HMENU, 0);
	}
}

void np2class_enablemenu(HWND hWnd, BOOL enable) {

	HMENU	hmenu;
	BOOL	draw;

	hmenu = (HMENU)GetWindowLongPtr(hWnd, NP2GWLP_HMENU);
	draw = FALSE;
	if (enable) {
		if (hmenu) {
			SetMenu(hWnd, hmenu);
			hmenu = NULL;
			draw = TRUE;
		}
	}
	else {
		if (hmenu == NULL) {
			hmenu = GetMenu(hWnd);
			if (hmenu) {
				SetMenu(hWnd, NULL);
				draw = TRUE;
			}
		}
	}
	SetWindowLongPtr(hWnd, NP2GWLP_HMENU, (LONG_PTR)hmenu);
	if (draw) {
		DrawMenuBar(hWnd);
	}
}

void np2class_windowtype(HWND hWnd, UINT8 type) {

	RECT	rect;
	DWORD	style;

	GetClientRect(hWnd, &rect);
	style = GetWindowLong(hWnd, GWL_STYLE);
	switch(type) {
		case 0:
		default:
			style |= WS_CAPTION;
			np2class_enablemenu(hWnd, TRUE);
			break;

		case 1:
			style |= WS_CAPTION;
			np2class_enablemenu(hWnd, FALSE);
			break;

		case 2:
			style &= ~WS_CAPTION;
			np2class_enablemenu(hWnd, FALSE);
			break;
	}
	SetWindowLong(hWnd, GWL_STYLE, style);
	SetWindowPos(hWnd, 0, 0, 0, 0, 0,
					SWP_FRAMECHANGED | SWP_DRAWFRAME | 
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	winloc_setclientsize(hWnd,
							rect.right - rect.left, rect.bottom - rect.top);
}

void np2class_frametype(HWND hWnd, UINT8 thick) {

	RECT	rect;
	DWORD	style;

	GetClientRect(hWnd, &rect);
	style = GetWindowLong(hWnd, GWL_STYLE);
	if (thick) {
		style |= WS_THICKFRAME;
	}
	else {
		style &= ~WS_THICKFRAME;
	}
	SetWindowLong(hWnd, GWL_STYLE, style);
	SetWindowPos(hWnd, 0, 0, 0, 0, 0,
					SWP_FRAMECHANGED | SWP_DRAWFRAME | 
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	winloc_setclientsize(hWnd,
							rect.right - rect.left, rect.bottom - rect.top);
}



HMENU np2class_gethmenu(HWND hWnd) {

	HMENU	ret;

	ret = (HMENU)GetWindowLongPtr(hWnd, NP2GWLP_HMENU);
	if (ret == NULL) {
		ret = GetMenu(hWnd);
	}
	return(ret);
}

