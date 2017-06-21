/**
 * @file	utf8ucs2.c
 * @brief	Implementation of converting UTF-8 to UCS2
 */

#include "compiler.h"
#include "codecnv.h"

static UINT utf8toucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);

/**
 * Maps a UTF-8 string to a UTF-16 string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_utf8toucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
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
		return utf8toucs2(lpOutput, cchOutput, lpInput, cchInput);
	}
	else
	{
		// String mode
		nLength = utf8toucs2(lpOutput, cchOutput - 1, lpInput, (UINT)strlen(lpInput));
		if (lpOutput)
		{
			lpOutput[nLength] = '\0';
		}
		return nLength + 1;
	}
}

/**
 * Maps a UTF-8 string to a UTF-16 string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT utf8toucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
{
	UINT nRemain;
	UINT c;
	int nBits;

	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0))
	{
		c = *lpInput++;
		cchInput--;

		if (c & 0x80)
		{
			nBits = 0;
			while ((nBits < 6) && (c & (0x80 >> nBits)))
			{
				nBits++;
			}

			c &= (0x7f >> nBits);
			nBits--;

			while ((nBits > 0) && (cchInput > 0) && (((*lpInput) & 0xc0) == 0x80))
			{
				c = (c << 6) | ((*lpInput++) & 0x3f);
				cchInput--;
				nBits--;
			}
		}

		nRemain--;
		if (lpOutput)
		{
			*lpOutput++ = (UINT16)c;
		}
	}
	return (UINT)(cchOutput - nRemain);
}
