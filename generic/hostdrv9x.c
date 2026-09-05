#include "compiler.h"

#if defined(SUPPORT_HOSTDRV9X)

/*
	ゲストOS(Win9x系)からホストOS(Win)にアクセス
	HOSTDRVのWindows9x対応バージョンです

	Windows 9x IFSMgrを経由します
	VxDからioreqを丸投げしてnp2側で処理します

	Windows 9x IFSMgr
		↓
	HOSTD9X.VXD
		↓
	I/Oポート 07E4h / 07E6h
		↓
	NP2側 hostdrv9x.c
		↓
	File API
		↓
	ホスト共有フォルダ
*/

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#endif
#include <shlwapi.h>
#include <wchar.h>
#define HD_W(s) L##s
#define HD_WC(c) L##c
#else
#include "hostdrvwincompat.h"
#endif
#include <wctype.h>
#include <stdlib.h>
#include <string.h>

#include "pccore.h"
#include "ini.h"
#include "iocore.h"
#include "cpucore.h"
#if defined(SUPPORT_IA32_HAXM)
#include "i386hax/haxfunc.h"
#include "i386hax/haxcore.h"
#endif
#include "dosio.h"
#include "statsave.h"
#include "hostdrv9x.h"
#include "hostdrv9xdef.h"
#if defined(SUPPORT_HOSTDRV)
#include "hostdrv.h"
#include "hostdrvs.h"
#endif

#if 0
#undef	TRACEOUT
static void trace_fmt_ex(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
#define USE_HOSTDRV9X_TRACEOUT
#endif	/* 1 */

#pragma code_seg(".MISCCODE")

#if !defined(CPUCORE_IA32)
#define cpu_kmemorywrite(a,v)   memp_write8(a,v)
#define cpu_kmemorywrite_w(a,v) memp_write16(a,v)
#define cpu_kmemorywrite_d(a,v) memp_write32(a,v)
#define cpu_kmemoryread(a)      memp_read8(a)
#define cpu_kmemoryread_w(a)    memp_read16(a)
#define cpu_kmemoryread_d(a)    memp_read32(a)
#endif

#define H9X_HANDLE_FILE 1
#define H9X_HANDLE_FIND 2
#define H9X_FIND_DOT_PENDING       0x01
#define H9X_FIND_DOTDOT_PENDING    0x02
#define H9X_FIND_HOST_NOT_STARTED  0x04
#define H9X_FIND_SFN_PATTERN_FALLBACK 0x08
#define H9X_ROOT_HANDLE 1
#define H9X_VOLUME_LABEL_HANDLE 0xfffffffeUL
#define H9X_VOLUME_LABEL_COOKIE 0xfffeU
#define H9X_VOLUME_LABEL_A "_HOSTDRIVE_"
#define H9X_VOLUME_LABEL_W HD_W("_HOSTDRIVE_")
#define H9X_FIND_DATA_SIZE 592
#define H9X_BY_HANDLE_INFO_SIZE 52

#define H9X_PERMIT_READ   0x01
#define H9X_PERMIT_WRITE  0x02
#define H9X_PERMIT_DELETE 0x04

/* srch_entry offsets with the Win98 DDK x86 packing. */
#define H9X_SE_NETKEY         17
#define H9X_SE_ATTRIB         21
#define H9X_SE_TIME           22
#define H9X_SE_DATE           24
#define H9X_SE_SIZE           26
#define H9X_SE_NAME           30

typedef HDRVSFNENTRY H9X_SFN_ENTRY;

typedef struct {
	UINT8 type;
	UINT8 findFlags;
	UINT16 generation;
	HANDLE handle;
	DWORD desiredAccess;
	DWORD shareMode;
	WCHAR path[MAX_PATH];
	WIN32_FIND_DATAW findData;
	UINT32 searchAttr;
	H9X_SFN_ENTRY *sfnMap;
	UINT32 sfnCount;
	UINT8 sfnMapBuilt;
} NP2HOSTDRV9X_HANDLE;

typedef struct {
	UINT32 dataAddr;
	UINT32 commandPos;
	UINT8 diagTag;
	UINT8 diagPos;
	UINT16 diagReserved;
	UINT32 diagValue;
	NP2HOSTDRV9X_HANDLE files[NP2HOSTDRV9X_FILES_MAX];
} NP2HOSTDRV9X_STATE;

static NP2HOSTDRV9X_STATE s_h9x;
static WCHAR s_h9xRoot[MAX_PATH];
static UINT8 s_h9xAcc;
static UINT8 s_h9xUseRealCapacity;
static UINT8 s_h9xWin95Compat;
static UINT32 s_h9xFakeTotalMB;
static UINT32 s_h9xFakeFreeMB;

/// <summary>
/// パスの安全性検証
/// </summary>
/// <param name="path">検証したいホストパス</param>
/// <returns></returns>
static int h9x_is_safe_stored_path(const WCHAR *path)
{
	WCHAR current[MAX_PATH];
	WCHAR relative[MAX_PATH];
	WCHAR component[MAX_PATH];
	WCHAR candidate[MAX_PATH];
	WCHAR *p;
	UINT32 rootLen;

	// NULLは不可
	if (!path || !s_h9xRoot[0]) return 0;
	
	// HOSTDRVルート部分のパス一致確認
	rootLen = (UINT32)wcslen(s_h9xRoot);
	if (_wcsnicmp(path, s_h9xRoot, rootLen) != 0) return 0;
	if (path[rootLen] != HD_WC('\0') &&
		!(rootLen > 0 && s_h9xRoot[rootLen - 1] == HD_WC('\\')) &&
		path[rootLen] != HD_WC('\\')) return 0;
	if (path[rootLen] == HD_WC('\0')) return 1;

	// パス階層ごとに問題ないか確認
	wcscpy(current, s_h9xRoot);
	wcsncpy(relative, path + rootLen, NELEMENTS(relative) - 1);
	relative[NELEMENTS(relative) - 1] = HD_WC('\0');
	p = relative;
	while (*p == HD_WC('\\')) p++;
	while (*p != HD_WC('\0'))
	{
		WCHAR *next = wcschr(p, HD_WC('\\'));
		UINT32 len = next ? (UINT32)(next - p) : (UINT32)wcslen(p);
		DWORD attrs;
		if (len == 0 || len >= NELEMENTS(component)) return 0;
		wcsncpy(component, p, len);
		component[len] = HD_WC('\0');
		if (!PathCombineW(candidate, current, component)) return 0;
		attrs = GetFileAttributesW(candidate);
		if (attrs == INVALID_FILE_ATTRIBUTES) return 0;
		if (attrs & FILE_ATTRIBUTE_REPARSE_POINT) return 0;
		wcscpy(current, candidate);
		if (!next) break;
		p = next + 1;
		while (*p == HD_WC('\\')) p++;
	}
	return _wcsicmp(current, path) == 0;
}

static const char *h9x_fn_name(UINT32 fn);
static void h9x_trace_guest_bytes(const char *label, UINT32 addr, UINT32 len);
static void h9x_trace_parsed_path(UINT32 ppath);
static int h9x_is_plain_83_name(const WCHAR *src);

static void h9x_memread(UINT32 addr, void *dst, UINT32 len)
{
	UINT8 *p = (UINT8 *)dst;
	while (len--) {
		*p++ = cpu_kmemoryread(addr++);
	}
}

static void h9x_memwrite(UINT32 addr, const void *src, UINT32 len)
{
	const UINT8 *p = (const UINT8 *)src;
	while (len--) {
		cpu_kmemorywrite(addr++, *p++);
	}
}


static void h9x_config_defaults(void)
{
	s_h9xUseRealCapacity = 0;
	s_h9xWin95Compat = 0;
	s_h9xFakeTotalMB = NP2HOSTDRV9X_FAKE_TOTAL_MB_DEFAULT;
	s_h9xFakeFreeMB = NP2HOSTDRV9X_FAKE_FREE_MB_DEFAULT;
}

static void h9x_control_dispatch(UINT32 function, UINT32 address)
{
	NP2HOSTDRV9X_CONTROL control;

	h9x_memread(address, &control, sizeof(control));
	if (control.size != sizeof(control) ||
		control.version != NP2HOSTDRV9X_CONTROL_VERSION) {
		control.result = H9X_ERROR_INVALID_DATA;
		h9x_memwrite(address, &control, sizeof(control));
		return;
	}

	control.result = H9X_ERROR_SUCCESS;
	switch (function) {
	case NP2HOSTDRV9X_CTL_QUERY_DOS:
		control.flags &= NP2HOSTDRV9X_CONTROL_REAL_CAPACITY;
		control.drive = NP2HOSTDRV9X_INVALID_DRIVE;
#if defined(SUPPORT_HOSTDRV)
		if (hostdrv_ismounted()) {
			control.flags |= NP2HOSTDRV9X_CONTROL_DOS_MOUNTED;
			control.drive = hostdrv_getdriveno();
		}
		if (hostdrv_issuspended())
			control.flags |= NP2HOSTDRV9X_CONTROL_DOS_SUSPENDED;
		if (hostdrv_iscddshidden())
			control.flags |= NP2HOSTDRV9X_CONTROL_DOS_CDS_HIDDEN;
#endif
		break;

	case NP2HOSTDRV9X_CTL_SUSPEND_DOS:
#if defined(SUPPORT_HOSTDRV)
		if (hostdrv_ismounted()) hostdrv_setsuspended(TRUE);
		if (hostdrv_ismounted()) {
			control.flags |= NP2HOSTDRV9X_CONTROL_DOS_MOUNTED;
			control.drive = hostdrv_getdriveno();
		}
		if (hostdrv_issuspended())
			control.flags |= NP2HOSTDRV9X_CONTROL_DOS_SUSPENDED;
		if (hostdrv_iscddshidden())
			control.flags |= NP2HOSTDRV9X_CONTROL_DOS_CDS_HIDDEN;
#else
		control.flags &= ~(NP2HOSTDRV9X_CONTROL_DOS_MOUNTED |
			NP2HOSTDRV9X_CONTROL_DOS_SUSPENDED);
		control.drive = NP2HOSTDRV9X_INVALID_DRIVE;
#endif
		break;

	case NP2HOSTDRV9X_CTL_RESUME_DOS:
#if defined(SUPPORT_HOSTDRV)
		hostdrv_setsuspended(FALSE);
		if (hostdrv_ismounted()) {
			control.flags |= NP2HOSTDRV9X_CONTROL_DOS_MOUNTED;
			control.drive = hostdrv_getdriveno();
		}
		control.flags &= ~(NP2HOSTDRV9X_CONTROL_DOS_SUSPENDED |
			NP2HOSTDRV9X_CONTROL_DOS_CDS_HIDDEN);
#else
		control.flags &= ~(NP2HOSTDRV9X_CONTROL_DOS_MOUNTED |
			NP2HOSTDRV9X_CONTROL_DOS_SUSPENDED);
		control.drive = NP2HOSTDRV9X_INVALID_DRIVE;
#endif
		break;

	case NP2HOSTDRV9X_CTL_SET_CONFIG:
		s_h9xUseRealCapacity =
			(control.flags & NP2HOSTDRV9X_CONTROL_REAL_CAPACITY) ? 1 : 0;
		s_h9xWin95Compat =
			(control.flags & NP2HOSTDRV9X_CONTROL_WIN95_COMPAT) ? 1 : 0;
		if (control.fakeTotalMB)
			s_h9xFakeTotalMB = control.fakeTotalMB;
		if (control.fakeFreeMB <= s_h9xFakeTotalMB)
			s_h9xFakeFreeMB = control.fakeFreeMB;
		else
			s_h9xFakeFreeMB = s_h9xFakeTotalMB;
		break;

	default:
		control.result = H9X_ERROR_INVALID_FUNCTION;
		break;
	}

	h9x_memwrite(address, &control, sizeof(control));
}

// メモリアクセスでページフォールトにならないか先に検証
static void h9x_preflight_linear(UINT32 addr, UINT32 len, int guestWrite)
{
#if defined(CPUCORE_IA32)
	int ucrw = (guestWrite ? CPU_PAGE_WRITE_DATA : CPU_PAGE_READ_DATA) |
		CPU_MODE_SUPERVISER;

	while (len) {
		UINT32 step = 0x1000 - (addr & 0x0fff);
		if (step > len) step = len;
		(void)laddr2paddr(addr, ucrw);
		addr += step;
		len -= step;
	}
#else
	(void)addr;
	(void)len;
	(void)guestWrite;
#endif
}

static UINT8 h9x_r8(UINT32 pir, UINT32 off)
{
	return cpu_kmemoryread(pir + off);
}

static UINT16 h9x_r16(UINT32 pir, UINT32 off)
{
	return cpu_kmemoryread_w(pir + off);
}

static UINT32 h9x_r32(UINT32 pir, UINT32 off)
{
	return cpu_kmemoryread_d(pir + off);
}

static void h9x_w8(UINT32 pir, UINT32 off, UINT8 v)
{
	cpu_kmemorywrite(pir + off, v);
}

static void h9x_w16(UINT32 pir, UINT32 off, UINT16 v)
{
	cpu_kmemorywrite_w(pir + off, v);
}

static void h9x_w32(UINT32 pir, UINT32 off, UINT32 v)
{
	cpu_kmemorywrite_d(pir + off, v);
}

static void h9x_set_error(UINT32 pir, UINT16 error)
{
	h9x_w16(pir, H9X_IR_ERROR, error);
}

static UINT16 h9x_map_error(DWORD error)
{
	switch (error) {
	case ERROR_SUCCESS: return H9X_ERROR_SUCCESS;
	case ERROR_FILE_NOT_FOUND: return H9X_ERROR_FILE_NOT_FOUND;
	case ERROR_PATH_NOT_FOUND: return H9X_ERROR_PATH_NOT_FOUND;
	case ERROR_TOO_MANY_OPEN_FILES: return H9X_ERROR_TOO_MANY_OPEN_FILES;
	case ERROR_ACCESS_DENIED: return H9X_ERROR_ACCESS_DENIED;
	case ERROR_INVALID_HANDLE: return H9X_ERROR_INVALID_HANDLE;
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY: return H9X_ERROR_NOT_ENOUGH_MEMORY;
	case ERROR_INVALID_DATA: return H9X_ERROR_INVALID_DATA;
	case ERROR_INVALID_DRIVE: return H9X_ERROR_INVALID_DRIVE;
	case ERROR_NO_MORE_FILES: return H9X_ERROR_NO_MORE_FILES;
	case ERROR_WRITE_PROTECT: return H9X_ERROR_WRITE_PROTECT;
	case ERROR_NOT_READY: return H9X_ERROR_NOT_READY;
	case ERROR_BAD_NETPATH: return H9X_ERROR_BAD_NETPATH;
	case ERROR_BAD_NET_NAME: return H9X_ERROR_BAD_NET_NAME;
	case ERROR_SHARING_VIOLATION: return H9X_ERROR_SHARING_VIOLATION;
	case ERROR_LOCK_VIOLATION: return H9X_ERROR_LOCK_VIOLATION;
	case ERROR_HANDLE_EOF: return H9X_ERROR_HANDLE_EOF;
	case ERROR_NOT_SUPPORTED: return H9X_ERROR_NOT_SUPPORTED;
	case ERROR_FILE_EXISTS: return H9X_ERROR_FILE_EXISTS;
	case ERROR_CANNOT_MAKE: return H9X_ERROR_CANNOT_MAKE;
	case ERROR_INVALID_PARAMETER: return H9X_ERROR_INVALID_PARAMETER;
	case ERROR_DISK_FULL: return H9X_ERROR_DISK_FULL;
	case ERROR_DIR_NOT_EMPTY: return H9X_ERROR_DIR_NOT_EMPTY;
	case ERROR_ALREADY_EXISTS: return H9X_ERROR_ALREADY_EXISTS;
	case ERROR_FILENAME_EXCED_RANGE: return H9X_ERROR_FILENAME_EXCED_RANGE;
	default: return (UINT16)((error <= 0xffff) ? error : H9X_ERROR_INVALID_FUNCTION);
	}
}

static UINT16 h9x_last_error(void)
{
	return h9x_map_error(GetLastError());
}

// 予約名か確認
static int h9x_is_reserved_name(const WCHAR *name)
{
	WCHAR base[16];
	int i = 0;
	while (*name && *name != HD_WC('.') && i < 15) {
		base[i++] = (WCHAR)towupper(*name++);
	}
	base[i] = 0;
	if (!wcscmp(base, HD_W("CON")) || !wcscmp(base, HD_W("PRN")) ||
		!wcscmp(base, HD_W("AUX")) || !wcscmp(base, HD_W("NUL"))) return 1;
	if (i == 4 && (!wcsncmp(base, HD_W("COM"), 3) || !wcsncmp(base, HD_W("LPT"), 3)) &&
		base[3] >= HD_WC('1') && base[3] <= HD_WC('9')) return 1;
	return 0;
}

static int h9x_element_equal(const WCHAR *p, const WCHAR *s)
{
	return _wcsicmp(p, s) == 0;
}

// 一致判定　頭に\があるのは無視して比較
static int h9x_resource_element_equal(const WCHAR *p, const WCHAR *s)
{
	if (!p || !s) return 0;
	while (*p == HD_WC('\\') || *p == HD_WC('/')) p++;
	return h9x_element_equal(p, s);
}


static int h9x_read_path_element(UINT32 ppath, UINT32 *poff, WCHAR *dst, UINT32 dstChars)
{
	UINT16 total;
	UINT16 elen;
	UINT32 chars;
	UINT32 i;
	UINT32 off;

	if (!ppath || !poff || !dst || dstChars < 2) return 0;
	total = cpu_kmemoryread_w(ppath);
	off = *poff;
	if (total < 4 || off + 2 > (UINT32)total + 2) return 0;
	elen = cpu_kmemoryread_w(ppath + off);
	if (elen < 2 || off + elen > (UINT32)total + 2) return 0;
	chars = (elen - 2) / 2;
	if (chars >= dstChars) return 0;
	for (i = 0; i < chars; i++) dst[i] = cpu_kmemoryread_w(ppath + off + 2 + i * 2);
	while (chars && dst[chars - 1] == 0) chars--;
	dst[chars] = 0;
	*poff = off + elen;
	return chars != 0;
}


// 元のパス文字列を読む　大文字小文字の状態を維持するために使用
static int h9x_read_unparsed_leaf(UINT32 pir, WCHAR *leaf, UINT32 leafChars)
{
	UINT32 address;
	WCHAR path[MAX_PATH];
	WCHAR c;
	const WCHAR *start;
	const WCHAR *end;
	const WCHAR *q;
	UINT32 i;
	UINT32 chars;

	if (!leaf || leafChars < 2) return 0;
	leaf[0] = 0;
	address = h9x_r32(pir, H9X_IR_UPATH);
	if (!address || address == 0xffffffffUL) return 0;

	for (i = 0; i < NELEMENTS(path) - 1; i++) {
		c = (WCHAR)cpu_kmemoryread_w(address + i * 2);
		path[i] = c;
		if (!c) break;
	}
	if (i == NELEMENTS(path) - 1) {
		path[i] = 0;
		return 0;
	}

	end = path + i;
	while (end > path && (end[-1] == HD_WC('\\') || end[-1] == HD_WC('/'))) end--;
	if (end == path) return 0;
	start = end;
	while (start > path && start[-1] != HD_WC('\\') && start[-1] != HD_WC('/')) start--;
	chars = (UINT32)(end - start);
	if (!chars || chars >= leafChars) return 0;
	memcpy(leaf, start, chars * sizeof(WCHAR));
	leaf[chars] = 0;

	if (!wcscmp(leaf, HD_W(".")) || !wcscmp(leaf, HD_W("..")) || h9x_is_reserved_name(leaf))
		return 0;
	for (q = leaf; *q; q++) {
		if (*q == HD_WC('\\') || *q == HD_WC('/') || *q == HD_WC(':') || *q == HD_WC('*') || *q == HD_WC('?'))
			return 0;
	}
	return 1;
}

static void h9x_preserve_unparsed_leaf_case(UINT32 pir, WCHAR *path)
{
	WCHAR leaf[MAX_PATH];
	WCHAR *base;
	size_t prefix;

	if (!path || !*path || !h9x_read_unparsed_leaf(pir, leaf, NELEMENTS(leaf)))
		return;
	base = wcsrchr(path, HD_WC('\\'));
	if (!base) base = wcsrchr(path, HD_WC('/'));
	base = base ? base + 1 : path;

	// パスが同じであれば大文字小文字の復元を行う
	if (_wcsicmp(base, leaf) != 0) return;
	prefix = (size_t)(base - path);
	if (prefix + wcslen(leaf) >= MAX_PATH) return;
	wcscpy(base, leaf);
}

static int h9x_is_our_resource(UINT32 ppath)
{
	WCHAR server[32];
	WCHAR share[32];
	UINT32 off = 4;
	if (!h9x_read_path_element(ppath, &off, server, NELEMENTS(server))) return 0;
	if (!h9x_read_path_element(ppath, &off, share, NELEMENTS(share))) return 0;
	return h9x_resource_element_equal(server, HD_W("NP2HOST")) &&
		h9x_resource_element_equal(share, HD_W("HOSTFS"));
}

// IFSMgrのParsedPathをホストのパスへ変換
static UINT16 h9x_path_from_parsed(UINT32 ppath, WCHAR *dst, int allowWildcard)
{
	UINT16 total;
	UINT32 off;
	UINT32 elementIndex = 0;
	UINT32 skipElements = 0;
	WCHAR element[MAX_PATH];
	size_t used;

	// パスが異常だったり長さが異常だったりした場合は不可
	if (!ppath || !dst || !s_h9xRoot[0]) return H9X_ERROR_PATH_NOT_FOUND;
	total = cpu_kmemoryread_w(ppath);
	if (total < 4 || total > 0x4000 || (total & 1)) return H9X_ERROR_INVALID_DATA;

	// 先頭がNP2HOST\HOSTFS\だった場合は除去してアクセス
	if (h9x_is_our_resource(ppath))
		skipElements = 2;

	// HOSTDRVルートから順に処理
	wcscpy(dst, s_h9xRoot);
	used = wcslen(dst);
	off = 4;
	for (;;) {
		UINT16 elen;
		UINT32 chars;
		UINT32 i;
		const WCHAR *q;
		int wildcard = 0;

		// 領域確認　範囲外ならエラー
		if (off + 2 > (UINT32)total + 2) return H9X_ERROR_INVALID_DATA;

		// データ長を読み取り 0は終端 最後にtotalと一致していなければ異常なのでエラー
		elen = cpu_kmemoryread_w(ppath + off);
		if (!elen) {
			if (off != total) return H9X_ERROR_INVALID_DATA;
			break;
		}

		// 短すぎるデータは異常、UTF-16なので奇数は異常、totalを超えたら異常なのでエラー
		if (elen < 4 || (elen & 1) || off + elen > (UINT32)total)
			return H9X_ERROR_INVALID_DATA;

		// UTF-16文字数を計算 WORDの長さフィールド分である2を引く
		chars = (elen - 2) / 2;

		// パスが長すぎるのは不可
		if (chars >= MAX_PATH) return H9X_ERROR_FILENAME_EXCED_RANGE;

		// UTF-16読み込み
		for (i = 0; i < chars; i++) element[i] = cpu_kmemoryread_w(ppath + off + 2 + i * 2);
		
		// 末尾NULL文字を消す
		while (chars && element[chars - 1] == 0) chars--;
		if (!chars) return H9X_ERROR_INVALID_DATA;
		element[chars] = 0;

		// スキップ分を飛ばして読み取り開始
		if (elementIndex >= skipElements) {
			// .と..は不可
			if (!wcscmp(element, HD_W(".")) || !wcscmp(element, HD_W(".."))) return H9X_ERROR_ACCESS_DENIED;
			
			// 予約名を拒否
			if (h9x_is_reserved_name(element)) return H9X_ERROR_ACCESS_DENIED;
			
			// 特殊文字を判定
			for (q = element; *q; q++) {
				if (*q == HD_WC('\\') || *q == HD_WC('/') || *q == HD_WC(':')) return H9X_ERROR_ACCESS_DENIED;
				if (*q == HD_WC('*') || *q == HD_WC('?')) wildcard = 1;
			}

			// ワイルドカードを除外
			if (wildcard && (!allowWildcard || off + elen != (UINT32)total))
				return H9X_ERROR_INVALID_PARAMETER;
			
			// 異常な長さは不可
			if (used + 1 + chars >= MAX_PATH) return H9X_ERROR_FILENAME_EXCED_RANGE;
			
			// 特殊ファイルでないことを確認
			dst[used++] = HD_WC('\\');
			wcscpy(dst + used, element);
			used += chars;
			if (!wildcard) {
				DWORD pathAttr = GetFileAttributesW(dst);
				if (pathAttr != INVALID_FILE_ATTRIBUTES &&
					(pathAttr & FILE_ATTRIBUTE_REPARSE_POINT))
					return H9X_ERROR_ACCESS_DENIED;
			}
		}
		elementIndex++;
		off += elen;
	}
	return H9X_ERROR_SUCCESS;
}


// 短いファイル名マップ生成
#if !defined(WIN32) && !defined(_WIN32)
static int h9x_sfn_wtooem(const WCHAR *src, OEMCHAR *dst, UINT cchDst)
{
	if (!src || !dst || cchDst == 0) return 0;
	return WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, (int)cchDst, NULL, NULL) != 0;
}

static int h9x_sfn_oemtow(const OEMCHAR *src, WCHAR *dst, UINT cchDst)
{
	if (!src || !dst || cchDst == 0) return 0;
	return MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, (int)cchDst) != 0;
}
#endif

static UINT16 h9x_sfn_build_map(const WCHAR *parent, H9X_SFN_ENTRY **outEntries, UINT32 *outCount)
{
	HDRVSFNENTRY *entries;
	UINT count;

	if (!parent || !outEntries || !outCount) return H9X_ERROR_INVALID_PARAMETER;
	*outEntries = NULL;
	*outCount = 0;
	if (wcslen(parent) >= MAX_PATH) return H9X_ERROR_FILENAME_EXCED_RANGE;
#if defined(WIN32) || defined(_WIN32)
	if (hostdrvs_getshortnamemap(parent, &entries, &count) != SUCCESS)
		return H9X_ERROR_NOT_ENOUGH_MEMORY;
#else
	{
		OEMCHAR oemParent[MAX_PATH * 4];
		if (!h9x_sfn_wtooem(parent, oemParent, NELEMENTS(oemParent)) ||
			hostdrvs_getshortnamemap(oemParent, &entries, &count) != SUCCESS)
			return H9X_ERROR_NOT_ENOUGH_MEMORY;
	}
#endif
	*outEntries = entries;
	*outCount = (UINT32)count;
	return H9X_ERROR_SUCCESS;
}

static int h9x_sfn_lookup_long(const H9X_SFN_ENTRY *entries, UINT32 count,
	const WCHAR *longName, WCHAR *shortName, UINT32 shortChars)
{
#if defined(WIN32) || defined(_WIN32)
	return hostdrvs_lookupshortname(entries, (UINT)count, longName, shortName,
		(UINT)shortChars) ? 1 : 0;
#else
	OEMCHAR oemLong[MAX_PATH * 4];
	OEMCHAR oemShort[64];
	if (!h9x_sfn_wtooem(longName, oemLong, NELEMENTS(oemLong))) return 0;
	if (!hostdrvs_lookupshortname(entries, (UINT)count, oemLong, oemShort,
		NELEMENTS(oemShort))) return 0;
	return h9x_sfn_oemtow(oemShort, shortName, shortChars);
#endif
}

static int h9x_sfn_lookup_short(const H9X_SFN_ENTRY *entries, UINT32 count,
	const WCHAR *shortName, WCHAR *longName, UINT32 longChars, DWORD *attributes)
{
	UINT32 attr;
	BOOL found;
	attr = 0;
#if defined(WIN32) || defined(_WIN32)
	found = hostdrvs_lookuplongname(entries, (UINT)count, shortName, longName,
		(UINT)longChars, &attr);
#else
	{
		OEMCHAR oemShort[64];
		OEMCHAR oemLong[MAX_PATH * 4];
		if (!h9x_sfn_wtooem(shortName, oemShort, NELEMENTS(oemShort))) return 0;
		found = hostdrvs_lookuplongname(entries, (UINT)count, oemShort, oemLong,
			NELEMENTS(oemLong), &attr);
		if (found && !h9x_sfn_oemtow(oemLong, longName, longChars)) found = FALSE;
	}
#endif
	if (found && attributes) *attributes = (DWORD)attr;
	return found ? 1 : 0;
}

static UINT16 h9x_sfn_get_short(const WCHAR *parent, const WCHAR *longName,
	WCHAR *shortName, UINT32 shortChars)
{
	if (!parent || !longName || !shortName || shortChars < 2)
		return H9X_ERROR_INVALID_PARAMETER;
#if defined(WIN32) || defined(_WIN32)
	return hostdrvs_lookupshortnamecached(parent, longName, shortName,
		(UINT)shortChars) ? H9X_ERROR_SUCCESS : H9X_ERROR_FILE_NOT_FOUND;
#else
	{
		OEMCHAR oemParent[MAX_PATH * 4];
		OEMCHAR oemLong[MAX_PATH * 4];
		OEMCHAR oemShort[64];
		if (!h9x_sfn_wtooem(parent, oemParent, NELEMENTS(oemParent)) ||
			!h9x_sfn_wtooem(longName, oemLong, NELEMENTS(oemLong)) ||
			!hostdrvs_lookupshortnamecached(oemParent, oemLong, oemShort, NELEMENTS(oemShort)) ||
			!h9x_sfn_oemtow(oemShort, shortName, shortChars))
			return H9X_ERROR_FILE_NOT_FOUND;
		return H9X_ERROR_SUCCESS;
	}
#endif
}

static UINT16 h9x_sfn_get_long(const WCHAR *parent, const WCHAR *shortName,
	WCHAR *longName, UINT32 longChars, DWORD *attributes)
{
	UINT32 attr;
	if (!parent || !shortName || !longName || longChars < 2)
		return H9X_ERROR_INVALID_PARAMETER;
	attr = 0;
#if defined(WIN32) || defined(_WIN32)
	if (!hostdrvs_lookuplongnamecached(parent, shortName, longName,
		(UINT)longChars, &attr))
		return H9X_ERROR_FILE_NOT_FOUND;
#else
	{
		OEMCHAR oemParent[MAX_PATH * 4];
		OEMCHAR oemShort[64];
		OEMCHAR oemLong[MAX_PATH * 4];
		if (!h9x_sfn_wtooem(parent, oemParent, NELEMENTS(oemParent)) ||
			!h9x_sfn_wtooem(shortName, oemShort, NELEMENTS(oemShort)) ||
			!hostdrvs_lookuplongnamecached(oemParent, oemShort, oemLong, NELEMENTS(oemLong), &attr) ||
			!h9x_sfn_oemtow(oemLong, longName, longChars))
			return H9X_ERROR_FILE_NOT_FOUND;
	}
#endif
	if (attributes) *attributes = (DWORD)attr;
	return H9X_ERROR_SUCCESS;
}

static int h9x_find_parent_path(const WCHAR *searchPath, WCHAR *parent, UINT32 parentChars)
{
	WCHAR *slash;

	if (!searchPath || !*searchPath || !parent || parentChars < 2) return 0;
	wcsncpy(parent, searchPath, parentChars - 1);
	parent[parentChars - 1] = 0;
	slash = wcsrchr(parent, HD_WC('\\'));
	if (!slash) return 0;
	if (slash == parent + 2 && parent[1] == HD_WC(':')) slash[1] = 0;
	else *slash = 0;
	return 1;
}

// SFNマップを確実に作成
static UINT16 h9x_find_ensure_sfn_map(NP2HOSTDRV9X_HANDLE *h)
{
	WCHAR parent[MAX_PATH];
	UINT16 error;

	if (!h) return H9X_ERROR_INVALID_PARAMETER;
	if (h->sfnMapBuilt) return H9X_ERROR_SUCCESS;
	if (!h9x_find_parent_path(h->path, parent, NELEMENTS(parent)))
		return H9X_ERROR_PATH_NOT_FOUND;
	if (!h9x_is_safe_stored_path(parent)) return H9X_ERROR_ACCESS_DENIED;
	error = h9x_sfn_build_map(parent, &h->sfnMap, &h->sfnCount);
	if (!error) h->sfnMapBuilt = 1;
	return error;
}

// Find時にSFNを取得
static int h9x_find_get_short_name(NP2HOSTDRV9X_HANDLE *h,
	const WIN32_FIND_DATAW *fd, WCHAR *shortName, UINT32 shortChars)
{
	UINT16 error;

	if (!h || !fd || !shortName || shortChars < 2) return 0;
	error = h9x_find_ensure_sfn_map(h);
	if (!error && h9x_sfn_lookup_long(h->sfnMap, h->sfnCount, fd->cFileName,
		shortName, shortChars))
		return 1;
	return 0;
}

// パス解決
static UINT16 h9x_resolve_existing_component(const WCHAR *parent,
	const WCHAR *input, int requireDirectory, WCHAR *resolved, UINT32 resolvedChars)
{
	WCHAR path[MAX_PATH];
	WIN32_FIND_DATAW fd;
	HANDLE find;
	DWORD mappedAttr = 0;
	UINT16 mapped;
	size_t used;

	if (!parent || !input || !*input || !resolved || resolvedChars < 2)
		return H9X_ERROR_INVALID_PARAMETER;

	used = wcslen(parent);
	if (used + 1 + wcslen(input) >= MAX_PATH)
		return H9X_ERROR_FILENAME_EXCED_RANGE;
	wcscpy(path, parent);
	path[used++] = HD_WC('\\');
	wcscpy(path + used, input);

	/* 通常のホストパスとして存在する場合は、その名前を優先する。 */
	find = FindFirstFileW(path, &fd);
	if (find != INVALID_HANDLE_VALUE) {
		FindClose(find);
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
			return H9X_ERROR_ACCESS_DENIED;
		if (requireDirectory && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			return H9X_ERROR_PATH_NOT_FOUND;
		wcsncpy(resolved, fd.cFileName, resolvedChars - 1);
		resolved[resolvedChars - 1] = 0;
		return H9X_ERROR_SUCCESS;
	}

	/* 通常名で見つからない場合のみ、共通SFNマップでSFN -> LFN変換を試みる。 */
	mapped = h9x_sfn_get_long(parent, input, resolved, resolvedChars, &mappedAttr);
	if (mapped == H9X_ERROR_SUCCESS) {
		used = wcslen(parent);
		if (used + 1 + wcslen(resolved) >= MAX_PATH)
			return H9X_ERROR_FILENAME_EXCED_RANGE;
		wcscpy(path, parent);
		path[used++] = HD_WC('\\');
		wcscpy(path + used, resolved);
		find = FindFirstFileW(path, &fd);
		if (find == INVALID_HANDLE_VALUE) {
			// Findで開けないならキャッシュ無効
			hostdrvs_invalidateshortnamecache();
			return requireDirectory ? H9X_ERROR_PATH_NOT_FOUND : H9X_ERROR_FILE_NOT_FOUND;
		} else {
			FindClose(find);
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
				return H9X_ERROR_ACCESS_DENIED;
			if (requireDirectory && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				return H9X_ERROR_PATH_NOT_FOUND;
			wcsncpy(resolved, fd.cFileName, resolvedChars - 1);
			resolved[resolvedChars - 1] = 0;
			#if defined(_WIN32) || defined(WIN32)
			TRACEOUT(("HOSTDRV9X PATH SFN: parent=[%ls] input=[%ls] -> [%ls]",
				parent, input, resolved));
			#endif
			return H9X_ERROR_SUCCESS;
		}
	}


	return requireDirectory ? H9X_ERROR_PATH_NOT_FOUND : H9X_ERROR_FILE_NOT_FOUND;
}

static UINT16 h9x_path_from_parsed_existing(UINT32 ppath, WCHAR *dst, int allowWildcard)
{
    UINT16 total;
    UINT32 off;
    UINT32 elementIndex = 0;
    UINT32 skipElements = 0;
    WCHAR element[MAX_PATH];
    WCHAR resolved[MAX_PATH];
    size_t used;

    if (!ppath || !dst || !s_h9xRoot[0]) return H9X_ERROR_PATH_NOT_FOUND;
    total = cpu_kmemoryread_w(ppath);
    if (total < 4 || total > 0x4000 || (total & 1)) return H9X_ERROR_INVALID_DATA;

	if (h9x_is_our_resource(ppath))
		skipElements = 2;

    wcscpy(dst, s_h9xRoot);
    off = 4;
    for (;;) {
        UINT16 elen;
        UINT32 chars;
        UINT32 i;
        const WCHAR *q;
        int wildcard = 0;
        int last;
        UINT16 error;

        if (off + 2 > (UINT32)total + 2) return H9X_ERROR_INVALID_DATA;
        elen = cpu_kmemoryread_w(ppath + off);
        if (!elen) {
            if (off != total) return H9X_ERROR_INVALID_DATA;
            break;
        }
        if (elen < 4 || (elen & 1) || off + elen > (UINT32)total)
            return H9X_ERROR_INVALID_DATA;
        chars = (elen - 2) / 2;
        if (chars >= MAX_PATH) return H9X_ERROR_FILENAME_EXCED_RANGE;
        for (i = 0; i < chars; i++) element[i] = cpu_kmemoryread_w(ppath + off + 2 + i * 2);
        while (chars && element[chars - 1] == 0) chars--;
        if (!chars) return H9X_ERROR_INVALID_DATA;
        element[chars] = 0;

        last = (off + elen == (UINT32)total);
        if (elementIndex >= skipElements) {
            if (!wcscmp(element, HD_W(".")) || !wcscmp(element, HD_W("..")))
                return H9X_ERROR_ACCESS_DENIED;
            if (h9x_is_reserved_name(element))
                return H9X_ERROR_ACCESS_DENIED;
            for (q = element; *q; q++) {
                if (*q == HD_WC('\\') || *q == HD_WC('/') || *q == HD_WC(':'))
                    return H9X_ERROR_ACCESS_DENIED;
                if (*q == HD_WC('*') || *q == HD_WC('?')) wildcard = 1;
            }
            if (wildcard) {
                if (!allowWildcard || !last)
                    return H9X_ERROR_INVALID_PARAMETER;
                used = wcslen(dst);
                if (used + 1 + chars >= MAX_PATH)
                    return H9X_ERROR_FILENAME_EXCED_RANGE;
                dst[used++] = HD_WC('\\');
                wcscpy(dst + used, element);
            } else {
                error = h9x_resolve_existing_component(dst, element, !last,
                    resolved, NELEMENTS(resolved));
                if (error) return error;
                used = wcslen(dst);
                if (used + 1 + wcslen(resolved) >= MAX_PATH)
                    return H9X_ERROR_FILENAME_EXCED_RANGE;
                dst[used++] = HD_WC('\\');
                wcscpy(dst + used, resolved);
                {
                    DWORD attrs = GetFileAttributesW(dst);
                    if (attrs != INVALID_FILE_ATTRIBUTES &&
                        (attrs & FILE_ATTRIBUTE_REPARSE_POINT))
                        return H9X_ERROR_ACCESS_DENIED;
                }
            }
        }
        elementIndex++;
        off += elen;
    }
    return H9X_ERROR_SUCCESS;
}

static UINT32 h9x_make_cookie(int index)
{
	return ((UINT32)s_h9x.files[index].generation << 12) | (UINT32)index;
}

static NP2HOSTDRV9X_HANDLE *h9x_get_handle(UINT32 cookie, UINT8 type)
{
	UINT32 index = cookie & 0x0fff;
	UINT16 generation = (UINT16)(cookie >> 12);
	NP2HOSTDRV9X_HANDLE *h;
	if (!index || index >= NP2HOSTDRV9X_FILES_MAX) return NULL;
	h = &s_h9x.files[index];
	if (h->type != type || h->generation != generation) return NULL;
	return h;
}

static NP2HOSTDRV9X_HANDLE *h9x_get_search_handle(UINT16 cookie)
{
	UINT32 index = cookie & 0x0fff;
	UINT16 generation = (UINT16)(cookie >> 12);
	NP2HOSTDRV9X_HANDLE *h;
	if (!index || index >= NP2HOSTDRV9X_FILES_MAX) return NULL;
	h = &s_h9x.files[index];
	if (h->type != H9X_HANDLE_FIND || (h->generation & 0x000f) != generation) return NULL;
	return h;
}

static int h9x_alloc_handle(UINT8 type)
{
	int i;
	for (i = 1; i < NP2HOSTDRV9X_FILES_MAX; i++) {
		if (!s_h9x.files[i].type) {
			UINT16 generation = (UINT16)(s_h9x.files[i].generation + 1);
			if (!generation) generation = 1;
			ZeroMemory(&s_h9x.files[i], sizeof(s_h9x.files[i]));
			s_h9x.files[i].generation = generation;
			s_h9x.files[i].type = type;
			s_h9x.files[i].handle = INVALID_HANDLE_VALUE;
			return i;
		}
	}
	return -1;
}

static void h9x_free_handle(NP2HOSTDRV9X_HANDLE *h)
{
	UINT16 generation;
	if (!h) return;
	generation = h->generation;
	if (h->sfnMap) {
		hostdrvs_freeshortnamemap(h->sfnMap);
		h->sfnMap = NULL;
		h->sfnCount = 0;
	}
	if (h->handle != NULL && h->handle != INVALID_HANDLE_VALUE) {
		if (h->type == H9X_HANDLE_FIND) FindClose(h->handle);
		else CloseHandle(h->handle);
	}
	ZeroMemory(h, sizeof(*h));
	h->generation = generation;
	h->handle = INVALID_HANDLE_VALUE;
}

static void h9x_close_all(void)
{
	int i;
	for (i = 1; i < NP2HOSTDRV9X_FILES_MAX; i++) {
		if (s_h9x.files[i].type) h9x_free_handle(&s_h9x.files[i]);
	}
}

static UINT32 h9x_attrs_from_win(DWORD attr)
{
	UINT32 out = 0;
	if (attr & FILE_ATTRIBUTE_READONLY) out |= H9X_FILE_ATTRIBUTE_READONLY;
	if (attr & FILE_ATTRIBUTE_HIDDEN) out |= H9X_FILE_ATTRIBUTE_HIDDEN;
	if (attr & FILE_ATTRIBUTE_SYSTEM) out |= H9X_FILE_ATTRIBUTE_SYSTEM;
	if (attr & FILE_ATTRIBUTE_DIRECTORY) out |= H9X_FILE_ATTRIBUTE_DIRECTORY;
	if (attr & FILE_ATTRIBUTE_ARCHIVE) out |= H9X_FILE_ATTRIBUTE_ARCHIVE;
	return out;
}

static DWORD h9x_attrs_to_win(UINT32 attr, DWORD oldattr)
{
	DWORD out = oldattr & ~(FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY |
		FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);
	if (attr & H9X_FILE_ATTRIBUTE_READONLY) out |= FILE_ATTRIBUTE_READONLY;
	if (attr & H9X_FILE_ATTRIBUTE_HIDDEN) out |= FILE_ATTRIBUTE_HIDDEN;
	if (attr & H9X_FILE_ATTRIBUTE_SYSTEM) out |= FILE_ATTRIBUTE_SYSTEM;
	if (attr & H9X_FILE_ATTRIBUTE_ARCHIVE) out |= FILE_ATTRIBUTE_ARCHIVE;
	if (!(out & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY |
		FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE)))
		out |= FILE_ATTRIBUTE_NORMAL;
	return out;
}

static void h9x_filetime_to_dos(const FILETIME *ft, UINT16 *date, UINT16 *time)
{
	FILETIME local;
	WORD d = 0, t = 0;
	if (FileTimeToLocalFileTime(ft, &local)) FileTimeToDosDateTime(&local, &d, &t);
	*date = d;
	*time = t;
}

static int h9x_dos_to_filetime(UINT16 date, UINT16 time, FILETIME *ft)
{
	FILETIME local;
	if (!DosDateTimeToFileTime(date, time, &local)) return 0;
	return LocalFileTimeToFileTime(&local, ft);
}

static void h9x_write_dostime(UINT32 pir, const FILETIME *ft)
{
	UINT16 date, time;
	h9x_filetime_to_dos(ft, &date, &time);
	h9x_w16(pir, H9X_IR_DOSTIME + 0, time);
	h9x_w16(pir, H9X_IR_DOSTIME + 2, date);
}

static UINT16 h9x_open(UINT32 pir)
{
	WCHAR path[MAX_PATH];
	DWORD access = 0;
	DWORD share = 0;
	DWORD disposition;
	DWORD flags = FILE_ATTRIBUTE_NORMAL;
	DWORD existingAttr;
	DWORD lastError;
	int namespaceChanged = 0;
	UINT8 mode = h9x_r8(pir, H9X_IR_FLAGS);
	UINT16 options = h9x_r16(pir, H9X_IR_OPTIONS);
	UINT32 attr = h9x_r32(pir, H9X_IR_ATTR);
	int index;
	HANDLE file;
	BY_HANDLE_FILE_INFORMATION info;
	UINT16 error = h9x_path_from_parsed(h9x_r32(pir, H9X_IR_PPATH), path, 0);
	if (error) return error;

	switch (mode & H9X_ACCESS_MODE_MASK) {
	case H9X_ACCESS_READONLY:
	case H9X_ACCESS_EXECUTE: access = GENERIC_READ; break;
	case H9X_ACCESS_WRITEONLY: access = GENERIC_WRITE; break;
	case H9X_ACCESS_READWRITE: access = GENERIC_READ | GENERIC_WRITE; break;
	default: return H9X_ERROR_INVALID_PARAMETER;
	}
	if ((access & GENERIC_READ) && !(s_h9xAcc & H9X_PERMIT_READ)) return H9X_ERROR_ACCESS_DENIED;
	if ((access & GENERIC_WRITE) && !(s_h9xAcc & H9X_PERMIT_WRITE)) return H9X_ERROR_WRITE_PROTECT;

	switch (mode & H9X_SHARE_MODE_MASK) {
	case H9X_SHARE_DENYREADWRITE: share = 0; break;
	case H9X_SHARE_DENYWRITE: share = FILE_SHARE_READ; break;
	case H9X_SHARE_DENYREAD: share = FILE_SHARE_WRITE; break;
	case H9X_SHARE_DENYNONE: share = FILE_SHARE_READ | FILE_SHARE_WRITE; break;
	case H9X_SHARE_COMPATIBILITY:
	case H9X_SHARE_FCB: share = FILE_SHARE_READ | FILE_SHARE_WRITE; break;
	default: share = FILE_SHARE_READ; break;
	}

	switch (options & H9X_ACTION_MASK) {
	case H9X_ACTION_OPENEXISTING: disposition = OPEN_EXISTING; break;
	case H9X_ACTION_REPLACEEXISTING: disposition = TRUNCATE_EXISTING; break;
	case H9X_ACTION_CREATENEW: disposition = CREATE_NEW; break;
	case H9X_ACTION_OPENALWAYS: disposition = OPEN_ALWAYS; break;
	case H9X_ACTION_CREATEALWAYS: disposition = CREATE_ALWAYS; break;
	default: return H9X_ERROR_INVALID_PARAMETER;
	}
	if (disposition != OPEN_EXISTING && !(s_h9xAcc & H9X_PERMIT_WRITE)) return H9X_ERROR_WRITE_PROTECT;

	/* ParsedPathがSFNで渡された場合はパス解決する */
	if (disposition != CREATE_NEW) {
		WCHAR resolvedPath[MAX_PATH];
		UINT16 resolveError = h9x_path_from_parsed_existing(h9x_r32(pir, H9X_IR_PPATH), resolvedPath, 0);
		if (!resolveError) {
			if (_wcsicmp(path, resolvedPath)) {
				TRACEOUT(("HOSTDRV9X OPEN: resolved existing path [%ls] -> [%ls]",
					path, resolvedPath));
			}
			wcscpy(path, resolvedPath);
		} else if (resolveError != H9X_ERROR_FILE_NOT_FOUND || (disposition != OPEN_ALWAYS && disposition != CREATE_ALWAYS)) {
			return resolveError;
		}
	}

	existingAttr = GetFileAttributesW(path);
	if (existingAttr == INVALID_FILE_ATTRIBUTES &&
		(disposition == CREATE_NEW || disposition == OPEN_ALWAYS || disposition == CREATE_ALWAYS)) {
		/* ParsedPathは大文字変換されている場合があるので、可能な限り元の大文字小文字を維持する */
		h9x_preserve_unparsed_leaf_case(pir, path);
	}
	if (existingAttr != INVALID_FILE_ATTRIBUTES && (existingAttr & FILE_ATTRIBUTE_DIRECTORY))
		flags |= FILE_FLAG_BACKUP_SEMANTICS;
	else
		flags = h9x_attrs_to_win(attr, FILE_ATTRIBUTE_NORMAL);

	/* ハンドル割り当てを先にする。CREATE_ALWAYS/TRUNCATE_EXISTING後にハンドル枯渇で失敗すると、ファイル内容だけ消える可能性がある */
	index = h9x_alloc_handle(H9X_HANDLE_FILE);
	if (index < 0) return H9X_ERROR_TOO_MANY_OPEN_FILES;

	file = CreateFileW(path, access, share, NULL, disposition, flags, NULL);
	lastError = GetLastError();
	if (file == INVALID_HANDLE_VALUE) {
		h9x_free_handle(&s_h9x.files[index]);
		return h9x_map_error(lastError);
	}
	if (disposition == CREATE_NEW ||
		((disposition == OPEN_ALWAYS || disposition == CREATE_ALWAYS) &&
		 lastError != ERROR_ALREADY_EXISTS))
		namespaceChanged = 1;
	if (namespaceChanged) hostdrvs_invalidateshortnamecache();
	s_h9x.files[index].handle = file;
	s_h9x.files[index].desiredAccess = access;
	s_h9x.files[index].shareMode = share;
	wcsncpy(s_h9x.files[index].path, path, MAX_PATH - 1);

	h9x_w32(pir, H9X_IR_FH, h9x_make_cookie(index));
	if (GetFileInformationByHandle(file, &info)) {
		h9x_w32(pir, H9X_IR_SIZE, info.nFileSizeLow);
		h9x_w32(pir, H9X_IR_ATTR, h9x_attrs_from_win(info.dwFileAttributes));
		h9x_write_dostime(pir, &info.ftLastWriteTime);
	}
	if ((options & H9X_ACTION_MASK) == H9X_ACTION_OPENALWAYS) {
		h9x_w16(pir, H9X_IR_OPTIONS, (lastError == ERROR_ALREADY_EXISTS) ? H9X_ACTION_OPENED : H9X_ACTION_CREATED);
	} else if ((options & H9X_ACTION_MASK) == H9X_ACTION_CREATEALWAYS) {
		h9x_w16(pir, H9X_IR_OPTIONS, (lastError == ERROR_ALREADY_EXISTS) ? H9X_ACTION_REPLACED : H9X_ACTION_CREATED);
	} else if ((options & H9X_ACTION_MASK) == H9X_ACTION_REPLACEEXISTING) {
		h9x_w16(pir, H9X_IR_OPTIONS, H9X_ACTION_REPLACED);
	} else if ((options & H9X_ACTION_MASK) == H9X_ACTION_CREATENEW) {
		h9x_w16(pir, H9X_IR_OPTIONS, H9X_ACTION_CREATED);
	} else {
		h9x_w16(pir, H9X_IR_OPTIONS, H9X_ACTION_OPENED);
	}
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_close(UINT32 pir)
{
	NP2HOSTDRV9X_HANDLE *h = h9x_get_handle(h9x_r32(pir, H9X_IR_FH), H9X_HANDLE_FILE);
	UINT8 closeType = h9x_r8(pir, H9X_IR_FLAGS);
	if (!h) return H9X_ERROR_INVALID_HANDLE;
	/* 最後のハンドルが閉じられた時に閉じる */
	if (closeType == H9X_CLOSE_FINAL) h9x_free_handle(h);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_readwrite(UINT32 pir, int write)
{
	NP2HOSTDRV9X_HANDLE *h = h9x_get_handle(h9x_r32(pir, H9X_IR_FH), H9X_HANDLE_FILE);
	UINT32 address = h9x_r32(pir, H9X_IR_DATA);
	UINT32 requested = h9x_r32(pir, H9X_IR_LENGTH);
	UINT32 position = h9x_r32(pir, H9X_IR_POS);
	UINT32 doneTotal = 0;
	UINT8 *buffer;
	LONG high = 0;

	TRACEOUT(("HOSTDRV9X %s: enter pir=%08X fh=%08X guest=%08X requested=%u pos=%u",
		write ? "WRITE" : "READ",
		(unsigned)pir,
		(unsigned)h9x_r32(pir, H9X_IR_FH),
		(unsigned)address,
		(unsigned)requested,
		(unsigned)position));

	if (!h) {
		TRACEOUT(("HOSTDRV9X %s: invalid handle", write ? "WRITE" : "READ"));
		return H9X_ERROR_INVALID_HANDLE;
	}
	if (write && !(s_h9xAcc & H9X_PERMIT_WRITE)) {
		TRACEOUT(("HOSTDRV9X WRITE: write protected"));
		return H9X_ERROR_WRITE_PROTECT;
	}
	if (!write && !(s_h9xAcc & H9X_PERMIT_READ)) {
		TRACEOUT(("HOSTDRV9X READ: read access denied"));
		return H9X_ERROR_ACCESS_DENIED;
	}
	if (!address && requested) {
		TRACEOUT(("HOSTDRV9X %s: NULL guest buffer", write ? "WRITE" : "READ"));
		return H9X_ERROR_INVALID_PARAMETER;
	}
	if (requested && address + requested - 1 < address) {
		TRACEOUT(("HOSTDRV9X %s: guest buffer wraps address space", write ? "WRITE" : "READ"));
		return H9X_ERROR_INVALID_PARAMETER;
	}

	/* 先にメモリアクセスをする。読み込み中にページフォールトが出て中途半端になるのを避ける。 */
	if (requested) {
		TRACEOUT(("HOSTDRV9X %s: preflight request begin guest=%08X len=%u access=%s",
			write ? "WRITE" : "READ",
			(unsigned)address,
			(unsigned)requested,
			write ? "guest-read" : "guest-write"));
		h9x_preflight_linear(address, requested, write ? 0 : 1);
		TRACEOUT(("HOSTDRV9X %s: preflight request done guest=%08X len=%u",
			write ? "WRITE" : "READ", (unsigned)address, (unsigned)requested));
	}

	TRACEOUT(("HOSTDRV9X %s: SetFilePointer begin handle=%p pos=%u",
		write ? "WRITE" : "READ", h->handle, (unsigned)position));
	if (SetFilePointer(h->handle, (LONG)position, &high, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
		GetLastError() != NO_ERROR) {
		UINT16 e = h9x_last_error();
		TRACEOUT(("HOSTDRV9X %s: SetFilePointer failed error=%u winerr=%u",
			write ? "WRITE" : "READ", (unsigned)e, (unsigned)GetLastError()));
		return e;
	}
	TRACEOUT(("HOSTDRV9X %s: SetFilePointer done", write ? "WRITE" : "READ"));

	/* DOS/IFSMgrはサイズ0で書き込むとその位置にEOFを設定する。WinAPI WriteFileはそれをしないので、SetEndOfFileで設定 */
	if (write && requested == 0) {
		BOOL ok;

		TRACEOUT(("HOSTDRV9X WRITE: zero-length write -> SetEndOfFile at pos=%u",
			(unsigned)position));
		ok = SetEndOfFile(h->handle);
		if (!ok) {
			UINT16 error = h9x_last_error();
			TRACEOUT(("HOSTDRV9X WRITE: SetEndOfFile failed error=%u winerr=%u",
				(unsigned)error, (unsigned)GetLastError()));
			h9x_w32(pir, H9X_IR_LENGTH, 0);
			return error;
		}

		h9x_w32(pir, H9X_IR_LENGTH, 0);
		h9x_w32(pir, H9X_IR_POS, position);
		TRACEOUT(("HOSTDRV9X WRITE: SetEndOfFile success new-eof=%u",
			(unsigned)position));
		return H9X_ERROR_SUCCESS;
	}

	buffer = (UINT8 *)malloc(0x10000);
	if (!buffer) {
		TRACEOUT(("HOSTDRV9X %s: malloc failed", write ? "WRITE" : "READ"));
		return H9X_ERROR_NOT_ENOUGH_MEMORY;
	}

	while (doneTotal < requested) {
		DWORD chunk = requested - doneTotal;
		DWORD done = 0;
		BOOL ok;
		UINT32 guest = address + doneTotal;

		if (chunk > 0x10000) chunk = 0x10000;


		if (write) {
			TRACEOUT(("HOSTDRV9X WRITE: guest copy-in begin guest=%08X chunk=%u",
				(unsigned)guest, (unsigned)chunk));
			h9x_memread(guest, buffer, chunk);
			TRACEOUT(("HOSTDRV9X WRITE: guest copy-in done"));

			TRACEOUT(("HOSTDRV9X WRITE: WriteFile begin chunk=%u", (unsigned)chunk));
			ok = WriteFile(h->handle, buffer, chunk, &done, NULL);
			TRACEOUT(("HOSTDRV9X WRITE: WriteFile end ok=%d done=%u winerr=%u",
				(int)ok, (unsigned)done, (unsigned)(ok ? ERROR_SUCCESS : GetLastError())));
		} else {
			TRACEOUT(("HOSTDRV9X READ: ReadFile begin chunk=%u", (unsigned)chunk));
			ok = ReadFile(h->handle, buffer, chunk, &done, NULL);
			TRACEOUT(("HOSTDRV9X READ: ReadFile end ok=%d done=%u winerr=%u",
				(int)ok, (unsigned)done, (unsigned)(ok ? ERROR_SUCCESS : GetLastError())));

			if (ok && done) {
				TRACEOUT(("HOSTDRV9X READ: guest copy-out begin guest=%08X done=%u",
					(unsigned)guest, (unsigned)done));
				h9x_memwrite(guest, buffer, done);
				TRACEOUT(("HOSTDRV9X READ: guest copy-out done"));
			}
		}

		if (!ok) {
			UINT16 error = h9x_last_error();
			free(buffer);
			h9x_w32(pir, H9X_IR_LENGTH, doneTotal);
			TRACEOUT(("HOSTDRV9X %s: host I/O failed error=%u doneTotal=%u",
				write ? "WRITE" : "READ", (unsigned)error, (unsigned)doneTotal));
			return error;
		}

		doneTotal += done;
		if (done < chunk) break;
	}

	free(buffer);
	h9x_w32(pir, H9X_IR_LENGTH, doneTotal);
	h9x_w32(pir, H9X_IR_POS, position + doneTotal);
	TRACEOUT(("HOSTDRV9X %s: success doneTotal=%u newpos=%u",
		write ? "WRITE" : "READ",
		(unsigned)doneTotal,
		(unsigned)(position + doneTotal)));
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_seek(UINT32 pir)
{
	NP2HOSTDRV9X_HANDLE *h = h9x_get_handle(h9x_r32(pir, H9X_IR_FH), H9X_HANDLE_FILE);
	DWORD method;
	LONG high = 0;
	DWORD pos;
	if (!h) return H9X_ERROR_INVALID_HANDLE;
	if (h9x_r8(pir, H9X_IR_FLAGS) == H9X_FILE_END) method = FILE_END;
	else if (h9x_r8(pir, H9X_IR_FLAGS) == H9X_FILE_BEGIN) method = FILE_BEGIN;
	else return H9X_ERROR_INVALID_PARAMETER;
	SetLastError(NO_ERROR);
	pos = SetFilePointer(h->handle, (LONG)h9x_r32(pir, H9X_IR_POS), &high, method);
	if (pos == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) return h9x_last_error();
	h9x_w32(pir, H9X_IR_POS, pos);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_commit(UINT32 pir)
{
	NP2HOSTDRV9X_HANDLE *h = h9x_get_handle(h9x_r32(pir, H9X_IR_FH), H9X_HANDLE_FILE);
	if (!h) return H9X_ERROR_INVALID_HANDLE;
	return FlushFileBuffers(h->handle) ? H9X_ERROR_SUCCESS : h9x_last_error();
}

static UINT16 h9x_filelocks(UINT32 pir)
{
	NP2HOSTDRV9X_HANDLE *h = h9x_get_handle(h9x_r32(pir, H9X_IR_FH), H9X_HANDLE_FILE);
	DWORD pos = h9x_r32(pir, H9X_IR_POS);
	DWORD len = h9x_r32(pir, H9X_IR_LOCKLEN);
	BOOL ok;
	if (!h) return H9X_ERROR_INVALID_HANDLE;
	if (h9x_r8(pir, H9X_IR_FLAGS) == H9X_UNLOCK_REGION)
		ok = UnlockFile(h->handle, pos, 0, len, 0);
	else if (h9x_r8(pir, H9X_IR_FLAGS) == H9X_LOCK_REGION)
		ok = LockFile(h->handle, pos, 0, len, 0);
	else
		return H9X_ERROR_INVALID_PARAMETER;
	return ok ? H9X_ERROR_SUCCESS : h9x_last_error();
}

static UINT16 h9x_filetimes(UINT32 pir)
{
	NP2HOSTDRV9X_HANDLE *h = h9x_get_handle(h9x_r32(pir, H9X_IR_FH), H9X_HANDLE_FILE);
	FILETIME creation, access, write;
	UINT8 fn = h9x_r8(pir, H9X_IR_FLAGS);
	if (!h) return H9X_ERROR_INVALID_HANDLE;
	if (fn != H9X_GET_MODIFY_DATETIME && fn != H9X_SET_MODIFY_DATETIME &&
		fn != H9X_GET_LAST_ACCESS_DATETIME && fn != H9X_SET_LAST_ACCESS_DATETIME &&
		fn != H9X_GET_CREATION_DATETIME && fn != H9X_SET_CREATION_DATETIME)
		return H9X_ERROR_INVALID_FUNCTION;
	if (!(fn & 1)) {
		if (!GetFileTime(h->handle, &creation, &access, &write)) return h9x_last_error();
		if (fn == H9X_GET_CREATION_DATETIME) h9x_write_dostime(pir, &creation);
		else if (fn == H9X_GET_LAST_ACCESS_DATETIME) h9x_write_dostime(pir, &access);
		else h9x_write_dostime(pir, &write);
		return H9X_ERROR_SUCCESS;
	} else {
		HANDLE file;
		FILETIME value;
		FILETIME *pc = NULL, *pa = NULL, *pw = NULL;
		DWORD attr;
		UINT16 time = h9x_r16(pir, H9X_IR_DOSTIME);
		UINT16 date = h9x_r16(pir, H9X_IR_DOSTIME + 2);
		UINT16 error;
		if (!(s_h9xAcc & H9X_PERMIT_WRITE)) return H9X_ERROR_WRITE_PROTECT;
		if (!h9x_dos_to_filetime(date, time, &value)) return H9X_ERROR_INVALID_PARAMETER;
		if (fn == H9X_SET_CREATION_DATETIME) pc = &value;
		else if (fn == H9X_SET_LAST_ACCESS_DATETIME) pa = &value;
		else pw = &value;
		if (!h9x_is_safe_stored_path(h->path)) return H9X_ERROR_ACCESS_DENIED;
		attr = GetFileAttributesW(h->path);
		if (attr == INVALID_FILE_ATTRIBUTES) return h9x_last_error();
		file = CreateFileW(h->path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, (attr & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : 0, NULL);
		if (file == INVALID_HANDLE_VALUE) return h9x_last_error();
		error = SetFileTime(file, pc, pa, pw) ? H9X_ERROR_SUCCESS : h9x_last_error();
		CloseHandle(file);
		return error;
	}
}

static UINT16 h9x_delete(UINT32 pir)
{
	WCHAR path[MAX_PATH];
	DWORD attr;
	UINT16 error = h9x_path_from_parsed_existing(h9x_r32(pir, H9X_IR_PPATH), path, 0);
	if (error) return error;
	if (!(s_h9xAcc & H9X_PERMIT_DELETE)) return H9X_ERROR_ACCESS_DENIED;
	attr = GetFileAttributesW(path);
	if (attr == INVALID_FILE_ATTRIBUTES) return h9x_last_error();
	if (attr & FILE_ATTRIBUTE_DIRECTORY) return H9X_ERROR_ACCESS_DENIED;
	if (DeleteFileW(path)) {
		hostdrvs_invalidateshortnamecache();
		return H9X_ERROR_SUCCESS;
	}
	return h9x_last_error();
}

static UINT16 h9x_write_parsed_dir_path(UINT32 address, const WCHAR *path)
{
	const WCHAR *p;
	const WCHAR *q;
	UINT32 total;
	UINT32 prefix;
	UINT32 off;
	UINT32 chars;
	UINT32 elen;
	UINT32 i;

	if (!address || address == 0xffffffffUL || !path)
		return H9X_ERROR_INVALID_PARAMETER;

	total = 4;
	prefix = 4;
	p = path;
	while (*p) {
		while (*p == HD_WC('\\') || *p == HD_WC('/')) p++;
		if (!*p) break;
		q = p;
		while (*q && *q != HD_WC('\\') && *q != HD_WC('/')) q++;
		chars = (UINT32)(q - p);
		if (!chars || chars >= MAX_PATH)
			return H9X_ERROR_FILENAME_EXCED_RANGE;
		elen = 2 + chars * 2;
		if (total + elen + 2 > (UINT32)(4 + MAX_PATH * 2))
			return H9X_ERROR_FILENAME_EXCED_RANGE;
		prefix = total;
		total += elen;
		p = q;
	}

	h9x_preflight_linear(address, total + 2, 1);
	cpu_kmemorywrite_w(address + 0, (UINT16)total);
	cpu_kmemorywrite_w(address + 2, (UINT16)prefix);

	off = 4;
	p = path;
	while (*p) {
		while (*p == HD_WC('\\') || *p == HD_WC('/')) p++;
		if (!*p) break;
		q = p;
		while (*q && *q != HD_WC('\\') && *q != HD_WC('/')) q++;
		chars = (UINT32)(q - p);
		elen = 2 + chars * 2;
		cpu_kmemorywrite_w(address + off, (UINT16)elen);
		for (i = 0; i < chars; i++)
			cpu_kmemorywrite_w(address + off + 2 + i * 2, p[i]);
		off += elen;
		p = q;
	}
	cpu_kmemorywrite_w(address + total, 0);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_append_component(WCHAR *dst, const WCHAR *component)
{
	size_t used;
	size_t add;

	if (!dst || !component || !*component)
		return H9X_ERROR_INVALID_PARAMETER;
	used = wcslen(dst);
	add = wcslen(component);
	if (used + (used ? 1 : 0) + add >= MAX_PATH)
		return H9X_ERROR_FILENAME_EXCED_RANGE;
	if (used)
		dst[used++] = HD_WC('\\');
	wcscpy(dst + used, component);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_query_dir_path(UINT32 pir, int wantShort)
{
	UINT32 ppath;
	UINT32 output;
	UINT16 total;
	UINT32 off;
	UINT32 elementIndex;
	UINT32 skipElements;
	WCHAR first[MAX_PATH];
	WCHAR second[MAX_PATH];
	WCHAR input[MAX_PATH];
	WCHAR converted[MAX_PATH];
	WCHAR canonical[MAX_PATH];
	WCHAR host[MAX_PATH];
	WCHAR search[MAX_PATH];
	WIN32_FIND_DATAW fd;
	HANDLE find;
	UINT16 error;
	size_t used;

	ppath = h9x_r32(pir, H9X_IR_PPATH);
	output = h9x_r32(pir, H9X_IR_AUX1);
	if (!ppath || !output || output == 0xffffffffUL)
		return H9X_ERROR_INVALID_PARAMETER;

	total = cpu_kmemoryread_w(ppath);
	if (total < 4 || total > 0x4000 || (total & 1))
		return H9X_ERROR_INVALID_DATA;

	skipElements = 0;
	off = 4;
	if (h9x_read_path_element(ppath, &off, first, NELEMENTS(first)) &&
		h9x_read_path_element(ppath, &off, second, NELEMENTS(second)) &&
		h9x_resource_element_equal(first, HD_W("NP2HOST")) &&
		h9x_resource_element_equal(second, HD_W("HOSTFS")))
		skipElements = 2;

	converted[0] = 0;
	wcscpy(host, s_h9xRoot);
	off = 4;
	elementIndex = 0;

	for (;;) {
		UINT32 before;

		before = off;
		if (before == (UINT32)total)
			break;
		if (!h9x_read_path_element(ppath, &off, input, NELEMENTS(input)))
			return H9X_ERROR_INVALID_DATA;

		if (elementIndex < skipElements) {
			error = h9x_append_component(converted, input);
			if (error) return error;
			elementIndex++;
			continue;
		}

		/* LFNで名前解決できなければSFNを使う */
		error = h9x_resolve_existing_component(host, input, 1,
			canonical, NELEMENTS(canonical));
		if (error) {
			#if defined(_WIN32) || defined(WIN32)
			TRACEOUT(("HOSTDRV9X DIR %s: component resolve failed parent=[%ls] input=[%ls] error=%u",
				wantShort ? "QUERY83" : "QUERYLONG", host, input, (unsigned)error));
			#endif
			return error;
		}

		used = wcslen(host);
		if (used + 1 + wcslen(canonical) >= MAX_PATH)
			return H9X_ERROR_FILENAME_EXCED_RANGE;
		wcscpy(search, host);
		used = wcslen(search);
		search[used++] = HD_WC('\\');
		wcscpy(search + used, canonical);

		find = FindFirstFileW(search, &fd);
		if (find == INVALID_HANDLE_VALUE) return h9x_last_error();
		FindClose(find);
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			return H9X_ERROR_PATH_NOT_FOUND;

		if (wantShort) {
			WCHAR shortName[14];

			error = h9x_sfn_get_short(host, fd.cFileName, shortName, NELEMENTS(shortName));
			if (error) {
				#if defined(_WIN32) || defined(WIN32)
				TRACEOUT(("HOSTDRV9X DIR QUERY83: alias generation failed parent=[%ls] name=[%ls] error=%u",
					host, fd.cFileName, (unsigned)error));
				#endif
				return error;
			}
			wcsncpy(input, shortName, MAX_PATH - 1);
			input[MAX_PATH - 1] = 0;
		} else {
			wcsncpy(input, fd.cFileName, MAX_PATH - 1);
			input[MAX_PATH - 1] = 0;
		}

		error = h9x_append_component(converted, input);
		if (error) return error;
		error = h9x_append_component(host, canonical);
		if (error) return error;
		elementIndex++;
	}

	error = h9x_write_parsed_dir_path(output, converted);
	if (error) return error;

	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X DIR %s: in=%08X out=%08X converted=[%ls]",
		wantShort ? "QUERY83" : "QUERYLONG",
		(unsigned)ppath, (unsigned)output, converted));
	#endif
	h9x_trace_parsed_path(output);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_dir(UINT32 pir)
{
	WCHAR path[MAX_PATH];
	DWORD attr;
	UINT8 fn = h9x_r8(pir, H9X_IR_FLAGS);
	UINT16 error;

	TRACEOUT(("HOSTDRV9X DIR: fn=%u ppath=%08X aux1=%08X data=%08X len=%08X",
		(unsigned)fn,
		(unsigned)h9x_r32(pir, H9X_IR_PPATH),
		(unsigned)h9x_r32(pir, H9X_IR_AUX1),
		(unsigned)h9x_r32(pir, H9X_IR_DATA),
		(unsigned)h9x_r32(pir, H9X_IR_LENGTH)));
	h9x_trace_parsed_path(h9x_r32(pir, H9X_IR_PPATH));

	if (fn == H9X_QUERY83_DIR)
		return h9x_query_dir_path(pir, 1);
	if (fn == H9X_QUERYLONG_DIR)
		return h9x_query_dir_path(pir, 0);

	if (fn == H9X_CREATE_DIR)
		error = h9x_path_from_parsed(h9x_r32(pir, H9X_IR_PPATH), path, 0);
	else
		error = h9x_path_from_parsed_existing(h9x_r32(pir, H9X_IR_PPATH), path, 0);
	if (error) return error;
	switch (fn) {
	case H9X_CREATE_DIR:
		if (!(s_h9xAcc & H9X_PERMIT_WRITE)) return H9X_ERROR_WRITE_PROTECT;
		if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
			h9x_preserve_unparsed_leaf_case(pir, path);
		if (CreateDirectoryW(path, NULL)) {
			hostdrvs_invalidateshortnamecache();
			return H9X_ERROR_SUCCESS;
		}
		return h9x_last_error();
	case H9X_DELETE_DIR:
		if (!(s_h9xAcc & H9X_PERMIT_DELETE)) return H9X_ERROR_ACCESS_DENIED;
		/* HOSTDRVルートの削除は拒否 */
		if (_wcsicmp(path, s_h9xRoot) == 0) return H9X_ERROR_ACCESS_DENIED;
		if (RemoveDirectoryW(path)) {
			hostdrvs_invalidateshortnamecache();
			return H9X_ERROR_SUCCESS;
		}
		return h9x_last_error();
	case H9X_CHECK_DIR:
		attr = GetFileAttributesW(path);
		if (attr == INVALID_FILE_ATTRIBUTES) return h9x_last_error();
		return (attr & FILE_ATTRIBUTE_DIRECTORY) ? H9X_ERROR_SUCCESS : H9X_ERROR_PATH_NOT_FOUND;
	default:
		return H9X_ERROR_INVALID_FUNCTION;
	}
}


static UINT16 h9x_fileattrib(UINT32 pir)
{
	WCHAR path[MAX_PATH];
	WIN32_FILE_ATTRIBUTE_DATA data;
	UINT8 fn = h9x_r8(pir, H9X_IR_FLAGS);
	UINT16 error = h9x_path_from_parsed_existing(h9x_r32(pir, H9X_IR_PPATH), path, 0);
	if (error) return error;
	if (!GetFileAttributesExW(path, GetFileExInfoStandard, &data)) return h9x_last_error();
	switch (fn) {
	case H9X_GET_ATTRIBUTES:
		h9x_w32(pir, H9X_IR_ATTR, h9x_attrs_from_win(data.dwFileAttributes));
		return H9X_ERROR_SUCCESS;
	case H9X_SET_ATTRIBUTES:
		if (!(s_h9xAcc & H9X_PERMIT_WRITE)) return H9X_ERROR_WRITE_PROTECT;
		return SetFileAttributesW(path, h9x_attrs_to_win(h9x_r32(pir, H9X_IR_ATTR), data.dwFileAttributes)) ?
			H9X_ERROR_SUCCESS : h9x_last_error();
	case H9X_GET_ATTRIB_COMP_FILESIZE:
		h9x_w32(pir, H9X_IR_SIZE, data.nFileSizeLow);
		return H9X_ERROR_SUCCESS;
	case H9X_GET_ATTRIB_MODIFY_DATETIME:
		h9x_write_dostime(pir, &data.ftLastWriteTime); return H9X_ERROR_SUCCESS;
	case H9X_GET_ATTRIB_LAST_ACCESS_DATETIME:
		h9x_write_dostime(pir, &data.ftLastAccessTime); return H9X_ERROR_SUCCESS;
	case H9X_GET_ATTRIB_CREATION_DATETIME:
		h9x_write_dostime(pir, &data.ftCreationTime); return H9X_ERROR_SUCCESS;
	case H9X_SET_ATTRIB_MODIFY_DATETIME:
	case H9X_SET_ATTRIB_LAST_ACCESS_DATETIME:
	case H9X_SET_ATTRIB_CREATION_DATETIME:
	{
		HANDLE h;
		FILETIME ft;
		FILETIME *pc = NULL, *pa = NULL, *pw = NULL;
		UINT16 time, date;
		if (!(s_h9xAcc & H9X_PERMIT_WRITE)) return H9X_ERROR_WRITE_PROTECT;
		time = h9x_r16(pir, H9X_IR_DOSTIME);
		date = h9x_r16(pir, H9X_IR_DOSTIME + 2);
		if (!h9x_dos_to_filetime(date, time, &ft)) return H9X_ERROR_INVALID_PARAMETER;
		if (fn == H9X_SET_ATTRIB_CREATION_DATETIME) pc = &ft;
		else if (fn == H9X_SET_ATTRIB_LAST_ACCESS_DATETIME) pa = &ft;
		else pw = &ft;
		h = CreateFileW(path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : 0, NULL);
		if (h == INVALID_HANDLE_VALUE) return h9x_last_error();
		if (!SetFileTime(h, pc, pa, pw)) error = h9x_last_error();
		else error = H9X_ERROR_SUCCESS;
		CloseHandle(h);
		return error;
	}
	case H9X_GET_ATTRIB_FIRST_CLUST:
		h9x_w32(pir, H9X_IR_AUX2, 0); return H9X_ERROR_SUCCESS;
	default:
		return H9X_ERROR_INVALID_FUNCTION;
	}
}

static UINT16 h9x_rename(UINT32 pir)
{
	WCHAR from[MAX_PATH], to[MAX_PATH];
	UINT16 error;
	if (!(s_h9xAcc & H9X_PERMIT_DELETE) || !(s_h9xAcc & H9X_PERMIT_WRITE)) return H9X_ERROR_ACCESS_DENIED;
	error = h9x_path_from_parsed_existing(h9x_r32(pir, H9X_IR_PPATH), from, 0);
	if (error) return error;
	if (_wcsicmp(from, s_h9xRoot) == 0) return H9X_ERROR_ACCESS_DENIED;
	error = h9x_path_from_parsed(h9x_r32(pir, H9X_IR_PPATH2), to, 0);
	if (error) return error;
	h9x_preserve_unparsed_leaf_case(pir, to);
	if (MoveFileW(from, to)) {
		hostdrvs_invalidateshortnamecache();
		return H9X_ERROR_SUCCESS;
	}
	return h9x_last_error();
}

static int h9x_find_accept(const WIN32_FIND_DATAW* fd, UINT32 searchAttr, int allowDotEntry)
{
	UINT32 attr = h9x_attrs_from_win(fd->dwFileAttributes);
	UINT32 must = (searchAttr >> 8) & 0x3f;
	UINT32 asked = searchAttr & 0x3f;
	if ((!wcscmp(fd->cFileName, HD_W(".")) || !wcscmp(fd->cFileName, HD_W(".."))) && !allowDotEntry) return 0;
	if (((attr ^ asked) & must) != 0) return 0;
	if ((attr & (H9X_FILE_ATTRIBUTE_HIDDEN | H9X_FILE_ATTRIBUTE_SYSTEM |
		H9X_FILE_ATTRIBUTE_DIRECTORY)) & ~asked) return 0;
	return 1;
}

static void h9x_make_dot_find_data(NP2HOSTDRV9X_HANDLE *h, WIN32_FIND_DATAW *fd, const WCHAR *name)
{
	WCHAR target[MAX_PATH];
	WIN32_FILE_ATTRIBUTE_DATA info;

	ZeroMemory(fd, sizeof(*fd));
	fd->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	if (h && h9x_find_parent_path(h->path, target, NELEMENTS(target))) {
		if (!wcscmp(name, HD_W(".."))) {
			if (!PathRemoveFileSpecW(target)) target[0] = HD_WC('\0');
		}
		if (target[0] && GetFileAttributesExW(target, GetFileExInfoStandard, &info)) {
			fd->dwFileAttributes = info.dwFileAttributes;
			fd->ftCreationTime = info.ftCreationTime;
			fd->ftLastAccessTime = info.ftLastAccessTime;
			fd->ftLastWriteTime = info.ftLastWriteTime;
			fd->nFileSizeHigh = info.nFileSizeHigh;
			fd->nFileSizeLow = info.nFileSizeLow;
		}
	}
	fd->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
	wcsncpy(fd->cFileName, name, NELEMENTS(fd->cFileName) - 1);
	fd->cFileName[NELEMENTS(fd->cFileName) - 1] = HD_WC('\0');
}

static const WCHAR *h9x_find_leaf_pattern(const WCHAR *path)
{
	const WCHAR *slash1;
	const WCHAR *slash2;

	if (!path) return NULL;
	slash1 = wcsrchr(path, HD_WC('\\'));
	slash2 = wcsrchr(path, HD_WC('/'));
	if (slash1 && slash2) return ((slash1 > slash2) ? slash1 : slash2) + 1;
	if (slash1) return slash1 + 1;
	if (slash2) return slash2 + 1;
	return path;
}

// SFN対応検索
static int h9x_find_start_sfn_pattern_fallback(NP2HOSTDRV9X_HANDLE *h)
{
	WCHAR parent[MAX_PATH];
	WCHAR enumPath[MAX_PATH];
	const WCHAR *pattern;

	if (!h) return 0;
	pattern = h9x_find_leaf_pattern(h->path);
	if (!pattern || !wcschr(pattern, HD_WC('~'))) return 0;
	if (!h9x_find_parent_path(h->path, parent, NELEMENTS(parent))) return 0;
	if (!PathCombineW(enumPath, parent, HD_W("*"))) return 0;

	h->handle = FindFirstFileW(enumPath, &h->findData);
	if (h->handle == INVALID_HANDLE_VALUE) return 0;
	h->findFlags |= H9X_FIND_SFN_PATTERN_FALLBACK;
	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X FIND: synthetic-SFN pattern fallback pattern=[%ls] enum=[%ls]", pattern, enumPath));
	#endif
	return 1;
}

static int h9x_find_candidate_matches_pattern(NP2HOSTDRV9X_HANDLE *h,
	const WIN32_FIND_DATAW *fd)
{
	WCHAR shortName[14];
	const WCHAR *pattern;

	if (!h || !fd) return 0;
	pattern = h9x_find_leaf_pattern(h->path);
	if (!pattern || !*pattern) return 0;

	if (PathMatchSpecW(fd->cFileName, pattern)) return 1;
	if (fd->cAlternateFileName[0] && PathMatchSpecW(fd->cAlternateFileName, pattern)) return 1;
	if (h9x_find_get_short_name(h, fd, shortName, NELEMENTS(shortName)) && PathMatchSpecW(shortName, pattern)) return 1;
	return 0;
}

static int h9x_find_next_accepted(NP2HOSTDRV9X_HANDLE *h)
{
	for (;;) {
		if (h->findFlags & H9X_FIND_DOT_PENDING) {
			h->findFlags &= (UINT8)~H9X_FIND_DOT_PENDING;
			h9x_make_dot_find_data(h, &h->findData, HD_W("."));
			if (h9x_find_accept(&h->findData, h->searchAttr, 1)) {
				TRACEOUT(("HOSTDRV9X FIND: synthetic accepted=[.]"));
				return 1;
			}
			continue;
		}
		if (h->findFlags & H9X_FIND_DOTDOT_PENDING) {
			h->findFlags &= (UINT8)~H9X_FIND_DOTDOT_PENDING;
			h9x_make_dot_find_data(h, &h->findData, HD_W(".."));
			if (h9x_find_accept(&h->findData, h->searchAttr, 1)) {
				TRACEOUT(("HOSTDRV9X FIND: synthetic accepted=[..]"));
				return 1;
			}
			continue;
		}

		if (h->findFlags & H9X_FIND_HOST_NOT_STARTED) {
			DWORD directError;

			h->findFlags &= (UINT8)~H9X_FIND_HOST_NOT_STARTED;
			h->handle = FindFirstFileW(h->path, &h->findData);
			if (h->handle == INVALID_HANDLE_VALUE) {
				directError = GetLastError();
				if (!h9x_find_start_sfn_pattern_fallback(h)) {
					SetLastError(directError);
					TRACEOUT(("HOSTDRV9X FIND: FindFirstFileW failed winerr=%u", (unsigned)directError));
					return 0;
				}
			}
			#if defined(_WIN32) || defined(WIN32)
			TRACEOUT(("HOSTDRV9X FIND: first candidate=[%ls] alt=[%ls] attr=%08X searchAttr=%08X",
				h->findData.cFileName, h->findData.cAlternateFileName,
				(unsigned)h->findData.dwFileAttributes, (unsigned)h->searchAttr));
			#endif
		}
		else {
			if (h->handle == NULL || h->handle == INVALID_HANDLE_VALUE) {
				SetLastError(ERROR_NO_MORE_FILES);
				return 0;
			}
			if (!FindNextFileW(h->handle, &h->findData)) {
				TRACEOUT(("HOSTDRV9X FIND: FindNextFileW failed winerr=%u", (unsigned)GetLastError()));
				return 0;
			}
			#if defined(_WIN32) || defined(WIN32)
			TRACEOUT(("HOSTDRV9X FIND: next candidate=[%ls] alt=[%ls] attr=%08X searchAttr=%08X",
				h->findData.cFileName, h->findData.cAlternateFileName,
				(unsigned)h->findData.dwFileAttributes, (unsigned)h->searchAttr));
			#endif
		}
		if ((h->findFlags & H9X_FIND_SFN_PATTERN_FALLBACK) && !h9x_find_candidate_matches_pattern(h, &h->findData)) { 
#if defined(_WIN32) || defined(WIN32)
			TRACEOUT(("HOSTDRV9X FIND: synthetic-pattern rejected=[%ls]",
				h->findData.cFileName));
#endif
			continue;
		}
		if (h9x_find_accept(&h->findData, h->searchAttr, 0)) {
			#if defined(_WIN32) || defined(WIN32)
			TRACEOUT(("HOSTDRV9X FIND: accepted=[%ls]", h->findData.cFileName));
			#endif
			return 1;
		}
		#if defined(_WIN32) || defined(WIN32)
		TRACEOUT(("HOSTDRV9X FIND: rejected=[%ls]", h->findData.cFileName));
		#endif
	}
}

static void h9x_write_filetime(UINT32 addr, const FILETIME *ft)
{
	h9x_w32(addr, 0, ft->dwLowDateTime);
	h9x_w32(addr, 4, ft->dwHighDateTime);
}

static void h9x_write_unicode_fixed(UINT32 addr, const WCHAR *text, UINT32 chars)
{
	UINT32 i;
	for (i = 0; i < chars; i++) {
		WCHAR c = (i < (UINT32)wcslen(text)) ? text[i] : 0;
		cpu_kmemorywrite_w(addr + i * 2, c);
		if (!c) {
			for (i++; i < chars; i++) cpu_kmemorywrite_w(addr + i * 2, 0);
			break;
		}
	}
}


static UINT16 h9x_write_parsed_path(UINT32 address, const WCHAR *path)
{
	WCHAR copy[MAX_PATH];
	WCHAR *element;
	UINT32 offset = 4;
	UINT16 last = 4;
	int haveElement = 0;

	if (!address || !path) return H9X_ERROR_INVALID_PARAMETER;
	if (wcslen(path) >= MAX_PATH) return H9X_ERROR_FILENAME_EXCED_RANGE;
	wcscpy(copy, path);

	element = copy;
	while (*element == HD_WC('\\') || *element == HD_WC('/')) element++;
	while (*element) {
		WCHAR *end = element;
		UINT32 chars;
		UINT16 length;
		UINT32 i;

		while (*end && *end != HD_WC('\\') && *end != HD_WC('/')) end++;
		chars = (UINT32)(end - element);
		if (chars) {
			length = (UINT16)(2 + chars * 2);
			if (offset + length + 2 > 0xffffUL) return H9X_ERROR_FILENAME_EXCED_RANGE;
			last = (UINT16)offset;
			cpu_kmemorywrite_w(address + offset, length);
			for (i = 0; i < chars; i++)
				cpu_kmemorywrite_w(address + offset + 2 + i * 2, element[i]);
			offset += length;
			haveElement = 1;
		}
		while (*end == HD_WC('\\') || *end == HD_WC('/')) end++;
		element = end;
	}

	cpu_kmemorywrite_w(address + 0, (UINT16)offset);
	cpu_kmemorywrite_w(address + 2, haveElement ? last : 4);
	cpu_kmemorywrite_w(address + offset, 0);
	return H9X_ERROR_SUCCESS;
}

static int h9x_find_pattern_is_root(const WCHAR *path)
{
	WCHAR dir[MAX_PATH];
	WCHAR *slash1;
	WCHAR *slash2;
	WCHAR *slash;
	UINT32 length;

	if (!path || !*path) return 0;
	wcsncpy(dir, path, MAX_PATH - 1);
	dir[MAX_PATH - 1] = 0;
	slash1 = wcsrchr(dir, HD_WC('\\'));
	slash2 = wcsrchr(dir, HD_WC('/'));
	slash = (slash1 && slash2) ? ((slash1 > slash2) ? slash1 : slash2) :
		(slash1 ? slash1 : slash2);
	if (!slash) return 0;
	*slash = 0;
	length = (UINT32)wcslen(dir);
	while (length > 3 && (dir[length - 1] == HD_WC('\\') || dir[length - 1] == HD_WC('/')))
		dir[--length] = 0;
	return (_wcsicmp(dir, s_h9xRoot) == 0);
}

static int h9x_find_pattern_matches_dot(const WCHAR *path, const WCHAR *name)
{
	const WCHAR *slash1;
	const WCHAR *slash2;
	const WCHAR *slash;
	const WCHAR *pattern;

	if (!path || !name) return 0;
	slash1 = wcsrchr(path, HD_WC('\\'));
	slash2 = wcsrchr(path, HD_WC('/'));
	slash = (slash1 && slash2) ? ((slash1 > slash2) ? slash1 : slash2) :
		(slash1 ? slash1 : slash2);
	pattern = slash ? slash + 1 : path;

	if (!wcscmp(pattern, HD_W("*")) || !wcscmp(pattern, HD_W("*.*"))) return 1;
	return PathMatchSpecW(name, pattern) ? 1 : 0;
}

static void h9x_fill_find_data(UINT32 address, const WIN32_FIND_DATAW *fd, NP2HOSTDRV9X_HANDLE *h)
{
	WIN32_FIND_DATAW out;
	WCHAR synthetic[14];

	out = *fd;
	if (!out.cAlternateFileName[0] && h &&
		h9x_find_get_short_name(h, fd, synthetic, NELEMENTS(synthetic)) &&
		_wcsicmp(synthetic, out.cFileName)) {
		wcsncpy(out.cAlternateFileName, synthetic, 13);
		out.cAlternateFileName[13] = 0;
		#if defined(_WIN32) || defined(WIN32)
		TRACEOUT(("HOSTDRV9X SFN: generated long=[%ls] short=[%ls]",
			out.cFileName, out.cAlternateFileName));
		#endif
	}

	h9x_w32(address, 0, out.dwFileAttributes);
	h9x_write_filetime(address + 4, &out.ftCreationTime);
	h9x_write_filetime(address + 12, &out.ftLastAccessTime);
	h9x_write_filetime(address + 20, &out.ftLastWriteTime);
	h9x_w32(address, 28, out.nFileSizeHigh);
	h9x_w32(address, 32, out.nFileSizeLow);
	h9x_w32(address, 36, 0);
	h9x_w32(address, 40, 0);
	h9x_write_unicode_fixed(address + 44, out.cFileName, MAX_PATH);
	h9x_write_unicode_fixed(address + 44 + MAX_PATH * 2, out.cAlternateFileName, 14);
}

static int h9x_is_83_char(WCHAR c)
{
	c = (WCHAR)towupper(c);
	if (c >= HD_WC('A') && c <= HD_WC('Z')) return 1;
	if (c >= HD_WC('0') && c <= HD_WC('9')) return 1;
	switch (c) {
	case HD_WC('$'): case HD_WC('%'): case HD_WC('\''): case HD_WC('-'): case HD_WC('_'):
	case HD_WC('@'): case HD_WC('~'): case HD_WC('`'): case HD_WC('!'): case HD_WC('('):
	case HD_WC(')'): case HD_WC('{'): case HD_WC('}'): case HD_WC('^'): case HD_WC('#'):
	case HD_WC('&'):
		return 1;
	}
	return 0;
}

static int h9x_is_plain_83_name(const WCHAR *src)
{
	const WCHAR *dot = NULL;
	const WCHAR *p;
	UINT32 baseLen;
	UINT32 extLen = 0;

	if (!src || !*src) return 0;
	for (p = src; *p; p++) {
		if (*p == HD_WC('.')) {
			if (dot) return 0;
			dot = p;
		} else if (!h9x_is_83_char(*p)) {
			return 0;
		}
	}
	baseLen = (UINT32)((dot ? dot : p) - src);
	if (!baseLen || baseLen > 8) return 0;
	if (dot) {
		extLen = (UINT32)(p - dot - 1);
		if (!extLen || extLen > 3) return 0;
	}
	return 1;
}

static void h9x_make_83_name(NP2HOSTDRV9X_HANDLE *h,
	const WIN32_FIND_DATAW *fd, char name[13])
{
	WCHAR mapped[14];
	WCHAR temp[13];
	UINT32 i;

	ZeroMemory(name, 13);

	/* "."と".."はSFNアルゴリズムで名前変換しない */
	if (!wcscmp(fd->cFileName, HD_W(".")) || !wcscmp(fd->cFileName, HD_W(".."))) {
		WideCharToMultiByte(CP_OEMCP, 0, fd->cFileName, -1, name, 13, NULL, NULL);
		return;
	}

	if (h && h9x_find_get_short_name(h, fd, mapped, NELEMENTS(mapped))) {
		if (WideCharToMultiByte(CP_OEMCP, 0, mapped, -1, name, 13, NULL, NULL))
			return;
		ZeroMemory(name, 13);
	}

	/* ホストファイルシステムがSFN情報を持っていたらそれを使う。そうでなければここでは空を返す */
	if (fd->cAlternateFileName[0]) {
		if (WideCharToMultiByte(CP_OEMCP, 0, fd->cAlternateFileName, -1,
			name, 13, NULL, NULL))
			return;
		ZeroMemory(name, 13);
	}

	if (h9x_is_plain_83_name(fd->cFileName)) {
		for (i = 0; fd->cFileName[i] && i < 12; i++)
			temp[i] = (WCHAR)towupper(fd->cFileName[i]);
		temp[i] = 0;
		WideCharToMultiByte(CP_OEMCP, 0, temp, -1, name, 13, NULL, NULL);
	}
}

static UINT32 h9x_search_result_address(UINT32 pir)
{
	UINT32 data = h9x_r32(pir, H9X_IR_DATA);
	UINT32 aux1 = h9x_r32(pir, H9X_IR_AUX1);

	if (data != 0 && data != 0xffffffffUL) return data;
	if (aux1 != 0 && aux1 != 0xffffffffUL) return aux1;
	return 0;
}

static void h9x_fill_search_entry(UINT32 address, UINT32 cookie, NP2HOSTDRV9X_HANDLE* h, const WIN32_FIND_DATAW* fd)
{
	UINT16 date, time;
	char name[13];

	if (address == 0 || address == 0xffffffffUL) {
		TRACEOUT(("HOSTDRV9X SEARCHENTRY: refusing invalid address=%08X", (unsigned)address));
		return;
	}
	h9x_w16(address, H9X_SE_NETKEY, (UINT16)(cookie & 0xffff));
	h9x_filetime_to_dos(&fd->ftLastWriteTime, &date, &time);
	h9x_w8(address, H9X_SE_ATTRIB, (UINT8)h9x_attrs_from_win(fd->dwFileAttributes));
	h9x_w16(address, H9X_SE_TIME, time);
	h9x_w16(address, H9X_SE_DATE, date);
	h9x_w32(address, H9X_SE_SIZE, fd->nFileSizeLow);
	h9x_make_83_name(h, fd, name);
	h9x_memwrite(address + H9X_SE_NAME, name, 13);
	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X SEARCHENTRY: addr=%08X cookie=%04X attr=%02X size=%u name=[%s] long=[%ls] alt=[%ls]",
		(unsigned)address, (unsigned)(cookie & 0xffff),
		(unsigned)h9x_attrs_from_win(fd->dwFileAttributes),
		(unsigned)fd->nFileSizeLow, name, fd->cFileName, fd->cAlternateFileName));
	#endif
	h9x_trace_guest_bytes("DTA-after", address, 48);
}

static void h9x_fill_volume_label_search_entry(UINT32 address)
{
	char name[13];

	if (address == 0 || address == 0xffffffffUL) {
		TRACEOUT(("HOSTDRV9X VOLUME LABEL: refusing invalid classic address=%08X", (unsigned)address));
		return;
	}

	ZeroMemory(name, sizeof(name));
	memcpy(name, H9X_VOLUME_LABEL_A, sizeof(H9X_VOLUME_LABEL_A));

	h9x_w16(address, H9X_SE_NETKEY, H9X_VOLUME_LABEL_COOKIE);
	h9x_w8(address, H9X_SE_ATTRIB, H9X_FILE_ATTRIBUTE_LABEL);
	h9x_w16(address, H9X_SE_TIME, 0);
	h9x_w16(address, H9X_SE_DATE, 0);
	h9x_w32(address, H9X_SE_SIZE, 0);
	h9x_memwrite(address + H9X_SE_NAME, name, 13);

	TRACEOUT(("HOSTDRV9X VOLUME LABEL: classic addr=%08X label=[%s]",
		(unsigned)address, H9X_VOLUME_LABEL_A));
	h9x_trace_guest_bytes("DTA-volume-label", address, 48);
}

static void h9x_fill_volume_label_find_data(UINT32 address)
{
	WIN32_FIND_DATAW fd;

	ZeroMemory(&fd, sizeof(fd));
	fd.dwFileAttributes = H9X_FILE_ATTRIBUTE_LABEL;
	wcscpy(fd.cFileName, H9X_VOLUME_LABEL_W);
	h9x_fill_find_data(address, &fd, NULL);

	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X VOLUME LABEL: lfn addr=%08X label=[%ls]",
		(unsigned)address, H9X_VOLUME_LABEL_W));
	#endif
}

static UINT16 h9x_findopen_volume_label(UINT32 pir, int lfn)
{
	UINT32 data;

	if (lfn) {
		data = h9x_r32(pir, H9X_IR_DATA);
		if (!data) return H9X_ERROR_INVALID_PARAMETER;
		h9x_fill_volume_label_find_data(data);
		h9x_w32(pir, H9X_IR_FH, H9X_VOLUME_LABEL_HANDLE);
	} else {
		data = h9x_search_result_address(pir);
		if (!data) {
			TRACEOUT(("HOSTDRV9X VOLUME LABEL: invalid classic SEARCH result buffer data=%08X aux1=%08X",
				(unsigned)h9x_r32(pir, H9X_IR_DATA),
				(unsigned)h9x_r32(pir, H9X_IR_AUX1)));
			return H9X_ERROR_INVALID_PARAMETER;
		}
		h9x_fill_volume_label_search_entry(data);
	}

	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_findopen_common(UINT32 pir, int lfn)
{
	WCHAR path[MAX_PATH];
	int index;
	UINT32 attr = h9x_r32(pir, H9X_IR_ATTR);
	UINT16 error;
	TRACEOUT(("HOSTDRV9X FINDOPEN_COMMON: lfn=%d pir=%08X attr=%08X ppath=%08X aux1=%08X data=%08X len=%08X",
		lfn, (unsigned)pir, (unsigned)attr,
		(unsigned)h9x_r32(pir, H9X_IR_PPATH),
		(unsigned)h9x_r32(pir, H9X_IR_AUX1),
		(unsigned)h9x_r32(pir, H9X_IR_DATA),
		(unsigned)h9x_r32(pir, H9X_IR_LENGTH)));
	h9x_trace_parsed_path(h9x_r32(pir, H9X_IR_PPATH));

	/* ボリュームラベルの要求ならボリュームラベルを返す */
	if ((attr & 0x3f) & H9X_FILE_ATTRIBUTE_LABEL) {
		TRACEOUT(("HOSTDRV9X FINDOPEN_COMMON: volume-label search lfn=%d -> [%s]",
			lfn, H9X_VOLUME_LABEL_A));
		return h9x_findopen_volume_label(pir, lfn);
	}

	error = h9x_path_from_parsed_existing(h9x_r32(pir, H9X_IR_PPATH), path, 1);
	if (error) {
		TRACEOUT(("HOSTDRV9X FINDOPEN_COMMON: path resolve error=%u",
			(unsigned)error));
		return error;
	}
	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X FINDOPEN_COMMON: host path=[%ls]", path));
	#endif
	index = h9x_alloc_handle(H9X_HANDLE_FIND);
	if (index < 0) return H9X_ERROR_TOO_MANY_OPEN_FILES;

	s_h9x.files[index].searchAttr = attr;
	wcsncpy(s_h9x.files[index].path, path, MAX_PATH - 1);
	s_h9x.files[index].path[MAX_PATH - 1] = HD_WC('\0');
	s_h9x.files[index].findFlags = H9X_FIND_HOST_NOT_STARTED;

	// ルートでないなら.と..を足す
	if (!h9x_find_pattern_is_root(path)) {
		WIN32_FIND_DATAW dotData;
		ZeroMemory(&dotData, sizeof(dotData));
		dotData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		if (h9x_find_pattern_matches_dot(path, HD_W("."))) {
			wcscpy(dotData.cFileName, HD_W("."));
			if (h9x_find_accept(&dotData, attr, 1))
				s_h9x.files[index].findFlags |= H9X_FIND_DOT_PENDING;
		}
		if (h9x_find_pattern_matches_dot(path, HD_W(".."))) {
			wcscpy(dotData.cFileName, HD_W(".."));
			if (h9x_find_accept(&dotData, attr, 1))
				s_h9x.files[index].findFlags |= H9X_FIND_DOTDOT_PENDING;
		}
	}
	if (!h9x_find_next_accepted(&s_h9x.files[index])) {
		error = h9x_last_error();
		h9x_free_handle(&s_h9x.files[index]);
		return error;
	}
	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X FINDOPEN_COMMON: handle index=%d generation=%u cookie=%08X accepted=[%ls]",
		index, (unsigned)s_h9x.files[index].generation,
		(unsigned)h9x_make_cookie(index), s_h9x.files[index].findData.cFileName));
	#endif
	h9x_w32(pir, H9X_IR_FH, h9x_make_cookie(index));
	if (lfn) {
		UINT32 data = h9x_r32(pir, H9X_IR_DATA);

		if (!data) {
			h9x_free_handle(&s_h9x.files[index]);
			return H9X_ERROR_INVALID_PARAMETER;
		}
		h9x_fill_find_data(data, &s_h9x.files[index].findData, &s_h9x.files[index]);
	} else {
		UINT32 data = h9x_search_result_address(pir);
		if (!data) {
			TRACEOUT(("HOSTDRV9X SEARCH FIRST: invalid result buffer data=%08X aux1=%08X",
				(unsigned)h9x_r32(pir, H9X_IR_DATA),
				(unsigned)h9x_r32(pir, H9X_IR_AUX1)));
			h9x_free_handle(&s_h9x.files[index]);
			return H9X_ERROR_INVALID_PARAMETER;
		}
		h9x_fill_search_entry(data, h9x_make_cookie(index), &s_h9x.files[index], &s_h9x.files[index].findData);
	}
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_findnext(UINT32 pir)
{
	UINT32 fh = h9x_r32(pir, H9X_IR_FH);
	NP2HOSTDRV9X_HANDLE *h;
	UINT32 data = h9x_r32(pir, H9X_IR_DATA);
	TRACEOUT(("HOSTDRV9X FINDNEXT: pir=%08X fh=%08X data=%08X len=%08X aux1=%08X",
		(unsigned)pir, (unsigned)fh, (unsigned)data,
		(unsigned)h9x_r32(pir, H9X_IR_LENGTH), (unsigned)h9x_r32(pir, H9X_IR_AUX1)));
	if (fh == H9X_VOLUME_LABEL_HANDLE) {
		TRACEOUT(("HOSTDRV9X FINDNEXT: synthetic volume label -> NO_MORE_FILES"));
		return H9X_ERROR_NO_MORE_FILES;
	}
	h = h9x_get_handle(fh, H9X_HANDLE_FIND);
	if (!h) return H9X_ERROR_INVALID_HANDLE;

	if (!data) return H9X_ERROR_INVALID_PARAMETER;
	if (!h9x_find_next_accepted(h)) {
		UINT16 error = h9x_last_error();
		TRACEOUT(("HOSTDRV9X FINDNEXT: no next entry error=%u", (unsigned)error));
		return error;
	}
	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X FINDNEXT: accepted=[%ls] alt=[%ls]",
		h->findData.cFileName, h->findData.cAlternateFileName));
	#endif
	h9x_fill_find_data(data, &h->findData, h);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_findclose(UINT32 pir)
{
	UINT32 fh = h9x_r32(pir, H9X_IR_FH);
	NP2HOSTDRV9X_HANDLE *h;

	if (fh == H9X_VOLUME_LABEL_HANDLE) {
		TRACEOUT(("HOSTDRV9X FINDCLOSE: synthetic volume label"));
		return H9X_ERROR_SUCCESS;
	}
	h = h9x_get_handle(fh, H9X_HANDLE_FIND);
	if (!h) return H9X_ERROR_INVALID_HANDLE;
	h9x_free_handle(h);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_search(UINT32 pir)
{
	UINT8 fn = h9x_r8(pir, H9X_IR_FLAGS);
	UINT32 data = h9x_search_result_address(pir);
	NP2HOSTDRV9X_HANDLE *h;
	UINT32 cookie;

	TRACEOUT(("HOSTDRV9X SEARCH: fn=%u attr=%08X result=%08X aux1=%08X data=%08X",
		(unsigned)fn,
		(unsigned)h9x_r32(pir, H9X_IR_ATTR),
		(unsigned)data,
		(unsigned)h9x_r32(pir, H9X_IR_AUX1),
		(unsigned)h9x_r32(pir, H9X_IR_DATA)));

	if (fn == H9X_SEARCH_FIRST) {
		UINT16 e = h9x_findopen_common(pir, 0);
		TRACEOUT(("HOSTDRV9X SEARCH FIRST result=%u", (unsigned)e));
		return e;
	}
	if (fn != H9X_SEARCH_NEXT) {
		TRACEOUT(("HOSTDRV9X SEARCH: invalid fn=%u", (unsigned)fn));
		return H9X_ERROR_INVALID_FUNCTION;
	}
	if (!data) {
		TRACEOUT(("HOSTDRV9X SEARCH NEXT: invalid result buffer data=%08X aux1=%08X",
			(unsigned)h9x_r32(pir, H9X_IR_DATA),
			(unsigned)h9x_r32(pir, H9X_IR_AUX1)));
		return H9X_ERROR_INVALID_PARAMETER;
	}
	h9x_trace_guest_bytes("DTA-before-next", data, 48);
	cookie = h9x_r16(data, H9X_SE_NETKEY);
	if ((UINT16)cookie == H9X_VOLUME_LABEL_COOKIE) {
		TRACEOUT(("HOSTDRV9X SEARCH NEXT: synthetic volume label -> NO_MORE_FILES"));
		return H9X_ERROR_NO_MORE_FILES;
	}
	TRACEOUT(("HOSTDRV9X SEARCH NEXT: cookie=%04X index=%u generation4=%u",
		(unsigned)cookie, (unsigned)(cookie & 0x0fff), (unsigned)(cookie >> 12)));
	h = h9x_get_search_handle((UINT16)cookie);
	if (!h) {
		TRACEOUT(("HOSTDRV9X SEARCH NEXT: invalid handle cookie=%04X", (unsigned)cookie));
		return H9X_ERROR_INVALID_HANDLE;
	}
	if (!h9x_find_next_accepted(h)) {
		UINT16 e = h9x_last_error();
		TRACEOUT(("HOSTDRV9X SEARCH NEXT: no next, error=%u winerr=%u", (unsigned)e, (unsigned)GetLastError()));
		return e;
	}
	h9x_fill_search_entry(data, h9x_make_cookie((int)(h - s_h9x.files)), h, &h->findData);
	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X SEARCH NEXT: success file=[%ls]", h->findData.cFileName));
	#endif
	return H9X_ERROR_SUCCESS;
}

/* Win95の場合ディスク容量が大きすぎるとおかしくなるので修正する */
static void h9x_fit_win95_disk_geometry(UINT64 totalBytes, UINT64 freeBytes,
	DWORD *sectorsPerCluster, DWORD *bytesPerSector,
	DWORD *freeClusters, DWORD *totalClusters)
{
	UINT32 spc = 8;
	const UINT32 bps = 512;
	UINT64 clusterBytes;
	UINT64 total;
	UINT64 freec;

	for (;;) {
		clusterBytes = (UINT64)spc * bps;
		total = totalBytes / clusterBytes;
		if (total <= (UINT64)0x10000UL || spc >= 0x8000) break;
		spc <<= 1;
	}

	total = totalBytes / clusterBytes;
	if (totalBytes && !total) total = 1;
	if (total > (UINT64)0xffffUL) total = (UINT64)0xffffUL;

	freec = freeBytes / clusterBytes;
	if (freec > total) freec = total;

	*sectorsPerCluster = spc;
	*bytesPerSector = bps;
	*totalClusters = (DWORD)total;
	*freeClusters = (DWORD)freec;
}

static UINT16 h9x_getdiskinfo(UINT32 pir)
{
	DWORD sectorsPerCluster = 0, bytesPerSector = 0, freeClusters = 0, totalClusters = 0;
	UINT64 clusterBytes;
	UINT64 clusters;
	UINT64 totalBytes;
	UINT64 freeBytes;

	#if defined(_WIN32) || defined(WIN32)
	TRACEOUT(("HOSTDRV9X GETDISKINFO: options=%04X root=[%ls] real=%u win95=%u fake=%u/%uMB",
		(unsigned)h9x_r16(pir, H9X_IR_OPTIONS), s_h9xRoot,
		(unsigned)s_h9xUseRealCapacity, (unsigned)s_h9xWin95Compat,
		(unsigned)s_h9xFakeFreeMB, (unsigned)s_h9xFakeTotalMB));
	#endif

	if (s_h9xUseRealCapacity) {
		if (!GetDiskFreeSpaceW(s_h9xRoot, &sectorsPerCluster, &bytesPerSector,
			&freeClusters, &totalClusters)) {
			UINT16 e = h9x_last_error();
			TRACEOUT(("HOSTDRV9X GETDISKINFO: GetDiskFreeSpaceW failed error=%u winerr=%u",
				(unsigned)e, (unsigned)GetLastError()));
			return e;
		}
		if (s_h9xWin95Compat) {
			clusterBytes = (UINT64)sectorsPerCluster * bytesPerSector;
			totalBytes = (UINT64)totalClusters * clusterBytes;
			freeBytes = (UINT64)freeClusters * clusterBytes;
			h9x_fit_win95_disk_geometry(totalBytes, freeBytes, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters);
		}
	}
	else {
		totalBytes = (UINT64)s_h9xFakeTotalMB * 1024 * 1024;
		freeBytes = (UINT64)s_h9xFakeFreeMB * 1024 * 1024;
		if (s_h9xWin95Compat) {
			h9x_fit_win95_disk_geometry(totalBytes, freeBytes, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters);
		}
		else {
			sectorsPerCluster = 8;
			bytesPerSector = 512;
			clusterBytes = (UINT64)sectorsPerCluster * bytesPerSector;
			clusters = totalBytes / clusterBytes;
			if (clusters > (UINT64)0xffffffffUL) clusters = (UINT64)0xffffffffUL;
			totalClusters = (DWORD)clusters;
			clusters = freeBytes / clusterBytes;
			if (clusters > totalClusters) clusters = totalClusters;
			freeClusters = (DWORD)clusters;
		}
	}

	TRACEOUT(("HOSTDRV9X GETDISKINFO: spc=%u bps=%u free=%u total=%u",
		(unsigned)sectorsPerCluster, (unsigned)bytesPerSector,
		(unsigned)freeClusters, (unsigned)totalClusters));
	h9x_w32(pir, H9X_IR_LENGTH, bytesPerSector);
	h9x_w16(pir, H9X_IR_SECTORS, (UINT16)sectorsPerCluster);
	h9x_w32(pir, H9X_IR_NUMFREE, freeClusters);
	h9x_w32(pir, H9X_IR_SIZE, totalClusters);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_flush(UINT32 pir)
{
	int i;
	(void)pir;
	for (i = 1; i < NP2HOSTDRV9X_FILES_MAX; i++) {
		if (s_h9x.files[i].type == H9X_HANDLE_FILE && s_h9x.files[i].handle != INVALID_HANDLE_VALUE)
			FlushFileBuffers(s_h9x.files[i].handle);
	}
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_handleinfo(UINT32 pir)
{
	NP2HOSTDRV9X_HANDLE *h = h9x_get_handle(h9x_r32(pir, H9X_IR_FH), H9X_HANDLE_FILE);

	TRACEOUT(("HOSTDRV9X HANDLEINFO: fh=%08X flags=%02X sfn=%04X len=%08X data=%08X aux1=%08X aux2=%08X aux3=%08X",
		(unsigned)h9x_r32(pir, H9X_IR_FH),
		(unsigned)h9x_r8(pir, H9X_IR_FLAGS),
		(unsigned)h9x_r16(pir, H9X_IR_SFN),
		(unsigned)h9x_r32(pir, H9X_IR_LENGTH),
		(unsigned)h9x_r32(pir, H9X_IR_DATA),
		(unsigned)h9x_r32(pir, H9X_IR_AUX1),
		(unsigned)h9x_r32(pir, H9X_IR_AUX2),
		(unsigned)h9x_r32(pir, H9X_IR_AUX3)));

	if (!h) {
		TRACEOUT(("HOSTDRV9X HANDLEINFO: invalid handle"));
		return H9X_ERROR_INVALID_HANDLE;
	}

	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_enumhandle(UINT32 pir)
{
	NP2HOSTDRV9X_HANDLE *h = h9x_get_handle(h9x_r32(pir, H9X_IR_FH), H9X_HANDLE_FILE);
	UINT8 fn = h9x_r8(pir, H9X_IR_FLAGS);
	UINT32 data = h9x_r32(pir, H9X_IR_DATA);
	if (!h) return H9X_ERROR_INVALID_HANDLE;
	if (fn == H9X_ENUMH_GETFILEINFO) {
		BY_HANDLE_FILE_INFORMATION info;
		if (!data || h9x_r32(pir, H9X_IR_LENGTH) < H9X_BY_HANDLE_INFO_SIZE) return H9X_ERROR_INVALID_PARAMETER;
		if (!GetFileInformationByHandle(h->handle, &info)) return h9x_last_error();
		h9x_w32(data, 0, info.dwFileAttributes);
		h9x_write_filetime(data + 4, &info.ftCreationTime);
		h9x_write_filetime(data + 12, &info.ftLastAccessTime);
		h9x_write_filetime(data + 20, &info.ftLastWriteTime);
		h9x_w32(data, 28, info.dwVolumeSerialNumber);
		h9x_w32(data, 32, info.nFileSizeHigh);
		h9x_w32(data, 36, info.nFileSizeLow);
		h9x_w32(data, 40, info.nNumberOfLinks);
		h9x_w32(data, 44, info.nFileIndexHigh);
		h9x_w32(data, 48, info.nFileIndexLow);
		h9x_w32(pir, H9X_IR_LENGTH, H9X_BY_HANDLE_INFO_SIZE);
		return H9X_ERROR_SUCCESS;
	}
	if (fn == H9X_ENUMH_GETFILENAME) {
		UINT32 ppath = h9x_r32(pir, H9X_IR_PPATH);
		const WCHAR *relative = h->path;
		if (!ppath) return H9X_ERROR_INVALID_PARAMETER;
		if (!_wcsnicmp(relative, s_h9xRoot, wcslen(s_h9xRoot)))
			relative += wcslen(s_h9xRoot);
		return h9x_write_parsed_path(ppath, relative);
	}
	if (fn == H9X_ENUMH_RESYNCFILEDIR) return H9X_ERROR_SUCCESS;
	return H9X_ERROR_INVALID_FUNCTION;
}

static UINT16 h9x_connect(UINT32 pir)
{
	UINT8 type = h9x_r8(pir, H9X_IR_FLAGS);
	if (type != H9X_RESTYPE_WILD && type != H9X_RESTYPE_DISK)
		return H9X_ERROR_BAD_NET_NAME;
	if (!h9x_is_our_resource(h9x_r32(pir, H9X_IR_PPATH)))
		return H9X_ERROR_BAD_NET_NAME;
	if (!s_h9xRoot[0] || !np2cfg.hdrvenable) return H9X_ERROR_NOT_READY;
	h9x_w32(pir, H9X_IR_RH, H9X_ROOT_HANDLE);
	h9x_w32(pir, H9X_IR_PATHSKIP, 2);
	return H9X_ERROR_SUCCESS;
}

static UINT16 h9x_query(UINT32 pir)
{
	static const char fsname[] = "HOSTFS";
	UINT16 level = h9x_r16(pir, H9X_IR_OPTIONS);
	TRACEOUT(("HOSTDRV9X QUERY: level=%u pir=%08X len=%08X flags=%02X ppath=%08X aux1=%08X data=%08X pos=%08X",
		(unsigned)level, (unsigned)pir, (unsigned)h9x_r32(pir, H9X_IR_LENGTH),
		(unsigned)h9x_r8(pir, H9X_IR_FLAGS), (unsigned)h9x_r32(pir, H9X_IR_PPATH),
		(unsigned)h9x_r32(pir, H9X_IR_AUX1), (unsigned)h9x_r32(pir, H9X_IR_DATA),
		(unsigned)h9x_r32(pir, H9X_IR_POS)));
	h9x_trace_parsed_path(h9x_r32(pir, H9X_IR_PPATH));

	if (level == 0) {
		UINT32 ppath = h9x_r32(pir, H9X_IR_PPATH);
		if (ppath) {
			UINT16 error = h9x_write_parsed_path(ppath, HD_W("NP2HOST\\HOSTFS"));
			if (error) return error;
		}
		h9x_w16(pir, H9X_IR_OPTIONS, H9X_RESSTAT_OK);
		h9x_w32(pir, H9X_IR_POS, 0);
		return H9X_ERROR_SUCCESS;
	}

	if (level == 2) {
		UINT32 data = h9x_r32(pir, H9X_IR_DATA);
		UINT32 capacity = h9x_r32(pir, H9X_IR_LENGTH);
		if (data && capacity) {
			UINT32 copy = sizeof(fsname);
			if (copy > capacity) copy = capacity;
			h9x_memwrite(data, fsname, copy);
			if (copy == capacity && capacity) cpu_kmemorywrite(data + capacity - 1, 0);
		}
		h9x_w16(pir, H9X_IR_OPTIONS, (UINT16)(H9X_FS_CASE_IS_PRESERVED |
			H9X_FS_UNICODE_STORED_ON_DISK | H9X_FS_VOL_SUPPORTS_LONG_NAMES));
		h9x_w32(pir, H9X_IR_POS, H9X_CACHE_BLOCK_SIZE);
		h9x_w32(pir, H9X_IR_LENGTH,
			((UINT32)H9X_MAX_PATH_LENGTH << 16) | H9X_MAX_COMPONENT_LENGTH);
		return H9X_ERROR_SUCCESS;
	}

	return H9X_ERROR_INVALID_FUNCTION;
}


static const char *h9x_fn_name(UINT32 fn)
{
	switch (fn) {
	case H9X_IFSFN_READ: return "READ";
	case H9X_IFSFN_WRITE: return "WRITE";
	case H9X_IFSFN_FINDNEXT: return "FINDNEXT";
	case H9X_IFSFN_SEEK: return "SEEK";
	case H9X_IFSFN_CLOSE: return "CLOSE";
	case H9X_IFSFN_COMMIT: return "COMMIT";
	case H9X_IFSFN_FILELOCKS: return "FILELOCKS";
	case H9X_IFSFN_FILETIMES: return "FILETIMES";
	case H9X_IFSFN_PIPEREQUEST: return "PIPEREQUEST";
	case H9X_IFSFN_HANDLEINFO: return "HANDLEINFO";
	case H9X_IFSFN_ENUMHANDLE: return "ENUMHANDLE";
	case H9X_IFSFN_FINDCLOSE: return "FINDCLOSE";
	case H9X_IFSFN_CONNECT: return "CONNECT";
	case H9X_IFSFN_DELETE: return "DELETE";
	case H9X_IFSFN_DIR: return "DIR";
	case H9X_IFSFN_FILEATTRIB: return "FILEATTRIB";
	case H9X_IFSFN_FLUSH: return "FLUSH";
	case H9X_IFSFN_GETDISKINFO: return "GETDISKINFO";
	case H9X_IFSFN_OPEN: return "OPEN";
	case H9X_IFSFN_RENAME: return "RENAME";
	case H9X_IFSFN_SEARCH: return "SEARCH";
	case H9X_IFSFN_QUERY: return "QUERY";
	case H9X_IFSFN_DISCONNECT: return "DISCONNECT";
	case H9X_IFSFN_UNCPIPEREQ: return "UNCPIPEREQ";
	case H9X_IFSFN_IOCTL16DRIVE: return "IOCTL16DRIVE";
	case H9X_IFSFN_GETDISKPARMS: return "GETDISKPARMS";
	case H9X_IFSFN_FINDOPEN: return "FINDOPEN";
	case H9X_IFSFN_DASDIO: return "DASDIO";
	default: return "UNKNOWN";
	}
}

#ifdef USE_HOSTDRV9X_TRACEOUT
static void h9x_trace_guest_bytes(const char* label, UINT32 addr, UINT32 len)
{
	UINT32 off;
	if (!addr) {
		TRACEOUT(("HOSTDRV9X %s: NULL", label));
		return;
	}
	if (len > 64) len = 64;
	for (off = 0; off < len; off += 16) {
		char line[160];
		char* q = line;
		UINT32 i;
		q += sprintf(q, "HOSTDRV9X %s %08X:", label, (unsigned)(addr + off));
		for (i = 0; i < 16 && off + i < len; i++)
			q += sprintf(q, " %02X", (unsigned)cpu_kmemoryread(addr + off + i));
		TRACEOUT(("%s", line));
	}
}

static void h9x_trace_parsed_path(UINT32 ppath)
{
	UINT16 total;
	UINT16 prefix;
	UINT32 off;
	UINT32 index = 0;
	if (!ppath) {
		TRACEOUT(("HOSTDRV9X PATH: NULL"));
		return;
	}
	total = cpu_kmemoryread_w(ppath);
	prefix = cpu_kmemoryread_w(ppath + 2);
	TRACEOUT(("HOSTDRV9X PATH: ppath=%08X total=%u prefix=%u",
		(unsigned)ppath, (unsigned)total, (unsigned)prefix));
	off = 4;
	while (off + 2 <= (UINT32)total + 2 && index < 16) {
		UINT16 elen = cpu_kmemoryread_w(ppath + off);
		WCHAR wbuf[MAX_PATH];
		char mbuf[MAX_PATH * 2];
		UINT32 chars;
		UINT32 i;
		if (!elen) {
			TRACEOUT(("HOSTDRV9X PATH: element[%u] END off=%u", (unsigned)index, (unsigned)off));
			break;
		}
		if (elen < 2 || off + elen >(UINT32)total + 2) {
			TRACEOUT(("HOSTDRV9X PATH: element[%u] INVALID elen=%u off=%u",
				(unsigned)index, (unsigned)elen, (unsigned)off));
			break;
		}
		chars = (elen - 2) / 2;
		if (chars >= NELEMENTS(wbuf)) chars = NELEMENTS(wbuf) - 1;
		for (i = 0; i < chars; i++) wbuf[i] = cpu_kmemoryread_w(ppath + off + 2 + i * 2);
		while (chars && !wbuf[chars - 1]) chars--;
		wbuf[chars] = 0;
		if (!WideCharToMultiByte(CP_ACP, 0, wbuf, -1, mbuf, sizeof(mbuf), NULL, NULL))
			strcpy(mbuf, "<convert-error>");
		TRACEOUT(("HOSTDRV9X PATH: element[%u] off=%u elen=%u text=[%s]",
			(unsigned)index, (unsigned)off, (unsigned)elen, mbuf));
		off += elen;
		index++;
	}
}
#else
static void h9x_trace_guest_bytes(const char* label, UINT32 addr, UINT32 len)
{
	// nothing to do
}
static void h9x_trace_parsed_path(UINT32 ppath)
{
	// nothing to do
}
#endif

static UINT16 h9x_dispatch(UINT32 fn, UINT32 pir)
{
	if (!pir) return H9X_ERROR_INVALID_PARAMETER;
	switch (fn) {
	case H9X_IFSFN_CONNECT: return h9x_connect(pir);
	case H9X_IFSFN_DISCONNECT: return H9X_ERROR_SUCCESS;
	case H9X_IFSFN_QUERY: return h9x_query(pir);
	case H9X_IFSFN_OPEN: return h9x_open(pir);
	case H9X_IFSFN_CLOSE: return h9x_close(pir);
	case H9X_IFSFN_READ: return h9x_readwrite(pir, 0);
	case H9X_IFSFN_WRITE: return h9x_readwrite(pir, 1);
	case H9X_IFSFN_SEEK: return h9x_seek(pir);
	case H9X_IFSFN_COMMIT: return h9x_commit(pir);
	case H9X_IFSFN_FILELOCKS: return h9x_filelocks(pir);
	case H9X_IFSFN_FILETIMES: return h9x_filetimes(pir);
	case H9X_IFSFN_DELETE: return h9x_delete(pir);
	case H9X_IFSFN_DIR: return h9x_dir(pir);
	case H9X_IFSFN_FILEATTRIB: return h9x_fileattrib(pir);
	case H9X_IFSFN_FLUSH: return h9x_flush(pir);
	case H9X_IFSFN_GETDISKINFO: return h9x_getdiskinfo(pir);
	case H9X_IFSFN_RENAME: return h9x_rename(pir);
	case H9X_IFSFN_SEARCH: return h9x_search(pir);
	case H9X_IFSFN_FINDOPEN: return h9x_findopen_common(pir, 1);
	case H9X_IFSFN_FINDNEXT: return h9x_findnext(pir);
	case H9X_IFSFN_FINDCLOSE: return h9x_findclose(pir);
	case H9X_IFSFN_HANDLEINFO: return h9x_handleinfo(pir);
	case H9X_IFSFN_ENUMHANDLE: return h9x_enumhandle(pir);
	case H9X_IFSFN_PIPEREQUEST:
	case H9X_IFSFN_UNCPIPEREQ:
	case H9X_IFSFN_IOCTL16DRIVE:
	case H9X_IFSFN_GETDISKPARMS:
	case H9X_IFSFN_DASDIO:
	default: return H9X_ERROR_INVALID_FUNCTION;
	}
}

static void h9x_invoke(void)
{
	NP2HOSTDRV9X_CALL call;
	UINT32 callAddr;
	UINT16 error;
	if (!s_h9x.dataAddr) return;
	callAddr = s_h9x.dataAddr;
#if defined(SUPPORT_IA32_HAXM)
	i386haxfunc_vcpu_getREGs(&np2haxstat.state);
	i386haxfunc_vcpu_getFPU(&np2haxstat.fpustate);
	np2haxstat.update_regs = np2haxstat.update_fpu = 0;
	ia32hax_copyregHAXtoNP2();
#endif
	h9x_memread(callAddr, &call, sizeof(call));
	if (call.signature != NP2HOSTDRV9X_CALL_SIGNATURE ||
		call.version != NP2HOSTDRV9X_CALL_VERSION || !call.ioreq) {
		TRACEOUT(("HOSTDRV9X INVOKE: invalid call sig=%08X ver=%08X fn=%u ioreq=%08X",
			(unsigned)call.signature, (unsigned)call.version,
			(unsigned)call.function, (unsigned)call.ioreq));
		return;
	}
	if (call.function >= NP2HOSTDRV9X_CONTROL_BASE) {
		TRACEOUT(("HOSTDRV9X CONTROL: fn=%08X data=%08X",
			(unsigned)call.function, (unsigned)call.ioreq));
		h9x_control_dispatch(call.function, call.ioreq);
		return;
	}
	TRACEOUT(("HOSTDRV9X CALL: fn=%u(%s) pir=%08X len=%08X flags=%02X user=%02X sfn=%04X pid=%08X ppath=%08X aux1=%08X data=%08X opt=%04X err=%04X rh=%08X fh=%08X pos=%08X aux2=%08X aux3=%08X",
		(unsigned)call.function, h9x_fn_name(call.function), (unsigned)call.ioreq,
		(unsigned)h9x_r32(call.ioreq, H9X_IR_LENGTH),
		(unsigned)h9x_r8(call.ioreq, H9X_IR_FLAGS),
		(unsigned)h9x_r8(call.ioreq, H9X_IR_USER),
		(unsigned)h9x_r16(call.ioreq, H9X_IR_SFN),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_PID),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_PPATH),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_AUX1),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_DATA),
		(unsigned)h9x_r16(call.ioreq, H9X_IR_OPTIONS),
		(unsigned)h9x_r16(call.ioreq, H9X_IR_ERROR),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_RH),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_FH),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_POS),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_AUX2),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_AUX3)));
	if (!s_h9xRoot[0] || !np2cfg.hdrvenable) error = H9X_ERROR_NOT_READY;
	else error = h9x_dispatch(call.function, call.ioreq);
	h9x_set_error(call.ioreq, error);
	TRACEOUT(("HOSTDRV9X RET : fn=%u(%s) error=%u len=%08X flags=%02X opt=%04X rh=%08X fh=%08X pos=%08X aux1=%08X data=%08X aux2=%08X aux3=%08X",
		(unsigned)call.function, h9x_fn_name(call.function), (unsigned)error,
		(unsigned)h9x_r32(call.ioreq, H9X_IR_LENGTH),
		(unsigned)h9x_r8(call.ioreq, H9X_IR_FLAGS),
		(unsigned)h9x_r16(call.ioreq, H9X_IR_OPTIONS),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_RH),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_FH),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_POS),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_AUX1),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_DATA),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_AUX2),
		(unsigned)h9x_r32(call.ioreq, H9X_IR_AUX3)));
}


static const char *h9x_diag_name(UINT8 tag)
{
	switch (tag) {
	case 0x01: return "init-enter";
	case 0x02: return "probe-values";
	case 0x03: return "probe-ok";
	case 0x04: return "ifsmgr-version";
	case 0x05: return "semaphore-result";
	case 0x06: return "registernet-begin";
	case 0x07: return "registernet-result";
	case 0x08: return "inituseadd-begin";
	case 0x09: return "inituseadd-result";
	case 0x0a: return "init-success";
	case 0x0b: return "dos-cds-hidden";
	case 0x11: return "already-initialized";
	case 0x20: return "connect-enter";
	case 0x21: return "connect-result";
	case 0x81: return "present-failed";
	case 0x82: return "probe-failed";
	case 0x84: return "ifsmgr-version-failed";
	case 0x85: return "semaphore-failed";
	case 0x87: return "registernet-failed";
	case 0x89: return "inituseadd-failed";
	case 0x8c: return "dos-cds-handoff-unavailable";
	default: return "unknown";
	}
}

#ifdef USE_HOSTDRV9X_TRACEOUT
static void IOOUTCALL h9x_o7e0(UINT port, REG8 dat)
{
	s_h9x.diagTag = dat;
	s_h9x.diagPos = 0;
	s_h9x.diagValue = 0;
	(void)port;
}

static void IOOUTCALL h9x_o7e2(UINT port, REG8 dat)
{
	if (s_h9x.diagPos < 4) {
		s_h9x.diagValue |= ((UINT32)dat << (s_h9x.diagPos * 8));
		s_h9x.diagPos++;
		if (s_h9x.diagPos == 4) {
			TRACEOUT(("HOSTDRV9X VXD DBG: tag=%02X (%s), value=%08X",
				s_h9x.diagTag, h9x_diag_name(s_h9x.diagTag), s_h9x.diagValue));
		}
	}
	(void)port;
}

static void IOOUTCALL h9x_o7e1(UINT port, REG8 dat)
{
	static int debugstridx = 0;
	static char dubugstr[256] = { 0 };
	if (dat < 0x20 || debugstridx >= sizeof(dubugstr) - 1) {
		if (debugstridx > 0) {
			dubugstr[debugstridx] = '\0';
			TRACEOUT(("HOSTDRV9x: %s", dubugstr));
		}
		debugstridx = 0;
	}
	else {
		dubugstr[debugstridx] = dat;
		debugstridx++;
	}
}
#endif

static void IOOUTCALL h9x_o7e4(UINT port, REG8 dat)
{
	s_h9x.dataAddr = ((UINT32)dat << 24) | (s_h9x.dataAddr >> 8);
	(void)port;
}

static void IOOUTCALL h9x_o7e6(UINT port, REG8 dat)
{
	static const char command[] = NP2HOSTDRV9X_COMMAND;
	if (dat == (REG8)command[s_h9x.commandPos]) {
		s_h9x.commandPos++;
		if (s_h9x.commandPos == sizeof(command) - 1) {
			/* ページフォールト時に戻ってこられるようにコマンド位置更新は成功まで保留 */
			s_h9x.commandPos = sizeof(command) - 2;
			TRACEOUT(("HOSTDRV9X CMD: invoke begin dataAddr=%08X", (unsigned)s_h9x.dataAddr));
			h9x_invoke();
			TRACEOUT(("HOSTDRV9X CMD: invoke complete dataAddr=%08X", (unsigned)s_h9x.dataAddr));
			s_h9x.commandPos = 0;
			s_h9x.dataAddr = 0;
		}
	} else {
		s_h9x.commandPos = (dat == (REG8)command[0]) ? 1 : 0;
	}
	(void)port;
}

static REG8 IOINPCALL h9x_i7e4(UINT port)
{
	(void)port;
	return NP2HOSTDRV9X_PROBE_ADDR;
}

static REG8 IOINPCALL h9x_i7e6(UINT port)
{
	(void)port;
	return NP2HOSTDRV9X_PROBE_CMD;
}

void hostdrv9x_updateHDrvRoot(void)
{
	TCHAR configured[MAX_PATH + 1];
	TCHAR full[MAX_PATH + 1];
	DWORD length;
	DWORD attributes;
	hostdrvs_invalidateshortnamecache();
	ZeroMemory(s_h9xRoot, sizeof(s_h9xRoot));
	s_h9xAcc = 0;
	if (_tcslen(np2cfg.hdrvroot) >= MAX_PATH) return;
	file_cpyname(configured, np2cfg.hdrvroot, NELEMENTS(configured));
	if (PathIsRelative(configured)) {
		TCHAR base[MAX_PATH + 1];
		TCHAR *last;
#if defined(WIN32) || defined(_WIN32)
		initgetfile(base, NELEMENTS(base));
#else
		{
			const OEMCHAR *hostBase = file_getcd(OEMTEXT(""));
			if (hostBase == NULL) return;
			file_cpyname((OEMCHAR *)base, hostBase, NELEMENTS(base));
		}
#endif
		last = _tcsrchr(base, '\\');
		if (last) *(last + 1) = 0; else base[0] = 0;
		if (_tcslen(base) + _tcslen(configured) >= MAX_PATH) return;
		_tcscat(base, configured);
		length = GetFullPathName(base, MAX_PATH, full, NULL);
	} else {
		length = GetFullPathName(configured, MAX_PATH, full, NULL);
	}
	if (!length || length >= MAX_PATH) return;
#ifdef UNICODE
	wcscpy(s_h9xRoot, full);
#else
	if (!MultiByteToWideChar(CP_ACP, 0, full, -1, s_h9xRoot, MAX_PATH)) {
		s_h9xRoot[0] = 0;
		return;
	}
#endif
	length = (DWORD)wcslen(s_h9xRoot);
	while (length > 3 && (s_h9xRoot[length - 1] == HD_WC('\\') || s_h9xRoot[length - 1] == HD_WC('/')))
		s_h9xRoot[--length] = 0;
	attributes = GetFileAttributesW(s_h9xRoot);
	if (attributes == INVALID_FILE_ATTRIBUTES ||
		!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
		s_h9xRoot[0] = 0;
		return;
	}
	s_h9xAcc = np2cfg.hdrvacc;
}

void hostdrv9x_initialize(void)
{
	ZeroMemory(&s_h9x, sizeof(s_h9x));
	hostdrvs_invalidateshortnamecache();
	h9x_config_defaults();
	hostdrv9x_updateHDrvRoot();
}

void hostdrv9x_deinitialize(void)
{
	h9x_close_all();
	hostdrvs_invalidateshortnamecache();
}

void hostdrv9x_reset(void)
{
	hostdrv9x_deinitialize();
	ZeroMemory(&s_h9x, sizeof(s_h9x));
	h9x_config_defaults();
	hostdrv9x_updateHDrvRoot();
}

void hostdrv9x_bind(void)
{
	if (!np2cfg.hdrvenable) return;

	// HOSTDRV for Win9x I/O ports
	iocore_attachout(NP2HOSTDRV9X_IO_ADDR, h9x_o7e4);
	iocore_attachout(NP2HOSTDRV9X_IO_CMD, h9x_o7e6);
	iocore_attachinp(NP2HOSTDRV9X_IO_ADDR, h9x_i7e4);
	iocore_attachinp(NP2HOSTDRV9X_IO_CMD, h9x_i7e6);

#ifdef USE_HOSTDRV9X_TRACEOUT
	// HOSTDRV for Win9x Debug I/O ports
	iocore_attachout(NP2HOSTDRV9X_IO_DIAG_TAG, h9x_o7e0);
	iocore_attachout(NP2HOSTDRV9X_IO_DIAG_DATA, h9x_o7e2);
	iocore_attachout(NP2HOSTDRV9X_IO_DIAG_TEXT, h9x_o7e1);
#endif
}

#define H9X_SF_FILES_MAGIC   0x31465839UL /* "9XF1" */
#define H9X_SF_FILES_VERSION 1UL

#pragma pack(push, 1)
typedef struct {
	UINT32 magic;
	UINT32 version;
	UINT32 count;
} H9X_SF_FILES_HEADER;

typedef struct {
	UINT32 index;
	UINT16 generation;
	UINT8 type;
	UINT8 reserved;
	UINT32 desiredAccess;
	UINT32 shareMode;
	UINT32 volumeSerialNumber;
	UINT32 fileIndexHigh;
	UINT32 fileIndexLow;
	UINT32 fileSizeHigh;
	UINT32 fileSizeLow;
	UINT32 lastWriteTimeLow;
	UINT32 lastWriteTimeHigh;
	WCHAR path[MAX_PATH];
} H9X_SF_FILE_RECORD;
#pragma pack(pop)

static int h9x_sf_file_identity(HANDLE file, H9X_SF_FILE_RECORD *record)
{
	BY_HANDLE_FILE_INFORMATION info;

	if (!record || file == NULL || file == INVALID_HANDLE_VALUE)
		return 0;
	if (!GetFileInformationByHandle(file, &info))
		return 0;

	record->volumeSerialNumber = info.dwVolumeSerialNumber;
	record->fileIndexHigh = info.nFileIndexHigh;
	record->fileIndexLow = info.nFileIndexLow;
	record->fileSizeHigh = info.nFileSizeHigh;
	record->fileSizeLow = info.nFileSizeLow;
	record->lastWriteTimeLow = info.ftLastWriteTime.dwLowDateTime;
	record->lastWriteTimeHigh = info.ftLastWriteTime.dwHighDateTime;
	return 1;
}

static int h9x_sf_file_identity_matches(const H9X_SF_FILE_RECORD *record,
	const BY_HANDLE_FILE_INFORMATION *info)
{
	if (!record || !info) return 0;
	return record->volumeSerialNumber == info->dwVolumeSerialNumber &&
		record->fileIndexHigh == info->nFileIndexHigh &&
		record->fileIndexLow == info->nFileIndexLow &&
		record->fileSizeHigh == info->nFileSizeHigh &&
		record->fileSizeLow == info->nFileSizeLow &&
		record->lastWriteTimeLow == info->ftLastWriteTime.dwLowDateTime &&
		record->lastWriteTimeHigh == info->ftLastWriteTime.dwHighDateTime;
}

int hostdrv9x_sfsave(STFLAGH sfh, const SFENTRY* tbl)
{
	NP2HOSTDRV9X_SFCONFIG config;
	H9X_SF_FILES_HEADER filesHeader;
	H9X_SF_FILE_RECORD record;
	UINT32 count;
	UINT32 i;
	int ret;

	count = 0;
	for (i = 1; i < NP2HOSTDRV9X_FILES_MAX; i++) {
		NP2HOSTDRV9X_HANDLE *h = &s_h9x.files[i];
		if (h->type == H9X_HANDLE_FILE &&
			h->handle != NULL && h->handle != INVALID_HANDLE_VALUE && h->path[0])
			count++;
	}

	ZeroMemory(&config, sizeof(config));
	config.version = 0;
	config.size = sizeof(config) + sizeof(filesHeader) +
		count * sizeof(H9X_SF_FILE_RECORD);
	config.useRealCapacity = s_h9xUseRealCapacity;
	config.win95Compat = s_h9xWin95Compat;
	config.fakeTotalMB = s_h9xFakeTotalMB;
	config.fakeFreeMB = s_h9xFakeFreeMB;

	ret = statflag_write(sfh, &config, sizeof(config));
	if (ret != STATFLAG_SUCCESS) return ret;

	filesHeader.magic = H9X_SF_FILES_MAGIC;
	filesHeader.version = H9X_SF_FILES_VERSION;
	filesHeader.count = count;
	ret = statflag_write(sfh, &filesHeader, sizeof(filesHeader));
	if (ret != STATFLAG_SUCCESS) return ret;

	for (i = 1; i < NP2HOSTDRV9X_FILES_MAX; i++) {
		NP2HOSTDRV9X_HANDLE *h = &s_h9x.files[i];
		if (h->type != H9X_HANDLE_FILE ||
			h->handle == NULL || h->handle == INVALID_HANDLE_VALUE || !h->path[0])
			continue;

		ZeroMemory(&record, sizeof(record));
		(void)h9x_sf_file_identity(h->handle, &record);
		record.index = i;
		record.generation = h->generation;
		record.type = H9X_HANDLE_FILE;
		record.desiredAccess = h->desiredAccess;
		record.shareMode = h->shareMode;
		wcsncpy(record.path, h->path, MAX_PATH - 1);
		record.path[MAX_PATH - 1] = L'\0';
		ret = statflag_write(sfh, &record, sizeof(record));
		if (ret != STATFLAG_SUCCESS) return ret;
	}

	(void)tbl;
	return STATFLAG_SUCCESS;
}

static int h9x_sfread_discard(STFLAGH sfh, UINT32 size)
{
	UINT8 discard[256];
	int ret;

	while (size) {
		UINT32 step = (size < (UINT32)sizeof(discard)) ?
			size : (UINT32)sizeof(discard);
		ret = statflag_read(sfh, discard, step);
		if (ret != STATFLAG_SUCCESS) return ret;
		size -= step;
	}
	return STATFLAG_SUCCESS;
}

static void h9x_sf_restore_file(const H9X_SF_FILE_RECORD *record)
{
	NP2HOSTDRV9X_HANDLE *h;
	BY_HANDLE_FILE_INFORMATION info;
	DWORD attr;
	DWORD flags;
	HANDLE file;

	if (!record || record->type != H9X_HANDLE_FILE ||
		record->index == 0 || record->index >= NP2HOSTDRV9X_FILES_MAX)
		return;

	h = &s_h9x.files[record->index];
	h->generation = record->generation;

	if (!record->path[0] || !h9x_is_safe_stored_path(record->path)) {
		TRACEOUT(("HOSTDRV9X SFLOAD: rejected saved path index=%u", (unsigned)record->index));
		return;
	}

	attr = GetFileAttributesW(record->path);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		TRACEOUT(("HOSTDRV9X SFLOAD: missing file index=%u path=[%ls]",
			(unsigned)record->index, record->path));
		return;
	}
	flags = (attr & FILE_ATTRIBUTE_DIRECTORY) ?
		FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL;

	file = CreateFileW(record->path, record->desiredAccess, record->shareMode, NULL, OPEN_EXISTING, flags, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		TRACEOUT(("HOSTDRV9X SFLOAD: reopen failed index=%u path=[%ls] winerr=%u",
			(unsigned)record->index, record->path, (unsigned)GetLastError()));
		return;
	}

	if (!GetFileInformationByHandle(file, &info) || !h9x_sf_file_identity_matches(record, &info)) {
		TRACEOUT(("HOSTDRV9X SFLOAD: file identity changed index=%u path=[%ls]",
			(unsigned)record->index, record->path));
		CloseHandle(file);
		return;
	}

	h->type = H9X_HANDLE_FILE;
	h->handle = file;
	h->desiredAccess = record->desiredAccess;
	h->shareMode = record->shareMode;
	wcsncpy(h->path, record->path, MAX_PATH - 1);
	h->path[MAX_PATH - 1] = L'\0';
	TRACEOUT(("HOSTDRV9X SFLOAD: restored file index=%u generation=%u cookie=%08X path=[%ls]",
		(unsigned)record->index, (unsigned)record->generation,
		(unsigned)h9x_make_cookie((int)record->index), h->path));
}

int hostdrv9x_sfload(STFLAGH sfh, const SFENTRY* tbl)
{
	NP2HOSTDRV9X_SFCONFIG config;
	H9X_SF_FILES_HEADER filesHeader;
	H9X_SF_FILE_RECORD record;
	UINT32 header[2];
	UINT32 savedSize;
	UINT32 payloadSize;
	UINT32 copySize;
	UINT32 extraSize;
	UINT32 i;
	const UINT32 headerSize = (UINT32)sizeof(header);
	const UINT32 configSize = (UINT32)sizeof(config);
	int ret;

	h9x_close_all();

	ZeroMemory(&config, sizeof(config));
	ret = statflag_read(sfh, header, sizeof(header));
	if (ret != STATFLAG_SUCCESS) return ret;
	if (header[0] != 0) return STATFLAG_VERSION;

	config.version = header[0];
	config.size = header[1];
	savedSize = header[1];
	if (savedSize < headerSize) return STATFLAG_FAILURE;

	payloadSize = savedSize - headerSize;
	copySize = payloadSize;
	if (copySize > configSize - headerSize)
		copySize = configSize - headerSize;

	if (copySize) {
		ret = statflag_read(sfh, ((UINT8*)&config) + headerSize, copySize);
		if (ret != STATFLAG_SUCCESS) return ret;
	}

	extraSize = (savedSize > configSize) ? savedSize - configSize : 0;
	if (payloadSize > copySize && savedSize <= configSize) {
		ret = h9x_sfread_discard(sfh, payloadSize - copySize);
		if (ret != STATFLAG_SUCCESS) return ret;
	}

	s_h9xUseRealCapacity = config.useRealCapacity ? 1 : 0;
	s_h9xWin95Compat = config.win95Compat ? 1 : 0;
	s_h9xFakeTotalMB = config.fakeTotalMB ?
		config.fakeTotalMB : NP2HOSTDRV9X_FAKE_TOTAL_MB_DEFAULT;
	if (config.fakeFreeMB <= s_h9xFakeTotalMB)
		s_h9xFakeFreeMB = config.fakeFreeMB;
	else
		s_h9xFakeFreeMB = s_h9xFakeTotalMB;

	if (extraSize >= sizeof(filesHeader)) {
		ret = statflag_read(sfh, &filesHeader, sizeof(filesHeader));
		if (ret != STATFLAG_SUCCESS) return ret;
		extraSize -= sizeof(filesHeader);

		if (filesHeader.magic == H9X_SF_FILES_MAGIC &&
			filesHeader.version == H9X_SF_FILES_VERSION &&
			filesHeader.count <= NP2HOSTDRV9X_FILES_MAX - 1 &&
			filesHeader.count <= extraSize / sizeof(record)) {
			for (i = 0; i < filesHeader.count; i++) {
				ret = statflag_read(sfh, &record, sizeof(record));
				if (ret != STATFLAG_SUCCESS) return ret;
				extraSize -= sizeof(record);
				record.path[MAX_PATH - 1] = L'\0';
				h9x_sf_restore_file(&record);
			}
		}
	}

	if (extraSize) {
		ret = h9x_sfread_discard(sfh, extraSize);
		if (ret != STATFLAG_SUCCESS) return ret;
	}

	(void)tbl;
	return STATFLAG_SUCCESS;
}

#endif /* SUPPORT_HOSTDRV9X */
