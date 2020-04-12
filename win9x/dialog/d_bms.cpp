/**
 * @file	d_bms.cpp
 * @brief	BMS設定ダイアログ
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "c_combodata.h"
#include "np2.h"
#include "soundmng.h"
#include "sysmng.h"
#include "misc/DlgProc.h"
#include "pccore.h"
#include "common/strres.h"

#include	"bmsio.h"

#if defined(SUPPORT_BMS)

static const TCHAR *iostr[2] = {_T("00EC"),_T("01D0")};

static void bmsupdatedialog(HWND hWnd) {
/*
	char work[32];

	// メモリサイズ
	wsprintf(work, "%5d", 128 * bmsiocfg.numbanks);
	SetDlgItemText(hWnd, IDC_BMSKB, work);
*/
}

static void bmscreate(HWND hWnd) {
	TCHAR work[32];
	UINT val;

	// 使用するか
	SetDlgItemCheck(hWnd, IDC_BMS, bmsiocfg.enabled);

	// IOポート
	SETLISTSTR(hWnd, IDC_BMSIO, iostr);
	if (bmsiocfg.port == 0x01d0) {
		val = 1;
	}
	else {
		val = 0;
	}
	SendDlgItemMessage(hWnd, IDC_BMSIO, CB_SETCURSEL, val, 0);

	// バンク数
	wsprintf(work, _T("%d"), bmsiocfg.numbanks);
	SetDlgItemText(hWnd, IDC_BMSBANKS, work);


	bmsupdatedialog(hWnd);

	SetFocus(GetDlgItem(hWnd, IDC_BMS));

}

static void bmsupdate(HWND hWnd) {
	UINT	update;
	TCHAR	work[32];
	UINT	val;

	update = 0;

	// 使用するか
	val = GetDlgItemCheck(hWnd, IDC_BMS);
	if (bmsiocfg.enabled != (BOOL)val) {
		bmsiocfg.enabled = val;
		update |= SYS_UPDATECFG;
	}

	// IOポート
	GetDlgItemText(hWnd, IDC_BMSIO, work, sizeof(work));
	val = milstr_solveHEX(work);
	if (bmsiocfg.port != val) {
		bmsiocfg.port = val;
		update |= SYS_UPDATECFG;
	}

	// バンク数
	GetDlgItemText(hWnd, IDC_BMSBANKS, work, sizeof(work));
	val = (UINT)milstr_solveINT(work);
	if (val > 255) {
		val = 255;
	}
	if (bmsiocfg.numbanks != val) {
		bmsiocfg.numbanks = val;
		update |= SYS_UPDATECFG;
	}

	sysmng_update(update);
}

LRESULT CALLBACK BMSDialogProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {

	switch (msg) {
		case WM_INITDIALOG:
			bmscreate(hWnd);
			return(FALSE);

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					bmsupdate(hWnd);
					EndDialog(hWnd, IDOK);
					break;

				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
					break;
/*
				case IDC_BASECLOCK:
					setclock(hWnd, 0);
					return(FALSE);

				case IDC_MULTIPLE:
					if (HIWORD(wp) == CBN_SELCHANGE) {
						UINT val;
						val = (UINT)SendDlgItemMessage(hWnd, IDC_MULTIPLE,
														CB_GETCURSEL, 0, 0);
						if (val < sizeof(mulval)/sizeof(UINT32)) {
							setclock(hWnd, mulval[val]);
						}
					}
					else {
						setclock(hWnd, 0);
					}
					return(FALSE);
*/
				default:
					return(FALSE);
			}
			break;

		case WM_CLOSE:
			PostMessage(hWnd, WM_COMMAND, IDCANCEL, 0);
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}

/**
 * BMS設定ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
void dialog_bms(HWND hwndParent)
{
	CBMSDlg dlg(hwndParent);
	dlg.DoModal();
}
#endif
