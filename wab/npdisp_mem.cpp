/**
 * @file	npdisp_mem.c
 * @brief	Implementation of the Neko Project II Display Adapter Memory Helper
 */

#include	"compiler.h"

#if defined(SUPPORT_WAB_NPDISP)

#include	<map>
#include	<vector>

#include	"pccore.h"
#include	"cpucore.h"

#include	"npdispdef.h"
#include	"npdisp.h"
#include	"npdisp_mem.h"
#include	"npdisp_palette.h"
#include	"wab.h"

extern NPDISP_WINDOWS	npdispwin;

#if 0
static void trace_fmt_exF(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUTF(s)	trace_fmt_exF s
#else
#define	TRACEOUTF(s)	(void)s
#endif	/* 1 */

#if 0
static void trace_fmt_exF(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT9(s)	trace_fmt_exF s
#else
#define	TRACEOUT9(s)	(void)s
#endif	/* 1 */

#if 0
static void trace_fmt_exDIBE(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUTDIBE(s)	trace_fmt_exDIBE s
#else
#define	TRACEOUTDIBE(s)	(void)s
#endif	/* 1 */

#if 0
static void trace_fmt_exF(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT10(s)	trace_fmt_exF s
#else
#define	TRACEOUT10(s)	(void)s
#endif	/* 1 */

#if 0
static void trace_fmt_exM(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUTM(s)	trace_fmt_exM s
#else
#define	TRACEOUTM(s)	(void)s
#endif	/* 1 */

static std::map<UINT32, NPDISP_MEMCACHE> npdisp_memcaches;

static NPDISP_MEMCACHE npdisp_common_memcache = { 0 };
static NPDISP_MEMCACHE* npdisp_current_memcache = &npdisp_common_memcache;

static UINT16 npdisp_selector_cache = 0; // 最後に使用したセレクタ
static UINT32 npdisp_seg_cache = 0; // 最後に使用したセレクタに対応するセグメント

static UINT32 npdisp_exception_eip = 0; // 例外発生時のEIPレジスタ

static sigjmp_buf npdisp_jmpbuf_bak; // 例外発生の捕捉用jmpbuf

/// <summary>
/// 先読みバッファの関数番号を指定　関数番号単位で独立なバッファを使用できる
/// </summary>
/// <param name="funcId">関数番号</param>
void npdisp_memory_setFunctionId(int funcId)
{
	if (funcId == 0) {
		// 共用バッファ
		npdisp_current_memcache = &npdisp_common_memcache;
	}
	else {
		// 関数毎バッファ
		auto it = npdisp_memcaches.find(funcId);
		if (it != npdisp_memcaches.end()) {
			npdisp_current_memcache = &(it->second);
		}
		else {
			NPDISP_MEMCACHE memcache = { 0 };
			memcache.funcId = funcId;
			npdisp_memcaches[funcId] = memcache;
			npdisp_current_memcache = &(npdisp_memcaches[funcId]);
		}
	}
}

/// <summary>
/// 全関数の先読みバッファや例外フラグ等を全てクリアする
/// </summary>
void npdisp_memory_clearallpreload()
{
	npdisp.longjmpnum = 0;

	npdisp_memcaches.clear();
	npdisp_common_memcache.npdisp_memwrite_bufwpos = 0;
	npdisp_common_memcache.npdisp_memread_buf.clear();
	npdisp_common_memcache.npdisp_memread_curpos = 0;
	npdisp_common_memcache.npdisp_memwrite_curpos = 0;
	npdisp_common_memcache.npdisp_memread_preloadcount = 0;
	npdisp_current_memcache = &npdisp_common_memcache;

	npdisp_memory_resetposition();
}

/// <summary>
/// 先読みバッファや例外フラグ等を全てクリアする
/// </summary>
void npdisp_memory_clearpreload()
{
	npdisp.longjmpnum = 0;

	if (npdisp_current_memcache) {
		npdisp_current_memcache->npdisp_memwrite_bufwpos = 0;
		npdisp_current_memcache->npdisp_memread_buf.clear();
		npdisp_current_memcache->npdisp_memread_curpos = 0;
		npdisp_current_memcache->npdisp_memwrite_curpos = 0;
		npdisp_current_memcache->npdisp_memread_preloadcount = 0;
	}
	if (npdisp_current_memcache == &npdisp_common_memcache) {
		npdisp.longjmpnum_nonfast = 0;
	}

	npdisp_memory_resetposition();
}
/// <summary>
/// メモリ読み書き開始位置を先頭へ戻す
/// </summary>
void npdisp_memory_resetposition()
{
	npdisp_exception_eip = CPU_EIP;
	npdisp.longjmpnum = 0;
	npdisp_selector_cache = 0;
	npdisp_seg_cache = 0;

	if (npdisp_current_memcache) {
		npdisp_current_memcache->npdisp_memread_curpos = 0;
		npdisp_current_memcache->npdisp_memwrite_curpos = 0;
		npdisp_current_memcache->npdisp_memread_preloadcount = 0;

		// バッファのサイズを先に取得しておく データ読み書きが進んでいるかの確認用
		npdisp_current_memcache->last_npdisp_memread_bufsize = npdisp_current_memcache->npdisp_memread_buf.size();
		npdisp_current_memcache->last_npdisp_memwrite_bufwpos = npdisp_current_memcache->npdisp_memwrite_bufwpos;
	}
	if (npdisp_current_memcache == &npdisp_common_memcache) {
		npdisp.longjmpnum_nonfast = 0;
	}
}
/// <summary>
/// 前回のnpdisp_memory_resetposition実行時から新たなデータ読み書きがあった場合は0以外を返す
/// </summary>
int npdisp_memory_hasNewCacheData()
{
	if (!npdisp_current_memcache) return 0;
	return npdisp_current_memcache->last_npdisp_memread_bufsize != npdisp_current_memcache->npdisp_memread_buf.size() || npdisp_current_memcache->last_npdisp_memwrite_bufwpos != npdisp_current_memcache->npdisp_memwrite_bufwpos;
}
/// <summary>
/// バッファの読み取りデータサイズを返す
/// </summary>
int npdisp_memory_getTotalReadSize()
{
	if (!npdisp_current_memcache) return 0;
	return npdisp_current_memcache->npdisp_memread_buf.size();
}
/// <summary>
/// バッファの書き込みデータサイズを返す
/// </summary>
int npdisp_memory_getTotalWriteSize()
{
	if (!npdisp_current_memcache) return 0;
	return npdisp_current_memcache->npdisp_memwrite_bufwpos;
}
/// <summary>
/// 読み取り開始時のEIPレジスタを返す
/// </summary>
UINT32 npdisp_memory_getLastEIP()
{
	return npdisp_exception_eip;
}

/***
 * セレクタ:オフセット形式のメモリを読み取るために使う関数群
 * セレクタ→セグメント→ページング→実メモリアドレス　の流れ
 ***/

/// <summary>
/// セレクタとオフセットからリニアアドレスを計算
/// </summary>
/// <param name="selector">セレクタ</param>
/// <param name="offset">オフセット</param>
/// <param name="lplAddr">リニアアドレス</param>
/// <returns>成功したら0以外、失敗したら0</returns>
static UINT32 selector_to_linear(UINT16 selector, UINT32 offset, UINT32 *lplAddr)
{
	selector_t sel;
	int rv;

	// 高速化 前回読み取りと同じセレクタなら使い回す
	if (selector == npdisp_selector_cache) {
		*lplAddr = npdisp_seg_cache + offset;
		return 1;
	}

	memset(&sel, 0, sizeof(sel));

	// セレクタを読み取り
	rv = parse_selector(&sel, selector);
	if (rv == 0) {
		// OK
		npdisp_selector_cache = selector;
		npdisp_seg_cache = sel.desc.u.seg.segbase; // セグメントを取得
		*lplAddr = sel.desc.u.seg.segbase + offset;
		return 1;
	}
	// Fail
	return 0;
}

/// <summary>
/// 指定したリニアアドレスを読み取って先読みバッファへ送る。先にページフォールトの発生を確認するために使用。
/// </summary>
/// <param name="vaddr">リニアアドレス</param>
/// <param name="size">読み取りサイズ</param>
/// <returns>成功は0以外、ページフォールトが発生した場合は0を返す</returns>
static int npdisp_preloadLMemory(UINT32 vaddr, UINT32 size)
{
	static UINT8 npdisp_memBuf[CPU_PAGE_SIZE];
	UINT32 readaddr = vaddr;
	UINT32 readsize = size;
	if (npdisp.longjmpnum) return 0;
	memcpy(npdisp_jmpbuf_bak, exec_1step_jmpbuf, sizeof(exec_1step_jmpbuf)); // 現在のsetjmpを退避
	npdisp.longjmpnum = sigsetjmp(exec_1step_jmpbuf, 1); // 新しい位置にセット
	if (npdisp.longjmpnum == 0) {
		// 既に読み取り済みの範囲ならそれを返す
		if (npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount < npdisp_current_memcache->npdisp_memread_buf.size()) {
			UINT32 mrsize = min(readsize, npdisp_current_memcache->npdisp_memread_buf.size() - (npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount));
			readsize -= mrsize;
			readaddr += mrsize;
			npdisp_current_memcache->npdisp_memread_preloadcount += mrsize;
		}

		// ページ単位で読みとり
		while (readsize > 0) {
			UINT32 inPageSize = CPU_PAGE_SIZE - (readaddr & CPU_PAGE_MASK);
			inPageSize = min(inPageSize, readsize);
			cpu_lmemoryreads(readaddr, npdisp_memBuf, inPageSize, CPU_PAGE_READ_DATA | CPU_MODE_SUPERVISER);
			npdisp_current_memcache->npdisp_memread_buf.insert(npdisp_current_memcache->npdisp_memread_buf.end(), npdisp_memBuf, npdisp_memBuf + inPageSize);
			readsize -= inPageSize;
			readaddr += inPageSize;
			npdisp_current_memcache->npdisp_memread_preloadcount += inPageSize;
		}
	}
	else {
		TRACEOUTF(("EXCEPTION Jump!"));
	}
	memcpy(exec_1step_jmpbuf, npdisp_jmpbuf_bak, sizeof(exec_1step_jmpbuf)); // setjmpを元に戻す
	return !npdisp.longjmpnum;
}
/// <summary>
/// 指定したリニアアドレスを読み取って先読みバッファへ送ると同時に、読んだデータも取得。
/// </summary>
/// <param name="vaddr">リニアアドレス</param>
/// <param name="size">読み取りサイズ</param>
/// <returns>成功は0以外、ページフォールトが発生した場合は0を返す</returns>
static int npdisp_preloadAndReadLMemory(UINT32 vaddr, void* buffer, UINT32 size)
{
	UINT32 readaddr = vaddr;
	UINT32 readsize = size;
	UINT8* readptr = (UINT8*)buffer;
	if (npdisp.longjmpnum) return 0;
	memcpy(npdisp_jmpbuf_bak, exec_1step_jmpbuf, sizeof(exec_1step_jmpbuf)); // 現在のsetjmpを退避
	npdisp.longjmpnum = sigsetjmp(exec_1step_jmpbuf, 1); // 新しい位置にセット
	if (npdisp.longjmpnum == 0) {
		// 既に読み取り済みの範囲ならそれを返す
		if (npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount < npdisp_current_memcache->npdisp_memread_buf.size()) {
			UINT32 mrsize = min(readsize, npdisp_current_memcache->npdisp_memread_buf.size() - (npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount));
			*readptr = npdisp_current_memcache->npdisp_memread_buf[npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount];
			memcpy(readptr, &npdisp_current_memcache->npdisp_memread_buf[npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount], mrsize);
			readsize -= mrsize;
			readptr += mrsize;
			readaddr += mrsize;
			npdisp_current_memcache->npdisp_memread_preloadcount += mrsize;
		}

		// ページ単位で読みとり
		while (readsize > 0) {
			UINT32 inPageSize = CPU_PAGE_SIZE - (readaddr & CPU_PAGE_MASK);
			inPageSize = min(inPageSize, readsize);
			cpu_lmemoryreads(readaddr, readptr, inPageSize, CPU_PAGE_READ_DATA | CPU_MODE_SUPERVISER);
			npdisp_current_memcache->npdisp_memread_buf.insert(npdisp_current_memcache->npdisp_memread_buf.end(), readptr, readptr + inPageSize);
			readsize -= inPageSize;
			readptr += inPageSize;
			readaddr += inPageSize;
			npdisp_current_memcache->npdisp_memread_preloadcount += inPageSize;
		}
	}
	else {
		TRACEOUTF(("EXCEPTION Jump!"));
	}
	memcpy(exec_1step_jmpbuf, npdisp_jmpbuf_bak, sizeof(exec_1step_jmpbuf)); // setjmpを元に戻す
	return !npdisp.longjmpnum;
}
/// <summary>
/// 指定したリニアアドレスを読み取って指定したバッファへ送る。既に読み取り済みのデータや先読みデータがある場合はそこから読み取る。
/// </summary>
/// <param name="vaddr">リニアアドレス</param>
/// <param name="buffer">読み取ったデータを格納するバッファ</param>
/// <param name="size">読み取りサイズ</param>
/// <returns>成功は0以外、ページフォールトが発生した場合は0を返す</returns>
static int npdisp_readLMemory(UINT32 vaddr, void* buffer, UINT32 size)
{
	int inCurPos = npdisp_current_memcache->npdisp_memread_curpos;
	UINT32 readaddr = vaddr;
	UINT32 readsize = size;
	UINT8* readptr = (UINT8*)buffer;
	if (npdisp.longjmpnum) return 0;
	memcpy(npdisp_jmpbuf_bak, exec_1step_jmpbuf, sizeof(exec_1step_jmpbuf)); // 現在のsetjmpを退避
	npdisp.longjmpnum = sigsetjmp(exec_1step_jmpbuf, 1); // 新しい位置にセット
	if (npdisp.longjmpnum == 0) {
		// 既に読み取り済みの範囲ならそれを返す
		if (npdisp_current_memcache->npdisp_memread_curpos < npdisp_current_memcache->npdisp_memread_buf.size()) {
			UINT32 mrsize = min(readsize, npdisp_current_memcache->npdisp_memread_buf.size() - npdisp_current_memcache->npdisp_memread_curpos);
			*readptr = npdisp_current_memcache->npdisp_memread_buf[npdisp_current_memcache->npdisp_memread_curpos];
			memcpy(readptr, &npdisp_current_memcache->npdisp_memread_buf[npdisp_current_memcache->npdisp_memread_curpos], mrsize);
			readsize -= mrsize;
			readptr += mrsize;
			readaddr += mrsize;
			npdisp_current_memcache->npdisp_memread_curpos += mrsize;
			if (npdisp_current_memcache->npdisp_memread_preloadcount >= mrsize) {
				npdisp_current_memcache->npdisp_memread_preloadcount -= mrsize;
			}
			else {
				npdisp_current_memcache->npdisp_memread_preloadcount = 0;
			}
		}

		// ページ単位で読みとり
		while (readsize > 0) {
			UINT32 inPageSize = CPU_PAGE_SIZE - (readaddr & CPU_PAGE_MASK);
			inPageSize = min(inPageSize, readsize);
			cpu_lmemoryreads(readaddr, readptr, inPageSize, CPU_PAGE_READ_DATA | CPU_MODE_SUPERVISER);
			npdisp_current_memcache->npdisp_memread_buf.insert(npdisp_current_memcache->npdisp_memread_buf.end(), readptr, readptr + inPageSize);
			npdisp_current_memcache->npdisp_memread_curpos += inPageSize;
			readsize -= inPageSize;
			readptr += inPageSize;
			readaddr += inPageSize;
			if (npdisp_current_memcache->npdisp_memread_preloadcount > inPageSize) {
				npdisp_current_memcache->npdisp_memread_preloadcount -= inPageSize;
			}
			else {
				npdisp_current_memcache->npdisp_memread_preloadcount = 0;
			}
		}
	}
	else {
		TRACEOUTF(("EXCEPTION Jump!"));
	}
	memcpy(exec_1step_jmpbuf, npdisp_jmpbuf_bak, sizeof(exec_1step_jmpbuf)); // setjmpを元に戻す
	return !npdisp.longjmpnum;
}
/// <summary>
/// 指定したリニアアドレスをへデータを書き込む。既に書き込み済みの場合はスキップする。
/// </summary>
/// <param name="vaddr">リニアアドレス</param>
/// <param name="buffer">書き込むデータ</param>
/// <param name="size">読み取りサイズ</param>
/// <returns>成功は0以外、ページフォールトが発生した場合は0を返す</returns>
static int npdisp_writeLMemory(UINT32 vaddr, void* buffer, UINT32 size)
{
	UINT32 writeaddr = vaddr;
	UINT32 writesize = size;
	UINT8* writeptr = (UINT8*)buffer;
	if (npdisp.longjmpnum) return 0;
	memcpy(npdisp_jmpbuf_bak, exec_1step_jmpbuf, sizeof(exec_1step_jmpbuf)); // 現在のsetjmpを退避
	npdisp.longjmpnum = sigsetjmp(exec_1step_jmpbuf, 1); // 新しい位置にセット
	if (npdisp.longjmpnum == 0) {
		// 既に書き込み済みの範囲ならスキップ
		UINT32 wnsize = npdisp_current_memcache->npdisp_memwrite_bufwpos - npdisp_current_memcache->npdisp_memwrite_curpos;
		if (wnsize >= size) {
			// 全部書き込み済み
			npdisp_current_memcache->npdisp_memwrite_curpos += size;
		}
		else {
			// 書き込み済み分があればスキップ
			writesize -= wnsize;
			writeptr += wnsize;
			writeaddr += wnsize;
			npdisp_current_memcache->npdisp_memwrite_curpos += wnsize;

			// ページ単位で書き込み
			while (writesize > 0) {
				UINT32 inPageSize = CPU_PAGE_SIZE - (writeaddr & CPU_PAGE_MASK);
				inPageSize = min(inPageSize, writesize);
				cpu_lmemorywrites(writeaddr, writeptr, inPageSize, CPU_PAGE_WRITE_DATA | CPU_MODE_SUPERVISER);
				npdisp_current_memcache->npdisp_memwrite_bufwpos += inPageSize;
				npdisp_current_memcache->npdisp_memwrite_curpos += inPageSize;
				writesize -= inPageSize;
				writeptr += inPageSize;
				writeaddr += inPageSize;
			}
		}
	}
	else {
		TRACEOUTF(("EXCEPTION Jump!"));
	}
	memcpy(exec_1step_jmpbuf, npdisp_jmpbuf_bak, sizeof(exec_1step_jmpbuf)); // setjmpを元に戻す
	return !npdisp.longjmpnum;
}

int npdisp_preloadAndReadMemoryWith32Offset(void* dst, UINT16 selector, UINT32 offset, int size)
{
	UINT16 seg = selector;
	UINT32 linearAddr;
	if (!selector) return 0;
	if (npdisp.longjmpnum) return 0;
	// 既に読み取り済みの範囲ならそれを返す
	if (npdisp_current_memcache->npdisp_memread_buf.size() - (int)(npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount) >= size) {
		UINT8* readptr = (UINT8*)dst;
		memcpy(readptr, &(npdisp_current_memcache->npdisp_memread_buf[npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount]), size);
		npdisp_current_memcache->npdisp_memread_preloadcount += size;
		return !npdisp.longjmpnum;
	}
	if (selector_to_linear(seg, offset, &linearAddr)) { // offsetを32bitで扱う
		return npdisp_preloadAndReadLMemory(linearAddr, dst, size);
	}
	return 0;
}
int npdisp_preloadAndReadMemory(void* dst, UINT32 lpAddr, int size)
{
	UINT16 seg = (lpAddr >> 16) & 0xffff;
	UINT16 ofs = lpAddr & 0xffff;
	UINT32 linearAddr;
	if (!lpAddr) return 0;
	if (npdisp.longjmpnum) return 0;
	// 既に読み取り済みの範囲ならそれを返す
	if (npdisp_current_memcache->npdisp_memread_buf.size() - (int)(npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount) >= size) {
		UINT8* readptr = (UINT8*)dst;
		memcpy(readptr, &(npdisp_current_memcache->npdisp_memread_buf[npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount]), size);
		npdisp_current_memcache->npdisp_memread_preloadcount += size;
		return !npdisp.longjmpnum;
	}
	if (selector_to_linear(seg, ofs, &linearAddr)) {
		return npdisp_preloadAndReadLMemory(linearAddr, dst, size);
	}
	return 0;
}
int npdisp_preloadMemoryWith32Offset(UINT16 selector, UINT32 offset, int size)
{
	UINT16 seg = selector;
	UINT32 linearAddr;
	if (!selector) return 0;
	if (npdisp.longjmpnum) return 0;
	// 既に読み取り済みの範囲ならそれを返す
	if (npdisp_current_memcache->npdisp_memread_buf.size() - (int)(npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount) >= size) {
		npdisp_current_memcache->npdisp_memread_preloadcount += size;
		return !npdisp.longjmpnum;
	}
	if (selector_to_linear(seg, offset, &linearAddr)) { // offsetを32bitで扱う
		return npdisp_preloadLMemory(linearAddr, size);
	}
	return 0;
}
int npdisp_preloadMemory(UINT32 lpAddr, int size)
{
	UINT16 seg = (lpAddr >> 16) & 0xffff;
	UINT16 ofs = lpAddr & 0xffff;
	UINT32 linearAddr;
	if (!lpAddr) return 0;
	if (npdisp.longjmpnum) return 0;
	// 既に読み取り済みの範囲ならそれを返す
	if (npdisp_current_memcache->npdisp_memread_buf.size() - (int)(npdisp_current_memcache->npdisp_memread_curpos + npdisp_current_memcache->npdisp_memread_preloadcount) >= size) {
		npdisp_current_memcache->npdisp_memread_preloadcount += size;
		return !npdisp.longjmpnum;
	}
	if (selector_to_linear(seg, ofs, &linearAddr)) {
		return npdisp_preloadLMemory(linearAddr, size);
	}
	return 0;
}
int npdisp_readMemoryWith32Offset(void* dst, UINT16 selector, UINT32 offset, int size)
{
	UINT16 seg = selector;
	UINT32 linearAddr;
	if (!selector) return 0;
	if (npdisp.longjmpnum) return 0;
	// 既に読み取り済みの範囲ならそれを返す
	if (npdisp_current_memcache->npdisp_memread_buf.size() - (int)npdisp_current_memcache->npdisp_memread_curpos >= size) {
		UINT8* readptr = (UINT8*)dst;
		memcpy(readptr, &(npdisp_current_memcache->npdisp_memread_buf[npdisp_current_memcache->npdisp_memread_curpos]), size);
		npdisp_current_memcache->npdisp_memread_curpos += size;
		if (npdisp_current_memcache->npdisp_memread_preloadcount > size) {
			npdisp_current_memcache->npdisp_memread_preloadcount -= size;
		}
		else {
			npdisp_current_memcache->npdisp_memread_preloadcount = 0;
		}
		return !npdisp.longjmpnum;
	}
	if (selector_to_linear(seg, offset, &linearAddr)) { // offsetを32bitで扱う
		return npdisp_readLMemory(linearAddr, dst, size);
	}
	return 0;
}
int npdisp_readMemory(void* dst, UINT32 lpAddr, int size) 
{
	UINT16 seg = (lpAddr >> 16) & 0xffff;
	UINT16 ofs = lpAddr & 0xffff;
	UINT32 linearAddr;
	if (!lpAddr) return 0;
	if (npdisp.longjmpnum) return 0;
	// 既に読み取り済みの範囲ならそれを返す
	if (npdisp_current_memcache->npdisp_memread_buf.size() - (int)npdisp_current_memcache->npdisp_memread_curpos >= size) {
		UINT8* readptr = (UINT8*)dst;
		memcpy(readptr, &(npdisp_current_memcache->npdisp_memread_buf[npdisp_current_memcache->npdisp_memread_curpos]), size);
		npdisp_current_memcache->npdisp_memread_curpos += size;
		if (npdisp_current_memcache->npdisp_memread_preloadcount > size) {
			npdisp_current_memcache->npdisp_memread_preloadcount -= size;
		}
		else {
			npdisp_current_memcache->npdisp_memread_preloadcount = 0;
		}
		return !npdisp.longjmpnum;
	}
	if (selector_to_linear(seg, ofs, &linearAddr)) {
		return npdisp_readLMemory(linearAddr, dst, size);
	}
	return 0;
}
int npdisp_writeMemoryWith32Offset(void* src, UINT16 selector, UINT32 offset, int size)
{
	UINT16 seg = selector;
	UINT32 linearAddr;
	if (!selector) return 0;
	if (npdisp.longjmpnum) return 0;
	// 既に書き込み済みの範囲なら何もしない
	if ((int)npdisp_current_memcache->npdisp_memwrite_bufwpos - (int)npdisp_current_memcache->npdisp_memwrite_curpos >= size) {
		npdisp_current_memcache->npdisp_memwrite_curpos += size;
		return !npdisp.longjmpnum;
	}
	if (selector_to_linear(seg, offset, &linearAddr))
	{
		return npdisp_writeLMemory(linearAddr, src, size);
	}
	return 0;
}
int npdisp_writeMemory(void* src, UINT32 lpAddr, int size) 
{
	UINT16 seg = (lpAddr >> 16) & 0xffff;
	UINT16 ofs = lpAddr & 0xffff;
	UINT32 linearAddr;
	if (!lpAddr) return 0;
	if (npdisp.longjmpnum) return 0;
	// 既に書き込み済みの範囲なら何もしない
	if ((int)npdisp_current_memcache->npdisp_memwrite_bufwpos - (int)npdisp_current_memcache->npdisp_memwrite_curpos >= size) {
		npdisp_current_memcache->npdisp_memwrite_curpos += size;
		return !npdisp.longjmpnum;
	}
	if (selector_to_linear(seg, ofs, &linearAddr)) 
	{
		return npdisp_writeLMemory(linearAddr, src, size);
	}
	return 0;
}
UINT8 npdisp_readMemory8With32Offset(UINT16 selector, UINT32 offset)
{
	UINT8 dst = 0;
	npdisp_readMemoryWith32Offset(&dst, selector, offset, 1);
	return dst;
}
UINT8 npdisp_readMemory8(UINT32 lpAddr) 
{
	UINT8 dst = 0;
	npdisp_readMemory(&dst, lpAddr, 1);
	return dst;
}
UINT16 npdisp_readMemory16(UINT32 lpAddr) 
{
	UINT16 dst = 0;
	npdisp_readMemory(&dst, lpAddr, 2);
	return dst;
}
UINT32 npdisp_readMemory32(UINT32 lpAddr) 
{
	UINT32 dst = 0;
	npdisp_readMemory(&dst, lpAddr, 4);
	return dst;
}
int npdisp_writeMemory8(UINT8 value, UINT32 lpAddr) 
{
	return npdisp_writeMemory(&value, lpAddr, 1);
}
int npdisp_writeMemory16(UINT16 value, UINT32 lpAddr) 
{
	return npdisp_writeMemory(&value, lpAddr, 2);
}
int npdisp_writeMemory32(UINT32 value, UINT32 lpAddr) 
{
	return npdisp_writeMemory(&value, lpAddr, 4);
}
void npdisp_writeReturnCode(NPDISP_REQUEST *lpReq, UINT32 dataAddr, UINT16 retCode) 
{
	lpReq->returnCode = retCode;
	npdisp_writeMemory((UINT8*)lpReq + 4, npdisp.dataAddr + 4, 2); // ReturnCode書き込み
}

char* npdisp_readMemoryString(UINT32 lpAddr) 
{
	char *strBuf;
	int addr;
	int len;
	if (!lpAddr) return NULL;
	for (addr = lpAddr; ((addr ^ lpAddr) & 0xffff0000) == 0 && npdisp_readMemory8(addr); addr++); // NULL文字がでるか、セグメントが変わる(=異常)まで回す
	if (((addr ^ lpAddr) & 0xffff0000) != 0) return NULL; // セグメントにめり込むサイズは異常

	len = addr - lpAddr + 1; // 長さ計算 NULL文字も含む

	strBuf = (char*)malloc(len);
	if (!strBuf) return NULL;
	if (!npdisp_readMemory(strBuf, lpAddr, len)) {
		free(strBuf);
		return NULL;
	}
	return strBuf;
}
char* npdisp_readMemoryStringWithCount(UINT32 lpAddr, int count)
{
	char* strBuf;
	int len;
	if (!lpAddr) return NULL;
	if (count <= 0) return NULL;

	strBuf = (char*)malloc(count + 1);
	if (!strBuf) return NULL;
	if (!npdisp_readMemory(strBuf, lpAddr, count)) {
		free(strBuf);
		return NULL;
	}
	strBuf[count] = '\0';
	return strBuf;
}

bool npdisp_isDisplayDevice(UINT32 lpAddr)
{
	UINT16 type = npdisp_readMemory16(lpAddr);
	if (type == NPDISP_DEVTYPE_DIBENG) {
		// DIBエンジンかもしれない
		NPDISP_PDEVICE pdev;
		if (npdisp_readMemory(&pdev, lpAddr, sizeof(NPDISP_PDEVICE))) {
			if ((pdev.dibe.deFlags & 0x25) == 0x04) { // NOT_FRAMEBUFFER, MINIDRIVER, SELECTEDDIBフラグを見る
				// SELECTEDDIBだけ立っていたらDIBセクション
				return false;
			}
		}
		// デバイスっぽい
		return true;
	}
	else if (type == NPDISP_DEVTYPE) {
		// デバイスで確定
		return true;
	}
	// それ以外
	return false;
}

UINT32 npdisp_readPBitmap(NPDISP_PBITMAP_EXT *bmp, UINT32 lpAddr, bool useSelected)
{
	UINT16 type = npdisp_readMemory16(lpAddr);
	if (type == NPDISP_DEVTYPE_DDB) {
		npdisp_readMemory(bmp, lpAddr, sizeof(NPDISP_PBITMAP_EXT));
	}
	else if (type == NPDISP_DEVTYPE_DIBENG) {
		// 必要情報はNPDISP_PBITMAP_EXTの範囲に収まるので、その範囲で読む
		npdisp_readMemory(bmp, lpAddr, sizeof(NPDISP_PBITMAP_EXT));
	}
	else {
		npdisp_readMemory(bmp, lpAddr, sizeof(NPDISP_PBITMAP));
		bmp->ddbmpKey = 0;
	}
	//if (bmp->bmWidth == 413 && (bmp->bmHeight == 146 || bmp->bmHeight == -146)) {
	//	bmp->ddbmpKey = bmp->ddbmpKey;
	//}
	return npdisp.longjmpnum == 0;
}

UINT32 npdisp_writePBitmap(NPDISP_PBITMAP_EXT* bmp, UINT32 lpAddr)
{
	if (bmp->bmType == NPDISP_DEVTYPE_DDB) {
		npdisp_writeMemory(bmp, lpAddr, sizeof(NPDISP_PBITMAP_EXT));
	}
	else if (bmp->bmType == NPDISP_DEVTYPE_DIBENG) {
		// 必要情報はNPDISP_PBITMAP_EXTの範囲に収まるので、その範囲で書く
		npdisp_writeMemory(bmp, lpAddr, sizeof(NPDISP_PBITMAP_EXT));
	}
	else {
		npdisp_writeMemory(bmp, lpAddr, sizeof(NPDISP_PBITMAP));
	}
	return npdisp.longjmpnum == 0;
}

// メモリ先読み
// 注意：これを呼んだ後にnpdisp_MakeBitmapFromPBITMAPをすぐに呼ぶこと。間に別のreadを噛ませてはいけない。
// 　　　また、複数npdisp_PreloadBitmapFromPBITMAPを呼んで複数npdisp_MakeBitmapFromPBITMAPしても構わないが、引数や呼ぶ順番を変えてはならない
void npdisp_PreloadBitmapFromPBITMAP(NPDISP_PBITMAP_EXT* srcPBmp, int dcIdx, int beginLine, int numLines, int beginX, int copyWidth) {
	if (npdisp.longjmpnum != 0) return;

	// DDBitmapキーが有効か確認
	if (srcPBmp->bmType == NPDISP_DEVTYPE_DDB && srcPBmp->ddbmpKey) {
		// DDBitmapを返すので読み込み不要
		return;
	}

	int i, j;
	int bpp = srcPBmp->bmPlanes * srcPBmp->bmBitsPixel;
	int	srcstride = srcPBmp->bmWidthBytes;

	UINT32 lpbiLen = 0;
	UINT16 bmBitsAddrSel = (srcPBmp->bmBitsAddr >> 16) & 0xffff;
	UINT32 bmBitsAddrOfs = srcPBmp->bmBitsAddr & 0xffff;
	if (srcPBmp->bmType == NPDISP_DEVTYPE_DIBENG) {
		// DIBエンジン
		NPDISP_DIBENGINE* dibe = (NPDISP_DIBENGINE*)srcPBmp;
		BITMAPINFOHEADER biHeader;
		UINT16 sel = (dibe->deBitmapInfoAddr >> 16) & 0xffff;
		UINT32 ofs = dibe->deBitmapInfoAddr & 0xffff;
		npdisp_preloadAndReadMemoryWith32Offset(&biHeader, sel, ofs, sizeof(BITMAPINFOHEADER));
		if (biHeader.biBitCount <= 8) {
			// パレット
			if (biHeader.biClrUsed != 0) {
				lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * biHeader.biClrUsed;
			}
			else {
				lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << (biHeader.biBitCount));
			}
		}
		else if ((biHeader.biBitCount == 15 || biHeader.biBitCount == 16 || biHeader.biBitCount == 32) && biHeader.biCompression == BI_BITFIELDS) {
			// ビットフィールド
			lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3;
		}
		else {
			// パレットなし
			lpbiLen = sizeof(BITMAPINFOHEADER);
		}
		srcstride = dibe->deDeltaScan;
		bmBitsAddrSel = dibe->deBitsSelector;
		bmBitsAddrOfs = dibe->deBitsOffset;
		if (lpbiLen > sizeof(BITMAPINFOHEADER)) {
			npdisp_preloadMemoryWith32Offset(sel, ofs + sizeof(BITMAPINFOHEADER), lpbiLen - sizeof(BITMAPINFOHEADER));
		}
	}
	if ((bmBitsAddrSel == 0 && bmBitsAddrOfs == 0)) {
		// 空のBITMAP DDB用
		return;
	}

	//lastPreloadB = npdisp_memread_preloadcount;
	//lastPreload_memread_curpos = npdisp_memread_curpos;
	//lastPreload_memread_size = npdisp_memread_buf.size();
	int	dststride = ((srcPBmp->bmWidth * bpp + 31) / 32) * 4;
	if (numLines == -1 || numLines > srcPBmp->bmHeight) numLines = srcPBmp->bmHeight;
	if (beginLine + numLines > srcPBmp->bmHeight) numLines = srcPBmp->bmHeight - beginLine;
	if (beginLine >= srcPBmp->bmHeight) {
		beginLine = 0;
		numLines = 0;
	}
	if (copyWidth == -1 || copyWidth > srcPBmp->bmWidth) copyWidth = srcPBmp->bmWidth;
	if (beginX + copyWidth > srcPBmp->bmWidth) copyWidth = srcPBmp->bmWidth - beginX;
	if (beginX >= srcPBmp->bmWidth) {
		beginX = 0;
		copyWidth = 0;
	}
	if (numLines > 0) {
		int endLine = beginLine + numLines;
		int endX = beginX + copyWidth;
		int beginXbyte = beginX * bpp / 8;
		int endXbyte = (endX * bpp + 7) / 8;
		TRACEOUTF(("preload beginX=%d, endX=%d", beginXbyte, endXbyte));
		//lastPreload_imgsize = srcstride * numLines;
		// 先に読み取り
		if (srcPBmp->bmType != NPDISP_DEVTYPE_DIBENG && srcPBmp->bmSegmentIndex != 0 && srcPBmp->bmScanSegment != 0) {
			// 64KB超え転送
			UINT16 seg = bmBitsAddrSel;
			UINT32 ofs = bmBitsAddrOfs;
			int remain = srcPBmp->bmHeight;
			int segBeginLine = beginLine / srcPBmp->bmScanSegment * srcPBmp->bmScanSegment;
			int segEndLine = (endLine + srcPBmp->bmScanSegment - 1) / srcPBmp->bmScanSegment * srcPBmp->bmScanSegment;
			seg += srcPBmp->bmSegmentIndex * (segBeginLine / srcPBmp->bmScanSegment);
			remain -= segBeginLine;
			// 1ラインずつ転送
			for (j = segBeginLine; j < segEndLine; j += srcPBmp->bmScanSegment) {
				UINT32 srcOfs = ofs;
				int looplen = srcPBmp->bmScanSegment < remain ? srcPBmp->bmScanSegment : remain;
				for (i = 0; i < looplen; i++) {
					npdisp_preloadMemoryWith32Offset(seg, srcOfs + beginXbyte, (endXbyte - beginXbyte));
					srcOfs += srcstride;
				}
				seg += srcPBmp->bmSegmentIndex;
				remain -= looplen;
			}
		}
		else {
			// 64KB未満転送
			UINT16 seg = bmBitsAddrSel;
			UINT32 ofs = bmBitsAddrOfs;
			if (dststride == srcstride && beginX == 0 && copyWidth == srcPBmp->bmWidth) {
				// アライメントが一致しているので一括転送可能
				UINT32 srcOfs = ofs + srcstride * beginLine;
				npdisp_preloadMemoryWith32Offset(seg, srcOfs, srcstride * numLines);
			}
			else {
				// アライメント合わせのために1ラインずつ転送
				UINT32 srcOfs = ofs;
				srcOfs += srcstride * beginLine;
				for (i = beginLine; i < endLine; i++) {
					npdisp_preloadMemoryWith32Offset(seg, srcOfs + beginXbyte, (endXbyte - beginXbyte));
					srcOfs += srcstride;
				}
			}
		}
	}
}

int npdisp_MakeBitmapFromPBITMAP(NPDISP_PBITMAP_EXT* srcPBmp, NPDISP_WINDOWS_BMPHDC* bmpHDC, int dcIdx, int beginLine, int numLines, int beginX, int copyWidth, UINT16* transTable) {
	int i, j;
	int bpp = srcPBmp->bmPlanes * srcPBmp->bmBitsPixel;
	int	srcstride = srcPBmp->bmWidthBytes;
	BITMAPINFO* lpbi = NULL;

	if (npdisp.longjmpnum != 0) return 0;

	// DDBitmapキーが有効か確認
	if (srcPBmp->bmType == NPDISP_DEVTYPE_DDB && srcPBmp->ddbmpKey) {
		// DDBitmapを返す
		auto it = npdispwin.bitmaps.find(srcPBmp->ddbmpKey);
		if (it != npdispwin.bitmaps.end()) {
			NPDISP_HOSTBITMAP value = it->second;
			if (value.bmphdc.hBmp) {
				*bmpHDC = value.bmphdc;
				bmpHDC->hdc = npdispwin.hdcCache[dcIdx];
				bmpHDC->hOldBmp = SelectObject(bmpHDC->hdc, bmpHDC->hBmp);
				//TRACEOUT10(("RES: %04x %04x %08x", srcPBmp->reserved1, srcPBmp->reserved2, srcPBmp->bmBitsAddr));
				if (bmpHDC->hOldBmp == NULL) {
					// 他が選択済み
					for (int i = 0; i < NELEMENTS(npdispwin.hdcCache); i++) {
						if (GetCurrentObject(npdispwin.hdcCache[0], OBJ_BITMAP) == bmpHDC->hBmp) {
							bmpHDC->hdc = npdispwin.hdcCache[i];
							break;
						}
					}
				}
				else {
					// パレット変換適用
					if (npdisp.usePalette) {
						if (bmpHDC->lpbi->bmiHeader.biBitCount == 8) {
							RGBQUAD pal[256];
							if (!transTable) {
								// 変換テーブルがル場合は別途適用されるのでここでは不要
								for (int i = 0; i < 256; i++) {
									pal[i].rgbRed = i;
									pal[i].rgbGreen = i;
									pal[i].rgbBlue = i;
									pal[i].rgbReserved = 0;
								}
							}
							else {
								for (int i = 0; i < 256; i++) {
									pal[i].rgbRed = npdisp_palette_transTbl[i];
									pal[i].rgbGreen = npdisp_palette_transTbl[i];
									pal[i].rgbBlue = npdisp_palette_transTbl[i];
									pal[i].rgbReserved = 0;
								}
							}
							SetDIBColorTable(bmpHDC->hdc, 0, 256, pal);
						}
					}
				}
				SetBkColor(bmpHDC->hdc, 0xffffff);
				SetTextColor(bmpHDC->hdc, 0x000000);
				bmpHDC->isDevMemBmp = 1;

				TRACEOUT9(("DDB %d w=%d, h=%d", srcPBmp->ddbmpKey, bmpHDC->lpbi->bmiHeader.biWidth, bmpHDC->lpbi->bmiHeader.biHeight));
				return 1; // OK
			}
			TRACEOUT9(("DDB Error"));
			return 0; // NG
		}
		else {
			TRACEOUT9(("DIB Error"));
			return 0; // NG
		}
	}
	else {
		TRACEOUT9(("DIB"));
	}

	bmpHDC->isDevMemBmp = 0;

	UINT32 lpbiLen = 0;
	UINT16 bmBitsAddrSel = (srcPBmp->bmBitsAddr >> 16) & 0xffff;
	UINT32 bmBitsAddrOfs = srcPBmp->bmBitsAddr & 0xffff;
	if (srcPBmp->bmType == NPDISP_DEVTYPE_DIBENG) {
		// DIBエンジン
		BITMAPINFOHEADER biHeader;
		NPDISP_DIBENGINE* dibe = (NPDISP_DIBENGINE*)srcPBmp;
		UINT16 sel = (dibe->deBitmapInfoAddr >> 16) & 0xffff;
		UINT32 ofs = dibe->deBitmapInfoAddr & 0xffff;
		npdisp_readMemoryWith32Offset(&biHeader, sel, ofs, sizeof(BITMAPINFOHEADER));
		if (biHeader.biBitCount <= 8) {
			// パレット
			if (biHeader.biClrUsed != 0) {
				lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * biHeader.biClrUsed;
			}
			else {
				lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << (biHeader.biBitCount));
			}
		}
		else if ((biHeader.biBitCount == 15 || biHeader.biBitCount == 16 || biHeader.biBitCount == 32) && biHeader.biCompression == BI_BITFIELDS) {
			// ビットフィールド
			lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3;
		}
		else {
			// パレットなし
			lpbiLen = sizeof(BITMAPINFOHEADER);
		}
		bmBitsAddrSel = dibe->deBitsSelector;
		bmBitsAddrOfs = dibe->deBitsOffset;
		srcstride = dibe->deDeltaScan;
		lpbi = (BITMAPINFO*)malloc(lpbiLen);
		lpbi->bmiHeader = biHeader;
		if (npdisp.usePalette) {
			npdisp.usePalette = npdisp.usePalette;
		}
		if (lpbiLen > sizeof(BITMAPINFOHEADER)) {
			npdisp_readMemoryWith32Offset(&(lpbi->bmiColors), sel, ofs + sizeof(BITMAPINFOHEADER), lpbiLen - sizeof(BITMAPINFOHEADER));
		}
		TRACEOUTDIBE(("Read DIBE w=%d h=%d, bpp=%d", biHeader.biWidth, biHeader.biHeight, biHeader.biBitCount));
	}

	if (lpbiLen == 0) {
		if (bpp <= 8) {
			lpbi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << bpp));
			if (lpbi) {
				if (bpp == 1) {
					// 2色パレットセット
					for (i = 0; i < NELEMENTS(npdisp_palette_rgb2); i++) {
						lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb2[i].r;
						lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb2[i].g;
						lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb2[i].b;
						lpbi->bmiColors[i].rgbReserved = 0;
					}
				}
				else if (bpp == 4) {
					// 16色パレットセット
					for (i = 0; i < NELEMENTS(npdisp_palette_rgb16); i++) {
						lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb16[i].r;
						lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb16[i].g;
						lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb16[i].b;
						lpbi->bmiColors[i].rgbReserved = 0;
					}
				}
				else if (bpp == 8) {
					// 256色パレットセット
					if (npdisp.usePalette) {
						if (transTable) {
							// パレット番号変換の上転送
							for (i = 0; i < NELEMENTS(npdisp_palette_rgb256); i++) {
								lpbi->bmiColors[i].rgbRed = transTable[i] & 0xff;
								lpbi->bmiColors[i].rgbGreen = transTable[i] & 0xff;
								lpbi->bmiColors[i].rgbBlue = transTable[i] & 0xff;
								lpbi->bmiColors[i].rgbReserved = 0;
							}
						}
						else {
							// 仮想パレット番号
							for (i = 0; i < NELEMENTS(npdisp_palette_rgb256); i++) {
								lpbi->bmiColors[i].rgbRed = i;
								lpbi->bmiColors[i].rgbGreen = i;
								lpbi->bmiColors[i].rgbBlue = i;
								lpbi->bmiColors[i].rgbReserved = 0;
							}
						}
					}
					else {
						for (i = 0; i < NELEMENTS(npdisp_palette_rgb256); i++) {
							lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb256[i].r;
							lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb256[i].g;
							lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb256[i].b;
							lpbi->bmiColors[i].rgbReserved = 0;
						}
					}
				}
			}
		}
		else if (bpp == 15 || bpp == 16) {
			lpbi = (BITMAPINFO*)malloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 3);
			if (bpp == 16) {
				// ビットフィールド 565
				lpbi->bmiHeader.biCompression = BI_BITFIELDS;
				*((DWORD*)(lpbi->bmiColors + 0)) = 0x0000F800;
				*((DWORD*)(lpbi->bmiColors + 1)) = 0x000007E0;
				*((DWORD*)(lpbi->bmiColors + 2)) = 0x0000001F;
			}
			else if (bpp == 15) {
				// ビットフィールド 555
				lpbi->bmiHeader.biCompression = BI_BITFIELDS;
				*((DWORD*)(lpbi->bmiColors + 0)) = 0x00007C00;
				*((DWORD*)(lpbi->bmiColors + 1)) = 0x000003E0;
				*((DWORD*)(lpbi->bmiColors + 2)) = 0x0000001F;
				lpbi->bmiHeader.biBitCount = 16;
			}
			bpp = 16; // 後続処理では16扱いにする
		}
		else {
			lpbi = (BITMAPINFO*)malloc(sizeof(BITMAPINFO));
		}
	}
	if (lpbi) {
		//HDC hdcScreen = GetDC(NULL);
		bmpHDC->hdc = npdispwin.hdcCache[dcIdx];
		if (bmpHDC->hdc) {
			lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			lpbi->bmiHeader.biWidth = srcPBmp->bmWidth;
			lpbi->bmiHeader.biHeight = -srcPBmp->bmHeight;
			lpbi->bmiHeader.biPlanes = srcPBmp->bmPlanes;
			lpbi->bmiHeader.biBitCount = srcPBmp->bmBitsPixel;
			if (bpp != 15 && bpp != 16) {
				lpbi->bmiHeader.biCompression = BI_RGB;
			}
			lpbi->bmiHeader.biSizeImage = 0;
			lpbi->bmiHeader.biXPelsPerMeter = 0;
			lpbi->bmiHeader.biYPelsPerMeter = 0;
			if (lpbiLen == 0) {
				lpbi->bmiHeader.biClrUsed = 1 << srcPBmp->bmBitsPixel;
				lpbi->bmiHeader.biClrImportant = lpbi->bmiHeader.biClrUsed;
			}
			bmpHDC->hBmp = CreateDIBSection(bmpHDC->hdc, lpbi, DIB_RGB_COLORS, &bmpHDC->pBits, NULL, 0);
			if (bmpHDC->hBmp) {
				HBITMAP hbmpSrcOld;
				bmpHDC->stride = ((srcPBmp->bmWidth * bpp + 31) / 32) * 4;
				if (numLines == -1 || numLines > srcPBmp->bmHeight) numLines = srcPBmp->bmHeight;
				if (beginLine + numLines > srcPBmp->bmHeight) numLines = srcPBmp->bmHeight - beginLine;
				if (beginLine >= srcPBmp->bmHeight) {
					beginLine = 0;
					numLines = 0;
				}
				if (copyWidth == -1 || copyWidth > srcPBmp->bmWidth) copyWidth = srcPBmp->bmWidth;
				if (beginX + copyWidth > srcPBmp->bmWidth) copyWidth = srcPBmp->bmWidth - beginX;
				if (beginX >= srcPBmp->bmWidth) {
					beginX = 0;
					copyWidth = 0;
				}

				if ((bmBitsAddrSel != 0 || bmBitsAddrOfs != 0) && numLines > 0) {
					int endLine = beginLine + numLines;
					int endX = beginX + copyWidth;
					int beginXbyte = beginX * bpp / 8;
					int endXbyte = (endX * bpp + 7) / 8;
					TRACEOUTF(("read beginX=%d, endX=%d", beginXbyte, endXbyte));
					if (srcPBmp->bmType != NPDISP_DEVTYPE_DIBENG && srcPBmp->bmSegmentIndex != 0 && srcPBmp->bmScanSegment != 0) {
						// 64KB超え転送
						UINT16 seg = bmBitsAddrSel;
						UINT32 ofs = bmBitsAddrOfs;
						int remain = srcPBmp->bmHeight;
						int segBeginLine = beginLine / srcPBmp->bmScanSegment * srcPBmp->bmScanSegment;
						int segEndLine = (endLine + srcPBmp->bmScanSegment - 1) / srcPBmp->bmScanSegment * srcPBmp->bmScanSegment;
						seg += srcPBmp->bmSegmentIndex * (segBeginLine / srcPBmp->bmScanSegment);
						remain -= segBeginLine;
						char* dstPtr = (char*)(bmpHDC->pBits) + bmpHDC->stride * segBeginLine;
						// 1ラインずつ転送
						for (j = segBeginLine; j < segEndLine; j += srcPBmp->bmScanSegment) {
							UINT32 srcOfs = ofs;
							int looplen = srcPBmp->bmScanSegment < remain ? srcPBmp->bmScanSegment : remain;
							for (i = 0; i < looplen; i++) {
								npdisp_readMemoryWith32Offset(dstPtr + beginXbyte, seg, srcOfs + beginXbyte, (endXbyte - beginXbyte));
								srcOfs += srcstride;
								dstPtr += bmpHDC->stride;
							}
							seg += srcPBmp->bmSegmentIndex;
							remain -= looplen;
						}
					}
					else {
						// 64KB未満転送
						UINT16 seg = bmBitsAddrSel;
						UINT32 ofs = bmBitsAddrOfs;
						if (bmpHDC->stride == srcstride && beginX == 0 && copyWidth == srcPBmp->bmWidth) {
							// アライメントが一致しているので一括転送可能
							char* dstPtr = (char*)(bmpHDC->pBits);
							UINT16 srcOfs = ofs + srcstride * beginLine;
							dstPtr += bmpHDC->stride * beginLine;
							npdisp_readMemoryWith32Offset(dstPtr, seg, srcOfs, srcstride * numLines);
						}
						else {
							// アライメント合わせのために1ラインずつ転送
							char* dstPtr = (char*)(bmpHDC->pBits);
							UINT32 srcOfs = ofs;
							srcOfs += srcstride * beginLine;
							dstPtr += bmpHDC->stride * beginLine;
							for (i = beginLine; i < endLine; i++) {
								npdisp_readMemoryWith32Offset(dstPtr + beginXbyte, seg, srcOfs + beginXbyte, (endXbyte - beginXbyte));
								srcOfs += srcstride;
								dstPtr += bmpHDC->stride;
							}
						}
					}
				}
				if (npdisp.longjmpnum == 0) {
					bmpHDC->hOldBmp = SelectObject(bmpHDC->hdc, bmpHDC->hBmp);
					bmpHDC->lpbi = lpbi;
					bmpHDC->hBmpDDB = NULL;
					SetBkColor(bmpHDC->hdc, 0xffffff);
					SetTextColor(bmpHDC->hdc, 0x000000);
					//BitBlt(np2wabwnd.hDCBuf, npdisp.width / 2, 0, npdisp.width / 2, npdisp.height, bmpHDC->hdc, 0, 0, SRCCOPY);
				}
				else {
					DeleteObject(bmpHDC->hBmp);
					bmpHDC->hdc = NULL;
					free(lpbi);
					bmpHDC->lpbi = NULL;
				}
			}
			else {
				bmpHDC->hdc = NULL;
				free(lpbi);
				bmpHDC->lpbi = NULL;
			}
		}
		else {
			bmpHDC->hdc = NULL;
			free(lpbi);
			bmpHDC->lpbi = NULL;
		}
		//ReleaseDC(NULL, hdcScreen); // もういらない
	}

	return bmpHDC->hdc != NULL;
}
void npdisp_WriteBitmapToPBITMAP(NPDISP_PBITMAP_EXT* dstPBmp, NPDISP_WINDOWS_BMPHDC* bmpHDC, int beginLine, int numLines, int beginX, int copyWidth) {
	if (!bmpHDC) return;

	if (npdisp.longjmpnum != 0) return;

	if (bmpHDC->pBits && bmpHDC->lpbi) {
		if (bmpHDC->hBmpDDB) {
			// DDB書き戻し
			const int ddbWidth = bmpHDC->lpbi->bmiHeader.biWidth;
			const int ddbHeight = (bmpHDC->lpbi->bmiHeader.biHeight >= 0) ? bmpHDC->lpbi->bmiHeader.biHeight : -bmpHDC->lpbi->bmiHeader.biHeight;
			HDC hdcTemp = npdispwin.hdcCache[2];
			HGDIOBJ hOldBmp = SelectObject(hdcTemp, bmpHDC->hBmp);
			SetTextColor(hdcTemp, 0);
			SetBkColor(hdcTemp, 0xffffff);
			BitBlt(hdcTemp, 0, 0, ddbWidth, ddbHeight, bmpHDC->hdc, 0, 0, SRCCOPY);
			SelectObject(hdcTemp, hOldBmp);
		}

		TRACEOUT9(("Write DIB %d w=%d, h=%d", dstPBmp->ddbmpKey, bmpHDC->lpbi->bmiHeader.biWidth, bmpHDC->lpbi->bmiHeader.biHeight));
		// DDBitmapキーが有効か確認
		if (bmpHDC->isDevMemBmp) {
			// DDBitmapなのでここで書き戻し不要
			return;
		}

		int	dststride = dstPBmp->bmWidthBytes;

		UINT16 bmBitsAddrSel = (dstPBmp->bmBitsAddr >> 16) & 0xffff;
		UINT32 bmBitsAddrOfs = dstPBmp->bmBitsAddr & 0xffff;
		if (dstPBmp->bmType == NPDISP_DEVTYPE_DIBENG) {
			// DIBエンジン
			NPDISP_DIBENGINE* dibe = (NPDISP_DIBENGINE*)dstPBmp;
			bmBitsAddrSel = dibe->deBitsSelector;
			bmBitsAddrOfs = dibe->deBitsOffset;
			TRACEOUTDIBE(("Write DIBE w=%d h=%d, bpp=%d", bmpHDC->lpbi->bmiHeader.biWidth, bmpHDC->lpbi->bmiHeader.biHeight, bmpHDC->lpbi->bmiHeader.biBitCount));
			dststride = dibe->deDeltaScan;
		}

		int i, j;
		int bpp = dstPBmp->bmPlanes * dstPBmp->bmBitsPixel;
		if (bpp == 15) {
			bpp = 16; // 後続処理では16扱いにする
		}
		if (numLines == -1 || numLines > dstPBmp->bmHeight) numLines = dstPBmp->bmHeight;
		if (beginLine + numLines > dstPBmp->bmHeight) numLines = dstPBmp->bmHeight - beginLine;
		if (beginLine >= dstPBmp->bmHeight) {
			beginLine = 0;
			numLines = 0;
		}
		if (copyWidth == -1 || copyWidth > dstPBmp->bmWidth) copyWidth = dstPBmp->bmWidth;
		if (beginX + copyWidth > dstPBmp->bmWidth) copyWidth = dstPBmp->bmWidth - beginX;
		if (beginX >= dstPBmp->bmWidth) {
			beginX = 0;
			copyWidth = 0;
		}
		if (copyWidth < 1) {
			return; // 書くものなし
		}
		if (numLines < 1) {
			return; // 書くものなし
		}
		int endLine = beginLine + numLines;
		int endX = beginX + copyWidth;
		int beginXbyte = beginX * bpp / 8;
		int endXbyte = (endX * bpp + 7) / 8;
		TRACEOUTF(("write beginX=%d, endX=%d", beginXbyte, endXbyte));

		if (dstPBmp->bmType != NPDISP_DEVTYPE_DIBENG && dstPBmp->bmSegmentIndex != 0 && dstPBmp->bmScanSegment != 0) {
			// 64KB超え転送
			UINT16 seg = bmBitsAddrSel;
			UINT32 ofs = bmBitsAddrOfs;
			int remain = dstPBmp->bmHeight;
			int segBeginLine = beginLine / dstPBmp->bmScanSegment * dstPBmp->bmScanSegment;
			int segEndLine = (endLine + dstPBmp->bmScanSegment - 1) / dstPBmp->bmScanSegment * dstPBmp->bmScanSegment;
			seg += dstPBmp->bmSegmentIndex * (segBeginLine / dstPBmp->bmScanSegment);
			remain -= segBeginLine;
			char* srcPtr = (char*)(bmpHDC->pBits) + bmpHDC->stride * segBeginLine;
			// 1ラインずつ転送
			for (j = segBeginLine; j < segEndLine; j += dstPBmp->bmScanSegment) {
				UINT32 dstOfs = ofs;
				int looplen = dstPBmp->bmScanSegment < remain ? dstPBmp->bmScanSegment : remain;
				for (i = 0; i < looplen; i++) {
					npdisp_writeMemoryWith32Offset(srcPtr + beginXbyte, seg, dstOfs + beginXbyte, (endXbyte - beginXbyte));
					dstOfs += dststride;
					srcPtr += bmpHDC->stride;
				}
				seg += dstPBmp->bmSegmentIndex;
				remain -= looplen;
			}
		}
		else {
			// 64KB未満転送
			UINT16 seg = bmBitsAddrSel;
			UINT32 ofs = bmBitsAddrOfs;
			if (bmpHDC->stride == dststride && beginX == 0 && copyWidth == dstPBmp->bmWidth) {
				char* srcPtr = (char*)(bmpHDC->pBits);
				UINT32 dstOfs = ofs + dststride * beginLine;
				srcPtr += bmpHDC->stride * beginLine;
				npdisp_writeMemoryWith32Offset(srcPtr, seg, dstOfs, dststride * numLines);
			}
			else {
				// アライメント合わせのために1ラインずつ転送
				char* srcPtr = (char*)(bmpHDC->pBits);
				UINT32 dstOfs = ofs;
				dstOfs += dststride * beginLine;
				srcPtr += bmpHDC->stride * beginLine;
				for (i = beginLine; i < endLine; i++) {
					npdisp_writeMemoryWith32Offset(srcPtr + beginXbyte, seg, dstOfs + beginXbyte, (endXbyte - beginXbyte));
					dstOfs += dststride;
					srcPtr += bmpHDC->stride;
				}
			}
		}
	}
}
// DIBモノクロビットマップからDDBモノクロビットマップを生成する　以降の操作はDDBに対して行われ、pBitsによるビットの操作は無効になる（無視される）
void npdisp_ConvertToDDBMonoBitmap(NPDISP_WINDOWS_BMPHDC* bmpHDC)
{
	if (npdisp.bpp == 1) return; // モノクロは特殊処理なので抜ける
	if (bmpHDC->lpbi->bmiHeader.biBitCount != 1) return;
	if (bmpHDC->hBmpDDB) return;

	HDC hdcTemp = npdispwin.hdcCache[2];
	const int ddbWidth = bmpHDC->lpbi->bmiHeader.biWidth;
	const int ddbHeight = (bmpHDC->lpbi->bmiHeader.biHeight >= 0) ? bmpHDC->lpbi->bmiHeader.biHeight : -bmpHDC->lpbi->bmiHeader.biHeight;
	bmpHDC->hBmpDDB = CreateBitmap(ddbWidth, ddbHeight, 1, 1, NULL);
	HGDIOBJ hOldBmp = SelectObject(hdcTemp, bmpHDC->hBmpDDB);
	SetTextColor(hdcTemp, 0);
	SetBkColor(hdcTemp, 0xffffff);
	BitBlt(hdcTemp, 0, 0, ddbWidth, ddbHeight, bmpHDC->hdc, 0, 0, SRCCOPY);
	SelectObject(hdcTemp, hOldBmp);
	SelectObject(bmpHDC->hdc, bmpHDC->hBmpDDB);
}
void npdisp_FreeBitmap(NPDISP_WINDOWS_BMPHDC* bmpHDC, bool force) {
	if (!bmpHDC) return;

	if (!force && bmpHDC->isDevMemBmp) {
		// HDCを戻すだけ 本体は削除しない
		if (bmpHDC->hBmpDDB) {
			// DDBになっていたら書き戻し&削除
			const int ddbWidth = bmpHDC->lpbi->bmiHeader.biWidth;
			const int ddbHeight = (bmpHDC->lpbi->bmiHeader.biHeight >= 0) ? bmpHDC->lpbi->bmiHeader.biHeight : -bmpHDC->lpbi->bmiHeader.biHeight;
			HDC hdcTemp = npdispwin.hdcCache[2];
			HGDIOBJ hOldBmp = SelectObject(hdcTemp, bmpHDC->hBmp);
			SetTextColor(hdcTemp, 0);
			SetBkColor(hdcTemp, 0xffffff);
			BitBlt(hdcTemp, 0, 0, ddbWidth, ddbHeight, bmpHDC->hdc, 0, 0, SRCCOPY);
			SelectObject(hdcTemp, hOldBmp);
			DeleteObject(bmpHDC->hBmpDDB);
			bmpHDC->hBmpDDB = NULL;
		}
		if (bmpHDC->hdc && bmpHDC->hOldBmp) {
			SelectObject(bmpHDC->hdc, bmpHDC->hOldBmp);
		}
		return;
	}

	if (bmpHDC->hBmpDDB) {
		// DDB削除
		if (bmpHDC->hdc && bmpHDC->hOldBmp) {
			SelectObject(bmpHDC->hdc, bmpHDC->hOldBmp);
		}
		DeleteObject(bmpHDC->hBmpDDB);
		bmpHDC->hBmpDDB = NULL;
	}

	if (bmpHDC->lpbi) {
		free(bmpHDC->lpbi);
		bmpHDC->lpbi = NULL;
	}
	if (bmpHDC->hBmp) {
		if (bmpHDC->hdc && bmpHDC->hOldBmp) {
			SelectObject(bmpHDC->hdc, bmpHDC->hOldBmp);
		}
		DeleteObject(bmpHDC->hBmp);
		bmpHDC->hBmp = NULL;
		bmpHDC->pBits = NULL;
	}
	if (bmpHDC->hdc) {
		bmpHDC->hdc = NULL;
	}
}

#endif