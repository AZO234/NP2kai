/**
 * @file	sdwasapi.h
 * @brief	WASAPI オーディオ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>
#include "sdbase.h"
#include "misc\threadbase.h"

/**
 * @brief デバイス
 */
struct WasapiDevice
{
	LPWSTR id;							//!< ID
	TCHAR szDevice[MAX_PATH];			//!< デバイス
};

/**
 * @brief WASAPI クラス
 */
class CSoundDeviceWasapi : public CSoundDeviceBase, protected CThreadBase
{
public:
	static void Initialize();
	static void Deinitialize();
	static void EnumerateDevices(std::vector<LPCTSTR>& devices);

	CSoundDeviceWasapi();
	virtual ~CSoundDeviceWasapi();
	virtual bool Open(LPCTSTR lpDevice = NULL, HWND hWnd = NULL);
	virtual void Close();
	virtual UINT CreateStream(UINT nSamplingRate, UINT nChannels, UINT nBufferSize = 0);
	virtual void DestroyStream();
	virtual bool PlayStream();
	virtual void StopStream();

protected:
	virtual bool Task();

private:
	static std::vector<WasapiDevice> sm_devices;	//!< デバイス リスト

	IMMDeviceEnumerator* m_pEnumerator;			//!< デバイス列挙インスタンス
	IMMDevice* m_pDevice;						//!< デバイス インスタンス
	IAudioClient* m_pAudioClient;				//!< オーディオ クライアント インスタンス
	WAVEFORMATEX* m_pwfx;						//!< フォーマット
	IAudioRenderClient* m_pRenderClient;		//!< オーディオ レンダラー クライアント インスタンス
	UINT32 m_nBufferSize;						//!< バッファ サイズ
	HANDLE m_hEvents[2];						//!< イベント
	void ResetStream();
	void FillStream();
};
