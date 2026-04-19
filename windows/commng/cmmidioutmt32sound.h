/**
 * @file	cmmidioutmt32sound.h
 * @brief	MIDI OUT MT32Sound クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#if defined(MT32SOUND_DLL)

#include "cmmidiout.h"
#include "sound.h"

class MT32Sound;

/**
 * @brief MIDI OUT MT32Sound クラス
 */
class CComMidiOutMT32Sound : public CComMidiOut
{
public:
	static CComMidiOutMT32Sound* CreateInstance();

	CComMidiOutMT32Sound(MT32Sound* pMT32Sound);
	virtual ~CComMidiOutMT32Sound();
	virtual void Short(UINT32 nMessage);
	virtual void Long(const UINT8* lpMessage, UINT cbMessage);

private:
	MT32Sound* m_pMT32Sound;	/*!< The instance of mt32sound */
	static void SOUNDCALL GetPcm(MT32Sound* pMT32Sound, SINT32* lpBuffer, UINT nBufferCount);
};

#endif	// defined(MT32SOUND_DLL)
