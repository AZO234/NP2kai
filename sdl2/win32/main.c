/**
 * @file	main.c
 * @brief	メイン
 */

#include "compiler.h"
#include "np2.h"
#include "..\fontmng.h"

#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "zlib.lib")

#if !defined(RESOURCE_US)
#pragma comment(lib, "SDL2_ttf.lib")
#endif

/**
 * メイン
 * @param[in] argc 引数
 * @param[in] argv 引数
 * @return リザルト コード
 */
int main(int argc, char *argv[])
{
	UINT nLength;
	TCHAR szFont[MAX_PATH];

	nLength = GetWindowsDirectory(szFont, SDL_arraysize(szFont));
	lstrcpy(szFont + nLength, TEXT("\\Fonts\\msgothic.ttc"));
	fontmng_setdeffontname(szFont);

	return np2_main(argc, argv);
}
