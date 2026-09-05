/*
    VirtualPC VHD image support
    fixed and dynamically expanding disk images

    author: lpproj, SimK (dynamic VHD support)
    license: same as xnp2 (under the 2-clause BSD)
*/
#ifndef HDD_VPCVHD_H
//#include <compiler.h>
//#include <common/strres.h>
//#include <dosio.h>
//#include <sysmng.h>
//#include <cpucore.h>
//#include <pccore.h>
//#include <fdd/sxsi.h>
//#include "hdd_vpc.h"
#endif

#include <stdlib.h>

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/*
 * NOTE: Dynamically expanding VHD layout:
 *
 *   +-------------------------------+  offset 0
 *   | footer copy (512 bytes)       |
 *   +-------------------------------+
 *   | dynamic header (cxsparse)     |  footer.DataOffset
 *   +-------------------------------+
 *   | BAT (32-bit sector offsets)   |  header.TableOffset
 *   +-------------------------------+
 *   | ... allocated blocks ...      |
 *   |   sector bitmap (512 aligned) |  <- BAT entry points here
 *   |   block data                  |
 *   +-------------------------------+
 *   | trailing footer               |  <- authoritative footer used here
 *   +-------------------------------+
 *
 * A BAT entry of 0xffffffff means that the logical block has never been
 * allocated.  Inside an allocated block, a clear bitmap bit means that the
 * corresponding 512-byte sector is logically zero.  VHD multibyte metadata
 * is big-endian; guest sector contents are stored verbatim.
 */

#define LOADBEWORD LOADMOTOROLAWORD
#define LOADBEDWORD LOADMOTOROLADWORD
#define LOADBEQWORD LOADMOTOROLAQWORD
#define STOREBEWORD STOREMOTOROLAWORD
#define STOREBEDWORD STOREMOTOROLADWORD
#define STOREBEQWORD STOREMOTOROLAQWORD

#define VPCVHD_SECTOR_SIZE          512U
#define VPCVHD_BAT_UNUSED           0xffffffffU
#define VPCVHD_BITMAP_CACHE_INVALID 0xffffffffU
#define VPCVHD_RECOVERY_SCAN_SECTORS 4U

#ifdef SUPPORT_NVL_IMAGES
extern BOOL nvl_check(void);
#endif

static const UINT8 CookieVPCVHDFooter[8] = { 'c','o','n','e','c','t','i','x' };
static const UINT8 CookieVPCVHDDDH[8] = { 'c','x','s','p','a','r','s','e' };
static const UINT8 CookieVPCQEMUCreator[4] = { 'q','e','m','u' };

/* load/store with "network order" (big endian) */

#if 0
static UINT16 LOADBEWORD(const void *p)
{
  const UINT8 *b = (UINT8 *)p;
  return( ((UINT16)(b[0])<<8) | b[1] );
}
static UINT32 LOADBEDWORD(const void *p)
{
  const UINT8 *b = (UINT8 *)p;
  return( ((UINT32)(b[0])<<24) | ((UINT32)(b[1])<<16) | ((UINT32)(b[2])<<8) | b[3] );
}
static UINT64 LOADBEQWORD(const void *p)
{
  const UINT8 *b = (UINT8 *)p;
  return( ((UINT64)(b[0])<<56) | ((UINT64)(b[1])<<48) | ((UINT64)(b[2])<<40) | ((UINT64)(b[3])<<32) | ((UINT32)(b[4])<<24) | ((UINT32)(b[5])<<16) | ((UINT32)(b[6])<<8) | b[7] );
}

static UINT16 STOREBEWORD(void *p, UINT16 v)
{
  UINT8 *b = (UINT8 *)p;
  b[0] = (UINT8)(v>>8);
  b[1] = (UINT8)v;
  return(v);
}
static UINT32 STOREBEDWORD(void *p, UINT32 v)
{
  UINT8 *b = (UINT8 *)p;
  b[0] = (UINT8)(v>>24);
  b[1] = (UINT8)(v>>16);
  b[2] = (UINT8)(v>>8);
  b[3] = (UINT8)v;
  return(v);
}
static UINT64 STOREBEQWORD(void *p, UINT64 v)
{
  UINT8 *b = (UINT8 *)p;
  b[0] = (UINT8)(v>>56);
  b[1] = (UINT8)(v>>48);
  b[2] = (UINT8)(v>>40);
  b[3] = (UINT8)(v>>32);
  b[4] = (UINT8)(v>>24);
  b[5] = (UINT8)(v>>16);
  b[6] = (UINT8)(v>>8);
  b[7] = (UINT8)v;
  return(v);
}
#endif

typedef struct _VPCVHDDYNAMIC {
	FILEH fh;
	VPCVHDFOOTER footer;          /* cached trailing footer contents */
	VPCVHD_FPOS footer_pos;       /* byte offset of the trailing footer */
	UINT footer_len;              /* normally 512; old images may use 511 */
	UINT64 current_size;          /* virtual disk size in bytes */
	UINT64 table_offset;          /* BAT byte offset in the host file */
	UINT32 block_size;            /* data bytes per dynamic block */
	UINT32 sectors_per_block;
	UINT32 bitmap_size;           /* bitmap rounded up to a 512-byte boundary */
	UINT32 bat_entries;           /* entries needed by CurrentSize */
	UINT32 *bat;                  /* host-endian cached BAT entries */
	UINT8 *bitmap;                /* one-block bitmap cache */
	UINT32 bitmap_block;          /* logical block represented by bitmap[] */
	UINT8 bitmap_dirty;           /* cached bitmap has bits not yet written to disk */
	UINT8 write_failed;           /* latch: stop all later writes after I/O error */
} VPCVHDDYNAMIC;

UINT32 vpc_calc_checksum(UINT8* buf, size_t size)
{
	UINT32 res = 0;
	size_t i;

	for (i = 0; i < size; i++)
		res += buf[i];

	return ~res;
}

static BRESULT vpcvhd_seek(FILEH fh, UINT64 pos)
{
	FILEPOS fpos;

	fpos = (FILEPOS)pos;
	if ((UINT64)fpos != pos)
		return FAILURE;
	return (file_seek(fh, fpos, FSEEK_SET) == fpos) ? SUCCESS : FAILURE;
}

static BRESULT vpcvhd_read_at(FILEH fh, UINT64 pos, void *buf, UINT size)
{
	if (vpcvhd_seek(fh, pos) != SUCCESS)
		return FAILURE;
	return (file_read(fh, buf, size) == size) ? SUCCESS : FAILURE;
}

static BRESULT vpcvhd_write_at(FILEH fh, UINT64 pos, const void *buf, UINT size)
{
	if (vpcvhd_seek(fh, pos) != SUCCESS)
		return FAILURE;
	return (file_write(fh, buf, size) == size) ? SUCCESS : FAILURE;
}

static BRESULT vpcvhd_write_zero(FILEH fh, UINT64 pos, UINT64 size)
{
	UINT8 zero[4096];

	ZeroMemory(zero, sizeof(zero));
	if (vpcvhd_seek(fh, pos) != SUCCESS)
		return FAILURE;
	while (size != 0) {
		UINT wsize;

		wsize = (size > sizeof(zero)) ? (UINT)sizeof(zero) : (UINT)size;
		if (file_write(fh, zero, wsize) != wsize)
			return FAILURE;
		size -= wsize;
	}
	return SUCCESS;
}

static BRESULT vpcvhd_sync(FILEH fh)
{
	return (file_sync(fh) == 0) ? SUCCESS : FAILURE;
}

// File I/Oエラーが起こったら安全のため書き込みを停止する
static BRESULT vpcvhd_dynamic_write_error(VPCVHDDYNAMIC *vhd)
{
	vhd->write_failed = 1;
	return FAILURE;
}

static int vpcvhd_ranges_overlap(UINT64 apos, UINT64 asize, UINT64 bpos, UINT64 bsize)
{
	UINT64 aend;
	UINT64 bend;

	aend = apos + asize;
	bend = bpos + bsize;
	if ((aend < apos) || (bend < bpos))
		return 1;
	return (apos < bend) && (bpos < aend);
}

static int vpcvhd_compare_bat_entry(const void *a, const void *b)
{
	UINT32 av;
	UINT32 bv;

	av = *(const UINT32 *)a;
	bv = *(const UINT32 *)b;
	if (av < bv)
		return -1;
	if (av > bv)
		return 1;
	return 0;
}

// Footerが正しいか確認
static BRESULT vpcvhd_validate_footer(VPCVHDFOOTER *footer, UINT flen)
{
	UINT32 checksum;
	UINT32 mychecksum;

	if ((flen != sizeof(*footer)) && (flen != sizeof(*footer) - 1))
		return FAILURE;
	if (memcmp(footer->Cookie, CookieVPCVHDFooter, sizeof(footer->Cookie)) != 0)
		return FAILURE;

	checksum = LOADBEDWORD(footer->CheckSum);
	STOREBEDWORD(footer->CheckSum, 0);
	mychecksum = vpc_calc_checksum((UINT8 *)footer, flen);
	STOREBEDWORD(footer->CheckSum, checksum);
	if (mychecksum != checksum) {
		TRACEOUT(("vpc_vhd: bad footer checksum (file=%x, calc=%x)", checksum, mychecksum));
		return FAILURE;
	}
	return SUCCESS;
}

static BRESULT vpcvhd_read_footer_at(FILEH fh, UINT64 pos, VPCVHDFOOTER *footer)
{
	ZeroMemory(footer, sizeof(*footer));
	if (vpcvhd_read_at(fh, pos, footer, sizeof(*footer)) != SUCCESS)
		return FAILURE;
	return vpcvhd_validate_footer(footer, sizeof(*footer));
}

static BRESULT vpcvhd_read_footer(FILEH fh, VPCVHDFOOTER *footer,
	VPCVHD_FPOS *footer_pos, UINT *footer_len, VPCVHD_FPOS *file_len)
{
	VPCVHD_FPOS vhdlen;
	VPCVHD_FPOS readlen;
	UINT flen;
	UINT i;

	flen = sizeof(*footer);
	ZeroMemory(footer, sizeof(*footer));
	vhdlen = file_seek(fh, 0, FSEEK_END);
	if ((vhdlen == (VPCVHD_FPOS)-1) || (vhdlen < (VPCVHD_FPOS)flen))
		return FAILURE;
	if (file_seek(fh, (FILEPOS)(vhdlen - flen), FSEEK_SET) != (FILEPOS)(vhdlen - flen))
		return FAILURE;
	readlen = file_read(fh, footer, sizeof(footer->Cookie));
	if (readlen != sizeof(footer->Cookie))
		return FAILURE;
	if (memcmp(footer->Cookie, CookieVPCVHDFooter, sizeof(footer->Cookie)) != 0) {
		for (i = 0; i < sizeof(footer->Cookie) - 1; ++i)
			footer->Cookie[i] = footer->Cookie[i + 1];
		if (file_read(fh, &(footer->Cookie[sizeof(footer->Cookie) - 1]), 1) != 1)
			return FAILURE;
		--flen;
	}
	if (memcmp(footer->Cookie, CookieVPCVHDFooter, sizeof(footer->Cookie)) != 0)
		return FAILURE;
	readlen = file_read(fh, (UINT8 *)footer + sizeof(footer->Cookie), flen - sizeof(footer->Cookie));
	if (readlen != (VPCVHD_FPOS)(flen - sizeof(footer->Cookie)))
		return FAILURE;
	if (vpcvhd_validate_footer(footer, flen) != SUCCESS)
		return FAILURE;

	*footer_pos = vhdlen - flen;
	*footer_len = flen;
	*file_len = vhdlen;
	return SUCCESS;
}

// Footerが壊れていた場合に先頭にあるバックアップから復元
static BRESULT vpcvhd_recover_dynamic_footer(FILEH fh, VPCVHDFOOTER *footer,
	VPCVHD_FPOS *footer_pos, UINT *footer_len, VPCVHD_FPOS *file_len)
{
	VPCVHDFOOTER leading;
	VPCVHDFOOTER candidate_footer;
	VPCVHD_FPOS vhdlen;
	UINT64 candidate;
	UINT i;

	vhdlen = file_seek(fh, 0, FSEEK_END);
	if ((vhdlen == (VPCVHD_FPOS)-1) ||
		(vhdlen < (VPCVHD_FPOS)(VPCVHD_SECTOR_SIZE * 2U)))
		return FAILURE;

	if (vpcvhd_read_footer_at(fh, 0, &leading) != SUCCESS)
		return FAILURE;
	if ((LOADBEDWORD(leading.FileFormatVersion) != 0x00010000U) ||
		(LOADBEDWORD(leading.DiskType) != VPCVHD_DISK_DYNAMIC) ||
		(LOADBEQWORD(leading.DataOffset) == (UINT64)-1))
		return FAILURE;

	candidate = ((UINT64)vhdlen - sizeof(candidate_footer)) &
		~((UINT64)VPCVHD_SECTOR_SIZE - 1);
	for (i = 0; i < VPCVHD_RECOVERY_SCAN_SECTORS; ++i) {
		if (candidate < VPCVHD_SECTOR_SIZE)
			break;
		if ((vpcvhd_read_footer_at(fh, candidate, &candidate_footer) == SUCCESS) &&
			(memcmp(&candidate_footer, &leading, sizeof(leading)) == 0)) {
			CopyMemory(footer, &leading, sizeof(*footer));
			*footer_pos = (VPCVHD_FPOS)candidate;
			*footer_len = sizeof(*footer);
			*file_len = vhdlen;
			TRACEOUT(("vpc_vhd: recovered dynamic footer at %luK using leading copy",
				(unsigned long)(candidate / 1024U)));
			return SUCCESS;
		}
		candidate -= VPCVHD_SECTOR_SIZE;
	}
	return FAILURE;
}

// Footerの読み取り　もし末尾のものが壊れていたら先頭のバックアップから復元を試みる
static BRESULT vpcvhd_read_footer_recover(FILEH fh, VPCVHDFOOTER *footer,
	VPCVHD_FPOS *footer_pos, UINT *footer_len, VPCVHD_FPOS *file_len,
	UINT8 *recovered)
{
	*recovered = 0;
	if (vpcvhd_read_footer(fh, footer, footer_pos, footer_len, file_len) == SUCCESS)
		return SUCCESS;
	if (vpcvhd_recover_dynamic_footer(fh, footer, footer_pos, footer_len, file_len) != SUCCESS)
		return FAILURE;
	*recovered = 1;
	return SUCCESS;
}

static BRESULT vpcvhd_dynamic_flush_bitmap(VPCVHDDYNAMIC *vhd)
{
	UINT64 block_pos;
	UINT32 entry;

	if (!vhd->bitmap_dirty)
		return SUCCESS;
	if (vhd->write_failed)
		return FAILURE;
	if ((vhd->bitmap_block == VPCVHD_BITMAP_CACHE_INVALID) ||
		(vhd->bitmap_block >= vhd->bat_entries))
		return vpcvhd_dynamic_write_error(vhd);
	entry = vhd->bat[vhd->bitmap_block];
	if (entry == VPCVHD_BAT_UNUSED)
		return vpcvhd_dynamic_write_error(vhd);
	block_pos = (UINT64)entry * VPCVHD_SECTOR_SIZE;
	if ((block_pos >= (UINT64)vhd->footer_pos) ||
		((UINT64)vhd->bitmap_size + vhd->block_size >
		 (UINT64)vhd->footer_pos - block_pos))
		return vpcvhd_dynamic_write_error(vhd);

	if (vpcvhd_sync(vhd->fh) != SUCCESS)
		return vpcvhd_dynamic_write_error(vhd);
	if (vpcvhd_write_at(vhd->fh, block_pos, vhd->bitmap,
		vhd->bitmap_size) != SUCCESS)
		return vpcvhd_dynamic_write_error(vhd);

	vhd->bitmap_dirty = 0;
	return SUCCESS;
}

static void vpcvhd_dynamic_free(VPCVHDDYNAMIC *vhd)
{
	if (vhd == NULL)
		return;
	if (vhd->fh != FILEH_INVALID) {
		if (!vhd->write_failed) {
			if ((vpcvhd_dynamic_flush_bitmap(vhd) != SUCCESS) ||
				(vpcvhd_sync(vhd->fh) != SUCCESS))
				TRACEOUT(("vpc_vhd: final flush failed while closing image"));
		}
		file_close(vhd->fh);
	}
	if (vhd->bat != NULL)
		_MFREE(vhd->bat);
	if (vhd->bitmap != NULL)
		_MFREE(vhd->bitmap);
	_MFREE(vhd);
}

static BRESULT vpcvhd_dynamic_load_bitmap(VPCVHDDYNAMIC *vhd, UINT32 block)
{
	UINT64 block_pos;
	UINT32 entry;

	if (vhd->bitmap_block == block)
		return SUCCESS;
	if (vhd->bitmap_dirty && (vpcvhd_dynamic_flush_bitmap(vhd) != SUCCESS))
		return FAILURE;
	if (block >= vhd->bat_entries)
		return FAILURE;
	entry = vhd->bat[block];
	if (entry == VPCVHD_BAT_UNUSED)
		return FAILURE;
	block_pos = (UINT64)entry * VPCVHD_SECTOR_SIZE;
	if ((block_pos >= (UINT64)vhd->footer_pos) ||
		((UINT64)vhd->bitmap_size + vhd->block_size > (UINT64)vhd->footer_pos - block_pos))
		return FAILURE;
	if (vpcvhd_read_at(vhd->fh, block_pos, vhd->bitmap, vhd->bitmap_size) != SUCCESS)
		return FAILURE;
	vhd->bitmap_block = block;
	vhd->bitmap_dirty = 0;
	return SUCCESS;
}

static BRESULT vpcvhd_dynamic_allocate_block(VPCVHDDYNAMIC *vhd, UINT32 block)
{
	UINT64 block_pos;
	UINT64 new_footer_pos;
	UINT64 new_file_len;
	UINT64 old_file_len;
	UINT64 alloc_size;
	UINT32 bat_entry;
	UINT8 be_entry[4];
	FILEPOS actual_len;

	if (vhd->write_failed)
		return FAILURE;
	if ((block >= vhd->bat_entries) || (vhd->bat[block] != VPCVHD_BAT_UNUSED))
		return (block < vhd->bat_entries) ? SUCCESS : FAILURE;
	if (vhd->bitmap_dirty && (vpcvhd_dynamic_flush_bitmap(vhd) != SUCCESS))
		return FAILURE;

	block_pos = (UINT64)vhd->footer_pos;
	if ((block_pos & (VPCVHD_SECTOR_SIZE - 1)) != 0)
		return vpcvhd_dynamic_write_error(vhd);
	if ((block_pos / VPCVHD_SECTOR_SIZE) >= VPCVHD_BAT_UNUSED)
		return vpcvhd_dynamic_write_error(vhd);

	alloc_size = (UINT64)vhd->bitmap_size + vhd->block_size;
	new_footer_pos = block_pos + alloc_size;
	new_file_len = new_footer_pos + vhd->footer_len;
	old_file_len = block_pos + vhd->footer_len;
	if ((new_footer_pos < block_pos) || (new_file_len < new_footer_pos) ||
		((UINT64)(VPCVHD_FPOS)new_footer_pos != new_footer_pos) ||
		((UINT64)(FILELEN)new_file_len != new_file_len) ||
		((UINT64)(FILELEN)old_file_len != old_file_len))
		return vpcvhd_dynamic_write_error(vhd);

	actual_len = file_seek(vhd->fh, 0, FSEEK_END);
	if ((actual_len < 0) || ((UINT64)actual_len != old_file_len))
		return vpcvhd_dynamic_write_error(vhd);

	/*
	 * 破損時に回復不能になるのを避けるために、容量拡張書き込みは以下の順にすること
	 *   1. 末尾に新しいFooterを追加書き込み
	 *   2. 新しく増えた領域をゼロクリア（旧Footerの消去）
	 *   3. ファイルシステム同期（書き込みを確実に完了させる）
	 *   4. BATエントリを更新
	 */
	if ((vpcvhd_write_at(vhd->fh, new_footer_pos, &vhd->footer, vhd->footer_len) != SUCCESS) ||
		(vpcvhd_sync(vhd->fh) != SUCCESS)) {
		/* Old trailing footer is still intact; best-effort rollback of extension. */
		if (file_setsize(vhd->fh, (FILELEN)old_file_len) == 0)
			vpcvhd_sync(vhd->fh);
		return vpcvhd_dynamic_write_error(vhd);
	}

	vhd->footer_pos = (VPCVHD_FPOS)new_footer_pos;
	if ((vpcvhd_write_zero(vhd->fh, block_pos, alloc_size) != SUCCESS) ||
		(vpcvhd_sync(vhd->fh) != SUCCESS))
		return vpcvhd_dynamic_write_error(vhd);

	bat_entry = (UINT32)(block_pos / VPCVHD_SECTOR_SIZE);
	STOREBEDWORD(be_entry, bat_entry);
	if (vpcvhd_write_at(vhd->fh, vhd->table_offset + (UINT64)block * 4,
		be_entry, sizeof(be_entry)) != SUCCESS)
		return vpcvhd_dynamic_write_error(vhd);

	vhd->bat[block] = bat_entry;
	ZeroMemory(vhd->bitmap, vhd->bitmap_size);
	vhd->bitmap_block = block;
	vhd->bitmap_dirty = 0;
	return SUCCESS;
}

static BRESULT vpcvhd_dynamic_read_bytes(VPCVHDDYNAMIC *vhd, UINT64 pos, UINT8 *buf, UINT size)
{
	while (size != 0) {
		UINT32 block;
		UINT32 block_ofs;
		UINT32 sector;
		UINT32 sector_ofs;
		UINT chunk;
		UINT32 entry;
		UINT64 data_pos;
		UINT8 mask;

		if ((pos >= vhd->current_size) || ((UINT64)size > vhd->current_size - pos))
			return FAILURE;
		block = (UINT32)(pos / vhd->block_size);
		block_ofs = (UINT32)(pos % vhd->block_size);
		sector = block_ofs / VPCVHD_SECTOR_SIZE;
		sector_ofs = block_ofs & (VPCVHD_SECTOR_SIZE - 1);
		chunk = min(size, VPCVHD_SECTOR_SIZE - sector_ofs);
		entry = vhd->bat[block];

		if (entry == VPCVHD_BAT_UNUSED) {
			ZeroMemory(buf, chunk);
		}
		else {
			if (vpcvhd_dynamic_load_bitmap(vhd, block) != SUCCESS)
				return FAILURE;
			mask = (UINT8)(0x80U >> (sector & 7));
			if ((vhd->bitmap[sector >> 3] & mask) == 0) {
				ZeroMemory(buf, chunk);
			}
			else {
				data_pos = (UINT64)entry * VPCVHD_SECTOR_SIZE + vhd->bitmap_size +
					(UINT64)sector * VPCVHD_SECTOR_SIZE + sector_ofs;
				if (vpcvhd_read_at(vhd->fh, data_pos, buf, chunk) != SUCCESS)
					return FAILURE;
			}
		}

		buf += chunk;
		size -= chunk;
		pos += chunk;
	}
	return SUCCESS;
}

static BRESULT vpcvhd_dynamic_write_bytes(VPCVHDDYNAMIC *vhd, UINT64 pos, const UINT8 *buf, UINT size)
{
	UINT8 zero_sector[VPCVHD_SECTOR_SIZE];

	if (vhd->write_failed)
		return FAILURE;
	ZeroMemory(zero_sector, sizeof(zero_sector));
	while (size != 0) {
		UINT32 block;
		UINT32 block_ofs;
		UINT32 sector;
		UINT32 sector_ofs;
		UINT chunk;
		UINT32 entry;
		UINT64 block_pos;
		UINT64 sector_pos;
		UINT64 data_pos;
		UINT32 bitmap_byte;
		UINT8 mask;
		int sector_present;

		if ((pos >= vhd->current_size) || ((UINT64)size > vhd->current_size - pos))
			return FAILURE;
		block = (UINT32)(pos / vhd->block_size);
		block_ofs = (UINT32)(pos % vhd->block_size);
		sector = block_ofs / VPCVHD_SECTOR_SIZE;
		sector_ofs = block_ofs & (VPCVHD_SECTOR_SIZE - 1);
		chunk = min(size, VPCVHD_SECTOR_SIZE - sector_ofs);

		if (vhd->bat[block] == VPCVHD_BAT_UNUSED) {
			if (vpcvhd_dynamic_allocate_block(vhd, block) != SUCCESS)
				return FAILURE;
		}
		entry = vhd->bat[block];
		if (vpcvhd_dynamic_load_bitmap(vhd, block) != SUCCESS)
			return vpcvhd_dynamic_write_error(vhd);

		bitmap_byte = sector >> 3;
		// Sector 0 -> byte 0 bit 7, Sector 1 -> byte 0 bit 6, ...
		mask = (UINT8)(0x80U >> (sector & 7));
		sector_present = ((vhd->bitmap[bitmap_byte] & mask) != 0);
		block_pos = (UINT64)entry * VPCVHD_SECTOR_SIZE;
		sector_pos = block_pos + vhd->bitmap_size +
			(UINT64)sector * VPCVHD_SECTOR_SIZE;
		data_pos = sector_pos + sector_ofs;

		if (!sector_present &&
			((sector_ofs != 0) || (chunk != VPCVHD_SECTOR_SIZE))) {
			if (vpcvhd_write_at(vhd->fh, sector_pos, zero_sector,
				VPCVHD_SECTOR_SIZE) != SUCCESS)
				return vpcvhd_dynamic_write_error(vhd);
		}

		if (vpcvhd_write_at(vhd->fh, data_pos, buf, chunk) != SUCCESS)
			return vpcvhd_dynamic_write_error(vhd);

		if (!sector_present) {
			vhd->bitmap[bitmap_byte] |= mask;
			vhd->bitmap_dirty = 1;
		}

		buf += chunk;
		size -= chunk;
		pos += chunk;
	}
	return SUCCESS;
}

static VPCVHDDYNAMIC *vpcvhd_dynamic_open_file(FILEH fh, VPCVHDFOOTER *footer,
	VPCVHD_FPOS footer_pos, UINT footer_len, VPCVHD_FPOS file_len, UINT8 footer_recovered)
{
	VPCVHDDYNAMIC *vhd;
	VPCVHDDDH ddh;
	UINT64 data_offset;
	UINT64 header_end;
	UINT64 table_offset;
	UINT64 table_bytes64;
	UINT64 table_region_size;
	UINT64 table_end;
	UINT64 current_size;
	UINT64 required_blocks;
	UINT64 bat_bytes64;
	UINT64 alloc_size;
	UINT64 block_span_sectors;
	UINT32 header_version;
	UINT32 max_entries;
	UINT32 block_size;
	UINT32 sectors_per_block;
	UINT32 bitmap_bytes;
	UINT32 bitmap_size;
	UINT32 checksum;
	UINT32 mychecksum;
	UINT8 *raw_bat;
	UINT32 allocated_count;
	UINT32 i;

	if (footer_pos < 0)
		return NULL;
	data_offset = LOADBEQWORD(footer->DataOffset);
	current_size = LOADBEQWORD(footer->CurrentSize);
	if ((data_offset == (UINT64)-1) || (current_size == 0) ||
		((current_size & (VPCVHD_SECTOR_SIZE - 1)) != 0))
		return NULL;
	if ((data_offset & (VPCVHD_SECTOR_SIZE - 1)) != 0)
		return NULL;
	header_end = data_offset + sizeof(ddh);
	if ((header_end < data_offset) || (header_end > (UINT64)footer_pos))
		return NULL;
	if (vpcvhd_read_at(fh, data_offset, &ddh, sizeof(ddh)) != SUCCESS)
		return NULL;
	if (memcmp(ddh.Cookie, CookieVPCVHDDDH, sizeof(ddh.Cookie)) != 0)
		return NULL;

	checksum = LOADBEDWORD(ddh.CheckSum);
	STOREBEDWORD(ddh.CheckSum, 0);
	mychecksum = vpc_calc_checksum((UINT8 *)&ddh, sizeof(ddh));
	STOREBEDWORD(ddh.CheckSum, checksum);
	if (mychecksum != checksum) {
		TRACEOUT(("vpc_vhd: bad dynamic header checksum (file=%x, calc=%x)", checksum, mychecksum));
		return NULL;
	}

	header_version = LOADBEDWORD(ddh.HeaderVersion);
	table_offset = LOADBEQWORD(ddh.TableOffset);
	max_entries = LOADBEDWORD(ddh.MaxTableEntries);
	block_size = LOADBEDWORD(ddh.BlockSize);
	if ((header_version != 0x00010000U) || (max_entries == 0) ||
		(block_size < VPCVHD_SECTOR_SIZE) ||
		((block_size & (VPCVHD_SECTOR_SIZE - 1)) != 0) ||
		((block_size & (block_size - 1)) != 0) ||
		((table_offset & (VPCVHD_SECTOR_SIZE - 1)) != 0))
		return NULL;

	sectors_per_block = block_size / VPCVHD_SECTOR_SIZE;
	if ((sectors_per_block == 0) || ((sectors_per_block & (sectors_per_block - 1)) != 0))
		return NULL;
	bitmap_bytes = (sectors_per_block + 7) >> 3;
	bitmap_size = (bitmap_bytes + VPCVHD_SECTOR_SIZE - 1) & ~(VPCVHD_SECTOR_SIZE - 1);
	if (bitmap_size < bitmap_bytes)
		return NULL;

	required_blocks = current_size / block_size;
	if ((current_size % block_size) != 0)
		++required_blocks;
	if ((required_blocks == 0) || (required_blocks > max_entries) ||
		(required_blocks > 0xffffffffU))
		return NULL;

	table_bytes64 = (UINT64)max_entries * 4;
	if ((table_bytes64 + VPCVHD_SECTOR_SIZE - 1 < table_bytes64))
		return NULL;
	table_region_size = (table_bytes64 + VPCVHD_SECTOR_SIZE - 1) &
		~((UINT64)VPCVHD_SECTOR_SIZE - 1);
	table_end = table_offset + table_region_size;
	if ((table_end < table_offset) || (table_offset >= (UINT64)footer_pos) ||
		(table_end > (UINT64)footer_pos))
		return NULL;

	if (vpcvhd_ranges_overlap(data_offset, sizeof(ddh), 0, VPCVHD_SECTOR_SIZE) ||
		vpcvhd_ranges_overlap(table_offset, table_region_size, 0, VPCVHD_SECTOR_SIZE) ||
		vpcvhd_ranges_overlap(data_offset, sizeof(ddh), table_offset, table_region_size))
		return NULL;

	bat_bytes64 = required_blocks * 4;
	if ((bat_bytes64 > 0xffffffffU) ||
		(bat_bytes64 > table_region_size) ||
		(bat_bytes64 > (UINT64)(size_t)-1))
		return NULL;

	alloc_size = (UINT64)bitmap_size + block_size;
	if ((alloc_size < bitmap_size) || ((alloc_size & (VPCVHD_SECTOR_SIZE - 1)) != 0))
		return NULL;
	block_span_sectors = alloc_size / VPCVHD_SECTOR_SIZE;
	if ((block_span_sectors == 0) || (block_span_sectors >= VPCVHD_BAT_UNUSED))
		return NULL;

	vhd = (VPCVHDDYNAMIC *)_MALLOC(sizeof(*vhd), "vpcvhd_dynamic");
	if (vhd == NULL)
		return NULL;
	ZeroMemory(vhd, sizeof(*vhd));
	vhd->fh = FILEH_INVALID;
	vhd->bat_entries = (UINT32)required_blocks;
	vhd->bat = (UINT32 *)_MALLOC((size_t)bat_bytes64, "vpcvhd_bat");
	vhd->bitmap = (UINT8 *)_MALLOC(bitmap_size, "vpcvhd_bitmap");
	raw_bat = (UINT8 *)_MALLOC((size_t)bat_bytes64, "vpcvhd_bat_raw");
	if ((vhd->bat == NULL) || (vhd->bitmap == NULL) || (raw_bat == NULL)) {
		if (raw_bat != NULL)
			_MFREE(raw_bat);
		vpcvhd_dynamic_free(vhd);
		return NULL;
	}
	if (vpcvhd_read_at(fh, table_offset, raw_bat, (UINT)bat_bytes64) != SUCCESS) {
		_MFREE(raw_bat);
		vpcvhd_dynamic_free(vhd);
		return NULL;
	}
	for (i = 0; i < vhd->bat_entries; ++i)
		vhd->bat[i] = LOADBEDWORD(raw_bat + (size_t)i * 4);

	allocated_count = 0;
	for (i = 0; i < vhd->bat_entries; ++i) {
		UINT64 block_pos;
		UINT64 block_end;

		if (vhd->bat[i] == VPCVHD_BAT_UNUSED)
			continue;
		block_pos = (UINT64)vhd->bat[i] * VPCVHD_SECTOR_SIZE;
		block_end = block_pos + alloc_size;
		if ((block_end < block_pos) || (block_end > (UINT64)footer_pos) ||
			vpcvhd_ranges_overlap(block_pos, alloc_size, 0, VPCVHD_SECTOR_SIZE) ||
			vpcvhd_ranges_overlap(block_pos, alloc_size, data_offset, sizeof(ddh)) ||
			vpcvhd_ranges_overlap(block_pos, alloc_size, table_offset, table_region_size)) {
			_MFREE(raw_bat);
			vpcvhd_dynamic_free(vhd);
			return NULL;
		}
		((UINT32 *)raw_bat)[allocated_count++] = vhd->bat[i];
	}

	if (allocated_count > 1) {
		UINT32 *sorted;

		sorted = (UINT32 *)raw_bat;
		qsort(sorted, allocated_count, sizeof(sorted[0]), vpcvhd_compare_bat_entry);
		for (i = 1; i < allocated_count; ++i) {
			if ((UINT64)sorted[i] < (UINT64)sorted[i - 1] + block_span_sectors) {
				_MFREE(raw_bat);
				vpcvhd_dynamic_free(vhd);
				return NULL;
			}
		}
	}
	_MFREE(raw_bat);

	CopyMemory(&vhd->footer, footer, sizeof(vhd->footer));
	vhd->footer_pos = footer_pos;
	vhd->footer_len = footer_len;
	vhd->current_size = current_size;
	vhd->table_offset = table_offset;
	vhd->block_size = block_size;
	vhd->sectors_per_block = sectors_per_block;
	vhd->bitmap_size = bitmap_size;
	vhd->bitmap_block = VPCVHD_BITMAP_CACHE_INVALID;
	vhd->bitmap_dirty = 0;
	vhd->write_failed = 0;
	vhd->fh = fh;

	if (footer_recovered &&
		((UINT64)file_len > (UINT64)footer_pos + footer_len)) {
		UINT64 recovered_len;

		recovered_len = (UINT64)footer_pos + footer_len;
		if (((UINT64)(FILELEN)recovered_len != recovered_len) ||
			(file_setsize(fh, (FILELEN)recovered_len) != 0) ||
			(vpcvhd_sync(fh) != SUCCESS)) {
			vhd->write_failed = 1;
			TRACEOUT(("vpc_vhd: recovery mounted read-only; trailing cleanup failed"));
		}
		else {
			TRACEOUT(("vpc_vhd: removed incomplete trailing footer append"));
		}
	}
	return vhd;
}

static BRESULT vpcvhd_dynamic_reopen(SXSIDEV sxsi)
{
	FILEH fh;
	VPCVHDFOOTER footer;
	VPCVHD_FPOS footer_pos;
	VPCVHD_FPOS file_len;
	UINT footer_len;
	UINT8 footer_recovered;
	VPCVHDDYNAMIC *vhd;

	fh = file_open(sxsi->fname);
	if (fh == FILEH_INVALID)
		return FAILURE;
	if (vpcvhd_read_footer_recover(fh, &footer, &footer_pos, &footer_len, &file_len,
		&footer_recovered) != SUCCESS) {
		file_close(fh);
		return FAILURE;
	}
	if ((LOADBEDWORD(footer.FileFormatVersion) != 0x00010000U) ||
		(LOADBEDWORD(footer.DiskType) != VPCVHD_DISK_DYNAMIC)) {
		file_close(fh);
		return FAILURE;
	}
	vhd = vpcvhd_dynamic_open_file(fh, &footer, footer_pos, footer_len, file_len,
		footer_recovered);
	if (vhd == NULL) {
		file_close(fh);
		return FAILURE;
	}
	sxsi->hdl = (INTPTR)vhd;
	return SUCCESS;
}

static REG8 vpcvhd_dynamic_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size)
{
	VPCVHDDYNAMIC *vhd;
	UINT64 byte_pos;

	if (sxsi_prepare(sxsi) != SUCCESS)
		return 0x60;
	if ((pos < 0) || ((UINT64)pos >= (UINT64)sxsi->totals))
		return 0x40;
	vhd = (VPCVHDDYNAMIC *)sxsi->hdl;
	if (vhd == NULL)
		return 0x60;
	byte_pos = (UINT64)pos * sxsi->size;
	CPU_REMCLOCK -= size;
	return (vpcvhd_dynamic_read_bytes(vhd, byte_pos, buf, size) == SUCCESS) ? 0x00 : 0xd0;
}

static REG8 vpcvhd_dynamic_write(SXSIDEV sxsi, FILEPOS pos, const UINT8 *buf, UINT size)
{
	VPCVHDDYNAMIC *vhd;
	UINT64 byte_pos;

	if (sxsi_prepare(sxsi) != SUCCESS)
		return 0x60;
	if ((pos < 0) || ((UINT64)pos >= (UINT64)sxsi->totals))
		return 0x40;
	vhd = (VPCVHDDYNAMIC *)sxsi->hdl;
	if (vhd == NULL)
		return 0x60;
	byte_pos = (UINT64)pos * sxsi->size;
	CPU_REMCLOCK -= size;
	return (vpcvhd_dynamic_write_bytes(vhd, byte_pos, buf, size) == SUCCESS) ? 0x00 : 0x70;
}

static REG8 vpcvhd_dynamic_format(SXSIDEV sxsi, FILEPOS pos)
{
	UINT16 i;
	UINT8 work[VPCVHD_SECTOR_SIZE];
	REG8 result;

	FillMemory(work, sizeof(work), 0xe5);
	for (i = 0; i < sxsi->sectors; ++i) {
		result = vpcvhd_dynamic_write(sxsi, pos + i, work, sxsi->size);
		if (result != 0x00)
			return result;
	}
	return 0x00;
}

static void vpcvhd_dynamic_close(SXSIDEV sxsi)
{
	VPCVHDDYNAMIC *vhd;

	vhd = (VPCVHDDYNAMIC *)sxsi->hdl;
	sxsi->hdl = (INTPTR)0;
	vpcvhd_dynamic_free(vhd);
}

BRESULT sxsihdd_vpcvhd_mount(SXSIDEV sxsi, FILEH fh)
{
	VPCVHDFOOTER footer;
	VPCVHD_FPOS footer_pos;
	VPCVHD_FPOS vhdlen;
	UINT footerlen;
	UINT32 disktype;
	UINT32 formatversion;
	UINT32 surfaces;
	UINT32 cylinders;
	UINT32 sectors;
	UINT64 totals;
	UINT8 footer_recovered;

	if (vpcvhd_read_footer_recover(fh, &footer, &footer_pos, &footerlen, &vhdlen,
		&footer_recovered) != SUCCESS)
		return FAILURE;

	formatversion = LOADBEDWORD(footer.FileFormatVersion);
	disktype = LOADBEDWORD(footer.DiskType);
	sectors = footer.SectorsPerCylinder;
	surfaces = footer.Heads;
	cylinders = LOADBEWORD(footer.Cylinder);
	if (formatversion != 0x00010000U) {
		TRACEOUT(("vpc_vhd: unsupported vhd format version"));
		return FAILURE;
	}
	if ((disktype != VPCVHD_DISK_FIXED) && (disktype != VPCVHD_DISK_DYNAMIC)) {
		TRACEOUT(("vpc_vhd: unsupported vhd image type %u", (unsigned)disktype));
		return FAILURE;
	}
#ifdef SUPPORT_NVL_IMAGES
	// NVL.DLLが使えるならそちらを使う
	if ((disktype == VPCVHD_DISK_DYNAMIC) && nvl_check()) {
		TRACEOUT(("vpc_vhd: dynamic image delegated to NVL.DLL"));
		return FAILURE;
	}
#endif
	if ((sectors == 0) || (surfaces == 0) || (cylinders == 0))
		return FAILURE;

	totals = (UINT64)sectors * surfaces * cylinders;

	/* QEMU hack for old fixed images with CHS rounded one cylinder up. */
	if ((disktype == VPCVHD_DISK_FIXED) &&
		!memcmp(footer.CreatorApplication, CookieVPCQEMUCreator, sizeof(footer.CreatorApplication)) &&
		(totals * VPCVHD_SECTOR_SIZE + footerlen > (UINT64)vhdlen)) {
		--cylinders;
		totals = (UINT64)sectors * surfaces * cylinders;
	}

	if (disktype == VPCVHD_DISK_FIXED) {
		// 容量固定
		if (footer_recovered)
			return FAILURE;
		if (totals * VPCVHD_SECTOR_SIZE + footerlen > (UINT64)vhdlen)
			return FAILURE;
#if defined(SUPPORT_LARGE_HDD)
		sxsi->totals = totals;
#else
		sxsi->totals = (long)totals;
		if ((UINT64)(sxsi->totals) != totals) {
			sxsi->totals = 0;
			return FAILURE;
		}
#endif
		/* the simplest way: reuse built-in methods for fixed image */
		sxsi->reopen = hdd_reopen;
		sxsi->read = hdd_read;
		sxsi->write = hdd_write;
		sxsi->format = hdd_format;
		sxsi->close = hdd_close;

		sxsi->hdl = (INTPTR)fh;
		sxsi->cylinders = (UINT16)cylinders;
		sxsi->size = VPCVHD_SECTOR_SIZE;
		sxsi->sectors = (UINT8)sectors;
		sxsi->surfaces = (UINT8)surfaces;
		sxsi->headersize = 0;
		sxsi->mediatype = gethddtype(sxsi);
		file_seek(fh, 0, FSEEK_SET);
		TRACEOUT(("vpc_vhd: vhd fixed image mounted (c=%u h=%u s=%u %luMbytes)",
			(unsigned)cylinders, (unsigned)surfaces, (unsigned)sectors,
			(unsigned long)(totals / 2U / 1024U)));
		return SUCCESS;
	}
	else {
		// 容量可変
		VPCVHDDYNAMIC *vhd;
		UINT64 current_size;
		UINT64 current_totals;

		current_size = LOADBEQWORD(footer.CurrentSize);
		if ((current_size == 0) || ((current_size & (VPCVHD_SECTOR_SIZE - 1)) != 0))
			return FAILURE;
		current_totals = current_size / VPCVHD_SECTOR_SIZE;
#if defined(SUPPORT_LARGE_HDD)
		sxsi->totals = (FILELEN)current_totals;
#else
		sxsi->totals = (long)current_totals;
		if ((UINT64)sxsi->totals != current_totals) {
			sxsi->totals = 0;
			return FAILURE;
		}
#endif
		vhd = vpcvhd_dynamic_open_file(fh, &footer, footer_pos, footerlen, vhdlen,
			footer_recovered);
		if (vhd == NULL)
			return FAILURE;
		sxsi->reopen = vpcvhd_dynamic_reopen;
		sxsi->read = vpcvhd_dynamic_read;
		sxsi->write = vpcvhd_dynamic_write;
		sxsi->format = vpcvhd_dynamic_format;
		sxsi->close = vpcvhd_dynamic_close;

		sxsi->hdl = (INTPTR)vhd;
		sxsi->cylinders = (UINT16)cylinders;
		sxsi->size = VPCVHD_SECTOR_SIZE;
		sxsi->sectors = (UINT8)sectors;
		sxsi->surfaces = (UINT8)surfaces;
		sxsi->headersize = 0;
		sxsi->mediatype = gethddtype(sxsi);
		TRACEOUT(("vpc_vhd: vhd dynamic image mounted (c=%u h=%u s=%u block=%luK %luMbytes)",
			(unsigned)cylinders, (unsigned)surfaces, (unsigned)sectors,
			(unsigned long)(vhd->block_size / 1024U),
			(unsigned long)(current_totals / 2U / 1024U)));
		return SUCCESS;
	}
}
