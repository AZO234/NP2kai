/**
 * @file	cmmidioutvst.h
 * @brief	MIDI OUT VST クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#if defined(SUPPORT_VSTi)

#include "cmmidiout.h"
#include "sound.h"
#include "vsthost\vstbuffer.h"
#include "vsthost\vsteditwnd.h"
#include "vsthost\vsteffect.h"
#include "vsthost\vstmidievent.h"

/**
 * @brief MIDI OUT VST クラス
 */
class CComMidiOutVst : public CComMidiOut
{
public:
	static bool IsEnabled();
	static CComMidiOutVst* CreateInstance();

	CComMidiOutVst();
	virtual ~CComMidiOutVst();
	virtual void Short(UINT32 nMessage);
	virtual void Long(const UINT8* lpMessage, UINT cbMessage);

private:
	UINT m_nBlockSize;			/*!< ブロック サイズ */
	UINT m_nIndex;				/*!< 読み取りインデックス */
	CVstEffect m_effect;		/*!< エフェクト */
	CVstEditWnd m_wnd;			/*!< ウィンドウ */
	CVstMidiEvent m_event;		/*!< イベント */
	CVstBuffer m_input;			/*!< 入力バッファ */
	CVstBuffer m_output;		/*!< 出力バッファ */

	bool Initialize(LPCTSTR lpPath);
	static void SOUNDCALL GetPcm(CComMidiOutVst*, SINT32* lpBuffer, UINT nBufferCount);
	void Process32(SINT32* lpBuffer, UINT nBufferCount);
};

#endif	// defined(SUPPORT_VSTi)
