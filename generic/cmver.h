
// vermouthのみ使用のCOMMNG-MIDI
// あまりに一緒すぎるんで 関数名変えてこっちへ

#include "commng.h"

// ---- com manager midi for vermouth

#define	COMSIG_MIDI		0x4944494d

#ifdef __cplusplus
extern "C" {
#endif

void cmvermouth_initialize(void);
COMMNG cmvermouth_create(void);
#if defined(VERMOUTH_LIB)
void cmvermouth_load(UINT rate);
void cmvermouth_unload(void);
#endif

#ifdef __cplusplus
}
#endif

