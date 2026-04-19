/**
 * @file	cmmidiout32.cpp
 * @brief	MIDI OUT win32 クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmmidiout32.h"
#include "cmmidi.h"

#if !defined(__GNUC__)
#pragma comment(lib, "winmm.lib")
#endif	// !defined(__GNUC__)

/**
 * インスタンスを作成
 * @param[in] lpMidiOut デバイス名
 * @return インスタンス
 */
CComMidiOut32* CComMidiOut32::CreateInstance(LPCTSTR lpMidiOut)
{
	UINT nId;
	if (!GetId(lpMidiOut, &nId))
	{
		return NULL;
	}

	HMIDIOUT hMidiOut = NULL;
	if (::midiOutOpen(&hMidiOut, nId, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
	{
		return NULL;
	}
	return new CComMidiOut32(hMidiOut);
}

/**
 * コンストラクタ
 * @param[in] hMidiOut ハンドル
 */
CComMidiOut32::CComMidiOut32(HMIDIOUT hMidiOut)
	: m_hMidiOut(hMidiOut)
	, m_bWaitingSentExclusive(false)
{
	ZeroMemory(&m_midihdr, sizeof(m_midihdr));
	::midiOutReset(m_hMidiOut);
}

/**
 * デストラクタ
 */
CComMidiOut32::~CComMidiOut32()
{
	WaitSentExclusive();
	::midiOutReset(m_hMidiOut);
	::midiOutClose(m_hMidiOut);
}

/**
 * エクスクルーシブ送信完了を待つ
 */
void CComMidiOut32::WaitSentExclusive()
{
	if (m_bWaitingSentExclusive)
	{
		m_bWaitingSentExclusive = false;
		while (midiOutUnprepareHeader(m_hMidiOut, &m_midihdr, sizeof(m_midihdr)) == MIDIERR_STILLPLAYING)
		{
		}
	}
}

/**
 * ショート メッセージ
 * @param[in] nMessage メッセージ
 */
void CComMidiOut32::Short(UINT32 nMessage)
{
	WaitSentExclusive();
	::midiOutShortMsg(m_hMidiOut, nMessage);
}

/**
 * ロング メッセージ
 * @param[in] lpMessage メッセージ ポインタ
 * @param[in] cbMessage メッセージ サイズ
 */
void CComMidiOut32::Long(const UINT8* lpMessage, UINT cbMessage)
{
	if (cbMessage == 0)
	{
		return;
	}

	WaitSentExclusive();

	m_excvbuf.resize(cbMessage);
	CopyMemory(&m_excvbuf[0], lpMessage, cbMessage);

	m_midihdr.lpData = &m_excvbuf[0];
	m_midihdr.dwFlags = 0;
	m_midihdr.dwBufferLength = cbMessage;
	::midiOutPrepareHeader(m_hMidiOut, &m_midihdr, sizeof(m_midihdr));
	::midiOutLongMsg(m_hMidiOut, &m_midihdr, sizeof(m_midihdr));
	m_bWaitingSentExclusive = true;
}

/**
 * ID を得る
 * @param[in] lpMidiOut デバイス名
 * @param[out] pId ID
 * @retval true 成功
 * @retval false 失敗
 */
bool CComMidiOut32::GetId(LPCTSTR lpMidiOut, UINT* pId)
{
	const UINT nNum = ::midiOutGetNumDevs();
	for (UINT i = 0; i < nNum; i++)
	{
		MIDIOUTCAPS moc;
		if (midiOutGetDevCaps(i, &moc, sizeof(moc)) != MMSYSERR_NOERROR)
		{
			continue;
		}
		if (!milstr_cmp(lpMidiOut, moc.szPname))
		{
			*pId = i;
			return true;
		}
	}

	if (!milstr_cmp(lpMidiOut, cmmidi_midimapper))
	{
		*pId = MIDI_MAPPER;
		return true;
	}
	return false;
}
