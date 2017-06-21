/**
 * @file	dclock.h
 * @brief	時刻表示クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

enum
{
	DCLOCK_WIDTH	= 56,
	DCLOCK_HEIGHT	= 12,
	DCLOCK_YALIGN	= (56 / 8)
};

struct DispClockPattern;

/**
 * @brief 時刻表示クラス
 */
class DispClock
{
public:
	static DispClock* GetInstance();

	DispClock();
	void Initialize();
	void SetPalettes(UINT bpp);
	const RGB32* GetPalettes() const;
	void Reset();
	void Update();
	void Redraw();
	bool IsDisplayed() const;
	void CountDown(UINT nFrames);
	bool Make();
	void Draw(UINT nBpp, void* lpBuffer, int nYAlign) const;

private:
	static DispClock sm_instance;		//!< 唯一のインスタンスです

	/**
	 * @brief QuadBytes
	 */
	union QuadBytes
	{
		UINT8 b[8];			//!< bytes
		UINT64 q;			//!< quad
	};

	const DispClockPattern* m_pPattern;	//!< パターン
	QuadBytes m_nCounter;				//!< カウンタ
	UINT8 m_cTime[8];					//!< 現在時間
	UINT8 m_cLastTime[8];				//!< 最後の時間
	UINT8 m_cDirty;						//!< 描画フラグ drawing;
	UINT8 m_cCharaters;					//!< 文字数
	RGB32 m_pal32[4];					//!< パレット
	RGB16 m_pal16[4];					//!< パレット
	UINT32 m_pal8[4][16];				//!< パレット パターン
	UINT8 m_buffer[(DCLOCK_HEIGHT * DCLOCK_YALIGN) + 4];	/*!< バッファ */

private:
	void SetPalette8();
	void SetPalette16();
	static UINT8 CountPos(UINT nCount);
	void Draw8(void* lpBuffer, int nYAlign) const;
	void Draw16(void* lpBuffer, int nYAlign) const;
	void Draw24(void* lpBuffer, int nYAlign) const;
	void Draw32(void* lpBuffer, int nYAlign) const;
};

/**
 * インスタンスを得る
 * @return インスタンス
 */
inline DispClock* DispClock::GetInstance()
{
	return &sm_instance;
}

/**
 * パレットを得る
 * @return パレット
 */
inline const RGB32* DispClock::GetPalettes() const
{
	return m_pal32;
}
