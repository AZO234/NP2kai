/**
 * @file	pmbase.cpp
 * @brief	印刷基底クラスの動作の実装を行います
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
#include "pmbase.h"

/**
 * コンストラクタ
 */
CPrintBase::CPrintBase()
{
	// nothing to do
}

/**
 * デストラクタ
 */
CPrintBase::~CPrintBase()
{
	// nothing to do
}

void CPrintBase::StartPrint(HDC hdc, int offsetXPixel, int offsetYPixel, int widthPixel, int heightPixel, float dpiX, float dpiY, float dotscale, bool rectdot)
{
	m_hdc = hdc;
	m_offsetXPixel = offsetXPixel;
	m_offsetYPixel = offsetYPixel;
	m_widthPixel = widthPixel;
	m_heightPixel = heightPixel;
	m_dpiX = dpiX;
	m_dpiY = dpiY;
	m_dotscale = dotscale;
	m_rectdot = rectdot;
}

void CPrintBase::EndPrint()
{
	// nothing to do
}

bool CPrintBase::Write(UINT8 data)
{
	// nothing to do
	return false;
}

PRINT_COMMAND_RESULT CPrintBase::DoCommand()
{
	// nothing to do
	return PRINT_COMMAND_RESULT_OK;
}

bool CPrintBase::HasRenderingCommand()
{
	// nothing to do
	return false;
}
