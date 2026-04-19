/**
 * @file	c_dipsw.h
 * @brief	DIPSW コントロール クラス群の宣言およびインターフェイスの定義をします
 */

#pragma once

#include "misc\WndProc.h"

/**
 * @brief MIDI デバイス クラス
 */
class CStaticDipSw : public CWndProc
{
public:
	virtual void PreSubclassWindow();
	static void Draw(HDC hdc, const void* lpBitmap);
};
