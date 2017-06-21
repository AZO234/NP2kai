/*!
 * @file	guard.h
 * @brief	クリティカル セクション クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <pthread.h>

/*!
 * @brief クリティカル セクション クラス
 */
class CGuard
{
public:
	/*! コンストラクタ */
	CGuard() { ::pthread_mutex_init(&m_cs, NULL); }

	/*! デストラクタ */
	~CGuard() { ::pthread_mutex_destroy(&m_cs); }

	/*! クリティカル セクション開始 */
	void Enter() { ::pthread_mutex_lock(&m_cs); }

	/*! クリティカル セクション終了 */
	void Leave() { ::pthread_mutex_unlock(&m_cs); }

private:
	pthread_mutex_t m_cs;		//!< クリティカル セクション情報
};
