/**
 * @file	pmpr201.h
 * @brief	PC-PR201系印刷クラスの宣言およびインターフェイスの定義をします
 */

 /*
  * Copyright (c) 2026 SimK
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#ifdef SUPPORT_PRINT_PR201

#pragma once

#include "pmbase.h"
#include "cmdparser.h"

typedef enum {
	PRINT_PR201_CODEMODE_8BIT = 0,
	PRINT_PR201_CODEMODE_7BIT = 1,
} PRINT_PR201_CODEMODE;

typedef enum {
	PRINT_PR201_PRINTMODE_N = 'N', // HSパイカ
	PRINT_PR201_PRINTMODE_H = 'H', // HDパイカ
	PRINT_PR201_PRINTMODE_Q = 'Q', // コンデンス
	PRINT_PR201_PRINTMODE_E = 'E', // エリート
	PRINT_PR201_PRINTMODE_P = 'P', // プロポーショナル
	PRINT_PR201_PRINTMODE_K = 'K', // 漢字横
	PRINT_PR201_PRINTMODE_t = 't', // 漢字縦
} PRINT_PR201_PRINTMODE;

typedef enum {
	PRINT_PR201_HSPMODE_NHS = '0',
	PRINT_PR201_HSPMODE_SHS = '1',
} PRINT_PR201_HSPMODE;

typedef enum {
	PRINT_PR201_CHARMODE_8BIT_KATAKANA = '$',
	PRINT_PR201_CHARMODE_8BIT_HIRAGANA = '&',
	PRINT_PR201_CHARMODE_7BIT_ASCII = '$',
	PRINT_PR201_CHARMODE_7BIT_HIRAGANA = '&',
	PRINT_PR201_CHARMODE_7BIT_CG = '#',
	// 以下、エミュレーションの都合上の値
	PRINT_PR201_CHARMODE_7BIT_KATAKANA = '%',
} PRINT_PR201_CHARMODE;

typedef enum {
	PRINT_PR201_SCRIPTMODE_DISABLE = '0',
	PRINT_PR201_SCRIPTMODE_SUPER = '1',
	PRINT_PR201_SCRIPTMODE_SUB = '2',
} PRINT_PR201_SCRIPTMODE;

typedef struct {
	float posX; // 描画位置X pixel
	float posY; // 描画位置Y pixel
	float actualLineHeight; // 現在の行の実際の高さ

	float leftMargin; // 左マージン幅 インチ単位
	float rightMargin;  // 右マージン幅 インチ単位
	float topMargin; // 上マージン幅 インチ単位

	bool isKanji; // 漢字(2byte)モードかどうか
	bool isRotHalf; // 半角縦描画モードかどうか
	bool isKumimoji; // 組文字モードかどうか（自動解除）
	UINT8 kumimojiBuf[2]; // 組文字用バッファ
	int kumimojiBufIdx; // 組文字用バッファ位置

	PRINT_PR201_CODEMODE codemode; // 8bit/7bitコードモード（PRINT_PR201_CHARMODEの解釈が変わる）

	bool isSelect; // SELECT/DESELECT状態
	PRINT_PR201_PRINTMODE mode; // 印字モード
	PRINT_PR201_HSPMODE hspMode; // HSパイカモード
	PRINT_PR201_CHARMODE charMode; // キャラクタモード
	PRINT_PR201_SCRIPTMODE scriptMode; // スクリプトモード
	bool downloadCharMode; // ダウンロード文字印字モード
	int charScaleX; // 文字スケールX
	int charScaleY; // 文字スケールY
	float charBaseLineOffset; // 文字ベースラインオフセット 縦倍角印刷でずれる
	float lpi; // lines per inch
	bool bold; // 太字
	int lineselect; // 下線・上線選択
	int linep1; // param1 S=実線
	int linep2; // param2 1=一重線, 1=二重線
	int linep3; // param3 線の太さ 2=細線, 4=中線
	int linecolor; // 線色
	bool lineenable; // 下線・上線有効
	int dotsp_left; // 左ドットスペース
	int dotsp_right; // 右ドットスペース
	bool copymode; // コピーモード
	int color; // 色
	bool isReverseLF; // 逆改行

	float fontsize; // フォントサイズポイント単位　実質横が縮小するだけで、縦は10.8ptのまま
	float kanjiwidth; // 漢字幅 インチ単位

	bool hasPrintDataInPage; // ページに何らかの印字があるかどうか
	bool hasGraphic; // グラフィック印字があるかどうか
	float graphicPosY; // グラフィックの描画位置Y pixel
	int maxCharScaleY; // 文字スケールY 行内最大値

	void SetDefault()
	{
		posX = 0;
		posY = 0;
		actualLineHeight = 0;

		leftMargin = 0; // 左マージン位置 インチ単位
		rightMargin = 13.6;  // 右マージン位置 インチ単位
		topMargin = 0; // 上マージン幅 インチ単位

		isKanji = false;
		isRotHalf = false;
		isKumimoji = false;
		kumimojiBufIdx = 0;

		codemode = PRINT_PR201_CODEMODE_8BIT;

		isSelect = true;
		mode = PRINT_PR201_PRINTMODE_N;
		hspMode = PRINT_PR201_HSPMODE_NHS;
		charMode = PRINT_PR201_CHARMODE_8BIT_KATAKANA;
		scriptMode = PRINT_PR201_SCRIPTMODE_DISABLE;
		downloadCharMode = false;
		charScaleX = 1;
		charScaleY = 1;
		charBaseLineOffset = 0;
		lpi = 6;
		bold = false;
		lineselect = 1;
		linep1 = 'S';
		linep2 = 1;
		linep3 = 2;
		linecolor = 0;
		lineenable = false;
		dotsp_left = 0;
		dotsp_right = 0;
		copymode = false;
		color = 0;
		isReverseLF = false;

		fontsize = 10.8;
		kanjiwidth = 3.0 / 20;

		hasPrintDataInPage = false;
		hasGraphic = false;
		graphicPosY = 0;
		maxCharScaleY = 1;
	}
} PRINT_PR201_STATE;

typedef struct {
	HFONT oldfont;			/*!< Old Font */
	HFONT fontbase;			/*!< Font Base */
	HFONT fontrot90;		/*!< Font Rotation */
	HFONT fontbold;			/*!< Bold Font Base */
	HFONT fontboldrot90;	/*!< Bold Font Rotation */
	HPEN penline; // ライン用ペン
	HBRUSH brsDot[8]; // ドット描画用ブラシ8色分

	UINT8 lastlinecolor;
	UINT8 lastlinep1;
	UINT8 lastlinep2;
	UINT8 lastlinep3;
} PRINT_PR201_GDIOBJ;

/**
 * @brief PC-PR201系印刷クラス
 */
class CPrintPR201 : public CPrintBase
{
public:
	CPrintPR201();
	virtual ~CPrintPR201();

	virtual void StartPrint(HDC hdc, int offsetXPixel, int offsetYPixel, int widthPixel, int heightPixel, float dpiX, float dpiY, float dotscale, bool rectdot);
	
	virtual void EndPrint();

	virtual bool Write(UINT8 data);
	
	virtual PRINT_COMMAND_RESULT DoCommand();

	virtual bool HasRenderingCommand();

	bool CheckOverflowLine(float addCharWidth);
	bool CheckOverflowPage(float addLineHeight);

	void UpdateFont();
	void UpdateLinePen();

	double CalcDotPitchX() {
		return m_dpiX / (float)160; // PC-PR201 1画素あたりが1/160インチ
	}
	double CalcDotPitchY() {
		return m_dpiY / (float)160; // PC-PR201 1画素あたりが1/160インチ
	}
	double CalcVFULineHeight() {
		return m_dpiY / 6; // 1/6 inch で1行
	}
	double CalcLineHeight() {
		return m_dpiY / m_state.lpi * m_state.maxCharScaleY; // 設定されている行の高さ
	}
	double CalcActualLineHeight() {
		return max(m_state.actualLineHeight, CalcLineHeight()); // 設定されている行の高さ
	}
	double CalcCPI() {
		return 1 / (10.8 / 72 / 2);
	}
	double CalcCurrentLetterWidth(bool isKanji = false) {
		float charWidth = (float)m_dpiX / CalcCPI();
		if (isKanji) {
			charWidth *= m_state.fontsize / 10.8;
			if (charWidth < m_dpiX * m_state.kanjiwidth / 2) {
				charWidth = m_dpiX * m_state.kanjiwidth / 2 * 0.98;
			}
		}
		if (m_state.mode == PRINT_PR201_PRINTMODE_Q) { // コンデンス
			charWidth *= 10.0 / 17;
		}
		else if (m_state.mode == PRINT_PR201_PRINTMODE_E) { // エリート
			charWidth *= 10.0 / 12;
		}
		else if (m_state.mode == PRINT_PR201_PRINTMODE_P) { // プロポーショナル XXX; 本当は字の幅が可変
			charWidth *= 0.9;
		}
		return charWidth;
	}

	PRINT_PR201_STATE m_state; // PC-PR201状態

	PRINT_PR201_GDIOBJ m_gdiobj; // GDI描画用オブジェクト

	UINT8* m_colorbuf;	// カラーバッファ
	int m_colorbuf_w;	// カラーバッファ幅
	int m_colorbuf_h;	// カラーバッファ高さ
	
private:
	void RenderGraphic();
	void Render(int count);

	PrinterCommandParser* m_parser;

	int m_cmdIndex; // 実行中コマンドのインデックス
	bool m_lastNewLine; // 前回行送りしたかどうか
	bool m_lastNewPage; // 前回ページ送りしたかどうか

	PRINT_PR201_STATE m_renderstate; // PC-PR201状態（描画用バックアップ）

};

#endif /* SUPPORT_PRINT_ESCP */
