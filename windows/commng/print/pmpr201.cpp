/**
 * @file	pmpr201.cpp
 * @brief	PC-PR201系印刷クラスの動作の実装を行います
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

#ifdef SUPPORT_PRINT_PR201

#include "np2.h"

#include "pmpr201.h"
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

typedef enum {
	COMMANDFUNC_RESULT_OK = 0, // コマンド実行成功
	COMMANDFUNC_RESULT_COMPLETELINE = 1, // 改行必要
	COMMANDFUNC_RESULT_COMPLETEPAGE = 2, // 改ページ必要
	COMMANDFUNC_RESULT_OVERFLOWLINE = 3, // 最後のコマンドの手前で改行必要
	COMMANDFUNC_RESULT_OVERFLOWPAGE = 4, // 最後のコマンドの手前で改ページ必要
	COMMANDFUNC_RESULT_RENDERLINE = 5, // 行描画必要（改行はせず左に戻るだけ）
} COMMANDFUNC_RESULT;

static void drawUnderline(CPrintPR201 *owner, float length) {

	float pitchX = owner->CalcDotPitchX();
	float pitchY = owner->CalcDotPitchY();
	float offsetY = 0;

	int x = owner->m_offsetXPixel + owner->m_state.leftMargin * owner->m_dpiX + owner->m_state.posX;
	float basePosY = owner->m_state.posY + owner->m_state.charBaseLineOffset - pitchY * 24 * (owner->m_state.charScaleY - 1) + offsetY;
	if (owner->m_state.lineenable) {
		float lineBeginX = owner->m_state.posX;
		float lineEndX = owner->m_state.posX + length;
		const float dotPitch = 1.0f / 160;
		int dotsize = (float)pitchX;
		dotsize *= owner->m_state.linep3 / 2;
		HPEN hOldPen = (HPEN)SelectObject(owner->m_hdc, owner->m_gdiobj.penline);
		int ypos = basePosY;
		if (owner->m_state.lineselect == 1) {
			// 下線
			ypos += owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + pitchY * 24 * owner->m_state.charScaleY - dotsize / 2;
		}
		else if (owner->m_state.lineselect == 2) {
			// 上線
			ypos += owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + dotsize / 2;
		}
		MoveToEx(owner->m_hdc, owner->m_offsetXPixel + owner->m_state.leftMargin * owner->m_dpiX + lineBeginX, ypos, NULL);
		LineTo(owner->m_hdc, owner->m_offsetXPixel + owner->m_state.leftMargin * owner->m_dpiX + lineEndX, ypos);
		SelectObject(owner->m_hdc, hOldPen);
	}
}

typedef COMMANDFUNC_RESULT (*PFNPRINTCMD_COMMANDFUNC)(void* param, const PRINTCMD_DATA& data, bool render);

static COMMANDFUNC_RESULT pmpr201_PutChar(void* param, const PRINTCMD_DATA& data, bool render);

static COMMANDFUNC_RESULT pmpr201_CommandVFU(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;

	UINT16* vfuData = (UINT16*)(&(data.data[0]));
	int vfuDataLen = data.data.size() / 2;

	//if (vfuDataLen >= 2) {
	//	// VFU処理
	//	int topPos = -1;
	//	int bottomPos = -1;
	//	for (int i = 0; i < vfuDataLen; i++) {
	//		if (vfuData[i] & 0x40) {
	//			if (vfuData[i] & 0x01) {
	//				if (topPos == -1) {
	//					topPos = i;
	//				}
	//				else {
	//					bottomPos = i;
	//					break;
	//				}
	//			}
	//		}
	//	}
	//	if (topPos != -1 && bottomPos != -1) {
	//		float paperLengthInch = (float)(bottomPos - topPos) / 6;
	//		float marginTop = ((float)owner->m_heightPixel / owner->m_dpiY - paperLengthInch) / 2;
	//		if (marginTop < 0) marginTop = 0;
	//		owner->m_state.topMargin = marginTop;
	//	}
	//	else {
	//		owner->m_state.topMargin = 0;
	//	}
	//}
	//else {
	//	owner->m_state.topMargin = 0;
	//}

	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandCR(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.posX = 0;
	if (owner->CheckOverflowPage(0)) {
		owner->m_state.posY = 0;
		return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
	}
	return COMMANDFUNC_RESULT_RENDERLINE;
}

static COMMANDFUNC_RESULT pmpr201_CommandLF(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (owner->m_state.actualLineHeight == 0) {
		owner->m_state.actualLineHeight = owner->CalcLineHeight(); // 行に何もない場合は現在設定の行高さとする
		if (owner->m_state.actualLineHeight == 0) {
			// ゼロ改行はCRと同じ扱いにする
			owner->m_state.posX = 0;
			return COMMANDFUNC_RESULT_RENDERLINE;
		}
	}
#ifdef DEBUG_LINE
	if (render) {
		TCHAR txt[32];
		_stprintf(txt, _T("h%.2f G%d"), owner->m_state.actualLineHeight, owner->m_state.hasGraphic);
		TextOut(owner->m_hdc, 0, owner->m_offsetYPixel + owner->m_state.posY, txt, _tcslen(txt));
		MoveToEx(owner->m_hdc, 0, owner->m_offsetYPixel + owner->m_state.posY, NULL);
		LineTo(owner->m_hdc, owner->m_widthPixel, owner->m_offsetYPixel + owner->m_state.posY);
	}
#endif
	owner->m_state.posY += owner->m_state.actualLineHeight * (owner->m_state.isReverseLF ? -1 : +1);
	owner->m_state.actualLineHeight = 0;
	owner->m_state.maxCharScaleY = 1;
	if (owner->CheckOverflowPage(0)) {
		owner->m_state.posY -= owner->m_heightPixel;
		return COMMANDFUNC_RESULT_COMPLETEPAGE;
	}
	return COMMANDFUNC_RESULT_COMPLETELINE;
}

static COMMANDFUNC_RESULT pmpr201_CommandHT(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	// TODO: 実装する
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandVT(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	float vtHeight = owner->CalcVFULineHeight() * 6; // TODO: 正式実装が必要 仮で6行毎にタブ
	owner->m_state.posY = floor((owner->m_state.posY + vtHeight) / vtHeight) * vtHeight * (owner->m_state.isReverseLF ? -1 : +1);
	if (owner->CheckOverflowPage(0)) {
		owner->m_state.posY = 0;
		return COMMANDFUNC_RESULT_COMPLETEPAGE;
	}
	return COMMANDFUNC_RESULT_COMPLETELINE;
}

static COMMANDFUNC_RESULT pmpr201_CommandFF(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (owner->m_state.actualLineHeight == 0) {
		owner->m_state.actualLineHeight = owner->CalcLineHeight(); // 行に何もない場合は現在設定の行高さとする
	}
#ifdef DEBUG_LINE
	if (render) {
		TCHAR txt[32];
		_stprintf(txt, _T("h%.2f G%d"), owner->m_state.actualLineHeight, owner->m_state.hasGraphic);
		TextOut(owner->m_hdc, 0, owner->m_offsetYPixel + owner->m_state.posY, txt, _tcslen(txt));
		MoveToEx(owner->m_hdc, 0, owner->m_offsetYPixel + owner->m_state.posY, NULL);
		LineTo(owner->m_hdc, owner->m_widthPixel, owner->m_offsetYPixel + owner->m_state.posY);
	}
#endif
	owner->m_state.posY = 0;
	return COMMANDFUNC_RESULT_COMPLETEPAGE;
}

static COMMANDFUNC_RESULT pmpr201_CommandSO(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (owner->m_state.codemode == PRINT_PR201_CODEMODE_8BIT) {
		owner->m_state.charScaleX = 2;
	}
	else {
		owner->m_state.charMode = PRINT_PR201_CHARMODE_7BIT_KATAKANA;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandSI(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (owner->m_state.codemode == PRINT_PR201_CODEMODE_8BIT) {
		owner->m_state.charScaleX = 1;
	}
	else {
		owner->m_state.charMode = PRINT_PR201_CHARMODE_7BIT_ASCII;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandDC2(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (owner->m_state.codemode == PRINT_PR201_CODEMODE_7BIT) {
		owner->m_state.charScaleX = 2;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandDC4(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (owner->m_state.codemode == PRINT_PR201_CODEMODE_7BIT) {
		owner->m_state.charScaleX = 1;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandCAN(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	// TODO: 実装する
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandDC1(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.isSelect = true;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandDC3(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.isSelect = false;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandUS(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	int val = data.data[0];
	if (val < 16) {
		// VFU実行 TODO:実装する
	}
	else {
		// n行改行
		int c = val - 16;
		if (c >= 1) {
			for (int i = 0; i < c; i++) {
				pmpr201_CommandLF(param, data, render);
			}
		}
		if (owner->CheckOverflowPage(0)) {
			owner->m_state.posY -= owner->m_heightPixel;
			return COMMANDFUNC_RESULT_COMPLETEPAGE;
		}
	}
	return COMMANDFUNC_RESULT_OK;
}



static COMMANDFUNC_RESULT pmpr201_CommandESCPrintMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.mode = (PRINT_PR201_PRINTMODE)data.cmd->cmd[1];
	owner->m_state.isKanji = owner->m_state.mode == PRINT_PR201_PRINTMODE_K || owner->m_state.mode == PRINT_PR201_PRINTMODE_t;
	if (render) owner->UpdateFont();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCHSPMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.hspMode = (PRINT_PR201_HSPMODE)data.data[0];
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCCharMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.charMode = (PRINT_PR201_CHARMODE)data.cmd->cmd[1];
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCScriptMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.scriptMode = (PRINT_PR201_SCRIPTMODE)data.data[0];
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCDownloadCharMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.downloadCharMode = (data.cmd->cmd[1] == '+');
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCe(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	int scaleY = data.data[0] - '0';
	int scaleX = data.data[1] - '0';
	if (1 <= scaleX && scaleX <= 9) owner->m_state.charScaleX = scaleX;
	if (1 <= scaleY && scaleY <= 9) owner->m_state.charScaleY = scaleY;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCR(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;

	COMMANDFUNC_RESULT r;

	int repcount = (data.data[0] - '0') * 100 + (data.data[1] - '0') * 10 + (data.data[2] - '0');

	PRINTCMD_DATA repdata;
	repdata.cmd = nullptr;
	repdata.data.push_back(data.data[3]);
	if (owner->m_state.isKanji) {
		repdata.data.push_back(data.data[4]);
	}
	for (int i = 0; i < repcount; i++) {
		r = pmpr201_PutChar(param, repdata, render);
		if (r != COMMANDFUNC_RESULT_OK) {
			if (r == COMMANDFUNC_RESULT_OVERFLOWLINE || r == COMMANDFUNC_RESULT_COMPLETELINE) {
				return COMMANDFUNC_RESULT_COMPLETELINE;
			}
			if (r == COMMANDFUNC_RESULT_OVERFLOWPAGE || r == COMMANDFUNC_RESULT_COMPLETEPAGE) {
				return COMMANDFUNC_RESULT_COMPLETEPAGE;
			}
		}
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCBoldMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (data.cmd->cmd[1] == '!') {
		owner->m_state.bold = true;
	}
	else if (data.cmd->cmd[1] == '\x22') { // "
		owner->m_state.bold = false;
	}
	if (render) owner->UpdateFont();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCLineSelect(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (data.data[0] == '1') {
		owner->m_state.lineselect = 1;
	}
	else if (data.data[0] == '2') {
		owner->m_state.lineselect = 2;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCLineEnable(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (data.cmd->cmd[1] == 'X') {
		owner->m_state.lineenable = true;
	}
	else if (data.cmd->cmd[1] == 'Y') {
		owner->m_state.lineenable = false;
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCDotSpace(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (owner->m_state.mode == PRINT_PR201_PRINTMODE_P ||
		owner->m_state.mode == PRINT_PR201_PRINTMODE_K ||
		owner->m_state.mode == PRINT_PR201_PRINTMODE_t) {
		float lineLen = owner->CalcDotPitchX() * data.cmd->cmd[1] * owner->m_state.charScaleX;
		if (render) {
			drawUnderline(owner, lineLen);
		}
		owner->m_state.posX += lineLen;
#ifdef DEBUG_LINE
		if (render) {
			MoveToEx(owner->m_hdc, owner->m_state.posX, owner->m_offsetYPixel + owner->m_state.posY, NULL);
			LineTo(owner->m_hdc, owner->m_state.posX, owner->m_offsetYPixel + owner->m_state.posY + owner->m_state.actualLineHeight);
		}
#endif
		if (owner->CheckOverflowLine(0)) {
			pmpr201_CommandLF(param, data, render);
			if (owner->CheckOverflowPage(0)) {
				owner->m_state.posY = 0;
				return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
			}
			return COMMANDFUNC_RESULT_OVERFLOWLINE;
		}
		else {
			if (owner->CheckOverflowPage(0)) {
				owner->m_state.posY = 0;
				return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
			}
		}
	}
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCGraph(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	float pitchX = owner->CalcDotPitchX();
	float pitchY = owner->CalcDotPitchY();
	if (owner->CheckOverflowPage(max(owner->m_state.actualLineHeight, pitchY * 24))) {
		owner->m_state.posY = 0;
		return owner->CheckOverflowPage(max(owner->m_state.actualLineHeight, pitchY * 24)) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
	}
	const int length = (data.data[0] - '0') * 1000 + (data.data[1] - '0') * 100 + (data.data[2] - '0') * 10 + (data.data[3] - '0');
	if (owner->CheckOverflowLine(pitchX * length)) {
		owner->m_state.posX = 0;
		return COMMANDFUNC_RESULT_OVERFLOWLINE;
	}

#ifdef DEBUG_LINE
	if (render) {
		MoveToEx(owner->m_hdc, owner->m_state.posX, owner->m_offsetYPixel + owner->m_state.posY, NULL);
		LineTo(owner->m_hdc, owner->m_state.posX, owner->m_offsetYPixel + owner->m_state.posY + owner->CalcLineHeight());
	}
#endif

	// 混色が必要なのでバッファにためておき行が終わったときに描画
	if (render) {
		int posx = (int)floor(owner->m_state.posX / pitchX + 0.5);
		int extstep = (int)floor(owner->m_state.charScaleX + 0.5);
		bool rep = false;
		const UINT8* buf = &(data.data[4]);
		switch (data.cmd->cmd[1]) {
		case 'V':
			rep = true;
		case 'S':
			for (int i = 0; i < length; i++) {
				int cx = posx + i * extstep;
				if (cx + extstep - 1 >= owner->m_colorbuf_w) break;
				UINT8 bitdata = buf[0];
				for (int cy = 0; cy < 16; cy += 2) {
					if (bitdata & 1) {
						int idx = cy * owner->m_colorbuf_w + cx;
						owner->m_colorbuf[idx] &= owner->m_state.color;
						if (extstep == 2) {
							owner->m_colorbuf[idx + 1] &= owner->m_state.color;
						}
					}
					bitdata >>= 1;
				}
				if (!rep) buf++;
			}
			break;

		case 'W':
			rep = true;
		case 'I':
			for (int i = 0; i < length; i++) {
				int cx = posx + i * extstep;
				if (cx + extstep - 1 >= owner->m_colorbuf_w) break;
				UINT16 bitdata = ((UINT16)buf[1] << 8) | buf[0];
				for (int cy = 0; cy < 16; cy++) {
					if (bitdata & 1) {
						int idx = cy * owner->m_colorbuf_w + cx;
						owner->m_colorbuf[idx] &= owner->m_state.color;
						if (extstep == 2) {
							owner->m_colorbuf[idx + 1] &= owner->m_state.color;
						}
					}
					bitdata >>= 1;
				}
				if (!rep) buf += 2;
			}
			break;

		case 'U':
			rep = true;
		case 'J':
			for (int i = 0; i < length; i++) {
				int cx = posx + i * extstep;
				if (cx + extstep - 1 >= owner->m_colorbuf_w) break;
				UINT32 bitdata = ((UINT32)buf[2] << 16) | ((UINT32)buf[1] << 8) | buf[0];
				for (int cy = 0; cy < 24; cy++) {
					if (bitdata & 1) {
						int idx = cy * owner->m_colorbuf_w + cx;
						owner->m_colorbuf[idx] &= owner->m_state.color;
						if (extstep == 2) {
							owner->m_colorbuf[idx + 1] &= owner->m_state.color;
						}
					}
					bitdata >>= 1;
				}
				if (!rep) buf += 3;
			}
			break;
		}
	}
	owner->m_state.posX += pitchX * length;

	owner->m_state.hasGraphic = true;
	owner->m_state.graphicPosY = owner->m_state.posY; // グラフィック描画Y座標を記憶

	// ページに何か印刷されているフラグを立てる
	owner->m_state.hasPrintDataInPage = true;

	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCF(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	const int length = (data.data[0] - '0') * 1000 + (data.data[1] - '0') * 100 + (data.data[2] - '0') * 10 + (data.data[3] - '0');
	owner->m_state.posX = length * owner->CalcDotPitchX();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCCopyMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.copymode = true;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCNativeMode(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.copymode = false;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCLeftMargin(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.copymode = false;
	float charWidthInInch = owner->CalcCurrentLetterWidth() / owner->m_dpiX;
	const int margin = (data.data[0] - '0') * 100 + (data.data[1] - '0') * 10 + (data.data[2] - '0');
	owner->m_state.leftMargin = margin * charWidthInInch;

	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCRightMargin(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.copymode = false;
	float charWidthInInch = owner->CalcCurrentLetterWidth() / owner->m_dpiX;
	const int margin = (data.data[0] - '0') * 100 + (data.data[1] - '0') * 10 + (data.data[2] - '0');
	owner->m_state.rightMargin = margin * charWidthInInch;

	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCRotHalf(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.isRotHalf = data.data[0] == '1';
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCKumimoji(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.isKumimoji = true;
	owner->m_state.kumimojiBufIdx = 0;

	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCA(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.lpi = 6;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCB(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.lpi = 8;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCT(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	const int value = (data.data[0] - '0') * 10 + (data.data[1] - '0');
	owner->m_state.lpi = 120.0f / value;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCf(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.isReverseLF = false;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCr(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.isReverseLF = true;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCa(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (!owner->m_state.hasPrintDataInPage) {
		return COMMANDFUNC_RESULT_OK; // 何も印刷されていなければ無視
	}
	return pmpr201_CommandFF(param, data, render);
}

static COMMANDFUNC_RESULT pmpr201_CommandESCb(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	if (!owner->m_state.hasPrintDataInPage) {
		return COMMANDFUNC_RESULT_OK; // 何も印刷されていなければ無視
	}
	return pmpr201_CommandFF(param, data, render);
}

static COMMANDFUNC_RESULT pmpr201_CommandESCC(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.color = (data.data[0] - '0') & 0x7;
	if (render) {
		owner->UpdateLinePen();
		SetTextColor(owner->m_hdc, ColorCodeToColorRef(owner->m_state.color));
	}
	return COMMANDFUNC_RESULT_OK;
}


static COMMANDFUNC_RESULT pmpr201_CommandFS04L(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.linep1 = data.data[0];
	owner->m_state.linep2 = data.data[1] - '0';
	owner->m_state.linep3 = data.data[2] - '0';
	if (render) owner->UpdateLinePen();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandFSw(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	int pnum[2] = { 0 };
	int pnumIdx = 0;
	const int dataLen = data.data.size();
	for (int i = 0; i < dataLen; i++) {
		if (data.data[i] == ',') {
			pnumIdx++;
			continue;
		}
		if (data.data[i] == '.') {
			break;
		}
		if (data.data[i] < '0' || '9' < data.data[i]) {
			return COMMANDFUNC_RESULT_OK;
		}
		pnum[pnumIdx] *= 10;
		pnum[pnumIdx] += data.data[i] - '0';
	}
	owner->m_state.dotsp_left = pnum[0];
	owner->m_state.dotsp_right = pnum[1];
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandFSFontSize(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	UINT32 val = (data.data[0] - '0') * 100 + (data.data[1] - '0') * 10 + (data.data[2] - '0');
	if (val == 120) owner->m_state.fontsize = 12.0;
	if (val == 108 || val == 105) owner->m_state.fontsize = 10.8;
	if (val == 96 || val == 95) owner->m_state.fontsize = 9.6;
	if (val == 72 || val == 70) owner->m_state.fontsize = 7.2;
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_CommandESCc1(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	owner->m_state.SetDefault();
	return COMMANDFUNC_RESULT_OK;
}

static COMMANDFUNC_RESULT pmpr201_PutChar(void* param, const PRINTCMD_DATA& data, bool render) {
	CPrintPR201* owner = (CPrintPR201*)param;
	float charWidth;
	TCHAR buf[3] = { 0 };
	UINT8 th[3] = { 0 };
	bool drawKumimoji = false;
	if (owner->m_state.isKanji) {
		if (data.data[0] == 0) {
			// 実質1バイト文字
			th[0] = data.data[1];
			charWidth = owner->CalcCurrentLetterWidth();
			if (owner->m_state.isKumimoji) {
				if (owner->m_state.kumimojiBufIdx < 2) {
					owner->m_state.kumimojiBuf[owner->m_state.kumimojiBufIdx] = th[0];
					owner->m_state.kumimojiBufIdx++;
				}
				if (owner->m_state.kumimojiBufIdx == 2) {
					drawKumimoji = true;
					th[0] = owner->m_state.kumimojiBuf[0];
					th[1] = owner->m_state.kumimojiBuf[1];
					charWidth = owner->CalcCurrentLetterWidth(true) * 2;
					owner->m_state.isKumimoji = false; // 組文字おわり
				}
				else {
					return COMMANDFUNC_RESULT_OK;
				}
			}
		}
		else {
			UINT16 sjis = jis_to_sjis(data.data[0] << 8 | data.data[1]);
			th[0] = sjis >> 8;
			th[1] = sjis & 0xff;
			charWidth = owner->CalcCurrentLetterWidth(true) * 2;
			owner->m_state.isKumimoji = false; // 組文字無効
		}
	}
	else {
		th[0] = data.data[0];
		charWidth = owner->CalcCurrentLetterWidth();
		owner->m_state.isKumimoji = false; // 組文字無効
	}
	UINT16 thw[3] = { 0 };
	codecnv_sjistoucs2(thw, drawKumimoji ? 2 : 1, (const char*)th, 2);
	buf[0] = (TCHAR)thw[0];
	buf[1] = (TCHAR)thw[1];

	float pitchX = owner->CalcDotPitchX();
	float pitchY = owner->CalcDotPitchY();
	owner->m_state.posX += owner->m_state.dotsp_left * pitchX;

	float lineHeight = owner->CalcActualLineHeight();
	if (owner->CheckOverflowPage(pitchY * 24)) {
		owner->m_state.posY = 0;
		return owner->CheckOverflowPage(pitchY * 24) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
	}
	float scaleX = owner->m_state.charScaleX;
	float scaleY = owner->m_state.charScaleY;
	float offsetY = 0;
	charWidth *= scaleX;
	if (owner->m_state.mode == PRINT_PR201_PRINTMODE_Q) { // コンデンス
		scaleX *= 10.0 / 17;
	}
	else if (owner->m_state.mode == PRINT_PR201_PRINTMODE_E) { // エリート
		scaleX *= 10.0 / 12;
	}
	else if (owner->m_state.mode == PRINT_PR201_PRINTMODE_P) { // プロポーショナル XXX; 本当は字の幅が可変
		scaleX *= 0.9;
	}
	if (owner->m_state.scriptMode == '1') {
		// sup
		scaleY /= 2;
	}
	else if (owner->m_state.scriptMode == '2') {
		// sub
		scaleY /= 2;
		offsetY = pitchY * 24 * scaleY;
	}
	if (owner->CheckOverflowLine(charWidth)) {
		owner->m_state.posX = 0;
		pmpr201_CommandLF(param, data, render);
		if (owner->CheckOverflowPage(0)) {
			owner->m_state.posY = 0;
			return owner->CheckOverflowPage(0) ? COMMANDFUNC_RESULT_COMPLETEPAGE : COMMANDFUNC_RESULT_OVERFLOWPAGE;
		}
		return COMMANDFUNC_RESULT_OVERFLOWLINE;
	}
	if (owner->m_state.isSelect) {
		if (owner->m_state.charScaleY > 1) {
			owner->m_state.charBaseLineOffset = max(owner->m_state.charBaseLineOffset, pitchY * 24 * (owner->m_state.charScaleY - 1));
		}
		// 混色はせずここで描画
		if (render) {
			int x = owner->m_offsetXPixel + owner->m_state.leftMargin * owner->m_dpiX + owner->m_state.posX;
			//if (owner->m_state.mode == PRINT_PR201_PRINTMODE_t) {
			//	GLYPHMETRICS gm = {0};
			//	MAT2 mat = { {0,1},{0,0},{0,0},{0,1} }; // identity
			//	DWORD r = GetGlyphOutline(owner->m_hdc, *buf, GGO_METRICS, &gm, 0, nullptr, &mat);
			//	if (r != GDI_ERROR)
			//	{
			//		// BlackBox補正
			//		x += -gm.gmptGlyphOrigin.x;
			//	}
			//}
			float basePosY = owner->m_state.posY + owner->m_state.charBaseLineOffset - pitchY * 24 * (owner->m_state.charScaleY - 1) + offsetY;
			if (scaleX != 1 || scaleY != 1 || drawKumimoji) {
				XFORM xf = { 0 };
				if (drawKumimoji) {
					int cx = charWidth / 2;
					int cy = (pitchY * 24 * owner->m_state.charScaleY) / 2;

					// スケール
					xf.eM11 = scaleX;  // X倍率
					xf.eM22 = scaleY;  // Y倍率
					xf.eM12 = xf.eM21 = 0.0f;
					xf.eDx = 0.0f;
					xf.eDy = 0.0f;

					// 原点へ移動
					XFORM t1 = {0};
					t1.eM11 = 1.0f; t1.eM22 = 1.0f;
					t1.eDx = -cx;  t1.eDy = -cy;

					// 回転
					XFORM r = {0};
					r.eM11 = 0;   r.eM12 = -1;
					r.eM21 = 1;   r.eM22 = 0;
					r.eDx = 0.0f; r.eDy = 0.0f;

					// 描画位置へ移動
					XFORM t2 = {0};
					t2.eM11 = 1.0f; t2.eM22 = 1.0f;
					t2.eDx = cx + x;  t2.eDy = cy + (owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + basePosY);

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
					TextOut(owner->m_hdc, x / xf.eM11, (owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + basePosY) / xf.eM22, buf, 1);
					ModifyWorldTransform(owner->m_hdc, nullptr, MWT_IDENTITY);
				}
			}
			else {
				TextOut(owner->m_hdc, x, owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + basePosY, buf, 1);
			}
			if (owner->m_state.lineenable) {
				float lineBeginX = owner->m_state.posX - owner->m_state.dotsp_left * pitchX;
				float lineEndX = owner->m_state.posX + charWidth + owner->m_state.dotsp_right * pitchX;
				const float dotPitch = 1.0f / 160;
				int dotsize = (float)pitchX;
				dotsize *= owner->m_state.linep3 / 2;
				HPEN hOldPen = (HPEN)SelectObject(owner->m_hdc, owner->m_gdiobj.penline);
				int ypos = basePosY;
				if (owner->m_state.lineselect == 1) {
					// 下線
					ypos += owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + pitchY * 24 * owner->m_state.charScaleY - dotsize / 2;
				}
				else if (owner->m_state.lineselect == 2) {
					// 上線
					ypos += owner->m_offsetYPixel + owner->m_state.topMargin * owner->m_dpiY + dotsize / 2;
				}
				MoveToEx(owner->m_hdc, owner->m_offsetXPixel + owner->m_state.leftMargin * owner->m_dpiX + lineBeginX, ypos, NULL);
				LineTo(owner->m_hdc, owner->m_offsetXPixel + owner->m_state.leftMargin * owner->m_dpiX + lineEndX, ypos);
				SelectObject(owner->m_hdc, hOldPen);
			}
		}

		owner->m_state.actualLineHeight = lineHeight;
		if (owner->m_state.maxCharScaleY < owner->m_state.charScaleY) {
			owner->m_state.maxCharScaleY = owner->m_state.charScaleY;
		}
	}
	owner->m_state.posX += charWidth;
	owner->m_state.posX += owner->m_state.dotsp_right * pitchX;

	// ページに何か印刷されているフラグを立てる
	owner->m_state.hasPrintDataInPage = true;

	return COMMANDFUNC_RESULT_OK;
}

static PRINTCMD_DEFINE s_commandTablePR201[] = {
	// 基本制御コード
	PRINTCMD_DEFINE_FIXEDLEN("\x01", 0, NULL), // SOH
	PRINTCMD_DEFINE_FIXEDLEN("\x02", 0, NULL), // STX
	PRINTCMD_DEFINE_FIXEDLEN("\x03", 0, NULL), // ETX
	PRINTCMD_DEFINE_FIXEDLEN("\x04", 0, NULL), // EOT
	PRINTCMD_DEFINE_FIXEDLEN("\x05", 0, NULL), // ENQ
	PRINTCMD_DEFINE_FIXEDLEN("\x06", 0, NULL), // ACK
	PRINTCMD_DEFINE_FIXEDLEN("\x07", 0, NULL), // BEL
	PRINTCMD_DEFINE_FIXEDLEN("\x08", 0, NULL), // BS
	PRINTCMD_DEFINE_FIXEDLEN("\x09", 0, pmpr201_CommandHT), // HT
	PRINTCMD_DEFINE_FIXEDLEN("\x0a", 0, pmpr201_CommandLF), // LF
	PRINTCMD_DEFINE_FIXEDLEN("\x0b", 0, pmpr201_CommandVT), // VT
	PRINTCMD_DEFINE_FIXEDLEN("\x0c", 0, pmpr201_CommandFF), // FF
	PRINTCMD_DEFINE_FIXEDLEN("\x0d", 0, pmpr201_CommandCR), // CR
	PRINTCMD_DEFINE_FIXEDLEN("\x0e", 0, pmpr201_CommandSO), // SO
	PRINTCMD_DEFINE_FIXEDLEN("\x0f", 0, pmpr201_CommandSI), // SI
	PRINTCMD_DEFINE_FIXEDLEN("\x10", 0, NULL), // DLE
	PRINTCMD_DEFINE_FIXEDLEN("\x11", 0, pmpr201_CommandDC1), // DC1
	PRINTCMD_DEFINE_FIXEDLEN("\x12", 0, pmpr201_CommandDC2), // DC2
	PRINTCMD_DEFINE_FIXEDLEN("\x13", 0, pmpr201_CommandDC3), // DC3
	PRINTCMD_DEFINE_FIXEDLEN("\x14", 0, pmpr201_CommandDC4), // DC4
	PRINTCMD_DEFINE_FIXEDLEN("\x15", 0, NULL), // NAK
	PRINTCMD_DEFINE_FIXEDLEN("\x16", 0, NULL), // SYN
	PRINTCMD_DEFINE_FIXEDLEN("\x17", 0, NULL), // ETB
	PRINTCMD_DEFINE_FIXEDLEN("\x18", 0, pmpr201_CommandCAN), // CAN
	PRINTCMD_DEFINE_FIXEDLEN("\x19", 0, NULL), // EM
	PRINTCMD_DEFINE_FIXEDLEN("\x1a", 0, NULL), // SUB
	//PRINTCMD_DEFINE_FIXEDLEN("\x1b", 0, NULL), // ESC -> 下の拡張制御コードへ
	//PRINTCMD_DEFINE_FIXEDLEN("\x1c", 0, NULL), // FS -> 下の拡張制御コードへ
	PRINTCMD_DEFINE_TERMINATOR("\x1d", '\x1e', pmpr201_CommandVFU), // GS VFU設定　RSで解除
	PRINTCMD_DEFINE_FIXEDLEN("\x1e", 0, NULL), // RS
	PRINTCMD_DEFINE_FIXEDLEN("\x1f", 1, pmpr201_CommandUS), // US

	// 拡張制御コード ESC
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""N", 0, pmpr201_CommandESCPrintMode), // HSパイカモード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""H", 0, pmpr201_CommandESCPrintMode), // HDパイカモード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""Q", 0, pmpr201_CommandESCPrintMode), // コンデンスモード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""E", 0, pmpr201_CommandESCPrintMode), // エリートモード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""P", 0, pmpr201_CommandESCPrintMode), // プロポーショナルモード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""K", 0, pmpr201_CommandESCPrintMode), // 漢字（横）モード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""t", 0, pmpr201_CommandESCPrintMode), // 漢字（縦）モード

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""n", 1, pmpr201_CommandESCHSPMode), // HSパイカモード

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""$", 0, pmpr201_CommandESCCharMode), // 力タカナモード設定(8bit) 英数モード設定(7bit)
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""&", 0, pmpr201_CommandESCCharMode), // ひらがなモード設定
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""#", 0, pmpr201_CommandESCCharMode), // CGグラフィックモード設定(7bit)

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""s", 1, pmpr201_CommandESCScriptMode), // スクリプト文字モード設定

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""+", 3 + 72, NULL), // 外字24x24登録
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""*", 3 + 32, NULL), // 外字16x16登録
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""l", 6, NULL), // ダウンロード文字の登録　複雑なのでVariableLengthCallback使用
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""l+", 0, pmpr201_CommandESCDownloadCharMode), // ダウンロード文字印字
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""l-", 0, pmpr201_CommandESCDownloadCharMode), // プリンタ内蔵文字印字
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""l0", 0, NULL), // ダウンロード文字クリア

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""e", 2, pmpr201_CommandESCe), // 文字スケール設定
	PRINTCMD_DEFINE_FIXEDLEN_V("\x1b""R", 4, pmpr201_CommandESCR), // キャラクタリピート　複雑なのでVariableLengthCallback使用

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""!", 0, pmpr201_CommandESCBoldMode), // 太字モード設定
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x22", 0, pmpr201_CommandESCBoldMode), // 太字モード解除

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""_", 1, pmpr201_CommandESCLineSelect), // オーバーライン・アンダーライン選択
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""X", 0, pmpr201_CommandESCLineEnable), // ライン有効
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""Y", 0, pmpr201_CommandESCLineEnable), // ライン無効

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""d", 1, NULL), // ドラフトモード（未実装）

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x00", 0, pmpr201_CommandESCDotSpace), // ドットスペース 0dot
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x01", 0, pmpr201_CommandESCDotSpace), // ドットスペース 1dot
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x02", 0, pmpr201_CommandESCDotSpace), // ドットスペース 2dot
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x03", 0, pmpr201_CommandESCDotSpace), // ドットスペース 3dot
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x04", 0, pmpr201_CommandESCDotSpace), // ドットスペース 4dot
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x05", 0, pmpr201_CommandESCDotSpace), // ドットスペース 5dot
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x06", 0, pmpr201_CommandESCDotSpace), // ドットスペース 6dot
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x07", 0, pmpr201_CommandESCDotSpace), // ドットスペース 7dot
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""\x08", 0, pmpr201_CommandESCDotSpace), // ドットスペース 8dot

	PRINTCMD_DEFINE_LENFIELD("\x1b""S", 4, 1, pmpr201_CommandESCGraph), // 8bit グラフィック印字
	PRINTCMD_DEFINE_LENFIELD("\x1b""I", 4, 2, pmpr201_CommandESCGraph), // 16bit グラフィック印字
	PRINTCMD_DEFINE_LENFIELD("\x1b""J", 4, 3, pmpr201_CommandESCGraph), // 24bit グラフィック印字
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""V", 5, pmpr201_CommandESCGraph), // 8bit グラフィック印字 リピート
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""W", 6, pmpr201_CommandESCGraph), // 16bit グラフィック印字 リピート
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""U", 7, pmpr201_CommandESCGraph), // 24bit グラフィック印字 リピート

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""F", 4, pmpr201_CommandESCF), // ドットアドレス

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""D", 0, pmpr201_CommandESCCopyMode), // コピーモードに設定
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""M", 0, pmpr201_CommandESCNativeMode), // ネイティブモードに設定

	PRINTCMD_DEFINE_FIXEDLEN("\x1b"">", 0, NULL), // 片方向印字モード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""]", 0, NULL), // 両方向印字モード

	PRINTCMD_DEFINE_TERMINATOR("\x1b""(", '.', NULL), // 水平タブ設定
	PRINTCMD_DEFINE_TERMINATOR("\x1b"")", '.', NULL), // 水平タブ部分クリア
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""2", 0, NULL), // 水平タブ全クリア

	PRINTCMD_DEFINE_TERMINATOR("\x1b""v", '.', NULL), // 簡易VFU
		
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""L", 3, pmpr201_CommandESCLeftMargin), // レフトマージン
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""/", 3, pmpr201_CommandESCRightMargin), // ライトマージン

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""h", 1, pmpr201_CommandESCRotHalf), // 半角文字縦印字
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""q", 0, pmpr201_CommandESCKumimoji), // 半角文字組文字縦印字

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""A", 0, pmpr201_CommandESCA), // 1/6インチ改行モード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""B", 0, pmpr201_CommandESCB), // 1/8インチ改行モード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""T", 2, pmpr201_CommandESCT), // n/120インチ改行モード

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""f", 0, pmpr201_CommandESCf), // 順方向改行モード
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""r", 0, pmpr201_CommandESCr), // 逆方向改行モード

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""a", 0, pmpr201_CommandESCa), // 全排出後全吸入
	PRINTCMD_DEFINE_FIXEDLEN("\x1b""b", 0, pmpr201_CommandESCb), // 全排出

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""O", 1, NULL), // ANK文字フォント

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""C", 1, pmpr201_CommandESCC), // カラー切り替え

	PRINTCMD_DEFINE_FIXEDLEN("\x1b""c1", 0, pmpr201_CommandESCc1), // リセット

	// 拡張制御コード FS
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""04L", 3, pmpr201_CommandFS04L), // ライン太さ指定
	PRINTCMD_DEFINE_TERMINATOR("\x1c""w", '.', pmpr201_CommandFSw), // 固定ドットスペース

	PRINTCMD_DEFINE_TERMINATOR("\x1c""m", '.', NULL), // 任意倍率文字
		
	PRINTCMD_DEFINE_TERMINATOR("\x1c""f", '.', NULL), // ホッパ切り替え
		
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""P", 0, NULL), // 縮小文字の組文字印字
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""A", 0, NULL), // 文字サイズ10.5pt　漢字3/20
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""B", 0, NULL), // 文字サイズ10.5pt　漢字1/5
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""C", 0, NULL), // 文字サイズ9.5pt　漢字1/6
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""D", 0, NULL), // 文字サイズ9.5pt　漢字2/15
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""F", 0, NULL), // 文字サイズ7pt　漢字1/10
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""G", 0, NULL), // 文字サイズ12pt　漢字1/6
	PRINTCMD_DEFINE_TERMINATOR("\x1c""p", '.', NULL), // 漢字文字幅
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""04S", 3, pmpr201_CommandFSFontSize), // 漢字文字サイズ
	PRINTCMD_DEFINE_FIXEDLEN("\x1c""06F", 5, NULL), // 文字フォント選択
	PRINTCMD_DEFINE_TERMINATOR("\x1c""c", '.', NULL), // 文字修飾

};

// ステータスによって長さが変わる可変長コマンド
static UINT32 pmpr201_VariableLengthCallback(void* param, const PRINTCMD_DEFINE& cmddef, const std::vector<UINT8>& curBuffer) {
	CPrintPR201* owner = (CPrintPR201*)param;

	if (cmddef.cmd[0] == '\x1b') {
		if (cmddef.cmd[1] == 'l') {
			// ダウンロード文字の登録
			switch (owner->m_state.mode) {
			case PRINT_PR201_PRINTMODE_N:
			case PRINT_PR201_PRINTMODE_H:
				return 54 - 4; // 既にデータを4byte読んでいるのでその分を引く
			case PRINT_PR201_PRINTMODE_Q:
				return 42 - 4; // 既にデータを4byte読んでいるのでその分を引く
			case PRINT_PR201_PRINTMODE_E:
				return 45 - 4; // 既にデータを4byte読んでいるのでその分を引く
				break;
			case PRINT_PR201_PRINTMODE_P:
			{
				const int w3 = (((int)curBuffer[4] - '0') * 10 + ((int)curBuffer[5] - '0')) * 3;
				if (w3 < 0 || 48 < w3) return PRINTCMD_CALLBACK_RESULT_INVALID; // データサイズは最大で48byte
				return w3; // プロポーショナルの場合はデータ前の領域が4byte分多いのでこれでOK
			}
			}
		}
		else if (cmddef.cmd[1] == 'R') {
			// キャラクタリピート
			return owner->m_state.isKanji ? 1 : 0; // 漢字モードは1byte多い
		}
	}
	return 0;
}

// 登録外コマンドの処理
static PRINTCMD_CALLBACK_RESULT pmpr201_CommandParseCallback(void* param, const std::vector<UINT8>& curBuffer) {
	CPrintPR201* owner = (CPrintPR201*)param;

	// 0xffは無効とする
	if (curBuffer[0] == 0xff) return PRINTCMD_CALLBACK_RESULT_INVALID;

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
CPrintPR201::CPrintPR201()
	: CPrintBase()
	, m_state()
	, m_renderstate()
	, m_colorbuf(NULL)
	, m_colorbuf_w(0)
	, m_colorbuf_h(0)
	, m_cmdIndex(0)
{
	m_parser = new PrinterCommandParser(s_commandTablePR201, NELEMENTS(s_commandTablePR201), pmpr201_CommandParseCallback, pmpr201_VariableLengthCallback, this);
	m_state.SetDefault();
	m_renderstate.SetDefault();
}

/**
 * デストラクタ
 */
CPrintPR201::~CPrintPR201()
{
	EndPrint();

	delete m_parser;
	if (m_colorbuf) {
		delete[] m_colorbuf;
		m_colorbuf = NULL;
		m_colorbuf_w = 0;
		m_colorbuf_h = 0;
	}
}

void CPrintPR201::StartPrint(HDC hdc, int offsetXPixel, int offsetYPixel, int widthPixel, int heightPixel, float dpiX, float dpiY, float dotscale, bool rectdot)
{
	CPrintBase::StartPrint(hdc, offsetXPixel, offsetYPixel, widthPixel, heightPixel, dpiX, dpiY, dotscale, rectdot);

	m_state.SetDefault();
	m_renderstate = m_state;
	m_cmdIndex = 0;
	m_lastNewLine = false;
	m_lastNewPage = false;

	const float dotPitch = CalcDotPitchX();
	m_colorbuf_w = (int)ceil(widthPixel / dotPitch);
	m_colorbuf_h = 24;
	m_colorbuf = new UINT8[m_colorbuf_w * m_colorbuf_h];
	memset(m_colorbuf, 0xff, m_colorbuf_w * m_colorbuf_h);

	// GDIオブジェクト用意
	memset(&m_gdiobj, 0, sizeof(m_gdiobj));

	const int fontPx = MulDiv(10.8, m_dpiY, 72); // 10.8pt
	LOGFONT lf = { 0 };
	lf.lfHeight = -fontPx;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	if (np2oscfg.prnfontM && _tcslen(np2oscfg.prnfontM) > 0) {
		lstrcpy(lf.lfFaceName, np2oscfg.prnfontM);
	}
	else {
		lstrcpy(lf.lfFaceName, _T("MS Mincho"));
	}
	m_gdiobj.fontbase = CreateFontIndirect(&lf);

	lf.lfWeight = FW_BOLD;
	m_gdiobj.fontbold = CreateFontIndirect(&lf);

	lstrcpy(lf.lfFaceName, _T("@"));
	if (np2oscfg.prnfontM && _tcslen(np2oscfg.prnfontM) > 0) {
		lstrcat(lf.lfFaceName, np2oscfg.prnfontM);
	}
	else {
		lstrcat(lf.lfFaceName, _T("MS Mincho"));
	}
	lf.lfWeight = FW_NORMAL;
	m_gdiobj.fontrot90 = CreateFontIndirect(&lf);

	lf.lfWeight = FW_BOLD;
	m_gdiobj.fontboldrot90 = CreateFontIndirect(&lf);

	m_gdiobj.oldfont = nullptr;
	if (m_gdiobj.fontbase) m_gdiobj.oldfont = (HFONT)SelectObject(m_hdc, m_gdiobj.fontbase);

	UpdateFont();

	UpdateLinePen();

	SetTextColor(m_hdc, ColorCodeToColorRef(m_state.color));

	m_gdiobj.brsDot[0] = (HBRUSH)GetStockObject(BLACK_BRUSH);
	for (int i = 1; i < _countof(m_gdiobj.brsDot); i++) {
		if (!m_gdiobj.brsDot[i]) {
			m_gdiobj.brsDot[i] = CreateSolidBrush(ColorCodeToColorRef(i));
		}
	}

	SetGraphicsMode(m_hdc, GM_ADVANCED);

	SetBkMode(m_hdc, TRANSPARENT);
}

void CPrintPR201::EndPrint()
{
	// 描画残り分を出力する
	std::vector<PRINTCMD_DATA>& cmdList = m_parser->GetParsedCommandList();
	Render(cmdList.size()); // データを描画する
	RenderGraphic(); // グラフィックも描画する
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
	if (m_gdiobj.oldfont) SelectObject(m_hdc, m_gdiobj.oldfont);
	if (m_gdiobj.fontbase) DeleteObject(m_gdiobj.fontbase);
	if (m_gdiobj.fontrot90) DeleteObject(m_gdiobj.fontrot90);
	if (m_gdiobj.fontbold) DeleteObject(m_gdiobj.fontbold);
	if (m_gdiobj.fontboldrot90) DeleteObject(m_gdiobj.fontboldrot90);
	if (m_gdiobj.penline) DeleteObject(m_gdiobj.penline);
	m_gdiobj.fontbase = NULL;
	m_gdiobj.fontrot90 = NULL;
	m_gdiobj.fontbold = NULL;
	m_gdiobj.fontboldrot90 = NULL;
	m_gdiobj.penline = NULL;
	m_gdiobj.brsDot[0] = NULL;
	for (int i = 1; i < _countof(m_gdiobj.brsDot); i++) {
		if (m_gdiobj.brsDot[i]) {
			DeleteObject(m_gdiobj.brsDot[i]);
			m_gdiobj.brsDot[i] = NULL;
		}
	}
}

bool CPrintPR201::Write(UINT8 data)
{
	return m_parser->PushByte(data);
}

void CPrintPR201::RenderGraphic()
{
	if (m_state.hasGraphic) {
		float pitchx = CalcDotPitchX();
		float pitchy = CalcDotPitchY();
		if (m_state.copymode) {
			//pitchy *= (float)24 / 16 * 120 / 160 * 160 / 24 / 6; // なぞのほせい
		}
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

void CPrintPR201::Render(int count)
{
	float lastPosX = m_state.posX;
	float lastPosY = m_state.posY;
	bool completeLine = false;

	// m_stateからm_renderstateへ事前計算データを代入
	m_renderstate.actualLineHeight = m_state.actualLineHeight;
	m_renderstate.charBaseLineOffset = m_state.charBaseLineOffset;
	if (m_renderstate.charBaseLineOffset > 0) {
		m_renderstate.charBaseLineOffset = m_renderstate.charBaseLineOffset;
	}

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
			cmdfunc = pmpr201_PutChar;
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

	// 行高さをクリア
	m_state.actualLineHeight = 0;

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

bool CPrintPR201::CheckOverflowLine(float addCharWidth)
{
	// return m_state.posX + addCharWidth > m_widthPixel;
	return m_state.posX + addCharWidth > (m_state.rightMargin - m_state.leftMargin) * m_dpiX;
}
bool CPrintPR201::CheckOverflowPage(float addLineHeight)
{
	return m_state.posY + addLineHeight > m_heightPixel;
}

PRINT_COMMAND_RESULT CPrintPR201::DoCommand()
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
			cmdfunc = pmpr201_PutChar;
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
				cmdList.erase(cmdList.begin(), cmdList.begin() + m_cmdIndex);
				m_cmdIndex = 0;
				m_lastNewLine = false;
				m_state.hasPrintDataInPage = false;
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

bool CPrintPR201::HasRenderingCommand()
{
	std::vector<PRINTCMD_DATA>& cmdList = m_parser->GetParsedCommandList();

	int cmdListLen = cmdList.size();
	if (cmdListLen <= m_cmdIndex) return false;

	for (int i = m_cmdIndex; i < cmdListLen; i++) {
		if ((PFNPRINTCMD_COMMANDFUNC)cmdList[m_cmdIndex].cmd) {
			PFNPRINTCMD_COMMANDFUNC cmdfunc = (PFNPRINTCMD_COMMANDFUNC)cmdList[m_cmdIndex].cmd->userdata;
			if (cmdfunc == pmpr201_CommandESCGraph) {
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

void CPrintPR201::UpdateFont()
{
	if (m_state.mode == PRINT_PR201_PRINTMODE_t) {
		if (m_gdiobj.fontrot90 && m_gdiobj.fontboldrot90) SelectObject(m_hdc, m_state.bold ? m_gdiobj.fontboldrot90 : m_gdiobj.fontrot90);
	}
	else {
		if (m_gdiobj.fontbase && m_gdiobj.fontbold) SelectObject(m_hdc, m_state.bold ? m_gdiobj.fontbold : m_gdiobj.fontbase);
	}
}

void CPrintPR201::UpdateLinePen()
{
	if (m_gdiobj.penline == NULL || m_state.linep1 != m_gdiobj.lastlinep1 || m_state.linep2 != m_gdiobj.lastlinep2 || m_state.linep3 != m_gdiobj.lastlinep3 || m_state.linecolor != m_gdiobj.lastlinecolor) {
		if (m_gdiobj.penline) {
			DeleteObject(m_gdiobj.penline);
			m_gdiobj.penline = NULL;
		}
		const float dotPitch = 1.0f / 160;
		int dotsize = (float)m_dpiX * dotPitch;
		dotsize *= m_state.linep3 / 2;
		if (dotsize <= 0) dotsize = 1;
		m_gdiobj.penline = CreatePen(PS_SOLID, dotsize, ColorCodeToColorRef(m_state.color));

		m_gdiobj.lastlinep1 = m_state.linep1;
		m_gdiobj.lastlinep2 = m_state.linep2;
		m_gdiobj.lastlinep3 = m_state.linep3;
		m_gdiobj.lastlinecolor = m_state.color;
	}
}

#endif /* SUPPORT_PRINT_PR201 */
