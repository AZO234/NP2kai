/**
 * @file	ucs2utf8.c
 * @brief	Implementation of converting UCS2 to UTF-8
 */

#ifdef CODECNV_TEST
#include "compiler_base.h"
#else
#include "compiler.h"
#endif
#include "codecnv.h"

static UINT ucs2toutf8(char *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput);

/**
 * Maps a UTF-16 string to a UTF-8 string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_ucs2toutf8(char *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput) {
	UINT n = 0;
	UINT nLength;

	if(lpOutput != NULL && lpInput != NULL) {
		if(cchOutput == 0) {
			lpOutput = NULL;
			cchOutput = (UINT)-1;
		}

		if(cchInput != (UINT)-1) {
			// Binary mode
			n = ucs2toutf8(lpOutput, cchOutput, lpInput, cchInput);
		} else {
			// String mode
			nLength = ucs2toutf8(lpOutput, cchOutput - 1, lpInput, codecnv_ucs2len(lpInput));
			lpOutput[nLength] = '\0';
			n = nLength + 1;
		}
	}

	return n;
}

/**
 * Get the length of a string
 * @param[in] lpString Null-terminated string
 * @return the number of characters in lpString
 */
UINT codecnv_ucs2len(const UINT16 *lpString) {
	const UINT16 *p = lpString;

	while (*p != 0) {
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
static UINT ucs2toutf8(char *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput) {
	UINT n, m;
	UINT nRemain;
	UINT32 c[2];

	n = m = 1;
	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0) && (m > 0) && (n > 0)) {
		n = codecnv_ucs2toucs4(c, 1, lpInput, 2);
		if(n) {
			m = codecnv_ucs4toutf8(lpOutput, nRemain, c, 1);
			lpInput += n;
			cchInput -= n;
			lpOutput += m;
			nRemain -= m;
		}
	}

	return (UINT)(cchOutput - nRemain);
}
