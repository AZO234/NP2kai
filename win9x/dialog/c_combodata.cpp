/**
 * @file	c_combodata.cpp
 * @brief	コンボ データ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "c_combodata.h"

/**
 * 追加
 * @param[in] lpValues 値の配列
 * @param[in] cchValues 値の数
 */
void CComboData::Add(const UINT32* lpValues, UINT cchValues)
{
	for (UINT i = 0; i < cchValues; i++)
	{
		Add(lpValues[i]);
	}
}

/**
 * 追加
 * @param[in] lpValues 値の配列
 * @param[in] cchValues 値の数
 */
void CComboData::Add(const Value* lpValues, UINT cchValues)
{
	for (UINT i = 0; i < cchValues; i++)
	{
		Add(lpValues[i].nNumber, lpValues[i].nItemData);
	}
}

/**
 * 追加
 * @param[in] lpEntries エントリの配列
 * @param[in] cchEntries エントリの数
 */
void CComboData::Add(const Entry* lpEntries, UINT cchEntries)
{
	for (UINT i = 0; i < cchEntries; i++)
	{
		std::tstring rString(LoadTString(lpEntries[i].lpcszString));
		Add(rString.c_str(), lpEntries[i].nItemData);
	}
}

/**
 * 追加
 * @param[in] nValue 値
 * @return インデックス
 */
int CComboData::Add(UINT32 nValue)
{
	return Add(nValue, nValue);
}

/**
 * 追加
 * @param[in] nValue 値
 * @param[in] nItemData データ
 * @return インデックス
 */
int CComboData::Add(UINT32 nValue, UINT32 nItemData)
{
	TCHAR szStr[16];
	wsprintf(szStr, TEXT("%u"), nValue);
	return Add(szStr, nItemData);
}

/**
 * 追加
 * @param[in] lpString 表示名
 * @param[in] nItemData データ
 * @return インデックス
 */
int CComboData::Add(LPCTSTR lpString, UINT32 nItemData)
{
	const int nIndex = AddString(lpString);
	if (nIndex >= 0)
	{
		SetItemData(nIndex, static_cast<DWORD_PTR>(nItemData));
	}
	return nIndex;
}

/**
 * アイテム検索
 * @param[in] nValue 値
 * @return インデックス
 */
int CComboData::FindItemData(UINT32 nValue) const
{
	const int nItems = GetCount();
	for (int i = 0; i < nItems; i++)
	{
		if (GetItemData(i) == nValue)
		{
			return i;
		}
	}
	return CB_ERR;
}

/**
 * カーソル設定
 * @param[in] nValue 値
 * @retval true 成功
 * @retval false 失敗
 */
bool CComboData::SetCurItemData(UINT32 nValue)
{
	const int nIndex = FindItemData(nValue);
	if (nIndex == CB_ERR)
	{
		return false;
	}
	SetCurSel(nIndex);
	return true;
}

/**
 * カーソルの値を取得
 * @param[in] nDefault デフォルト値
 * @return 値
 */
UINT32 CComboData::GetCurItemData(UINT32 nDefault) const
{
	const int nIndex = GetCurSel();
	if (nIndex >= 0)
	{
		return static_cast<UINT32>(GetItemData(nIndex));
	}
	return nDefault;
}
