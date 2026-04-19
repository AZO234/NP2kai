/**
 * @file	cmserial.cpp
 * @brief	シリアル クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmserial.h"

/**
 * インスタンス作成
 * @param[in] nPort ポート番号
 * @param[in] cParam パラメタ
 * @param[in] nSpeed スピード
 * @return インスタンス
 */
CComSerial* CComSerial::CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck)
{
	CComSerial* pSerial = new CComSerial;
	if (!pSerial->Initialize(nPort, cParam, nSpeed, fixedspeed, DSRcheck))
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
	, m_readovl()
	, m_readovl_pending(false)
	, m_readovl_buf(0)
	, m_blocktransfer(false)
	, m_blockbuffer_pos(0)
	, m_blockbuffer_size(0)
	, m_fixedspeed(0)
	, m_lastdata(0)
	, m_lastdatafail(0)
	, m_lastdatatime(0)
	, m_errorstat(0)
{
	HANDLE hEvent;

	// Write OVERLAPPED作成
	for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
		hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		memset(&m_writeovl[i], 0, sizeof(OVERLAPPED));
		m_writeovl[i].hEvent = hEvent;
		m_writeovl_pending[i] = false;
	}

	// Read OVERLAPPED作成
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    memset(&m_readovl, 0, sizeof(OVERLAPPED));
    m_readovl.hEvent = hEvent;
}

/**
 * デストラクタ
 */
CComSerial::~CComSerial()
{

	if (m_hSerial != INVALID_HANDLE_VALUE)
	{
		// Write OVERLAPPED完了待機
		for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
			if(m_writeovl_pending[i]){
				DWORD cbNumberOfBytesTransferred = 0;
				GetOverlappedResult(m_hSerial, &m_writeovl[i], &cbNumberOfBytesTransferred, TRUE);
			}
		}
		// Read OVERLAPPED完了待機
		if(m_readovl_pending){
			DWORD cbNumberOfBytesTransferred = 0;
			GetOverlappedResult(m_hSerial, &m_readovl, &cbNumberOfBytesTransferred, TRUE);
		}

		::CloseHandle(m_hSerial);
		m_hSerial = INVALID_HANDLE_VALUE;
	}

	// Write OVERLAPPED破棄
	for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
		if(m_writeovl[i].hEvent){
			CloseHandle(m_writeovl[i].hEvent);
		}
		memset(&m_writeovl, 0, sizeof(OVERLAPPED));
	}
	
	// Read OVERLAPPED破棄
	if(m_readovl.hEvent){
		CloseHandle(m_readovl.hEvent);
	}
    memset(&m_readovl, 0, sizeof(OVERLAPPED));
}

/**
 * 初期化
 * @param[in] nPort ポート番号
 * @param[in] cParam パラメタ
 * @param[in] nSpeed スピード
 * @retval true 成功
 * @retval false 失敗
 */
bool CComSerial::Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck)
{
	TCHAR szName[16];
	wsprintf(szName, TEXT("COM%u"), nPort);
	m_hSerial = CreateFile(szName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (m_hSerial == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	m_fixedspeed = !!fixedspeed;

	PurgeComm(m_hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

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
	dcb.fDtrControl = DTR_CONTROL_ENABLE; // DTR ON
	dcb.fRtsControl = RTS_CONTROL_ENABLE; // RTS ON
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
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDsrSensitivity = (DSRcheck ? TRUE : FALSE); // TRUEにするとDSRビットが立っていないときの受信データを無視する
	::SetCommState(m_hSerial, &dcb);

	// タイムアウト設定（非同期待ちするので要らない気もする）
	COMMTIMEOUTS tmo;
	tmo.ReadIntervalTimeout = 20;
	tmo.ReadTotalTimeoutConstant = 10;
	tmo.ReadTotalTimeoutMultiplier = 100;
	tmo.WriteTotalTimeoutConstant = 10;
	tmo.WriteTotalTimeoutMultiplier = 100;
	::SetCommTimeouts(m_hSerial, &tmo);

	return true;
}

/**
 * エラー状態を設定 (bit0: パリティ, bit1: オーバーラン, bit2: フレーミング, bit3: ブレーク信号)
 * @param[in] errorcode ClearCommErrorエラーコード
 */
void CComSerial::CheckCommError(DWORD errorcode)
{
	if(errorcode & CE_RXPARITY){
		m_errorstat |= 0x1;
	}
	if(errorcode & CE_RXOVER){
		m_errorstat |= 0x2;
	}
	if(errorcode & CE_FRAME){
		m_errorstat |= 0x4;
	}
	if(errorcode & CE_BREAK){
		m_errorstat |= 0x8;
	}
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
	CheckCommError(err);
	if(m_readovl_pending){
		// 非同期I/O待ちの場合
		DWORD cbNumberOfBytesTransferred = 0;
		if(GetOverlappedResult(m_hSerial, &m_readovl, &cbNumberOfBytesTransferred, FALSE)){
			*pData = m_readovl_buf;
			m_readovl_pending = false;
			return 1;
		}
	}else{
		if (ct.cbInQue)
		{
			DWORD dwReadSize;
			if (::ReadFile(m_hSerial, &m_readovl_buf, 1, &dwReadSize, &m_readovl))
			{
				*pData = m_readovl_buf;
				return 1;
			}else{
				// 非同期I/O待ち開始 バッファにデータが来るまでReadFileしないので基本的にここには来ない
				DWORD lastError = GetLastError();
				if(lastError==ERROR_IO_PENDING){
					m_readovl_pending = true;
				}
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
UINT CComSerial::Write(UINT8 cData)
{
	DWORD dwWrittenSize;
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		m_lastdatafail = 1;
		return 0;
	}
	int emptycount = 0;
	int idx = -1;
	for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
		if(m_writeovl_pending[i]){
			// 非同期I/O待ちの場合
			DWORD cbNumberOfBytesTransferred = 0;
			if(GetOverlappedResult(m_hSerial, &m_writeovl[i], &cbNumberOfBytesTransferred, FALSE)){
				m_writeovl_pending[i] = false;
				// 使用可能番号として割り当て
				idx = i;
				emptycount++;
			}
		}else{
			// 使用可能番号として割り当て
			idx = i;
			emptycount++;
		}
	}
	if(m_blocktransfer){
		// ブロック単位書き込み
		if(m_blockbuffer_pos < m_blockbuffer_size){
			m_blockbuffer[m_blockbuffer_pos] = cData;
			m_blockbuffer_pos++;
		}
		if(m_blockbuffer_pos == m_blockbuffer_size){
			if(idx==-1){
				// 空きがないので書き込み不可
				m_lastdatafail = 1;
				m_lastdata = cData;
				m_lastdatatime = GetTickCount();
				return 0;
			}
			if(::WriteFile(m_hSerial, m_blockbuffer, m_blockbuffer_size, &dwWrittenSize, &m_writeovl[idx])){
				// 書き込めた場合
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
				m_blockbuffer_pos = 0;
				return 1;
			}else{
				// 非同期I/O待ち開始
				DWORD lastError = GetLastError();
				if(lastError==ERROR_IO_PENDING){
					m_writeovl_pending[idx] = true;
				}
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
				m_blockbuffer_pos = 0;
				return 1;
			}
		}
		m_lastdatafail = 0;
		m_lastdata = 0;
		m_lastdatatime = 0;
		return 1;
	}else{
		// 1byte書き込み
		if(idx==-1){
			// 空きがないので書き込み不可
			m_lastdatafail = 1;
			m_lastdata = cData;
			m_lastdatatime = GetTickCount();
			return 0;
		}
		if(::WriteFile(m_hSerial, &cData, 1, &dwWrittenSize, &m_writeovl[idx])){
			// 書き込めた場合
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
			return 1;
		}else{
			// 非同期I/O待ち開始
			DWORD lastError = GetLastError();
			if(lastError==ERROR_IO_PENDING){
				m_writeovl_pending[idx] = true;
			}
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
			return 1;
		}
	}
}

/**
 * 書き込みリトライ
 * @return サイズ
 */
UINT CComSerial::WriteRetry()
{
	DWORD dwWrittenSize;
	int emptycount = 0;
	int idx = -1;
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		m_lastdatafail = 1;
		return 0;
	}
	for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
		if(m_writeovl_pending[i]){
			// 非同期I/O待ちの場合
			DWORD cbNumberOfBytesTransferred = 0;
			if(GetOverlappedResult(m_hSerial, &m_writeovl[i], &cbNumberOfBytesTransferred, FALSE)){
				m_writeovl_pending[i] = false;
				// 使用可能番号として割り当て
				idx = i;
				emptycount++;
			}
		}else{
			// 使用可能番号として割り当て
			idx = i;
			emptycount++;
		}
	}
	if(idx==-1){
		// 空きがないので書き込み不可
		if (GetTickCount() - m_lastdatatime > 3000) {
			// 3秒間バッファデータが減りそうにないならあきらめる
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
			return 1; 
		}
		return 0;
	}
	if(m_blocktransfer){
		// ブロック単位書き込み
		if(m_blockbuffer_pos == m_blockbuffer_size){
			if(::WriteFile(m_hSerial, m_blockbuffer, m_blockbuffer_size, &dwWrittenSize, &m_writeovl[idx])){
				// 書き込めた場合
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
				m_blockbuffer_pos = 0;
				return 1;
			}else{
				// 非同期I/O待ち開始
				DWORD lastError = GetLastError();
				if(lastError==ERROR_IO_PENDING){
					m_writeovl_pending[idx] = true;
				}
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
				m_blockbuffer_pos = 0;
				return 1;
			}
		}else{
			// 基本的にはここには来ないはず
			m_blockbuffer[m_blockbuffer_pos] = m_lastdata;
			m_blockbuffer_pos++;
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
		}
		return 1;
	}else{
		// 1byte書き込み
		if(::WriteFile(m_hSerial, &m_lastdata, 1, &dwWrittenSize, &m_writeovl[idx])){
			// 書き込めた場合
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
			return 1;
		}else{
			// 非同期I/O待ち開始
			DWORD lastError = GetLastError();
			if(lastError==ERROR_IO_PENDING){
				m_writeovl_pending[idx] = true;
			}
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
			return 1;
		}
	}
}

/**
 * ブロック単位転送開始
 */
void CComSerial::BeginBlockTransfer()
{
	if(!m_blocktransfer){
		DCB dcb;
		::GetCommState(m_hSerial, &dcb);
		if(dcb.BaudRate >= 115200){
			m_blockbuffer_size = 64;
		}else if(dcb.BaudRate >= 57600){
			m_blockbuffer_size = 32;
		}else if(dcb.BaudRate >= 19200){
			m_blockbuffer_size = 16;
		}else if(dcb.BaudRate >= 9600){
			m_blockbuffer_size = 8;
		}else if(dcb.BaudRate >= 4800){
			m_blockbuffer_size = 4;
		}else if(dcb.BaudRate >= 2400){
			m_blockbuffer_size = 2;
		}else{
			return; // ブロック転送しない
		}
		m_blockbuffer_pos = 0;
		if(!LastWriteSuccess()){
			m_blockbuffer[0] = m_lastdata;
			m_blockbuffer_pos++;
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
		}
		m_blocktransfer = true;
	}
}
/**
 * ブロック単位転送終了
 */
void CComSerial::EndBlockTransfer()
{
	if(m_blocktransfer){
		DWORD dwWrittenSize;
		if(m_blockbuffer_pos > 0){
			int emptycount = 0;
			int idx = -1;
			for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
				if(m_writeovl_pending[i]){
					// 非同期I/O待ちの場合
					DWORD cbNumberOfBytesTransferred = 0;
					if(GetOverlappedResult(m_hSerial, &m_writeovl[i], &cbNumberOfBytesTransferred, FALSE)){
						m_writeovl_pending[i] = false;
						// 使用可能番号として割り当て
						idx = i;
						emptycount++;
					}
				}else{
					// 使用可能番号として割り当て
					idx = i;
					emptycount++;
				}
			}
			if(idx==-1){
				// 空きがないので0番の書き込み完了を待つ
				DWORD cbNumberOfBytesTransferred = 0;
				GetOverlappedResult(m_hSerial, &m_writeovl[0], &cbNumberOfBytesTransferred, TRUE);
				idx = 0;
			}
			if(::WriteFile(m_hSerial, m_blockbuffer, m_blockbuffer_pos, &dwWrittenSize, &m_writeovl[idx])){
				// 書き込めた場合
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
			}else{
				// 非同期I/O待ち開始
				DWORD lastError = GetLastError();
				if(lastError==ERROR_IO_PENDING){
					m_writeovl_pending[idx] = true;
				}
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
			}
			m_blockbuffer_pos = 0;
		}
		m_blocktransfer = false;
	}
}

/**
 * 最後の書き込みが成功しているかチェック
 * @return サイズ
 */
UINT CComSerial::LastWriteSuccess()
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
 * bit 7: ~CI (RI, RING)
 * bit 6: ~CS (CTS)
 * bit 5: ~CD (DCD, RLSD)
 * bit 4: reserved
 * bit 3: reserved
 * bit 2: reserved
 * bit 1: reserved
 * bit 0: ~DSR (DR)
 * @return ステータス 
 */
UINT8 CComSerial::GetStat()
{
	UINT8 ret = 0;
	DWORD modemStat;
	if (m_hSerial == INVALID_HANDLE_VALUE)
	{
		return 0xf1;
	}
	if(::GetCommModemStatus(m_hSerial, &modemStat)){
		if(!(modemStat & MS_DSR_ON)){
			ret |= 0x01;
		}
		if(!(modemStat & MS_CTS_ON)){
			ret |= 0x40;
		}
		if(!(modemStat & MS_RING_ON)){
			ret |= 0x80;
		}
		if(!(modemStat & MS_RLSD_ON)){
			ret |= 0x20;
		}
		return ret;
	}else{
		DWORD err;
		COMSTAT ct;
		::ClearCommError(m_hSerial, &err, &ct);
		CheckCommError(err);
		if (ct.fDsrHold)
		{
			ret |= 0x01;
		}
		if (ct.fCtsHold)
		{
			ret |= 0x40;
		}
		if (ct.fRlsdHold)
		{
			ret |= 0x20;
		}
		return ret;
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
	if (m_hSerial == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	switch (nMessage)
	{
		case COMMSG_CHANGESPEED: // 通信速度変更
			if(!m_fixedspeed){
				int newspeed = *(reinterpret_cast<int*>(nParam));
				for (UINT i = 0; i < NELEMENTS(cmserial_speed); i++)
				{
					if (cmserial_speed[i] >= newspeed)
					{
						DCB dcb;
						::GetCommState(m_hSerial, &dcb);
						if(cmserial_speed[i] != dcb.BaudRate){
							dcb.BaudRate = cmserial_speed[i];
							::SetCommState(m_hSerial, &dcb);
						}
						break;
					}
				}
			}
			break;
			
		case COMMSG_CHANGEMODE: // 通信モード変更
			if(!m_fixedspeed){
				bool changed = false;
				UINT8 newmode = *(reinterpret_cast<UINT8*>(nParam)); // I/O 32h モードセットのデータ
				BYTE stopbits_value[] = {ONESTOPBIT, ONESTOPBIT, ONE5STOPBITS, TWOSTOPBITS};
				BYTE parity_value[] = {NOPARITY, ODDPARITY, NOPARITY, EVENPARITY};
				BYTE bytesize_value[] = {5, 6, 7, 8};
				DCB dcb;
				::GetCommState(m_hSerial, &dcb);
				if(dcb.StopBits != stopbits_value[(newmode >> 6) & 0x3]){
					dcb.StopBits = stopbits_value[(newmode >> 6) & 0x3];
					changed = true;
				}
				if(dcb.Parity != parity_value[(newmode >> 4) & 0x3]){
					dcb.Parity = parity_value[(newmode >> 4) & 0x3];
					changed = true;
				}
				if(dcb.ByteSize != bytesize_value[(newmode >> 2) & 0x3]){
					dcb.ByteSize = bytesize_value[(newmode >> 2) & 0x3];
					changed = true;
				}
				if(changed){
					::PurgeComm(m_hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
					::SetCommState(m_hSerial, &dcb);
				}
			}
			break;

		case COMMSG_SETCOMMAND: // RTSとDTRフラグのセット
			{
				UINT8 cmd = *(reinterpret_cast<UINT8*>(nParam)); // I/O 32h コマンドセットのデータ
				::PurgeComm(m_hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
				if(cmd & 0x20){ // RTS
					::EscapeCommFunction(m_hSerial, SETRTS);
				}else{
					::EscapeCommFunction(m_hSerial, CLRRTS);
				}
				if(cmd & 0x02){ // DTR
					::EscapeCommFunction(m_hSerial, SETDTR);
				}else{
					::EscapeCommFunction(m_hSerial, CLRDTR);
				}
			}
			break;

		case COMMSG_PURGE: // バッファデータ破棄
			{
				::PurgeComm(m_hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
			}
			break;
			

		case COMMSG_SETFLAG:
			{
				COMFLAG flag = reinterpret_cast<COMFLAG>(nParam);
				if ((flag) && (flag->size == sizeof(_COMFLAG)))
				{
					return 1;
				}
			}
			break;

		case COMMSG_GETFLAG:
			{
				// dummy data
				COMFLAG flag = (COMFLAG)_MALLOC(sizeof(_COMFLAG), "RS232C FLAG");
				if (flag)
				{
					flag->size = sizeof(_COMFLAG);
					flag->sig = COMSIG_COM1;
					flag->ver = 0;
					flag->param = 0;
					return reinterpret_cast<INTPTR>(flag);
				}
			}
			break;

		case COMMSG_GETERROR: // 通信エラー取得
			{
				// エラー状態 (bit0: パリティ, bit1: オーバーラン, bit2: フレーミング, bit3: ブレーク信号)
				UINT8 *errflag = (reinterpret_cast<UINT8*>(nParam)); // I/O 32h コマンドセットのデータ
				if(errflag){
					*errflag = m_errorstat;
				}
			}
			break;

		case COMMSG_CLRERROR: // 通信エラークリア
			{
				m_errorstat = 0;
			}
			break;

		default:
			break;
	}
	return 0;
}

