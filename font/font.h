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

#define HF_FILENAME "hook_fontrom.txt"
typedef void(*hook_fontrom_output_t)(const char* strOutput);
extern UINT hf_enable;
extern UINT hf_codeul, hf_count;
void hook_fontrom_defenable(void);
void hook_fontrom_defdisable(void);
void hook_fontrom_setoutput(hook_fontrom_output_t fncOutput);
void hook_fontrom_flush(void);
void hook_fontrom(UINT32 u32Address);

#ifdef __cplusplus
}
#endif

