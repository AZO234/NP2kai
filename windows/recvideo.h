/**
 * @file	recvideo.h
 * @brief	録画クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#if defined(SUPPORT_RECVIDEO)

#include <vfw.h>

// #define AVI_SPLIT_SIZE		(1024 * 1024 * 1024)		/**< 分割サイズ */

/**
 * @brief 録画クラス
 */
class RecodeVideo
{
public:
	static RecodeVideo& GetInstance();

	RecodeVideo();
	~RecodeVideo();
	bool Open(HWND hWnd, LPCTSTR lpFilename);
	void Close();
	void Write();
	void Update();
	bool IsEnabled() const;

private:
	static RecodeVideo sm_instance;		/**< 唯一のインスタンスです */

	bool m_bEnabled;					/**< 有効フラグ */
	bool m_bDirty;						/**< ダーティ フラグ */

	int m_nStep;						/**< クロック */
	UINT8* m_pWork8;					/**< ワーク */
	UINT8* m_pWork24;					/**< ワーク */

	PAVIFILE m_pAvi;					/**< AVIFILE */
	PAVISTREAM m_pStm;					/**< AVISTREAM */
	PAVISTREAM m_pStmTmp;				/**< AVISTREAM */
	UINT m_nFrame;						/**< フレーム数 */

	BITMAPINFOHEADER m_bmih;			/**< BITMAPINFOHEADER */
	COMPVARS m_cv;						/**< COMPVARS */

#if defined(AVI_SPLIT_SIZE)
	int m_nNumber;						/**< ファイル番号 */
	DWORD m_dwSize;						/**< サイズ */
	TCHAR m_szPath[MAX_PATH];			/**< ベース パス */
#endif	// defined(AVI_SPLIT_SIZE)

	bool OpenFile(LPCTSTR lpFilename);
	void CloseFile();
};

/**
 * インスタンスを得る
 * @return インスタンス
 */
inline RecodeVideo& RecodeVideo::GetInstance()
{
	return sm_instance;
}

/**
 * 有効?
 * @retval true 有効
 * @retval false 無効
 */
inline bool RecodeVideo::IsEnabled() const
{
	return m_bEnabled;
}

#define recvideo_open			RecodeVideo::GetInstance().Open
#define recvideo_close			RecodeVideo::GetInstance().Close
#define recvideo_write			RecodeVideo::GetInstance().Write
#define recvideo_update			RecodeVideo::GetInstance().Update
#define recvideo_isEnabled		RecodeVideo::GetInstance().IsEnabled

#else	// defined(SUPPORT_RECVIDEO)

static inline bool recvideo_open(HWND hWnd, LPCTSTR f) { return false; }
static inline void recvideo_close() { }
static inline void recvideo_write() { }
static inline void recvideo_update() { }
static inline bool recvideo_isEnabled() { return false; }

#endif	// defined(SUPPORT_RECVIDEO)
