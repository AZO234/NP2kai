/**
 * @file	soundmng.h
 * @brief	サウンド マネージャ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * PCM 番号
 */
enum SoundPCMNumber
{
	SOUND_PCMSEEK		= 0,		/*!< ヘッド移動 */
	SOUND_PCMSEEK1,					/*!< 1クラスタ移動 */
	SOUND_RELAY1					/*!< リレー */
};

#ifdef __cplusplus
extern "C"
{
#endif
	
UINT soundmng_create(UINT rate, UINT ms);
void soundmng_destroy(void);
void soundmng_reset(void);
void soundmng_play(void);
void soundmng_stop(void);
#define soundmng_sync()
void soundmng_setreverse(BOOL bReverse);
void soundmng_setvolume(int nVolume);

BRESULT soundmng_pcmplay(enum SoundPCMNumber nNum, BOOL bLoop);
void soundmng_pcmstop(enum SoundPCMNumber nNum);

#ifdef __cplusplus
}

#include "soundmng\sdbase.h"

/**
 * サウンド プロシージャ
 */
enum SoundProc
{
	SNDPROC_MASTER		= 0,
	SNDPROC_MAIN,
	SNDPROC_TOOL,
	SNDPROC_SUBWIND,
	SNDPROC_USER
};

/**
 * @brief サウンド マネージャ クラス
 */
class CSoundMng : public ISoundData
{
public:
	/**
	 * デバイス タイプ
	 */
	enum DeviceType
	{
		kDefault			= 0,	/*!< Default */
		kDSound3,					/*!< Direct Sound3 */
		kWasapi,					/*!< WASAPI */
		kAsio						/*!< ASIO */
	};

	static CSoundMng* GetInstance();
	static void Initialize();
	static void Deinitialize();

	CSoundMng();
	bool Open(DeviceType nType, LPCTSTR lpName, HWND hWnd);
	void Close();
	void Enable(SoundProc nProc);
	void Disable(SoundProc nProc);
	UINT CreateStream(UINT nSamplingRate, UINT ms);
	void DestroyStream();
	void ResetStream();
	void PlayStream();
	void StopStream();
	void SetReverse(bool bReverse);
	void SetMasterVolume(int nVolume);
	void LoadPCM(SoundPCMNumber nNum, LPCTSTR lpFilename);
	void ReloadPCM(SoundPCMNumber nNum);
	void SetPCMVolume(SoundPCMNumber nNum, int nVolume);
	bool PlayPCM(SoundPCMNumber nNum, BOOL bLoop);
	void StopPCM(SoundPCMNumber nNum);
	virtual UINT Get16(SINT16* lpBuffer, UINT nBufferCount);

private:
	static CSoundMng sm_instance;		//!< 唯一のインスタンスです
	
	void InitializeSoundCriticalSection();
	void FinalizeSoundCriticalSection();
	void EnterSoundCriticalSection();
	void LeaveSoundCriticalSection();
	void EnterAllCriticalSection();
	void LeaveAllCriticalSection();

	/**
	 * satuation関数型宣言
	 */
	typedef void (PARTSCALL * FNMIX)(SINT16*, const SINT32*, UINT);

	CSoundDeviceBase* m_pSoundDevice;	//!< サウンド デバイス
	UINT m_nMute;						//!< ミュート フラグ
	FNMIX m_fnMix;						//!< satuation関数ポインタ
	
	bool m_sound_cs_initialized;						//!< クリティカルセクション 初期化済みフラグ
	CRITICAL_SECTION m_sound_cs;						//!< クリティカルセクション
};

/**
 * インスタンスを得る
 * @return インスタンス
 */
inline CSoundMng* CSoundMng::GetInstance()
{
	return &sm_instance;
}

#endif
