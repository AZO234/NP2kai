/*
 * Copyright (c) 2003 NONAKA Kimihiro
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

#ifndef	IA32_CPU_PAGING_H__
#define	IA32_CPU_PAGING_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ページ・ディレクトリ・エントリ (4K バイトページ使用時)
 *
 *  31                                    12 11   9 8  7 6 5  4   3   2   1  0 
 * +----------------------------------------+------+-+--+-+-+---+---+---+---+-+
 * |   ページ・テーブルのベース・アドレス   |使用可|G|PS|-|A|PCD|PWT|U/S|R/W|P|
 * +----------------------------------------+------+-+--+-+-+---+---+---+---+-+
 *                                              |   |  | | |  |   |   |   |  |
 * 9-11: システム・プログラマが使用可能 --------+   |  | | |  |   |   |   |  |
 *    8: グローバル・ページ(無視される) ------------+  | | |  |   |   |   |  |
 *    7: ページ・サイズ (0 = 4k バイトページ) ---------+ | |  |   |   |   |  |
 *    6: 予約 (-) ---------------------------------------+ |  |   |   |   |  |
 *    5: アクセス -----------------------------------------+  |   |   |   |  |
 *    4: キャッシュ無効 --------------------------------------+   |   |   |  |
 *    3: ライトスルー --------------------------------------------+   |   |  |
 *    2: ユーザ／スーパバイザ (0 = スーパバイザ) ---------------------+   |  |
 *    1: 読み取り／書き込み (0 = 読み取りのみ) ---------------------------+  |
 *    0: ページ存在 ---------------------------------------------------------+
 */
#define	CPU_PDE_BASEADDR_MASK	0xfffff000
#define	CPU_PDE_GLOBAL_PAGE	(1 << 8)
#define	CPU_PDE_PAGE_SIZE	(1 << 7)
#define	CPU_PDE_DIRTY		(1 << 6)
#define	CPU_PDE_ACCESS		(1 << 5)
#define	CPU_PDE_CACHE_DISABLE	(1 << 4)
#define	CPU_PDE_WRITE_THROUGH	(1 << 3)
#define	CPU_PDE_USER_MODE	(1 << 2)
#define	CPU_PDE_WRITABLE	(1 << 1)
#define	CPU_PDE_PRESENT		(1 << 0)

/*
 * ページ・ディレクトリ・エントリ (4M バイトページ使用時)
 * 
 *  31                        22 21       12 11   9 8  7 6 5  4   3   2   1  0 
 * +----------------------------+-----------+------+-+--+-+-+---+---+---+---+-+
 * |ページテーブルの物理アドレス|  予約済み |使用可|G|PS|D|A|PCD|PWT|U/S|R/W|P|
 * +----------------------------+-----------+------+-+--+-+-+---+---+---+---+-+
 *                                              |   |  | | |  |   |   |   |  |
 * 9-11: システム・プログラマが使用可能 --------+   |  | | |  |   |   |   |  |
 *    8: グローバル・ページ ------------------------+  | | |  |   |   |   |  |
 *    7: ページ・サイズ (1 = 4M バイトページ) ---------+ | |  |   |   |   |  |
 *    6: ダーティ ---------------------------------------+ |  |   |   |   |  |
 *    5: アクセス -----------------------------------------+  |   |   |   |  |
 *    4: キャッシュ無効 --------------------------------------+   |   |   |  |
 *    3: ライトスルー --------------------------------------------+   |   |  |
 *    2: ユーザ／スーパバイザ (0 = スーパバイザ) ---------------------+   |  |
 *    1: 読み取り／書き込み (0 = 読み取りのみ) ---------------------------+  |
 *    0: ページ存在 ---------------------------------------------------------+
 */
#define	CPU_PDE_4M_BASEADDR_MASK	0xffc00000
#define	CPU_PDE_4M_GLOBAL_PAGE		(1 << 8)
#define	CPU_PDE_4M_PAGE_SIZE		(1 << 7)
#define	CPU_PDE_4M_DIRTY		(1 << 6)
#define	CPU_PDE_4M_ACCESS		(1 << 5)
#define	CPU_PDE_4M_CACHE_DISABLE	(1 << 4)
#define	CPU_PDE_4M_WRITE_THROUGH	(1 << 3)
#define	CPU_PDE_4M_USER_MODE		(1 << 2)
#define	CPU_PDE_4M_WRITABLE		(1 << 1)
#define	CPU_PDE_4M_PRESENT		(1 << 0)

/*
 * ページ・テーブル・エントリ (4k バイト・ページ)
 *
 *  31                                    12 11   9 8 7 6 5  4   3   2   1  0 
 * +----------------------------------------+------+-+-+-+-+---+---+---+---+-+
 * |        ページのベース・アドレス        |使用可|G|-|D|A|PCD|PWT|U/S|R/W|P|
 * +----------------------------------------+------+-+-+-+-+---+---+---+---+-+
 *                                              |   | | | |  |   |   |   |  |
 *  9-11: システム・プログラマが使用可能 -------+   | | | |  |   |   |   |  |
 *     8: グローバル・ページ -----------------------+ | | |  |   |   |   |  |
 *     7: 予約 (-) -----------------------------------+ | |  |   |   |   |  |
 *     6: ダーティ -------------------------------------+ |  |   |   |   |  |
 *     5: アクセス ---------------------------------------+  |   |   |   |  |
 *     4: キャッシュ無効 ------------------------------------+   |   |   |  |
 *     3: ライトスルー ------------------------------------------+   |   |  |
 *     2: ユーザ／スーパバイザ (0 = スーパバイザ) -------------------+   |  |
 *     1: 読み取り／書き込み (0 = 読み取りのみ) -------------------------+  |
 *     0: ページ存在 -------------------------------------------------------+
 */
#define	CPU_PTE_BASEADDR_MASK	0xfffff000
#define	CPU_PTE_GLOBAL_PAGE	(1 << 8)
#define	CPU_PTE_PAGE_SIZE	(1 << 7)
#define	CPU_PTE_DIRTY		(1 << 6)
#define	CPU_PTE_ACCESS		(1 << 5)
#define	CPU_PTE_CACHE_DISABLE	(1 << 4)
#define	CPU_PTE_WRITE_THROUGH	(1 << 3)
#define	CPU_PTE_USER_MODE	(1 << 2)
#define	CPU_PTE_WRITABLE	(1 << 1)
#define	CPU_PTE_PRESENT		(1 << 0)

#define CPU_PAGE_SIZE      	 0x1000
#define CPU_PAGE_MASK       	(CPU_PAGE_SIZE - 1)

/* ucrw */
#define	CPU_PAGE_WRITE		(1 << 0)
#define	CPU_PAGE_CODE		(1 << 1)
#define	CPU_PAGE_DATA		(1 << 2)
#define	CPU_PAGE_USER_MODE	(1 << 3)	/* == CPU_MODE_USER */
#define	CPU_PAGE_READ_CODE	(CPU_PAGE_CODE)
#define	CPU_PAGE_READ_DATA	(CPU_PAGE_DATA)
#define	CPU_PAGE_WRITE_DATA	(CPU_PAGE_WRITE|CPU_PAGE_DATA)

UINT8 MEMCALL cpu_memory_access_la_RMW_b(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg);
UINT16 MEMCALL cpu_memory_access_la_RMW_w(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg);
UINT32 MEMCALL cpu_memory_access_la_RMW_d(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg);
UINT8 MEMCALL cpu_linear_memory_read_b(UINT32 laddr, int ucrw);
UINT16 MEMCALL cpu_linear_memory_read_w(UINT32 laddr, int ucrw);
UINT32 MEMCALL cpu_linear_memory_read_d(UINT32 laddr, int ucrw);
UINT64 MEMCALL cpu_linear_memory_read_q(UINT32 laddr, int ucrw);
REG80 MEMCALL cpu_linear_memory_read_f(UINT32 laddr, int ucrw);
void MEMCALL cpu_linear_memory_write_b(UINT32 laddr, UINT8 value, int ucrw);
void MEMCALL cpu_linear_memory_write_w(UINT32 laddr, UINT16 value, int ucrw);
void MEMCALL cpu_linear_memory_write_d(UINT32 laddr, UINT32 value, int ucrw);
void MEMCALL cpu_linear_memory_write_q(UINT32 laddr, UINT64 value, int ucrw);
void MEMCALL cpu_linear_memory_write_f(UINT32 laddr, const REG80 *value, int ucrw);

/*
 * linear address memory access function with TLB
 */
/* RMW */
STATIC_INLINE UINT8 MEMCALL
cpu_lmemory_RMW_b(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg)
{
	UINT32 result;
	UINT8 value;

	if (!CPU_STAT_PAGING) {
		value = cpu_memoryread_b(laddr);
		result = (*func)(value, arg);
		cpu_memorywrite_b(laddr, (UINT8)result);
		return value;
	}
	return cpu_memory_access_la_RMW_b(laddr, func, arg);
}

STATIC_INLINE UINT16 MEMCALL
cpu_lmemory_RMW_w(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg)
{
	UINT32 result;
	UINT16 value;

	if (!CPU_STAT_PAGING) {
		value = cpu_memoryread_w(laddr);
		result = (*func)(value, arg);
		cpu_memorywrite_w(laddr, (UINT16)result);
		return value;
	}
	return cpu_memory_access_la_RMW_w(laddr, func, arg);
}

STATIC_INLINE UINT32 MEMCALL
cpu_lmemory_RMW_d(UINT32 laddr, UINT32 (CPUCALL *func)(UINT32, void *), void *arg)
{
	UINT32 result;
	UINT32 value;

	if (!CPU_STAT_PAGING) {
		value = cpu_memoryread_d(laddr);
		result = (*func)(value, arg);
		cpu_memorywrite_d(laddr, result);
		return value;
	}
	return cpu_memory_access_la_RMW_d(laddr, func, arg);
}

/* read */
STATIC_INLINE UINT8 MEMCALL
cpu_lmemoryread_b(UINT32 laddr, int ucrw)
{

	if (!CPU_STAT_PAGING)
		return cpu_memoryread_b(laddr);
	return cpu_linear_memory_read_b(laddr, ucrw);
}
#define	cpu_lmemoryread(a,ucrw) cpu_lmemoryread_b((a),(ucrw))

STATIC_INLINE UINT16 MEMCALL
cpu_lmemoryread_w(UINT32 laddr, int ucrw)
{

	if (!CPU_STAT_PAGING)
		return cpu_memoryread_w(laddr);
	return cpu_linear_memory_read_w(laddr, ucrw);
}

STATIC_INLINE UINT32 MEMCALL
cpu_lmemoryread_d(UINT32 laddr, int ucrw)
{

	if (!CPU_STAT_PAGING)
		return cpu_memoryread_d(laddr);
	return cpu_linear_memory_read_d(laddr, ucrw);
}

STATIC_INLINE UINT64
cpu_lmemoryread_q(UINT32 laddr, int ucrw)
{

	if (!CPU_STAT_PAGING)
		return cpu_memoryread_q(laddr);
	return cpu_linear_memory_read_q(laddr, ucrw);
}

STATIC_INLINE REG80
cpu_lmemoryread_f(UINT32 laddr, int ucrw)
{

	if (!CPU_STAT_PAGING)
		return cpu_memoryread_f(laddr);
	return cpu_linear_memory_read_f(laddr, ucrw);
}

/* write */
STATIC_INLINE void MEMCALL
cpu_lmemorywrite_b(UINT32 laddr, UINT8 value, int ucrw)
{

	if (!CPU_STAT_PAGING) {
		cpu_memorywrite_b(laddr, value);
		return;
	}
	cpu_linear_memory_write_b(laddr, value, ucrw);
}
#define	cpu_lmemorywrite(a,v,ucrw) cpu_lmemorywrite_b((a),(v),(ucrw))

STATIC_INLINE void MEMCALL
cpu_lmemorywrite_w(UINT32 laddr, UINT16 value, int ucrw)
{

	if (!CPU_STAT_PAGING) {
		cpu_memorywrite_w(laddr, value);
		return;
	}
	cpu_linear_memory_write_w(laddr, value, ucrw);
}

STATIC_INLINE void MEMCALL
cpu_lmemorywrite_d(UINT32 laddr, UINT32 value, int ucrw)
{

	if (!CPU_STAT_PAGING) {
		cpu_memorywrite_d(laddr, value);
		return;
	}
	cpu_linear_memory_write_d(laddr, value, ucrw);
}

STATIC_INLINE void MEMCALL
cpu_lmemorywrite_q(UINT32 laddr, UINT64 value, int ucrw)
{

	if (!CPU_STAT_PAGING) {
		cpu_memorywrite_q(laddr, value);
		return;
	}
	cpu_linear_memory_write_q(laddr, value, ucrw);
}

STATIC_INLINE void MEMCALL
cpu_lmemorywrite_f(UINT32 laddr, const REG80 *value, int ucrw)
{

	if (!CPU_STAT_PAGING) {
		cpu_memorywrite_f(laddr, value);
		return;
	}
	cpu_linear_memory_write_f(laddr, value, ucrw);
}


/*
 * linear address memory access with superviser mode
 */
#define	cpu_kmemoryread(a) \
	cpu_lmemoryread((a),CPU_PAGE_READ_DATA|CPU_MODE_SUPERVISER)
#define	cpu_kmemoryread_w(a) \
	cpu_lmemoryread_w((a),CPU_PAGE_READ_DATA|CPU_MODE_SUPERVISER)
#define	cpu_kmemoryread_d(a) \
	cpu_lmemoryread_d((a),CPU_PAGE_READ_DATA|CPU_MODE_SUPERVISER)
#define	cpu_kmemorywrite(a,v) \
	cpu_lmemorywrite((a),(v),CPU_PAGE_WRITE_DATA|CPU_MODE_SUPERVISER)
#define	cpu_kmemorywrite_w(a,v) \
	cpu_lmemorywrite_w((a),(v),CPU_PAGE_WRITE_DATA|CPU_MODE_SUPERVISER)
#define	cpu_kmemorywrite_d(a,v) \
	cpu_lmemorywrite_d((a),(v),CPU_PAGE_WRITE_DATA|CPU_MODE_SUPERVISER)

/*
 * linear address memory access function
 */
void MEMCALL cpu_memory_access_la_region(UINT32 address, UINT length, int ucrw, UINT8 *data);
UINT32 MEMCALL laddr2paddr(UINT32 laddr, int ucrw);

STATIC_INLINE UINT32 MEMCALL
laddr_to_paddr(UINT32 laddr, int ucrw)
{

	if (!CPU_STAT_PAGING)
		return laddr;
	return laddr2paddr(laddr, ucrw);
}

/*
 * TLB function
 */
struct tlb_entry;
void tlb_init(void);
void MEMCALL tlb_flush(BOOL allflush);
void MEMCALL tlb_flush_page(UINT32 laddr);
struct tlb_entry *MEMCALL tlb_lookup(UINT32 laddr, int ucrw);

#ifdef __cplusplus
}
#endif

#endif	/* !IA32_CPU_PAGING_H__ */
