/**
 * @file	cmdparser.h
 * @brief	エスケープシーケンスなどのコマンドをパースします
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

#include <vector>

// コマンド定義用マクロ　将来コマンド長さ上限が増える場合はPRINTCMD_DEFINE構造体内のchar cmdの長さも増やすこと
// 
// （原則）
// コマンドは「コマンド文字列」＋「パラメータ」で構成されている
// コマンド文字列は他と被らない一意なもので、パラメータの長さはコマンドによって決定する
// 定義されたコマンドに該当しないものは未定義コマンドコールバックに渡される。
// 必要に応じて、コールバックの中でコマンドを独自にパースすることができる。
// （特殊な状況）
// ・コマンドの後のパラメータが「パラメータ長さ（ASCII数値n桁）」＋「パラメータ」の場合は、PRINTCMD_DEFINE_LENFIELDを使用する
// ・コマンドの後のパラメータが特定の文字で終端するようになっている場合は、PRINTCMD_DEFINE_TERMINATORを使用する
// ・コマンドの後のパラメータ長さがパラメータの内容に依存した可変長の場合、PRINTCMD_DEFINE_FIXEDLEN_Vを用いてコールバックで長さを返す
// 

// コマンド文字列リテラル
#define PRINTCMD_DEFINE_FIXEDLEN(cmdstr, datalength, proc)				{cmdstr, sizeof(cmdstr)-1, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, false}
#define PRINTCMD_DEFINE_LENFIELD(cmdstr, fieldlen, elementsize, proc)	{cmdstr, sizeof(cmdstr)-1, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, false}
#define PRINTCMD_DEFINE_TERMINATOR(cmdstr, terminator_char, proc)		{cmdstr, sizeof(cmdstr)-1, PRINTCMD_TYPE_TERMINATOR, 0, 0, 0, terminator_char, proc, false}
#define PRINTCMD_DEFINE_FIXEDLEN_V(cmdstr, datalength, proc)			{cmdstr, sizeof(cmdstr)-1, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, true}
#define PRINTCMD_DEFINE_LENFIELD_V(cmdstr, fieldlen, elementsize, proc)	{cmdstr, sizeof(cmdstr)-1, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, true}

// コマンド長さ1byte
#define PRINTCMD1_DEFINE_FIXEDLEN(cmd, datalength, proc)				{{cmd, '\0'}, 1, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, false}
#define PRINTCMD1_DEFINE_LENFIELD(cmd, fieldlen, elementsize, proc)		{{cmd, '\0'}, 1, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, false}
#define PRINTCMD1_DEFINE_TERMINATOR(cmd, terminator_char, proc)			{{cmd, '\0'}, 1, PRINTCMD_TYPE_TERMINATOR, 0, 0, 0, terminator_char, proc, false}
#define PRINTCMD1_DEFINE_FIXEDLEN_V(cmd, datalength, proc)				{{cmd, '\0'}, 1, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, true}
#define PRINTCMD1_DEFINE_LENFIELD_V(cmd, fieldlen, elementsize, proc)	{{cmd, '\0'}, 1, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, true}

// コマンド長さ2byte
#define PRINTCMD2_DEFINE_FIXEDLEN(cmd1, cmd2, datalength, proc)					{{cmd1, cmd2, '\0'}, 2, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, false}
#define PRINTCMD2_DEFINE_LENFIELD(cmd1, cmd2, fieldlen, elementsize, proc)		{{cmd1, cmd2, '\0'}, 2, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, false}
#define PRINTCMD2_DEFINE_TERMINATOR(cmd1, cmd2, terminator_char, proc)			{{cmd1, cmd2, '\0'}, 2, PRINTCMD_TYPE_TERMINATOR, 0, 0, 0, terminator_char, proc, false}
#define PRINTCMD2_DEFINE_FIXEDLEN_V(cmd1, cmd2, datalength, proc)				{{cmd1, cmd2, '\0'}, 2, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, true}
#define PRINTCMD2_DEFINE_LENFIELD_V(cmd1, cmd2, fieldlen, elementsize, proc)	{{cmd1, cmd2, '\0'}, 2, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, true}

// コマンド長さ3byte
#define PRINTCMD3_DEFINE_FIXEDLEN(cmd1, cmd2, cmd3, datalength, proc)				{{cmd1, cmd2, cmd3, '\0'}, 3, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, false}
#define PRINTCMD3_DEFINE_LENFIELD(cmd1, cmd2, cmd3, fieldlen, elementsize, proc)	{{cmd1, cmd2, cmd3, '\0'}, 3, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, false}
#define PRINTCMD3_DEFINE_TERMINATOR(cmd1, cmd2, cmd3, terminator_char, proc)		{{cmd1, cmd2, cmd3, '\0'}, 3, PRINTCMD_TYPE_TERMINATOR, 0, 0, 0, terminator_char, proc, false}
#define PRINTCMD3_DEFINE_FIXEDLEN_V(cmd1, cmd2, cmd3, datalength, proc)				{{cmd1, cmd2, cmd3, '\0'}, 3, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, true}
#define PRINTCMD3_DEFINE_LENFIELD_V(cmd1, cmd2, cmd3, fieldlen, elementsize, proc)	{{cmd1, cmd2, cmd3, '\0'}, 3, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, true}

// コマンド長さ4byte
#define PRINTCMD4_DEFINE_FIXEDLEN(cmd1, cmd2, cmd3, cmd4, datalength, proc)					{{cmd1, cmd2, cmd3, cmd4, '\0'}, 4, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, false}
#define PRINTCMD4_DEFINE_LENFIELD(cmd1, cmd2, cmd3, cmd4, fieldlen, elementsize, proc)		{{cmd1, cmd2, cmd3, cmd4, '\0'}, 4, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, false}
#define PRINTCMD4_DEFINE_TERMINATOR(cmd1, cmd2, cmd3, cmd4, terminator_char, proc)			{{cmd1, cmd2, cmd3, cmd4, '\0'}, 4, PRINTCMD_TYPE_TERMINATOR, 0, 0, 0, terminator_char, proc, false}
#define PRINTCMD4_DEFINE_FIXEDLEN_V(cmd1, cmd2, cmd3, cmd4, datalength, proc)				{{cmd1, cmd2, cmd3, cmd4, '\0'}, 4, PRINTCMD_TYPE_FIXEDLEN, datalength, 0, 0, 0, proc, true}
#define PRINTCMD4_DEFINE_LENFIELD_V(cmd1, cmd2, cmd3, cmd4, fieldlen, elementsize, proc)	{{cmd1, cmd2, cmd3, cmd4, '\0'}, 4, PRINTCMD_TYPE_LENFIELD, 0, fieldlen, elementsize, 0, proc, true}

typedef enum {
	PRINTCMD_TYPE_FIXEDLEN = 0,
	PRINTCMD_TYPE_LENFIELD = 1,
	PRINTCMD_TYPE_TERMINATOR = 2,
} PRINTCMD_TYPE;

typedef enum {
	PRINTCMD_CALLBACK_RESULT_CONTINUE = 0, // コマンドに続きあり
	PRINTCMD_CALLBACK_RESULT_COMPLETE = 1, // コマンドが完了した
	PRINTCMD_CALLBACK_RESULT_INVALID = 2, // コマンドが無効
	PRINTCMD_CALLBACK_RESULT_CANCEL = 3 // コマンド継続を取り消し、改めて現在のデータからコマンドを解釈する
} PRINTCMD_CALLBACK_RESULT;

typedef struct {
	UINT8 cmd[5]; // コマンド　複数バイトある場合は先に来るものから順に書く　null終端
	int cmdlen;
	PRINTCMD_TYPE type;

	// 固定長　
	UINT32 fixedlen_datalength; // コマンドを除いたデータ長さ

	// 可変長
	UINT32 lenfield_length; // 要素数を表す文字列部分の長さ
	UINT32 lenfield_elementsize; // 要素1つあたりのサイズ

	// 終端文字
	UINT8 terminator_char; // 終端文字

	// 任意データ
	void *userdata;

	// ステータスによって長さが変わる
	bool varlength;
} PRINTCMD_DEFINE;

typedef struct {
	PRINTCMD_DEFINE *cmd;
	std::vector<UINT8> data;
} PRINTCMD_DATA;

/**
 * @brief PrinterCommandParser
 */
class PrinterCommandParser
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="commandTable">コマンドの定義テーブル</param>
	/// <param name="commandTableLength">コマンドの定義テーブルの長さ</param>
	/// <param name="unknownCommandCallback">定義外のコマンドが来たときの処理関数（引数は現在のコマンドデータ全体）　nullの場合は常時無効コマンド(PRINTCMD_CALLBACK_RESULT_INVALID)扱い</param>
	/// <param name="variableLengthCallback">状態によって長さが変わるコマンドの追加長さを返す関数　nullの場合は追加無し（=元の長さ）扱い 負の値にはできない 　curBufferはコマンド部分を含めたデータ全体が取得できる</param>
	/// <param name="callbackParam">定義外のコマンドが来たときの処理関数に渡す任意のポインタ</param>
	PrinterCommandParser(const PRINTCMD_DEFINE *commandTable, int commandTableLength, PRINTCMD_CALLBACK_RESULT (*unknownCommandCallback)(void* param, const std::vector<UINT8>& curBuffer), UINT32 (*variableLengthCallback)(void* param, const PRINTCMD_DEFINE& cmddef, const std::vector<UINT8>& curBuffer), void* callbackParam);
	/// <summary>
	/// デストラクタ
	/// </summary>
	~PrinterCommandParser();

	/// <summary>
	/// データをPushします
	/// </summary>
	/// <param name="data">新しいデータ</param>
	/// <returns>コマンドが完全に解釈されてリストに追加されたらtrueを返す</returns>
	bool PushByte(UINT8 data);

	/// <summary>
	/// パースされたコマンドのリストを取得します。リセットする場合はclearを呼んでOK。
	/// </summary>
	/// <returns>パースされたコマンドのリスト</returns>
	std::vector<PRINTCMD_DATA>& GetParsedCommandList();
	
private:
	PrinterCommandParser(const PrinterCommandParser&); // コピーコンストラクタ禁止
	PrinterCommandParser& operator=(const PrinterCommandParser&); // コピー代入禁止

	/// <summary>
	/// コマンドリストに現在のバッファを追加
	/// </summary>
	void AddToCommandList();

    std::vector<UINT8> m_buffer;
    std::vector<PRINTCMD_DEFINE> m_commandTable;
    std::vector<PRINTCMD_DATA> m_parsedCommandList;
	PRINTCMD_CALLBACK_RESULT (*m_unknownCommandCallback)(void* param, const std::vector<UINT8>& curBuffer); 
	UINT32(*m_variableLengthCallback)(void* param, const PRINTCMD_DEFINE& cmddef, const std::vector<UINT8>& curBuffer);
	void* m_callbackParam;

	PRINTCMD_DEFINE *m_cmddef;
	bool m_dataRemainValid;
	int m_dataRemain;
	bool m_terminatorValid;
	UINT8 m_terminator;
	int m_cmdlen;
	bool m_addLengthChecked;
};
