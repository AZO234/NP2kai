/**
 * @file	c86ctlgimic.h
 * @brief	Interface of G.I.M.I.C
 */

#pragma once

#include "c86ctlrealchipbase.h"
#include "misc/guard.h"
#include "misc/threadbase.h"
#include "misc/usbdev.h"

namespace c86ctl
{

/**
 * @brief The class of G.I.M.I.C
 */
class CGimic : public CRealChipBase::CDevice, protected CThreadBase
{
public:
	CGimic(UINT nIndex);
	virtual ~CGimic();

	// IRealUnknown
	virtual size_t AddRef();
	virtual size_t Release();

	// IRealChipBase
	virtual C86CtlErr initialize();
	virtual C86CtlErr deinitialize();
	virtual size_t getNumberOfChip();
	virtual C86CtlErr getChipInterface(size_t id, IID riid, void** ppi);

protected:
	virtual bool Task();

private:
	/**
	 * @brief FM data
	 */
	struct FMDATA
	{
		UINT16 wAddr;		/*!< address */
		UINT8 cData;		/*!< data */
		UINT8 cPadding;		/*!< padding */
	};

	/**
	 * @brief The class of the gimic
	 */
	class Gimic2 : public IGimic2
	{
	public:
		Gimic2(CGimic* pDevice);

		// IRealUnknown
		virtual size_t AddRef();
		virtual size_t Release();

		// IGimic
		C86CtlErr getFWVer(UINT* pnMajor, UINT* pnMinor, UINT* pnRev, UINT* pnBuild);
		C86CtlErr getMBInfo(Devinfo* pInfo);
		C86CtlErr getModuleInfo(Devinfo* pInfo);
		C86CtlErr setSSGVolume(UINT8 cVolume);
		C86CtlErr getSSGVolume(UINT8* pcVolume);
		C86CtlErr setPLLClock(UINT nClock);
		C86CtlErr getPLLClock(UINT* pnClock);

		// IGimic2
		C86CtlErr getModuleType(ChipType* pnType);

	private:
		CGimic* m_pDevice;		/*!< device */
		CGimic* GetDevice();
	};

	/**
	 * @brief The class of the chip
	 */
	class Chip3 : public IRealChip3
	{
	public:
		Chip3(CGimic* pDevice);

		// IRealUnknown
		virtual size_t AddRef();
		virtual size_t Release();

		// IRealChip
		virtual C86CtlErr reset();
		virtual void out(UINT nAddr, UINT8 cData);
		virtual UINT8 in(UINT nAddr);

		// IRealChip2
		virtual C86CtlErr getChipStatus(UINT nAddr, UINT8* pcStatus);
		virtual void directOut(UINT nAddr, UINT8 cData);

		// IRealChip3
		virtual C86CtlErr getChipType(ChipType* pnType);

	private:
		CGimic* m_pDevice;		/*!< device */
		UINT8 m_sReg[0x200];	/*!< register */
		CGimic* GetDevice();
	};

	size_t m_nRef;			/*!< The reference counter */
	UINT m_nIndex;			/*!< The index of devices */
	ChipType m_nChipType;	/*!< The type of chip */
	UINT m_nQueIndex;		/*!< The position in que */
	UINT m_nQueCount;		/*!< The count in que */
	CUsbDev m_usb;			/*!< USB */
	CGuard m_usbGuard;		/*!< The guard of accessing USB */
	CGuard m_queGuard;		/*!< The guard of que */
	Gimic2 m_gimic2;		/*!< gimic2 */
	Chip3 m_chip3;			/*!< chip3 */
	FMDATA m_que[0x400];	/*!< que */

	C86CtlErr Transaction(const void* lpOutput, int cbOutput, void* lpInput = NULL, int cbInput = 0);
	C86CtlErr Reset();
	C86CtlErr GetInfo(UINT8 cParam, c86ctl::Devinfo* pInfo);
	static void TailZeroFill(char* lpBuffer, size_t cbBuffer);
	UINT GetChipAddr(UINT nAddr) const;
	void Out(UINT nAddr, UINT8 cData);

	friend class Gimic2;
	friend class Chip3;
};

}	// namespace c86ctl
