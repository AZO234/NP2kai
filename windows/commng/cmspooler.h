/**
 * @file	cmspooler.h
 * @brief	Windowsスプーラ印刷 クラスの宣言およびインターフェイスの定義をします
 */

 /*
  * Copyright (c) 2026 SimK
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#pragma once

#include <vector>

#include "cmbase.h"
#include "print/pmbase.h"

#define PRINT_EMU_MODE_RAW		0
#define PRINT_EMU_MODE_PR201	1
#define PRINT_EMU_MODE_ESCP		2

#define ESCPEMU_PAGE_ALIGNMENT_LEFT		0
#define ESCPEMU_PAGE_ALIGNMENT_CENTER	1

/**
 * @brief commng パラレル デバイス クラス
 */
class CComSpooler : public CComBase
{
public:
	int m_pageTimeout;				/*!< プリンタタイムアウト */
	HANDLE m_hThreadTimeout;
	HANDLE m_hThreadExitEvent;
	CRITICAL_SECTION m_csPrint;
	DWORD m_lastSendTime;
	bool m_hasValidData;			/*!< 有効なデータが送られたか */
	UINT8 m_lastData;				/*!< 最後に送られたデータ */
	int m_dataCounter;				/*!< プリンタに送られたデータ数 */

	static CComSpooler* CreateInstance(COMCFG *comcfg);

	void CCEndThread();
	void CCEndDocPrinter();

protected:
	CComSpooler();
	virtual ~CComSpooler();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);

private:
	UINT8 m_emulation;				/*!< プリンタエミュレーションモード */
	bool m_isOpened;				/*!< プリンタ開いている */
	bool m_isStart;					/*!< ページ開始状態 */
	TCHAR m_printerName[MAX_PATH];  /*!< プリンタ名 */
	HANDLE m_hPrinter;				/*!< プリンタ ハンドル */
	DWORD m_jobId;					/*!< プリンタジョブID */
	bool m_lastHasError;			/*!< プリンタオープン等に失敗 */

	std::vector<UINT8> m_devmodebuf; /*!< プリンタ設定保持用バッファ */

	// プリンタエミュレーション用
	CPrintBase *m_print;			/*!< プリンタエミュレーション */

	HDC m_hdc;						/*!< GDIプリンタHDC */
	bool m_requestNewPage;			/*!< 改ページリクエスト（空白ページが末尾に付くのを防ぐため用） */

	bool m_rectdot;				/*!< 点を矩形で描画 */
	float m_dotscale;			/*!< 点の大きさ補正 */
	int m_pageAlignment;		/*!< ページアライメント */
	int m_additionalOfsX;		/*!< 追加位置オフセット X */
	int m_additionalOfsY;		/*!< 追加位置オフセット Y */
	float m_scale;				/*!< スケール調整 */

	bool SetConfigurations(COMCFG* comcfg);
	bool Initialize(COMCFG* comcfg);
	bool CCOpenPrinter();
	void CCStartDocPrinter();

	void CCStartPrint();
};
