/**
 * @file	eucsjis.c
 * @brief	Implementation of converting EUC to S-JIS
 */

#include "compiler.h"
#include "codecnv.h"

static UINT euctosjis(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);

/**
 * Maps a EUC string to a S-JIS string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_euctosjis(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
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
		return euctosjis(lpOutput, cchOutput, lpInput, cchInput);
	}
	else
	{
		// String mode
		nLength = euctosjis(lpOutput, cchOutput - 1, lpInput, (UINT)strlen(lpInput));
		if (lpOutput)
		{
			lpOutput[nLength] = '\0';
		}
		return nLength + 1;
	}
}

/**
 * Maps a EUC string to a S-JIS string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT euctosjis(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
{
	UINT nRemain;
	char h;
	UINT l;

	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0))
	{
		cchInput--;
		h = *lpInput++;
		if ((h & 0x80) == 0)
		{
			nRemain--;
			if (lpOutput)
			{
				*lpOutput++ = h;
			}
		}
		else if (h == (char)0x8e)
		{
			if (cchInput == 0)
			{
				break;
			}
			cchInput--;
			h = *lpInput++;

			nRemain--;
			if (lpOutput)
			{
				*lpOutput++ = h;
			}

		}
		else
		{
			if (cchInput == 0)
			{
				break;
			}
			cchInput--;
			l = *lpInput++;
			if (l == 0)
			{
				continue;
			}

			if (nRemain < 2)
			{
				break;
			}
			nRemain -= 2;
			if (lpOutput)
			{
				h &= 0x7f;
				l &= 0x7f;
				l += ((h & 1) - 1) & 0x5e;
				if (l >= 0x60)
				{
					l++;
				}
				*lpOutput++ = (char)(((h + 0x121) >> 1) ^ 0x20);
				*lpOutput++ = (char)(l + 0x1f);
			}
		}
	}

	return (UINT)(cchOutput - nRemain);
}
