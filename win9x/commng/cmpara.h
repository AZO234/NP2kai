/**
 * @file	cmpara.h
 * @brief	パラレル クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "cmbase.h"

/**
 * @brief commng パラレル デバイス クラス
 */
class CComPara : public CComBase
{
public:
	static CComPara* CreateInstance(UINT nPort);

protected:
	CComPara();
	virtual ~CComPara();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);

private:
	HANDLE m_hParallel;			/*!< パラレル ハンドル */

	bool Initialize(UINT nPort);
};
