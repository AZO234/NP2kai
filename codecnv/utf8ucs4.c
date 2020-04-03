/**
 * @file	utf8ucs4.c
 * @brief	Implementation of converting UTF-8 to UCS4
 */

#ifdef CODECNV_TEST
#include "compiler_base.h"
#else
#include "compiler.h"
#endif
#include "codecnv.h"

static UINT utf8toucs4(UINT32 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);

/**
 * Maps a UTF-8 string to a UTF-32 string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_utf8toucs4(UINT32 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput) {
	UINT n = 0;
	UINT nLength;

	if(lpOutput != NULL && lpInput != NULL) {
		if(cchOutput == 0) {
			cchOutput = (UINT)-1;
		}

		if(cchInput != (UINT)-1) {
			// Binary mode
			n = utf8toucs4(lpOutput, cchOutput, lpInput, cchInput);
		} else {
			// String mode
			nLength = utf8toucs4(lpOutput, cchOutput - 1, lpInput, (UINT)strlen(lpInput));
			lpOutput[nLength] = '\0';
			n = nLength + 1;
		}
	}

	return n;
}

/**
 * Maps a UTF-8 char to a UTF-32 character
 * @param[out] lpOutput Pointer to a buffer that receives the converted character
 * @param[in] lpInput Pointer to the character to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters read to the buffer indicated by lpInput
 */
UINT codecnv_utf8toucs4_1(UINT32 *lpOutput, const char *lpInput, UINT cchInput) {
	UINT c;
	UINT n;

	c = (UINT8)lpInput[0];
	if(c < 0x80) {
		n = 1;
	} else if(0xC2 <= c && c < 0xE0) {
		n = 2;
	} else if(0xE0 <= c && c < 0xF0) {
		n = 3;
	} else if(0xF0 <= c && c < 0xF8) {
		n = 4;
	} else {
		n = 0;
	}

	if(cchInput < n) {
		n = 0;
	}

	switch(n) {
	case 1:
		lpOutput[0] = (UINT8)lpInput[0];
		break;
	case 2:
		lpOutput[0]  = ((UINT8)lpInput[0] & 0x1F) << 6;
		lpOutput[0] |=  (UINT8)lpInput[1] & 0x3F;
		break;
	case 3:
		lpOutput[0]  = ((UINT8)lpInput[0] & 0x0F) << 12;
		lpOutput[0] |= ((UINT8)lpInput[1] & 0x3F) <<  6;
		lpOutput[0] |=  (UINT8)lpInput[2] & 0x3F;
		break;
	case 4:
		lpOutput[0]  = ((UINT8)lpInput[0] & 0x07) << 18;
		lpOutput[0] |= ((UINT8)lpInput[1] & 0x3F) << 12;
		lpOutput[0] |= ((UINT8)lpInput[2] & 0x3F) <<  6;
		lpOutput[0] |=  (UINT8)lpInput[3] & 0x3F;
		break;
	}

	return n;
}

/**
 * Maps a UTF-8 string to a UTF-32 string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT utf8toucs4(UINT32 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput)
{
	UINT nRemain;
	UINT n = 1;

	nRemain = cchOutput;
	while ((cchInput > 0) && (nRemain > 0) && (n > 0)) {
		n = codecnv_utf8toucs4_1(lpOutput, lpInput, nRemain);
		cchInput -= n;
		lpInput += n;
		lpOutput++;
		nRemain--;
	}

	return (UINT)(cchOutput - nRemain);
}
