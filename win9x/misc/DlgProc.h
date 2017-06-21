/**
 * @file	DlgProc.h
 * @brief	ダイアログ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <commctrl.h>
#include "tstring.h"
#include "WndProc.h"

/**
 * @brief ダイアログ クラス
 */
class CDlgProc : public CWndProc
{
public:
	CDlgProc();
	CDlgProc(UINT nIDTemplate, HWND hwndParent = NULL);
	virtual ~CDlgProc();
	virtual INT_PTR DoModal();
	virtual BOOL OnInitDialog();

	/**
	 * モーダル ダイアログ ボックスを終了する
	 * @param[in] nResult DoModalの呼び出し元に返す値
	 */
	void CDlgProc::EndDialog(int nResult)
	{
		::EndDialog(m_hWnd, nResult);
	}

protected:
	LPCTSTR m_lpszTemplateName;		//!< テンプレート名
	HWND m_hwndParent;				//!< 親ウィンドウ

	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnOK();
	virtual void OnCancel();

#if defined(_WIN64)
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#else	// defined(_WIN64)
	static BOOL CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif	// defined(_WIN64)
};


/**
 * @brief コンボ ボックス
 */
class CComboBoxProc : public CWndProc
{
public:
	/**
	 * コンボ ボックスのリスト ボックスに文字列を追加します
	 * @param[in] lpszString 追加された null で終わる文字列へのポインター
	 * @return 文字列が挿入された位置を示すインデックス
	 */
	int AddString(LPCTSTR lpszString)
	{
		return static_cast<int>(::SendMessage(m_hWnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(lpszString)));
	}

	/**
	 * コンボ ボックスのリスト ボックス部分の項目数を取得するには、このメンバー関数を呼び出します
	 * @return 項目の数
	 */
	int GetCount() const
	{
		return static_cast<int>(::SendMessage(m_hWnd, CB_GETCOUNT, 0, 0));
	}

	/**
	 * コンボ ボックスのどの項目が選択されたかを判定するためにこのメンバー関数を呼び出します
	 * @return コンボ ボックスのリスト ボックスで現在選択されている項目のインデックス
	 */
	int GetCurSel() const
	{
		return static_cast<int>(::SendMessage(m_hWnd, CB_GETCURSEL, 0, 0));
	}

	/**
	 * 指定したコンボ ボックスの項目に関連付けられたアプリケーションに用意された 32 ビット値を取得します
	 * @param[in] nIndex コンボ ボックスのリスト ボックスの項目のインデックス
	 * @return 32 ビット値
	 */
	DWORD_PTR GetItemData(int nIndex) const
	{
		return static_cast<DWORD_PTR>(::SendMessage(m_hWnd, CB_GETITEMDATA, static_cast<WPARAM>(nIndex), 0));
	}

	/**
	 * コンボ ボックスのリスト ボックスで指定されているプレフィックスを含む最初の文字列を検索します
	 * @param[in] nStartAfter 検索する最初の項目の前の項目のインデックス
	 * @param[in] lpszString 検索する文字列
	 * @return インデックス
	 */
	int FindString(int nStartAfter, LPCTSTR lpszString) const
	{
		return static_cast<int>(::SendMessage(m_hWnd, CB_FINDSTRING, nStartAfter, reinterpret_cast<LPARAM>(lpszString)));
	}

	/**
	 * コンボ ボックスのリスト ボックスで指定されている最初の文字列を検索します
	 * @param[in] nStartAfter 検索する最初の項目の前の項目のインデックス
	 * @param[in] lpszString 検索する文字列
	 * @return インデックス
	 */
	int FindStringExact(int nStartAfter, LPCTSTR lpszString) const
	{
		return static_cast<int>(::SendMessage(m_hWnd, CB_FINDSTRINGEXACT, nStartAfter, reinterpret_cast<LPARAM>(lpszString)));
	}

	/**
	 * コンボ ボックスのリスト ボックスに文字列を追加します
	 * @param[in] nIndex 文字列を受け取るリスト ボックスの位置
	 * @param[in] lpszString 追加された null で終わる文字列へのポインター
	 * @return 文字列が挿入された位置を示すインデックス
	 */
	int InsertString(int nIndex, LPCTSTR lpszString)
	{
		return static_cast<int>(::SendMessage(m_hWnd, CB_INSERTSTRING, static_cast<WPARAM>(nIndex), reinterpret_cast<LPARAM>(lpszString)));
	}

	/**
	 * コンボ ボックスのリスト ボックスとエディット コントロールからすべての項目を削除します。
	 */
	void ResetContent()
	{
		::SendMessage(m_hWnd, CB_RESETCONTENT, 0, 0);
	}

	/**
	 * 32 ビット値をコンボ ボックスの指定項目に関連付けられる
	 * @param[in] nIndex 項目に始まるインデックスを設定するためのメソッドが含まれます
	 * @param[in] dwItemData 新しい値を項目に関連付けるに含まれています
	 * @return エラーの時は CB_ERR
	 */
	int SetItemData(int nIndex, DWORD_PTR dwItemData)
	{
		return static_cast<int>(::SendMessage(m_hWnd, CB_SETITEMDATA, static_cast<WPARAM>(nIndex), static_cast<LPARAM>(dwItemData)));
	}

	/**
	 * コンボ ボックスのリスト ボックスの文字列を選択します
	 * @param[in] nSelect 文字列のインデックスを選択するように指定します
	 * @return メッセージが成功した場合は選択された項目のインデックス
	 */
	int SetCurSel(int nSelect)
	{
		return static_cast<int>(::SendMessage(m_hWnd, CB_SETCURSEL, static_cast<WPARAM>(nSelect), 0));
	}
};

/**
 * @brief スライダー ボックス
 */
class CSliderProc : public CWndProc
{
public:
	/**
	 * スライダーの現在位置を取得します
	 * @return 現在位置を返します
	 */
	int GetPos() const
	{
		return static_cast<int>(::SendMessage(m_hWnd, TBM_GETPOS, 0, 0));
	}

	/**
	 * スライダー コントロールでスライダーの最小範囲を設定します
	 * @param[in] nMin スライダーの最小の位置
	 * @param[in] bRedraw 再描画のフラグ
	 */
	void SetRangeMin(int nMin, BOOL bRedraw)
	{
		::SendMessage(m_hWnd, TBM_SETRANGEMIN, bRedraw, nMin);
	}

	/**
	 * スライダー コントロールでスライダーの最大範囲を設定します
	 * @param[in] nMax スライダーの最大の位置
	 * @param[in] bRedraw 再描画のフラグ
	 */
	void SetRangeMax(int nMax, BOOL bRedraw)
	{
		::SendMessage(m_hWnd, TBM_SETRANGEMAX, bRedraw, nMax);
	}

	/**
	 * スライダーの現在位置を設定します
	 * @param[in] nPos 新しいスライダーの位置を指定します
	 */
	void SetPos(int nPos)
	{
		::SendMessage(m_hWnd, TBM_SETPOS, TRUE, nPos);
	}
};

/**
 * @brief ファイル選択
 */
class CFileDlg
{
public:
	CFileDlg(BOOL bOpenFileDialog, LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL, DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR lpszFilter = NULL, HWND hParentWnd = NULL);
	int DoModal();

	/**
	 * ファイル名の取得
	 * @return full path and filename
	 */
	LPCTSTR GetPathName() const
	{
		return m_ofn.lpstrFile;
	}

	/**
	 * Readoly?
	 * @return TRUE if readonly checked
	 */
	BOOL GetReadOnlyPref() const
	{
		return (m_ofn.Flags & OFN_READONLY) ? TRUE : FALSE;
	}

public:
	OPENFILENAME m_ofn;				//!< open file parameter block

protected:
	BOOL m_bOpenFileDialog;			//!< TRUE for file open, FALSE for file save
	std::tstring m_strFilter;		//!< filter string
	TCHAR m_szFileTitle[64];		//!< contains file title after return
	TCHAR m_szFileName[_MAX_PATH];	//!< contains full path name after return
};
