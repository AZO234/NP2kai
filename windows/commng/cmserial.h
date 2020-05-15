/**
 * @file	cmserial.h
 * @brief	シリアル クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "cmbase.h"

extern const UINT32 cmserial_speed[11];

/**
 * @brief commng シリアル デバイス クラス
 */
class CComSerial : public CComBase
{
public:
	static CComSerial* CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed);

protected:
	CComSerial();
	virtual ~CComSerial();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT WriteRetry();
	virtual UINT LastWriteSuccess(); // 最後の書き込みが成功しているかチェック
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);

private:
	HANDLE m_hSerial;		/*!< シリアル ハンドル */

	bool m_fixedspeed;	/*!< 通信速度固定 */
	UINT8 m_lastdata; // 最後に送ろうとしたデータ
	UINT8 m_lastdatafail; // データを送るのに失敗していたら0以外
	DWORD m_lastdatatime; // データを送るのに失敗した時間（あまりにも失敗し続けるようなら無視する）

	bool Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed);
};
