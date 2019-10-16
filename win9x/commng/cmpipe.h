/**
 * @file	cmpipe.h
 * @brief	名前付きパイプ シリアル クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "cmbase.h"

#if defined(SUPPORT_NAMED_PIPE)

/**
 * @brief commng シリアル デバイス クラス
 */
class CComPipe : public CComBase
{
public:
	static CComPipe* CreateInstance(LPCTSTR pipename, LPCTSTR servername);

protected:
	CComPipe();
	virtual ~CComPipe();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);

private:
	HANDLE m_hSerial;		/*!< 名前付きパイプ ハンドル */
	bool m_isserver;		/*!< サーバーかどうか */
	OEMCHAR	m_pipename[MAX_PATH]; // The name of the named-pipe
	OEMCHAR	m_pipeserv[MAX_PATH]; // The server name of the named-pipe

	bool Initialize(LPCTSTR pipename, LPCTSTR servername);
};

#endif
