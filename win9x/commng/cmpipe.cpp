/**
 * @file	cmpipe.cpp
 * @brief	名前付きパイプ シリアル クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmpipe.h"
#include "codecnv/codecnv.h"

#if defined(SUPPORT_NAMED_PIPE)

/**
 * インスタンス作成
 * @param[in] nPort ポート番号
 * @param[in] pipename パイプの名前
 * @param[in] servername サーバーの名前
 * @return インスタンス
 */
CComPipe* CComPipe::CreateInstance(LPCSTR pipename, LPCSTR servername)
{
	CComPipe* pSerial = new CComPipe;
	if (!pSerial->Initialize(pipename, servername))
	{
		delete pSerial;
		pSerial = NULL;
	}
	return pSerial;
}

/**
 * コンストラクタ
 */
CComPipe::CComPipe()
	: CComBase(COMCONNECT_SERIAL)
	, m_hSerial(INVALID_HANDLE_VALUE)
	, m_isserver(false)
	, m_lastdata(0)
	, m_lastdatafail(0)
{
}

/**
 * デストラクタ
 */
CComPipe::~CComPipe()
{
	if (m_hSerial != INVALID_HANDLE_VALUE)
	{
		if(m_isserver){
			DisconnectNamedPipe(m_hSerial);
		}
		::CloseHandle(m_hSerial);
		m_hSerial = INVALID_HANDLE_VALUE;
	}
}

/**
 * 初期化
 * @param[in] nPort ポート番号
 * @param[in] pipename パイプの名前
 * @param[in] servername サーバーの名前
 * @retval true 成功
 * @retval false 失敗
 */
bool CComPipe::Initialize(LPCSTR pipename, LPCSTR servername)
{
	wchar_t wpipename[MAX_PATH];
	wchar_t wservername[MAX_PATH];
	wchar_t wName[MAX_PATH];
	if(pipename==NULL){
		// No pipe name
		return false;
	}
	codecnv_utf8toucs2((UINT16*)wpipename, MAX_PATH, pipename, -1);
	if(wcslen(wpipename) == 0){
		// No pipe name
		return false;
	}
	codecnv_utf8toucs2((UINT16*)wservername, MAX_PATH, servername!=NULL ? servername : "", -1);
	if(wcslen(wpipename) + wcslen(wservername) > MAX_PATH - 16){
		// too long pipe name
		return false;
	}
	if(wcschr(wpipename, '\\') != NULL || wcschr(wservername, '\\') != NULL){
		// cannot use \ in pipe name
		return false;
	}
	if(wservername && wcslen(wservername)!=0){
		swprintf(wName, MAX_PATH, L"\\\\%ls\\pipe\\%ls", wservername, wpipename);
	}else{
		swprintf(wName, MAX_PATH, L"\\\\.\\pipe\\%ls", wpipename);
	}

	if(m_hSerial != INVALID_HANDLE_VALUE){
		if(m_isserver){
			DisconnectNamedPipe(m_hSerial);
		}
		::CloseHandle(m_hSerial);
		m_hSerial = INVALID_HANDLE_VALUE;
	}

	// クライアント接続をトライ
	m_hSerial = CreateFileW(wName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		// サーバーで再トライ
		m_hSerial = CreateNamedPipeW(wName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE|PIPE_NOWAIT, 1, 1024, 1024, 500, NULL);
		if (m_hSerial == INVALID_HANDLE_VALUE) {
			return false;
		}
		m_isserver = true;
		//ConnectNamedPipe(m_hSerial, NULL); // With a nonblocking-wait handle, the connect operation returns zero immediately, and the GetLastError function returns ERROR_PIPE_LISTENING. とのことなので常時0
	}

	_tcscpy(m_pipename, pipename);
	_tcscpy(m_pipeserv, servername);

	return true;
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComPipe::Read(UINT8* pData)
{
	DWORD dwReadSize;
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		return 0;
	}
	if (PeekNamedPipe(m_hSerial, NULL, 0, NULL, &dwReadSize, NULL)){
		if(dwReadSize > 0){
			if (::ReadFile(m_hSerial, pData, 1, &dwReadSize, NULL))
			{
				return 1;
			}
		}
	}else{
		DWORD err = GetLastError();
		if(m_isserver){
			if(err==ERROR_BROKEN_PIPE){
				DisconnectNamedPipe(m_hSerial);
				ConnectNamedPipe(m_hSerial, NULL);
			}
		}else{
			if(err==ERROR_PIPE_NOT_CONNECTED){
				Initialize(m_pipename, m_pipeserv);
			}
		}
	}
	return 0;
}

/**
 * 書き込み
 * @param[out] cData データ
 * @return サイズ
 */
UINT CComPipe::Write(UINT8 cData)
{
	UINT ret;
	DWORD dwWrittenSize;
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		m_lastdatafail = 1;
		return 0;
	}
	ret = (::WriteFile(m_hSerial, &cData, 1, &dwWrittenSize, NULL)) ? 1 : 0;
	if(dwWrittenSize==0) {
		if(m_lastdatafail && GetTickCount() - m_lastdatatime > 1000){
			return 1; // バッファデータが減りそうにないならあきらめる（毎秒1byte(8bit)は流石にあり得ない）
		}
		m_lastdatafail = 1;
		m_lastdata = cData;
		m_lastdatatime = GetTickCount();
		return 0;
	}else{
		m_lastdatafail = 0;
		m_lastdata = 0;
		m_lastdatatime = 0;
	}
	return ret;
}

/**
 * 書き込みリトライ
 * @return サイズ
 */
UINT CComPipe::WriteRetry()
{
	UINT ret;
	DWORD dwWrittenSize;
	if(m_lastdatafail){
		if (GetTickCount() - m_lastdatatime > 1000) return 1; // バッファデータが減りそうにないならあきらめる（毎秒1byte(8bit)は流石にあり得ない）
		if (m_hSerial == INVALID_HANDLE_VALUE) {
			return 0;
		}
		ret = (::WriteFile(m_hSerial, &m_lastdata, 1, &dwWrittenSize, NULL)) ? 1 : 0;
		if(dwWrittenSize==0) {
			return 0;
		}
		m_lastdatafail = 0;
		m_lastdata = 0;
		m_lastdatatime = 0;
		return ret;
	}
	return 1;
}

/**
 * 最後の書き込みが成功しているかチェック
 * @return サイズ
 */
UINT CComPipe::LastWriteSuccess()
{
	if(m_lastdatafail && GetTickCount() - m_lastdatatime > 3000){
		return 1; // 3秒間バッファデータが減りそうにないならあきらめる
	}
	if(m_lastdatafail){
		return 0;
	}
	return 1;
}

/**
 * ステータスを得る
 * @return ステータス
 */
UINT8 CComPipe::GetStat()
{
	return 0x20;
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComPipe::Message(UINT nMessage, INTPTR nParam)
{
	//switch (nMessage)
	//{
	//	default:
	//		break;
	//}
	return 0;
}

#endif
