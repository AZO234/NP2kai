/**
 * @file	c86ctlrealchipbase.h
 * @brief	Interface of IRealChipBase
 */

#pragma once

#include <vector>
#include "c86ctl.h"

namespace c86ctl
{

/**
 * @brief The class of the chip base
 */
class CRealChipBase : public IRealChipBase
{
public:
	/**
	 * @brief The class of the device
	 */
	class CDevice : public IRealChipBase
	{
	public:
		virtual ~CDevice();
	};

	static CRealChipBase* GetInstance();

	CRealChipBase();
	virtual ~CRealChipBase();

	// IRealUnknown
	virtual size_t AddRef();
	virtual size_t Release();

	// IRealChipBase
	virtual C86CtlErr initialize();
	virtual C86CtlErr deinitialize();
	virtual size_t getNumberOfChip();
	virtual C86CtlErr getChipInterface(size_t id, IID riid, void** ppi);

private:
	static CRealChipBase sm_instance;		/*!< instance */
	size_t m_nRef;							/*!< The reference counter */
	std::vector<CDevice*> m_devices;		/*!< The instance of devices */
};

/**
 * Gets instance
 * @return The instance of sound manager
 */
inline CRealChipBase* CRealChipBase::GetInstance()
{
	return &sm_instance;
}

}	// namespace c86ctl
