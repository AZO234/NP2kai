/**
 * @file	cmserial.h
 * @brief	�V���A�� �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂�
 */

#pragma once

#include "cmbase.h"

#define SERIAL_OVERLAP_COUNT	4
#define SERIAL_BLOCKBUFFER_SIZE_MAX	128

/**
 * ���x�e�[�u��
 */
const UINT32 cmserial_speed[] = {110, 300, 600, 1200, 2400, 4800,
							9600, 14400, 19200, 28800, 38400, 57600, 115200};

/**
 * @brief commng �V���A�� �f�o�C�X �N���X
 */
class CComSerial : public CComBase
{
public:
	static CComSerial* CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck);

protected:
	CComSerial();
	virtual ~CComSerial();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT WriteRetry();
	virtual UINT LastWriteSuccess(); // �Ō�̏������݂��������Ă��邩�`�F�b�N
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);
	virtual void BeginBlockTransfer();
	virtual void EndBlockTransfer();

private:
	HANDLE m_hSerial;		/*!< �V���A�� �n���h�� */
	
	OVERLAPPED m_writeovl[SERIAL_OVERLAP_COUNT];	/*!< ��������OVERLAPPED */
	OVERLAPPED m_readovl;	/*!< �ǂݍ���OVERLAPPED */
	bool m_writeovl_pending[SERIAL_OVERLAP_COUNT];	/*!< ��������OVERLAPPED�ҋ@�� */
	bool m_readovl_pending;	/*!< �ǂݍ���OVERLAPPED�ҋ@�� */
	UINT8 m_readovl_buf;	/*!< �ǂݍ���OVERLAPPED�o�b�t�@ */
	
	bool m_blocktransfer;			/*!< �u���b�N�P�ʏ������݃��[�h */
	UINT8 m_blockbuffer[SERIAL_BLOCKBUFFER_SIZE_MAX];		/*!< �u���b�N�o�b�t�@ */
	int m_blockbuffer_pos;		/*!< �u���b�N�o�b�t�@�������݈ʒu */
	int m_blockbuffer_size;		/*!< �u���b�N�o�b�t�@�T�C�Y */

	bool m_fixedspeed;	/*!< �ʐM���x�Œ� */
	UINT8 m_lastdata; // �Ō�ɑ��낤�Ƃ����f�[�^
	UINT8 m_lastdatafail; // �f�[�^�𑗂�̂Ɏ��s���Ă�����0�ȊO
	DWORD m_lastdatatime; // �f�[�^�𑗂�̂Ɏ��s�������ԁi���܂�ɂ����s��������悤�Ȃ疳������j

	UINT8 m_errorstat; // �G���[��� (bit0: �p���e�B, bit1: �I�[�o�[����, bit2: �t���[�~���O)

	bool Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck);
	void CheckCommError(DWORD errorcode);
};
