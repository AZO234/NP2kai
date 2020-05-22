/**
 * @file	cmwacom.h
 * @brief	Wacom Tablet クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "cmbase.h"

#ifdef SUPPORT_WACOM_TABLET

#include "wintab/MSGPACK.H"
#include "wintab/WINTAB.H"
#define PACKETDATA			(PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_ORIENTATION | PK_CURSOR | PK_STATUS | PK_TANGENT_PRESSURE | PK_ORIENTATION)
#define PACKETMODE			0
#include "wintab/PKTDEF.H"
#include "wintab/Utils.h"

#pragma pack(1)
typedef struct tagCMWACOM_CONFIG {
	bool enable; // ペンタブモード有効（猫マウス操作不可）
	bool start; // STでtrue, SPでfalse（座標送信再開／一時停止？）
	bool scrnsizemode; // 画面サイズ指定モード
	bool disablepressure; // 筆圧無効モード（データ形式が変わる）
	bool relmode; // 相対座標モード
	bool csvmode; // CSV座標モード（筆圧無効モードでのみ有効）
	bool suppress; // 抑制モード（同じ座標値は送らない）
	bool mode19200; // 19200bpsモード
	SINT32 resolution_w;
	SINT32 resolution_h;
	SINT32 screen_w;
	SINT32 screen_h;
} CMWACOM_CONFIG;
#pragma pack()

void cmwacom_initialize(void);
void cmwacom_reset(void);
void cmwacom_finalize(void);
bool cmwacom_skipMouseEvent(void);
void cmwacom_setNCControl(bool enable);

/**
 * @brief commng Wacom Tablet デバイス クラス
 */
class CComWacom : public CComBase
{
public:
	static CComWacom* CreateInstance(HWND hWnd);
	void InitializeTabletDevice();
	void FinalizeTabletDevice();
	HCTX GetHTab();
	bool HandlePacketMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	bool HandleMouseMoveMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	bool HandleMouseUpMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	bool HandleMouseDownMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	bool SkipMouseEvent();
	void SetExclusiveMode(bool enable);
	void SetNCControl(bool enable);
	void GetConfig(CMWACOM_CONFIG *cfg);
	void SetConfig(CMWACOM_CONFIG cfg);
	
	int m_skiptabletevent;

protected:
	CComWacom();
	virtual ~CComWacom();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT msg, INTPTR param);

private:
	enum
	{
		WACOM_BUFFER			= (1 << 6),
		WACOM_CMDBUFFER			= (1 << 8),
	};

	bool m_fixedaspect;

	HWND m_hwndMain;				/*!< Main Window Handle */
	HCTX m_hTab;					/*!< WinTab Handle */
	HMGR m_hMgr;
	BOOL m_ObtBuf[64];
	UINT8 m_sBuffer[WACOM_BUFFER];	/*!< バッファ */
	SINT32 m_sBuffer_wpos;			/*!< バッファ書き込み位置 */
	SINT32 m_sBuffer_rpos;			/*!< バッファ読み込み位置 */

	UINT8 m_lastdata[32];
	SINT32 m_lastdatalen;
	
	int m_skipmouseevent;
	bool m_exclusivemode;
	bool m_nccontrol;
	
	SINT32 m_wait;
	bool m_sendlastdata; // 最後のデータを再送信する
	
	SINT32 m_mousedown;
	SINT32 m_mouseX;
	SINT32 m_mouseY;

	char m_cmdbuf[WACOM_CMDBUFFER];
	SINT32 m_cmdbuf_pos;
	
	CMWACOM_CONFIG m_config;
	//SINT32 m_resolution_w;
	//SINT32 m_resolution_h;

	//bool m_scrnsizemode;
	//SINT32 m_screen_w;
	//SINT32 m_screen_h;
	
	SINT32 m_minX;
	SINT32 m_maxX;
	SINT32 m_resX;
	SINT32 m_minY;
	SINT32 m_maxY;
	SINT32 m_resY;

	SINT32 m_rawX;
	SINT32 m_rawY;
	SINT32 m_rawButtons;
	
	SINT32 m_rawPressure;
	SINT32 m_rawPressureMax;
	SINT32 m_rawPressureMin;
	double m_pressure;
	
	SINT32 m_rawTangentPressure;
	SINT32 m_rawTangentPressureMax;
	SINT32 m_rawTangentPressureMin;
	double m_tangentPressure;
	
	SINT32 m_rawAzimuth;
	SINT32 m_rawAzimuthMax;
	SINT32 m_rawAzimuthMin;
	double m_azimuth;
	
	SINT32 m_rawAltitude;
	SINT32 m_rawAltitudeMax;
	SINT32 m_rawAltitudeMin;
	double m_altitude;
	
	SINT32 m_rawTwist;
	SINT32 m_rawTwistMax;
	SINT32 m_rawTwistMin;
	double m_twist;
	
	double m_tiltX;
	double m_tiltY;
	
	double m_rotationDeg;
	
	SINT32 m_rawStatus;
	
	bool Initialize(HWND hWnd);
	
	bool SendDataToReadBuffer(const char *data, int len);
	bool SendDataToReadBuffer(const UINT8 *data, int len);
};

#endif
