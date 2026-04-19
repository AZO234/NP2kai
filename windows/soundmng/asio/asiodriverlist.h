/**
 * @file	asiodriverlist.h
 * @brief	ASIO ドライバ リスト クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>

interface IASIO;

/**
 * ASIO ドライバ情報
 */
struct AsioDriverInfo
{
	CLSID clsid;					/*!< クラス ID */
	TCHAR szDllPath[MAX_PATH];		/*!< DLL パス */
	TCHAR szDriverName[128];		/*!< ドライバ名 */
};

/**
 * @brief ASIO ドライバ リスト クラス
 */
class AsioDriverList : public std::vector<AsioDriverInfo>
{
public:
	void EnumerateDrivers();
	IASIO* OpenDriver(LPCTSTR lpDriverName);

private:
	static bool FindDrvPath(LPCTSTR lpClsId, LPTSTR lpDllPath, UINT cchDllPath);
};
