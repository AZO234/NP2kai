プリプロセッサ定義による確実な使用／不使用切り替えを実現するため以下の#ifdefを追加。
#if defined(SUPPORT_FMGEN)

SUPPORT_FMGENを定義しない場合、fmgenのソースはコンパイルされません。