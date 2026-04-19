/**
 * @file	cmmidiin32.h
 * @brief	MIDI IN win32 クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <deque>
#include <map>

/**
 * @brief MIDI IN win32 クラス
 */
class CComMidiIn32
{
public:
	static CComMidiIn32* CreateInstance(LPCTSTR lpMidiIn);

	CComMidiIn32(HMIDIIN hMidiIn);
	~CComMidiIn32();
	UINT Read(UINT8* pData);
	static void RecvData(HMIDIIN hMidiIn, UINT nMessage);
	static void RecvExcv(HMIDIIN hMidiIn, MIDIHDR* lpMidiHdr);

private:
	static std::map<HMIDIIN, CComMidiIn32*> sm_midiinMap;	/*!< ハンドル マップ */

	HMIDIIN m_hMidiIn;						/*!< MIDIIN ハンドル */
	MIDIHDR m_midihdr;						/*!< MIDIHDR */
	std::deque<char> m_buffer;				/*!< 受信バッファ */
	char m_midiinBuffer[1024];				/*!< バッファ */

	static bool GetId(LPCTSTR lpMidiIn, UINT* pId);
	static CComMidiIn32* GetInstance(HMIDIIN hMidiIn);
	void OnRecvData(UINT nMessage);
	void OnRecvExcv(MIDIHDR* lpMidiHdr);
};
