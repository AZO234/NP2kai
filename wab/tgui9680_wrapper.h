#pragma once

#if defined(SUPPORT_TRIDENT_TGUI)

typedef unsigned long long int uint64_t;
typedef unsigned long int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;
typedef signed long long int int64_t;
typedef signed long int int32_t;
typedef signed short int int16_t;
typedef signed char int8_t;

typedef struct mem_mapping_t
{
        struct mem_mapping_t *prev, *next;

        int enable;
                
        uint32_t base;
        uint32_t size;

        uint8_t  (*read_b)(uint32_t addr, void *priv);
        uint16_t (*read_w)(uint32_t addr, void *priv);
        uint32_t (*read_l)(uint32_t addr, void *priv);
        void (*write_b)(uint32_t addr, uint8_t  val, void *priv);
        void (*write_w)(uint32_t addr, uint16_t val, void *priv);
        void (*write_l)(uint32_t addr, uint32_t val, void *priv);
        
        uint8_t *exec;
        
        uint32_t flags;
        
        void *p;
} mem_mapping_t;

typedef struct rom_t
{
        uint8_t *rom;
        uint32_t mask;
        mem_mapping_t mapping;
} rom_t;

typedef struct
{
        uint8_t r, g, b;
} RGB;
typedef RGB PALETTE[256];

typedef struct pc_timer_t
{
	uint32_t ts_integer;
	uint32_t ts_frac;
	int enabled;

	void (*callback)(void *p);
	void *p;

	struct pc_timer_t *prev, *next;
} pc_timer_t;

typedef struct svga_t
{
        mem_mapping_t mapping;
        
        uint8_t crtcreg;
        uint8_t crtc[128];
        uint8_t gdcreg[64];
        int gdcaddr;
        uint8_t attrregs[32];
        int attraddr, attrff;
        int attr_palette_enable;
        uint8_t seqregs[64];
        int seqaddr;
        
        uint8_t miscout;
        int vidclock;

        /*The three variables below allow us to implement memory maps like that seen on a 1MB Trio64 :
          0MB-1MB - VRAM
          1MB-2MB - VRAM mirror
          2MB-4MB - open bus
          4MB-xMB - mirror of above
          
          For the example memory map, decode_mask would be 4MB-1 (4MB address space), vram_max would be 2MB
          (present video memory only responds to first 2MB), vram_mask would be 1MB-1 (video memory wraps at 1MB)
        */
        uint32_t decode_mask;
        uint32_t vram_max;
        uint32_t vram_mask;
        
        uint8_t la, lb, lc, ld;
        
        uint8_t dac_mask, dac_status;
        int dac_read, dac_write, dac_pos;
        int dac_r, dac_g;
                
        uint8_t cgastat;
        
        uint8_t plane_mask;
        
        int fb_only;
        
        int fast;
        uint8_t colourcompare, colournocare;
        int readmode, writemode, readplane;
        int chain4, chain2_write, chain2_read;
        uint8_t writemask;
        uint32_t charseta, charsetb;

        int set_reset_disabled;
        
        uint8_t egapal[16];
        uint32_t pallook[256];
        PALETTE vgapal;
        
        int ramdac_type;

        int vtotal, dispend, vsyncstart, split, vblankstart;
        int hdisp,  hdisp_old, htotal,  hdisp_time, rowoffset;
        int lowres, interlace;
        int linedbl, rowcount;
        double clock;
        uint32_t ma_latch;
        int bpp;
        
        uint64_t dispontime, dispofftime;
        pc_timer_t timer;
        
        uint8_t scrblank;
        
        int dispon;
        int hdisp_on;

        uint32_t ma, maback, ca;
        int vc;
        int sc;
        int linepos, vslines, linecountff, oddeven;
        int con, cursoron, blink;
        int scrollcache;
        int char_width;
        
        int firstline, lastline;
        int firstline_draw, lastline_draw;
        int displine;
        
        uint8_t *vram;
        uint8_t *changedvram;
        uint32_t vram_display_mask;
        uint32_t banked_mask;

        uint32_t write_bank, read_bank;
                
        int fullchange;
        
        int video_res_x, video_res_y, video_bpp;
        int frames, fps;

        struct
        {
                int ena;
                int x, y;
                int xoff, yoff;
                int xsize, ysize;
                uint32_t addr;
                uint32_t pitch;
                int v_acc, h_acc;
        } hwcursor, hwcursor_latch, overlay, overlay_latch;
        
        int hwcursor_on;
        int overlay_on;
        
        int hwcursor_oddeven;
        int overlay_oddeven;
        
        void (*render)(struct svga_t *svga);
        void (*recalctimings_ex)(struct svga_t *svga);

        void    (*video_out)(uint16_t addr, uint8_t val, void *p);
        uint8_t (*video_in) (uint16_t addr, void *p);

        void (*hwcursor_draw)(struct svga_t *svga, int displine);

        void (*overlay_draw)(struct svga_t *svga, int displine);
        
        void (*vblank_start)(struct svga_t *svga);
        
        /*Called when VC=R18 and friends. If this returns zero then MA resetting
          is skipped. Matrox Mystique in Power mode reuses this counter for
          vertical line interrupt*/
        int (*line_compare)(struct svga_t *svga);
        
        /*Called at the start of vertical sync*/
        void (*vsync_callback)(struct svga_t *svga);
        
        /*If set then another device is driving the monitor output and the SVGA
          card should not attempt to display anything */
        int override;
        void *p;

        uint8_t ksc5601_sbyte_mask;
        
        int vertical_linedbl;
        
        /*Used to implement CRTC[0x17] bit 2 hsync divisor*/
        int hsync_divisor;
} svga_t;

typedef struct tkd8001_ramdac_t
{
        int state;
        uint8_t ctrl;
} tkd8001_ramdac_t;

typedef void thread_t;
typedef void event_t;

#define CONFIG_SELECTION 3

typedef struct device_config_selection_t
{
        char description[256];
        int value;
} device_config_selection_t;

typedef struct device_config_t
{
        char name[256];
        char description[256];
        int type;
        char default_string[256];
        int default_int;
        device_config_selection_t selection[16];
} device_config_t;

typedef struct device_t
{
        char name[50];
        uint32_t flags;
        void *(*init)();
        void (*close)(void *p);
        int  (*available)();
        void (*speed_changed)(void *p);
        void (*force_redraw)(void *p);
        void (*add_status_info)(char *s, int max_len, void *p);
        device_config_t *config;
} device_t;


// function

void tkd8001_ramdac_out(uint16_t addr, uint8_t val, tkd8001_ramdac_t *ramdac, svga_t *svga)
{
//        pclog("OUT RAMDAC %04X %02X %04X:%04X\n",addr,val,CS,pc);
        switch (addr)
        {
                case 0x3C6:
                if (ramdac->state == 4)
                {
                        ramdac->state = 0;
                        ramdac->ctrl = val;
                        switch (val >> 5)
                        {
                                case 0: case 1: case 2: case 3:
                                svga->bpp = 8;
                                break;
                                case 5:
                                svga->bpp = 15;
                                break;
                                case 6:
                                svga->bpp = 24;
                                break;
                                case 7:
                                svga->bpp = 16;
                                break;
                        }
                        return;
                }
               // tkd8001_state = 0;
                break;
                case 0x3C7: case 0x3C8: case 0x3C9:
                ramdac->state = 0;
                break;
        }
        svga_out(addr, val, svga);
}

uint8_t tkd8001_ramdac_in(uint16_t addr, tkd8001_ramdac_t *ramdac, svga_t *svga)
{
//        pclog("IN RAMDAC %04X %04X:%04X\n",addr,CS,pc);
        switch (addr)
        {
                case 0x3C6:
                if (ramdac->state == 4)
                {
                        //tkd8001_state = 0;
                        return ramdac->ctrl;
                }
                ramdac->state++;
                break;
                case 0x3C7: case 0x3C8: case 0x3C9:
                ramdac->state = 0;
                break;
        }
        return svga_in(addr, svga);
}

#endif
