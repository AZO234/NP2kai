/* === Shift_JIS and UTF-16(UCS2) code pair === (C) AZO */

#ifndef _SJISUTF16PAIR_H_
#define _SJISUTF16PAIR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
  uint16_t sjis;
  uint16_t utf16u;
  uint16_t utf16l;
} sjisutf16pair_t;

extern sjisutf16pair_t sjisutf16pairs[];

#define sjisutf16pair_count (sizeof(sjisutf16pairs) / sizeof(sjisutf16pair_t))

#ifdef __cplusplus
}
#endif

#endif  // _SJISUTF16PAIR_H_

