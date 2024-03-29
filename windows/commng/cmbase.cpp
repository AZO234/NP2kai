/**
 * @file	cmbase.h
 * @brief	commng 基底クラスの動作の定義を行います
 */

#include <compiler.h>
#include "cmbase.h"

/**
 * コンストラクタ
 * @param[in] nConnect 接続フラグ
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
 * デストラクタ
 */
CComBase::~CComBase()
{
}

/**
 * Read
 * @param[in] cm COMMNG インスタンス
 * @param[out] pData
 * @return result
 */
UINT CComBase::cRead(COMMNG cm, UINT8* pData)
{
	return static_cast<CComBase*>(cm)->Read(pData);
}

/**
 * Write
 * @param[in] cm COMMNG インスタンス
 * @param[in] cData
 * @return result
 */
UINT CComBase::cWrite(COMMNG cm, UINT8 cData)
{
	return static_cast<CComBase*>(cm)->Write(cData);
}

/**
 * Write Retry
 * @param[in] cm COMMNG インスタンス
 * @return result
 */
UINT CComBase::cWriteRetry(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->WriteRetry();
}

/**
 * Begin Block Transfer
 * @param[in] cm COMMNG インスタンス
 * @return result
 */
void CComBase::cBeginBlockTransfer(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->BeginBlockTransfer();
}
/**
 * End Block Transfer
 * @param[in] cm COMMNG インスタンス
 * @return result
 */
void CComBase::cEndBlockTransfer(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->EndBlockTransfer();
}
/**
 * Last Write Success
 * @param[in] cm COMMNG インスタンス
 * @return result
 */
UINT CComBase::cLastWriteSuccess(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->LastWriteSuccess();
}

/**
 * ステータスを得る
 * @param[in] cm COMMNG インスタンス
 * @return ステータス
 */
UINT8 CComBase::cGetStat(COMMNG cm)
{
	return static_cast<CComBase*>(cm)->GetStat();
}

/**
 * メッセージ
 * @param[in] cm COMMNG インスタンス
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComBase::cMessage(COMMNG cm, UINT nMessage, INTPTR nParam)
{
	return static_cast<CComBase*>(cm)->Message(nMessage, nParam);
}

/**
 * リリース
 * @param[in] cm COMMNG インスタンス
 */
void CComBase::cRelease(COMMNG cm)
{
	delete static_cast<CComBase*>(cm);
}
