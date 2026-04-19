/**
 * @file	cmnull.h
 * @brief	commng NULL デバイス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "cmbase.h"

/**
 * @brief commng NULL デバイス クラス
 */
class CComNull : public CComBase
{
public:
	CComNull();

protected:
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);
};
