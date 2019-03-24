/**
 * @file	usbdev.cpp
 * @brief	USB アクセス クラスの動作の定義を行います
 */

#include "compiler.h"
#include "usbdev.h"

#define	XFER_TIMEOUT	1000	/* ms */

#ifdef USE_LIBUSB1
#if 0
class USBDeviceLock {
public:
	USBDeviceLock(libusb_device_handle *handle)
	    : m_handle(handle)
	    , m_claimed(false)
	{
		if (libusb_claim_interface(m_handle, 0) == 0)
			m_claimed = true;
	}

	~USBDeviceLock() {
		if (m_claimed)
			libusb_release_interface(m_handle, 0);
	}

private:
	libusb_device_handle *m_handle;
	bool m_claimed;
};
#endif
#endif

/**
 * コンストラクタ
 */
CUsbDev::CUsbDev()
#ifdef USE_LIBUSB1
    : m_ctx(NULL)
    , m_handle(NULL)
    , m_readEp(0)
    , m_writeEp(0)
#endif
{
#ifdef USE_LIBUSB1
	if (libusb_init(&m_ctx) != 0)
		printf("USB library init failed.");
#endif
}

/**
 * デストラクタ
 */
CUsbDev::~CUsbDev()
{
#ifdef USE_LIBUSB1
	if (m_ctx != NULL) {
		libusb_exit(m_ctx);
		m_ctx = NULL;
	}
#endif
}

/**
 * USB オープン
 * @param[in] vid VID
 * @param[in] pid PID
 * @param[in] nIndex インデックス
 * @retval true 成功
 * @retval false 失敗
 */
bool CUsbDev::Open(unsigned int vid, unsigned int pid, unsigned int nIndex)
{
#ifdef USE_LIBUSB1
	libusb_device **list = NULL;
	libusb_device_handle *handle = NULL;
	struct libusb_config_descriptor *conf = NULL;

	if (m_ctx == NULL)
		return false;

	ssize_t ndevs = libusb_get_device_list(m_ctx, &list);
	if (ndevs < 0) {
		printf("Error: Couldn't get device list: %s\n",
		    libusb_error_name(ndevs));
		return false;
	}

	unsigned int idx = 0;
	for (ssize_t i = 0; i < ndevs; ++i) {
		libusb_device *dev = list[i];

		struct libusb_device_descriptor desc;
		int error;
		error = libusb_get_device_descriptor(dev, &desc);
		if (error != 0) {
			printf("Error: Couldn't get device descriptor: %s\n",
			    libusb_error_name(error));
			continue;
		}
		if (desc.idVendor != vid || desc.idProduct != pid)
			continue;
		if (idx++ != nIndex)
			continue;

		error = libusb_open(dev, &handle);
		if (error != 0) {
			printf("Error: Couldn't open device: %s\n",
			    libusb_error_name(error));
			break;
		}

		/* Set default configration */
		error = libusb_set_configuration(handle, 1);
		if (error != 0) {
			printf("Error: Couldn't set configuration: %s\n",
			    libusb_error_name(error));
			break;
		}

		error = libusb_get_active_config_descriptor(dev, &conf);
		if (error != 0) {
			printf("Error: Couldn't get config descriptor: %s\n",
			    libusb_error_name(error));
			break;
		}

		uint8_t rep = 0, wep = 0;
		const libusb_interface_descriptor *iface =
		    &conf->interface[0].altsetting[0];
		for (int i = 0; i < iface->bNumEndpoints; ++i) {
			const struct libusb_endpoint_descriptor *ep =
			    &iface->endpoint[i];
			if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) !=
			    LIBUSB_TRANSFER_TYPE_BULK)
				continue;
			if ((ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) ==
			    LIBUSB_ENDPOINT_IN) {
				if (rep == 0)
					rep = ep->bEndpointAddress;
			} else {
				if (wep == 0)
					wep = ep->bEndpointAddress;
			}
		}
		if (rep == 0 || wep == 0) {
			printf("Error: no valid ep: read=0x%02x, write=%02x\n",
			    rep, wep);
			break;
		}

#if 1
		error = libusb_claim_interface(handle, 0);
		if (error != 0) {
			printf("Error: Couldn't claim interface: %s\n",
			    libusb_error_name(error));
			break;
		}
#endif

		libusb_free_config_descriptor(conf);
		libusb_free_device_list(list, 1);

		m_handle = handle;
		m_readEp = rep;
		m_writeEp = wep;
		return true;
	}

	if (conf != NULL)
		libusb_free_config_descriptor(conf);
	if (handle != NULL)
		libusb_close(handle);
	if (list != NULL)
		libusb_free_device_list(list, 1);
#endif
	return false;
}

/**
 * USB クローズ
 */
void CUsbDev::Close()
{
#ifdef USE_LIBUSB1
	if (m_handle != NULL) {
#if 1
		libusb_release_interface(m_handle, 0);
#endif
		libusb_close(m_handle);
		m_handle = NULL;
	}
#endif
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
#ifdef USE_LIBUSB1
	if (m_handle != NULL) {
//		USBDeviceLock lock(m_handle);
		int numBytesXfer = libusb_control_transfer(m_handle, nType,
		    nRequest, nValue, nIndex,
		    static_cast<unsigned char *>(lpBuffer), cbBuffer,
		    XFER_TIMEOUT);
		if (numBytesXfer == cbBuffer)
			return cbBuffer;
#ifdef DEBUG
		printf("Error: control transfer failed: xfer size: %d\n",
		    numBytesXfer);
#endif
	}
#endif
	return -1;
}

/**
 * データ送信
 * @param[in] lpBuffer バッファ
 * @param[in] cbBuffer バッファ長
 * @return サイズ
 */
int CUsbDev::WriteBulk(const void* lpBuffer, int cbBuffer)
{
#ifdef USE_LIBUSB1
	if (m_handle != NULL) {
//		USBDeviceLock lock(m_handle);
		int numBytesWrite;
		int error = libusb_bulk_transfer(m_handle, m_writeEp,
		    static_cast<unsigned char *>(const_cast<void *>(lpBuffer)),
		    cbBuffer, &numBytesWrite, XFER_TIMEOUT);
		if (error == 0)
			return numBytesWrite;
#ifdef DEBUG
		printf("Error: bulk write failed: %s\n",
		    libusb_error_name(error));
#endif
	}
#endif
	return -1;
}

/**
 * データ受信
 * @param[out] lpBuffer バッファ
 * @param[in] cbBuffer バッファ長
 * @return サイズ
 */
int CUsbDev::ReadBulk(void* lpBuffer, int cbBuffer)
{
#ifdef USE_LIBUSB1
	if (m_handle != NULL) {
//		USBDeviceLock lock(m_handle);
		int numBytesRead;
		int error = libusb_bulk_transfer(m_handle, m_readEp,
		    static_cast<unsigned char *>(lpBuffer), cbBuffer,
		    &numBytesRead, XFER_TIMEOUT);
		if (error == 0)
			return numBytesRead;
#ifdef DEBUG
		printf("Error: bulk read failed: %s\n",
		    libusb_error_name(error));
#endif
	}
#endif
	return -1;
}
