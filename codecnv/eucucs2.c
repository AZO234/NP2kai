/**
 * @file	eucucs2.c
 * @brief	Implementation of converting EUC to UTF-16
 */

#include "compiler.h"
#include "codecnv.h"

static UINT euctoucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);

/**
 * Maps a EUC string to a UTF-16 string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_euctoucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
{
	UINT nLength;

	if (lpInput == NULL)
	{
		return 0;
	}

	if (cchOutput == 0)
	{
		lpOutput = NULL;
		cchOutput = (UINT)-1;
	}

	if (cchInput != (UINT)-1)
	{
		// Binary mode
		return euctoucs2(lpOutput, cchOutput, lpInput, cchInput);
	}
	else
	{
		// String mode
		nLength = euctoucs2(lpOutput, cchOutput - 1, lpInput, (UINT)strlen(lpInput));
		if (lpOutput)
		{
			lpOutput[nLength] = '\0';
		}
		return nLength + 1;
	}
}

/**
 * Maps a EUC string to a UTF-16 string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT euctoucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
{
	UINT nLength;
	char* pWork;

	nLength = codecnv_euctosjis(NULL, 0, lpInput, cchInput);
	if (nLength)
	{
		pWork = (char*)malloc(nLength * sizeof(char));
		nLength = codecnv_euctosjis(pWork, nLength, lpInput, cchInput);
		nLength = codecnv_sjistoucs2(lpOutput, cchOutput, pWork, nLength);
		free(pWork);
	}
	return nLength;
}
