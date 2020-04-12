#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cs4231io.h"
#include	"cs4231.h"
#include	"sound.h"
#include	"fmboard.h"


static const UINT8 cs4231dma[] = {0xff,0x00,0x01,0x03,0xff,0x00,0x01,0x03};
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
REG8 sa3_control;
UINT8 sa3data[256];
static void IOOUTCALL csctrl_o480(UINT port, REG8 dat) {

	sa3_control = dat;
	(void)port;

}
static REG8 IOINPCALL csctrl_i480(UINT port) {
	TRACEOUT(("read %x",port));
	return sa3_control;
	(void)port;

}
static void IOOUTCALL csctrl_o481(UINT port, REG8 dat) {
	sa3data[sa3_control] = dat;
	(void)port;

}
static REG8 IOINPCALL csctrl_i481(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return sa3data[sa3_control];
}
REG8 f4a_control;
UINT8 f4bdata[256];
static void IOOUTCALL csctrl_of4a(UINT port, REG8 dat) {
	f4a_control = dat;
	(void)port;
}

static REG8 IOINPCALL csctrl_if4a(UINT port) {
	TRACEOUT(("read %x",port));
	return f4a_control;
	(void)port;
}

static void IOOUTCALL csctrl_of4b(UINT port, REG8 dat) {
	f4bdata[f4a_control] = dat;
	(void)port;
}

static REG8 IOINPCALL csctrl_if4b(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return f4bdata[f4a_control];
}

static REG8 IOINPCALL csctrl_iac6d(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return (0x54);
}


static REG8 IOINPCALL csctrl_iac6e(UINT port) {

	(void)port;
	return 0;
}

static REG8 IOINPCALL srnf_i51ee(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return (0x02);
}

static REG8 IOINPCALL srnf_i51ef(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return 0xc2;
}

static REG8 IOINPCALL srnf_i56ef(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return (0x9f);
}
static REG8 IOINPCALL srnf_i57ef(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return (0xc0);
}

static REG8 IOINPCALL srnf_i59ef(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return 0x3;
}

static REG8 IOINPCALL srnf_i5bef(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return (0x0e);
}
	
static REG8 IOINPCALL ifab(UINT port) {
	TRACEOUT(("read %x",port));
	(void)port;
	return (0);
}

// ----

void cs4231io_reset(void) {
	
	UINT8 sndirq, snddma;

	cs4231.enable = 1;

/* [cs4231.adrsの書き方]
		bit	
		7	未使用
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
			100b〜101b= 未定義
			111b= DMAを使用しない
*/		
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16){
		sndirq = np2cfg.sndwssirq;
		snddma = np2cfg.sndwssdma;
		//cs4231.adrs = 0x0a;////0b00 001 010  INT0 DMA1
		//cs4231.adrs = 0x22;////0b00 100 010  INT5 DMA1
	}else if(g_nSoundID==SOUNDID_PC_9801_86_118 || g_nSoundID==SOUNDID_PC_9801_86_118_SB16){
		UINT8 irq86table[4] = {0x03, 0x0d, 0x0a, 0x0c};
		UINT8 nIrq86 = (np2cfg.snd86opt & 0x10) | ((np2cfg.snd86opt & 0x4) << 5) | ((np2cfg.snd86opt & 0x8) << 3);
		UINT8 irq86 = irq86table[nIrq86 >> 6];
		sndirq = np2cfg.snd118irqp;
		snddma = np2cfg.snd118dma;
		if(sndirq == irq86){
			if(irq86!=3){
				sndirq = 0x3;
			}else{
				sndirq = 0xC;
			}
		}
	}else if(g_nSoundID==SOUNDID_WAVESTAR){
		//UINT8 irq86table[4] = {0x03, 0x0d, 0x0a, 0x0c};
		//UINT8 nIrq86 = (np2cfg.snd86opt & 0x10) | ((np2cfg.snd86opt & 0x4) << 5) | ((np2cfg.snd86opt & 0x8) << 3);
		//UINT8 irq86 = irq86table[nIrq86 >> 6];
		sndirq = 12;// IRQ12固定　irq86;
		snddma = 3;// DMA#3固定 np2cfg.snd118dma; 
	}else{
		sndirq = np2cfg.snd118irqp;
		snddma = np2cfg.snd118dma;
		//cs4231.adrs = 0x23;////0b00 100 011  INT5 DMA3
	}
	cs4231.adrs = 0;
	switch(sndirq){
	case 3:
		cs4231.adrs |= (0x1 << 3);
		break;
	case 5:
		cs4231.adrs |= (0x2 << 3);
		break;
	case 10:
		cs4231.adrs |= (0x3 << 3);
		break;
	case 12:
		cs4231.adrs |= (0x4 << 3);
		break;
	default:
		break;
	}
	switch(snddma){
	case 0:
		cs4231.adrs |= (0x1);
		break;
	case 1:
		cs4231.adrs |= (0x2);
		break;
	case 3:
		cs4231.adrs |= (0x3);
		break;
	default:
		break;
	}
	cs4231.dmairq = cs4231irq[(cs4231.adrs >> 3) & 7]; // IRQをセット
	cs4231.dmach = cs4231dma[cs4231.adrs & 7]; // DMAチャネルをセット
	cs4231.port[0] = 0x0f40; //WSS BASE I/O port
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_PC_9801_86_118 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_118_SB16){
		cs4231.port[1] = 0xb460; // Sound ID I/O port (A460hは86音源が使うのでB460hに変更)
	}else{
		cs4231.port[1] = 0xa460; // Sound ID I/O port
	}
	cs4231.port[2] = 0x0f48; // WSS FIFO port
	cs4231.port[4] = np2cfg.snd118io;//0x0188; // OPN port
	cs4231.port[5] = 0x0f4a; // canbe mixer i/o port?
	cs4231.port[6] = 0x548e; // YMF-701/715?
	cs4231.port[8] = 0x1480; // Joystick
	cs4231.port[9] = 0x1488; // OPL3
	cs4231.port[10] = 0x148c; // MIDI
	cs4231.port[11] = 0x0480; //9801-118 control?
	cs4231.port[14] = 0x148e; //9801-118 config 
	cs4231.port[15] = 0xa460; //空いてるのでこっちを利用

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
	if(g_nSoundID==SOUNDID_PC_9801_118 || g_nSoundID==SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
		cs4231.reg.chipid	=0xa2;//19 from PC-9801-118 CS4231
	}else{
		cs4231.reg.chipid	=0x80;//19 from PC-9821Nr166 YMF715
	}
	cs4231.reg.monoinput=0xc0;//1a from PC-9821Nr166
	cs4231.reg.reserved3=0x80; //1b from PC-9821Nr166
	cs4231.reg.reserved4=0x80; //1d from PC-9821Nr166
	cs4231.intflag = 0xcc;

	sa3data[7] = 7;
	sa3data[8] = 7;
	switch (cs4231.dmairq){
		case 0x0c:f4bdata[1] = 0;break;
		case 0x0a:f4bdata[1] = 0x02;break;
		case 0x03:f4bdata[1] = 0x03;break;
		case 0x05:f4bdata[1] = 0x08;break;
	}
}

void cs4231io_bind(void) {

	sound_streamregist(&cs4231, (SOUNDCB)cs4231_getpcm); // CS4231用 オーディオ再生ストリーム
	if(g_nSoundID!=SOUNDID_WAVESTAR){
		iocore_attachout(0xc24, csctrl_oc24);
		iocore_attachout(0xc2b, csctrl_oc2b);
		iocore_attachout(0xc2d, csctrl_oc2d);
		iocore_attachinp(0xc24, csctrl_ic24);
		iocore_attachinp(0xc2b, csctrl_ic2b);
		iocore_attachinp(0xc2d, csctrl_ic2d);
	}
	if (cs4231.dmach != 0xff) {
		dmac_attach(DMADEV_CS4231, cs4231.dmach); // CS4231のDMAチャネルを割り当て
	}
	if(!(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16)){
		iocore_attachout(0x480, csctrl_o480);
		iocore_attachinp(0x480, csctrl_i480);
		iocore_attachinp(0x481, csctrl_i481);
		iocore_attachinp(0xac6d, csctrl_iac6d);
		iocore_attachinp(0xac6e, csctrl_iac6e);

/*　必要な時だけ有効にすべき
//WSN-F???
		iocore_attachinp(0x51ee, srnf_i51ee);//7番めに読まれる
		iocore_attachinp(0x51ef, srnf_i51ef);//1番最初にC2を返す
//		iocore_attachinp(0x52ef, srnf_i52ef);//f40等を読み書きしたあとここを読んでエラー
		iocore_attachinp(0x56ef, srnf_i56ef);//2番めに読まれて割り込み等の設定？　4番めに2回読まれ直す
		iocore_attachinp(0x57ef, srnf_i57ef);//5番めに読まれる
		iocore_attachinp(0x59ef, srnf_i59ef);//3番めに読まれて何か調査 ３と４でとりあえず通る
//		iocore_attachinp(0x5aef, srnf_i5aef);//8番めに読まれて終わり
		iocore_attachinp(0x5bef, srnf_i5bef);//6番めに読まれる
*/
	}
}
void cs4231io_unbind(void) {

	iocore_detachout(0xc24);
	iocore_detachout(0xc2b);
	iocore_detachout(0xc2d);
	iocore_detachinp(0xc24);
	iocore_detachinp(0xc2b);
	iocore_detachinp(0xc2d);
	if (cs4231.dmach != 0xff) {
		dmac_detach(DMADEV_CS4231); // CS4231のDMAチャネルを割り当て
	}
	if(!(g_nSoundID==SOUNDID_PC_9801_86_WSS || g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_WAVESTAR || g_nSoundID==SOUNDID_WSS_SB16 || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16)){
		iocore_detachout(0x480);
		iocore_detachinp(0x480);
		iocore_detachinp(0x481);
		iocore_detachinp(0xac6d);
		iocore_detachinp(0xac6e);

/*　必要な時だけ有効にすべき
//WSN-F???
		iocore_detachinp(0x51ee);//7番めに読まれる
		iocore_detachinp(0x51ef);//1番最初にC2を返す
//		iocore_detachinp(0x52ef);//f40等を読み書きしたあとここを読んでエラー
		iocore_detachinp(0x56ef);//2番めに読まれて割り込み等の設定？　4番めに2回読まれ直す
		iocore_detachinp(0x57ef);//5番めに読まれる
		iocore_detachinp(0x59ef);//3番めに読まれて何か調査 ３と４でとりあえず通る
//		iocore_detachinp(0x5aef);//8番めに読まれて終わり
		iocore_detachinp(0x5bef);//6番めに読まれる
*/
	}
}

int acicounter;
// CS4231 I/O WRITE
void IOOUTCALL cs4231io0_w8(UINT port, REG8 value) {

	switch(port - cs4231.port[0]) {
		case 0x00: // PCM音源の割り込みアドレス設定
			cs4231.adrs = value &= ~0x40;
			cs4231.dmairq = cs4231irq[(value >> 3) & 7];
			cs4231.dmach = cs4231dma[value & 7];
			dmac_detach(DMADEV_CS4231);
			if (cs4231.dmach != 0xff) {
				if ((cs4231.adrs >> 2) & 1){
					if (cs4231.dmach == 0)dmac_attach(DMADEV_NONE, 1);
					else dmac_attach(DMADEV_NONE, 0);
				}
				dmac_attach(DMADEV_CS4231, cs4231.dmach); // CS4231のDMAチャネルを割り当て
#if 0
				if (cs4231.reg.iface & SDC) {
					dmac.dmach[cs4231.dmach].ready = 1;
					dmac_check();
				}
#endif
			}
			break;
			
		case 0x04: // Index Address Register (R0) INIT MCE TRD IA4 IA3 IA2 IA1 IA0
			if ( !(cs4231.index & MCE) && (value & MCE) && (cs4231.reg.iface & (CAL0|CAL1) ) ) acicounter = 1;
			if (!(cs4231.index & MCE)) cs4231.intflag |= (PRDY|CRDY);
			cs4231.index = value & ~(INIT|TRD);
			break;
		case 0x05: // Indexed Data Register (R1) ID7 ID6 ID5 ID4 ID3 ID2 ID1 ID0
			cs4231_control(cs4231.index & 0x1f, value); // cs4231c.c内で処理
			break;

		case 0x06: // Status Register (R2, Read Only) CU/L CL/R CRDY SER PU/L PL/R PRDY INT
			// PI,CI,TI割り込みビットを全部クリア
			if (cs4231.intflag & INt) {
				pic_resetirq(cs4231.dmairq);
				//nevent_set(NEVENT_CS4231, 0, cs4231_dma, NEVENT_ABSOLUTE);
			}
			cs4231.intflag &= ~INt;
			cs4231.reg.featurestatus &= ~(PI|TI|CI);
			break;

		case 0x07: // Capture I/O Data Register (R3, Read Only) CD7 CD6 CD5 CD4 CD3 CD2 CD1 CD0
			cs4231_datasend(value);
			break;
	}
}
void IOOUTCALL cs4231io0_w8_wavestar(UINT port, REG8 value) {
	cs4231io0_w8(((port - 0xA460) >> 1) + cs4231.port[0] + 1, value);
}
// CS4231 I/O READ
REG8 IOINPCALL cs4231io0_r8(UINT port) {

	switch(port - cs4231.port[0]) {
		case 0x00: // PCM音源の割り込みアドレス設定
			return(cs4231.adrs);
		case 0x03: // Windows Sound System ID (Read Only)
			return(0x04);
//			return(0x05);//PC-9821Nr
		case 0x04: // Index Address Register (R0) INIT MCE TRD IA4 IA3 IA2 IA1 IA0
			return(cs4231.index & ~(INIT|TRD|MCE));
		case 0x05: // Indexed Data Register (R1) ID7 ID6 ID5 ID4 ID3 ID2 ID1 ID0
			{
				switch (cs4231.index & 0x1f){
					case 0x0b: // Error Status and Initialization (I11, Read Only) COR PUR ACI DRS ORR1 ORR0 ORL1 ORL0
						if(acicounter){
							TRACEOUT(("acicounter"));
							acicounter -= 1;
							cs4231.reg.errorstatus |= ACI;
						}else{
							cs4231.reg.errorstatus &= ~ACI;
						}
						break;	
					case 0x0d: // Loopback Control (I13) LBA5 LBA4 LBA3 LBA2 LBA1 LBA0 res LBE
						return 0;					
					default:
						break;
				}
				return(*(((UINT8 *)(&cs4231.reg)) + (cs4231.index & 0x1f)));
			}
		case 0x06: // Status Register (R2, Read Only) CU/L CL/R CRDY SER PU/L PL/R PRDY INT
			if (cs4231.reg.errorstatus & (1 << 6)) cs4231.intflag |= SER;
			return (cs4231.intflag);
		case 0x07: // Capture I/O Data Register (R3, Read Only) CD7 CD6 CD5 CD4 CD3 CD2 CD1 CD0
			return (0x80);
	}
	return(0);
}
REG8 IOINPCALL cs4231io0_r8_wavestar(UINT port) {
	return cs4231io0_r8(((port - 0xA460) >> 1) + cs4231.port[0] + 1);
}

// canbe mixer i/o port? WRITE
void IOOUTCALL cs4231io5_w8(UINT port, REG8 value) {

	switch(port - cs4231.port[5]) {
		case 0x00:
			cs4231.extindex = value;
			break;

		case 0x01:
			switch(cs4231.extindex){
			case 0x02: // MODEM L ?
			case 0x03: // MODEM R ?
			case 0x30: // FM音源 L
			case 0x31: // FM音源 R
			case 0x32: // CD-DA L
			case 0x33: // CD-DA R
			case 0x34: // TV L
			case 0x35: // TV R
			case 0x36: // MODEM mono ?
				// bit7:mute, bit6,5:reserved, bit4-0:volume(00000(MAX) - 11111(MIN))
				cs4231.devvolume[cs4231.extindex] = value;
			}
			break;
	}
}
// canbe mixer i/o port? READ
REG8 IOINPCALL cs4231io5_r8(UINT port) {

	switch(port - cs4231.port[5]) {
		case 0x00:
			return(cs4231.extindex);

		case 0x01:
			switch(cs4231.extindex){
			case 1:
				return(0);				// means opna int5 ???
			case 0x02: // MODEM L ?
			case 0x03: // MODEM R ?
			case 0x30: // FM音源 L
			case 0x31: // FM音源 R
			case 0x32: // CD-DA L
			case 0x33: // CD-DA R
			case 0x34: // TV L
			case 0x35: // TV R
			case 0x36: // MODEM mono ?
				// bit7:mute, bit6,5:reserved, bit4-0:volume(00000(MAX) - 11111(MIN))
				return cs4231.devvolume[cs4231.extindex];
			}
			break;
	}
	return(0xff);
}
