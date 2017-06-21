#include	"compiler.h"
#include	"parts.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"font.h"
#include	"fontdata.h"


static void x1knjcpy(UINT8 *dst, const UINT8 *src, int from, int to) {

	int		i, j, k;
const UINT8	*p;
	UINT8	*q;
	UINT	sjis;

	for (i=from; i<to; i++) {
		q = dst + 0x21000 + (i << 4);
		for (j=0x21; j<0x7f; j++) {
			p = NULL;
			// 漢字のポインタを求める
			sjis = jis2sjis(((i + 0x20) << 8) | j);
			if (sjis >= 0x8140 && sjis < 0x84c0) {
				p = src + 0x00000 + ((sjis - 0x8140) << 5);
			}
			else if (sjis >= 0x8890 && sjis < 0xa000) {
				p = src + 0x07000 + ((sjis - 0x8890) << 5);
			}
			else if (sjis >= 0xe040 && sjis < 0xeab0) {
				p = src + 0x35e00 + ((sjis - 0xe040) << 5);
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

UINT8 fontx1_read(const OEMCHAR *filename, UINT8 loading) {

	FILEH	fh;
	UINT8	*work;
	OEMCHAR	fname[MAX_PATH];

	work = (UINT8 *)_MALLOC(306176, "x1font");
	if (work == NULL) {
		goto frx1_err1;
	}
	file_cpyname(fname, filename, NELEMENTS(fname));

	// 8dot ANKを読み込む必要はある？
	if (loading & FONT_ANK8) {
		file_cutname(fname);
		file_catname(fname, x1ank1name, NELEMENTS(fname));
		fh = file_open_rb(fname);
		if (fh != FILEH_INVALID) {
			if (file_read(fh, work, 2048) == 2048) {
				loading &= ~FONT_ANK8;
				fontdata_ank8store(work + 0x100, 0x20, 0x60);
				fontdata_ank8store(work + 0x500, 0xa0, 0x40);
			}
			file_close(fh);
		}
	}

	// 16dot ANKを読み込む必要はあるか？
	if (loading & FONTLOAD_ANK) {
		file_cutname(fname);
		file_catname(fname, x1ank2name, NELEMENTS(fname));
		fh = file_open_rb(fname);
		if (fh != FILEH_INVALID) {
			if (file_read(fh, work, 4096) == 4096) {

				// 16dot ASCIIを読む必要があるか
				if (loading & FONT_ANK16a) {
					loading &= ~FONT_ANK16a;
					CopyMemory(fontrom + 0x80200, work + 0x200, 0x60*16);
					fontdata_patch16a();
				}

				// 16dot ANK(0x80～)を読む必要があるか
				if (loading & FONT_ANK16b) {
					loading &= ~FONT_ANK16b;
					CopyMemory(fontrom + 0x80a00, work + 0xa00, 0x40*16);
					fontdata_patch16b();
				}
			}
			file_close(fh);
		}
	}

	// 漢字を読み込む必要はあるか？
	if (loading & (FONT_KNJ1 | FONT_KNJ2)) {
		file_cutname(fname);
		file_catname(fname, x1knjname, NELEMENTS(fname));
		fh = file_open_rb(fname);
		if (fh != FILEH_INVALID) {
			if (file_read(fh, work, 306176) == 306176) {

				// 第一水準漢字を読み込む？
				if (loading & FONT_KNJ1) {
					loading &= ~FONT_KNJ1;
					x1knjcpy(fontrom, work, 0x01, 0x30);
					fontdata_patchjis();
				}

				// 第二水準を読む必要はある？
				if (loading & FONT_KNJ2) {
					loading &= ~FONT_KNJ2;
					x1knjcpy(fontrom, work, 0x31, 0x50);
				}
			}
			file_close(fh);
		}
	}

	// メモリを解放する
	_MFREE(work);

frx1_err1:
	return(loading);
}

