/**
 * @file	viewitem.h
 * @brief	DebugUty 用ビュー基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

class CDebugUtyView;

/**
 * @brief デバグ表示アイテムの基底クラス
 */
class CDebugUtyItem
{
public:
	static CDebugUtyItem* New(UINT nID, CDebugUtyView* lpView, const CDebugUtyItem* lpItem = NULL);

	CDebugUtyItem(CDebugUtyView* lpView, UINT nID);
	virtual ~CDebugUtyItem();
	UINT GetID() const;

	virtual void Initialize(const CDebugUtyItem* lpItem = NULL);
	virtual bool Update();
	virtual bool Lock();
	virtual void Unlock();
	virtual bool IsLocked();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void OnPaint(HDC hDC, const RECT& rect);

protected:
	CDebugUtyView* m_lpView;	//!< ビュー クラス
	UINT m_nID;					//!< ID
};

/**
 * ID を返す
 * @return ID
 */
inline UINT CDebugUtyItem::GetID() const
{
	return m_nID;
}
