/**
 * @file	npdisp_rle.c
 * @brief	Implementation of the Neko Project II Display Adapter RLE
 */

#include	"compiler.h"

#if defined(SUPPORT_WAB_NPDISP)

#include <map>
#include <vector>

#include "pccore.h"
#include "cpucore.h"

#include "npdispdef.h"
#include "npdisp.h"
#include "npdisp_mem.h"
#include "npdisp_rle.h"

static int bmp_abs_i32(SINT32 v, SINT32 *out)
{
    if (v == INT32_MIN) return 0;
    *out = (v < 0) ? -v : v;
    return 1;
}

static int calc_bmp_stride_bytes(SINT32 width, UINT16 bpp, UINT32 *out_stride)
{
    if (width <= 0) return 0;

    /* stride = ((width * bpp + 31) / 32) * 4 */
    UINT32 bits = (UINT32)(UINT32)width * (UINT32)bpp;
    UINT32 stride = ((bits + 31u) / 32u) * 4u;
    if (stride > SIZE_MAX) return 0;

    *out_stride = (UINT32)stride;
    return 1;
}

static void put_pixel_8bpp(UINT8 *dst, UINT32 stride, SINT32 width, SINT32 height_abs,
                           int x, int y_mem, UINT8 val)
{
    if (x < 0 || x >= width || y_mem < 0 || y_mem >= height_abs) return;
    dst[(UINT32)y_mem * stride + (UINT32)x] = val;
}

static void put_pixel_4bpp(UINT8 *dst, UINT32 stride, SINT32 width, SINT32 height_abs,
                           int x, int y_mem, UINT8 val)
{
    if (x < 0 || x >= width || y_mem < 0 || y_mem >= height_abs) return;

    UINT8 *p = dst + (UINT32)y_mem * stride + (UINT32)(x >> 1);
    val &= 0x0F;

    if ((x & 1) == 0) {
        *p = (UINT8)((*p & 0x0F) | (val << 4));
    } else {
        *p = (UINT8)((*p & 0xF0) | val);
    }
}

static int map_rle_y_to_memory_row(int y_rle, SINT32 height_abs, int top_down)
{
    if (top_down) {
        return y_rle;
    } else {
        return (int)(height_abs - 1 - y_rle);
    }
}

int npdisp_DecompressRLEBMP(const BITMAPINFOHEADER* bih, const UINT8* rle_data, int rle_size, UINT8* out_pixels, UINT8* out_validpixels)
{
    SINT32 width, height_abs;
    int top_down;
    UINT32 stride, image_size;
    UINT8 *dst;
    UINT8* dstValid = NULL;
    UINT32 pos = 0;
    int x = 0;
    int y_rle = 0;

    if (!bih || !rle_data || !out_pixels) return 0;

    if (!bmp_abs_i32(bih->biHeight, &height_abs)) return 0;
    width = bih->biWidth;
    top_down = (bih->biHeight < 0);

    if (width <= 0 || height_abs <= 0) return 0;
    if (bih->biPlanes != 1) return 0;

    if (!((bih->biCompression == BI_RLE8 && bih->biBitCount == 8) ||
          (bih->biCompression == BI_RLE4 && bih->biBitCount == 4))) {
        return 0;
    }

    if (!calc_bmp_stride_bytes(width, bih->biBitCount, &stride)) return 0;
    if ((UINT32)stride * (UINT32)height_abs > SIZE_MAX) return 0;
    image_size = stride * (UINT32)height_abs;

    dst = out_pixels;
    memset(dst, 0, image_size);
    if (out_validpixels) {
        dstValid = out_validpixels;
        memset(dstValid, 0xff, image_size);
    }

    if (bih->biCompression == BI_RLE8) {
        while (pos + 2 <= rle_size) {
            UINT8 count = rle_data[pos++];
            UINT8 value = rle_data[pos++];

            if (count != 0) {
                /* Encoded mode */
                for (UINT8 i = 0; i < count; ++i) {
                    if (y_rle >= 0 && y_rle < height_abs) {
                        int y_mem = map_rle_y_to_memory_row(y_rle, height_abs, top_down);
                        put_pixel_8bpp(dst, stride, width, height_abs, x, y_mem, value);
                        if (dstValid) {
                            put_pixel_8bpp(dstValid, stride, width, height_abs, x, y_mem, 0);
                        }
                    }
                    x++;
                }
            } else {
                /* Escape */
                if (value == 0) {
                    /* EOL */
                    x = 0;
                    y_rle++;
                    if (y_rle > height_abs) break;
                } else if (value == 1) {
                    /* EOB */
                    return 1;
                } else if (value == 2) {
                    /* Delta */
                    if (pos + 2 > rle_size) break;
                    {
                        UINT8 dx = rle_data[pos++];
                        UINT8 dy = rle_data[pos++];
                        x += dx;
                        y_rle += dy;
                    }
                } else {
                    /* Absolute mode */
                    UINT8 n = value;
                    if (pos + n > rle_size) break;

                    for (UINT8 i = 0; i < n; ++i) {
                        UINT8 px = rle_data[pos++];
                        if (y_rle >= 0 && y_rle < height_abs) {
                            int y_mem = map_rle_y_to_memory_row(y_rle, height_abs, top_down);
                            put_pixel_8bpp(dst, stride, width, height_abs, x, y_mem, px);
                            if (dstValid) {
                                put_pixel_8bpp(dstValid, stride, width, height_abs, x, y_mem, 0);
                            }
                        }
                        x++;
                    }

                    if (n & 1) {
                        if (pos + 1 > rle_size) break;
                        pos++;
                    }
                }
            }
        }
    } else { /* BI_RLE4 */
        while (pos + 2 <= rle_size) {
            UINT8 count = rle_data[pos++];
            UINT8 value = rle_data[pos++];

            if (count != 0) {
                /* Encoded mode */
                UINT8 hi = (UINT8)((value >> 4) & 0x0F);
                UINT8 lo = (UINT8)(value & 0x0F);

                for (UINT8 i = 0; i < count; ++i) {
                    UINT8 px = ((i & 1) == 0) ? hi : lo;
                    if (y_rle >= 0 && y_rle < height_abs) {
                        int y_mem = map_rle_y_to_memory_row(y_rle, height_abs, top_down);
                        put_pixel_4bpp(dst, stride, width, height_abs, x, y_mem, px);
                        if (dstValid) {
                            put_pixel_4bpp(dstValid, stride, width, height_abs, x, y_mem, 0);
                        }
                    }
                    x++;
                }
            } else {
                /* Escape */
                if (value == 0) {
                    /* EOL */
                    x = 0;
                    y_rle++;
                    if (y_rle > height_abs) break;
                } else if (value == 1) {
                    /* EOB */
                    return 1;
                } else if (value == 2) {
                    /* Delta */
                    if (pos + 2 > rle_size) break;
                    {
                        UINT8 dx = rle_data[pos++];
                        UINT8 dy = rle_data[pos++];
                        x += dx;
                        y_rle += dy;
                    }
                } else {
                    /* Absolute mode */
                    UINT8 n = value;
                    UINT32 byte_count = (UINT32)(n + 1) / 2;

                    if (pos + byte_count > rle_size) break;

                    for (UINT8 i = 0; i < n; ++i) {
                        UINT8 b = rle_data[pos + (UINT32)(i >> 1)];
                        UINT8 px = ((i & 1) == 0)
                                   ? (UINT8)((b >> 4) & 0x0F)
                                   : (UINT8)(b & 0x0F);

                        if (y_rle >= 0 && y_rle < height_abs) {
                            int y_mem = map_rle_y_to_memory_row(y_rle, height_abs, top_down);
                            put_pixel_4bpp(dst, stride, width, height_abs, x, y_mem, px);
                            if (dstValid) {
                                put_pixel_4bpp(dstValid, stride, width, height_abs, x, y_mem, 0);
                            }
                        }
                        x++;
                    }

                    pos += byte_count;

                    if (byte_count & 1) {
                        if (pos + 1 > rle_size) break;
                        pos++;
                    }
                }
            }
        }
    }
    return 0;
}

int npdisp_RLEBMPReadAndCalcSize(const UINT32 lpDIBitsAddr, const UINT32 biCompression, UINT8* rle_data_buf, int rle_size_max)
{
    UINT8 b1, b2;
    long pos = 0;
    UINT16 selector = lpDIBitsAddr >> 16;
    UINT32 offset = lpDIBitsAddr & 0xffff;

    while (pos + 2 <= rle_size_max) {
        b1 = rle_data_buf[pos] = npdisp_readMemory8With32Offset(selector, offset + pos);
        pos++;
        b2 = rle_data_buf[pos] = npdisp_readMemory8With32Offset(selector, offset + pos);
        pos++;
        if (npdisp.longjmpnum) return 0;

        if (b1 != 0) {
            /* Encoded mod */
            continue;
        }

        /*
         * Escape mode:
         *   00 00 = End of line
         *   00 01 = End of bitmap
         *   00 02 = Delta + 2 bytes
         *   00 nn = Absolute mode (nn >= 3)
         */
        if (b2 == 0) {
            /* End of line */
            continue;
        }
        else if (b2 == 1) {
            /* End of bitmap */
            return pos;
        }
        else if (b2 == 2) {
            /* Delta */
            rle_data_buf[pos] = npdisp_readMemory8With32Offset(selector, offset + pos);
            pos++;
            rle_data_buf[pos] = npdisp_readMemory8With32Offset(selector, offset + pos);
            pos++;
            if (npdisp.longjmpnum) return 0;
            continue;
        }
        else {
            /*
             * Absolute mode
             */
            UINT8 count = b2;
            UINT32 data_bytes;
            UINT32 i;

            if (biCompression == BI_RLE8) {
                data_bytes = count;
            }
            else if (biCompression == BI_RLE4) {
                data_bytes = (count + 1) / 2;
            }
            else {
                return 0;
            }

            if (!npdisp_readMemoryWith32Offset(rle_data_buf + pos, selector, offset + pos, data_bytes)) {
                return 0;
            }
            pos += data_bytes;

            /* Absolute modeÇÕÉèÅ[Éhã´äE */
            if (data_bytes & 1) {
                rle_data_buf[pos] = npdisp_readMemory8With32Offset(selector, offset + pos);
                pos++;
            }
        }
    }
    return 0;
}

#endif