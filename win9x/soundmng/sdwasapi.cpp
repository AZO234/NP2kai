/**
 * @file	sdwasapi.cpp
 * @brief	WASAPI オーディオ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "sdwasapi.h"
#include <atlbase.h>
#include <Functiondiscoverykeys_devpkey.h>

//! デバイス リスト
std::vector<WasapiDevice> CSoundDeviceWasapi::sm_devices;

//! MMDeviceEnumerator
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);

//! IMMDeviceEnumerator
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

//! IAudioClient
const IID IID_IAudioClient = __uuidof(IAudioClient);

//! IAudioRenderClient
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

/**
 * 初期化
 */
void CSoundDeviceWasapi::Initialize()
{
	IMMDeviceEnumerator* pEnumerator = NULL;
	if (SUCCEEDED(::CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<PVOID*>(&pEnumerator))))
	{
		IMMDeviceCollection* pDevices = NULL;
		if (SUCCEEDED(pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices)))
		{
			UINT nNumDevices = 0;
	    	if (SUCCEEDED(pDevices->GetCount(&nNumDevices)))
			{
				for (UINT i = 0; i < nNumDevices; i++)
				{
					IMMDevice* pDevice = NULL;
					if (FAILED(pDevices->Item(i, &pDevice)))
					{
						continue;
					}

					WasapiDevice dev;
					ZeroMemory(&dev, sizeof(dev));
					if (FAILED(pDevice->GetId(&dev.id)))
					{
						pDevice->Release();
						continue;
					}

					IPropertyStore* pPropStore = NULL;
					if (SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pPropStore)))
					{
						PROPVARIANT varName;
						::PropVariantInit(&varName);
						pPropStore->GetValue(PKEY_Device_FriendlyName, &varName);

						USES_CONVERSION;
						::lstrcpyn(dev.szDevice, W2CT(varName.pwszVal), _countof(dev.szDevice));
						::PropVariantClear(&varName);
						pPropStore->Release();
					}
					pDevice->Release();

					sm_devices.push_back(dev);
				}
			}
			pDevices->Release();
		}
		pEnumerator->Release();
	}
}

/**
 * 解放
 */
void CSoundDeviceWasapi::Deinitialize()
{
	if (!sm_devices.empty())
	{
		for (std::vector<WasapiDevice>::iterator it = sm_devices.begin(); it != sm_devices.end(); ++it)
		{
			::CoTaskMemFree(it->id);
		}
		sm_devices.clear();
	}
}

/**
 * 列挙
 * @param[out] devices デバイス リスト
 */
void CSoundDeviceWasapi::EnumerateDevices(std::vector<LPCTSTR>& devices)
{
	for (std::vector<WasapiDevice>::const_iterator it = sm_devices.begin(); it != sm_devices.end(); ++it)
	{
		devices.push_back(it->szDevice);
	}
}

/**
 * コンストラクタ
 */
CSoundDeviceWasapi::CSoundDeviceWasapi()
	: m_pEnumerator(NULL)
	, m_pDevice(NULL)
	, m_pAudioClient(NULL)
	, m_pwfx(NULL)
	, m_pRenderClient(NULL)
	, m_nBufferSize(0)
{
	ZeroMemory(m_hEvents, sizeof(m_hEvents));
}

/**
 * デストラクタ
 */
CSoundDeviceWasapi::~CSoundDeviceWasapi()
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
bool CSoundDeviceWasapi::Open(LPCTSTR lpDevice, HWND hWnd)
{
	if (FAILED(::CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<PVOID*>(&m_pEnumerator))))
	{
		return false;
	}

	HRESULT hr = E_FAIL;
	if ((lpDevice) && (lpDevice[0] != '\0'))
	{
		for (std::vector<WasapiDevice>::const_iterator it = sm_devices.begin(); it != sm_devices.end(); ++it)
		{
			if (::lstrcmpi(lpDevice, it->szDevice) == 0)
			{
				hr = m_pEnumerator->GetDevice(it->id, &m_pDevice);
				break;
			}
		}
	}
	else
	{
		hr = m_pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_pDevice);
	}

	if (FAILED(hr))
	{
		Close();
		return false;
	}
	return true;
}

/**
 * クローズ
 */
void CSoundDeviceWasapi::Close()
{
	DestroyStream();

	if (m_pDevice)
	{
		m_pDevice->Release();
		m_pDevice = NULL;
	}
	if (m_pEnumerator)
	{
		m_pEnumerator->Release();
		m_pEnumerator = NULL;
	}
}

/**
 * ストリームの作成
 * @param[in] nSamplingRate サンプリング レート
 * @param[in] nChannels チャネル数
 * @param[in] nBufferSize バッファ サイズ
 * @return バッファ サイズ
 */
UINT CSoundDeviceWasapi::CreateStream(UINT nSamplingRate, UINT nChannels, UINT nBufferSize)
{
	do
	{
		if (m_pDevice == NULL)
		{
			break;
		}

		if (FAILED(m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, reinterpret_cast<PVOID*>(&m_pAudioClient))))
		{
			break;
		}

		m_pwfx = static_cast<WAVEFORMATEX*>(::CoTaskMemAlloc(sizeof(WAVEFORMATEX)));
		if (m_pwfx == NULL)
		{
			break;
		}

		ZeroMemory(m_pwfx, sizeof(*m_pwfx));
		m_pwfx->wFormatTag = WAVE_FORMAT_PCM;
		m_pwfx->nChannels = nChannels;
		m_pwfx->nSamplesPerSec = nSamplingRate;
		m_pwfx->nAvgBytesPerSec = nSamplingRate * nChannels * (16 / 8);
		m_pwfx->nBlockAlign = (16 / 8) * nChannels;
		m_pwfx->wBitsPerSample = 16;

		if (FAILED(m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, m_pwfx, NULL)))
		{
			// printf("invalid format\n");
			break;
		}

		REFERENCE_TIME hnsRequestedDuration = 0;
		if (nBufferSize == 0)
		{
			m_pAudioClient->GetDevicePeriod(&hnsRequestedDuration, NULL);
		}
		else
		{
			hnsRequestedDuration = static_cast<REFERENCE_TIME>(((nBufferSize * 10000000.0) / nSamplingRate) + 0.5);
		}
		HRESULT hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, hnsRequestedDuration, hnsRequestedDuration, m_pwfx, NULL);
		if (hr == AUDCLNT_ERR(0x019) /* AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED */)
		{
			UINT32 nCurrentBufferSize = 0;
			if (FAILED(m_pAudioClient->GetBufferSize(&nCurrentBufferSize)))
			{
				break;
			}
			hnsRequestedDuration = static_cast<REFERENCE_TIME>(((nCurrentBufferSize * 10000000.0) / nSamplingRate) + 0.5);
			hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, hnsRequestedDuration, hnsRequestedDuration, m_pwfx, NULL);
		}
		if (FAILED(hr))
		{
			printf("Failed initiaziling (%x)\n", hr);
			break;
		}

		if (FAILED(m_pAudioClient->GetBufferSize(&m_nBufferSize)))
		{
			break;
		}

		m_hEvents[0] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (FAILED(m_pAudioClient->SetEventHandle(m_hEvents[0])))
		{
			break;
		}

		if (FAILED(m_pAudioClient->GetService(IID_IAudioRenderClient, reinterpret_cast<PVOID*>(&m_pRenderClient))))
		{
			break;
		}

		ResetStream();

		m_hEvents[1] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		CThreadBase::Start();

		return m_nBufferSize;

	} while (false /*CONSTCOND*/);

	DestroyStream();
	return 0;
}

/**
 * ストリームを破棄
 */
void CSoundDeviceWasapi::DestroyStream()
{
	if (m_hEvents[1])
	{
		::SetEvent(m_hEvents[1]);
		CThreadBase::Stop();
	}

	if (m_pAudioClient)
	{
		m_pAudioClient->Stop();
		m_pAudioClient->Release();
		m_pAudioClient = NULL;
	}

	if (m_pRenderClient)
	{
		m_pRenderClient->Release();
		m_pRenderClient = NULL;
	}

	if (m_pwfx)
	{
		::CoTaskMemFree(m_pwfx);
		m_pwfx = NULL;
	}

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
void CSoundDeviceWasapi::ResetStream()
{
	if (m_pRenderClient)
	{
		// Grab the entire buffer for the initial fill operation.
		BYTE* pData = NULL;
		HRESULT hr = m_pRenderClient->GetBuffer(m_nBufferSize, &pData);
		if (SUCCEEDED(hr))
		{
			ZeroMemory(pData, m_nBufferSize * m_pwfx->nBlockAlign);
			m_pRenderClient->ReleaseBuffer(m_nBufferSize, AUDCLNT_BUFFERFLAGS_SILENT);
		}
	}
}

/**
 * ストリームの再生
 * @retval true 成功
 * @retval false 失敗
 */
bool CSoundDeviceWasapi::PlayStream()
{
	if (m_pAudioClient)
	{
		if (SUCCEEDED(m_pAudioClient->Start()))
		{
			return true;
		}
	}
	return false;
}

/**
 * ストリームの停止
 */
void CSoundDeviceWasapi::StopStream()
{
	if (m_pAudioClient)
	{
		m_pAudioClient->Stop();
	}
}

/**
 * 同期
 * @retval true 継続
 * @retval false 終了
 */
bool CSoundDeviceWasapi::Task()
{
	 switch (::WaitForMultipleObjects(_countof(m_hEvents), m_hEvents, 0, INFINITE))
	 {
		case WAIT_OBJECT_0 + 0:
			FillStream();
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
 */
void CSoundDeviceWasapi::FillStream()
{
	if (m_pRenderClient)
	{
		BYTE* pData = NULL;
		HRESULT hr = m_pRenderClient->GetBuffer(m_nBufferSize, &pData);
		if (SUCCEEDED(hr))
		{
			UINT nStreamLength = 0;
			if (m_pSoundData)
			{
				nStreamLength = m_pSoundData->Get16(reinterpret_cast<short*>(pData), m_nBufferSize);
			}
			if (nStreamLength != m_nBufferSize)
			{
				ZeroMemory(pData + (nStreamLength * m_pwfx->nBlockAlign), (m_nBufferSize - nStreamLength) * m_pwfx->nBlockAlign);
			}
			m_pRenderClient->ReleaseBuffer(m_nBufferSize, 0);
		}
		else
		{
			printf("err.\n");
		}
	}
}
