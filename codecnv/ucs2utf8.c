/**
 * @file	ucs2utf8.c
 * @brief	Implementation of converting UCS2 to UTF-8
 */

#include "compiler.h"
#include "codecnv.h"

static UINT ucs2len(const UINT16 *lpString);
static UINT ucs2toutf8(char *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput);

/**
 * Maps a UTF-16 string to a UTF-8 string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_ucs2toutf8(char *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput)
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
		return ucs2toutf8(lpOutput, cchOutput, lpInput, cchInput);
	}
	else
	{
		// String mode
		nLength = ucs2toutf8(lpOutput, cchOutput - 1, lpInput, ucs2len(lpInput));
		if (lpOutput)
		{
			lpOutput[nLength] = '\0';
		}
		return nLength + 1;
	}
}

/**
 * Get the length of a string
 * @param[in] lpString Null-terminated string
 * @return the number of characters in lpString
 */
static UINT ucs2len(const UINT16 *lpString)
{
	const UINT16 *p = lpString;
	while (*p != 0)
	{
		p++;
	}
	return (UINT)(p - lpString);
}

/**
 * Maps a UTF-16 string to a UTF-8 string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT ucs2toutf8(char *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput)
{
	UINT nRemain;
	UINT c;

	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0))
	{
		c = *lpInput++;
		cchInput--;

		if (c < 0x80)
		{
			nRemain--;
			if (lpOutput)
			{
				*lpOutput++ = (char)c;
			}
		}
		else if (c < 0x800)
		{
			if (nRemain < 2)
			{
				break;
			}
			nRemain -= 2;
			if (lpOutput)
			{
				*lpOutput++ = (char)(0xc0 + ((c >> 6) & 0x1f));
				*lpOutput++ = (char)(0x80 + ((c >> 0) & 0x3f));
			}
		}
		else
		{
			if (nRemain < 3)
			{
				break;
			}
			nRemain -= 3;
			if (lpOutput)
			{
				*lpOutput++ = (char)(0xe0 + ((c >> 12) & 0x0f));
				*lpOutput++ = (char)(0x80 + ((c >> 6) & 0x3f));
				*lpOutput++ = (char)(0x80 + ((c >> 0) & 0x3f));
			}
		}
	}
	return (UINT)(cchOutput - nRemain);
}
