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

#if defined(TIOCSSERIAL)
#include <linux/serial.h>

static int
speed_to_const(speed_t speed)
{

	switch (speed) {
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
#ifdef B7200
	case 7200: return B7200;
#endif
	case 9600: return B9600;
#ifdef B14400
	case 14400: return B14400;
#endif
	case 19200: return B19200;
#ifdef B28800
	case 28800: return B28800;
#endif
	case 38400: return B38400;
#ifdef B57600
	case 57600: return B57600;
#endif
#ifdef B76800
	case 76800: return B76800;
#endif
#ifdef B115200
	case 115200: return B115200;
#endif
#ifdef B230400
	case 230400: return B230400;
#endif
#ifdef B460800
	case 460800: return B460800;
#endif
#ifdef B500000
	case 500000: return B500000;
#endif
#ifdef B576000
	case 576000: return B576000;
#endif
#ifdef B921600
	case 921600: return B921600;
#endif
#ifdef B1000000
	case 1000000: return B1000000;
#endif
#ifdef B1152000
	case 1152000: return B1152000;
#endif
#ifdef B1500000
	case 1500000: return B1500000;
#endif
#ifdef B2000000
	case 2000000: return B2000000;
#endif
#ifdef B2500000
	case 2500000: return B2500000;
#endif
#ifdef B3000000
	case 3000000: return B3000000;
#endif
#ifdef B3500000
	case 3500000: return B3500000;
#endif
#ifdef B4000000
	case 4000000: return B4000000;
#endif
	}
	return 0;
}
#endif

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
#if defined(TIOCSSERIAL)
	struct serial_struct ss;
	int cspeed;
#endif

	Close();

	int fd = ::open(bsdPath, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		return false;
	}

	if (!::isatty(fd)) {
		printf("Error not TTY device %s: %s(%d).\n",
		    bsdPath, strerror(errno), errno);
		goto err;
	}

	if (::ioctl(fd, TIOCEXCL) < 0) {
		printf("Error setting TIOCEXCL on %s: %s(%d).\n",
		    bsdPath, strerror(errno), errno);
		goto err;
	}

	if (::fcntl(fd, F_SETFL, 0) < 0) {
		printf("Error clearing O_NONBLOCK %s: %s(%d).\n",
		    bsdPath, strerror(errno), errno);
		goto err;
	}

	struct termios options;
	if (::tcgetattr(fd, &options) < 0) {
		printf("Error getting tty attributes %s: %s(%d).\n",
		    bsdPath, strerror(errno), errno);
		goto err;
	}

	// Print the current input and output baud rates.
	printf("Current input baud rate is %d\n",
	    (int) cfgetispeed(&options));
	printf("Current output baud rate is %d\n",
	    (int) cfgetospeed(&options));

	tcflush(fd, TCIFLUSH);

	cfmakeraw(&options);
	options.c_cflag |= CLOCAL | CREAD;
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_lflag &= ~(ICANON | ECHO | ISIG);
	options.c_iflag &= ~(IXON | IXOFF | IXANY);
	options.c_oflag &= ~OPOST;

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 10;

#if defined(IOSSIOSPEED)
	// The baud rate, word length, and handshake options can be set
	// as follows:
	cfsetspeed(&options, B19200);		// Set 19200 baud
#elif defined(TIOCSSERIAL)
	cspeed = speed_to_const(speed);
	printf("Converted baud rate is %d(%d)\n", speed, cspeed);
	memset(&ss, 0, sizeof(ss));
	if (::ioctl(fd, TIOCGSERIAL, &ss) < 0) {
		if (cspeed == 0) {
			printf("Error getting serial attributes %s: %s(%d).\n",
			    bsdPath, strerror(errno), errno);
			goto err;
		}
	}
	if (cspeed == 0) {
		ss.flags &= ~ASYNC_SPD_MASK;
		ss.flags |= ASYNC_SPD_CUST;
		ss.custom_divisor = (ss.baud_base + (speed / 2)) / speed;
		if (ss.custom_divisor < 1)
			ss.custom_divisor = 1;
	} else {
		ss.flags &= ~ASYNC_SPD_CUST;
		ss.custom_divisor = 0;
	}
	cfsetispeed(&options, cspeed == 0 ? B38400 : cspeed);
	cfsetospeed(&options, cspeed == 0 ? B38400 : cspeed);
	if (::ioctl(fd, TIOCSSERIAL, &ss) < 0) {
		if (cspeed == 0) {
			printf("Error setting serial attributes %s: %s(%d).\n",
			    bsdPath, strerror(errno), errno);
			goto err;
		}
	}
	if (::ioctl(fd, TIOCGSERIAL, &ss) < 0) {
		if (cspeed == 0) {
			printf("Error getting serial attributes %s: %s(%d).\n",
			    bsdPath, strerror(errno), errno);
			goto err;
		}
	}
	if (cspeed == 0 && ss.custom_divisor * speed != ss.baud_base) {
		if (ss.custom_divisor != 0) {
			printf("Actual baud rate is %d / %d = %.3f\n",
			    ss.baud_base, ss.custom_divisor,
			    (double)ss.baud_base / ss.custom_divisor);
		}
	}
#else
	cfsetispeed(&options, speed);
	cfsetospeed(&options, speed);
#endif
	if (!SetParam(param, &options.c_cflag)) {
		goto err;
	}

	printf("Input baud rate changed to %d\n",
	    (int) cfgetispeed(&options));
	printf("Output baud rate changed to %d\n",
	    (int) cfgetospeed(&options));

	if (::tcsetattr(fd, TCSANOW, &options) == -1) {
		printf("Error setting tty attributes %s: %s(%d).\n",
		    bsdPath, strerror(errno), errno);
		goto err;
	}

#if defined(IOSSIOSPEED)
	// The IOSSIOSPEED ioctl can be used to set arbitrary baud rates
	// other than those specified by POSIX. The driver for the
	// underlying serial hardware ultimately determines which baud
	// rates can be used. This ioctl sets both the input and output
	// speed.
	speed_t setSpeed = speed;
	if (::ioctl(fd, IOSSIOSPEED, &setSpeed) < 0) {
		printf("Error calling ioctl(IOSSIOSPEED) %s: %s(%d).\n",
		    bsdPath, strerror(errno), errno);
		goto err;
	}
#endif	/* IOSSIOSPEED */

	memset(&options, 0, sizeof(options));
	if (::tcgetattr(fd, &options) < 0) {
		printf("Error getting tty attributes %s: %s(%d).\n",
		    bsdPath, strerror(errno), errno);
		goto err;
	}

	printf("Actual input baud rate changed to %d\n",
	    (int) ::cfgetispeed(&options));
	printf("Actual output baud rate changed to %d\n",
	    (int) ::cfgetospeed(&options));

	m_fd = fd;
	return true;

err:
	tcflush(fd, TCOFLUSH);
	::close(fd);
	return false;
}

/**
 * クローズする
 */
void CTty::Close()
{
	if (m_fd >= 0) {
#if defined(TIOCSSERIAL)
		struct serial_struct ss;
		::memset(&ss, 0, sizeof(ss));
		if (::ioctl(m_fd, TIOCGSERIAL, &ss) < 0)
			goto close;
		ss.flags &= ~ASYNC_SPD_CUST;
		ss.custom_divisor = 0;
		if (::ioctl(m_fd, TIOCSSERIAL, &ss) < 0)
			goto close;
close:
#endif
		tcflush(m_fd, TCOFLUSH);
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
	if (m_fd < 0) {
		return 0;
	}
	if ((data_ptr == NULL) || (data_size <= 0)) {
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
	if (m_fd < 0) {
		return 0;
	}
	if ((data_ptr == NULL) || (data_size <= 0)) {
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
	if (cflag_ptr != NULL) {
		cflag = *cflag_ptr;
	}

	if (param != NULL) {
		cflag &= ~CSIZE;
		switch (param[0]) {
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

		switch (param[1]) {
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
		case 'S':		// for space parity
		default:
			return false;
		}

		if (::strcmp(param + 2, "1") == 0) {
			cflag &= ~CSTOPB;
		} else if (::strcmp(param + 2, "2") == 0) {
			cflag |= CSTOPB;
		} else {
			return false;
		}
	}

	if (cflag_ptr != NULL) {
		*cflag_ptr = cflag;
	}
	return true;
}
