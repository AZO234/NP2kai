/**
 * @file	sddsound3.cpp
 * @brief	DSound3 �I�[�f�B�I �N���X�̓���̒�`���s���܂�
 */

#include <math.h>
#include "compiler.h"
#include "sddsound3.h"
#include "soundmng.h"
#include "misc\extrom.h"

#if !defined(__GNUC__)
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dsound.lib")
#endif	// !defined(__GNUC__)

#ifndef DSBVOLUME_MAX
#define DSBVOLUME_MAX		0							/*!< ���H�����[���ő�l */
#endif
#ifndef DSBVOLUME_MIN
#define DSBVOLUME_MIN		(-10000)					/*!< ���H�����[���ŏ��l */
#endif

#define NP2VOLUME2DSDB(a)	((LONG)(10*log((a)/100.0f)/log(2.0)*100))

//! �f�o�C�X ���X�g
std::vector<DSound3Device> CSoundDeviceDSound3::sm_devices;

//! �}�X�^�{�����[���g�p�\�H 
bool CSoundDeviceDSound3::s_mastervol_available = true;


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
 * ������
 */
void CSoundDeviceDSound3::Initialize()
{
	::DirectSoundEnumerate(EnumCallback, NULL);
}

/**
 * �f�o�C�X�񋓃R�[���o�b�N
 * @param[in] lpGuid GUID
 * @param[in] lpcstrDescription �f�o�C�X��
 * @param[in] lpcstrModule ���W���[����
 * @param[in] lpContext �R���e�L�X�g
 * @retval TRUE �p��
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
 * ��
 * @param[out] devices �f�o�C�X ���X�g
 */
void CSoundDeviceDSound3::EnumerateDevices(std::vector<LPCTSTR>& devices)
{
	for (std::vector<DSound3Device>::const_iterator it = sm_devices.begin(); it != sm_devices.end(); ++it)
	{
		devices.push_back(it->szDevice);
	}
}

/**
 * �R���X�g���N�^
 */
CSoundDeviceDSound3::CSoundDeviceDSound3()
	: m_lpDSound(NULL)
	, m_lpDSStream(NULL)
	, m_nChannels(0)
	, m_nBufferSize(0)
	, m_dwHalfBufferSize(0)
	, m_mastervolume(100)
{
	ZeroMemory(m_hEvents, sizeof(m_hEvents));
	ZeroMemory(m_pcmvolume, sizeof(m_pcmvolume));
}

/**
 * �f�X�g���N�^
 */
CSoundDeviceDSound3::~CSoundDeviceDSound3()
{
	Close();
}

/**
 * �I�[�v��
 * @param[in] lpDevice �f�o�C�X��
 * @param[in] hWnd �E�B���h�E �n���h��
 * @retval true ����
 * @retval false ���s
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

	// DirectSound�̏�����
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
 * �N���[�Y
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
 * �X�g���[���̍쐬
 * @param[in] nSamplingRate �T���v�����O ���[�g
 * @param[in] nChannels �`���l����
 * @param[in] nBufferSize �o�b�t�@ �T�C�Y
 * @return �o�b�t�@ �T�C�Y
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
	dsbdesc.dwFlags = DSBCAPS_CTRLPAN /*| (s_mastervol_available ? DSBCAPS_CTRLVOLUME : 0)*/ |
						DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPOSITIONNOTIFY |
						DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
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
	
	SetMasterVolume(m_mastervolume);

	ResetStream();
	CThreadBase::Start();
	return nBufferSize;
}

/**
 * �X�g���[����j��
 */
void CSoundDeviceDSound3::DestroyStream()
{
	if (m_hEvents[1])
	{
		::SetEvent(m_hEvents[1]);
	}
	CThreadBase::Stop();

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
 * �X�g���[�������Z�b�g
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
 * �X�g���[���̍Đ�
 * @retval true ����
 * @retval false ���s
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
 * �X�g���[���̒�~
 */
void CSoundDeviceDSound3::StopStream()
{
	if (m_lpDSStream)
	{
		m_lpDSStream->Stop();
	}
}

/**
 * �X�g���[�� ���H�����[���ݒ�
 * @param[in] nVolume ���H�����[��(max 100)
 */
void CSoundDeviceDSound3::SetMasterVolume(int nVolume)
{

	m_mastervolume = nVolume;
	if(s_mastervol_available){
		int numlen = m_pcm.size();
		UINT *nums = new UINT[numlen];
		int i = 0;
		for( auto it = m_pcm.begin(); it != m_pcm.end() ; ++it ) {
			nums[i] = it->first;
			i++;
		}
		for(i=0; i<numlen; i++) {
			int nNum = nums[i];
			ReloadPCM(nNum);
		}
		delete[] nums;
	}
}

/**
 * ����
 * @retval true �p��
 * @retval false �I��
 */
bool CSoundDeviceDSound3::Task()
{
	if(!m_hEvents[0] || !m_hEvents[1]){
		return false;
	}
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
 * �X�g���[�����X�V����
 * @param[in] dwPosition �X�V�ʒu
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
 * PCM �o�b�t�@��j������
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
	for (std::map<UINT, TCHAR*>::iterator it = m_pcmfile.begin(); it != m_pcmfile.begin(); ++it)
	{
		TCHAR* lpFilename = it->second;
		delete[] lpFilename;
	}
	m_pcmfile.clear();
}

/**
 * PCM ���X�g�b�v
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
 * PCM �f�[�^�ǂݍ���
 * @param[in] nNum PCM �ԍ�
 * @param[in] lpFilename �t�@�C����
 * @retval true ����
 * @retval false ���s
 */
bool CSoundDeviceDSound3::LoadPCM(UINT nNum, LPCTSTR lpFilename)
{
	UnloadPCM(nNum);
	
	int nVolume = 100;
	if(nNum	< PCMVOLUME_MAXCOUNT){
		nVolume = m_pcmvolume[nNum];
	}
	nVolume = nVolume * m_mastervolume / 100;
	LPDIRECTSOUNDBUFFER lpDSBuffer = CreateWaveBuffer(lpFilename, nVolume);
	if (lpDSBuffer)
	{
		m_pcm[nNum] = lpDSBuffer;
		if(m_pcmfile.find(nNum)==m_pcmfile.end()){
			// �V�K�쐬
			TCHAR *filename = new TCHAR[OEMSTRLEN(lpFilename)+1];
			_tcscpy(filename, lpFilename);
			m_pcmfile[nNum] = filename;
		}else{
			// �X�V
			_tcscpy(m_pcmfile[nNum], lpFilename);
		}
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * PCM �f�[�^�ēǂݍ���
 * @param[in] nNum PCM �ԍ�
 * @param[in] lpFilename �t�@�C����
 * @retval true ����
 * @retval false ���s
 */
bool CSoundDeviceDSound3::ReloadPCM(UINT nNum)
{
	if(m_pcm.find(nNum) == m_pcm.end()) return false; // ���݂��Ă��Ȃ�

	UnloadPCM(nNum);
	
	int nVolume = 100;
	if(nNum	< PCMVOLUME_MAXCOUNT){
		nVolume = m_pcmvolume[nNum];
	}
	nVolume = nVolume * m_mastervolume / 100;
	LPDIRECTSOUNDBUFFER lpDSBuffer = CreateWaveBuffer(m_pcmfile[nNum], nVolume);
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
 * PCM �f�[�^�ǂݍ���
 * @param[in] lpFilename �t�@�C����
 * @return �o�b�t�@
 */
LPDIRECTSOUNDBUFFER CSoundDeviceDSound3::CreateWaveBuffer(LPCTSTR lpFilename, int volume100)
{
	LPDIRECTSOUNDBUFFER lpDSBuffer = NULL;
	CExtRom extrom;

	do
	{
		if (!extrom.Open(lpFilename, 3))
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
		PCMWAVEFORMAT pcmwf = {0};
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
		dsbdesc.dwFlags = DSBCAPS_CTRLPAN /*| (s_mastervol_available ? DSBCAPS_CTRLVOLUME : 0) */| DSBCAPS_CTRLFREQUENCY | DSBCAPS_STATIC | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
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
		
		if(pcmwf.wBitsPerSample==8)
		{
			unsigned char *buf = new unsigned char[cbBlock1];
			extrom.Read(buf, cbBlock1);
			for(DWORD i=0;i<cbBlock1;i++){
				buf[i] = (unsigned char)((buf[i] - 0x80) * volume100 / 100 + 0x80);
			}
			memcpy(lpBlock1, buf, cbBlock1);
			delete[] buf;
		}
		else if(pcmwf.wBitsPerSample==16)
		{
			short *buf = new short[cbBlock1/2];
			extrom.Read(buf, cbBlock1);
			for(DWORD i=0;i<cbBlock1/2;i++){
				buf[i] = (short)((int)buf[i] * volume100 / 100);
			}
			memcpy(lpBlock1, buf, cbBlock1);
			delete[] buf;
		}else{
			extrom.Read(lpBlock1, cbBlock1);
		}

		if ((lpBlock2) && (cbBlock2))
		{
			if(pcmwf.wBitsPerSample==8)
			{
				unsigned char *buf = new unsigned char[cbBlock2];
				extrom.Read(buf, cbBlock2);
				for(DWORD i=0;i<cbBlock2;i++){
					buf[i] = (unsigned char)((buf[i] - 0x80) * volume100 / 100 + 0x80);
				}
				memcpy(lpBlock2, buf, cbBlock2);
				delete[] buf;
			}
			else if(pcmwf.wBitsPerSample==16)
			{
				short *buf = new short[cbBlock2/2];
				extrom.Read(buf, cbBlock2);
				for(DWORD i=0;i<cbBlock2/2;i++){
					buf[i] = (short)((int)buf[i] * volume100 / 100);
				}
				memcpy(lpBlock2, buf, cbBlock2);
				delete[] buf;
			}else{
				extrom.Read(lpBlock2, cbBlock2);
			}
		}

		lpDSBuffer->Unlock(lpBlock1, cbBlock1, lpBlock2, cbBlock2);
	} while (0 /*CONSTCOND*/);

	return lpDSBuffer;
}

/**
 * PCM ���A�����[�h
 * @param[in] nNum PCM �ԍ�
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
 * PCM ���H�����[���ݒ�
 * @param[in] nNum PCM �ԍ�
 * @param[in] nVolume ���H�����[��
 */
void CSoundDeviceDSound3::SetPCMVolume(UINT nNum, int nVolume)
{
	if(s_mastervol_available){
		int volume = nVolume;
		if(nNum	< PCMVOLUME_MAXCOUNT){
			m_pcmvolume[nNum] = nVolume;
		}
		ReloadPCM(nNum);
	}
}

/**
 * PCM �Đ�
 * @param[in] nNum PCM �ԍ�
 * @param[in] bLoop ���[�v �t���O
 * @retval true ����
 * @retval false ���s
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
 * PCM ��~
 * @param[in] nNum PCM �ԍ�
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
