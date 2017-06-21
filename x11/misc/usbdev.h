/**
 * @file	usbdev.h
 * @brief	USB アクセス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "compiler.h"

#ifdef USE_LIBUSB1
#include <libusb.h>
#endif

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
#ifdef USE_LIBUSB1
	libusb_context *m_ctx;
	libusb_device_handle *m_handle;
	unsigned char m_readEp, m_writeEp;
#endif
};

/**
 * オープン済?
 * @retval true オープン済
 * @retval false 未オープン
 */
inline bool CUsbDev::IsOpened() const
{
#ifdef USE_LIBUSB1
	if (m_handle != NULL)
		return true;
#endif
	return false;
}
