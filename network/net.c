/**
 * @file	net.c
 * @brief	Virtual LAN Interface
 *
 * @author	$Author: SimK $
 */

#include	"compiler.h"
#if defined(_MSC_VER)
#include	<io.h>
#else
#include	<unistd.h>
#endif
#include	"codecnv/codecnv.h"

//#define TRACEOUT(a) printf(a);printf("\n");
#define TRACEOUT(a)

#if defined(SUPPORT_NET)

#include	"pccore.h"
#include	"net.h"
#ifdef SUPPORT_LGY98
#include	"lgy98.h"
#endif

#if defined(_WINDOWS)
#include <winioctl.h>
#include <tchar.h>

#if defined(_WINDOWS)
#include	<process.h>
#endif

#pragma warning(disable: 4996)
#pragma comment(lib, "Advapi32.lib")

#define DEVICE_PATH_FMT _T("\\\\.\\Global\\%s.tap")
 
#define TAP_CONTROL_CODE(request,method) \
  CTL_CODE (FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)
 
#define TAP_IOCTL_SET_MEDIA_STATUS \
  TAP_CONTROL_CODE (6, METHOD_BUFFERED)

#else

// for Linux
#include <semaphore.h>
#include <pthread.h>

#include <sched.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/time.h>

/*
unsigned GetTickCount()
{
        struct timeval tv;
        if(gettimeofday(&tv, NULL) != 0)
                return 0;

        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
*/

#if defined(__APPLE__)
#include <sys/kern_control.h>
#include <net/if.h>
#include <net/if_utun.h>
#else
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>	/* struct ethhdr */
#endif

#endif // defined(_WINDOWS)
 
#define NET_BUFLEN (10*1024) // バッファ1つの長さ（XXX: パケットサイズの最大値にしないと無駄。もっと言えば可変長で大きな1つのバッファに入れるべき？）
#define NET_ARYLEN (128) // バッファの数

	NP2NET	np2net;
	
static OEMCHAR np2net_tapName[MAX_PATH]; // TAPデバイス名

static int		np2net_hThreadexit = 0; // スレッド終了フラグ

#if defined(_WINDOWS)
static char *GetNetWorkDeviceGuid(const char *, char *, DWORD); // TAPデバイス名からGUIDを取得する

static HANDLE	np2net_hTap = NULL; // TAPデバイスの読み書きハンドル
static HANDLE	np2net_hThreadR = NULL; // Read用スレッド
static HANDLE	np2net_hThreadW = NULL; // Write用スレッド
#else

// for Linux
static int	np2net_hTap = -1; // TAPデバイスの読み書きハンドル
static int			np2net_hThreadE = 0; // Thread Running Flag
static pthread_t	np2net_hThreadR = NULL; // Read用スレッド
static pthread_t	np2net_hThreadW = NULL; // Write用スレッド
#endif // defined(_WINDOWS)

static UINT8	np2net_membuf[NET_ARYLEN][NET_BUFLEN]; // 送信用バッファ
static int		np2net_membuflen[NET_ARYLEN]; // 送信用バッファにあるデータの長さ
static int		np2net_membuf_readpos = 0; // バッファ読み取り位置
static int		np2net_membuf_writepos = 0; // バッファ書き込み位置

static int		np2net_pmm = 0; // CPU負荷低減モード（通信は若干遅くなると思われる）
static int		np2net_highspeedmode = 0; // 高速送受信モード
static UINT32		np2net_highspeeddatacount = 0; // 送受信データ数カウンタ

#if defined(_WINDOWS)
static HANDLE		np2net_write_hEvent;
static OVERLAPPED	np2net_write_ovl;
#endif // defined(_WINDOWS)

#if defined(_WINDOWS)
// パケットデータを TAP へ書き出す
static int doWriteTap(HANDLE hTap, const UINT8 *pSendBuf, UINT32 len)
{
	#define ETHERDATALEN_MIN 46
	DWORD dwWriteLen;

	if (!WriteFile(hTap, pSendBuf, len, &dwWriteLen, &np2net_write_ovl)) {
		DWORD err = GetLastError();
		if (err == ERROR_IO_PENDING) {
			if(WaitForSingleObject(np2net_write_hEvent, 3000)!=WAIT_TIMEOUT){ // 完了待ち
				GetOverlappedResult(hTap, &np2net_write_ovl, &dwWriteLen, FALSE);
			} 
		} else {
			TRACEOUT(("LGY-98: WriteFile err=0x%08X\n", err));
			return -1;
		}
	}
	//TRACEOUT(("LGY-98: send %u bytes\n", dwWriteLen));
	return 0;
}
#else

// for Linux
// パケットデータを TAP へ書き出す
static int doWriteTap(int hTap, const UINT8 *pSendBuf, UINT32 len)
{
	#define ETHERDATALEN_MIN 46
	UINT32 dwWriteLen;

	if ((dwWriteLen = write(hTap, pSendBuf, len)) == -1) {
		TRACEOUT(("LGY-98: WriteFile err"));
		return -1;
	}
	//TRACEOUT(("LGY-98: send %u bytes\n", dwWriteLen));
	return 0;
}

#endif // defined(_WINDOWS)

// パケットデータをバッファに送る（実際の送信はnp2net_ThreadFuncW内で行われる）
static int sendDataToBuffer(const UINT8 *pSendBuf, UINT32 len){
	if(len > NET_BUFLEN){
		TRACEOUT(("LGY-98: too large packet!! %d bytes", len));
		return 1;
	}
	if(np2net_membuf_readpos==(np2net_membuf_writepos+1)%NET_ARYLEN){
		np2net_highspeedmode = 1;
		TRACEOUT(("LGY-98: buffer full"));
		while(np2net_membuf_readpos==(np2net_membuf_writepos+1)%NET_ARYLEN){
			//Sleep(0); // バッファがいっぱいなので待つ
			return 1; // バッファがいっぱいなので捨てる
		}
	}
	memcpy(np2net_membuf[np2net_membuf_writepos], pSendBuf, len);
	np2net_membuflen[np2net_membuf_writepos] = len;
	np2net_membuf_writepos = (np2net_membuf_writepos+1)%NET_ARYLEN;
	np2net_highspeeddatacount += len*50;
	return 0;
}

// パケット送信時に呼ばれる（デフォルト処理）
static void np2net_default_send_packet(const UINT8 *buf, int size)
{
	sendDataToBuffer(buf, size);
}
// パケット受信時に呼ばれる（デフォルト処理）
static void np2net_default_recieve_packet(const UINT8 *buf, int size)
{
	// 何もしない
}

static void np2net_updateHighSpeedMode(){
	static UINT32	np2net_highspeedtimer = 0; // 送受信データカウント基準時刻
	static UINT32	np2net_highspeeddataspeed = 0; // 1秒当たりの送受信データ数
	//HDC hdc;
	//RECT r = {0, 0, 100, 100};
	int timediff;
	if(np2net_pmm && np2net_membuf_readpos!=(np2net_membuf_writepos+1)%NET_ARYLEN){
		timediff = GetTickCount() - np2net_highspeedtimer;
		if(timediff<0) timediff = INT_MAX;
		if((!np2net_highspeedmode && timediff>1000)
			|| (np2net_highspeedmode && timediff>8000)){
			np2net_highspeedtimer = GetTickCount();
			np2net_highspeeddataspeed = np2net_highspeeddatacount*1000 / timediff;
			np2net_highspeeddatacount = 0;
			if(np2net_highspeeddataspeed < 3000){
				np2net_highspeedmode = 0;
			}else{
				np2net_highspeedmode = 1;
			}
		}
	}else{
		np2net_highspeedmode = 1;
	}
}

#if defined(_WINDOWS)
//  非同期で通信してみる（Write）
static unsigned int __stdcall np2net_ThreadFuncW(LPVOID vdParam) {
	while (!np2net_hThreadexit) {
		if(np2net.recieve_packet != np2net_default_recieve_packet){
			if(np2net_membuf_readpos!=np2net_membuf_writepos){
				doWriteTap(np2net_hTap, (UCHAR*)(np2net_membuf[np2net_membuf_readpos]), np2net_membuflen[np2net_membuf_readpos]);
				np2net_membuf_readpos = (np2net_membuf_readpos+1)%NET_ARYLEN;
			}else{
				Sleep(0);
			}
		}else{
			Sleep(1000);
		}
		np2net_updateHighSpeedMode();
		if(!np2net_highspeedmode) 
			Sleep(50);
	}
	return 0;
}
//  非同期で通信してみる（Read）
static unsigned int __stdcall np2net_ThreadFuncR(LPVOID vdParam) {
	HANDLE hEvent = NULL;
	DWORD dwLen;
	OVERLAPPED ovl;
	int nodatacount = 0;
	int sleepcount = 0;
	int timediff = 0;
	CHAR np2net_Buf[NET_BUFLEN];

	// OVERLAPPED非同期読み取り準備
	memset(&ovl, 0, sizeof(OVERLAPPED));
	ovl.hEvent = hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	ovl.Offset = 0;
	ovl.OffsetHigh = 0;
 
	while (!np2net_hThreadexit) {
		if (!ReadFile(np2net_hTap, np2net_Buf, sizeof(np2net_Buf), &dwLen, &ovl)) {
			DWORD err = GetLastError();
			if (err == ERROR_IO_PENDING) {
				// 読み取り待ち
				if(WaitForSingleObject(hEvent, 3000)!=WAIT_TIMEOUT){ // 受信完了待ち
					GetOverlappedResult(np2net_hTap, &ovl, &dwLen, FALSE);
					if(dwLen>0){
						//TRACEOUT(("LGY-98: recieve %u bytes¥n", dwLen));
						np2net.recieve_packet((UINT8*)np2net_Buf, dwLen); // 受信できたので通知する
						np2net_highspeeddatacount += dwLen;
					}
				} 
			} else {
				// 読み取りエラー
				printf("TAP-Win32: ReadFile err=0x%08X\n", err);
				//CloseHandle(hTap);
				//return -1;
				Sleep(1);
			}
		} else {
			// 読み取り成功
			if(dwLen>0){
				//TRACEOUT(("LGY-98: recieve %u bytes\n", dwLen));
				np2net.recieve_packet((UINT8*)np2net_Buf, dwLen); // 受信できたので通知する
				np2net_highspeeddatacount += dwLen;
			}else{
				Sleep(1);
			}
		}
		np2net_updateHighSpeedMode();
		if(!np2net_highspeedmode) {
			Sleep(50);
		}else{
			Sleep(1);
		}
	}
	CloseHandle(hEvent);
	hEvent = NULL;
	return 0;
}
#else

// for Linux
//  非同期で通信してみる（Write）
static void* np2net_ThreadFuncW(void *thdata) {
	while (!np2net_hThreadexit) {
		if(np2net.recieve_packet != np2net_default_recieve_packet){
			if(np2net_membuf_readpos!=np2net_membuf_writepos){
				doWriteTap(np2net_hTap, np2net_membuf[np2net_membuf_readpos], np2net_membuflen[np2net_membuf_readpos]);
				np2net_membuf_readpos = (np2net_membuf_readpos+1)%NET_ARYLEN;
			}else{
				sched_yield();
			}
		}else{
			sleep(1000);
		}
		np2net_updateHighSpeedMode();
		if(!np2net_highspeedmode) 
			sleep(50);
	}
	return (void*) NULL;
}
//  非同期で通信してみる（Read）
static void* np2net_ThreadFuncR(void *thdata) {
	UINT32 dwLen;
	UINT8 np2net_Buf[NET_BUFLEN];

	while (!np2net_hThreadexit) {
		if ((dwLen = read(np2net_hTap, np2net_Buf, sizeof(np2net_Buf))) < 0) {
			// 読み取りエラー
			printf("TAP-Win32: ReadFile err");
			sched_yield();
		} else {
			// 読み取り成功
			if(dwLen>0){
				//TRACEOUT(("LGY-98: recieve %u bytes\n", dwLen));
				np2net.recieve_packet((UINT8*)np2net_Buf, dwLen); // 受信できたので通知する
				np2net_highspeeddatacount += dwLen;
			}else{
				sched_yield();
			}
		}
		np2net_updateHighSpeedMode();
		if(!np2net_highspeedmode) 
			sleep(50);
	}
	return (void*) NULL;
}

#endif // defined(_WINDOWS)

//  TAPデバイスを閉じる
static void np2net_closeTAP(){
#if defined(_WINDOWS)
    if (np2net_hTap != NULL) {
		if(np2net_hThreadR){
			np2net_hThreadexit = 1;
			if(WaitForSingleObject(np2net_hThreadR, 5000) == WAIT_TIMEOUT){
				TerminateThread(np2net_hThreadR, 0);
			}
			if(WaitForSingleObject(np2net_hThreadW, 1000) == WAIT_TIMEOUT){
				TerminateThread(np2net_hThreadW, 0);
			}
			np2net_membuf_readpos = np2net_membuf_writepos;
			np2net_hThreadexit = 0;
			CloseHandle(np2net_hThreadR);
			CloseHandle(np2net_hThreadW);
			np2net_hThreadR = NULL;
			np2net_hThreadW = NULL;
		}
		CloseHandle(np2net_hTap);
		TRACEOUT(("LGY-98: TAP is closed"));
		np2net_hTap = NULL;
    }
#else
    if (np2net_hTap >= 0) {
		if(np2net_hThreadE){
			np2net_hThreadexit = 1;
			pthread_join(np2net_hThreadR , NULL);
			pthread_join(np2net_hThreadW , NULL);
			np2net_membuf_readpos = np2net_membuf_writepos;
			np2net_hThreadexit = 0;
			np2net_hThreadR = NULL;
			np2net_hThreadW = NULL;
			np2net_hThreadE = 0;
		}
        close(np2net_hTap);
		np2net_hTap = -1;
		TRACEOUT(("LGY-98: TAP is closed"));
    }
#endif // defined(_WINDOWS)
}
//  TAPデバイスを開く
static int np2net_openTAP(const char* tapname){
#if defined(_WINDOWS)
	DWORD dwID;
	DWORD dwLen;
	ULONG status = TRUE;
	char Buf[2048];
	char DevicePath[256];
	char TAPname[MAX_PATH] = "TAP1";
	wchar_t wDevicePath[256];

	if(*tapname){
		strcpy(TAPname, tapname);
	}

	np2net_closeTAP();

	// 指定された表示名から TAP の GUID を得る
	if (!GetNetWorkDeviceGuid(TAPname, Buf, 2048)) {
		TRACEOUT(("LGY-98: [%s] GUID is not found\n", TAPname));
		return 1;
	}
	TRACEOUT(("LGY-98: [%s] GUID = %s\n", TAPname, Buf));
	sprintf(DevicePath, DEVICE_PATH_FMT, Buf);

	// TAP デバイスを開く
	codecnv_utf8toucs2(wDevicePath, MAX_PATH, DevicePath, -1);
	np2net_hTap = CreateFileW(wDevicePath, GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);
 
	if (np2net_hTap == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		TRACEOUT(("LGY-98: Failed to open [%s] error:%d", DevicePath, err));
		return 2;
	}

	TRACEOUT(("LGY-98: TAP is opened"));
	
	// TAP デバイスをアクティブに
	status = TRUE;
	if (!DeviceIoControl(np2net_hTap,TAP_IOCTL_SET_MEDIA_STATUS,
				&status, sizeof(status), &status, sizeof(status),
				&dwLen, NULL)) {
		TRACEOUT(("LGY-98: TAP_IOCTL_SET_MEDIA_STATUS err"));
		np2net_closeTAP();
		return 3;
	}
 
	np2net_hThreadR = (HANDLE)_beginthreadex(NULL , 0 , np2net_ThreadFuncR  , NULL , 0 , &dwID);
	np2net_hThreadW = (HANDLE)_beginthreadex(NULL , 0 , np2net_ThreadFuncW , NULL , 0 , &dwID);
#else
	struct ifreq ifr;
#if defined(__APPLE__)
	np2net_hTap = open("/dev/tap0", O_RDWR);
#else
	np2net_hTap = open("/dev/net/tun", O_RDWR);
#endif
	if(np2net_hTap < 0){
#if defined(__APPLE__)
		TRACEOUT(("LGY-98: Failed to open [%s]", "/dev/tap0"));
#else
		TRACEOUT(("LGY-98: Failed to open [%s]", "/dev/net/tun"));
#endif
		return 2;
	}
	memset(&ifr, 0, sizeof(ifr));

#if defined(__APPLE__)
	strcpy(ifr.ifr_name, "tap%d");
#else
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strcpy(ifr.ifr_name, "tap%d");
	
	if (ioctl(np2net_hTap, TUNSETIFF, (void *)&ifr) < 0) {
		TRACEOUT(("LGY-98: TUNSETIFF err"));
		np2net_closeTAP();
		return 3;
	}
#endif
	if(pthread_create(&np2net_hThreadR , NULL , np2net_ThreadFuncR , NULL) != 0){
		TRACEOUT(("LGY-98: thread_create(READ) err"));
		np2net_closeTAP();
		return 3;
	}
	if(pthread_create(&np2net_hThreadW , NULL , np2net_ThreadFuncW , NULL) != 0){
		TRACEOUT(("LGY-98: thread_create(WRITE) err"));
		np2net_closeTAP();
		return 3;
	}

	TRACEOUT(("LGY-98: TAP is opened"));
#endif // defined(_WINDOWS)
	return 0;
}

// NP2起動時の処理
void np2net_init(void)
{
#if defined(_WINDOWS)
	memset(&np2net_write_ovl, 0, sizeof(OVERLAPPED));
	np2net_write_ovl.hEvent = np2net_write_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	np2net_write_ovl.Offset = 0;
	np2net_write_ovl.OffsetHigh = 0;
#endif // defined(_WINDOWS)

	memset(np2net_tapName, 0, sizeof(np2net_tapName));
	np2net.send_packet = np2net_default_send_packet;
	np2net.recieve_packet = np2net_default_recieve_packet;
}
// リセット時に呼ばれる？
void np2net_reset(const NP2CFG *pConfig){
	strcpy(np2net_tapName, pConfig->np2nettap);
	np2net_pmm = pConfig->np2netpmm;
	if(pConfig->uselgy98){ // XXX: 使われていないならTAPデバイスはオープンしない
		np2net_openTAP(np2net_tapName);
	}
}
// リセット時に呼ばれる？（np2net_resetより後・iocore_attach〜が使える）
void np2net_bind(void){
}
// NP2終了時の処理
void np2net_shutdown(void)
{
	np2net_hThreadexit = 1;
	np2net_closeTAP();
#ifdef SUPPORT_LGY98
	lgy98_shutdown();
#endif

}

#if defined(_WINDOWS)
// 参考文献: http://dsas.blog.klab.org/archives/51012690.html

// ネットワークデバイス表示名からデバイス GUID 文字列を検索
static char *GetNetWorkDeviceGuid(const char *pDisplayName, char *pszBuf, DWORD cbBuf)
{
  const wchar_t *SUBKEY = L"SYSTEM\\CurrentControlSet\\Control\\Network";
 
#define BUFSZ 256
  // HKLM\SYSTEM\\CurrentControlSet\\Control\\Network\{id1]\{id2}\Connection\Name が
  // ネットワークデバイス名（ユニーク）の格納されたエントリであり、
  // {id2} がこのデバイスの GUID である
 
  HKEY hKey1, hKey2, hKey3;
  LONG nResult;
  DWORD dwIdx1, dwIdx2;
  char szData[64];
  wchar_t *pKeyName1, *pKeyName2, *pKeyName3, *pKeyName4;

  DWORD dwSize, dwType = REG_SZ;
  BOOL bDone = FALSE;
  FILETIME ft;

  hKey1 = hKey2 = hKey3 = NULL;
  pKeyName1 = pKeyName2 = pKeyName3 = pKeyName4 = NULL;
 
  // 主キーのオープン
  nResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, SUBKEY, 0, KEY_READ, &hKey1);
  if (nResult != ERROR_SUCCESS) {
    return NULL;
  }
  pKeyName1 = (wchar_t*)malloc(sizeof(wchar_t)*BUFSZ);
  pKeyName2 = (wchar_t*)malloc(sizeof(wchar_t)*BUFSZ);
  pKeyName3 = (wchar_t*)malloc(sizeof(wchar_t)*BUFSZ);
  pKeyName4 = (wchar_t*)malloc(sizeof(wchar_t)*BUFSZ);
 
  dwIdx1 = 0;
  while (bDone != TRUE) { // {id1} を列挙するループ
 
    dwSize = BUFSZ;
    nResult = RegEnumKeyExW(hKey1, dwIdx1++, pKeyName1,
                          &dwSize, NULL, NULL, NULL, &ft);
    if (nResult == ERROR_NO_MORE_ITEMS) {
      break;
    }
 
    // SUBKEY\{id1} キーをオープン
    swprintf(pKeyName2, BUFSZ, L"%ls\\%ls", SUBKEY, pKeyName1);
    nResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, pKeyName2,
                          0, KEY_READ, &hKey2);
    if (nResult != ERROR_SUCCESS) {
      continue;
    }
    dwIdx2 = 0;
    while (1) { // {id2} を列挙するループ
      dwSize = BUFSZ;
      nResult = RegEnumKeyExW(hKey2, dwIdx2++, pKeyName3,
                          &dwSize, NULL, NULL, NULL, &ft);
      if (nResult == ERROR_NO_MORE_ITEMS) {
        break;
      }
 
      if (nResult != ERROR_SUCCESS) {
        continue;
      }
 
      // SUBKEY\{id1}\{id2]\Connection キーをオープン
      swprintf(pKeyName4, BUFSZ, L"%ls\\%ls\\%ls",
                      pKeyName2, pKeyName3, L"Connection");
      nResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      pKeyName4, 0, KEY_READ, &hKey3);
      if (nResult != ERROR_SUCCESS) {
        continue;
      }
 
      // SUBKEY\{id1}\{id2]\Connection\Name 値を取得
      dwSize = sizeof(szData);
      nResult = RegQueryValueExW(hKey3, L"Name",
                      0, &dwType, (LPBYTE)szData, &dwSize);
 
      if (nResult == ERROR_SUCCESS) {
        if (stricmp(szData, pDisplayName) == 0) {
          codecnv_ucs2toutf8(pszBuf, MAX_PATH, pKeyName3, -1);
          bDone = TRUE;
          break;
        }
      }
      RegCloseKey(hKey3);
      hKey3 = NULL;
    }
    RegCloseKey(hKey2);
    hKey2 = NULL;
  }
 
  if (hKey1) { RegCloseKey(hKey1); }
  if (hKey2) { RegCloseKey(hKey2); }
  if (hKey3) { RegCloseKey(hKey3); }
 
  if (pKeyName1) { free(pKeyName1); }
  if (pKeyName2) { free(pKeyName2); }
  if (pKeyName3) { free(pKeyName3); }
  if (pKeyName4) { free(pKeyName4); }
 
  // GUID を発見できず
  if (bDone != TRUE) {
    return NULL;
  }
  return pszBuf;
}
#endif // defined(_WINDOWS)

#endif	/* SUPPORT_NET */
