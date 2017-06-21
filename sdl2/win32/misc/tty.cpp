/**
 * @file	ttyl.cpp
 * @brief	シリアル通信クラスの動作の定義を行います
 */

#include "compiler.h"
#include "tty.h"
#include <algorithm>
#include <setupapi.h>
#include <tchar.h>

#pragma comment(lib, "setupapi.lib")

/**
 * コンストラクタ
 */
CTty::CTty()
	: m_hFile(INVALID_HANDLE_VALUE)
{
}

/**
 * デストラクタ
 */
CTty::~CTty()
{
	Close();
}

/**
 * オープンする
 * @param[in] lpDevName デバイス名
 * @param[in] nSpeed ボーレート
 * @param[in] lpcszParam パラメタ
 * @retval true 成功
 * @retval false 失敗
 */
bool CTty::Open(LPCTSTR lpDevName, UINT nSpeed, LPCTSTR lpcszParam)
{
	Close();

	if (!SetParam(lpcszParam, NULL))
	{
		return false;
	}

	HANDLE hFile = ::CreateFile(lpDevName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DCB dcb;
	::GetCommState(hFile, &dcb);
	if (nSpeed != 0)
	{
		dcb.BaudRate = nSpeed;
	}
	SetParam(lpcszParam, &dcb);

	dcb.fOutxCtsFlow = FALSE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	if (!::SetCommState(hFile, &dcb))
	{
		::CloseHandle(hFile);
		return false;
	}

	m_hFile = hFile;
	return true;
}

/**
 * クローズする
 */
void CTty::Close()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

/**
 * データ受信
 * @param[in] lpcvData 送信データのポインタ
 * @param[in] nDataSize 送信データのサイズ
 * @return 送信バイト数
 */
ssize_t CTty::Read(LPVOID lpcvData, ssize_t nDataSize)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	if ((lpcvData == NULL) || (nDataSize <= 0))
	{
		return 0;
	}

	DWORD dwErrors;
	COMSTAT stat;
	if (!::ClearCommError(m_hFile, &dwErrors, &stat))
	{
		return -1;
	}

	DWORD dwReadLength = (std::min)(stat.cbInQue, static_cast<DWORD>(nDataSize));
	if (dwReadLength == 0)
	{
		return 0;
	}

	DWORD dwReadSize = 0;
	if (!::ReadFile(m_hFile, lpcvData, dwReadLength, &dwReadSize, NULL))
	{
		return -1;
	}
	return static_cast<ssize_t>(dwReadSize);
}

/**
 * データ送信
 * @param[in] lpcvData 送信データのポインタ
 * @param[in] nDataSize 送信データのサイズ
 * @return 送信バイト数
 */
ssize_t CTty::Write(LPCVOID lpcvData, ssize_t nDataSize)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	if ((lpcvData == NULL) || (nDataSize <= 0))
	{
		return 0;
	}

	DWORD dwWrittenSize = 0;
	if (!::WriteFile(m_hFile, lpcvData, nDataSize, &dwWrittenSize, NULL))
	{
		// DEBUGLOG(_T("Failed to write."));
		return -1;
	}
	return static_cast<ssize_t>(dwWrittenSize);
}

/**
 * パラメータ設定
 * @param[in] lpcszParam パラメタ
 * @param[in, out] dcb DCB 構造体のポインタ
 * @retval true 成功
 * @retval false 失敗
 */
bool CTty::SetParam(LPCTSTR lpcszParam, DCB* dcb)
{
	BYTE cByteSize = 8;
	BYTE cParity = NOPARITY;
	BYTE cStopBits = ONESTOPBIT;

	if (lpcszParam != NULL)
	{
		TCHAR c = lpcszParam[0];
		if ((c < TEXT('4')) || (c > TEXT('8')))
		{
			return false;
		}
		cByteSize = static_cast<BYTE>(c - TEXT('0'));

		c = lpcszParam[1];
		switch (c & (~0x20))
		{
			case TEXT('N'):		// for no parity
				cParity = NOPARITY;
				break;

			case TEXT('E'):		// for even parity
				cParity = EVENPARITY;
				break;

			case TEXT('O'):		// for odd parity
				cParity = ODDPARITY;
				break;

			case TEXT('M'):		// for mark parity
				cParity = MARKPARITY;
				break;

			case TEXT('S'):		// for for space parity
				cParity = SPACEPARITY;
				break;

			default:
				return false;
		}

		if (::lstrcmp(lpcszParam + 2, TEXT("1")) == 0)
		{
			cStopBits = ONESTOPBIT;
		}
		else if (::lstrcmp(lpcszParam + 2, TEXT("1.5")) == 0)
		{
			cStopBits = ONE5STOPBITS;
		}
		else if (::lstrcmp(lpcszParam + 2, TEXT("2")) == 0)
		{
			cStopBits = TWOSTOPBITS;
		}
		else
		{
			return false;
		}
	}

	if (dcb != NULL)
	{
		dcb->ByteSize = cByteSize;
		dcb->Parity = cParity;
		dcb->StopBits = cStopBits;
	}
	return true;
}
