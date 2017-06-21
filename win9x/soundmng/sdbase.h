/**
 * @file	sdbase.h
 * @brief	サウンド デバイス基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * @brief サウンド データ取得インタフェイス
 */
class ISoundData
{
public:
	/**
	 * ストリーム データを得る
	 * @param[out] lpBuffer バッファ
	 * @param[in] nBufferCount バッファ カウント
	 * @return サンプル数
	 */
	virtual UINT Get16(SINT16* lpBuffer, UINT nBufferCount) = 0;
};

/**
 * @brief サウンド デバイス基底クラス
 */
class CSoundDeviceBase
{
public:
	/**
	 * コンストラクタ
	 */
	CSoundDeviceBase()
		: m_pSoundData(NULL)
	{
	}

	/**
	 * デストラクタ
	 */
	virtual ~CSoundDeviceBase()
	{
	}

	/**
	 * ストリーム データの設定
	 * @param[in] pSoundData サウンド データ
	 */
	void SetStreamData(ISoundData* pSoundData)
	{
		m_pSoundData = pSoundData;
	}

	/**
	 * オープン
	 * @param[in] lpDevice デバイス名
	 * @param[in] hWnd ウィンドウ ハンドル
	 * @retval true 成功
	 * @retval false 失敗
	 */
	virtual bool Open(LPCTSTR lpDevice = NULL, HWND hWnd = NULL) = 0;

	/**
	 * クローズ
	 */
	virtual void Close() = 0;

	/**
	 * ストリームの作成
	 * @param[in] nSamplingRate サンプリング レート
	 * @param[in] nChannels チャネル数
	 * @param[in] nBufferSize バッファ サイズ
	 * @return バッファ サイズ
	 */
	virtual UINT CreateStream(UINT nSamplingRate, UINT nChannels, UINT nBufferSize = 0) = 0;

	/**
	 * ストリームを破棄
	 */
	virtual void DestroyStream() = 0;

	/**
	 * ストリームをリセット
	 */
	virtual void ResetStream()
	{
	}

	/**
	 * ストリームの再生
	 * @retval true 成功
	 * @retval false 失敗
	 */
	virtual bool PlayStream() = 0;

	/**
	 * ストリームの停止
	 */
	virtual void StopStream() = 0;

	/**
	 * PCM データ読み込み
	 * @param[in] nNum PCM 番号
	 * @param[in] lpFilename ファイル名
	 * @retval true 成功
	 * @retval false 失敗
	 */
	virtual bool LoadPCM(UINT nNum, LPCTSTR lpFilename)
	{
		return false;
	}

	/**
	 * PCM をアンロード
	 * @param[in] nNum PCM 番号
	 */
	virtual void UnloadPCM(UINT nNum)
	{
	}

	/**
	 * PCM ヴォリューム設定
	 * @param[in] nNum PCM 番号
	 * @param[in] nVolume ヴォリューム
	 */
	virtual void SetPCMVolume(UINT nNum, int nVolume)
	{
	}

	/**
	 * PCM 再生
	 * @param[in] nNum PCM 番号
	 * @param[in] bLoop ループ フラグ
	 * @retval true 成功
	 * @retval false 失敗
	 */
	virtual bool PlayPCM(UINT nNum, BOOL bLoop)
	{
		return false;
	}

	/**
	 * PCM 停止
	 * @param[in] nNum PCM 番号
	 */
	virtual void StopPCM(UINT nNum)
	{
	}

	/**
	 * PCM をストップ
	 */
	virtual void StopAllPCM()
	{
	}

protected:
	ISoundData* m_pSoundData;		/*!< サウンド データ インスタンス */
};
