/**
 * @file	WndBase.h
 * @brief	ウィンドウ基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * @brief ウィンドウ基底クラス
 */
class CWndBase
{
public:
	HWND m_hWnd;            /*!< must be first data member */

	CWndBase(HWND hWnd = NULL);
	CWndBase& operator=(HWND hWnd);
	void Attach(HWND hWnd);
	HWND Detach();
//	BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu);
//	BOOL DestroyWindow();

	// Attributes
	operator HWND() const;
	DWORD GetStyle() const;
	BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);

	// Message Functions
	LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
	BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

	// Window Text Functions
	BOOL SetWindowText(LPCTSTR lpString);
	int GetWindowText(LPTSTR lpszStringBuf, int nMaxCount) const;
	int GetWindowTextLength() const;

	// Font Functions
	void SetFont(HFONT hFont, BOOL bRedraw = TRUE);

	// Menu Functions (non-child windows only)
	HMENU GetMenu() const;
	BOOL DrawMenuBar();
	HMENU GetSystemMenu(BOOL bRevert) const;

	// Window Size and Position Functions
	void MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE);

	// Window Size and Position Functions
	BOOL GetWindowRect(LPRECT lpRect) const;
	BOOL GetClientRect(LPRECT lpRect) const;

	// Coordinate Mapping Functions
	BOOL ClientToScreen(LPPOINT lpPoint) const;
	int MapWindowPoints(HWND hWndTo, LPPOINT lpPoint, UINT nCount) const;

	// Update and Painting Functions
	HDC BeginPaint(LPPAINTSTRUCT lpPaint);
	void EndPaint(LPPAINTSTRUCT lpPaint);
	BOOL UpdateWindow();
	BOOL Invalidate(BOOL bErase = TRUE);
	BOOL InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE);
	BOOL ShowWindow(int nCmdShow);

	// Window State Functions
	BOOL EnableWindow(BOOL bEnable = TRUE);
	HWND SetFocus();

	// Dialog-Box Item Functions
	BOOL CheckDlgButton(int nIDButton, UINT nCheck);
	UINT GetDlgItemInt(int nID, BOOL* lpTrans = NULL, BOOL bSigned = TRUE) const;
	UINT GetDlgItemText(int nID, LPTSTR lpStr, int nMaxCount) const;
	UINT IsDlgButtonChecked(int nIDButton) const;
	LRESULT SendDlgItemMessage(int nID, UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
	BOOL SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE);
	BOOL SetDlgItemText(int nID, LPCTSTR lpszString);

	// Window Access Functions
	CWndBase GetParent() const;

	// Window Tree Access
	CWndBase GetDlgItem(int nID) const;

	// Misc. Operations
	int SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE);
	BOOL IsWindow() const;
};

/**
 * コンストラクタ
 * @param[in] hWnd ウィンドウ ハンドル
 */
inline CWndBase::CWndBase(HWND hWnd)
	: m_hWnd(hWnd)
{
}

/**
 * オペレータ
 * @param[in] hWnd ウィンドウ ハンドル
 * @return インスタンス
 */
inline CWndBase& CWndBase::operator=(HWND hWnd)
{
	m_hWnd = hWnd;
	return *this;
}

/**
 * アタッチ
 * @param[in] hWnd ウィンドウ ハンドル
 */
inline void CWndBase::Attach(HWND hWnd)
{
	m_hWnd = hWnd;
}

/**
 * デタッチ
 * @return 以前のインスタンス
 */
inline HWND CWndBase::Detach()
{
	HWND hWnd = m_hWnd;
	m_hWnd = NULL;
	return hWnd;
}

/**
 * HWND オペレータ
 * @return HWND
 */
inline CWndBase::operator HWND() const
{
	return m_hWnd;
}

/**
 * 現在のウィンドウ スタイルを返します
 * @return ウィンドウのスタイル
 */
inline DWORD CWndBase::GetStyle() const
{
	return static_cast<DWORD>(::GetWindowLong(m_hWnd, GWL_STYLE));
}

/**
 * ウィンドウ スタイルを変更します
 * @param[in] dwRemove スタイルのリビジョン中に削除するウィンドウ スタイルを指定します
 * @param[in] dwAdd スタイルのリビジョン中に追加するウィンドウ スタイルを指定します
 * @param[in] nFlags SetWindowPosに渡すフラグ
 * @retval TRUE 変更された
 * @retval FALSE 変更されなかった
 */
inline BOOL CWndBase::ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	const DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
	const DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if (dwStyle == dwNewStyle)
	{
		return FALSE;
	}
	::SetWindowLong(m_hWnd, GWL_STYLE, dwNewStyle);
	if (nFlags != 0)
	{
		::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
	}
	return TRUE;
}

/**
 * このウィンドウに指定されたメッセージを送信します
 * @param[in] message 送信されるメッセージを指定します
 * @param[in] wParam 追加のメッセージ依存情報を指定します
 * @param[in] lParam 追加のメッセージ依存情報を指定します
 * @return メッセージの処理の結果
 */
inline LRESULT CWndBase::SendMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(m_hWnd, message, wParam, lParam);
}

/**
 * メッセージをウィンドウのメッセージ キューに置き、対応するウィンドウがメッセージを処理するのを待たずに返されます
 * @param[in] message ポストするメッセージを指定します
 * @param[in] wParam メッセージの付加情報を指定します
 * @param[in] lParam メッセージの付加情報を指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::PostMessage(m_hWnd, message, wParam, lParam);
}

/**
 * 指定されたウィンドウのタイトルバーのテキストを変更します
 * @param[in] lpString 新しいウィンドウタイトルまたはコントロールのテキストとして使われる、NULL で終わる文字列へのポインタを指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::SetWindowText(LPCTSTR lpString)
{
	return ::SetWindowText(m_hWnd, lpString);
}

/**
 * 指定されたウィンドウのタイトルバーのテキストをバッファへコピーします
 * @param[in] lpszStringBuf バッファへのポインタを指定します。このバッファにテキストが格納されます
 * @param[in] nMaxCount バッファにコピーする文字の最大数を指定します。テキストのこのサイズを超える部分は、切り捨てられます。NULL 文字も数に含められます
 * @return コピーされた文字列の文字数が返ります (終端の NULL 文字は含められません)
 */
inline int CWndBase::GetWindowText(LPTSTR lpszStringBuf, int nMaxCount) const
{
	return ::GetWindowText(m_hWnd, lpszStringBuf, nMaxCount);
}

/**
 * 指定されたウィンドウのタイトルバーテキストの文字数を返します
 * @return 関数が成功すると、テキストの文字数が返ります
 */
inline int CWndBase::GetWindowTextLength() const
{
	return ::GetWindowTextLength(m_hWnd);
}

/**
 * 指定したフォントを使用します
 * @param[in] hFont フォント ハンドル
 * @param[in] bRedraw メッセージを処理した直後にウィンドウを再描画する場合は TRUE
 */
inline void CWndBase::SetFont(HFONT hFont, BOOL bRedraw)
{
	::SendMessage(m_hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), bRedraw);
}

/**
 * 指定されたウィンドウに割り当てられているメニューのハンドルを取得します
 * @return メニューのハンドルが返ります
 */
inline HMENU CWndBase::GetMenu() const
{
	return ::GetMenu(m_hWnd);
}

/**
 * 指定されたウィンドウのメニューバーを再描画します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::DrawMenuBar()
{
	return ::DrawMenuBar(m_hWnd);
}

/**
 * 指定されたウィンドウに割り当てられているシステム メニューのハンドルを取得します
 * @param[in] bRevert 実行されるアクションを指定します
 * @return メニューのハンドルが返ります
 */
inline HMENU CWndBase::GetSystemMenu(BOOL bRevert) const
{
	return ::GetSystemMenu(m_hWnd, bRevert);
}

/**
 * 位置とサイズを変更します
 * @param[in] x 左側の新しい位置を指定します
 * @param[in] y 上側の新しい位置を指定します
 * @param[in] nWidth 新しい幅を指定します
 * @param[in] nHeight 新しい高さを指定します
 * @param[in] bRepaint 再描画する必要があるかどうかを指定します
 * @
 */
inline void CWndBase::MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint)
{
	::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint);
}

/**
 * 指定されたウィンドウの左上端と右下端の座標をスクリーン座標で取得します
 * @param[out] lpRect 構造体へのポインタを指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::GetWindowRect(LPRECT lpRect) const
{
	return ::GetWindowRect(m_hWnd, lpRect);
}

/**
 * lpRectが指す構造に CWnd のクライアント領域のクライアント座標をコピーします
 * @param[out] lpRect 構造体へのポインタを指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::GetClientRect(LPRECT lpRect) const
{
	return ::GetClientRect(m_hWnd, lpRect);
}

/**
 * 指定された点を、クライアント座標からスクリーン座標へ変換します
 * @param[in,out] lpPoint 変換対象のクライアント座標を保持している、1 個の 構造体へのポインタを指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::ClientToScreen(LPPOINT lpPoint) const
{
	return ::ClientToScreen(m_hWnd, lpPoint);
}

/**
 * 複数の点を、あるウィンドウを基準とする座標空間から、他のウィンドウを基準とする座標空間へ変換（マップ）します
 * @param[in] hWndTo 変換後の点を保持する（変換先）ウィンドウのハンドルを指定します
 * @param[in,out] lpPoint 変換対象の点の座標を保持している 構造体からなる 1 つの配列へのポインタを指定します
 * @param[in] nCount lpPoint パラメータで、複数の POINT 構造体からなる 1 つの配列へのポインタを指定した場合、配列内の POINT 構造体の数を指定します
 * @return 関数が成功すると、各点の移動距離を示す 32 ビット値が返ります
 */
inline int CWndBase::MapWindowPoints(HWND hWndTo, LPPOINT lpPoint, UINT nCount) const
{
	return ::MapWindowPoints(m_hWnd, hWndTo, lpPoint, nCount);
}

/**
 * 描画を開始します
 * @param[out] lpPaint 描画情報へのポインタを指定します
 * @return デバイス コンテキスト
 */
inline HDC CWndBase::BeginPaint(LPPAINTSTRUCT lpPaint)
{
	return ::BeginPaint(m_hWnd, lpPaint);
}

/**
 * 描画の終了します
 * @param[in] lpPaint 描画情報へのポインタを指定します
 */
inline void CWndBase::EndPaint(LPPAINTSTRUCT lpPaint)
{
	::EndPaint(m_hWnd, lpPaint);
}

/**
 * 指定されたウィンドウの更新リージョンが空ではない場合、ウィンドウへ メッセージを送信し、そのウィンドウのクライアント領域を更新します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::UpdateWindow()
{
	return ::UpdateWindow(m_hWnd);
}

/**
 * 指定されたウィンドウのすべてを更新リージョンにします
 * @param[in] bErase 更新リージョンを処理するときに、更新リージョン内の背景を消去するかどうかを指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::Invalidate(BOOL bErase)
{
	return ::InvalidateRect(m_hWnd, NULL, bErase);
}

/**
 * 指定されたウィンドウの更新リージョンに1個の長方形を追加します
 * @param[in] lpRect 更新リージョンへ追加したい長方形のクライアント座標を保持する1個の構造体へのポインタを指定します
 * @param[in] bErase 更新リージョンを処理するときに、更新リージョン内の背景を消去するかどうかを指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::InvalidateRect(LPCRECT lpRect, BOOL bErase)
{
	return ::InvalidateRect(m_hWnd, lpRect, bErase);
}

/**
 * 指定されたウィンドウの表示状態を設定します
 * @param[in] nCmdShow ウィンドウの表示方法を指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::ShowWindow(int nCmdShow)
{
	return ::ShowWindow(m_hWnd, nCmdShow);
}

/**
 * 指定されたウィンドウまたはコントロールで、マウス入力とキーボード入力を有効または無効にします
 * @param[in] bEnable ウィンドウを有効にするか無効にするかを指定します
 * @retval TRUE ウィンドウが既に無効になっている
 * @retval FALSE ウィンドウが無効になっていなかった
 */
inline BOOL CWndBase::EnableWindow(BOOL bEnable)
{
	return ::EnableWindow(m_hWnd, bEnable);
}

/**
 * 入力フォーカスを要求します
 * @return 直前に入力フォーカスを持っていたウィンドウ ハンドル
 */
inline HWND CWndBase::SetFocus()
{
	return ::SetFocus(m_hWnd);
}

/**
 * ボタンコントロールのチェック状態を変更します
 * @param[in] nIDButton 状態を変更したいボタンの識別子を指定します
 * @param[in] nCheck ボタンのチェック状態を指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::CheckDlgButton(int nIDButton, UINT nCheck)
{
	return ::CheckDlgButton(m_hWnd, nIDButton, nCheck);
}

/**
 * ダイアログボックス内の指定されたコントロールのテキストを、整数値へ変換します
 * @param[in] nID 変換したいテキストを持つコントロールの識別子を指定します
 * @param[in] lpTrans 成功か失敗の値を受け取る変数へのポインタを指定します
 * @param[in] bSigned テキストを符号付きとして扱って符号付きの値を返すかどうかを指定します
 * @return コントロールテキストに相当する整数値が返ります
 */
inline UINT CWndBase::GetDlgItemInt(int nID, BOOL* lpTrans, BOOL bSigned) const
{
	return ::GetDlgItemInt(m_hWnd, nID, lpTrans, bSigned);
}

/**
 * ダイアログボックス内の指定されたコントロールに関連付けられているタイトルまたはテキストを取得します
 * @param[in] nID 取得したいタイトルまたはテキストを保持しているコントロールの識別子を指定します
 * @param[out] lpStr タイトルまたはテキストを受け取るバッファへのポインタを指定します
 * @param[in] nMaxCount lpStr パラメータが指すバッファへコピーされる文字列の最大の長さを TCHAR 単位で指定します
 * @return バッファへコピーされた文字列の長さ（ 終端の NULL を含まない）が TCHAR 単位で返ります
 */
inline UINT CWndBase::GetDlgItemText(int nID, LPTSTR lpStr, int nMaxCount) const
{
	return ::GetDlgItemText(m_hWnd, nID, lpStr, nMaxCount);
}

/**
 * ボタンコントロールのチェック状態を取得します
 * @param[in] nIDButton ボタンコントロールの識別子を指定します
 * @return チェック状態
 */
inline UINT CWndBase::IsDlgButtonChecked(int nIDButton) const
{
	return ::IsDlgButtonChecked(m_hWnd, nIDButton);
}

/**
 * ダイアログボックス内の指定されたコントロールへメッセージを送信します。
 * @param[in] nID メッセージを受け取るコントロールの識別子を指定します
 * @param[in] message 送信したいメッセージを指定します
 * @param[in] wParam メッセージの追加情報を指定します
 * @param[in] lParam メッセージの追加情報を指定します
 * @return メッセージ処理の結果が返ります
 */
inline LRESULT CWndBase::SendDlgItemMessage(int nID, UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::SendDlgItemMessage(m_hWnd, nID, message, wParam, lParam);
}

/**
 * 指定された整数値を文字列へ変換し、ダイアログボックス内のコントロールにテキストとして設定します
 * @param[in] nID 変更を加えたいコントロールの識別子を指定します
 * @param[in] nValue 整数値を指定します
 * @param[in] bSigned nValue パラメータの値が符号付きかどうかを指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::SetDlgItemInt(int nID, UINT nValue, BOOL bSigned)
{
	return ::SetDlgItemInt(m_hWnd, nID, nValue, bSigned);
}

/**
 * ダイアログボックス内のコントロールのタイトルまたはテキストを設定します
 * @param[in] nID テキストを設定したいコントロールの識別子を指定します
 * @param[in] lpszString コントロールへコピーしたいテキストを保持する、NULL で終わる文字列へのポインタを指定します
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
inline BOOL CWndBase::SetDlgItemText(int nID, LPCTSTR lpszString)
{
	return ::SetDlgItemText(m_hWnd, nID, lpszString);
}

/**
 * 指定された子ウィンドウの親ウィンドウまたはオーナーウィンドウのハンドルを返します
 * @return 親ウィンドウのハンドル
 */
inline CWndBase CWndBase::GetParent() const
{
	return CWndBase(::GetParent(m_hWnd));
}

/**
 * 指定されたダイアログボックス内のコントロールのハンドルを取得します
 * @param[in] nID ハンドルを取得したいコントロールの識別子を指定します
 * @return ウィンドウ
 */
inline CWndBase CWndBase::GetDlgItem(int nID) const
{
	return CWndBase(::GetDlgItem(m_hWnd, nID));
}

/**
 * スクロールバーのさまざまなパラメータを設定します
 * @param[in] nBar パラメータを設定するべきスクロールバーのタイプを指定します
 * @param[in] lpScrollInfo 設定するべき情報を保持している、1個の構造体へのポインタを指定します
 * @param[in] bRedraw スクロールバーを再描画するかどうかを指定します
 * @return スクロールバーの現在のスクロール位置が返ります
 */
inline int CWndBase::SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw)
{
	return ::SetScrollInfo(m_hWnd, nBar, lpScrollInfo, bRedraw);
}

/**
 * ウィンドウが存在しているかどうかを調べます
 * @return 指定したウィンドウハンドルを持つウィンドウが存在している場合は、0 以外の値が返ります
 */
inline BOOL CWndBase::IsWindow() const
{
	return ::IsWindow(m_hWnd);
}
