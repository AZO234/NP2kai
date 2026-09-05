/**
 * @file	soundrom.h
 * @brief	Sound BIOSインターフェース
 */

#pragma once

typedef struct {
	OEMCHAR	name[24];
	UINT32	address;
} SOUNDROM;


#ifdef __cplusplus
extern "C" {
#endif

extern	SOUNDROM	soundrom;

void soundrom_reset(void);
void soundrom_load(UINT32 address, const OEMCHAR *primary);
void soundrom_loadex(UINT sw, const OEMCHAR *primary);
void soundrom_loadsne(const OEMCHAR *primary);

/* エミュレーションSound BIOSスタブのフック命令を書き換え */
void soundrom_patchhookinst(void);

/* CPUコア用フック補助関数 */
UINT soundrom_isbiosaddr(UINT32 adrs);
UINT soundrom_biosfunc(UINT32 adrs);

#ifdef __cplusplus
}
#endif
