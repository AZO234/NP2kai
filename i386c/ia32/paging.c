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

#include "compiler.h"
#include "cpu.h"
#include "ia32.mcr"

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
};
static void MEMCALL tlb_update(UINT32 laddr, UINT entry, int ucrw);

/* paging */
static UINT32 MEMCALL paging(UINT32 laddr, int ucrw);

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

	return cpu_memoryread(paging(laddr, ucrw));
}

UINT16 MEMCALL
cpu_linear_memory_read_w(UINT32 laddr, int ucrw)
{
	UINT32 paddr[2];
	UINT16 value;

	paddr[0] = paging(laddr, ucrw);
	if ((laddr + 1) & CPU_PAGE_MASK)
		return cpu_memoryread_w(paddr[0]);

	paddr[1] = paging(laddr + 1, ucrw);
	value = cpu_memoryread_b(paddr[0]);
	value += (UINT16)cpu_memoryread_b(paddr[1]) << 8;
	return value;
}

UINT32 MEMCALL
cpu_linear_memory_read_d(UINT32 laddr, int ucrw)
{
	UINT32 paddr[2];
	UINT32 value;
	UINT remain;

	paddr[0] = paging(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value))
		return cpu_memoryread_d(paddr[0]);

	paddr[1] = paging(laddr + remain, ucrw);
	switch (remain) {
	case 3:
		value = cpu_memoryread(paddr[0]);
		value += (UINT32)cpu_memoryread_w(paddr[0] + 1) << 8;
		value += (UINT32)cpu_memoryread(paddr[1]) << 24;
		break;

	case 2:
		value = cpu_memoryread_w(paddr[0]);
		value += (UINT32)cpu_memoryread_w(paddr[1]) << 16;
		break;

	case 1:
		value = cpu_memoryread(paddr[0]);
		value += (UINT32)cpu_memoryread_w(paddr[1]) << 8;
		value += (UINT32)cpu_memoryread(paddr[1] + 2) << 24;
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

	paddr[0] = paging(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value))
		return cpu_memoryread_q(paddr[0]);

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
	UINT i, j;

	paddr[0] = paging(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value))
		return cpu_memoryread_f(paddr[0]);

	paddr[1] = paging(laddr + remain, ucrw);
	for (i = 0; i < remain; ++i) {
		value.b[i] = cpu_memoryread(paddr[0] + i);
	}
	for (j = 0; i < 10; ++i, ++j) {
		value.b[i] = cpu_memoryread(paddr[1] + j);
	}
	return value;
}

/* write */
void MEMCALL
cpu_linear_memory_write_b(UINT32 laddr, UINT8 value, int ucrw)
{

	cpu_memorywrite(paging(laddr, ucrw), value);
}

void MEMCALL
cpu_linear_memory_write_w(UINT32 laddr, UINT16 value, int ucrw)
{
	UINT32 paddr[2];

	paddr[0] = paging(laddr, ucrw);
	if ((laddr + 1) & CPU_PAGE_MASK) {
		cpu_memorywrite_w(paddr[0], value);
		return;
	}

	paddr[1] = paging(laddr + 1, ucrw);
	cpu_memorywrite(paddr[0], (UINT8)value);
	cpu_memorywrite(paddr[1], (UINT8)(value >> 8));
}

void MEMCALL
cpu_linear_memory_write_d(UINT32 laddr, UINT32 value, int ucrw)
{
	UINT32 paddr[2];
	UINT remain;

	paddr[0] = paging(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value)) {
		cpu_memorywrite_d(paddr[0], value);
		return;
	}

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

	paddr[0] = paging(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value)) {
		cpu_memorywrite_q(paddr[0], value);
		return;
	}

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
	UINT i, j;

	paddr[0] = paging(laddr, ucrw);
	remain = CPU_PAGE_SIZE - (laddr & CPU_PAGE_MASK);
	if (remain >= sizeof(value)) {
		cpu_memorywrite_f(paddr[0], value);
		return;
	}

	paddr[1] = paging(laddr + remain, ucrw);
	for (i = 0; i < remain; ++i) {
		cpu_memorywrite(paddr[0] + i, value->b[i]);
	}
	for (j = 0; i < 10; ++i, ++j) {
		cpu_memorywrite(paddr[1] + j, value->b[i]);
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
	UINT32 paddr;		/* physical address */
	UINT32 pde_addr;	/* page directory entry address */
	UINT32 pde;		/* page directory entry */
	UINT32 pte_addr;	/* page table entry address */
	UINT32 pte;		/* page table entry */
	UINT bit;
	UINT err;

	ep = tlb_lookup(laddr, ucrw);
	if (ep != NULL)
		return ep->paddr + (laddr & CPU_PAGE_MASK);

	pde_addr = CPU_STAT_PDE_BASE + ((laddr >> 20) & 0xffc);
	pde = cpu_memoryread_d(pde_addr);
	if (!(pde & CPU_PDE_PRESENT)) {
		VERBOSE(("paging: PTE page is not present"));
		VERBOSE(("paging: CPU_CR3 = 0x%08x", CPU_CR3));
		VERBOSE(("paging: laddr = 0x%08x, pde_addr = 0x%08x, pde = 0x%08x", laddr, pde_addr, pde));
		err = 0;
		goto pf_exception;
	}
	if (!(pde & CPU_PDE_ACCESS)) {
		pde |= CPU_PDE_ACCESS;
		cpu_memorywrite_d(pde_addr, pde);
	}

	pte_addr = (pde & CPU_PDE_BASEADDR_MASK) + ((laddr >> 10) & 0xffc);
	pte = cpu_memoryread_d(pte_addr);
	if (!(pte & CPU_PTE_PRESENT)) {
		VERBOSE(("paging: page is not present"));
		VERBOSE(("paging: laddr = 0x%08x, pde_addr = 0x%08x, pde = 0x%08x", laddr, pde_addr, pde));
		VERBOSE(("paging: pte_addr = 0x%08x, pte = 0x%08x", pte_addr, pte));
		err = 0;
		goto pf_exception;
	}
	if (!(pte & CPU_PTE_ACCESS)) {
		pte |= CPU_PTE_ACCESS;
		cpu_memorywrite_d(pte_addr, pte);
	}

	/* make physical address */
	paddr = (pte & CPU_PTE_BASEADDR_MASK) + (laddr & CPU_PAGE_MASK);

	bit  = ucrw & (CPU_PAGE_WRITE|CPU_PAGE_USER_MODE);
	bit |= (pde & pte & (CPU_PTE_WRITABLE|CPU_PTE_USER_MODE));
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
		cpu_memorywrite_d(pte_addr, pte);
	}

	tlb_update(laddr, pte, (bit & (CPU_PTE_WRITABLE|CPU_PTE_USER_MODE)) + ((ucrw & CPU_PAGE_CODE) ? 1 : 0));

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
#if (CPU_FEATURES & CPU_FEATURE_PGE) == CPU_FEATURE_PGE
#define	TLB_IS_GLOBAL(ep)	((ep)->tag & TLB_ENTRY_TAG_GLOBAL)
#else
#define	TLB_IS_GLOBAL(ep)	0
#endif

#define	TLB_SET_TAG_FLAGS(ep, entry, bit) \
do { \
	(ep)->tag |= (entry) & (CPU_PTE_GLOBAL_PAGE|CPU_PTE_DIRTY); \
	(ep)->tag |= (bit) & (CPU_PTE_WRITABLE|CPU_PTE_USER_MODE); \
} while (/*CONSTCOND*/ 0)

#define	NTLB		2	/* 0: DTLB, 1: ITLB */
#define	NENTRY		(1 << 6)
#define	TLB_ENTRY_SHIFT	12
#define	TLB_ENTRY_MASK	(NENTRY - 1)

typedef struct {
	struct tlb_entry entry[NENTRY];
} tlb_t;
static tlb_t tlb[NTLB];

void
tlb_init(void)
{

	memset(tlb, 0, sizeof(tlb));
}

void MEMCALL
tlb_flush(BOOL allflush)
{
	struct tlb_entry *ep;
	int i;
	int n;

	if (allflush) {
		tlb_init();
		return;
	}

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
tlb_flush_page(UINT32 laddr)
{
	struct tlb_entry *ep;
	int idx;
	int n;

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
}

static void MEMCALL
tlb_update(UINT32 laddr, UINT entry, int bit)
{
	struct tlb_entry *ep;
	int idx;
	int n;

	n = bit & 1;
	idx = (laddr >> TLB_ENTRY_SHIFT) & TLB_ENTRY_MASK;
	ep = &tlb[n].entry[idx];

	TLB_SET_VALID(ep);
	TLB_SET_TAG_ADDR(ep, laddr);
	TLB_SET_TAG_FLAGS(ep, entry, bit);
	ep->paddr = entry & CPU_PTE_BASEADDR_MASK;
}
