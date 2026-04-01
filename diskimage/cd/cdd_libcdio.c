/**
 * @file cdd_libcdio.c
 * @brief libcdio backend for NP2kai CD image support
 *
 * diskimage/cd/ に置く。ビルドシステムに追加して -lcdio をリンクする。
 *
 * 必要な定義:
 *   SUPPORT_KAI_IMAGES  (他の cdd_* と共通)
 *   SUPPORT_LIBCDIO     (このファイル固有のオプトイン)
 *
 * ----------------------------------------------------------------
 * 設計メモ
 * ----------------------------------------------------------------
 * sxsicd_gettrk() は sxsi->hdl を CDINFO (_CDINFO*) にキャストする。
 * バイナリ互換を保つため _LCDD_HDL の先頭メンバを _CDINFO にする:
 *
 *   typedef struct {
 *       _CDINFO  ci;     // ← 先頭固定。sxsicd_gettrk() のキャストが通る
 *       CdIo_t  *cdio;  // libcdio ハンドル
 *   } _LCDD_HDL;
 *
 * ci.fh は FILEH_INVALID に設定し、I/O は libcdio に完全委譲する。
 * sxsicd_readraw() は fh == FILEH_INVALID を検出して
 * lcdd_readraw_sector() を呼ぶ（cdd_libcdio.h の統合手順を参照）。
 */

#include <compiler.h>
#include <dosio.h>
#include <fdd/sxsi.h>

#if defined(SUPPORT_KAI_IMAGES) && defined(SUPPORT_LIBCDIO)

#include <stdlib.h>
#include <string.h>

#include <cdio/cdio.h>
#include <cdio/cd_types.h>
#include <cdio/sector.h>
#include <cdio/track.h>

#include "diskimage/cddfile.h"
#include "diskimage/cd/cdd_libcdio.h"

/* -----------------------------------------------------------------------
 * sxsi->hdl に格納する複合構造体
 * _CDINFO を先頭に置くこと – 絶対に移動しないこと。
 * ----------------------------------------------------------------------- */
typedef struct {
	_CDINFO  ci;        /* NP2kai トラック情報 – 必ず先頭              */
	CdIo_t  *cdio;     /* libcdio ハンドル                             */
} _LCDD_HDL, *LCDD_HDL;

#define LCDD(sxsi)  ((LCDD_HDL)((sxsi)->hdl))

/* -----------------------------------------------------------------------
 * 内部ヘルパー
 * ----------------------------------------------------------------------- */

static UINT8 fmt_to_adrctl(track_format_t fmt)
{
	return (fmt == TRACK_FORMAT_AUDIO) ? TRACKTYPE_AUDIO : TRACKTYPE_DATA;
}

static UINT16 fmt_to_secsize(track_format_t fmt)
{
	return (fmt == TRACK_FORMAT_AUDIO) ? (UINT16)CDIO_CD_FRAMESIZE_RAW
	                                   : (UINT16)CDIO_CD_FRAMESIZE;
}

/* LBA lsn のトラックのセクタサイズを TOC から返す */
static UINT16 secsize_at_lsn(LCDD_HDL h, lsn_t lsn)
{
	SINT32 i;
	for (i = (SINT32)h->ci.trks - 1; i >= 0; i--) {
		if (h->ci.trk[i].pos <= (UINT32)lsn) {
			return h->ci.trk[i].sector_size;
		}
	}
	return CDIO_CD_FRAMESIZE;   /* フォールバック */
}

/* -----------------------------------------------------------------------
 * libcdio の TOC から _CDTRK 配列を構築する。
 * 戻り値: 埋めたトラック数（0 なら失敗）
 * ----------------------------------------------------------------------- */
static UINT build_trkinfo(CdIo_t *cdio, _CDTRK *trk, UINT max_trks)
{
	track_t  first, last, t;
	UINT     idx = 0;

	first = cdio_get_first_track_num(cdio);
	last  = cdio_get_last_track_num(cdio);
	if (first == CDIO_INVALID_TRACK || last == CDIO_INVALID_TRACK) {
		return 0;
	}

	for (t = first; t <= last && idx < max_trks; t++) {
		lsn_t          lsn_start, lsn_last;
		UINT32         sectors;
		track_format_t fmt;
		UINT16         secsize;
		_CDTRK        *p = &trk[idx];

		lsn_start = cdio_get_track_lsn     (cdio, t);
		lsn_last  = cdio_get_track_last_lsn(cdio, t);
		if (lsn_start == CDIO_INVALID_LSN || lsn_last == CDIO_INVALID_LSN) {
			continue;
		}

		sectors = (UINT32)(lsn_last - lsn_start + 1);
		fmt     = cdio_get_track_format(cdio, t);
		secsize = fmt_to_secsize(fmt);

		ZeroMemory(p, sizeof(_CDTRK));

		p->adr_ctl        = fmt_to_adrctl(fmt);
		p->point          = (UINT8)t;

		/* pos/pos0 – sxsicd_readraw のセクタサイズ検索に使われる */
		p->pos            = (UINT32)lsn_start;
		p->pos0           = (UINT32)lsn_start;

		/* CD-disc 絶対セクタ位置 */
		p->pregap_sector  = (UINT32)lsn_start;
		p->start_sector   = (UINT32)lsn_start;
		p->end_sector     = (UINT32)lsn_last;

		/* イメージファイル上のセクタ位置（単一ファイル想定） */
		p->img_pregap_sec = (UINT32)lsn_start;
		p->img_start_sec  = (UINT32)lsn_start;
		p->img_end_sec    = (UINT32)lsn_last;

		/* イメージファイル上のバイトオフセット */
		p->pregap_offset  = (UINT64)lsn_start      * secsize;
		p->start_offset   = (UINT64)lsn_start      * secsize;
		p->end_offset     = (UINT64)(lsn_last + 1) * secsize;

		p->sector_size    = secsize;
		p->pregap_sectors = 0;
		p->track_sectors  = sectors;
		p->sectors        = sectors;
		p->str_sec        = (UINT32)lsn_start;
		p->end_sec        = (UINT32)lsn_last;

		idx++;
	}
	return idx;
}

/* -----------------------------------------------------------------------
 * SXSIDEV コールバック
 * ----------------------------------------------------------------------- */

static BRESULT lcdd_reopen(SXSIDEV sxsi)
{
	LCDD_HDL h = LCDD(sxsi);
	if (h->cdio != NULL) {
		return SUCCESS;
	}
	h->cdio = cdio_open(h->ci.path, DRIVER_UNKNOWN);
	return (h->cdio != NULL) ? SUCCESS : FAILURE;
}

/**
 * lcdd_read – sxsi->read コールバック（2048 バイト論理セクタ単位）。
 *
 * pos  = LBA, size = バイト数（2048 の倍数）。
 * データトラックは cdio_read_data_sectors で 2048 B を読む。
 * オーディオ / Mode-2 は cdio_read_sector で 2352 B RAW フレームを返す
 *（sec2352_read と同様の動作）。
 */
static REG8 lcdd_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size)
{
	LCDD_HDL  h;
	lsn_t     lsn;
	UINT      i, nsecs;
	UINT16    secsize;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return 0x60;
	}
	if (pos < 0 || pos >= sxsi->totals) {
		return 0x40;
	}

	h     = LCDD(sxsi);
	lsn   = (lsn_t)pos;
	nsecs = (size + CDIO_CD_FRAMESIZE - 1) / CDIO_CD_FRAMESIZE;

	for (i = 0; i < nsecs; i++) {
		secsize = secsize_at_lsn(h, lsn + (lsn_t)i);

		if (secsize == CDIO_CD_FRAMESIZE) {
			/* データセクタ: 2048 バイトのユーザーデータ */
			if (cdio_read_data_sectors(h->cdio, buf, lsn + (lsn_t)i,
			                           CDIO_CD_FRAMESIZE, 1) != DRIVER_OP_SUCCESS) {
				return 0xd0;
			}
			buf += CDIO_CD_FRAMESIZE;
		}
		else {
			/* オーディオ / Mode-2: 2352 バイト RAW フレーム */
			UINT8 raw[CDIO_CD_FRAMESIZE_RAW];
			if (cdio_read_sector(h->cdio, raw, lsn + (lsn_t)i,
			                     TRACK_FORMAT_AUDIO) != DRIVER_OP_SUCCESS) {
				return 0xd0;
			}
			memcpy(buf, raw, CDIO_CD_FRAMESIZE_RAW);
			buf += CDIO_CD_FRAMESIZE_RAW;
		}
	}
	return 0x00;
}

static void lcdd_close(SXSIDEV sxsi)
{
	LCDD_HDL h = LCDD(sxsi);
	if (h && h->cdio) {
		cdio_destroy(h->cdio);
		h->cdio = NULL;
	}
}

static void lcdd_destroy(SXSIDEV sxsi)
{
	LCDD_HDL h = LCDD(sxsi);
	if (h) {
		if (h->cdio) {
			cdio_destroy(h->cdio);
		}
		_MFREE(h);
		sxsi->hdl = (INTPTR)NULL;
	}
}

/* -----------------------------------------------------------------------
 * lcdd_readraw_sector – sxsicd_readraw() から fh == FILEH_INVALID 時に呼ばれる
 *
 * LBA 'pos' の 2352 バイト RAW セクタを buf に読む。
 * 戻り値: SUCCESS / FAILURE
 * ----------------------------------------------------------------------- */
BRESULT lcdd_readraw_sector(SXSIDEV sxsi, FILEPOS pos, void *buf)
{
	LCDD_HDL h;
	lsn_t    lsn;
	UINT16   secsize;

	if (pos < 0 || pos >= sxsi->totals) {
		return FAILURE;
	}
	if (sxsi_prepare(sxsi) != SUCCESS) {
		return FAILURE;
	}

	h      = LCDD(sxsi);
	lsn    = (lsn_t)pos;
	secsize = secsize_at_lsn(h, lsn);

	if (secsize == CDIO_CD_FRAMESIZE) {
		/* データ専用イメージでは RAW 読み出し不可 */
		return FAILURE;
	}

	/* 2352 バイト RAW セクタ読み出し */
	if (cdio_read_sector(h->cdio, buf, lsn, TRACK_FORMAT_AUDIO) != DRIVER_OP_SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

/* -----------------------------------------------------------------------
 * lcdd_readraw_forhash – sxsicd_readraw_forhash() から fh==0/INVALID 時に呼ばれる
 *
 * *puSize に読み出しバイト数を設定する。戻り値: 0=成功, 非0=エラーコード。
 * ----------------------------------------------------------------------- */
UINT lcdd_readraw_forhash(SXSIDEV sxsi, UINT uSecNo, UINT8 *pu8Buf, UINT *puSize)
{
	LCDD_HDL h;
	UINT16   secsize;
	lsn_t    lsn = (lsn_t)uSecNo;

	if (!pu8Buf || !puSize) {
		return 1;
	}
	*puSize = 0;

	h       = LCDD(sxsi);
	secsize = secsize_at_lsn(h, lsn);

	if (secsize == CDIO_CD_FRAMESIZE) {
		/* データセクタ: 2048 バイト */
		if (cdio_read_data_sectors(h->cdio, pu8Buf, lsn,
		                           CDIO_CD_FRAMESIZE, 1) != DRIVER_OP_SUCCESS) {
			return 6;
		}
		*puSize = CDIO_CD_FRAMESIZE;
	}
	else {
		/* オーディオ / Mode-2: 2352 バイト RAW */
		if (cdio_read_sector(h->cdio, pu8Buf, lsn,
		                     TRACK_FORMAT_AUDIO) != DRIVER_OP_SUCCESS) {
			return 6;
		}
		*puSize = CDIO_CD_FRAMESIZE_RAW;
	}
	return 0;
}

/* -----------------------------------------------------------------------
 * opencdio – 公開エントリポイント
 * ----------------------------------------------------------------------- */
BRESULT opencdio(SXSIDEV sxsi, const OEMCHAR *path)
{
	CdIo_t   *cdio = NULL;
	LCDD_HDL  h    = NULL;
	_CDTRK    trk[99];
	UINT      trks;
	lsn_t     last_lsn;
	UINT      mediatype, i;

	ZeroMemory(trk, sizeof(trk));

	/* 1. イメージを開く（フォーマット自動判別） */
	cdio = cdio_open(path, DRIVER_UNKNOWN);
	if (cdio == NULL) {
		goto err_out;
	}

	/* 2. TOC を _CDTRK 配列に変換 */
	trks = build_trkinfo(cdio, trk, NELEMENTS(trk));
	if (trks == 0) {
		goto err_cdio;
	}

	/* 3. ディスクの総セクタ数（リードアウト LSN） */
	last_lsn = cdio_get_disc_last_lsn(cdio);
	if (last_lsn == CDIO_INVALID_LSN) {
		last_lsn = (lsn_t)(trk[trks - 1].end_sector + 1);
	}

	/* 4. メディアタイプフラグ */
	mediatype = 0;
	for (i = 0; i < trks; i++) {
		if (trk[i].adr_ctl == TRACKTYPE_DATA) {
			mediatype |= SXSIMEDIA_DATA;
		} else {
			mediatype |= SXSIMEDIA_AUDIO;
		}
	}

	/* 5. 複合ハンドルを確保 */
	h = (LCDD_HDL)_MALLOC(sizeof(_LCDD_HDL), path);
	if (h == NULL) {
		goto err_cdio;
	}
	ZeroMemory(h, sizeof(_LCDD_HDL));

	h->ci.fh   = FILEH_INVALID;   /* I/O は libcdio に委譲; fh は使わない */
	h->ci.trks = trks;
	CopyMemory(h->ci.trk, trk, trks * sizeof(_CDTRK));
	milstr_ncpy(h->ci.path, path, NELEMENTS(h->ci.path));
	h->cdio = cdio;

	/* 6. SXSIDEV を初期化 */
	sxsi->reopen    = lcdd_reopen;
	sxsi->read      = lcdd_read;
	sxsi->write     = NULL;
	sxsi->format    = NULL;
	sxsi->close     = lcdd_close;
	sxsi->destroy   = lcdd_destroy;

	sxsi->hdl       = (INTPTR)h;
	sxsi->totals    = (FILELEN)last_lsn;
	sxsi->cylinders = 0;
	sxsi->size      = CDIO_CD_FRAMESIZE;   /* 論理セクタ 2048 バイト */
	sxsi->sectors   = 1;
	sxsi->surfaces  = 1;
	sxsi->headersize= 0;
	sxsi->mediatype = (UINT8)mediatype;
	milstr_ncpy(sxsi->fname, path, NELEMENTS(sxsi->fname));

	return SUCCESS;

err_cdio:
	cdio_destroy(cdio);
err_out:
	return FAILURE;
}

#endif  /* SUPPORT_KAI_IMAGES && SUPPORT_LIBCDIO */
