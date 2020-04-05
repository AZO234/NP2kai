/* === ISO-2022-JP and UTF-16(UCS2) code pair === (C) AZO */

#ifndef _JISUTF16PAIR_H_
#define _JISUTF16PAIR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
  uint16_t jis;
  uint16_t utf16u;
  uint16_t utf16l;
} jisutf16pair_t;

extern jisutf16pair_t jisutf16pairs[];

#define jisutf16pair_count (sizeof(jisutf16pairs) / sizeof(jisutf16pair_t))

#ifdef __cplusplus
}
#endif

#endif  // _JISUTF16PAIR_H_

