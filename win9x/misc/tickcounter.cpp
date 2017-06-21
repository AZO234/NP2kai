/**
 * @file	tickcounter.cpp
 * @brief	TICK カウンタの動作の定義を行います
 */

#include "compiler.h"
#include "tickcounter.h"

/**
 * @brief TICK カウンター クラス
 */
class TickCounter
{
public:
	TickCounter();
	DWORD Get();

private:
	LARGE_INTEGER m_nFreq;		//!< 周波数
	LARGE_INTEGER m_nLast;		//!< 最後のカウンタ
	DWORD m_dwLastTick;			//!< 最後の TICK
};

/**
 * コンストラクタ
 */
TickCounter::TickCounter()
{
	m_nFreq.QuadPart = 0;
	::QueryPerformanceFrequency(&m_nFreq);
	if (m_nFreq.QuadPart)
	{
		m_dwLastTick = ::GetTickCount();
		::QueryPerformanceCounter(&m_nLast);
	}
}

/**
 * TICK を得る
 * @return TICK
 */
DWORD TickCounter::Get()
{
	if (m_nFreq.QuadPart)
	{
		LARGE_INTEGER nNow;
		::QueryPerformanceCounter(&nNow);
		const ULONGLONG nPast = nNow.QuadPart - m_nLast.QuadPart;

		const DWORD dwTick = static_cast<DWORD>((nPast * 1000U) / m_nFreq.QuadPart);
		const DWORD dwRet = m_dwLastTick + dwTick;
		if (dwTick >= 1000)
		{
			const DWORD dwSeconds = dwTick / 1000;
			m_nLast.QuadPart += m_nFreq.QuadPart * dwSeconds;
			m_dwLastTick += dwSeconds * 1000;
		}
		return dwRet;
	}
	else
	{
		return ::GetTickCount();
	}
}


// ---- C インタフェイス

//! カウンタ インスタンス
static TickCounter s_tick;

/**
 * カウンタを得る
 * @return TICK
 */
DWORD GetTickCounter()
{
	return s_tick.Get();
}
