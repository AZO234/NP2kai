#include	<compiler.h>

#if 0
#undef	TRACEOUT
//#define USE_TRACEOUT_VS
//#define MEM_BDA_TRACEOUT
//#define MEM_D8_TRACEOUT
#ifdef USE_TRACEOUT_VS
static void trace_fmt_ex(const char *fmt, ...)
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
#else
#define	TRACEOUT(s)	(void)(s)
#endif
#endif	/* 1 */

#ifndef NP2_MEMORY_ASM

#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<mem/memtram.h>
#include	<mem/memvram.h>
#include	<mem/memegc.h>
#if defined(SUPPORT_PC9821)
#include	<mem/memvga.h>
#endif
#include	<mem/memems.h>
#include	<mem/memepp.h>
#include	<vram/vram.h>
#include	<font/font.h>
#if defined(SUPPORT_CL_GD5430)
#include	<wab/cirrus_vga_extern.h>
#endif
#if defined(SUPPORT_WAB_NPDISP)
#include	"wab/npdisp.h"
#endif
#if defined(SUPPORT_WAB_GA1280A)
#include	"wab/ga1280a.h"
#endif
#if defined(SUPPORT_PCI)
#include	<bios/bios.h>
#endif
#if defined(SUPPORT_IA32_HAXM)
#include	<i386hax/haxfunc.h>
#include	<i386hax/haxcore.h>
#endif


#if defined(SUPPORT_IA32_HAXM)
	UINT8	*mem = NULL; // Alloc in pccore_mem_malloc()
#else
	UINT8	mem[0x200000];
#endif


/*
 * -- Fast-path MMIO guard table --
 *
 * The fast CPU memory path may directly access mem or CPU_EXTMEMBASE. 
 * Devices whose MMIO/VRAM windows overlap that address space must mark the corresponding blocks here.
 *
 * Granularity:
 *   00000000-000fffff : 4KB blocks
 *   00100000-00ffffff : 64KB blocks
 *   01000000-ffffffff : 1MB blocks
 * 
 * -- 高速処理用MMIOアクセステーブル --
 * 
 * MMIOやバンクメモリなど少しでも特殊アクセスされる可能性があるメモリ領域を登録する。
 * 普通のメモリ領域が登録されていても低速版へフォールバックするのでOK。
 * 素直なアクセスができるメモリ領域に高速アクセスできる方が全体として速くなる。
 * 
 * メモリ範囲粒度:
 *   00000000-000fffff : 4KB blocks（現状I286_MEMWRITEMAX以上は常時低速アクセス扱いなので使用しない）
 *   00100000-00ffffff : 64KB blocks
 *   01000000-ffffffff : 1MB blocks
 */
#define MEMP_FASTMMIO_LOW_LIMIT      0x00100000UL
#define MEMP_FASTMMIO_MID_LIMIT      0x01000000UL
#define MEMP_FASTMMIO_LOW_SHIFT      12
#define MEMP_FASTMMIO_MID_SHIFT      16
#define MEMP_FASTMMIO_HIGH_SHIFT     20
#define MEMP_FASTMMIO_LOW_COUNT      (MEMP_FASTMMIO_LOW_LIMIT >> MEMP_FASTMMIO_LOW_SHIFT)
#define MEMP_FASTMMIO_MID_COUNT      ((MEMP_FASTMMIO_MID_LIMIT - MEMP_FASTMMIO_LOW_LIMIT) >> MEMP_FASTMMIO_MID_SHIFT)
#define MEMP_FASTMMIO_HIGH_COUNT     0x1000

#if defined(_MSC_VER)
#define MEMP_FASTMMIO_INLINE static __forceinline
#elif defined(__GNUC__)
#define MEMP_FASTMMIO_INLINE static __inline__ __attribute__((always_inline))
#else
#define MEMP_FASTMMIO_INLINE static
#endif

#ifdef _MSC_VER
#define MEMP_ALIGN_CACHE __declspec(align(64))
#else
#define MEMP_ALIGN_CACHE __attribute__((aligned(64)))
#endif

//static MEMP_ALIGN_CACHE unsigned int memp_fastmmio_low[MEMP_FASTMMIO_LOW_COUNT];
static MEMP_ALIGN_CACHE unsigned int memp_fastmmio_mid[MEMP_FASTMMIO_MID_COUNT];
static MEMP_ALIGN_CACHE unsigned int memp_fastmmio_high[MEMP_FASTMMIO_HIGH_COUNT];

MEMP_FASTMMIO_INLINE UINT32 memp_fastmmio_block_end(UINT32 address)
{
	if (address < MEMP_FASTMMIO_LOW_LIMIT) {
		return address | ((1UL << MEMP_FASTMMIO_LOW_SHIFT) - 1);
	}
	if (address < MEMP_FASTMMIO_MID_LIMIT) {
		return address | ((1UL << MEMP_FASTMMIO_MID_SHIFT) - 1);
	}
	return address | ((1UL << MEMP_FASTMMIO_HIGH_SHIFT) - 1);
}

// 指定アドレスが直接アクセス可能かどうか調べる　OK=0、不可=0以外
MEMP_FASTMMIO_INLINE int memp_fastmmio_addr_is_marked(UINT32 address)
{
	if (address < MEMP_FASTMMIO_LOW_LIMIT) {
		return 1; // 常時低速アクセスとする I286_MEMWRITEMAX未満はここではなく別途判定される
	}
	if (address < MEMP_FASTMMIO_MID_LIMIT) {
		return memp_fastmmio_mid[(address - MEMP_FASTMMIO_LOW_LIMIT) >> MEMP_FASTMMIO_MID_SHIFT] != 0;
	}
	return memp_fastmmio_high[address >> MEMP_FASTMMIO_HIGH_SHIFT] != 0;
}

// 指定アドレス範囲が直接アクセス可能かどうか調べる　OK=0、不可=0以外
MEMP_FASTMMIO_INLINE int memp_fastmmio_range_is_marked(UINT32 address, int size)
{
	// XXX: 開始アドレスしか見ていないので途中からMMIOアドレスに入るとおかしくなるがそんな変なアクセスはしないと信じる → 残念ながらありました
	//return memp_fastmmio_addr_is_marked(address);

	// 厳密に判定したければこっち
	return memp_fastmmio_addr_is_marked(address) || memp_fastmmio_addr_is_marked(address + size - 1);
}

// MMIOアクセステーブル　インクリメント
static void memp_fastmmio_block_inc(UINT32 address)
{
	UINT idx;

	if (address < MEMP_FASTMMIO_LOW_LIMIT) {
		// 特殊判定されるので登録不要
		//idx = (UINT)(address >> MEMP_FASTMMIO_LOW_SHIFT);
		//if (memp_fastmmio_low[idx] != 0xff) memp_fastmmio_low[idx]++;
		return;
	}
	if (address < MEMP_FASTMMIO_MID_LIMIT) {
		idx = (UINT)((address - MEMP_FASTMMIO_LOW_LIMIT) >> MEMP_FASTMMIO_MID_SHIFT);
		if (memp_fastmmio_mid[idx] != UINT_MAX) memp_fastmmio_mid[idx]++;
		return;
	}
	idx = (UINT)(address >> MEMP_FASTMMIO_HIGH_SHIFT);
	if (memp_fastmmio_high[idx] != UINT_MAX) memp_fastmmio_high[idx]++;
}
// MMIOアクセステーブル　デクリメント
static void memp_fastmmio_block_dec(UINT32 address)
{
	UINT idx;

	if (address < MEMP_FASTMMIO_LOW_LIMIT) {
		// 特殊判定されるので登録不要
		//idx = (UINT)(address >> MEMP_FASTMMIO_LOW_SHIFT);
		//if (memp_fastmmio_low[idx] != 0) memp_fastmmio_low[idx]--;
		return;
	}
	if (address < MEMP_FASTMMIO_MID_LIMIT) {
		idx = (UINT)((address - MEMP_FASTMMIO_LOW_LIMIT) >> MEMP_FASTMMIO_MID_SHIFT);
		if (memp_fastmmio_mid[idx] != 0) memp_fastmmio_mid[idx]--;
		return;
	}
	idx = (UINT)(address >> MEMP_FASTMMIO_HIGH_SHIFT);
	if (memp_fastmmio_high[idx] != 0) memp_fastmmio_high[idx]--;
}
// MMIOアクセスマップに直接アクセス不可領域を登録　カウンタ管理なので重複登録されてもよい
void MEMCALL memp_mmio_range_add(UINT32 address, UINT32 leng)
{
	UINT32 end;
	UINT32 bend;

	if (!leng) return;
	end = address + leng - 1;
	if (end < address) end = 0xffffffffUL;
	for (;;) {
		memp_fastmmio_block_inc(address);
		bend = memp_fastmmio_block_end(address);
		if (bend >= end || bend == 0xffffffffUL) break;
		address = bend + 1;
	}
	tlb_flush_all(); // メモリ直接アクセス可能かが変わり得るのでTLBクリア
}
// MMIOアクセスマップから直接アクセス不可領域を削除
void MEMCALL memp_mmio_range_remove(UINT32 address, UINT32 leng)
{
	UINT32 end;
	UINT32 bend;

	if (!leng) return;
	end = address + leng - 1;
	if (end < address) end = 0xffffffffUL;
	for (;;) {
		memp_fastmmio_block_dec(address);
		bend = memp_fastmmio_block_end(address);
		if (bend >= end || bend == 0xffffffffUL) break;
		address = bend + 1;
	}
	tlb_flush_all(); // メモリ直接アクセス可能かが変わり得るのでTLBクリア
}
// MMIOアクセスマップ初期化　ついでに怪しい領域は先に登録しておく
void MEMCALL memp_mmio_map_reset(void)
{
	//ZeroMemory(memp_fastmmio_low, sizeof(memp_fastmmio_low));
	ZeroMemory(memp_fastmmio_mid, sizeof(memp_fastmmio_mid));
	ZeroMemory(memp_fastmmio_high, sizeof(memp_fastmmio_high));

	// 駄目な可能性がある範囲は事前登録
	memp_mmio_range_add(I286_MEMWRITEMAX, USE_HIMEM - I286_MEMWRITEMAX); // I286_MEMWRITEMAX 〜 USE_HIMEM
	memp_mmio_range_add(I286_MEMREADMAX, USE_HIMEM - I286_MEMREADMAX); // I286_MEMREADMAX 〜 USE_HIMEM
	if (CPU_EXTLIMIT16 > USE_HIMEM) {
		if (0x00f00000 > CPU_EXTLIMIT16) {
			memp_mmio_range_add(CPU_EXTLIMIT16, 0x00f00000 - CPU_EXTLIMIT16); // 0x00f00000 〜 CPU_EXTLIMIT16
		}
		memp_mmio_range_add(0x00f00000, 0x01000000 - 0x00f00000);
	}
#if defined(CPU_EXTLIMIT)
	if (CPU_EXTLIMIT > 0) {
		memp_mmio_range_add(CPU_EXTLIMIT, 0 - CPU_EXTLIMIT); // メモリがない領域
	}
	else
#endif
	if (CPU_EXTLIMIT16 > 0) {
		memp_mmio_range_add(CPU_EXTLIMIT16, 0 - CPU_EXTLIMIT16); // メモリがない領域
	}
	else {
		memp_mmio_range_add(USE_HIMEM, 0 - USE_HIMEM); // メモリがない領域
	}
	memp_mmio_range_add(0xfff00000, 0xfff80000 - 0xfff00000); // 256色モード
}

// ページング時にメモリ直接アクセス可能かどうか調べて可能ならポインタを返す。不可ならNULLを返す。
// 複雑な判定は止めて少しでも怪しければNULLを返す
UINT8 * MEMCALL memp_get_direct_host_page(UINT32 address)
{
	if (address < I286_MEMWRITEMAX && address + CPU_PAGE_SIZE < I286_MEMWRITEMAX) { 
		return mem + address; // I286_MEMWRITEMAX未満は安全。I286_MEMREADMAXよりI286_MEMWRITEMAXの方が小さいので小さい方で判定
	}
	address &= CPU_ADRSMASK;
	address &= 0xfffff000UL;

	// NG領域を事前判定
	if (CPU_EXTMEMBASE == NULL) {
		return NULL; // 拡張メモリ無しは不可
	}
	if (address < USE_HIMEM) {
		return NULL; // HIMEM未満は複雑なので全部不可とする
	}
	if (address >= CPU_EXTLIMIT16) { // 16MB以内メモリ領域を超えていたら追加判定
		if (address < 0x00100000UL) {
			return NULL; // CPU_EXTLIMIT16以上0x100000未満はメモリ無しなので不可。16MB超えのメモリを積んでいたらCPU_EXTLIMIT16==0x100000なのでここには来ない
		}
		if ((address + CPU_PAGE_SIZE) < address) {
			return NULL; // ページサイズを足してオーバーフローするメモリアドレスは不可
		}
#if defined(CPU_EXTLIMIT)
		if ((address + CPU_PAGE_SIZE) > CPU_EXTLIMIT) {
			return NULL; // 全メモリ容量超えの領域も不可とする
		}
#endif
	}
	// 高速MMIOアクセステーブルで判定
	if (memp_fastmmio_addr_is_marked(address) ||
	    memp_fastmmio_addr_is_marked(address + (CPU_PAGE_SIZE - 1))) {
		return NULL;
	}
	return CPU_EXTMEMBASE + address;
}


typedef void (MEMCALL * MEM8WRITE)(UINT32 address, REG8 value);
typedef REG8 (MEMCALL * MEM8READ)(UINT32 address);
typedef void (MEMCALL * MEM16WRITE)(UINT32 address, REG16 value);
typedef REG16 (MEMCALL * MEM16READ)(UINT32 address);
typedef void (MEMCALL * MEM32WRITE)(UINT32 address, UINT32 value);
typedef UINT32 (MEMCALL * MEM32READ)(UINT32 address);


// ---- MAIN

static REG8 MEMCALL memmain_rd8(UINT32 address) {

	return(mem[address]);
}

static REG16 MEMCALL memmain_rd16(UINT32 address) {

const UINT8	*ptr;

	ptr = mem + address;
	return(LOADINTELWORD(ptr));
}

static UINT32 MEMCALL memmain_rd32(UINT32 address) {

const UINT8	*ptr;

	ptr = mem + address;
	return(LOADINTELDWORD(ptr));
}

static void MEMCALL memmain_wr8(UINT32 address, REG8 value) {

	mem[address] = (UINT8)value;
}

static void MEMCALL memmain_wr16(UINT32 address, REG16 value) {

	UINT8	*ptr;

	ptr = mem + address;
	STOREINTELWORD(ptr, value);
}

static void MEMCALL memmain_wr32(UINT32 address, UINT32 value) {

	UINT8	*ptr;

	ptr = mem + address;
	STOREINTELDWORD(ptr, value);
}


// ---- N/C

static REG8 MEMCALL memnc_rd8(UINT32 address) {

	(void)address;
	return(0xff);
}

static REG16 MEMCALL memnc_rd16(UINT32 address) {

	(void)address;
	return(0xffff);
}

static UINT32 MEMCALL memnc_rd32(UINT32 address) {

	(void)address;
	return(0xffffffff);
}

static void MEMCALL memnc_wr8(UINT32 address, REG8 value) {

	(void)address;
	(void)value;
}

static void MEMCALL memnc_wr16(UINT32 address, REG16 value) {

	(void)address;
	(void)value;
}

static void MEMCALL memnc_wr32(UINT32 address, UINT32 value) {

	(void)address;
	(void)value;
}


// ---- memory 000000-0ffffff + 64KB

typedef struct {
	MEM8READ	rd8[0x22];
	MEM8WRITE	wr8[0x22];
	MEM16READ	rd16[0x22];
	MEM16WRITE	wr16[0x22];
	MEM32READ	rd32[0x22];
	MEM32WRITE	wr32[0x22];
} MEMFN0;

typedef struct {
	MEM8READ	brd8;		// E8000-F7FFF byte read
	MEM8READ	ird8;		// F8000-FFFFF byte read
	MEM8WRITE	bwr8;		// E8000-FFFFF byte write
	MEM16READ	brd16;		// E8000-F7FFF word read
	MEM16READ	ird16;		// F8000-FFFFF word read
	MEM16WRITE	bwr16;		// F8000-FFFFF word write
	MEM32READ	brd32;		// E8000-F7FFF word read
	MEM32READ	ird32;		// F8000-FFFFF word read
	MEM32WRITE	bwr32;		// F8000-FFFFF word write
} MMAPTBL;

typedef struct {
	MEM8READ	rd8;
	MEM8WRITE	wr8;
	MEM16READ	rd16;
	MEM16WRITE	wr16;
	MEM32READ	rd32;
	MEM32WRITE	wr32;
} VACCTBL;

static MEMFN0 memfn0 = {
	   {memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 00
		memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 20
		memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 40
		memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 60
		memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 80
		memtram_rd8,	memvram0_rd8,	memvram0_rd8,	memvram0_rd8,	// a0
		memems_rd8,		memems_rd8,		memmain_rd8,	memmain_rd8,	// c0
		memvram0_rd8,	memmain_rd8,	memmain_rd8,	memf800_rd8,	// e0
		memmain_rd8,	memmain_rd8},

	   {memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 00
		memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 20
		memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 40
		memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 60
		memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 80
		memtram_wr8,	memvram0_wr8,	memvram0_wr8,	memvram0_wr8,	// a0
		memems_wr8,		memems_wr8,		memd000_wr8,	memd000_wr8,	// c0
		memvram0_wr8,	memnc_wr8,		memnc_wr8,		memnc_wr8,		// e0
		memmain_wr8,	memmain_wr8},

	   {memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 00
		memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 20
		memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 40
		memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 60
		memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 80
		memtram_rd16,	memvram0_rd16,	memvram0_rd16,	memvram0_rd16,	// a0
		memems_rd16,	memems_rd16,	memmain_rd16,	memmain_rd16,	// c0
		memvram0_rd16,	memmain_rd16,	memmain_rd16,	memf800_rd16,	// e0
		memmain_rd16,	memmain_rd16},

	   {memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 00
		memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 20
		memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 40
		memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 60
		memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 80
		memtram_wr16,	memvram0_wr16,	memvram0_wr16,	memvram0_wr16,	// a0
		memems_wr16,	memems_wr16,	memd000_wr16,	memd000_wr16,	// c0
		memvram0_wr16,	memnc_wr16,		memnc_wr16,		memnc_wr16,		// e0
		memmain_wr16,	memmain_wr16},

	   {memmain_rd32,	memmain_rd32,	memmain_rd32,	memmain_rd32,	// 00
		memmain_rd32,	memmain_rd32,	memmain_rd32,	memmain_rd32,	// 20
		memmain_rd32,	memmain_rd32,	memmain_rd32,	memmain_rd32,	// 40
		memmain_rd32,	memmain_rd32,	memmain_rd32,	memmain_rd32,	// 60
		memmain_rd32,	memmain_rd32,	memmain_rd32,	memmain_rd32,	// 80
		memtram_rd32,	memvram0_rd32,	memvram0_rd32,	memvram0_rd32,	// a0
		memems_rd32,	memems_rd32,	memmain_rd32,	memmain_rd32,	// c0
		memvram0_rd32,	memmain_rd32,	memmain_rd32,	memf800_rd32,	// e0
		memmain_rd32,	memmain_rd32},

	   {memmain_wr32,	memmain_wr32,	memmain_wr32,	memmain_wr32,	// 00
		memmain_wr32,	memmain_wr32,	memmain_wr32,	memmain_wr32,	// 20
		memmain_wr32,	memmain_wr32,	memmain_wr32,	memmain_wr32,	// 40
		memmain_wr32,	memmain_wr32,	memmain_wr32,	memmain_wr32,	// 60
		memmain_wr32,	memmain_wr32,	memmain_wr32,	memmain_wr32,	// 80
		memtram_wr32,	memvram0_wr32,	memvram0_wr32,	memvram0_wr32,	// a0
		memems_wr32,	memems_wr32,	memd000_wr32,	memd000_wr32,	// c0
		memvram0_wr32,	memnc_wr32,		memnc_wr32,		memnc_wr32,		// e0
		memmain_wr32,	memmain_wr32}};

static const MMAPTBL mmaptbl[2] = {
		   {memmain_rd8,	memf800_rd8,	memnc_wr8,
			memmain_rd16,	memf800_rd16,	memnc_wr16,
			memmain_rd32,	memf800_rd32,	memnc_wr32},
		   {memf800_rd8,	memf800_rd8,	memepson_wr8,
			memf800_rd16,	memf800_rd16,	memepson_wr16,
			memf800_rd32,	memf800_rd32,	memepson_wr32}};

static const VACCTBL vacctbl[0x10] = {
		{memvram0_rd8,	memvram0_wr8,	memvram0_rd16,	memvram0_wr16,	memvram0_rd32,	memvram0_wr32,	},	// 00
		{memvram1_rd8,	memvram1_wr8,	memvram1_rd16,	memvram1_wr16,	memvram1_rd32,	memvram1_wr32,	},
		{memvram0_rd8,	memvram0_wr8,	memvram0_rd16,	memvram0_wr16,	memvram0_rd32,	memvram0_wr32,	},
		{memvram1_rd8,	memvram1_wr8,	memvram1_rd16,	memvram1_wr16,	memvram1_rd32,	memvram1_wr32,	},
		{memvram0_rd8,	memvram0_wr8,	memvram0_rd16,	memvram0_wr16,	memvram0_rd32,	memvram0_wr32,	},	// 40
		{memvram1_rd8,	memvram1_wr8,	memvram1_rd16,	memvram1_wr16,	memvram1_rd32,	memvram1_wr32,	},
		{memvram0_rd8,	memvram0_wr8,	memvram0_rd16,	memvram0_wr16,	memvram0_rd32,	memvram0_wr32,	},
		{memvram1_rd8,	memvram1_wr8,	memvram1_rd16,	memvram1_wr16,	memvram1_rd32,	memvram1_wr32,	},
		{memtcr0_rd8,	memtdw0_wr8,	memtcr0_rd16,	memtdw0_wr16,	memtcr0_rd32,	memtdw0_wr32,	},	// 80
		{memtcr1_rd8,	memtdw1_wr8,	memtcr1_rd16,	memtdw1_wr16,	memtcr1_rd32,	memtdw1_wr32,	},
		{memegc_rd8,	memegc_wr8,		memegc_rd16,	memegc_wr16,	memegc_rd32,	memegc_wr32,	},
		{memegc_rd8,	memegc_wr8,		memegc_rd16,	memegc_wr16,	memegc_rd32,	memegc_wr32,	},
		{memvram0_rd8,	memrmw0_wr8,	memvram0_rd16,	memrmw0_wr16,	memvram0_rd32,	memrmw0_wr32,	},	// c0
		{memvram1_rd8,	memrmw1_wr8,	memvram1_rd16,	memrmw1_wr16,	memvram1_rd32,	memrmw1_wr32,	},
		{memegc_rd8,	memegc_wr8,		memegc_rd16,	memegc_wr16,	memegc_rd32,	memegc_wr32,	},
		{memegc_rd8,	memegc_wr8,		memegc_rd16,	memegc_wr16,	memegc_rd32,	memegc_wr32,	}};


void MEMCALL memm_arch(UINT type) {

const MMAPTBL	*mm;

	mm = mmaptbl + (type & 1);

	memfn0.rd8[0xe8000 >> 15] = mm->brd8;
	memfn0.rd8[0xf0000 >> 15] = mm->brd8;
	memfn0.rd8[0xf8000 >> 15] = mm->ird8;
	memfn0.wr8[0xe8000 >> 15] = mm->bwr8;
	memfn0.wr8[0xf0000 >> 15] = mm->bwr8;
	memfn0.wr8[0xf8000 >> 15] = mm->bwr8;

	memfn0.rd16[0xe8000 >> 15] = mm->brd16;
	memfn0.rd16[0xf0000 >> 15] = mm->brd16;
	memfn0.rd16[0xf8000 >> 15] = mm->ird16;
	memfn0.wr16[0xe8000 >> 15] = mm->bwr16;
	memfn0.wr16[0xf0000 >> 15] = mm->bwr16;
	memfn0.wr16[0xf8000 >> 15] = mm->bwr16;

	memfn0.rd32[0xe8000 >> 15] = mm->brd32;
	memfn0.rd32[0xf0000 >> 15] = mm->brd32;
	memfn0.rd32[0xf8000 >> 15] = mm->ird32;
	memfn0.wr32[0xe8000 >> 15] = mm->bwr32;
	memfn0.wr32[0xf0000 >> 15] = mm->bwr32;
	memfn0.wr32[0xf8000 >> 15] = mm->bwr32;
}

void MEMCALL memm_vram(UINT func) {

const VACCTBL	*vacc;

#if defined(SUPPORT_PC9821)
	if (!(func & 0x20)) {
#endif	// defined(SUPPORT_PC9821)
		vacc = vacctbl + (func & 0x0f);
#if defined(SUPPORT_IA32_HAXM)
		if (np2hax.enable) {
			if ((func & 0x0f) < 8) {
				if(np2haxcore.lastVRAMMMIO){
					i386hax_vm_setmemoryarea(mem+0xA8000, 0xA8000, 0x8000);
					i386hax_vm_setmemoryarea(mem+0xB0000, 0xB0000, 0x10000);
					np2haxcore.lastVRAMMMIO = 0;
				}
			}else{
				if(!np2haxcore.lastVRAMMMIO){
					i386hax_vm_removememoryarea(mem+0xA8000, 0xA8000, 0x8000);
					i386hax_vm_removememoryarea(mem+0xB0000, 0xB0000, 0x10000);
					np2haxcore.lastVRAMMMIO = 1;
				}
			}
		}
#endif

		memfn0.rd8[0xa8000 >> 15] = vacc->rd8;
		memfn0.rd8[0xb0000 >> 15] = vacc->rd8;
		memfn0.rd8[0xb8000 >> 15] = vacc->rd8;
		memfn0.rd8[0xe0000 >> 15] = vacc->rd8;

		memfn0.wr8[0xa8000 >> 15] = vacc->wr8;
		memfn0.wr8[0xb0000 >> 15] = vacc->wr8;
		memfn0.wr8[0xb8000 >> 15] = vacc->wr8;
		memfn0.wr8[0xe0000 >> 15] = vacc->wr8;

		memfn0.rd16[0xa8000 >> 15] = vacc->rd16;
		memfn0.rd16[0xb0000 >> 15] = vacc->rd16;
		memfn0.rd16[0xb8000 >> 15] = vacc->rd16;
		memfn0.rd16[0xe0000 >> 15] = vacc->rd16;

		memfn0.wr16[0xa8000 >> 15] = vacc->wr16;
		memfn0.wr16[0xb0000 >> 15] = vacc->wr16;
		memfn0.wr16[0xb8000 >> 15] = vacc->wr16;
		memfn0.wr16[0xe0000 >> 15] = vacc->wr16;
		
		memfn0.rd32[0xa8000 >> 15] = vacc->rd32;
		memfn0.rd32[0xb0000 >> 15] = vacc->rd32;
		memfn0.rd32[0xb8000 >> 15] = vacc->rd32;
		memfn0.rd32[0xe0000 >> 15] = vacc->rd32;

		memfn0.wr32[0xa8000 >> 15] = vacc->wr32;
		memfn0.wr32[0xb0000 >> 15] = vacc->wr32;
		memfn0.wr32[0xb8000 >> 15] = vacc->wr32;
		memfn0.wr32[0xe0000 >> 15] = vacc->wr32;

		if (!(func & (1 << VOPBIT_ANALOG))) {					// digital
			memfn0.rd8[0xe0000 >> 15] = memnc_rd8;
			memfn0.wr8[0xe0000 >> 15] = memnc_wr8;
			memfn0.rd16[0xe0000 >> 15] = memnc_rd16;
			memfn0.wr16[0xe0000 >> 15] = memnc_wr16;
			memfn0.rd32[0xe0000 >> 15] = memnc_rd32;
			memfn0.wr32[0xe0000 >> 15] = memnc_wr32;
		}
#if defined(SUPPORT_PC9821)
	}
	else {
#if defined(SUPPORT_IA32_HAXM)
		if (np2hax.enable) {
			if(!np2haxcore.lastVRAMMMIO){
				i386hax_vm_removememoryarea(mem+0xA8000, 0xA8000, 0x8000);
				i386hax_vm_removememoryarea(mem+0xB0000, 0xB0000, 0x10000);
				np2haxcore.lastVRAMMMIO = 1;
			}
		}
#endif

		memfn0.rd8[0xa8000 >> 15] = memvga0_rd8;
		memfn0.rd8[0xb0000 >> 15] = memvga1_rd8;
		memfn0.rd8[0xb8000 >> 15] = memnc_rd8;
		memfn0.rd8[0xe0000 >> 15] = memvgaio_rd8;

		memfn0.wr8[0xa8000 >> 15] = memvga0_wr8;
		memfn0.wr8[0xb0000 >> 15] = memvga1_wr8;
		memfn0.wr8[0xb8000 >> 15] = memnc_wr8;
		memfn0.wr8[0xe0000 >> 15] = memvgaio_wr8;

		memfn0.rd16[0xa8000 >> 15] = memvga0_rd16;
		memfn0.rd16[0xb0000 >> 15] = memvga1_rd16;
		memfn0.rd16[0xb8000 >> 15] = memnc_rd16;
		memfn0.rd16[0xe0000 >> 15] = memvgaio_rd16;

		memfn0.wr16[0xa8000 >> 15] = memvga0_wr16;
		memfn0.wr16[0xb0000 >> 15] = memvga1_wr16;
		memfn0.wr16[0xb8000 >> 15] = memnc_wr16;
		memfn0.wr16[0xe0000 >> 15] = memvgaio_wr16;

		memfn0.rd32[0xa8000 >> 15] = memvga0_rd32;
		memfn0.rd32[0xb0000 >> 15] = memvga1_rd32;
		memfn0.rd32[0xb8000 >> 15] = memnc_rd32;
		memfn0.rd32[0xe0000 >> 15] = memvgaio_rd32;

		memfn0.wr32[0xa8000 >> 15] = memvga0_wr32;
		memfn0.wr32[0xb0000 >> 15] = memvga1_wr32;
		memfn0.wr32[0xb8000 >> 15] = memnc_wr32;
		memfn0.wr32[0xe0000 >> 15] = memvgaio_wr32;
	}
#endif	// defined(SUPPORT_PC9821)
}


// ---- memory f00000-fffffff

typedef struct {
	MEM8READ	rd8[8];
	MEM8WRITE	wr8[8];
	MEM16READ	rd16[8];
	MEM16WRITE	wr16[8];
	MEM32READ	rd32[8];
	MEM32WRITE	wr32[8];
} MEMFNF;


static REG8 MEMCALL memsys_rd8(UINT32 address) {

	address -= 0xf00000;
	return(memfn0.rd8[address >> 15](address));
}

static REG16 MEMCALL memsys_rd16(UINT32 address) {

	address -= 0xf00000;
	return(memfn0.rd16[address >> 15](address));
}

static UINT32 MEMCALL memsys_rd32(UINT32 address) {

	address -= 0xf00000;
	return(memfn0.rd32[address >> 15](address));
}

static void MEMCALL memsys_wr8(UINT32 address, REG8 value) {

	address -= 0xf00000;
	memfn0.wr8[address >> 15](address, value);
}

static void MEMCALL memsys_wr16(UINT32 address, REG16 value) {

	address -= 0xf00000;
	memfn0.wr16[address >> 15](address, value);
}

static void MEMCALL memsys_wr32(UINT32 address, UINT32 value) {

	address -= 0xf00000;
	memfn0.wr32[address >> 15](address, value);
}

#if defined(SUPPORT_PC9821)
static const MEMFNF memfnf = {
	   {memvgaf_rd8,	memvgaf_rd8,	memvgaf_rd8,	memvgaf_rd8,
		memnc_rd8,		memsys_rd8,		memsys_rd8,		memsys_rd8},
	   {memvgaf_wr8,	memvgaf_wr8,	memvgaf_wr8,	memvgaf_wr8,
		memnc_wr8,		memsys_wr8,		memsys_wr8,		memsys_wr8},

	   {memvgaf_rd16,	memvgaf_rd16,	memvgaf_rd16,	memvgaf_rd16,
		memnc_rd16,		memsys_rd16,	memsys_rd16,	memsys_rd16},
	   {memvgaf_wr16,	memvgaf_wr16,	memvgaf_wr16,	memvgaf_wr16,
		memnc_wr16,		memsys_wr16,	memsys_wr16,	memsys_wr16},

	   {memvgaf_rd32,	memvgaf_rd32,	memvgaf_rd32,	memvgaf_rd32,
		memnc_rd32,		memsys_rd32,	memsys_rd32,	memsys_rd32},
	   {memvgaf_wr32,	memvgaf_wr32,	memvgaf_wr32,	memvgaf_wr32,
		memnc_wr32,		memsys_wr32,	memsys_wr32,	memsys_wr32}};
#else
static const MEMFNF memfnf = {
	   {memnc_rd8,		memnc_rd8,		memnc_rd8,		memnc_rd8,
		memnc_rd8,		memsys_rd8,		memsys_rd8,		memsys_rd8},
	   {memnc_wr8,		memnc_wr8,		memnc_wr8,		memnc_wr8,
		memnc_wr8,		memsys_wr8,		memsys_wr8,		memsys_wr8},

	   {memnc_rd16,		memnc_rd16,		memnc_rd16,		memnc_rd16,
		memnc_rd16,		memsys_rd16,	memsys_rd16,	memsys_rd16},
	   {memnc_wr16,		memnc_wr16,		memnc_wr16,		memnc_wr16,
		memnc_wr16,		memsys_wr16,	memsys_wr16,	memsys_wr16},

	   {memvgaf_rd32,	memvgaf_rd32,	memvgaf_rd32,	memvgaf_rd32,
		memnc_rd32,		memsys_rd32,	memsys_rd32,	memsys_rd32},
	   {memvgaf_wr32,	memvgaf_wr32,	memvgaf_wr32,	memvgaf_wr32,
		memnc_wr32,		memsys_wr32,	memsys_wr32,	memsys_wr32}};
#endif

// ----- slow版 fastで処理される部分は省略されているので、fast版からの呼び出しのみ可能

static REG8 MEMCALL memp_read8_slow(UINT32 address) {
#if defined(SUPPORT_WAB_GA1280A)
	{
		REG8 retValue;
		if (ga1280a_memp_try_read8(address, &retValue)) return retValue;
	}
#endif
#if defined(SUPPORT_CL_GD5430)
	if (np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)) {
		UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
		UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
		UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
		if (np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr) {
			UINT32 addrofs = address - np2clvga.pciLFB_Addr;
			if (addrofs < 0x1000000) {
				return cirrus_linear_readb(cirrusvga_opaque, address);
			}
			else if (addrofs < 0x1000000 + 0x400000) {
				return cirrus_linear_bitblt_readb(cirrusvga_opaque, address);
			}
			else {
				return 0xff;
			}
		}
		else if (np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr) {
			if (np2clvga.gd54xxtype == CIRRUS_98ID_PCI || ((np2clvga.pciMMIO_Addr & 0xfff00000) != 0xf00000 || !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))))
				return cirrus_mmio_read[0](cirrusvga_opaque, address);
		}
		if (np2clvga.gd54xxtype != CIRRUS_98ID_PCI) {
			if (vramWndAddr) {
				if (vramWndAddr <= address) {
					if (address < vramWndAddr + VRAMWINDOW_SIZE) {
						return cirrus_linear_readb(cirrusvga_opaque, address);
					}
					else if (address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE) {
						if (address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT)
							return cirrus_linear_readb(cirrusvga_opaque, address);
					}
				}
			}
			if (vramWndAddr3) {
				UINT32 addr3 = address;
				if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX && !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) {
					return CIRRUS_VRAMWND3_FUNC_rb(cirrusvga_opaque, addr3);
				}
			}
			if (vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))) {
				UINT32 addr2 = address;
				if ((vramWndAddr2 & 0xfff00000UL) == 0) {
					UINT32 addrtmp = addr2 & 0xfff80000UL;
					if (addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL) {
						// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
						addr2 = addr2 & 0xfffff;
					}
				}
				if ((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2) {
					return CIRRUS_VRAMWND2_FUNC_rb(cirrusvga_opaque, addr2);
				}
			}
		}
	}
#endif
	address = address & CPU_ADRSMASK;
	if (address < USE_HIMEM) {
		return(memfn0.rd8[address >> 15](address));
	}
	else if (address < CPU_EXTLIMIT16) {
		return(CPU_EXTMEMBASE[address]);
	}
	else if (address < 0x00f00000) {
		return(0xff);
	}
	else if (address < 0x01000000) {
		return(memfnf.rd8[(address >> 17) & 7](address));
	}
#if defined(CPU_EXTLIMIT)
	else if (address < CPU_EXTLIMIT) {
		return(CPU_EXTMEMBASE[address]);
	}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
	else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
		return(memvgaf_rd8(address));
	}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
	else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
		return(npdisp.mm_screenPtr[address - npdisp.mm_vramPhysicalAddr]);
	}
#endif	// defined(SUPPORT_WAB_NPDISP)
	else {
		//			TRACEOUT(("out of mem (read8): %x", address));
		return(0xff);
	}

}

static REG16 MEMCALL memp_read16_slow(UINT32 address) {
	REG16 ret;

	if ((address + 1) & 0x7fff) {			// non 32kb boundary
#if defined(SUPPORT_WAB_GA1280A)
		{
			REG16 retValue;
			if (ga1280a_memp_try_read16(address, &retValue)) return retValue;
		}
#endif
#if defined(SUPPORT_CL_GD5430)
		if (np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)) {
			UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
			UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
			UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
			if (np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr) {
				UINT32 addrofs = address - np2clvga.pciLFB_Addr;
				if (addrofs < 0x1000000) {
					return cirrus_linear_readw(cirrusvga_opaque, address);
				}
				else if (addrofs < 0x1000000 + 0x400000) {
					return cirrus_linear_bitblt_readw(cirrusvga_opaque, address);
				}
				else {
					return 0xffff;
				}
			}
			else if (np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr) {
				if (np2clvga.gd54xxtype == CIRRUS_98ID_PCI || ((np2clvga.pciMMIO_Addr & 0xfff00000) != 0xf00000 || !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))))
					return cirrus_mmio_read[1](cirrusvga_opaque, address);
			}
			if (np2clvga.gd54xxtype != CIRRUS_98ID_PCI) {
				if (vramWndAddr) {
					if (vramWndAddr <= address) {
						if (address < vramWndAddr + VRAMWINDOW_SIZE) {
							return cirrus_linear_readw(cirrusvga_opaque, address);
						}
						else if (address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE) {
							if (address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT)
								return cirrus_linear_readw(cirrusvga_opaque, address);
						}
					}
				}
				if (vramWndAddr3) {
					UINT32 addr3 = address;
					if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX && !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) {
						return CIRRUS_VRAMWND3_FUNC_rw(cirrusvga_opaque, addr3);
					}
				}
				if (vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))) {
					UINT32 addr2 = address;
					if ((vramWndAddr2 & 0xfff00000UL) == 0) {
						UINT32 addrtmp = addr2 & 0xfff80000UL;
						if (addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL) {
							// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
							addr2 = addr2 & 0xfffff;
						}
					}
					if ((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2) {
						return CIRRUS_VRAMWND2_FUNC_rw(cirrusvga_opaque, addr2);
					}
				}
			}
		}
#endif
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			return(memfn0.rd16[address >> 15](address));
		}
		else if (address < CPU_EXTLIMIT16) {
			return(LOADINTELWORD(CPU_EXTMEMBASE + address));
		}
		else if (address < 0x00f00000) {
			return(0xffff);
		}
		else if (address < 0x01000000) {
			return(memfnf.rd16[(address >> 17) & 7](address));
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			return(LOADINTELWORD(CPU_EXTMEMBASE + address));
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			return(memvgaf_rd16(address));
		}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
		else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
			return(LOADINTELWORD(npdisp.mm_screenPtr + (address - npdisp.mm_vramPhysicalAddr)));
		}
#endif	// defined(SUPPORT_WAB_NPDISP)
		else {
			//			TRACEOUT(("out of mem (read16): %x", address));
			return(0xffff);
		}
	}
	else {
		ret = memp_read8_fast(address + 0);
		ret |= (REG16)(memp_read8_fast(address + 1) << 8);
		return(ret);
	}

}

static UINT32 MEMCALL memp_read32_slow(UINT32 address) {
	UINT32 ret;

	if ((address & 0x7fff) <= 0x8000 - 4) {			// non 32kb boundary
#if defined(SUPPORT_WAB_GA1280A)
		{
			UINT32 retValue;
			if (ga1280a_memp_try_read32(address, &retValue)) return retValue;
		}
#endif
#if defined(SUPPORT_CL_GD5430)
		if (np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)) {
			UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
			UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
			UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
			if (np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr) {
				UINT32 addrofs = address - np2clvga.pciLFB_Addr;
				if (addrofs < 0x1000000) {
					return cirrus_linear_readl(cirrusvga_opaque, address);
				}
				else if (addrofs < 0x1000000 + 0x400000) {
					return cirrus_linear_bitblt_readl(cirrusvga_opaque, address);
				}
				else {
					return 0xffffffff;
				}
			}
			else if (np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr) {
				if (np2clvga.gd54xxtype == CIRRUS_98ID_PCI || ((np2clvga.pciMMIO_Addr & 0xfff00000) != 0xf00000 || !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))))
					return cirrus_mmio_read[2](cirrusvga_opaque, address);
			}
			if (np2clvga.gd54xxtype != CIRRUS_98ID_PCI) {
				if (vramWndAddr) {
					if (vramWndAddr <= address) {
						if (address < vramWndAddr + VRAMWINDOW_SIZE) {
							return cirrus_linear_readl(cirrusvga_opaque, address);
						}
						else if (address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE) {
							if (address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT)
								return cirrus_linear_readl(cirrusvga_opaque, address);
						}
					}
				}
				if (vramWndAddr3) {
					UINT32 addr3 = address;
					if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX && !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) {
						return CIRRUS_VRAMWND3_FUNC_rl(cirrusvga_opaque, addr3);
					}
				}
				if (vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))) {
					UINT32 addr2 = address;
					if ((vramWndAddr2 & 0xfff00000UL) == 0) {
						UINT32 addrtmp = addr2 & 0xfff80000UL;
						if (addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL) {
							// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
							addr2 = addr2 & 0xfffff;
						}
					}
					if ((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2) {
						return CIRRUS_VRAMWND2_FUNC_rl(cirrusvga_opaque, addr2);
					}
				}
			}
		}
#endif
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			return(memfn0.rd32[address >> 15](address));
		}
		else if (address < CPU_EXTLIMIT16) {
			return(LOADINTELDWORD(CPU_EXTMEMBASE + address));
		}
		else if (address < 0x00f00000) {
			return(0xffff);
		}
		else if (address < 0x01000000) {
			return(memfnf.rd32[(address >> 17) & 7](address));
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			return(LOADINTELDWORD(CPU_EXTMEMBASE + address));
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			return(memvgaf_rd32(address));
		}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
		else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
			return(LOADINTELDWORD(npdisp.mm_screenPtr + (address - npdisp.mm_vramPhysicalAddr)));
		}
#endif	// defined(SUPPORT_WAB_NPDISP)
		else {
			//			TRACEOUT(("out of mem (read32): %x", address));
			return(0xffffffff);
		}
	}
	else {
		if (!(address & 1)) {
			ret = memp_read16_fast(address + 0);
			ret |= (UINT32)memp_read16_fast(address + 2) << 16;
		}
		else {
			ret = memp_read8_fast(address + 0);
			ret |= (UINT32)memp_read16_fast(address + 1) << 8;
			ret |= (UINT32)memp_read8_fast(address + 3) << 24;
		}
		return(ret);
	}

}

static PF_UINT8 MEMCALL memp_read8_codefetch_slow(UINT32 address) {
	//if(pcidev.bios32entrypoint <= address && address < pcidev.bios32entrypoint + sizeof(pcidev.biosdata.data8)){
	//	printf("BIOS32 (read8): %x");
	//}
	address = address & CPU_ADRSMASK;
	if (address < USE_HIMEM) {
		return(memfn0.rd8[address >> 15](address));
	}
	else if (address < CPU_EXTLIMIT16) {
		return(CPU_EXTMEMBASE[address]);
	}
	else if (address < 0x00f00000) {
		return(0xff);
	}
	else if (address < 0x01000000) {
		return(memfnf.rd8[(address >> 17) & 7](address));
	}
#if defined(CPU_EXTLIMIT)
	else if (address < CPU_EXTLIMIT) {
		return(CPU_EXTMEMBASE[address]);
	}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
	else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
		return(memvgaf_rd8(address));
	}
#endif	// defined(SUPPORT_PC9821)
	else {
		//			TRACEOUT(("out of mem (read8): %x", address));
		return(0xff);
	}

}

static PF_UINT16 MEMCALL memp_read16_codefetch_slow(UINT32 address) {
	PF_UINT16 ret;

	if ((address + 1) & 0x7fff) {			// non 32kb boundary
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			return(memfn0.rd16[address >> 15](address));
		}
		else if (address < CPU_EXTLIMIT16) {
			return(LOADINTELWORD(CPU_EXTMEMBASE + address));
		}
		else if (address < 0x00f00000) {
			return(0xffff);
		}
		else if (address < 0x01000000) {
			return(memfnf.rd16[(address >> 17) & 7](address));
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			return(LOADINTELWORD(CPU_EXTMEMBASE + address));
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			return(memvgaf_rd16(address));
		}
#endif	// defined(SUPPORT_PC9821)
		else {
			//			TRACEOUT(("out of mem (read16): %x", address));
			return(0xffff);
		}
	}
	else {
		ret = memp_read8_fast(address + 0);
		ret |= (UINT32)(memp_read8_fast(address + 1) << 8);
		return(ret);
	}

}

static PF_UINT32 MEMCALL memp_read32_codefetch_slow(UINT32 address) {
	UINT32 ret;

	if ((address & 0x7fff) <= 0x8000 - 4) {			// non 32kb boundary
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			return(memfn0.rd32[address >> 15](address));
		}
		else if (address < CPU_EXTLIMIT16) {
			return(LOADINTELDWORD(CPU_EXTMEMBASE + address));
		}
		else if (address < 0x00f00000) {
			return(0xffff);
		}
		else if (address < 0x01000000) {
			return(memfnf.rd32[(address >> 17) & 7](address));
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			return(LOADINTELDWORD(CPU_EXTMEMBASE + address));
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			return(memvgaf_rd32(address));
		}
#endif	// defined(SUPPORT_PC9821)
		else {
			//			TRACEOUT(("out of mem (read32): %x", address));
			return(0xffffffff);
		}
	}
	else {
		if (!(address & 1)) {
			ret = memp_read16_fast(address + 0);
			ret |= (UINT32)memp_read16_fast(address + 2) << 16;
		}
		else {
			ret = memp_read8_fast(address + 0);
			ret |= (UINT32)memp_read16_fast(address + 1) << 8;
			ret |= (UINT32)memp_read8_fast(address + 3) << 24;
		}
		return(ret);
	}

}

static void MEMCALL memp_write8_slow(UINT32 address, REG8 value) {
#if defined(SUPPORT_WAB_GA1280A)
	if (ga1280a_memp_try_write8(address, value)) return;
#endif
#if defined(SUPPORT_CL_GD5430)
	if (np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)) {
		UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
		UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
		UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
		if (np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr) {
			UINT32 addrofs = address - np2clvga.pciLFB_Addr;
			if (addrofs < 0x1000000) {
				cirrus_linear_writeb(cirrusvga_opaque, address, value);
			}
			else if (addrofs < 0x1000000 + 0x400000) {
				cirrus_linear_bitblt_writeb(cirrusvga_opaque, address, value);
			}
			return;
		}
		else if (np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr) {
			cirrus_mmio_write[0](cirrusvga_opaque, address, value);
			if (np2clvga.gd54xxtype == CIRRUS_98ID_PCI)
				return;
		}
		//if(TEST_START_ADDR < address && address <= TEST_END_ADDR){
		//	printf("%d: %d¥n", address, value);
		//}
#if defined(SUPPORT_VGA_MODEX)
			// PC/AT互換機 標準VGA相当 書き込み限定で許可
		if (np2clvga.modex && vramWndAddr3 == 0xa0000) {
			UINT32 addr3 = address;
			if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX) {
				cirrus_vga_mem_writeb(cirrusvga_opaque, addr3, value);
				vramWndAddr3 = 0;
			}
		}
#endif
		if (np2clvga.gd54xxtype != CIRRUS_98ID_PCI) {
			if (vramWndAddr) {
				if (vramWndAddr <= address) {
					if (address < vramWndAddr + VRAMWINDOW_SIZE) {
						g_cirrus_linear_write[0](cirrusvga_opaque, address, value);
						return;
					}
					else if (address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE) {
						if (address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT) {
							g_cirrus_linear_write[0](cirrusvga_opaque, address, value);
							return;
						}
					}
				}
			}
			if (vramWndAddr3) {
				UINT32 addr3 = address;
				if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX) {
					CIRRUS_VRAMWND3_FUNC_wb(cirrusvga_opaque, addr3, value);
					//TRACEOUT(("mem (write8): %x", address));
					//if(!(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))
					//	return;
				}
			}
			if (vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))) {
				UINT32 addr2 = address;
				if ((vramWndAddr2 & 0xfff00000UL) == 0) {
					UINT32 addrtmp = addr2 & 0xfff80000UL;
					if (addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL) {
						// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
						addr2 = addr2 & 0xfffff;
					}
				}
				if ((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2) {
					CIRRUS_VRAMWND2_FUNC_wb(cirrusvga_opaque, addr2, value);
					//if((vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) && !(gdc.display & (1 << GDCDISP_31))) 
					//	return;
				}
			}
		}
	}
#endif
	address = address & CPU_ADRSMASK;
	if (address < USE_HIMEM) {
		memfn0.wr8[address >> 15](address, value);
	}
	else if (address < CPU_EXTLIMIT16) {
		CPU_EXTMEMBASE[address] = (UINT8)value;
	}
	else if (address < 0x00f00000) {
	}
	else if (address < 0x01000000) {
		memfnf.wr8[(address >> 17) & 7](address, value);
	}
#if defined(CPU_EXTLIMIT)
	else if (address < CPU_EXTLIMIT) {
		CPU_EXTMEMBASE[address] = (UINT8)value;
	}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
	else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
		memvgaf_wr8(address, value);
	}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
	else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
		npdisp.mm_screenPtr[address - npdisp.mm_vramPhysicalAddr] = value;
	}
#endif	// defined(SUPPORT_WAB_NPDISP)
	else {
		TRACEOUT(("out of mem (write8): %x", address));
	}

}

static void MEMCALL memp_write16_slow(UINT32 address, REG16 value) {
	if ((address + 1) & 0x7fff) {			// non 32kb boundary
#if defined(SUPPORT_WAB_GA1280A)
		if (ga1280a_memp_try_write16(address, value)) return;
#endif
#if defined(SUPPORT_CL_GD5430)
		if (np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)) {
			UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
			UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
			UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
			if (np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr) {
				UINT32 addrofs = address - np2clvga.pciLFB_Addr;
				if (addrofs < 0x1000000) {
					cirrus_linear_writew(cirrusvga_opaque, address, value);
				}
				else if (addrofs < 0x1000000 + 0x400000) {
					cirrus_linear_bitblt_writew(cirrusvga_opaque, address, value);
				}
				return;
			}
			else if (np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr) {
				cirrus_mmio_write[1](cirrusvga_opaque, address, value);
				if (np2clvga.gd54xxtype == CIRRUS_98ID_PCI)
					return;
			}
			//if(TEST_START_ADDR < address && address <= TEST_END_ADDR){
			//	printf("%d: %d¥n", address, value);
			//}
#if defined(SUPPORT_VGA_MODEX)
				// PC/AT互換機 標準VGA相当 書き込み限定で許可
			if (np2clvga.modex && vramWndAddr3 == 0xa0000) {
				UINT32 addr3 = address;
				if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX) {
					cirrus_vga_mem_writew(cirrusvga_opaque, addr3, value);
					vramWndAddr3 = 0;
				}
			}
#endif
			if (np2clvga.gd54xxtype != CIRRUS_98ID_PCI) {
				if (vramWndAddr) {
					if (vramWndAddr <= address) {
						if (address < vramWndAddr + VRAMWINDOW_SIZE) {
							g_cirrus_linear_write[1](cirrusvga_opaque, address, value);
							return;
						}
						else if (address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE) {
							if (address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT) {
								g_cirrus_linear_write[1](cirrusvga_opaque, address, value);
								return;
							}
						}
					}
				}
				if (vramWndAddr3) {
					UINT32 addr3 = address;
					if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX) {
						CIRRUS_VRAMWND3_FUNC_ww(cirrusvga_opaque, addr3, value);
						//TRACEOUT(("mem (write16): %x", address));
						//if(!(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))
						//	return;
					}
				}
				if (vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))) {
					UINT32 addr2 = address;
					if ((vramWndAddr2 & 0xfff00000UL) == 0) {
						UINT32 addrtmp = addr2 & 0xfff80000UL;
						if (addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL) {
							// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
							addr2 = addr2 & 0xfffff;
						}
					}
					if ((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2) {
						CIRRUS_VRAMWND2_FUNC_ww(cirrusvga_opaque, addr2, value);
						//if((vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) && !(gdc.display & (1 << GDCDISP_31))) 
						//	return;
					}
				}
			}
		}
#endif
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			memfn0.wr16[address >> 15](address, value);
		}
		else if (address < CPU_EXTLIMIT16) {
			STOREINTELWORD(CPU_EXTMEMBASE + address, value);
		}
		else if (address < 0x00f00000) {
		}
		else if (address < 0x01000000) {
			memfnf.wr16[(address >> 17) & 7](address, value);
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			STOREINTELWORD(CPU_EXTMEMBASE + address, value);
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			memvgaf_wr16(address, value);
		}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
		else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
			STOREINTELWORD(npdisp.mm_screenPtr + (address - npdisp.mm_vramPhysicalAddr), value);
		}
#endif	// defined(SUPPORT_WAB_NPDISP)
		else {
			TRACEOUT(("out of mem (write16): %x", address));
		}
	}
	else {
		memp_write8_fast(address + 0, (UINT8)value);
		memp_write8_fast(address + 1, (UINT8)(value >> 8));
	}

}

static void MEMCALL memp_write32_slow(UINT32 address, UINT32 value) {
	if ((address & 0x7fff) <= 0x8000 - 4) {			// non 32kb boundary
#if defined(SUPPORT_WAB_GA1280A)
		if (ga1280a_memp_try_write32(address, value)) return;
#endif
#if defined(SUPPORT_CL_GD5430)
		if (np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)) {
			UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
			UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
			UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
			if (np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr) {
				UINT32 addrofs = address - np2clvga.pciLFB_Addr;
				if (addrofs < 0x1000000) {
					cirrus_linear_writel(cirrusvga_opaque, address, value);
				}
				else if (addrofs < 0x1000000 + 0x400000) {
					cirrus_linear_bitblt_writel(cirrusvga_opaque, address, value);
				}
				return;
			}
			else if (np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr) {
				cirrus_mmio_write[2](cirrusvga_opaque, address, value);
				if (np2clvga.gd54xxtype == CIRRUS_98ID_PCI)
					return;
			}
			//if(TEST_START_ADDR < address && address <= TEST_END_ADDR){
			//	printf("%d: %d¥n", address, value);
			//}
#if defined(SUPPORT_VGA_MODEX)
				// PC/AT互換機 標準VGA相当 書き込み限定で許可
			if (np2clvga.modex && vramWndAddr3 == 0xa0000) {
				UINT32 addr3 = address;
				if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX) {
					cirrus_vga_mem_writel(cirrusvga_opaque, addr3, value);
					vramWndAddr3 = 0;
				}
			}
#endif
			if (np2clvga.gd54xxtype != CIRRUS_98ID_PCI) {
				if (vramWndAddr) {
					if (vramWndAddr <= address) {
						if (address < vramWndAddr + VRAMWINDOW_SIZE) {
							g_cirrus_linear_write[2](cirrusvga_opaque, address, value);
							return;
						}
						else if (address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE) {
							if (address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT) {
								g_cirrus_linear_write[2](cirrusvga_opaque, address, value);
								return;
							}
						}
					}
				}
				if (vramWndAddr3) {
					UINT32 addr3 = address;
					if (vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX) {
						CIRRUS_VRAMWND3_FUNC_wl(cirrusvga_opaque, addr3, value);
						//TRACEOUT(("mem (write32): %x", address));
						//if(!(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))
						//	return;
					}
				}
				if (vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))) {
					UINT32 addr2 = address;
					if ((vramWndAddr2 & 0xfff00000UL) == 0) {
						UINT32 addrtmp = addr2 & 0xfff80000UL;
						if (addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL) {
							// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
							addr2 = addr2 & 0xfffff;
						}
					}
					if ((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2) {
						CIRRUS_VRAMWND2_FUNC_wl(cirrusvga_opaque, addr2, value);
						//if((vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) && !(gdc.display & (1 << GDCDISP_31))) 
						//	return;
					}
				}
			}
		}
#endif
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			memfn0.wr32[address >> 15](address, value);
		}
		else if (address < CPU_EXTLIMIT16) {
			STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
		}
		else if (address < 0x00f00000) {
		}
		else if (address < 0x01000000) {
			memfnf.wr32[(address >> 17) & 7](address, value);
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			memvgaf_wr32(address, value);
		}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
		else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
			STOREINTELDWORD(npdisp.mm_screenPtr + (address - npdisp.mm_vramPhysicalAddr), value);
		}
#endif	// defined(SUPPORT_WAB_NPDISP)
		else {
			TRACEOUT(("out of mem (write32): %x", address));
		}
	}
	else {
		if (!(address & 1)) {
			memp_write16_fast(address + 0, (UINT16)value);
			memp_write16_fast(address + 2, (UINT16)(value >> 16));
		}
		else {
			memp_write8_fast(address + 0, (UINT8)value);
			memp_write16_fast(address + 1, (UINT16)(value >> 8));
			memp_write8_fast(address + 3, (UINT8)(value >> 24));
		}
	}

}

static void MEMCALL memp_write8_paging_slow(UINT32 address, REG8 value) {
	address = address & CPU_ADRSMASK;
	if (address < USE_HIMEM) {
		memfn0.wr8[address >> 15](address, value);
	}
	else if (address < CPU_EXTLIMIT16) {
		CPU_EXTMEMBASE[address] = (UINT8)value;
	}
	else if (address < 0x00f00000) {
	}
	else if (address < 0x01000000) {
		memfnf.wr8[(address >> 17) & 7](address, value);
	}
#if defined(CPU_EXTLIMIT)
	else if (address < CPU_EXTLIMIT) {
		CPU_EXTMEMBASE[address] = (UINT8)value;
	}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
	else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
		memvgaf_wr8(address, value);
	}
#endif	// defined(SUPPORT_PC9821)
	else {
		TRACEOUT(("out of mem (write8): %x", address));
	}

}

static void MEMCALL memp_write16_paging_slow(UINT32 address, REG16 value) {
	if ((address + 1) & 0x7fff) {			// non 32kb boundary
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			memfn0.wr16[address >> 15](address, value);
		}
		else if (address < CPU_EXTLIMIT16) {
			STOREINTELWORD(CPU_EXTMEMBASE + address, value);
		}
		else if (address < 0x00f00000) {
		}
		else if (address < 0x01000000) {
			memfnf.wr16[(address >> 17) & 7](address, value);
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			STOREINTELWORD(CPU_EXTMEMBASE + address, value);
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			memvgaf_wr16(address, value);
		}
#endif	// defined(SUPPORT_PC9821)
		else {
			TRACEOUT(("out of mem (write16): %x", address));
		}
	}
	else {
		memp_write8_paging(address + 0, (UINT8)value);
		memp_write8_paging(address + 1, (UINT8)(value >> 8));
	}

}

static void MEMCALL memp_write32_paging_slow(UINT32 address, UINT32 value) {
	if ((address & 0x7fff) <= 0x8000 - 4) {			// non 32kb boundary
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			memfn0.wr32[address >> 15](address, value);
		}
		else if (address < CPU_EXTLIMIT16) {
			STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
		}
		else if (address < 0x00f00000) {
		}
		else if (address < 0x01000000) {
			memfnf.wr32[(address >> 17) & 7](address, value);
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			memvgaf_wr32(address, value);
		}
#endif	// defined(SUPPORT_PC9821)
		else {
			TRACEOUT(("out of mem (write32): %x", address));
		}
	}
	else {
		if (!(address & 1)) {
			memp_write16_paging(address + 0, (UINT16)value);
			memp_write16_paging(address + 2, (UINT16)(value >> 16));
		}
		else {
			memp_write8_paging(address + 0, (UINT8)value);
			memp_write16_paging(address + 1, (UINT16)(value >> 8));
			memp_write8_paging(address + 3, (UINT8)(value >> 24));
		}
	}

}

// ---- 通常メモリ読み込み関数
REG8 MEMCALL memp_read8(UINT32 address) {
	
#ifdef MEM_BDA_TRACEOUT
	if(0x400 <= address && address < 0x600){
		switch(address){
		case 0x55f:
		case 0x58a:
			break;
		default:
			TRACEOUT(("BDA (read8): %x ret %x", address, mem[address]));
		}
	}
#endif
#ifdef MEM_D8_TRACEOUT
	if(0xD8000 <= address && address < 0xDC000){
		TRACEOUT(("D8000h (read8): %x ret %x", address, mem[address]));
	}
#endif
	//if (0xD4000 <= address && address <= 0xD5FFF) {
	//	printf("GP-IB BIOS memread");
	//}
	//if (0x400 <= address && address <= 0x5ff) {
	//	printf("BDA (read8): %x ret %x", address, mem[address]);
	//}
	//if (address == 0xf0000) {
	//	printf("SYS (read8): %x ret %x", address, *((UINT8*)(mem+address)));
	//}
	if (address < I286_MEMREADMAX) {
		return(mem[address]);
	}
	else {
#if defined(SUPPORT_WAB_GA1280A)
		{
			REG8 retValue;
			if (ga1280a_memp_try_read8(address, &retValue)) return retValue;
		}
#endif
#if defined(SUPPORT_CL_GD5430)
		if(np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)){
			UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
			UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
			UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
			if(np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr){
				UINT32 addrofs = address - np2clvga.pciLFB_Addr;
				if(addrofs < 0x1000000){
					return cirrus_linear_readb(cirrusvga_opaque, address);
				}else if( addrofs < 0x1000000 + 0x400000){
					return cirrus_linear_bitblt_readb(cirrusvga_opaque, address);
				}else{
					return 0xff;
				}
			}else if(np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr){
				if(np2clvga.gd54xxtype==CIRRUS_98ID_PCI || ((np2clvga.pciMMIO_Addr & 0xfff00000) != 0xf00000 || !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))))
					return cirrus_mmio_read[0](cirrusvga_opaque, address);
			}
			if(np2clvga.gd54xxtype!=CIRRUS_98ID_PCI){
				if(vramWndAddr){
					if(vramWndAddr <= address){
						if(address < vramWndAddr + VRAMWINDOW_SIZE){
							return cirrus_linear_readb(cirrusvga_opaque, address);
						}else if(address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE){
							if(address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT)
								return cirrus_linear_readb(cirrusvga_opaque, address);
						}
					}
				}
				if(vramWndAddr3){
					UINT32 addr3 = address;
					if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX && !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))){
						return CIRRUS_VRAMWND3_FUNC_rb(cirrusvga_opaque, addr3);
					}
				}
				if(vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))){
					UINT32 addr2 = address;
					if((vramWndAddr2 & 0xfff00000UL) == 0){
						UINT32 addrtmp = addr2 & 0xfff80000UL;
						if(addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL){
							// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
							addr2 = addr2 & 0xfffff;
						}
					}
					if((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2){
						return CIRRUS_VRAMWND2_FUNC_rb(cirrusvga_opaque, addr2);
					}
				}
			}
		}
#endif
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			return(memfn0.rd8[address >> 15](address));
		}
		else if (address < CPU_EXTLIMIT16) {
			return(CPU_EXTMEMBASE[address]);
		}
		else if (address < 0x00f00000) {
			return(0xff);
		}
		else if (address < 0x01000000) {
			return(memfnf.rd8[(address >> 17) & 7](address));
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			return(CPU_EXTMEMBASE[address]);
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			return(memvgaf_rd8(address));
		}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
		else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
			return(npdisp.mm_screenPtr[address - npdisp.mm_vramPhysicalAddr]);
		}
#endif	// defined(SUPPORT_WAB_NPDISP)
		else {
//			TRACEOUT(("out of mem (read8): %x", address));
			return(0xff);
		}
	}
}

REG16 MEMCALL memp_read16(UINT32 address) {

	REG16	ret;
	
#ifdef MEM_BDA_TRACEOUT
	if(0x400 <= address && address < 0x600){
		switch(address){
		case 0x55f:
		case 0x58a:
			break;
		default:
			TRACEOUT(("BDA (read16): %x ret %x", address, *((UINT16*)(mem+address))));
		}
	}
#endif
#ifdef MEM_D8_TRACEOUT
	if(0xD8000 <= address && address < 0xDC000){
		TRACEOUT(("D8000h (read16): %x ret %x", address, *((UINT16*)(mem+address))));
	}
#endif
	//if (address == 0xf0000) {
	//	printf("SYS (read16): %x ret %x", address, *((UINT16*)(mem+address)));
	//}
	//if (0x400 <= address && address <= 0x5ff && address != 0x58a && address != 0x58b) {
	//	printf("BDA (read16): %x ret %x", address, *((UINT16*)(mem+address)));
	//}
	if (address < (I286_MEMREADMAX - 1)) {
		return(LOADINTELWORD(mem + address));
	}
	else {
		if ((address + 1) & 0x7fff) {			// non 32kb boundary
#if defined(SUPPORT_WAB_GA1280A)
			{
				REG16 retValue;
				if (ga1280a_memp_try_read16(address, &retValue)) return retValue;
			}
#endif
#if defined(SUPPORT_CL_GD5430)
			if(np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)){
				UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
				UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
				UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
				if(np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr){
					UINT32 addrofs = address - np2clvga.pciLFB_Addr;
					if(addrofs < 0x1000000){
						return cirrus_linear_readw(cirrusvga_opaque, address);
					}else if( addrofs < 0x1000000 + 0x400000){
						return cirrus_linear_bitblt_readw(cirrusvga_opaque, address);
					}else{
						return 0xffff;
					}
				}else if(np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr){
					if(np2clvga.gd54xxtype==CIRRUS_98ID_PCI || ((np2clvga.pciMMIO_Addr & 0xfff00000) != 0xf00000 || !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))))
						return cirrus_mmio_read[1](cirrusvga_opaque, address);
				}
				if(np2clvga.gd54xxtype!=CIRRUS_98ID_PCI){
					if(vramWndAddr){
						if(vramWndAddr <= address){
							if(address < vramWndAddr + VRAMWINDOW_SIZE){
								return cirrus_linear_readw(cirrusvga_opaque, address);
							}else if(address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE){
								if(address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT)
									return cirrus_linear_readw(cirrusvga_opaque, address);
							}
						}
					}
					if(vramWndAddr3){
						UINT32 addr3 = address;
						if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX && !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))){
							return CIRRUS_VRAMWND3_FUNC_rw(cirrusvga_opaque, addr3);
						}
					}
					if(vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))){
						UINT32 addr2 = address;
						if((vramWndAddr2 & 0xfff00000UL) == 0){
							UINT32 addrtmp = addr2 & 0xfff80000UL;
							if(addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL){
								// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
								addr2 = addr2 & 0xfffff;
							}
						}
						if((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2){
							return CIRRUS_VRAMWND2_FUNC_rw(cirrusvga_opaque, addr2);
						}
					}
				}
			}
#endif
			address = address & CPU_ADRSMASK;
			if (address < USE_HIMEM) {
				return(memfn0.rd16[address >> 15](address));
			}
			else if (address < CPU_EXTLIMIT16) {
				return(LOADINTELWORD(CPU_EXTMEMBASE + address));
			}
			else if (address < 0x00f00000) {
				return(0xffff);
			}
			else if (address < 0x01000000) {
				return(memfnf.rd16[(address >> 17) & 7](address));
			}
	#if defined(CPU_EXTLIMIT)
			else if (address < CPU_EXTLIMIT) {
				return(LOADINTELWORD(CPU_EXTMEMBASE + address));
			}
	#endif	// defined(CPU_EXTLIMIT)
	#if defined(SUPPORT_PC9821)
			else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
				return(memvgaf_rd16(address));
			}
	#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
			else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
				return(LOADINTELWORD(npdisp.mm_screenPtr + (address - npdisp.mm_vramPhysicalAddr)));
			}
#endif	// defined(SUPPORT_WAB_NPDISP)
			else {
	//			TRACEOUT(("out of mem (read16): %x", address));
				return(0xffff);
			}
		}
		else {
			ret = memp_read8(address + 0);
			ret |= (REG16)(memp_read8(address + 1) << 8);
			return(ret);
		}
	}
}

UINT32 MEMCALL memp_read32(UINT32 address) {

	//UINT32	pos;
	UINT32	ret;
	
#ifdef MEM_BDA_TRACEOUT
	if(0x400 <= address && address < 0x600){
		switch(address){
		case 0x55f:
		case 0x58a:
			break;
		default:
			TRACEOUT(("BDA (read32): %x ret %x", address, *((UINT32*)(mem+address))));
		}
	}
#endif
#ifdef MEM_D8_TRACEOUT
	if(0xD8000 <= address && address < 0xDC000){
		TRACEOUT(("D8000h (read32): %x ret %x", address, *((UINT32*)(mem+address))));
	}
#endif
	//if (0x400 <= address && address <= 0x5ff) {
	//	printf("BDA (read32): %x ret %x", address, *((UINT32*)(mem+address)));
	//}
	//if (address == 0xf0000) {
	//	printf("SYS (read32): %x ret %x", address, *((UINT32*)(mem+address)));
	//}
	if (address < (I286_MEMREADMAX - 3)) {
		return(LOADINTELDWORD(mem + address));
	}
	else{
		if ((address & 0x7fff) <= 0x8000 - 4) {			// non 32kb boundary
#if defined(SUPPORT_WAB_GA1280A)
			{
				UINT32 retValue;
				if (ga1280a_memp_try_read32(address, &retValue)) return retValue;
			}
#endif
#if defined(SUPPORT_CL_GD5430)
			if(np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)){
				UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
				UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
				UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
				if(np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr){
					UINT32 addrofs = address - np2clvga.pciLFB_Addr;
					if(addrofs < 0x1000000){
						return cirrus_linear_readl(cirrusvga_opaque, address);
					}else if( addrofs < 0x1000000 + 0x400000){
						return cirrus_linear_bitblt_readl(cirrusvga_opaque, address);
					}else{
						return 0xffffffff;
					}
				}else if(np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr){
					if(np2clvga.gd54xxtype==CIRRUS_98ID_PCI || ((np2clvga.pciMMIO_Addr & 0xfff00000) != 0xf00000 || !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))))
						return cirrus_mmio_read[2](cirrusvga_opaque, address);
				}
				if(np2clvga.gd54xxtype!=CIRRUS_98ID_PCI){
					if(vramWndAddr){
						if(vramWndAddr <= address){
							if(address < vramWndAddr + VRAMWINDOW_SIZE){
								return cirrus_linear_readl(cirrusvga_opaque, address);
							}else if(address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE){
								if(address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT)
									return cirrus_linear_readl(cirrusvga_opaque, address);
							}
						}
					}
					if(vramWndAddr3){
						UINT32 addr3 = address;
						if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX && !(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))){
							return CIRRUS_VRAMWND3_FUNC_rl(cirrusvga_opaque, addr3);
						}
					}
					if(vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))){
						UINT32 addr2 = address;
						if((vramWndAddr2 & 0xfff00000UL) == 0){
							UINT32 addrtmp = addr2 & 0xfff80000UL;
							if(addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL){
								// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
								addr2 = addr2 & 0xfffff;
							}
						}
						if((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2){
							return CIRRUS_VRAMWND2_FUNC_rl(cirrusvga_opaque, addr2);
						}
					}
				}
			}
#endif
			address = address & CPU_ADRSMASK;
			if (address < USE_HIMEM) {
				return(memfn0.rd32[address >> 15](address));
			}
			else if (address < CPU_EXTLIMIT16) {
				return(LOADINTELDWORD(CPU_EXTMEMBASE + address));
			}
			else if (address < 0x00f00000) {
				return(0xffff);
			}
			else if (address < 0x01000000) {
				return(memfnf.rd32[(address >> 17) & 7](address));
			}
	#if defined(CPU_EXTLIMIT)
			else if (address < CPU_EXTLIMIT) {
				return(LOADINTELDWORD(CPU_EXTMEMBASE + address));
			}
	#endif	// defined(CPU_EXTLIMIT)
	#if defined(SUPPORT_PC9821)
			else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
				return(memvgaf_rd32(address));
			}
	#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
			else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
				return(LOADINTELDWORD(npdisp.mm_screenPtr + (address - npdisp.mm_vramPhysicalAddr)));
			}
#endif	// defined(SUPPORT_WAB_NPDISP)
			else {
	//			TRACEOUT(("out of mem (read32): %x", address));
				return(0xffffffff);
			}
		}
		else {
			if (!(address & 1)) {
				ret = memp_read16(address + 0);
				ret |= (UINT32)memp_read16(address + 2) << 16;
			}
			else {
				ret = memp_read8(address + 0);
				ret |= (UINT32)memp_read16(address + 1) << 8;
				ret |= (UINT32)memp_read8(address + 3) << 24;
			}
			return(ret);
		}
	}
}

// ----- 高速版読み込み　普通のメモリを優先的に処理する
REG8 MEMCALL memp_read8_fast(UINT32 address) {
	UINT32 raw = address;

	if (address < I286_MEMREADMAX) {
		return mem[address];
	}
	address = address & CPU_ADRSMASK;
	if (memp_fastmmio_addr_is_marked(address)) return memp_read8_slow(raw);
	return CPU_EXTMEMBASE[address];
}

REG16 MEMCALL memp_read16_fast(UINT32 address) {
	UINT32 raw = address;

	if (address < (I286_MEMREADMAX - 1)) {
		return LOADINTELWORD(mem + address);
	}
	address = address & CPU_ADRSMASK;
	if (!((address + 1) & 0x7fff) || memp_fastmmio_range_is_marked(address, 2)) return memp_read16_slow(raw);
	return LOADINTELWORD(CPU_EXTMEMBASE + address);
}

UINT32 MEMCALL memp_read32_fast(UINT32 address) {
	UINT32 raw = address;

	if (address < (I286_MEMREADMAX - 3)) {
		return LOADINTELDWORD(mem + address);
	}
	address = address & CPU_ADRSMASK;
	if (!((address & 0x7fff) <= 0x8000 - 4) || memp_fastmmio_range_is_marked(address, 4)) return memp_read32_slow(raw);
	return LOADINTELDWORD(CPU_EXTMEMBASE + address);
}

// ---- 通常メモリ読み込み関数（codefetch用）
PF_UINT8 MEMCALL memp_read8_codefetch(UINT32 address) {
	
	if (address < I286_MEMREADMAX) {
		return(mem[address]);
	}
	else {
		//if(pcidev.bios32entrypoint <= address && address < pcidev.bios32entrypoint + sizeof(pcidev.biosdata.data8)){
		//	printf("BIOS32 (read8): %x");
		//}
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			return(memfn0.rd8[address >> 15](address));
		}
		else if (address < CPU_EXTLIMIT16) {
			return(CPU_EXTMEMBASE[address]);
		}
		else if (address < 0x00f00000) {
			return(0xff);
		}
		else if (address < 0x01000000) {
			return(memfnf.rd8[(address >> 17) & 7](address));
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			return(CPU_EXTMEMBASE[address]);
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			return(memvgaf_rd8(address));
		}
#endif	// defined(SUPPORT_PC9821)
		else {
//			TRACEOUT(("out of mem (read8): %x", address));
			return(0xff);
		}
	}
}
PF_UINT16 MEMCALL memp_read16_codefetch(UINT32 address) {

	PF_UINT16	ret;
	
	if (address < (I286_MEMREADMAX - 1)) {
		return(LOADINTELWORD(mem + address));
	}
	else {
		if ((address + 1) & 0x7fff) {			// non 32kb boundary
			address = address & CPU_ADRSMASK;
			if (address < USE_HIMEM) {
				return(memfn0.rd16[address >> 15](address));
			}
			else if (address < CPU_EXTLIMIT16) {
				return(LOADINTELWORD(CPU_EXTMEMBASE + address));
			}
			else if (address < 0x00f00000) {
				return(0xffff);
			}
			else if (address < 0x01000000) {
				return(memfnf.rd16[(address >> 17) & 7](address));
			}
	#if defined(CPU_EXTLIMIT)
			else if (address < CPU_EXTLIMIT) {
				return(LOADINTELWORD(CPU_EXTMEMBASE + address));
			}
	#endif	// defined(CPU_EXTLIMIT)
	#if defined(SUPPORT_PC9821)
			else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
				return(memvgaf_rd16(address));
			}
	#endif	// defined(SUPPORT_PC9821)
			else {
	//			TRACEOUT(("out of mem (read16): %x", address));
				return(0xffff);
			}
		}
		else {
			ret = memp_read8(address + 0);
			ret |= (UINT32)(memp_read8(address + 1) << 8);
			return(ret);
		}
	}
}

UINT32 MEMCALL memp_read32_codefetch(UINT32 address) {

	//UINT32	pos;
	UINT32	ret;
	
	if (address < (I286_MEMREADMAX - 3)) {
		return(LOADINTELDWORD(mem + address));
	}
	else{
		if ((address & 0x7fff) <= 0x8000 - 4) {			// non 32kb boundary
			address = address & CPU_ADRSMASK;
			if (address < USE_HIMEM) {
				return(memfn0.rd32[address >> 15](address));
			}
			else if (address < CPU_EXTLIMIT16) {
				return(LOADINTELDWORD(CPU_EXTMEMBASE + address));
			}
			else if (address < 0x00f00000) {
				return(0xffff);
			}
			else if (address < 0x01000000) {
				return(memfnf.rd32[(address >> 17) & 7](address));
			}
	#if defined(CPU_EXTLIMIT)
			else if (address < CPU_EXTLIMIT) {
				return(LOADINTELDWORD(CPU_EXTMEMBASE + address));
			}
	#endif	// defined(CPU_EXTLIMIT)
	#if defined(SUPPORT_PC9821)
			else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
				return(memvgaf_rd32(address));
			}
	#endif	// defined(SUPPORT_PC9821)
			else {
	//			TRACEOUT(("out of mem (read32): %x", address));
				return(0xffffffff);
			}
		}
		else {
			if (!(address & 1)) {
				ret = memp_read16(address + 0);
				ret |= (UINT32)memp_read16(address + 2) << 16;
			}
			else {
				ret = memp_read8(address + 0);
				ret |= (UINT32)memp_read16(address + 1) << 8;
				ret |= (UINT32)memp_read8(address + 3) << 24;
			}
			return(ret);
		}
	}
}

// ---- 高速版読み込み（codefetch用）　普通のメモリを優先的に処理する
PF_UINT8 MEMCALL memp_read8_codefetch_fast(UINT32 address) {
	UINT32 raw = address;

	if (address < I286_MEMREADMAX) {
		return mem[address];
	}
	address = address & CPU_ADRSMASK;
	if (memp_fastmmio_addr_is_marked(address)) return memp_read8_codefetch_slow(raw);
	return CPU_EXTMEMBASE[address];
}

PF_UINT16 MEMCALL memp_read16_codefetch_fast(UINT32 address) {
	UINT32 raw = address;

	if (address < (I286_MEMREADMAX - 1)) {
		return LOADINTELWORD(mem + address);
	}
	address = address & CPU_ADRSMASK;
	if (!((address + 1) & 0x7fff) || memp_fastmmio_range_is_marked(address, 2)) return memp_read16_codefetch_slow(raw);
	return LOADINTELWORD(CPU_EXTMEMBASE + address);
}

PF_UINT32 MEMCALL memp_read32_codefetch_fast(UINT32 address) {
	UINT32 raw = address;

	if (address < (I286_MEMREADMAX - 3)) {
		return LOADINTELDWORD(mem + address);
	}
	address = address & CPU_ADRSMASK;
	if (!((address & 0x7fff) <= 0x8000 - 4) || memp_fastmmio_range_is_marked(address, 4)) return memp_read32_codefetch_slow(raw);
	return LOADINTELDWORD(CPU_EXTMEMBASE + address);
}

// ---- 通常メモリ読み込み関数（paging用）
PF_UINT8 MEMCALL memp_read8_paging(UINT32 address) {

	return memp_read8_codefetch_fast(address);
}
PF_UINT16 MEMCALL memp_read16_paging(UINT32 address) {

	return memp_read16_codefetch_fast(address);
}

PF_UINT32 MEMCALL memp_read32_paging(UINT32 address) {

	return memp_read32_codefetch_fast(address);
}

PF_UINT8 MEMCALL memp_read8_paging_fast(UINT32 address) {

	return memp_read8_codefetch_fast(address);
}
PF_UINT16 MEMCALL memp_read16_paging_fast(UINT32 address) {

	return memp_read16_codefetch_fast(address);
}

PF_UINT32 MEMCALL memp_read32_paging_fast(UINT32 address) {

	return memp_read32_codefetch_fast(address);
}

// ---- 通常メモリ書き込み関数
void MEMCALL memp_write8(UINT32 address, REG8 value) {
	
#ifdef MEM_BDA_TRACEOUT
	if(0x400 <= address && address < 0x600){
		switch(address){
		case 0x55e:
		case 0x564:
		case 0x565:
		case 0x566:
		case 0x567:
		case 0x568:
		case 0x569:
		case 0x56a:
		case 0x58a:
			break;
		case 0x4f8:
		case 0x4f9:
		case 0x4fa:
		case 0x4fb:
		case 0x4fc:
			break;
		default:
			TRACEOUT(("BDA (write8): %x val %x -> %x", address, mem[address], value));
		}
	}
#endif
	if (address==0x0457) return; // XXX: IDEのデータ破壊回避のための暫定
	if (address < I286_MEMWRITEMAX) {
		mem[address] = (UINT8)value;
	}
	else {
#if defined(SUPPORT_WAB_GA1280A)
		if (ga1280a_memp_try_write8(address, value)) return;
#endif
#if defined(SUPPORT_CL_GD5430)
		if(np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)){
			UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
			UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
			UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
			if(np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr){
				UINT32 addrofs = address - np2clvga.pciLFB_Addr;
				if(addrofs < 0x1000000){
					cirrus_linear_writeb(cirrusvga_opaque, address, value);
				}else if( addrofs < 0x1000000 + 0x400000){
					cirrus_linear_bitblt_writeb(cirrusvga_opaque, address, value);
				}
				return;
			}else if(np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr){
				cirrus_mmio_write[0](cirrusvga_opaque, address, value);
				if(np2clvga.gd54xxtype==CIRRUS_98ID_PCI)
					return;
			}
			//if(TEST_START_ADDR < address && address <= TEST_END_ADDR){
			//	printf("%d: %d\n", address, value);
			//}
#if defined(SUPPORT_VGA_MODEX)
			// PC/AT互換機 標準VGA相当 書き込み限定で許可
			if(np2clvga.modex && vramWndAddr3==0xa0000){
				UINT32 addr3 = address;
				if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX){
					cirrus_vga_mem_writeb(cirrusvga_opaque, addr3, value);
					vramWndAddr3 = 0;
				}
			}
#endif
			if(np2clvga.gd54xxtype!=CIRRUS_98ID_PCI){
				if(vramWndAddr){
					if(vramWndAddr <= address){
						if(address < vramWndAddr + VRAMWINDOW_SIZE){
							g_cirrus_linear_write[0](cirrusvga_opaque, address, value);
							return;
						}else if(address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE){
							if(address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT){
								g_cirrus_linear_write[0](cirrusvga_opaque, address, value);
								return;
							}
						}
					}
				}
				if(vramWndAddr3){
					UINT32 addr3 = address;
					if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX){
						CIRRUS_VRAMWND3_FUNC_wb(cirrusvga_opaque, addr3, value);
						//TRACEOUT(("mem (write8): %x", address));
						//if(!(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))
						//	return;
					}
				}
				if(vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))){
					UINT32 addr2 = address;
					if((vramWndAddr2 & 0xfff00000UL) == 0){
						UINT32 addrtmp = addr2 & 0xfff80000UL;
						if(addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL){
							// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
							addr2 = addr2 & 0xfffff;
						}
					}
					if((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2){
						CIRRUS_VRAMWND2_FUNC_wb(cirrusvga_opaque, addr2, value);
						//if((vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) && !(gdc.display & (1 << GDCDISP_31))) 
						//	return;
					}
				}
			}
		}
#endif
// 強制RAM化
			if ((address >= 0xa5000) && (address < 0xa7fff)) {
				if (CPU_RAM_D000 & (1 << ((address >> 12) & 15))) {
					mem[address] = (UINT8)value;
				}
			}
//
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			memfn0.wr8[address >> 15](address, value);
		}
		else if (address < CPU_EXTLIMIT16) {
			CPU_EXTMEMBASE[address] = (UINT8)value;
		}
		else if (address < 0x00f00000) {
		}
		else if (address < 0x01000000) {
			memfnf.wr8[(address >> 17) & 7](address, value);
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			CPU_EXTMEMBASE[address] = (UINT8)value;
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			memvgaf_wr8(address, value);
		}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
		else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
			npdisp.mm_screenPtr[address - npdisp.mm_vramPhysicalAddr] = value;
		}
#endif	// defined(SUPPORT_WAB_NPDISP)
		else {
			TRACEOUT(("out of mem (write8): %x", address));
		}
	}
}

void MEMCALL memp_write16(UINT32 address, REG16 value) {

	
#ifdef MEM_BDA_TRACEOUT
	if(0x400 <= address && address < 0x600){
		switch(address){
		case 0x55e:
		case 0x58a:
			break;
		case 0x4f8:
		case 0x4f9:
		case 0x4fa:
		case 0x4fb:
		case 0x4fc:
		case 0x4fd:
			break;
		default:
			TRACEOUT(("BDA (write16): %x val %x -> %x", address, *((UINT16*)(mem+address)), value));
		}
	}
#endif
	if (address < (I286_MEMWRITEMAX - 1)) {
		STOREINTELWORD(mem + address, value);
	}
	else{
		if ((address + 1) & 0x7fff) {			// non 32kb boundary
#if defined(SUPPORT_WAB_GA1280A)
			if (ga1280a_memp_try_write16(address, value)) return;
#endif
#if defined(SUPPORT_CL_GD5430)
			if(np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)){
				UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
				UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
				UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
				if(np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr){
					UINT32 addrofs = address - np2clvga.pciLFB_Addr;
					if(addrofs < 0x1000000){
						cirrus_linear_writew(cirrusvga_opaque, address, value);
					}else if( addrofs < 0x1000000 + 0x400000){
						cirrus_linear_bitblt_writew(cirrusvga_opaque, address, value);
					}
					return;
				}else if(np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr){
					cirrus_mmio_write[1](cirrusvga_opaque, address, value);
					if(np2clvga.gd54xxtype==CIRRUS_98ID_PCI)
						return;
				}
				//if(TEST_START_ADDR < address && address <= TEST_END_ADDR){
				//	printf("%d: %d\n", address, value);
				//}
#if defined(SUPPORT_VGA_MODEX)
				// PC/AT互換機 標準VGA相当 書き込み限定で許可
				if(np2clvga.modex && vramWndAddr3==0xa0000){
					UINT32 addr3 = address;
					if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX){
						cirrus_vga_mem_writew(cirrusvga_opaque, addr3, value);
						vramWndAddr3 = 0;
					}
				}
#endif
				if(np2clvga.gd54xxtype!=CIRRUS_98ID_PCI){
					if(vramWndAddr){
						if(vramWndAddr <= address){
							if(address < vramWndAddr + VRAMWINDOW_SIZE){
								g_cirrus_linear_write[1](cirrusvga_opaque, address, value);
								return;
							}else if(address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE){
								if(address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT){
									g_cirrus_linear_write[1](cirrusvga_opaque, address, value);
									return;
								}
							}
						}
					}
					if(vramWndAddr3){
						UINT32 addr3 = address;
						if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX){
							CIRRUS_VRAMWND3_FUNC_ww(cirrusvga_opaque, addr3, value);
							//TRACEOUT(("mem (write16): %x", address));
							//if(!(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))
							//	return;
						}
					}
					if(vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))){
						UINT32 addr2 = address;
						if((vramWndAddr2 & 0xfff00000UL) == 0){
							UINT32 addrtmp = addr2 & 0xfff80000UL;
							if(addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL){
								// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
								addr2 = addr2 & 0xfffff;
							}
						}
						if((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2){
							CIRRUS_VRAMWND2_FUNC_ww(cirrusvga_opaque, addr2, value);
							//if((vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) && !(gdc.display & (1 << GDCDISP_31))) 
							//	return;
						}
					}
				}
			}
#endif
// 強制RAM化
			if ((address >= 0xa5000) && (address < 0xa7fff)) {

				UINT8	*ptr;
				UINT16	bit;

				ptr = mem + address;
				bit = 1 << ((address >> 12) & 15);
				if ((address + 1) & 0xfff) {
					if (CPU_RAM_D000 & bit) {
						STOREINTELWORD(ptr, value);
					}
				}
				else {
					if (CPU_RAM_D000 & bit) {
						ptr[0] = (UINT8)value;
					}
					if (CPU_RAM_D000 & (bit << 1)) {
						ptr[1] = (UINT8)(value >> 8);
					}
				}
			}
//
			address = address & CPU_ADRSMASK;
			if (address < USE_HIMEM) {
				memfn0.wr16[address >> 15](address, value);
			}
			else if (address < CPU_EXTLIMIT16) {
				STOREINTELWORD(CPU_EXTMEMBASE + address, value);
			}
			else if (address < 0x00f00000) {
			}
			else if (address < 0x01000000) {
				memfnf.wr16[(address >> 17) & 7](address, value);
			}
#if defined(CPU_EXTLIMIT)
			else if (address < CPU_EXTLIMIT) {
				STOREINTELWORD(CPU_EXTMEMBASE + address, value);
			}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
			else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
				memvgaf_wr16(address, value);
			}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
			else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
				STOREINTELWORD(npdisp.mm_screenPtr + (address - npdisp.mm_vramPhysicalAddr), value);
			}
#endif	// defined(SUPPORT_WAB_NPDISP)
			else {
				TRACEOUT(("out of mem (write16): %x", address));
			}
		}
		else {
			memp_write8(address + 0, (UINT8)value);
			memp_write8(address + 1, (UINT8)(value >> 8));
		}
	}
}

void MEMCALL memp_write32(UINT32 address, UINT32 value) {

	//UINT32	pos;
	
#ifdef MEM_BDA_TRACEOUT
	if(0x400 <= address && address < 0x600){
		switch(address){
		case 0x55e:
		case 0x58a:
			break;
		case 0x4f8:
		case 0x4f9:
		case 0x4fa:
		case 0x4fb:
		case 0x4fc:
		case 0x4fd:
			break;
		default:
			TRACEOUT(("BDA (write32): %x val %x -> %x", address, *((UINT32*)(mem+address)), value));
		}
	}
#endif
	if (address < (I286_MEMWRITEMAX - 3)) {
		STOREINTELDWORD(mem + address, value);
		return;
	}
	else{
		if ((address & 0x7fff) <= 0x8000 - 4) {			// non 32kb boundary
#if defined(SUPPORT_WAB_GA1280A)
			if (ga1280a_memp_try_write32(address, value)) return;
#endif
#if defined(SUPPORT_CL_GD5430)
			if(np2clvga.enabled && cirrusvga_opaque && (cirrusvga_wab_46e8 & 0x08)){
				UINT32 vramWndAddr = np2clvga.VRAMWindowAddr;
				UINT32 vramWndAddr2 = np2clvga.VRAMWindowAddr2;
				UINT32 vramWndAddr3 = np2clvga.VRAMWindowAddr3;
				if(np2clvga.pciLFB_Addr && (address & np2clvga.pciLFB_Mask) == np2clvga.pciLFB_Addr){
					UINT32 addrofs = address - np2clvga.pciLFB_Addr;
					if(addrofs < 0x1000000){
						cirrus_linear_writel(cirrusvga_opaque, address, value);
					}else if( addrofs < 0x1000000 + 0x400000){
						cirrus_linear_bitblt_writel(cirrusvga_opaque, address, value);
					}
					return;
				}else if(np2clvga.pciMMIO_Addr && (address & np2clvga.pciMMIO_Mask) == np2clvga.pciMMIO_Addr){
					cirrus_mmio_write[2](cirrusvga_opaque, address, value);
					if(np2clvga.gd54xxtype==CIRRUS_98ID_PCI)
						return;
				}
				//if(TEST_START_ADDR < address && address <= TEST_END_ADDR){
				//	printf("%d: %d\n", address, value);
				//}
#if defined(SUPPORT_VGA_MODEX)
				// PC/AT互換機 標準VGA相当 書き込み限定で許可
				if(np2clvga.modex && vramWndAddr3==0xa0000){
					UINT32 addr3 = address;
					if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX){
						cirrus_vga_mem_writel(cirrusvga_opaque, addr3, value);
						vramWndAddr3 = 0;
					}
				}
#endif
				if(np2clvga.gd54xxtype!=CIRRUS_98ID_PCI){
					if(vramWndAddr){
						if(vramWndAddr <= address){
							if(address < vramWndAddr + VRAMWINDOW_SIZE){
								g_cirrus_linear_write[2](cirrusvga_opaque, address, value);
								return;
							}else if(address < vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SIZE){
								if(address >= vramWndAddr + VRAMWINDOW_SIZE + EXT_WINDOW_SHFT){
									g_cirrus_linear_write[2](cirrusvga_opaque, address, value);
									return;
								}
							}
						}
					}
					if(vramWndAddr3){
						UINT32 addr3 = address;
						if(vramWndAddr3 <= addr3 && addr3 < vramWndAddr3 + VRA3WINDOW_SIZEX){
							CIRRUS_VRAMWND3_FUNC_wl(cirrusvga_opaque, addr3, value);
							//TRACEOUT(("mem (write32): %x", address));
							//if(!(gdc.analog & ((1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))
							//	return;
						}
					}
					if(vramWndAddr2 && (vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E))))){
						UINT32 addr2 = address;
						if((vramWndAddr2 & 0xfff00000UL) == 0){
							UINT32 addrtmp = addr2 & 0xfff80000UL;
							if(addrtmp == 0xfff80000UL || addrtmp == 0x00f80000UL){
								// XXX: 0xFFFA0000 - 0xFFFFFFFF or 0xFA0000 - 0xFFFFFF
								addr2 = addr2 & 0xfffff;
							}
						}
						if((addr2 & CIRRUS_VRAMWINDOW2MASK) == vramWndAddr2){
							CIRRUS_VRAMWND2_FUNC_wl(cirrusvga_opaque, addr2, value);
							//if((vramWndAddr2 != 0xE0000 || !(gdc.analog & ((1 << GDCANALOG_16) | (1 << GDCANALOG_256) | (1 << GDCANALOG_256E)))) && !(gdc.display & (1 << GDCDISP_31))) 
							//	return;
						}
					}
				}
			}
#endif
			address = address & CPU_ADRSMASK;
			if (address < USE_HIMEM) {
				memfn0.wr32[address >> 15](address, value);
			}
			else if (address < CPU_EXTLIMIT16) {
				STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
			}
			else if (address < 0x00f00000) {
			}
			else if (address < 0x01000000) {
				memfnf.wr32[(address >> 17) & 7](address, value);
			}
#if defined(CPU_EXTLIMIT)
			else if (address < CPU_EXTLIMIT) {
				STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
			}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
			else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
				memvgaf_wr32(address, value);
			}
#endif	// defined(SUPPORT_PC9821)
#if defined(SUPPORT_WAB_NPDISP)
			else if (npdisp.mm_vramPhysicalAddr && npdisp.mm_screenPtr && npdisp.mm_vramPhysicalAddr <= address && address < npdisp.mm_vramPhysicalAddr + npdisp.mm_screenSize) {
				STOREINTELDWORD(npdisp.mm_screenPtr + (address - npdisp.mm_vramPhysicalAddr), value);
			}
#endif	// defined(SUPPORT_WAB_NPDISP)
			else {
				TRACEOUT(("out of mem (write32): %x", address));
			}
		}
		else {
			if (!(address & 1)) {
				memp_write16(address + 0, (UINT16)value);
				memp_write16(address + 2, (UINT16)(value >> 16));
			}
			else {
				memp_write8(address + 0, (UINT8)value);
				memp_write16(address + 1, (UINT16)(value >> 8));
				memp_write8(address + 3, (UINT8)(value >> 24));
			}
		}
	}
}

// ---- 高速版書き込み　普通のメモリを優先的に処理する
void MEMCALL memp_write8_fast(UINT32 address, REG8 value) {
	UINT32 raw = address;

	if (address == 0x0457) return;
	if (address < I286_MEMWRITEMAX) {
		mem[address] = (UINT8)value;
		return;
	}
	address = address & CPU_ADRSMASK;
	if (memp_fastmmio_addr_is_marked(address)) {
		memp_write8_slow(raw, value);
		return;
	}
	CPU_EXTMEMBASE[address] = (UINT8)value;
}

void MEMCALL memp_write16_fast(UINT32 address, REG16 value) {
	UINT32 raw = address;

	if (address < (I286_MEMWRITEMAX - 1)) {
		STOREINTELWORD(mem + address, value);
		return;
	}
	address = address & CPU_ADRSMASK;
	if (!((address + 1) & 0x7fff) || memp_fastmmio_range_is_marked(address, 2)) {
		memp_write16_slow(raw, value);
		return;
	}
	STOREINTELWORD(CPU_EXTMEMBASE + address, value);
}

void MEMCALL memp_write32_fast(UINT32 address, UINT32 value) {
	UINT32 raw = address;

	if (address < (I286_MEMWRITEMAX - 3)) {
		STOREINTELDWORD(mem + address, value);
		return;
	}
	address = address & CPU_ADRSMASK;
	if (!((address & 0x7fff) <= 0x8000 - 4) || memp_fastmmio_range_is_marked(address, 4)) {
		memp_write32_slow(raw, value);
		return;
	}
	STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
}

// ---- 通常メモリ書き込み関数（paging用）
void MEMCALL memp_write8_paging(UINT32 address, REG8 value) {
	
	//if (address==0x0457) return; // XXX: IDEのデータ破壊回避のための暫定
	if (address < I286_MEMWRITEMAX) {
		mem[address] = (UINT8)value;
	}
	else {
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			memfn0.wr8[address >> 15](address, value);
		}
		else if (address < CPU_EXTLIMIT16) {
			CPU_EXTMEMBASE[address] = (UINT8)value;
		}
		else if (address < 0x00f00000) {
		}
		else if (address < 0x01000000) {
			memfnf.wr8[(address >> 17) & 7](address, value);
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			CPU_EXTMEMBASE[address] = (UINT8)value;
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			memvgaf_wr8(address, value);
		}
#endif	// defined(SUPPORT_PC9821)
		else {
			TRACEOUT(("out of mem (write8): %x", address));
		}
	}
}

void MEMCALL memp_write16_paging(UINT32 address, REG16 value) {
	
	if (address < (I286_MEMWRITEMAX - 1)) {
		STOREINTELWORD(mem + address, value);
	}
	else{
		if ((address + 1) & 0x7fff) {			// non 32kb boundary
			address = address & CPU_ADRSMASK;
			if (address < USE_HIMEM) {
				memfn0.wr16[address >> 15](address, value);
			}
			else if (address < CPU_EXTLIMIT16) {
				STOREINTELWORD(CPU_EXTMEMBASE + address, value);
			}
			else if (address < 0x00f00000) {
			}
			else if (address < 0x01000000) {
				memfnf.wr16[(address >> 17) & 7](address, value);
			}
#if defined(CPU_EXTLIMIT)
			else if (address < CPU_EXTLIMIT) {
				STOREINTELWORD(CPU_EXTMEMBASE + address, value);
			}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
			else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
				memvgaf_wr16(address, value);
			}
#endif	// defined(SUPPORT_PC9821)
			else {
				TRACEOUT(("out of mem (write16): %x", address));
			}
		}
		else {
			memp_write8_paging(address + 0, (UINT8)value);
			memp_write8_paging(address + 1, (UINT8)(value >> 8));
		}
	}
}

void MEMCALL memp_write32_paging(UINT32 address, UINT32 value) {
	
	if (address < (I286_MEMWRITEMAX - 3)) {
		STOREINTELDWORD(mem + address, value);
		return;
	}
	else{
		if ((address & 0x7fff) <= 0x8000 - 4) {			// non 32kb boundary
			address = address & CPU_ADRSMASK;
			if (address < USE_HIMEM) {
				memfn0.wr32[address >> 15](address, value);
			}
			else if (address < CPU_EXTLIMIT16) {
				STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
			}
			else if (address < 0x00f00000) {
			}
			else if (address < 0x01000000) {
				memfnf.wr32[(address >> 17) & 7](address, value);
			}
#if defined(CPU_EXTLIMIT)
			else if (address < CPU_EXTLIMIT) {
				STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
			}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
			else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
				memvgaf_wr32(address, value);
			}
#endif	// defined(SUPPORT_PC9821)
			else {
				TRACEOUT(("out of mem (write32): %x", address));
			}
		}
		else {
			if (!(address & 1)) {
				memp_write16_paging(address + 0, (UINT16)value);
				memp_write16_paging(address + 2, (UINT16)(value >> 16));
			}
			else {
				memp_write8_paging(address + 0, (UINT8)value);
				memp_write16_paging(address + 1, (UINT16)(value >> 8));
				memp_write8_paging(address + 3, (UINT8)(value >> 24));
			}
		}
	}
}

// ---- 高速版書き込み（paging用）　普通のメモリを優先的に処理する
void MEMCALL memp_write8_paging_fast(UINT32 address, REG8 value) {
	UINT32 raw = address;

	if (address < I286_MEMWRITEMAX) {
		mem[address] = (UINT8)value;
		return;
	}
	address = address & CPU_ADRSMASK;
	if (memp_fastmmio_addr_is_marked(address)) {
		memp_write8_paging_slow(raw, value);
		return;
	}
	CPU_EXTMEMBASE[address] = (UINT8)value;
}

void MEMCALL memp_write16_paging_fast(UINT32 address, REG16 value) {
	UINT32 raw = address;

	if (address < (I286_MEMWRITEMAX - 1)) {
		STOREINTELWORD(mem + address, value);
		return;
	}
	address = address & CPU_ADRSMASK;
	if (!((address + 1) & 0x7fff) || memp_fastmmio_range_is_marked(address, 2)) {
		memp_write16_paging_slow(raw, value);
		return;
	}
	STOREINTELWORD(CPU_EXTMEMBASE + address, value);
}

void MEMCALL memp_write32_paging_fast(UINT32 address, UINT32 value) {
	UINT32 raw = address;

	if (address < (I286_MEMWRITEMAX - 3)) {
		STOREINTELDWORD(mem + address, value);
		return;
	}
	address = address & CPU_ADRSMASK;
	if (!((address & 0x7fff) <= 0x8000 - 4) || memp_fastmmio_range_is_marked(address, 4)) {
		memp_write32_paging_slow(raw, value);
		return;
	}
	STOREINTELDWORD(CPU_EXTMEMBASE + address, value);
}


void MEMCALL memp_reads(UINT32 address, void *dat, UINT leng) {

	UINT8 *out = (UINT8 *)dat;
	UINT diff;
	
	/* fast memory access */
	if ((address + leng) < I286_MEMREADMAX) {
		CopyMemory(dat, mem + address, leng);
		return;
	}
	address = address & CPU_ADRSMASK;
	if ((address >= USE_HIMEM) && (address < CPU_EXTLIMIT16)) {
		diff = CPU_EXTLIMIT16 - address;
		if (diff >= leng) {
			CopyMemory(dat, CPU_EXTMEMBASE + address, leng);
			return;
		}
		CopyMemory(dat, CPU_EXTMEMBASE + address, diff);
		out += diff;
		leng -= diff;
		address += diff;
	}

	/* XXX: Cirrusを無視しているがCirrusはmemp_readsを呼ばないのでOK */
	if ((address >= 0x01000000) && (address < CPU_EXTLIMIT)) {
		diff = CPU_EXTLIMIT - address;
		if (diff >= leng) {
			CopyMemory(dat, CPU_EXTMEMBASE + address, leng);
			return;
		}
		CopyMemory(dat, CPU_EXTMEMBASE + address, diff);
		out += diff;
		leng -= diff;
		address += diff;
	}

	/* slow memory access */
	while (leng-- > 0) {
		*out++ = memp_read8(address++);
	}
}

void MEMCALL memp_writes(UINT32 address, const void *dat, UINT leng) {

	const UINT8 *out = (UINT8 *)dat;
	UINT diff;

	/* fast memory access */
	if ((address + leng) < I286_MEMREADMAX) {
		CopyMemory(mem + address, dat, leng);
		return;
	}
	address = address & CPU_ADRSMASK;
	if ((address >= USE_HIMEM) && (address < CPU_EXTLIMIT16)) {
		diff = CPU_EXTLIMIT16 - address;
		if (diff >= leng) {
			CopyMemory(CPU_EXTMEMBASE + address, dat, leng);
			return;
		}
		CopyMemory(CPU_EXTMEMBASE + address, dat, diff);
		out += diff;
		leng -= diff;
		address += diff;
	}

	/* XXX: Cirrusを無視しているがCirrusはmemp_writesを呼ばないのでOK */
	if ((address >= 0x01000000) && (address < CPU_EXTLIMIT)) {
		diff = CPU_EXTLIMIT - address;
		if (diff >= leng) {
			CopyMemory(CPU_EXTMEMBASE + address, dat, leng);
			return;
		}
		CopyMemory(CPU_EXTMEMBASE + address, dat, diff);
		out += diff;
		leng -= diff;
		address += diff;
	}

	/* slow memory access */
	while (leng-- > 0) {
		memp_write8(address++, *out++);
	}
}


// ---- Logical Space (BIOS)

static UINT32 MEMCALL physicaladdr(UINT32 addr, BOOL wr) {

	UINT32	a;
	UINT32	pde;
	UINT32	pte;

	a = CPU_STAT_PDE_BASE + ((addr >> 20) & 0xffc);
	pde = memp_read32(a);
	if (!(pde & CPU_PDE_PRESENT)) {
		goto retdummy;
	}
	if (!(pde & CPU_PDE_ACCESS)) {
		memp_write8(a, (UINT8)(pde | CPU_PDE_ACCESS));
	}
	a = (pde & CPU_PDE_BASEADDR_MASK) + ((addr >> 10) & 0xffc);
	pte = cpu_memoryread_d(a);
	if (!(pte & CPU_PTE_PRESENT)) {
		goto retdummy;
	}
	if (!(pte & CPU_PTE_ACCESS)) {
		memp_write8(a, (UINT8)(pte | CPU_PTE_ACCESS));
	}
	if ((wr) && (!(pte & CPU_PTE_DIRTY))) {
		memp_write8(a, (UINT8)(pte | CPU_PTE_DIRTY));
	}
	addr = (pte & CPU_PTE_BASEADDR_MASK) + (addr & 0x00000fff);
	return(addr);

 retdummy:
	return(0x01000000);	/* XXX */
}


void MEMCALL meml_reads(UINT32 address, void *dat, UINT leng) {

	UINT	size;

	if (!CPU_STAT_PAGING) {
		memp_reads(address, dat, leng);
	}
	else {
		while(leng) {
			size = 0x1000 - (address & 0xfff);
			size = MIN(size, leng);
			memp_reads(physicaladdr(address, FALSE), dat, size);
			address += size;
			dat = ((UINT8 *)dat) + size;
			leng -= size;
		}
	}
}

void MEMCALL meml_writes(UINT32 address, const void *dat, UINT leng) {

	UINT	size;

	if (!CPU_STAT_PAGING) {
		memp_writes(address, dat, leng);
	}
	else {
		while(leng) {
			size = 0x1000 - (address & 0xfff);
			size = MIN(size, leng);
			memp_writes(physicaladdr(address, TRUE), dat, size);
			address += size;
			dat = ((UINT8 *)dat) + size;
			leng -= size;
		}
	}
}


REG8 MEMCALL memr_read8(UINT seg, UINT off) {

	UINT32	addr;

	addr = (seg << 4) + LOW16(off);
	if (CPU_STAT_PAGING) {
		addr = physicaladdr(addr, FALSE);
	}
	return(memp_read8(addr));
}

REG16 MEMCALL memr_read16(UINT seg, UINT off) {

	UINT32	addr;

	addr = (seg << 4) + LOW16(off);
	if (!CPU_STAT_PAGING) {
		return(memp_read16(addr));
	}
	else if ((addr + 1) & 0xfff) {
		return(memp_read16(physicaladdr(addr, FALSE)));
	}
	return(memr_read8(seg, off) + (memr_read8(seg, off + 1) << 8));
}

void MEMCALL memr_write8(UINT seg, UINT off, REG8 dat) {

	UINT32	addr;

	addr = (seg << 4) + LOW16(off);
	if (CPU_STAT_PAGING) {
		addr = physicaladdr(addr, TRUE);
	}
	memp_write8(addr, dat);
}

void MEMCALL memr_write16(UINT seg, UINT off, REG16 dat) {

	UINT32	addr;

	addr = (seg << 4) + LOW16(off);
	if (!CPU_STAT_PAGING) {
		memp_write16(addr, dat);
	}
	else if ((addr + 1) & 0xfff) {
		memp_write16(physicaladdr(addr, TRUE), dat);
	}
	else {
		memr_write8(seg, off, (REG8)dat);
		memr_write8(seg, off + 1, (REG8)(dat >> 8));
	}
}

void MEMCALL memr_reads(UINT seg, UINT off, void *dat, UINT leng) {

	UINT32	addr;
	UINT	rem;
	UINT	size;

	while(leng) {
		off = LOW16(off);
		addr = (seg << 4) + off;
		rem = 0x10000 - off;
		size = MIN(leng, rem);
		if (CPU_STAT_PAGING) {
			rem = 0x1000 - (addr & 0xfff);
			size = MIN(size, rem);
			addr = physicaladdr(addr, FALSE);
		}
		memp_reads(addr, dat, size);
		off += size;
		dat = ((UINT8 *)dat) + size;
		leng -= size;
	}
}

void MEMCALL memr_writes(UINT seg, UINT off, const void *dat, UINT leng) {

	UINT32	addr;
	UINT	rem;
	UINT	size;

	while(leng) {
		off = LOW16(off);
		addr = (seg << 4) + off;
		rem = 0x10000 - off;
		size = MIN(leng, rem);
		if (CPU_STAT_PAGING) {
			rem = 0x1000 - (addr & 0xfff);
			size = MIN(size, rem);
			addr = physicaladdr(addr, TRUE);
		}
		memp_writes(addr, dat, size);
		off += size;
		dat = ((UINT8 *)dat) + size;
		leng -= size;
	}
}

#endif
