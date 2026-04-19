/**
 * @file	tstring.h
 * @brief	文字列クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <string>

namespace std
{
#ifdef _UNICODE
typedef wstring			tstring;				//!< tchar string 型定義
#else	// _UNICODE
typedef string			tstring;				//!< tchar string型定義
#endif	// _UNICODE
}

std::tstring LoadTString(UINT uID);
std::tstring LoadTString(LPCTSTR lpString);
