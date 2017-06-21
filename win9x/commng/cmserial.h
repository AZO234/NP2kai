/**
 * @file	cmserial.h
 * @brief	シリアル クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "cmbase.h"

extern const UINT32 cmserial_speed[10];

/**
 * @brief commng シリアル デバイス クラス
 */
class CComSerial : public CComBase
{
public:
	static CComSerial* CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed);

protected:
	CComSerial();
	virtual ~CComSerial();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);

private:
	HANDLE m_hSerial;		/*!< シリアル ハンドル */

	bool Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed);
};
