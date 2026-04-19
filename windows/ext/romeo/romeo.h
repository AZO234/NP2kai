/**
 * @file	romeo.h
 * @brief	ROMEO 用の PCI 定義です
 */

#pragma once

#define	ROMEO_VENDORID		0x6809		/*!< ベンダー ID */
#define	ROMEO_DEVICEID		0x2151		/*!< デバイス ID */
#define	ROMEO_DEVICEID2		0x8121		/*!< for Developer version */

/**
 * PCIDEBUG リザルト コード
 */
enum
{
	PCIERR_SUCCESS			= 0x00,		/*!< 成功 */
	PCIERR_INVALIDCLASS		= 0x83,		/*!< 不正なデバイス クラス */
	PCIERR_DEVNOTFOUND		= 0x86		/*!< デバイスが見つからない */
};

/**
 * コンフィグレーション レジスタ
 */
enum
{
	ROMEO_DEVICE_VENDOR		= 0x00,		/*!< ベンダ/デバイス ID */
	ROMEO_STATUS_COMMAND	= 0x04,		/*!< コマンド/ステータス レジスタ */
	ROMEO_CLASS_REVISON		= 0x08,		/*!< リビジョン ID / クラス コード */
	ROMEO_HEADTYPE			= 0x0c,		/*!< キャッシュ ライン サイズ / マスタ レイテンシ タイマ / ヘッダ タイプ */
	ROMEO_BASEADDRESS0		= 0x10,		/*!< ベース アドレス 0 */
	ROMEO_BASEADDRESS1		= 0x14,		/*!< ベース アドレス 1 */
	ROMEO_SUB_DEVICE_VENDOR	= 0x2c,		/*!< サブ システム ベンダID */
	ROMEO_PCIINTERRUPT		= 0x3c		/*!< インタラプト ライン / インタラプト ピン / 最小グラント / 最大レイテンシ */
};

/**
 * アドレス
 */
enum
{
	ROMEO_YM2151ADDR		= 0x0000,	/*!< YM2151 アドレス */
	ROMEO_YM2151DATA		= 0x0004,	/*!< YM2151 データ */
	ROMEO_CMDQUEUE			= 0x0018,	/*!< コマンド キュー */
	ROMEO_YM2151CTRL		= 0x001c,	/*!< YM2151 コントロール */
	ROMEO_YMF288ADDR1		= 0x0100,	/*!< YMF288 アドレス */
	ROMEO_YMF288DATA1		= 0x0104,	/*!< YMF288 データ */
	ROMEO_YMF288ADDR2		= 0x0108,	/*!< YMF288 拡張アドレス */
	ROMEO_YMF288DATA2		= 0x010c,	/*!< YMF288 拡張データ */
	ROMEO_YMF288CTRL		= 0x011c	/*!< YMF288 コントロール */
};
