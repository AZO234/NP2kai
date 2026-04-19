/**
 * @file	pmbase.h
 * @brief	印刷基底クラスの宣言およびインターフェイスの定義をします
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

#pragma once

typedef enum {
	PRINT_COMMAND_RESULT_OK = 0, // 処理できる分を全て処理した
	PRINT_COMMAND_RESULT_COMPLETEPAGE = 2, // 改ページ必要
} PRINT_COMMAND_RESULT;

/**
 * @brief 印刷基底クラス
 */
class CPrintBase
{
public:
	CPrintBase();
	virtual ~CPrintBase();

	/// <summary>
	/// 印刷開始時に呼ばれる
	/// </summary>
	/// <param name="hdc">印刷描画先のHDC</param>
	/// <param name="offsetXPixel">描画先のオフセットX座標 (pixel)</param>
	/// <param name="offsetYPixel">描画先のオフセットY座標 (pixel)</param>
	/// <param name="widthPixel">描画先の幅 (pixel)</param>
	/// <param name="heightPixel">描画先の高さ (pixel)</param>
	/// <param name="dpiX">描画先のDPI X方向</param>
	/// <param name="dpiY">描画先のDPI Y方向</param>
	/// <param name="dotscale">点描画サイズのスケール</param>
	/// <param name="rectdot">矩形描ドット描画指示</param>
	virtual void StartPrint(HDC hdc, int offsetXPixel, int offsetYPixel, int widthPixel, int heightPixel, float dpiX, float dpiY, float dotscale, bool rectdot);

	/// <summary>
	/// 印刷終了時に呼ばれる
	/// </summary>
	virtual void EndPrint();

	/// <summary>
	/// 印刷データ書き込み時に呼ばれる　
	/// </summary>
	/// <param name="data">書き込むデータ</param>
	/// <returns>コマンドが新たに1つ解釈されたらtrueを返す</returns>
	virtual bool Write(UINT8 data);

	/// <summary>
	/// コマンド実行指示
	/// </summary>
	/// <returns>コマンド実行結果</returns>
	virtual PRINT_COMMAND_RESULT DoCommand();

	/// <summary>
	/// 待機中コマンドに描画コマンドがあるか確認する。実際の改ページ処理に使う
	/// </summary>
	/// <returns>描画コマンドがあればtrue</returns>
	virtual bool HasRenderingCommand();

	HDC m_hdc;
	int m_offsetXPixel;
	int m_offsetYPixel;
	int m_widthPixel;
	int m_heightPixel;
	float m_dpiX;
	float m_dpiY;
	float m_dotscale;
	bool m_rectdot;
};
