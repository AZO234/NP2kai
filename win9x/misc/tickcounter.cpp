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
	LARGE_INTEGER Get_rawclock();
	LARGE_INTEGER Get_clockpersec();
	void SetMode(int mode);
	int GetMode();

private:
	LARGE_INTEGER m_nFreq;		//!< 周波数
	LARGE_INTEGER m_nLast;		//!< 最後のカウンタ
	DWORD m_dwLastTick;			//!< 最後の TICK
	int m_mode;					//!< カウンタモード
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
		::QueryPerformanceCounter(&m_nLast);
		m_mode = TCMODE_PERFORMANCECOUNTER;
	}
	else
	{
		m_mode = TCMODE_TIMEGETTIME;
	}
	m_dwLastTick = ::timeGetTime();
	//m_dwLastTick = ::GetTickCount();
}

/**
 * TICK を得る
 * @return TICK
 */
DWORD TickCounter::Get()
{
	switch(m_mode)
	{
	case TCMODE_GETTICKCOUNT:
		return ::GetTickCount();

	case TCMODE_TIMEGETTIME:
		return ::timeGetTime();

	case TCMODE_PERFORMANCECOUNTER:
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
	}
	return ::GetTickCount();
}

LARGE_INTEGER TickCounter::Get_rawclock()
{
	LARGE_INTEGER nNow = {0};
	switch(m_mode)
	{
	case TCMODE_GETTICKCOUNT:
		nNow.LowPart = ::GetTickCount();
		return nNow;

	case TCMODE_TIMEGETTIME:
		nNow.LowPart = ::timeGetTime();
		return nNow;

	case TCMODE_PERFORMANCECOUNTER:
		{
			::QueryPerformanceCounter(&nNow);
			return nNow;
		}
	}
	nNow.LowPart = ::GetTickCount();
	return nNow;
}
LARGE_INTEGER TickCounter::Get_clockpersec()
{
	LARGE_INTEGER nClk = {0};
	switch(m_mode)
	{
	case TCMODE_GETTICKCOUNT:
		nClk.LowPart = 1000;
		return nClk;

	case TCMODE_TIMEGETTIME:
		nClk.LowPart = 1000;
		return nClk;

	case TCMODE_PERFORMANCECOUNTER:
		{
			::QueryPerformanceFrequency(&nClk);
			return nClk;
		}
	}
	nClk.LowPart = 1000;
	return nClk;
}

/**
 * モード強制設定
 */
void TickCounter::SetMode(int mode)
{
	if (mode==TCMODE_DEFAULT)
	{
		mode = TCMODE_PERFORMANCECOUNTER;
	}
	if (mode==TCMODE_PERFORMANCECOUNTER && m_nFreq.QuadPart==0)
	{
		mode = TCMODE_TIMEGETTIME;
	}
	switch(mode)
	{
	case TCMODE_GETTICKCOUNT:
		m_mode = mode;
		m_dwLastTick = ::GetTickCount();
		break;

	case TCMODE_TIMEGETTIME:
		m_mode = mode;
		m_dwLastTick = ::timeGetTime();
		break;

	case TCMODE_PERFORMANCECOUNTER:
		m_mode = mode;
		::QueryPerformanceCounter(&m_nLast);
		break;
	}
}
int TickCounter::GetMode()
{
	return m_mode;
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

/**
 * カウンタモード設定
 */
void SetTickCounterMode(int mode)
{
	s_tick.SetMode(mode);
}
int GetTickCounterMode()
{
	return s_tick.GetMode();
}

/**
 * クロックを得る
 * @return TICK
 */
LARGE_INTEGER GetTickCounter_Clock()
{
	return s_tick.Get_rawclock();
}

/**
 * 1秒あたりのクロックを得る
 */
LARGE_INTEGER GetTickCounter_ClockPerSec()
{
	return s_tick.Get_clockpersec();
}
