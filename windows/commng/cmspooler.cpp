/**
 * @file	cmspooler.cpp
 * @brief	Windowsスプーラ印刷 クラスの動作の定義を行います
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

#include "compiler.h"
#include "np2.h"
#include "cmspooler.h"
#ifdef SUPPORT_PRINT_PR201
#include "print/pmpr201.h"
#endif
#ifdef SUPPORT_PRINT_ESCP
#include "print/pmescp.h"
#endif

#include <process.h>
#include <codecnv/codecnv.h>

#if 0
#undef	TRACEOUT
static void trace_fmt_ex(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
static void trace_fmt_exw(const WCHAR* fmt, ...)
{
	WCHAR stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vswprintf(stmp, 2048, fmt, ap);
	wcscat(stmp, L"\n");
	va_end(ap);
	OutputDebugStringW(stmp);
}
#define	TRACEOUTW(s)	trace_fmt_exw s
#else
#define	TRACEOUTW(s)	(void)0
#endif	/* 1 */

static float graph_escp_lpi = (float)160 / 24;

 // モーダルダイアログを勝手に表示されたときに、マウスカーソルを出すようにする
static HHOOK g_hCbt = nullptr;
static bool g_mouseOn = false;
static LRESULT CALLBACK CbtProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HCBT_CREATEWND)
	{
		g_mouseOn = true;
		ShowCursor(TRUE);
	}
	return CallNextHookEx(g_hCbt, nCode, wParam, lParam);
}
static void InstallThreadModalDetectHook()
{
	if (g_hCbt) return;
	g_mouseOn = false;
	DWORD tid = GetCurrentThreadId();
	g_hCbt = SetWindowsHookExW(WH_CBT, CbtProc, nullptr, tid);
}
static void UninstallThreadModalDetectHook()
{
	if (!g_hCbt) return;
	UnhookWindowsHookEx(g_hCbt);
	g_hCbt = nullptr;
	if (g_mouseOn) {
		ShowCursor(FALSE);
	}
}

static unsigned int __stdcall cComSpooler_TimeoutThread(LPVOID vdParam)
{
	CComSpooler* t = (CComSpooler*)vdParam;
	
	while (WaitForSingleObject(t->m_hThreadExitEvent, 500) == WAIT_TIMEOUT) {
		EnterCriticalSection(&t->m_csPrint);
		if (GetTickCounter() - t->m_lastSendTime >= t->m_pageTimeout) {
			t->CCEndDocPrinter();
			t->m_hThreadTimeout = NULL;
			LeaveCriticalSection(&t->m_csPrint);
			break;
		}
		LeaveCriticalSection(&t->m_csPrint);
	}
	return 0;
}

static bool GetActualPrinterName(const LPTSTR printerName, LPTSTR actualPrinterName)
{
	if (printerName && _tcslen(printerName) > 0)
	{
		_tcscpy(actualPrinterName, printerName);
	}
	else
	{
		DWORD buflen = MAX_PATH;
		if (!GetDefaultPrinter(actualPrinterName, &buflen)) {
			return false;
		}
		if (_tcslen(actualPrinterName) == 0) return false;
	}
	return true;
}

static unsigned short jis_to_sjis(unsigned short jis)
{
	UINT8 j1 = (UINT8)(jis >> 8);
	UINT8 j2 = (UINT8)(jis & 0xFF);

	/* JIS X 0208 の有効範囲チェック */
	if (j1 < 0x21 || j1 > 0x7E || j2 < 0x21 || j2 > 0x7E) {
		return 0;
	}

	/*
	 *   s1 = (j1 + 1)/2 + 0x70;  s1 >= 0xA0 なら s1 += 0x40;
	 *   s2 = j2 + 0x1F;          j1 が偶数なら s2 += 0x5E;
	 *   s2 >= 0x7F なら s2++;
	 */
	UINT8 s1 = (UINT8)(((j1 + 1) >> 1) + 0x70);
	if (s1 >= 0xA0) {
		s1 = (UINT8)(s1 + 0x40);
	}

	UINT8 s2 = (UINT8)(j2 + 0x1F);
	if ((j1 & 1) == 0) {               /* j1 が偶数（区が偶数） */
		s2 = (UINT8)(s2 + 0x5E);
	}
	if (s2 >= 0x7F) {
		s2 = (UINT8)(s2 + 1);
	}

	return (UINT16)((UINT16)s1 << 8 | s2);
}

static COLORREF ColorCodeToColorRef(UINT8 colorCode)
{
	switch (colorCode) {
	case 0:
		return RGB(0,0,0);
	case 1:
		return RGB(0, 0, 255);
	case 2:
		return RGB(255, 0, 0);
	case 3:
		return RGB(255, 0, 255);
	case 4:
		return RGB(0, 255, 0);
	case 5:
		return RGB(0, 255, 255);
	case 6:
		return RGB(255, 255, 0);
	case 7:
		return RGB(255, 255, 255);
	}
	return RGB(0, 0, 0);
}


/**
 * インスタンス作成
 * @param[in] comcfg COMCFG
 * @return インスタンス
 */
CComSpooler* CComSpooler::CreateInstance(COMCFG* comcfg)
{
	CComSpooler* pPara = new CComSpooler;
	if (!pPara->Initialize(comcfg))
	{
		delete pPara;
		pPara = NULL;
	}
	return pPara;
}

/**
 * コンストラクタ
 */
CComSpooler::CComSpooler()
	: CComBase(COMCONNECT_PARALLEL)
	, m_emulation(PRINT_EMU_MODE_RAW)
	, m_pageTimeout(5000)
	, m_lastSendTime(0)
	, m_isOpened(false)
	, m_isStart(false)
	, m_printerName()
	, m_hPrinter(NULL)
	, m_hThreadTimeout(NULL)
	, m_hThreadExitEvent(NULL)
	, m_csPrint()
	, m_hasValidData(false)
	, m_lastData(0)
	, m_dataCounter(0)
	, m_jobId(0)
	, m_lastHasError(false)
	, m_devmodebuf()
	, m_hdc(NULL)
	, m_print(NULL)
	, m_additionalOfsX(0)
	, m_additionalOfsY(0)
{
	InitializeCriticalSection(&m_csPrint);
	m_hThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

/**
 * デストラクタ
 */
CComSpooler::~CComSpooler()
{
	if (m_isOpened)
	{
		CCEndThread();
		CCEndDocPrinter();

		if (!m_emulation) {
			::ClosePrinter(m_hPrinter);
			m_hPrinter = NULL;
			m_isOpened = false;
		}
	}
	CloseHandle(m_hThreadExitEvent);
	DeleteCriticalSection(&m_csPrint);
}

/**
 * プリンタ開く
 */
bool CComSpooler::CCOpenPrinter()
{
	EnterCriticalSection(&m_csPrint);

	if (m_isOpened) goto finalize;

	TCHAR printerName[MAX_PATH];
	if (!GetActualPrinterName(m_printerName, printerName)) {
		goto finalize;
	}
	if (!m_emulation) {
		if (!::OpenPrinter(printerName, &m_hPrinter, nullptr)) {
			m_isOpened = true;
			goto finalize;
		}
	}
	m_isOpened = true;

finalize:
	LeaveCriticalSection(&m_csPrint);
	return m_isOpened;
}
/**
 * プリンタ印刷開始
 */
void CComSpooler::CCStartDocPrinter()
{
	InstallThreadModalDetectHook();
	EnterCriticalSection(&m_csPrint);

	if (!m_isOpened) goto finalize;
	if (m_isStart) goto finalize;

	SYSTEMTIME st;
	GetLocalTime(&st);

	TCHAR documentName[MAX_PATH] = { 0 };
	_stprintf(documentName, _T("NP2_PRINT_%04u%02u%02u_%02u%02u%02u"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	if (m_print) {
		delete m_print;
		m_print = NULL;
	}
	switch (m_emulation) {
#ifdef SUPPORT_PRINT_PR201
	case PRINT_EMU_MODE_PR201:
		m_print = new CPrintPR201();
		break;
#endif
#ifdef SUPPORT_PRINT_ESCP
	case PRINT_EMU_MODE_ESCP:
		m_print = new CPrintESCP();
		break;
#endif
	}

	if (!m_print) {
		// RAW出力モード
		DOC_INFO_1 di = { 0 };
		di.pDocName = documentName;
		di.pOutputFile = nullptr;
		di.pDatatype = _T("RAW");

		if (np2oscfg.prncfgpp) {
			TCHAR printerName[MAX_PATH];
			if (!GetActualPrinterName(m_printerName, printerName)) {
				goto finalize;
			}
			LONG dmsize = DocumentProperties(
				NULL,
				m_hPrinter,
				printerName,
				NULL,
				NULL,
				0
			);
			bool devmodebufResized = false;
			if (m_devmodebuf.size() != dmsize) {
				// プリンタデータサイズが変わっているならクリア
				m_devmodebuf.resize(dmsize);
				devmodebufResized = true;
			}
			DEVMODE* pDevMode = (DEVMODE*)(&m_devmodebuf[0]);
			if (devmodebufResized) {
				// 現在の設定取得
				DocumentProperties(
					NULL,
					m_hPrinter,
					printerName,
					pDevMode,
					NULL,
					DM_OUT_BUFFER
				);
			}
			if (!AdvancedDocumentProperties(g_hWndMain, m_hPrinter, printerName, pDevMode, pDevMode)) {
				goto finalize;
			}

			DWORD dwNeeded;
			GetPrinter(m_hPrinter, 2, NULL, 0, &dwNeeded);
			PRINTER_INFO_2 *pi = (PRINTER_INFO_2*)malloc(dwNeeded);
			if (pi) {
				GetPrinter(m_hPrinter, 2, (LPBYTE)pi, dwNeeded, &dwNeeded);
				pi->pDevMode = pDevMode;
				SetPrinter(m_hPrinter, 2, (LPBYTE)pi, 0);
				free(pi);
			}
		}

		m_jobId = ::StartDocPrinter(m_hPrinter, 1, (LPBYTE)&di);
		if (m_jobId == 0) {
			goto finalize;
		}

		if (!::StartPagePrinter(m_hPrinter)) {
			::EndDocPrinter(m_hPrinter);
			goto finalize;
		}
	}
	else {
		// エミュレーションモード
		TCHAR printerName[MAX_PATH];
		if (!GetActualPrinterName(m_printerName, printerName)) {
			goto finalize;
		}

		HANDLE hPrinter;
		OpenPrinter(printerName, &hPrinter, NULL);
		if (!hPrinter) {
			goto finalize;
		}
		LONG dmsize = DocumentProperties(
			NULL,
			hPrinter,
			printerName,
			NULL,
			NULL,
			0
		);
		bool devmodebufResized = false;
		if (m_devmodebuf.size() != dmsize) {
			// プリンタデータサイズが変わっているならクリア
			m_devmodebuf.resize(dmsize);
			devmodebufResized = true;
		}
		DEVMODE* pDevMode = (DEVMODE*)(&m_devmodebuf[0]);
		if (devmodebufResized) {
			// 現在の設定取得
			DocumentProperties(
				NULL,
				hPrinter,
				printerName,
				pDevMode,
				NULL,
				DM_OUT_BUFFER
			);
		}
		if (np2oscfg.prncfgpp) {
			if (!AdvancedDocumentProperties(g_hWndMain, hPrinter, printerName, pDevMode, pDevMode)) {
				ClosePrinter(hPrinter);
				goto finalize;
			}
		}
		ClosePrinter(hPrinter);
		//pDevMode->dmFields |= DM_PAPERSIZE | DM_ORIENTATION;
		//pDevMode->dmPaperSize = DMPAPER_A4;
		//pDevMode->dmOrientation = DMORIENT_PORTRAIT;
		//pDevMode->dmFields |= DM_PAPERWIDTH | DM_PAPERLENGTH;
		//pDevMode->dmPaperWidth = 1000;
		//pDevMode->dmPaperLength = 1480;
		m_hdc = CreateDCW(L"WINSPOOL", printerName, nullptr, pDevMode);
		if (!m_hdc) {
			goto finalize;
		}

		DOCINFO di = { 0 };
		di.cbSize = sizeof(di);
		di.lpszDocName = documentName;
		m_jobId = ::StartDoc(m_hdc, &di);
		if (m_jobId <= 0) {
			m_jobId = 0;
			DeleteDC(m_hdc);
			goto finalize;
		}

		if (::StartPage(m_hdc) <= 0) {
			EndDoc(m_hdc);
			DeleteDC(m_hdc);
			goto finalize;
		}

		CCStartPrint();
	}

	// タイムアウト監視開始
	if (m_pageTimeout > 0 && !m_hThreadTimeout) {
		unsigned int dwID;
		m_hThreadTimeout = (HANDLE)_beginthreadex(NULL, 0, cComSpooler_TimeoutThread, this, 0, &dwID);
	}

	m_lastSendTime = GetTickCounter();
	m_dataCounter = 0;
	m_hasValidData = false;
	m_isStart = true;
finalize:
	LeaveCriticalSection(&m_csPrint);
	UninstallThreadModalDetectHook();
}
/**
 * タイムアウト監視スレッド終了
 */
void CComSpooler::CCEndThread()
{
	// タイムアウト監視終了
	if (m_hThreadTimeout) {
		SetEvent(m_hThreadExitEvent);
		if (WaitForSingleObject(m_hThreadTimeout, 10000) == WAIT_TIMEOUT)
		{
			TerminateThread(m_hThreadTimeout, 0); // ゾンビスレッド死すべし
		}
		CloseHandle(m_hThreadTimeout);
		m_hThreadTimeout = NULL;
	}
}

/**
 * プリンタ印刷終了
 */
void CComSpooler::CCEndDocPrinter()
{
	EnterCriticalSection(&m_csPrint);

	if (!m_isOpened) goto finalize;
	if (!m_isStart) goto finalize;

	if (!m_print) {
		// RAW出力モード
		if ((m_dataCounter <= 1 && m_lastData >= 0x80) || (!m_hasValidData && m_dataCounter <= 100)) {
			// ごみデータと思われるので捨てる
			::SetJob(m_hPrinter, m_jobId, 0, NULL, JOB_CONTROL_CANCEL);
		}
		::EndPagePrinter(m_hPrinter);
		::EndDocPrinter(m_hPrinter);
	}
	else {
		// エミュレーションモード
		m_print->EndPrint();

		if ((m_dataCounter <= 1 && m_lastData >= 0x80) || (!m_hasValidData && m_dataCounter <= 100)) {
			// ごみデータと思われるので捨てる
			::AbortDoc(m_hdc);
		}
		else {
			::EndPage(m_hdc);
			::EndDoc(m_hdc);
		}

		DeleteDC(m_hdc);
		m_hdc = NULL;
	}

	if (m_print) {
		delete m_print;
		m_print = NULL;
	}

	m_isStart = false;
finalize:
	LeaveCriticalSection(&m_csPrint);
}

bool CComSpooler::SetConfigurations(COMCFG* comcfg)
{
	if (comcfg->spoolPrinterName) {
		_tcscpy(m_printerName, comcfg->spoolPrinterName);
	}
	m_emulation = comcfg->spoolEmulation;
	m_pageTimeout = comcfg->spoolTimeout;
	m_dotscale = comcfg->spoolDotSize / 100.0;
	m_rectdot = comcfg->spoolRectDot ? true : false;
	m_pageAlignment = comcfg->spoolPageAlignment;
	m_additionalOfsX = comcfg->spoolOffsetXmm;
	m_additionalOfsY = comcfg->spoolOffsetYmm;
	m_scale = comcfg->spoolScale / 100.0f;

	m_lastHasError = false;

	return true;
}

/**
 * 初期化
 * @param[in] comcfg COMCFG
 * @retval true 成功
 * @retval false 失敗
 */
bool CComSpooler::Initialize(COMCFG* comcfg)
{
	SetConfigurations(comcfg);

	return true;
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComSpooler::Read(UINT8* pData)
{
	return 0;
}

/**
 * 書き込み
 * @param[out] cData データ
 * @return サイズ
 */
UINT CComSpooler::Write(UINT8 cData)
{
	UINT ret = 0;
	DWORD lastSendTime = m_lastSendTime;
	m_lastSendTime = GetTickCounter();

	EnterCriticalSection(&m_csPrint);
	DWORD dwWrittenSize;
	if (!m_isOpened) {
		if (m_lastHasError && m_lastSendTime - lastSendTime < 1000) {
			// 短時間で送られ過ぎないようにする
			m_lastSendTime = GetTickCounter();
			goto finalize;
		}
		if (!CCOpenPrinter()) {
			m_lastHasError = true;
			m_lastSendTime = GetTickCounter();
			goto finalize;
		}
		m_lastHasError = false;
	}

	if (!m_isStart) {
		// いきなりEOTは無視
		if (cData == 0x04) {
			ret = 1; // 成功扱い
			goto finalize;
		}

		if (m_lastHasError && !m_isStart && m_lastSendTime - lastSendTime < 1000) {
			// 短時間で送られ過ぎないようにする
			m_lastSendTime = GetTickCounter();
			goto finalize;
		}
		CCStartDocPrinter();
		if (!m_isStart) {
			m_lastHasError = true;
			m_lastSendTime = GetTickCounter();
			goto finalize;
		}
	}

	m_lastHasError = false;

	m_lastSendTime = GetTickCounter();
	if (m_dataCounter < 10000) {
		m_dataCounter++;
	}
	if (0x08 <= cData && cData <= 0x0d || 0x20 <= cData) {
		m_hasValidData = true;
	}
	if (!m_print) {
		ret = (::WritePrinter(m_hPrinter, &cData, 1, &dwWrittenSize)) ? 1 : 0;
	}
	else {
		if (m_print->Write(cData)) {
			// 1コマンド完成
			PRINT_COMMAND_RESULT writeResult;
			do {
				if (m_requestNewPage && m_print->HasRenderingCommand()) {
					// 改ページ実行
					::EndPage(m_hdc);
					::StartPage(m_hdc);
					m_requestNewPage = false;
				}
				writeResult = m_print->DoCommand();
				if (writeResult == PRINT_COMMAND_RESULT_COMPLETEPAGE) {
					m_requestNewPage = true;
				}
			} while (writeResult != PRINT_COMMAND_RESULT_OK);
		}
		ret = 1;
	}
	m_lastData = cData;

finalize:
	LeaveCriticalSection(&m_csPrint);
	return ret;
}

/**
 * ステータスを得る
 * bit 7: ~CI (RI, RING)
 * bit 6: ~CS (CTS)
 * bit 5: ~CD (DCD, RLSD)
 * bit 4: reserved
 * bit 3: reserved
 * bit 2: reserved
 * bit 1: reserved
 * bit 0: ~DSR (DR)
 * @return ステータス
 */
UINT8 CComSpooler::GetStat()
{
	return 0x00;
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComSpooler::Message(UINT nMessage, INTPTR nParam)
{
	switch (nMessage)
	{
		case COMMSG_REOPEN:
			EnterCriticalSection(&m_csPrint);
			if (m_isOpened)
			{
				CCEndThread();
				CCEndDocPrinter();

				::ClosePrinter(m_hPrinter);
				m_hPrinter = NULL;
				m_isOpened = false;
				m_lastHasError = false;
			}

			if (nParam) {
				COMCFG* newParam = (COMCFG*)nParam;
				if (_tcscmp(newParam->spoolPrinterName, m_printerName) != 0) {
					m_devmodebuf.clear();
				}
				SetConfigurations(newParam);
			}

			LeaveCriticalSection(&m_csPrint);
			break;

		default:
			break;
	}
	return 0;
}

// プリンタエミュレーション

void CComSpooler::CCStartPrint()
{
	float dpiX = GetDeviceCaps(m_hdc, LOGPIXELSX);
	float dpiY = GetDeviceCaps(m_hdc, LOGPIXELSY);
	float physWidth = GetDeviceCaps(m_hdc, PHYSICALWIDTH);
	float physHeight = GetDeviceCaps(m_hdc, PHYSICALHEIGHT);

	float pageoffsetx = m_additionalOfsX * dpiX / 25.4;
	float pageoffsety = m_additionalOfsY * dpiY / 25.4;
	if (m_pageAlignment == ESCPEMU_PAGE_ALIGNMENT_CENTER) {
		// ソフトによって値がバラバラなので意味不明　やむを得ずA4＆一太郎限定とする
		pageoffsetx -= 2176.0 / 160 * dpiX / 2 - 210 / 2 * dpiX / 25.4; // プリンタドット数/2 - A4短辺/2
	}
	float pagescaleoffsetx = 1;
	float pagescaleoffsety = 1;
	if (m_scale > 0) {
		// スケール　中央寄せする
		pagescaleoffsetx = (1 - m_scale) * physWidth / 2;
		pagescaleoffsety = (1 - m_scale) * physHeight / 2;
		pageoffsetx *= m_scale;
		pageoffsety *= m_scale;
		physWidth *= m_scale;
		physHeight *= m_scale;
		dpiX *= m_scale;
		dpiY *= m_scale;
	}
	
	m_print->StartPrint(m_hdc, pageoffsetx + pagescaleoffsetx, pageoffsety + pagescaleoffsety, physWidth, physHeight, dpiX, dpiY, m_dotscale, m_rectdot);

	m_requestNewPage = false;
}
