/**
 * @file	asiodriverlist.cpp
 * @brief	ASIO ドライバ リスト クラスの動作の定義を行います
 */

#include "compiler.h"
#include "asiodriverlist.h"
#include <shlwapi.h>
#include <atlbase.h>
#include "asiosdk.h"

#pragma comment(lib, "shlwapi.lib")

/**
 * ドライバを列挙する
 */
void AsioDriverList::EnumerateDrivers()
{
	clear();
	CRegKey asio;
	if (asio.Open(HKEY_LOCAL_MACHINE, TEXT("software\\asio"), KEY_ENUMERATE_SUB_KEYS) == ERROR_SUCCESS)
	{
		for (DWORD dwIndex = 0; ; dwIndex++)
		{
			TCHAR szKeyName[MAX_PATH];
			if (::RegEnumKey(asio, dwIndex, szKeyName, _countof(szKeyName)) != ERROR_SUCCESS)
			{
				break;
			}

			AsioDriverInfo info;
			ZeroMemory(&info, sizeof(info));

			CRegKey drv;
			if (drv.Open(asio, szKeyName, KEY_READ) != ERROR_SUCCESS)
			{
				continue;
			}

			TCHAR szClsId[256];
			ULONG nSize = _countof(szClsId);
			if (drv.QueryValue(szClsId, TEXT("clsid"), &nSize) != ERROR_SUCCESS)
			{
				continue;
			}
			if (!FindDrvPath(szClsId, info.szDllPath, _countof(info.szDllPath)))
			{
				continue;
			}

			USES_CONVERSION;
			CLSID clsid;
			if (CLSIDFromString(T2W(szClsId), &clsid) == S_OK)
			{
				info.clsid = clsid;
			}

			nSize = _countof(info.szDriverName);
			if (drv.QueryValue(info.szDriverName, TEXT("description"), &nSize) != ERROR_SUCCESS)
			{
				::lstrcpyn(info.szDriverName, szKeyName, _countof(info.szDriverName));
			}
			push_back(info);
		}
	}
}

/**
 * デバイス パスを取得
 * @param[in] lpClsId クラス ID
 * @param[out] lpDllPath DLL パス
 * @param[in] cchDllPath DLL バッファ長さ
 * @retval true 成功
 * @retval false 失敗
 */
bool AsioDriverList::FindDrvPath(LPCTSTR lpClsId, LPTSTR lpDllPath, UINT cchDllPath)
{
	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, TEXT("clsid\\%s\\InprocServer32"), lpClsId);

	CRegKey inproc;
	if (inproc.Open(HKEY_CLASSES_ROOT, szPath, KEY_READ) != ERROR_SUCCESS)
	{
		return false;
	}

	ULONG nSize = cchDllPath;
	if (inproc.QueryValue(lpDllPath, NULL, &nSize) != ERROR_SUCCESS)
	{
		return false;
	}

	if (!::PathFileExists(lpDllPath))
	{
		return false;
	}
	return true;
}

/**
 * ドライバをロード
 * @param[in] lpDriverName ドライバ名
 * @return インスタンス
 */
IASIO* AsioDriverList::OpenDriver(LPCTSTR lpDriverName)
{
	AsioDriverList::const_iterator it = begin();
	while ((it != end()) && (::lstrcmpi(it->szDriverName, lpDriverName) != 0))
	{
		++it;
	}
	if (it != end())
	{
		VOID* pAsio = NULL;
		if (::CoCreateInstance(it->clsid, 0, CLSCTX_INPROC_SERVER, it->clsid, &pAsio) == S_OK)
		{
			return static_cast<IASIO*>(pAsio);
		}
	}
	return NULL;
}
