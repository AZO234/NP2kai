/**
 * @file	c_combodata.h
 * @brief	コンボ データ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "misc/DlgProc.h"

/**
 * @brief コンボ データ クラス
 */
class CComboData : public CComboBoxProc
{
public:
	/**
	 * @brief エントリー
	 */
	struct Value
	{
		UINT32 nNumber;				/*!< 数値 */
		UINT32 nItemData;			/*!< 値 */
	};

	/**
	 * @brief エントリー
	 */
	struct Entry
	{
		LPCTSTR lpcszString;		/*!< 文字列 */
		UINT32 nItemData;			/*!< 値 */
	};

	void Add(const UINT32* lpValues, UINT cchValues);
	void Add(const Value* lpValues, UINT cchValues);
	void Add(const Entry* lpEntries, UINT cchEntries);
	int Add(UINT32 nValue);
	int Add(UINT32 nValue, UINT32 nItemData);
	int Add(LPCTSTR lpString, UINT32 nItemData);
	int FindItemData(UINT32 nValue) const;
	bool SetCurItemData(UINT32 nValue);
	UINT32 GetCurItemData(UINT32 nDefault) const;
};
