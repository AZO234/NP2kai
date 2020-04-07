/**
 * @file	font.h
 * @brief	CGROM and font loader
 *
 * @author	$Author: yui $
 * @date	$Date: 2011/02/23 10:11:44 $
 */

#define	FONTMEMORYBIND				// 520KBくらいメモリ削除(ぉぃ

#define FONTMEMORYSIZE 0x84000

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FONTMEMORYBIND
#define	fontrom		(mem + FONT_ADRS)
#else
extern	UINT8	__font[FONTMEMORYSIZE];
#define	fontrom		(__font)
#endif

void font_initialize(void);
void font_setchargraph(BOOL epson);
UINT8 font_load(const OEMCHAR *filename, BOOL force);

extern UINT hf_enable;
void hook_fontrom_flush(void);
void hook_fontrom(UINT32 address);

#ifdef __cplusplus
}
#endif

