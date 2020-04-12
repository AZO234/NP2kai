/**
 * @file	ucs2ucs4.c
 * @brief	Implementation of converting UTF-16 to UTF-32
 */

#ifdef CODECNV_TEST
#include "compiler_base.h"
#else
#include "compiler.h"
#endif
#include "codecnv.h"

static BOOL isucs2highsurrogate(const UINT16 c);
static BOOL isucs2lowsurrogate(const UINT16 c);
static UINT ucs2toucs4(UINT32 *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput);

/**
 * Maps a UTF-16 string to a UTF-8 string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_ucs2toucs4(UINT32 *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput) {
	UINT n = 0;
	UINT nLength;

	if(lpOutput != NULL && lpInput != NULL) {
		if(cchOutput == 0) {
			lpOutput = NULL;
			cchOutput = (UINT)-1;
		}

		if(cchInput != (UINT)-1) {
			// Binary mode
			n = ucs2toucs4(lpOutput, cchOutput, lpInput, cchInput);
		} else {
			// String mode
			nLength = ucs2toucs4(lpOutput, cchOutput - 1, lpInput, codecnv_ucs2len(lpInput));
			lpOutput[nLength] = '\0';
			n = nLength + 1;
		}
	}

	return n;
}

static BOOL isucs2highsurrogate(const UINT16 c) {
	return 0xD800 <= c && c < 0xDC00;
}

static BOOL isucs2lowsurrogate(const UINT16 c) {
	return 0xDC00 <= c && c < 0xE000;
}

/**
 * Maps a UTF-16 char to a UTF-32 character
 * @param[out] lpOutput Pointer to a buffer that receives the converted character
 * @param[in] lpInput Pointer to the character to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters read to the buffer indicated by lpInput
 */
UINT codecnv_ucs2toucs4_1(UINT32 *lpOutput, const UINT16 *lpInput, UINT cchInput) {
	int n = 0;

	if(isucs2highsurrogate(lpInput[0])) {
		if(isucs2lowsurrogate(lpInput[1]) && cchInput >= 2) {
			lpOutput[0]  = 0x10000;
			lpOutput[0] += ((UINT32)lpInput[0] - 0xD800) * 0x400;
			lpOutput[0] +=  (UINT32)lpInput[1] - 0xDC00         ;
			n = 2;
		} else if(lpInput[1] == 0 && cchInput >= 1) {
			lpOutput[0] = lpInput[0];
			n = 1;
		}
	} else if(isucs2lowsurrogate(lpInput[0])) {
		if(lpInput[1] == 0 && cchInput >= 1) {
			lpOutput[0] = lpInput[0];
			n = 1;
		}
	} else {
		if(cchInput >= 1) {
			lpOutput[0] = lpInput[0];
			n = 1;
		}
	}

	return n;
}

/**
 * Maps a UTF-16 string to a UTF-32 string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT ucs2toucs4(UINT32 *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput)
{
	UINT nRemain;
	int n = 1;

	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0) && (n > 0)) {
		n = codecnv_ucs2toucs4_1(lpOutput, lpInput, nRemain);
		cchInput -= n;
		lpInput += n;
		lpOutput++;
		nRemain--;
	}

	return (UINT)(cchOutput - nRemain);
}
