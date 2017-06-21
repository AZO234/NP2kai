#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"font.h"
#include	"fontdata.h"


#define	V98FILESIZE		0x46800

static void v98knjcpy(UINT8 *dst, const UINT8 *src, int from, int to) {

	int		i, j, k;
const UINT8	*p;
	UINT8	*q;

	for (i=from; i<to; i++) {
		p = src + 0x1800 + (0x60 * 32 * (i - 1));
		q = dst + 0x20000 + (i << 4);
		for (j=0x20; j<0x80; j++) {
			for (k=0; k<16; k++) {
				*(q + 0x800) = *(p+16);
				*q++ = *p++;
			}
			p += 16;
			q += 0x1000 - 16;
		}
	}
}

UINT8 fontv98_read(const OEMCHAR *filename, UINT8 loading) {

	FILEH	fh;
	UINT8	*v98fnt;

	if (!(loading & FONTLOAD_ALL)) {
		goto frv_err1;
	}

	// ファイルをオープン
	fh = file_open_rb(filename);
	if (fh == FILEH_INVALID) {
		goto frv_err1;
	}

	v98fnt = (UINT8 *)_MALLOC(V98FILESIZE, "v98font");
	if (v98fnt == NULL) {
		goto frv_err2;
	}

	// FONT.ROM の読み込み
	if (file_read(fh, v98fnt, V98FILESIZE) != V98FILESIZE) {
		goto frv_err3;
	}

	// 8x8 フォントを読む必要がある？
	if (loading & FONT_ANK8) {
		loading &= ~FONT_ANK8;
		fontdata_ank8store(v98fnt, 0, 256);
	}
	// 8x16 フォント(～0x7f)を読む必要がある？
	if (loading & FONT_ANK16a) {
		loading &= ~FONT_ANK16a;
		CopyMemory(fontrom + 0x80000, v98fnt + 0x0800, 16*128);
	}
	// 8x16 フォント(0x80～)を読む必要がある？
	if (loading & FONT_ANK16b) {
		loading &= ~FONT_ANK16b;
		CopyMemory(fontrom + 0x80800, v98fnt + 0x1000, 16*128);
	}

	// 第一水準漢字を読む必要がある？
	if (loading & FONT_KNJ1) {
		loading &= ~FONT_KNJ1;
		v98knjcpy(fontrom, v98fnt, 0x01, 0x30);
	}
	// 第二水準漢字を読む必要がある？
	if (loading & FONT_KNJ2) {
		loading &= ~FONT_KNJ2;
		v98knjcpy(fontrom, v98fnt, 0x30, 0x56);
	}
	// 拡張漢字を読む必要がある？
	if (loading & FONT_KNJ3) {
		loading &= ~FONT_KNJ3;
		v98knjcpy(fontrom, v98fnt, 0x58, 0x5d);
	}

frv_err3:
	_MFREE(v98fnt);

frv_err2:
	file_close(fh);							// 後始末

frv_err1:
	return(loading);
}

