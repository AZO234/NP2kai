/* === ISO-2022-JP and UTF-16(UCS2) convert table generator === (C) AZO */

// build: gcc jisutf16tablegen.c jisutf16pair.c -o jisutf16tablegen
// usage: ./jisutf16tablegen > jisutf16table.txt

#include "jisutf16pair.h"
#include <stdio.h>
#include <stdint.h>

uint16_t jistoutf16(const uint8_t type, const uint16_t jis) {
  uint16_t loc;
  uint16_t res = 0;

  for(loc = 0; loc < jisutf16pair_count; loc++) {
    if(type == jisutf16pairs[loc].type && jis == jisutf16pairs[loc].jis) {
      res = jisutf16pairs[loc].utf16;
      break;
    }
  }

  return res;
}

int main(int iArgc, char* strArgv[]) {
  uint8_t t;
  uint16_t jisu, jisl, jis, utf16;
  uint32_t n;
  char strOutput[128];

  n = 0;
  for(jisl = 0x00; jisl <= 0x7F; jisl++) {
    if(n % 8 == 0) {
      printf("\n ");
    }
    utf16 = jistoutf16(0, jisl);
    printf(" 0x%04X,", utf16);
    n++;
  }
  printf("\n\n");

  n = 0;
  for(jisl = 0x00; jisl <= 0x7F; jisl++) {
    if(n % 8 == 0) {
      printf("\n ");
    }
    utf16 = jistoutf16(1, jisl);
    printf(" 0x%04X,", utf16);
    n++;
  }
  printf("\n\n");

  n = 0;
  for(jisu = 0x21; jisu < 0x7F; jisu++) {
    for(jisl = 0x21; jisl < 0x7F; jisl++) {
      if(n % 8 == 0) {
        printf("\n ");
      }
      jis = (jisu << 8) | jisl;
      utf16 = jistoutf16(2, jis);
      printf(" 0x%04X,", utf16);
      n++;
    }
  }
  printf("\n\n");

  return 0;
}

