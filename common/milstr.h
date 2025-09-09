#ifndef _MILSTR_H_
#define _MILSTR_H_

#ifndef STRCALL
#define	STRCALL
#endif

#ifdef MILSTR_TEST
#include "compiler_base.h"
#else
#include "compiler.h"
#endif

// Must use milstr macros only.

#ifdef __cplusplus
extern "C" {
#endif

// １文字分のサイズを取得
int STRCALL milank_charsize(const OEMCHAR *str);
int STRCALL milsjis_charsize(const char *str);
int STRCALL mileuc_charsize(const char *str);
int STRCALL milutf8_charsize(const char *str);

// 大文字小文字を同一視して比較
// ret 0:一致
int STRCALL milank_cmp(const char *str, const char *cmp);
int STRCALL milsjis_cmp(const OEMCHAR *str, const OEMCHAR *cmp);
int STRCALL mileuc_cmp(const OEMCHAR *str, const OEMCHAR *cmp);
int STRCALL milutf8_cmp(const OEMCHAR *str, const OEMCHAR *cmp);

// 大文字小文字を同一視してmaxlen分比較
// ret 0:一致
int STRCALL milank_ncmp(const char *str, const char *cmp, unsigned int maxlen);
int STRCALL milsjis_ncmp(const OEMCHAR *str, const OEMCHAR *cmp, unsigned int maxlen);
int STRCALL mileuc_ncmp(const OEMCHAR *str, const OEMCHAR *cmp, unsigned int maxlen);
int STRCALL milutf8_ncmp(const OEMCHAR *str, const OEMCHAR *cmp, unsigned int maxlen);

// 大文字小文字を 同一視してcmpのヌルまで比較
// ret 0:一致
int STRCALL milank_memcmp(const char *str, const char *cmp);
int STRCALL milsjis_memcmp(const char *str, const char *cmp);
int STRCALL mileuc_memcmp(const char *str, const char *cmp);
int STRCALL milutf8_memcmp(const char *str, const char *cmp);

// 大文字小文字を 同一視してmaxlen分またはcmpのヌルまで比較
// ret 0:一致
int STRCALL milank_memncmp(const char *str, const char *cmp, unsigned int maxlen);
int STRCALL milsjis_memncmp(const char *str, const char *cmp, unsigned int maxlen);
int STRCALL mileuc_memncmp(const char *str, const char *cmp, unsigned int maxlen);
int STRCALL milutf8_memncmp(const char *str, const char *cmp, unsigned int maxlen);

// str[pos]が漢字１バイト目かどうか…
int STRCALL milsjis_kanji1st(const char *str, unsigned int pos);
int STRCALL mileuc_kanji1st(const char *str, unsigned int pos);
int STRCALL milutf8_kanji1st(const char *str, unsigned int pos);

// str[pos]が漢字２バイト目かどうか…
int STRCALL milsjis_kanji2nd(const char *str, unsigned int pos);
int STRCALL mileuc_kanji2nd(const char *str, unsigned int pos);
int STRCALL milutf8_kanji2nd(const char *str, unsigned int pos);

// maxlen分だけ文字列をコピー
void STRCALL milank_ncpy(char *dst, const char *src, unsigned int maxlen);
void STRCALL milsjis_ncpy(OEMCHAR *dst, const OEMCHAR *src, unsigned int maxlen);
void STRCALL mileuc_ncpy(OEMCHAR *dst, const OEMCHAR *src, unsigned int maxlen);
void STRCALL milutf8_ncpy(OEMCHAR *dst, const OEMCHAR *src, unsigned int maxlen);

// maxlen分だけ文字列をキャット
void STRCALL milank_ncat(char *dst, const char *src, unsigned int maxlen);
void STRCALL milsjis_ncat(OEMCHAR *dst, const OEMCHAR *src, unsigned int maxlen);
void STRCALL mileuc_ncat(OEMCHAR *dst, const OEMCHAR *src, unsigned int maxlen);
void STRCALL milutf8_ncat(OEMCHAR *dst, const OEMCHAR *src, unsigned int maxlen);

// 文字を検索
OEMCHAR * STRCALL milank_chr(const char *str, char c);
char * STRCALL milsjis_chr(const OEMCHAR *str, OEMCHAR c);
char * STRCALL mileuc_chr(const OEMCHAR *str, OEMCHAR c);
char * STRCALL milutf8_chr(const OEMCHAR *str, OEMCHAR c);


// 0~9, A~Z のみを大文字小文字を同一視して比較
// ret 0:一致
int STRCALL milstr_extendcmp(const OEMCHAR *str, const OEMCHAR *cmp);

// 次の語を取得
OEMCHAR * STRCALL milstr_nextword(const OEMCHAR *str);

// 文字列からARGの取得
int STRCALL milstr_getarg(OEMCHAR *str, OEMCHAR *arg[], int maxarg);

// HEX2INT
long STRCALL milstr_solveHEX(const OEMCHAR *str);

// STR2INT
long STRCALL milstr_solveINT(const OEMCHAR *str);

// STRLIST
OEMCHAR * STRCALL milstr_list(const OEMCHAR *lststr, UINT pos);

#ifdef __cplusplus
}
#endif


// ---- macros

#if defined(OSLANG_SJIS)
#define	milstr_charsize(s)		milsjis_charsize(s)
#define	milstr_cmp(s, c)		milsjis_cmp(s, c)
#define	milstr_ncmp(s, c, n)		milsjis_ncmp(s, c, n)
#define	milstr_memcmp(s, c)		milsjis_memcmp(s, c)
#define	milstr_memncmp(s, c, n)		milsjis_memncmp(s, c, n)
#define	milstr_kanji1st(s, p)	milsjis_kanji1st(s, p)
#define	milstr_kanji2nd(s, p)	milsjis_kanji2nd(s, p)
#define	milstr_ncpy(d, s, l)	milsjis_ncpy(d, s, l)
#define	milstr_ncat(d, s, l)	milsjis_ncat(d, s, l)
#define	milstr_chr(s, c)		milsjis_chr(s, c)
#elif defined(OSLANG_EUC)
#define	milstr_charsize(s)		mileuc_charsize(s)
#define	milstr_cmp(s, c)		mileuc_cmp(s, c)
#define	milstr_ncmp(s, c, n)		mileuc_ncmp(s, c, n)
#define	milstr_memcmp(s, c)		mileuc_memcmp(s, c)
#define	milstr_memncmp(s, c, n)		mileuc_memncmp(s, c, n)
#define	milstr_kanji1st(s, p)	mileuc_kanji1st(s, p)
#define	milstr_kanji2nd(s, p)	mileuc_kanji2nd(s, p)
#define	milstr_ncpy(d, s, l)	mileuc_ncpy(d, s, l)
#define	milstr_ncat(d, s, l)	mileuc_ncat(d, s, l)
#define	milstr_chr(s, c)		mileuc_chr(s, c)
#elif defined(OSLANG_UTF8)
#define	milstr_charsize(s)		milutf8_charsize(s)
#define	milstr_cmp(s, c)		milutf8_cmp(s, c)
#define	milstr_ncmp(s, c, n)		milutf8_ncmp(s, c, n)
#define	milstr_memcmp(s, c)		milutf8_memcmp(s, c)
#define	milstr_memncmp(s, c, n)		milutf8_memncmp(s, c, n)
#define	milstr_kanji1st(s, p)	milutf8_kanji1st(s, p)
#define	milstr_kanji2nd(s, p)	milutf8_kanji2nd(s, p)
#define	milstr_ncpy(d, s, l)	milutf8_ncpy(d, s, l)
#define	milstr_ncat(d, s, l)	milutf8_ncat(d, s, l)
#define	milstr_chr(s, c)		milutf8_chr(s, c)
#else
#define	milstr_charsize(s)		milank_charsize(s)
#define	milstr_cmp(s, c)		milank_cmp(s, c)
#define	milstr_ncmp(s, c, n)		milank_ncmp(s, c, n)
#define	milstr_memcmp(s, c)		milank_memcmp(s, c)
#define	milstr_memncmp(s, c, n)		milank_memncmp(s, c, n)
#define	milstr_kanji1st(s, p)	(0)
#define	milstr_kanji2nd(s, p)	(0)
#define	milstr_ncpy(d, s, l)	milank_ncpy(d, s, l)
#define	milstr_ncat(d, s, l)	milank_ncat(d, s, l)
#define	milstr_chr(s, c)		milank_chr(s, c)
#endif

#endif  // _MILSTR_H_

