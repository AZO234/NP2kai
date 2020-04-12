/**
 * @file	utf8ucs2.c
 * @brief	Implementation of converting UTF-8 to UCS2
 */

#ifdef CODECNV_TEST
#include "compiler_base.h"
#else
#include "compiler.h"
#endif
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
UINT codecnv_utf8toucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)	{
	UINT n = 0;
	UINT nLength;

	if(lpOutput != NULL && lpInput != NULL) {
		if(cchOutput == 0) {
			lpOutput = NULL;
			cchOutput = (UINT)-1;
		}

		if(cchInput != (UINT)-1) {
			// Binary mode
			n = utf8toucs2(lpOutput, cchOutput, lpInput, cchInput);
		} else {
			// String mode
			nLength = utf8toucs2(lpOutput, cchOutput - 1, lpInput, (UINT)strlen(lpInput));
			lpOutput[nLength] = '\0';
			n = nLength + 1;
		}
	}

	return n;
}

/**
 * Maps a UTF-8 string to a UTF-16 string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT utf8toucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput) {
	UINT n, m;
	UINT nRemain;
	UINT32 c[2];

	n = m = 1;
	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0) && (m > 0) && (n > 0)) {
		n = codecnv_utf8toucs4_1(c, lpInput, cchInput);
		if(n) {
			c[1] = '\0';
			m = codecnv_ucs4toucs2(lpOutput, 2, c, 1);
			lpInput += n;
			cchInput -= n;
			lpOutput += m;
			nRemain -= m;
		}
	}

	return (UINT)(cchOutput - nRemain);
}
