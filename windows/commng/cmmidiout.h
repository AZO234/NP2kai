/**
 * @file	cmmidiout.h
 * @brief	MIDI OUT 基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * @brief MIDI OUT 基底クラス
 */
class CComMidiOut
{
public:
	/**
	 * デストラクタ
	 */
	virtual ~CComMidiOut()
	{
	}

	/**
	 * ショート メッセージ
	 * @param[in] nMessage メッセージ
	 */
	virtual void Short(UINT32 nMessage)
	{
	}

	/**
	 * ロング メッセージ
	 * @param[in] lpMessage メッセージ ポインタ
	 * @param[in] cbMessage メッセージ サイズ
	 */
	virtual void Long(const UINT8* lpMessage, UINT cbMessage)
	{
	}
};
