/**
 * @file	cmnull.h
 * @brief	commng NULL デバイス クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmnull.h"

/**
 * コンストラクタ
 */
CComNull::CComNull()
	: CComBase(COMCONNECT_OFF)
{
}

/**
 * Read
 * @param[out] pData
 * @return result
 */
UINT CComNull::Read(UINT8* pData)
{
	return 0;
}

/**
 * Write
 * @param[in] cData
 * @return result
 */
UINT CComNull::Write(UINT8 cData)
{
	return 0;
}

/**
 * ステータスを得る
 * @return ステータス
 */
UINT8 CComNull::GetStat()
{
	return 0xf0;
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComNull::Message(UINT nMessage, INTPTR nParam)
{
	return 0;
}
