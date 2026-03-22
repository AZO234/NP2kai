/**
 * @file cdd_libcdio.h
 * @brief libcdio backend for NP2kai CD image support
 *
 * Build guards: SUPPORT_KAI_IMAGES && SUPPORT_LIBCDIO
 * Linker:  -lcdio   (pkg-config --libs libcdio)
 *
 * ----------------------------------------------------------------
 * Integration – fdd/sxsicd.c に加える変更
 * ----------------------------------------------------------------
 *
 * [1] SUPPORT_KAI_IMAGES ブロック内の include 群の末尾に追加:
 *
 *   #ifdef SUPPORT_LIBCDIO
 *   #include "diskimage/cd/cdd_libcdio.h"
 *   #endif
 *
 * [2] sxsicd_open() の最後の return(openiso(...)) を置換:
 *
 *   #ifdef SUPPORT_LIBCDIO
 *       return opencdio(sxsi, fname);
 *   #else
 *       return openiso(sxsi, fname);
 *   #endif
 *
 * [3] sxsicd_readraw() の
 *       fh = ((CDINFO)sxsi->hdl)->fh;
 *     の直後に挿入:
 *
 *   #ifdef SUPPORT_LIBCDIO
 *       if (fh == FILEH_INVALID) {
 *           return lcdd_readraw_sector(sxsi, pos, buf);
 *       }
 *   #endif
 *
 * [4] sxsicd_readraw_forhash() の
 *       fh = ((CDINFO)sxsi->hdl)->fh;
 *     の直後に挿入:
 *
 *   #ifdef SUPPORT_LIBCDIO
 *       if (!fh) {
 *           return (int)lcdd_readraw_forhash(sxsi, uSecNo, pu8Buf, puSize);
 *       }
 *   #endif
 */

#ifndef _CDD_LIBCDIO_H_
#define _CDD_LIBCDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * opencdio – libcdio で CD イメージを開く。
 *
 * ISO / BIN+CUE / CCD+IMG / NRG / CDI / TOC など libcdio が対応する
 * 全フォーマットを自動判別して開く。
 * 成功時は SXSIDEV が完全に初期化される（他の cdd_* ドライバと同等）。
 */
BRESULT opencdio(SXSIDEV sxsi, const OEMCHAR *path);

/**
 * lcdd_readraw_sector – LBA 'pos' の 2352 バイト RAW セクタを読む。
 *
 * sxsicd_readraw() が ci.fh == FILEH_INVALID を検出したときに呼ぶ。
 * 戻り値: SUCCESS / FAILURE
 */
BRESULT lcdd_readraw_sector(SXSIDEV sxsi, FILEPOS pos, void *buf);

/**
 * lcdd_readraw_forhash – ハッシュ計算用のセクタ読み出し。
 *
 * sxsicd_readraw_forhash() が fh == 0/INVALID を検出したときに呼ぶ。
 * *puSize に読み出しバイト数を設定する。戻り値: 0=成功, 非0=失敗。
 */
UINT lcdd_readraw_forhash(SXSIDEV sxsi, UINT uSecNo, UINT8 *pu8Buf, UINT *puSize);

#ifdef __cplusplus
}
#endif

#endif  /* _CDD_LIBCDIO_H_ */
