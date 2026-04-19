/**
 * @file	pmescp.h
 * @brief	ESC/P系印刷クラスの宣言およびインターフェイスの定義をします
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

#ifdef SUPPORT_PRINT_ESCP

#pragma once

#include "pmbase.h"
#include "cmdparser.h"

#define HTAB_SETS		33
#define VTAB_CHANNELS	8
#define VTAB_SETS		17

typedef enum {
	ESCP_LINEPOS_UNDER		= 1,
	ESCP_LINEPOS_STRIKET	= 2,
	ESCP_LINEPOS_UPPER		= 3
} ESCP_LINEPOS;

typedef enum {
	ESCP_LINEMODE_OFF = 0,
	ESCP_LINEMODE_SINGLE = 1,
	ESCP_LINEMODE_DOUBLE = 2,
	ESCP_LINEMODE_SINGLEDOT = 3,
	ESCP_LINEMODE_DOUBLEDOT = 4,
} ESCP_LINEMODE;

typedef struct {
	float posX; // 描画位置X pixel
	float posY; // 描画位置Y pixel

	float defUnit; // 単位
	float charPitchX; // 現在の1文字の幅
	float charPitchKanjiX; // 現在の1文字の幅（漢字）
	float charPoint; // 現在の文字のポイント
	float linespacing; // 現在の行の高さ
	float pagelength; // ページの長さ（インチ）
	float leftMargin; // 左マージン（インチ）
	float rightMargin; // 右マージン（インチ）
	float topMargin; // 上マージン（インチ）
	float bottomMargin; // 下マージン（インチ）
	int color; // 色
	ESCP_LINEPOS linepos; // 線の位置
	ESCP_LINEMODE linemode; // 線のタイプ

	float charBaseLineOffset; // ベースラインのオフセット

	float hTabPositions[HTAB_SETS]; // 水平タブ位置（文字単位）
	float vTabPositions[VTAB_CHANNELS][VTAB_SETS]; // 垂直タブ位置（行単位）
	int vTabCh; // 垂直タブチャネル

	float exSpc; // 1文字あたりの追加スペース（LQ 1/180単位、Draft 1/120単位）

	float leftSpcKanji; // 漢字1文字あたりの追加左スペース（1/160インチ単位）
	float rightSpcKanji; // 漢字1文字あたりの追加右スペース（1/160インチ単位）
	float leftSpcHalfKanji; // 半角漢字1文字あたりの追加左スペース（1/160インチ単位）
	float rightSpcHalfKanji; // 半角漢字1文字あたりの追加右スペース（1/160インチ単位）

	float graphicsPitchX; // グラフィックのピッチX
	float graphicsPitchY; // グラフィックのピッチY
	float graphicPosY; // グラフィックの描画位置Y pixel

	bool isCustomDefUnit; // defUnitが指定されているかどうか
	bool isKanji; // 漢字(2byte)モードかどうか
	bool isHalfKanji; // 半角漢字モードかどうか
	bool isRotKanji; // 縦漢字モードかどうか
	bool isGraphMode; // グラフィックモードかどうか
	bool isSansSerif; // サンセリフ書体かどうか
	bool isBold; // 太字かどうか
	bool isItalic; // 斜体かどうか
	bool isSup; // 上付かどうか
	bool isSub; // 下付かどうか
	bool isCondensed; // コンデンスモードかどうか
	bool isDoubleWidth; // 倍角モードかどうか
	bool isDoubleWidthSingleLine; // 単一行倍角モードかどうか
	bool isDoubleHeight; // 縦倍角モードかどうか
	bool hasGraphic; // グラフィック印字があるかどうか
	bool isKumimoji; // 組文字モードかどうか（自動解除）
	UINT8 kumimojiBuf[2]; // 組文字用バッファ
	bool isReverseLF; // 逆改行

	void SetDefault()
	{
		posX = 0;
		posY = 0;

		defUnit = 1.0 / 180;

		charPitchX = 1 / 10.0;// 0.15 / 2;
		charPitchKanjiX = 21 / 160.0;
		charPoint = 10.8;
		linespacing = 1 / 6.0; //0.15;
		pagelength = 11.69; // XXX: A4縦
		leftMargin = 0; // 左マージン（インチ）
		rightMargin = 0; // 右マージン（インチ）
		topMargin = 0; // 上マージン（インチ）
		bottomMargin = 0; // 下マージン（インチ）
		color = 0;
		linepos = ESCP_LINEPOS_UNDER;
		linemode = ESCP_LINEMODE_OFF;

		charBaseLineOffset = 0;

		vTabCh = 0;
		for (int i = 0; i < HTAB_SETS; i++) {
			hTabPositions[i] = i * 8 * charPitchX;
		}
		for (int ch = 0; ch < VTAB_CHANNELS; ch++) {
			for (int i = 0; i < VTAB_SETS; i++) {
				vTabPositions[ch][i] = 0 * linespacing;
			}
		}

		exSpc = 0;

		leftSpcKanji = 0;
		rightSpcKanji = 3 / 160.0;
		leftSpcHalfKanji = 0;
		rightSpcHalfKanji = 2 / 160.0;

		graphicsPitchX = 1;
		graphicsPitchY = 1;
		graphicPosY = 0;

		isCustomDefUnit = false;
		isKanji = false;
		isHalfKanji = false;
		isRotKanji = false;
		isGraphMode = false;
		isSansSerif = false;
		isBold = false;
		isItalic = false;
		isSup = false;
		isSub = false;
		isCondensed = false;
		isDoubleWidth = false;
		isDoubleWidthSingleLine = false;
		isDoubleHeight = false;
		isKumimoji = false;
		isReverseLF = false;
	}

	double CalcDotPitchX() {
		return 1 / (float)160; // 1画素あたりが1/160インチ
	}
	double CalcDotPitchY() {
		return 1 / (float)160; // 1画素あたりが1/160インチ
	}
	double CalcCharPitchX() {
		return isKanji ? charPitchKanjiX : charPitchX;
	}
} PRINT_ESCP_STATE;

typedef struct {
	HFONT oldfont;				/*!< Old Font */
	HFONT fontbase;				/*!< Font Base */
	HFONT fontSansSerif;		/*!< Font Sans-serif */
	HFONT fontBoldbase;			/*!< Bold Font Base */
	HFONT fontBoldSansSerif;	/*!< Bold Font Sans-serif */
	HFONT fontItalicbase;		/*!< Italic Font Base */
	HFONT fontItalicSansSerif;	/*!< Italic Font Sans-serif */
	HFONT fontItalicBoldbase;		/*!< Italic Bold Font Base */
	HFONT fontItalicBoldSansSerif;	/*!< Italic Bold Font Sans-serif */
	HBRUSH brsDot[8]; // ドット描画用ブラシ8色分
	HPEN penLine; // 線描画用ペン
	ESCP_LINEPOS lastlinepos; // 線の位置
	ESCP_LINEMODE lastlinemode; // 線のタイプ
	int lastlinecolor; // 色
} PRINT_ESCP_GDIOBJ;

/**
 * @brief ESC/P系印刷クラス
 */
class CPrintESCP : public CPrintBase
{
public:
	CPrintESCP();
	virtual ~CPrintESCP();

	virtual void StartPrint(HDC hdc, int offsetXPixel, int offsetYPixel, int widthPixel, int heightPixel, float dpiX, float dpiY, float dotscale, bool rectdot);
	
	virtual void EndPrint();

	virtual bool Write(UINT8 data);

	bool CheckOverflowLine(float addCharWidth);

	bool CheckOverflowPage(float addLineHeight);

	virtual PRINT_COMMAND_RESULT DoCommand();

	virtual bool HasRenderingCommand();

	void UpdateFont();
	void UpdateFontSize();
	void UpdateLinePen();

	PRINT_ESCP_STATE m_state; // ESC/P状態

	PRINT_ESCP_GDIOBJ m_gdiobj; // GDI描画用オブジェクト

	UINT8* m_colorbuf;	// カラーバッファ
	int m_colorbuf_w;	// カラーバッファ幅
	int m_colorbuf_h;	// カラーバッファ高さ
private:
	void CPrintESCP::RenderGraphic();
	void Render(int count);

	PrinterCommandParser* m_parser;

	int m_cmdIndex; // 実行中コマンドのインデックス
	bool m_lastNewLine; // 前回行送りしたかどうか
	bool m_lastNewPage; // 前回ページ送りしたかどうか

	PRINT_ESCP_STATE m_renderstate; // ESC/P状態 描画用

	void ReleaseFont();
	void ReleaseLinePen();
};

#endif /* SUPPORT_PRINT_ESCP */