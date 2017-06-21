/**
 * @file	c86ctlc86box.h
 * @brief	Interface of C86BOX
 */

#pragma once

#include <vector>
#include "c86ctlrealchipbase.h"
#include "misc/guard.h"
#include "misc/threadbase.h"
#include "misc/usbdev.h"

namespace c86ctl
{

/**
 * @brief The class of C86BOX
 */
class CC86Box : public CRealChipBase::CDevice, protected CThreadBase
{
public:
	CC86Box(UINT nIndex);
	virtual ~CC86Box();

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
	 * @brief The class of the chip
	 */
	class Chip3 : public IRealChip3
	{
	public:
		Chip3(CC86Box* pC86Box, UINT nDevId, ChipType nChipType);

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
		CC86Box* m_pC86Box;			/*!< The instance of the device */
		UINT m_nDevId;				/*!< The type of devices */
		ChipType m_nChipType;		/*!< The type of chip */
		UINT8 m_sReg[0x200];		/*!< register */

		CC86Box* GetDevice();
	};

	size_t m_nRef;					/*!< The reference counter */
	UINT m_nIndex;					/*!< The index of devices */
	CUsbDev m_usb;					/*!< USB */
	CGuard m_usbGuard;				/*!< The quard of accessing USB */
	CGuard m_queGuard;				/*!< The quard of que */
	UINT m_nQueIndex;				/*!< The position in que */
	UINT m_nQueCount;				/*!< The count in que */
	UINT m_que[0x400];				/*!< que */
	std::vector<Chip3*> m_chips;	/*!< The list of chips */

	C86CtlErr Transaction(const void* lpOutput, int cbOutput, void* lpInput = NULL, int cbInput = 0);
	C86CtlErr Reset();
	void Out(UINT nDevId, UINT nAddr, UINT8 cData);

	friend class Chip3;
};

}	// namespace c86ctl
