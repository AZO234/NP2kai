/**
 * @file	sdasio.h
 * @brief	ASIO オーディオ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "asio\asiosdk.h"
#include "asio\asiodriverlist.h"
#include "sdbase.h"

/**
 * @brief ASIO オーディオ クラス
 */
class CSoundDeviceAsio : public CSoundDeviceBase
{
public:
	static void Initialize();
	static void EnumerateDevices(std::vector<LPCTSTR>& devices);

	CSoundDeviceAsio();
	virtual ~CSoundDeviceAsio();
	virtual bool Open(LPCTSTR lpDevice = NULL, HWND hWnd = NULL);
	virtual void Close();
	virtual UINT CreateStream(UINT nSamplingRate, UINT nChannels, UINT nBufferSize = 0);
	virtual void DestroyStream();
	virtual bool PlayStream();
	virtual void StopStream();

private:
	static CSoundDeviceAsio* sm_pInstance;			/*!< 現在のインスタンス */
	IASIO* m_pAsioDriver;							/*!< ASIO ドライバ */
	UINT m_nBufferLength;							/*!< バッファ サイズ */
	std::vector<ASIOBufferInfo> m_bufferInfo;		/*!< バッファ */
	ASIOCallbacks m_callback;						/*!< コールバック */
	static AsioDriverList sm_asioDriverList;		/*!< ドライバ リスト */
	static void cBufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
	static void cSampleRateDidChange(ASIOSampleRate sRate);
	static long cAsioMessage(long selector, long value, void* message, double* opt);
	static ASIOTime* cBufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
	void BufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
};
