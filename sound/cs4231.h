/**
sudo make - * @file	cs4231.h
 * @brief	Interface of the CS4231
 */

#pragma once

#include "sound.h"
#include "io/dmac.h"

enum {
	CS4231_BUFFERS	= (1 << 11),
	CS4231_BUFMASK	= (CS4231_BUFFERS - 1),
	CS4231_MAXDMAREADBYTES	= (1 << 9)
};

typedef struct {
	UINT8	adc_l;				// 0
	UINT8	adc_r;				// 1
	UINT8	aux1_l;				// 2
	UINT8	aux1_r;				// 3
	UINT8	aux2_l;				// 4
	UINT8	aux2_r;				// 5
	UINT8	dac_l;				// 6
	UINT8	dac_r;				// 7
	UINT8	datafmt;			// 8
	UINT8	iface;				// 9
	UINT8	pinctrl;			// a
	UINT8	errorstatus;		//b
	UINT8	mode_id;		//c
	UINT8	loopctrl;		//d
	UINT8	playcount[2];		//e-f
	UINT8	featurefunc[2];		//10-11
	UINT8	line_l;		//12
	UINT8	line_r;		//13
	UINT8	timer[2];			//14-15
	UINT8	reserved1;		//16
	UINT8	reserved2;		//17
	UINT8	featurestatus;		//18
	UINT8	chipid;		//19
	UINT8	monoinput;		//1a
	UINT8	reserved3;		//1b
	UINT8	cap_datafmt;		//1c
	UINT8	reserved4;		//1d
	UINT8	cap_basecount[2];		//1e-1f
} CS4231REG;

typedef struct {
	UINT		bufsize; // サウンド再生用の循環バッファサイズ。データのread/writeは4byte単位（16bitステレオの1サンプル単位）で行うこと
	UINT		bufdatas; // = (bufwpos-bufpos)&CS4231_BUFMASK
	UINT		bufpos; // バッファの読み取り位置。bufwposと一致してもよいが追い越してはいけない
	UINT		bufwpos; // バッファの書き込み位置。周回遅れのbufposに追いついてはいけない（一致も不可）
	UINT32		pos12;
	UINT32		step12;

	UINT8		enable; // CS4231有効フラグ
	UINT8		portctrl;
	UINT8		dmairq; // CS4231 IRQ
	UINT8		dmach; // CS4231 DMAチャネル
	UINT16		port[16]; // I/Oポートアドレス（再配置可能）
	UINT8		adrs; // DMA読み取りアドレス
	UINT8		index; // Index Address Register
	UINT8		intflag; // Status Register
	UINT8		outenable;
	UINT8		extfunc;
	UINT8		extindex;
	
	UINT16		timer; // 廃止
	SINT32		timercounter; // TI割り込み用のダウンカウンタ（の予定）

	CS4231REG	reg;
	UINT8		buffer[CS4231_BUFFERS]; // DMA読み取りアドレス

	UINT8		devvolume[0x100]; // CS4231内蔵ボリューム

	SINT32		totalsample; // PI割り込み用のデータ数カウンタ
} _CS4231, *CS4231;

typedef struct {
	UINT	rate;
} CS4231CFG;


#ifdef __cplusplus
extern "C"
{
#endif

// Index Address Register (R0) 0xf44
#define TRD (1 << 5) //cs4231.index bit5 Transfer Request Disable
#define MCE (1 << 6) //cs4231.index bit6 Mode Change Enable

// Status Register (R2, Read Only) 0xf46
#define INt (1 << 0) //cs4231.intflag bit0 Interrupt Status
#define PRDY (1 << 1) //cs4231.intflag bit1 Playback Data Ready(PIO data)
#define PLR (1 << 2) //cs4231.intflag bit2 Playback Left/Right Sample
#define PULR (1 << 3) //cs4231.intflag bit3 Playback Upper/Lower Byte
#define SER (1 << 4) //cs4231.intflag bit4 Sample Error
#define CRDY (1 << 5) //cs4231.intflag bit5 Capture Data Ready
#define CLR (1 << 6) //cs4231.intflag bit6 Capture Left/Right Sample
#define CUL (1 << 7) //cs4231.intglag bit7 Capture Upper/Lower Byte

//cs4231.reg.iface(9) Interface Configuration (I9)
#define PEN (1 << 0) //bit0 Playback Enable set and reset without MCE
#define CEN (1 << 1) //bit1 Capture Enable
#define SDC (1 << 2) //bit2 Single DMA Channel 0 Dual 1 Single 逆と思ってたので修正すべし
#define CAL0 (1 << 3) //bit3 Calibration 0 No Calibration 1 Converter calibration
#define CAL1 (1 << 4) //bit4 2 DAC calibration 3 Full Calibration
#define PPIO (1 << 6) //bit6 Playback PIO Enable 0 DMA 1 PIO
#define CPIO (1 << 7) //bit7 Capture PIO Enable 0 DMA 1 PIO

//cs4231.reg.errorstatus(11)0xb  Error Status and Initialization (I11, Read Only)
#define ACI (1 << 5) //bit5 Auto-calibrate In-Progress

//cs4231.reg.pinctrl(10)0xa  Pin Control (I10)
#define IEN (1 << 1) //bit1 Interrupt Enable reflect cs4231.intflag bit0
#define DEN (1 << 3) //bit3 Dither Enable only active in 8-bit unsigned mode

//cs4231.reg.modeid(12)0xc  MODE and ID (I12)
#define MODE2 (1 << 6) //bit6

//cs4231.reg.featurestatus(24)0x18  Alternate Feature Status (I24)
#define PI (1 << 4) //bit4 Playback Interrupt pending from Playback DMA count registers
#define CI (1 << 5) //bit5 Capture Interrupt pending from record DMA count registers when SDC=1 non-functional
#define TI (1 << 6) //bit6 Timer Interrupt pending from timer count registers
// PI,CI,TI bits are reset by writing a "0" to the particular interrupt bit or by writing any value to the Status register

void cs4231_dma(NEVENTITEM item);
REG8 DMACCALL cs4231dmafunc(REG8 func);
void cs4231_datasend(REG8 dat);

void cs4231_initialize(UINT rate);
void cs4231_setvol(UINT vol);

void cs4231_reset(void);
void cs4231_update(void);
void cs4231_control(UINT index, REG8 dat);

void SOUNDCALL cs4231_getpcm(CS4231 cs, SINT32 *pcm, UINT count);

#ifdef __cplusplus
}
#endif
