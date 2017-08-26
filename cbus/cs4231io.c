#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cs4231io.h"
#include	"cs4231.h"
#include	"sound.h"
#include	"fmboard.h"


static const UINT8 cs4231dma[] = {0xff,0x00,0x01,0x03,0xff,0xff,0xff,0xff};
static const UINT8 cs4231irq[] = {0xff,0x03,0x06,0x0a,0x0c,0xff,0xff,0xff};


static void IOOUTCALL csctrl_oc24(UINT port, REG8 dat) {

	cs4231.portctrl = dat;
	(void)port;
}

static void IOOUTCALL csctrl_oc2b(UINT port, REG8 dat) {

	UINT	num;

	if ((cs4231.portctrl & 0x60) == 0x60) {
		num = cs4231.portctrl & 0xf;
			cs4231.port[num] &= 0xff00;
			cs4231.port[num] |= dat;
	}
	(void)port;
}

static void IOOUTCALL csctrl_oc2d(UINT port, REG8 dat) {

	UINT	num;

	if ((cs4231.portctrl & 0x60) == 0x60) {
		num = cs4231.portctrl & 0xf;
			cs4231.port[num] &= 0x00ff;
			cs4231.port[num] |= (dat << 8);
	}
	(void)port;
}

static REG8 IOINPCALL csctrl_ic24(UINT port) {

	(void)port;
	return((0xe0 | cs4231.portctrl));
}

static REG8 IOINPCALL csctrl_ic2b(UINT port) {

	UINT	num;

	(void)port;
	num = cs4231.portctrl & 0xf;

	return((REG8)(cs4231.port[num] & 0xff));

}

static REG8 IOINPCALL csctrl_ic2d(UINT port) {

	UINT	num;

	(void)port;
	num = cs4231.portctrl & 0xf;
	return((REG8)(cs4231.port[num] >> 8));
}
REG8 sa3_control;
static void IOOUTCALL csctrl_o480(UINT port, REG8 dat) {

	sa3_control = dat;
	(void)port;

}
static REG8 IOINPCALL csctrl_i480(UINT port) {

	return sa3_control;
	(void)port;

}

static REG8 IOINPCALL csctrl_i481(UINT port) {

	(void)port;
	if(sa3_control == 0x17) return 0;
	if(sa3_control == 0x18) return 0;
	else return 0;
}

static REG8 IOINPCALL csctrl_iac6d(UINT port) {

	(void)port;
	return (0x54);
}


static REG8 IOINPCALL csctrl_iac6e(UINT port) {

	(void)port;
	return 0;
}
static void IOOUTCALL csctrl_o51ee(UINT port, REG8 dat) {
	(void)port;
}
static void IOOUTCALL csctrl_o51ef(UINT port, REG8 dat) {
	(void)port;
}
static void IOOUTCALL csctrl_o56ef(UINT port, REG8 dat) {
	(void)port;
}
static void IOOUTCALL csctrl_o57ef(UINT port, REG8 dat) {
	(void)port;
}
static void IOOUTCALL csctrl_o5bef(UINT port, REG8 dat) {
	(void)port;
}

static REG8 IOINPCALL csctrl_i51ee(UINT port) {
	(void)port;
	return 0x1;//0b1;
}
static REG8 IOINPCALL csctrl_i51ef(UINT port) {
	(void)port;
	return (0xc2);
}
static REG8 IOINPCALL csctrl_i56ef(UINT port) {
	(void)port;
	return 0x10;//(0b00010000);
}
static REG8 IOINPCALL csctrl_i57ef(UINT port) {
	(void)port;
	return 0xc0;//(0b11000000);
}
static REG8 IOINPCALL csctrl_i5bef(UINT port) {
	(void)port;
	return 0x0a;//(0b00001010);
}



// ----

void cs4231io_reset(void) {

	cs4231.enable = 1;
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS){
		cs4231.adrs = 0x0a;////0b00 001 010  INT0 DMA1
	}else{
		cs4231.adrs = 0x22;////0b00 100 010  INT5 DMA1
	}
	cs4231.dmairq = cs4231irq[(cs4231.adrs >> 3) & 7];
	cs4231.dmach = cs4231dma[cs4231.adrs & 7];
	
/*		7	未使用
	R/W	6	不明
	R/W	5-3	PCM音源割り込みアドレス
			000b= サウンド機能を使用しない
			001b= INT 0
			010b= INT 1
			011b= INT 41
			100b= INT 5
	R/W	2-0	DMAチャネル設定
			000b= DMAを使用しない
			001b= DMA #0
			010b= DMA #1 (PC-9821Npを除く)
			011b= DMA #3
			100b～101b= 未定義
			111b= DMAを使用しない
*/			
	if ((cs4231.adrs & 7) == 0x1/*0b001*/) cs4231.dmach = 0x00;
	if ((cs4231.adrs & 7) == 0x2/*0b010*/) cs4231.dmach = 0x01;
	if ((cs4231.adrs & 7) == 0x3/*0b011*/) cs4231.dmach = 0x03;
	if ((cs4231.adrs & 7) == 0x6/*0b110*/) cs4231.dmach = 0x01;//YMF-701,715
	if ((cs4231.adrs & 7) == 0x0/*0b000*/) cs4231.dmach = 0x01;//YMF-701,715
	if ((cs4231.adrs & 7) == 0x7/*0b111*/) cs4231.dmach = 0xff;
	if (cs4231.dmach != 0xff) {
		dmac_attach(DMADEV_CS4231, cs4231.dmach);
	}
	cs4231.port[0] = 0x0f40; //WSS BASE I/O port
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS){
		cs4231.port[1] = 0xb460; // Sound ID I/O port
	}else{
		cs4231.port[1] = 0xa460; // Sound ID I/O port
	}
	cs4231.port[2] = 0x0f48; // WSS FIFO port
	cs4231.port[4] = 0x0188; // OPN port
	cs4231.port[5] = 0x0f4a;  // canbe mixer i/o port?
/*Port	　	　bit　	
0F4Ah	　R/W	　7-0 	音量制御する周辺デバイスＩＤ
PnP により移動可			　00h: ？
			　01h: OPNA ON/OFF???
			　02h: 内蔵モデム(ステレオ)？ 左音量
			　03h: 内蔵モデム(ステレオ)？ 右音量
			　30h: FM音源 左音量
			　31h: FM音源 右音量
			　32h: CD-ROM 左音量
			　33h: CD-ROM 右音量
			　34h: ＴＶ 左音量
			　35h: ＴＶ 右音量
			　36h: 内蔵モデム(モノラル)？
			　
0F4Bh	　R/W	　7　 	ミュート設定
PnP により移動可			　0:ミュートしない
			　1:ミュートする
		　6-5	未使用
		　4-0	音量設定
			　00000b:最大
　　（
　　 ）
　11111b:最小
*/
	cs4231.port[6] = 0x548e; // YMF-701/715?
	cs4231.port[8] = 0x1480; // Joystick
	cs4231.port[9] = 0x1488; // OPL3
	cs4231.port[10] = 0x148c; // MIDI
	cs4231.port[11] = 0x0480; //9801-118 control? OPLという情報もありだが…
	cs4231.port[14] = 0x148e; //9801-118 config 
	cs4231.port[15] = 0xffff;

	TRACEOUT(("CS4231 - IRQ = %d", cs4231.dmairq));
	TRACEOUT(("CS4231 - DMA channel = %d", cs4231.dmach));
	cs4231.reg.aux1_l = 0x88;//2
	cs4231.reg.aux1_r = 0x88;//3
	cs4231.reg.aux2_l = 0x88;//4
	cs4231.reg.aux2_r = 0x88;//5
	cs4231.reg.iface  = 0x08;//9
	cs4231.reg.mode_id = 0xca;//c from PC-9821Nr166
	cs4231.reg.featurefunc[0]=0x80; //10 from PC-9821Nr166
	cs4231.reg.line_l = 0x88;//12
	cs4231.reg.line_r = 0x88;//13
	cs4231.reg.reserved1=0x80; //16 from PC-9821Nr166
	cs4231.reg.reserved2=0x80; //17 from PC-9821Nr166
	if(g_nSoundID==SOUNDID_PC_9801_118){
		cs4231.reg.chipid	=0xa2;//19 from PC-9801-118 CS4231
	}else{
		cs4231.reg.chipid	=0x80;//19 from PC-9821Nr166 YMF715
	}
	cs4231.reg.monoinput=0xc0;//1a from PC-9821Nr166
	cs4231.reg.reserved3=0x80; //1b from PC-9821Nr166
	cs4231.reg.reserved4=0x80; //1d from PC-9821Nr166
	cs4231.intflag = 0x22;// 0x22??
}

void cs4231io_bind(void) {

	sound_streamregist(&cs4231, (SOUNDCB)cs4231_getpcm);
	iocore_attachout(0xc24, csctrl_oc24);
	iocore_attachout(0xc2b, csctrl_oc2b);
	iocore_attachout(0xc2d, csctrl_oc2d);
	iocore_attachinp(0xc24, csctrl_ic24);
	iocore_attachinp(0xc2b, csctrl_ic2b);
	iocore_attachinp(0xc2d, csctrl_ic2d);
	if(g_nSoundID!=SOUNDID_PC_9801_86_WSS && g_nSoundID!=SOUNDID_MATE_X_PCM){
		iocore_attachout(0x480, csctrl_o480);
		iocore_attachinp(0x480, csctrl_i480);
		iocore_attachinp(0x481, csctrl_i481);
		iocore_attachinp(0xac6d, csctrl_iac6d);
		iocore_attachinp(0xac6e, csctrl_iac6e);


//WSN-F???
		iocore_attachout(0x51ee, csctrl_o51ee);
		iocore_attachout(0x51ef, csctrl_o51ef);
		iocore_attachout(0x56ef, csctrl_o56ef);
		iocore_attachout(0x57ef, csctrl_o57ef);
		iocore_attachout(0x5bef, csctrl_o5bef);
		iocore_attachinp(0x51ee, csctrl_i51ee);
		iocore_attachinp(0x51ef, csctrl_i51ef);
		iocore_attachinp(0x56ef, csctrl_i56ef);
		iocore_attachinp(0x57ef, csctrl_i57ef);
		iocore_attachinp(0x5bef, csctrl_i5bef);
	}
}

void IOOUTCALL cs4231io0_w8(UINT port, REG8 value) {

	switch(port - cs4231.port[0]) {
		case 0x00:
			cs4231.adrs = value;
			cs4231.dmairq = cs4231irq[(value >> 3) & 7];
			cs4231.dmach = cs4231dma[value & 7];
			if ((value & 7) == 0x1/*0b001*/) cs4231.dmach = 0x00;
			if ((value & 7) == 0x2/*0b010*/) cs4231.dmach = 0x01;
			if ((value & 7) == 0x3/*0b011*/) cs4231.dmach = 0x03;
			if ((value & 7) == 0x6/*0b110*/) cs4231.dmach = 0x01;// YMF-701,715
			if ((value & 7) == 0x0/*0b000*/) cs4231.dmach = 0x01;// YMF-701.715
			if ((value & 7) == 0x7/*0b111*/) cs4231.dmach = 0xff;
			dmac_detach(DMADEV_CS4231);
			if (cs4231.dmach != 0xff) {
				dmac_attach(DMADEV_CS4231, cs4231.dmach);
#if 0
				if (cs4231.sdc_enable) {
					dmac.dmach[cs4231.dmach].ready = 1;
					dmac_check();
				}
#endif
			}
			break;
			
		case 0x04:
			cs4231.index = value;
			TRACEOUT(("index %x", cs4231.index & 0x1f));
			break;
		case 0x05:
			cs4231_control(cs4231.index & 0x1f, value);
			TRACEOUT(("register %x", value ));
			break;

		case 0x06:
			pic_resetirq(cs4231.dmairq);
			cs4231.intflag &= 0xfe;
			cs4231.reg.featurestatus &= 0x8f;
			//TRACEOUT(("status %x", value));
			break;

		case 0x07:
			cs4231_datasend(value);
//			TRACEOUT(("PCM %x", value));
			break;
	}
}

REG8 IOINPCALL cs4231io0_r8(UINT port) {

	REG8 ret;
	switch(port - cs4231.port[0]) {
		case 0x00:
			return(cs4231.adrs);
		case 0x03:
			return(0x04);
//			return(0x05);//PC-9821Nr
		case 0x04:
			TRACEOUT(("index r 0x%02x", cs4231.index & 0x7f));
			if (cs4231.reg.mode_id & 0x40) return (cs4231.index &= 0x0f);
			else return (cs4231.index &= 0x1f);
//			return(cs4231.index & 0x7f);
		case 0x05:
			if ((cs4231.index & 0x1f) == 0x0c) return (cs4231.reg.mode_id |= 0x8a); //why 8a???
			return(*(((UINT8 *)(&cs4231.reg)) + (cs4231.index & 0x1f)));
		case 0x06:
			TRACEOUT(("status r 0x%02x", cs4231.intflag));
			pic_resetirq(cs4231.dmairq);
			return(cs4231.intflag);
	}
	return(0);
}

void IOOUTCALL cs4231io5_w8(UINT port, REG8 value) {

	switch(port - cs4231.port[5]) {
		case 0x00:
			cs4231.extindex = value;
			break;
	}
}

REG8 IOINPCALL cs4231io5_r8(UINT port) {

	switch(port - cs4231.port[5]) {
		case 0x00:
			return(cs4231.extindex);

		case 0x01:
			if (cs4231.extindex == 1) {
				return(0);				// means opna int5 ???
			}
			break;
	}
	return(0xff);
}