/**
 * @file	d_clnd.cpp
 * @brief	カレンダ設定ダイアログ
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "np2.h"
#include "sysmng.h"
#include "timemng.h"
#include "misc/DlgProc.h"
#include "calendar.h"
#include "pccore.h"
#include "common/strres.h"

/**
 * @brief カレンダ設定ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
class CCalendarDlg : public CDlgProc
{
public:
	CCalendarDlg(HWND hwndParent);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	void SetTime(const UINT8* cbuf);
	void EnableVirtualCalendar(BOOL bEnabled);
	static UINT8 getbcd(LPCTSTR str, int len);
};

/**
 * @brief ダイアログ アイテム
 */
struct Item
{
	UINT16	res;		//!< ID
	UINT8	min;		//!< 最小値
	UINT8	max;		//!< 最大値
};

/**
 * アイテム
 */
static const Item s_vircal[6] =
{
	{IDC_VIRYEAR,	0x00, 0x99},
	{IDC_VIRMONTH,	0x01, 0x12},
	{IDC_VIRDAY,	0x01, 0x31},
	{IDC_VIRHOUR,	0x00, 0x23},
	{IDC_VIRMINUTE,	0x00, 0x59},
	{IDC_VIRSECOND,	0x00, 0x59}
};

/**
 * コンストラクタ
 * @param[in] hwndParent 親ウィンドウ
 */
CCalendarDlg::CCalendarDlg(HWND hwndParent)
	: CDlgProc(IDD_CALENDAR, hwndParent)
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL CCalendarDlg::OnInitDialog()
{
	// 時間をセット。
	UINT8 cbuf[6];
	calendar_getvir(cbuf);
	SetTime(cbuf);

	const UINT nID = (np2cfg.calendar) ? IDC_CLNDREAL : IDC_CLNDVIR;
	EnableVirtualCalendar((nID == IDC_CLNDVIR) ? TRUE : FALSE);
	CheckDlgButton(nID, BST_CHECKED);
	GetDlgItem(nID).SetFocus();
	return FALSE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void CCalendarDlg::OnOK()
{
	const UINT8 bMode = (IsDlgButtonChecked(IDC_CLNDREAL) != BST_UNCHECKED) ? 1 : 0;
	if (np2cfg.calendar != bMode)
	{
		np2cfg.calendar = bMode;
		sysmng_update(SYS_UPDATECFG);
	}

	UINT8 cbuf[6];
	for (UINT i = 0; i < 6; i++)
	{
		TCHAR work[32];
		GetDlgItemText(s_vircal[i].res, work, NELEMENTS(work));
		UINT8 b = getbcd(work, 2);
		if ((b >= s_vircal[i].min) && (b <= s_vircal[i].max))
		{
			if (i == 1)
			{
				b = ((b & 0x10) * 10) + (b << 4);
			}
			cbuf[i] = b;
		}
	}
	calendar_set(cbuf);
	
	CDlgProc::OnOK();
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL CCalendarDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_CLNDVIR:
			EnableVirtualCalendar(TRUE);
			return TRUE;

		case IDC_CLNDREAL:
			EnableVirtualCalendar(FALSE);
			return TRUE;

		case IDC_SETNOW:
			{
				UINT8 cbuf[6];
				calendar_getreal(cbuf);
				SetTime(cbuf);
			}
			return TRUE;
	}
	return FALSE;
}

/**
 * 時間を設定する
 * @param[in] cbuf カレンダ情報
 */
void CCalendarDlg::SetTime(const UINT8* cbuf)
{
	for (UINT i = 0; i < 6; i++)
	{
		TCHAR work[8];
		if (i != 1)
		{
			wsprintf(work, str_2x, cbuf[i]);
		}
		else
		{
			wsprintf(work, str_2d, cbuf[1] >> 4);
		}
		SetDlgItemText(s_vircal[i].res, work);
	}
}

/**
 * 仮想カレンダ アイテムの一括設定
 * @param[in] bEnabled 有効フラグ
 */
void CCalendarDlg::EnableVirtualCalendar(BOOL bEnabled)
{
	for (UINT i = 0; i < 6; i++)
	{
		GetDlgItem(s_vircal[i].res).EnableWindow(bEnabled);
	}
	GetDlgItem(IDC_SETNOW).EnableWindow(bEnabled);
}

/**
 * BCD を得る
 * @param[in] str 文字列
 * @param[in] len 長さ
 * @return 値
 */
UINT8 CCalendarDlg::getbcd(LPCTSTR str, int len)
{
	UINT ret = 0;
	while (len--)
	{
		TCHAR c = *str++;
		if (!c)
		{
			break;
		}
		if ((c < '0') || (c > '9'))
		{
			return 0xff;
		}
		ret <<= 4;
		ret |= (UINT)(c - '0');
	}
	return static_cast<UINT8>(ret);
}

/**
 * カレンダ設定ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
void dialog_calendar(HWND hwndParent)
{
	CCalendarDlg dlg(hwndParent);
	dlg.DoModal();
}
