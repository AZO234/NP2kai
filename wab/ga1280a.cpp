/**
 * @file    ga1280a.cpp
 * @brief   Implementation of the I-O DATA GA-1280A
 *
 * This file is a C++ port of the Neetan GA-1280A.
 * https://github.com/neetandev/neetan
 */

#include    "compiler.h"

#if defined(SUPPORT_WAB_GA1280A)

#include    <vector>
#include    <algorithm>

#include    "pccore.h"
#include    "wab.h"
#include	"statsave.h"
#ifdef __cplusplus
extern "C" {
#endif
    int ga1280a_sfsave(STFLAGH sfh, const SFENTRY* tbl);
    int ga1280a_sfload(STFLAGH sfh, const SFENTRY* tbl);
#ifdef __cplusplus
}
#endif
#include    "dosio.h"
#include    "cpucore.h"
#include    "cpumem.h"
#include    "iocore.h"
#include    "soundmng.h"

#if defined(SUPPORT_IA32_HAXM)
#include "i386hax/haxfunc.h"
#include "i386hax/haxcore.h"
#endif

#include    "ga1280adef.h"
#include    "ga1280a.h"

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

#define GA1280A_MEMWAIT 16
#define GA1280A_MEMWAIT_LINE 32

GA1280A     ga1280a;

static GA1280A_STATE s_ga;
static GA1280A_MMIOCache s_mmio;
static std::vector<LinePoint> s_line_points;
static std::vector<UINT32> s_framebuf;

static bool s_ga1280a_memp_map_registered;

static void ga1280a_unregister_memp_map(void)
{
    if (!s_ga1280a_memp_map_registered) return;
    memp_mmio_range_remove(GA1280A_CONVENTIONAL_WINDOW_BASE,
        GA1280A_CONVENTIONAL_WINDOW_END - GA1280A_CONVENTIONAL_WINDOW_BASE);
    memp_mmio_range_remove(GA1280A_FLAT_APERTURE_BASE,
        GA1280A_FLAT_APERTURE_END - GA1280A_FLAT_APERTURE_BASE);
    s_ga1280a_memp_map_registered = false;
}

static void ga1280a_update_memp_map(void)
{
    ga1280a_unregister_memp_map();
    if (ga1280a.enabled) {
        memp_mmio_range_add(GA1280A_CONVENTIONAL_WINDOW_BASE,
            GA1280A_CONVENTIONAL_WINDOW_END - GA1280A_CONVENTIONAL_WINDOW_BASE);
        memp_mmio_range_add(GA1280A_FLAT_APERTURE_BASE,
            GA1280A_FLAT_APERTURE_END - GA1280A_FLAT_APERTURE_BASE);
        s_ga1280a_memp_map_registered = true;
    }
}

static UINT32 clampu32(UINT32 value, UINT32 minv, UINT32 maxv)
{
    if (value < minv) return minv;
    if (value > maxv) return maxv;
    return value;
}

static UINT32 divceil(UINT32 value, UINT32 divisor)
{
    return divisor ? ((value + divisor - 1) / divisor) : 0;
}

static UINT32 align_up(UINT32 value, UINT32 alignment)
{
    if (alignment <= 1) return value;
    return ((value + alignment - 1) / alignment) * alignment;
}

static void rebuild_mmio_cache(void);
static UINT32 mask_for_active_color(void);
static bool indexed8_high_color_context(void);
static bool indexed8_16_color_context(void);
static bool indexed8_plane_page_context(void);
static UINT32 lowbit_shift(UINT32 mask);
static UINT32 logical_mask_from_plane_mask(UINT32 plane_mask);
static UINT32 pack_color_to_plane_mask(UINT32 color, UINT32 plane_mask);
static UINT32 unpack_color_from_plane_mask(UINT32 pixel, UINT32 plane_mask);
static bool indexed8_linear_window_selected(void);
static bool uses_packed_indexed_host_read_pixels(void);
static bool uses_packed_direct16_host_read_pixels(void);
static bool write_bit_mask_allows(UINT32 x);
static UINT32 read_pixel_color(UINT32 x, UINT32 y);
static void write_pixel_mixed(UINT32 x, UINT32 y, UINT32 color, PixelMix mix);
static void write_pixel_rop(UINT32 x, UINT32 y, UINT32 source, UINT8 rop);
static void execute_pop2(UINT16 opcode);
static void write_pdt_word(UINT16 value);
static UINT16 read_pdt_word(void);
static bool read_word(UINT8 selector, UINT8 offset, UINT16* ret);
static bool write_word(UINT8 selector, UINT8 offset, UINT16 value);
static bool read_byte(UINT8 selector, UINT8 offset, UINT8* ret);
static bool write_byte(UINT8 selector, UINT8 offset, UINT8 value);
static bool mapped_register_write_byte(UINT32 offset, UINT8 value);
static bool mapped_register_read_byte(UINT32 offset, UINT8* ret);
static bool mapped_register_write_word(UINT32 offset, UINT16 value);
static bool mapped_register_read_word(UINT32 offset, UINT16* ret);
static void consume_image_restore_indexed_pixel(UINT32 color);
static void consume_image_restore_byte(UINT8 value);
static void consume_image_restore_word(UINT16 value);
static void consume_pattern_expand_word(UINT16 value);
static UINT16 read_pixel_read_word(void);
static UINT32 pixel_map_width(void);
static UINT32 pixel_map_height(void);
static UINT32 bytes_per_pixel(void);
static UINT32 display_start(void);
static UINT32 display_pixels_per_crtc_unit(void);
static UINT32 horizontal_pixels_per_crtc_unit(void);
static void update_dimensions_from_crtc(void);
static void update_plane_mode_after_vdac_index_write(UINT8 value);
static void update_plane_mode_after_vdac_mask_write(UINT8 value);
static UINT8 host_window_read(UINT32 offset);
static void host_window_write(UINT32 offset, UINT8 value);
static UINT8 flat_aperture_read_byte_at_offset(UINT32 offset);
static void flat_aperture_write_byte_at_offset(UINT32 offset, UINT8 value);
static UINT16 flat_aperture_read_word_at_offset(UINT32 offset);
static void flat_aperture_write_word_at_offset(UINT32 offset, UINT16 value);
static UINT32 flat_aperture_read_dword_at_offset(UINT32 offset);
static void flat_aperture_write_dword_at_offset(UINT32 offset, UINT32 value);

static void execute_pattern_expand_rectangle(bool opaque);

// ***** MMIO用

int MEMCALL ga1280a_memp_translate_address(UINT32 address, UINT32* physical)
{
    UINT32 addr;

    if (!ga1280a.enabled) {
        return FALSE;
    }

    addr = address & CPU_ADRSMASK;
    if ((GA1280A_CONVENTIONAL_WINDOW_BASE <= addr) && (addr < GA1280A_CONVENTIONAL_WINDOW_END)) {
        *physical = addr;
        return TRUE;
    }
    if ((GA1280A_FLAT_APERTURE_BASE <= addr) && (addr < GA1280A_FLAT_APERTURE_END)) {
        *physical = addr;
        return TRUE;
    }
    return FALSE;
}

int MEMCALL ga1280a_memp_range_may_hit(UINT32 address, UINT leng)
{
    UINT32 start;
    UINT32 end;

    if (!ga1280a.enabled || !leng) {
        return FALSE;
    }

    start = address & CPU_ADRSMASK;
    end = start + leng - 1;
    if (end < start) {
        return TRUE;
    }

    if ((start < GA1280A_CONVENTIONAL_WINDOW_END) && (end >= GA1280A_CONVENTIONAL_WINDOW_BASE)) {
        return TRUE;
    }
    if ((start < GA1280A_FLAT_APERTURE_END) && (end >= GA1280A_FLAT_APERTURE_BASE)) {
        return TRUE;
    }
    return FALSE;
}

int MEMCALL ga1280a_memp_try_read8(UINT32 address, REG8* value)
{
    UINT32 gaaddr;

    if (!ga1280a_memp_translate_address(address, &gaaddr)) {
        return FALSE;
    }
    return ga1280a_memp_read8(gaaddr, value) ? TRUE : FALSE;
}

int MEMCALL ga1280a_memp_try_read16(UINT32 address, REG16* value)
{
    UINT32 gaaddr;

    if (!ga1280a_memp_translate_address(address, &gaaddr)) {
        return FALSE;
    }
    return ga1280a_memp_read16(gaaddr, value) ? TRUE : FALSE;
}

int MEMCALL ga1280a_memp_try_read32(UINT32 address, UINT32* value)
{
    UINT32 gaaddr;

    if (!ga1280a_memp_translate_address(address, &gaaddr)) {
        return FALSE;
    }
    return ga1280a_memp_read32(gaaddr, value) ? TRUE : FALSE;
}

int MEMCALL ga1280a_memp_try_write8(UINT32 address, REG8 value)
{
    UINT32 gaaddr;

    if (!ga1280a_memp_translate_address(address, &gaaddr)) {
        return FALSE;
    }
    return ga1280a_memp_write8(gaaddr, value) ? TRUE : FALSE;
}

int MEMCALL ga1280a_memp_try_write16(UINT32 address, REG16 value)
{
    UINT32 gaaddr;

    if (!ga1280a_memp_translate_address(address, &gaaddr)) {
        return FALSE;
    }
    return ga1280a_memp_write16(gaaddr, value) ? TRUE : FALSE;
}

int MEMCALL ga1280a_memp_try_write32(UINT32 address, UINT32 value)
{
    UINT32 gaaddr;

    if (!ga1280a_memp_translate_address(address, &gaaddr)) {
        return FALSE;
    }
    return ga1280a_memp_write32(gaaddr, value) ? TRUE : FALSE;
}




static void mark_updated(void)
{
    ga1280a.updated = 1;
}

static void mark_palette_updated(void)
{
    ga1280a.paletteUpdated = 1;
    mark_updated();
}

static void update_public_state(void)
{
    ga1280a.width = s_ga.active_width;
    ga1280a.height = s_ga.active_height;

    if (ga1280a.enabled) {
        REG8 newrelay = (s_ga.mod2 & 0x80) ? 3 : 0;
        if (np2wab.relaystateint != newrelay) {
            np2wab.relaystateint = newrelay;
            np2wab_setRelayState(np2wab.relaystateint | np2wab.relaystateext);
        }
        ga1280a.active = newrelay ? 1 : 0;
    }
}

static void init_state(void)
{
    ZeroMemory(&s_ga, sizeof(s_ga));
    s_ga.gaport = DEFAULT_GAPORT;
    s_ga.clip_ex = DEFAULT_WIDTH - 1;
    s_ga.clip_ey = DEFAULT_HEIGHT - 1;
    s_ga.last_wba1_window_size = WINSIZE_DISABLED;
    s_ga.vdac_mask = 0;
    s_ga.active_width = DEFAULT_WIDTH;
    s_ga.active_height = DEFAULT_HEIGHT;
    s_ga.plane_mode = PLANE_INDEXED8;
    s_ga.stream.kind = STREAM_INACTIVE;
    for (int i = 0; i < ROP_PATTERN_ROWS; i++) s_ga.rop_pattern[i] = 0xff;
    for (int i = 0; i < CURSOR_MASK_BYTES; i++) s_ga.cursor.sepa.cursor_and_pattern[i] = 0xff;
    s_line_points.clear();
    rebuild_mmio_cache();
    update_public_state();
}

static UINT32 window_size_bytes(GAWindowSize size)
{
    switch (size) {
    case WINSIZE_16K: return 16 * 1024;
    case WINSIZE_32K: return 32 * 1024;
    case WINSIZE_64K: return 64 * 1024;
    case WINSIZE_128K: return 128 * 1024;
    default: return 0;
    }
}

static GAWindowSize window_size_from(UINT16 value)
{
    switch ((value >> 8) & 0x00f0) {
    case 0x20: return WINSIZE_16K;
    case 0x30: return WINSIZE_32K;
    case 0x40: return WINSIZE_64K;
    case 0x50: return WINSIZE_128K;
    default: return WINSIZE_DISABLED;
    }
}

static UINT16 window_segment_from(UINT16 value)
{
    if ((value & WBA_LOW_BYTE_SEGMENT_MASK) == 0) {
        return (UINT16)(((value >> 8) & 0x000f) << 12);
    }
    return (UINT16)((value & WBA_LOW_BYTE_SEGMENT_MASK) << 8);
}

static void rebuild_mmio_cache(void)
{
    GAWindowSize wba1_size = window_size_from(s_ga.wba1);
    GAWindowSize wba2_size = window_size_from(s_ga.wba2);
    GAWindowSize host_size = WINSIZE_DISABLED;
    UINT16 host_segment = 0;

    ZeroMemory(&s_mmio, sizeof(s_mmio));

    if (wba1_size != WINSIZE_DISABLED) {
        host_size = wba1_size;
        host_segment = window_segment_from(s_ga.wba1);
    }
    else if (wba2_size != WINSIZE_DISABLED) {
        host_size = wba2_size;
        host_segment = window_segment_from(s_ga.wba2);
    }

    if (host_size != WINSIZE_DISABLED) {
        UINT32 bytes = window_size_bytes(host_size);
        if (bytes != 0) {
            s_mmio.host_window_enabled = true;
            s_mmio.host_window_base = ((UINT32)host_segment) << 4;
            s_mmio.host_window_bytes = bytes;
        }
    }

    if (!s_mmio.host_window_enabled && s_ga.wba1 == 0) {
        UINT32 bytes = window_size_bytes(s_ga.last_wba1_window_size);
        if (bytes != 0) {
            s_mmio.closed_mapped_register_enabled = true;
            s_mmio.closed_mapped_register_base = CONVENTIONAL_WINDOW_BASE;
            s_mmio.closed_mapped_register_mask = bytes - 1;
        }
    }

    if (wba1_size != WINSIZE_DISABLED) {
        s_mmio.mapped_register_window_size = wba1_size;
    }
    else if (s_ga.wba1 == 0) {
        s_mmio.mapped_register_window_size = s_ga.last_wba1_window_size;
    }
    else {
        s_mmio.mapped_register_window_size = WINSIZE_DISABLED;
    }
    s_mmio.mapped_register_window_bytes = window_size_bytes(s_mmio.mapped_register_window_size);
    s_mmio.mapped_register_aperture_enabled =
        (s_ga.mod1 == 2) || (s_mmio.mapped_register_window_size != WINSIZE_DISABLED);

    if ((s_ga.wba1 & WBA_LOW_BYTE_SEGMENT_MASK) == 0) {
        s_mmio.flat_window_bytes = window_size_bytes(wba1_size);
    }

    s_mmio.flat_aperture_enabled = true;
    s_mmio.flat_aperture_base = FLAT_APERTURE_BASE;
    s_mmio.flat_aperture_bytes = FLAT_APERTURE_BYTES;
}

static UINT16 window_segment(void)
{
    if (!s_mmio.host_window_enabled) return window_segment_from(s_ga.wba1);
    return (UINT16)(s_mmio.host_window_base >> 4);
}

static GAWindowSize window_size(void)
{
    if (!s_mmio.host_window_enabled) return WINSIZE_DISABLED;
    switch (s_mmio.host_window_bytes) {
    case 16 * 1024: return WINSIZE_16K;
    case 32 * 1024: return WINSIZE_32K;
    case 64 * 1024: return WINSIZE_64K;
    case 128 * 1024: return WINSIZE_128K;
    default: return WINSIZE_DISABLED;
    }
}

static GAWindowSize closed_wba1_mapped_window_size(void)
{
    return s_mmio.closed_mapped_register_enabled ? s_ga.last_wba1_window_size : WINSIZE_DISABLED;
}

static GAWindowSize mapped_register_window_size(void)
{
    return s_mmio.mapped_register_window_size;
}

static bool window_offset(UINT32 address, UINT32* ret)
{
    if (!s_mmio.host_window_enabled) return false;
    UINT32 offset = address - s_mmio.host_window_base;
    if (offset >= s_mmio.host_window_bytes) return false;
    *ret = offset;
    return true;
}

static bool mapped_register_offset(UINT32 address, UINT32* ret)
{
    UINT32 offset;
    if (window_offset(address, &offset)) {
        *ret = offset;
        return true;
    }
    if (s_mmio.host_window_enabled) return false;
    if (!s_mmio.closed_mapped_register_enabled) return false;
    offset = address - s_mmio.closed_mapped_register_base;
    if (offset >= CONVENTIONAL_WINDOW_BYTES) return false;
    *ret = offset & s_mmio.closed_mapped_register_mask;
    return true;
}

static bool flat_aperture_offset(UINT32 address, UINT32 bytes, UINT32* ret)
{
    if (!s_mmio.flat_aperture_enabled || !bytes) return false;
    UINT32 offset = address - s_mmio.flat_aperture_base;
    if (offset >= s_mmio.flat_aperture_bytes) return false;
    if (bytes > s_mmio.flat_aperture_bytes - offset) return false;
    *ret = offset;
    return true;
}

static bool mapped_register_address_at(UINT32 offset, UINT32 base_offset, UINT32 plus_two_offset,
    UINT8* selector, UINT8* register_offset, UINT8* byte_offset)
{
    UINT32 aperture_base;
    UINT8 regoff;
    if (base_offset <= offset && offset < base_offset + MAPPED_REGISTER_APERTURE_BYTES) {
        aperture_base = base_offset;
        regoff = OFFSET_BASE;
    }
    else if (plus_two_offset <= offset && offset < plus_two_offset + MAPPED_REGISTER_APERTURE_BYTES) {
        aperture_base = plus_two_offset;
        regoff = OFFSET_PLUS_TWO;
    }
    else {
        return false;
    }
    UINT32 relative = offset - aperture_base;
    *selector = (UINT8)(relative / 2);
    *register_offset = regoff;
    *byte_offset = (UINT8)(relative & 1);
    return true;
}

static bool mapped_register_address(UINT32 offset, UINT8* selector, UINT8* register_offset, UINT8* byte_offset)
{
    if (s_mmio.mapped_register_window_size != WINSIZE_DISABLED) {
        if (mapped_register_address_at(offset, COMPATIBILITY_MAPPED_REGISTER_BASE_OFFSET,
            COMPATIBILITY_MAPPED_REGISTER_PLUS_TWO_OFFSET,
            selector, register_offset, byte_offset)) {
            return true;
        }
    }

    UINT32 size = s_mmio.mapped_register_window_bytes;
    if (size < 0x100) return false;
    UINT32 base = size - 0x100;
    return mapped_register_address_at(offset, base, base + 0x40, selector, register_offset, byte_offset);
}

static bool mapped_register_aperture_enabled(void)
{
    return s_mmio.mapped_register_aperture_enabled;
}

static void write_wba1(UINT16 value)
{
    s_ga.wba1 = value;
    GAWindowSize size = window_size_from(value);
    if (size != WINSIZE_DISABLED && window_segment_from(value) != 0) {
        s_ga.last_wba1_window_size = size;
    }
    rebuild_mmio_cache();
    s_ga.wba1_write_count++;
}

static void write_wba2(UINT16 value)
{
    s_ga.wba2 = value;
    rebuild_mmio_cache();
}

static bool decode_port(UINT port, UINT8* selector, UINT8* offset)
{
    if (FIXED_WINDOW_PORT <= port && port <= FIXED_WINDOW_PORT + 1) {
        *selector = SELECTOR_WBA1;
        *offset = (UINT8)(port - FIXED_WINDOW_PORT);
        return true;
    }
    UINT8 sel = (UINT8)(port >> 8);
    if (sel < 0x01 || sel > 0x1f) return false;
    switch (port & 0x00ff) {
    case DEFAULT_GAPORT: *offset = OFFSET_BASE; break;
    case DEFAULT_GAPORT + 1: *offset = OFFSET_BASE_PLUS_ONE; break;
    case DEFAULT_GAPORT + 2: *offset = OFFSET_PLUS_TWO; break;
    case DEFAULT_GAPORT + 3: *offset = OFFSET_PLUS_THREE; break;
    default: return false;
    }
    *selector = sel;
    return true;
}

static bool word_base_offset(UINT8 offset, UINT8* base)
{
    switch (offset) {
    case OFFSET_BASE:
    case OFFSET_BASE_PLUS_ONE:
        *base = OFFSET_BASE;
        return true;
    case OFFSET_PLUS_TWO:
    case OFFSET_PLUS_THREE:
        *base = OFFSET_PLUS_TWO;
        return true;
    default:
        return false;
    }
}

static bool is_high_byte_offset(UINT8 offset)
{
    return (offset & 1) != 0;
}

static UINT8 status_register(void)
{
    return 0x10 | 0x40 | 0x03;
}

static bool crtc_matches_full_color_mode(void)
{
    return s_ga.crtc_registers[0x00] == 0x00a6 &&
        s_ga.crtc_registers[0x02] == 0x007f &&
        s_ga.crtc_registers[0x10] == 0x020b &&
        s_ga.crtc_registers[0x12] == 0x01df &&
        s_ga.crtc_registers[0x36] == 0x5084;
}

static void enter_full_color_mode(void)
{
    s_ga.plane_mode = PLANE_FULLCOLOR24;
    s_ga.indexed8_high_color_mode = false;
    s_ga.active_width = FULL_COLOR_WIDTH;
    s_ga.active_height = FULL_COLOR_HEIGHT;
    update_public_state();
}

static void update_full_color_mode_from_crtc(void)
{
    if (crtc_matches_full_color_mode()) enter_full_color_mode();
}

static void observe_full_color_helper_write(UINT8 selector, UINT8 offset, UINT8 value)
{
    if (!crtc_matches_full_color_mode()) {
        s_ga.full_color_helper_step = 0;
        return;
    }

    UINT8 esel = 0, eoff = 0, eval = 0;
    switch (s_ga.full_color_helper_step) {
    case 0: esel = SELECTOR_VDAC_ARW_RS; eoff = OFFSET_BASE_PLUS_ONE; eval = 0x02; break;
    case 1: esel = SELECTOR_VDAC_ARW_RS; eoff = OFFSET_BASE; eval = 0x18; break;
    case 2: esel = SELECTOR_VDAC_ARW_RS; eoff = OFFSET_BASE_PLUS_ONE; eval = 0x01; break;
    case 3: esel = SELECTOR_VDAC_MSK; eoff = OFFSET_BASE; eval = 0x22; break;
    case 4: esel = SELECTOR_VDAC_ARW_RS; eoff = OFFSET_BASE_PLUS_ONE; eval = 0x00; break;
    case 5: esel = SELECTOR_SYSTEM_PDT; eoff = OFFSET_BASE; eval = 0x03; break;
    default: s_ga.full_color_helper_step = 0; return;
    }

    if (selector == esel && offset == eoff && value == eval) {
        s_ga.full_color_helper_step++;
        if (s_ga.full_color_helper_step == 6) {
            enter_full_color_mode();
            s_ga.full_color_helper_step = 0;
        }
    }
    else {
        s_ga.full_color_helper_step = 0;
    }
}

static bool current_vsync_active(void)
{
    // XXX: GDCのvsyncを借りる
    return (gdc.vsync & 0x20) != 0;
}

static UINT16 crtc_data_word(int index)
{
    UINT16 stored = s_ga.crtc_registers[index & 0x7f];
    bool vsync = current_vsync_active();
    s_ga.vsync_active = vsync;
    if ((index & 0x7f) == CRTC_INDEX_VSYNC_STATUS) {
        UINT16 masked = (UINT16)(stored & ~CRTC_BIT_VSYNC_ACTIVE);
        return vsync ? (UINT16)(masked | CRTC_BIT_VSYNC_ACTIVE) : masked;
    }
    if ((index & 0x7f) == CRTC_INDEX_GA1280_VSYNC_STATUS) {
        UINT16 masked = (UINT16)(stored & ~CRTC_BIT_GA1280_VSYNC_ACTIVE);
        return vsync ? (UINT16)(masked | CRTC_BIT_GA1280_VSYNC_ACTIVE) : masked;
    }
    return stored;
}

static void write_crtc_data_low_byte(int index, UINT8 value)
{
    index &= 0x7f;
    s_ga.crtc_registers[index] = (UINT16)((s_ga.crtc_registers[index] & 0xff00) | value);
    update_full_color_mode_from_crtc();
    update_dimensions_from_crtc();
    s_ga.crtc_write_count++;
}

static void write_crtc_data_word(int index, UINT16 value)
{
    index &= 0x7f;
    s_ga.crtc_registers[index] = value;
    update_full_color_mode_from_crtc();
    update_dimensions_from_crtc();
    s_ga.crtc_write_count++;
}

static UINT8 read_id_stream(void)
{
    UINT8 value = ID_STREAM[s_ga.id_stream_cursor & 0x0f];
    s_ga.id_stream_cursor = (UINT8)((s_ga.id_stream_cursor + 1) & 0x0f);
    return value;
}

static bool cursor_bank(UINT8 bank)
{
    return s_ga.vdac_rs == bank;
}

static UINT8 read_vdac_arw(void)
{
    if (cursor_bank(1)) return s_ga.cursor_color_index;
    if (cursor_bank(3)) return (UINT8)s_ga.cursor_x;
    return s_ga.palette_index_write;
}

static UINT8 read_vdac_arr(void)
{
    if (cursor_bank(3)) return (UINT8)(s_ga.cursor_y >> 8);
    return s_ga.palette_index_read;
}

static UINT8 read_palette_component(void)
{
    UINT8 index = s_ga.palette_index_read;
    UINT8 phase = (UINT8)(s_ga.palette_rgb_phase % 3);
    UINT8 value = s_ga.palette[index][phase];
    s_ga.palette_rgb_phase = (UINT8)((s_ga.palette_rgb_phase + 1) % 3);
    if (s_ga.palette_rgb_phase == 0) s_ga.palette_index_read++;
    return value;
}

static UINT8 read_cursor_color_component(void)
{
    UINT8 index = (UINT8)(s_ga.cursor_color_index & 1);
    UINT8 phase = (UINT8)(s_ga.cursor_color_rgb_phase % 3);
    UINT8 value = s_ga.cursor_colors[index][phase];
    s_ga.cursor_color_rgb_phase = (UINT8)((s_ga.cursor_color_rgb_phase + 1) % 3);
    if (s_ga.cursor_color_rgb_phase == 0) s_ga.cursor_color_index = (UINT8)((s_ga.cursor_color_index + 1) & 1);
    return value;
}

static UINT8 read_vdac_cpr(void)
{
    if (cursor_bank(1)) return read_cursor_color_component();
    if (cursor_bank(3)) return (UINT8)(s_ga.cursor_x >> 8);
    return read_palette_component();
}

static UINT8 read_vdac_msk(void)
{
    if (cursor_bank(3)) return (UINT8)s_ga.cursor_y;
    return s_ga.vdac_mask;
}

static void write_cursor_pattern_byte(UINT8 value)
{
    UINT16 index = (UINT16)(s_ga.cursor_pattern_index % CURSOR_PATTERN_BYTES);
    if (index < CURSOR_MASK_BYTES) s_ga.cursor.sepa.cursor_xor_pattern[index] = value;
    else s_ga.cursor.sepa.cursor_and_pattern[index - CURSOR_MASK_BYTES] = value;
    s_ga.cursor_pattern_index = (UINT16)((s_ga.cursor_pattern_index + 1) % CURSOR_PATTERN_BYTES);
    mark_updated();
}

static void write_cursor_color_component(UINT8 value)
{
    UINT8 index = (UINT8)(s_ga.cursor_color_index & 1);
    UINT8 phase = (UINT8)(s_ga.cursor_color_rgb_phase % 3);
    s_ga.cursor_colors[index][phase] = value;
    s_ga.cursor_color_rgb_phase = (UINT8)((s_ga.cursor_color_rgb_phase + 1) % 3);
    if (s_ga.cursor_color_rgb_phase == 0) s_ga.cursor_color_index = (UINT8)((s_ga.cursor_color_index + 1) & 1);
    mark_updated();
}

static void write_palette_component(UINT8 value)
{
    UINT8 index = s_ga.palette_index_write;
    UINT8 phase = (UINT8)(s_ga.palette_rgb_phase % 3);
    s_ga.palette[index][phase] = value;
    s_ga.palette_rgb_phase = (UINT8)((s_ga.palette_rgb_phase + 1) % 3);
    if (s_ga.palette_rgb_phase == 0) s_ga.palette_index_write++;
    mark_palette_updated();
}

static void write_vdac_rs(UINT8 value)
{
    s_ga.vdac_rs = value;
    if (value == 1) s_ga.cursor_color_rgb_phase = 0;
    if (value == 2) {
        s_ga.cursor_pattern_index = 0;
    }
    observe_full_color_helper_write(SELECTOR_VDAC_ARW_RS, OFFSET_BASE_PLUS_ONE, value);
}

static void write_vdac_arw(UINT8 value)
{
    if (cursor_bank(1)) {
        s_ga.cursor_color_index = (UINT8)(value & 1);
        s_ga.cursor_color_rgb_phase = 0;
        return;
    }
    if (cursor_bank(3)) {
        s_ga.cursor_x = (UINT16)((s_ga.cursor_x & 0xff00) | value);
        mark_updated();
        return;
    }
    s_ga.palette_index_write = value;
    s_ga.palette_rgb_phase = 0;
    update_plane_mode_after_vdac_index_write(value);
    observe_full_color_helper_write(SELECTOR_VDAC_ARW_RS, OFFSET_BASE, value);
}

static void write_vdac_arr(UINT8 value)
{
    if (cursor_bank(2)) {
        write_cursor_pattern_byte(value);
        return;
    }
    if (cursor_bank(3)) {
        s_ga.cursor_y = (UINT16)((s_ga.cursor_y & 0x00ff) | ((UINT16)value << 8));
        s_ga.cursor_visible = true;
        mark_updated();
        return;
    }
    s_ga.palette_index_read = value;
    s_ga.palette_rgb_phase = 0;
}

static void write_vdac_cpr(UINT8 value)
{
    if (cursor_bank(1)) {
        write_cursor_color_component(value);
        return;
    }
    if (cursor_bank(3)) {
        s_ga.cursor_x = (UINT16)((s_ga.cursor_x & 0x00ff) | ((UINT16)value << 8));
        mark_updated();
        return;
    }
    write_palette_component(value);
}

static void write_vdac_msk(UINT8 value)
{
    if (cursor_bank(3)) {
        s_ga.cursor_y = (UINT16)((s_ga.cursor_y & 0xff00) | value);
        mark_updated();
        return;
    }
    s_ga.vdac_mask = value;
    update_plane_mode_after_vdac_mask_write(value);
    observe_full_color_helper_write(SELECTOR_VDAC_MSK, OFFSET_BASE, value);
    mark_palette_updated();
}

static UINT16 read_tile_word(void)
{
    int count = s_ga.tile_pattern_count;
    if (count <= 0) return s_ga.tile;
    int index = s_ga.tile_read_index % TILE_PATTERN_WORDS;
    UINT16 value = s_ga.tile_pattern[index];
    if (count == TILE_PATTERN_WORDS) s_ga.tile_read_index = (UINT8)((index + 1) % TILE_PATTERN_WORDS);
    else s_ga.tile_read_index = (UINT8)((index + 1) % count);
    return value;
}

static void write_tile_word(UINT16 value)
{
    s_ga.tile = value;
    int index = s_ga.tile_write_index % TILE_PATTERN_WORDS;
    s_ga.tile_pattern[index] = value;
    if (s_ga.tile_pattern_count < TILE_PATTERN_WORDS) s_ga.tile_pattern_count++;
    s_ga.tile_write_index = (UINT8)((s_ga.tile_write_index + 1) % TILE_PATTERN_WORDS);
    s_ga.tile_read_index = (s_ga.tile_pattern_count == TILE_PATTERN_WORDS) ? s_ga.tile_write_index : 0;
}

static void reset_rop_pattern_stream(UINT8 value)
{
    s_ga.unknown_sel_15_off2 = value;
    s_ga.rop_pattern_index = (UINT8)(value & 0x07);
    s_ga.reset_unknown_write_count++;
}

static void write_rop_pattern_byte(UINT8 value)
{
    int index = s_ga.rop_pattern_index % ROP_PATTERN_ROWS;
    s_ga.rop_pattern[index] = value;
    s_ga.rop_pattern_index = (UINT8)((s_ga.rop_pattern_index + 1) % ROP_PATTERN_ROWS);
}

static void write_rop_pattern_word(UINT16 value)
{
    s_ga.unknown_sel_14_off2 = value;
    write_rop_pattern_byte((UINT8)value);
    write_rop_pattern_byte((UINT8)(value >> 8));
}

static bool read_word(UINT8 selector, UINT8 offset, UINT16* ret)
{
    switch ((selector << 8) | offset) {
    case (SELECTOR_INDEX << 8) | OFFSET_BASE: *ret = s_ga.index; return true;
    case (SELECTOR_SRW << 8) | OFFSET_BASE: *ret = s_ga.srw; return true;
    case (SELECTOR_SRR << 8) | OFFSET_BASE: *ret = s_ga.srr; return true;
    case (SELECTOR_WPM << 8) | OFFSET_BASE: *ret = s_ga.wpm; return true;
    case (SELECTOR_WBM << 8) | OFFSET_BASE: *ret = s_ga.wbm; return true;
    case (SELECTOR_PRS << 8) | OFFSET_BASE: *ret = (UINT16)(s_ga.prs | (s_ga.prs_high << 8)); return true;
    case (SELECTOR_RPE << 8) | OFFSET_BASE: *ret = (UINT16)(s_ga.rpe | (s_ga.rpe_high << 8)); return true;
    case (SELECTOR_COL << 8) | OFFSET_BASE: *ret = s_ga.col; return true;
    case (SELECTOR_TILE << 8) | OFFSET_BASE: *ret = read_tile_word(); return true;
    case (SELECTOR_UNKNOWN_0F << 8) | OFFSET_BASE: *ret = s_ga.unknown_sel_0f_off0; return true;
    case (SELECTOR_FCOL << 8) | OFFSET_BASE: *ret = s_ga.fcol; return true;
    case (SELECTOR_BCOL_PMW << 8) | OFFSET_BASE: *ret = s_ga.bcol; return true;
    case (SELECTOR_MIX << 8) | OFFSET_BASE: *ret = (UINT16)(s_ga.fmix | (s_ga.bmix << 8)); return true;
    case (SELECTOR_CWB_UNKNOWN << 8) | OFFSET_BASE: *ret = s_ga.cwb; return true;
    case (SELECTOR_WBA1 << 8) | OFFSET_BASE: *ret = s_ga.wba1; return true;
    case (SELECTOR_WBA2 << 8) | OFFSET_BASE: *ret = s_ga.wba2; return true;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_BASE: *ret = s_ga.system_register; return true;
    case (SELECTOR_CRTC_POP2 << 8) | OFFSET_BASE: *ret = crtc_data_word(s_ga.crtc_index & 0x7f); return true;
    case (SELECTOR_STATUS_SSV << 8) | OFFSET_PLUS_TWO: *ret = s_ga.ssv; return true;
    case (SELECTOR_CRTC_POP1 << 8) | OFFSET_PLUS_TWO: *ret = s_ga.pop1; return true;
    case (SELECTOR_CRTC_POP2 << 8) | OFFSET_PLUS_TWO: *ret = s_ga.pop2; return true;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_PLUS_TWO: *ret = read_pdt_word(); return true;
    case (SELECTOR_SRW << 8) | OFFSET_PLUS_TWO: *ret = s_ga.errs; return true;
    case (SELECTOR_SRR << 8) | OFFSET_PLUS_TWO: *ret = s_ga.k1; return true;
    case (SELECTOR_WPM << 8) | OFFSET_PLUS_TWO: *ret = s_ga.k2; return true;
    case (0x04 << 8) | OFFSET_PLUS_TWO: *ret = s_ga.opd1; return true;
    case (SELECTOR_WBM << 8) | OFFSET_PLUS_TWO: *ret = s_ga.opd2; return true;
    case (SELECTOR_PRS << 8) | OFFSET_PLUS_TWO: *ret = s_ga.lins; return true;
    case (0x08 << 8) | OFFSET_PLUS_TWO: *ret = s_ga.srcx; return true;
    case (SELECTOR_COL << 8) | OFFSET_PLUS_TWO: *ret = s_ga.srcy; return true;
    case (0x0A << 8) | OFFSET_PLUS_TWO: *ret = s_ga.dstx; return true;
    case (SELECTOR_TILE << 8) | OFFSET_PLUS_TWO: *ret = s_ga.dsty; return true;
    case (SELECTOR_BCOL_PMW << 8) | OFFSET_PLUS_TWO: *ret = s_ga.pmw; return true;
    case (SELECTOR_PMH << 8) | OFFSET_PLUS_TWO: *ret = s_ga.pmh; return true;
    case (SELECTOR_MIX << 8) | OFFSET_PLUS_TWO: *ret = s_ga.unknown_sel_14_off2; return true;
    default: return false;
    }
}

static void write_cwb(UINT16 value)
{
    s_ga.cwb = value;
    switch (value & 0xf000) {
    case 0x0000: s_ga.clip_sy = (UINT16)(value & 0x0fff); break;
    case 0x1000: s_ga.clip_sx = (UINT16)(value & 0x0fff); break;
    case 0x2000: s_ga.clip_ey = (UINT16)(value & 0x0fff); break;
    case 0x3000: s_ga.clip_ex = (UINT16)(value & 0x0fff); break;
    case 0x4000:
        s_ga.clip_enabled = (value & CLIP_CONTROL_ENABLE) != 0;
        s_ga.clip_outside = (value & CLIP_CONTROL_OUTSIDE) != 0;
        break;
    default:
        break;
    }
}

static bool write_word(UINT8 selector, UINT8 offset, UINT16 value)
{
    switch ((selector << 8) | offset) {
    case (SELECTOR_INDEX << 8) | OFFSET_BASE: s_ga.index = value; break;
    case (SELECTOR_SRW << 8) | OFFSET_BASE: s_ga.srw = value; break;
    case (SELECTOR_SRR << 8) | OFFSET_BASE: s_ga.srr = value; break;
    case (SELECTOR_WPM << 8) | OFFSET_BASE: s_ga.wpm = value; break;
    case (SELECTOR_WBM << 8) | OFFSET_BASE: s_ga.wbm = value; break;
    case (SELECTOR_PRS << 8) | OFFSET_BASE: s_ga.prs = (UINT8)value; s_ga.prs_high = (UINT8)(value >> 8); break;
    case (SELECTOR_RPE << 8) | OFFSET_BASE: s_ga.rpe = (UINT8)value; s_ga.rpe_high = (UINT8)(value >> 8); break;
    case (SELECTOR_COL << 8) | OFFSET_BASE: s_ga.col = value; break;
    case (SELECTOR_TILE << 8) | OFFSET_BASE: write_tile_word(value); break;
    case (SELECTOR_ROT << 8) | OFFSET_BASE: s_ga.rot = (UINT8)value; s_ga.rot_high = (UINT8)(value >> 8); break;
    case (SELECTOR_MOD << 8) | OFFSET_BASE: s_ga.mod1 = (UINT8)value; s_ga.mod2 = (UINT8)(value >> 8); rebuild_mmio_cache(); update_public_state(); break;
    case (SELECTOR_UNKNOWN_0F << 8) | OFFSET_BASE: s_ga.unknown_sel_0f_off0 = value; s_ga.reset_unknown_write_count++; break;
    case (SELECTOR_FCOL << 8) | OFFSET_BASE: s_ga.fcol = value; break;
    case (SELECTOR_BCOL_PMW << 8) | OFFSET_BASE: s_ga.bcol = value; break;
    case (SELECTOR_MIX << 8) | OFFSET_BASE: s_ga.fmix = (UINT8)value; s_ga.bmix = (UINT8)(value >> 8); break;
    case (SELECTOR_CWB_UNKNOWN << 8) | OFFSET_BASE: write_cwb(value); break;
    case (SELECTOR_WBA1 << 8) | OFFSET_BASE: write_wba1(value); break;
    case (SELECTOR_WBA2 << 8) | OFFSET_BASE: write_wba2(value); break;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_BASE: s_ga.system_register = value; break;
    case (SELECTOR_CRTC_POP1 << 8) | OFFSET_BASE:
        s_ga.crtc_index = (UINT8)value;
        write_crtc_data_low_byte(s_ga.crtc_index & 0x7f, (UINT8)(value >> 8));
        break;
    case (SELECTOR_CRTC_POP2 << 8) | OFFSET_BASE: write_crtc_data_word(s_ga.crtc_index & 0x7f, value); break;
    case (SELECTOR_SRW << 8) | OFFSET_PLUS_TWO: s_ga.errs = value; break;
    case (SELECTOR_SRR << 8) | OFFSET_PLUS_TWO: s_ga.k1 = value; break;
    case (SELECTOR_WPM << 8) | OFFSET_PLUS_TWO: s_ga.k2 = value; break;
    case (0x04 << 8) | OFFSET_PLUS_TWO: s_ga.opd1 = value; break;
    case (SELECTOR_WBM << 8) | OFFSET_PLUS_TWO: s_ga.opd2 = value; break;
    case (SELECTOR_PRS << 8) | OFFSET_PLUS_TWO: s_ga.lins = value; break;
    case (0x08 << 8) | OFFSET_PLUS_TWO: s_ga.srcx = value; break;
    case (SELECTOR_COL << 8) | OFFSET_PLUS_TWO: s_ga.srcy = value; break;
    case (0x0A << 8) | OFFSET_PLUS_TWO: s_ga.dstx = value; break;
    case (SELECTOR_TILE << 8) | OFFSET_PLUS_TWO: s_ga.dsty = value; break;
    case (SELECTOR_MIX << 8) | OFFSET_PLUS_TWO: write_rop_pattern_word(value); s_ga.reset_unknown_write_count++; break;
    case (SELECTOR_BCOL_PMW << 8) | OFFSET_PLUS_TWO: s_ga.pmw = value; break;
    case (SELECTOR_PMH << 8) | OFFSET_PLUS_TWO: s_ga.pmh = value; break;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_PLUS_TWO: write_pdt_word(value); break;
    case (SELECTOR_STATUS_SSV << 8) | OFFSET_PLUS_TWO: s_ga.ssv = value; break;
    case (SELECTOR_CRTC_POP1 << 8) | OFFSET_PLUS_TWO: s_ga.pop1 = value; break;
    case (SELECTOR_CRTC_POP2 << 8) | OFFSET_PLUS_TWO: s_ga.pop2 = value; execute_pop2(value); break;
    default: return false;
    }
    s_ga.register_write_count++;
    return true;
}

static bool read_byte(UINT8 selector, UINT8 offset, UINT8* ret)
{
    switch ((selector << 8) | offset) {
    case (SELECTOR_PRS << 8) | OFFSET_BASE: *ret = s_ga.prs; return true;
    case (SELECTOR_PRS << 8) | OFFSET_BASE_PLUS_ONE: *ret = s_ga.prs_high; return true;
    case (SELECTOR_RPE << 8) | OFFSET_BASE: *ret = s_ga.rpe; return true;
    case (SELECTOR_RPE << 8) | OFFSET_BASE_PLUS_ONE: *ret = s_ga.rpe_high; return true;
    case (SELECTOR_ROT << 8) | OFFSET_BASE: *ret = s_ga.rot; return true;
    case (SELECTOR_ROT << 8) | OFFSET_BASE_PLUS_ONE: *ret = s_ga.rot_high; return true;
    case (SELECTOR_MOD << 8) | OFFSET_BASE: *ret = s_ga.mod1; return true;
    case (SELECTOR_MOD << 8) | OFFSET_BASE_PLUS_ONE: *ret = s_ga.mod2; return true;
    case (SELECTOR_MIX << 8) | OFFSET_BASE: *ret = s_ga.fmix; return true;
    case (SELECTOR_MIX << 8) | OFFSET_BASE_PLUS_ONE: *ret = s_ga.bmix; return true;
    case (SELECTOR_WBA1 << 8) | OFFSET_BASE: *ret = (UINT8)s_ga.wba1; return true;
    case (SELECTOR_WBA1 << 8) | OFFSET_BASE_PLUS_ONE: *ret = (UINT8)(s_ga.wba1 >> 8); return true;
    case (SELECTOR_VDAC_ARW_RS << 8) | OFFSET_BASE: *ret = read_vdac_arw(); return true;
    case (SELECTOR_VDAC_ARW_RS << 8) | OFFSET_BASE_PLUS_ONE: *ret = s_ga.vdac_rs; return true;
    case (SELECTOR_VDAC_ARR << 8) | OFFSET_BASE: *ret = read_vdac_arr(); return true;
    case (SELECTOR_VDAC_CPR << 8) | OFFSET_BASE: *ret = read_vdac_cpr(); return true;
    case (SELECTOR_VDAC_MSK << 8) | OFFSET_BASE: *ret = read_vdac_msk(); return true;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_BASE: *ret = (UINT8)s_ga.system_register; return true;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_BASE_PLUS_ONE: *ret = s_ga.system_auxiliary_register; return true;
    case (SELECTOR_STATUS_SSV << 8) | OFFSET_BASE: *ret = status_register(); return true;
    case (SELECTOR_STATUS_SSV << 8) | OFFSET_BASE_PLUS_ONE: *ret = read_id_stream(); return true;
    case (SELECTOR_CRTC_POP1 << 8) | OFFSET_BASE: *ret = s_ga.crtc_index; return true;
    case (SELECTOR_CRTC_POP2 << 8) | OFFSET_BASE: *ret = (UINT8)crtc_data_word(s_ga.crtc_index & 0x7f); return true;
    default:
        UINT8 base;
        UINT16 word;
        if (!word_base_offset(offset, &base)) return false;
        if (!read_word(selector, base, &word)) return false;
        *ret = is_high_byte_offset(offset) ? (UINT8)(word >> 8) : (UINT8)word;
        return true;
    }
}

static bool write_byte(UINT8 selector, UINT8 offset, UINT8 value)
{
    switch ((selector << 8) | offset) {
    case (SELECTOR_PRS << 8) | OFFSET_BASE: s_ga.prs = value; break;
    case (SELECTOR_PRS << 8) | OFFSET_BASE_PLUS_ONE: s_ga.prs_high = value; break;
    case (SELECTOR_RPE << 8) | OFFSET_BASE: s_ga.rpe = value; break;
    case (SELECTOR_RPE << 8) | OFFSET_BASE_PLUS_ONE: s_ga.rpe_high = value; break;
    case (SELECTOR_ROT << 8) | OFFSET_BASE: s_ga.rot = value; break;
    case (SELECTOR_ROT << 8) | OFFSET_BASE_PLUS_ONE: s_ga.rot_high = value; break;
    case (SELECTOR_MOD << 8) | OFFSET_BASE: s_ga.mod1 = value; rebuild_mmio_cache(); update_public_state(); break;
    case (SELECTOR_MOD << 8) | OFFSET_BASE_PLUS_ONE: s_ga.mod2 = value; rebuild_mmio_cache(); update_public_state(); break;
    case (SELECTOR_MIX << 8) | OFFSET_BASE: s_ga.fmix = value; break;
    case (SELECTOR_MIX << 8) | OFFSET_BASE_PLUS_ONE: s_ga.bmix = value; break;
    case (SELECTOR_WBA1 << 8) | OFFSET_BASE: write_wba1((UINT16)((s_ga.wba1 & 0xff00) | value)); break;
    case (SELECTOR_WBA1 << 8) | OFFSET_BASE_PLUS_ONE: write_wba1((UINT16)((s_ga.wba1 & 0x00ff) | ((UINT16)value << 8))); break;
    case (SELECTOR_VDAC_ARW_RS << 8) | OFFSET_BASE: write_vdac_arw(value); s_ga.ramdac_write_count++; break;
    case (SELECTOR_VDAC_ARW_RS << 8) | OFFSET_BASE_PLUS_ONE: write_vdac_rs(value); s_ga.ramdac_write_count++; break;
    case (SELECTOR_VDAC_ARR << 8) | OFFSET_BASE: write_vdac_arr(value); s_ga.ramdac_write_count++; break;
    case (SELECTOR_VDAC_CPR << 8) | OFFSET_BASE: write_vdac_cpr(value); s_ga.ramdac_write_count++; break;
    case (SELECTOR_VDAC_MSK << 8) | OFFSET_BASE: write_vdac_msk(value); s_ga.ramdac_write_count++; break;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_BASE:
        s_ga.system_register = (UINT16)((s_ga.system_register & 0xff00) | value);
        observe_full_color_helper_write(selector, offset, value);
        break;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_BASE_PLUS_ONE: s_ga.system_auxiliary_register = value; break;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_PLUS_TWO:
        s_ga.pdt_write_low = value;
        s_ga.pdt_write_low_valid = true;
        s_ga.pdt = (UINT16)((s_ga.pdt & 0xff00) | value);
        s_ga.pdt_latch[0] = s_ga.pdt;
        s_ga.pdt_read_phase = 0;
        break;
    case (SELECTOR_SYSTEM_PDT << 8) | OFFSET_PLUS_THREE:
    {
        UINT16 word;
        if (s_ga.pdt_write_low_valid) {
            word = (UINT16)(s_ga.pdt_write_low | ((UINT16)value << 8));
        }
        else {
            word = (UINT16)((s_ga.pdt & 0x00ff) | ((UINT16)value << 8));
        }
        write_pdt_word(word);
        break;
    }
    case (SELECTOR_STATUS_SSV << 8) | OFFSET_BASE: return false;
    case (SELECTOR_CRTC_POP1 << 8) | OFFSET_BASE: s_ga.crtc_index = value; break;
    case (SELECTOR_CRTC_POP2 << 8) | OFFSET_BASE: write_crtc_data_low_byte(s_ga.crtc_index & 0x7f, value); break;
    case (SELECTOR_MIX << 8) | OFFSET_PLUS_TWO: write_rop_pattern_byte(value); break;
    case (SELECTOR_CWB_UNKNOWN << 8) | OFFSET_PLUS_TWO: reset_rop_pattern_stream(0); break;
    default:
        UINT8 base;
        UINT16 previous;
        if (!word_base_offset(offset, &base)) return false;
        if (!read_word(selector, base, &previous)) return false;
        if (is_high_byte_offset(offset)) previous = (UINT16)((previous & 0x00ff) | ((UINT16)value << 8));
        else previous = (UINT16)((previous & 0xff00) | value);
        return write_word(selector, base, previous);
        //UINT8 base;
        //UINT16 previous;
        //if (!word_base_offset(offset, &base)) return false;
        //if (!read_word(selector, base, &previous)) return false;
        //if (!is_high_byte_offset(offset)) {
        //    previous = (UINT16)((previous & 0xff00) | value);
        //    return write_word(selector, base, previous);
        //}
    }
    s_ga.register_write_count++;
    return true;
}

static UINT32 pixel_map_width(void)
{
    if (s_ga.plane_mode == PLANE_FULLCOLOR24) return s_ga.active_width;
    return s_ga.pmw ? clampu32((UINT32)s_ga.pmw + 1, 1, GA1280_MAX_PIXEL_MAP_WIDTH) : s_ga.active_width;
}

static UINT32 pixel_map_height(void)
{
    if (s_ga.plane_mode == PLANE_FULLCOLOR24) return s_ga.active_height;
    return s_ga.pmh ? clampu32((UINT32)s_ga.pmh + 1, 1, GA1280_MAX_PIXEL_MAP_HEIGHT) : s_ga.active_height;
}

static UINT32 bytes_per_pixel(void)
{
    switch (s_ga.plane_mode) {
    case PLANE_DIRECTCOLOR16: return 2;
    case PLANE_FULLCOLOR24: return 3;
    case PLANE_INDEXED8:
    default: return 1;
    }
}

static UINT32 packed_stride(void)
{
    return pixel_map_width() * bytes_per_pixel();
}

static bool packed_pixel_offset(UINT32 x, UINT32 y, UINT32* offset)
{
    if (x >= pixel_map_width() || y >= pixel_map_height()) return false;
    UINT32 bpp = bytes_per_pixel();
    UINT64 off = (UINT64)y * packed_stride() + (UINT64)x * bpp;
    if (off + bpp > GA1280_VRAM_BYTES) return false;
    *offset = (UINT32)off;
    return true;
}

static UINT32 read_packed_pixel(UINT32 x, UINT32 y)
{
    UINT32 off;
    if (!packed_pixel_offset(x, y, &off)) return 0;
    switch (s_ga.plane_mode) {
    case PLANE_DIRECTCOLOR16:
        return (UINT32)s_ga.vram[off] | ((UINT32)s_ga.vram[off + 1] << 8);
    case PLANE_FULLCOLOR24:
        return (UINT32)s_ga.vram[off] | ((UINT32)s_ga.vram[off + 1] << 8) | ((UINT32)s_ga.vram[off + 2] << 16);
    case PLANE_INDEXED8:
    default:
        return s_ga.vram[off];
    }
}

static bool read_packed_pixel_checked(UINT32 x, UINT32 y, UINT32* color)
{
    UINT32 off;
    if (!packed_pixel_offset(x, y, &off)) return false;
    *color = read_packed_pixel(x, y);
    return true;
}

static void write_packed_pixel(UINT32 x, UINT32 y, UINT32 color)
{
    UINT32 off;
    if (!packed_pixel_offset(x, y, &off)) return;
    switch (s_ga.plane_mode) {
    case PLANE_DIRECTCOLOR16:
        s_ga.vram[off] = (UINT8)color;
        s_ga.vram[off + 1] = (UINT8)(color >> 8);
        break;
    case PLANE_FULLCOLOR24:
        s_ga.vram[off] = (UINT8)color;
        s_ga.vram[off + 1] = (UINT8)(color >> 8);
        s_ga.vram[off + 2] = (UINT8)(color >> 16);
        break;
    case PLANE_INDEXED8:
    default:
        s_ga.vram[off] = (UINT8)color;
        break;
    }
    mark_updated();
}

static UINT32 display_start(void)
{
    return (UINT32)(s_ga.crtc_registers[CRTC_INDEX_DISPLAY_START_LOW] & 0xff) |
        ((UINT32)(s_ga.crtc_registers[CRTC_INDEX_DISPLAY_START_MID] & 0xff) << 8) |
        ((UINT32)(s_ga.crtc_registers[CRTC_INDEX_DISPLAY_START_HIGH] & 0xff) << 16);
}

static UINT32 display_pixels_per_crtc_unit(void)
{
    switch (s_ga.plane_mode) {
    case PLANE_INDEXED8: return 4;
    case PLANE_DIRECTCOLOR16: return 2;
    case PLANE_FULLCOLOR24: return 1;
    default: return 1;
    }
}

static UINT32 horizontal_pixels_per_crtc_unit(void)
{
    if (s_ga.plane_mode == PLANE_FULLCOLOR24) return 4;
    return (s_ga.plane_mode == PLANE_INDEXED8) ? 16 : 8;
}

static void update_dimensions_from_crtc(void)
{
    if (s_ga.plane_mode == PLANE_FULLCOLOR24) {
        s_ga.active_width = FULL_COLOR_WIDTH;
        s_ga.active_height = FULL_COLOR_HEIGHT;
        update_public_state();
        return;
    }
    UINT32 width = ((UINT32)s_ga.crtc_registers[0x02] + 1) * horizontal_pixels_per_crtc_unit();
    UINT32 height = (UINT32)s_ga.crtc_registers[0x12] + 1;
    if (s_ga.crtc_registers[0x02] != 0) s_ga.active_width = clampu32(width, 1, GA1280_MAX_VISIBLE_WIDTH);
    if (s_ga.crtc_registers[0x12] != 0) s_ga.active_height = clampu32(height, 1, GA1280_MAX_VISIBLE_HEIGHT);
    update_public_state();
}

static void update_plane_mode_after_vdac_index_write(UINT8 value)
{
    if (crtc_matches_full_color_mode()) {
        enter_full_color_mode();
        return;
    }
    if (s_ga.vdac_rs != 2) return;
    if (value == 0x38) {
        s_ga.plane_mode = PLANE_DIRECTCOLOR16;
        s_ga.indexed8_high_color_mode = false;
        update_dimensions_from_crtc();
    }
    else if (value == 0x48) {
        s_ga.plane_mode = PLANE_INDEXED8;
        s_ga.indexed8_high_color_mode = true;
        update_dimensions_from_crtc();
    }
}

static void update_plane_mode_after_vdac_mask_write(UINT8)
{
    if (crtc_matches_full_color_mode()) enter_full_color_mode();
}

static bool raster_line_checked(UINT16 start, UINT32 line_offset, UINT32* line)
{
    UINT32 height = pixel_map_height();
    if (!height) return false;

    if (s_ga.active_width > 1024) {
        // 1280x1024は折り返す（根拠なし）
        *line = ((UINT32)start + line_offset) % height;
    }
    else {
        UINT32 y = (UINT32)start + line_offset;
        if (y >= height) return false;
        *line = y;
    }
    return true;
}

static UINT32 host_bytes_per_line(void)
{
    return divceil(pixel_map_width(), 8);
}

static bool host_window_position(UINT32 offset, UINT16 start_line, UINT32* line, UINT32* byte_in_line)
{
    UINT32 bpl = host_bytes_per_line();
    if (!bpl) return false;
    if (!raster_line_checked(start_line, offset / bpl, line)) return false;
    *byte_in_line = offset % bpl;
    return true;
}

static int active_plane_count(void)
{
    switch (s_ga.plane_mode) {
    case PLANE_DIRECTCOLOR16: return 16;
    case PLANE_FULLCOLOR24: return 24;
    case PLANE_INDEXED8:
    default: return 8;
    }
}

static bool indexed8_high_color_context(void)
{
    return s_ga.plane_mode == PLANE_INDEXED8 && s_ga.indexed8_high_color_mode;
}

static bool indexed8_linear_window_selected(void)
{
    return indexed8_high_color_context() &&
        window_size_from(s_ga.wba1) == WINSIZE_DISABLED &&
        window_size_from(s_ga.wba2) != WINSIZE_DISABLED;
}

static bool indexed8_plane_page_context(void)
{
    return s_ga.plane_mode == PLANE_INDEXED8 && !s_ga.indexed8_high_color_mode;
}

static bool indexed8_16_color_context(void)
{
    return indexed8_plane_page_context() && s_ga.vdac_mask == 0x0f;
}

static UINT32 lowbit_shift(UINT32 mask)
{
    UINT32 shift = 0;
    if (mask == 0) return 0;
    while (((mask >> shift) & 1u) == 0 && shift < 31) shift++;
    return shift;
}

static UINT32 logical_mask_from_plane_mask(UINT32 plane_mask)
{
    if (plane_mask == 0) return 0;
    if (!indexed8_plane_page_context()) return plane_mask;
    if ((plane_mask & 0xffu) == 0xffu) return 0xffu;
    return (plane_mask >> lowbit_shift(plane_mask)) & 0xffu;
}

static UINT32 pack_color_to_plane_mask(UINT32 color, UINT32 plane_mask)
{
    if (plane_mask == 0) return 0;
    if (indexed8_plane_page_context() && (plane_mask & 0xffu) != 0xffu) {
        UINT32 shift = lowbit_shift(plane_mask);
        UINT32 logical_mask = logical_mask_from_plane_mask(plane_mask);
        return (color & logical_mask) << shift;
    }
    return color & plane_mask;
}

static UINT32 unpack_color_from_plane_mask(UINT32 pixel, UINT32 plane_mask)
{
    if (plane_mask == 0) return 0;
    if (indexed8_plane_page_context() && (plane_mask & 0xffu) != 0xffu) {
        return (pixel & plane_mask) >> lowbit_shift(plane_mask);
    }
    return pixel & plane_mask;
}

static UINT32 active_read_plane_mask(void)
{
    switch (s_ga.plane_mode) {
    case PLANE_INDEXED8: return s_ga.rpe;
    case PLANE_DIRECTCOLOR16:
    case PLANE_FULLCOLOR24:
    default: return (UINT32)s_ga.rpe | ((UINT32)s_ga.rpe_high << 8);
    }
}

static UINT32 active_write_plane_mask(void)
{
    if (s_ga.prs & 0x80) {
        UINT32 plane = s_ga.prs & 0x0f;
        return (plane < (UINT32)active_plane_count()) ? (1u << plane) : 0;
    }
    switch (s_ga.plane_mode) {
    case PLANE_INDEXED8:
        if (indexed8_linear_window_selected()) return 0x00ff;
        return s_ga.wpm & 0x00ff;
    case PLANE_DIRECTCOLOR16: return s_ga.wpm;
    case PLANE_FULLCOLOR24: return (UINT32)s_ga.wpm | 0xff0000u;
    default: return s_ga.wpm;
    }
}

static bool vram_read_plane_byte(int plane, UINT32 line, UINT32 byte_in_line, UINT8* ret)
{
    if (plane < 0 || plane >= active_plane_count() || line >= pixel_map_height()) return false;
    UINT32 x_base = byte_in_line * 8;
    if (x_base >= pixel_map_width()) return false;
    UINT32 plane_bit = 1u << plane;
    UINT8 result = 0;
    for (UINT32 bit_index = 0; bit_index < 8; bit_index++) {
        UINT32 x = x_base + bit_index;
        if (x >= pixel_map_width()) break;
        if (read_packed_pixel(x, line) & plane_bit) result |= (UINT8)(0x80 >> bit_index);
    }
    CPU_REMCLOCK -= 8 * GA1280A_MEMWAIT;
    *ret = result;
    return true;
}

static void vram_write_plane_byte_masked(int plane, UINT32 line, UINT32 byte_in_line, UINT8 value, UINT8 bit_mask)
{
    if (plane < 0 || plane >= active_plane_count() || line >= pixel_map_height()) return;
    UINT32 x_base = byte_in_line * 8;
    if (x_base >= pixel_map_width()) return;
    UINT32 plane_bit = 1u << plane;
    for (UINT32 bit_index = 0; bit_index < 8; bit_index++) {
        UINT8 bit = (UINT8)(0x80 >> bit_index);
        if ((bit_mask & bit) == 0) continue;
        UINT32 x = x_base + bit_index;
        if (x >= pixel_map_width()) break;
        UINT32 pixel = read_packed_pixel(x, line);
        if (value & bit) pixel |= plane_bit;
        else pixel &= ~plane_bit;
        write_packed_pixel(x, line, pixel);
    }
    CPU_REMCLOCK -= 8 * GA1280A_MEMWAIT;
}

static bool uses_packed_host_pixels(void)
{
    if (s_ga.mod1 != 0 || s_ga.wbm != 0xffff) return false;
    return s_ga.plane_mode == PLANE_FULLCOLOR24 && s_ga.wpm == 0xffff;
}

static bool uses_packed_indexed_host_pixels(void)
{
    if (!indexed8_linear_window_selected()) return false;
    if (s_ga.wbm != 0xffff) return false;
    if ((s_ga.wpm & 0x00ff) != 0x00ff) return false;

    return true;
}

static bool uses_packed_indexed_host_read_pixels(void)
{
    return indexed8_linear_window_selected();
}

static bool uses_packed_direct16_host_pixels(void)
{
    if (s_ga.plane_mode != PLANE_DIRECTCOLOR16) return false;
    if (s_ga.wbm != 0xffff) return false;
    if (s_ga.wpm != 0xffff) return false;
    if (window_size_from(s_ga.wba1) != WINSIZE_DISABLED) return false;
    if (window_size_from(s_ga.wba2) == WINSIZE_DISABLED) return false;

    if (s_ga.mod1 == 0) return true;
    if (s_ga.mod1 == HOST_WRITE_PIXEL_MASK_MODE) return true;

    return false;
}

static bool uses_packed_direct16_host_read_pixels(void)
{
    if (s_ga.plane_mode != PLANE_DIRECTCOLOR16) return false;
    if (window_size_from(s_ga.wba1) != WINSIZE_DISABLED) return false;
    if (window_size_from(s_ga.wba2) == WINSIZE_DISABLED) return false;
    return true;
}

static bool packed_indexed_position(UINT32 offset, UINT16 start, UINT32* x, UINT32* y)
{
    UINT32 width = pixel_map_width();
    if (!width) return false;
    *x = offset % width;
    return raster_line_checked(start, offset / width, y);
}

static bool packed_direct16_position(UINT32 offset, UINT16 start, UINT32* x, UINT32* y, UINT32* component)
{
    UINT32 width = pixel_map_width();
    if (!width) return false;
    UINT32 pixel_offset = offset / 2;
    *x = pixel_offset % width;
    *component = offset & 1;
    return raster_line_checked(start, pixel_offset / width, y);
}

static bool packed_full_color_position(UINT32 offset, UINT16 start, UINT32* x, UINT32* y, UINT32* component)
{
    UINT32 width = pixel_map_width();
    if (!width) return false;
    UINT32 pixel_offset = offset / 3;
    *x = pixel_offset % width;
    *component = offset % 3;
    return raster_line_checked(start, pixel_offset / width, y);
}

static bool flat_aperture_position(UINT16 start, UINT32 offset, UINT32 bpp, UINT32* x, UINT32* y, UINT32* component)
{
    UINT32 width = pixel_map_width();
    if (!width || !bpp) return false;
    UINT32 pixel_offset = offset / bpp;
    *x = pixel_offset % width;
    *component = offset % bpp;
    return raster_line_checked(start, pixel_offset / width, y);
}

static UINT32 flat_window_size(void)
{
    return s_mmio.flat_window_bytes;
}

static UINT8 host_window_read_packed_direct16(UINT32 offset)
{
    UINT32 x, y, component;
    if (!packed_direct16_position(offset, s_ga.srr, &x, &y, &component)) return 0xff;
    UINT32 color = read_packed_pixel(x, y);
    return component == 0 ? (UINT8)color : (UINT8)(color >> 8);
}

static void host_window_write_packed_direct16(UINT32 offset, UINT8 value)
{
    UINT32 x, y, component;
    if (!packed_direct16_position(offset, s_ga.srw, &x, &y, &component)) return;
    UINT32 color = read_packed_pixel(x, y);
    if (component == 0) color = (color & 0xff00) | value;
    else color = (color & 0x00ff) | ((UINT32)value << 8);
    write_packed_pixel(x, y, color);
}

static UINT8 host_window_read_packed_full_color(UINT32 offset)
{
    UINT32 x, y, component;
    if (!packed_full_color_position(offset, s_ga.srr, &x, &y, &component)) return 0xff;
    UINT32 color = read_packed_pixel(x, y);
    if (component == 0) return (UINT8)(color >> 16);
    if (component == 1) return (UINT8)(color >> 8);
    return (UINT8)color;
}

static void host_window_write_packed_full_color(UINT32 offset, UINT8 value)
{
    UINT32 x, y, component;
    if (!packed_full_color_position(offset, s_ga.srw, &x, &y, &component)) return;
    UINT32 color = read_packed_pixel(x, y);
    if (component == 0) color = (color & 0x00ffff) | ((UINT32)value << 16);
    else if (component == 1) color = (color & 0xff00ff) | ((UINT32)value << 8);
    else color = (color & 0xffff00) | value;
    write_packed_pixel(x, y, color & 0x00ffffff);
}

static UINT8 host_window_read_packed_indexed(UINT32 offset)
{
    UINT32 x, y;
    if (!packed_indexed_position(offset, s_ga.srr, &x, &y)) return 0xff;
    return (UINT8)read_packed_pixel(x, y);
}

static void host_window_write_packed_indexed(UINT32 offset, UINT8 palette_index)
{
    UINT32 x, y;
    if (!packed_indexed_position(offset, s_ga.srw, &x, &y)) return;


    write_packed_pixel(x, y, palette_index);
}

static void host_window_rotate_word(UINT32 offset)
{
    if (offset & 1) return;
    UINT32 line, byte_in_line;
    if (!host_window_position(offset, s_ga.srw, &line, &byte_in_line)) return;
    byte_in_line &= ~1u;
    UINT8 low_mask = (UINT8)s_ga.wbm;
    UINT8 high_mask = (UINT8)(s_ga.wbm >> 8);
    if (!low_mask && !high_mask) return;
    UINT32 plane_mask = active_write_plane_mask();
    UINT32 rotate_count = s_ga.rot & 0x0f;
    for (int plane = 0; plane < active_plane_count(); plane++) {
        if ((plane_mask & (1u << plane)) == 0) continue;
        UINT8 low = 0, high = 0;
        vram_read_plane_byte(plane, line, byte_in_line, &low);
        vram_read_plane_byte(plane, line, byte_in_line + 1, &high);
        UINT16 word = (UINT16)(low | ((UINT16)high << 8));
        UINT16 rotated = (UINT16)((word << rotate_count) | (word >> (16 - rotate_count)));
        vram_write_plane_byte_masked(plane, line, byte_in_line, (UINT8)rotated, low_mask);
        vram_write_plane_byte_masked(plane, line, byte_in_line + 1, (UINT8)(rotated >> 8), high_mask);
    }
    CPU_REMCLOCK -= active_plane_count() * GA1280A_MEMWAIT;
}

static void host_window_write_raw(UINT32 line, UINT32 byte_in_line, UINT8 value, UINT8 bit_mask)
{
    UINT32 plane_mask = active_write_plane_mask();
    for (int plane = 0; plane < active_plane_count(); plane++) {
        if ((plane_mask & (1u << plane)) == 0) continue;
        vram_write_plane_byte_masked(plane, line, byte_in_line, value, bit_mask);
    }
    CPU_REMCLOCK -= active_plane_count() * GA1280A_MEMWAIT;
}

static void host_window_write_color_expand(UINT32 line, UINT32 byte_in_line, UINT8 source_bits, UINT8 bit_mask)
{
    UINT32 x_base = byte_in_line * 8;
    for (UINT32 bit_index = 0; bit_index < 8; bit_index++) {
        UINT8 bit = (UINT8)(0x80 >> bit_index);
        if ((bit_mask & bit) == 0) continue;
        UINT32 color = (source_bits & bit) ? s_ga.fcol : s_ga.bcol;
        UINT8 mix = (source_bits & bit) ? s_ga.fmix : s_ga.bmix;
        write_pixel_rop(x_base + bit_index, line, color, mix);
    }
    CPU_REMCLOCK -= 8 * GA1280A_MEMWAIT;
}

static UINT8 host_window_read_pixel_mask(UINT32 line, UINT32 byte_in_line)
{
    UINT32 plane_mask = active_read_plane_mask();
    if (plane_mask == 0) {
        UINT8 ret;
        return vram_read_plane_byte(s_ga.prs & 0x0f, line, byte_in_line, &ret) ? ret : 0xff;
    }
    UINT8 value = 0;
    for (int plane = 0; plane < active_plane_count(); plane++) {
        if ((plane_mask & (1u << plane)) == 0) continue;
        UINT8 b = 0;
        vram_read_plane_byte(plane, line, byte_in_line, &b);
        value |= b;
    }
    CPU_REMCLOCK -= active_plane_count() * GA1280A_MEMWAIT;
    return value;
}

static void host_window_write_pixel_mask(UINT32 line, UINT32 byte_in_line, UINT8 pixel_mask)
{
    if (!pixel_mask) return;
    UINT32 plane_mask = active_write_plane_mask();
    UINT32 color = s_ga.col;
    for (int plane = 0; plane < active_plane_count(); plane++) {
        if ((plane_mask & (1u << plane)) == 0) continue;
        UINT8 value = (color & (1u << plane)) ? 0xff : 0;
        vram_write_plane_byte_masked(plane, line, byte_in_line, value, pixel_mask);
    }
    CPU_REMCLOCK -= active_plane_count() * GA1280A_MEMWAIT;
}

static UINT8 host_window_read(UINT32 offset)
{
    if (uses_packed_host_pixels()) {
        if (s_ga.plane_mode == PLANE_FULLCOLOR24) return host_window_read_packed_full_color(offset);
    }
    if (uses_packed_indexed_host_read_pixels()) return host_window_read_packed_indexed(offset);
    if (uses_packed_direct16_host_read_pixels()) return host_window_read_packed_direct16(offset);

    UINT32 line, byte_in_line;
    if (!host_window_position(offset, s_ga.srr, &line, &byte_in_line)) return 0xff;
    if (s_ga.mod1 == HOST_WRITE_PIXEL_MASK_MODE) return host_window_read_pixel_mask(line, byte_in_line);

    UINT8 ret;
    return vram_read_plane_byte(s_ga.prs & 0x0f, line, byte_in_line, &ret) ? ret : 0xff;
}

static void host_window_write_data(UINT32 offset, UINT8 value)
{
    s_ga.host_window_write_count++;
    if (s_ga.stream.kind == STREAM_IMAGE_RESTORE) {
        consume_image_restore_byte(value);
        return;
    }
    if (uses_packed_host_pixels()) {
        if (s_ga.plane_mode == PLANE_FULLCOLOR24) host_window_write_packed_full_color(offset, value);
        return;
    }
    if (uses_packed_indexed_host_pixels()) {
        host_window_write_packed_indexed(offset, value);
        return;
    }
    if (uses_packed_direct16_host_pixels()) {
        host_window_write_packed_direct16(offset, value);
        return;
    }
    if (s_ga.mod1 == HOST_WRITE_ROTATE_WORD_MODE) {
        host_window_rotate_word(offset);
        return;
    }


    UINT32 line, byte_in_line;
    if (!host_window_position(offset, s_ga.srw, &line, &byte_in_line)) return;
    UINT8 bit_mask = (byte_in_line & 1) ? (UINT8)(s_ga.wbm >> 8) : (UINT8)s_ga.wbm;
    if (!bit_mask) return;
    if (s_ga.mod1 == HOST_WRITE_PIXEL_MASK_MODE) host_window_write_pixel_mask(line, byte_in_line, (UINT8)(value & bit_mask));
    else if (s_ga.mod1 == HOST_WRITE_COLOR_EXPAND_MODE) host_window_write_color_expand(line, byte_in_line, value, bit_mask);
    else host_window_write_raw(line, byte_in_line, value, bit_mask);
}

static void host_window_write(UINT32 offset, UINT8 value)
{
    if (mapped_register_write_byte(offset, value)) return;
    host_window_write_data(offset, value);
}

static UINT8 flat_aperture_read_byte_at_offset(UINT32 offset)
{
    UINT32 fsize = flat_window_size();
    if (fsize) {
        if (offset >= fsize) return 0xff;
        UINT8 regv;
        if (mapped_register_read_byte(offset, &regv)) return regv;
        return host_window_read(offset);
    }

    if (s_ga.plane_mode == PLANE_INDEXED8) {
        UINT32 x, y, comp;
        if (!flat_aperture_position(s_ga.srr, offset, 1, &x, &y, &comp)) return 0xff;
        return (UINT8)read_pixel_color(x, y);
    }
    if (s_ga.plane_mode == PLANE_DIRECTCOLOR16) {
        UINT32 x, y, comp;
        if (!flat_aperture_position(s_ga.srr, offset, 2, &x, &y, &comp)) return 0xff;
        UINT16 color = (UINT16)read_pixel_color(x, y);
        return comp == 0 ? (UINT8)color : (UINT8)(color >> 8);
    }
    return host_window_read_packed_full_color(offset);
}

static void flat_aperture_write_byte_at_offset(UINT32 offset, UINT8 value)
{
    s_ga.flat_aperture_write_count++;
    UINT32 fsize = flat_window_size();
    if (fsize) {
        if (offset < fsize) host_window_write(offset, value);
        return;
    }
    if (s_ga.stream.kind == STREAM_IMAGE_RESTORE) {
        consume_image_restore_byte(value);
        return;
    }
    if (s_ga.plane_mode == PLANE_INDEXED8) {
        UINT32 x, y, comp;
        if (!flat_aperture_position(s_ga.srw, offset, 1, &x, &y, &comp)) return;
        write_pixel_mixed(x, y, value, PIXEL_MIX_SOURCE);
    }
    else if (s_ga.plane_mode == PLANE_DIRECTCOLOR16) {
        UINT32 x, y, comp;
        if (!flat_aperture_position(s_ga.srw, offset, 2, &x, &y, &comp)) return;
        UINT32 color = read_pixel_color(x, y);
        if (comp == 0) color = (color & 0xff00) | value;
        else color = (color & 0x00ff) | ((UINT32)value << 8);
        write_pixel_mixed(x, y, color, PIXEL_MIX_SOURCE);
    }
    else {
        host_window_write_packed_full_color(offset, value);
    }
}

static UINT16 flat_aperture_read_word_at_offset(UINT32 offset)
{
    UINT32 fsize = flat_window_size();
    if (fsize) {
        if (offset + 1 >= fsize) return 0xffff;
        UINT16 value;
        if (mapped_register_read_word(offset, &value)) return value;
    }
    return (UINT16)(flat_aperture_read_byte_at_offset(offset) |
        ((UINT16)flat_aperture_read_byte_at_offset(offset + 1) << 8));
}

static void flat_aperture_write_word_at_offset(UINT32 offset, UINT16 value)
{
    UINT32 fsize = flat_window_size();
    if (fsize) {
        s_ga.flat_aperture_write_count += 2;
        if (offset + 1 >= fsize) return;
        if (mapped_register_write_word(offset, value)) return;
        host_window_write(offset, (UINT8)value);
        host_window_write(offset + 1, (UINT8)(value >> 8));
        return;
    }
    flat_aperture_write_byte_at_offset(offset, (UINT8)value);
    flat_aperture_write_byte_at_offset(offset + 1, (UINT8)(value >> 8));
}

static UINT32 flat_aperture_read_dword_at_offset(UINT32 offset)
{
    return (UINT32)flat_aperture_read_word_at_offset(offset) |
        ((UINT32)flat_aperture_read_word_at_offset(offset + 2) << 16);
}

static void flat_aperture_write_dword_at_offset(UINT32 offset, UINT32 value)
{
    flat_aperture_write_word_at_offset(offset, (UINT16)value);
    flat_aperture_write_word_at_offset(offset + 2, (UINT16)(value >> 16));
}

static bool mapped_register_read_byte(UINT32 offset, UINT8* ret)
{
    if (!mapped_register_aperture_enabled()) return false;
    UINT8 selector, regoff, byteoff;
    if (!mapped_register_address(offset, &selector, &regoff, &byteoff)) return false;
    if (byteoff == 0 || regoff == OFFSET_BASE) return read_byte(selector, (UINT8)(regoff + byteoff), ret);
    UINT16 word;
    if (!read_word(selector, regoff, &word)) return false;
    *ret = (UINT8)(word >> 8);
    return true;
}

static bool mapped_register_write_byte(UINT32 offset, UINT8 value)
{
    if (!mapped_register_aperture_enabled()) return false;
    UINT8 selector, regoff, byteoff;
    if (!mapped_register_address(offset, &selector, &regoff, &byteoff)) return false;
    if (byteoff == 0 || regoff == OFFSET_BASE || selector == SELECTOR_SYSTEM_PDT) {
        return write_byte(selector, (UINT8)(regoff + byteoff), value);
    }
    UINT16 previous;
    if (!read_word(selector, regoff, &previous)) return false;
    return write_word(selector, regoff, (UINT16)((previous & 0x00ff) | ((UINT16)value << 8)));
}

static bool mapped_register_read_word(UINT32 offset, UINT16* ret)
{
    if (!mapped_register_aperture_enabled()) return false;
    UINT8 selector, regoff, byteoff;
    if (!mapped_register_address(offset, &selector, &regoff, &byteoff)) return false;
    if (byteoff != 0) return false;
    return read_word(selector, regoff, ret);
}

static bool mapped_register_write_word(UINT32 offset, UINT16 value)
{
    if (!mapped_register_aperture_enabled()) return false;
    UINT8 selector, regoff, byteoff;
    if (!mapped_register_address(offset, &selector, &regoff, &byteoff)) return false;
    return byteoff == 0 && write_word(selector, regoff, value);
}

static UINT32 active_color_mask(void)
{
    switch (s_ga.plane_mode) {
    case PLANE_INDEXED8:
        if (indexed8_plane_page_context()) {
            UINT32 plane_mask = active_write_plane_mask() & 0xffu;
            if (plane_mask != 0 && plane_mask != 0xffu) {
                return logical_mask_from_plane_mask(plane_mask);
            }
        }
        return 0xff;
    case PLANE_DIRECTCOLOR16: return 0xffff;
    case PLANE_FULLCOLOR24: return 0x00ffffff;
    default: return 0xffffffff;
    }
}

static UINT32 mask_for_active_color(void)
{
    return active_color_mask();
}

static bool pixel_writable(UINT32 x, UINT32 y)
{
    if (x >= pixel_map_width() || y >= pixel_map_height()) return false;
    if (!write_bit_mask_allows(x)) return false;
    if (!s_ga.clip_enabled) return true;
    bool inside = x >= s_ga.clip_sx && x <= s_ga.clip_ex && y >= s_ga.clip_sy && y <= s_ga.clip_ey;
    return s_ga.clip_outside ? !inside : inside;
}

static bool write_bit_mask_allows(UINT32 x)
{
    UINT32 byte_in_line = x / 8;
    UINT8 bit = (UINT8)(0x80 >> (x & 7));
    UINT8 mask = (byte_in_line & 1) ? (UINT8)(s_ga.wbm >> 8) : (UINT8)s_ga.wbm;
    return (mask & bit) != 0;
}

static UINT32 read_pixel_color(UINT32 x, UINT32 y)
{
    UINT32 pixel = read_packed_pixel(x, y);

    if (indexed8_plane_page_context()) {
        UINT32 plane_mask = active_read_plane_mask() & 0xffu;
        if (plane_mask != 0 && plane_mask != 0xffu) {
            return unpack_color_from_plane_mask(pixel, plane_mask);
        }
    }

    return pixel & mask_for_active_color();
}

static UINT32 read_pixel_color_for_write(UINT32 x, UINT32 y)
{
    UINT32 pixel = read_packed_pixel(x, y);

    if (indexed8_plane_page_context()) {
        UINT32 plane_mask = active_write_plane_mask() & 0xffu;
        if (plane_mask != 0 && plane_mask != 0xffu) {
            return unpack_color_from_plane_mask(pixel, plane_mask);
        }
    }

    return pixel & mask_for_active_color();
}

static UINT32 read_pixel_color_signed(SINT32 x, SINT32 y)
{
    if (x < 0 || y < 0) return 0;
    return read_pixel_color((UINT32)x, (UINT32)y);
}

static void write_pixel_color(UINT32 x, UINT32 y, UINT32 color)
{
    UINT32 plane_mask = active_write_plane_mask();
    UINT32 current;
    if (plane_mask == 0) return;
    if (!read_packed_pixel_checked(x, y, &current)) return;
    write_packed_pixel(x, y, (current & ~plane_mask) | pack_color_to_plane_mask(color, plane_mask));
}

static void write_pixel_mixed(UINT32 x, UINT32 y, UINT32 color, PixelMix mix)
{
    if (!pixel_writable(x, y)) return;
    UINT32 masked = color & mask_for_active_color();
    if (mix == PIXEL_MIX_XOR) masked = (read_pixel_color_for_write(x, y) ^ masked) & mask_for_active_color();
    write_pixel_color(x, y, masked);
}

static void write_pixel_mixed_signed(SINT32 x, SINT32 y, UINT32 color, PixelMix mix)
{
    if (x < 0 || y < 0) return;
    write_pixel_mixed((UINT32)x, (UINT32)y, color, mix);
}

static UINT8 apply_rop(UINT8 rop, bool source_bit, bool dest_bit)
{
    UINT8 index = (source_bit ? 2 : 0) | (dest_bit ? 1 : 0);
    return (UINT8)((rop >> index) & 1);
}

static void warn_unknown_mix(UINT8)
{
    s_ga.unknown_mix_warning_count++;
}

static void write_pixel_rop(UINT32 x, UINT32 y, UINT32 source, UINT8 rop)
{
    if (!pixel_writable(x, y)) return;
    UINT32 dest = read_pixel_color_for_write(x, y);
    UINT32 mask = mask_for_active_color();
    UINT32 result = 0;
    switch (rop & 0x0f) {
    case 0x00: result = 0; break;
    case 0x01: result = ~(source | dest); break;
    case 0x02: result = (~source) & dest; break;
    case 0x03: result = ~source; break;
    case 0x04: result = source & (~dest); break;
    case 0x05: result = ~dest; break;
    case 0x06: result = source ^ dest; break;
    case 0x07: result = ~(source & dest); break;
    case 0x08: result = source & dest; break;
    case 0x09: result = ~(source ^ dest); break;
    case 0x0a: result = dest; break;
    case 0x0b: result = (~source) | dest; break;
    case 0x0c: result = source; break;
    case 0x0d: result = source | (~dest); break;
    case 0x0e: result = source | dest; break;
    case 0x0f: result = mask; break;
    default: result = source; warn_unknown_mix(rop); break;
    }
    write_pixel_color(x, y, result & mask);
}

static void write_pixel_rop_signed(SINT32 x, SINT32 y, UINT32 source, UINT8 rop)
{
    if (x < 0 || y < 0) return;
    write_pixel_rop((UINT32)x, (UINT32)y, source, rop);
}

static PixelMix foreground_mix(void)
{
    if (s_ga.fmix == MIX_XOR) return PIXEL_MIX_XOR;
    if (s_ga.fmix == MIX_SOURCE) return PIXEL_MIX_SOURCE;
    warn_unknown_mix(s_ga.fmix);
    return PIXEL_MIX_SOURCE;
}

static SINT32 signed_word(UINT16 v)
{
    return (SINT16)v;
}

static SINT32 directed_coordinate(UINT16 start, UINT32 delta, UINT8 direction, bool x_axis)
{
    bool descending = x_axis ? ((direction & DIRECTION_DESCENDING_X) != 0) : ((direction & DIRECTION_DESCENDING_Y) != 0);
    SINT32 s = signed_word(start);
    return descending ? s - (SINT32)delta : s + (SINT32)delta;
}

static UINT32 directed_coordinate_u32(UINT32 start, UINT32 delta, UINT8 direction, bool x_axis)
{
    bool descending = x_axis ? ((direction & DIRECTION_DESCENDING_X) != 0) : ((direction & DIRECTION_DESCENDING_Y) != 0);
    return descending ? (start - delta) : (start + delta);
}

static bool line_style_bit(UINT16 style, UINT32 step)
{
    return (style & (0x8000 >> (step & 0x0f))) != 0;
}

static bool pattern_word_bit(UINT16 word, UINT32 column)
{
    if (column < 8) return (word & (0x0080 >> column)) != 0;
    if (column < 16) return (word & (0x8000 >> (column - 8))) != 0;
    return false;
}

static UINT16 pattern_word_bit_mask(UINT32 column)
{
    if (column < 8) return (UINT16)(0x0080 >> column);
    if (column < 16) return (UINT16)(0x8000 >> (column - 8));
    return 0;
}

static bool rop_pattern_bit(UINT32 x, UINT32 y)
{
    UINT8 row = s_ga.rop_pattern[y & 7];
    return (row & (0x80 >> (x & 7))) != 0;
}

static void execute_solid_rectangle_color(UINT32 color, PixelMix mix)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    UINT32 start_x = s_ga.dstx;
    UINT32 start_y = s_ga.dsty;
    for (UINT32 y = start_y; y < start_y + height; y++) {
        for (UINT32 x = start_x; x < start_x + width; x++) write_pixel_mixed(x, y, color, mix);
    }
    CPU_REMCLOCK -= width * height * GA1280A_MEMWAIT;
}

static void execute_solid_rectangle(void)
{
    execute_solid_rectangle_color(s_ga.col, foreground_mix());
}

static void execute_rop_solid_rectangle_foreground(void)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    for (UINT32 y = s_ga.dsty; y < (UINT32)s_ga.dsty + height; y++) {
        for (UINT32 x = s_ga.dstx; x < (UINT32)s_ga.dstx + width; x++) write_pixel_rop(x, y, s_ga.fcol, s_ga.fmix);
    }
    CPU_REMCLOCK -= width * height * GA1280A_MEMWAIT;
}

static void execute_rop_rectangle_foreground(void)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    for (UINT32 row = 0; row < height; row++) {
        for (UINT32 col = 0; col < width; col++) {
            UINT32 sx = (UINT32)s_ga.srcx + col;
            UINT32 sy = (UINT32)s_ga.srcy + row;
            bool bit = rop_pattern_bit(sx, sy);
            write_pixel_rop((UINT32)s_ga.dstx + col, (UINT32)s_ga.dsty + row, bit ? s_ga.fcol : s_ga.bcol, bit ? s_ga.fmix : s_ga.bmix);
        }
    }
    CPU_REMCLOCK -= width * height * GA1280A_MEMWAIT;
}

static void execute_dstphase_rop_rectangle_foreground(void)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    for (UINT32 row = 0; row < height; row++) {
        for (UINT32 col = 0; col < width; col++) {
            UINT32 dx = (UINT32)s_ga.dstx + col;
            UINT32 dy = (UINT32)s_ga.dsty + row;
            bool bit = rop_pattern_bit(dx, dy);
            write_pixel_rop(dx, dy, bit ? s_ga.fcol : s_ga.bcol, bit ? s_ga.fmix : s_ga.bmix);
        }
    }
}

static void execute_copy_rectangle(UINT8 direction)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    for (UINT32 row = 0; row < height; row++) {
        for (UINT32 col = 0; col < width; col++) {
            SINT32 sx = directed_coordinate(s_ga.srcx, col, direction, true);
            SINT32 sy = directed_coordinate(s_ga.srcy, row, direction, false);
            SINT32 dx = directed_coordinate(s_ga.dstx, col, direction, true);
            SINT32 dy = directed_coordinate(s_ga.dsty, row, direction, false);
            write_pixel_mixed_signed(dx, dy, read_pixel_color_signed(sx, sy), PIXEL_MIX_SOURCE);
        }
    }
    CPU_REMCLOCK -= width * height * GA1280A_MEMWAIT;
}

static void execute_copy_rectangle_with_mix(UINT8 direction)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    for (UINT32 row = 0; row < height; row++) {
        for (UINT32 col = 0; col < width; col++) {
            SINT32 sx = directed_coordinate(s_ga.srcx, col, direction, true);
            SINT32 sy = directed_coordinate(s_ga.srcy, row, direction, false);
            SINT32 dx = directed_coordinate(s_ga.dstx, col, direction, true);
            SINT32 dy = directed_coordinate(s_ga.dsty, row, direction, false);
            write_pixel_rop_signed(dx, dy, read_pixel_color_signed(sx, sy), s_ga.fmix);
        }
    }
    CPU_REMCLOCK -= width * height * GA1280A_MEMWAIT;
}

static bool is_shadow_glyph_mask_copy(void)
{
    if (s_ga.plane_mode != PLANE_INDEXED8) {
        return false;
    }

    if (s_ga.srcy < s_ga.active_height && s_ga.srcx < s_ga.active_width) {
        return false;
    }

    if (s_ga.fmix != 0x0e || s_ga.bmix != 0x02) {
        return false;
    }

    if ((s_ga.fcol & 0x000f) != 0x000f) {
        return false;
    }

    if ((s_ga.bcol & 0x000f) != 0x0000) {
        return false;
    }

    if ((active_read_plane_mask() & 0x000f) == 0) {
        return false;
    }

    return true;
}

static void execute_hga_copy_rectangle_alt_with_mix(UINT8 direction)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    for (UINT32 row = 0; row < height; row++) {
        for (UINT32 col = 0; col < width; col++) {
            SINT32 sx = directed_coordinate(s_ga.srcx, col, direction, true);
            SINT32 sy = directed_coordinate(s_ga.srcy, row, direction, false);
            SINT32 dx = directed_coordinate(s_ga.dstx, col, direction, true);
            SINT32 dy = directed_coordinate(s_ga.dsty, row, direction, false);
            UINT32 src = read_pixel_color_signed(sx, sy);
            write_pixel_rop_signed(dx, dy, src, (src & mask_for_active_color()) ? (16 - s_ga.fmix) : (16 - s_ga.bmix));
        }
    }
    CPU_REMCLOCK -= width * height * GA1280A_MEMWAIT;
}

static void execute_shadow_glyph_mask_copy_with_mix(UINT8 direction)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    UINT32 read_mask = active_read_plane_mask();
    for (UINT32 row = 0; row < height; row++) {
        for (UINT32 col = 0; col < width; col++) {
            SINT32 sx = directed_coordinate(s_ga.srcx, col, direction, true);
            SINT32 sy = directed_coordinate(s_ga.srcy, row, direction, false);
            SINT32 dx = directed_coordinate(s_ga.dstx, col, direction, true);
            SINT32 dy = directed_coordinate(s_ga.dsty, row, direction, false);
            UINT32 mask = read_pixel_color_signed(sx, sy);
            bool on = (mask & read_mask) != 0;
            if (on) {
                write_pixel_rop_signed(dx, dy, s_ga.fcol, s_ga.fmix);
            }
            else {
                write_pixel_rop_signed(dx, dy, s_ga.bcol, s_ga.bmix);
            }
        }
    }
    CPU_REMCLOCK -= width * height * GA1280A_MEMWAIT;
}

static void execute_tiled_rectangle(void)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;
    UINT32 source_x = s_ga.srcx;
    UINT32 source_y = s_ga.srcy;
    UINT32 dest_x = s_ga.dstx;
    UINT32 dest_y = s_ga.dsty;

    UINT32 pmw = pixel_map_width();
    UINT32 pmh = pixel_map_height();
    if (!pmw || !pmh) return;

    source_x %= pmw;
    source_y %= pmh;

    UINT32 tile_base_x = source_x & ~(TILE_WIDTH - 1);

    for (UINT32 row = 0; row < height; row++) {
        for (UINT32 col = 0; col < width; col++) {
            UINT32 tile_col = (source_x + col) % TILE_WIDTH;
            UINT32 tile_row = (source_y + row) % TILE_HEIGHT;
            UINT32 x = (tile_base_x + tile_row * TILE_WIDTH + tile_col) % pmw;

            write_pixel_mixed(dest_x + col, dest_y + row,
                read_pixel_color(x, source_y),
                PIXEL_MIX_SOURCE);
        }
    }
    CPU_REMCLOCK -= width * height * GA1280A_MEMWAIT;
}

static void compute_line_points(UINT8 direction)
{
    s_line_points.clear();

    UINT32 major_len = (UINT32)s_ga.opd1;
    SINT32 x = (SINT32)s_ga.dstx;
    SINT32 y = (SINT32)s_ga.dsty;
    SINT32 x_step = (direction & DIRECTION_DESCENDING_X) ? -1 : 1;
    SINT32 y_step = (direction & DIRECTION_DESCENDING_Y) ? -1 : 1;
    SINT32 err = (SINT16)s_ga.errs;
    SINT32 k1 = (SINT16)s_ga.k1;
    SINT32 k2 = (SINT16)s_ga.k2;
    bool y_major = (direction & DIRECTION_Y_MAJOR) != 0;

    for (UINT32 step = 0; step <= major_len; step++) {
        LinePoint p = { step, x, y };
        s_line_points.push_back(p);

        if (step == major_len) break;

        if (err >= 0) {
            if (y_major) x += x_step;
            else y += y_step;
            err += k2;
        }
        else {
            err += k1;
        }

        if (y_major) y += y_step;
        else x += x_step;
    }

    CPU_REMCLOCK -= (major_len + 1) * GA1280A_MEMWAIT;
}

static void execute_solid_line(UINT8 direction)
{
    compute_line_points(direction);
    PixelMix mix = foreground_mix();
    for (size_t i = 0; i < s_line_points.size(); i++) write_pixel_mixed_signed(s_line_points[i].x, s_line_points[i].y, s_ga.col, mix);
    CPU_REMCLOCK -= s_line_points.size() * GA1280A_MEMWAIT_LINE;
}

static void execute_styled_line(UINT8 direction)
{
    compute_line_points(direction);
    PixelMix mix = foreground_mix();
    for (size_t i = 0; i < s_line_points.size(); i++) {
        if (line_style_bit(s_ga.lins, s_line_points[i].step)) write_pixel_mixed_signed(s_line_points[i].x, s_line_points[i].y, s_ga.col, mix);
    }
    CPU_REMCLOCK -= s_line_points.size() * GA1280A_MEMWAIT_LINE;
}

static void execute_rop_line(UINT8 direction)
{
    compute_line_points(direction);
    for (size_t i = 0; i < s_line_points.size(); i++) {
        if (line_style_bit(s_ga.lins, s_line_points[i].step)) write_pixel_rop_signed(s_line_points[i].x, s_line_points[i].y, s_ga.fcol, s_ga.fmix);
        else write_pixel_rop_signed(s_line_points[i].x, s_line_points[i].y, s_ga.bcol, s_ga.bmix);
    }
    CPU_REMCLOCK -= s_line_points.size() * GA1280A_MEMWAIT_LINE;
}

static void execute_host_color_expand(void)
{
    execute_pattern_expand_rectangle(true);
    //s_ga.stream.kind = STREAM_INACTIVE;
}

static void execute_image_restore(UINT8 direction)
{
    ImageRestoreState& st = s_ga.stream.image;
    ZeroMemory(&st, sizeof(st));
    st.x = s_ga.dstx;
    st.y = s_ga.dsty;
    st.width = (UINT32)s_ga.opd1 + 1;
    st.height = (UINT32)s_ga.opd2 + 1;
    st.xor_pixels = (s_ga.fmix == MIX_XOR);
    st.direction = direction;
    st.has_rop = false;
    s_ga.stream.kind = STREAM_IMAGE_RESTORE;

    //TRACEOUT(("IMGREST START dir=%u dst=(%u,%u) size=(%u,%u) "
    //    "src=(%u,%u) plane=%d FMIX=%02X WPM=%04X WBM=%04X",
    //    direction,
    //    st.x, st.y, st.width, st.height,
    //    s_ga.srcx, s_ga.srcy,
    //    s_ga.plane_mode,
    //    s_ga.fmix, s_ga.wpm, s_ga.wbm));
}

static void execute_rop_image_restore(UINT8 direction)
{
    ImageRestoreState& st = s_ga.stream.image;
    ZeroMemory(&st, sizeof(st));
    st.x = s_ga.dstx;
    st.y = s_ga.dsty;
    st.width = (UINT32)s_ga.opd1 + 1;
    st.height = (UINT32)s_ga.opd2 + 1;
    st.xor_pixels = false;
    st.direction = direction;
    st.has_rop = true;
    st.rop = s_ga.fmix;
    s_ga.stream.kind = STREAM_IMAGE_RESTORE;
}

static bool image_restore_destination(ImageRestoreState& st, UINT32* x, UINT32* y)
{
    if (st.pixel_index >= st.width * st.height) return false;
    UINT32 row = st.pixel_index / st.width;
    UINT32 col = st.pixel_index % st.width;
    *x = directed_coordinate_u32(st.x, col, st.direction, true);
    *y = directed_coordinate_u32(st.y, row, st.direction, false);
    return true;
}

static void finish_image_restore(void)
{
    s_ga.stream.kind = STREAM_INACTIVE;
    s_ga.stream.image.byte_phase = 0;
    s_ga.pdt_write_low_valid = false;
}

static void advance_image_restore_pixel(ImageRestoreState& st)
{
    st.pixel_index++;
    if (st.pixel_index >= st.width * st.height) {
        //TRACEOUT(("IMGREST END by pixel_count pix=%u size=(%u,%u) in=(%u,%u)",
        //    st.pixel_index, st.width, st.height, st.input_column, st.input_row));
        finish_image_restore();
    }
}

static void consume_image_restore_pixel(UINT32 color)
{
    if (s_ga.stream.kind != STREAM_IMAGE_RESTORE) return;
    ImageRestoreState& st = s_ga.stream.image;
    UINT32 x, y;
    if (!image_restore_destination(st, &x, &y)) { s_ga.stream.kind = STREAM_INACTIVE; return; }
    if (st.has_rop) write_pixel_rop(x, y, color, st.rop);
    else write_pixel_mixed(x, y, color, st.xor_pixels ? PIXEL_MIX_XOR : PIXEL_MIX_SOURCE);
    advance_image_restore_pixel(st);
    //if (st.width > 512) {
    //    st.width = st.width;
    //    //TRACEOUT(("V:%04x", color));
    //}
}

static void consume_image_restore_indexed_pixel(UINT32 color)
{
    if (s_ga.stream.kind != STREAM_IMAGE_RESTORE) return;
    ImageRestoreState& st = s_ga.stream.image;
    UINT32 padded_width = align_up(st.width, INDEXED_IMAGE_RESTORE_ROW_ALIGNMENT);
    if (padded_width == 0) return;
    if (st.input_column < st.width && st.input_row < st.height) consume_image_restore_pixel(color);
    st.input_column++;
    if (st.input_column >= padded_width) {
        st.input_column = 0;
        st.input_row++;
        if (st.input_row >= st.height) s_ga.stream.kind = STREAM_INACTIVE;
    }
}

static void consume_image_restore_direct16_word(UINT16 value)
{
    if (s_ga.stream.kind != STREAM_IMAGE_RESTORE) return;

    ImageRestoreState& st = s_ga.stream.image;

    UINT32 padded_width = align_up(st.width, DIRECT_COLOR16_IMAGE_RESTORE_ROW_ALIGNMENT);
    if (padded_width == 0) return;

    if (st.input_column < st.width && st.input_row < st.height) {
        consume_image_restore_pixel(value);

        if (s_ga.stream.kind != STREAM_IMAGE_RESTORE) {
            return;
        }
    }

    st.input_column++;
    if (st.input_column >= padded_width) {
        st.input_column = 0;
        st.input_row++;
        if (st.input_row >= st.height) {
            //TRACEOUT(("IMGREST END by padded_row size=(%u,%u)", st.width, st.height));
            finish_image_restore();
        }
    }
}

static void consume_image_restore_rgb_byte(UINT8 value)
{
    if (s_ga.stream.kind != STREAM_IMAGE_RESTORE) return;
    ImageRestoreState& st = s_ga.stream.image;
    st.byte_accumulator[st.byte_phase] = value;
    st.byte_phase = (UINT8)((st.byte_phase + 1) % 3);
    if (st.byte_phase == 0) {
        UINT32 color = ((UINT32)st.byte_accumulator[0] << 16) | ((UINT32)st.byte_accumulator[1] << 8) | st.byte_accumulator[2];
        consume_image_restore_pixel(color);
    }
}

static void consume_image_restore_direct16_byte(UINT8 value)
{
    if (s_ga.stream.kind != STREAM_IMAGE_RESTORE) return;

    ImageRestoreState& st = s_ga.stream.image;

    if (st.byte_phase == 0) {
        st.byte_accumulator[0] = value;
        st.byte_phase = 1;
        return;
    }

    UINT16 word = (UINT16)(st.byte_accumulator[0] | ((UINT16)value << 8));
    st.byte_phase = 0;

    consume_image_restore_direct16_word(word);
}

static void consume_image_restore_byte(UINT8 value)
{
    if (s_ga.plane_mode == PLANE_INDEXED8) {
        consume_image_restore_indexed_pixel(value);
    }
    else if (s_ga.plane_mode == PLANE_DIRECTCOLOR16) {
        consume_image_restore_direct16_byte(value);
    }
    else {
        consume_image_restore_rgb_byte(value);
    }
}

static void consume_image_restore_word(UINT16 value)
{
    if (s_ga.plane_mode == PLANE_INDEXED8) {
        consume_image_restore_indexed_pixel(value & 0xff);
        consume_image_restore_indexed_pixel((value >> 8) & 0xff);
    }
    else if (s_ga.plane_mode == PLANE_DIRECTCOLOR16) {
        //TRACEOUT(("X=%d, Y=%d, INDEX=%d", s_ga.stream.image.input_column, s_ga.stream.image.input_row, s_ga.stream.image.pixel_index));
        consume_image_restore_direct16_word(value);
    }
    else {
        consume_image_restore_rgb_byte((UINT8)value);
        consume_image_restore_rgb_byte((UINT8)(value >> 8));
    }
}

static void execute_scanline_pixel_read(void)
{
    PixelReadState& st = s_ga.stream.pixel;
    ZeroMemory(&st, sizeof(st));
    st.x = s_ga.srcx;
    st.y = s_ga.srcy;
    st.width = (UINT32)s_ga.opd1 + 1;
    st.height = (UINT32)s_ga.opd2 + 1;
    s_ga.stream.kind = STREAM_PIXEL_READ;
    s_ga.pdt_read_phase = 0;
    s_ga.pdt_write_low_valid = false;
}

static void execute_rect_pixel_read(void)
{
    PixelReadState& st = s_ga.stream.pixel;
    ZeroMemory(&st, sizeof(st));

    st.x = s_ga.srcx;
    st.y = s_ga.srcy;
    st.width = (UINT32)s_ga.opd1 + 1;
    st.height = (UINT32)s_ga.opd2 + 1;
    st.row = 0;
    st.column = 0;

    s_ga.stream.kind = STREAM_PIXEL_READ;
    s_ga.pdt_read_phase = 0;
    s_ga.pdt_write_low_valid = false;
}

static void execute_pixel_read(void)
{
    UINT32 width = (UINT32)s_ga.opd1 + 1;
    UINT32 height = (UINT32)s_ga.opd2 + 1;

    if (s_ga.plane_mode == PLANE_DIRECTCOLOR16 && (width > 1 || height > 1)) {
        execute_rect_pixel_read();
        return;
    }

    if (s_ga.pop1 == POP1_SCANLINE_PIXEL_READ) {
        execute_scanline_pixel_read();
        return;
    }

    s_ga.stream.kind = STREAM_INACTIVE;
    s_ga.pdt_write_low_valid = false;

    UINT32 color = read_pixel_color(s_ga.srcx, s_ga.srcy);
    if (s_ga.plane_mode == PLANE_FULLCOLOR24) {
        s_ga.pdt_latch[0] = (UINT16)color;
        s_ga.pdt_latch[1] = (UINT16)((color >> 16) & 0x00ff);
        s_ga.pdt_latch[2] = 0;
        s_ga.pdt_latch[3] = 0;
    }
    else {
        UINT16 value = (UINT16)color;
        s_ga.pdt_latch[0] = value;
        s_ga.pdt_latch[1] = value;
        s_ga.pdt_latch[2] = value;
        s_ga.pdt_latch[3] = value;
    }

    s_ga.pdt = (UINT16)color;
    s_ga.pdt_read_phase = 0;
}

static bool pixel_read_selected_plane_bit(UINT32 x, UINT32 y)
{
    if (x >= pixel_map_width() || y >= pixel_map_height()) return false;

    UINT32 plane_mask = active_read_plane_mask();
    if (plane_mask == 0) return false;

    return (read_packed_pixel(x, y) & plane_mask) != 0;
}

static void advance_pixel_read_word(PixelReadState& st)
{
    st.column += PIXEL_READ_WORD_WIDTH;
    if (st.column >= st.width) {
        st.column = 0;
        st.row++;
        if (st.row >= st.height) s_ga.stream.kind = STREAM_INACTIVE;
    }
}

static void advance_pixel_read_columns(PixelReadState& st, UINT32 columns, UINT32 input_width)
{
    st.column += columns;

    if (st.column >= input_width) {
        st.column = 0;
        st.row++;

        if (st.row >= st.height) {
            s_ga.stream.kind = STREAM_INACTIVE;
        }
    }
}

static UINT16 read_pixel_read_direct16_word(void)
{
    PixelReadState& st = s_ga.stream.pixel;

    UINT32 input_width = align_up(st.width, DIRECT_COLOR16_IMAGE_RESTORE_ROW_ALIGNMENT);
    if (input_width == 0 || st.row >= st.height) {
        s_ga.stream.kind = STREAM_INACTIVE;
        return s_ga.pdt;
    }

    UINT16 value = 0;

    if (st.column < st.width) {
        value = (UINT16)read_pixel_color(st.x + st.column, st.y + st.row);
    }

    s_ga.pdt = value;
    //if (st.width > 512) {
    //    st.width = st.width;
    //    //TRACEOUT(("V:%04x", value));
    //}
    advance_pixel_read_columns(st, 1, input_width);
    return value;
}

static UINT16 read_pixel_read_word(void)
{
    if (s_ga.stream.kind != STREAM_PIXEL_READ) return s_ga.pdt;

    if (s_ga.plane_mode == PLANE_DIRECTCOLOR16) {
        return read_pixel_read_direct16_word();
    }

    PixelReadState& st = s_ga.stream.pixel;
    UINT16 value = 0;

    for (UINT32 i = 0; i < PIXEL_READ_WORD_WIDTH; i++) {
        UINT32 col = st.column + i;
        if (col >= st.width || st.row >= st.height) continue;

        if (pixel_read_selected_plane_bit(st.x + col, st.y + st.row)) {
            value |= pattern_word_bit_mask(i);
        }
    }

    s_ga.pdt = value;
    advance_pixel_read_word(st);
    return value;
}

static void execute_pattern_expand_rectangle(bool opaque)
{
    PatternExpandState& st = s_ga.stream.pattern;
    ZeroMemory(&st, sizeof(st));
    st.x = s_ga.dstx;
    st.y = s_ga.dsty;
    st.width = (UINT32)s_ga.opd1 + 1;
    st.height = (UINT32)s_ga.opd2 + 1;
    st.foreground_color = s_ga.fcol;
    st.background_color = s_ga.bcol;
    st.foreground_mix = s_ga.fmix;
    st.background_mix = s_ga.bmix;
    st.opaque = opaque;
    s_ga.stream.kind = STREAM_PATTERN_EXPAND;
    s_ga.pdt_write_low_valid = false;
}

static void advance_pattern_expand_chunk(PatternExpandState& st, UINT32 input_width)
{
    st.column += 16;
    if (st.column >= input_width) {
        st.column = 0;
        st.row++;
        if (st.row >= st.height) s_ga.stream.kind = STREAM_INACTIVE;
    }
}

static void draw_pattern_expand_row(PatternExpandState& st, UINT16 source_word, UINT16 mask_word)
{
    for (UINT32 i = 0; i < 16; i++) {
        UINT32 col = st.column + i;
        if (col >= st.width || st.row >= st.height) continue;
        if ((mask_word & pattern_word_bit_mask(i)) == 0) continue;
        bool src = pattern_word_bit(source_word, i);
        if (src) write_pixel_rop(st.x + col, st.y + st.row, st.foreground_color, st.foreground_mix);
        else if (st.background_mix != MIX_DESTINATION) write_pixel_rop(st.x + col, st.y + st.row, st.background_color, st.background_mix);
    }
}

static UINT32 pattern_expand_input_width(const PatternExpandState& st)
{
    return align_up(st.width, st.opaque ? 32u : 16u);
}

static void consume_pattern_expand_word(UINT16 value)
{
    if (s_ga.stream.kind != STREAM_PATTERN_EXPAND) return;
    PatternExpandState& st = s_ga.stream.pattern;
    if (st.opaque) {
        draw_pattern_expand_row(st, value, 0xffff);
        advance_pattern_expand_chunk(st, pattern_expand_input_width(st));
        return;
    }
    if (st.word_phase == 0) {
        st.source_word = value;
        st.word_phase = 1;
    }
    else {
        draw_pattern_expand_row(st, st.source_word, value);
        st.word_phase = 0;
        advance_pattern_expand_chunk(st, pattern_expand_input_width(st));
    }
}

static void write_pdt_word(UINT16 value)
{
    s_ga.pdt_write_low_valid = false;
    s_ga.pdt = value;
    s_ga.pdt_latch[0] = value;
    s_ga.pdt_read_phase = 0;

    if (s_ga.stream.kind == STREAM_IMAGE_RESTORE) {
        //TRACEOUT(("PDT IMGREST value=%04X plane=%d", value, s_ga.plane_mode));
        consume_image_restore_word(value);
    }
    else if (s_ga.stream.kind == STREAM_PIXEL_READ) {
        s_ga.stream.kind = STREAM_INACTIVE;
    }
    else if (s_ga.stream.kind == STREAM_PATTERN_EXPAND) {
        consume_pattern_expand_word(value);
    }
}

static UINT16 read_pdt_word(void)
{
    if (s_ga.stream.kind == STREAM_PIXEL_READ) return read_pixel_read_word();
    UINT8 phase = (UINT8)(s_ga.pdt_read_phase & 3);
    UINT16 value = s_ga.pdt_latch[phase];
    s_ga.pdt = value;
    s_ga.pdt_read_phase = (UINT8)((s_ga.pdt_read_phase + 1) & 3);
    return value;
}

static void warn_unknown_command(UINT16)
{
    s_ga.unknown_command_warning_count++;
}

static void execute_pop2(UINT16 opcode)
{
    TRACEOUT(("TEXT POP2=%04X src=(%u,%u) dst=(%u,%u) size=(%u,%u) "
        "FCOL=%04X BCOL=%04X MIX=%02X/%02X MOD=%02X WPM=%04X WBM=%04X RPE=%02X%02X",
        opcode,
        s_ga.srcx, s_ga.srcy,
        s_ga.dstx, s_ga.dsty,
        (UINT32)s_ga.opd1 + 1,
        (UINT32)s_ga.opd2 + 1,
        s_ga.fcol, s_ga.bcol,
        s_ga.fmix, s_ga.bmix,
        s_ga.mod1,
        s_ga.wpm, s_ga.wbm,
        s_ga.rpe_high, s_ga.rpe));
    if (opcode == OPCODE_SOLID_RECTANGLE || opcode == OPCODE_SOLID_RECTANGLE_SOURCE || opcode == OPCODE_SOLID_RECTANGLE_ALTERNATE) {
        execute_solid_rectangle();
    }
    else if (opcode == OPCODE_ROP_SOLID_RECTANGLE_FOREGROUND || opcode == OPCODE_ROP_SOLID_RECTANGLE_ALTERNATE || opcode == OPCODE_HGA_ROP_SOLID_RECTANGLE_FOREGROUND) {
        execute_rop_solid_rectangle_foreground();
    }
    else if (opcode == OPCODE_ROP_RECTANGLE_FOREGROUND) {
        //for (UINT32 r = 0; r < 8; r++) {
        //    char buf[256];
        //    char* p = buf;
        //    p += sprintf(p, "PATTERN sampled row %u:", r);
        //    for (UINT32 c = 0; c < 8; c++) {
        //        bool bit = rop_pattern_bit((UINT32)s_ga.dstx + c, (UINT32)s_ga.dsty + r);
        //        p += sprintf(p, " %d", bit ? 1 : 0);
        //    }

        //    TRACEOUT(("%s", buf));
        //}
        execute_rop_rectangle_foreground();
    }
    else if (opcode == OPCODE_DSTPHASE_ROP_RECTANGLE_FOREGROUND) {
        execute_dstphase_rop_rectangle_foreground();
    }
    else if (opcode == OPCODE_HOST_COLOR_EXPAND) {
        execute_host_color_expand();
    }
    else if (opcode == OPCODE_TILED_RECTANGLE) {
        TRACEOUT(("TILE50E8 src=(%u,%u) dst=(%u,%u) size=(%u,%u)",
            s_ga.srcx, s_ga.srcy, s_ga.dstx, s_ga.dsty,
            (UINT32)s_ga.opd1 + 1, (UINT32)s_ga.opd2 + 1));
        UINT32 source_x = s_ga.srcx;
        UINT32 source_y = s_ga.srcy;
        UINT32 tile_base_x = source_x & ~(TILE_WIDTH - 1);

        for (UINT32 r = 0; r < 8; r++) {
            char buf[256];
            char* p = buf;
            p += sprintf(p, "TILE sampled row %u:", r);

            for (UINT32 c = 0; c < 8; c++) {
                UINT32 sx = tile_base_x + r * 8 + ((source_x + c) & 7);
                UINT32 v = read_packed_pixel(sx, source_y) & 0xff;
                p += sprintf(p, " %02X", v);
            }

            TRACEOUT(("%s", buf));
        }
        execute_tiled_rectangle();
    }
    else if (opcode == OPCODE_PATTERN_EXPAND_RECTANGLE) {
        execute_pattern_expand_rectangle(false);
    }
    else if (opcode == OPCODE_OPAQUE_PATTERN_EXPAND_RECTANGLE) {
        execute_pattern_expand_rectangle(true);
    }
    else if (opcode == OPCODE_PIXEL_READ) {
        execute_pixel_read();
    }
    else if ((opcode & ~0x0007) == OPCODE_IMAGE_RESTORE) {
        execute_image_restore((UINT8)(opcode & 7));
    }
    else if ((opcode & ~0x0007) == OPCODE_HGA_ROP_IMAGE_RESTORE) {
        execute_rop_image_restore((UINT8)(opcode & 7));
    }
    else if ((opcode & ~0x0007) == OPCODE_HGA_COPY_RECTANGLE_BASE) {
        execute_copy_rectangle_with_mix((UINT8)(opcode & 7));
    }
    else if ((opcode & ~0x0007) == OPCODE_HGA_COPY_RECTANGLE_ALT_BASE) {
        if (is_shadow_glyph_mask_copy()) {
            execute_hga_copy_rectangle_alt_with_mix((UINT8)(opcode & 7));
        }
        else {
            execute_shadow_glyph_mask_copy_with_mix((UINT8)(opcode & 7));
        }
    }
    else if ((opcode & ~0x0007) == OPCODE_COPY_RECTANGLE_BASE) {
        if (is_shadow_glyph_mask_copy()) {
            execute_shadow_glyph_mask_copy_with_mix((UINT8)(opcode & 7));
        }
        else {
            execute_copy_rectangle((UINT8)(opcode & 7));
        }
    }
    else if ((opcode & ~0x0007) == OPCODE_SOLID_LINE_BASE) {
        execute_solid_line((UINT8)(opcode & 7));
    }
    else if ((opcode & ~0x0007) == OPCODE_STYLED_LINE_BASE) {
        execute_styled_line((UINT8)(opcode & 7));
    }
    else if ((opcode & ~0x0007) == OPCODE_ROP_LINE_BASE || (opcode & ~0x0007) == OPCODE_HGA_ROP_LINE_BASE) {
        execute_rop_line((UINT8)(opcode & 7));
    }
    else {
        warn_unknown_command(opcode);
    }
}

static UINT8 ramdac_component_to_8(UINT8 value)
{
#if defined(GA1280A_RAMDAC_6BIT)
    value &= 0x3f;
    return (UINT8)((value << 2) | ((value & 1) ? 3 : 0));
#else
    return value;
#endif
}

static UINT32 rgb565_to_xrgb(UINT16 c)
{
    UINT32 r = (c >> 11) & 0x1f;
    UINT32 g = (c >> 5) & 0x3f;
    UINT32 b = c & 0x1f;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return (r << 16) | (g << 8) | b;
}

static UINT8 display_palette_index(UINT32 pixel)
{
    if (indexed8_plane_page_context()) {
        UINT32 mask = s_ga.vdac_mask & 0xffu;
        if (mask != 0 && mask != 0xffu) {
            return (UINT8)unpack_color_from_plane_mask(pixel, mask);
        }
    }
    return (UINT8)pixel;
}

static UINT32 pixel_to_xrgb(UINT32 pixel)
{
    if (s_ga.plane_mode == PLANE_INDEXED8) {
        UINT8 index = display_palette_index(pixel);
        return ((UINT32)ramdac_component_to_8(s_ga.palette[index][0]) << 16) |
            ((UINT32)ramdac_component_to_8(s_ga.palette[index][1]) << 8) |
            ramdac_component_to_8(s_ga.palette[index][2]);
    }
    if (s_ga.plane_mode == PLANE_DIRECTCOLOR16) return rgb565_to_xrgb((UINT16)pixel);
    return pixel & 0x00ffffff;
}

static UINT32 cursor_color_xrgb(int index)
{
    index &= 1;
    return ((UINT32)ramdac_component_to_8(s_ga.cursor_colors[index][0]) << 16) |
        ((UINT32)ramdac_component_to_8(s_ga.cursor_colors[index][1]) << 8) |
        ramdac_component_to_8(s_ga.cursor_colors[index][2]);
}

static void render_cursor(UINT32* dst, UINT32 width, UINT32 height)
{
    if (!s_ga.cursor_visible) return;

    int remOffset = (CURSOR_PATTERN_BYTES - s_ga.cursor_pattern_index) & (CURSOR_PATTERN_BYTES - 1);

    SINT32 cx = (SINT32)s_ga.cursor_x - 32;
    SINT32 cy = (SINT32)s_ga.cursor_y - 32;

    UINT32 fg = cursor_color_xrgb(0);
    UINT32 bg = cursor_color_xrgb(1);

    for (UINT32 y = 0; y < 32; y++) {
        SINT32 dy = cy + (SINT32)y;
        if (dy < 0 || dy >= (SINT32)height) continue;
        for (UINT32 x = 0; x < 32; x++) {
            SINT32 dx = cx + (SINT32)x;
            if (dx < 0 || dx >= (SINT32)width) continue;
            UINT32 byte_index = y * 4 + (x >> 3);
            UINT8 bit = (UINT8)(0x80 >> (x & 7));
            // XXX: 根拠無しの推測　マウスカーソルデータを中途半端に書くと、足りない分は0扱いでその分だけオフセットが付いた状態のデータが描画される。Win3.1 64k色でこのHackを使用している
            bool xor_bit = (byte_index >= remOffset) ? (s_ga.cursor.cursor_pattern[byte_index - remOffset] & bit) != 0 : false;
            bool and_bit = (byte_index + CURSOR_MASK_BYTES >= remOffset) ? (s_ga.cursor.cursor_pattern[byte_index + CURSOR_MASK_BYTES - remOffset] & bit) != 0 : false;
            UINT32& p = dst[dy * width + dx];
            if (!and_bit && !xor_bit) p = bg;
            else if (!and_bit && xor_bit) p = fg;
            else if (and_bit && xor_bit) p ^= 0x00ffffff;
        }
    }
}

static void IOOUTCALL ga1280a_ob(UINT port, REG8 dat)
{
    UINT8 selector, offset;
    if (!decode_port(port, &selector, &offset)) return;
    write_byte(selector, offset, dat);
    ga1280a.reg.b[(port >> 8) & 0x1f][port & 3] = dat;
}

static REG8 IOINPCALL ga1280a_ib(UINT port)
{
    UINT8 selector, offset, ret;
    if (decode_port(port, &selector, &offset) && read_byte(selector, offset, &ret)) return ret;
    return ga1280a.reg.b[(port >> 8) & 0x1f][port & 3];
}

void IOOUTCALL ga1280a_ow(UINT port, UINT16 dat)
{
    UINT8 selector, offset;
    if (!decode_port(port, &selector, &offset)) return;
    write_word(selector, offset & ~1, dat);
    ga1280a.reg.w[(port >> 8) & 0x1f][(port >> 1) & 1] = dat;
}

UINT16 IOINPCALL ga1280a_iw(UINT port)
{
    UINT8 selector, offset;
    UINT16 ret;
    if (decode_port(port, &selector, &offset) && read_word(selector, offset & ~1, &ret)) return ret;
    return ga1280a.reg.w[(port >> 8) & 0x1f][(port >> 1) & 1];
}

int MEMCALL ga1280a_memp_read8(UINT32 address, REG8* lpRetValue)
{
    UINT32 offset;
    UINT8 value;
    if (mapped_register_offset(address, &offset) && mapped_register_read_byte(offset, &value)) {
        *lpRetValue = value;
        return 1;
    }
    if (window_offset(address, &offset)) {
        *lpRetValue = host_window_read(offset);
        return 1;
    }
    if (flat_aperture_offset(address, 1, &offset)) {
        *lpRetValue = flat_aperture_read_byte_at_offset(offset);
        return 1;
    }
    return 0;
}

int MEMCALL ga1280a_memp_read16(UINT32 address, REG16* lpRetValue)
{
    UINT32 offset;
    UINT16 value;
    if (mapped_register_offset(address, &offset) && mapped_register_read_word(offset, &value)) {
        *lpRetValue = value;
        return 1;
    }
    if (flat_aperture_offset(address, 2, &offset)) {
        *lpRetValue = flat_aperture_read_word_at_offset(offset);
        return 1;
    }
    REG8 b0, b1;
    if (ga1280a_memp_read8(address, &b0) && ga1280a_memp_read8(address + 1, &b1)) {
        *lpRetValue = (REG16)(b0 | ((REG16)b1 << 8));
        return 1;
    }
    return 0;
}

int MEMCALL ga1280a_memp_read32(UINT32 address, UINT32* lpRetValue)
{
    UINT32 offset;
    if (flat_aperture_offset(address, 4, &offset)) {
        *lpRetValue = flat_aperture_read_dword_at_offset(offset);
        return 1;
    }
    REG16 w0, w1;
    if (ga1280a_memp_read16(address, &w0) && ga1280a_memp_read16(address + 2, &w1)) {
        *lpRetValue = (UINT32)w0 | ((UINT32)w1 << 16);
        return 1;
    }
    return 0;
}

int MEMCALL ga1280a_memp_write8(UINT32 address, REG8 value)
{
    UINT32 offset;
    if (window_offset(address, &offset)) {
        host_window_write(offset, value);
        return 1;
    }
    if (mapped_register_offset(address, &offset) && mapped_register_write_byte(offset, value)) return 1;
    if (flat_aperture_offset(address, 1, &offset)) {
        flat_aperture_write_byte_at_offset(offset, value);
        return 1;
    }
    return 0;
}

int MEMCALL ga1280a_memp_write16(UINT32 address, REG16 value)
{
    UINT32 offset;
    if (mapped_register_offset(address, &offset) && mapped_register_write_word(offset, value)) return 1;
    if (flat_aperture_offset(address, 2, &offset)) {
        flat_aperture_write_word_at_offset(offset, value);
        return 1;
    }
    if (ga1280a_memp_write8(address, (REG8)value) && ga1280a_memp_write8(address + 1, (REG8)(value >> 8))) return 1;
    return 0;
}

int MEMCALL ga1280a_memp_write32(UINT32 address, UINT32 value)
{
    UINT32 offset;
    if (flat_aperture_offset(address, 4, &offset)) {
        flat_aperture_write_dword_at_offset(offset, value);
        return 1;
    }
    if (ga1280a_memp_write16(address, (REG16)value) && ga1280a_memp_write16(address + 2, (REG16)(value >> 16))) return 1;
    return 0;
}

int ga1280a_drawGraphic(void)
{
    UINT32 updated = ga1280a.updated;
    UINT32 paletteUpdated = ga1280a.paletteUpdated;
    ga1280a.updated = 0;
    ga1280a.paletteUpdated = 0;

    if (!updated && !paletteUpdated) return 0;

    update_public_state();
    UINT32 width = s_ga.active_width;
    UINT32 height = s_ga.active_height;
    if (!width || !height) return 0;

    np2wab.realWidth = (int)width;
    np2wab.realHeight = (int)height;

    size_t pixels = (size_t)width * (size_t)height;
    if (s_framebuf.size() < pixels) s_framebuf.resize(pixels);

    UINT64 start = (UINT64)display_start() * display_pixels_per_crtc_unit();// +1024 * 16; // DEBUG
    UINT32 pmw = pixel_map_width();
    UINT32 pmh = pixel_map_height();
    UINT64 map_pixels = (UINT64)pmw * (UINT64)pmh;
    if (!pmw || !pmh || !map_pixels) return 0;

    start %= map_pixels;
    for (UINT32 y = 0; y < height; y++) {
        UINT64 line = (start + (UINT64)y * pmw) % map_pixels;
        for (UINT32 x = 0; x < width; x++) {
            UINT64 pos = (line + x) % map_pixels;
            UINT32 sx = (UINT32)(pos % pmw);
            UINT32 sy = (UINT32)(pos / pmw);
            UINT32 pixel = read_packed_pixel(sx, sy);
            s_framebuf[y * width + x] = pixel_to_xrgb(pixel);
        }
    }
    render_cursor(&s_framebuf[0], width, height);

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)width;
    bmi.bmiHeader.biHeight = -(LONG)height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    SetDIBitsToDevice(np2wabwnd.hDCBuf, 0, 0, width, height, 0, 0, 0, height, &s_framebuf[0], &bmi, DIB_RGB_COLORS);
    return 1;
}

void ga1280a_reset(const NP2CFG* pConfig)
{
    memset(&ga1280a, 0, sizeof(ga1280a));
    ga1280a.enabled = pConfig->usega1280a;
    ga1280a.width = DEFAULT_WIDTH;
    ga1280a.height = DEFAULT_HEIGHT;
    init_state();
    ga1280a_update_memp_map();
    if (ga1280a.enabled && pConfig->uselgy98 && (pConfig->lgy98io >> 8) <= 0x20) {
        msgbox("I/O port conflict", "I/O port conflict between the GA-1280A and the LGY-98 has been detected.");
    }
}

void ga1280a_bind(void)
{
    ga1280a_update_memp_map();
    if (ga1280a.enabled) {
        for (int i = 1; i < 0x20; i++) {
            for (int j = 0xd8; j <= 0xdb; j++) {
                iocore_attachout(j | (i << 8), ga1280a_ob);
                iocore_attachinp(j | (i << 8), ga1280a_ib);
            }
        }
        iocore_attachout(FIXED_WINDOW_PORT, ga1280a_ob);
        iocore_attachinp(FIXED_WINDOW_PORT, ga1280a_ib);
        iocore_attachout(FIXED_WINDOW_PORT + 1, ga1280a_ob);
        iocore_attachinp(FIXED_WINDOW_PORT + 1, ga1280a_ib);
    }
}

void ga1280a_unbind(void)
{
    ga1280a_unregister_memp_map();
    for (int i = 1; i < 0x20; i++) {
        for (int j = 0xd8; j <= 0xdb; j++) {
            iocore_detachout(j | (i << 8));
            iocore_detachinp(j | (i << 8));
        }
    }
    iocore_detachout(FIXED_WINDOW_PORT);
    iocore_detachinp(FIXED_WINDOW_PORT);
    iocore_detachout(FIXED_WINDOW_PORT + 1);
    iocore_detachinp(FIXED_WINDOW_PORT + 1);
}

void ga1280a_shutdown()
{
    ga1280a_unregister_memp_map();
    s_line_points.clear();
    s_framebuf.clear();
}



// ---------- state save

#define GA1280A_SF_VERSION 1

/*
 * GA-1280A state-save payload format.
 *
 *   UINT32 version
 *   UINT32 payload_size
 *
 * If ga1280a.enabled is zero, payload_size is zero and no structure blocks
 * follow.  Loading such a state resets GA-1280A to the disabled reset state.
 *
 * If ga1280a.enabled is non-zero, the payload is:
 *
 *     UINT32 ga1280a_size
 *     UINT8  ga1280a_data[ga1280a_size]
 *     UINT32 s_ga_size
 *     UINT8  s_ga_data[s_ga_size]
 *
 * The top-level payload_size allows this module to skip future appended data.
 * Each structure also has its own size field so older states can zero-fill
 * missing tail fields and newer states can discard extra tail fields per
 * structure without shifting the following block.
 */

static void ga1280a_sfappend(std::vector<UINT8>& buffer, const void* data, UINT32 size)
{
    const UINT8* p = (const UINT8*)data;
    if (size) buffer.insert(buffer.end(), p, p + size);
}

static void ga1280a_sfappend_block(std::vector<UINT8>& buffer, const void* data, UINT32 size)
{
    ga1280a_sfappend(buffer, &size, sizeof(size));
    ga1280a_sfappend(buffer, data, size);
}

static int ga1280a_sfread_discard(STFLAGH sfh, UINT32 size)
{
    UINT8 discard[1024];
    while (size) {
        UINT32 step = (size < (UINT32)sizeof(discard)) ? size : (UINT32)sizeof(discard);
        int ret = statflag_read(sfh, discard, step);
        if (ret != STATFLAG_SUCCESS) return ret;
        size -= step;
    }
    return STATFLAG_SUCCESS;
}

static void ga1280a_sfload_disabled_state(void)
{
    ZeroMemory(&ga1280a, sizeof(ga1280a));
    ga1280a.width = DEFAULT_WIDTH;
    ga1280a.height = DEFAULT_HEIGHT;
    ga1280a.enabled = 0;
    init_state();
    s_line_points.clear();
    s_framebuf.clear();
    ga1280a_update_memp_map();
}

static int ga1280a_sfread_block(STFLAGH sfh, UINT32* remaining, void* data, UINT32 size)
{
    UINT32 saved_size = 0;
    UINT32 copy_size;
    int ret;

    ZeroMemory(data, size);
    if (*remaining == 0) {
        return STATFLAG_SUCCESS;
    }
    if (*remaining < sizeof(saved_size)) {
        return STATFLAG_FAILURE;
    }

    ret = statflag_read(sfh, &saved_size, sizeof(saved_size));
    if (ret != STATFLAG_SUCCESS) return ret;
    *remaining -= sizeof(saved_size);

    if (saved_size > *remaining) return STATFLAG_FAILURE;

    copy_size = (saved_size < size) ? saved_size : size;
    if (copy_size) {
        ret = statflag_read(sfh, data, copy_size);
        if (ret != STATFLAG_SUCCESS) return ret;
        *remaining -= copy_size;
    }
    if (saved_size > copy_size) {
        ret = ga1280a_sfread_discard(sfh, saved_size - copy_size);
        if (ret != STATFLAG_SUCCESS) return ret;
        *remaining -= saved_size - copy_size;
    }
    return STATFLAG_SUCCESS;
}

int ga1280a_sfsave(STFLAGH sfh, const SFENTRY* tbl)
{
    UINT32 sfVersion = GA1280A_SF_VERSION;
    UINT32 statLen;
    int ret;
    std::vector<UINT8> buffer;

    (void)tbl;

    if (ga1280a.enabled) {
        ga1280a_sfappend_block(buffer, &ga1280a, (UINT32)sizeof(ga1280a));
        ga1280a_sfappend_block(buffer, &s_ga, (UINT32)sizeof(s_ga));
    }

    statLen = (UINT32)buffer.size();

    ret = statflag_write(sfh, &sfVersion, sizeof(sfVersion));
    if (ret != STATFLAG_SUCCESS) return ret;
    ret = statflag_write(sfh, &statLen, sizeof(statLen));
    if (ret != STATFLAG_SUCCESS) return ret;
    if (statLen) {
        ret = statflag_write(sfh, &buffer[0], statLen);
        if (ret != STATFLAG_SUCCESS) return ret;
    }
    return STATFLAG_SUCCESS;
}

int ga1280a_sfload(STFLAGH sfh, const SFENTRY* tbl)
{
    UINT32 sfVersion = 0;
    UINT32 statLen = 0;
    UINT32 remaining;
    int ret;

    (void)tbl;

    ret = statflag_read(sfh, &sfVersion, sizeof(sfVersion));
    if (ret != STATFLAG_SUCCESS) return ret;
    ret = statflag_read(sfh, &statLen, sizeof(statLen));
    if (ret != STATFLAG_SUCCESS) return ret;
    if (statLen == 0) {
        ga1280a_sfload_disabled_state();
        return STATFLAG_SUCCESS;
    }

    if (sfVersion != GA1280A_SF_VERSION) {
        return STATFLAG_VERSION;
    }

    remaining = statLen;

    ret = ga1280a_sfread_block(sfh, &remaining, &ga1280a, (UINT32)sizeof(ga1280a));
    if (ret != STATFLAG_SUCCESS) return ret;
    ret = ga1280a_sfread_block(sfh, &remaining, &s_ga, (UINT32)sizeof(s_ga));
    if (ret != STATFLAG_SUCCESS) return ret;

    if (remaining) {
        ret = ga1280a_sfread_discard(sfh, remaining);
        if (ret != STATFLAG_SUCCESS) return ret;
    }

    if (!ga1280a.enabled) {
        ga1280a_sfload_disabled_state();
        return STATFLAG_SUCCESS;
    }

    s_line_points.clear();
    s_framebuf.clear();
    rebuild_mmio_cache();
    update_public_state();
    ga1280a.updated = 1;
    ga1280a.paletteUpdated = 1;
    ga1280a_update_memp_map();

    return STATFLAG_SUCCESS;
}


#endif
