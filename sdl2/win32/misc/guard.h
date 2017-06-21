/*!
 * @file	guard.h
 * @brief	クリティカル セクション クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/*!
 * @brief クリティカル セクション クラス
 */
class CGuard
{
public:
	/*! コンストラクタ */
	CGuard() { ::InitializeCriticalSection(&m_cs); }

	/*! デストラクタ */
	~CGuard() { ::DeleteCriticalSection(&m_cs); }

	/*! クリティカル セクション開始 */
	void Enter() { ::EnterCriticalSection(&m_cs); }

	/*! クリティカル セクション終了 */
	void Leave() { ::LeaveCriticalSection(&m_cs); }

private:
	CRITICAL_SECTION m_cs;		//!< クリティカル セクション情報
};
