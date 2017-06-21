/**
 * @file	sjiseuc.c
 * @brief	Implementation of converting S-JIS to EUC
 */

#include "compiler.h"
#include "codecnv.h"

static UINT sjistoeuc(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);

/**
 * Maps a S-JIS string to a EUC string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_sjistoeuc(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
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
		return sjistoeuc(lpOutput, cchOutput, lpInput, cchInput);
	}
	else
	{
		// String mode
		nLength = sjistoeuc(lpOutput, cchOutput - 1, lpInput, (UINT)strlen(lpInput));
		if (lpOutput)
		{
			lpOutput[nLength] = '\0';
		}
		return nLength + 1;
	}
}

/**
 * Maps a S-JIS string to a EUC string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT sjistoeuc(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
{
	UINT nRemain;
	char c;
	UINT c2;

	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0))
	{
		cchInput--;
		c = *lpInput++;
		if ((c & 0x80) == 0)
		{
			nRemain--;
			if (lpOutput)
			{
				*lpOutput++ = c;
			}
		}
		else if ((((c ^ 0x20) - 0xa1) & 0xff) < 0x2f)
		{
			if (cchInput == 0)
			{
				break;
			}
			cchInput--;
			c2 = (UINT8)*lpInput++;
			if (c2 == '\0')
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
				c2 += 0x62 - ((c2 & 0x80) >> 7);
				if (c2 < 256)
				{
					c2 = (c2 - 0xa2) & 0x1ff;
				}
				c2 += 0x9fa1;
				*lpOutput++ = (char)(((c & 0x3f) << 1) + (c2 >> 8));
				*lpOutput++ = (char)c2;
			}
		}
		else if (((c - 0xa0) & 0xff) < 0x40)
		{
			if (nRemain < 2)
			{
				break;
			}
			nRemain -= 2;
			if (lpOutput)
			{
				*lpOutput++ = (char)0x8e;
				*lpOutput++ = c;
			}
		}
	}

	return (UINT)(cchOutput - nRemain);
}
