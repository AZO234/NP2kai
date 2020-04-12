/**
 * @file	ucs4utf8.c
 * @brief	Implementation of converting UCS4 to UTF-8
 */

#ifdef CODECNV_TEST
#include "compiler_base.h"
#else
#include "compiler.h"
#endif
#include "codecnv.h"

static UINT ucs4toutf8(char *lpOutput, UINT cchOutput, const UINT32 *lpInput, UINT cchInput);

/**
 * Maps a UTF-32 string to a UTF-8 string
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
UINT codecnv_ucs4toutf8(char *lpOutput, UINT cchOutput, const UINT32 *lpInput, UINT cchInput) {
	UINT n = 0;
	UINT nLength;

	if(lpOutput != NULL && lpInput != NULL) {
		if(cchOutput == 0) {
			lpOutput = NULL;
			cchOutput = (UINT)-1;
		}

		if(cchInput != (UINT)-1) {
			// Binary mode
			n = ucs4toutf8(lpOutput, cchOutput, lpInput, cchInput);
		} else {
			// String mode
			nLength = ucs4toutf8(lpOutput, cchOutput - 1, lpInput, codecnv_ucs4len(lpInput));
			lpOutput[nLength] = '\0';
			n = nLength + 1;
		}
	}

	return n;
}

/**
 * Maps a UTF-32 string to a UTF-8 string (inner)
 * @param[out] lpOutput Pointer to a buffer that receives the converted string
 * @param[in] cchOutput Size, in characters, of the buffer indicated by lpOutput
 * @param[in] lpInput Pointer to the character string to convert
 * @param[in] cchInput Size, in characters, of the buffer indicated by lpInput
 * @return The number of characters written to the buffer indicated by lpOutput
 */
static UINT ucs4toutf8(char *lpOutput, UINT cchOutput, const UINT32 *lpInput, UINT cchInput)
{
	UINT nRemain;
	UINT c;

	nRemain = cchOutput;
	while((cchInput > 0) && (nRemain > 0)) {
		c = *lpInput++;
		cchInput--;

		if(c < 0x80) {
			nRemain--;
			if(lpOutput) {
				*lpOutput++ = (char)c;
			}
		} else if(c < 0x800) {
			if(nRemain < 2) {
				break;
			}
			nRemain -= 2;
			if(lpOutput) {
				*lpOutput++ = (char)(0xc0 + ((c >> 6) & 0x1f));
				*lpOutput++ = (char)(0x80 + ((c >> 0) & 0x3f));
			}
		} else if(c < 0x10000) {
			if (nRemain < 3) {
				break;
			}
			nRemain -= 3;
			if(lpOutput) {
				*lpOutput++ = (char)(0xe0 + ((c >> 12) & 0x0f));
				*lpOutput++ = (char)(0x80 + ((c >>  6) & 0x3f));
				*lpOutput++ = (char)(0x80 + ((c >>  0) & 0x3f));
			}
		} else {
			if(nRemain < 4) {
				break;
			}
			nRemain -= 4;
			if(lpOutput) {
				*lpOutput++ = (char)(0xe0 + ((c >> 18) & 0x07));
				*lpOutput++ = (char)(0x80 + ((c >> 12) & 0x3f));
				*lpOutput++ = (char)(0x80 + ((c >>  6) & 0x3f));
				*lpOutput++ = (char)(0x80 + ((c >>  0) & 0x3f));
			}
		}
	}

	return (UINT)(cchOutput - nRemain);
}
