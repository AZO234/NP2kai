/**
 * @file	codecnv.h
 * @brief	Interface of converting buffer
 */

#if !defined(NP2_CODECNV_H__)
#define NP2_CODECNV_H__

#ifdef __cplusplus
extern "C"
{
#endif

UINT codecnv_sjistoeuc(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);
UINT codecnv_sjistoucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);
UINT codecnv_sjistoutf8(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);

UINT codecnv_euctosjis(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);
UINT codecnv_euctoucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);

UINT codecnv_utf8toucs2(UINT16 *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);
UINT codecnv_utf8tosjis(char *lpOutput, UINT cchOutput, const char *lpInput, UINT cchInput);

UINT codecnv_ucs2tosjis(char *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput);
UINT codecnv_ucs2toutf8(char *lpOutput, UINT cchOutput, const UINT16 *lpInput, UINT cchInput);

#ifdef __cplusplus
}
#endif

#endif	// !defined(NP2_CODECNV_H__)




// codecnv仕様
//
//	(dst != NULL) 時はテストのみ行なう。この場合dcnt=無限と扱う。
//
//	scnt == -1 の場合、String Modeとなる。
//	・NULLまで変換する
//	・dcntが足りなくても dstは必ずNULL Terminateとなる。
//
//	戻り値: 書き込みバッファ数が返る。(NULL Terminateも含む)
//
//	codecnv_AtoB(NULL, 0, "ABC\0DEF", -1) == 4
//	codecnv_AtoB(buf, 5, "ABC\0DEF", -1) == 4 / Copy(buf, "ABC\0", 4)
//	codecnv_AtoB(buf, 3, "ABC\0DEF", -1) == 3 / Copy(buf, "AB\0", 3)
//
//
//	scnt != -1 の場合、Binary Modeとなる。
//	・scnt分変換を行なう
//
//	戻り値: 書き込みバッファ数が返る。
//
//	codecnv_AtoB(NULL, 0, "ABC\0DEF", 7) == 7
//	codecnv_AtoB(NULL, 0, "ABC\0DEF", 4) == 4
//	codecnv_AtoB(buf, 7, "ABC\0DEF", 5) == 5 / Copy(buf, "ABC\0D", 5)
//	codecnv_AtoB(buf, 3, "ABC\0DEF", 6) == 3 / Copy(buf, "ABC", 3)
//	codecnv_AtoB(buf, 3, "ABC\0DEF", 0) == 0 / Copy(buf, "", 0)

