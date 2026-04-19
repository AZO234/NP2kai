/**
 * @file	cmfile.h
 * @brief	ファイルダンプ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "cmbase.h"

/**
 * @brief commng パラレル デバイス クラス
 */
class CComFile : public CComBase
{
public:
	int m_pageTimeout;				/*!< プリンタタイムアウト */
	HANDLE m_hThreadTimeout;
	HANDLE m_hThreadExitEvent;
	CRITICAL_SECTION m_csPrint;
	DWORD m_lastSendTime;

	static CComFile* CreateInstance(LPCTSTR dirpath, int pageTimeout);

	void CCEndThread();
	void CCCloseFile();

protected:
	CComFile();
	virtual ~CComFile();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);

private:
	TCHAR m_dirpath[MAX_PATH];  /*!< 保存先ディレクトリパス */
	HANDLE m_hFile;				/*!< ファイル ハンドル */

	bool Initialize(LPCTSTR dirpath, int pageTimeout);
	void CCStartThread();
};
