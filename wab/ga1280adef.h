/**
 * @file	ga1280adef.h
 * @brief	Definition of the I-O DATA GA-1280A
 */

#pragma once

#if defined(SUPPORT_WAB_GA1280A)

static const UINT8 ID_STREAM[16] = { '.','O',' ','D','A','T','A',' ','D','E','V','I','C','E',' ','I' };

enum {
    DEFAULT_GAPORT = 0x00D8,
    GA1280_MAX_VISIBLE_WIDTH = 1600,
    GA1280_MAX_VISIBLE_HEIGHT = 1024,
    GA1280_MAX_PIXEL_MAP_WIDTH = 2048,
    GA1280_MAX_PIXEL_MAP_HEIGHT = 2048,
    DEFAULT_WIDTH = 640,
    DEFAULT_HEIGHT = 480,
    FULL_COLOR_WIDTH = 512,
    FULL_COLOR_HEIGHT = 480,
    CURSOR_MASK_BYTES = 128,
    CURSOR_PATTERN_BYTES = CURSOR_MASK_BYTES * 2,
    TILE_PATTERN_WORDS = 8,
    ROP_PATTERN_ROWS = 8,
    GA1280_VRAM_BYTES = 2 * 1024 * 1024,

    SELECTOR_INDEX = 0x00,
    SELECTOR_SRW = 0x01,
    SELECTOR_SRR = 0x02,
    SELECTOR_WPM = 0x03,
    SELECTOR_WBM = 0x05,
    SELECTOR_PRS = 0x06,
    SELECTOR_RPE = 0x07,
    SELECTOR_COL = 0x09,
    SELECTOR_TILE = 0x0B,
    SELECTOR_ROT = 0x0D,
    SELECTOR_MOD = 0x0E,
    SELECTOR_UNKNOWN_0F = 0x0F,
    SELECTOR_FCOL = 0x10,
    SELECTOR_BCOL_PMW = 0x12,
    SELECTOR_PMH = 0x13,
    SELECTOR_MIX = 0x14,
    SELECTOR_CWB_UNKNOWN = 0x15,
    SELECTOR_WBA1 = 0x16,
    SELECTOR_WBA2 = 0x17,
    SELECTOR_VDAC_ARW_RS = 0x18,
    SELECTOR_VDAC_ARR = 0x19,
    SELECTOR_VDAC_CPR = 0x1A,
    SELECTOR_VDAC_MSK = 0x1B,
    SELECTOR_SYSTEM_PDT = 0x1C,
    SELECTOR_STATUS_SSV = 0x1D,
    SELECTOR_CRTC_POP1 = 0x1E,
    SELECTOR_CRTC_POP2 = 0x1F,

    OFFSET_BASE = 0,
    OFFSET_BASE_PLUS_ONE = 1,
    OFFSET_PLUS_TWO = 2,
    OFFSET_PLUS_THREE = 3,

    FIXED_WINDOW_PORT = 0x1600,
    COMPATIBILITY_MAPPED_REGISTER_BASE_OFFSET = 0x1F00,
    COMPATIBILITY_MAPPED_REGISTER_PLUS_TWO_OFFSET = 0x1F40,
    MAPPED_REGISTER_APERTURE_BYTES = 0x40,
    WBA_LOW_BYTE_SEGMENT_MASK = 0x00FE,

    CONVENTIONAL_WINDOW_BASE = 0x000C0000,
    CONVENTIONAL_WINDOW_BYTES = 0x30000,
    FLAT_APERTURE_BASE = 0x00F00000,
    FLAT_APERTURE_BYTES = 0x10000,

    CRTC_INDEX_HORIZONTAL_TOTAL = 0x00,
    CRTC_INDEX_VERTICAL_TOTAL = 0x10,
    CRTC_INDEX_VERTICAL_DISPLAY_END = 0x12,
    CRTC_INDEX_VSYNC_STATUS = 0x1F,
    CRTC_INDEX_GA1280_VSYNC_STATUS = 0x3F,
    CRTC_INDEX_DISPLAY_START_LOW = 0x30,
    CRTC_INDEX_DISPLAY_START_MID = 0x31,
    CRTC_INDEX_DISPLAY_START_HIGH = 0x32,
    CRTC_BIT_VSYNC_ACTIVE = 0x02,
    CRTC_BIT_GA1280_VSYNC_ACTIVE = 0x0400,

    HOST_WRITE_PIXEL_MASK_MODE = 0x01,
    HOST_WRITE_ROTATE_WORD_MODE = 0x02,
    HOST_WRITE_COLOR_EXPAND_MODE = 0x04,

    OPCODE_SOLID_RECTANGLE = 0x6FE8,
    OPCODE_ROP_SOLID_RECTANGLE_FOREGROUND = 0x4AE8,
    OPCODE_ROP_SOLID_RECTANGLE_ALTERNATE = 0x42F8,
    OPCODE_HGA_ROP_SOLID_RECTANGLE_FOREGROUND = 0x6AE8,
    OPCODE_ROP_RECTANGLE_FOREGROUND = 0x6A28,
    OPCODE_DSTPHASE_ROP_RECTANGLE_FOREGROUND = 0x4A28,
    OPCODE_SOLID_RECTANGLE_SOURCE = 0x4FE8,
    OPCODE_SOLID_RECTANGLE_ALTERNATE = 0x4FF8,
    OPCODE_HOST_COLOR_EXPAND = 0x0AC8,
    OPCODE_TILED_RECTANGLE = 0x50E8,
    OPCODE_IMAGE_RESTORE = 0x45E8,
    OPCODE_HGA_ROP_IMAGE_RESTORE = 0x4528,
    OPCODE_PATTERN_EXPAND_RECTANGLE = 0x4688,
    OPCODE_OPAQUE_PATTERN_EXPAND_RECTANGLE = 0x4A88,
    OPCODE_PIXEL_READ = 0x20E8,
    OPCODE_HGA_COPY_RECTANGLE_BASE = 0x6028,
    OPCODE_HGA_COPY_RECTANGLE_ALT_BASE = 0x6008,
    OPCODE_COPY_RECTANGLE_BASE = 0x60E8,
    OPCODE_SOLID_LINE_BASE = 0x1FE8,
    OPCODE_STYLED_LINE_BASE = 0x1348,
    OPCODE_ROP_LINE_BASE = 0x1A48,
    OPCODE_HGA_ROP_LINE_BASE = 0x1A58,

    DIRECTION_Y_MAJOR = 0x01,
    DIRECTION_DESCENDING_Y = 0x02,
    DIRECTION_DESCENDING_X = 0x04,
    NORMAL_WRITE_BIT = 0x1000,
    MIX_XOR = 0x06,
    MIX_DESTINATION = 0x0A,
    MIX_SOURCE = 0x0C,
    CLIP_CONTROL_ENABLE = 0x0001,
    CLIP_CONTROL_OUTSIDE = 0x0002,
    POP1_SCANLINE_PIXEL_READ = 0x3000,
    INDEXED_IMAGE_RESTORE_ROW_ALIGNMENT = 4,
    DIRECT_COLOR16_IMAGE_RESTORE_ROW_ALIGNMENT = 2,
    PIXEL_READ_WORD_WIDTH = 16,
    TILE_WIDTH = 8,
    TILE_HEIGHT = 8
};

enum GAPlaneMode {
    PLANE_INDEXED8,
    PLANE_DIRECTCOLOR16,
    PLANE_FULLCOLOR24
};

enum GAWindowSize {
    WINSIZE_DISABLED,
    WINSIZE_16K,
    WINSIZE_32K,
    WINSIZE_64K,
    WINSIZE_128K
};

enum GAStreamKind {
    STREAM_INACTIVE,
    STREAM_IMAGE_RESTORE,
    STREAM_PIXEL_READ,
    STREAM_PATTERN_EXPAND
};

enum PixelMix {
    PIXEL_MIX_SOURCE,
    PIXEL_MIX_XOR
};

struct ImageRestoreState {
    UINT32 x;
    UINT32 y;
    UINT32 width;
    UINT32 height;
    UINT32 pixel_index;
    UINT32 input_column;
    UINT32 input_row;
    UINT8 byte_phase;
    UINT8 byte_accumulator[3];
    bool xor_pixels;
    UINT8 direction;
    bool has_rop;
    UINT8 rop;
};

struct PixelReadState {
    UINT32 x;
    UINT32 y;
    UINT32 width;
    UINT32 height;
    UINT32 row;
    UINT32 column;
};

struct PatternExpandState {
    UINT32 x;
    UINT32 y;
    UINT32 width;
    UINT32 height;
    UINT32 row;
    UINT32 column;
    UINT8 word_phase;
    UINT16 source_word;
    UINT32 foreground_color;
    UINT32 background_color;
    UINT8 foreground_mix;
    UINT8 background_mix;
    bool opaque;
};

struct StreamState {
    GAStreamKind kind;
    ImageRestoreState image;
    PixelReadState pixel;
    PatternExpandState pattern;
};

struct LinePoint {
    UINT32 step;
    SINT32 x;
    SINT32 y;
};

struct ModeRefreshEntry {
    UINT16 h;
    UINT16 v;
    UINT32 hz;
};

static const ModeRefreshEntry MODE_REFRESH_TABLE[] = {
    {0x009D, 0x032E, 86}, {0x0063, 0x020B, 60}, {0x0069, 0x01B6, 56},
    {0x007F, 0x026F, 56}, {0x00A7, 0x0324, 60}, {0x00A5, 0x0324, 70},
    {0x00A6, 0x0332, 80}, {0x0082, 0x0298, 72}, {0x0067, 0x0206, 72},
    {0x00CA, 0x03F6, 60}, {0x00CC, 0x03DC, 66}, {0x0067, 0x01EE, 51},
    {0x0087, 0x01EE, 52}, {0x0079, 0x01EE, 52}, {0x00D1, 0x042B, 60},
    {0x008C, 0x0236, 56}, {0x0106, 0x046C, 90}, {0x00A6, 0x020B, 60},
    {0x0100, 0x041D, 60}
};

struct GA1280A_STATE {
    UINT16 gaport;
    UINT8 id_stream_cursor;
    UINT16 index;
    UINT16 srw;
    UINT16 srr;
    UINT16 wpm;
    UINT16 wbm;
    UINT8 prs;
    UINT8 prs_high;
    UINT8 rpe;
    UINT8 rpe_high;
    UINT16 col;
    UINT16 tile;
    UINT16 tile_pattern[TILE_PATTERN_WORDS];
    UINT8 tile_pattern_count;
    UINT8 tile_write_index;
    UINT8 tile_read_index;
    UINT8 rop_pattern[ROP_PATTERN_ROWS];
    UINT8 rop_pattern_index;
    UINT8 rot;
    UINT8 rot_high;
    UINT8 mod1;
    UINT8 mod2;
    UINT16 fcol;
    UINT16 bcol;
    UINT8 fmix;
    UINT8 bmix;
    UINT16 cwb;
    UINT16 clip_sx;
    UINT16 clip_sy;
    UINT16 clip_ex;
    UINT16 clip_ey;
    bool clip_enabled;
    bool clip_outside;
    UINT16 wba1;
    GAWindowSize last_wba1_window_size;
    UINT16 wba2;
    UINT8 palette_index_write;
    UINT8 palette_index_read;
    UINT8 palette_rgb_phase;
    UINT8 palette[256][3];
    UINT8 vdac_mask;
    UINT8 vdac_rs;
    UINT8 cursor_color_index;
    UINT8 cursor_color_rgb_phase;
    UINT8 cursor_colors[2][3];
    UINT16 cursor_pattern_index;
    union {
        UINT8 cursor_pattern[CURSOR_MASK_BYTES * 2];
        struct {
            UINT8 cursor_xor_pattern[CURSOR_MASK_BYTES];
            UINT8 cursor_and_pattern[CURSOR_MASK_BYTES];
        } sepa;
    } cursor;
    UINT16 cursor_x;
    UINT16 cursor_y;
    bool cursor_visible;
    UINT16 system_register;
    UINT8 system_auxiliary_register;
    UINT8 crtc_index;
    UINT16 crtc_registers[128];
    UINT32 active_width;
    UINT32 active_height;
    GAPlaneMode plane_mode;
    UINT8 vram[GA1280_VRAM_BYTES];
    UINT16 errs;
    UINT16 k1;
    UINT16 k2;
    UINT16 opd1;
    UINT16 opd2;
    UINT16 lins;
    UINT16 srcx;
    UINT16 srcy;
    UINT16 dstx;
    UINT16 dsty;
    UINT16 pmw;
    UINT16 pmh;
    UINT16 pdt;
    UINT16 pdt_latch[4];
    UINT8 pdt_read_phase;
    UINT8 pdt_write_low;
    bool pdt_write_low_valid;
    bool indexed8_high_color_mode;
    UINT16 ssv;
    UINT8 status_control;
    UINT16 pop1;
    UINT16 pop2;
    UINT16 unknown_sel_0f_off0;
    UINT16 unknown_sel_14_off2;
    UINT8 unknown_sel_15_off2;
    UINT64 register_write_count;
    UINT64 wba1_write_count;
    UINT64 crtc_write_count;
    UINT64 ramdac_write_count;
    UINT64 host_window_write_count;
    UINT64 flat_aperture_write_count;
    UINT64 reset_unknown_write_count;
    UINT64 unknown_command_warning_count;
    UINT64 unknown_mix_warning_count;
    bool vsync_active;
    UINT8 full_color_helper_step;
    StreamState stream;
};

struct GA1280A_MMIOCache {
    bool host_window_enabled;
    UINT32 host_window_base;
    UINT32 host_window_bytes;

    bool closed_mapped_register_enabled;
    UINT32 closed_mapped_register_base;
    UINT32 closed_mapped_register_mask;

    bool flat_aperture_enabled;
    UINT32 flat_aperture_base;
    UINT32 flat_aperture_bytes;

    UINT32 flat_window_bytes;

    GAWindowSize mapped_register_window_size;
    UINT32 mapped_register_window_bytes;
    bool mapped_register_aperture_enabled;
};

#endif