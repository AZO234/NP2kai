/**
 * @file	viewitem.cpp
 * @brief	DebugUty 用ビュー基底クラスの動作の定義を行います
 */

#include "compiler.h"
#include "resource.h"
#include "np2.h"
#include "viewitem.h"
#include "viewer.h"
#include "viewmem.h"
#include "viewreg.h"
#include "viewseg.h"
#include "view1mb.h"
#include "viewasm.h"
#include "viewsnd.h"

/**
 * インスタンス作成
 * @param[in] nID ビュー ID
 * @param[in] lpView ビューワ インスタンス
 * @param[in] lpItem 基準となるアイテム
 * @return インスタンス
 */
CDebugUtyItem* CDebugUtyItem::New(UINT nID, CDebugUtyView* lpView, const CDebugUtyItem* lpItem)
{
	CDebugUtyItem* lpNewItem = NULL;
	switch (nID)
	{
		case IDM_VIEWMODEREG:
			lpNewItem = new CDebugUtyReg(lpView);
			break;

		case IDM_VIEWMODESEG:
			lpNewItem = new CDebugUtySeg(lpView);
			break;

		case IDM_VIEWMODE1MB:
			lpNewItem = new CDebugUty1MB(lpView);
			break;

		case IDM_VIEWMODEASM:
			lpNewItem = new CDebugUtyAsm(lpView);
			break;

		case IDM_VIEWMODESND:
			lpNewItem = new CDebugUtySnd(lpView);
			break;
	}
	if (lpNewItem == NULL)
	{
		lpNewItem = new CDebugUtyItem(lpView, nID);
	}
	lpNewItem->Initialize(lpItem);
	return lpNewItem;
}

/**
 * コンストラクタ
 * @param[in] lpView ビューワ インスタンス
 * @param[in] nID ビュー ID
 */
CDebugUtyItem::CDebugUtyItem(CDebugUtyView* lpView, UINT nID)
	: m_lpView(lpView)
	, m_nID(nID)
{
}

/**
 * デストラクタ
 */
CDebugUtyItem::~CDebugUtyItem()
{
}

/**
 * 初期化
 * @param[in] lpItem リファレンス
 */
void CDebugUtyItem::Initialize(const CDebugUtyItem* lpItem)
{
	m_lpView->SetVScroll(0, 0);
}

/**
 * 更新
 * @retval true 更新あり
 * @retval false 更新なし
 */
bool CDebugUtyItem::Update()
{
	return false;
}

/**
 * ロック
 * @retval true 成功
 * @retval false 失敗
 */
bool CDebugUtyItem::Lock()
{
	return false;
}

/**
 * アンロック
 */
void CDebugUtyItem::Unlock()
{
}

/**
 * ロック中?
 * @retval true ロック中である
 * @retval false ロック中でない
 */
bool CDebugUtyItem::IsLocked()
{
	return false;
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 * @retval FALSE アプリケーションがこのメッセージを処理しなかった
 */
BOOL CDebugUtyItem::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

/**
 * 描画
 * @param[in] hDC デバイス コンテキスト
 * @param[in] rect 領域
 */
void CDebugUtyItem::OnPaint(HDC hDC, const RECT& rect)
{
}
