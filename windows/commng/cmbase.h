/**
 * @file	cmbase.h
 * @brief	commng 基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "commng.h"

/**
 * @brief commng 基底クラス
 */
class CComBase : public _commng
{
protected:
	CComBase(UINT nConnect);
	virtual ~CComBase();

	/**
	 * Read
	 * @param[out] pData
	 * @return result
	 */
	virtual UINT Read(UINT8* pData) = 0;

	/**
	 * Write
	 * @param[in] cData
	 * @return result
	 */
	virtual UINT Write(UINT8 cData) = 0;
	
	/**
	 * Write Retry
	 * @return result
	 */
	virtual UINT WriteRetry(){
		return 1; // 常時成功扱い
	}
	
	/**
	 * ブロック単位転送開始
	 */
	virtual void BeginBlockTransfer(){
	}
	/**
	 * ブロック単位転送終了
	 */
	virtual void EndBlockTransfer(){
	}
	
	/**
	 * Last Write Success
	 * @return result
	 */
	virtual UINT LastWriteSuccess(){
		return 1; // 常時成功扱い
	}

	/**
	 * ステータスを得る
	 * @return ステータス
	 */
	virtual UINT8 GetStat() = 0;

	/**
	 * メッセージ
	 * @param[in] nMessage メッセージ
	 * @param[in] nParam パラメタ
	 * @return リザルト コード
	 */
	virtual INTPTR Message(UINT nMessage, INTPTR nParam) = 0;

private:
	static UINT cRead(COMMNG cm, UINT8* pData);
	static UINT cWrite(COMMNG cm, UINT8 cData);
	static UINT cWriteRetry(COMMNG cm);
	static void cBeginBlockTransfer(COMMNG cm);
	static void cEndBlockTransfer(COMMNG cm);
	static UINT cLastWriteSuccess(COMMNG cm);
	static UINT8 cGetStat(COMMNG cm);
	static INTPTR cMessage(COMMNG cm, UINT nMessage, INTPTR nParam);
	static void cRelease(COMMNG cm);
};
