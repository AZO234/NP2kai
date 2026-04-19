/**
 * @file	c_midi.h
 * @brief	MIDI コントロール クラス群の宣言およびインターフェイスの定義をします
 */

#pragma once

#include "misc\DlgProc.h"

/**
 * @brief MIDI デバイス クラス
 */
class CComboMidiDevice : public CComboBoxProc
{
public:
	virtual void PreSubclassWindow();
	void EnumerateMidiIn();
	void EnumerateMidiOut();
	void SetCurString(LPCTSTR lpDevice);
};

/**
 * @brief MIDI モジュール クラス
 */
class CComboMidiModule : public CComboBoxProc
{
public:
	virtual void PreSubclassWindow();
};

/**
 * @brief MIMPI ファイル
 */
class CEditMimpiFile : public CWndProc
{
public:
	void Browse();
};
