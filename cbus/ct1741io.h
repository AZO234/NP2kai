#pragma once

#ifdef __cplusplus
extern "C" {
#endif


enum DSP_STATUS {
	DSP_STATUS_NORMAL,
	DSP_STATUS_RESET
};
typedef enum {
	DSP_DMA_NONE,
	DSP_DMA_2,DSP_DMA_3,DSP_DMA_4,DSP_DMA_8,
	DSP_DMA_16,DSP_DMA_16_ALIASED,
} DMA_MODES;
typedef enum {
	DSP_MODE_NONE,
	DSP_MODE_DAC,
	DSP_MODE_DMA,
	DSP_MODE_DMA_PAUSE,
	DSP_MODE_DMA_MASKED
} DSP_MODES;



#define DSP_NO_COMMAND 0
#define SB_SH	14

#define DMA_BUFSIZE  1024
#define DMA_BUFMASK  1023
#define DSP_BUFSIZE 64


typedef struct {
	BOOL stereo,sign,autoinit;
	DMA_MODES mode;
	UINT32 rate,mul;
	UINT32 total,left,min;
//	unsigned __int64 start;
	union {
		UINT8  b8[DMA_BUFSIZE];
		SINT16 b16[DMA_BUFSIZE];
	} buf;
//	UINT32 bits;
	DMACH	chan;
	UINT32 remain_size;

	UINT		bufsize; // サウンド再生用の循環バッファサイズ。データのread/writeは4byte単位（16bitステレオの1サンプル単位）で行うこと
	UINT		bufdatas; // = (bufwpos-bufpos)&CS4231_BUFMASK
	UINT		bufpos; // バッファの読み取り位置。bufwposと一致してもよいが追い越してはいけない
	UINT		bufwpos; // バッファの書き込み位置。周回遅れのbufposに追いついてはいけない（一致も不可）
	UINT32		pos12;
	UINT32		step12;
	UINT8		buffer[DMA_BUFSIZE];
	UINT32		rate2;
	
	UINT8 lastautoinit;
	UINT8 last16mode;
	UINT32 laststartcount;
	UINT32 laststartaddr;
} DMA_INFO;

typedef struct {
	DMA_INFO dma;
	UINT8 state;
	UINT8 cmd;
	UINT8 cmd_len;
	UINT8 cmd_in_pos;
	UINT8 cmd_in[DSP_BUFSIZE];
	struct {
		UINT8 data[DSP_BUFSIZE];
		UINT32 pos,used;
	} in,out;
	UINT8 test_register;
	UINT32 write_busy;
	DSP_MODES mode;
	UINT32 freq;
	UINT8 dmairq;
	UINT8 dmach;
	UINT8 cmd_o;
	
	int smpcounter2; // DMA転送開始以降に送られた有効なデータ数の合計
	int smpcounter; // DMA転送開始以降に送られたDMAデータ数の合計（無効なデータも含む）
	
	UINT8 speaker;
	UINT8 uartmode;
} DSP_INFO;

extern 


void ct1741io_reset();
void ct1741io_bind(void);
void ct1741io_unbind(void);
REG8 DMACCALL ct1741dmafunc(REG8 func);
void ct1741_set_dma_irq(UINT8 irq);
void ct1741_set_dma_ch(UINT8 dmach);
UINT8 ct1741_get_dma_irq();
UINT8 ct1741_get_dma_ch();
void ct1741_initialize(UINT rate);

void ct1741_dma(NEVENTITEM item);
#ifdef __cplusplus
}
#endif
