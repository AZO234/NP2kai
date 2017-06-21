/**
 * @file	externalchipmanager.h
 * @brief	外部チップ管理クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include "externalchip.h"
#include "c86ctl/c86ctlif.h"
#include "scci/scciif.h"

/**
 * @brief 外部チップ管理クラス
 */
class CExternalChipManager
{
public:
	static CExternalChipManager* GetInstance();

	CExternalChipManager();
	void Initialize();
	void Deinitialize();
	IExternalChip* GetInterface(IExternalChip::ChipType nChipType, UINT nClock);
	void Release(IExternalChip* pChip);
	void Reset();
	void Mute(bool bMute);

private:
	static CExternalChipManager sm_instance;	/*!< 唯一のインスタンスです */
	std::vector<IExternalChip*> m_chips;		/*!< モジュール */

	C86CtlIf m_c86ctl;							/*!< C86Ctl インスタンス */
	CScciIf m_scci;								/*!< SCCI インスタンス */
	IExternalChip* GetInterfaceInner(IExternalChip::ChipType nChipType, UINT nClock);
};

/**
 * インスタンスを得る
 * @return インスタンス
 */
inline CExternalChipManager* CExternalChipManager::GetInstance()
{
	return &sm_instance;
}
