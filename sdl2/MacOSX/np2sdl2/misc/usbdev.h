/**
 * @file	usbdev.h
 * @brief	USB アクセス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <IOKit/usb/IOUSBLib.h>

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
	IOUSBDeviceInterface** m_device;			/*!< デバイス */
	IOUSBInterfaceInterface** m_interface;		/*!< インタフェイス */
	static IOReturn ConfigureDevice(IOUSBDeviceInterface** dev);
};

/**
 * オープン済?
 * @retval true オープン済
 * @retval false 未オープン
 */
inline bool CUsbDev::IsOpened() const
{
	return (m_interface != NULL);
}
