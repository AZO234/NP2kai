/**
 * @file	c86ctlrealchipbase.cpp
 * @brief	Implementation of IRealChipBase
 */

#include "compiler.h"
#include "c86ctlrealchipbase.h"
#include "c86ctlc86box.h"
#include "c86ctlgimic.h"

namespace c86ctl
{

/*! instance */
CRealChipBase CRealChipBase::sm_instance;

/**
 * Gets interfaces
 * @param[in] riid The interface ID
 * @param[out] ppi The pointer of the interface
 * @return C86CTL_ERR
 */
C86CtlErr CreateInstance(IID riid, void** ppi)
{
	if (ppi == NULL)
	{
		return C86CTL_ERR_INVALID_PARAM;
	}

	if (riid != IID_IRealChipBase)
	{
		return C86CTL_ERR_UNSUPPORTED;
	}

	CRealChipBase* pBase = CRealChipBase::GetInstance();
	pBase->AddRef();
	*ppi = pBase;
	return C86CTL_ERR_NONE;
}

/**
 * Constructor
 */
CRealChipBase::CRealChipBase()
	: m_nRef(0)
{
}

/**
 * Destructor
 */
CRealChipBase::~CRealChipBase()
{
}

/**
 * Increments the reference count
 * @return The new reference count
 */
size_t CRealChipBase::AddRef()
{
	m_nRef++;
	return m_nRef;
}

/**
 * Decrements the reference count
 * @return The new reference count
 */
size_t CRealChipBase::Release()
{
	m_nRef--;
	return m_nRef;
}

/**
 * Initialize
 * @return C86CTL_ERR
 */
C86CtlErr CRealChipBase::initialize()
{
	for (UINT i = 0; i < 1; i++)
	{
		CGimic* pGimic = new CGimic(i);
		if (pGimic->initialize() != C86CTL_ERR_NONE)
		{
			pGimic->deinitialize();
			delete pGimic;
			continue;
		}
		m_devices.push_back(pGimic);
	}

	for (UINT i = 0; i < 1; i++)
	{
		CC86Box* pC86Box = new CC86Box(i);
		if (pC86Box->initialize() != C86CTL_ERR_NONE)
		{
			pC86Box->deinitialize();
			delete pC86Box;
			continue;
		}
		m_devices.push_back(pC86Box);
	}

	return C86CTL_ERR_NONE;
}

/**
 * Deinitialize
 * @return C86CTL_ERR
 */
C86CtlErr CRealChipBase::deinitialize()
{
	for (std::vector<CDevice*>::iterator it = m_devices.begin(); it != m_devices.end(); ++it)
	{
		CDevice* pDevice = *it;
		pDevice->deinitialize();
		delete pDevice;
	}
	m_devices.clear();

	return C86CTL_ERR_NONE;
}

/**
 * Gets the count of chips
 * @return The chips
 */
size_t CRealChipBase::getNumberOfChip()
{
	size_t nChips = 0;
	for (std::vector<CDevice*>::iterator it = m_devices.begin(); it != m_devices.end(); ++it)
	{
		nChips += (*it)->getNumberOfChip();
	}
	return nChips;
}

/**
 * Gets interfaces
 * @param[in] id ID
 * @param[in] riid The interface ID
 * @param[out] ppi The pointer of the interface
 * @return C86CTL_ERR
 */
C86CtlErr CRealChipBase::getChipInterface(size_t id, IID riid, void** ppi)
{
	if (ppi == NULL)
	{
		return C86CTL_ERR_INVALID_PARAM;
	}

	for (std::vector<CDevice*>::iterator it = m_devices.begin(); it != m_devices.end(); ++it)
	{
		CDevice* pDevice = *it;
		const size_t nChips = pDevice->getNumberOfChip();
		if (id < nChips)
		{
			return pDevice->getChipInterface(id, riid, ppi);
		}
		id -= nChips;
	}
	return C86CTL_ERR_NODEVICE;
}

/**
 * Destructor
 */
CRealChipBase::CDevice::~CDevice()
{
}

}	// namespace c86ctl
