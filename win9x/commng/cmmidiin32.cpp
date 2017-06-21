/**
 * @file	cmmidiin32.cpp
 * @brief	MIDI IN win32 クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmmidiin32.h"
#include "np2.h"

#if !defined(__GNUC__)
#pragma comment(lib, "winmm.lib")
#endif	// !defined(__GNUC__)

/*!< ハンドル マップ */
std::map<HMIDIIN, CComMidiIn32*> CComMidiIn32::sm_midiinMap;

/**
 * インスタンスを作成
 * @param[in] lpMidiIn デバイス名
 * @return インスタンス
 */
CComMidiIn32* CComMidiIn32::CreateInstance(LPCTSTR lpMidiIn)
{
	UINT nId;
	if (!GetId(lpMidiIn, &nId))
	{
		return NULL;
	}

	HMIDIIN hMidiIn = NULL;
	if (::midiInOpen(&hMidiIn, nId, reinterpret_cast<DWORD_PTR>(g_hWndMain), 0, CALLBACK_WINDOW) != MMSYSERR_NOERROR)
	{
		return NULL;
	}
	return new CComMidiIn32(hMidiIn);
}

/**
 * コンストラクタ
 * @param[in] hMidiIn ハンドル
 */
CComMidiIn32::CComMidiIn32(HMIDIIN hMidiIn)
	: m_hMidiIn(hMidiIn)
{
	::midiInReset(hMidiIn);

	sm_midiinMap[hMidiIn] = this;

	ZeroMemory(&m_midihdr, sizeof(m_midihdr));
	ZeroMemory(m_midiinBuffer, sizeof(m_midiinBuffer));

	m_midihdr.lpData = m_midiinBuffer;
	m_midihdr.dwBufferLength = sizeof(m_midiinBuffer);
	::midiInPrepareHeader(hMidiIn, &m_midihdr, sizeof(m_midihdr));
	::midiInAddBuffer(hMidiIn, &m_midihdr, sizeof(m_midihdr));
	::midiInStart(hMidiIn);
}

/**
 * デストラクタ
 */
CComMidiIn32::~CComMidiIn32()
{
	::midiInStop(m_hMidiIn);
	::midiInUnprepareHeader(m_hMidiIn, &m_midihdr, sizeof(m_midihdr));
	sm_midiinMap.erase(m_hMidiIn);

	::midiInReset(m_hMidiIn);
	::midiInClose(m_hMidiIn);
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComMidiIn32::Read(UINT8* pData)
{
	if (!m_buffer.empty())
	{
		*pData = m_buffer.front();
		m_buffer.pop_front();
		return 1;
	}
	else
	{
		return 0;
	}
}

/**
 * ID を得る
 * @param[in] lpMidiIn デバイス名
 * @param[out] pId ID
 * @retval true 成功
 * @retval false 失敗
 */
bool CComMidiIn32::GetId(LPCTSTR lpMidiIn, UINT* pId)
{
	const UINT nNum = ::midiInGetNumDevs();
	for (UINT i = 0; i < nNum; i++)
	{
		MIDIINCAPS mic;
		if (::midiInGetDevCaps(i, &mic, sizeof(mic)) != MMSYSERR_NOERROR)
		{
			continue;
		}
		if (!milstr_cmp(lpMidiIn, mic.szPname))
		{
			*pId = i;
			return true;
		}
	}
	return false;
}

/**
 * インスタンスを検索
 * @param[in] hMidiIn ハンドル
 * @return インスタンス
 */
CComMidiIn32* CComMidiIn32::GetInstance(HMIDIIN hMidiIn)
{
	std::map<HMIDIIN, CComMidiIn32*>::iterator it = sm_midiinMap.find(hMidiIn);
	if (it != sm_midiinMap.end())
	{
		return it->second;
	}
	return NULL;
}

/**
 * メッセージ受信
 * @param[in] hMidiIn ハンドル
 * @param[in] nMessage メッセージ
 */
void CComMidiIn32::RecvData(HMIDIIN hMidiIn, UINT nMessage)
{
	CComMidiIn32* pMidiIn32 = GetInstance(hMidiIn);
	if (pMidiIn32)
	{
		pMidiIn32->OnRecvData(nMessage);
	}
}

/**
 * メッセージ受信
 * @param[in] nMessage メッセージ
 */
void CComMidiIn32::OnRecvData(UINT nMessage)
{
	switch (nMessage & 0xf0)
	{
		case 0xc0:
		case 0xd0:
			m_buffer.push_back(static_cast<char>(nMessage));
			m_buffer.push_back(static_cast<char>(nMessage >> 8));
			break;

		case 0x80:
		case 0x90:
		case 0xa0:
		case 0xb0:
		case 0xe0:
			m_buffer.push_back(static_cast<char>(nMessage));
			m_buffer.push_back(static_cast<char>(nMessage >> 8));
			m_buffer.push_back(static_cast<char>(nMessage >> 16));
			break;
	}
}

/**
 * メッセージ受信
 * @param[in] hMidiIn ハンドル
 * @param[in] lpMidiHdr メッセージ
 */
void CComMidiIn32::RecvExcv(HMIDIIN hMidiIn, MIDIHDR* lpMidiHdr)
{
	CComMidiIn32* pMidiIn32 = GetInstance(hMidiIn);
	if (pMidiIn32)
	{
		pMidiIn32->OnRecvExcv(lpMidiHdr);
	}
}

/**
 * メッセージ受信
 * @param[in] lpMidiHdr メッセージ
 */
void CComMidiIn32::OnRecvExcv(MIDIHDR* lpMidiHdr)
{
	for (DWORD i = 0; i < lpMidiHdr->dwBytesRecorded; i++)
	{
		m_buffer.push_back(lpMidiHdr->lpData[i]);
	}

	::midiInUnprepareHeader(m_hMidiIn, &m_midihdr, sizeof(m_midihdr));
	::midiInPrepareHeader(m_hMidiIn, &m_midihdr, sizeof(m_midihdr));
	::midiInAddBuffer(m_hMidiIn, &m_midihdr, sizeof(m_midihdr));
}
