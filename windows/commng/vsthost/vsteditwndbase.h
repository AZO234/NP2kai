/**
 * @file	vsteditwndbase.h
 * @brief	VST edit ウィンドウ基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * @brief VST edit ウィンドウ基底クラス
 */
class IVstEditWnd
{
public:
	virtual bool OnResize(int nWidth, int nHeight) = 0;
	virtual bool OnUpdateDisplay() = 0;

};
