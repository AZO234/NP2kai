/**
 * @file	cmmidiout32.h
 * @brief	MIDI OUT win32 クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include "cmmidiout.h"

/**
 * @brief MIDI OUT win32 クラス
 */
class CComMidiOut32 : public CComMidiOut
{
public:
	static CComMidiOut32* CreateInstance(LPCTSTR lpMidiOut);

	CComMidiOut32(HMIDIOUT hMidiOut);
	virtual ~CComMidiOut32();
	virtual void Short(UINT32 nMessage);
	virtual void Long(const UINT8* lpMessage, UINT cbMessage);
	static bool GetId(LPCTSTR lpMidiOut, UINT* pId);

private:
	HMIDIOUT m_hMidiOut;					/*!< MIDIOUT ハンドル */
	MIDIHDR m_midihdr;						/*!< MIDIHDR */
	bool m_bWaitingSentExclusive;			/*!< エクスクルーシヴ送信中 */
	std::vector<char> m_excvbuf;			/*!< エクスクルーシヴ バッファ */

	void WaitSentExclusive();
};
