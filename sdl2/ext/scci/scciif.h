/**
 * @file	scciif.h
 * @brief	SCCI アクセス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "../externalchip.h"

namespace scci
{
	class SoundChip;
	class SoundInterfaceManager;
}

/**
 * @brief SCCI アクセス クラス
 */
class CScciIf
{
public:
	CScciIf();
	~CScciIf();
	bool Initialize();
	void Deinitialize();
	void Reset();
	IExternalChip* GetInterface(IExternalChip::ChipType nChipType, UINT nClock);

private:
	scci::SoundInterfaceManager* m_pManager;	/*!< マネージャ */

	/**
	 * @brief チップ クラス
	 */
	class Chip : public IExternalChip
	{
	public:
		Chip(CScciIf* pScciIf, scci::SoundChip* pSoundChip);
		virtual ~Chip();
		operator scci::SoundChip*();
		virtual ChipType GetChipType();
		virtual void Reset();
		virtual void WriteRegister(UINT nAddr, UINT8 cData);
		virtual INTPTR Message(UINT nMessage, INTPTR nParameter = 0);

	private:
		CScciIf* m_pScciIf;				/*!< 親インスタンス */
		scci::SoundChip* m_pSoundChip;	/*!< チップ インスタンス */
	};

	void Detach(Chip* pChip);
	friend class Chip;
};
