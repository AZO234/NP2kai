/**
 * @file	cmmidioutvermouth.cpp
 * @brief	MIDI OUT Vermouth クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmmidioutvermouth.h"

#if defined(VERMOUTH_LIB)

//! ハンドル
extern MIDIMOD vermouth_module;

/**
 * インスタンスを作成
 * @return インスタンス
 */
CComMidiOutVermouth* CComMidiOutVermouth::CreateInstance()
{
	MIDIHDL vermouth = midiout_create(vermouth_module, 512);
	if (vermouth == NULL)
	{
		return NULL;
	}
	return new CComMidiOutVermouth(vermouth);
}

/**
 * コンストラクタ
 * @param[in] vermouth ハンドル
 */
CComMidiOutVermouth::CComMidiOutVermouth(MIDIHDL vermouth)
	: m_vermouth(vermouth)
{
	::sound_streamregist(m_vermouth, reinterpret_cast<SOUNDCB>(GetPcm));
}

/**
 * デストラクタ
 */
CComMidiOutVermouth::~CComMidiOutVermouth()
{
	::midiout_destroy(m_vermouth);
}

/**
 * ショート メッセージ
 * @param[in] nMessage メッセージ
 */
void CComMidiOutVermouth::Short(UINT32 nMessage)
{
	sound_sync();
	::midiout_shortmsg(m_vermouth, nMessage);
}

/**
 * ロング メッセージ
 * @param[in] lpMessage メッセージ ポインタ
 * @param[in] cbMessage メッセージ サイズ
 */
void CComMidiOutVermouth::Long(const UINT8* lpMessage, UINT cbMessage)
{
	sound_sync();
	::midiout_longmsg(m_vermouth, lpMessage, cbMessage);
}

/**
 * プロセス
 * @param[in] vermouth ハンドル
 * @param[out] lpBuffer バッファ
 * @param[in] nBufferCount サンプル数
 */
void SOUNDCALL CComMidiOutVermouth::GetPcm(MIDIHDL vermouth, SINT32* lpBuffer, UINT nBufferCount)
{
	while (nBufferCount)
	{
		UINT nSize = nBufferCount;
		const SINT32* ptr = ::midiout_get(vermouth, &nSize);
		if (ptr == NULL)
		{
			break;
		}
		nBufferCount -= nSize;
		do
		{
			lpBuffer[0] += ptr[0];
			lpBuffer[1] += ptr[1];
			ptr += 2;
			lpBuffer += 2;
		} while (--nSize);
	}
}

#endif	// defined(VERMOUTH_LIB)
