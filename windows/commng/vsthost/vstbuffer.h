/**
 * @file	vstbuffer.h
 * @brief	VST バッファ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * @brief VST バッファ クラス
 */
class CVstBuffer
{
public:
	CVstBuffer();
	CVstBuffer(UINT nChannels, UINT nSamples);
	~CVstBuffer();
	void Alloc(UINT nChannels, UINT nSamples);
	void Delloc();
	void ZeroFill();
	float** GetBuffer();
	void GetShort(short* lpBuffer) const;

private:
	UINT m_nChannels;		/*!< チャンネル数 */
	UINT m_nSamples;		/*!< サンプル数 */
	float** m_pBuffers;		/*!< バッファ */
};

/**
 * バッファを得る
 * @return バッファ
 */
inline float** CVstBuffer::GetBuffer()
{
	return m_pBuffers;
}
