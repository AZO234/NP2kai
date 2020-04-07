/* === ISO-2022-JP and UTF-16(UCS2) code pair === (C) AZO */

#ifndef _JISUTF16PAIR_H_
#define _JISUTF16PAIR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct jisutf16pair_t_ {
  uint8_t type;
  uint16_t jis;
  uint16_t utf16;
} jisutf16pair_t;

extern jisutf16pair_t jisutf16pairs[];
extern uint16_t jisutf16pair_count;

#ifdef __cplusplus
}
#endif

#endif  // _JISUTF16PAIR_H_

