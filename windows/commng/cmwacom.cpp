/**
 * @file	cmwacom.cpp
 * @brief	Wacom Tablet クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmwacom.h"
#include "np2.h"

#ifdef SUPPORT_WACOM_TABLET

#include <math.h>
#include "pccore.h"
#include "iocore.h"
#include "mousemng.h"
#include "scrnmng.h"

typedef CComWacom *CMWACOM;

const char cmwacom_RData[] = "~RE202C900,002,02,1270,1270\r";
const char cmwacom_ModelData[] = "~#KT-0405-R00 V1.3-2\r";
const char cmwacom_CData[] = "~C06400,04800\r";

#define TABLET_BASERASOLUTION	1000

#define TABLET_RAWMAX_X	5040
#define TABLET_RAWMAX_Y	3779

static bool g_wacom_initialized = false;
static bool g_wacom_allocated = false;
static CComWacom *g_cmwacom = NULL;
static WNDPROC g_lpfnDefProc = NULL;
static bool g_exclusivemode = false;
static bool g_nccontrol = false;
static DWORD g_datatime = 0;
static SINT32 g_lastPosX = 0;
static SINT32 g_lastPosY = 0;
static bool g_lastPosValid = false;

static HCTX g_fakeContext = NULL; // 入力が変になるのを修正

void cmwacom_initialize(void){
	if(!g_wacom_initialized){
		if ( LoadWintab( ) ){
			g_datatime = GetTickCount();
			g_wacom_initialized = true;
			
			if(!g_fakeContext){
				LOGCONTEXTA lcMine;
				gpWTInfoA(WTI_DEFSYSCTX, 0, &lcMine);
				lcMine.lcOptions |= CXO_MARGIN;// | CXO_MESSAGES;
				lcMine.lcMsgBase = WT_DEFBASE;
				lcMine.lcPktData = PACKETDATA;
				lcMine.lcPktMode = PACKETMODE;
				lcMine.lcMoveMask = PACKETDATA;
				lcMine.lcBtnUpMask = lcMine.lcBtnDnMask;
				g_fakeContext = gpWTOpenA(g_hWndMain, &lcMine, TRUE);
			}
		}else{
			g_wacom_initialized = false;
		}
	}
}
void cmwacom_reset(void){
	if(g_cmwacom){
		CMWACOM_CONFIG cfg;
		g_cmwacom->GetConfig(&cfg);
		cfg.enable = false;
		g_cmwacom->SetConfig(cfg);
	}
	g_wacom_initialized = false;
}
void cmwacom_finalize(void){
	if(g_cmwacom){
		g_cmwacom->FinalizeTabletDevice();
	}
	if(g_wacom_initialized){
		if(g_fakeContext){
			gpWTClose(g_fakeContext);
			g_fakeContext = NULL;
		}
		UnloadWintab();
	}
	g_cmwacom = NULL;
	g_wacom_initialized = false;
}
bool cmwacom_skipMouseEvent(void){
	return g_cmwacom && g_cmwacom->SkipMouseEvent();
}
void cmwacom_setExclusiveMode(bool enable){
	g_exclusivemode = enable;
	if(g_cmwacom){
		g_cmwacom->SetExclusiveMode(enable);
	}
}
void cmwacom_setNCControl(bool enable){
	g_nccontrol = enable;
	if(g_cmwacom){
		g_cmwacom->SetNCControl(enable);
	}
}

LRESULT CALLBACK tabletWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	if(g_cmwacom){
		switch (msg) {
		case WT_PACKET:
			if(g_cmwacom->HandlePacketMessage(hWnd, msg, wParam, lParam)){
				return FALSE;
			}
			break;
		case WM_MOUSEMOVE:
			if(g_cmwacom->HandleMouseMoveMessage(hWnd, msg, wParam, lParam)){
				return FALSE;
			}
			break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			if(g_cmwacom->HandleMouseUpMessage(hWnd, msg, wParam, lParam)){
				return FALSE;
			}
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			if(g_cmwacom->HandleMouseDownMessage(hWnd, msg, wParam, lParam)){
				return FALSE;
			}
			break;
		case WM_ACTIVATE:
			if (wParam) {
				gpWTEnable(g_cmwacom->GetHTab(), TRUE);
				gpWTOverlap(g_cmwacom->GetHTab(), TRUE);
				{
					SINT16 x, y;
					mousemng_getstat(&x, &y, false);
					mousemng_setstat(x, y, uPD8255A_LEFTBIT|uPD8255A_RIGHTBIT);
				}
				g_cmwacom->m_skiptabletevent = 20;
			}else{
				gpWTEnable(g_cmwacom->GetHTab(), FALSE);
			}
			break;
		}
	}
	return CallWindowProc(g_lpfnDefProc, hWnd, msg, wParam, lParam);
}


/**
 * インスタンス作成
 * @param[in] lpMidiOut MIDIOUT デバイス
 * @param[in] lpMidiIn MIDIIN デバイス
 * @param[in] lpModule モジュール
 * @return インスタンス
 */
CComWacom* CComWacom::CreateInstance(HWND hWnd)
{
	CComWacom* pWacom = new CComWacom;
	if (!pWacom->Initialize(hWnd))
	{
		delete pWacom;
		pWacom = NULL;
	}
	return pWacom;
}

/**
 * コンストラクタ
 */
CComWacom::CComWacom()
	: CComBase(COMCONNECT_TABLET)
	, m_hwndMain(NULL)
	, m_hTab(NULL)
	, m_hMgr(NULL)
	, m_cmdbuf_pos(0)
	, m_sBuffer_wpos(0)
	, m_sBuffer_rpos(0)
	, m_wait(0)
	, m_skipmouseevent(0)
	, m_skiptabletevent(0)
	, m_exclusivemode(false)
	, m_nccontrol(false)
	, m_lastdatalen(0)
	, m_sendlastdata(false)
{
	m_config.enable = false;
	m_config.scrnsizemode = false;
	m_config.disablepressure = false;
	m_config.relmode = false;
	m_config.csvmode = false;
	m_config.suppress = false;
	m_config.start = true;
	m_config.mode19200 = false;
	m_config.resolution_w = TABLET_BASERASOLUTION;
	m_config.resolution_h = TABLET_BASERASOLUTION;
	m_config.screen_w = 640;
	m_config.screen_h = 480;
	ZeroMemory(m_sBuffer, sizeof(m_sBuffer));
}

/**
 * デストラクタ
 */
CComWacom::~CComWacom()
{
	if(g_lpfnDefProc){
		SetWindowLongPtr(m_hwndMain, GWLP_WNDPROC, (LONG_PTR)g_lpfnDefProc);
		g_lpfnDefProc = NULL;
	}
	FinalizeTabletDevice();
	g_cmwacom = NULL;
}

/**
 * 初期化
 * @retval true 成功
 * @retval false 失敗
 */
bool CComWacom::Initialize(HWND hWnd)
{
	if(!g_wacom_initialized){
		return false; // 初期化されていない
	}
	if(g_wacom_allocated){
		return false; // 複数個の利用は不可
	}

	if (!gpWTInfoA || !gpWTInfoA(0, 0, NULL)) {
		return false; // WinTab使用不可
	}
	
	m_hwndMain = hWnd;
	
	if(m_hwndMain && !g_lpfnDefProc){
		g_lpfnDefProc = (WNDPROC)GetWindowLongPtr(m_hwndMain, GWLP_WNDPROC);
		SetWindowLongPtr(m_hwndMain, GWLP_WNDPROC, (LONG_PTR)tabletWndProc);
	}

	g_cmwacom = this;

	SetExclusiveMode(g_exclusivemode);
	SetNCControl(g_nccontrol);

	InitializeTabletDevice();
	
	return true;
}
void CComWacom::GetConfig(CMWACOM_CONFIG *cfg){
	*cfg = m_config;
}
void CComWacom::SetConfig(CMWACOM_CONFIG cfg){
	m_config = cfg;
}

void CComWacom::InitializeTabletDevice(){
	LOGCONTEXTA lcMine;
	AXIS axis;
	AXIS pressAxis;
	AXIS rotAxis[3] = {0};
	
	if(!g_wacom_initialized){
		return; // 初期化されていない
	}
	if(g_wacom_allocated){
		return; // 複数個の利用は不可
	}

	if (!gpWTInfoA(0, 0, NULL)) {
		return; // WinTab使用不可
	}
	
	if(m_exclusivemode){
		gpWTInfoA(WTI_DEVICES, DVC_X, &axis);
		m_minX = axis.axMin;
		m_maxX = axis.axMax; /* Ｘ方向の最大座標 */
		m_resX = axis.axResolution; /* Ｘ座標の分解能 単位:line/inch */
		gpWTInfoA(WTI_DEVICES, DVC_Y, &axis);
		m_minY = axis.axMin;
		m_maxY = axis.axMax; /* Ｙ方向の最大座標 */
		m_resY = axis.axResolution; /* Ｙ座標の分解能 単位:line/inch */
		gpWTInfoA(WTI_DEFSYSCTX, 0, &lcMine);
	}

	gpWTInfoA(m_exclusivemode ? WTI_DEFCONTEXT : WTI_DEFSYSCTX, 0, &lcMine);
	lcMine.lcOptions |= CXO_MESSAGES|CXO_MARGIN; /* Wintabﾒｯｾｰｼﾞが渡されるようにする */
	lcMine.lcMsgBase = WT_DEFBASE;
	lcMine.lcPktData = PACKETDATA;
	lcMine.lcPktMode = PACKETMODE;
	lcMine.lcMoveMask = PACKETDATA;
	lcMine.lcBtnUpMask = lcMine.lcBtnDnMask;
	if(m_exclusivemode){
		lcMine.lcInOrgX = m_minX;
		lcMine.lcInOrgY = m_minY;
		lcMine.lcInExtX = m_maxX; /* ﾀﾌﾞﾚｯﾄの有効範囲全域を操作ｴﾘｱとします */
		lcMine.lcInExtY = m_maxY;
		lcMine.lcOutOrgX = 0;
		lcMine.lcOutOrgY = 0;
		lcMine.lcOutExtX = m_maxX-m_minX; /* ﾀﾌﾞﾚｯﾄの分解能と入力ﾃﾞｰﾀの分解能を */
		lcMine.lcOutExtY = m_maxY-m_minY; /* あわせます */

	}else{
		m_minX = lcMine.lcOutOrgX;
		m_minY = lcMine.lcOutOrgY;
		m_maxX = lcMine.lcOutExtX;
		m_maxY = lcMine.lcOutExtY;
	}
	if(g_fakeContext){
		gpWTEnable(g_fakeContext, FALSE);
	}
	m_hTab = gpWTOpenA(m_hwndMain, &lcMine, GetForegroundWindow()==m_hwndMain ? TRUE : FALSE);
	if (!m_hTab)
	{
		return;
	}
	// WTI_DEVICESのDVC_NPRESSUREを取得 （筆圧値の最大、最小）
	gpWTInfoA(WTI_DEVICES, DVC_NPRESSURE, &pressAxis);
	m_rawPressureMax = pressAxis.axMax;
	m_rawPressureMin = pressAxis.axMin;
	gpWTInfoA(WTI_DEVICES, DVC_TPRESSURE, &pressAxis);
	m_rawTangentPressureMax = pressAxis.axMax;
	m_rawTangentPressureMin = pressAxis.axMin;
	gpWTInfoA(WTI_DEVICES, DVC_ROTATION, rotAxis);
	m_rawAzimuthMax = rotAxis[0].axMax;
	m_rawAzimuthMin = rotAxis[0].axMin;
	m_rawAltitudeMax = rotAxis[1].axMax;
	m_rawAltitudeMin = rotAxis[1].axMin;
	m_rawTwistMax = rotAxis[2].axMax;
	m_rawTwistMin = rotAxis[2].axMin;

    m_hMgr = gpWTMgrOpen(m_hwndMain, WT_DEFBASE);
    if(m_hMgr) {
        m_ObtBuf[0] = 1;
        gpWTMgrExt(m_hMgr, WTX_OBT, m_ObtBuf);
    }
	
	// 偽lastdata
	m_lastdata[0] = 0xA0;
	m_lastdata[1] = 0x00;
	m_lastdata[2] = 0x00;
	m_lastdata[3] = 0x00;
	m_lastdata[4] = 0x00;
	m_lastdata[5] = 0x00;
	m_lastdata[6] = 0x00;
	m_lastdatalen = 7;
}
void CComWacom::FinalizeTabletDevice(){
	if (m_hTab)
	{
		if (m_hMgr) {
			// Out of Bounds Tracking の解除
			m_ObtBuf[0] = 0;
			gpWTMgrExt(m_hMgr, WTX_OBT, m_ObtBuf);
			gpWTMgrClose(m_hMgr);
			m_hMgr = NULL;
		}
		gpWTClose(m_hTab);
		m_hTab = NULL;
		g_wacom_allocated = false;
		
		if(g_fakeContext){
			gpWTEnable(g_fakeContext, TRUE);
		}
	}
}

HCTX CComWacom::GetHTab(){
	return m_hTab;
}
bool CComWacom::HandlePacketMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	PACKET tPckt;
	static SINT32 last_rawButtons;
	
	m_fixedaspect = (np2oscfg.pentabfa ? true : false);

	if (gpWTPacket((HCTX)LOWORD(lParam), (UINT)wParam, (LPSTR)&tPckt)) {
		bool proximityflag = false;
		int newbuttons;
		if(!m_config.enable){
			m_skipmouseevent = 0;
			return true;
		}
		if(!m_exclusivemode && !m_nccontrol){
			m_skipmouseevent = 0;
			return true;
		}
		if(m_skiptabletevent>0){
			m_skiptabletevent--;
			return true;
		}
		if(tPckt.pkStatus & TPS_PROXIMITY){
			proximityflag = true;
		}
		
		m_skipmouseevent = 2; // WinTabのバグ回避

		m_rawX = tPckt.pkX; /* デジタイザ上のＸ座標 */
		m_rawY = tPckt.pkY; /* デジタイザ上のＹ座標 */
		newbuttons = LOWORD(tPckt.pkButtons); /* ボタン番号 */
		m_rawStatus = tPckt.pkStatus;

		m_rawPressure = LOWORD(tPckt.pkNormalPressure); /* 筆圧 */
		if(m_rawPressureMax-m_rawPressureMin > 0){
			if(m_rawPressure <= m_rawPressureMin){
				m_pressure = 0.0;
			}else{
				m_pressure = (double)(m_rawPressure-m_rawPressureMin)/(m_rawPressureMax-m_rawPressureMin);
			}
		}else{
			m_pressure = 0.0;
		}
		
		if((tPckt.pkStatus & TPS_INVERT)){
			newbuttons |= 0x4;
		}
		m_rawButtons = newbuttons;

		m_rawTangentPressure = tPckt.pkTangentPressure;
		if(m_rawTangentPressureMax>0){
			if(m_rawTangentPressure < m_rawTangentPressureMin){
				m_tangentPressure = 0.0;
			}else{
				m_tangentPressure = (double)(m_rawTangentPressure-m_rawTangentPressureMin)/(m_rawTangentPressureMax-m_rawTangentPressureMin);
			}
		}else{
			m_tangentPressure = 0.0;
		}
			   
		m_rawAzimuth = tPckt.pkOrientation.orAzimuth;
		if(m_rawAzimuthMax>0){
			m_azimuth = (double)(m_rawAzimuth)/m_rawAzimuthMax;
		}else{
			m_azimuth = 0.0;
		}

		m_rawAltitude = tPckt.pkOrientation.orAltitude;
		if(m_rawAltitudeMax>0){
			m_altitude = (double)(m_rawAltitude)/m_rawAltitudeMax;
		}else{
			m_altitude = 0.0;
		}

		m_rawTwist = tPckt.pkOrientation.orTwist;
		if(m_rawTwistMax>0){
			m_twist = (double)(m_rawTwist)/m_rawTwistMax;
			m_rotationDeg = (double)(m_rawTwist*360)/m_rawTwistMax;
		}else{
			m_twist = 0.0;
			m_rotationDeg = 0.0;
		}

		if(proximityflag){
			m_pressure = 0.0;
		}

		m_tiltX = m_altitude * cos(m_azimuth*2*M_PI);
		m_tiltY = m_altitude * sin(m_azimuth*2*M_PI);

		if(m_wait==0){
			UINT16 pktPressure;
			SINT32 pktXtmp, pktYtmp;
			UINT16 pktX, pktY;
			SINT32 tablet_resX = TABLET_RAWMAX_X * m_config.resolution_w / TABLET_BASERASOLUTION;
			SINT32 tablet_resY = TABLET_RAWMAX_Y * m_config.resolution_h / TABLET_BASERASOLUTION;
			char buf[32];
			pktPressure = (UINT32)(m_pressure * 255);
			// ペンON/OFFそれぞれで255段階（らしい）
			if(!m_config.disablepressure){
				if(m_pressure < 0.5){
					m_rawButtons &= ~0x1;
					pktPressure = pktPressure = (UINT32)((m_pressure) / (1.0 - 0.5) * 255);
				}else{
					pktPressure = pktPressure = (UINT32)((m_pressure - 0.5) / (1.0 - 0.5) * 255);
				}
			}
			if(m_exclusivemode){
				// 排他モード（マウスキャプチャ中）
				if(m_config.scrnsizemode){
					tablet_resX = m_config.screen_w;
					tablet_resY = m_config.screen_h;
				}
				if(m_fixedaspect){
					// アスペクト比がArtPad IIと同じになるように修正
					if(tablet_resX * (m_maxY - m_minY) > tablet_resY * (m_maxX - m_minX)){
						pktXtmp = (m_rawX * tablet_resX / (m_maxX - m_minX));
						pktYtmp = tablet_resY - (m_rawY * tablet_resX / (m_maxX - m_minX)); // 左下原点ですってよ
					}else{
						pktXtmp = (m_rawX * tablet_resY / (m_maxY - m_minY));
						pktYtmp = tablet_resY - (m_rawY * tablet_resY / (m_maxY - m_minY)); // 左下原点ですってよ
					}
				}else{
					pktXtmp = (m_rawX * tablet_resX / (m_maxX - m_minX));
					pktYtmp = tablet_resY - (m_rawY * tablet_resY / (m_maxY - m_minY)); // 左下原点ですってよ
				}
			}else{
				// ホストマウス連動モード（マウスキャプチャなし操作モード）
				RECT rectClient;
				POINT pt;
				scrnmng_getrect(&rectClient);
				pt.x = rectClient.left;
				pt.y = rectClient.top;
				ClientToScreen(hWnd, &pt);
				if(m_config.scrnsizemode){
					tablet_resX = m_config.screen_w;
					tablet_resY = m_config.screen_h;
				}
				if(rectClient.right - rectClient.left != 0 && rectClient.bottom - rectClient.top != 0){
					pktXtmp = ((m_rawX - pt.x) * tablet_resX / (rectClient.right - rectClient.left));
					pktYtmp = (((m_maxY - m_rawY) - pt.y) * tablet_resY / (rectClient.bottom - rectClient.top));
				}else{
					pktXtmp = m_rawX - pt.x;
					pktYtmp = m_rawY - pt.y;
				}
				if(pktXtmp < 0 || pktYtmp < 0 || pktXtmp > tablet_resX || pktYtmp > tablet_resY){
					// 範囲外は移動のみ可
					m_rawButtons = m_rawButtons & 0x4;
					pktPressure = 0;
					proximityflag = true;
				}
			}
			// はみ出さないように座標範囲を修正
			if(pktXtmp < 0) pktXtmp = 0;
			if(pktYtmp < 0) pktYtmp = 0;
			if(pktXtmp > tablet_resX) pktXtmp = tablet_resX;
			if(pktYtmp > tablet_resY) pktYtmp = tablet_resY;
			
			if(m_config.relmode){
				// 相対座標モード
				if(g_lastPosValid){
					pktX = (SINT16)((SINT32)pktXtmp - g_lastPosX);
					pktY = (SINT16)((SINT32)pktYtmp - g_lastPosY);
				}else{
					pktX = 0;
					pktY = 0;
				}
			}else{
				// 絶対座標モード
				pktX = (UINT16)pktXtmp;
				pktY = (UINT16)pktYtmp;
			}

			if(m_config.disablepressure && m_config.csvmode){
				// CSV座標モード（筆圧無効モードでないと使えない）
				int slen = sprintf(buf, "#,%05d,%05d,%02d\r\n", pktX, pktY, proximityflag ? 99 : m_rawButtons);
				if(slen > 0){
					memcpy(m_lastdata, buf, slen);
					m_lastdatalen = slen;
					if(SendDataToReadBuffer(buf, slen)){
						//m_wait += 0;
					}
				}
			}else{
				if(m_config.disablepressure){
					// 筆圧無効モード
					buf[0] = 0xe0|((pktX >> 14) & 0x3);
					buf[1] = ((pktX >> 7) & 0x7f);
					buf[2] = (pktX & 0x7f);
					buf[3] = ((pktY >> 14) & 0x3);
					buf[4] = ((pktY >> 7) & 0x7f);
					buf[5] = (pktY & 0x7f);
					buf[6] = m_rawButtons;
				}else{
					// 通常モード
					buf[0] = (proximityflag ? 0xA0 : 0xE0)|((m_rawButtons & ~0x1) ? 0x8 : 0)|((pktX >> 14) & 0x3)|((((pktPressure) & 0) << 2));
					buf[1] = ((pktX >> 7) & 0x7f);
					buf[2] = (pktX & 0x7f);
					buf[3] = ((m_rawButtons & 0xf) << 3)|((pktY >> 14) & 0x3)|((((pktPressure >> 1) & 0) << 2));
					buf[4] = ((pktY >> 7) & 0x7f);
					buf[5] = (pktY & 0x7f);
					buf[6] = (((m_rawButtons & ~0x4) ? 0x00 : 0x40))|((pktPressure >> 2)&0x3f);
				}
				memcpy(m_lastdata, buf, 7);
				m_lastdatalen = 7;
				if(SendDataToReadBuffer(buf, 7)){
					//m_wait += 7;
				}
			}
			g_lastPosX = pktXtmp;
			g_lastPosY = pktYtmp;
			g_lastPosValid = true;
		}
	}

	return true;
}
bool CComWacom::HandleMouseMoveMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	if(!m_config.enable) return false;
	if(m_skipmouseevent > 0){
		m_skipmouseevent--;
		return true;
	}
	return false;
}
bool CComWacom::HandleMouseUpMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	if(!m_config.enable) return false;
	if(!m_exclusivemode && m_skipmouseevent > 0){
		return true;
	}
	return false;
}
bool CComWacom::HandleMouseDownMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	if(!m_config.enable) return false;
	if(!m_exclusivemode && m_skipmouseevent > 0){
		return true;
	}
	return false;
}
bool CComWacom::SkipMouseEvent(){
	return (m_skipmouseevent > 0);
}
void CComWacom::SetExclusiveMode(bool enable){
	if(m_exclusivemode != enable){
		FinalizeTabletDevice();
		m_exclusivemode = enable;
		InitializeTabletDevice();
	}
}
void CComWacom::SetNCControl(bool enable){
	m_nccontrol = enable;
}

bool CComWacom::SendDataToReadBuffer(const char *data, int len){
	return SendDataToReadBuffer((const UINT8 *)data, len);
}
bool CComWacom::SendDataToReadBuffer(const UINT8 *data, int len){
	int bufused = (m_sBuffer_wpos - m_sBuffer_rpos) & (WACOM_BUFFER - 1);
	if(bufused + len >= WACOM_BUFFER){
#if defined(SUPPORT_RS232C_FIFO)
		if(rs232cfifo.port138 & 0x1){
			// 催促しない
		}else 
#endif
		{
			// Buffer Full
			if (sysport.c & 1) {
				pic_setirq(4); // XXX: 催促
			}
		}
		return false;
	}
	for(int i=0;i<len;i++){
		m_sBuffer[m_sBuffer_wpos] = *data;
		data++;
		m_sBuffer_wpos = (m_sBuffer_wpos + 1) & (WACOM_BUFFER - 1);
	}
	return true;
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComWacom::Read(UINT8* pData)
{
	static int nodatacounter = 0;
	DWORD datatime = GetTickCount();
	if(m_wait > 0){
		m_wait--;
		g_datatime = datatime;
		return 0;
	}
	if(datatime - g_datatime > 500){
		// データが古いので捨てましょう
		m_sBuffer_rpos = m_sBuffer_wpos;
		g_datatime = datatime;
		return 0;
	}
	g_datatime = datatime;
	if(m_sBuffer_wpos != m_sBuffer_rpos){
		nodatacounter = 0;
		*pData = m_sBuffer[m_sBuffer_rpos];
		m_sBuffer_rpos = (m_sBuffer_rpos + 1) & (WACOM_BUFFER - 1);
		return 1;
	}else{
		nodatacounter++;
		//// XXX: 長期間データがないとWin9xでおかしくなるようなので暫定対策
		//if(m_config.mode19200 && nodatacounter > 256){
		//	if(m_lastdatalen > 0){
		//		char buf[10];
		//		memcpy(buf, m_lastdata, 7);
		//		if(m_rawButtons != 0) {
		//			buf[0] |= 0x40;
		//		}else{
		//			buf[0] &= ~0x40;
		//		}
		//		if(m_config.start) SendDataToReadBuffer(buf, m_lastdatalen);
		//	}
		//	nodatacounter = 0;
		//}
		g_lastPosValid = false;
	}
	return 0;
}

/**
 * 書き込み
 * @param[out] cData データ
 * @return サイズ
 */
UINT CComWacom::Write(UINT8 cData)
{
	CMWACOM wtab = this;

	if(m_cmdbuf_pos == WACOM_CMDBUFFER){
		m_cmdbuf_pos = 0; // Buffer Full!
	}
	if (m_hTab)
	{
		if(cData=='\r' || cData=='\n'){ // コマンド終端の時（XXX: LFもコマンド終端扱い？）
			m_cmdbuf[m_cmdbuf_pos] = '\0';
			if(strcmp(m_cmdbuf, "#")==0){
				// Reset to protocol IV command set
				m_sBuffer_rpos = m_sBuffer_wpos; // データ放棄
				m_config.scrnsizemode = false;
				m_config.disablepressure = false;
				m_config.csvmode = false;
				m_config.relmode = false;
				m_config.start = true; // Start sending coordinates
				m_config.mode19200 = false;
			}else if(strcmp(m_cmdbuf, "#~*F202C800")==0){
				// 19200 bps mode
				m_sBuffer_rpos = m_sBuffer_wpos; // データ放棄
				m_config.scrnsizemode = false;
				m_config.disablepressure = false;
				m_config.csvmode = false;
				m_config.relmode = false;
				m_config.start = true; // Start sending coordinates
				m_config.mode19200 = true;
			}else if(strncmp(m_cmdbuf, "~*F2039100,000,00,1000,1000", 3)==0){
				// 19200 bps mode & Enable Pressure (win3.1)
				m_sBuffer_rpos = m_sBuffer_wpos; // データ放棄
				m_config.scrnsizemode = false;
				m_config.disablepressure = false;
				m_config.csvmode = false;
				m_config.relmode = false;
				m_config.start = true; // Start sending coordinates
				m_config.mode19200 = true;
			}else if(strncmp(m_cmdbuf, "~*E2039100,000,00,1000,1000", 3)==0){
				// 9600 bps mode & Enable Pressure (win3.1)
				m_sBuffer_rpos = m_sBuffer_wpos; // データ放棄
				m_config.scrnsizemode = false;
				m_config.disablepressure = false;
				m_config.csvmode = false;
				m_config.relmode = false;
				m_config.start = true; // Start sending coordinates
				m_config.mode19200 = false;
			}else if(strcmp(m_cmdbuf, "$")==0){
				// Reset to 9600 bps (sent at 19200 bps)? & Disable Pressure
				m_sBuffer_rpos = m_sBuffer_wpos; // データ放棄
				m_config.scrnsizemode = false;
				m_config.disablepressure = true;
				m_config.mode19200 = false; // 違うかも
			}else if(strncmp(m_cmdbuf, "ST", 2)==0){
				m_config.start = true; // Start sending coordinates
			}else if(strncmp(m_cmdbuf, "@ST", 2)==0){
				// Start sending coordinates & get current position
				UINT8 data[] = {0xA0};
				if(m_lastdatalen > 0){
					char buf[10];
					memcpy(buf, m_lastdata, 7);
					if(m_rawButtons != 0) {
						buf[0] |= 0x40;
					}else{
						buf[0] &= ~0x40;
					}
					SendDataToReadBuffer(buf, m_lastdatalen);
				}else{
					SendDataToReadBuffer(data, sizeof(data));
				}
				m_config.start = true;
			}else if(strncmp(m_cmdbuf, "SP", 2)==0){
				m_config.start = false; // Stop sending coordinates
			}else if(strcmp(m_cmdbuf, "~R")==0){
				SendDataToReadBuffer(cmwacom_RData, sizeof(cmwacom_RData));
				if(m_wait < WACOM_BUFFER) m_wait += sizeof(cmwacom_RData)*2;
			}else if(strcmp(m_cmdbuf, "~C")==0){
				SendDataToReadBuffer(cmwacom_CData, sizeof(cmwacom_CData));
				if(m_wait < WACOM_BUFFER) m_wait += sizeof(cmwacom_CData)*2;
			}else if(strncmp(m_cmdbuf, "NR", 2)==0){
				// Set Resolution
				m_config.resolution_w = m_config.resolution_h = atoi(m_cmdbuf + 2);
				m_config.scrnsizemode = false;
			}else if(strcmp(m_cmdbuf, "AS0")==0){
				m_config.csvmode = true;
			}else if(strcmp(m_cmdbuf, "AS1")==0){
				m_config.csvmode = false;
			}else if(strcmp(m_cmdbuf, "DE0")==0){
				m_config.relmode = false;
			}else if(strcmp(m_cmdbuf, "DE1")==0){
				m_config.relmode = true;
			}else if(strcmp(m_cmdbuf, "SU01")==0){
				m_config.suppress = true;
			}else if(strcmp(m_cmdbuf, "SU00")==0){
				m_config.suppress = false;
			}else if(strncmp(m_cmdbuf, "SC", 2)==0){
				// Set Screen Size?
				int w, h;
				char *spos = strchr(m_cmdbuf, ',');
				if(spos){
					w = atoi(m_cmdbuf + 2);
					h = atoi(spos + 1);
					if(w > 0 && h > 0){
						m_config.screen_w = w;
						m_config.screen_h = h;
						m_config.scrnsizemode = true;
					}
				}
			}else if(strncmp(m_cmdbuf, "TE", 2)==0){
				// I'm fine!
				char data[256];
				if(strlen(m_cmdbuf) <= 2){ // TE only
					sprintf(data, "KT-0405-R00 V1.3-2 95/04/28 by WACOM\r\nI AM FINE.\r\n");
				}else{ // TExxxx 4文字を越えた部分は捨てる
					sprintf(data, "KT-0405-R00 V1.3-2 95/04/28 by WACOM\r\n%.4s\r\nI AM FINE.\r\n", m_cmdbuf + 2);
				}
				m_sBuffer_rpos = m_sBuffer_wpos; //バッファ消す
				SendDataToReadBuffer(data, (int)strlen(data));
				m_lastdatalen = 0;
				m_wait = sizeof(data);
				m_config.enable = true;
				m_config.mode19200 = false;
			}
			m_cmdbuf_pos = 0;
		}else{
			m_cmdbuf[m_cmdbuf_pos] = cData;
			m_cmdbuf_pos++;
			if(m_cmdbuf_pos >= 2 && strncmp(&m_cmdbuf[m_cmdbuf_pos-2], "~#", 2)==0){ // 例外的に即応答
				m_sBuffer_rpos = m_sBuffer_wpos; //バッファ消す
				SendDataToReadBuffer(cmwacom_ModelData, sizeof(cmwacom_ModelData));
				//if(m_wait < WACOM_BUFFER) m_wait += sizeof(cmwacom_ModelData)*2;
//#if defined(SUPPORT_RS232C_FIFO)
//				if(rs232cfifo.port138 & 0x1){
//					// FIFOモードではウェイトなし
//					m_wait = 0;
//				}else
//#endif
				{
					m_wait = 4;
				}
				m_cmdbuf_pos = 0;
				m_config.enable = true;
			}
		}
//#if defined(SUPPORT_RS232C_FIFO)
//		if(rs232cfifo.port138 & 0x1){
//			// FIFOモードではウェイトなし
//			m_wait = 0;
//		}
//#endif
	}
	return 1;
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
UINT8 CComWacom::GetStat()
{
	//if(m_sBuffer_wpos != m_sBuffer_rpos){
	//	if(!m_dcdflag){
	//		m_dcdflag = true;
	//		return 0x20;
	//	}else{
	//		return 0x00;
	//	}
	//}else{
		return 0xa0;
	//}
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComWacom::Message(UINT nMessage, INTPTR nParam)
{
	switch (nMessage)
	{
		case COMMSG_PURGE:
			{
				m_sBuffer_rpos = m_sBuffer_wpos; //バッファ消す
			}
			break;

		case COMMSG_SETFLAG:
			{
				COMFLAG flag = reinterpret_cast<COMFLAG>(nParam);
				if ((flag) && (flag->size == sizeof(_COMFLAG) + sizeof(m_config)) && (flag->sig == COMSIG_MIDI))
				{
					CopyMemory(&m_config, flag + 1, sizeof(m_config));
					return 1;
				}
			}
			break;

		case COMMSG_GETFLAG:
			{
				COMFLAG flag = (COMFLAG)_MALLOC(sizeof(_COMFLAG) + sizeof(m_config), "PENTAB FLAG");
				if (flag)
				{
					flag->size = sizeof(_COMFLAG) + sizeof(m_config);
					flag->sig = COMSIG_MIDI;
					flag->ver = 0;
					flag->param = 0;
					CopyMemory(flag + 1, &m_config, sizeof(m_config));
					return reinterpret_cast<INTPTR>(flag);
				}
			}
			break;

		default:
			break;
	}
	return 0;
}

#endif
