/**
 * @file	cmpara.cpp
 * @brief	パラレル クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmpara.h"

/**
 * インスタンス作成
 * @param[in] nPort ポート番号
 * @return インスタンス
 */
CComPara* CComPara::CreateInstance(UINT nPort)
{
	CComPara* pPara = new CComPara;
	if (!pPara->Initialize(nPort))
	{
		delete pPara;
		pPara = NULL;
	}
	return pPara;
}

/**
 * コンストラクタ
 */
CComPara::CComPara()
	: CComBase(COMCONNECT_PARALLEL)
	, m_hParallel(INVALID_HANDLE_VALUE)
{
}

/**
 * デストラクタ
 */
CComPara::~CComPara()
{
	if (m_hParallel != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hParallel);
		m_hParallel = INVALID_HANDLE_VALUE;
	}
}

/**
 * 初期化
 * @param[in] nPort ポート番号
 * @retval true 成功
 * @retval false 失敗
 */
bool CComPara::Initialize(UINT nPort)
{
	wchar_t wName[16];
	swprintf(wName, 16, L"LPT%u", nPort);
	m_hParallel = CreateFileW(wName, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
	return (m_hParallel != INVALID_HANDLE_VALUE);
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComPara::Read(UINT8* pData)
{
	return 0;
}

/**
 * 書き込み
 * @param[out] cData データ
 * @return サイズ
 */
UINT CComPara::Write(UINT8 cData)
{
	DWORD dwWrittenSize;
	return (::WriteFile(m_hParallel, &cData, 1, &dwWrittenSize, NULL)) ? 1 : 0;
}

/**
 * ステータスを得る
 * @return ステータス
 */
UINT8 CComPara::GetStat()
{
	return 0x00;
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComPara::Message(UINT nMessage, INTPTR nParam)
{
	return 0;
}
