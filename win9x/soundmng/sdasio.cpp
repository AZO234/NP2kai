/**
 * @file	sdasio.cpp
 * @brief	ASIO オーディオ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "sdasio.h"

/*! 唯一のインスタンスです */
CSoundDeviceAsio* CSoundDeviceAsio::sm_pInstance;

/*! ドライバ リスト */
AsioDriverList CSoundDeviceAsio::sm_asioDriverList;

/**
 * 初期化
 */
void CSoundDeviceAsio::Initialize()
{
	sm_asioDriverList.EnumerateDrivers();
}

/**
 * 列挙
 * @param[out] devices デバイス リスト
 */
void CSoundDeviceAsio::EnumerateDevices(std::vector<LPCTSTR>& devices)
{
	for (AsioDriverList::const_iterator it = sm_asioDriverList.begin(); it != sm_asioDriverList.end(); ++it)
	{
		devices.push_back(it->szDriverName);
	}
}

/**
 * コンストラクタ
 */
CSoundDeviceAsio::CSoundDeviceAsio()
	: m_pAsioDriver(NULL)
	, m_nBufferLength(0)
{
}

/**
 * デストラクタ
 */
CSoundDeviceAsio::~CSoundDeviceAsio()
{
	Close();
}

/**
 * 初期化
 * @param[in] lpDevice デバイス名
 * @param[in] hWnd ウィンドウ ハンドル
 * @retval true 成功
 * @retval false 失敗
 */
bool CSoundDeviceAsio::Open(LPCTSTR lpDevice, HWND hWnd)
{
	if (lpDevice == NULL)
	{
		return false;
	}

	if (m_pAsioDriver != NULL)
	{
		return false;
	}

	m_pAsioDriver = sm_asioDriverList.OpenDriver(lpDevice);
	if (m_pAsioDriver == NULL)
	{
		return false;
	}

	if (m_pAsioDriver->init(hWnd) == ASIOFalse)
	{
		Close();
		return false;
	}
	return true;
}

/**
 * 解放
 */
void CSoundDeviceAsio::Close()
{
	DestroyStream();
	if (m_pAsioDriver)
	{
		m_pAsioDriver->Release();
		m_pAsioDriver = NULL;
	}
}

/**
 * オープン
 * @param[in] nSamplingRate サンプリング レート
 * @param[in] nChannels チャネル数
 * @param[in] nBufferSize バッファ サイズ
 * @return バッファ サイズ
 */
UINT CSoundDeviceAsio::CreateStream(UINT nSamplingRate, UINT nChannels, UINT nBufferSize)
{
	if (m_pAsioDriver == NULL)
	{
		return 0;
	}

	do
	{
		long minSize;
		long maxSize;
		long preferredSize;
		long granularity;
		if (m_pAsioDriver->getBufferSize(&minSize, &maxSize, &preferredSize, &granularity) != ASE_OK)
		{
			break;
		}

		ZeroMemory(&m_callback, sizeof(m_callback));
		m_callback.bufferSwitch = cBufferSwitch;
		m_callback.sampleRateDidChange = cSampleRateDidChange;
		m_callback.asioMessage = cAsioMessage;
		m_callback.bufferSwitchTimeInfo = cBufferSwitchTimeInfo;

		m_bufferInfo.clear();
		for (UINT i = 0; i < nChannels; i++)
		{
			ASIOBufferInfo bufferInfo;
			ZeroMemory(&bufferInfo, sizeof(bufferInfo));
			bufferInfo.isInput = ASIOFalse;
			bufferInfo.channelNum = i;
			m_bufferInfo.push_back(bufferInfo);
		}

		m_nBufferLength = preferredSize;
		m_pAsioDriver->createBuffers(&m_bufferInfo.at(0), static_cast<long>(m_bufferInfo.size()), preferredSize, &m_callback);

		//! サンプルレートを設定する.
		m_pAsioDriver->setSampleRate(nSamplingRate);

		sm_pInstance = this;
		return m_nBufferLength;
	} while(false /*CONSTCOND*/);

	Close();
	return 0;
}

/**
 * 破棄
 */
void CSoundDeviceAsio::DestroyStream()
{
	if (sm_pInstance == this)
	{
		sm_pInstance = NULL;
	}

	StopStream();
	if (m_pAsioDriver)
	{
		m_pAsioDriver->disposeBuffers();
	}

	m_bufferInfo.clear();
	m_nBufferLength = 0;
}

/**
 * 再生
 * @retval true 成功
 * @retval false 失敗
 */
bool CSoundDeviceAsio::PlayStream()
{
	return (m_pAsioDriver) && (m_pAsioDriver->start() == ASE_OK);
}

/**
 * 停止
 */
void CSoundDeviceAsio::StopStream()
{
	if (m_pAsioDriver)
	{
		m_pAsioDriver->stop();
	}
}

/**
 * Processing
 * @param[in] doubleBufferIndex The current buffer half index (0 or 1)
 * @param[in] directProcess immediately start processing
 */
void CSoundDeviceAsio::cBufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
{
	if (sm_pInstance)
	{
		sm_pInstance->BufferSwitch(doubleBufferIndex, directProcess);
	}
}

/**
 * Informs the host application that a sample rate change was detected
 * @param[in] sRate The detected sample rate
 */
void CSoundDeviceAsio::cSampleRateDidChange(ASIOSampleRate sRate)
{
}

/**
 * Generic callback use for various purposes
 * @param[in] selector What kind of message is send
 * @param[in] value The single value
 * @param[in] message The message parameter
 * @param[in] opt The optional parameter
 * @return Specific to the selector
 */
long CSoundDeviceAsio::cAsioMessage(long selector, long value, void* message, double* opt)
{
	return 0;
}
/**
 * Indicates that both input and output are to be processed
 * @param[in] params The pointer to ASIOTime structure
 * @param[in] doubleBufferIndex The current buffer half index (0 or 1)
 * @param[in] directProcess immediately start processing
 * @return The pointer to ASIOTime structure with "output" time code information
 */
ASIOTime* CSoundDeviceAsio::cBufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
{
	return NULL;
}

/**
 * Processing
 * @param[in] doubleBufferIndex The current buffer half index (0 or 1)
 * @param[in] directProcess immediately start processing
 */
void CSoundDeviceAsio::BufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
{
	if (m_nBufferLength == 0)
	{
		return;
	}

	UINT nStreamLength = 0;
	std::vector<short> stream(m_nBufferLength * m_bufferInfo.size());
	if (m_pSoundData)
	{
		nStreamLength = m_pSoundData->Get16(&stream.at(0), m_nBufferLength);
	}

	const short* lpStream = &stream.at(0);
	const UINT nAlign = static_cast<UINT>(m_bufferInfo.size());

	for (std::vector<ASIOBufferInfo>::iterator it = m_bufferInfo.begin(); it != m_bufferInfo.end(); ++it)
	{
		void* lpBuffer = it->buffers[doubleBufferIndex];

		ASIOChannelInfo info;
		info.channel = it->channelNum;
		info.isInput = it->isInput;
		m_pAsioDriver->getChannelInfo(&info);

		switch (info.type)
		{
			case ASIOSTInt16LSB:
				{
					short* lpOutput = static_cast<short*>(lpBuffer);
					for (UINT i = 0; i < nStreamLength; i++)
					{
						lpOutput[i] = lpStream[i * nAlign];
					}
					if (nStreamLength != m_nBufferLength)
					{
						memset(lpOutput + nStreamLength, 0, (m_nBufferLength - nStreamLength) * sizeof(short));
					}
				}
				break;

			case ASIOSTInt24LSB:
				{
					char* lpOutput = static_cast<char*>(lpBuffer);
					for (UINT i = 0; i < nStreamLength; i++)
					{
						const short wSample = lpStream[i * nAlign];
						lpOutput[i * 3 + 0] = static_cast<char>(wSample >> 8);
						lpOutput[i * 3 + 1] = static_cast<char>(wSample >> 0);
						lpOutput[i * 3 + 2] = static_cast<char>(wSample >> 8);
					}
					if (nStreamLength != m_nBufferLength)
					{
						memset(lpOutput + nStreamLength * 3,  0, (m_nBufferLength - nStreamLength) * sizeof(char) * 3);
					}
				}
				memset(lpBuffer, 0, m_nBufferLength * 3);
				break;

			case ASIOSTInt32LSB:
				{
					short* lpOutput = static_cast<short*>(lpBuffer);
					for (UINT i = 0; i < nStreamLength; i++)
					{
						const short wSample = lpStream[i * nAlign];
						lpOutput[i * 2 + 0] = wSample;
						lpOutput[i * 2 + 1] = wSample;
					}
					if (nStreamLength != m_nBufferLength)
					{
						memset(lpOutput + nStreamLength * 2,  0, (m_nBufferLength - nStreamLength) * sizeof(short) * 2);
					}
				}
				break;

			case ASIOSTInt32LSB16:
			case ASIOSTInt32LSB18:
			case ASIOSTInt32LSB20:
			case ASIOSTInt32LSB24:
			case ASIOSTFloat32LSB:
			case ASIOSTInt32MSB:
			case ASIOSTInt32MSB16:
			case ASIOSTInt32MSB18:
			case ASIOSTInt32MSB20:
			case ASIOSTInt32MSB24:
			case ASIOSTFloat32MSB:
				memset(lpBuffer, 0, m_nBufferLength * 4);
				break;

			case ASIOSTFloat64LSB:
			case ASIOSTFloat64MSB: 
				memset(lpBuffer, 0, m_nBufferLength * 8);
				break;

			case ASIOSTInt16MSB:
				memset(lpBuffer, 0, m_nBufferLength * 2);
				break;

			case ASIOSTInt24MSB:
				memset(lpBuffer, 0, m_nBufferLength * 3);
				break;
		}

		lpStream++;
	}

	m_pAsioDriver->outputReady();
}
