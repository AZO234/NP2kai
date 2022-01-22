/**
 * @file	cmserial.cpp
 * @brief	�V���A�� �N���X�̓���̒�`���s���܂�
 */

#include <compiler.h>
#include "cmserial.h"

/**
 * �C���X�^���X�쐬
 * @param[in] nPort �|�[�g�ԍ�
 * @param[in] cParam �p�����^
 * @param[in] nSpeed �X�s�[�h
 * @return �C���X�^���X
 */
CComSerial* CComSerial::CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck)
{
	CComSerial* pSerial = new CComSerial;
	if (!pSerial->Initialize(nPort, cParam, nSpeed, fixedspeed, DSRcheck))
	if (!pSerial->Initialize(nPort, cParam, nSpeed, fixedspeed))
	{
		delete pSerial;
		pSerial = NULL;
	}
	return pSerial;
}

/**
 * �R���X�g���N�^
 */
CComSerial::CComSerial()
	: CComBase(COMCONNECT_SERIAL)
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

	// Write OVERLAPPED�쐬
	for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
		hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		memset(&m_writeovl[i], 0, sizeof(OVERLAPPED));
		m_writeovl[i].hEvent = hEvent;
		m_writeovl_pending[i] = false;
	}

	// Read OVERLAPPED�쐬
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    memset(&m_readovl, 0, sizeof(OVERLAPPED));
    m_readovl.hEvent = hEvent;
}

/**
 * �f�X�g���N�^
 */
CComSerial::~CComSerial()
{

	if (m_hSerial != INVALID_HANDLE_VALUE)
	{
		// Write OVERLAPPED�����ҋ@
		for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
			if(m_writeovl_pending[i]){
				DWORD cbNumberOfBytesTransferred = 0;
				GetOverlappedResult(m_hSerial, &m_writeovl[i], &cbNumberOfBytesTransferred, TRUE);
			}
		}
		// Read OVERLAPPED�����ҋ@
		if(m_readovl_pending){
			DWORD cbNumberOfBytesTransferred = 0;
			GetOverlappedResult(m_hSerial, &m_readovl, &cbNumberOfBytesTransferred, TRUE);
		}

		::CloseHandle(m_hSerial);
		m_hSerial = INVALID_HANDLE_VALUE;
	}

	// Write OVERLAPPED�j��
	for(int i=0;i<SERIAL_OVERLAP_COUNT;i++){
		if(m_writeovl[i].hEvent){
			CloseHandle(m_writeovl[i].hEvent);
		}
		memset(&m_writeovl, 0, sizeof(OVERLAPPED));
	}
	
	// Read OVERLAPPED�j��
	if(m_readovl.hEvent){
		CloseHandle(m_readovl.hEvent);
	}
    memset(&m_readovl, 0, sizeof(OVERLAPPED));
}

/**
 * ������
 * @param[in] nPort �|�[�g�ԍ�
 * @param[in] cParam �p�����^
 * @param[in] nSpeed �X�s�[�h
 * @retval true ����
 * @retval false ���s
 */
bool CComSerial::Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck)
{
	wchar_t wName[16];
	swprintf(wName, 16, L"COM%u", nPort);
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
	dcb.fDsrSensitivity = (DSRcheck ? TRUE : FALSE); // TRUE�ɂ����DSR�r�b�g�������Ă��Ȃ��Ƃ��̎�M�f�[�^�𖳎�����
	::SetCommState(m_hSerial, &dcb);

	// �^�C���A�E�g�ݒ�i�񓯊��҂�����̂ŗv��Ȃ��C������j
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
 * �G���[��Ԃ�ݒ� (bit0: �p���e�B, bit1: �I�[�o�[����, bit2: �t���[�~���O, bit3: �u���[�N�M��)
 * @param[in] errorcode ClearCommError�G���[�R�[�h
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
 * �ǂݍ���
 * @param[out] pData �o�b�t�@
 * @return �T�C�Y
 */
UINT CComSerial::Read(UINT8* pData)
{
	DWORD err;
	COMSTAT ct;
	::ClearCommError(m_hSerial, &err, &ct);
	CheckCommError(err);
	if(m_readovl_pending){
		// �񓯊�I/O�҂��̏ꍇ
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
				// �񓯊�I/O�҂��J�n �o�b�t�@�Ƀf�[�^������܂�ReadFile���Ȃ��̂Ŋ�{�I�ɂ����ɂ͗��Ȃ�
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
 * ��������
 * @param[out] cData �f�[�^
 * @return �T�C�Y
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
			// �񓯊�I/O�҂��̏ꍇ
			DWORD cbNumberOfBytesTransferred = 0;
			if(GetOverlappedResult(m_hSerial, &m_writeovl[i], &cbNumberOfBytesTransferred, FALSE)){
				m_writeovl_pending[i] = false;
				// �g�p�\�ԍ��Ƃ��Ċ��蓖��
				idx = i;
				emptycount++;
			}
		}else{
			// �g�p�\�ԍ��Ƃ��Ċ��蓖��
			idx = i;
			emptycount++;
		}
	}
	if(m_blocktransfer){
		// �u���b�N�P�ʏ�������
		if(m_blockbuffer_pos < m_blockbuffer_size){
			m_blockbuffer[m_blockbuffer_pos] = cData;
			m_blockbuffer_pos++;
		}
		if(m_blockbuffer_pos == m_blockbuffer_size){
			if(idx==-1){
				// �󂫂��Ȃ��̂ŏ������ݕs��
				m_lastdatafail = 1;
				m_lastdata = cData;
				m_lastdatatime = GetTickCount();
				return 0;
			}
			if(::WriteFile(m_hSerial, m_blockbuffer, m_blockbuffer_size, &dwWrittenSize, &m_writeovl[idx])){
				// �������߂��ꍇ
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
				m_blockbuffer_pos = 0;
				return 1;
			}else{
				// �񓯊�I/O�҂��J�n
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
		// 1byte��������
		if(idx==-1){
			// �󂫂��Ȃ��̂ŏ������ݕs��
			m_lastdatafail = 1;
			m_lastdata = cData;
			m_lastdatatime = GetTickCount();
			return 0;
		}
		if(::WriteFile(m_hSerial, &cData, 1, &dwWrittenSize, &m_writeovl[idx])){
			// �������߂��ꍇ
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
			return 1;
		}else{
			// �񓯊�I/O�҂��J�n
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
 * �������݃��g���C
 * @return �T�C�Y
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
			// �񓯊�I/O�҂��̏ꍇ
			DWORD cbNumberOfBytesTransferred = 0;
			if(GetOverlappedResult(m_hSerial, &m_writeovl[i], &cbNumberOfBytesTransferred, FALSE)){
				m_writeovl_pending[i] = false;
				// �g�p�\�ԍ��Ƃ��Ċ��蓖��
				idx = i;
				emptycount++;
			}
		}else{
			// �g�p�\�ԍ��Ƃ��Ċ��蓖��
			idx = i;
			emptycount++;
		}
	}
	if(idx==-1){
		// �󂫂��Ȃ��̂ŏ������ݕs��
		if (GetTickCount() - m_lastdatatime > 3000) {
			// 3�b�ԃo�b�t�@�f�[�^�����肻���ɂȂ��Ȃ炠����߂�
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
			return 1; 
		}
		return 0;
	}
	if(m_blocktransfer){
		// �u���b�N�P�ʏ�������
		if(m_blockbuffer_pos == m_blockbuffer_size){
			if(::WriteFile(m_hSerial, m_blockbuffer, m_blockbuffer_size, &dwWrittenSize, &m_writeovl[idx])){
				// �������߂��ꍇ
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
				m_blockbuffer_pos = 0;
				return 1;
			}else{
				// �񓯊�I/O�҂��J�n
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
			// ��{�I�ɂ͂����ɂ͗��Ȃ��͂�
			m_blockbuffer[m_blockbuffer_pos] = m_lastdata;
			m_blockbuffer_pos++;
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
		}
		return 1;
	}else{
		// 1byte��������
		if(::WriteFile(m_hSerial, &m_lastdata, 1, &dwWrittenSize, &m_writeovl[idx])){
			// �������߂��ꍇ
			m_lastdatafail = 0;
			m_lastdata = 0;
			m_lastdatatime = 0;
			return 1;
		}else{
			// �񓯊�I/O�҂��J�n
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
 * �u���b�N�P�ʓ]���J�n
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
			return; // �u���b�N�]�����Ȃ�
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
 * �u���b�N�P�ʓ]���I��
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
					// �񓯊�I/O�҂��̏ꍇ
					DWORD cbNumberOfBytesTransferred = 0;
					if(GetOverlappedResult(m_hSerial, &m_writeovl[i], &cbNumberOfBytesTransferred, FALSE)){
						m_writeovl_pending[i] = false;
						// �g�p�\�ԍ��Ƃ��Ċ��蓖��
						idx = i;
						emptycount++;
					}
				}else{
					// �g�p�\�ԍ��Ƃ��Ċ��蓖��
					idx = i;
					emptycount++;
				}
			}
			if(idx==-1){
				// �󂫂��Ȃ��̂�0�Ԃ̏������݊�����҂�
				DWORD cbNumberOfBytesTransferred = 0;
				GetOverlappedResult(m_hSerial, &m_writeovl[0], &cbNumberOfBytesTransferred, TRUE);
				idx = 0;
			}
			if(::WriteFile(m_hSerial, m_blockbuffer, m_blockbuffer_pos, &dwWrittenSize, &m_writeovl[idx])){
				// �������߂��ꍇ
				m_lastdatafail = 0;
				m_lastdata = 0;
				m_lastdatatime = 0;
			}else{
				// �񓯊�I/O�҂��J�n
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
 * �Ō�̏������݂��������Ă��邩�`�F�b�N
 * @return �T�C�Y
 */
UINT CComSerial::LastWriteSuccess()
{
	if(m_lastdatafail && GetTickCount() - m_lastdatatime > 3000){
		return 1; // 3�b�ԃo�b�t�@�f�[�^�����肻���ɂȂ��Ȃ炠����߂�
	}
	if(m_lastdatafail){
		return 0;
	}
	return 1;
}

/**
 * �X�e�[�^�X�𓾂�
 * bit 7: ~CI (RI, RING)
 * bit 6: ~CS (CTS)
 * bit 5: ~CD (DCD, RLSD)
 * bit 4: reserved
 * bit 3: reserved
 * bit 2: reserved
 * bit 1: reserved
 * bit 0: ~DSR (DR)
 * @return �X�e�[�^�X 
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
 * ���b�Z�[�W
 * @param[in] nMessage ���b�Z�[�W
 * @param[in] nParam �p�����^
 * @return ���U���g �R�[�h
 */
INTPTR CComSerial::Message(UINT nMessage, INTPTR nParam)
{
	if (m_hSerial == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	switch (nMessage)
	{
		case COMMSG_CHANGESPEED: // �ʐM���x�ύX
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
			
		case COMMSG_CHANGEMODE: // �ʐM���[�h�ύX
			if(!m_fixedspeed){
				bool changed = false;
				UINT8 newmode = *(reinterpret_cast<UINT8*>(nParam)); // I/O 32h ���[�h�Z�b�g�̃f�[�^
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

		case COMMSG_SETCOMMAND: // RTS��DTR�t���O�̃Z�b�g
			{
				UINT8 cmd = *(reinterpret_cast<UINT8*>(nParam)); // I/O 32h �R�}���h�Z�b�g�̃f�[�^
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

		case COMMSG_PURGE: // �o�b�t�@�f�[�^�j��
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

		case COMMSG_GETERROR: // �ʐM�G���[�擾
			{
				// �G���[��� (bit0: �p���e�B, bit1: �I�[�o�[����, bit2: �t���[�~���O, bit3: �u���[�N�M��)
				UINT8 *errflag = (reinterpret_cast<UINT8*>(nParam)); // I/O 32h �R�}���h�Z�b�g�̃f�[�^
				if(errflag){
					*errflag = m_errorstat;
				}
			}
			break;

		case COMMSG_CLRERROR: // �ʐM�G���[�N���A
			{
				m_errorstat = 0;
			}
			break;

		default:
			break;
	}
	return 0;
}

