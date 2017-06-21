/**
 * @file	cmmidioutvst.cpp
 * @brief	MIDI OUT VST クラスの動作の定義を行います
 */

#include "compiler.h"

#if defined(SUPPORT_VSTi)

#include "cmmidioutvst.h"
#include <shlwapi.h>
#include "np2.h"
#include "sound/sound.h"

#pragma comment(lib, "shlwapi.lib")

/**
 * VSTi モジュール ファイル名を得る
 * @param[out] lpModule VSTi モジュール ファイル名
 * @param[in] cchModule VSTi モジュール ファイル名のバッファの長さ
 */
static void GetPath(LPTSTR lpModule, UINT cchModule)
{
	::ExpandEnvironmentStrings(np2oscfg.szVSTiFile, lpModule, cchModule);
}

/**
 * VSTi は有効か?
 * @retval true 有効
 * @retval false 無効
 */
bool CComMidiOutVst::IsEnabled()
{
	TCHAR szModule[MAX_PATH];
	GetPath(szModule, _countof(szModule));
	return (::PathFileExists(szModule) != FALSE);
}

/**
 * インスタンスを作成
 * @return インスタンス
 */
CComMidiOutVst* CComMidiOutVst::CreateInstance()
{
	CComMidiOutVst* pVst = new CComMidiOutVst;

	TCHAR szModule[MAX_PATH];
	GetPath(szModule, _countof(szModule));
	if (!pVst->Initialize(szModule))
	{
		delete pVst;
		pVst = NULL;
	}
	return pVst;
}

/**
 * コンストラクタ
 */
CComMidiOutVst::CComMidiOutVst()
	: m_nBlockSize(128)
	, m_nIndex(0)
{
}

/**
 * デストラクタ
 */
CComMidiOutVst::~CComMidiOutVst()
{
	m_wnd.Destroy();
	m_effect.Unload();
}

/**
 * 初期化
 * @param[in] lpPath パス
 * @retval true 成功
 * @retval false 失敗
 */
bool CComMidiOutVst::Initialize(LPCTSTR lpPath)
{
	if (soundcfg.rate == 0)
	{
		return false;
	}

	if (!m_effect.Load(lpPath))
	{
		printf("Cloudn't attach VSTi.\n");
		return false;
	}

	// Effect をオープン
	m_effect.open();

	// サンプリング レートを設定
	m_effect.setSampleRate(static_cast<float>(soundcfg.rate));

	// ブロックサイズを設定
	m_effect.setBlockSize(m_nBlockSize);
	m_effect.resume();

	m_effect.beginSetProgram();
	m_effect.setProgram(0);
	m_effect.endSetProgram();

	m_input.Alloc(2, m_nBlockSize);
	m_output.Alloc(2, m_nBlockSize);

	::sound_streamregist(this, reinterpret_cast<SOUNDCB>(GetPcm));

	m_wnd.Create(&m_effect, TEXT("NP2 VSTi"), WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX);

	return true;
}

/**
 * ショート メッセージ
 * @param[in] nMessage メッセージ
 */
void CComMidiOutVst::Short(UINT32 nMessage)
{
	sound_sync();
	m_event.ShortMessage(m_nIndex, nMessage);
}

/**
 * ロング メッセージ
 * @param[in] lpMessage メッセージ ポインタ
 * @param[in] cbMessage メッセージ サイズ
 */
void CComMidiOutVst::Long(const UINT8* lpMessage, UINT cbMessage)
{
	sound_sync();
	m_event.LongMessage(m_nIndex, lpMessage, cbMessage);
}

/**
 * プロセス
 * @param[in] pVst ハンドル
 * @param[out] lpBuffer バッファ
 * @param[in] nBufferCount サンプル数
 */
void SOUNDCALL CComMidiOutVst::GetPcm(CComMidiOutVst* pVst, SINT32* lpBuffer, UINT nBufferCount)
{
	pVst->Process32(lpBuffer, nBufferCount);
}

/**
 * プロセス (32bit)
 * @param[out] lpBuffer バッファ
 * @param[in] nBufferCount サンプル数
 */
void CComMidiOutVst::Process32(SINT32* lpBuffer, UINT nBufferCount)
{
	while (nBufferCount)
	{
		if (m_nIndex >= m_nBlockSize)
		{
			m_nIndex = 0;
			m_effect.processEvents(m_event.GetEvents());
			m_effect.processReplacing(m_input.GetBuffer(), m_output.GetBuffer(), m_nBlockSize);
			m_event.Clear();
		}

		UINT nSize = m_nBlockSize - m_nIndex;
		nSize = min(nSize, nBufferCount);
		nBufferCount -= nSize;
		float** output = m_output.GetBuffer();
		do
		{
			lpBuffer[0] += static_cast<SINT32>(output[0][m_nIndex] * 32767.0f - 0.5f);
			lpBuffer[1] += static_cast<SINT32>(output[1][m_nIndex] * 32767.0f - 0.5f);
			lpBuffer += 2;
			m_nIndex++;
		} while (--nSize);
	}
}

#endif	// defined(SUPPORT_VSTi)
