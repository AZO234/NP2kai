/**
 * @file	vstmidievent.h
 * @brief	VST MIDI クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include <pluginterfaces/vst2.x/aeffectx.h>

/**
 * @brief VST MIDI クラス
 */
class CVstMidiEvent
{
public:
	CVstMidiEvent();
	~CVstMidiEvent();
	void Clear();
	void ShortMessage(UINT nTick, UINT nMessage);
	void LongMessage(UINT nTick, const void* lpMessage, UINT cbMessage);
	const VstEvents* GetEvents();

protected:

private:
	UINT m_nEvents;							/*!< イベント数 */
	std::vector<unsigned char> m_header;	/*!< ヘッダ */
	std::vector<unsigned char> m_event;		/*!< イベント */
	void Add(const VstEvent* pEvent, const void* lpMessage = NULL, UINT cbMessage = 0);
};
