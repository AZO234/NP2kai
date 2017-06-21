/**
 * @file	vstmidievent.cpp
 * @brief	VST MIDI クラスの動作の定義を行います
 */

#include "compiler.h"
#include "vstmidievent.h"

/**
 * コンストラクタ
 */
CVstMidiEvent::CVstMidiEvent()
	: m_nEvents(0)
{
}

/**
 * デストラクタ
 */
CVstMidiEvent::~CVstMidiEvent()
{
}

/**
 * クリア
 */
void CVstMidiEvent::Clear()
{
	m_nEvents = 0;
	m_header.clear();
	m_event.clear();
}

/**
 * ショート メッセージ
 * @param[in] nTick ティック
 * @param[in] nMessage メッセージ
 */
void CVstMidiEvent::ShortMessage(UINT nTick, UINT nMessage)
{
	VstMidiEvent event;
	memset(&event, 0, sizeof(event));
	event.type = kVstMidiType;
	event.byteSize = sizeof(event);
	event.deltaFrames = nTick;
	event.flags = 1;
	event.midiData[0] = static_cast<char>(nMessage >> 0);
	event.midiData[1] = static_cast<char>(nMessage >> 8);
	event.midiData[2] = static_cast<char>(nMessage >> 16);
	Add(reinterpret_cast<VstEvent*>(&event));
}

/**
 * ロング メッセージ
 * @param[in] nTick ティック
 * @param[in] lpMessage メッセージ
 * @param[in] cbMessage メッセージ サイズ
 */
void CVstMidiEvent::LongMessage(UINT nTick, const void* lpMessage, UINT cbMessage)
{
	VstMidiSysexEvent event;
	memset(&event, 0, sizeof(event));
	event.type = kVstSysExType;
	event.byteSize = sizeof(event);
	event.deltaFrames = nTick;
	event.flags = 1;
	event.dumpBytes = cbMessage;
	Add(reinterpret_cast<VstEvent*>(&event), lpMessage, cbMessage);
}

/**
 * イベント追加
 * @param[in] pEvent イベント
 * @param[in] lpMessage 追加メッセージ
 * @param[in] cbMessage 追加メッセージ サイズ
 */
void CVstMidiEvent::Add(const VstEvent* pEvent, const void* lpMessage, UINT cbMessage)
{
	m_event.insert(m_event.end(), reinterpret_cast<const unsigned char*>(pEvent), reinterpret_cast<const unsigned char*>(pEvent) + pEvent->byteSize);
	if ((lpMessage) && (cbMessage))
	{
		m_event.insert(m_event.end(), static_cast<const unsigned char*>(lpMessage), static_cast<const unsigned char*>(lpMessage) + cbMessage);
		// needs padding?
	}
	m_nEvents++;
}

/**
 * イベントの作成
 * @return イベント
 */
const VstEvents* CVstMidiEvent::GetEvents()
{
	m_header.resize(offsetof(VstEvents, events[m_nEvents]));

	VstEvents* pEvents = reinterpret_cast<VstEvents*>(&m_header.at(0));
	pEvents->numEvents = m_nEvents;
	pEvents->reserved = 0;

	UINT nOffset = 0;
	for (UINT i = 0; i < m_nEvents; i++)
	{
		VstEvent* pEvent = reinterpret_cast<VstEvent*>(&m_event.at(nOffset));
		pEvents->events[i] = pEvent;
		nOffset += pEvent->byteSize;

		if (pEvent->type == kVstSysExType)
		{
			VstMidiSysexEvent *pSysexEvent = reinterpret_cast<VstMidiSysexEvent*>(pEvent);
			pSysexEvent->sysexDump = reinterpret_cast<char*>(pSysexEvent + 1);
			nOffset += pSysexEvent->dumpBytes;
		}
	}
	return pEvents;
}
