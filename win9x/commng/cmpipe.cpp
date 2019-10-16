/**
 * @file	cmpipe.cpp
 * @brief	名前付きパイプ シリアル クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmpipe.h"

#if defined(SUPPORT_NAMED_PIPE)

/**
 * インスタンス作成
 * @param[in] nPort ポート番号
 * @param[in] pipename パイプの名前
 * @param[in] servername サーバーの名前
 * @return インスタンス
 */
CComPipe* CComPipe::CreateInstance(LPCTSTR pipename, LPCTSTR servername)
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
bool CComPipe::Initialize(LPCTSTR pipename, LPCTSTR servername)
{
	TCHAR szName[MAX_PATH];
	if(pipename==NULL || _tcslen(pipename) == 0){
		// No pipe name
		return false;
	}
	if(_tcslen(pipename) + (servername!=NULL ? _tcslen(servername) : 0) > MAX_PATH - 16){
		// too long pipe name
		return false;
	}
	if(_tcschr(pipename, '\\') != NULL || (servername!=NULL && _tcschr(servername, '\\') != NULL)){
		// cannot use \ in pipe name
		return false;
	}
	if(servername && _tcslen(servername)!=0){
		wsprintf(szName, TEXT("\\\\%s\\pipe\\%s"), servername, pipename);
	}else{
		wsprintf(szName, TEXT("\\\\.\\pipe\\%s"), pipename);
	}

	if(m_hSerial != INVALID_HANDLE_VALUE){
		if(m_isserver){
			DisconnectNamedPipe(m_hSerial);
		}
		::CloseHandle(m_hSerial);
		m_hSerial = INVALID_HANDLE_VALUE;
	}

	// クライアント接続をトライ
	m_hSerial = CreateFile(szName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		// サーバーで再トライ
		m_hSerial = CreateNamedPipe(szName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE|PIPE_NOWAIT, 1, 1024, 1024, 500, NULL);
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
	LPTSTR lpMsgBuf;
	UINT ret;
	DWORD dwWrittenSize;
	DWORD errornum;
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		return 0;
	}
	ret = (::WriteFile(m_hSerial, &cData, 1, &dwWrittenSize, NULL)) ? 1 : 0;
	if(dwWrittenSize==0) {
		return 0;
	}
	return ret;
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
	switch (nMessage)
	{
		default:
			break;
	}
	return 0;
}

#endif
