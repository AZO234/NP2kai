/**
 * @file	font.c
 * @brief	CGROM and font loader
 *
 * @author	$Author: yui $
 * @date	$Date: 2011/02/23 10:11:44 $
 */

#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"pccore.h"
#include	"cpucore.h"
#include	"font.h"
#include	"fontdata.h"
#include	"fontmake.h"
#include	"codecnv/codecnv.h"
#include	"io/cgrom.h"

#ifndef FONTMEMORYBIND
	UINT8	__font[FONTMEMORYSIZE];
#endif

static const OEMCHAR fonttmpname[] = OEMTEXT("font.tmp");

/**
 * Initializes CGROM
 */
void font_initialize(void) {

	ZeroMemory(fontrom, FONTMEMORYSIZE);
	font_setchargraph(FALSE);
}

/**
 * Builds charactor graphics
 * @param[in] epson If this parameter is FALSE, patched NEC charactor
 */
void font_setchargraph(BOOL epson) {

	UINT8	*p;
	UINT8	*q;
	UINT	i;
	UINT	j;
	UINT32	dbit;

	p = fontrom + 0x81000;
	q = fontrom + 0x82000;
	for (i=0; i<256; i++) {
		q += 8;
		for (j=0; j<4; j++) {
			dbit = 0;
			if (i & (0x01 << j)) {
				dbit |= 0xf0f0f0f0;
			}
			if (i & (0x10 << j)) {
				dbit |= 0x0f0f0f0f;
			}
			*(UINT32 *)p = dbit;
			p += 4;
			*(UINT16 *)q = (UINT16)dbit;
			q += 2;
		}
	}

	if (!epson) {
		*(UINT16 *)(fontrom + 0x81000 + (0xf2 * 16)) = 0;
		fontrom[0x82000 + (0xf2 * 8)] = 0;
	}
}

/**
 * Retrieves the font type of the specified file.
 * @param[in] fname The name of the font file
 * @return font type
 */
static UINT8 fonttypecheck(const OEMCHAR *fname) {

const OEMCHAR	*p;

	p = file_getext(fname);
	if (!file_cmpname(p, str_bmp)) {
		return(FONTTYPE_PC98);
	}
	if (!file_cmpname(p, str_bmp_b)) {
		return(FONTTYPE_PC98);
	}
	p = file_getname(fname);
	if (!file_cmpname(p, v98fontname)) {
		return(FONTTYPE_V98);
	}
	if (!file_cmpname(p, v98fontname_s)) {
		return(FONTTYPE_V98);
	}
	if ((!file_cmpname(p, pc88ankname)) ||
		(!file_cmpname(p, pc88knj1name)) ||
		(!file_cmpname(p, pc88knj2name))) {
		return(FONTTYPE_PC88);
	}
	if ((!file_cmpname(p, fm7ankname)) ||
		(!file_cmpname(p, fm7knjname))) {
		return(FONTTYPE_FM7);
	}
	if ((!file_cmpname(p, x1ank1name)) ||
		(!file_cmpname(p, x1ank2name)) ||
		(!file_cmpname(p, x1knjname))) {
		return(FONTTYPE_X1);
	}
	if (!file_cmpname(p, x68kfontname)) {
		return(FONTTYPE_X68);
	}
	if (!file_cmpname(p, x68kfontname_s)) {
		return(FONTTYPE_X68);
	}
	return(FONTTYPE_NONE);
}

/**
 * Loads font files
 * @param[in] filename The name of the font file
 * @param[in] force If this parameter is TRUE, load file always
 *                  If this parameter is FALSE, load when font is not ready
 * @return font type
 */
UINT8 font_load(const OEMCHAR *filename, BOOL force) {

	UINT	i;
const UINT8	*p;
	UINT8	*q;
	UINT	j;
	OEMCHAR	fname[MAX_PATH];
	UINT8	type;
	UINT8	loading;

	if (filename) {
		file_cpyname(fname, filename, NELEMENTS(fname));
	}
	else {
		fname[0] = '\0';
	}
	type = fonttypecheck(fname);
	if ((!type) && (!force)) {
		return(0);
	}

	// 外字: font[??560-??57f], font[??d60-??d7f] は削らないように…
	for (i=0; i<0x80; i++) {
		q = fontrom + (i << 12);
		ZeroMemory(q + 0x000, 0x0560 - 0x000);
		ZeroMemory(q + 0x580, 0x0d60 - 0x580);
		ZeroMemory(q + 0xd80, 0x1000 - 0xd80);
	}

	fontdata_ank8store(fontdata_8, 0, 256);
	p = fontdata_8;
	q = fontrom + 0x80000;
	for (i=0; i<256; i++) {
		for (j=0; j<8; j++) {
			q[0] = p[0];
			q[1] = p[0];
			p += 1;
			q += 2;
		}
	}

	loading = 0xff;
	switch(type) {
		case FONTTYPE_PC98:
			loading = fontpc98_read(fname, loading);
			break;

		case FONTTYPE_V98:
			loading = fontv98_read(fname, loading);
			break;

		case FONTTYPE_PC88:
			loading = fontpc88_read(fname, loading);
			break;

		case FONTTYPE_FM7:
			loading = fontfm7_read(fname, loading);
			break;

		case FONTTYPE_X1:
			loading = fontx1_read(fname, loading);
			break;

		case FONTTYPE_X68:
			loading = fontx68k_read(fname, loading);
			break;
	}
	loading = fontpc98_read(file_getcd(pc98fontname), loading);
	loading = fontpc98_read(file_getcd(pc98fontname_s), loading);
	loading = fontv98_read(file_getcd(v98fontname), loading);
	loading = fontv98_read(file_getcd(v98fontname_s), loading);
	loading = fontpc88_read(file_getcd(pc88ankname), loading);
	if (loading & FONTLOAD_16) {
		file_cpyname(fname, file_getcd(fonttmpname), NELEMENTS(fname));
		if (file_attr(fname) == -1) {
			makepc98bmp(fname);
		}
		loading = fontpc98_read(fname, loading);
	}
	return(type);
}

#define HF_BUFFERSIZE 0x10000

extern _CGROM cgrom;

UINT hf_enable;
UINT hf_codeul, hf_count;
static FILEH hf_file;
static UINT hf_type, hf_prespc;
static char hf_buffer[HF_BUFFERSIZE];
static UINT16 hf_u16buffer[HF_BUFFERSIZE];
static char* hf_bufloc = hf_buffer;
static hook_fontrom_output_t hf_fucOutput = NULL;

static void hook_fontrom_defoutput(const char* strString) {
  if(hf_file) {
    file_write(hf_file, strString, strlen(strString));
    file_write(hf_file, "\n", 1);
  }
}

void hook_fontrom_setoutput(hook_fontrom_output_t fncOutput) {
  hf_fucOutput = fncOutput;
}

void hook_fontrom_defenable(void) {
  if(!hf_file) {
    hook_fontrom_setoutput(hook_fontrom_defoutput);
    hf_file = file_create_c(HF_FILENAME);
    if(!hf_file) {
      hf_enable = 0;
    }
  }
}

void hook_fontrom_defdisable(void) {
  if(hf_file) {
    hook_fontrom_flush();
    file_close(hf_file);
    hf_file = NULL;
  }
}

void hook_fontrom_flush(void) {
  UINT ntype;

  if(hf_enable != 1) {
    return;
  }

  if(hf_bufloc != hf_buffer) {
    *hf_bufloc = '\0';
    hf_bufloc++;
    codecnv_jistoucs2(&ntype, hf_u16buffer, HF_BUFFERSIZE, hf_buffer, -1, hf_type);
    codecnv_ucs2toutf8(hf_buffer, HF_BUFFERSIZE, hf_u16buffer, -1);
    if(hf_fucOutput) {
      hf_fucOutput(hf_buffer);
    }
    hf_bufloc = hf_buffer;
  }
}

void hook_fontrom(UINT32 u32Address) {
  UINT c, spc = 0, output = 0;
  UINT16 jis;
  UINT32 address_org;

  if(hf_enable != 1) {
    return;
  }

  u32Address >>= 4;

  if(u32Address >= 0x8000) {
    c = u32Address & 0xFF;
//    if(c <= 0x1B || (c >= 0xF8 && c <= 0xFB) || c >= 0xFD) {
    if(c <= 0x1F || c >= 0x7F) {
      hf_prespc = 1;
      output = 1;
    } else if(c == 0x20 && hf_prespc) {
      output = 1;
    } else {
      if(c == 0x20) {
        hf_prespc = 1;
      } else {
        hf_prespc = 0;
      }
      if(c >= 0x20 && c <= 0x7E) {
        if(hf_type != 0) {
          *hf_bufloc = 0x1B;
          hf_bufloc++;
          *hf_bufloc = 0x28;
          hf_bufloc++;
          *hf_bufloc = 0x42;
          hf_bufloc++;
          hf_type = 0;
        }
      }
      *hf_bufloc = c;
      hf_bufloc++;
      if(hf_bufloc - hf_buffer >= HF_BUFFERSIZE - 0x1000) {
        output = 1;
      }
    }
  } else {
    if(u32Address == 0x7FFF) {
      hf_prespc = 0;
      c = (cgrom.code >> 8) & 0x7F;
      if(c >= 0x21 && c <= 0x5F) {
        if(hf_type != 1) {
          *hf_bufloc = 0x1B;
          hf_bufloc++;
          *hf_bufloc = 0x28;
          hf_bufloc++;
          *hf_bufloc = 0x49;
          hf_bufloc++;
          hf_type = 1;
        }
        *hf_bufloc = c;
        hf_bufloc++;
        if(hf_bufloc - hf_buffer >= HF_BUFFERSIZE - 0x1000) {
          output = 1;
        }
      } else {
        return;
      }
    } else {
      if(u32Address & 0x80) {
        return;
      }
      u32Address &= 0x7F7F;
      jis = (((u32Address & 0x7F) + 0x20) << 8) | ((u32Address >> 8) & 0x7F);
      if(jis >= 0x2121 && jis <= 0x7C7E && (jis & 0x7F) >= 0x21 && (jis & 0x7F) <= 0x7E) {
        if(hf_prespc && jis == 0x2121) {
           output = 1;
        } else {
          if(jis == 0x2121) {
            hf_prespc = 1;
          } else {
            hf_prespc = 0;
          }
          if(hf_type != 2) {
            *hf_bufloc = 0x1B;
            hf_bufloc++;
            *hf_bufloc = 0x24;
            hf_bufloc++;
            *hf_bufloc = 0x42;
            hf_bufloc++;
            hf_type = 2;
          }
          *hf_bufloc = (jis >> 8) & 0x7F;
          hf_bufloc++;
          *hf_bufloc =  jis       & 0x7F;
          hf_bufloc++;
          if(hf_bufloc - hf_buffer >= HF_BUFFERSIZE - 0x1000) {
            output = 1;
          }
        }
      } else {
        hf_prespc = 1;
        output = 1;
      }
    }
  }
  if(output) {
    hook_fontrom_flush();
  }
}

