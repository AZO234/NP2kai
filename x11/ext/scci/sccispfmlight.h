/**
 * @file	sccispfmlight.h
 * @brief	Interface of accessing SPFM Light
 */

#pragma once

#include "sccisoundchip.h"
#include "sccisoundinterface.h"
#include "misc/guard.h"
#include "misc/threadbase.h"
#include "misc/tty.h"

namespace scci
{

/**
 * @brief The class of SPFM Light
 */
class CSpfmLight : public CSoundInterface, protected CThreadBase
{
public:
	CSpfmLight(CSoundInterfaceManager* pManager, const std::oemstring& deviceName, UINT nDelay);
	virtual ~CSpfmLight();

	virtual bool Initialize();
	virtual void Deinitialize();
	virtual size_t AddRef();
	virtual size_t Release();
	virtual void Add(const SCCI_SOUND_CHIP_INFO& info);

	virtual bool isSupportLowLevelApi();
	virtual bool setData(const unsigned char* pData, size_t dSendDataLen);
	virtual size_t getData(unsigned char* pData, size_t dGetDataLen);
	virtual bool setDelay(UINT dDelay);
	virtual UINT getDelay();
	virtual bool reset();

protected:
	virtual bool Task();

private:
	/**
	 * @brief event
	 */
	struct QueData
	{
		UINT nTimestamp;			/*!< Timestamp */
		UINT nData;					/*!< data */
	};

	/**
	 * @brief The class of Chip
	 */
	class Chip : public CSoundChip
	{
	public:
		Chip(CSoundInterface* pInterface, const SCCI_SOUND_CHIP_INFO& info);
		virtual bool setRegister(UINT dAddr, UINT dData);
		virtual bool init();
	};

	UINT m_nDelay;					/*!< delay time */
	bool m_bReseted;				/*!< Reset flag */
	CTty m_serial;					/*!< Serial */
	CGuard m_ttyGuard;				/*!< The quard of accessing USB */
	CGuard m_queGuard;				/*!< The quard of que */
	UINT m_nQueIndex;				/*!< The position in que */
	UINT m_nQueCount;				/*!< The count in que */
	QueData m_que[0x400];			/*!< que */

	ssize_t Read(unsigned char* lpBuffer, ssize_t cbBuffer, UINT nUntil);
	ssize_t Write(const unsigned char* lpBuffer, ssize_t cbBuffer, UINT nUntil);
	bool AddEvent(UINT nData);

	friend class Chip;
};

}	// namespace scci
