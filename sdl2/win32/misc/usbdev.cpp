/**
 * @file	usbdev.cpp
 * @brief	USB アクセス クラスの動作の定義を行います
 */

#include "compiler.h"
#include "usbdev.h"
#include <setupapi.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")

// デバイスドライバのinf内で定義したGUID
// (WinUSB.sys使用デバイスに対する識別子）
// {63275336-530B-4069-92B6-5F8AE3465462}
DEFINE_GUID(GUID_DEVINTERFACE_WINUSB_GIMIC,
0x63275336, 0x530b, 0x4069, 0x92, 0xb6, 0x5f, 0x8a, 0xe3, 0x46, 0x54, 0x62);

// デバイスドライバのinf内で定義したGUID
// (WinUSB.sys使用デバイスに対する識別子）
// {b0320d09-0791-4c3f-a741-9ef97c8885d6}
DEFINE_GUID(GUID_DEVINTERFACE_WINUSB_C86BOX,
0xb0320d09, 0x0791, 0x4c3f, 0xa7, 0x41, 0x9e, 0xf9, 0x7c, 0x88, 0x85, 0xd6);

/**
 * コンストラクタ
 */
CUsbDev::CUsbDev()
	: m_hDev(INVALID_HANDLE_VALUE)
	, m_hWinUsb(NULL)
	, m_cOutPipeId(0)
	, m_cInPipeId(0)
{
}

/**
 * デストラクタ
 */
CUsbDev::~CUsbDev()
{
	Close();
}

/**
 * USB オープン
 * @param[in] vid ベンダー ID
 * @param[in] pid プロダクト ID
 * @param[in] nIndex インデックス
 * @retval true 成功
 * @retval false 失敗
 */
bool CUsbDev::Open(unsigned int vid, unsigned int pid, unsigned int nIndex)
{
	if (nIndex != 0)
	{
		return false;
	}

	if (vid == 0x16c0)
	{
		if (pid == 0x05e5)
		{
			return Open(GUID_DEVINTERFACE_WINUSB_GIMIC);
		}
		else if (pid == 0x27d8)
		{
			return Open(GUID_DEVINTERFACE_WINUSB_C86BOX);
		}
	}
	return false;
}

/**
 * USB オープン
 * @param[in] InterfaceGuid インタフェイス
 * @retval true 成功
 * @retval false 失敗
 */
bool CUsbDev::Open(const GUID& InterfaceGuid)
{
	TCHAR szDevicePath[512];
	LPTSTR lpDevicePath = GetDevicePath(InterfaceGuid, szDevicePath, _countof(szDevicePath));
	return OpenDevice(lpDevicePath);
}

/**
 * デバイス パス取得
 * @param[in] InterfaceGuid GUID
 * @param[out] lpDevicePath デバイス パス バッファ
 * @param[in] cchDevicePath デバイス パス バッファ長
 * @return パス
 */
LPTSTR CUsbDev::GetDevicePath(const GUID& InterfaceGuid, LPTSTR lpDevicePath, int cchDevicePath)
{
	HDEVINFO hDeviceInfo = ::SetupDiGetClassDevs(&InterfaceGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDeviceInfo == NULL)
	{
		printf("error device\n");
		return false;
	}

	LPTSTR lpRet = NULL;
	for (DWORD i = 0; lpRet == NULL ; i++)
	{
		SP_DEVICE_INTERFACE_DATA interfaceData;
		interfaceData.cbSize = sizeof(interfaceData);
		if (!::SetupDiEnumDeviceInterfaces(hDeviceInfo, NULL, &InterfaceGuid, i, &interfaceData))
		{
			break;
		}

		ULONG nRequiredLength = 0;
		::SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &interfaceData, NULL, 0, &nRequiredLength, NULL);

		PSP_DEVICE_INTERFACE_DETAIL_DATA pDetailData = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(::LocalAlloc(LMEM_FIXED, nRequiredLength));
		if (pDetailData == NULL)
		{
			continue;
		}

		pDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		ULONG nLength = nRequiredLength;
		if (::SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &interfaceData, pDetailData, nLength, &nRequiredLength, NULL))
		{
			lpRet = ::lstrcpyn(lpDevicePath, pDetailData->DevicePath, cchDevicePath);
		}
		::LocalFree(pDetailData);
	}
	::SetupDiDestroyDeviceInfoList(hDeviceInfo);

	return lpRet;
}

/**
 * デバイス オープン
 * @param[in] lpDevicePath デバイス名
 * @retval true 成功
 * @retval false 失敗
 */
bool CUsbDev::OpenDevice(LPCTSTR lpDevicePath)
{
	HANDLE hDev = ::CreateFile(lpDevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hDev == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	HANDLE hWinUsb = NULL;
	if (!::WinUsb_Initialize(hDev, &hWinUsb))
	{
		::CloseHandle(hDev);
		return false;
	}

	USB_INTERFACE_DESCRIPTOR desc;
	if (!::WinUsb_QueryInterfaceSettings(hWinUsb, 0, &desc))
	{
		::WinUsb_Free(hWinUsb);
		::CloseHandle(hDev);
		return false;
	}

	UCHAR cOutPipeId = 0;
	UCHAR cInPipeId = 0;
	for (int i = 0; i < desc.bNumEndpoints; i++)
	{
		WINUSB_PIPE_INFORMATION pipeInfo;
		if (::WinUsb_QueryPipe(hWinUsb, 0, (UCHAR)i, &pipeInfo))
		{
			if (pipeInfo.PipeType == UsbdPipeTypeBulk)
			{
				if (USB_ENDPOINT_DIRECTION_OUT(pipeInfo.PipeId))
				{
					cOutPipeId = pipeInfo.PipeId;
					// outPipeMaxPktSize = pipeInfo.MaximumPacketSize;
				}
				else if (USB_ENDPOINT_DIRECTION_IN(pipeInfo.PipeId))
				{
					cInPipeId = pipeInfo.PipeId;
					// inPipeMaxPktSize = pipeInfo.MaximumPacketSize;
				}
			}
		}
	}

	if (cOutPipeId != 0)
	{
		ULONG nTimeOut = 500;
		::WinUsb_SetPipePolicy(hWinUsb, cOutPipeId, PIPE_TRANSFER_TIMEOUT, sizeof(nTimeOut), &nTimeOut);
		::WinUsb_FlushPipe(hWinUsb, cOutPipeId);
	}
	if (cInPipeId != 0)
	{
		ULONG nTimeOut = 500;
		::WinUsb_SetPipePolicy(hWinUsb, cInPipeId, PIPE_TRANSFER_TIMEOUT, sizeof(nTimeOut), &nTimeOut);
		::WinUsb_FlushPipe(hWinUsb, cInPipeId);
	}

	m_hDev = hDev;
	m_hWinUsb = hWinUsb;
	m_cOutPipeId = cOutPipeId;
	m_cInPipeId = cInPipeId;

	return true;
}

/**
 * USB クローズ
 */
void CUsbDev::Close()
{
	if (m_hDev != INVALID_HANDLE_VALUE)
	{
		::WinUsb_Free(m_hWinUsb);
		::CloseHandle(m_hDev);

		m_hDev = INVALID_HANDLE_VALUE;
		m_hWinUsb = NULL;
		m_cOutPipeId = 0;
		m_cInPipeId = 0;
	}
}

/**
 * コントロール
 * @param[in] nType タイプ
 * @param[in] nRequest リクエスト
 * @param[in] nValue 値
 * @param[in] nIndex インデックス
 * @param[out] lpBuffer バッファ
 * @param[in] cbBuffer バッファ長
 * @return サイズ
 */
int CUsbDev::CtrlXfer(int nType, int nRequest, int nValue, int nIndex, void* lpBuffer, int cbBuffer)
{
	if (m_hWinUsb == NULL)
	{
		return -1;
	}

	WINUSB_SETUP_PACKET setup;
	setup.RequestType = nType;
	setup.Request = nRequest;
	setup.Value = nValue;
	setup.Index = nIndex;
	setup.Length = cbBuffer;

	ULONG nTransferred = 0;
	if (!::WinUsb_ControlTransfer(m_hWinUsb, setup, static_cast<PUCHAR>(lpBuffer), cbBuffer, &nTransferred, NULL))
	{
		return -1;
	}
	return static_cast<int>(nTransferred);
}

/**
 * データ送信
 * @param[in] lpBuffer バッファ
 * @param[in] cbBuffer バッファ長
 * @return サイズ
 */
int CUsbDev::WriteBulk(const void* lpBuffer, int cbBuffer)
{
	if (m_cOutPipeId == 0)
	{
		return -1;
	}

	DWORD dwLength = 0;
	while (!::WinUsb_WritePipe(m_hWinUsb, m_cOutPipeId, static_cast<PUCHAR>(const_cast<void*>(lpBuffer)), static_cast<ULONG>(cbBuffer), &dwLength, 0))
	{
		if (GetLastError() == ERROR_SEM_TIMEOUT)
		{
			::Sleep(1);
			continue;
		}

		Close();
		return -1;
	}
	return static_cast<int>(dwLength);
}

/**
 * データ受信
 * @param[out] lpBuffer バッファ
 * @param[in] cbBuffer バッファ長
 * @return サイズ
 */
int CUsbDev::ReadBulk(void* lpBuffer, int cbBuffer)
{
	if (m_cInPipeId == 0)
	{
		return -1;
	}

	DWORD dwLength = 0;
	if (!::WinUsb_ReadPipe(m_hWinUsb, m_cInPipeId, static_cast<PUCHAR>(lpBuffer), static_cast<ULONG>(cbBuffer), &dwLength, 0))
	{
		Close();
		return -1;
	}
	return static_cast<int>(dwLength);
}
