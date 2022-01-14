/**
 * @file	ucs4ucs2.c
 * @brief	Implementation of converting UCS4 to UCS2
 */

#ifdef CODECNV_TEST
#include "compiler_base.h"
#else
#include <compiler.h>
#endif
#include <codecnv/codecnv.h>

static UINT ucs4toucs2(UINT16 *lpOutput, UINT cchOutput, const UINT32 *lpInput, UINT cchInput);

/**
 * Maps a UTF-32 string to a UTF-16 string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_ucs4toucs2(UINT16 *lpOutput, UINT cchOutput, const UINT32 *lpInput, UINT cchInput) {
	UINT n = 0;
	UINT nLength;

	if (lpInput != NULL) {
		if (!lpOutput || cchOutput == 0) {
			lpOutput = NULL;
			if (cchInput != (UINT)-1) {
				cchOutput = (UINT)-1;
			} else {
				cchOutput = 0;
			}
		}

		if (cchInput != (UINT)-1) {
			// Binary mode
			n = ucs4toucs2(lpOutput, cchOutput, lpInput, cchInput);
		} else {
			// String mode
			nLength = ucs4toucs2(lpOutput, cchOutput - 1, lpInput, codecnv_ucs4len(lpInput));
			if (lpOutput) {
				lpOutput[nLength] = '\0';
			}
			n = nLength + 1;
		}
	}

	return n;
}

/**
 * Get the length of input string
 * @param[in] lpString Null-terminated string
 * @return the number of characters in lpString
 */
UINT codecnv_ucs4len(const UINT32 *lpString) {
	const UINT32 *p = lpString;

	while(*p != 0) {
		p++;
	}

	return (UINT)(p - lpString);
}

/**
 * Maps a UTF-32 char to a UTF-16 character
 * @param[out] lpOutput Pointer to a buffer that receives the converted character
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character to convert
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_ucs4toucs2_1(UINT16 *lpOutput, UINT cchOutput, const UINT32 *lpInput) {
	UINT n = 0;

	if (lpInput[0] > 0x10FFFF) {
		return n;
	}

	if(lpInput[0] < 0x10000 && cchOutput > 1) {
		if (lpOutput) {
			lpOutput[0] = (UINT16)lpInput[0];
			lpOutput[1] = 0;
		}
		n = 1;
	} else if(cchOutput > 2) {
		if (lpOutput) {
			lpOutput[0] = (UINT16)((lpInput[0] - 0x10000) / 0x400 + 0xD800);
			lpOutput[1] = (UINT16)((lpInput[0] - 0x10000) % 0x400 + 0xDC00);
		}
		n = 2;
	}

	return n;
}

/**
 * Maps a UTF-32 string to a UTF-16 string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT ucs4toucs2(UINT16 *lpOutput, UINT cchOutput, const UINT32 *lpInput, UINT cchInput) {
	UINT nRemain;
	UINT n = 1;
	UINT len = 0;

	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0) && (n > 0)) {
		if (nRemain <= 0 && (INT)cchOutput > 0) {
			break;
		}
		n = codecnv_ucs4toucs2_1(lpOutput, nRemain, lpInput);
		if (lpOutput) {
			lpOutput += n;
			nRemain -= n;
		}
		lpInput++;
		cchInput--;
		len += n;
	}

	return len;
}
