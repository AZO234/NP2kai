#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"font.h"
#include	"fontdata.h"


static void x68kknjcpy(UINT8 *dst, const UINT8 *src, int from, int to) {

	int		i, j, k;
const UINT8	*p;
	UINT8	*q;

	for (i=from; i<to; i++) {
		q = dst + 0x21000 + (i << 4);
		for (j=0x21; j<0x7f; j++) {
			p = NULL;
			// 漢字のポインタを求める
			if ((i >= 0x01) && (i < 0x08)) {			// 2121～277e
				p = src + 0x00000
					+ ((((i - 0x01) * 0x5e) + (j - 0x21)) * 0x20);
			}
			else if ((i >= 0x10) && (i < 0x30)) {		// 3021～5f7e
				p = src + 0x05e00
					+ ((((i - 0x10) * 0x5e) + (j - 0x21)) * 0x20);
			}
			else if ((i >= 0x30) && (i < 0x54)) {		// 5021～737e
				p = src + 0x1d600
					+ ((((i - 0x30) * 0x5e) + (j - 0x21)) * 0x20);
			}
			else if ((i == 0x54) && (j < 0x25)) {		// 7421～7424
				p = src + 0x1d600
					+ ((((0x54 - 0x30) * 0x5e) + (j - 0x21)) * 0x20);
			}
			if (p) {							// 規格内コードならば
				// コピーする
				for (k=0; k<16; k++) {
					*(q+k) = *p++;
					*(q+k+0x800) = *p++;
				}
			}
			q += 0x1000;
		}
	}
}

UINT8 fontx68k_read(const OEMCHAR *filename, UINT8 loading) {

	FILEH	fh;
	UINT8	*work;

	// ファイルをオープン
	fh = file_open_rb(filename);
	if (fh == FILEH_INVALID) {
		goto fr68_err1;
	}

	// メモリアロケート
	work = (UINT8 *)_MALLOC(0x3b800, "x68kfont");
	if (work == NULL) {
		goto fr68_err2;
	}

	// CGROM.DAT の読み込み
	if (file_read(fh, work, 0x3b800) != 0x3b800) {
		goto fr68_err3;
	}

	// 8dot ANKを読む必要があるか
	if (loading & FONT_ANK8) {
		loading &= ~FONT_ANK8;
		fontdata_ank8store(work + 0x3a100, 0x20, 0x60);
		fontdata_ank8store(work + 0x3a500, 0xa0, 0x40);
	}

	// 16dot ASCIIを読む必要があるか
	if (loading & FONT_ANK16a) {
		loading &= ~FONT_ANK16a;
		CopyMemory(fontrom + 0x80200, work + 0x3aa00, 0x60*16);
		fontdata_patch16a();
	}

	// 16dot ANK(0x80～)を読む必要があるか
	if (loading & FONT_ANK16b) {
		loading &= ~FONT_ANK16b;
		CopyMemory(fontrom + 0x80a00, work + 0x3b200, 0x40*16);
		fontdata_patch16b();
	}

	// 第一水準漢字を読み込む？
	if (loading & FONT_KNJ1) {
		loading &= ~FONT_KNJ1;
		x68kknjcpy(fontrom, work, 0x01, 0x30);
		fontdata_patchjis();
	}

	// 第二水準を読む必要はある？
	if (loading & FONT_KNJ2) {
		loading &= ~FONT_KNJ2;
		x68kknjcpy(fontrom, work, 0x30, 0x60);
	}

fr68_err3:
	_MFREE(work);

fr68_err2:
	file_close(fh);

fr68_err1:
	return(loading);
}

