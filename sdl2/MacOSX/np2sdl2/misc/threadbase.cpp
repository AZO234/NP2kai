/**
 * @file	threadbase.cpp
 * @brief	スレッド基底クラスの動作の定義を行います
 */

#include "compiler.h"
#include "threadbase.h"

/**
 * コンストラクタ
 */
CThreadBase::CThreadBase()
	: m_bCreated(false)
	, m_bDone(false)
	, m_stack_size(0)
{
}

/**
 * デストラクタ
 */
CThreadBase::~CThreadBase()
{
	Stop();
}

/**
 * スレッド開始
 * @retval true 成功
 */
bool CThreadBase::Start()
{
	if (m_bCreated)
	{
		return false;
	}

	/* スタック サイズ調整 */
	pthread_attr_t tattr;
	::pthread_attr_init(&tattr);
	if (m_stack_size != 0)
	{
		::pthread_attr_setstacksize(&tattr, m_stack_size);
	}

	m_bDone = false;
	if (::pthread_create(&m_thread, &tattr, StartRoutine, this) != 0)
	{
		return false;
	}

	m_bCreated = true;
	return true;
}

/**
 * スレッド終了
 * @retval true 成功
 */
void CThreadBase::Stop()
{
	if (m_bCreated)
	{
		m_bDone = true;
		::pthread_join(m_thread, NULL);
		m_bCreated = false;
	}
}

/**
 * スレッド処理
 * @param[in] arg this ポインタ
 * @retval 0 常に0
 */
void* CThreadBase::StartRoutine(void* arg)
{
	CThreadBase& obj = *(static_cast<CThreadBase*>(arg));
	while ((!obj.m_bDone) && (obj.Task()))
	{
	}

	return 0;
}
