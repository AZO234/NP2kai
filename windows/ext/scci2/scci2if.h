/**
 * @file	scciif.h
 * @brief	SCCI アクセス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "../externalchip.h"

class Scci2SoundChip;
class Scci2SoundInterfaceManager;

/**
 * @brief SCCI アクセス クラス
 */
class CScci2If
{
public:
	CScci2If();
	~CScci2If();
	bool Initialize();
	void Deinitialize();
	void Reset();
	IExternalChip* GetInterface(IExternalChip::ChipType nChipType, UINT nClock);

private:
	HMODULE m_hModule;					/*!< モジュール */
	Scci2SoundInterfaceManager* m_pManager;	/*!< マネージャ */

	/**
	 * @brief チップ クラス
	 */
	class Chip : public IExternalChip
	{
	public:
		Chip(CScci2If* pScciIf, Scci2SoundChip* pSoundChip);
		virtual ~Chip();
		operator Scci2SoundChip*();
		virtual ChipType GetChipType();
		virtual void Reset();
		virtual void WriteRegister(UINT nAddr, UINT8 cData);
		virtual INTPTR Message(UINT nMessage, INTPTR nParameter = 0);

	private:
		CScci2If* m_pScciIf;			/*!< 親インスタンス */
		Scci2SoundChip* m_pSoundChip;	/*!< チップ インスタンス */
	};

	void Detach(Chip* pChip);
	friend class Chip;
};
