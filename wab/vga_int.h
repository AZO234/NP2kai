/*
 * QEMU internal VGA defines.
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#define MSR_COLOR_EMULATION 0x01
#define MSR_PAGE_SELECT     0x20

#define ST01_V_RETRACE      0x08
#define ST01_DISP_ENABLE    0x01

/* bochs VBE support */
#define CONFIG_BOCHS_VBE

#define VBE_DISPI_MAX_XRES              1600
#define VBE_DISPI_MAX_YRES              1200
#define VBE_DISPI_MAX_BPP               32

#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9
#define VBE_DISPI_INDEX_NB              0xa

#define VBE_DISPI_ID0                   0xB0C0
#define VBE_DISPI_ID1                   0xB0C1
#define VBE_DISPI_ID2                   0xB0C2
#define VBE_DISPI_ID3                   0xB0C3
#define VBE_DISPI_ID4                   0xB0C4

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_GETCAPS               0x02
#define VBE_DISPI_8BIT_DAC              0x20
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

#define VBE_DISPI_LFB_PHYSICAL_ADDRESS  0xE0000000

#ifdef CONFIG_BOCHS_VBE

#define VGA_STATE_COMMON_BOCHS_VBE              \
    uint16_t_ vbe_index;                         \
    uint16_t_ vbe_regs[VBE_DISPI_INDEX_NB];      \
    uint32_t_ vbe_start_addr;                    \
    uint32_t_ vbe_line_offset;                   \
    uint32_t_ vbe_bank_mask;

#else

#define VGA_STATE_COMMON_BOCHS_VBE

#endif /* !CONFIG_BOCHS_VBE */

#define CH_ATTR_SIZE (160 * 100)
#define VGA_MAX_HEIGHT 2048

struct vga_precise_retrace {
    int64_t ticks_per_char;
    int64_t total_chars;
    int htotal;
    int hstart;
    int hend;
    int vstart;
    int vend;
    int freq;
};

union vga_retrace {
    struct vga_precise_retrace precise;
};

struct VGAState;
typedef uint8_t (* vga_retrace_fn)(struct VGAState *s);
typedef void (* vga_update_retrace_info_fn)(struct VGAState *s);

#define VGA_STATE_COMMON                                                \
    uint8_t *vram_ptr;                                                  \
    ram_addr_t vram_offset;                                             \
    unsigned int vram_size;                                             \
    uint32_t_ lfb_addr;                                                  \
    uint32_t_ lfb_end;                                                   \
    uint32_t_ map_addr;                                                  \
    uint32_t_ map_end;                                                   \
    uint32_t_ lfb_vram_mapped; /* whether 0xa0000 is mapped as ram */    \
    unsigned long bios_offset;                                          \
    unsigned int bios_size;                                             \
    int it_shift;                                                       \
    /*PCIDevice *pci_dev;*/                                                 \
    uint32_t_ latch;                                                     \
    uint8_t sr_index;                                                   \
    uint8_t sr[256];                                                    \
    uint8_t gr_index;                                                   \
    uint8_t gr[256];                                                    \
    uint8_t ar_index;                                                   \
    uint8_t ar[21];                                                     \
    int ar_flip_flop;                                                   \
    uint8_t cr_index;                                                   \
    uint8_t cr[256]; /* CRT registers */                                \
    uint8_t msr; /* Misc Output Register */                             \
    uint8_t fcr; /* Feature Control Register */                         \
    uint8_t st00; /* status 0 */                                        \
    uint8_t st01; /* status 1 */                                        \
    uint8_t dac_state;                                                  \
    uint8_t dac_sub_index;                                              \
    uint8_t dac_read_index;                                             \
    uint8_t dac_write_index;                                            \
    uint8_t dac_cache[3]; /* used when writing */                       \
    int dac_8bit;                                                       \
    uint8_t palette[768];                                               \
    int32_t bank_offset;                                                \
    int vga_io_memory;                                             \
    int (*get_bpp)(struct VGAState *s);                                 \
    void (*get_offsets)(struct VGAState *s,                             \
                        uint32_t_ *pline_offset,                         \
                        uint32_t_ *pstart_addr,                          \
                        uint32_t_ *pline_compare);                       \
    void (*get_resolution)(struct VGAState *s,                          \
                        int *pwidth,                                    \
                        int *pheight);                                  \
    VGA_STATE_COMMON_BOCHS_VBE                                          \
    /* display refresh support */                                       \
    DisplayState *ds;                                                   \
    uint32_t_ font_offsets[2];                                           \
    int graphic_mode;                                                   \
    uint8_t shift_control;                                              \
    uint8_t double_scan;                                                \
    uint32_t_ line_offset;                                               \
    uint32_t_ line_compare;                                              \
    uint32_t_ start_addr;                                                \
    uint32_t_ plane_updated;                                             \
    uint32_t_ last_line_offset;                                          \
    uint8_t last_cw, last_ch;                                           \
    uint32_t_ last_width, last_height; /* in chars or pixels */          \
    uint32_t_ last_scr_width, last_scr_height; /* in pixels */           \
    uint32_t_ last_depth; /* in bits */                                  \
    uint8_t cursor_start, cursor_end;                                   \
    uint32_t_ cursor_offset;                                             \
    unsigned int (*rgb_to_pixel)(unsigned int r,                        \
                                 unsigned int g, unsigned b);           \
    vga_hw_update_ptr update;                                           \
    vga_hw_invalidate_ptr invalidate;                                   \
    vga_hw_screen_dump_ptr screen_dump;                                 \
    vga_hw_text_update_ptr text_update;                                 \
    /* hardware mouse cursor support */                                 \
    uint32_t_ invalidated_y_table[VGA_MAX_HEIGHT / 32];                  \
    void (*cursor_invalidate)(struct VGAState *s);                      \
    void (*cursor_draw_line)(struct VGAState *s, uint8_t *d, int y);    \
    /* tell for each page if it has been updated since the last time */ \
    uint32_t_ last_palette[256];                                         \
    uint32_t_ last_ch_attr[CH_ATTR_SIZE]; /* XXX: make it dynamic */     \
    /* retrace */                                                       \
    vga_retrace_fn retrace;                                             \
    vga_update_retrace_info_fn update_retrace_info;                     \
    union vga_retrace retrace_info;


typedef struct VGAState {
    VGA_STATE_COMMON
} VGAState;

typedef void vga_draw_line_func(VGAState *s1, uint8_t *d, const uint8_t *s, int width);

static int c6_to_8(int v)
{
    int b;
    v &= 0x3f;
    b = v & 1;
    return (v << 2) | (b << 1) | b;
}

void vga_common_init(VGAState *s, uint8_t *vga_ram_base, ram_addr_t vga_ram_offset, int vga_ram_size){}
void vga_init(VGAState *s){}
void vga_reset(void *s){}

void vga_dirty_log_start(VGAState *s){}
void vga_dirty_log_stop(VGAState *s){}

#define cbswap_32(__x) \
((uint32_t_)( \
		(((uint32_t_)(__x) & (uint32_t_)0x000000ffUL) << 24) | \
		(((uint32_t_)(__x) & (uint32_t_)0x0000ff00UL) <<  8) | \
		(((uint32_t_)(__x) & (uint32_t_)0x00ff0000UL) >>  8) | \
		(((uint32_t_)(__x) & (uint32_t_)0xff000000UL) >> 24) ))

#ifdef WORDS_BIGENDIAN
#define PAT(x) x
#else
#define PAT(x) cbswap_32(x)
#endif

#ifdef WORDS_BIGENDIAN
#define GET_PLANE(data, p) (((data) >> (24 - (p) * 8)) & 0xff)
#else
#define GET_PLANE(data, p) (((data) >> ((p) * 8)) & 0xff)
#endif

static const uint32_t_ mask16[16] = {
    PAT(0x00000000),
    PAT(0x000000ff),
    PAT(0x0000ff00),
    PAT(0x0000ffff),
    PAT(0x00ff0000),
    PAT(0x00ff00ff),
    PAT(0x00ffff00),
    PAT(0x00ffffff),
    PAT(0xff000000),
    PAT(0xff0000ff),
    PAT(0xff00ff00),
    PAT(0xff00ffff),
    PAT(0xffff0000),
    PAT(0xffff00ff),
    PAT(0xffffff00),
    PAT(0xffffffff),
};

uint32_t_ vga_mem_readb(void *opaque, target_phys_addr_t addr){
	
    VGAState *s = (VGAState *)opaque;
    int memory_map_mode, plane;
    uint32_t_ ret;

    /* convert to VGA memory offset */
    memory_map_mode = (s->gr[6] >> 2) & 3;
    addr &= 0x1ffff;
    switch(memory_map_mode) {
    case 0:
        break;
    case 1:
        if (addr >= 0x10000)
            return 0xff;
        addr += s->bank_offset;
        break;
    case 2:
        addr -= 0x10000;
        if (addr >= 0x8000)
            return 0xff;
        break;
    default:
    case 3:
        addr -= 0x18000;
        if (addr >= 0x8000)
            return 0xff;
        break;
    }

    if (s->sr[4] & 0x08) {
        /* chain 4 mode : simplest access */
        ret = s->vram_ptr[addr];
    } else if (s->gr[5] & 0x10) {
        /* odd/even mode (aka text mode mapping) */
        plane = (s->gr[4] & 2) | (addr & 1);
        ret = s->vram_ptr[((addr & ~1) << 1) | plane];
    } else {
        /* standard VGA latched access */
        s->latch = ((uint32_t_ *)s->vram_ptr)[addr];

        if (!(s->gr[5] & 0x08)) {
            /* read mode 0 */
            plane = s->gr[4];
            ret = GET_PLANE(s->latch, plane);
        } else {
            /* read mode 1 */
            ret = (s->latch ^ mask16[s->gr[2]]) & mask16[s->gr[7]];
            ret |= ret >> 16;
            ret |= ret >> 8;
            ret = (~ret) & 0xff;
        }
    }
    return ret;
}
void vga_mem_writeb(void *opaque, target_phys_addr_t addr, uint32_t_ val){
    VGAState *s = (VGAState *)opaque;
    int memory_map_mode, plane, write_mode, b, func_select, mask;
    uint32_t_ write_mask, bit_mask, set_mask;

	////val = ((val & 0x3) << 6) | ((val & 0xc) << 2) | ((val & 0x30) >> 2) | ((val & 0xc0) >> 4);

#ifdef DEBUG_VGA_MEM
    printf("vga: [0x%x] = 0x%02x\n", addr, val);
#endif
    /* convert to VGA memory offset */
    memory_map_mode = (s->gr[6] >> 2) & 3;
    addr &= 0x1ffff;
    switch(memory_map_mode) {
    case 0:
        break;
    case 1:
        if (addr >= 0x10000)
            return;
        addr += s->bank_offset;
        break;
    case 2:
        addr -= 0x10000;
        if (addr >= 0x8000)
            return;
        break;
    default:
    case 3:
        addr -= 0x18000;
        if (addr >= 0x8000)
            return;
        break;
    }

    if (s->sr[4] & 0x08) {
        /* chain 4 mode : simplest access */
        plane = addr & 3;
        mask = (1 << plane);
        if (s->sr[2] & mask) {
            s->vram_ptr[addr] = val;
#ifdef DEBUG_VGA_MEM
            printf("vga: chain4: [0x%x]\n", addr);
#endif
            s->plane_updated |= mask; /* only used to detect font change */
            cpu_physical_memory_set_dirty(s->vram_offset + addr);
        }
    } else if (s->gr[5] & 0x10) {
        /* odd/even mode (aka text mode mapping) */
        plane = (s->gr[4] & 2) | (addr & 1);
        mask = (1 << plane);
        if (s->sr[2] & mask) {
            addr = ((addr & ~1) << 1) | plane;
            s->vram_ptr[addr] = val;
#ifdef DEBUG_VGA_MEM
            printf("vga: odd/even: [0x%x]\n", addr);
#endif
            s->plane_updated |= mask; /* only used to detect font change */
            cpu_physical_memory_set_dirty(s->vram_offset + addr);
        }
    } else {
        /* standard VGA latched access */
        write_mode = s->gr[5] & 3;
        switch(write_mode) {
        default:
        case 0:
            /* rotate */
            b = s->gr[3] & 7;
            val = ((val >> b) | (val << (8 - b))) & 0xff;
            val |= val << 8;
            val |= val << 16;

            /* apply set/reset mask */
            set_mask = mask16[s->gr[1]];
            val = (val & ~set_mask) | (mask16[s->gr[0]] & set_mask);
            bit_mask = s->gr[8];
            break;
        case 1:
            val = s->latch;
            goto do_write;
        case 2:
            val = mask16[val & 0x0f];
            bit_mask = s->gr[8];
            break;
        case 3:
            /* rotate */
            b = s->gr[3] & 7;
            val = (val >> b) | (val << (8 - b));

            bit_mask = s->gr[8] & val;
            val = mask16[s->gr[0]];
            break;
        }

        /* apply logical operation */
        func_select = s->gr[3] >> 3;
        switch(func_select) {
        case 0:
        default:
            /* nothing to do */
            break;
        case 1:
            /* and */
            val &= s->latch;
            break;
        case 2:
            /* or */
            val |= s->latch;
            break;
        case 3:
            /* xor */
            val ^= s->latch;
            break;
        }

        /* apply bit mask */
        bit_mask |= bit_mask << 8;
        bit_mask |= bit_mask << 16;
        val = (val & bit_mask) | (s->latch & ~bit_mask);

    do_write:
        /* mask data according to sr[2] */
        mask = s->sr[2];
        s->plane_updated |= mask; /* only used to detect font change */
        write_mask = mask16[mask];
		write_mask = ((write_mask & 0xff) << 24) | ((write_mask & 0xff00) << 8) | ((write_mask & 0xff0000) >> 8) | ((write_mask & 0xff000000) >> 24); // XXX: なんかひっくり返さないと駄目
        ((uint32_t_ *)s->vram_ptr)[addr] = (((uint32_t_ *)s->vram_ptr)[addr] & ~write_mask) | (val & write_mask);
#ifdef DEBUG_VGA_MEM
            printf("vga: latch: [0x%x] mask=0x%08x val=0x%08x\n",
                   addr * 4, write_mask, val);
#endif
            cpu_physical_memory_set_dirty(s->vram_offset + (addr << 2));
    }
}
void vga_invalidate_scanlines(VGAState *s, int y1, int y2){
    int y;
    if (y1 >= VGA_MAX_HEIGHT)
        return;
    if (y2 >= VGA_MAX_HEIGHT)
        y2 = VGA_MAX_HEIGHT;
    for(y = y1; y < y2; y++) {
        s->invalidated_y_table[y >> 5] |= 1 << (y & 0x1f);
    }
}
int ppm_save(const char *filename, struct DisplaySurface *ds){return 0;}

static void vga_draw_cursor_line(uint8_t *d1,
 
                                 const uint8_t *src1,
 
                                 int poffset, int w,
 
                                 unsigned int color0,
 
                                 unsigned int color1,
 
                                 unsigned int color_xor)
 
{
 
    const uint8_t *plane0, *plane1;
 
    int x, b0, b1;
 
    uint8_t *d;
 

 
    d = d1;
 
    plane0 = src1;
 
    plane1 = src1 + poffset;
 
    for (x = 0; x < w; x++) {
 
        b0 = (plane0[x >> 3] >> (7 - (x & 7))) & 1;

        b1 = (plane1[x >> 3] >> (7 - (x & 7))) & 1;

        switch (b0 | (b1 << 1)) {

        case 0:

            break;

        case 1:

            ((uint32_t_ *)d)[0] ^= color_xor;
			((uint32_t_ *)d)[0] |= 0xff000000;

            break;

        case 2:

            ((uint32_t_ *)d)[0] = color0;
			((uint32_t_ *)d)[0] |= 0xff000000;
            break;

        case 3:

            ((uint32_t_ *)d)[0] = color1;
			((uint32_t_ *)d)[0] |= 0xff000000;

            break;

        }

        d += 4;

    }

}
//
//void vga_draw_cursor_line_8(uint8_t *d1, const uint8_t *src1,
//                            int poffset, int w,
//                            unsigned int color0, unsigned int color1,
//							unsigned int color_xor);
//void vga_draw_cursor_line_16(uint8_t *d1, const uint8_t *src1,
//                             int poffset, int w,
//                             unsigned int color0, unsigned int color1,
//							 unsigned int color_xor);
//void vga_draw_cursor_line_32(uint8_t *d1, const uint8_t *src1,
//                             int poffset, int w,
//                             unsigned int color0, unsigned int color1,
//							 unsigned int color_xor);

extern const uint8_t sr_mask[8];
extern const uint8_t gr_mask[16];
