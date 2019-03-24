/**
 * @file	tty.h
 * @brief	シリアル通信クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <termios.h>

/**
 * @brief シリアル通信
 */
class CTty
{
public:
	CTty();
	~CTty();
	bool Open(const char* dev, unsigned int speed = 0, const char* param = NULL);
	void Close();
	ssize_t Read(void* data_ptr, ssize_t data_size);
	ssize_t Write(const void* data_ptr, ssize_t data_size);
	bool IsOpened() const;

private:
	int m_fd;		//!< ファイル ディスクリプタ
	static bool SetParam(const char* param, tcflag_t* cflag_ptr);
};

/**
 * オープン済?
 * @retval true オープン済
 * @retval false 未オープン
 */
inline bool CTty::IsOpened() const
{
	return (m_fd >= 0);
}
