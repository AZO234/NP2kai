/**
 * @file	sstp.cpp
 * @brief	Sakura Script Transfer Protocol handler
 *
 * @author	$Author: yui $
 * @date	$Date: 2011/03/07 09:54:11 $
 */

#include "compiler.h"
#include <winsock.h>
#include <commctrl.h>
#include "np2.h"
#include "scrnmng.h"
#include "sstp.h"
#if defined(OSLANG_UCS2)
#include "oemtext.h"
#endif

#if !defined(__GNUC__)
#pragma comment(lib, "wsock32.lib")
#endif	// !defined(__GNUC__)

static	HWND		sstphwnd = NULL;
static	int			sstp_stat = SSTP_READY;
static	SOCKET		hSocket = INVALID_SOCKET;
static	WSAData		wsadata;
static	char		sstpstr[0x1000];
static	char		sstprcv[0x1000];
static	DWORD		sstppos = 0;
static	void		(*sstpproc)(HWND, char *) = NULL;

static const OEMCHAR sendermes[] = 										\
					OEMTEXT("SEND SSTP/1.4\r\n")						\
					OEMTEXT("Sender: Neko Project II\r\n")				\
					OEMTEXT("Script: \\h\\s0%s\\e\r\n")					\
					OEMTEXT("Option: notranslate\r\n")					\
					OEMTEXT("Charset: Shift_JIS\r\n")					\
					OEMTEXT("\r\n");


static HANDLE check_ssp(void) {

	HANDLE	hssp;

	hssp = OpenMutex(MUTEX_ALL_ACCESS, FALSE, _T("ssp"));
	if (hssp != NULL) {
		CloseHandle(hssp);
	}
	return(hssp);
}


// ------------------------------------------------------------------ Async...

BOOL sstp_send(const OEMCHAR *msg, void (*proc)(HWND hWnd, char *msg)) {

	sockaddr_in	s_in;

	if (hSocket != INVALID_SOCKET) {
		sstp_stat = SSTP_BUSY;
		return(FAILURE);
	}
	if ((!np2oscfg.sstp) || (scrnmng_isfullscreen()) || (!check_ssp())) {
		sstp_stat = SSTP_NOSEND;
		return(FAILURE);
	}

	if ((!sstphwnd) || (WSAStartup(0x0101, &wsadata))) {
		sstp_stat = SSTP_ERROR;
		return(FAILURE);
	}

#if defined(OSLANG_UCS2)
	OEMCHAR	oem[0x1000];
	OEMSPRINTF(oem, sendermes, msg);
	oemtext_oemtosjis(sstpstr, NELEMENTS(sstpstr), oem, -1);
#else
	OEMSPRINTF(sstpstr, sendermes, msg);
#endif
	sstprcv[0] = 0;
	sstppos = 0;

	hSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET) {
		goto sstp_senderror;
	}

	if (WSAAsyncSelect(hSocket, sstphwnd, WM_SSTP,
							FD_CONNECT + FD_READ + FD_WRITE + FD_CLOSE)) {
		goto sstp_senderror;
	}

	s_in.sin_family = AF_INET;
	*(DWORD *)(&s_in.sin_addr) = 0x0100007f;
	s_in.sin_port = htons(np2oscfg.sstpport);
	if (connect(hSocket, (sockaddr *)&s_in, sizeof(s_in)) == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			goto sstp_senderror;
		}
	}
	sstp_stat = SSTP_SENDING;
	sstpproc = proc;
	return(SUCCESS);

sstp_senderror:;
	if (hSocket != INVALID_SOCKET) {
		closesocket(hSocket);
		hSocket = INVALID_SOCKET;
	}
	WSACleanup();
	sstp_stat = SSTP_ERROR;
	return(FAILURE);
}



void sstp_connect(void) {

	if (hSocket != INVALID_SOCKET) {
		send(hSocket, sstpstr, (int)strlen(sstpstr), 0);
	}
}

void sstp_readSocket(void) {

	if (hSocket != INVALID_SOCKET) {
		DWORD	available;
		int		len;
		char	buf[256];
		while(1) {
			if (ioctlsocket(hSocket, FIONREAD, &available) != 0) {
				break;
			}
			if (!available) {
				break;
			}
			if (available >= sizeof(buf)) {
				available = sizeof(buf);
			}
			len = recv(hSocket, buf, available, 0);
			if (len >= (int)((sizeof(sstprcv) - 1) - sstppos)) {
				len = (int)((sizeof(sstprcv) - 1) - sstppos);
			}
			if (len > 0) {
				CopyMemory(sstprcv + sstppos, buf, len);
				sstppos += len;
				sstprcv[sstppos] = '\0';
			}
		}
	}
}

static BOOL disconnection(void) {

	if (hSocket != INVALID_SOCKET) {
		closesocket(hSocket);
		WSACleanup();
		hSocket = INVALID_SOCKET;
		sstp_stat = SSTP_READY;
		return(SUCCESS);
	}
	return(FAILURE);
}

void sstp_disconnect(void) {

	if (!disconnection()) {
		if (sstpproc) {
			sstpproc(sstphwnd, sstprcv);
		}
	}
}


// ---------------------------------------------------------------------------

void sstp_construct(HWND hwnd) {

	sstphwnd = hwnd;
	sstp_stat = SSTP_READY;
	hSocket = INVALID_SOCKET;
}

void sstp_destruct(void) {

	disconnection();
}

int sstp_result(void) {

	return(sstp_stat);
}



// ----------------------------------------------------------------- 送信逃げ

// 関数一発、送信逃げ。

BOOL sstp_sendonly(const OEMCHAR *msg) {

	WSAData		lwsadata;
	SOCKET		lSocket;
	sockaddr_in	s_in;
	char		msgstr[0x1000];
	BOOL		ret = FAILURE;

	if ((np2oscfg.sstp) && (check_ssp()) &&
		(!WSAStartup(0x0101, &lwsadata))) {
		if ((lSocket = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET) {
			s_in.sin_family = AF_INET;
			*(DWORD *)(&s_in.sin_addr) = 0x0100007f;
			s_in.sin_port = htons(np2oscfg.sstpport);
			if (connect(lSocket, (sockaddr *)&s_in, sizeof(s_in))
															!= SOCKET_ERROR) {
#if defined(OSLANG_UCS2)
				OEMCHAR	oem[0x1000];
				OEMSPRINTF(oem, sendermes, msg);
				oemtext_oemtosjis(msgstr, NELEMENTS(msgstr), oem, -1);
#else
				OEMSPRINTF(msgstr, sendermes, msg);
#endif
				send(lSocket, msgstr, (int)strlen(msgstr), 0);
				ret = SUCCESS;
			}
			closesocket(lSocket);
		}
		WSACleanup();
	}
	return(ret);
}

