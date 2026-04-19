/**
 * @file	pmescp.cpp
 * @brief	ESC/P系印刷クラスの動作の実装を行います
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

#include "compiler.h"

#ifdef SUPPORT_PRINT_ESCP

#include "np2.h"

#include "pmescp.h"
#include "codecnv/codecnv.h"

//#define DEBUG_LINE

static unsigned short jis_to_sjis(unsigned short jis)
{
	UINT8 j1 = (UINT8)(jis >> 8);
	UINT8 j2 = (UINT8)(jis & 0xFF);

	/* JIS X 0208 の有効範囲チェック */
	if (j1 < 0x21 || j1 > 0x7E || j2 < 0x21 || j2 > 0x7E) {
		return 0;
	}

	/*
	 *   s1 = (j1 + 1)/2 + 0x70;  s1 >= 0xA0 なら s1 += 0x40;
	 *   s2 = j2 + 0x1F;          j1 が偶数なら s2 += 0x5E;
	 *   s2 >= 0x7F なら s2++;
	 */
	UINT8 s1 = (UINT8)(((j1 + 1) >> 1) + 0x70);
	if (s1 >= 0xA0) {
		s1 = (UINT8)(s1 + 0x40);
	}

	UINT8 s2 = (UINT8)(j2 + 0x1F);
	if ((j1 & 1) == 0) {               /* j1 が偶数（区が偶数） */
		s2 = (UINT8)(s2 + 0x5E);
	}
	if (s2 >= 0x7F) {
		s2 = (UINT8)(s2 + 1);
	}

	return (UINT16)((UINT16)s1 << 8 | s2);
}

static int jis_zenkaku_alnum_to_ascii(UINT8* jisbytes)
{
	UINT16 jis = (jisbytes[0] << 8) | jisbytes[1];
	// 全角数字 '０'..'９' : 0x2330..0x2339
	if (jis >= 0x2330 && jis <= 0x2339) {
		jis = (unsigned char)('0' + (jis - 0x2330));
		jisbytes[0] = jis >> 8;
		jisbytes[1] = jis & 0xff;
		return 1;
	}
	// 全角英大文字 'Ａ'..'Ｚ' : 0x2341..0x235A
	if (jis >= 0x2341 && jis <= 0x235A) {
		jis = (unsigned char)('A' + (jis - 0x2341));
		jisbytes[0] = jis >> 8;
		jisbytes[1] = jis & 0xff;
		return 1;
	}
	// 全角英小文字 'ａ'..'ｚ' : 0x2361..0x237A
	if (jis >= 0x2361 && jis <= 0x237A) {
		jis = (unsigned char)('a' + (jis - 0x2361));
		jisbytes[0] = jis >> 8;
		jisbytes[1] = jis & 0xff;
		return 1;
	}
	// その他記号類･･･無理やり変換
	if (0x2121 <= jis && jis <= 0x217e) {
		const char jisasctbl[] = {
			' ', 0  , 0  , ',', '.', 0  , ':', ';',
			'?', '!', 0  , 0  , '\'','`', 0  , '^',
			0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,
			0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,
			0  , 0  , '|', 0  , 0  , 0  , 0  , 0  ,
			'"', '(', ')', 0  , 0  , '[', ']', '{',
			'}', 0  , 0  , 0  , 0  , 0  , 0  , 0  ,
			0  , 0  , 0  , '+', '-', 0  , 0  , 0  ,
			'=', 0  , '<', '>', 0  , 0  , 0  , 0  ,
			0  , 0  , 0  , '\'','″',0  , '\\','$',
			0  , 0  , '%', '#', '&', '*', '@', 0  ,
			0  , 0  , 0  , 0  , 0  , 0
		};
		char asc = jisasctbl[jis - 0x2121];
		if (asc) {
			jisbytes[0] = 0; jisbytes[1] = asc;
			return 1;
		}
	}

	// 範囲外
	return 0;
}

//// 実際の混色
//static COLORREF ColorCodeToColorRef(UINT8 colorCode)
//{
//	switch (colorCode) {
//	case 0:
//		return RGB(0, 0, 0);
//	case 1:
//		return RGB(40, 60, 120);
//	case 2:
//		return RGB(150, 60, 40);
//	case 3:
//		return RGB(200, 70, 140);
//	case 4:
//		return RGB(50, 110, 60);
//	case 5:
//		return RGB(0, 160, 175);
//	case 6:
//		return RGB(245, 235, 50);
//	case 7:
//		return RGB(255, 255, 255);
//	}
//	return RGB(0, 0, 0);
//}

// 理想的な色
static COLORREF ColorCodeToColorRef(UINT8 colorCode)
{
	switch (colorCode) {
	case 0:
		return RGB(0, 0, 0);
	case 1:
		return RGB(0, 0, 255);
	case 2:
		return RGB(255, 0, 0);
	case 3:
		return RGB(255, 0, 255);
	case 4:
		return RGB(0, 255, 0);
	case 5:
		return RGB(0, 255, 255);
	case 6:
		return RGB(255, 255, 0);
	case 7:
		return RGB(255, 255, 255);
	}
	return RGB(0, 0, 0);
}

// PC-PR201のコードの方が扱いやすい（ビット演算できる）ので変換
static UINT8 ColorCodeToPR201Table[] = {
	0, 3, 5, 1, 6, 2, 4, 7
};

typedef enum {
	COMMANDFUNC_RESULT_OK = 0, // コマンド実行成功
	COMMANDFUNC_RESULT_COMPLETELINE = 1, // 改行必要
	COMMANDFUNC_RESULT_COMPLETEPAGE = 2, // 改ページ必要
	COMMANDFUNC_RESULT_OVERFLOWLINE = 3, // 最後のコマンドの手前で改行必要
	COMMANDFUNC_RESULT_OVERFLOWPAGE = 4, // 最後のコマンドの手前で改ページ必要
	COMMANDFUNC_RESULT_RENDERLINE = 5, // 行描画必要（改行はせず左に戻るだけ）
} COMMANDFUNC_RESULT;

typedef COMMANDFUNC_RESULT(*PFNPRINTCMD_COMMANDFUNC)(void* param, const PRINTCMD_DATA& data, bool render);

static COMMANDFUNC_RESULT pmescp_PutChar(void* param, const PRINTCMD_DATA& data, bool render);

static COMMANDFUNC_RESULT pmescp_CommandCR(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.posX = 0;
	if (owner->CheckOverflowPage(0)) {
		owner->m_state.posY = 0;
		return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
	}
	return COMMANDFUNC_RESULT_RENDERLINE;
}

static COMMANDFUNC_RESULT pmescp_CommandLF(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
#ifdef DEBUG_LINE
	if (render) {
		TCHAR txt[32];
		_stprintf(txt, _T("h%.2f"), owner->m_state.linespacing);
		TextOut(owner->m_hdc, 0, owner->m_offsetYPixel + owner->m_state.posY, txt, _tcslen(txt));
		MoveToEx(owner->m_hdc, 0 , owner->m_offsetYPixel + owner->m_state.posY, NULL);
		LineTo(owner->m_hdc, owner->m_widthPixel, owner->m_offsetYPixel + owner->m_state.posY);
	}
#endif
	owner->m_state.posY += (owner->m_state.linespacing * owner->m_dpiY + owner->m_state.charBaseLineOffset) * (owner->m_state.isReverseLF ? -1 : +1);
	owner->m_state.posX = 0;
	if (owner->m_state.isDoubleWidthSingleLine)
		owner->m_state.isDoubleWidth = false; {
		owner->m_state.isDoubleWidthSingleLine = false;
	}
	if (owner->CheckOverflowPage(0)) {
		owner->m_state.posY = 0;
		return COMMANDFUNC_RESULT_COMPLETEPAGE;
	}
	return COMMANDFUNC_RESULT_COMPLETELINE;
}

static COMMANDFUNC_RESULT pmescp_CommandFF(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.posY = 0;
	owner->m_state.posX = 0;
	if (owner->m_state.isDoubleWidthSingleLine)
		owner->m_state.isDoubleWidth = false; {
		owner->m_state.isDoubleWidthSingleLine = false;
	}
	return COMMANDFUNC_RESULT_COMPLETEPAGE;
}

static COMMANDFUNC_RESULT pmescp_CommandHT(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float charWidthInPixel = owner->m_state.CalcCharPitchX() * owner->m_dpiX;
	float newPos = 0;
	for (int i = 0; i < _countof(owner->m_state.hTabPositions); i++) {
		if (owner->m_state.hTabPositions[i] * charWidthInPixel > owner->m_widthPixel - (owner->m_state.leftMargin + owner->m_state.rightMargin)) return COMMANDFUNC_RESULT_OK;
		if (owner->m_state.posX < owner->m_state.hTabPositions[i] * charWidthInPixel) {
			owner->m_state.posX = owner->m_state.hTabPositions[i] * charWidthInPixel;
			return COMMANDFUNC_RESULT_OK;
		}
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandVT(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float charHeightInPixel = owner->m_state.linespacing * owner->m_dpiX;
	float newPos = 0;
	for (int i = 0; i < _countof(owner->m_state.vTabPositions); i++) {
		if (owner->m_state.vTabPositions[owner->m_state.vTabCh][i] * charHeightInPixel > owner->m_heightPixel - (owner->m_state.topMargin + owner->m_state.bottomMargin)) return COMMANDFUNC_RESULT_OK;
		if (owner->m_state.posY < owner->m_state.vTabPositions[owner->m_state.vTabCh][i] * charHeightInPixel) {
			owner->m_state.posX = 0;
			owner->m_state.posY = owner->m_state.vTabPositions[owner->m_state.vTabCh][i] * charHeightInPixel;
			if (owner->m_state.isDoubleWidthSingleLine)
				owner->m_state.isDoubleWidth = false; {
				owner->m_state.isDoubleWidthSingleLine = false;
			}
			return COMMANDFUNC_RESULT_COMPLETELINE;
		}
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandBS(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float moveLen = owner->m_state.CalcCharPitchX() * owner->m_dpiX;
	if (owner->m_state.posX < moveLen) return COMMANDFUNC_RESULT_OK;
	owner->m_state.posX -= moveLen;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandSI(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isCondensed = true;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandDC2(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isCondensed = false;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandSO(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isDoubleWidth = true;
	owner->m_state.isDoubleWidthSingleLine = true;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandDC4(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isDoubleWidth = false;
	owner->m_state.isDoubleWidthSingleLine = true;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetPageLengthDefinedUnit(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 2 && data.data[1] == 0){
		float defUnit = owner->m_state.isCustomDefUnit ? owner->m_state.defUnit : 1.0 / 360;
		owner->m_state.pagelength = (data.data[2] + data.data[3] * 256) * defUnit;
		owner->m_state.topMargin = 0;
		owner->m_state.bottomMargin = 0;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetPageFormat(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 4 && data.data[1] == 0) {
		float defUnit = owner->m_state.isCustomDefUnit ? owner->m_state.defUnit : 1.0 / 360;
		owner->m_state.topMargin = (data.data[2] + data.data[3] * 256) * defUnit;
		owner->m_state.bottomMargin = (data.data[4] + data.data[5] * 256) * defUnit;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetPageLengthLines(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.pagelength = data.data[0] * owner->m_state.linespacing;
	owner->m_state.topMargin = 0;
	owner->m_state.bottomMargin = 0;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetPageLengthInch(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.pagelength = data.data[0];
	owner->m_state.topMargin = 0;
	owner->m_state.bottomMargin = 0;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetBottomMargin(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.topMargin = 0;
	owner->m_state.bottomMargin = data.data[0] * owner->m_state.linespacing;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCCancelBottomMargin(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.topMargin = 0;
	owner->m_state.bottomMargin = 0;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetRightMargin(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.rightMargin = data.data[0] * owner->m_state.CalcCharPitchX();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetLeftMargin(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.leftMargin = data.data[0] * owner->m_state.CalcCharPitchX();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandFSKanjiON(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isKanji = true;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandFSKanjiOFF(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isKanji = false;
	//owner->m_state.isDoubleWidth = false;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandFSHalfKanjiON(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isHalfKanji = true;
	//owner->m_state.isDoubleWidth = false;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandFSHalfKanjiOFF(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isHalfKanji = false;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandFSSetSpcKanji(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float dotPitchX = owner->m_state.CalcDotPitchX();
	owner->m_state.leftSpcKanji = data.data[0] * dotPitchX;
	owner->m_state.rightSpcKanji = data.data[1] * dotPitchX;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandFSSetSpcHalfKanji(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float dotPitchX = owner->m_state.CalcDotPitchX();
	owner->m_state.leftSpcHalfKanji = data.data[0] * dotPitchX;
	owner->m_state.rightSpcHalfKanji = data.data[1] * dotPitchX;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandFSSet4Kanji(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float dotPitchX = owner->m_state.CalcDotPitchX();
	if (data.data[0] == 0) {
		owner->m_state.isDoubleWidth = false;
		owner->m_state.isDoubleHeight = false;
	}
	else if (data.data[0] == 1) {
		owner->m_state.isDoubleWidth = true;
		owner->m_state.isDoubleHeight = true;
	}
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandFSSetTKanji(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isRotKanji = true;
	if (render) owner->UpdateFontSize();
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandFSResetTKanji(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isRotKanji = false;
	if (render) owner->UpdateFontSize();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandFSKumimoji(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (!owner->m_state.isKanji) return COMMANDFUNC_RESULT_OK; // 漢字モードでないとき組文字は不可とする
	UINT8 thb[4] = { 0 };
	thb[0] = data.data[0];
	thb[1] = data.data[1];
	thb[2] = data.data[2];
	thb[3] = data.data[3];
	jis_zenkaku_alnum_to_ascii(thb);
	jis_zenkaku_alnum_to_ascii(thb + 2);
	if (thb[0] != 0 || thb[2] != 0) return COMMANDFUNC_RESULT_OK; // 全角文字の組文字は不可とする
	owner->m_state.kumimojiBuf[0] = thb[1];
	owner->m_state.kumimojiBuf[1] = thb[3];
	owner->m_state.isKumimoji = true;
	pmescp_PutChar(param, data, render);
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandFSSetFontKanji(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 0) {
		owner->m_state.isSansSerif = false;
		if (render) owner->UpdateFont();
	}
	else if (data.data[0] == 1) {
		owner->m_state.isSansSerif = true;
		if (render) owner->UpdateFont();
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetUnit(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 1 && data.data[1] == 0) {
		owner->m_state.defUnit = data.data[2] / 3600.0;
		owner->m_state.isCustomDefUnit = true;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCKanjiMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	UINT8 flags = data.data[0];
	owner->m_state.isDoubleWidth = !!(flags & 0x8);
	owner->m_state.isDoubleHeight = !!(flags & 0x4);
	owner->m_state.isHalfKanji = !!(flags & 0x2);
	owner->m_state.isRotKanji = !!(flags & 0x1);
	if (render) owner->UpdateFontSize();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetAbsHPosition(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float defUnit = owner->m_state.isCustomDefUnit ? owner->m_state.defUnit : 1.0 / 60;
	owner->m_state.posX = ((data.data[0] + data.data[1] * 256) * defUnit - owner->m_state.leftMargin) * owner->m_dpiX;
#ifdef DEBUG_LINE
	if (render) {
		MoveToEx(owner->m_hdc, owner->m_state.posX, owner->m_offsetYPixel + owner->m_state.posY, NULL);
		LineTo(owner->m_hdc, owner->m_state.posX, owner->m_offsetYPixel + owner->m_state.posY + owner->m_state.linespacing * owner->m_dpiY);
	}
#endif
	if (owner->CheckOverflowLine(0)) {
		pmescp_CommandLF(param, data, render);
		if (owner->CheckOverflowPage(0)) {
			owner->m_state.posY = 0;
			return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
		}
		return COMMANDFUNC_RESULT_COMPLETELINE;
	}
	else {
		if (owner->CheckOverflowPage(0)) {
			owner->m_state.posY = 0;
			return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
		}
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetHPosition(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float defUnit = owner->m_state.isCustomDefUnit ? owner->m_state.defUnit : 1.0 / 180;
	if(data.data[1] > 127) return COMMANDFUNC_RESULT_OK; // 無効入力
	owner->m_state.posX += (data.data[0] + data.data[1] * 256) * defUnit * owner->m_dpiX;
#ifdef DEBUG_LINE
	if (render) {
		MoveToEx(owner->m_hdc, owner->m_state.posX, owner->m_offsetYPixel + owner->m_state.posY, NULL);
		LineTo(owner->m_hdc, owner->m_state.posX, owner->m_offsetYPixel + owner->m_state.posY + owner->m_state.linespacing * owner->m_dpiY);
	}
#endif
	if (owner->CheckOverflowLine(0)) {
		pmescp_CommandLF(param, data, render);
		if (owner->CheckOverflowPage(0)) {
			owner->m_state.posY = 0;
			return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
		}
		return COMMANDFUNC_RESULT_COMPLETELINE;
	}
	else {
		if (owner->CheckOverflowPage(0)) {
			owner->m_state.posY = 0;
			return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
		}
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetAbsVPosition(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float defUnit = owner->m_state.isCustomDefUnit ? owner->m_state.defUnit : 1.0 / 360;
	owner->m_state.posY = ((data.data[0] + data.data[1] * 256) * defUnit - owner->m_state.topMargin) * owner->m_dpiY;
	if (owner->CheckOverflowPage(0)) {
		owner->m_state.posY = 0;
		return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
	}
	return COMMANDFUNC_RESULT_COMPLETELINE;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetVPosition(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float defUnit = owner->m_state.isCustomDefUnit ? owner->m_state.defUnit : 1.0 / 360;
	owner->m_state.posY += (data.data[0] + data.data[1] * 256) * defUnit * owner->m_dpiY;
	if (owner->CheckOverflowPage(0)) {
		owner->m_state.posY = 0;
		return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
	}
	return COMMANDFUNC_RESULT_COMPLETELINE;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetAdvVPosition(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.posY += (data.data[0] / 180.0) * owner->m_dpiY;
	if (owner->CheckOverflowPage(0)) {
		owner->m_state.posY = 0;
		return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
	}
	return COMMANDFUNC_RESULT_COMPLETELINE;
}

static COMMANDFUNC_RESULT pmescp_CommandESCHVSkip(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 0) {
		owner->m_state.posX += data.data[1] * owner->m_state.CalcCharPitchX() * owner->m_dpiX;
		if (owner->CheckOverflowLine(0)) {
			pmescp_CommandLF(param, data, render);
			if (owner->CheckOverflowPage(0)) {
				owner->m_state.posY = 0;
				return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
			}
			return COMMANDFUNC_RESULT_COMPLETELINE;
		}
		else {
			if (owner->CheckOverflowPage(0)) {
				owner->m_state.posY = 0;
				return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
			}
		}
		return COMMANDFUNC_RESULT_OK;
	}
	else if (data.data[0] == 1) {
		owner->m_state.posX = 0;
		owner->m_state.posY += data.data[1] * owner->m_state.linespacing * owner->m_dpiX;
		if (owner->CheckOverflowPage(0)) {
			owner->m_state.posY = 0;
			return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
		}
		return COMMANDFUNC_RESULT_COMPLETELINE;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetLineSpacing1_8(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.linespacing = 1 / 8.0;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetLineSpacing7_72(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.linespacing = 7 / 72.0;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetLineSpacing1_6(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.linespacing = 1 / 6.0;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetLineSpacingN_180(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.linespacing = data.data[0] / 180.0;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetLineSpacingN_360(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.linespacing = data.data[0] / 360.0;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetLineSpacingN_60(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.linespacing = data.data[0] / 60.0;
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetHTabs(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	int len = data.data.size();
	if (len > HTAB_SETS) len = HTAB_SETS;
	for (int i = 0; i < data.data.size(); i++) {
		owner->m_state.hTabPositions[i] = data.data[i] * owner->m_state.charPitchX;
	}
	for (int i = len; i < HTAB_SETS; i++) {
		owner->m_state.hTabPositions[i] = 0;
	}
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetVTabs(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	int len = data.data.size();
	if (len > VTAB_SETS) len = VTAB_SETS;
	for (int i = 0; i < len; i++) {
		owner->m_state.vTabPositions[owner->m_state.vTabCh][i] = data.data[i] * owner->m_state.linespacing;
	}
	for (int i = len; i < VTAB_SETS; i++) {
		owner->m_state.vTabPositions[owner->m_state.vTabCh][i] = 0;
	}
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetVFUTabs(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] < VTAB_CHANNELS) {
		int len = data.data.size() - 1;
		if (len > VTAB_SETS) len = VTAB_SETS;
		for (int i = 0; i < len; i++) {
			owner->m_state.vTabPositions[data.data[0]][i] = data.data[i + 1] * owner->m_state.linespacing;
		}
		for (int i = len; i < VTAB_SETS; i++) {
			owner->m_state.vTabPositions[data.data[0]][i] = 0;
		}
	}
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSetVFUTabCh(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] < VTAB_CHANNELS) {
		owner->m_state.vTabCh = data.data[0];
	}
	return COMMANDFUNC_RESULT_OK;
}
static COMMANDFUNC_RESULT pmescp_CommandESCSelectTypeface(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	
	if (data.data[0] == 0) {
		owner->m_state.isSansSerif = false;
		if (render) owner->UpdateFont();
	}
	else {
		owner->m_state.isSansSerif = true;
		if (render) owner->UpdateFont();
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelectPitchPoint(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	int m = data.data[0];
	int n = data.data[1] + data.data[2] * 256;

	if (m >= 5) {
		owner->m_state.charPitchX = m / 360.0;
	}
	if (n > 0) {
		owner->m_state.charPoint = n / 2.0;
		if (render) owner->UpdateFontSize();
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelect10_5pt10cpi(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.charPitchX = 1 / 10.0;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelect10_5pt12cpi(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.charPitchX = 1 / 12.0;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelect10_5pt15cpi(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.charPitchX = 1 / 15.0;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelectICSpc(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.exSpc = data.data[0] / 180.0;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelectBold(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isBold = true;
	if (render) owner->UpdateFont();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCCancelBold(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isBold = false;
	if (render) owner->UpdateFont();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelectItalic(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isItalic = true;
	if (render) owner->UpdateFont();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCCancelItalic(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isItalic = false;
	if (render) owner->UpdateFont();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCUnderline(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 0 || data.data[0] == '0') {
		owner->m_state.linemode = ESCP_LINEMODE_OFF;
	}
	else if (data.data[0] == 1 || data.data[0] == '1') {
		owner->m_state.linemode = ESCP_LINEMODE_SINGLE;
		owner->m_state.linepos = ESCP_LINEPOS_UNDER;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCLineScore(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] != 3 || data.data[1] != 0 || data.data[2] != 1) return COMMANDFUNC_RESULT_OK;
	owner->m_state.linepos = (ESCP_LINEPOS)data.data[3];
	owner->m_state.linemode = (ESCP_LINEMODE)data.data[4];

	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelectSupSub(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 1 || data.data[0] == 49) {
		owner->m_state.isSup = false;
		owner->m_state.isSub = true;
	}
	else if (data.data[0] == 0 || data.data[0] == 48) {
		owner->m_state.isSup = true;
		owner->m_state.isSub = false;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCCancelSupSub(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.isSup = false;
	owner->m_state.isSub = false;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCDoubleWidth(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 1 || data.data[0] == 49) {
		owner->m_state.isDoubleWidth = true;
	}
	else if (data.data[0] == 0 || data.data[0] == 48) {
		owner->m_state.isDoubleWidth = false;
	}
	owner->m_state.isDoubleWidthSingleLine = false;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCDoubleHeight(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 1 || data.data[0] == 49) {
		owner->m_state.isDoubleHeight = true;
	}
	else if (data.data[0] == 0 || data.data[0] == 48) {
		owner->m_state.isDoubleHeight = false;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCPaperLoadEject(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 'R') {
		return pmescp_CommandFF(param, data, render);
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSelectGraphicMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] == 1 && data.data[1] == 0 && (data.data[2] == 1 || data.data[2] == 49)) {
		owner->m_state.linespacing = owner->m_state.CalcDotPitchX() * 24;
		owner->m_state.isGraphMode = true;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCPrintGraphics(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;

	//owner->m_state.hasGraphic = true;
	//owner->m_state.graphicsPitchX = 3600.0 / data.data[1];
	//owner->m_state.graphicsPitchY = 3600.0 / data.data[2];
	//int vDotCount = data.data[3];
	//if (data.data[0] == 0) {
	//	owner->m_state.linespacing = owner->m_state.graphicsPitchY * vDotCount;
	//	int hDotCount = data.data[4] + data.data[5] * 256;
	//	int dataLen = vDotCount * ((hDotCount + 7) / 8);
	//	if (render) {
	//		int basePosIdx = (int)floor(owner->m_state.posX / owner->m_state.graphicsPitchX + 0.5);
	//		int posIdx = basePosIdx;
	//		int posIdxY = 0;
	//		const UINT8* dataptr = &(data.data[0]) + 6;
	//		if (vDotCount == 1) {
	//			for (int i = 0; i < dataLen; i++) {
	//				UINT8 bitdata = *dataptr;
	//				for (int sh = 0; sh < 8; sh++) {
	//					if (bitdata & 0x80) {
	//						owner->m_colorbuf[posIdx] &= 0;
	//					}
	//					bitdata <<= 1;
	//					posIdx++;
	//				}
	//				if (posIdx >= owner->m_colorbuf_w) goto finish;
	//				dataptr++;
	//			}
	//		}
	//		else if (vDotCount == 8) {
	//			for (int i = 0; i < dataLen; i++) {
	//				UINT8 bitdata = *dataptr;
	//				for (int sh = 0; sh < 8; sh++) {
	//					if (bitdata & 0x80) {
	//						owner->m_colorbuf[posIdx + posIdxY * owner->m_colorbuf_w] &= 0;
	//					}
	//					bitdata <<= 1;
	//					posIdx++;
	//					if (posIdx >= owner->m_colorbuf_w) goto finish;
	//					if (posIdx >= hDotCount) break;
	//				}
	//				posIdxY++;
	//				posIdx = basePosIdx;
	//				dataptr++;
	//			}
	//		}
	//		else if (vDotCount == 24) {
	//			dataLen = (dataLen / 3) * 3;
	//			for (int i = 0; i < dataLen; i+=3) {
	//				UINT32 bitdata = ((UINT32)dataptr[2] << 16) | ((UINT32)dataptr[1] << 8) | dataptr[0];
	//				for (int sh = 0; sh < 24; sh++) {
	//					if (bitdata & 0x800000) {
	//						owner->m_colorbuf[posIdx + posIdxY * owner->m_colorbuf_w] &= 0;
	//					}
	//					bitdata <<= 1;
	//					posIdxY++;
	//				}
	//				posIdxY = 0;
	//				posIdx++;
	//				if (posIdx >= owner->m_colorbuf_w) goto finish;
	//				dataptr+=3;
	//			}
	//		}
	//	finish:;
	//	}
	//	owner->m_state.posX += owner->m_state.graphicsPitchX * hDotCount;
	//}
	//else if (data.data[0] == 1) {
	//	// ランレングス
	//}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCPrintBitImage(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;

	owner->m_state.hasGraphic = true;
	int m = data.data[0];
	int n = data.data[1] + data.data[2] * 256;

	int bytesPerCol = 1;
	if (32 <= m) bytesPerCol = 3;
	if (71 <= m) return COMMANDFUNC_RESULT_OK;

	float pitchX = owner->m_state.CalcDotPitchX();
	float pitchY = owner->m_state.CalcDotPitchY();
	float ppitchX = 60;
	float ppitchY = 60;
	if (m == 1 || m == 2 || m == 33)  ppitchX = 120;
	if (m == 3)  ppitchX = 240;
	if (m == 4)  ppitchX = 80;
	if (m == 6 || m == 38)  ppitchX = 90;
	if (m == 39)  ppitchX = 180;
	if (m == 40)  ppitchX = 360;
	if (32 <= m) ppitchY = 180;
	owner->m_state.graphicsPitchX = 1 / ppitchX;// pitchX;// owner->m_dpiX* pitchX * ppitchX;
	owner->m_state.graphicsPitchY = 1 / ppitchY; //pitchY;// owner->m_dpiY* pitchY * ppitchY;
	int vDotCount = bytesPerCol * 8;
	//owner->m_state.linespacing = owner->m_state.graphicsPitchY * vDotCount;
	int hDotCount = n;
	int dataLen = bytesPerCol * hDotCount;
	if (render) {
		int posIdx = (int)floor(owner->m_state.posX / (owner->m_dpiX * owner->m_state.graphicsPitchX) + 0.5);
		int posIdxY = 0;
		const UINT8* dataptr = &(data.data[0]) + 3;
		if (vDotCount == 8) {
			for (int i = 0; i < hDotCount; i++) {
				UINT8 bitdata = *dataptr;
				if (posIdx >= owner->m_colorbuf_w) goto finish;
				for (int sh = 0; sh < 8; sh++) {
					if (bitdata & 0x80) {
						owner->m_colorbuf[posIdx + posIdxY * owner->m_colorbuf_w] &= owner->m_state.color;
					}
					bitdata <<= 1;
					posIdxY++;
				}
				posIdxY = 0;
				posIdx++;
				dataptr++;
			}
		}
		else if (vDotCount == 24) {
			for (int i = 0; i < hDotCount; i++) {
				UINT32 bitdata = ((UINT32)dataptr[0] << 16) | ((UINT32)dataptr[1] << 8) | dataptr[2];
				if (posIdx >= owner->m_colorbuf_w) goto finish;
				for (int sh = 0; sh < 24; sh++) {
					if (bitdata & 0x800000) {
						owner->m_colorbuf[posIdx + posIdxY * owner->m_colorbuf_w] &= owner->m_state.color;
					}
					bitdata <<= 1;
					posIdxY++;
				}
				posIdxY = 0;
				posIdx++;
				dataptr+=3;
			}
		}
	finish:;
	}
	owner->m_state.graphicPosY = owner->m_state.posY;
	owner->m_state.posX += hDotCount * (owner->m_dpiX * owner->m_state.graphicsPitchX);
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCSetColor(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	if (data.data[0] < 7) {
		owner->m_state.color = ColorCodeToPR201Table[data.data[0] & 0x7];
		SetTextColor(owner->m_hdc, ColorCodeToColorRef(owner->m_state.color));
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmescp_CommandESCInitParams(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	owner->m_state.SetDefault();
	return COMMANDFUNC_RESULT_OK;
}


static COMMANDFUNC_RESULT pmescp_PutChar(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintESCP* owner = (CPrintESCP*)param;
	float charWidth = owner->m_state.CalcCharPitchX() * owner->m_dpiX;
	float scaleX = 1;
	float scaleY = 1;
	TCHAR buf[3] = { 0 };
	UINT8 th[3] = { 0 };
	bool drawKumimoji = false;
	if (owner->m_state.isKumimoji) {
		th[0] = owner->m_state.kumimojiBuf[0];
		th[1] = owner->m_state.kumimojiBuf[1];
		owner->m_state.isKumimoji = false;
		drawKumimoji = true;
	}
	else {
		if (owner->m_state.isKanji) {
			UINT8 thb[3] = { 0 };
			thb[0] = data.data[0];
			thb[1] = data.data[1];
			if (owner->m_state.isHalfKanji) {
				if (jis_zenkaku_alnum_to_ascii(thb)) {
					// ASCIIへ変換できたら文字幅は半角と同じ
					charWidth *= 0.5;
				}
				else {
					// ASCIIへ変換できない場合は0.5倍で代用
					scaleX = 0.5;
				}
			}
			if (thb[0] == 0) {
				// 実質1バイト文字
				th[0] = thb[1];
			}
			else {
				UINT16 sjis = jis_to_sjis(thb[0] << 8 | thb[1]);
				th[0] = sjis >> 8;
				th[1] = sjis & 0xff;
			}
		}
		else {
			th[0] = data.data[0];
		}
	}
	UINT16 thw[3] = { 0 };
	codecnv_sjistoucs2(thw, drawKumimoji ? 2 : 1, (const char*)th, 2);
	buf[0] = (TCHAR)thw[0];
	buf[1] = (TCHAR)thw[1];

	if (owner->CheckOverflowPage(owner->m_state.CalcCharPitchX() * owner->m_dpiX)) {
		owner->m_state.posY = 0;
		return owner->CheckOverflowPage(owner->m_state.CalcCharPitchX() * owner->m_dpiX) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
	}
	if (owner->m_state.isCondensed) {
		scaleX *= 0.6;
	}
	charWidth *= scaleX;
	float charOffsetLeft = 0;
	float charOffsetRight = 0;
	if (owner->m_state.isKanji) {
		float kScale = 1;// owner->m_state.isDoubleWidth ? 2 : 1;
		if (owner->m_state.isHalfKanji) {
			charOffsetLeft = owner->m_state.leftSpcHalfKanji * owner->m_dpiX * kScale;
			charOffsetRight = owner->m_state.rightSpcHalfKanji * owner->m_dpiX * kScale;
			charWidth -= owner->m_state.CalcDotPitchX() * owner->m_dpiX * kScale * 1.275; // 謎
		}
		else {
			charOffsetLeft = owner->m_state.leftSpcKanji * owner->m_dpiX * kScale;
			charOffsetRight = owner->m_state.rightSpcKanji * owner->m_dpiX * kScale;
			charWidth -= owner->m_state.CalcDotPitchX() * owner->m_dpiX * kScale * 0.555; // 謎
		}
	}
	else {
		charOffsetLeft = 0;
		charOffsetRight = owner->m_state.exSpc * owner->m_dpiX;
	}
	//if (owner->m_state.isKanji) {
	//	if (owner->m_state.isHalfKanji) {
	//		charWidth += owner->m_state.leftSpcHalfKanji * owner->m_dpiX;
	//	}
	//	else {
	//		charWidth += owner->m_state.leftSpcKanji * owner->m_dpiX;
	//	}
	//}
	//else {
		charWidth += charOffsetLeft + charOffsetRight;
	//}
	if (owner->m_state.isDoubleWidth) {
		scaleX *= 2.0;
		charWidth *= 2;
	}
	int charHeightScale = owner->m_state.isDoubleHeight ? 2 : 1;
	scaleY *= charHeightScale;
	owner->m_state.charBaseLineOffset = max(owner->m_state.charBaseLineOffset, owner->m_state.CalcDotPitchY() * 24 * owner->m_dpiX * (charHeightScale - 1));
	if (owner->CheckOverflowLine(charWidth)) {
		owner->m_state.posX = 0;
		owner->m_state.posY += owner->m_state.linespacing * owner->m_dpiY;
		return COMMANDFUNC_RESULT_OVERFLOWLINE;
	}
	// 混色はせずここで描画
	charWidth -= charOffsetLeft;
	owner->m_state.posX += charOffsetLeft;
	float offsetY = 0;
	if (owner->m_state.isSup) {
		scaleY *= 2.0f / 3;
	}
	else if (owner->m_state.isSub) {
		scaleY *= 2.0f / 3;
		offsetY = (owner->m_state.charBaseLineOffset + owner->m_state.CalcDotPitchY() * 24 * owner->m_dpiX) * 1 / 3;
	}

	if (render) {
		int x = (owner->m_state.leftMargin * owner->m_dpiX + owner->m_offsetXPixel + owner->m_state.posX);
		//if (owner->m_state.isRotKanji) {
		//	GLYPHMETRICS gm = {0};
		//	MAT2 mat = { {0,1},{0,0},{0,0},{0,1} }; // identity
		//	DWORD r = GetGlyphOutline(owner->m_hdc, *buf, GGO_METRICS, &gm, 0, nullptr, &mat);
		//	if (r != GDI_ERROR)
		//	{
		//		// BlackBox補正
		//		x += -gm.gmptGlyphOrigin.x;
		//	}
		//}
		float posY = owner->m_state.topMargin * owner->m_dpiY + owner->m_state.posY + owner->m_state.charBaseLineOffset - owner->m_state.CalcDotPitchY() * 24 * owner->m_dpiX * (charHeightScale - 1) + offsetY;
		if (scaleX != 1 || scaleY != 1 || drawKumimoji) {
			XFORM xf = { 0 };
			if (drawKumimoji) {
				int cx = charWidth / 2;
				int cy = (owner->m_state.CalcDotPitchY() * 24 * owner->m_dpiX * charHeightScale) / 2;

				// スケール
				xf.eM11 = scaleX;  // X倍率
				xf.eM22 = scaleY;  // Y倍率
				xf.eM12 = xf.eM21 = 0.0f;
				xf.eDx = 0.0f;
				xf.eDy = 0.0f;

				// 原点へ移動
				XFORM t1 = { 0 };
				t1.eM11 = 1.0f; t1.eM22 = 1.0f;
				t1.eDx = -cx;  t1.eDy = -cy;

				// 回転
				XFORM r = { 0 };
				r.eM11 = 0;   r.eM12 = -1;
				r.eM21 = 1;   r.eM22 = 0;
				r.eDx = 0.0f; r.eDy = 0.0f;

				// 描画位置へ移動
				XFORM t2 = { 0 };
				t2.eM11 = 1.0f; t2.eM22 = 1.0f;
				t2.eDx = cx + x;  t2.eDy = cy + (owner->m_offsetYPixel + posY);

				SetWorldTransform(owner->m_hdc, &xf);
				if (!ModifyWorldTransform(owner->m_hdc, &t1, MWT_RIGHTMULTIPLY)) return COMMANDFUNC_RESULT_OK;
				if (!ModifyWorldTransform(owner->m_hdc, &r, MWT_RIGHTMULTIPLY)) return COMMANDFUNC_RESULT_OK;
				if (!ModifyWorldTransform(owner->m_hdc, &t2, MWT_RIGHTMULTIPLY)) return COMMANDFUNC_RESULT_OK;
				TextOut(owner->m_hdc, 0, 0, buf, 2);
				ModifyWorldTransform(owner->m_hdc, nullptr, MWT_IDENTITY);
			}
			else {
				xf.eM11 = scaleX;  // X倍率
				xf.eM22 = scaleY;  // Y倍率
				xf.eM12 = xf.eM21 = 0.0f;
				xf.eDx = 0.0f;
				xf.eDy = 0.0f;

				SetWorldTransform(owner->m_hdc, &xf);
				TextOut(owner->m_hdc, x / xf.eM11, (owner->m_offsetYPixel + posY) / xf.eM22, buf, 1);
				ModifyWorldTransform(owner->m_hdc, nullptr, MWT_IDENTITY);
			}
		}
		else {
			TextOut(owner->m_hdc, x, owner->m_offsetYPixel + posY, buf, 1);
		}
		if (owner->m_state.linemode != ESCP_LINEMODE_OFF) {
			owner->UpdateLinePen();
			float lineBeginX = x - charOffsetLeft;
			float lineEndX = x + charWidth;
			const float dotPitch = 1.0f / 160;
			int lineWidth = (int)ceil(owner->m_state.CalcDotPitchY() * owner->m_dpiY);
			float charHeight = owner->m_state.CalcDotPitchY() * 24 * owner->m_dpiX * charHeightScale;
			HPEN hOldPen = (HPEN)SelectObject(owner->m_hdc, owner->m_gdiobj.penLine);
			int lineposY = posY - lineWidth / 2;
			if (owner->m_state.linepos == ESCP_LINEPOS_UNDER) {
				// 下線
				lineposY += owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + charHeight - lineWidth;
				MoveToEx(owner->m_hdc, lineBeginX, lineposY, NULL);
				LineTo(owner->m_hdc, lineEndX, lineposY);
				if (owner->m_state.linemode == ESCP_LINEMODE_DOUBLE || owner->m_state.linemode == ESCP_LINEMODE_DOUBLEDOT) {
					lineposY -= 2 * lineWidth;
					MoveToEx(owner->m_hdc, lineBeginX, lineposY, NULL);
					LineTo(owner->m_hdc, lineEndX, lineposY);
				}
			}
			else if (owner->m_state.linepos == ESCP_LINEPOS_STRIKET) {
				// 取り消し線
				if (owner->m_state.linemode == ESCP_LINEMODE_DOUBLE || owner->m_state.linemode == ESCP_LINEMODE_DOUBLEDOT) {
					lineposY += owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + charHeight / 2 - lineWidth / 2 - lineWidth;
					MoveToEx(owner->m_hdc, lineBeginX, lineposY, NULL);
					LineTo(owner->m_hdc, lineEndX, lineposY);
					lineposY += 2 * lineWidth;
					MoveToEx(owner->m_hdc, lineBeginX, lineposY, NULL);
					LineTo(owner->m_hdc, lineEndX, lineposY);
				}
				else {
					lineposY += owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + charHeight / 2 - lineWidth / 2;
					MoveToEx(owner->m_hdc, lineBeginX, lineposY, NULL);
					LineTo(owner->m_hdc, lineEndX, lineposY);
				}
			}
			else if (owner->m_state.linepos == ESCP_LINEPOS_UPPER) {
				// 上線
				lineposY += owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY;
				MoveToEx(owner->m_hdc, lineBeginX, lineposY, NULL);
				LineTo(owner->m_hdc, lineEndX, lineposY);
				if (owner->m_state.linemode == ESCP_LINEMODE_DOUBLE || owner->m_state.linemode == ESCP_LINEMODE_DOUBLEDOT) {
					lineposY += 2 * lineWidth;
					MoveToEx(owner->m_hdc, lineBeginX, lineposY, NULL);
					LineTo(owner->m_hdc, lineEndX, lineposY);
				}
			}
			SelectObject(owner->m_hdc, hOldPen);
		}
	}
	owner->m_state.posX += charWidth;
	return COMMANDFUNC_RESULT_OK;
}

static PRINTCMD_DEFINE s_commandTableESCP[] = {
	// 基本制御コード
	PRINTCMD_DEFINE_FIXEDLEN("\x01", 0, NULL), // SOH
	PRINTCMD_DEFINE_FIXEDLEN("\x02", 0, NULL), // STX
	PRINTCMD_DEFINE_FIXEDLEN("\x03", 0, NULL), // ETX
	PRINTCMD_DEFINE_FIXEDLEN("\x04", 0, NULL), // EOT
	PRINTCMD_DEFINE_FIXEDLEN("\x05", 0, NULL), // ENQ
	PRINTCMD_DEFINE_FIXEDLEN("\x06", 0, NULL), // ACK
	PRINTCMD_DEFINE_FIXEDLEN("\x07", 0, NULL), // BEL
	PRINTCMD_DEFINE_FIXEDLEN("\x08", 0, pmescp_CommandBS), // BS
	PRINTCMD_DEFINE_FIXEDLEN("\x09", 0, pmescp_CommandHT), // HT
	PRINTCMD_DEFINE_FIXEDLEN("\x0a", 0, pmescp_CommandLF), // LF
	PRINTCMD_DEFINE_FIXEDLEN("\x0b", 0, pmescp_CommandVT), // VT
	PRINTCMD_DEFINE_FIXEDLEN("\x0c", 0, pmescp_CommandFF), // FF
	PRINTCMD_DEFINE_FIXEDLEN("\x0d", 0, pmescp_CommandCR), // CR
	PRINTCMD_DEFINE_FIXEDLEN("\x0e", 0, pmescp_CommandSO), // SO
	PRINTCMD_DEFINE_FIXEDLEN("\x0f", 0, pmescp_CommandSI), // SI
	PRINTCMD_DEFINE_FIXEDLEN("\x10", 0, NULL), // DLE
	PRINTCMD_DEFINE_FIXEDLEN("\x11", 0, NULL), // DC1
	PRINTCMD_DEFINE_FIXEDLEN("\x12", 0, pmescp_CommandDC2), // DC2
	PRINTCMD_DEFINE_FIXEDLEN("\x13", 0, NULL), // DC3
	PRINTCMD_DEFINE_FIXEDLEN("\x14", 0, pmescp_CommandDC4), // DC4
	PRINTCMD_DEFINE_FIXEDLEN("\x15", 0, NULL), // NAK
	PRINTCMD_DEFINE_FIXEDLEN("\x16", 0, NULL), // SYN
	PRINTCMD_DEFINE_FIXEDLEN("\x17", 0, NULL), // ETB
	PRINTCMD_DEFINE_FIXEDLEN("\x18", 0, NULL), // CAN
	PRINTCMD_DEFINE_FIXEDLEN("\x19", 0, NULL), // EM
	PRINTCMD_DEFINE_FIXEDLEN("\x1a", 0, NULL), // SUB
	//PRINTCMD_DEFINE_FIXEDLEN("\x1b", 0, NULL), // ESC -> 下の拡張制御コードへ
	//PRINTCMD_DEFINE_FIXEDLEN("\x1c", 1, NULL), // FS -> 下の拡張制御コードへ
	PRINTCMD_DEFINE_FIXEDLEN("\x1d", 0, NULL), // GS
	PRINTCMD_DEFINE_FIXEDLEN("\x1e", 0, NULL), // RS
	PRINTCMD_DEFINE_FIXEDLEN("\x1f", 0, NULL), // US

	// 拡張制御コード ESC
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(C", 4, pmescp_CommandESCSetPageLengthDefinedUnit),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(c", 6, pmescp_CommandESCSetPageFormat),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""C", 1, pmescp_CommandESCSetPageLengthLines),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""C\0", 1, pmescp_CommandESCSetPageLengthInch),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""N", 1, pmescp_CommandESCSetBottomMargin),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""O", 0, pmescp_CommandESCCancelBottomMargin),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""Q", 1, pmescp_CommandESCSetRightMargin),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""l", 1, pmescp_CommandESCSetLeftMargin),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""$", 2, pmescp_CommandESCSetAbsHPosition),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\\", 2, pmescp_CommandESCSetHPosition),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(V", 4, pmescp_CommandESCSetAbsVPosition),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(v", 4, pmescp_CommandESCSetVPosition),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""J", 1, pmescp_CommandESCSetAdvVPosition),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""f", 2, pmescp_CommandESCHVSkip),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(U", 3, pmescp_CommandESCSetUnit),

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""0", 0, pmescp_CommandESCSetLineSpacing1_8),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""1", 0, pmescp_CommandESCSetLineSpacing7_72),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""2", 0, pmescp_CommandESCSetLineSpacing1_6),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""3", 1, pmescp_CommandESCSetLineSpacingN_180),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""+", 1, pmescp_CommandESCSetLineSpacingN_360),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""A", 1, pmescp_CommandESCSetLineSpacingN_60),

	PRINTCMD_DEFINE_TERMINATOR("\x1b""D", '\0', pmescp_CommandESCSetHTabs),
	PRINTCMD_DEFINE_TERMINATOR("\x1b""B", '\0', pmescp_CommandESCSetVTabs),
	PRINTCMD_DEFINE_TERMINATOR("\x1b""b", '\0', pmescp_CommandESCSetVFUTabs),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""/", 1, pmescp_CommandESCSetVFUTabCh),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""e", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""a", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(t", 5, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""t", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""R", 1, NULL),

	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""&\0", 5, NULL),

	PRINTCMD_DEFINE_FIXEDLEN("\x1b"":\0", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""%", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""x", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""k", 1, pmescp_CommandESCSelectTypeface),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""X", 3, pmescp_CommandESCSelectPitchPoint),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""c", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""P", 0, pmescp_CommandESCSelect10_5pt10cpi),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""M", 0, pmescp_CommandESCSelect10_5pt12cpi),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""g", 0, pmescp_CommandESCSelect10_5pt15cpi),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""p", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b"" ", 1, pmescp_CommandESCSelectICSpc),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""E", 0, pmescp_CommandESCSelectBold),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""F", 0, pmescp_CommandESCCancelBold),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""G", 0, pmescp_CommandESCSelectBold),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""H", 0, pmescp_CommandESCCancelBold),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""4", 0, pmescp_CommandESCSelectItalic),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""5", 0, pmescp_CommandESCCancelItalic),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""!", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""G", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""H", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""-", 1, pmescp_CommandESCUnderline),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(-", 5, pmescp_CommandESCLineScore),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""S", 1, pmescp_CommandESCSelectSupSub),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""T", 0, pmescp_CommandESCCancelSupSub),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""q", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x0e", 0, pmescp_CommandSO),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x0f", 0, pmescp_CommandSI),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""W", 1, pmescp_CommandESCDoubleWidth),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""w", 1, pmescp_CommandESCDoubleHeight),
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""(^", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""6", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""7", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""I", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""m", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x19", 1, pmescp_CommandESCPaperLoadEject),

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""U", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""<", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""8", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""9", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""s", 1, NULL),

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(G", 3, pmescp_CommandESCSelectGraphicMode),
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b"".", 6, pmescp_CommandESCPrintGraphics),
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""*", 3, pmescp_CommandESCPrintBitImage),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""?", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""K", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""L", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""Y", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""Z", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""^", 3, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""r", 1, pmescp_CommandESCSetColor),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""(B", 8, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""@", 0, pmescp_CommandESCInitParams),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""#", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""=", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b"">", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""j", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""i", 1, NULL),

	// 拡張制御コード FS
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""S", 2, pmescp_CommandFSSetSpcKanji),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""T", 2, pmescp_CommandFSSetSpcHalfKanji),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""W", 1, pmescp_CommandFSSet4Kanji),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""J", 0, pmescp_CommandFSSetTKanji),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""K", 0, pmescp_CommandFSResetTKanji),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""C", 2, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""D", 4, pmescp_CommandFSKumimoji),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""k", 1, pmescp_CommandFSSetFontKanji),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""#", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""U", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""V", 0, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""Y", 6, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""r", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""-", 1, NULL),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""&", 0, pmescp_CommandFSKanjiON),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c"".", 0, pmescp_CommandFSKanjiOFF),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""\x0f", 0, pmescp_CommandFSHalfKanjiON),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""\x12", 0, pmescp_CommandFSHalfKanjiOFF),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""!", 1, pmescp_CommandESCKanjiMode),
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""\x0e", 0, pmescp_CommandSO), // SO
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""\x14", 0, pmescp_CommandDC4), // DC4
};

// ステータスによって長さが変わる可変長コマンド
static UINT32 pmescp_VariableLengthCallback(void* param, const PRINTCMD_DEFINE& cmddef, const std::vector<UINT8>& curBuffer) {
	CPrintESCP* owner = (CPrintESCP*)param;

	if (cmddef.cmd[0] == '\x1b') {
		if (cmddef.cmdlen == 3 && cmddef.cmd[1] == '&' && cmddef.cmd[2] == '\0') {
			const int a1 = curBuffer[6];
			return a1 * 3; // or Super/subscript characters a1 * 2
		}
		else if (cmddef.cmdlen == 3 && cmddef.cmd[1] == '(' && cmddef.cmd[2] == '^') {
			const int nL = curBuffer[3];
			const int nH = curBuffer[4];
			return nH * 256 + nL;
		}
		else if (cmddef.cmdlen == 2 && cmddef.cmd[1] == '.') {
			int vDotCount = curBuffer[5];
			int hDotCount = curBuffer[6] + curBuffer[7] * 256;
			int dataLen = vDotCount * ((hDotCount + 7) / 8);
			return dataLen;
		}
		else if (cmddef.cmdlen == 2 && cmddef.cmd[1] == '*') {
			int m = curBuffer[2];
			int n = curBuffer[3] + curBuffer[4] * 256;
			int bytesPerCol = 1;
			if (32 <= m)bytesPerCol = 3;
			if (71 <= m)bytesPerCol = 6;
			return n * bytesPerCol;
		}
		else if (cmddef.cmdlen == 2 && cmddef.cmd[1] == '^') {
			//TODO: つくる
			return 0;
		}
		else if (cmddef.cmdlen == 2 && 
			(cmddef.cmd[1] == 'K' || cmddef.cmd[1] == 'L' || cmddef.cmd[1] == 'Y' || cmddef.cmd[1] == 'Z')) {
			const int nL = curBuffer[2];
			const int nH = curBuffer[3];
			return nH * 256 + nL;
		}
		else if (cmddef.cmdlen == 3 && cmddef.cmd[1] == '(' && cmddef.cmd[2] == 'B') {
			const int nL = curBuffer[3];
			const int nH = curBuffer[4];
			return nH * 256 + nL - 6;
		}
	}

	return 0;
}

// 登録外コマンドの処理
static PRINTCMD_CALLBACK_RESULT pmescp_CommandParseCallback(void* param, const std::vector<UINT8>& curBuffer) {
	CPrintESCP* owner = (CPrintESCP*)param;

	// 通常は1byte文字 漢字モードなら2byte
	if (owner->m_state.isKanji) {
		if (curBuffer.size() == 1) return PRINTCMD_CALLBACK_RESULT_CONTINUE;
		if (curBuffer[1] < 0x20) {
			// 制御文字が来たらコマンド無効で再解釈し直す
			return PRINTCMD_CALLBACK_RESULT_CANCEL;
		}
	}

	return PRINTCMD_CALLBACK_RESULT_COMPLETE;
}
/**
 * コンストラクタ
 */
CPrintESCP::CPrintESCP()
	: CPrintBase()
	, m_state()
	, m_renderstate()
	, m_colorbuf(NULL)
	, m_colorbuf_w(0)
	, m_colorbuf_h(0)
	, m_cmdIndex(0)
{
	m_parser = new PrinterCommandParser(s_commandTableESCP, NELEMENTS(s_commandTableESCP), pmescp_CommandParseCallback, pmescp_VariableLengthCallback, this);
}

/**
 * デストラクタ
 */
CPrintESCP::~CPrintESCP()
{
	delete m_parser;
	if (m_colorbuf) {
		delete[] m_colorbuf;
		m_colorbuf = NULL;
		m_colorbuf_w = 0;
		m_colorbuf_h = 0;
	}
}

void CPrintESCP::StartPrint(HDC hdc, int offsetXPixel, int offsetYPixel, int widthPixel, int heightPixel, float dpiX, float dpiY, float dotscale, bool rectdot)
{
	CPrintBase::StartPrint(hdc, offsetXPixel, offsetYPixel, widthPixel, heightPixel, dpiX, dpiY, dotscale, rectdot);

	m_state.SetDefault();
	m_renderstate = m_state;
	m_cmdIndex = 0;
	m_lastNewLine = false;
	m_lastNewPage = false;

	const float dotPitch = m_dpiX * 1.0 / 360; // バッファは有り得る最小のピッチで用意
	m_colorbuf_w = (int)ceil(widthPixel / dotPitch);
	m_colorbuf_h = 24;
	m_colorbuf = new UINT8[m_colorbuf_w * m_colorbuf_h];
	memset(m_colorbuf, 0xff, m_colorbuf_w * m_colorbuf_h);

	// GDIオブジェクト用意
	memset(&m_gdiobj, 0, sizeof(m_gdiobj));

	UpdateFontSize();

	m_gdiobj.brsDot[0] = (HBRUSH)GetStockObject(BLACK_BRUSH);
	for (int i = 1; i < _countof(m_gdiobj.brsDot); i++) {
		if (!m_gdiobj.brsDot[i]) {
			m_gdiobj.brsDot[i] = CreateSolidBrush(ColorCodeToColorRef(i));
		}
	}

	SetGraphicsMode(m_hdc, GM_ADVANCED);

	SetBkMode(m_hdc, TRANSPARENT);

	UpdateFont();

	SetTextColor(m_hdc, ColorCodeToColorRef(m_state.color));
}

void CPrintESCP::EndPrint()
{
	// 描画残り分を出力する
	std::vector<PRINTCMD_DATA>& cmdList = m_parser->GetParsedCommandList();
	Render(cmdList.size()); // データを描画する
	//RenderGraphic(); // グラフィックも描画する
	m_cmdIndex = 0;
	cmdList.clear();

	CPrintBase::EndPrint();

	if (m_colorbuf) {
		delete[] m_colorbuf;
		m_colorbuf = NULL;
		m_colorbuf_w = 0;
		m_colorbuf_h = 0;
	}

	// GDIオブジェクト削除
	m_gdiobj.brsDot[0] = NULL;
	for (int i = 1; i < _countof(m_gdiobj.brsDot); i++) {
		if (m_gdiobj.brsDot[i]) {
			DeleteObject(m_gdiobj.brsDot[i]);
			m_gdiobj.brsDot[i] = NULL;
		}
	}

	ReleaseFont();
	ReleaseLinePen();
}

bool CPrintESCP::Write(UINT8 data)
{
	return m_parser->PushByte(data);
}

void CPrintESCP::RenderGraphic()
{
	if (m_state.hasGraphic) {
		float pitchx = m_dpiX * m_state.graphicsPitchX; // m_dpiX* m_state.CalcDotPitchX();
		float pitchy = m_dpiY * m_state.graphicsPitchY; // m_dpiY* m_state.CalcDotPitchY();

		int r = pitchx / 2 * m_dotscale;
		int rx = (float)ceil(pitchx / 2 * m_dotscale);
		int ry = (float)ceil(pitchy / 2 * m_dotscale);
		if (r == 0) r = 1;
		HBRUSH hBrush = m_gdiobj.brsDot[m_state.color];
		HPEN hPen = (HPEN)GetStockObject(NULL_PEN);
		HGDIOBJ oldPen = SelectObject(m_hdc, hPen);
		HGDIOBJ oldBrush = SelectObject(m_hdc, hBrush);
		int curColor = m_state.color;
		for (int y = 0; y < m_colorbuf_h; y++) {
			int cy = m_offsetYPixel + m_state.topMargin * m_dpiY + (int)(m_state.graphicPosY + y * pitchy);
			int beginX = -1;
			for (int x = 0; x <= m_colorbuf_w; x++) { // わざと右端+1回分回す
				int idx = y * m_colorbuf_w + x;
				int drawBeginX = -1;
				int drawEndX = -1;
				bool updateBrush = false;
				if (x < m_colorbuf_w) {
					// 連続してドットがある範囲を求める
					if ((m_colorbuf[idx] & 0x7) != 0x7) {
						if (beginX == -1) {
							beginX = x;
						}
						if ((m_colorbuf[idx] & 0x7) != curColor) {
							drawBeginX = beginX;
							drawEndX = x - 1;
							beginX = x;
							updateBrush = true;
						}
					}
					else if (beginX != -1) {
						drawBeginX = beginX;
						drawEndX = x - 1;
						beginX = -1;
					}
				}
				else if (beginX != -1) {
					// 右端に到達したら絶対描画
					drawBeginX = beginX;
					drawEndX = x - 1;
				}
				if (drawBeginX != -1) {
					if (m_rectdot) {
						// ドットをくっつけて1つにして描画
						int cxBegin = m_offsetXPixel + m_state.leftMargin * m_dpiX + (int)(drawBeginX * pitchx);
						int cxEnd = m_offsetXPixel + m_state.leftMargin * m_dpiX + (int)(drawEndX * pitchx);
						Rectangle(m_hdc, cxBegin - rx, cy - ry, cxEnd + rx, cy + ry);
					}
					else {
						// 1個ずつ点を打っていく
						for (int i = drawBeginX; i <= drawEndX; i++) {
							int cx = m_offsetXPixel + m_state.leftMargin * m_dpiX + (int)(i * pitchx);
							Ellipse(m_hdc, cx - r, cy - r, cx + r, cy + r);
						}
					}
				}
				if (updateBrush) {
					curColor = (m_colorbuf[idx] & 0x7);
					SelectObject(m_hdc, m_gdiobj.brsDot[curColor]);
				}
			}
		}
		SelectObject(m_hdc, oldBrush);
		SelectObject(m_hdc, oldPen);
		memset(m_colorbuf, 0xff, m_colorbuf_w * m_colorbuf_h); // グラフィック印字バッファを白紙に戻す
		m_state.hasGraphic = false;
	}
}

void CPrintESCP::Render(int count)
{
	float lastPosX = m_state.posX;
	float lastPosY = m_state.posY;
	bool completeLine = false;

	// m_stateからm_renderstateへ事前計算データを代入
	m_renderstate.charBaseLineOffset = m_state.charBaseLineOffset;

	// 前回のレンダリング完了時の状態に戻す
	m_state = m_renderstate;

	// 描画実行
	std::vector<PRINTCMD_DATA>& cmdList = m_parser->GetParsedCommandList();
	for (int i = 0; i < count; i++) {
		PFNPRINTCMD_COMMANDFUNC cmdfunc;
		if ((PFNPRINTCMD_COMMANDFUNC)cmdList[i].cmd) {
			cmdfunc = (PFNPRINTCMD_COMMANDFUNC)cmdList[i].cmd->userdata;
		}
		else {
			cmdfunc = pmescp_PutChar;
		}
		if (cmdfunc) {
			COMMANDFUNC_RESULT cmdResult = cmdfunc(this, cmdList[i], true);
			if (cmdResult == COMMANDFUNC_RESULT_OVERFLOWLINE || cmdResult == COMMANDFUNC_RESULT_COMPLETELINE ||
				cmdResult == COMMANDFUNC_RESULT_OVERFLOWPAGE || cmdResult == COMMANDFUNC_RESULT_COMPLETEPAGE) {
				completeLine = true;
				// グラフィック印字があれば描画
				RenderGraphic();
			}
		}
	}

	// 行描画終わりならベースラインオフセットをクリア
	if (completeLine) {
		m_state.charBaseLineOffset = 0;
	}

	// 座標だけは元の値の方を信用する
	// そうしないと何かの拍子にm_stateとm_renderstateが食い違ったときに無限ループしたりするので危険
	m_state.posX = lastPosX;
	m_state.posY = lastPosY;

	// レンダリング完了時の状態を記憶
	m_renderstate = m_state;
}

bool CPrintESCP::CheckOverflowLine(float addCharWidth)
{
	//return m_state.posX + addCharWidth > m_widthPixel;
	return m_state.posX + addCharWidth > m_widthPixel - (m_state.leftMargin + m_state.rightMargin) * m_dpiX;
}
bool CPrintESCP::CheckOverflowPage(float addLineHeight)
{
	return m_state.posY + addLineHeight > min(m_heightPixel, (m_state.pagelength - (m_state.topMargin + m_state.bottomMargin)) * m_dpiX);
}

PRINT_COMMAND_RESULT CPrintESCP::DoCommand()
{
	std::vector<PRINTCMD_DATA>& cmdList = m_parser->GetParsedCommandList();

	int cmdListLen = cmdList.size();
	if (cmdListLen <= m_cmdIndex) return PRINT_COMMAND_RESULT_OK;

	for (; m_cmdIndex < cmdListLen; m_cmdIndex++) {
		if (m_cmdIndex < 0) {
			// BUG!!!!!
			m_cmdIndex = 0;
			cmdList.clear();
			return PRINT_COMMAND_RESULT_OK;
		}
		PFNPRINTCMD_COMMANDFUNC cmdfunc;
		if ((PFNPRINTCMD_COMMANDFUNC)cmdList[m_cmdIndex].cmd) {
			cmdfunc = (PFNPRINTCMD_COMMANDFUNC)cmdList[m_cmdIndex].cmd->userdata;
		}
		else {
			cmdfunc = pmescp_PutChar;
		}
		if (cmdfunc) {
			COMMANDFUNC_RESULT cmdResult = cmdfunc(this, cmdList[m_cmdIndex], false);
			if (cmdResult == COMMANDFUNC_RESULT_OVERFLOWLINE || cmdResult == COMMANDFUNC_RESULT_COMPLETELINE) {
				// 改行を実行
				if (cmdResult == COMMANDFUNC_RESULT_COMPLETELINE || m_lastNewLine) {
					m_cmdIndex++; // 現在のコマンドは完了しているので1つ進める（そうでない場合最後のコマンドは未実行なので残す）
				}
				else {
					m_lastNewLine = true; // OVERFLOW無限ループを回避
				}
				Render(m_cmdIndex); // データを描画する
				cmdList.erase(cmdList.begin(), cmdList.begin() + m_cmdIndex);
				m_cmdIndex = -1; // ループ時に+1されるので-1
				cmdListLen = cmdList.size();
				m_lastNewPage = false;
			}
			else if (cmdResult == COMMANDFUNC_RESULT_OVERFLOWPAGE || cmdResult == COMMANDFUNC_RESULT_COMPLETEPAGE) {
				// 改ページを実行
				if (cmdResult == COMMANDFUNC_RESULT_COMPLETEPAGE || m_lastNewPage) {
					m_cmdIndex++; // 現在のコマンドは完了しているので1つ進める（そうでない場合最後のコマンドは未実行なので残す）
				}
				else {
					m_lastNewPage = true; // OVERFLOW無限ループを回避
				}
				Render(m_cmdIndex); // データを描画する
				RenderGraphic();
				cmdList.erase(cmdList.begin(), cmdList.begin() + m_cmdIndex);
				m_cmdIndex = 0;
				m_lastNewLine = false;
				return PRINT_COMMAND_RESULT_COMPLETEPAGE;
			}
			else if (cmdResult == COMMANDFUNC_RESULT_RENDERLINE) {
				// 行の描画を実行
				m_cmdIndex++; // 現在のコマンドは完了しているので1つ進める（そうでない場合最後のコマンドは未実行なので残す）
				Render(m_cmdIndex); // データを描画する
				cmdList.erase(cmdList.begin(), cmdList.begin() + m_cmdIndex);
				m_cmdIndex = -1; // ループ時に+1されるので-1
				cmdListLen = cmdList.size();
				m_lastNewLine = false;
				m_lastNewPage = false;
			}
			else {
				m_lastNewLine = false;
				m_lastNewPage = false;
			}
		}
	}
	return PRINT_COMMAND_RESULT_OK;
}

bool CPrintESCP::HasRenderingCommand()
{
	std::vector<PRINTCMD_DATA>& cmdList = m_parser->GetParsedCommandList();

	int cmdListLen = cmdList.size();
	if (cmdListLen <= m_cmdIndex) return false;

	for (int i = m_cmdIndex; i < cmdListLen; i++) {
		if ((PFNPRINTCMD_COMMANDFUNC)cmdList[m_cmdIndex].cmd) {
			PFNPRINTCMD_COMMANDFUNC cmdfunc = (PFNPRINTCMD_COMMANDFUNC)cmdList[m_cmdIndex].cmd->userdata;
			if (cmdfunc == pmescp_CommandESCPrintGraphics || cmdfunc == pmescp_CommandESCPrintBitImage) {
				// グラフィック描画
				return true;
			}
		}
		else {
			// 文字描画
			return true;
		}
	}
	return false;
}

void CPrintESCP::UpdateFont() 
{
	if (m_state.isSansSerif) {
		if (m_state.isBold && m_state.isItalic) {
			SelectObject(m_hdc, m_gdiobj.fontItalicBoldSansSerif);
		}
		else if (m_state.isBold) {
			SelectObject(m_hdc, m_gdiobj.fontBoldSansSerif);
		}
		else if (m_state.isItalic) {
			SelectObject(m_hdc, m_gdiobj.fontItalicSansSerif);
		}
		else {
			SelectObject(m_hdc, m_gdiobj.fontSansSerif);
		}
	}
	else {
		if (m_state.isBold && m_state.isItalic) {
			SelectObject(m_hdc, m_gdiobj.fontItalicBoldSansSerif);
		}
		else if (m_state.isBold) {
			SelectObject(m_hdc, m_gdiobj.fontBoldbase);
		}
		else if (m_state.isItalic) {
			SelectObject(m_hdc, m_gdiobj.fontItalicbase);
		}
		else {
			SelectObject(m_hdc, m_gdiobj.fontbase);
		}
	}
}

void CPrintESCP::ReleaseFont()
{
	if (m_gdiobj.oldfont) SelectObject(m_hdc, m_gdiobj.oldfont);
	if (m_gdiobj.fontbase) DeleteObject(m_gdiobj.fontbase);
	if (m_gdiobj.fontSansSerif) DeleteObject(m_gdiobj.fontSansSerif);
	if (m_gdiobj.fontBoldbase) DeleteObject(m_gdiobj.fontBoldbase);
	if (m_gdiobj.fontBoldSansSerif) DeleteObject(m_gdiobj.fontBoldSansSerif);
	if (m_gdiobj.fontItalicbase) DeleteObject(m_gdiobj.fontItalicbase);
	if (m_gdiobj.fontItalicSansSerif) DeleteObject(m_gdiobj.fontItalicSansSerif);
	if (m_gdiobj.fontItalicBoldbase) DeleteObject(m_gdiobj.fontItalicBoldbase);
	if (m_gdiobj.fontItalicBoldSansSerif) DeleteObject(m_gdiobj.fontItalicBoldSansSerif);
	m_gdiobj.oldfont = NULL;
	m_gdiobj.fontbase = NULL;
	m_gdiobj.fontSansSerif = NULL;
	m_gdiobj.fontBoldbase = NULL;
	m_gdiobj.fontBoldSansSerif = NULL;
	m_gdiobj.fontItalicbase = NULL;
	m_gdiobj.fontItalicSansSerif = NULL;
	m_gdiobj.fontItalicBoldbase = NULL;
	m_gdiobj.fontItalicBoldSansSerif = NULL;
}

void CPrintESCP::UpdateFontSize()
{
	ReleaseFont();

	LOGFONT lf = { 0 };
	lf.lfFaceName[0] = '\0';
	if (m_state.isRotKanji) {
		lstrcpy(lf.lfFaceName, _T("@"));
	}
	if (np2oscfg.prnfontM && _tcslen(np2oscfg.prnfontM) > 0) {
		lstrcat(lf.lfFaceName, np2oscfg.prnfontM);
	}
	else {
		lstrcat(lf.lfFaceName, _T("MS Mincho"));
	}
	lf.lfHeight = -MulDiv(m_state.charPoint, m_dpiY, 72);
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	m_gdiobj.fontbase = CreateFontIndirect(&lf);
	lf.lfWeight = FW_BOLD;
	m_gdiobj.fontBoldbase = CreateFontIndirect(&lf);
	lf.lfItalic = TRUE;
	m_gdiobj.fontItalicBoldbase = CreateFontIndirect(&lf);
	lf.lfWeight = FW_NORMAL;
	m_gdiobj.fontItalicbase = CreateFontIndirect(&lf);

	lf.lfItalic = FALSE;
	lf.lfWeight = FW_NORMAL;
	lf.lfFaceName[0] = '\0';
	if (m_state.isRotKanji) {
		lstrcpy(lf.lfFaceName, _T("@"));
	}
	if (np2oscfg.prnfontG && _tcslen(np2oscfg.prnfontG) > 0) {
		lstrcat(lf.lfFaceName, np2oscfg.prnfontG);
	}
	else {
		lstrcat(lf.lfFaceName, _T("MS Gothic"));
	}
	m_gdiobj.fontSansSerif = CreateFontIndirect(&lf);
	lf.lfWeight = FW_BOLD;
	m_gdiobj.fontBoldSansSerif = CreateFontIndirect(&lf);
	lf.lfItalic = TRUE;
	m_gdiobj.fontItalicBoldSansSerif = CreateFontIndirect(&lf);
	lf.lfWeight = FW_NORMAL;
	m_gdiobj.fontItalicSansSerif = CreateFontIndirect(&lf);

	m_gdiobj.oldfont = nullptr;
	if (m_gdiobj.fontbase) m_gdiobj.oldfont = (HFONT)SelectObject(m_hdc, m_gdiobj.fontbase);

	UpdateFont();
}

void CPrintESCP::ReleaseLinePen()
{
	if (m_gdiobj.penLine) DeleteObject(m_gdiobj.penLine);
	m_gdiobj.penLine = NULL;
}

void CPrintESCP::UpdateLinePen()
{
	if (m_state.linemode == m_gdiobj.lastlinemode &&
		m_state.color == m_gdiobj.lastlinecolor) return;

	ReleaseLinePen();

	if (m_state.linemode != ESCP_LINEMODE_OFF) {
		m_gdiobj.penLine = CreatePen(
			(m_state.linemode == ESCP_LINEMODE_SINGLEDOT || m_state.linemode == ESCP_LINEMODE_DOUBLEDOT) ? PS_DASH : PS_SOLID,
			(int)ceil(m_state.CalcDotPitchY() * m_dpiY),
			ColorCodeToColorRef(m_state.color)
		);
	}

	m_gdiobj.lastlinemode = m_state.linemode;
	m_gdiobj.lastlinecolor = m_state.color;
}

#endif /* SUPPORT_PRINT_ESCP */
