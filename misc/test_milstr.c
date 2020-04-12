/* === milstr test === (c) 2020 AZO */
// Windows(MSVC):
//   cl test_milstr.c -DMILSTR_TEST ../common/milstr.c -I../ -I../common -utf-8 -Wall
// Windows(MSYS/MinGW):
//   gcc test_milstr.c -DMILSTR_TEST ../common/milstr.c -I../ -I../common -Wall -Wextra -o test_milstr
// Linux:
//   gcc test_milstr.c -DMILSTR_TEST ../common/milstr.c -I../ -I../common -Wall -Wextra -o test_milstr

/*
・UTF-8で統一したい。（でも未来ではまた変わってるんだろうな）
　ソース、スクリプト、Webなどのテキスト世界ではUTF-8が定着しつつある。
・milstrで全てを吸収・解決したい。
・ワイド文字(wchar_t)の存在も忘れちゃいけない。
・文字列取り扱いのセキュリティ対応

このプログラムで動作をチェックしつつ、compiler.hに設定していけばいいかなと。
※このプログラムは、MILSTR_TEST で compiler.h を無効にしている。

コンピュータの成り立ちから連綿と続く未解決な「文字」紛争、バベルの塔の再建。
（欧米が先行し、他地域の文化・文字を理解するのに時間がかかる）

バイト文字。文字はUTF-8(8-32bit)。既存char・main()がそのまま使える。
ワイド文字。文字はWindowsで16bit(UTF-16)、他は32bit(UTF-32)。既存char・main()は使えない。
milstrは前者を軸にして、後者をサポートする。

バイト文字→難易度HARD・自由度○・移植性○・セキュリティ△
ワイド文字→難易度EASY・自由度△・移植性✕・セキュリティ○
アセンブリはVERY HARDですかそうですかｗ

char(文字)型=1Byteという掟は変わらないが、int8_tで代用出来るはず。
※BYTEやuint8_tでの代用が望ましいが、
  Unicodeの「次Byteに続く」フラグがbit7なので、正負で分けちゃう人もきっと多い。
  if((uint8_t)c & 0x80) より if(c < 0) の方がCPU処理は速い、はず。

VC++のCString、std::stringクラスはTCHAR.Hのラッパーで、
_UNICODE・UNICODEなし:バイト文字
_UNICODE・UNICODEあり:ワイド文字

文字列で注意しなければならないのが、バッファオーバーラン。
多少重くなっても、バッファサイズ（0以上）は必ず考慮する。
処理内でポインタ戻りをする時も厳重注意。バッファアンダーラン。
OK: strnlen strncpy snprintf
ダメ: strlen strcpy sprintf

OEM？mil？何なんでしょう。
NPCHAR、NPTEXTにしたいニャン。（NP3が出るかもしれないからね）

C/C++20のワイド文字でchar8_t・char16_t・char32_tが登場予定。

・廃止予定
OEMCHAR : charのみ
OEMTEXT : 標準文字列のみ

milank、mileucはお役御免、かな。milsjisもそのうち・・・。
ANK=Alphabet Numeric Katakana=1Byte。
*/

#include "milstr.h"

#define COUNT_BUFFER 256

char strString1[COUNT_BUFFER];
char strString2[COUNT_BUFFER];

int main(int iArgc, char* strArgv[]) {
  // test000 : OEMSTRCPY, OEMPRINTFSTR
  OEMPRINTFSTR("* test000 : OEMSTRCPY(OEMSTRNCPY), OEMPRINTFSTR(OEMPRINTF)" OEMNEWLINE);
  OEMSTRCPY(strString1, "000output" OEMNEWLINE);
  OEMPRINTFSTR(strString1);
  OEMSTRCPY(strString1, "000ｱｳﾄﾌﾟｯﾄ" OEMNEWLINE);
  OEMPRINTFSTR(strString1);
  OEMSTRCPY(strString1, "000アウトプット" OEMNEWLINE);
  OEMPRINTFSTR(strString1);
  OEMSTRCPY(strString1, "000出力" OEMNEWLINE);
  OEMPRINTFSTR(strString1);

  return 0;
}

