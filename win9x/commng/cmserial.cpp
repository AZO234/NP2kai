/**
 * @file	cmserial.cpp
 * @brief	シリアル クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmserial.h"

/**
 * 速度テーブル
 */
const UINT32 cmserial_speed[10] = {110, 300, 1200, 2400, 4800,
							9600, 19200, 38400, 57600, 115200};

/**
 * インスタンス作成
 * @param[in] nPort ポート番号
 * @param[in] cParam パラメタ
 * @param[in] nSpeed スピード
 * @return インスタンス
 */
CComSerial* CComSerial::CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed)
{
	CComSerial* pSerial = new CComSerial;
	if (!pSerial->Initialize(nPort, cParam, nSpeed))
	{
		delete pSerial;
		pSerial = NULL;
	}
	return pSerial;
}

/**
 * コンストラクタ
 */
CComSerial::CComSerial()
	: CComBase(COMCONNECT_SERIAL)
	, m_hSerial(INVALID_HANDLE_VALUE)
{
}

/**
 * デストラクタ
 */
CComSerial::~CComSerial()
{
	if (m_hSerial != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hSerial);
	}
}

/**
 * 初期化
 * @param[in] nPort ポート番号
 * @param[in] cParam パラメタ
 * @param[in] nSpeed スピード
 * @retval true 成功
 * @retval false 失敗
 */
bool CComSerial::Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed)
{
	TCHAR szName[16];
	wsprintf(szName, TEXT("COM%u"), nPort);
	m_hSerial = CreateFile(szName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
	if (m_hSerial == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DCB dcb;
	::GetCommState(m_hSerial, &dcb);
	for (UINT i = 0; i < NELEMENTS(cmserial_speed); i++)
	{
		if (cmserial_speed[i] >= nSpeed)
		{
			dcb.BaudRate = cmserial_speed[i];
			break;
		}
	}
	dcb.ByteSize = (UINT8)(((cParam >> 2) & 3) + 5);
	switch (cParam & 0x30)
	{
		case 0x10:
			dcb.Parity = ODDPARITY;
			break;

		case 0x30:
			dcb.Parity = EVENPARITY;
			break;

		default:
			dcb.Parity = NOPARITY;
			break;
	}
	switch (cParam & 0xc0)
	{
		case 0x80:
			dcb.StopBits = ONE5STOPBITS;
			break;

		case 0xc0:
			dcb.StopBits = TWOSTOPBITS;
			break;

		default:
			dcb.StopBits = ONESTOPBIT;
			break;
	}
	::SetCommState(m_hSerial, &dcb);
	return true;
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComSerial::Read(UINT8* pData)
{
	DWORD err;
	COMSTAT ct;
	::ClearCommError(m_hSerial, &err, &ct);
	if (ct.cbInQue)
	{
		DWORD dwReadSize;
		if (::ReadFile(m_hSerial, pData, 1, &dwReadSize, NULL))
		{
			return 1;
		}
	}
	return 0;
}

/**
 * 書き込み
 * @param[out] cData データ
 * @return サイズ
 */
UINT CComSerial::Write(UINT8 cData)
{
	DWORD dwWrittenSize;
	return (::WriteFile(m_hSerial, &cData, 1, &dwWrittenSize, NULL)) ? 1 : 0;
}

/**
 * ステータスを得る
 * @return ステータス
 */
UINT8 CComSerial::GetStat()
{
	DCB dcb;
	::GetCommState(m_hSerial, &dcb);
	if (!dcb.fDsrSensitivity)
	{
		return 0x20;
	}
	else
	{
		return 0x00;
	}
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComSerial::Message(UINT nMessage, INTPTR nParam)
{
	return 0;
}
