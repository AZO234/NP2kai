#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	<mem/memegc.h>
#include	<vram/vram.h>

// PEGC �v���[�����[�h
// �֘A: vram.c, vram.h, memvga.c, memvga.h
//
// �����Q�l: 
//   MAME PEGC (BSD-3-Clause) https://github.com/mamedev/mame/blob/master/src/mame/nec/pc9821.cpp
//   SL9821 �Z�p�I�Ȃ͂Ȃ� - PEGC https://www.satotomi.com/sl9821/sl9821_tec5.html


#if 0
#undef  TRACEOUT
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
#define TRACEOUT(s) trace_fmt_ex s
#endif

#ifdef SUPPORT_PEGC

#define PEGC_VRAM_MASK		0x7ffff
#define PEGC_SHIFTBUF_SIZE	64

// �]�������}�X�N
#define PEGC_REMAIN_COUNT_MASK	0x1fffUL

// �ŏ���READ/WRITE���ǂ����𔻒f����t���O
#define PEGC_FIRST_READ_FLAG	0x40UL
#define PEGC_FIRST_WRITE_FLAG	0x80UL

static UINT32 pegc_shift_count(void) {
	return (UINT32)pegc.lastdatalen;
}

static void pegc_set_shift_count(UINT32 count) {
	if (count > PEGC_SHIFTBUF_SIZE) {
		count = PEGC_SHIFTBUF_SIZE;
	}
	pegc.lastdatalen = (UINT16)count;
}

static UINT pegc_pattern_mask(void) {
	return (((UINT32)pegc.databit32) ? 31 : 15);
}

static void pegc_set_pattern_width(UINT width) {
	pegc.databit32 = (UINT16)((width > 16) ? 1 : 0);
}

static UINT32 pegc_transfer_count(void) {
	return (pegc.remain & PEGC_REMAIN_COUNT_MASK);
}

static void pegc_set_transfer_count(UINT32 count) {
	pegc.remain = count & PEGC_REMAIN_COUNT_MASK;
}

static int pegc_first_read(void) {
	return ((pegc.flags & PEGC_FIRST_READ_FLAG) != 0);
}

static int pegc_first_write(void) {
	return ((pegc.flags & PEGC_FIRST_WRITE_FLAG) != 0);
}

static void pegc_clear_first_read(void) {
	pegc.flags &= ~PEGC_FIRST_READ_FLAG;
}

static void pegc_clear_first_write(void) {
	pegc.flags &= ~PEGC_FIRST_WRITE_FLAG;
}

void pegc_transfer_reset(void) {
	pegc.remain = 0;
	pegc_clear_first_read();
	pegc_clear_first_write();
	pegc_set_shift_count(0);
}

static void pegc_transfer_begin(void) {
	UINT32 length;

	if (pegc_transfer_count() != 0) {
		return;
	}
	length = (LOADINTELWORD(vramop.mio2 + PEGC_REG_LENGTH) & 0x0fff) + 1;
	pegc.remain = length;
	pegc.flags |= PEGC_FIRST_READ_FLAG | PEGC_FIRST_WRITE_FLAG;
	pegc_set_shift_count(0);
}

static void pegc_transfer_finish(void) {
	pegc.remain = 0;
	pegc_clear_first_read();
	pegc_clear_first_write();
	pegc_set_shift_count(0);
}

static void pegc_shift_push(UINT8 value) {
	UINT count;
	UINT i;

	count = pegc_shift_count();
	if (count >= PEGC_SHIFTBUF_SIZE) {
		// PEGC_SHIFTBUF_SIZE�𒴂������ԌÂ����̂�j��
		for (i = 1; i < PEGC_SHIFTBUF_SIZE; i++) {
			pegc.lastdata[i - 1] = pegc.lastdata[i];
		}
		count = PEGC_SHIFTBUF_SIZE - 1;
	}
	pegc.lastdata[count++] = value;
	pegc_set_shift_count(count);
}

static void pegc_shift_consume(UINT count) {
	UINT length;
	UINT i;

	length = pegc_shift_count();
	if (count >= length) {
		pegc_set_shift_count(0);
		return;
	}
	for (i = count; i < length; i++) {
		pegc.lastdata[i - count] = pegc.lastdata[i];
	}
	pegc_set_shift_count(length - count);
}

static UINT32 pegc_pattern_plane(UINT plane) {
	return LOADINTELDWORD(vramop.mio2 + PEGC_REG_PATTERN + plane * 4);
}

static void pegc_set_pattern_plane(UINT plane, UINT32 value) {
	STOREINTELDWORD(vramop.mio2 + PEGC_REG_PATTERN + plane * 4, value);
}

static UINT8 pegc_pattern_pixel_raw(UINT pixel) {
	UINT plane;
	UINT8 color;
	UINT32 bit;

	if (pixel >= 32) {
		return 0;
	}
	bit = 1UL << pixel;
	color = 0;
	for (plane = 0; plane < 8; plane++) {
		if (pegc_pattern_plane(plane) & bit) {
			color |= (UINT8)(1 << plane);
		}
	}
	return color;
}

static UINT8 pegc_pattern_pixel(UINT pixel) {
	return pegc_pattern_pixel_raw(pixel & pegc_pattern_mask());
}

static void pegc_set_pattern_pixel_raw(UINT pixel, UINT8 color) {
	UINT plane;
	UINT32 data;
	UINT32 bit;

	if (pixel >= 32) {
		return;
	}
	bit = 1UL << pixel;
	for (plane = 0; plane < 8; plane++) {
		data = pegc_pattern_plane(plane);
		if (color & (1 << plane)) {
			data |= bit;
		}
		else {
			data &= ~bit;
		}
		pegc_set_pattern_plane(plane, data);
	}
}

static void pegc_set_pattern_pixel(UINT pixel, UINT8 color) {
	pegc_set_pattern_pixel_raw(pixel & pegc_pattern_mask(), color);
}

REG8 pegc_pattern_rd8(UINT pos) {
	UINT rel;
	UINT index;
	UINT bytepos;
	UINT32 data;

	if (pos < PEGC_REG_PATTERN) {
		return 0;
	}
	rel = pos - PEGC_REG_PATTERN;
	if (LOADINTELWORD(vramop.mio2 + PEGC_REG_PLANE_ROP) & 0x8000) {
		// �s�N�Z����
		if ((rel & 3) != 0) {
			return 0;
		}
		index = rel >> 2;
		if (index > 31) {
			return 0;
		}
		return pegc_pattern_pixel_raw(index);
	}
	else {
		// �v���[����
		index = rel >> 2;
		bytepos = rel & 3;
		if (index >= 8) {
			return 0;
		}
		if ((pegc_pattern_mask() == 15) && (bytepos >= 2)) {
			return 0;
		}
		data = pegc_pattern_plane(index);
		return (REG8)(data >> (bytepos * 8));
	}
}

void pegc_pattern_wr8(UINT pos, REG8 value) {
	UINT rel;
	UINT index;
	UINT bytepos;
	UINT32 data;
	UINT32 mask;

	if (pos < PEGC_REG_PATTERN) {
		return;
	}
	rel = pos - PEGC_REG_PATTERN;
	if (LOADINTELWORD(vramop.mio2 + PEGC_REG_PLANE_ROP) & 0x8000) {
		if ((rel & 3) != 0) {
			return;
		}
		index = rel >> 2;
		if (index > 31) {
			return;
		}
		pegc_set_pattern_pixel_raw(index, value);
		return;
	}

	index = rel >> 2;
	bytepos = rel & 3;
	if (index >= 8) {
		return;
	}
	if (bytepos >= 2) {
		pegc_set_pattern_width(32);
	}
	data = pegc_pattern_plane(index);
	mask = 0xffUL << (bytepos * 8);
	data = (data & ~mask) | ((UINT32)value << (bytepos * 8));
	pegc_set_pattern_plane(index, data);
}

void pegc_pattern_wr16(UINT pos, REG16 value) {
	UINT rel;
	UINT index;
	UINT32 data;

	if (pos < PEGC_REG_PATTERN) {
		return;
	}
	rel = pos - PEGC_REG_PATTERN;
	if (LOADINTELWORD(vramop.mio2 + PEGC_REG_PLANE_ROP) & 0x8000) {
		if ((rel & 3) != 0) {
			return;
		}
		index = rel >> 2;
		if (index > 31) {
			return;
		}
		pegc_set_pattern_pixel_raw(index, (UINT8)value);
		return;
	}

	if ((rel & 3) != 0) {
		return;
	}
	index = rel >> 2;
	if (index >= 8) {
		return;
	}
	pegc_set_pattern_width(16);
	data = pegc_pattern_plane(index);
	data = (data & 0xffff0000UL) | value;
	pegc_set_pattern_plane(index, data);
}

void pegc_pattern_wr32(UINT pos, UINT32 value) {
	UINT rel;
	UINT index;

	if (pos < PEGC_REG_PATTERN) {
		return;
	}
	rel = pos - PEGC_REG_PATTERN;
	if ((rel & 3) != 0) {
		return;
	}
	index = rel >> 2;
	if (LOADINTELWORD(vramop.mio2 + PEGC_REG_PLANE_ROP) & 0x8000) {
		if (index > 31) {
			return;
		}
		pegc_set_pattern_pixel_raw(index, (UINT8)value);
		return;
	}
	if (index >= 8) {
		return;
	}
	pegc_set_pattern_width(32);
	pegc_set_pattern_plane(index, value);
}

static int pegc_plane_base_address(UINT32 address, UINT32 *addr) {
	UINT32 result;

	if ((address < 0xa8000) || (address >= 0xb8000)) {
		return 0;
	}

	// 1��ʃ��[�h�Ȃ�A8000h�`B0000h�`
	// 2��ʃ��[�h�Ȃ�A8000h�̂ݗL���AGDC I/O A6h�ŕ`��y�[�W��I��
	if (!(gdc.analog & (1 << GDCANALOG_256E))) {
		if (address >= 0xb0000) {
			return 0;
		}
		result = (address - 0xa8000) * 8;
		if (gdcs.access) {
			result += 0x40000;
		}
	}
	else {
		result = (address - 0xa8000) * 8;
	}
	*addr = result & PEGC_VRAM_MASK;
	return 1;
}

static UINT32 pegc_pixel_address(UINT32 base, UINT width, UINT shift,
								UINT index, int decrement) {
	UINT32 addr;

	if (decrement) {
		addr = base + width - 1 - shift - index;
	}
	else {
		addr = base + shift + index;
	}
	return (addr & PEGC_VRAM_MASK);
}

static UINT32 pegc_data_bit(UINT pixel) {
	return 1UL << (((pixel >> 3) << 3) + (7 - (pixel & 7)));
}

static UINT8 pegc_rop(UINT8 ropcode, UINT8 src, UINT8 dst,
					 UINT8 pat1, UINT8 pat2) {
	UINT8 result;

	result = 0;
	if (ropcode & 0x80) result |= src & dst & pat1;
	if (ropcode & 0x40) result |= src & dst & (UINT8)~pat1;
	if (ropcode & 0x20) result |= src & (UINT8)~dst & pat1;
	if (ropcode & 0x10) result |= src & (UINT8)~dst & (UINT8)~pat1;
	if (ropcode & 0x08) result |= (UINT8)~src & dst & pat2;
	if (ropcode & 0x04) result |= (UINT8)~src & dst & (UINT8)~pat2;
	if (ropcode & 0x02) result |= (UINT8)~src & (UINT8)~dst & pat2;
	if (ropcode & 0x01) result |= (UINT8)~src & (UINT8)~dst & (UINT8)~pat2;
	return result;
}

static UINT32 pegc_memvgaplane_read(UINT32 address, UINT width) {
	UINT i;
	UINT read_count;
	UINT src_shift;
	UINT32 base;
	UINT32 addr;
	UINT32 ret;
	UINT16 rop;
	UINT8 data;
	UINT8 plane_mask;
	UINT8 palette1;
	int cpu_data;
	int decrement;
	int pattern_update;
	int compare_enable;

	ret = 0;
	if (!pegc_plane_base_address(address, &base)) {
		return 0;
	}

	rop = LOADINTELWORD(vramop.mio2 + PEGC_REG_PLANE_ROP);
	cpu_data = ((rop & 0x0100) != 0);
	decrement = ((rop & 0x0200) != 0);
	pattern_update = ((rop & 0x2000) != 0) && !cpu_data;
	compare_enable = ((LOADINTELWORD(vramop.mio2 + PEGC_REG_DATASELECT) & 1) != 0) && !cpu_data;
	plane_mask = vramop.mio2[PEGC_REG_PLANE_ACCESS];
	palette1 = vramop.mio2[PEGC_REG_PALETTE1];

	for (i = 0; i < width; i++) {
		addr = pegc_pixel_address(base, width, 0, i, decrement);
		data = vramex[addr];
		if (compare_enable && (((data ^ palette1) & (UINT8)~plane_mask) == 0)) {
			ret |= 1UL << i;
		}
		if (pattern_update) {
			if (i == 0) {
				pegc_set_pattern_width(width);
			}
			pegc_set_pattern_pixel(addr & pegc_pattern_mask(), data);
		}
	}

	if (!cpu_data) {
		pegc_transfer_begin();
		src_shift = pegc_first_read() ?
			(LOADINTELWORD(vramop.mio2 + PEGC_REG_SHIFT) & (width - 1)) : 0;
		TRACEOUT((
			"PEGC RD adr=%08x width=%u base=%08x "
			"ROP=%04x SHIFT=%04x LEN=%04x "
			"dec=%d srcshift=%u firstR=%d "
			"remain=%u shcnt=%u",
			address,
			width,
			base,
			rop,
			LOADINTELWORD(vramop.mio2 + PEGC_REG_SHIFT),
			LOADINTELWORD(vramop.mio2 + PEGC_REG_LENGTH),
			decrement,
			src_shift,
			pegc_first_read(),
			pegc_transfer_count(),
			pegc_shift_count()
			));
		read_count = width - src_shift;
		for (i = 0; i < read_count; i++) {
			addr = pegc_pixel_address(base, width, src_shift, i, decrement);
			pegc_shift_push(vramex[addr]);
		}
		TRACEOUT((
			"PEGC RD-END adr=%08x pushed=%u remain=%u shcnt=%u",
			address,
			read_count,
			pegc_transfer_count(),
			pegc_shift_count()
			));
		pegc_clear_first_read();
	}
	return ret;
}

static void pegc_mark_vram(UINT32 addr, UINT8 *pages) {
	UINT8 bit;

	bit = (addr & 0x40000) ? 2 : 1;
	vramupdate[LOW15(addr >> 3)] |= bit;
	*pages |= bit;
}

static void pegc_memvgaplane_write(UINT32 address, UINT32 value, UINT width) {
	UINT i;
	UINT push_count;
	UINT process_count;
	UINT src_shift;
	UINT dst_shift;
	UINT shift_count;
	UINT32 base;
	UINT32 addr;
	UINT32 bitmask;
	UINT32 remain;
	UINT32 pixel_mask;
	UINT16 rop;
	UINT8 ropcode;
	UINT8 ropmethod;
	UINT8 plane_mask;
	UINT8 palette1;
	UINT8 palette2;
	UINT8 src;
	UINT8 dst;
	UINT8 pat1;
	UINT8 pat2;
	UINT8 result;
	UINT8 pages;
	int cpu_data;
	int decrement;
	int rop_enable;

	if (!pegc_plane_base_address(address, &base)) {
		return;
	}

	rop = LOADINTELWORD(vramop.mio2 + PEGC_REG_PLANE_ROP);
	ropcode = (UINT8)rop;
	ropmethod = (UINT8)((rop >> 10) & 3);
	rop_enable = ((rop & 0x1000) != 0);
	cpu_data = ((rop & 0x0100) != 0);
	decrement = ((rop & 0x0200) != 0);
	plane_mask = vramop.mio2[PEGC_REG_PLANE_ACCESS];
	palette1 = vramop.mio2[PEGC_REG_PALETTE1];
	palette2 = vramop.mio2[PEGC_REG_PALETTE2];
	pixel_mask = LOADINTELDWORD(vramop.mio2 + PEGC_REG_MASK);
	pages = 0;

	pegc_transfer_begin();
	dst_shift = pegc_first_write() ?
		((LOADINTELWORD(vramop.mio2 + PEGC_REG_SHIFT) >> 8) & (width - 1)) : 0;
	TRACEOUT((
		"PEGC WR adr=%08x width=%u base=%08x "
		"ROP=%04x SHIFT=%04x LEN=%04x "
		"dec=%d dstshift=%u firstW=%d "
		"remain=%u shcnt=%u",
		address,
		width,
		base,
		rop,
		LOADINTELWORD(vramop.mio2 + PEGC_REG_SHIFT),
		LOADINTELWORD(vramop.mio2 + PEGC_REG_LENGTH),
		decrement,
		dst_shift,
		pegc_first_write(),
		pegc_transfer_count(),
		pegc_shift_count()
		));
	if (cpu_data) {
		src_shift = pegc_first_read() ?
			(LOADINTELWORD(vramop.mio2 + PEGC_REG_SHIFT) & (width - 1)) : 0;
		push_count = width - src_shift;
		for (i = 0; i < push_count; i++) {
			bitmask = pegc_data_bit(i + src_shift);
			pegc_shift_push((value & bitmask) ? 0xff : 0x00);
		}
		pegc_clear_first_read();
	}

	process_count = width - dst_shift;
	remain = pegc_transfer_count();
	shift_count = pegc_shift_count();
	if (process_count > remain) {
		process_count = (UINT)remain;
	}

	// �f�[�^������Ȃ��ꍇ��nop
	if (shift_count < process_count) {
		TRACEOUT((
			"PEGC WR-UNDERFLOW adr=%08x need=%u available=%u "
			"remain=%u dstshift=%u",
			address,
			process_count,
			shift_count,
			remain,
			dst_shift
			));
		return;
	}

	for (i = 0; i < process_count; i++) {
		addr = pegc_pixel_address(base, width, dst_shift, i, decrement);
		bitmask = pegc_data_bit(i + dst_shift);
		if (pixel_mask & bitmask) {
			src = pegc.lastdata[i];
			dst = vramex[addr];
			if (rop_enable) {
				switch (ropmethod) {
				case 0:
					pat1 = pegc_pattern_pixel(addr);
					pat2 = pat1;
					break;
				case 1:
					pat1 = palette2;
					pat2 = palette2;
					break;
				case 2:
					pat1 = palette1;
					pat2 = palette1;
					break;
				default:
					pat1 = palette1;
					pat2 = palette2;
					break;
				}
				result = pegc_rop(ropcode, src, dst, pat1, pat2);
				vramex[addr] = (dst & plane_mask) | (result & (UINT8)~plane_mask);
			}
			else {
				vramex[addr] = (dst & plane_mask) | (src & (UINT8)~plane_mask);
			}
			pegc_mark_vram(addr, &pages);
		}
		remain--;
	}

	pegc_shift_consume(process_count);
	pegc_clear_first_write();
	if (remain == 0) {
		pegc_transfer_finish();
	}
	else {
		pegc_set_transfer_count(remain);
	}

	TRACEOUT((
		"PEGC WR-END adr=%08x process=%u remain=%u shcnt=%u",
		address,
		process_count,
		pegc_transfer_count(),
		pegc_shift_count()
		));

	if (pages) {
		gdcs.grphdisp |= pages;
	}
}

REG16 MEMCALL pegc_memvgaplane_rd16(UINT32 address) {
	return (REG16)pegc_memvgaplane_read(address, 16);
}

void MEMCALL pegc_memvgaplane_wr16(UINT32 address, REG16 value) {
	pegc_memvgaplane_write(address, value, 16);
}

UINT32 MEMCALL pegc_memvgaplane_rd32(UINT32 address) {
	return pegc_memvgaplane_read(address, 32);
}

void MEMCALL pegc_memvgaplane_wr32(UINT32 address, UINT32 value) {
	pegc_memvgaplane_write(address, value, 32);
}

void pegc_reset(const NP2CFG *pConfig) {
	ZeroMemory(&pegc, sizeof(pegc));
	pegc.enable = np2cfg.usepegcplane;
	pegc_set_pattern_width(16);
	(void)pConfig;
}

void pegc_bind(void) {
}

#endif
