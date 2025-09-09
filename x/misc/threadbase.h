/**
 * @file	threadbase.h
 * @brief	スレッド基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <pthread.h>
#include <unistd.h>

/**
 * @brief スレッド基底クラス
 */
class CThreadBase
{
public:
	CThreadBase();
	virtual ~CThreadBase();

	bool Start();
	void Stop();
	void SetStackSize(size_t stack_size);
	static void Delay(unsigned int usec);

protected:
	virtual bool Task()=0;		//!< スレッド タスク

private:
	pthread_t m_thread;			//!< スレッド フラグ
	bool m_bCreated;			//!< スレッド作成フラグ
	bool m_bDone;				//!< 終了フラグ
	size_t m_stack_size;		//!< スタック サイズ
	static void* StartRoutine(void* arg);
};

/**
 * スタック サイズの設定
 * @param[in] stack_size スタック サイズ
 */
inline void CThreadBase::SetStackSize(size_t stack_size)
{
	m_stack_size = stack_size;
}

/**
 * スリープ
 * @param[in] usec マイクロ秒
 */
inline void CThreadBase::Delay(unsigned int usec)
{
	::usleep(usec);
}
