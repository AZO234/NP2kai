/**
 * @file	c86ctlif.h
 * @brief	G.I.M.I.C アクセス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <map>
#include "../externalchip.h"

namespace c86ctl
{
	interface IRealChipBase;
	interface IGimic;
	interface IRealChip;
}

/**
 * @brief G.I.M.I.C アクセス クラス
 */
class C86CtlIf
{
public:
	C86CtlIf();
	~C86CtlIf();
	bool Initialize();
	void Deinitialize();
	void Reset();
	IExternalChip* GetInterface(IExternalChip::ChipType nChipType, UINT nClock);

private:
	HMODULE m_hModule;					/*!< モジュール ハンドル */
	c86ctl::IRealChipBase* m_pChipBase;	/*!< チップ ベース インスタンス */

	/**
	 * @brief チップ クラス
	 */
	class Chip : public IExternalChip
	{
	public:
		Chip(C86CtlIf* pC86CtlIf, c86ctl::IRealChip* pRealChip, c86ctl::IGimic* pGimic, ChipType nChipType, UINT nClock);
		virtual ~Chip();
		virtual ChipType GetChipType();
		virtual void Reset();
		virtual void WriteRegister(UINT nAddr, UINT8 cData);
		virtual INTPTR Message(UINT nMessage, INTPTR nParameter = 0);

	private:
		C86CtlIf* m_pC86CtlIf;				/*!< C86Ctl インスタンス */
		c86ctl::IRealChip* m_pRealChip;		/*!< チップ インスタンス */
		c86ctl::IGimic* m_pGimic;			/*!< G.I.M.I.C インスタンス */
		ChipType m_nChipType;				/*!< チップ タイプ */
		UINT m_nClock;						/*!< チップ クロック */
	};

	std::map<int, Chip*> m_chips;			/*!< チップ */
	void Detach(Chip* pChip);
	friend class Chip;
};
