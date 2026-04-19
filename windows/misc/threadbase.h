/*!
 * @file	threadbase.h
 * @brief	スレッド基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/*!
 * @brief スレッド基底クラス
 */
class CThreadBase
{
public:
	CThreadBase();
	virtual ~CThreadBase();

	bool Start();
	void Stop();
	bool Restart();
	void SetStackSize(unsigned int nStackSize);
	static void Delay(unsigned int usec);

protected:
	virtual bool Task() = 0;	//!< スレッド タスク

private:
	HANDLE m_hThread;			//!< スレッド ハンドル
	DWORD m_dwThreadId;			//!< スレッド ID
	bool m_bAbort;				//!< 中断フラグ
	bool m_bDone;				//!< 完了フラグ
	unsigned int m_nStackSize;		//!< スタック サイズ
	//static DWORD __stdcall ThreadProc(LPVOID pParam);
	static unsigned int __stdcall ThreadProc(LPVOID pParam);
};

/**
 * スタック サイズの設定
 * @param[in] nStackSize スタック サイズ
 */
inline void CThreadBase::SetStackSize(unsigned int nStackSize)
{
	m_nStackSize = nStackSize;
}

/**
 * スリープ
 * @param[in] usec マイクロ秒
 */
inline void CThreadBase::Delay(unsigned int usec)
{
	::Sleep((usec + 999) / 1000);
}
