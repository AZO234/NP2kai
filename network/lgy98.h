/*
 * QEMU NE2000 emulation
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

#pragma once

#if defined(SUPPORT_LGY98)

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct {
	UINT8 enabled;
	UINT  baseaddr;
	UINT8 irq;
} LGY98CFG;

extern LGY98CFG lgy98cfg;

//void IOOUTCALL ideio_w16(UINT port, REG16 value);
//REG16 IOINPCALL ideio_r16(UINT port);
	
void  IOOUTCALL lgy98_ob200_8(UINT addr, REG8 value);
REG8  IOOUTCALL lgy98_ib200_8(UINT addr);
void  IOOUTCALL lgy98_ob200_16(UINT addr, REG16 value);
REG16 IOOUTCALL lgy98_ib200_16(UINT addr);

void lgy98_reset(const NP2CFG *pConfig);
void lgy98_bind(void);
void lgy98_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SUPPORT_LGY98 */
