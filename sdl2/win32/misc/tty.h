/**
 * @file	tty.h
 * @brief	シリアル通信クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * @brief シリアル通信
 */
class CTty
{
public:
	CTty();
	~CTty();
	bool Open(LPCTSTR lpDevName, UINT nSpeed = 0, LPCTSTR lpcszParam = NULL);
	bool IsOpened() const;
	void Close();
	ssize_t Read(LPVOID lpcvData, ssize_t nDataSize);
	ssize_t Write(LPCVOID lpcvData, ssize_t nDataSize);

private:
	HANDLE m_hFile;				/*!< ファイル ハンドル */
	bool OpenPort(LPCTSTR lpPortName, UINT nSpeed, LPCTSTR lpcszParam);
	static bool SetParam(LPCTSTR lpcszParam, DCB* dcb = NULL);
};

/**
 * オープン済?
 * @retval true オープン済
 * @retval false 未オープン
 */
inline bool CTty::IsOpened() const
{
	return (m_hFile != INVALID_HANDLE_VALUE);
}
