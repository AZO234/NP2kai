/**
 * @file	cmserial.h
 * @brief	シリアル クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "cmbase.h"

#define SERIAL_OVERLAP_COUNT	4
#define SERIAL_BLOCKBUFFER_SIZE_MAX	128

/**
 * 速度テーブル
 */
const UINT32 cmserial_speed[] = {110, 300, 600, 1200, 2400, 4800,
							9600, 14400, 19200, 28800, 38400, 57600, 115200};

/**
 * @brief commng シリアル デバイス クラス
 */
class CComSerial : public CComBase
{
public:
	static CComSerial* CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck);

protected:
	CComSerial();
	virtual ~CComSerial();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT WriteRetry();
	virtual UINT LastWriteSuccess(); // 最後の書き込みが成功しているかチェック
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);
	virtual void BeginBlockTransfer();
	virtual void EndBlockTransfer();

private:
	HANDLE m_hSerial;		/*!< シリアル ハンドル */
	
	OVERLAPPED m_writeovl[SERIAL_OVERLAP_COUNT];	/*!< 書き込みOVERLAPPED */
	OVERLAPPED m_readovl;	/*!< 読み込みOVERLAPPED */
	bool m_writeovl_pending[SERIAL_OVERLAP_COUNT];	/*!< 書き込みOVERLAPPED待機中 */
	bool m_readovl_pending;	/*!< 読み込みOVERLAPPED待機中 */
	UINT8 m_readovl_buf;	/*!< 読み込みOVERLAPPEDバッファ */
	
	bool m_blocktransfer;			/*!< ブロック単位書き込みモード */
	UINT8 m_blockbuffer[SERIAL_BLOCKBUFFER_SIZE_MAX];		/*!< ブロックバッファ */
	int m_blockbuffer_pos;		/*!< ブロックバッファ書き込み位置 */
	int m_blockbuffer_size;		/*!< ブロックバッファサイズ */

	bool m_fixedspeed;	/*!< 通信速度固定 */
	UINT8 m_lastdata; // 最後に送ろうとしたデータ
	UINT8 m_lastdatafail; // データを送るのに失敗していたら0以外
	DWORD m_lastdatatime; // データを送るのに失敗した時間（あまりにも失敗し続けるようなら無視する）

	UINT8 m_errorstat; // エラー状態 (bit0: パリティ, bit1: オーバーラン, bit2: フレーミング)

	bool Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck);
	void CheckCommError(DWORD errorcode);
};
