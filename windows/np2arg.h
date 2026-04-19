/**
 *	@file	np2arg.h
 *	@brief	引数情報クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * @brief 引数情報クラス
 */
class Np2Arg
{
public:
	static Np2Arg* GetInstance();
	static void Release();

	Np2Arg();
	~Np2Arg();
	void Parse();
	void ClearDisk();
	LPCTSTR disk(int nDrive) const;
	LPCTSTR cdisk(int nDrive) const;
	LPCTSTR iniFilename() const;
	bool fullscreen() const;
	void setiniFilename(LPTSTR newfile);

private:
	static Np2Arg sm_instance;		//!< 唯一のインスタンスです

	LPCTSTR m_lpDisk[4];	//!< ディスク
	LPCTSTR m_lpCDisk[4];	//!< CD
	LPTSTR m_lpIniFile;	//!< 設定ファイル
	bool m_fFullscreen;		//!< フルスクリーン モード
	LPTSTR m_lpArg;			//!< ワーク
};

/**
 * インスタンスを得る
 * @return インスタンス
 */
inline Np2Arg* Np2Arg::GetInstance()
{
	return &sm_instance;
}

/**
 * メモリ解放
 */
inline void Np2Arg::Release()
{
	if(sm_instance.m_lpArg){
		free(sm_instance.m_lpArg);
		sm_instance.m_lpArg = NULL;
	}
	if(sm_instance.m_lpIniFile){
		free(sm_instance.m_lpIniFile); // np21w ver0.86 rev8
		sm_instance.m_lpIniFile = NULL;
	}
}

/**
 * ディスク パスを得る
 * @param[in] nDrive ドライブ
 * @return ディスク パス
 */
inline LPCTSTR Np2Arg::disk(int nDrive) const
{
	return m_lpDisk[nDrive];
}

/**
 * CD パスを得る
 * @param[in] nDrive ドライブ
 * @return ディスク パス
 */
inline LPCTSTR Np2Arg::cdisk(int nDrive) const
{
	return m_lpCDisk[nDrive];
}

/**
 * INI パスを得る
 * @return INI パス
 */
inline LPCTSTR Np2Arg::iniFilename() const
{
	return m_lpIniFile;
}

/**
 * フルスクリーンかを得る
 * @retval true フルスクリーン モード
 * @retval false ウィンドウ モード
 */
inline bool Np2Arg::fullscreen() const
{
	return m_fFullscreen;
}

/**
 * INI パスを得る
 * @return INI パス
 */
inline void Np2Arg::setiniFilename(LPTSTR newfile)
{
	if(m_lpIniFile){
		free(m_lpIniFile);
		m_lpIniFile = NULL;
	}
	m_lpIniFile = newfile;
}

