/**
 * @file	cmdparser.cpp
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

#include "compiler.h"
#include "cmdparser.h"

PrinterCommandParser::PrinterCommandParser(const PRINTCMD_DEFINE *commandTable, int commandTableLength, PRINTCMD_CALLBACK_RESULT (*unknownCommandCallback)(void* param, const std::vector<UINT8>& curBuffer), UINT32 (*variableLengthCallback)(void* param, const PRINTCMD_DEFINE& cmddef, const std::vector<UINT8>& curBuffer), void* callbackParam)
	: m_buffer()
	, m_commandTable()
	, m_parsedCommandList()
	, m_unknownCommandCallback(unknownCommandCallback)
	, m_variableLengthCallback(variableLengthCallback)
	, m_callbackParam(callbackParam)
	, m_cmdlen(0)
	, m_dataRemainValid(false)
	, m_dataRemain(0)
	, m_terminatorValid(false)
	, m_terminator('\0')
{
	m_commandTable.insert(m_commandTable.end(), commandTable, commandTable + commandTableLength);
}

PrinterCommandParser::~PrinterCommandParser()
{
}

void PrinterCommandParser::AddToCommandList()
{
	PRINTCMD_DATA d = { 0 };
	std::vector<UINT8> sub(m_buffer.begin() + m_cmdlen, m_buffer.end());
	d.cmd = m_cmddef;
	d.data = sub;
	m_parsedCommandList.push_back(d);
	m_buffer.clear();
}

bool PrinterCommandParser::PushByte(UINT8 data)
{
	m_buffer.push_back(data);

	// データ長さが確定している場合はその長さまで読む（再パース省略）
	if (m_dataRemainValid) {
		m_dataRemain--;
		if (m_dataRemain <= 0) {
			if (!m_addLengthChecked){
				// 可変長コールバックは読めるだけ読んでから送る
				if (m_cmddef->varlength && m_variableLengthCallback) {
					m_dataRemain += (*m_variableLengthCallback)(m_callbackParam, *m_cmddef, m_buffer); // 加算
				}
				m_addLengthChecked = true;
			}
			if (m_dataRemain == 0) {
				m_dataRemainValid = false;
				AddToCommandList();
				return true;
			}
		}
		return false;
	}

	// 終端文字がある場合はその文字が来るまで回す（再パース省略）
	if (m_terminatorValid) {
		m_dataRemain--;
		if (data == m_terminator) {
			m_terminatorValid = false;
			AddToCommandList();
			return true;
		}
		return false;
	}

	const int bufLen = m_buffer.size();
	int bufPos = 0;
	UINT8 cmd = m_buffer[0];
	int partialMatchCount = 0;
	int matchMaxLen = 0;
	for (std::vector<PRINTCMD_DEFINE>::iterator it = m_commandTable.begin(); it != m_commandTable.end(); ++it) {
		bufPos = 0;

		// コマンド一致を確認
		bool match = true;
		UINT8* lpCmds = it->cmd;
		int cmdlen = it->cmdlen;
		for (int i = 0; i < cmdlen; i++) {
			if (bufPos == bufLen) {
				// 一致しているが途中で終わっている
				partialMatchCount++;
			}
			if (it->cmd[bufPos] != m_buffer[bufPos]) {
				match = false;
				break;
			}
			bufPos++;
		}
		if (match) {
			if (matchMaxLen < cmdlen) {
				matchMaxLen = cmdlen;
			}
		}
	}
	if (partialMatchCount > 0) {
		// 部分一致の候補があるなら様子見
		return false;
	}
	for (std::vector<PRINTCMD_DEFINE>::iterator it = m_commandTable.begin(); it != m_commandTable.end(); ++it) {
		bufPos = 0;

		// コマンド一致を確認
		bool match = true;
		UINT8 *lpCmds = it->cmd;
		int cmdlen = it->cmdlen; 
		if (cmdlen < matchMaxLen) continue; // 一致長さ最大のものだけ採用
		for (int i = 0; i < cmdlen; i++) {
			if (bufPos == bufLen) return false; // 一致しているが途中で終わっている場合は次ヘ回す
			if (it->cmd[bufPos] != m_buffer[bufPos]) {
				match = false;
				break;
			}
			bufPos++;
		}
		if (!match) continue;

		m_cmddef = &(*it);
		m_cmdlen = cmdlen;
		m_addLengthChecked = false;

		// 読み取り済みデータ長さ
		const int dataLen = bufLen - m_cmdlen;

		// 種類に応じてコマンド完了判定
		switch (it->type) {
		case PRINTCMD_TYPE_FIXEDLEN:
		{
			// 残り読み取り数が確定
			if (dataLen > it->fixedlen_datalength) {
				m_dataRemain = 0; //異常
			}
			else {
				m_dataRemain = it->fixedlen_datalength - dataLen;
			}
			if (m_dataRemain == 0) {
				if (m_cmddef->varlength && m_variableLengthCallback) {
					m_dataRemain += (*m_variableLengthCallback)(m_callbackParam, *m_cmddef, m_buffer); // 加算
					m_addLengthChecked = true;
				}
				if (m_dataRemain == 0) {
					// もう既にOK
					AddToCommandList();
					return true;
				}
				else {
					m_dataRemainValid = true;
					return false;
				}
			}
			else {
				m_dataRemainValid = true;
				return false;
			}
		}
		case PRINTCMD_TYPE_LENFIELD:
		{
			if (dataLen < it->lenfield_length) return false; // まだデータが足りない場合は次ヘ回す

			// 読み取り済みデータ長さ
			int elementCount = 0;
			int i = 0;
			for (int i = 0; i < it->lenfield_length; i++) {
				elementCount *= 10;
				if (m_buffer[bufPos] < '0' || '9' < m_buffer[bufPos]) {
					// 異常データ　バッファをクリア
					m_buffer.clear();
					return false;
				}
				elementCount += (int)m_buffer[bufPos] - '0';
				bufPos++;
			}

			// 残り読み取り数が確定
			if (dataLen - it->lenfield_length > elementCount * it->lenfield_elementsize) {
				m_dataRemain = 0; //異常
			}
			else {
				m_dataRemain = elementCount * it->lenfield_elementsize - (dataLen - it->lenfield_length);
			}
			if (m_dataRemain == 0) {
				if (m_cmddef->varlength && m_variableLengthCallback) {
					m_dataRemain += (*m_variableLengthCallback)(m_callbackParam, *m_cmddef, m_buffer); // 加算
					m_addLengthChecked = true;
				}
				if (m_dataRemain == 0) {
					// もう既にOK
					AddToCommandList();
					return true;
				}
				else {
					m_dataRemainValid = true;
					return false;
				}
			}
			else {
				m_dataRemainValid = true;
				return false;
			}

			return false;
		}
		case PRINTCMD_TYPE_TERMINATOR:
		{
			// 終了文字が定義された
			m_terminatorValid = true;
			m_terminator = it->terminator_char;
			return false;
		}
		default:
		{
			// 異常データ　バッファをクリア
			m_buffer.clear();
			return false;
		}
		}
	}

	// コマンド見つからず
	if (m_unknownCommandCallback) {
		PRINTCMD_CALLBACK_RESULT result = (*m_unknownCommandCallback)(m_callbackParam, m_buffer);
		switch (result) {
		case PRINTCMD_CALLBACK_RESULT_CONTINUE:
			// 継続
			return false;

		case PRINTCMD_CALLBACK_RESULT_COMPLETE:
		{
			// 完了
			PRINTCMD_DATA d = { 0 };
			std::vector<UINT8> sub(m_buffer.begin(), m_buffer.end());
			d.data = sub;
			m_parsedCommandList.push_back(d);
			m_buffer.clear();
			return true;
		}

		case PRINTCMD_CALLBACK_RESULT_INVALID:
			// 無効
			m_buffer.clear();
			return false;

		case PRINTCMD_CALLBACK_RESULT_CANCEL:
			// コマンド継続を取り消し、改めて現在のデータからコマンドを解釈する
			m_buffer.clear();
			return PushByte(data);

		}
	}

	// 異常データ　バッファをクリア
	m_buffer.clear();
	return false;
}

std::vector<PRINTCMD_DATA>& PrinterCommandParser::GetParsedCommandList()
{
	return m_parsedCommandList;
}
