/*
 * QEMU Cirrus CLGD 54xx VGA Emulator.
 *
 * Copyright (c) 2004 Fabrice Bellard
 * Copyright (c) 2004 Makoto Suzuki (suzu)
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
/*
 * Reference: Finn Thogersons' VGADOC4b
 *   available at http://home.worldonline.dk/~finth/
 */

#pragma once
/*
#define cpu_to_le16wu(p, v) STOREINTELWORD(p, v) // XXX: 
#define cpu_to_le32wu(p, v) STOREINTELDWORD(p, v) // XXX: 

#define le16_to_cpu(a)		LOADINTELWORD(a)
#define le32_to_cpu(a)		LOADINTELDWORD(a)
#define cpu_to_le16w(a,b)	STOREINTELWORD(a,b)
#define cpu_to_le32w(a,b)	STOREINTELDWORD(a,b)
*/

#define cpu_to_le16wu(p, v) STOREINTELWORD(p, v) // XXX: 
#define cpu_to_le32wu(p, v) STOREINTELDWORD(p, v) // XXX: 

#define le16_to_cpu(a)		((UINT16)(a))
#define le32_to_cpu(a)		((UINT32)(a))
#define cpu_to_le16w(a,b)	STOREINTELWORD(a,b)
#define cpu_to_le32w(a,b)	STOREINTELDWORD(a,b)

#define TARGET_PAGE_BITS 12 // i386
#define TARGET_PAGE_SIZE (1 << TARGET_PAGE_BITS)
#define TARGET_PAGE_MASK ~(TARGET_PAGE_SIZE - 1)
#define TARGET_PAGE_ALIGN(addr) (((addr) + TARGET_PAGE_SIZE - 1) & TARGET_PAGE_MASK)

#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#define stringify(s)	tostring(s)
#define tostring(s)	#s

#define IO_MEM_SHIFT       3
#define IO_MEM_NB_ENTRIES  (1 << (TARGET_PAGE_BITS  - IO_MEM_SHIFT))

#define IO_MEM_RAM         (0 << IO_MEM_SHIFT) /* hardcoded offset */
#define IO_MEM_ROM         (1 << IO_MEM_SHIFT) /* hardcoded offset */
#define IO_MEM_UNASSIGNED  (2 << IO_MEM_SHIFT)
#define IO_MEM_NOTDIRTY    (3 << IO_MEM_SHIFT)

typedef unsigned long console_ch_t;

typedef void (*vga_hw_update_ptr)(void *);
typedef void (*vga_hw_invalidate_ptr)(void *);
typedef void (*vga_hw_screen_dump_ptr)(void *, const char *);
typedef void (*vga_hw_text_update_ptr)(void *, console_ch_t *);

typedef struct PixelFormat {
    uint8_t bits_per_pixel;
    uint8_t bytes_per_pixel;
    uint8_t depth; /* color depth in bits */
    uint32_t_ rmask, gmask, bmask, amask;
    uint8_t rshift, gshift, bshift, ashift;
    uint8_t rmax, gmax, bmax, amax;
    uint8_t rbits, gbits, bbits, abits;
} PixelFormat;

typedef struct DisplaySurface {
    uint8_t flags;
    int width;
    int height;
    int linesize;        /* bytes per line */
    uint8_t *data;

    struct PixelFormat pf;
} DisplaySurface;

typedef struct DisplayChangeListener {
    int idle;
    UINT64 gui_timer_interval;

    void (*dpy_update)(struct DisplayState *s, int x, int y, int w, int h);
    void (*dpy_resize)(struct DisplayState *s);
    void (*dpy_setdata)(struct DisplayState *s);
    void (*dpy_refresh)(struct DisplayState *s);
    void (*dpy_copy)(struct DisplayState *s, int src_x, int src_y,
                     int dst_x, int dst_y, int w, int h);
    void (*dpy_fill)(struct DisplayState *s, int x, int y,
                     int w, int h, uint32_t_ c);
    void (*dpy_text_cursor)(struct DisplayState *s, int x, int y);

    struct DisplayChangeListener *next;
} DisplayChangeListener;

struct DisplayState {
    struct DisplaySurface *surface;
    void *opaque;
    //struct QEMUTimer *gui_timer;

    struct DisplayChangeListener* listeners;

    void (*mouse_set)(int x, int y, int on);
    void (*cursor_define)(int width, int height, int bpp, int hot_x, int hot_y,
                          uint8_t *image, uint8_t *mask);

    struct DisplayState *next;
};
typedef struct DisplayState DisplayState;

DisplayState *graphic_console_init(vga_hw_update_ptr update,
                                   vga_hw_invalidate_ptr invalidate,
                                   vga_hw_screen_dump_ptr screen_dump,
                                   vga_hw_text_update_ptr text_update,
								   void *opaque);

typedef void QEMUResetHandler(void *opaque);

typedef void (IOPortWriteFunc)(void *opaque, uint32_t_ address, uint32_t_ data);
typedef uint32_t_ (IOPortReadFunc)(void *opaque, uint32_t_ address);

struct QEMUFile {
	int dummy;
};
typedef struct QEMUFile QEMUFile;



/// つくらんといかんね
target_phys_addr_t isa_mem_base = 0;

static void cpu_physical_memory_set_dirty(ram_addr_t addr)
{
    //phys_ram_dirty[addr >> TARGET_PAGE_BITS] = 0xff; // XXX:
}

void vga_hw_update(void){}

void qemu_console_copy(DisplayState *ds, int src_x, int src_y, int dst_x, int dst_y, int w, int h){}
/*{
    if (is_graphic_console()) {
        dpy_copy(ds, src_x, src_y, dst_x, dst_y, w, h);
    }
}*/
void cpu_physical_sync_dirty_bitmap(target_phys_addr_t start_addr, target_phys_addr_t end_addr){}

int register_ioport_write(int start, int length, int size, IOPortWriteFunc *func, void *opaque){return 0;}
int register_ioport_read(int start, int length, int size, IOPortReadFunc *func, void *opaque){return 0;}

//int cpu_register_io_memory(int io_index, CPUReadMemoryFunc **mem_read, CPUWriteMemoryFunc **mem_write, void *opaque){return 0;}
int cpu_register_io_memory(int io_index,
                           CPUReadMemoryFunc **mem_read,
                           CPUWriteMemoryFunc **mem_write,
                           void *opaque)
{
    //int i, subwidth = 0;

    //if (io_index <= 0) {
    //    io_index = 0;
    //} else {
    //    if (io_index >= IO_MEM_NB_ENTRIES)
    //        return -1;
    //}

    //for(i = 0;i < 3; i++) {
    //    if (!mem_read[i] || !mem_write[i])
    //        subwidth = IO_MEM_SUBWIDTH;
    //    io_mem_read[io_index][i] = mem_read[i];
    //    io_mem_write[io_index][i] = mem_write[i];
    //}
    //io_mem_opaque[io_index] = opaque;
    //return (io_index << IO_MEM_SHIFT) | subwidth;
	return 0;
}

void qemu_register_coalesced_mmio(target_phys_addr_t addr, ram_addr_t size){}

CPUWriteMemoryFunc **cpu_get_io_memory_write(int io_index){return NULL;}
CPUReadMemoryFunc **cpu_get_io_memory_read(int io_index){return NULL;}

void qemu_register_reset(QEMUResetHandler *func, void *opaque){}

void qemu_free(void *ptr)
{
    free(ptr);
}

void *qemu_malloc(size_t size)
{
    return malloc(size);
}

void *qemu_mallocz(size_t size)
{
    void *ptr;
    ptr = qemu_malloc(size);
    if (!ptr)
        return NULL;
    memset(ptr, 0, size);
    return ptr;
}
