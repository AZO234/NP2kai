
// ---- 定義

  最適化の為のメモリ使用量の抑制
    MEMOPTIMIZE = 0～2

    CPUにより以下の数値をセットされることを期待している
      MEMOPTIMIZE未定義 … Celeron333A以降のセカンドキャッシュ有効機
      MEMOPTIMIZE = 0   … x86
      MEMOPTIMIZE = 1   … PowerPC等のデスクトップ用RISC
      MEMOPTIMIZE = 2   … StrongARM等の組み込み用RISC


  コンパイラの引き数・戻り値の最適化
    引き数・戻り値でint型以外を指定した場合に、最適化が有効に働かない
    コンパイラ向けの定義です。
    通常は common.h の物を使用します。
      REG8 … UINT8型 / (sizeof(REG8) != 1)の場合 上位ビットを0fillする事
      REG16 … UINT16型 / (sizeof(REG16) != 2)の場合 上位ビットを0fillする事
　　　いずれも値をセットする側が0fillし、参照側は0fillしたものと見なします。


  OSの言語の選択
    OSLANG_SJIS … Shift-JISの漢字コードを解釈する
    OSLANG_EUC  … EUCの漢字コードを解釈する

    OSLINEBREAK_CR   … MacOS   "\r"
    OSLINEBREAK_LF   … Unix    "\n"
    OSLINEBREAK_CRLF … Windows "\r\n"

      ※現在は以下のソースコード内で個別に設定しています。
        (Windowsが APIによって \r\nの場合と\nの場合があるので…)
        ・common/_memory.c
        ・debugsub.c
        ・statsave.c

    (milstr.h選択用)
    SUPPORT_ANK      … ANK文字列操作関数をリンクする
    SUPPORT_SJIS     … SJIS文字列操作関数をリンクする
    SUPPORT_EUC      … EUC文字列操作関数をリンクする

      ※現在milstr.hですべて定義されたままになっています。
        ver0.73でmilstr.hの定義を外し compiler.hで指定した物となります。


　CPUCORE_IA32
　　IA32アーキテクチャを採用
　　　i386cを使用する場合の注意点
　　  ・CPU panic や警告表示時に msgbox() という API を使用します。
　　　　compiler.h あたりで適当に定義してください。
　　　・sigsetjmp(3), siglongjmp(3) が無いアーキテクチャは以下の define を
　　　　compiler.h あたりに追加してください。
　　　　----------------------------------------------------------------------
        #define sigjmp_buf              jmp_buf
        #define sigsetjmp(env, mask)    setjmp(env)
        #define siglongjmp(env, val)    longjmp(env, val)
　　　　----------------------------------------------------------------------

  CPUSTRUC_MEMWAIT
　　　cpucore構造体にメモリウェイト値を移動する(vramop)

　SUPPORT_CRT15KHZ
　　　水平走査15.98kHzをサポートする(DIPSW1-1)

　SUPPORT_CRT31KHZ
　　　水平走査31.47kHzをサポートする
　　　Fellowタイプはこれ

　SUPPORT_PC9821
　　　PC-9821拡張のサポート
　　　当然ですが 386必須です。
　　　また SUPPORT_CRT31KHZも必要です(ハイレゾBIOSを使用する為)

　SUPPORT_PC9861K
　　　PC-9861K(RS-232C拡張I/F)をサポート

　SUPPORT_IDEIO
　　　IDEの I/Oレベルでのサポート
　　　でも ATAのリード程度しかできない…

　SUPPORT_SASI
　　　SASI HDDをサポート
　　　定義がなければ常時IDEとして作動します。

　SUPPORT_SCSI
　　　SCSI HDDをサポート…全然動かない

　SUPPORT_S98
　　　S98ログを取得

　SUPPORT_WAVEREC
　　Soundレベルで waveファイルの書き出し関数をサポート
　　但し書き出し中は サウンド出力が止まるので　ほぼデバグ用


// ---- screen

  PC-9801シリーズの画面サイズは標準で 641x400。
  VGAでは収まらないので 強制的にVGAに収める為に 画面横サイズは width + extend
とする。
  8 < width < 640
  8 < height < 480
  extend = 0 or 1

typedef struct {
	BYTE	*ptr;		// VRAMポインタ
	int		xalign;		// x方向オフセット
	int		yalign;		// y方向オフセット
	int		width;		// 横幅
	int		height;		// 縦幅
	UINT	bpp;		// スクリーン色ビット
	int		extend;		// 幅拡張
} SCRNSURF;

  サーフェスサイズは (width + extern) x height。


const SCRNSURF *scrnmng_surflock(void);
  画面描画開始

void scrnmng_surfunlock(const SCRNSURF *surf);
  画面描画終了(このタイミングで描画)


void scrnmng_setwidth(int posx, int width)
void scrnmng_setextend(int extend)
void scrnmng_setheight(int posy, int height)
  描画サイズの変更
  ウィンドウサイズの変更する
  フルスクリーン中であれば 表示領域を変更。
  SCRNSURFではこの値を返すようにする
  posx, widthは 8の倍数

BOOL scrnmng_isfullscreen(void) … NP2コアでは未使用
  フルスクリーン状態の取得
    return: 非0でフルスクリーン

BOOL scrnmng_haveextend(void)
  横幅状態の取得
    return: 非0で 横幅拡張サポート

UINT scrnmng_getbpp(void)
  スクリーン色ビット数の取得
    return: ビット数(8/16/24/32)

void scrnmng_palchanged(void)
  パレット更新の通知(8bitスクリーンサポート時のみ)

RGB16 scrnmng_makepal16(RGB32 pal32)
  RGB32から 16bit色を作成する。(16bitスクリーンサポート時のみ)



// ---- sound

NP2のサウンドデータは sound.cの以下の関数より取得
  const SINT32 *sound_pcmlock(void)
  void sound_pcmunlock(const SINT32 *hdl)


SOUND_CRITICAL  セマフォを入れる(see sndcsec.c)
SOUNDRESERVE    予約バッファのサイズ(ミリ秒)
  サウンドを割り込み処理する場合の指定。
  割り込みの最大延滞時間をSOUNDRESERVEで指定。
  (Win9xの場合、自前でリングバッファを見張るので 割り込み無し・指定時間通りに
  サウンドライトが来るので、この処理は不要だった)


UINT soundmng_create(UINT rate, UINT ms)
  サウンドストリームの確保
    input:  rate    サンプリングレート(11025/22050/44100)
            ms      サンプリングバッファサイズ(ミリ秒)
    return: 獲得したバッファのサンプリング数

            msに従う必要はない(SDLとかバッファサイズが限定されるので)
            NP2のサウンドバッファ操作は 返り値のみを利用しています。


void soundmng_destroy(void)
  サウンドストリームの終了

void soundmng_reset(void)
  サウンドストリームのリセット

void soundmng_play(void)
  サウンドストリームの再生

void soundmng_stop(void)
  サウンドストリームの停止

void soundmng_sync(void)
  サウンドストリームのコールバック

void soundmng_setreverse(BOOL reverse)
  サウンドストリームの出力反転設定
    input:  reverse 非0で左右反転

BOOL soundmng_pcmplay(UINT num, BOOL loop)
  PCM再生
    input:  num     PCM番号
            loop    非0でループ

void soundmng_pcmstop(UINT num)
  PCM停止
    input:  num     PCM番号



// ---- mouse

BYTE mousemng_getstat(SINT16 *x, SINT16 *y, int clear)
  マウスの状態取得
    input:  clear   非0で 状態を取得後にカウンタをリセットする
    output: *x      clearからのx方向カウント
            *y      clearからのy方向カウント
    return: bit7    左ボタンの状態 (0:押下)
            bit5    右ボタンの状態 (0:押下)



// ---- serial/parallel/midi

COMMNG commng_create(UINT device)
  シリアルオープン
    input:  デバイス番号
    return: ハンドル (失敗時NULL)


void commng_destroy(COMMNG hdl)
  シリアルクローズ
    input:  ハンドル (失敗時NULL)



// ---- joy stick

BYTE joymng_getstat(void)
  ジョイスティックの状態取得

    return: bit0    上ボタンの状態 (0:押下)
            bit1    下ボタンの状態
            bit2    左ボタンの状態
            bit3    右ボタンの状態
            bit4    連射ボタン１の状態
            bit5    連射ボタン２の状態
            bit6    ボタン１の状態
            bit7    ボタン２の状態


// ----

void sysmng_update(UINT bitmap)
  状態が変化した場合にコールされる。

void sysmng_cpureset(void)
  リセット時にコールされる



void taskmng_exit(void)
  システムを終了する。

