#include	"compiler.h"
#include	"oemtext.h"
#include	"codecnv/textcnv.h"


// Use WinAPI version

UINT oemtext_mbtoucs2(UINT cp, wchar_t *dst, UINT dcnt, const char *src, UINT scnt) {

	int		srccnt;
	int		dstcnt;
	int		r;

	if (((SINT)scnt) > 0) {
		srccnt = scnt;
	}
	else {
		srccnt = -1;
	}
	if (((SINT)dcnt) > 0) {
		dstcnt = dcnt;
		if (srccnt < 0) {
			dstcnt = dstcnt - 1;
			if (dstcnt == 0) {
				if (dst) {
					dst[0] = '\0';
				}
				return(1);
			}
		}
	}
	else {
		dstcnt = 0;
	}
	r = MultiByteToWideChar(cp, MB_PRECOMPOSED, src, srccnt, dst, dstcnt);
	if ((r == 0) && (dstcnt != 0)) {
		r = dstcnt;
		if (srccnt < 0) {
			if (dst) {
				dst[r] = '\0';
			}
			r++;
		}
	}
	return(r);
}

UINT oemtext_ucs2tomb(UINT cp, char *dst, UINT dcnt, const wchar_t *src, UINT scnt) {

	int		srccnt;
	int		dstcnt;
	int		r;

	if (((SINT)scnt) > 0) {
		srccnt = scnt;
	}
	else {
		srccnt = -1;
	}
	if (((SINT)dcnt) > 0) {
		dstcnt = dcnt;
		if (srccnt < 0) {
			dstcnt = dstcnt - 1;
			if (dstcnt == 0) {
				if (dst) {
					dst[0] = '\0';
				}
				return(1);
			}
		}
	}
	else {
		dstcnt = 0;
	}
	r = WideCharToMultiByte(cp, 0, src, srccnt, dst, dstcnt, NULL, NULL);
	if ((r == 0) && (dstcnt != 0)) {
		r = dstcnt;
		if (srccnt < 0) {
			if (dst) {
				dst[r] = '\0';
			}
			r++;
		}
	}
	return(r);
}

UINT oemtext_mbtoutf8(UINT cp, char *dst, UINT dcnt, const char *src, UINT scnt) {

	UINT	leng;
	wchar_t	*ucs2;
	UINT	ret;

	(void)scnt;

	leng = MultiByteToWideChar(cp, MB_PRECOMPOSED, src, scnt, NULL, 0);
	if (leng == 0) {
		return(0);
	}
	ucs2 = (wchar_t *)_MALLOC(leng * sizeof(wchar_t), "");
	if (ucs2 == NULL) {
		return(0);
	}
	MultiByteToWideChar(cp, MB_PRECOMPOSED, src, scnt, ucs2, leng);
	if (((SINT)scnt) < 0) {
		leng = (UINT)-1;
	}
	ret = codecnv_ucs2toutf8(dst, dcnt, (UINT16 *)ucs2, leng);
	_MFREE(ucs2);
	return(ret);
}

UINT oemtext_utf8tomb(UINT cp, char *dst, UINT dcnt, const char *src, UINT scnt) {

	UINT	leng;
	wchar_t	*ucs2;
	UINT	ret;

	(void)scnt;

	leng = codecnv_utf8toucs2(NULL, 0, src, scnt);
	if (leng == 0) {
		return(0);
	}
	ucs2 = (wchar_t *)_MALLOC(leng * sizeof(wchar_t), "");
	if (ucs2 == NULL) {
		return(0);
	}
	codecnv_utf8toucs2((UINT16 *)ucs2, leng, src, scnt);
	if (((SINT)scnt) < 0) {
		leng = (UINT)-1;
	}
	ret = WideCharToMultiByte(cp, 0, ucs2, leng, dst, dcnt, NULL, NULL);
	_MFREE(ucs2);
	return(ret);
}


// ----

UINT oemtext_chartoucs2(wchar_t *dst, UINT dcnt, const char *src, UINT scnt) {

	return(oemtext_mbtoucs2(CP_ACP, dst, dcnt, src, scnt));
}

UINT oemtext_ucs2tochar(char *dst, UINT dcnt, const wchar_t *src, UINT scnt) {

	return(oemtext_ucs2tomb(CP_ACP, dst, dcnt, src, scnt));
}

UINT oemtext_chartoutf8(char *dst, UINT dcnt, const char *src, UINT scnt) {

	return(oemtext_mbtoutf8(CP_ACP, dst, dcnt, src, scnt));
}

UINT oemtext_utf8tochar(char *dst, UINT dcnt, const char *src, UINT scnt) {

	return(oemtext_utf8tomb(CP_ACP, dst, dcnt, src, scnt));
}


// ---- textcnv

UINT textcnv_getinfo(TCINF *inf, const UINT8 *hdr, UINT hdrsize) {

	UINT	textcode;
	TCINF	info;

	textcode = TEXTCNV_DEFAULT;
	ZeroMemory(&info, sizeof(info));
	if ((hdrsize >= 3) &&
		(hdr[0] == 0xef) && (hdr[1] == 0xbb) && (hdr[2] == 0xbf)) {
		// UTF-8
		textcode = TEXTCNV_UTF8;
		info.width = 1;
		info.hdrsize = 3;
	}
	else if ((hdrsize >= 2) && (hdr[0] == 0xff) && (hdr[1] == 0xfe)) {
		// UCSLE
		textcode = TEXTCNV_UCS2;
#if defined(BYTESEX_BIG)
		info.xendian = 1;
#endif
		info.width = 2;
		info.hdrsize = 2;
	}
	else if ((hdrsize >= 2) && (hdr[0] == 0xfe) && (hdr[1] == 0xff)) {
		// UCS2BE
		// UCSLE
		textcode = TEXTCNV_UCS2;
#if defined(BYTESEX_LITTLE)
		info.xendian = 1;
#endif
		info.width = 2;
		info.hdrsize = 2;
	}
	else {
		info.width = 1;
	}

#if defined(OSLANG_SJIS)
	switch(textcode) {
		case TEXTCNV_DEFAULT:
		case TEXTCNV_SJIS:
			info.caps = TEXTCNV_READ | TEXTCNV_WRITE;
			break;

		case TEXTCNV_UTF8:
			info.caps = TEXTCNV_READ | TEXTCNV_WRITE;
			info.tooem = (TCTOOEM)oemtext_utf8tochar;
			info.fromoem = (TCFROMOEM)oemtext_chartoutf8;
			break;

		case TEXTCNV_UCS2:
			info.caps = TEXTCNV_READ | TEXTCNV_WRITE;
			info.tooem = (TCTOOEM)oemtext_ucs2tochar;
			info.fromoem = (TCFROMOEM)oemtext_chartoucs2;
			break;
	}
#elif defined(OSLANG_EUC)
	switch(textcode) {
		case TEXTCNV_DEFAULT:
		case TEXTCNV_EUC:
			info.caps = TEXTCNV_READ | TEXTCNV_WRITE;
			break;
	}
#elif defined(OSLANG_UCS2)
	switch(textcode) {
		case TEXTCNV_DEFAULT:
		case TEXTCNV_SJIS:
			info.caps = TEXTCNV_READ | TEXTCNV_WRITE;
			info.tooem = (TCTOOEM)oemtext_chartoucs2;
			info.fromoem = (TCFROMOEM)oemtext_ucs2tochar;
			break;

		case TEXTCNV_UTF8:
			info.caps = TEXTCNV_READ | TEXTCNV_WRITE;
			info.tooem = (TCTOOEM)codecnv_utf8toucs2;
			info.fromoem = (TCFROMOEM)codecnv_ucs2toutf8;
			break;

		case TEXTCNV_UCS2:
			info.caps = TEXTCNV_READ | TEXTCNV_WRITE;
			break;
	}
#else	// defined(OSLANG_*)
	switch(textcode) {
		case TEXTCNV_DEFAULT:
			info.caps = TEXTCNV_READ | TEXTCNV_WRITE;
			break;
	}
#endif	// defined(OSLANG_*)
	if (inf) {
		*inf = info;
	}
	return(info.caps);
}

