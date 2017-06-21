/**
 * @file	usbdev.cpp
 * @brief	USB アクセス クラスの動作の定義を行います
 */

#include "compiler.h"
#include "usbdev.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/USBSpec.h>

/**
 * コンストラクタ
 */
CUsbDev::CUsbDev()
	: m_device(NULL)
	, m_interface(NULL)
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
 * @param[in] vid VID
 * @param[in] pid PID
 * @param[in] nIndex インデックス
 * @retval true 成功
 * @retval false 失敗
 */
bool CUsbDev::Open(unsigned int vid, unsigned int pid, unsigned int nIndex)
{
	// 探すデバイス
	const SInt32 usbVendor = vid;
	const SInt32 usbProduct = pid;

	if (nIndex != 0)
	{
		return false;
	}

	// Set up matching dictionary for class IOUSBDevice and its subclasses
	CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
	if (matchingDict == NULL)
	{
		printf("Couldn't create a USB matching dictionary\n");
		return false;
	}

	// Add the vendor and product IDs to the matching dictionary.
	// This is the second key in the table of device-matching keys of the
	// USB Common Class Specification
	CFDictionarySetValue(matchingDict, CFSTR(kUSBVendorName), CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbVendor));
	CFDictionarySetValue(matchingDict, CFSTR(kUSBProductName), CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbProduct));

	// インタフェイスを得る
	io_iterator_t iterator = 0;
	IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iterator);
	io_service_t usbDevice = IOIteratorNext(iterator);
	if (usbDevice == 0)
	{
		printf("Device not found\n");
		return false;
	}
	IOObjectRelease(iterator);

	// Create an intermediate plug-in
	IOCFPlugInInterface** plugInInterface = NULL;
	SInt32 score = 0;
	IOReturn kr = IOCreatePlugInInterfaceForService(usbDevice, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
	kr = IOObjectRelease(usbDevice);
	if ((kr != kIOReturnSuccess) || (plugInInterface == NULL))
	{
		printf("Unable to create a plug-in (%08x)\n", kr);
		return false;
	}

	// Now create the device interface
	IOUSBDeviceInterface** dev = NULL;
	HRESULT result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID*)&dev);
	(*plugInInterface)->Release(plugInInterface);
	if ((result != S_OK) || (dev == NULL))
	{
		printf("Couldn't create a device interface (%08x)\n", (int)result);
		return false;
	}

	// Open the device to change its state
	kr = (*dev)->USBDeviceOpen(dev);
	if (kr != kIOReturnSuccess)
	{
		printf("Unable to open device: %08x\n", kr);
		(*dev)->Release(dev);
		return false;
	}

	// Configure device
	kr = ConfigureDevice(dev);
	if (kr != kIOReturnSuccess)
	{
		printf("Unable to configure device: %08x\n", kr);
		(*dev)->USBDeviceClose(dev);
		(*dev)->Release(dev);
		return false;
	}

	// Placing the constant kIOUSBFindInterfaceDontCare into the following
	// fields of the IOUSBFindInterfaceRequest structure will allow you
	// to find all the interfaces
	IOUSBFindInterfaceRequest request;
	request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
	request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
	request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

	// Get an iterator for the interfaces on the device
	io_iterator_t iterator2;
	kr = (*dev)->CreateInterfaceIterator(dev, &request, &iterator2);
	while (1 /*EVER*/)
	{
		io_service_t usbInterface = IOIteratorNext(iterator2);
		if (usbInterface == 0)
		{
			break;
		}

		// Create an intermediate plug-in
		IOCFPlugInInterface** plugInInterface = NULL;
		SInt32 score;
		kr = IOCreatePlugInInterfaceForService(usbInterface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
		kr = IOObjectRelease(usbInterface);
		if ((kr != kIOReturnSuccess) || (plugInInterface == NULL))
		{
			printf("Unable to create a plug-in (%08x)\n", kr);
			continue;
		}

		// Now create the device interface for the interface
		IOUSBInterfaceInterface** interface = NULL;
		HRESULT result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID*)&interface);
		(*plugInInterface)->Release(plugInInterface);
		if ((result != S_OK) || (interface == NULL))
		{
			printf("Couldn't create a device interface for the interface(%08x)\n", (int) result);
			continue;
		}

		// Get interface class and subclass
		UInt8 interfaceClass;
		kr = (*interface)->GetInterfaceClass(interface, &interfaceClass);

		UInt8 interfaceSubClass;
		kr = (*interface)->GetInterfaceSubClass(interface, &interfaceSubClass);

		printf("Interface class: %d, subclass: %d\n", interfaceClass, interfaceSubClass);

		// Now open the interface. This will cause the pipes associated with
		// the endpoints in the interface descriptor to be instantiated
		kr = (*interface)->USBInterfaceOpen(interface);
		if (kr != kIOReturnSuccess)
		{
			printf("Unable to open interface (%08x)\n", kr);
			(*interface)->Release(interface);
			continue;
		}

		// Get the number of endpoints associated with this interface
		UInt8 interfaceNumEndpoints;
		kr = (*interface)->GetNumEndpoints(interface, &interfaceNumEndpoints);
		if (kr != kIOReturnSuccess)
		{
			printf("Unable to get number of endpoints (%08x)\n", kr);
			(*interface)->USBInterfaceClose(interface);
			(*interface)->Release(interface);
			continue;
		}
 
		printf("Interface has %d endpoints\n", interfaceNumEndpoints);
		// Access each pipe in turn, starting with the pipe at index 1
		// The pipe at index 0 is the default control pipe and should be
		// accessed using (*usbDevice)->DeviceRequest() instead
		for (int pipeRef = 1; pipeRef <= interfaceNumEndpoints; pipeRef++)
		{
			printf(" PipeRef %d: ", pipeRef);

			UInt8 direction;
			UInt8 number;
			UInt8 transferType;
			UInt16 maxPacketSize;
			UInt8 interval;
			kr = (*interface)->GetPipeProperties(interface, pipeRef, &direction, &number, &transferType, &maxPacketSize, &interval);
			if (kr != kIOReturnSuccess)
			{
				printf("Unable to get properties of pipe(%08x)\n", kr);
				continue;
			}

			const char* message;
			switch (direction)
			{
				case kUSBOut:
					message = "out";
					break;

				case kUSBIn:
					message = "in";
					break;

				case kUSBNone:
					message = "none";
					break;

				case kUSBAnyDirn:
					message = "any";
					break;

				default:
					message = "???";
					break;
			}
			printf("direction %s, ", message);

			switch (transferType)
			{
				case kUSBControl:
					message = "control";
					break;

				case kUSBIsoc:
					message = "isoc";
					break;

				case kUSBBulk:
					message = "bulk";
					break;

				case kUSBInterrupt:
					message = "interrupt";
					break;

				case kUSBAnyType:
					message = "any";
					break;

				default:
					message = "???";
					break;
			}
			printf("transfer type %s, maxPacketSize %d\n", message, maxPacketSize);
		}

		// Query G.I.M.I.C module info.
		m_device = dev;
		m_interface = interface;
		return true;
	}

	(*dev)->USBDeviceClose(dev);
	(*dev)->Release(dev);

	return false;
}

/**
 * こんてぃぐあ
 * @param[in] dev Device interface
 * @return IOReturn
 */
IOReturn CUsbDev::ConfigureDevice(IOUSBDeviceInterface** dev)
{
	// Get the number of configurations. The sample code always chooses
	// the first configuration (at index 0) but your code may need a
	// different one
	UInt8 numConfig = 0;
	IOReturn kr = (*dev)->GetNumberOfConfigurations(dev, &numConfig);
	if (!numConfig)
	{
		return -1;
	}

	//Get the configuration descriptor for index 0
	IOUSBConfigurationDescriptorPtr configDesc;
	kr = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &configDesc);
	if (kr != kIOReturnSuccess)
	{
		printf("Couldn't get configuration descriptor for index %d (err = %08x)\n", 0, kr);
		return kr;
	}

	// Set the device's configuration. The configuration value is found in
	// the bConfigurationValue field of the configuration descriptor
	kr = (*dev)->SetConfiguration(dev, configDesc->bConfigurationValue);
	if (kr != kIOReturnSuccess)
	{
		printf("Couldn't set configuration to value %d (err = %08x)\n", 0, kr);
		return kr;
	}
	return kIOReturnSuccess;
}

/**
 * USB クローズ
 */
void CUsbDev::Close()
{
	if (m_interface)
	{
		(*m_interface)->USBInterfaceClose(m_interface);
		(*m_interface)->Release(m_interface);
		m_interface = NULL;
	}

	if (m_device)
	{
		(*m_device)->USBDeviceClose(m_device);
		(*m_device)->Release(m_device);
		m_device = NULL;
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
	if (m_interface == NULL)
	{
		return -1;
	}

	IOUSBDevRequest req;
	req.bmRequestType = nType;
	req.bRequest = nRequest;
	req.wValue = nValue;
	req.wIndex = nIndex;
	req.wLength = cbBuffer;
	req.pData = lpBuffer;
	req.wLenDone = 0;

	IOReturn kr = (*m_interface)->ControlRequest(m_interface, 0, &req);
	if (kr != kIOReturnSuccess)
	{
		::printf("Unable to perform control request (%08x)\n", kr);
		return -1;
	}
	return static_cast<int>(req.wLenDone);
}

/**
 * データ送信
 * @param[in] lpBuffer バッファ
 * @param[in] cbBuffer バッファ長
 * @return サイズ
 */
int CUsbDev::WriteBulk(const void* lpBuffer, int cbBuffer)
{
	if (m_interface == NULL)
	{
		return -1;
	}

	IOReturn kr = (*m_interface)->WritePipe(m_interface, 1, const_cast<void*>(lpBuffer), static_cast<UInt32>(cbBuffer));
	if (kr != kIOReturnSuccess)
	{
		::printf("Unable to perform bulk write (%08x)\n", kr);
		return -1;
	}
	return static_cast<int>(cbBuffer);
}

/**
 * データ受信
 * @param[out] lpBuffer バッファ
 * @param[in] cbBuffer バッファ長
 * @return サイズ
 */
int CUsbDev::ReadBulk(void* lpBuffer, int cbBuffer)
{
	if (m_interface == NULL)
	{
		return -1;
	}

	UInt32 numBytesRead = static_cast<UInt32>(cbBuffer);
	IOReturn kr = (*m_interface)->ReadPipe(m_interface, 2, lpBuffer, &numBytesRead);
	if (kr != kIOReturnSuccess)
	{
		::printf("Unable to perform bulk read (%08x)\n", kr);
		return -1;
	}
	return static_cast<int>(numBytesRead);
}
