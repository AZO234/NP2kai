/*!
 * @file	threadbase.cpp
 * @brief	スレッド基底クラスの動作の定義を行います
 */

#include <compiler.h>
#include "threadbase.h"
#include <process.h>

/*!
 * @brief コンストラクタ
 */
CThreadBase::CThreadBase()
	: m_hThread(INVALID_HANDLE_VALUE)
	, m_dwThreadId(0)
	, m_bAbort(false)
	, m_bDone(false)
	, m_nStackSize(0)
{
}

/*!
 * @brief デストラクタ
 */
CThreadBase::~CThreadBase()
{
	Stop();
}

/*!
 * @brief スレッド開始
 *
 * @retval true 成功
 */
bool CThreadBase::Start()
{
	if (m_hThread != INVALID_HANDLE_VALUE)
	{
		return false;
	}

	m_bAbort = false;
	m_bDone = false;
	//DWORD dwThreadId = 0;
	unsigned int dwThreadId = 0;
	//HANDLE hThread = ::CreateThread(NULL, m_nStackSize, &ThreadProc, this, 0, &dwThreadId);
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, m_nStackSize, &ThreadProc, this, 0, &dwThreadId);
	if (hThread == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	m_hThread = hThread;
	m_dwThreadId = dwThreadId;
	return true;
}

/*!
 * @brief スレッド終了
 *
 * @retval true 成功
 */
void CThreadBase::Stop()
{
	if (m_hThread != INVALID_HANDLE_VALUE)
	{
		m_bAbort = true;
		::WaitForSingleObject(m_hThread, INFINITE);
		::CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
	}
}

/*!
 * @brief スレッド再開
 *
 * @retval true 成功
 */
bool CThreadBase::Restart()
{
	if ((m_hThread != INVALID_HANDLE_VALUE) && (m_bDone))
	{
		Stop();
	}
	return Start();
}

/*!
 * スレッド処理
 * @param[in] pParam this ポインタ
 * @retval 0 常に0
 */
//DWORD __stdcall CThreadBase::ThreadProc(LPVOID pParam)
unsigned int __stdcall  CThreadBase::ThreadProc(LPVOID pParam)
{
	CThreadBase& obj = *(static_cast<CThreadBase*>(pParam));
	while ((!obj.m_bAbort) && (obj.Task()))
	{
	}

	obj.m_bDone = true;
	return 0;
}
