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
	static UINT8 cGetStat(COMMNG cm);
	static INTPTR cMessage(COMMNG cm, UINT nMessage, INTPTR nParam);
	static void cRelease(COMMNG cm);
};
