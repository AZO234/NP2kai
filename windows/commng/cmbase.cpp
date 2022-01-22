/**
 * @file	cmbase.h
 * @brief	commng ���N���X�̓���̒�`���s���܂�
 */

#include <compiler.h>
#include "cmbase.h"

/**
 * �R���X�g���N�^
 * @param[in] nConnect �ڑ��t���O
 */
CComBase::CComBase(UINT nConnect)
{
	this->connect = nConnect;
	this->read = cRead;
	this->write = cWrite;
	this->writeretry = cWriteRetry;
	this->beginblocktranster = cBeginBlockTransfer;
	this->endblocktranster = cEndBlockTransfer;
	this->lastwritesuccess = cLastWriteSuccess;
	this->getstat = cGetStat;
	this->msg = cMessage;
	this->release = cRelease;
}

/**
 * �f�X�g���N�^
 */
CComBase::~CComBase()
{
}

/**
 * Read
 * @param[in] cm COMMNG �C���X�^���X
 * @param[out] pData
 * @return result
 */
UINT CComBase::cRead(COMMNG cm, UINT8* pData)
{
	return static_cast<CComBase*>(cm)->Read(pData);
}

/**
 * Write
 * @param[in] cm COMMNG �C���X�^���X
 * @param[in] cData
 * @return result
 */
UINT CComBase::cWrite(COMMNG cm, UINT8 cData)
{
	return static_cast<CComBase*>(cm)->Write(cData);
}

/**
 * Write Retry
 * @param[in] cm COMMNG �C���X�^���X
 * @return result
 */
UINT CComBase::cWriteRetry(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->WriteRetry();
}

/**
 * Begin Block Transfer
 * @param[in] cm COMMNG �C���X�^���X
 * @return result
 */
void CComBase::cBeginBlockTransfer(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->BeginBlockTransfer();
}
/**
 * End Block Transfer
 * @param[in] cm COMMNG �C���X�^���X
 * @return result
 */
void CComBase::cEndBlockTransfer(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->EndBlockTransfer();
}
/**
 * Last Write Success
 * @param[in] cm COMMNG �C���X�^���X
 * @return result
 */
UINT CComBase::cLastWriteSuccess(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->LastWriteSuccess();
}

/**
 * �X�e�[�^�X�𓾂�
 * @param[in] cm COMMNG �C���X�^���X
 * @return �X�e�[�^�X
 */
UINT8 CComBase::cGetStat(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->GetStat();
}

/**
 * ���b�Z�[�W
 * @param[in] cm COMMNG �C���X�^���X
 * @param[in] nMessage ���b�Z�[�W
 * @param[in] nParam �p�����^
 * @return ���U���g �R�[�h
 */
INTPTR CComBase::cMessage(COMMNG cm, UINT nMessage, INTPTR nParam)
{
	return static_cast<CComBase*>(cm)->Message(nMessage, nParam);
}

/**
 * �����[�X
 * @param[in] cm COMMNG �C���X�^���X
 */
void CComBase::cRelease(COMMNG cm)
{
	delete static_cast<CComBase*>(cm);
}
