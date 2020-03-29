#include	"compiler.h"
#include	"np2.h"

void __msgbox(const char *title, const char *msg) {

#if !defined(_UNICODE)
	const TCHAR *_title = title;
	const TCHAR *_msg = msg;
#else
	TCHAR _title[256];
	TCHAR _msg[2048];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, title, -1,
												_title, NELEMENTS(_title));
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, msg, -1,
												_msg, NELEMENTS(_msg));
#endif
	MessageBox(NULL, _msg, _title, MB_OK);
}


// WinAPIだと Win95でバグあるの
int _loadstringresource(HINSTANCE hInstance, UINT uID,
										LPTSTR lpszBuffer, int nBufferMax)
{
	HMODULE	hModule;
	HRSRC	hRsrc;
	DWORD	dwResSize;
	HGLOBAL	hGlobal;
	UINT16	*pRes;
	DWORD	dwPos;
	int		nLength;

	hModule = (HMODULE)hInstance;
	hRsrc = FindResource(hModule, MAKEINTRESOURCE((uID >> 4) + 1), RT_STRING);
	if (hRsrc == NULL)
	{
		return 0;
	}
	dwResSize = SizeofResource(hModule, hRsrc);
	hGlobal = LoadResource(hModule, hRsrc);
	if (hGlobal == NULL)
	{
		return 0;
	}
	pRes = (UINT16 *)LockResource(hGlobal);
	dwPos = 0;
	uID = uID & 15;
	while((uID) && (dwPos < dwResSize))
	{
		dwPos += pRes[dwPos] + 1;
		uID--;
	}
	if (dwPos >= dwResSize)
	{
		return 0;
	}

	nLength = pRes[dwPos];
	dwPos++;
	nLength = min(nLength, (int)(dwResSize - dwPos));
#if defined(_UNICODE)
	if ((lpszBuffer != NULL) && (nBufferMax > 0))
	{
		nBufferMax--;
		nLength = min(nLength, nBufferMax);
		if (nLength)
		{
			CopyMemory(lpszBuffer, pRes + dwPos, nLength * sizeof(UINT16));
		}
		lpszBuffer[nLength] = '\0';
	}
#else
	if ((lpszBuffer != NULL) && (nBufferMax > 0))
	{
		nBufferMax--;
		if (nBufferMax == 0)
		{
			nLength = 0;
		}
	}
	else
	{
		lpszBuffer = NULL;
		nBufferMax = 0;
	}
	nLength = WideCharToMultiByte(CP_ACP, 0, (WCHAR *)(pRes + dwPos), nLength,
										lpszBuffer, nBufferMax, NULL, NULL);
	if (lpszBuffer)
	{
		lpszBuffer[nLength] = '\0';
	}
#endif
	return(nLength);
}

// WinAPIだと Win95でバグあるの
LPTSTR _lockstringresource(HINSTANCE hInstance, LPCTSTR lpcszString)
{
	LPTSTR	lpszRet;
	int		nSize;

	lpszRet = NULL;
	if (HIWORD(lpcszString))
	{
		nSize = (lstrlen(lpcszString) + 1) * sizeof(TCHAR);
		lpszRet = (LPTSTR)_MALLOC(nSize, "");
		if (lpszRet)
		{
			CopyMemory(lpszRet, lpcszString, nSize);
		}
	}
	else if (LOWORD(lpcszString))
	{
		nSize = loadstringresource((UINT)lpcszString, NULL, 0);
		if (nSize)
		{
			lpszRet = (LPTSTR)_MALLOC((nSize + 1) * sizeof(TCHAR), "");
			if (lpszRet)
			{
				loadstringresource((UINT)lpcszString, lpszRet, nSize + 1);
			}
		}
	}
	return lpszRet;
}


// ----

int loadstringresource(UINT uID, LPTSTR lpszBuffer, int nBufferMax)
{
	return _loadstringresource(g_hInstance, uID, lpszBuffer, nBufferMax);
}

LPTSTR lockstringresource(LPCTSTR lpcszString)
{
	return  _lockstringresource(g_hInstance, lpcszString);
}

void unlockstringresource(LPTSTR lpszString)
{
	if (lpszString)
	{
		_MFREE(lpszString);
	}
}

