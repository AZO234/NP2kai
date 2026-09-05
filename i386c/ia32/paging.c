/*
 * Copyright (c) 2003-2004 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <compiler.h>
#include "cpu.h"
#include "ia32.mcr"

// USE_LEGACY_MEMORY_ACCESSがある場合は旧ルーチンを使用する
#if defined(USE_LEGACY_MEMORY_ACCESS)
#define	IA32_MEMORY_FAST_PATH	0
#else
#define	IA32_MEMORY_FAST_PATH	1
#endif

#if defined(SUPPORT_WAB_NPDISP)
#define NPDISP_WINDOW_MAGIC	0x504e
extern UINT32	g_npdisp_windowAddr;
#ifdef __cplusplus
extern "C" {
#endif
void npdisp_exec(void);
#ifdef __cplusplus
}
#endif
#endif

/*
 * ページフォルト例外
 *
 * 4-31: 予約済み
 *    3: RSVD: 0 = フォルトの原因は予約ビット違反ではなかった．
 *             1 = ページ・フォルトの原因は，違反とマークされた PTE または
 *                 PDE の予約ビット位置のうち一つで，1 が検出されたことである．
 *    2: U/S:  0 = フォルトの原因となったアクセスはプロセッサがスーパバイザ・
 *                 モードで実行中に行われた．
 *             1 = フォルトの原因となったアクセスはプロセッサがユーザ・モードで
 *                 実行中に行われた．
 *    1: W/R:  0 = フォルトの原因となったアクセスが読み取りであった．
 *             1 = フォルトの原因となったアクセスが書き込みであった．
 *    0: P:    0 = フォルトの原因が不在ページであった．
 *             1 = フォルトの原因がページ・レベル保護違反であった．
 */

/*
 * 下巻 4.12. ページ保護とセグメント保護の組み合わせ
 * 「表 4-2. ページ・ディレクトリとページ・テーブルの保護の組み合わせ」
 *
 * +------------+------------+------------+
 * |    PDE     |    PTE     |   merge    |
 * +-----+------+-----+------+-----+------+
 * | pri | type | pri | type | pri | type |
 * +-----+------+-----+------+-----+------+
 * |  u  |  ro  |  u  |  ro  |  u  |  ro  |
 * |  u  |  ro  |  u  |  rw  |  u  |  ro  |
 * |  u  |  rw  |  u  |  ro  |  u  |  ro  |
 * |  u  |  rw  |  u  |  rw  |  u  |  rw  |
 * |  u  |  ro  |  s  |  ro  |  s  | rw/p |
 * |  u  |  ro  |  s  |  rw  |  s  | rw/p |
 * |  u  |  rw  |  s  |  ro  |  s  | rw/p |
 * |  u  |  rw  |  s  |  rw  |  s  |  rw  |
 * |  s  |  ro  |  u  |  ro  |  s  | rw/p |
 * |  s  |  ro  |  u  |  rw  |  s  | rw/p |
 * |  s  |  rw  |  u  |  ro  |  s  | rw/p |
 * |  s  |  rw  |  u  |  rw  |  s  |  rw  |
 * |  s  |  ro  |  s  |  ro  |  s  | rw/p |
 * |  s  |  ro  |  s  |  rw  |  s  | rw/p |
 * |  s  |  rw  |  s  |  ro  |  s  | rw/p |
 * |  s  |  rw  |  s  |  rw  |  s  |  rw  |
 * +-----+------+-----+------+-----+------+
 *
 * ※ rw/p : CR0 の WP ビットが ON の場合には ro
 */

/*
 * メモリアクセス/PxE(上記参照)/CPL/CR0 とページアクセス権の関係
 *
 * +-----+-----+-----+-----+-----+---+
 * | CR0 | CPL | PxE | PxE | ope |   |
 * | W/P | u/s | u/s | r/w | r/w |   |
 * +-----+-----+-----+-----+-----+---+
 * | n/a |  s  |  s  | n/a |  r  | o |
 * | n/a |  s  |  u  | n/a |  r  | o |
 * | n/a |  u  |  s  | n/a |  r  | x |
 * | n/a |  u  |  u  | n/a |  r  | o |
 * +-----+-----+-----+-----+-----+---+
 * |  n  |  s  |  s  |  r  |  w  | o |
 * |  n  |  s  |  u  |  r  |  w  | o |
 * |  n  |  u  |  s  |  r  |  w  | x |
 * |  n  |  u  |  u  |  r  |  w  | x |
 * +-----+-----+-----+-----+-----+---+
 * |  p  |  s  |  s  |  r  |  w  | x |
 * |  p  |  s  |  u  |  r  |  w  | x |
 * |  p  |  u  |  s  |  r  |  w  | x |
 * |  p  |  u  |  u  |  r  |  w  | x |
 * +-----+-----+-----+-----+-----+---+
 * |  n  |  s  |  s  |  w  |  w  | o |
 * |  n  |  s  |  u  |  w  |  w  | o |
 * |  n  |  u  |  s  |  w  |  w  | x |
 * |  n  |  u  |  u  |  w  |  w  | o |
 * +-----+-----+-----+-----+-----+---+
 * |  p  |  s  |  s  |  w  |  w  | o |
 * |  p  |  s  |  u  |  w  |  w  | x |
 * |  p  |  u  |  s  |  w  |  w  | x |
 * |  p  |  u  |  u  |  w  |  w  | o |
 * +-----+-----------+-----+-----+---+
 */
#if !defined(USE_PAGE_ACCESS_TABLE)
#define	page_access	0xd0ddd0ff
#else	/* USE_PAGE_ACCESS_TABLE */
static const UINT8 page_access_bit[32] = {
	1,	/* CR0: n, CPL: s, PTE: s, PTE: r, ope: r */
	1,	/* CR0: n, CPL: s, PTE: s, PTE: r, ope: w */
	1,	/* CR0: n, CPL: s, PTE: s, PTE: w, ope: r */
	1,	/* CR0: n, CPL: s, PTE: s, PTE: w, ope: w */

	1,	/* CR0: n, CPL: s, PTE: u, PTE: r, ope: r */
	1,	/* CR0: n, CPL: s, PTE: u, PTE: r, ope: w */
	1,	/* CR0: n, CPL: s, PTE: u, PTE: w, ope: r */
	1,	/* CR0: n, CPL: s, PTE: u, PTE: w, ope: w */

	0,	/* CR0: n, CPL: u, PTE: s, PTE: r, ope: r */
	0,	/* CR0: n, CPL: u, PTE: s, PTE: r, ope: w */
	0,	/* CR0: n, CPL: u, PTE: s, PTE: w, ope: r */
	0,	/* CR0: n, CPL: u, PTE: s, PTE: w, ope: w */

	1,	/* CR0: n, CPL: u, PTE: u, PTE: r, ope: r */
	0,	/* CR0: n, CPL: u, PTE: u, PTE: r, ope: w */
	1,	/* CR0: n, CPL: u, PTE: u, PTE: w, ope: r */
	1,	/* CR0: n, CPL: u, PTE: u, PTE: w, ope: w */

	1,	/* CR0: p, CPL: s, PTE: s, PTE: r, ope: r */
	0,	/* CR0: p, CPL: s, PTE: s, PTE: r, ope: w */
	1,	/* CR0: p, CPL: s, PTE: s, PTE: w, ope: r */
	1,	/* CR0: p, CPL: s, PTE: s, PTE: w, ope: w */

	1,	/* CR0: p, CPL: s, PTE: u, PTE: r, ope: r */
	0,	/* CR0: p, CPL: s, PTE: u, PTE: r, ope: w */
	1,	/* CR0: p, CPL: s, PTE: u, PTE: w, ope: r */
	1,	/* CR0: p, CPL: s, PTE: u, PTE: w, ope: w */

	0,	/* CR0: p, CPL: u, PTE: s, PTE: r, ope: r */
	0,	/* CR0: p, CPL: u, PTE: s, PTE: r, ope: w */
	0,	/* CR0: p, CPL: u, PTE: s, PTE: w, ope: r */
	0,	/* CR0: p, CPL: u, PTE: s, PTE: w, ope: w */

	1,	/* CR0: p, CPL: u, PTE: u, PTE: r, ope: r */
	0,	/* CR0: p, CPL: u, PTE: u, PTE: r, ope: w */
	1,	/* CR0: p, CPL: u, PTE: u, PTE: w, ope: r */
	1,	/* CR0: p, CPL: u, PTE: u, PTE: w, ope: w */
};
#endif	/* !USE_PAGE_ACCESS_TABLE */

/*
 *--
 * 32bit 物理アドレス 4k ページ
 *
 * リニア・アドレス
 *  31                    22 21                  12 11                       0
 * +------------------------+----------------------+--------------------------+
 * |  ページ・ディレクトリ  |   ページ・テーブル   |        オフセット        |
 * +------------------------+----------------------+--------------------------+
 *             |                        |                       |
 * +-----------+            +-----------+                       +----------+
 * |                        |                                              |
 * |  ページ・ディレクトリ  |   ページ・テーブル            ページ         |
 * | +--------------------+ | +-------------------+   +------------------+ |
 * | |                    | | |                   |   |                  | |
 * | |                    | | +-------------------+   |                  | |
 * | |                    | +>| page table entry  |-+ |                  | |
 * | +--------------------+   +-------------------+ | |                  | |
 * +>|page directory entry|-+ |                   | | +------------------+ |
 *   +--------------------+ | |                   | | | physical address |<+
 *   |                    | | |                   | | +------------------+
 *   |                    | | |                   | | |                  |
 * +>+--------------------+ +>+-------------------+ +>+------------------+
 * |
 * +- CR3(物理アドレス)
 */
/* TLB */
struct tlb_entry {
	UINT32	tag;	/* linear address */
#define	TLB_ENTRY_TAG_VALID		(1 << 0)
/*	pde & pte & CPU_PTE_WRITABLE	(1 << 1)	*/
/*	pde & pte & CPU_PTE_USER_MODE	(1 << 2)	*/
#define	TLB_ENTRY_TAG_DIRTY		CPU_PTE_DIRTY		/* (1 << 6) */
#define	TLB_ENTRY_TAG_GLOBAL		CPU_PTE_GLOBAL_PAGE	/* (1 << 8) */
#define	TLB_ENTRY_TAG_MAX_SHIFT		12
	UINT32	paddr;	/* physical address */
#if IA32_MEMORY_FAST_PATH
	UINT8	*host_page;	/* direct host pointer for ordinary RAM page */
	UINT32	fast_flags;	/* predecoded access/direct flags */
#endif
};
/*
 * TLB fast path
 */
#define	TLB_TAG_SHIFT		TLB_ENTRY_TAG_MAX_SHIFT
#define	TLB_TAG_MASK		(~((1 << TLB_TAG_SHIFT) - 1))
#define	TLB_GET_TAG_ADDR(ep)	((ep)->tag & TLB_TAG_MASK)
#define	TLB_SET_TAG_ADDR(ep, addr) \
do { \
	(ep)->tag &= ~TLB_TAG_MASK; \
	(ep)->tag |= (addr) & TLB_TAG_MASK; \
} while (/*CONSTCOND(*/ 0)

#define	TLB_IS_VALID(ep)	((ep)->tag & TLB_ENTRY_TAG_VALID)
#define	TLB_SET_VALID(ep)	((ep)->tag = TLB_ENTRY_TAG_VALID)
#define	TLB_SET_INVALID(ep)	((ep)->tag = 0)

#define	TLB_IS_WRITABLE(ep)	((ep)->tag & CPU_PTE_WRITABLE)
#define	TLB_IS_USERMODE(ep)	((ep)->tag & CPU_PTE_USER_MODE)
#define	TLB_IS_DIRTY(ep)	((ep)->tag & TLB_ENTRY_TAG_DIRTY)
#if (CPU_FEATURES_ALL & CPU_FEATURE_PGE) == CPU_FEATURE_PGE
#define	TLB_IS_GLOBAL(ep)	((ep)->tag & TLB_ENTRY_TAG_GLOBAL)
#else
#define	TLB_IS_GLOBAL(ep)	0
#endif

#define	TLB_SET_TAG_FLAGS(ep, entry, bit) \
do { \
	(ep)->tag |= (entry) & (CPU_PTE_GLOBAL_PAGE|CPU_PTE_DIRTY); \
	(ep)->tag |= (bit) & (CPU_PTE_WRITABLE|CPU_PTE_USER_MODE); \
} while (/*CONSTCOND*/ 0)

#if IA32_MEMORY_FAST_PATH
// ページ高速アクセスの簡易判定のためのフラグ
// このフラグ判定で弾かれた場合は通常のpaging関数で詳細判定する
#define	TLBF_SUPER_READ		0x00000001UL // スーパーバイザモード読み取り許可
#define	TLBF_USER_READ		0x00000002UL // ユーザーモード読み取り許可
#define	TLBF_SUPER_WRITE	0x00000004UL // CR0.WP==0の場合のスーパーバイザモード書き込み許可
#define	TLBF_SUPER_WRITE_WP	0x00000008UL // CR0.WP==1の場合のスーパーバイザモード書き込み許可
#define	TLBF_USER_WRITE		0x00000010UL // ユーザーモード書き込み許可
#define	TLBF_CODE_SUPER		0x00000020UL // スーパーバイザモード命令フェッチ許可
#define	TLBF_CODE_USER		0x00000040UL // ユーザーモード命令フェッチ許可
#define	TLBF_DIRECT_READ	0x00000100UL // host_pageポインタへ直接読み取り可能
#define	TLBF_DIRECT_WRITE	0x00000200UL // host_pageポインタへ直接書き込み可能
#endif

#define	NTLB		2	/* 0: DTLB, 1: ITLB */
#define	NENTRY		(1 << 6)
#define	TLB_ENTRY_SHIFT	12
#define	TLB_ENTRY_MASK	(NENTRY - 1)

typedef struct {
	struct tlb_entry entry[NENTRY];
} tlb_t;
static tlb_t tlb[NTLB];

#if IA32_MEMORY_FAST_PATH
#if defined(__GNUC__)
#define TLB_FAST_INLINE static __inline__ __attribute__((always_inline))
#elif defined(_MSC_VER)
#define TLB_FAST_INLINE static __inline
#else
#define TLB_FAST_INLINE static INLINE
#endif

#define	TLB_USER_INDEX(ucrw)	(((ucrw) & CPU_PAGE_USER_MODE) >> 3)

static UINT32 tlb_data_read_fast_flags[2] = {
	TLBF_SUPER_READ,
	TLBF_USER_READ
};
static UINT32 tlb_data_write_fast_flags[2] = {
	TLBF_SUPER_WRITE,
	TLBF_USER_WRITE
};
static UINT32 tlb_code_read_fast_flags[2] = {
	TLBF_CODE_SUPER,
	TLBF_CODE_USER
};

TLB_FAST_INLINE struct tlb_entry *
tlb_lookup_data_read_fast(UINT32 laddr, int ucrw)
{
	struct tlb_entry *ep;
	UINT32 flag;
	int idx;

	idx = (laddr >> TLB_ENTRY_SHIFT) & TLB_ENTRY_MASK;
	ep = &tlb[0].entry[idx];
	flag = tlb_data_read_fast_flags[TLB_USER_INDEX(ucrw)];

	if (TLB_IS_VALID(ep) &&
	    ((laddr & TLB_TAG_MASK) == TLB_GET_TAG_ADDR(ep)) &&
	    (ep->fast_flags & flag)) {
		return ep;
	}
	return NULL;
}

TLB_FAST_INLINE struct tlb_entry *
tlb_lookup_data_write_fast(UINT32 laddr, int ucrw)
{
	struct tlb_entry *ep;
	UINT32 flag;
	int idx;

	idx = (laddr >> TLB_ENTRY_SHIFT) & TLB_ENTRY_MASK;
	ep = &tlb[0].entry[idx];
	flag = tlb_data_write_fast_flags[TLB_USER_INDEX(ucrw)];

	if (TLB_IS_VALID(ep) &&
	    ((laddr & TLB_TAG_MASK) == TLB_GET_TAG_ADDR(ep)) &&
	    (ep->fast_flags & flag)) {
		return ep;
	}
	return NULL;
}

TLB_FAST_INLINE struct tlb_entry *
tlb_lookup_code_fast(UINT32 laddr, int ucrw)
{
	struct tlb_entry *ep;
	UINT32 flag;
	int idx;

	idx = (laddr >> TLB_ENTRY_SHIFT) & TLB_ENTRY_MASK;
	ep = &tlb[1].entry[idx];
	flag = tlb_code_read_fast_flags[TLB_USER_INDEX(ucrw)];

	if (TLB_IS_VALID(ep) &&
	    ((laddr & TLB_TAG_MASK) == TLB_GET_TAG_ADDR(ep)) &&
	    (ep->fast_flags & flag)) {
		return ep;
	}
	return NULL;
}

TLB_FAST_INLINE struct tlb_entry *
tlb_lookup_fast(UINT32 laddr, int ucrw)
{
	/* Preserve the original precedence for the unlikely WRITE|CODE case. */
	if (ucrw & CPU_PAGE_WRITE) {
		return tlb_lookup_data_write_fast(laddr, ucrw);
	}
	if (ucrw & CPU_PAGE_CODE) {
		return tlb_lookup_code_fast(laddr, ucrw);
	}
	return tlb_lookup_data_read_fast(laddr, ucrw);
}


TLB_FAST_INLINE UINT32
tlb_make_fast_flags(UINT entry, int bit, int n, int direct)
{
	UINT32 flags;
	int writable;
	int user;

	flags = TLBF_SUPER_READ;
	writable = (bit & CPU_PTE_WRITABLE) != 0;
	user = (bit & CPU_PTE_USER_MODE) != 0;

	if (user) {
		flags |= TLBF_USER_READ;
	}
	if (n == 1) {
		flags |= TLBF_CODE_SUPER;
		if (user) {
			flags |= TLBF_CODE_USER;
		}
	}

	if (entry & CPU_PTE_DIRTY) {
		flags |= TLBF_SUPER_WRITE;
		if (writable) {
			flags |= TLBF_SUPER_WRITE_WP;
			if (user) {
				flags |= TLBF_USER_WRITE;
			}
		}
	}

	if (direct) {
		flags |= TLBF_DIRECT_READ;
		if (flags & (TLBF_SUPER_WRITE|TLBF_SUPER_WRITE_WP|TLBF_USER_WRITE)) {
			flags |= TLBF_DIRECT_WRITE;
		}
	}
	return flags;
}
#endif	/* IA32_MEMORY_FAST_PATH */

void MEMCALL
tlb_update_access_flags(void)
{
#if IA32_MEMORY_FAST_PATH
	tlb_data_read_fast_flags[0] = TLBF_SUPER_READ;
	tlb_data_read_fast_flags[1] = TLBF_USER_READ;
	tlb_data_write_fast_flags[0] = CPU_STAT_WP ? TLBF_SUPER_WRITE_WP : TLBF_SUPER_WRITE;
	tlb_data_write_fast_flags[1] = TLBF_USER_WRITE;
	tlb_code_read_fast_flags[0] = TLBF_CODE_SUPER;
	tlb_code_read_fast_flags[1] = TLBF_CODE_USER;
#endif
}

static struct tlb_entry * MEMCALL tlb_update(UINT32 laddr, UINT entry, int ucrw);

/* paging */
static UINT32 MEMCALL paging(UINT32 laddr, int ucrw);
static UINT32 MEMCALL paging_walk(UINT32 laddr, int ucrw, struct tlb_entry **cached_ep);

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
#define	PAGING_AFTER_TLB_MISS(laddr, ucrw)	paging_walk((laddr), (ucrw), NULL)
#else
#define	PAGING_AFTER_TLB_MISS(laddr, ucrw)	paging((laddr), (ucrw))
#endif

struct tlb_entry* MEMCALL tlb_lookup(UINT32 laddr, int ucrw);

/* 命令フェッチ時のページをキャッシュする　基本は順次アクセスなので1ページのみキャッシュ */
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)

#define TLB_PAGE_OFFSET(laddr)	((laddr) & CPU_PAGE_MASK)
#define TLB_PAGE_BASE(laddr)	((laddr) & ~CPU_PAGE_MASK)

typedef struct codefetch_cache_entry {
	UINT32	lpage;		/* キャッシュ対象の線形ページ先頭アドレス　linear page base */
	int	ucrw;		/* アクセス種別　permission key used to create this cache */
	UINT8	*host_page;	/* 直接アクセス可能な場合のポインタ　direct host pointer for this linear page */
} codefetch_cache_entry_t;

static codefetch_cache_entry_t codefetch_cache;

static void MEMCALL
codefetch_cache_invalidate(void)
{
	codefetch_cache.host_page = NULL;
}

static void MEMCALL
codefetch_cache_invalidate_page(UINT32 laddr)
{
	if (codefetch_cache.host_page != NULL &&
	    codefetch_cache.lpage == TLB_PAGE_BASE(laddr)) {
		codefetch_cache.host_page = NULL;
	}
}

static UINT8 * MEMCALL
codefetch_cache_lookup(UINT32 laddr, int ucrw)
{
	if (codefetch_cache.host_page != NULL &&
	    codefetch_cache.lpage == TLB_PAGE_BASE(laddr) &&
	    codefetch_cache.ucrw == ucrw) {
		return codefetch_cache.host_page;
	}
	return NULL;
}

static UINT8 * MEMCALL
codefetch_cache_update(UINT32 laddr, int ucrw, struct tlb_entry *ep)
{
	if (ep != NULL && (ep->fast_flags & TLBF_DIRECT_READ)) {
		codefetch_cache.lpage = TLB_PAGE_BASE(laddr);
		codefetch_cache.ucrw = ucrw;
		codefetch_cache.host_page = ep->host_page;
		return ep->host_page;
	}
	return NULL;
}
#endif


#if defined(USE_CPU_BULKREP)
UINT8 * MEMCALL
cpu_lmemory_get_direct_host_ptr(UINT32 laddr, UINT leng, int ucrw)
{
	UINT32 inPageSize;
	UINT32 paddr;
	UINT8 *host_page;

	if (leng == 0) {
		return NULL;
	}

	/* Keep the bulk operation inside one x86 4KB page. */
	inPageSize = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (leng > inPageSize) {
		return NULL;
	}

	if (CPU_STAT_PAGING) {
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
		struct tlb_entry *ep;
		UINT offset;

		ep = tlb_lookup_fast(laddr, ucrw);
		if (ep == NULL) {
			/* May set A/D bits or raise the exact first-page exception. */
			(void)paging_walk(laddr, ucrw, &ep);
		}
		if (ep != NULL) {
			offset = laddr & CPU_PAGE_MASK;
			if ((ucrw & CPU_PAGE_WRITE) != 0) {
				if (ep->fast_flags & TLBF_DIRECT_WRITE) {
					return ep->host_page + offset;
				}
			} else {
				if (ep->fast_flags & TLBF_DIRECT_READ) {
					return ep->host_page + offset;
				}
			}
		}
		return NULL;
#else
		paddr = paging(laddr, ucrw) & ~CPU_PAGE_MASK;
#endif
	} else {
		paddr = laddr & ~CPU_PAGE_MASK;
	}

	host_page = memp_get_direct_host_page(paddr);
	if (host_page == NULL) {
		return NULL;
	}
	return host_page + (laddr & CPU_PAGE_MASK);
}
#endif

/*
 * linear memory access
 */
/* RMW */
UINT8 MEMCALL
cpu_memory_access_la_RMW_b(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg)
{
	const int ucrw = CPU_PAGE_WRITE_DATA | CPU_STAT_USER_MODE;
	UINT32 paddr;
	UINT32 result;
	UINT8 value;

	paddr = paging(laddr, ucrw);
	value = cpu_memoryread(paddr);
	result = (*func)(value, arg);
	cpu_memorywrite(paddr, (UINT8)result);
	return value;
}

UINT16 MEMCALL
cpu_memory_access_la_RMW_w(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg)
{
	const int ucrw = CPU_PAGE_WRITE_DATA | CPU_STAT_USER_MODE;
	UINT32 paddr[2];
	UINT32 result;
	UINT16 value;

	paddr[0] = paging(laddr, ucrw);
	if ((laddr + 1) & CPU_PAGE_MASK) {
		value = cpu_memoryread_w(paddr[0]);
		result = (*func)(value, arg);
		cpu_memorywrite_w(paddr[0], (UINT16)result);
		return value;
	}

	paddr[1] = paging(laddr + 1, ucrw);
	value = cpu_memoryread_b(paddr[0]);
	value += (UINT16)cpu_memoryread_b(paddr[1]) << 8;
	result = (*func)(value, arg);
	cpu_memorywrite(paddr[0], (UINT8)result);
	cpu_memorywrite(paddr[1], (UINT8)(result >> 8));
	return value;
}

UINT32 MEMCALL
cpu_memory_access_la_RMW_d(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg)
{
	const int ucrw = CPU_PAGE_WRITE_DATA | CPU_STAT_USER_MODE;
	UINT32 paddr[2];
	UINT32 result;
	UINT32 value;
	int remain;

	paddr[0] = paging(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= 4) {
		value = cpu_memoryread_d(paddr[0]);
		result = (*func)(value, arg);
		cpu_memorywrite_d(paddr[0], result);
		return value;
	}

	paddr[1] = paging(laddr + remain, ucrw);
	switch (remain) {
	case 3:
		value = cpu_memoryread(paddr[0]);
		value += (UINT32)cpu_memoryread_w(paddr[0] + 1) << 8;
		value += (UINT32)cpu_memoryread(paddr[1]) << 24;
		result = (*func)(value, arg);
		cpu_memorywrite(paddr[0], (UINT8)result);
		cpu_memorywrite_w(paddr[0] + 1, (UINT16)(result >> 8));
		cpu_memorywrite(paddr[1], (UINT8)(result >> 24));
		break;

	case 2:
		value = cpu_memoryread_w(paddr[0]);
		value += (UINT32)cpu_memoryread_w(paddr[1]) << 16;
		result = (*func)(value, arg);
		cpu_memorywrite_w(paddr[0], (UINT16)result);
		cpu_memorywrite_w(paddr[1], (UINT16)(result >> 16));
		break;

	case 1:
		value = cpu_memoryread(paddr[0]);
		value += (UINT32)cpu_memoryread_w(paddr[1]) << 8;
		value += (UINT32)cpu_memoryread(paddr[1] + 2) << 24;
		result = (*func)(value, arg);
		cpu_memorywrite(paddr[0], (UINT8)result);
		cpu_memorywrite_w(paddr[1], (UINT16)(result >> 8));
		cpu_memorywrite(paddr[1] + 2, (UINT8)(result >> 24));
		break;

	default:
		ia32_panic("cpu_memory_access_la_RMW_d: out of range (remain=%d)\n", remain);
		value = 0;	/* XXX compiler happy */
		break;
	}
	return value;
}

/* read */
UINT8 MEMCALL
cpu_linear_memory_read_b(UINT32 laddr, int ucrw)
{
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	struct tlb_entry *ep;
	UINT offset;

	ep = tlb_lookup_data_read_fast(laddr, ucrw);
	if (ep != NULL) {
		offset = TLB_PAGE_OFFSET(laddr);
		if (ep->fast_flags & TLBF_DIRECT_READ) {
			return ep->host_page[offset];
		}
		return cpu_memoryread(ep->paddr + offset);
	}
#endif
	return cpu_memoryread(PAGING_AFTER_TLB_MISS(laddr, ucrw));
}
PF_UINT8 MEMCALL
cpu_linear_memory_read_b_codefetch(UINT32 laddr, int ucrw)
{
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	struct tlb_entry *ep;
	UINT offset;
	UINT8 *host_page;

	offset = TLB_PAGE_OFFSET(laddr);
	host_page = codefetch_cache_lookup(laddr, ucrw);
	if (host_page != NULL) {
		return host_page[offset];
	}

	ep = tlb_lookup_code_fast(laddr, ucrw);
	if (ep != NULL) {
		host_page = codefetch_cache_update(laddr, ucrw, ep);
		if (host_page != NULL) {
			return host_page[offset];
		}
		return cpu_memoryread_codefetch(ep->paddr + offset);
	}
#endif
	return cpu_memoryread_codefetch(PAGING_AFTER_TLB_MISS(laddr, ucrw));
}

UINT16 MEMCALL
cpu_linear_memory_read_w(UINT32 laddr, int ucrw)
{
	UINT32 paddr[2];
	UINT16 value;
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	struct tlb_entry *ep;
	UINT offset;
#endif

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	ep = tlb_lookup_data_read_fast(laddr, ucrw);
	if (ep != NULL) {
		offset = TLB_PAGE_OFFSET(laddr);
		if ((laddr + 1) & CPU_PAGE_MASK) {
			if (ep->fast_flags & TLBF_DIRECT_READ) {
				return LOADINTELWORD(ep->host_page + offset);
			}
			return cpu_memoryread_w(ep->paddr + offset);
		}
		paddr[0] = ep->paddr + offset;
	} else
#endif
	{
		paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
		if ((laddr + 1) & CPU_PAGE_MASK)
			return cpu_memoryread_w(paddr[0]);
	}

	paddr[1] = paging(laddr + 1, ucrw);
	value = cpu_memoryread_b(paddr[0]);
	value |= (UINT16)cpu_memoryread_b(paddr[1]) << 8;
	return value;
}
PF_UINT16 MEMCALL
cpu_linear_memory_read_w_codefetch(UINT32 laddr, int ucrw)
{
	UINT32 paddr[2];
	PF_UINT16 value;
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	struct tlb_entry *ep;
	UINT offset;
	UINT8 *host_page;
#endif
	
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	offset = TLB_PAGE_OFFSET(laddr);
	if ((laddr + 1) & CPU_PAGE_MASK) {
		host_page = codefetch_cache_lookup(laddr, ucrw);
		if (host_page != NULL) {
			return LOADINTELWORD(host_page + offset);
		}
	}

	ep = tlb_lookup_code_fast(laddr, ucrw);
	if (ep != NULL) {
		if ((laddr + 1) & CPU_PAGE_MASK) {
			host_page = codefetch_cache_update(laddr, ucrw, ep);
			if (host_page != NULL) {
				return LOADINTELWORD(host_page + offset);
			}
			return cpu_memoryread_w_codefetch(ep->paddr + offset);
		}
		paddr[0] = ep->paddr + offset;
	} else
#endif
	{
		paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
		if ((laddr + 1) & CPU_PAGE_MASK)
			return cpu_memoryread_w_codefetch(paddr[0]);
	}

	paddr[1] = paging(laddr + 1, ucrw);
	value = cpu_memoryread_b_codefetch(paddr[0]);
	value |= (PF_UINT16)cpu_memoryread_b_codefetch(paddr[1]) << 8;
	return value;
}


UINT32 MEMCALL
cpu_linear_memory_read_d(UINT32 laddr, int ucrw)
{
	UINT32 paddr[2];
	UINT32 value;
	UINT remain;

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	{
		struct tlb_entry *ep;
		UINT offset;

		ep = tlb_lookup_data_read_fast(laddr, ucrw);
		if (ep != NULL) {
			offset = TLB_PAGE_OFFSET(laddr);
			remain = CPU_PAGE_SIZE - offset;
			if (remain >= sizeof(value)) {
				if (ep->fast_flags & TLBF_DIRECT_READ) {
					return LOADINTELDWORD(ep->host_page + offset);
				}
				return cpu_memoryread_d(ep->paddr + offset);
			}
			paddr[0] = ep->paddr + offset;
		} else {
			paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
			remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
			if (remain >= sizeof(value))
				return cpu_memoryread_d(paddr[0]);
		}
	}
#else
	paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value))
		return cpu_memoryread_d(paddr[0]);
#endif

	paddr[1] = paging(laddr + remain, ucrw);
	switch (remain) {
	case 3:
		value = cpu_memoryread(paddr[0]);
		value |= (UINT32)cpu_memoryread_w(paddr[0] + 1) << 8;
		value |= (UINT32)cpu_memoryread(paddr[1]) << 24;
		break;

	case 2:
		value = cpu_memoryread_w(paddr[0]);
		value |= (UINT32)cpu_memoryread_w(paddr[1]) << 16;
		break;

	case 1:
		value = cpu_memoryread(paddr[0]);
		value |= (UINT32)cpu_memoryread_w(paddr[1]) << 8;
		value |= (UINT32)cpu_memoryread(paddr[1] + 2) << 24;
		break;

	default:
		ia32_panic("cpu_linear_memory_read_d: out of range (remain=%d)\n", remain);
		value = 0;	/* XXX compiler happy */
		break;
	}
	return value;
}
PF_UINT32 MEMCALL
cpu_linear_memory_read_d_codefetch(UINT32 laddr, int ucrw)
{
	UINT32 paddr[2];
	UINT32 value;
	UINT remain;

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	{
		struct tlb_entry *ep;
		UINT offset;
		UINT8 *host_page;

		offset = TLB_PAGE_OFFSET(laddr);
		remain = CPU_PAGE_SIZE - offset;
		if (remain >= sizeof(value)) {
			host_page = codefetch_cache_lookup(laddr, ucrw);
			if (host_page != NULL) {
				return LOADINTELDWORD(host_page + offset);
			}
		}

		ep = tlb_lookup_code_fast(laddr, ucrw);
		if (ep != NULL) {
			if (remain >= sizeof(value)) {
				host_page = codefetch_cache_update(laddr, ucrw, ep);
				if (host_page != NULL) {
					return LOADINTELDWORD(host_page + offset);
				}
				return cpu_memoryread_d_codefetch(ep->paddr + offset);
			}
			paddr[0] = ep->paddr + offset;
		} else {
			paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
			if (remain >= sizeof(value))
				return cpu_memoryread_d_codefetch(paddr[0]);
		}
	}
#else
	paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value))
		return cpu_memoryread_d_codefetch(paddr[0]);
#endif

	paddr[1] = paging(laddr + remain, ucrw);
	switch (remain) {
	case 3:
		value = cpu_memoryread_codefetch(paddr[0]);
		value |= (UINT32)cpu_memoryread_w_codefetch(paddr[0] + 1) << 8;
		value |= (UINT32)cpu_memoryread_codefetch(paddr[1]) << 24;
		break;

	case 2:
		value = cpu_memoryread_w_codefetch(paddr[0]);
		value |= (UINT32)cpu_memoryread_w_codefetch(paddr[1]) << 16;
		break;

	case 1:
		value = cpu_memoryread_codefetch(paddr[0]);
		value |= (UINT32)cpu_memoryread_w_codefetch(paddr[1]) << 8;
		value |= (UINT32)cpu_memoryread_codefetch(paddr[1] + 2) << 24;
		break;

	default:
		ia32_panic("cpu_linear_memory_read_d: out of range (remain=%d)\n", remain);
		value = 0;	/* XXX compiler happy */
		break;
	}
	return value;
}

UINT64 MEMCALL
cpu_linear_memory_read_q(UINT32 laddr, int ucrw)
{
	UINT32 paddr[2];
	UINT64 value;
	UINT remain;

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	{
		struct tlb_entry *ep;
		UINT offset;
		UINT8 *host;

		ep = tlb_lookup_data_read_fast(laddr, ucrw);
		if (ep != NULL) {
			offset = TLB_PAGE_OFFSET(laddr);
			remain = CPU_PAGE_SIZE - offset;
			if (remain >= sizeof(value)) {
				if (ep->fast_flags & TLBF_DIRECT_READ) {
					host = ep->host_page + offset;
					value = LOADINTELDWORD(host);
					value |= (UINT64)LOADINTELDWORD(host + 4) << 32;
					return value;
				}
				return cpu_memoryread_q(ep->paddr + offset);
			}
			paddr[0] = ep->paddr + offset;
		} else {
			paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
			remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
			if (remain >= sizeof(value))
				return cpu_memoryread_q(paddr[0]);
		}
	}
#else
	paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value))
		return cpu_memoryread_q(paddr[0]);
#endif

	paddr[1] = paging(laddr + remain, ucrw);
	switch (remain) {
	case 7:
		value = cpu_memoryread(paddr[0]);
		value += (UINT64)cpu_memoryread_w(paddr[0] + 1) << 8;
		value += (UINT64)cpu_memoryread_d(paddr[0] + 3) << 24;
		value += (UINT64)cpu_memoryread(paddr[1]) << 56;
		break;

	case 6:
		value = cpu_memoryread_w(paddr[0]);
		value += (UINT64)cpu_memoryread_d(paddr[0] + 2) << 16;
		value += (UINT64)cpu_memoryread_w(paddr[1]) << 48;
		break;

	case 5:
		value = cpu_memoryread(paddr[0]);
		value += (UINT64)cpu_memoryread_d(paddr[0] + 1) << 8;
		value += (UINT64)cpu_memoryread_w(paddr[1]) << 40;
		value += (UINT64)cpu_memoryread(paddr[1] + 2) << 56;
		break;

	case 4:
		value = cpu_memoryread_d(paddr[0]);
		value += (UINT64)cpu_memoryread_d(paddr[1]) << 32;
		break;

	case 3:
		value = cpu_memoryread(paddr[0]);
		value += (UINT64)cpu_memoryread_w(paddr[0] + 1) << 8;
		value += (UINT64)cpu_memoryread_d(paddr[1]) << 24;
		value += (UINT64)cpu_memoryread(paddr[1] + 4) << 56;
		break;

	case 2:
		value = cpu_memoryread_w(paddr[0]);
		value += (UINT64)cpu_memoryread_d(paddr[1]) << 16;
		value += (UINT64)cpu_memoryread_w(paddr[1] + 4) << 48;
		break;

	case 1:
		value = cpu_memoryread(paddr[0]);
		value += (UINT64)cpu_memoryread_d(paddr[1]) << 8;
		value += (UINT64)cpu_memoryread_w(paddr[1] + 4) << 40;
		value += (UINT64)cpu_memoryread(paddr[1] + 6) << 56;
		break;

	default:
		ia32_panic("cpu_linear_memory_read_q: out of range (remain=%d)\n", remain);
		value = 0;	/* XXX compiler happy */
		break;
	}
	return value;
}

REG80 MEMCALL
cpu_linear_memory_read_f(UINT32 laddr, int ucrw)
{
	UINT32 paddr[2];
	REG80 value;
	UINT remain;
	UINT size;
	UINT i, j;

	size = sizeof(value.b);
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	{
		struct tlb_entry *ep;
		UINT offset;
		UINT8 *host;

		ep = tlb_lookup_data_read_fast(laddr, ucrw);
		if (ep != NULL) {
			offset = TLB_PAGE_OFFSET(laddr);
			remain = CPU_PAGE_SIZE - offset;
			if (remain >= size) {
				if (ep->fast_flags & TLBF_DIRECT_READ) {
					host = ep->host_page + offset;
					value.d.l[0] = LOADINTELDWORD(host);
					value.d.l[1] = LOADINTELDWORD(host + 4);
					value.d.h = LOADINTELWORD(host + 8);
					return value;
				}
				return cpu_memoryread_f(ep->paddr + offset);
			}
			paddr[0] = ep->paddr + offset;
		} else {
			paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
			remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
			if (remain >= size)
				return cpu_memoryread_f(paddr[0]);
		}
	}
#else
	paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= size)
		return cpu_memoryread_f(paddr[0]);
#endif

	paddr[1] = paging(laddr + remain, ucrw);
	for (i = 0; i < remain; ++i) {
		value.b[i] = cpu_memoryread(paddr[0] + i);
	}
	for (j = 0; i < size; ++i, ++j) {
		value.b[i] = cpu_memoryread(paddr[1] + j);
	}
	return value;
}

void MEMCALL
cpu_linear_memory_reads(UINT32 laddr, void* dat, UINT leng, int ucrw)
{
	UINT32 paddr;
	UINT64 value;
	UINT8* p = (UINT8*)dat;

	while (leng > 0) {
		UINT32 inPageSize = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
		inPageSize = MIN(inPageSize, leng);
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
		{
			struct tlb_entry *ep;
			UINT offset;

			ep = tlb_lookup_data_read_fast(laddr, ucrw);
			if (ep != NULL) {
				offset = TLB_PAGE_OFFSET(laddr);
				if (ep->fast_flags & TLBF_DIRECT_READ) {
					CopyMemory(p, ep->host_page + offset, inPageSize);
				} else {
					memp_reads(ep->paddr + offset, p, inPageSize);
				}
			} else {
				paddr = PAGING_AFTER_TLB_MISS(laddr, ucrw);
				memp_reads(paddr, p, inPageSize);
			}
		}
#else
		paddr = PAGING_AFTER_TLB_MISS(laddr, ucrw);
		memp_reads(paddr, p, inPageSize);
#endif
		p += inPageSize;
		laddr += inPageSize;
		leng -= inPageSize;
	}
}

/* write */
void MEMCALL
cpu_linear_memory_write_b(UINT32 laddr, UINT8 value, int ucrw)
{
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	struct tlb_entry *ep;
	UINT offset;

	ep = tlb_lookup_data_write_fast(laddr, ucrw);
	if (ep != NULL) {
		offset = TLB_PAGE_OFFSET(laddr);
		if (ep->fast_flags & TLBF_DIRECT_WRITE) {
			ep->host_page[offset] = value;
			return;
		}
		cpu_memorywrite(ep->paddr + offset, value);
		return;
	}
#endif
	cpu_memorywrite(PAGING_AFTER_TLB_MISS(laddr, ucrw), value);
}

void MEMCALL
cpu_linear_memory_write_w(UINT32 laddr, UINT16 value, int ucrw)
{
	UINT32 paddr[2];

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	{
		struct tlb_entry *ep;
		UINT offset;

		ep = tlb_lookup_data_write_fast(laddr, ucrw);
		if (ep != NULL) {
			offset = TLB_PAGE_OFFSET(laddr);
			if ((laddr + 1) & CPU_PAGE_MASK) {
				if (ep->fast_flags & TLBF_DIRECT_WRITE) {
					STOREINTELWORD(ep->host_page + offset, value);
					return;
				}
				cpu_memorywrite_w(ep->paddr + offset, value);
				return;
			}
			paddr[0] = ep->paddr + offset;
		} else {
			paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
			if ((laddr + 1) & CPU_PAGE_MASK) {
				cpu_memorywrite_w(paddr[0], value);
				return;
			}
		}
	}
#else
	paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
	if ((laddr + 1) & CPU_PAGE_MASK) {
		cpu_memorywrite_w(paddr[0], value);
		return;
	}
#endif

	paddr[1] = paging(laddr + 1, ucrw);
	cpu_memorywrite(paddr[0], (UINT8)value);
	cpu_memorywrite(paddr[1], (UINT8)(value >> 8));
}

void MEMCALL
cpu_linear_memory_write_d(UINT32 laddr, UINT32 value, int ucrw)
{
	UINT32 paddr[2];
	UINT remain;

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	{
		struct tlb_entry *ep;
		UINT offset;

		ep = tlb_lookup_data_write_fast(laddr, ucrw);
		if (ep != NULL) {
			offset = TLB_PAGE_OFFSET(laddr);
			remain = CPU_PAGE_SIZE - offset;
			if (remain >= sizeof(value)) {
				if (ep->fast_flags & TLBF_DIRECT_WRITE) {
					STOREINTELDWORD(ep->host_page + offset, value);
					return;
				}
				cpu_memorywrite_d(ep->paddr + offset, value);
				return;
			}
			paddr[0] = ep->paddr + offset;
		} else {
			paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
			remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
			if (remain >= sizeof(value)) {
				cpu_memorywrite_d(paddr[0], value);
				return;
			}
		}
	}
#else
	paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value)) {
		cpu_memorywrite_d(paddr[0], value);
		return;
	}
#endif

	paddr[1] = paging(laddr + remain, ucrw);
	switch (remain) {
	case 3:
		cpu_memorywrite(paddr[0], (UINT8)value);
		cpu_memorywrite_w(paddr[0] + 1, (UINT16)(value >> 8));
		cpu_memorywrite(paddr[1], (UINT8)(value >> 24));
		break;

	case 2:
		cpu_memorywrite_w(paddr[0], (UINT16)value);
		cpu_memorywrite_w(paddr[1], (UINT16)(value >> 16));
		break;

	case 1:
		cpu_memorywrite(paddr[0], (UINT8)value);
		cpu_memorywrite_w(paddr[1], (UINT16)(value >> 8));
		cpu_memorywrite(paddr[1] + 2, (UINT8)(value >> 24));
		break;

	default:
		ia32_panic("cpu_linear_memory_write_d: out of range (remain=%d)\n", remain);
		break;
	}
}

void MEMCALL
cpu_linear_memory_write_q(UINT32 laddr, UINT64 value, int ucrw)
{
	UINT32 paddr[2];
	UINT remain;

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	{
		struct tlb_entry *ep;
		UINT offset;
		UINT8 *host;

		ep = tlb_lookup_data_write_fast(laddr, ucrw);
		if (ep != NULL) {
			offset = TLB_PAGE_OFFSET(laddr);
			remain = CPU_PAGE_SIZE - offset;
			if (remain >= sizeof(value)) {
				if (ep->fast_flags & TLBF_DIRECT_WRITE) {
					host = ep->host_page + offset;
					STOREINTELDWORD(host, (UINT32)value);
					STOREINTELDWORD(host + 4, (UINT32)(value >> 32));
					return;
				}
				cpu_memorywrite_q(ep->paddr + offset, value);
				return;
			}
			paddr[0] = ep->paddr + offset;
		} else {
			paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
			remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
			if (remain >= sizeof(value)) {
				cpu_memorywrite_q(paddr[0], value);
				return;
			}
		}
	}
#else
	paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value)) {
		cpu_memorywrite_q(paddr[0], value);
		return;
	}
#endif

	paddr[1] = paging(laddr + remain, ucrw);
	switch (remain) {
	case 7:
		cpu_memorywrite(paddr[0], (UINT8)value);
		cpu_memorywrite_w(paddr[0] + 1, (UINT16)(value >> 8));
		cpu_memorywrite_d(paddr[0] + 3, (UINT32)(value >> 24));
		cpu_memorywrite(paddr[1], (UINT8)(value >> 56));
		break;

	case 6:
		cpu_memorywrite_w(paddr[0], (UINT16)value);
		cpu_memorywrite_d(paddr[0] + 2, (UINT32)(value >> 16));
		cpu_memorywrite_w(paddr[1], (UINT16)(value >> 48));
		break;

	case 5:
		cpu_memorywrite(paddr[0], (UINT8)value);
		cpu_memorywrite_d(paddr[0] + 1, (UINT32)(value >> 8));
		cpu_memorywrite_w(paddr[1], (UINT16)(value >> 40));
		cpu_memorywrite(paddr[1] + 2, (UINT8)(value >> 56));
		break;

	case 4:
		cpu_memorywrite_d(paddr[0], (UINT32)value);
		cpu_memorywrite_d(paddr[1], (UINT32)(value >> 32));
		break;

	case 3:
		cpu_memorywrite(paddr[0], (UINT8)value);
		cpu_memorywrite_w(paddr[0] + 1, (UINT16)(value >> 8));
		cpu_memorywrite_d(paddr[1], (UINT32)(value >> 24));
		cpu_memorywrite(paddr[1] + 4, (UINT8)(value >> 56));
		break;

	case 2:
		cpu_memorywrite_w(paddr[0], (UINT16)value);
		cpu_memorywrite_d(paddr[1], (UINT32)(value >> 16));
		cpu_memorywrite_w(paddr[1] + 4, (UINT16)(value >> 48));
		break;

	case 1:
		cpu_memorywrite(paddr[0], (UINT8)value);
		cpu_memorywrite_d(paddr[1], (UINT32)(value >> 8));
		cpu_memorywrite_w(paddr[1] + 4, (UINT16)(value >> 40));
		cpu_memorywrite(paddr[1] + 6, (UINT8)(value >> 56));
		break;

	default:
		ia32_panic("cpu_linear_memory_write_q: out of range (remain=%d)\n", remain);
		break;
	}
}

void MEMCALL
cpu_linear_memory_write_f(UINT32 laddr, const REG80 *value, int ucrw)
{
	UINT32 paddr[2];
	UINT remain;
	UINT size;
	UINT i, j;

	size = sizeof(value->b);
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	{
		struct tlb_entry *ep;
		UINT offset;
		UINT8 *host;

		ep = tlb_lookup_data_write_fast(laddr, ucrw);
		if (ep != NULL) {
			offset = TLB_PAGE_OFFSET(laddr);
			remain = CPU_PAGE_SIZE - offset;
			if (remain >= size) {
				if (ep->fast_flags & TLBF_DIRECT_WRITE) {
					host = ep->host_page + offset;
					STOREINTELDWORD(host, value->d.l[0]);
					STOREINTELDWORD(host + 4, value->d.l[1]);
					STOREINTELWORD(host + 8, value->d.h);
					return;
				}
				cpu_memorywrite_f(ep->paddr + offset, value);
				return;
			}
			paddr[0] = ep->paddr + offset;
		} else {
			paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
			remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
			if (remain >= size) {
				cpu_memorywrite_f(paddr[0], value);
				return;
			}
		}
	}
#else
	paddr[0] = PAGING_AFTER_TLB_MISS(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= size) {
		cpu_memorywrite_f(paddr[0], value);
		return;
	}
#endif

	paddr[1] = paging(laddr + remain, ucrw);
	for (i = 0; i < remain; ++i) {
		cpu_memorywrite(paddr[0] + i, value->b[i]);
	}
	for (j = 0; i < size; ++i, ++j) {
		cpu_memorywrite(paddr[1] + j, value->b[i]);
	}
}

void MEMCALL
cpu_linear_memory_writes(UINT32 laddr, void* dat, UINT leng, int ucrw)
{
	UINT32 paddr;
	UINT64 value;
	UINT8* p = (UINT8*)dat;

	while (leng > 0) {
		UINT32 inPageSize = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
		inPageSize = MIN(inPageSize, leng);
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
		{
			struct tlb_entry *ep;
			UINT offset;

			ep = tlb_lookup_data_write_fast(laddr, ucrw);
			if (ep != NULL) {
				offset = TLB_PAGE_OFFSET(laddr);
				if (ep->fast_flags & TLBF_DIRECT_WRITE) {
					CopyMemory(ep->host_page + offset, p, inPageSize);
				} else {
					memp_writes(ep->paddr + offset, p, inPageSize);
				}
			} else {
				paddr = PAGING_AFTER_TLB_MISS(laddr, ucrw);
				memp_writes(paddr, p, inPageSize);
			}
		}
#else
		paddr = PAGING_AFTER_TLB_MISS(laddr, ucrw);
		memp_writes(paddr, p, inPageSize);
#endif
		p += inPageSize;
		laddr += inPageSize;
		leng -= inPageSize;
	}
}

/*
 * linear address memory access function
 */
void MEMCALL
cpu_memory_access_la_region(UINT32 laddr, UINT length, int ucrw, UINT8 *data)
{
	UINT32 paddr;
	UINT remain;	/* page remain */
	UINT r;

	while (length > 0) {
		remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
		if (!CPU_STAT_PAGING) {
			paddr = laddr;
		} else {
			paddr = paging(laddr, ucrw);
		}

		r = (remain > length) ? length : remain;
		if (!(ucrw & CPU_PAGE_WRITE)) {
			cpu_memoryread_region(paddr, data, r);
		} else {
			cpu_memorywrite_region(paddr, data, r);
		}

		laddr += r;
		data += r;
		length -= r;
	}
}

UINT32 MEMCALL
laddr2paddr(UINT32 laddr, int ucrw)
{

	return paging(laddr, ucrw);
}

/*
 * paging
 */
static UINT32 MEMCALL
paging(UINT32 laddr, int ucrw)
{
	struct tlb_entry *ep;

#if !defined(SUPPORT_IA32_HAXM) // HAXMはエミュレーションTLBを使わない
#if IA32_MEMORY_FAST_PATH
	ep = tlb_lookup_fast(laddr, ucrw);
#else
	ep = tlb_lookup(laddr, ucrw);
#endif
	if (ep != NULL)
		return ep->paddr + (laddr & CPU_PAGE_MASK);
#endif

	return paging_walk(laddr, ucrw, NULL);
}

static UINT32 MEMCALL
paging_walk(UINT32 laddr, int ucrw, struct tlb_entry **cached_ep)
{
	struct tlb_entry *ep;
	UINT32 paddr;		/* physical address */
	UINT32 pde_addr;	/* page directory entry address */
	UINT32 pde;		/* page directory entry */
	UINT32 pte_addr;	/* page table entry address */
	UINT32 pte;		/* page table entry */
	UINT bit;
	UINT err;

	if (cached_ep != NULL) {
		*cached_ep = NULL;
	}

	pde_addr = CPU_STAT_PDE_BASE + ((laddr >> 20) & 0xffc);
	pde = cpu_memoryread_d_paging(pde_addr);
	if (!(pde & CPU_PDE_PRESENT)) {
		VERBOSE(("paging: PTE page is not present"));
		VERBOSE(("paging: CPU_CR3 = 0x%08x", CPU_CR3));
		VERBOSE(("paging: laddr = 0x%08x, pde_addr = 0x%08x, pde = 0x%08x", laddr, pde_addr, pde));
		err = 0;
		goto pf_exception;
	}
	if (!(pde & CPU_PDE_ACCESS)) {
		pde |= CPU_PDE_ACCESS;
		cpu_memorywrite_d_paging(pde_addr, pde);
	}

#if defined(SUPPORT_IA32_HAXM) // HAXMは4MBページのことがある
	if ((CPU_CR4 & CPU_CR4_PSE) && (pde & CPU_PDE_PAGE_SIZE)) {
		/* 4MB page size */
		paddr = (pde & CPU_PDE_4M_BASEADDR_MASK) | (laddr & 0x003fffff);

		bit = ucrw & (CPU_PAGE_WRITE | CPU_PAGE_USER_MODE);
		bit |= (pde & (CPU_PTE_WRITABLE | CPU_PTE_USER_MODE));
		bit |= CPU_STAT_WP;
	}
	else
#endif
	{
		/* 4KB page size */
		pte_addr = (pde & CPU_PDE_BASEADDR_MASK) + ((laddr >> 10) & 0xffc);
		pte = cpu_memoryread_d_paging(pte_addr);
		if (!(pte & CPU_PTE_PRESENT)) {
			VERBOSE(("paging: page is not present"));
			VERBOSE(("paging: laddr = 0x%08x, pde_addr = 0x%08x, pde = 0x%08x", laddr, pde_addr, pde));
			VERBOSE(("paging: pte_addr = 0x%08x, pte = 0x%08x", pte_addr, pte));
			err = 0;
			goto pf_exception;
		}
		if (!(pte & CPU_PTE_ACCESS)) {
			pte |= CPU_PTE_ACCESS;
			cpu_memorywrite_d_paging(pte_addr, pte);
		}

		/* make physical address */
		paddr = (pte & CPU_PTE_BASEADDR_MASK) + (laddr & CPU_PAGE_MASK);

		bit = ucrw & (CPU_PAGE_WRITE | CPU_PAGE_USER_MODE);
		bit |= (pde & pte & (CPU_PTE_WRITABLE | CPU_PTE_USER_MODE));
		bit |= CPU_STAT_WP;

#if !defined(USE_PAGE_ACCESS_TABLE)
		if (!(page_access & (1 << bit)))
#else
		if (!(page_access_bit[bit]))
#endif
		{
			VERBOSE(("paging: page access violation."));
			VERBOSE(("paging: laddr = 0x%08x, pde_addr = 0x%08x, pde = 0x%08x", laddr, pde_addr, pde));
			VERBOSE(("paging: pte_addr = 0x%08x, pte = 0x%08x", pte_addr, pte));
			VERBOSE(("paging: paddr = 0x%08x, bit = 0x%08x", paddr, bit));
			err = 1;
			goto pf_exception;
		}

		if ((ucrw & CPU_PAGE_WRITE) && !(pte & CPU_PTE_DIRTY)) {
			pte |= CPU_PTE_DIRTY;
			cpu_memorywrite_d_paging(pte_addr, pte);
		}

		ep = tlb_update(laddr, pte, (bit & (CPU_PTE_WRITABLE | CPU_PTE_USER_MODE)) + ((ucrw & CPU_PAGE_CODE) ? 1 : 0));
		if (cached_ep != NULL) {
			*cached_ep = ep;
		}
	}

	return paddr;

pf_exception:
	CPU_CR2 = laddr;
	err |= (ucrw & CPU_PAGE_WRITE) << 1;
	err |= (ucrw & CPU_PAGE_USER_MODE) >> 1;
	EXCEPTION(PF_EXCEPTION, err);
	return 0;	/* compiler happy */
}

/* 
 * TLB
 */
void
tlb_init(void)
{
	memset(tlb, 0, sizeof(tlb));
	tlb_update_access_flags();
#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	codefetch_cache_invalidate();
#endif
}

void MEMCALL
tlb_flush()
{
	struct tlb_entry *ep;
	int i;
	int n;

	/* Keep the precomputed CR0.WP-dependent write flag synchronized. */
	tlb_update_access_flags();

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	codefetch_cache_invalidate();
#endif

	for (n = 0; n < NTLB; n++) {
		for (i = 0; i < NENTRY ; i++) {
			ep = &tlb[n].entry[i];
			if (TLB_IS_VALID(ep) && !TLB_IS_GLOBAL(ep)) {
				TLB_SET_INVALID(ep);
			}
		}
	}
}

void MEMCALL
tlb_flush_all()
{
	tlb_init();
}

void MEMCALL
tlb_flush_page(UINT32 laddr)
{
	struct tlb_entry *ep;
	int idx;
	int n;

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	codefetch_cache_invalidate_page(laddr);
#endif

	idx = (laddr >> TLB_ENTRY_SHIFT) & TLB_ENTRY_MASK;

	for (n = 0; n < NTLB; n++) {
		ep = &tlb[n].entry[idx];
		if (TLB_IS_VALID(ep)) {
			if ((laddr & TLB_TAG_MASK) == TLB_GET_TAG_ADDR(ep)) {
				TLB_SET_INVALID(ep);
			}
		}
	}
}

struct tlb_entry * MEMCALL
tlb_lookup(UINT32 laddr, int ucrw)
{
#if IA32_MEMORY_FAST_PATH
	return tlb_lookup_fast(laddr, ucrw);
#else
	struct tlb_entry *ep;
	UINT bit;
	int idx;
	int n;

	n = (ucrw & CPU_PAGE_CODE) ? 1 : 0;
	idx = (laddr >> TLB_ENTRY_SHIFT) & TLB_ENTRY_MASK;
	ep = &tlb[n].entry[idx];

	if (TLB_IS_VALID(ep)) {
		if ((laddr & TLB_TAG_MASK) == TLB_GET_TAG_ADDR(ep)) {
			bit = ucrw & (CPU_PAGE_WRITE|CPU_PAGE_USER_MODE);
			bit |= ep->tag & (CPU_PTE_WRITABLE|CPU_PTE_USER_MODE);
			bit |= CPU_STAT_WP;
#if !defined(USE_PAGE_ACCESS_TABLE)
			if ((page_access & (1 << bit)))
#else
			if (page_access_bit[bit])
#endif
			{
				if (!(ucrw & CPU_PAGE_WRITE) || TLB_IS_DIRTY(ep)) {
					return ep;
				}
			}
		}
	}
	return NULL;
#endif
}

static struct tlb_entry * MEMCALL
tlb_update(UINT32 laddr, UINT entry, int bit)
{
	struct tlb_entry *ep;
	int idx;
	int n;

	n = bit & 1;
	idx = (laddr >> TLB_ENTRY_SHIFT) & TLB_ENTRY_MASK;
	ep = &tlb[n].entry[idx];

#if IA32_MEMORY_FAST_PATH && !defined(SUPPORT_IA32_HAXM)
	if (n == 1) {
		codefetch_cache_invalidate_page(laddr);
	}
#endif
	TLB_SET_VALID(ep);
	TLB_SET_TAG_ADDR(ep, laddr);
	TLB_SET_TAG_FLAGS(ep, entry, bit);
	ep->paddr = entry & CPU_PTE_BASEADDR_MASK;
#if IA32_MEMORY_FAST_PATH
	ep->host_page = memp_get_direct_host_page(ep->paddr);
	ep->fast_flags = tlb_make_fast_flags(entry, bit, n, ep->host_page != NULL);
#endif
	return ep;
}
