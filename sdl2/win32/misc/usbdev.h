/**
 * @file	usbdev.h
 * @brief	USB アクセス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WINXP
#include <WinUsb.h>

/**
 * @brief USB アクセス クラス
 */
class CUsbDev
{
public:
	CUsbDev();
	~CUsbDev();
	bool Open(unsigned int vid, unsigned int pid, unsigned int nIndex = 0);
	void Close();
	int CtrlXfer(int nType, int nRequest, int nValue = 0, int nIndex = 0, void* lpBuffer = NULL, int cbBuffer = 0);
	int WriteBulk(const void* lpBuffer, int cbBuffer);
	int ReadBulk(void* lpBuffer, int cbBuffer);
	bool IsOpened() const;

private:
	HANDLE m_hDev;						/*!< デバイス ハンドル */
	WINUSB_INTERFACE_HANDLE m_hWinUsb;	/*!< WinUSB */
	UCHAR m_cOutPipeId;					/*!< パイプ ID */
	UCHAR m_cInPipeId;					/*!< パイプ id */
	static LPTSTR GetDevicePath(const GUID& InterfaceGuid, LPTSTR lpDevicePath, int cchDevicePath);
	bool Open(const GUID& InterfaceGuid);
	bool OpenDevice(LPCTSTR lpDevicePath);
};

/**
 * オープン済?
 * @retval true オープン済
 * @retval false 未オープン
 */
inline bool CUsbDev::IsOpened() const
{
	return (m_hDev != INVALID_HANDLE_VALUE);
}
