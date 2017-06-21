/**
 * @file	sddsound3.cpp
 * @brief	DSound3 オーディオ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "sddsound3.h"
#include "soundmng.h"
#include "misc\extrom.h"

#if !defined(__GNUC__)
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dsound.lib")
#endif	// !defined(__GNUC__)

#ifndef DSBVOLUME_MAX
#define DSBVOLUME_MAX		0							/*!< ヴォリューム最大値 */
#endif
#ifndef DSBVOLUME_MIN
#define DSBVOLUME_MIN		(-10000)					/*!< ヴォリューム最小値 */
#endif

//! デバイス リスト
std::vector<DSound3Device> CSoundDeviceDSound3::sm_devices;

/**
 * @brief RIFF chunk
 */
struct RiffChunk
{
	UINT32 riff;				/*!< 'RIFF' */
	UINT32 nFileSize;			/*!< fileSize */
	UINT32 nFileType;			/*!< fileType */
};

/**
 * @brief chunk
 */
struct Chunk
{
	UINT32 id;					/*!< chunkID */
	UINT32 nSize;				/*!< chunkSize */
};

/**
 * 初期化
 */
void CSoundDeviceDSound3::Initialize()
{
	::DirectSoundEnumerate(EnumCallback, NULL);
}

/**
 * デバイス列挙コールバック
 * @param[in] lpGuid GUID
 * @param[in] lpcstrDescription デバイス名
 * @param[in] lpcstrModule モジュール名
 * @param[in] lpContext コンテキスト
 * @retval TRUE 継続
 */
BOOL CALLBACK CSoundDeviceDSound3::EnumCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
	if (lpGuid != NULL)
	{
		DSound3Device device;
		ZeroMemory(&device, sizeof(device));
		device.guid = *lpGuid;
		::lstrcpyn(device.szDevice, lpcstrDescription, _countof(device.szDevice));
		sm_devices.push_back(device);
	}
	return TRUE;
}

/**
 * 列挙
 * @param[out] devices デバイス リスト
 */
void CSoundDeviceDSound3::EnumerateDevices(std::vector<LPCTSTR>& devices)
{
	for (std::vector<DSound3Device>::const_iterator it = sm_devices.begin(); it != sm_devices.end(); ++it)
	{
		devices.push_back(it->szDevice);
	}
}

/**
 * コンストラクタ
 */
CSoundDeviceDSound3::CSoundDeviceDSound3()
	: m_lpDSound(NULL)
	, m_lpDSStream(NULL)
	, m_nChannels(0)
	, m_nBufferSize(0)
	, m_dwHalfBufferSize(0)
{
	ZeroMemory(m_hEvents, sizeof(m_hEvents));
}

/**
 * デストラクタ
 */
CSoundDeviceDSound3::~CSoundDeviceDSound3()
{
	Close();
}

/**
 * オープン
 * @param[in] lpDevice デバイス名
 * @param[in] hWnd ウィンドウ ハンドル
 * @retval true 成功
 * @retval false 失敗
 */
bool CSoundDeviceDSound3::Open(LPCTSTR lpDevice, HWND hWnd)
{
	if (hWnd == NULL)
	{
		return false;
	}

	LPGUID lpGuid = NULL;
	if ((lpDevice) && (lpDevice[0] != '\0'))
	{
		std::vector<DSound3Device>::const_iterator it = sm_devices.begin();
		while ((it != sm_devices.end()) && (::lstrcmpi(lpDevice, it->szDevice) != 0))
		{
			++it;
		}
		if (it == sm_devices.end())
		{
			return false;
		}
		lpGuid = const_cast<LPGUID>(&it->guid);
	}

	// DirectSoundの初期化
	LPDIRECTSOUND lpDSound;
	if (FAILED(DirectSoundCreate(lpGuid, &lpDSound, 0)))
	{
		return false;
	}
	if (FAILED(lpDSound->SetCooperativeLevel(hWnd, DSSCL_PRIORITY)))
	{
		if (FAILED(lpDSound->SetCooperativeLevel(hWnd, DSSCL_NORMAL)))
		{
			lpDSound->Release();
			return false;
		}
	}

	m_lpDSound = lpDSound;
	return true;
}

/**
 * クローズ
 */
void CSoundDeviceDSound3::Close()
{
	DestroyAllPCM();
	DestroyStream();

	if (m_lpDSound)
	{
		m_lpDSound->Release();
		m_lpDSound = NULL;
	}
}

/**
 * ストリームの作成
 * @param[in] nSamplingRate サンプリング レート
 * @param[in] nChannels チャネル数
 * @param[in] nBufferSize バッファ サイズ
 * @return バッファ サイズ
 */
UINT CSoundDeviceDSound3::CreateStream(UINT nSamplingRate, UINT nChannels, UINT nBufferSize)
{
	if (m_lpDSound == NULL)
	{
		return 0;
	}

	if (nBufferSize == 0)
	{
		nBufferSize = nSamplingRate / 10;
	}

	m_nChannels = nChannels;
	m_nBufferSize = nBufferSize;
	m_dwHalfBufferSize = nBufferSize * nChannels * sizeof(short);

	PCMWAVEFORMAT pcmwf;
	ZeroMemory(&pcmwf, sizeof(pcmwf));
	pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmwf.wf.nChannels = nChannels;
	pcmwf.wf.nSamplesPerSec = nSamplingRate;
	pcmwf.wBitsPerSample = 16;
	pcmwf.wf.nBlockAlign = nChannels * (pcmwf.wBitsPerSample / 8);
	pcmwf.wf.nAvgBytesPerSec = nSamplingRate * pcmwf.wf.nBlockAlign;

	DSBUFFERDESC dsbdesc;
	ZeroMemory(&dsbdesc, sizeof(dsbdesc));
	dsbdesc.dwSize = sizeof(dsbdesc);
	dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |
						DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPOSITIONNOTIFY |
						DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	dsbdesc.lpwfxFormat = reinterpret_cast<LPWAVEFORMATEX>(&pcmwf);
	dsbdesc.dwBufferBytes = m_dwHalfBufferSize * 2;
	HRESULT hr = m_lpDSound->CreateSoundBuffer(&dsbdesc, &m_lpDSStream, NULL);
	if (FAILED(hr))
	{
		dsbdesc.dwSize = (sizeof(DWORD) * 4) + sizeof(LPWAVEFORMATEX);
		hr = m_lpDSound->CreateSoundBuffer(&dsbdesc, &m_lpDSStream, NULL);
	}
	if (FAILED(hr))
	{
		DestroyStream();
		return 0;
	}

	LPDIRECTSOUNDNOTIFY pNotify;
	if (FAILED(m_lpDSStream->QueryInterface(IID_IDirectSoundNotify, reinterpret_cast<LPVOID*>(&pNotify))))
	{
		DestroyStream();
		return 0;
	}

	for (UINT i = 0; i < _countof(m_hEvents); i++)
	{
		m_hEvents[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	DSBPOSITIONNOTIFY pos[2];
	ZeroMemory(pos, sizeof(pos));
	for (UINT i = 0; i < _countof(pos); i++)
	{
		pos[i].dwOffset = m_dwHalfBufferSize * i;
		pos[i].hEventNotify = m_hEvents[0];
	}
	pNotify->SetNotificationPositions(_countof(pos), pos);
	pNotify->Release();

	ResetStream();
	CThreadBase::Start();
	return nBufferSize;
}

/**
 * ストリームを破棄
 */
void CSoundDeviceDSound3::DestroyStream()
{
	if (m_hEvents[1])
	{
		::SetEvent(m_hEvents[1]);
		CThreadBase::Stop();
	}

	if (m_lpDSStream)
	{
		m_lpDSStream->Stop();
		m_lpDSStream->Release();
		m_lpDSStream = NULL;
	}

	m_nChannels = 0;
	m_nBufferSize = 0;
	m_dwHalfBufferSize = 0;
	for (UINT i = 0; i < _countof(m_hEvents); i++)
	{
		if (m_hEvents[i])
		{
			::CloseHandle(m_hEvents[i]);
			m_hEvents[i] = NULL;
		}
	}
}

/**
 * ストリームをリセット
 */
void CSoundDeviceDSound3::ResetStream()
{
	if (m_lpDSStream)
	{
		LPVOID lpBlock1;
		DWORD cbBlock1;
		LPVOID lpBlock2;
		DWORD cbBlock2;
		if (SUCCEEDED(m_lpDSStream->Lock(0, m_dwHalfBufferSize * 2, &lpBlock1, &cbBlock1, &lpBlock2, &cbBlock2, 0)))
		{
			ZeroMemory(lpBlock1, cbBlock1);
			if ((lpBlock2) && (cbBlock2))
			{
				ZeroMemory(lpBlock2, cbBlock2);
			}
			m_lpDSStream->Unlock(lpBlock1, cbBlock1, lpBlock2, cbBlock2);
			m_lpDSStream->SetCurrentPosition(0);
		}
	}
}

/**
 * ストリームの再生
 * @retval true 成功
 * @retval false 失敗
 */
bool CSoundDeviceDSound3::PlayStream()
{
	if (m_lpDSStream)
	{
		m_lpDSStream->Play(0, 0, DSBPLAY_LOOPING);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * ストリームの停止
 */
void CSoundDeviceDSound3::StopStream()
{
	if (m_lpDSStream)
	{
		m_lpDSStream->Stop();
	}
}

/**
 * 同期
 * @retval true 継続
 * @retval false 終了
 */
bool CSoundDeviceDSound3::Task()
{
	 switch (WaitForMultipleObjects(_countof(m_hEvents), m_hEvents, 0, INFINITE))
	 {
		case WAIT_OBJECT_0 + 0:
			if (m_lpDSStream)
			{
				DWORD dwCurrentPlayCursor;
				DWORD dwCurrentWriteCursor;
				if (SUCCEEDED(m_lpDSStream->GetCurrentPosition(&dwCurrentPlayCursor, &dwCurrentWriteCursor)))
				{
					const DWORD dwPos = (dwCurrentPlayCursor >= m_dwHalfBufferSize) ? 0 : m_dwHalfBufferSize;
					FillStream(dwPos);
				}
			}
			break;

		case WAIT_OBJECT_0 + 1:
			return false;

		default:
			break;
	}
	return true;
}

/**
 * ストリームを更新する
 * @param[in] dwPosition 更新位置
 */
void CSoundDeviceDSound3::FillStream(DWORD dwPosition)
{
	LPVOID lpBlock1;
	DWORD cbBlock1;
	LPVOID lpBlock2;
	DWORD cbBlock2;
	HRESULT hr = m_lpDSStream->Lock(dwPosition, m_dwHalfBufferSize, &lpBlock1, &cbBlock1, &lpBlock2, &cbBlock2, 0);
	if (hr == DSERR_BUFFERLOST)
	{
		m_lpDSStream->Restore();
		hr = m_lpDSStream->Lock(dwPosition, m_dwHalfBufferSize, &lpBlock1, &cbBlock1, &lpBlock2, &cbBlock2, 0);
	}
	if (SUCCEEDED(hr))
	{
		UINT nStreamLength = 0;
		if (m_pSoundData)
		{
			nStreamLength = m_pSoundData->Get16(static_cast<SINT16*>(lpBlock1), m_nBufferSize);
		}
		if (nStreamLength != m_nBufferSize)
		{
			ZeroMemory(static_cast<short*>(lpBlock1) + nStreamLength * m_nChannels, (m_nBufferSize - nStreamLength) * m_nChannels * sizeof(short));
		}
		m_lpDSStream->Unlock(lpBlock1, cbBlock1, lpBlock2, cbBlock2);
	}
}

/**
 * PCM バッファを破棄する
 */
void CSoundDeviceDSound3::DestroyAllPCM()
{
	for (std::map<UINT, LPDIRECTSOUNDBUFFER>::iterator it = m_pcm.begin(); it != m_pcm.begin(); ++it)
	{
		LPDIRECTSOUNDBUFFER lpDSBuffer = it->second;
		lpDSBuffer->Stop();
		lpDSBuffer->Release();
	}
	m_pcm.clear();
}

/**
 * PCM をストップ
 */
void CSoundDeviceDSound3::StopAllPCM()
{
	for (std::map<UINT, LPDIRECTSOUNDBUFFER>::iterator it = m_pcm.begin(); it != m_pcm.begin(); ++it)
	{
		LPDIRECTSOUNDBUFFER lpDSBuffer = it->second;
		lpDSBuffer->Stop();
	}
}

/**
 * PCM データ読み込み
 * @param[in] nNum PCM 番号
 * @param[in] lpFilename ファイル名
 * @retval true 成功
 * @retval false 失敗
 */
bool CSoundDeviceDSound3::LoadPCM(UINT nNum, LPCTSTR lpFilename)
{
	UnloadPCM(nNum);

	LPDIRECTSOUNDBUFFER lpDSBuffer = CreateWaveBuffer(lpFilename);
	if (lpDSBuffer)
	{
		m_pcm[nNum] = lpDSBuffer;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * PCM データ読み込み
 * @param[in] lpFilename ファイル名
 * @return バッファ
 */
LPDIRECTSOUNDBUFFER CSoundDeviceDSound3::CreateWaveBuffer(LPCTSTR lpFilename)
{
	LPDIRECTSOUNDBUFFER lpDSBuffer = NULL;
	CExtRom extrom;

	do
	{
		if (!extrom.Open(lpFilename))
		{
			break;
		}

		RiffChunk riff;
		if (extrom.Read(&riff, sizeof(riff)) != sizeof(riff))
		{
			break;
		}
		if ((riff.riff != MAKEFOURCC('R','I','F','F')) || (riff.nFileType != MAKEFOURCC('W','A','V','E')))
		{
			break;
		}

		bool bValid = false;
		Chunk chunk;
		PCMWAVEFORMAT pcmwf;
		while (true /*CONSTCOND*/)
		{
			if (extrom.Read(&chunk, sizeof(chunk)) != sizeof(chunk))
			{
				bValid = false;
				break;
			}
			if (chunk.id == MAKEFOURCC('f','m','t',' '))
			{
				if (chunk.nSize >= sizeof(pcmwf))
				{
					if (extrom.Read(&pcmwf, sizeof(pcmwf)) != sizeof(pcmwf))
					{
						bValid = false;
						break;
					}
					chunk.nSize -= sizeof(pcmwf);
					bValid = true;
				}
			}
			else if (chunk.id == MAKEFOURCC('d','a','t','a'))
			{
				break;
			}
			if (chunk.nSize)
			{
				extrom.Seek(chunk.nSize, FILE_CURRENT);
			}
		}
		if (!bValid)
		{
			break;
		}

		if (pcmwf.wf.wFormatTag != WAVE_FORMAT_PCM)
		{
			break;
		}

		DSBUFFERDESC dsbdesc;
		ZeroMemory(&dsbdesc, sizeof(dsbdesc));
		dsbdesc.dwSize = sizeof(dsbdesc);
		dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_STATIC | DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		dsbdesc.dwBufferBytes = chunk.nSize;
		dsbdesc.lpwfxFormat = reinterpret_cast<LPWAVEFORMATEX>(&pcmwf);

		HRESULT hr = m_lpDSound->CreateSoundBuffer(&dsbdesc, &lpDSBuffer, NULL);
		if (FAILED(hr))
		{
			dsbdesc.dwSize = (sizeof(DWORD) * 4) + sizeof(LPWAVEFORMATEX);
			hr = m_lpDSound->CreateSoundBuffer(&dsbdesc, &lpDSBuffer, NULL);
		}
		if (FAILED(hr))
		{
			break;
		}

		LPVOID lpBlock1;
		DWORD cbBlock1;
		LPVOID lpBlock2;
		DWORD cbBlock2;
		hr = lpDSBuffer->Lock(0, chunk.nSize, &lpBlock1, &cbBlock1, &lpBlock2, &cbBlock2, 0);
		if (hr == DSERR_BUFFERLOST)
		{
			lpDSBuffer->Restore();
			hr = lpDSBuffer->Lock(0, chunk.nSize, &lpBlock1, &cbBlock1, &lpBlock2, &cbBlock2, 0);
		}
		if (FAILED(hr))
		{
			lpDSBuffer->Release();
			lpDSBuffer = NULL;
			break;
		}

		extrom.Read(lpBlock1, cbBlock1);
		if ((lpBlock2) && (cbBlock2))
		{
			extrom.Read(lpBlock2, cbBlock2);
		}
		lpDSBuffer->Unlock(lpBlock1, cbBlock1, lpBlock2, cbBlock2);
	} while (0 /*CONSTCOND*/);

	return lpDSBuffer;
}

/**
 * PCM をアンロード
 * @param[in] nNum PCM 番号
 */
void CSoundDeviceDSound3::UnloadPCM(UINT nNum)
{
	std::map<UINT, LPDIRECTSOUNDBUFFER>::iterator it = m_pcm.find(nNum);
	if (it != m_pcm.end())
	{
		LPDIRECTSOUNDBUFFER lpDSBuffer = it->second;
		m_pcm.erase(it);

		lpDSBuffer->Stop();
		lpDSBuffer->Release();
	}
}

/**
 * PCM ヴォリューム設定
 * @param[in] nNum PCM 番号
 * @param[in] nVolume ヴォリューム
 */
void CSoundDeviceDSound3::SetPCMVolume(UINT nNum, int nVolume)
{
	std::map<UINT, LPDIRECTSOUNDBUFFER>::iterator it = m_pcm.find(nNum);
	if (it != m_pcm.end())
	{
		LPDIRECTSOUNDBUFFER lpDSBuffer = it->second;
		lpDSBuffer->SetVolume((((DSBVOLUME_MAX - DSBVOLUME_MIN) * nVolume) / 100) + DSBVOLUME_MIN);
	}
}

/**
 * PCM 再生
 * @param[in] nNum PCM 番号
 * @param[in] bLoop ループ フラグ
 * @retval true 成功
 * @retval false 失敗
 */
bool CSoundDeviceDSound3::PlayPCM(UINT nNum, BOOL bLoop)
{
	std::map<UINT, LPDIRECTSOUNDBUFFER>::iterator it = m_pcm.find(nNum);
	if (it != m_pcm.end())
	{
		LPDIRECTSOUNDBUFFER lpDSBuffer = it->second;
//		lpDSBuffer->SetCurrentPosition(0);
		lpDSBuffer->Play(0, 0, (bLoop) ? DSBPLAY_LOOPING : 0);
		return true;
	}
	return false;
}

/**
 * PCM 停止
 * @param[in] nNum PCM 番号
 */
void CSoundDeviceDSound3::StopPCM(UINT nNum)
{
	std::map<UINT, LPDIRECTSOUNDBUFFER>::iterator it = m_pcm.find(nNum);
	if (it != m_pcm.end())
	{
		LPDIRECTSOUNDBUFFER lpDSBuffer = it->second;
		lpDSBuffer->Stop();
	}
}
