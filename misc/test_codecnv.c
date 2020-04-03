/* === codecnv test === (c) 2020 AZO */
// Windows(MSVC):
//   cl test_codecnv.c -DCODECNV_TEST ../codecnv/sjisucs2.c ../codecnv/ucs2sjis.c ../codecnv/ucs2ucs4.c ../codecnv/ucs2utf8.c ../codecnv/ucs4ucs2.c ../codecnv/ucs4utf8.c ../codecnv/utf8ucs2.c ../codecnv/utf8ucs4.c -I../ -I../common -utf-8 -Wall
// Windows(MSYS/MinGW):
//   gcc test_codecnv.c -DCODECNV_TEST ../codecnv/sjisucs2.c ../codecnv/ucs2sjis.c ../codecnv/ucs2ucs4.c ../codecnv/ucs2utf8.c ../codecnv/ucs4ucs2.c ../codecnv/ucs4utf8.c ../codecnv/utf8ucs2.c ../codecnv/utf8ucs4.c -I../ -I../codecnv -Wall -Wextra -o test_codecnv
// Linux:
//   gcc test_codecnv.c -DCODECNV_TEST ../codecnv/sjisucs2.c ../codecnv/ucs2sjis.c ../codecnv/ucs2ucs4.c ../codecnv/ucs2utf8.c ../codecnv/ucs4ucs2.c ../codecnv/ucs4utf8.c ../codecnv/utf8ucs2.c ../codecnv/utf8ucs4.c -I../ -I../codecnv -Wall -Wextra -o test_codecnv

#include "compiler_base.h"
#include "codecnv.h"

#define COUNT_BUFFER 256

char strU8String[COUNT_BUFFER];
char strSJString[COUNT_BUFFER];
UINT16 su16String[COUNT_BUFFER];
UINT32 su32String[COUNT_BUFFER];

int main(int iArgc, char* strArgv[]) {
  int i;
  int iLen;
  UINT uiCount;

  // test000
  printf("=== test000\n");
  strcpy(strU8String, "0123ABCｺﾝﾆﾁﾊこんにちは今日は");
  iLen = strlen(strU8String);
  printf("UTF-8 length: %d\n", iLen);
  printf("UTF-8 string: %s\n", strU8String);
  printf("UTF-8 code: ");
  for(i = 0; i < iLen; i++) {
    printf(" %02X", ((UINT8*)strU8String)[i]);
  }
  printf("\n");

  // test001 : UTF-8 to UTF-32
  printf("=== test001 : UTF-8 to UTF-32\n");
  uiCount = codecnv_utf8toucs4(su32String, COUNT_BUFFER, strU8String, -1);
  iLen = codecnv_ucs4len(su32String);
  printf("UTF-32 count: %d\n", uiCount);
  printf("UTF-32 length: %d\n", iLen);
  printf("UTF-32 code: ");
  for(i = 0; i < iLen; i++) {
    printf(" %08X", su32String[i]);
  }
  printf("\n");

  // test002 : UTF-32 to UTF-16
  printf("=== test002 : UTF-32 to UTF-16\n");
  uiCount = codecnv_ucs4toucs2(su16String, COUNT_BUFFER, su32String, -1);
  iLen = codecnv_ucs2len(su16String);
  printf("UTF-16 count: %d\n", uiCount);
  printf("UTF-16 length: %d\n", iLen);
  printf("UTF-16 code: ");
  for(i = 0; i < iLen; i++) {
    printf(" %04X", su16String[i]);
  }
  printf("\n");

  // test003 : UTF-32 to UTF-8
  printf("=== test003 : UTF-32 to UTF-8\n");
  uiCount = codecnv_ucs4toutf8(strU8String, COUNT_BUFFER, su32String, -1);
  iLen = strlen(strU8String);
  printf("UTF-8 count: %d\n", uiCount);
  printf("UTF-8 length: %d\n", iLen);
  printf("UTF-8 string: %s\n", strU8String);
  printf("UTF-8 code: ");
  for(i = 0; i < iLen; i++) {
    printf(" %02X", ((UINT8*)strU8String)[i]);
  }
  printf("\n");

  // test004 : UTF-8 to UTF-16
  printf("=== test004 : UTF-8 to UTF-16\n");
  uiCount = codecnv_utf8toucs2(su16String, COUNT_BUFFER, strU8String, -1);
  iLen = codecnv_ucs2len(su16String);
  printf("UTF-16 count: %d\n", uiCount);
  printf("UTF-16 length: %d\n", iLen);
  printf("UTF-16 code: ");
  for(i = 0; i < iLen; i++) {
    printf(" %04X", su16String[i]);
  }
  printf("\n");

  // test005 : UTF-16 to UTF-8
  printf("=== test005 : UTF-16 to UTF-8\n");
  uiCount = codecnv_ucs2toutf8(strU8String, COUNT_BUFFER, su16String, -1);
  iLen = strlen(strU8String);
  printf("UTF-8 count: %d\n", uiCount);
  printf("UTF-8 length: %d\n", iLen);
  printf("UTF-8 string: %s\n", strU8String);
  printf("UTF-8 code: ");
  for(i = 0; i < iLen; i++) {
    printf(" %02X", ((UINT8*)strU8String)[i]);
  }
  printf("\n");



  return 0;
}

