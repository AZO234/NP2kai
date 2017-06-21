/**
 * @file	mt32snd.h
 * @brief	MT32Sound アクセス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#if defined(MT32SOUND_DLL)

/**
 * @brief MT32Sound アクセス クラス
 */
class MT32Sound
{
public:
	static MT32Sound* GetInstance();

	MT32Sound();
	~MT32Sound();
	bool Initialize();
	void Deinitialize();
	bool IsEnabled() const;
	void SetRate(UINT nRate);

	bool Open();
	void Close();
	void ShortMsg(UINT32 msg);
	void LongMsg(const UINT8* lpBuffer, UINT cchBuffer);
	UINT Mix(SINT32* lpBuffer, UINT cchBuffer);

private:
	static MT32Sound sm_instance;	//!< 唯一のインスタンスです

	//! @brief ロード関数
	struct ProcItem
	{
		LPCSTR lpSymbol;			//!< 関数名
		size_t nOffset;				//!< オフセット
	};

	// 定義
	typedef int (*FnOpen)(int rate, int reverb, int def, int revtype, int revtime, int revlvl); 	/*!< オープン */
	typedef int (*FnClose)(void);																	/*!< クローズ */
	typedef int (*FnWrite)(unsigned char data);														/*!< ライト */
	typedef int (*FnMix)(void *buff, unsigned long size);											/*!< ミックス */

	HMODULE m_hModule;	/*!< モジュール */
	bool m_bOpened;		/*!< オープン フラグ */
	UINT m_nRate;		/*!< サンプリング レート */
	FnOpen m_fnOpen;	/*!< オープン関数 */
	FnClose m_fnClose;	/*!< クローズ関数 */
	FnWrite m_fnWrite;	/*!< ライト関数 */
	FnMix m_fnMix;		/*!< ミックス関数 */
};

/**
 * インスタンスを取得
 * @return インスタンス
 */
inline MT32Sound* MT32Sound::GetInstance()
{
	return &sm_instance;
}

/**
 * 有効?
 * @retval true 有効
 * @retval false 無効
 */
inline bool MT32Sound::IsEnabled() const
{
	return (m_hModule != NULL);
}

/**
 * サンプリング レートを設定
 * @param[in] nRate レート
 */
inline void MT32Sound::SetRate(UINT nRate)
{
	m_nRate = nRate;
}

#endif
