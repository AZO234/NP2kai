

#ifdef __cplusplus
extern "C" {
#endif

void cs4231io_reset(void);
void cs4231io_bind(void);
void cs4231io_unbind(void);

void IOOUTCALL cs4231io0_w8(UINT port, REG8 value);
REG8 IOINPCALL cs4231io0_r8(UINT port);
void IOOUTCALL cs4231io0_w8_wavestar(UINT port, REG8 value);
REG8 IOINPCALL cs4231io0_r8_wavestar(UINT port);
void IOOUTCALL cs4231io2_w8(UINT port, REG8 value);
REG8 IOINPCALL cs4231io2_r8(UINT port);
void IOOUTCALL cs4231io5_w8(UINT port, REG8 value);
REG8 IOINPCALL cs4231io5_r8(UINT port);

#ifdef __cplusplus
}
#endif

//Index Address Register 0xf44
#define TRD (1 << 5) //cs4231.index bit5 Transfer Request Disable
#define MCE (1 << 6) //cs4231.index bit6 Mode Change Enable
#define INIT (1 << 7)//cs4231.index bit7 Ititialization

//Status Register 0xf46
#define INt (1 << 0) //cs4231.intflag bit0 Interrupt Status
#define PRDY (1 << 1) //cs4231.intflag bit1 Playback Data Ready(PIO data)
#define PLR (1 << 2) //cs4231.intflag bit2 Playback Left/Right Sample
#define PULR (1 << 3) //cs4231.intflag bit3 Playback Upper/Lower Byte
#define SER (1 << 4) //cs4231.intflag bit4 Sample Error
#define CRDY (1 << 5) //cs4231.intflag bit5 Capture Data Ready(PIO)
#define CLR (1 << 6) //cs4231.intflag bit6 Capture Left/Right Sample
#define CUL (1 << 7) //cs4231.intglag bit7 Capture Upper/Lower Byte

//cs4231.reg.iface(9)
#define PEN (1 << 0) //bit0 Playback Enable set and reset without MCE
#define CEN (1 << 1) //bit1 Capture Enable
#define SDC (1 << 2) //bit2 Single DMA Channel 0 Dual 1 Single 逆と思ってたので修正すべし
#define CAL0 (1 << 3) //bit3 Calibration 0 No Calibration 1 Converter calibration
#define CAL1 (1 << 4) //bit4 2 DAC calibration 3 Full Calibration
#define PPIO (1 << 6) //bit6 Playback PIO Enable 0 DMA 1 PIO
#define CPIO (1 << 7) //bit7 Capture PIO Enable 0 DMA 1 PIO

//cs4231.reg.errorstatus(10)
#define ACI (1 << 5) //bit5 Auto-calibrate In-Progress

//cs4231.reg.pinctrl(11)
#define IEN (1 << 1) //bit1 Interrupt Enable reflect cs4231.intflag bit0
#define DEN (1 << 3) //bit3 Dither Enable only active in 8-bit unsigned mode

//cs4231.reg.modeid(12)
#define MODE2 (1 << 6) //bit6

//cs4231.reg.featurestatus(24)
#define PI (1 << 4) //bit4 Playback Interrupt pending from Playback DMA count registers
#define CI (1 << 5) //bit5 Capture Interrupt pending from record DMA count registers when SDC=1 non-functional
#define TI (1 << 6) //bit6 Timer Interrupt pending from timer count registers
// PI,CI,TI bits are reset by writing a "0" to the particular interrupt bit or by writing any value to the Status register
