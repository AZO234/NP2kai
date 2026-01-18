/**
 * @file	ct1741.h
 * @brief	Interface of the Creative SoundBlaster16 CT1741
 */

#pragma once

#ifdef SUPPORT_SOUND_SB16

#include "sound.h"
#include <io/dmac.h>

// DSPリセット状態
typedef enum {
	CT1741_DSPRST_NORMAL,
	CT1741_DSPRST_RESET,
	CT1741_DSPRST_SPECIAL,
} CT1741_DSPRST;

// DMA転送モード
typedef enum {
	CT1741_DMAMODE_NONE,
	CT1741_DMAMODE_2, 
	CT1741_DMAMODE_3, 
	CT1741_DMAMODE_4, 
	CT1741_DMAMODE_8,
	CT1741_DMAMODE_16,
	CT1741_DMAMODE_16A,
} CT1741_DMAMODE;

// DSPモード
typedef enum {
	CT1741_DSPMODE_NONE, // 何も再生していない
	CT1741_DSPMODE_DAC, // PIO再生モード
	CT1741_DSPMODE_DMA, // DMA再生モード
	CT1741_DSPMODE_DMA_PAUSE, // DMA再生モード（一時停止中）
	CT1741_DSPMODE_DMA2, // DMA再生モード（旧版互換用）
	CT1741_DSPMODE_DMA_IN, // DMA録音モード
} CT1741_DSPMODE;

// DSP DMAバッファサイズ
#define CT1741_DMA_BUFSIZE  1024
// DSP DMAバッファマスク
#define CT1741_DMA_BUFMASK  (CT1741_DMA_BUFSIZE - 1)
// DSP DMA読み取り単位
#define CT1741_DMA_READINTERVAL	(CT1741_DMA_BUFSIZE / 8)

// DSP コマンドバッファサイズ
#define CT1741_DSP_BUFSIZE 64

// DSP DMA情報　廃止フラグは未使用だが従来互換のため残している
typedef struct {
	BOOL stereo; // ステレオ再生
	BOOL obsolete1; // 廃止
	BOOL autoinit; // 自動継続再生モード
	CT1741_DMAMODE mode; // DMA転送モード
	UINT32 obsolete2; // 廃止
	UINT32 obsolete3; // 廃止
	UINT32 total; // DMA転送データ長さ
	UINT32 obsolete4;  // 廃止
	UINT32 obsolete5; // 廃止
	SINT16 obsolete6[CT1741_DMA_BUFSIZE]; // 廃止
	DMACH dmach; // 使用するDMAチャネル
	UINT32 obsolete7; // 廃止

	UINT bufsize; // サウンド再生用の循環バッファサイズ。データのread/writeは4byte単位（16bitステレオの1サンプル単位）で行うこと
	UINT bufdatas; // バッファ内のデータバイト数
	UINT bufpos; // バッファの読み取り位置。bufwposと一致してもよいが追い越してはいけない
	UINT bufwpos; // バッファの書き込み位置。周回遅れのbufposに追いついてはいけない（一致も不可）
	UINT32 obsolete_pos12; // サンプリング変換用（廃止）
	UINT32 obsolete_step12; // サンプリング変換用（廃止）
	UINT8 buffer[CT1741_DMA_BUFSIZE]; // PCMバッファ
	UINT32 rate2; // 再生サンプリングレート（実質廃止）

	UINT8 lastautoinit; // 最後に使用した自動継続再生モード状態
	UINT8 last16mode; // 最後に使用した16bit転送モード状態
	UINT32 laststartcount; // 最後に使用したDMA転送サイズ
	UINT32 laststartaddr; // 最後に使用したDMA転送開始アドレス
} DMA_INFO;

// DSP情報　廃止フラグは未使用だが従来互換のため残している
typedef struct {
	DMA_INFO dma; // DMA転送情報
	UINT8 resetout; // DSP_RESETOUT
	UINT8 cmd; // DSPコマンド
	UINT8 cmdlen; // DSPコマンドパラメータ長さ
	UINT8 obsolete1; // 廃止
	UINT8 obsolete2[CT1741_DSP_BUFSIZE]; // 廃止
	struct {
		UINT8 data[CT1741_DSP_BUFSIZE];
		UINT32 datalen;
		UINT32 reserved;
	} dspin; // DSP入力バッファ CPU -> DSP
	struct {
		UINT8 data[CT1741_DSP_BUFSIZE];
		UINT32 rpos;
		UINT32 datalen;
	} dspout; // DSP出力リングバッファ DSP -> CPU
	UINT8 testreg; // DSPテスト用レジスタ
	UINT32 wbusy; // 書き込み中かどうか
	CT1741_DSPMODE mode; // DSPモード
	UINT32 freq; // DSPのサンプリングレート
	UINT8 dmairq; // DSPのI/Oポートで読み書きされるIRQ　実際のDMAの値とは違うので注意（ct1741_set_dma_irq, ct1741_get_dma_irqを参照のこと）
	UINT8 dmachnum; // DSPのI/Oポートで読み書きされるDMAチャネル番号　実際のDMAの値とは違うので注意（ct1741_set_dma_ch, ct1741_get_dma_chを参照のこと）
	UINT8 cmd_o; // 直前のDSPコマンド

	int smpcounter2; // DMA転送開始以降に送られた有効なデータ数の合計
	int smpcounter; // DMA転送開始以降に送られたDMAデータ数の合計（無効なデータも含む）

	UINT8 speaker; // 音声出力有効状態
	UINT8 uartmode; // MIDI出力モード
} DSP_INFO;

// SB16情報
typedef struct {
	UINT8	dmairq; // SB16のIRQ
	UINT8	dmachnum; // SB16のDMAチャネル番号
	UINT16	base; // SB16のベースアドレス

	UINT8	mixsel; // SB16ミキサーレジスタのアクセスするインデックス
	UINT8	mixreg[0x100]; // SB16ミキサーレジスタ
	UINT32	mixregexp[0x100]; // SB16ミキサーレジスタ（音量スケール換算済み）

	DSP_INFO dsp_info; // DSP情報
} SB16;

// ステートセーブ互換性維持用
#define SIZEOF_SB16_OLD	4704

// DSP PIOバッファサイズ
#define CT1741_PIO_BUFSIZE 16384
// DSP PIOバッファマスク
#define CT1741_PIO_BUFMASK (CT1741_PIO_BUFSIZE - 1)

// 再生用に使用する情報
typedef struct {
	UINT8 buffer[CT1741_PIO_BUFSIZE];
	int bufpos;
	int bufwpos;
	int bufdatas;
} CT1741_PLAYINFO_PIO;
typedef struct {
	int bufdatasrem; // 停止した瞬間までに送られていたデータ数
	int playwaitcounter; // 再生ディレイカウンタ

	// PIO再生用バッファなど
	CT1741_PLAYINFO_PIO pio;

	// エミュレータ側の再生サンプリングレート
	UINT32 playrate;
} CT1741_PLAYINFO;

extern CT1741_PLAYINFO ct1741_playinfo;

// 各再生モードのアライメント
extern int CT1741_BUF_ALIGN[];

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(SUPPORT_MULTITHREAD)
void ct1741cs_enter_criticalsection(void);
void ct1741cs_leave_criticalsection(void);
#endif

void ct1741_initialize(UINT rate);

void ct1741_setpicirq(void);
void ct1741_resetpicirq(void);

void ct1741_set_dma_irq(UINT8 irq);
UINT8 ct1741_get_dma_irq();

void ct1741_set_dma_ch(UINT8 dmach);
UINT8 ct1741_get_dma_ch();

void ct1741_dma(NEVENTITEM item);
void ct1741_startdma();

REG8 DMACCALL ct1741dmafunc(REG8 func);

void SOUNDCALL ct1741_getpcm(DMA_INFO* ct, SINT32* pcm, UINT count);

#ifdef __cplusplus
};
#endif

#endif