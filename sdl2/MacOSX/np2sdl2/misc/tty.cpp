/**
 * @file	tty.cpp
 * @brief	シリアル通信クラスの動作の定義を行います
 */

#include "compiler.h"
#include "tty.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>

/**
 * コンストラクタ
 */
CTty::CTty()
	: m_fd(-1)
{
}

/**
 * デストラクタ
 */
CTty::~CTty()
{
	Close();
}

/**
 * オープンする
 * @param[in] bsdPath デバイス
 * @param[in] speed ボーレート
 * @param[in] param パラメタ
 * @retval true 成功
 * @retval false 失敗
 */
bool CTty::Open(const char* bsdPath, unsigned int speed, const char* param)
{
	Close();

	// Open the serial port read/write, with no controlling terminal, and don't wait for a connection.
	// The O_NONBLOCK flag also causes subsequent I/O on the device to be non-blocking.
	int fd = ::open(bsdPath, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
	{
		return false;
	}

	do
	{
		// Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
		// unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned processes.
		if (::ioctl(fd, TIOCEXCL) == -1)
		{
			printf("Error setting TIOCEXCL on %s - %s(%d).\n", bsdPath, strerror(errno), errno);
			break;
		}

		// Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
		if (::fcntl(fd, F_SETFL, 0) == -1)
		{
			printf("Error clearing O_NONBLOCK %s - %s(%d).\n", bsdPath, strerror(errno), errno);
			break;
		}

		// Get the current options and save them so we can restore the default settings later.
		struct termios options;
		if (::tcgetattr(fd, &options) == -1)
		{
			printf("Error getting tty attributes %s - %s(%d).\n", bsdPath, strerror(errno), errno);
			break;
		}

		// Print the current input and output baud rates.
		printf("Current input baud rate is %d\n", (int) cfgetispeed(&options));
		printf("Current output baud rate is %d\n", (int) cfgetospeed(&options));

		tcflush(fd, TCIFLUSH);

		// Set raw input (non-canonical) mode, with reads blocking until either a single character
		// has been received or a one second timeout expires.
		cfmakeraw(&options);
		options.c_cc[VMIN] = 0;
		options.c_cc[VTIME] = 10;

		options.c_cflag |= (CLOCAL | CREAD);
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		options.c_iflag &= ~(IXON | IXOFF | IXANY);
		options.c_oflag &= ~OPOST;

		// The baud rate, word length, and handshake options can be set as follows:
		cfsetspeed(&options, B19200);		// Set 19200 baud
		if (!SetParam(param, &options.c_cflag))
		{
			break;
		}

		// Print the new input and output baud rates. Note that the IOSSIOSPEED ioctl interacts with the serial driver
		// directly bypassing the termios struct. This means that the following two calls will not be able to read
		// the current baud rate if the IOSSIOSPEED ioctl was used but will instead return the speed set by the last call
		// to cfsetspeed.
		printf("Input baud rate changed to %d\n", (int) cfgetispeed(&options));
		printf("Output baud rate changed to %d\n", (int) cfgetospeed(&options));

		// Cause the new options to take effect immediately.
		if (::tcsetattr(fd, TCSANOW, &options) == -1)
		{
			printf("Error setting tty attributes %s - %s(%d).\n", bsdPath, strerror(errno), errno);
			break;
		}

		// The IOSSIOSPEED ioctl can be used to set arbitrary baud rates
		// other than those specified by POSIX. The driver for the underlying serial hardware
		// ultimately determines which baud rates can be used. This ioctl sets both the input
		// and output speed.
		speed_t setSpeed = speed;
		if (::ioctl(fd, IOSSIOSPEED, &setSpeed) == -1)
		{
			printf("Error calling ioctl(..., IOSSIOSPEED, ...) %s - %s(%d).\n", bsdPath, strerror(errno), errno);
			break;
		}
		printf("speed=%d\n", speed);

		m_fd = fd;
		return true;
	} while (0 /*CONSTCOND*/);

	::close(fd);
	return false;
}

/**
 * クローズする
 */
void CTty::Close()
{
	if (m_fd >= 0)
	{
		::close(m_fd);
		m_fd = -1;
	}
}

/**
 * データ受信
 * @param[in] data_ptr 受信データのポインタ
 * @param[in] data_size 受信データのサイズ
 * @return 送信サイズ
 */
ssize_t CTty::Read(void* data_ptr, ssize_t data_size)
{
	if (m_fd < 0)
	{
		return 0;
	}
	if ((data_ptr == NULL) || (data_size <= 0))
	{
		return 0;
	}

	return ::read(m_fd, data_ptr, data_size);
}

/**
 * データ送信
 * @param[in] data_ptr 送信データのポインタ
 * @param[in] data_size 送信データのサイズ
 * @return 送信サイズ
 */
ssize_t CTty::Write(const void* data_ptr, ssize_t data_size)
{
	if (m_fd < 0)
	{
		return 0;
	}
	if ((data_ptr == NULL) || (data_size <= 0))
	{
		return 0;
	}

	return ::write(m_fd, data_ptr, data_size);
}

/**
 * パラメータ設定
 * @param[in] param パラメタ
 * @param[out] cflag_ptr フラグ
 * @retval true 成功
 * @retval false 失敗
 */
bool CTty::SetParam(const char* param, tcflag_t* cflag_ptr)
{
	tcflag_t cflag = 0;
	if (cflag_ptr != NULL)
	{
		cflag = *cflag_ptr;
	}

	if (param != NULL)
	{
		cflag &= ~CSIZE;
		switch (param[0])
		{
			case '5':
				cflag |= CS5;
				break;

			case '6':
				cflag |= CS6;
				break;

			case '7':
				cflag |= CS7;
				break;

			case '8':
				cflag |= CS8;
				break;

			case '4':
			default:
				return false;
		}

		switch (param[1])
		{
			case 'N':		// for no parity
				cflag &= ~(PARENB | PARODD);
				break;

			case 'E':		// for even parity
				cflag |= PARENB;
				break;

			case 'O':		// for odd parity
				cflag |= PARENB | PARODD;
				break;

			case 'M':		// for mark parity
			case 'S':		// for for space parity
			default:
				return false;
		}

		if (::strcmp(param + 2, "1") == 0)
		{
			cflag &= ~CSTOPB;
		}
		else if (::strcmp(param + 2, "2") == 0)
		{
			cflag |= CSTOPB;
		}
		else
		{
			return false;
		}
	}

	if (cflag_ptr != NULL)
	{
		*cflag_ptr = cflag;
	}
	return true;
}
