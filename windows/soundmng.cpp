/**
 * @file	soundmng.cpp
 * @brief	サウンド マネージャ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "soundmng.h"
#include "np2.h"
#if defined(SUPPORT_ROMEO)
#include "ext\externalchipmanager.h"
#endif
#if defined(MT32SOUND_DLL)
#include "ext\mt32snd.h"
#endif
#if defined(SUPPORT_ASIO)
#include "soundmng\sdasio.h"
#endif	// defined(SUPPORT_ASIO)
#include "soundmng\sddsound3.h"
#if defined(SUPPORT_WASAPI)
#include "soundmng\sdwasapi.h"
#endif	// defined(SUPPORT_WASAPI)
#include "common\parts.h"
#include "sound\sound.h"
#if defined(VERMOUTH_LIB)
#include "sound\vermouth\vermouth.h"
#endif
#include	"np2mt.h"

#if !defined(_WIN64)
#ifdef __cplusplus
extern "C"
{
#endif
/**
 * satuation
 * @param[out] dst 出力バッファ
 * @param[in] src 入力バッファ
 * @param[in] size サイズ
 */
void __fastcall satuation_s16mmx(SINT16 *dst, const SINT32 *src, UINT size);
#ifdef __cplusplus
}
#endif
#endif

#if defined(VERMOUTH_LIB)
	MIDIMOD		vermouth_module = NULL;
#endif

//! 唯一のインスタンスです
CSoundMng CSoundMng::sm_instance;

/**
 * 初期化
 */
void CSoundMng::Initialize()
{
#if defined(SUPPORT_ASIO) || defined(SUPPORT_WASAPI)
	::CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif	// defined(SUPPORT_ASIO) || defined(SUPPORT_WASAPI)

	CSoundDeviceDSound3::s_mastervol_available = true;//np2oscfg.usemastervolume ? true : false;
	CSoundDeviceDSound3::Initialize();
#if defined(SUPPORT_WASAPI)
	CSoundDeviceWasapi::Initialize();
#endif	// defined(SUPPORT_WASAPI)

#if defined(SUPPORT_ASIO)
	CSoundDeviceAsio::Initialize();
#endif	// defined(SUPPORT_ASIO)

#if defined(SUPPORT_ROMEO)
	CExternalChipManager::GetInstance()->Initialize();
#endif	// defined(SUPPORT_ROMEO)

	CSoundMng::GetInstance()->InitializeSoundCriticalSection();
}

/**
 * 解放
 */
void CSoundMng::Deinitialize()
{
#if defined(SUPPORT_ROMEO)
	CExternalChipManager::GetInstance()->Deinitialize();
#endif	// defined(SUPPORT_ROMEO)

#if defined(SUPPORT_WASAPI)
	CSoundDeviceWasapi::Deinitialize();
#endif	// defined(SUPPORT_WASAPI)

#if defined(SUPPORT_ASIO) || defined(SUPPORT_WASAPI)
	::CoUninitialize();
#endif	// defined(SUPPORT_ASIO) || defined(SUPPORT_WASAPI)
	
	CSoundMng::GetInstance()->FinalizeSoundCriticalSection();
}

/**
 * コンストラクタ
 */
CSoundMng::CSoundMng()
	: m_pSoundDevice(NULL)
	, m_nMute(0)
	, m_sound_cs_initialized(false)
{
	SetReverse(false);
}

/**
 * オープン
 * @param[in] nType デバイス タイプ
 * @param[in] lpName デバイス名
 * @param[in] hWnd ウィンドウ ハンドル
 * @retval true 成功
 * @retval false 失敗
 */
bool CSoundMng::Open(DeviceType nType, LPCTSTR lpName, HWND hWnd)
{
	EnterAllCriticalSection();

	Close();

	CSoundDeviceBase* pSoundDevice = NULL;
	switch (nType)
	{
		case kDefault:
			pSoundDevice = new CSoundDeviceDSound3;
			lpName = NULL;
			break;

		case kDSound3:
			pSoundDevice = new CSoundDeviceDSound3;
			break;

#if defined(SUPPORT_WASAPI)
		case kWasapi:
			pSoundDevice = new CSoundDeviceWasapi;
			break;
#endif	// defined(SUPPORT_WASAPI)

#if defined(SUPPORT_ASIO)
		case kAsio:
			pSoundDevice = new CSoundDeviceAsio;
			break;
#endif	// defined(SUPPORT_ASIO)
	}

	if (pSoundDevice)
	{
		if (!pSoundDevice->Open(lpName, hWnd))
		{
			delete pSoundDevice;
			pSoundDevice = NULL;
		}
	}

	if (pSoundDevice == NULL)
	{
		LeaveSoundCriticalSection();
		return false;
	}

	m_pSoundDevice = pSoundDevice;

#if defined(MT32SOUND_DLL)
	MT32Sound::GetInstance()->Initialize();
#endif
	
	LeaveAllCriticalSection();
	return true;
}

/**
 * クローズ
 */
void CSoundMng::Close()
{
	EnterAllCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->Close();
		delete m_pSoundDevice;
		m_pSoundDevice = NULL;
	}

#if defined(MT32SOUND_DLL)
	MT32Sound::GetInstance()->Deinitialize();
#endif
	LeaveAllCriticalSection();
}

/**
 * サウンド有効
 * @param[in] nProc プロシージャ
 */
void CSoundMng::Enable(SoundProc nProc)
{
	EnterSoundCriticalSection();
	const UINT nBit = 1 << nProc;
	if (!(m_nMute & nBit))
	{
		LeaveSoundCriticalSection();
		return;
	}
	m_nMute &= ~nBit;
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			m_pSoundDevice->PlayStream();
		}
#if defined(SUPPORT_ROMEO)
		CExternalChipManager::GetInstance()->Mute(false);
#endif	// defined(SUPPORT_ROMEO)
	}
	LeaveSoundCriticalSection();
}

/**
 * サウンド無効
 * @param[in] nProc プロシージャ
 */
void CSoundMng::Disable(SoundProc nProc)
{
	EnterSoundCriticalSection();
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			m_pSoundDevice->StopStream();
			m_pSoundDevice->StopAllPCM();
		}
#if defined(SUPPORT_ROMEO)
		CExternalChipManager::GetInstance()->Mute(true);
#endif	// defined(SUPPORT_ROMEO)
	}
	m_nMute |= (1 << nProc);
	LeaveSoundCriticalSection();
}

/**
 * ストリームを作成
 * @param[in] nSamplingRate サンプリング レート
 * @param[in] ms バッファ長(ミリ秒)
 * @return バッファ数
 */
UINT CSoundMng::CreateStream(UINT nSamplingRate, UINT ms)
{
	EnterAllCriticalSection();
	if (m_pSoundDevice == NULL)
	{
		LeaveSoundCriticalSection();
		return 0;
	}

	if (ms < 40)
	{
		ms = 40;
	}
	else if (ms > 1000)
	{
		ms = 1000;
	}
	UINT nBuffer = (nSamplingRate * ms) / 2000;
	nBuffer = (nBuffer + 1) & (~1);

	nBuffer = m_pSoundDevice->CreateStream(nSamplingRate, 2, nBuffer);
	if (nBuffer == 0)
	{
		LeaveSoundCriticalSection();
		return 0;
	}
	m_pSoundDevice->SetStreamData(this);

#if defined(VERMOUTH_LIB)
	vermouth_module = midimod_create(nSamplingRate);
	midimod_loadall(vermouth_module);
#endif

#if defined(MT32SOUND_DLL)
	MT32Sound::GetInstance()->SetRate(nSamplingRate);
#endif

	LeaveAllCriticalSection();
	return nBuffer;
}

/**
 * ストリームを破棄
 */
inline void CSoundMng::DestroyStream()
{
	if (m_pSoundDevice)
	{
		m_pSoundDevice->DestroyStream();
	}

#if defined(VERMOUTH_LIB)
	midimod_destroy(vermouth_module);
	vermouth_module = NULL;
#endif
#if defined(MT32SOUND_DLL)
	MT32Sound::GetInstance()->SetRate(0);
#endif
}

/**
 * ストリームのリセット
 */
inline void CSoundMng::ResetStream()
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->ResetStream();
	}
	LeaveSoundCriticalSection();
}

/**
 * ストリームの再生
 */
inline void CSoundMng::PlayStream()
{
	EnterSoundCriticalSection();
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			m_pSoundDevice->PlayStream();
		}
	}
	LeaveSoundCriticalSection();
}

/**
 * ストリームの停止
 */
inline void CSoundMng::StopStream()
{
	EnterSoundCriticalSection();
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			m_pSoundDevice->StopStream();
		}
	}
	LeaveSoundCriticalSection();
}

/**
 * ストリームを得る
 * @param[out] lpBuffer バッファ
 * @param[in] nBufferCount バッファ カウント
 * @return サンプル数
 */
UINT CSoundMng::Get16(SINT16* lpBuffer, UINT nBufferCount)
{
	EnterSoundCriticalSection();
	const SINT32* lpSource = ::sound_pcmlock();
	if (lpSource)
	{
		(*m_fnMix)(lpBuffer, lpSource, nBufferCount * 4);
		::sound_pcmunlock(lpSource);
		LeaveSoundCriticalSection();
		return nBufferCount;
	}
	else
	{
		LeaveSoundCriticalSection();
		return 0;
	}
}

/**
 * パン反転を設定する
 * @param[in] bReverse 反転フラグ
 */
inline void CSoundMng::SetReverse(bool bReverse)
{
	EnterSoundCriticalSection();
	if (!bReverse)
	{
#if !defined(_WIN64)
		if (mmxflag)
		{
			m_fnMix = satuation_s16;
		}
		else {
			m_fnMix = satuation_s16mmx;
		}
#else
		m_fnMix = satuation_s16;
#endif
	}
	else
	{
		m_fnMix = satuation_s16x;
	}
	LeaveSoundCriticalSection();
}

/**
 * マスター ヴォリューム設定
 * @param[in] nVolume ヴォリューム
 */
void CSoundMng::SetMasterVolume(int nVolume)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->SetMasterVolume(nVolume);
	}
	LeaveSoundCriticalSection();
}

/**
 * PCM データ読み込み
 * @param[in] nNum PCM 番号
 * @param[in] lpFilename ファイル名
 */
void CSoundMng::LoadPCM(SoundPCMNumber nNum, LPCTSTR lpFilename)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->LoadPCM(nNum, lpFilename);
	}
	LeaveSoundCriticalSection();
}

/**
 * PCM データ再読み込み
 * @param[in] nNum PCM 番号
 * @param[in] lpFilename ファイル名
 */
void CSoundMng::ReloadPCM(SoundPCMNumber nNum)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->ReloadPCM(nNum);
	}
	LeaveSoundCriticalSection();
}

/**
 * PCM ヴォリューム設定
 * @param[in] nNum PCM 番号
 * @param[in] nVolume ヴォリューム
 */
void CSoundMng::SetPCMVolume(SoundPCMNumber nNum, int nVolume)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->SetPCMVolume(nNum, nVolume);
	}
	LeaveSoundCriticalSection();
}

/**
 * PCM 再生
 * @param[in] nNum PCM 番号
 * @param[in] bLoop ループ フラグ
 * @retval true 成功
 * @retval false 失敗
 */
inline bool CSoundMng::PlayPCM(SoundPCMNumber nNum, BOOL bLoop)
{
	EnterSoundCriticalSection();
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			LeaveSoundCriticalSection();
			return m_pSoundDevice->PlayPCM(nNum, bLoop);
		}
	}
	LeaveSoundCriticalSection();
	return false;
}

/**
 * PCM 停止
 * @param[in] nNum PCM 番号
 */
inline void CSoundMng::StopPCM(SoundPCMNumber nNum)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->StopPCM(nNum);
	}
	LeaveSoundCriticalSection();
}

// クリティカルセクション関連
void CSoundMng::InitializeSoundCriticalSection()
{
	if(!m_sound_cs_initialized){
		InitializeCriticalSection(&m_sound_cs);
		m_sound_cs_initialized = true;
	}
}
void CSoundMng::FinalizeSoundCriticalSection()
{
	if(m_sound_cs_initialized){
		DeleteCriticalSection(&m_sound_cs);
		m_sound_cs_initialized = false;
	}
}
void CSoundMng::EnterSoundCriticalSection()
{
	if(m_sound_cs_initialized){
		EnterCriticalSection(&m_sound_cs);
	}
}
void CSoundMng::LeaveSoundCriticalSection()
{
	if(m_sound_cs_initialized){
		LeaveCriticalSection(&m_sound_cs);
	}
}
void CSoundMng::EnterAllCriticalSection()
{
	np2_multithread_EnterCriticalSection();
	EnterSoundCriticalSection();
}
void CSoundMng::LeaveAllCriticalSection()
{
	LeaveSoundCriticalSection();
	np2_multithread_LeaveCriticalSection();
}

// ---- C ラッパー

/**
 * ストリーム作成
 * @param[in] rate サンプリング レート
 * @param[in] ms バッファ長(ミリ秒)
 * @return バッファ サイズ
 */
UINT soundmng_create(UINT rate, UINT ms)
{
	UINT result;
	result = CSoundMng::GetInstance()->CreateStream(rate, ms);
	return result;
}

/**
 * ストリーム破棄
 */
void soundmng_destroy(void)
{
	CSoundMng::GetInstance()->DestroyStream();
}

/**
 * ストリーム リセット
 */
void soundmng_reset(void)
{
	CSoundMng::GetInstance()->ResetStream();
}

/**
 * ストリーム開始
 */
void soundmng_play(void)
{
	CSoundMng::GetInstance()->PlayStream();
}

/**
 * ストリーム停止
 */
void soundmng_stop(void)
{
	CSoundMng::GetInstance()->StopStream();
}

/**
 * ストリーム パン反転設定
 * @param[in] bReverse 反転
 */
void soundmng_setreverse(BOOL bReverse)
{
	CSoundMng::GetInstance()->SetReverse((bReverse) ? true : false);
}

/**
 * ボリューム設定
 * @param[in] nVolume ボリューム(最小 0 ～ 100 最大)
 */
void soundmng_setvolume(int nVolume)
{
	CSoundMng::GetInstance()->SetMasterVolume(nVolume);
}

/**
 * PCM 再生
 * @param[in] nNum PCM 番号
 * @param[in] bLoop ループ
 * @retval SUCCESS 成功
 * @retval FAILURE 失敗
 */
BRESULT soundmng_pcmplay(enum SoundPCMNumber nNum, BOOL bLoop)
{
	BRESULT result;
	result = (CSoundMng::GetInstance()->PlayPCM(nNum, bLoop)) ? SUCCESS : FAILURE;
	return result;
}

/**
 * PCM 停止
 * @param[in] nNum PCM 番号
 */
void soundmng_pcmstop(enum SoundPCMNumber nNum)
{
	CSoundMng::GetInstance()->StopPCM(nNum);
}


