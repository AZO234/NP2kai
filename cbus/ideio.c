#include	"compiler.h"

#if 1
#undef	TRACEOUT
#define	TRACEOUT(s)	(void)(s)
//static void trace_fmt_ex(const char *fmt, ...)
//{
//	char stmp[2048];
//	va_list ap;
//	va_start(ap, fmt);
//	vsprintf(stmp, fmt, ap);
//	strcat(stmp, "¥n");
//	va_end(ap);
//	OutputDebugStringA(stmp);
//}
//#define	TRACEOUT(s)	trace_fmt_ex s
#endif	/* 1 */

// winでidentifyまでは取得に行くんだけどな…ってAnex86も同じか

#if defined(SUPPORT_IDEIO)

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"ideio.h"
#include	"atapicmd.h"
#include	"fdd/sxsi.h"
#include	"sound.h"
#include	"idebios.res"
#include	"bios/biosmem.h"
#include	"fmboard.h"
#include	"cs4231io.h"

	IDEIO	ideio;

static IDEDEV getidedev(void) {

	UINT	bank;

	bank = ideio.bank[1] & 0x7f;
	if (bank < 2) {
		return(ideio.dev + bank);
	}
	else {
		return(NULL);
	}
}

static IDEDRV getidedrv(void) {

	IDEDEV	dev;
	IDEDRV	drv;

	dev = getidedev();
	if (dev) {
		drv = dev->drv + dev->drivesel;
		if (drv->device != IDETYPE_NONE) {
			return(drv);
		}
	}
	return(NULL);
}

static const char serial[] = "824919341192        ";
static const char firm[] = "A5U.1200";
static const char model[] = "QUANTUM FIREBALL CR                     ";

static const char cdrom_serial[] = "1.0                 ";
static const char cdrom_firm[]   = "        ";
static const char cdrom_model[]  = "NEC CD-ROM DRIVE:98                     ";

static BRESULT setidentify(IDEDRV drv) {

	SXSIDEV sxsi;
	UINT16	tmp[256];
	UINT8	*p;
	UINT	i;
	UINT32	size;

	sxsi = sxsi_getptr(drv->sxsidrv);
	if ((sxsi == NULL) || (!(sxsi->flag & SXSIFLAG_READY) && drv->device != IDETYPE_CDROM)) {
		return(FAILURE);
	}

	ZeroMemory(tmp, sizeof(tmp));
	// とりあえず使ってる部分だけ
	if (drv->device == IDETYPE_HDD) {
		tmp[0] = 0x0040;		// non removable device
		tmp[1] = sxsi->cylinders;
		tmp[3] = sxsi->surfaces;
		tmp[4] = sxsi->sectors * 512;
		tmp[6] = sxsi->sectors;
		for (i=0; i<10; i++) {
			tmp[10+i] = (serial[i*2] << 8) + serial[i*2+1];
		}
		tmp[10+2] = '0'+drv->sxsidrv; // シリアル番号はユニークにしておかないと駄目っぽい
		tmp[22] = 4;
		for (i=0; i<4; i++) {
			tmp[23+i] = (firm[i*2] << 8) + firm[i*2+1];
		}
		for (i=0; i<20; i++) {
			tmp[27+i] = (model[i*2] << 8) + model[i*2+1];
		}
#if IDEIO_MULTIPLE_MAX > 0
		tmp[47] = 0x8000 | IDEIO_MULTIPLE_MAX;	// multiple
#endif
		tmp[49] = 0x0200;		// support LBA
		tmp[51] = 0x0200;
		tmp[53] = 0x0001;
		size = sxsi->cylinders * sxsi->surfaces * sxsi->sectors;
		tmp[54] = size / drv->surfaces / drv->sectors;//sxsi->cylinders;
		tmp[55] = drv->surfaces;//sxsi->surfaces;
		tmp[56] = drv->sectors;//sxsi->sectors;
		size = (UINT32)tmp[54] * tmp[55] * tmp[56];
		tmp[57] = (UINT16)size;
		tmp[58] = (UINT16)(size >> 16);
#if IDEIO_MULTIPLE_MAX > 0
		tmp[59] = 0x0100 | drv->mulmode;	// current multiple mode
#endif
		tmp[60] = (UINT16)size;
		tmp[61] = (UINT16)(size >> 16);
		tmp[63] = 0x0000;		// no support multiword DMA

		tmp[80] = 0x003e;		// support ATA-1 to 5
		tmp[81] = 0;
		tmp[82] = 0x0200;		// support DEVICE RESET
	}
	else if (drv->device == IDETYPE_CDROM) {
		tmp[0] = 0x8580;		// ATAPI,CD-ROM,removable,12bytes PACKET
		for (i=0; i<10; i++) {
			tmp[10+i] = (cdrom_serial[i*2] << 8) + cdrom_serial[i*2+1];
		}
		tmp[10+2] = '0'+drv->sxsidrv; // シリアル番号はユニークにしておかないと駄目っぽい
		for (i=0; i<4; i++) {
			tmp[23+i] = (cdrom_firm[i*2] << 8) + cdrom_firm[i*2+1];
		}
		for (i=0; i<20; i++) {
			tmp[27+i] = (cdrom_model[i*2] << 8) + cdrom_model[i*2+1];
		}
		tmp[49] = 0x0200;		// support LBA
		tmp[53] = 0x0001;
		tmp[63] = 0x0000;		// no support multiword DMA
		tmp[80] = 0x003e;		// support ATA-1 to 5
		tmp[82] = 0x0214;		// support PACKET/DEVICE RESET
		tmp[126] = 0x0000;		// ATAPI byte count
	}
	if (drv->sxsidrv & 0x1){
		// slave
		tmp[93] = 0x4b00;
	}else{
		// master
		tmp[93] = 0x407b;
	}

	p = drv->buf;
	for (i=0; i<256; i++) {
		p[0] = (UINT8)tmp[i];
		p[1] = (UINT8)(tmp[i] >> 8);
		p += 2;
	}
	drv->bufdir = IDEDIR_IN;
	drv->buftc = IDETC_TRANSFEREND;
	drv->bufpos = 0;
	drv->bufsize = 512;
	return(SUCCESS);
}

static void setintr(IDEDRV drv) {

	if (!(drv->ctrl & IDECTRL_NIEN)) {
		TRACEOUT(("ideio: setintr()"));
		ideio.bank[0] = ideio.bank[1] | 0x80;			// ????
		pic_setirq(IDE_IRQ);
		//mem[MEMB_DISK_INTH] |= 0x01; 
	}
}

// 割り込み後にBSYを解除し、DRQをセットする（コマンド継続中など）
void ideioint(NEVENTITEM item) {
	
	IDEDRV	drv;
	IDEDEV  dev;

	//ドライブがあるか
	dev = getidedev();
	if (dev == NULL) {
		return;
	}

	drv = getidedrv();
	if (drv == NULL) {
		return;
	}

	//BUSY解除
	if(dev->drv[0].status != 0xFF){
		dev->drv[0].status |= IDESTAT_DRQ;
		dev->drv[0].status &= ~IDESTAT_BSY;
	}
	if(dev->drv[1].status != 0xFF){
		dev->drv[1].status |= IDESTAT_DRQ;
		dev->drv[1].status &= ~IDESTAT_BSY;
	}

	//割り込み実行//(割り込みはドライブ毎には指定できない仕様)
	if(!(dev->drv[0].ctrl & IDECTRL_NIEN) || !(dev->drv[1].ctrl & IDECTRL_NIEN)){
		TRACEOUT(("ideio: run setdintr()"));
		pic_setirq(IDE_IRQ);
		//mem[MEMB_DISK_INTH] |= 0x01; 
	}
   (void)item;
}
// 割り込み後にBSYを解除し、DRQも解除する（コマンド終了時など）
void ideioint2(NEVENTITEM item) {
	
	IDEDRV	drv;
	IDEDEV  dev;

	//ドライブがあるか
	dev = getidedev();
	if (dev == NULL) {
		return;
	}

	drv = getidedrv();
	if (drv == NULL) {
		return;
	}

	//BUSY解除
	if(dev->drv[0].status != 0xFF){
		dev->drv[0].status &= ~IDESTAT_DRQ;
		dev->drv[0].status &= ~IDESTAT_BSY;
	}
	if(dev->drv[1].status != 0xFF){
		dev->drv[1].status &= ~IDESTAT_DRQ;
		dev->drv[1].status &= ~IDESTAT_BSY;
	}

	//割り込み実行//(割り込みはドライブ毎には指定できない仕様)
	if(!(dev->drv[0].ctrl & IDECTRL_NIEN) || !(dev->drv[1].ctrl & IDECTRL_NIEN)){
		TRACEOUT(("ideio: run setdintr()"));
		pic_setirq(IDE_IRQ);
		//mem[MEMB_DISK_INTH] |= 0x01; 
	}
   (void)item;
}

// 遅延付き割り込み
static void setdintr(IDEDRV drv, UINT8 errno, UINT8 status, UINT32 delay) {

	if (!(drv->ctrl & IDECTRL_NIEN)) {
		//drv->status |= IDESTAT_BSY;
		ideio.bank[0] = ideio.bank[1] | 0x80;           // ????
		TRACEOUT(("ideio: reg setdintr()"));

		//// 指定した時間遅延（マイクロ秒）
		//nevent_set(NEVENT_SASIIO, (pccore.realclock / 1000 / 1000) * delay, ideioint, NEVENT_ABSOLUTE);

		// 指定した時間遅延（クロック数）
		nevent_set(NEVENT_SASIIO, delay, ideioint, NEVENT_ABSOLUTE);
	}
}
static void setdintr2(IDEDRV drv, UINT8 errno, UINT8 status, UINT32 delay) {

	if (!(drv->ctrl & IDECTRL_NIEN)) {
		//drv->status |= IDESTAT_BSY;
		ideio.bank[0] = ideio.bank[1] | 0x80;           // ????
		TRACEOUT(("ideio: reg setdintr()"));

		//// 指定した時間遅延（マイクロ秒）
		//nevent_set(NEVENT_SASIIO, (pccore.realclock / 1000 / 1000) * delay, ideioint, NEVENT_ABSOLUTE);

		// 指定した時間遅延（クロック数）
		nevent_set(NEVENT_SASIIO, delay, ideioint2, NEVENT_ABSOLUTE);
	}
}


static void cmdabort(IDEDRV drv) {

	TRACEOUT(("ideio: cmdabort()"));
	drv->status = IDESTAT_DRDY | IDESTAT_ERR;
	drv->error = IDEERR_ABRT;
	setintr(drv);
}

static void drvreset(IDEDRV drv) {

	if (drv->device == IDETYPE_CDROM) {
		drv->hd = 0x10;
		drv->sc = 0x01;
		drv->sn = 0x01;
		drv->cy = 0xeb14;
		drv->status = 0;
	}
	else {
		drv->hd = 0x00;
		drv->sc = 0x01;
		drv->sn = 0x01;
		drv->cy = 0x0000;
		//drv->status = IDESTAT_DRDY;
		drv->status = IDESTAT_DRDY | IDESTAT_DSC;
	}
}

static void panic(const char *str, ...) {

	char	buf[2048];
	va_list	ap;

	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);

	msgbox("ide_panic", buf);
	exit(1);
}


// ----

static void incsec(IDEDRV drv) {

	if (!(drv->dr & IDEDEV_LBA)) {
		drv->sn++;
		if (drv->sn <= drv->sectors) {
			return;
		}
		drv->sn = 1;
		drv->hd++;
		if (drv->hd < drv->surfaces) {
			return;
		}
		drv->hd = 0;
		drv->cy++;
	}
	else {
		drv->sn++;
		if (drv->sn) {
			return;
		}
		drv->cy++;
		if (drv->cy) {
			return;
		}
		drv->hd++;
	}
}

void ideio_setcursec(FILEPOS pos) {
	IDEDRV drv;
	drv = getidedrv();
	if (drv) {
		if (!(drv->dr & IDEDEV_LBA)) {
			drv->sn = (pos % drv->sectors) + 1;
			pos /= drv->sectors;
			drv->hd = (pos % drv->surfaces);
			pos /= drv->surfaces;
			drv->cy = pos & 0xffff;
		}
		else {
			drv->sn = (pos & 0xff);
			drv->cy = ((pos >> 8) & 0xffff);
			drv->hd = ((pos >> 24) & 0xff);
		}
	}
}
static FILEPOS getcursec(const IDEDRV drv) {

	FILEPOS	ret;

	if (!(drv->dr & IDEDEV_LBA)) {
		ret = drv->cy;
		ret *= drv->surfaces;
		ret += drv->hd;
		ret *= drv->sectors;
		ret += (drv->sn - 1);
	}
	else {
		ret = drv->sn;
		ret |= (drv->cy << 8);
		ret |= (drv->hd << 24);
	}
	return(ret);
}

static void readsec(IDEDRV drv) {

	FILEPOS	sec;

	if (drv->device != IDETYPE_HDD) {
		goto read_err;
	}
	sec = getcursec(drv);
	//TRACEOUT(("readsec->drv %d sec %x cnt %d thr %d",
	//							drv->sxsidrv, sec, drv->mulcnt, drv->multhr));
	if (sxsi_read(drv->sxsidrv, sec, drv->buf, 512)) {
		TRACEOUT(("read error!"));
		goto read_err;
	}
	drv->bufdir = IDEDIR_IN;
	drv->buftc = IDETC_TRANSFEREND;
	drv->bufpos = 0;
	drv->bufsize = 512;
	// READはI/Oポートで読み取るデータが準備できたら割り込み
	if ((drv->mulcnt & (drv->multhr - 1)) == 0) {
		drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
		drv->error = 0;
		if(ideio.rwait > 0){
			drv->status |= IDESTAT_BSY;
			drv->status &= ~IDESTAT_DRQ;
			setdintr(drv, 0, 0, ideio.rwait);
		}else{
			setintr(drv);
		}
		//setintr(drv);
	}
	drv->mulcnt++;
	return;

read_err:
	cmdabort(drv);
}

static void writeinit(IDEDRV drv) {
	if (drv->device == IDETYPE_NONE) {
		goto write_err;
	}

	drv->bufdir = IDEDIR_OUT;
	drv->buftc = IDETC_TRANSFEREND;
	drv->bufpos = 0;
	drv->bufsize = 512;

	if ((drv->mulcnt & (drv->multhr - 1)) == 0) {
		drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
		drv->error = 0;
	}
	return;

write_err:
	cmdabort(drv);
}

static void writesec(IDEDRV drv) {

	if (drv->device == IDETYPE_NONE) {
		goto write_err;
	}

	drv->bufdir = IDEDIR_OUT;
	drv->buftc = IDETC_TRANSFEREND;
	drv->bufpos = 0;
	drv->bufsize = 512;
	
	// WRITEはデータ書き込みが完了したら割り込み
	if ((drv->mulcnt & (drv->multhr - 1)) == 0) {
		drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
		drv->error = 0;
		setintr(drv);
		//if(ideio.bios == IDETC_BIOS && ideio.wwait > 0){
		//	drv->status &= ~IDESTAT_DRQ;
		//	drv->status |= IDESTAT_BSY;
		//	setdintr(drv, 0, 0, ideio.wwait);
		//}else{
		//	setintr(drv);
		//}
	}else{
		drv->status &= ~IDESTAT_BSY;
	}
	return;

write_err:
	cmdabort(drv);
}


// ----

static void IOOUTCALL ideio_o430(UINT port, REG8 dat) {

	TRACEOUT(("ideio setbank%d %.2x [%.4x:%.8x]",
									(port >> 1) & 1, dat, CPU_CS, CPU_EIP));
	if (!(dat & 0x80)) {
		//char buf[100] = {0};
		ideio.bank[(port >> 1) & 1] = dat & 0x71;
		//sprintf(buf, "0x%x¥n", dat);
		//OutputDebugStringA(buf);
	}
}

static REG8 IOINPCALL ideio_i430(UINT port) {

	UINT	bank;
	REG8	ret;

	bank = (port >> 1) & 1;
	ret = ideio.bank[bank];
	if ((port >> 1) & 1) {
		// 432h
	}
	else {
		// 430h
		IDEDEV	dev;
		dev = getidedev();
		//
		// Win2000はbit6が1の時スレーブデバイスを見に行く
		//
		if (dev->drv[1].device != IDETYPE_NONE) {
			ret |= 0x40;
		}
	}
	ideio.bank[bank] = ret & (~0x80);
	TRACEOUT(("ideio getbank%d %.2x [%.4x:%.8x]",
									(port >> 1) & 1, ret, CPU_CS, CPU_EIP));
	return(ret & 0x7f);
}



// ----

static void IOOUTCALL ideio_o433(UINT port, REG8 dat) {

}

static REG8 IOINPCALL ideio_i433(UINT port) {
	
	UINT	bank;
	REG8	ret;
	
	bank = (port >> 1) & 1;
	ret = (ideio.bank[bank] & 0x1) ? 0x2 : 0x0;

	if(ret == 0x2 && ideio.dev[1].drv[0].device==IDETYPE_NONE && ideio.dev[1].drv[1].device==IDETYPE_NONE){
		ret = 0;
	}
	//OutputDebugStringA("IN 433h¥n");
	return(ret);
}

static void IOOUTCALL ideio_o435(UINT port, REG8 dat) {

}

static REG8 IOINPCALL ideio_i435(UINT port) {

	return(0x00);
}



// ----

static void IOOUTCALL ideio_o642(UINT port, REG8 dat) {

	IDEDRV	drv;

	drv = getidedrv();
	if (drv) {
		drv->wp = dat;
		TRACEOUT(("ideio set WP %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o644(UINT port, REG8 dat) {

	IDEDRV	drv;

	(void)port;
	drv = getidedrv();
	if (drv) {
		drv->sc = dat;
		TRACEOUT(("ideio set SC %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o646(UINT port, REG8 dat) {

	IDEDRV	drv;

	drv = getidedrv();
	if (drv) {
		drv->sn = dat;
		TRACEOUT(("ideio set SN %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o648(UINT port, REG8 dat) {

	IDEDRV	drv;

	drv = getidedrv();
	if (drv) {
		drv->cy &= 0xff00;
		drv->cy |= dat;
		TRACEOUT(("ideio set CYL %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o64a(UINT port, REG8 dat) {

	IDEDRV	drv;

	drv = getidedrv();
	if (drv) {
		drv->cy &= 0x00ff;
		drv->cy |= dat << 8;
		TRACEOUT(("ideio set CYH %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o64c(UINT port, REG8 dat) {

	IDEDEV	dev;
	UINT	drvnum;

	dev = getidedev();
	if (dev == NULL) {
		return;
	}
#if defined(TRACE)
	if ((dat & 0xf0) != 0xa0) {
		TRACEOUT(("ideio set SDH illegal param? (%.2x)", dat));
	}
#endif
	drvnum = (dat >> 4) & 1;
	if(dev->drivesel != drvnum){
		//dev->drv[drvnum].status = dev->drv[drvnum].status & ~(IDESTAT_DRQ|IDESTAT_BSY);
		//drvreset(&(dev->drv[drvnum]));
		//dev->drv[drvnum].status = IDESTAT_DRDY | IDESTAT_DSC;
		//dev->drv[drvnum].error = IDEERR_AMNF;
		//if(!drvnum) dev->drv[drvnum].error |= IDEERR_BBK;
	}
	dev->drv[drvnum].dr = dat & 0xf0;
	dev->drv[drvnum].hd = dat & 0x0f;
	dev->drivesel = drvnum;
	TRACEOUT(("ideio set DRHD %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));

	(void)port;
}

static void IOOUTCALL ideio_o64e(UINT port, REG8 dat) {

	IDEDRV	drv, d;
	IDEDEV	dev;
	int		i;

	// execute device diagnostic
	if (dat == 0x90) {
		TRACEOUT(("ideio set cmd %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
		TRACEOUT(("ideio: execute device diagnostic"));
		dev = getidedev();
		if (dev) {
			for (i = 0; i < 2; i++) {
				d = dev->drv + i;
				drvreset(d);
				d->error = 0x01;
				if (dev->drv[i].device == IDETYPE_NONE) {
					d->error = 0x00;
				}
				if (i == 0) {
					if (dev->drv[1].device == IDETYPE_NONE) {
						d->error |= 0x80;
					}
				}
			}
		}
		return;
	}

	drv = getidedrv();
	if (drv == NULL) {
		return;
	}
	drv->cmd = dat;
	TRACEOUT(("ideio set cmd %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	switch(dat) {
		case 0x00:		// NOP
			break;
		case 0x08:		// device reset
			TRACEOUT(("ideio: device reset"));
			drvreset(drv);
			drv->error = 0x01;
			dev = getidedev();
			if (dev) {
				if (dev->drv[dev->drivesel].device == IDETYPE_NONE) {
					drv->error = 0x00;
				}
				if (dev->drivesel == 0) {
					if (dev->drv[1].device == IDETYPE_NONE) {
						drv->error |= 0x80;
					}
				}
			}
			setintr(drv);
			break;

		case 0x10: case 0x11: case 0x12: case 0x13:	// recalibrate
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			TRACEOUT(("ideio: recalibrate"));
			if (drv->device == IDETYPE_HDD) {
				//drv->hd = 0x00;
				//drv->sc = 0x00;
				drv->cy = 0x0000;
				//if (!(drv->dr & IDEDEV_LBA)) {
				//	drv->sn = 0x01;
				//}
				//else {
				//	drv->sn = 0x00;
				//}
				drv->status = IDESTAT_DRDY | IDESTAT_DSC;
				drv->error = 0;
				setintr(drv);
			}
			else {
				cmdabort(drv);
			}
			break;

		case 0x20:		// read (with retry)
		case 0x21:		// read
			TRACEOUT(("ideio: read sector"));
			if (drv->device == IDETYPE_HDD) {
				drv->mulcnt = 0;
				drv->multhr = 1;
				readsec(drv);
			}
			else {
				cmdabort(drv);
			}
			break;

		case 0x30:		// write (with retry)
		case 0x31:		// write
			TRACEOUT(("ideio: write sector"));
			if (drv->device == IDETYPE_HDD) {
				drv->mulcnt = 0;
				drv->multhr = 1;
				writeinit(drv);
			}
			else {
				cmdabort(drv);
			}
			break;
		case 0x40:		// read verify(w)
		case 0x41:		// read verify(w/o)
			drv->status = drv->status & ~IDESTAT_BSY;
			setintr(drv);
			break;
		case 0x91:		// INITIALIZE DEVICE PARAMETERS
			TRACEOUT(("ideio: INITIALIZE DEVICE PARAMETERS dh=%x sec=%x",
											drv->dr | drv->hd, drv->sc));
			if (drv->device == IDETYPE_HDD) {
				drv->surfaces = drv->hd + 1;
				drv->sectors = drv->sc;
				drv->status &= ~(IDESTAT_BSY | IDESTAT_DRQ | IDESTAT_ERR | 0x20);
				drv->status |= IDESTAT_DRDY;
				setintr(drv);
			}
			else {
				cmdabort(drv);
			}
			break;

		case 0xa0:		// send packet
			TRACEOUT(("ideio: packet"));
			if (drv->device == IDETYPE_CDROM) {
				drv->sc &= ~(IDEINTR_REL | IDEINTR_IO);
				drv->sc |= IDEINTR_CD;
				drv->status &= ~(IDESTAT_BSY | IDESTAT_DMRD | IDESTAT_SERV | IDESTAT_CHK);
				drv->status |= IDESTAT_DRDY | IDESTAT_DRQ | IDESTAT_DSC;
				drv->error = 0;
				drv->bufpos = 0;
				drv->bufsize = 12;
				drv->bufdir = IDEDIR_OUT;
				drv->buftc = IDETC_TRANSFEREND;
				break;
			}
			cmdabort(drv);
			break;

		case 0xa1:		// identify packet device
			TRACEOUT(("ideio: identify packet device"));
			if (drv->device == IDETYPE_CDROM && setidentify(drv) == SUCCESS) {
				drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
				drv->error = 0;
				setintr(drv);
			}
			else {
				cmdabort(drv);
			}
			break;
			
		case 0xb0:		// SMART
			cmdabort(drv);
			break;

		case 0xc4:		// read multiple
			TRACEOUT(("ideio: read multiple"));
#if IDEIO_MULTIPLE_MAX > 0
			if (drv->device == IDETYPE_HDD) {
				drv->mulcnt = 0;
				drv->multhr = drv->mulmode;
				readsec(drv);
				break;
			}
#endif
			cmdabort(drv);
			break;

		case 0xc5:		// write multiple
			TRACEOUT(("ideio: write multiple"));
#if IDEIO_MULTIPLE_MAX > 0
			if (drv->device == IDETYPE_HDD) {
				drv->mulcnt = 0;
				drv->multhr = drv->mulmode;
				writesec(drv);
				break;
			}
#endif
			cmdabort(drv);
			break;

		case 0xc6:		// set multiple mode
			TRACEOUT(("ideio: set multiple mode"));
			if (drv->device == IDETYPE_HDD) {
				switch(drv->sc) {
#if IDEIO_MULTIPLE_MAX > 0
				case 2: case 4: case 8: case 16: case 32: case 64: case 128:
					if (drv->sc <= IDEIO_MULTIPLE_MAX) {
						drv->mulmode = drv->sc;
						setintr(drv);
						break;
					}
					/*FALLTHROUGH*/
#endif
				default:
					cmdabort(drv);
					break;
				}
			}
			else {
				cmdabort(drv);
			}
			break;

		case 0xe0:		// STANDBY IMMEDIATE
			TRACEOUT(("ideio: STANDBY IMMEDIATE dr = %.2x", drv->dr));
			//cmdabort(drv);
			break;

		case 0xe1:		// idle immediate
			TRACEOUT(("ideio: idle immediate dr = %.2x", drv->dr));
			//必ず成功するはず
			if(drv->status & IDESTAT_DRDY){
				drv->status = IDESTAT_DRDY | IDESTAT_DSC;
				drv->error = 0;
				setintr(drv);
			}else{
				cmdabort(drv);
			}
			break;
			
		case 0xe5:		// Check power mode
			drv->sc = 0xff;
			drv->status = drv->status & ~IDESTAT_BSY;
			setintr(drv);
			break;

		case 0xe7:		// flush cache
			TRACEOUT(("ideio: flush cache"));
			drv->status = IDESTAT_DRDY;
			drv->error = 0;
			setintr(drv);
			break;

		case 0xec:		// identify device
			TRACEOUT(("ideio: identify device"));
			if (drv->device == IDETYPE_HDD && setidentify(drv) == SUCCESS) {
				drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
				drv->error = 0;
				setintr(drv);
			}
			else if (drv->device == IDETYPE_CDROM) {
				drvreset(drv);
				cmdabort(drv);
			}
			else{
				cmdabort(drv);
			}
			break;

		case 0xef:		// set features
			TRACEOUT(("ideio: set features reg = %.2x", drv->wp));
			//if(drv->device == IDETYPE_CDROM){
			//	switch(drv->wp) {
			//	case 0x95: // Enable Media Status Notification
			//		ideio_mediastatusnotification[drv->sxsidrv] = 1;
			//		drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
			//		drv->error = 0;
			//		break;
			//	case 0x31: // Disable Media Status Notification
			//		ideio_mediastatusnotification[drv->sxsidrv] = 0;
			//		drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
			//		drv->error = 0;
			//		break;
			//	default:
			//		cmdabort(drv);
			//	}
			//}else{
				cmdabort(drv);
			//}
			break;
			
		case 0xda:		// GET MEDIA STATUS
			TRACEOUT(("ideio: GET MEDIA STATUS dev=%d", drv->device));
			//if (ideio_mediastatusnotification[drv->sxsidrv]) {
			//	SXSIDEV sxsi;
			//	drv->status = IDESTAT_DRDY | IDESTAT_ERR;
			//	drv->error = 0;
			//	sxsi = sxsi_getptr(drv->sxsidrv);
			//	if ((sxsi == NULL) || !(sxsi->flag & SXSIFLAG_READY)) {
			//		drv->error |= 0x02; // No media
			//	}else if(ideio_mediachangeflag[drv->sxsidrv]){
			//		drv->error |= IDEERR_MCNG;
			//		ideio_mediachangeflag[drv->sxsidrv] = 0;
			//	}
			//	setintr(drv);
			//}
			//else {
				cmdabort(drv);
			//}
			break;

		case 0xde:		// media lock
			TRACEOUT(("ideio: media lock dev=%d", drv->device));
			//cmdabort(drv);
			break;

		case 0xdf:		// media unlock
			TRACEOUT(("ideio: media unlock dev=%d", drv->device));
			//cmdabort(drv);
			break;

		case 0xf8:		// READ NATIVE MAX ADDRESS
			TRACEOUT(("ideio: READ NATIVE MAX ADDRESS reg = %.2x", drv->wp));
			cmdabort(drv);
			break;
			
		default:
			panic("ideio: unknown command %.2x", dat);
			break;
	}
	(void)port;
}

static void IOOUTCALL ideio_o74c(UINT port, REG8 dat) {

	IDEDEV	dev;
	REG8	modify;

	dev = getidedev();
	if (dev == NULL) {
		return;
	}
	modify = dev->drv[0].ctrl ^ dat;
	dev->drv[0].ctrl = dat;
	dev->drv[1].ctrl = dat;
	if (modify & IDECTRL_SRST) {
		if (dat & IDECTRL_SRST) {
			dev->drv[0].status = 0;
			dev->drv[0].error = 0;
			dev->drv[1].status = 0;
			dev->drv[1].error = 0;
		}
		else {
			drvreset(&dev->drv[0]);
			if (dev->drv[0].device == IDETYPE_HDD) {
				dev->drv[0].status = IDESTAT_DRDY | IDESTAT_DSC;
				dev->drv[0].error = IDEERR_AMNF;
			}
			if (dev->drv[0].device == IDETYPE_CDROM) {
				dev->drv[0].status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_ERR;
				dev->drv[0].error = IDEERR_AMNF;
			}
			drvreset(&dev->drv[1]);
			if (dev->drv[1].device == IDETYPE_HDD) {
				dev->drv[1].status = IDESTAT_DRDY | IDESTAT_DSC;
				dev->drv[1].error = IDEERR_AMNF;
			}
			if (dev->drv[1].device == IDETYPE_CDROM) {
				dev->drv[1].status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_ERR;
				dev->drv[1].error = IDEERR_AMNF;
			}
		}
	}
	TRACEOUT(("ideio interrupt %sable", (dat & IDECTRL_NIEN) ? "dis" : "en"));
	TRACEOUT(("ideio devctrl %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	(void)port;
}

static void IOOUTCALL ideio_o74e(UINT port, REG8 dat) {
	
	TRACEOUT(("ideio %.4x,%.2x [%.4x:%.8x]", port, dat, CPU_CS, CPU_EIP));
	(void)port;
	(void)dat;
}


// ----

static REG8 IOINPCALL ideio_i642(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		drv->status &= ~IDESTAT_ERR;
		TRACEOUT(("ideio get error %.2x [%.4x:%.8x]",
												drv->error, CPU_CS, CPU_EIP));
		return(drv->error);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i644(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		UINT8 ret = drv->sc;
		TRACEOUT(("ideio get SC %.2x [%.4x:%.8x]", drv->sc, CPU_CS, CPU_EIP));
		//if (drv->device == IDETYPE_CDROM && drv->cmd == 0xa0) {
		//	drv->sc = 7; // ????
		//}
		return(ret);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i646(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio get SN %.2x [%.4x:%.8x]", drv->sn, CPU_CS, CPU_EIP));
		return(drv->sn);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i648(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio get CYL %.4x [%.4x:%.8x]", drv->cy, CPU_CS, CPU_EIP));
		return((UINT8)drv->cy);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i64a(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio get CYH %.4x [%.4x:%.8x]", drv->cy, CPU_CS, CPU_EIP));
		return((REG8)(drv->cy >> 8));
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i64c(UINT port) {

	IDEDRV	drv;
	REG8	ret;

	(void)port;

	drv = getidedrv();
	if (drv) {
		ret = drv->dr | drv->hd;
		TRACEOUT(("ideio get DRHD %.2x [%.4x:%.8x]", ret, CPU_CS, CPU_EIP));
		return(ret);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i64e(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio status %.2x [%.4x:%.8x]",
											drv->status, CPU_CS, CPU_EIP));
		if (!(drv->ctrl & IDECTRL_NIEN)) {
			TRACEOUT(("ideio: resetirq"));
			pic_resetirq(IDE_IRQ);
			//mem[MEMB_DISK_INTH] &= ~0x01; 
		}
		return(drv->status);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i74c(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio alt status %.2x [%.4x:%.8x]",
											drv->status, CPU_CS, CPU_EIP));
		return(drv->status);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i74e(UINT port) {
	
	IDEDEV	dev;
	IDEDRV	drv;
	REG8 ret;

	dev = getidedev();
	drv = getidedrv();
	ret = 0xc0;
	ret |= (~(drv->hd) & 0x0f) << 2;
	if (dev->drivesel == 0) {
		ret |= 2; /* master */
	} else {
		ret |= 1; /* slave */
	}
	return ret;
}

static REG8 IOINPCALL ideio_i1e8e(UINT port) {
    if((CPU_RAM_D000 & 0x1c00) == 0x1c00){
        return(0x81);
    }
    return(0x80);
}
 
static void IOOUTCALL ideio_o1e8e(UINT port, REG8 dat) {
    switch(dat){
        case 0x00:
        case 0x80:
            TRACEOUT(("remove RAM on 0xDA000-DBFFF but ignore"));
            //CPU_RAM_D000 |= 0x0000;
            break;
        case 0x81:
            TRACEOUT(("connect RAM on 0xDA000-DBFFF"));
            CPU_RAM_D000 |= 0x1c00;
            break;
        default:
            break;
    }
}


// ---- data

void IOOUTCALL ideio_w16(UINT port, REG16 value) {

	IDEDEV  dev;
	IDEDRV	drv;
	UINT8	*p;
	FILEPOS	sec;

	dev = getidedev();
	drv = getidedrv();
	if ((drv != NULL) &&
		(drv->status & IDESTAT_DRQ) && (drv->bufdir == IDEDIR_OUT)) {
		p = drv->buf + drv->bufpos;
		p[0] = (UINT8)value;
		p[1] = (UINT8)(value >> 8);
		//TRACEOUT(("ide-data send %.4x (%.4x) [%.4x:%.8x]",
		//								value, drv->bufpos, CPU_CS, CPU_EIP));
		drv->bufpos += 2;
		if (drv->bufpos >= drv->bufsize) {
			drv->status &= ~IDESTAT_DRQ;
			switch(drv->cmd) {
				case 0x30:
				case 0x31:
				case 0xc5:
					drv->status |= IDESTAT_BSY;
					sec = getcursec(drv);
					//TRACEOUT(("writesec->drv %d sec %x cnt %d thr %d",
					//			drv->sxsidrv, sec, drv->mulcnt, drv->multhr));
					if (sxsi_write(drv->sxsidrv, sec, drv->buf, drv->bufsize)) {
						TRACEOUT(("write error!"));
						cmdabort(drv);
						break;
					}
					drv->mulcnt++;
					incsec(drv);
					drv->sc--;
					if (drv->sc) {
						writesec(drv);
					}else{
						// 1セクタ書き込み完了
						if(ideio.bios == IDETC_BIOS && ideio.wwait > 0){
							setdintr2(drv, 0, 0, ideio.wwait);
						}else{
							setintr(drv);
							drv->status &= ~(IDESTAT_BSY);
						}
					}
					break;

				case 0xa0:
					TRACEOUT(("ideio: execute atapi packet command"));
					atapicmd_a0(drv);
					break;
			}
		}
	}
	(void)port;
}

REG16 IOINPCALL ideio_r16(UINT port) {

	IDEDRV	drv;
	REG16	ret;
	UINT8	*p;

	(void)port;

	drv = getidedrv();
	if (drv == NULL) {
		return(0xff);
	}
	ret = 0;
	if ((drv->status & IDESTAT_DRQ) && (drv->bufdir == IDEDIR_IN)) {
		p = drv->buf + drv->bufpos;
		ret = p[0] + (p[1] << 8);
		//TRACEOUT(("ide-data recv %.4x (%.4x) [%.4x:%.8x]",
		//								ret, drv->bufpos, CPU_CS, CPU_EIP));
		drv->bufpos += 2;
		if (drv->bufpos >= drv->bufsize) {
			drv->status &= ~IDESTAT_DRQ;
			switch(drv->cmd) {
				case 0x20:
				case 0x21:
				case 0xc4:
					incsec(drv);
					drv->sc--;
					if (drv->sc) {
						readsec(drv);
					}else{
						// 読み取り終わり
					}
					break;

				case 0xa0:
					if (drv->buftc == IDETC_ATAPIREAD) {
						atapi_dataread(drv);
						break;
					}
					drv->sc = IDEINTR_IO | IDEINTR_CD;
					drv->status &= ~(IDESTAT_BSY | IDESTAT_SERV | IDESTAT_CHK | IDESTAT_DRQ); // clear DRQ bit np21w ver0.86 rev38
					drv->status |= IDESTAT_DRDY | IDESTAT_DSC;
					drv->error = 0;
					setintr(drv);
					break;
			}
		}
	}
	return(ret);
}


// ----

#if 1
static SINT32 cdda_softvolume_L = 0;
static SINT32 cdda_softvolume_R = 0;
static SINT32 cdda_softvolumereg_L = 0xff;
static SINT32 cdda_softvolumereg_R = 0xff;
static BRESULT SOUNDCALL playdevaudio(IDEDRV drv, SINT32 *pcm, UINT count) {

	SXSIDEV	sxsi;
	UINT	r;
const UINT8	*ptr;
	SINT	sampl;
	SINT	sampr;
	SINT32	buf_l;
	SINT32	buf_r;
	SINT32	buf_count;
	SINT32	samplen_n;
	SINT32	samplen_d;
static SINT32	sampcount2_n = 0;
	SINT32	sampcount2_d;
	UINT	mute = 0;

	samplen_n = soundcfg.rate;
	samplen_d = 44100;
	//if(samplen_n > samplen_d){
	//	// XXX: サンプリングレートが大きい場合のオーバーフロー対策･･･
	//	samplen_n /= 100;
	//	samplen_d /= 100;
	//}
	//if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_MATE_X_PCM || g_nSoundID == SOUNDID_PC_9801_86_WSS || g_nSoundID == SOUNDID_WAVESTAR || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
	//	if(cdda_softvolumereg_L != cs4231.devvolume[0x32]){
	//		cdda_softvolumereg_L = cs4231.devvolume[0x32];
	//		if(cdda_softvolumereg_L & 0x80){ // CD L Mute
	//			cdda_softvolume_L = 0;
	//		}else{
	//			cdda_softvolume_L = ((‾cdda_softvolumereg_L) & 0x1f); // CD L Volume
	//		}
	//	}
	//	if(cdda_softvolumereg_R != cs4231.devvolume[0x33]){
	//		cdda_softvolumereg_R = cs4231.devvolume[0x33];
	//		if(cdda_softvolumereg_R & 0x80){ // CD R Mute
	//			cdda_softvolume_R = 0;
	//		}else{
	//			cdda_softvolume_R = ((‾cdda_softvolumereg_R) & 0x1f); // CD R Volume
	//		}
	//	}
	//}else{
		cdda_softvolume_L = 0x1f;
		cdda_softvolume_R = 0x1f;
		cdda_softvolumereg_L = 0;
		cdda_softvolumereg_R = 0;
	//}

	sxsi = sxsi_getptr(drv->sxsidrv);
	if ((sxsi == NULL) || (sxsi->devtype != SXSIDEV_CDROM) ||
		(!(sxsi->flag & SXSIFLAG_READY))) {
		drv->daflag = 0x14;
		return(FAILURE);
	}
	while(count) {
		r = MIN(count, drv->dabufrem * samplen_n / samplen_d);
		if (r) {
			count -= r;
			ptr = drv->dabuf + 2352 - (drv->dabufrem * 4);
			drv->dabufrem -= r * samplen_d / samplen_n;
			if(samplen_n < samplen_d){
				//sampcount2_n = 0;
				sampcount2_d = samplen_d;
				buf_l = buf_r = buf_count = 0;
				do {
					sampl = ((SINT8)ptr[1] << 8) + ptr[0];
					sampr = ((SINT8)ptr[3] << 8) + ptr[2];
					ptr += 4;
					sampcount2_n += samplen_n;
					buf_l += (SINT)((int)(sampl)*np2cfg.davolume*cdda_softvolume_L/255/31);
					buf_r += (SINT)((int)(sampr)*np2cfg.davolume*cdda_softvolume_R/255/31);
					buf_count++;
					if(sampcount2_n > sampcount2_d){
						pcm[0] += buf_l / buf_count;
						pcm[1] += buf_r / buf_count;
						//pcm[0] += (SINT)((int)(sampl)*np2cfg.davolume/255);
						//pcm[1] += (SINT)((int)(sampr)*np2cfg.davolume/255);
						pcm += 2 * (sampcount2_n / sampcount2_d);
						--r;
						sampcount2_n = sampcount2_n % sampcount2_d;
						buf_l = buf_r = buf_count = 0;
					}
				} while(r > 0);
			}else{
				sampcount2_n = samplen_n;
				sampcount2_d = samplen_d;
				do {
					sampl = ((SINT8)ptr[1] << 8) + ptr[0];
					sampr = ((SINT8)ptr[3] << 8) + ptr[2];
					pcm[0] += (SINT)((int)(sampl)*np2cfg.davolume*cdda_softvolume_L/255/31);
					pcm[1] += (SINT)((int)(sampr)*np2cfg.davolume*cdda_softvolume_L/255/31);
					sampcount2_n -= sampcount2_d;
					if(sampcount2_n <= 0){
						sampcount2_n += samplen_n;
						ptr += 4;
					}
					pcm += 2;
				} while(--r);
			}
		}
		if (count == 0) {
			break;
		}
		if (drv->dalength == 0) {
			drv->daflag = 0x13;
			return(FAILURE);
		}
		if(np2cfg.cddtskip){
			CDTRK	trk;
			UINT	tracks;
			trk = sxsicd_gettrk(sxsi, &tracks);
			r = tracks;
			while(r) {
				r--;
				if (trk[r].pos <= drv->dacurpos) {
					break;
				}
			}
			if(trk[r].adr_ctl!=TRACKTYPE_AUDIO){
				if(drv->dacurpos != 0){
					// オーディオトラックでない場合は強制的に次に飛ばす
					if(r==tracks-1){
						drv->dacurpos = trk[0].pos;
					}else{
						drv->dacurpos = trk[r+1].pos;
					}
				}
				mute = 1;
			}
		}
		if(mute){
			memset(drv->dabuf, 0, sizeof(drv->dabuf));
		}else{
			if (sxsicd_readraw(sxsi, drv->dacurpos, drv->dabuf) != SUCCESS) {
				drv->daflag = 0x14;
				return(FAILURE);
			}
		}
		drv->dalength--;
		drv->dacurpos++;
		drv->dabufrem = sizeof(drv->dabuf) / 4;
	}
	return(SUCCESS);
}

static void SOUNDCALL playaudio(void *hdl, SINT32 *pcm, UINT count) {

	UINT	bit;
	IDEDRV	drv;

	bit = ideio.daplaying;
	if (!bit) {
		return;
	}
	if (bit & 4) {
		drv = ideio.dev[1].drv + 0;
		if (playdevaudio(drv, pcm, count) != SUCCESS) {
			bit = bit & (~4);
		}
	}
	ideio.daplaying = bit;
}
#endif

// ----

static void devinit(IDEDRV drv, REG8 sxsidrv) {

	SXSIDEV	sxsi;

	ZeroMemory(drv, sizeof(_IDEDRV));
	drv->sxsidrv = sxsidrv;
	//drv->dr = 0xa0;
	sxsi = sxsi_getptr(sxsidrv);
	if ((sxsi != NULL) && (np2cfg.idetype[sxsidrv] == SXSIDEV_HDD) && 
			(sxsi->devtype == SXSIDEV_HDD) && (sxsi->flag & SXSIFLAG_READY)) {
		drv->status = IDESTAT_DRDY | IDESTAT_DSC;
		drv->error = IDEERR_AMNF;
		drv->device = IDETYPE_HDD;
		drv->surfaces = sxsi->surfaces;
		drv->sectors = sxsi->sectors;
		drv->mulmode = IDEIO_MULTIPLE_MAX;
	}
	else if ((sxsi != NULL) && (np2cfg.idetype[sxsidrv] == SXSIDEV_CDROM) && 
			(sxsi->devtype == SXSIDEV_CDROM)) {
		drv->device = IDETYPE_CDROM;
		drvreset(drv);
		drv->error = 0;
		drv->media = IDEIO_MEDIA_EJECTABLE;
		if (sxsi->flag & SXSIFLAG_READY) {
			drv->media |= (IDEIO_MEDIA_CHANGED|IDEIO_MEDIA_LOADED);
		}
		drv->daflag = 0x15;
		drv->damsfbcd = 0;
	}
	else {
		drv->status = IDESTAT_ERR;
		drv->error = IDEERR_TR0;
		drv->device = IDETYPE_NONE;
	}
	//memset(ideio_mediastatusnotification, 0, sizeof(ideio_mediastatusnotification));
	//memset(ideio_mediachangeflag, 0, sizeof(ideio_mediachangeflag));
}

void ideio_initialize(void) {
	atapi_initialize();
}

void ideio_deinitialize(void) {
	atapi_deinitialize();
}

void ideio_basereset() {
	REG8	i;
	IDEDRV	drv;

	for (i=0; i<4; i++) {
		drv = ideio.dev[i >> 1].drv + (i & 1);
		devinit(drv, i);
	}
}
void ideio_reset(const NP2CFG *pConfig) {
	REG8	i;
	
	OEMCHAR	path[MAX_PATH];
	FILEH	fh;
	OEMCHAR tmpbiosname[16];
	UINT8 useidebios;
	UINT32 biosaddr;

	ZeroMemory(&ideio, sizeof(ideio));

	ideio_basereset();
	
	ideio.rwait = np2cfg.iderwait;
	ideio.wwait = np2cfg.idewwait;
	ideio.bios = IDETC_NOBIOS;

	if(pccore.hddif & PCHDD_IDE){
		if(pConfig->idebaddr){
			biosaddr = (UINT32)pConfig->idebaddr << 12;
			useidebios = np2cfg.idebios && np2cfg.usebios;
			if(useidebios && np2cfg.autoidebios){
				SXSIDEV	sxsi;
				for (i=0; i<4; i++) {
					sxsi = sxsi_getptr(i);
					if ((sxsi != NULL) && (np2cfg.idetype[i] == SXSIDEV_HDD) && 
							(sxsi->devtype == SXSIDEV_HDD) && (sxsi->flag & SXSIFLAG_READY)) {
						if(sxsi->surfaces != 8 || sxsi->sectors != 17){
							TRACEOUT(("Incompatible CHS parameter detected. IDE BIOS automatically disabled."));
							useidebios = 0;
						}
					}
				}
			}
			if(useidebios){
				_tcscpy(tmpbiosname, OEMTEXT("ide.rom"));
				getbiospath(path, tmpbiosname, NELEMENTS(path));
				fh = file_open_rb(path);
				if (fh == FILEH_INVALID) {
					_tcscpy(tmpbiosname, OEMTEXT("d8000.rom"));
					getbiospath(path, tmpbiosname, NELEMENTS(path));
					fh = file_open_rb(path);
				}
				if (fh == FILEH_INVALID) {
					_tcscpy(tmpbiosname, OEMTEXT("bank3.bin"));
					getbiospath(path, tmpbiosname, NELEMENTS(path));
					fh = file_open_rb(path);
				}
				if (fh == FILEH_INVALID) {
					_tcscpy(tmpbiosname, OEMTEXT("bios9821.rom"));
					getbiospath(path, tmpbiosname, NELEMENTS(path));
					fh = file_open_rb(path);
				}
			}else{
				fh = FILEH_INVALID;
			}
			if (fh != FILEH_INVALID) {
				// IDE BIOS
				if (file_read(fh, mem + biosaddr, 0x2000) == 0x2000) {
					ideio.bios = IDETC_BIOS;
					TRACEOUT(("load ide.rom"));
					_tcscpy(ideio.biosname, tmpbiosname);
					CPU_RAM_D000 &= ~(0x3 << 8);
				}else{
					//CopyMemory(mem + biosaddr, idebios, sizeof(idebios));
					TRACEOUT(("use simulate ide.rom"));
				}
				file_close(fh);
			}else{
				//CopyMemory(mem + biosaddr, idebios, sizeof(idebios));
				TRACEOUT(("use simulate ide.rom"));
			}
		}
	}

	(void)pConfig;
}

void ideio_bindCDDA(void) {
	if (pccore.hddif & PCHDD_IDE) {
		sound_streamregist(NULL, (SOUNDCB)playaudio);
	}
}

void ideio_bind(void) {

	if (pccore.hddif & PCHDD_IDE) {
		ideio_bindCDDA();

		iocore_attachout(0x0430, ideio_o430);
		iocore_attachout(0x0432, ideio_o430);
		iocore_attachinp(0x0430, ideio_i430);
		iocore_attachinp(0x0432, ideio_i430);

		iocore_attachout(0x0642, ideio_o642);
		iocore_attachout(0x0644, ideio_o644);
		iocore_attachout(0x0646, ideio_o646);
		iocore_attachout(0x0648, ideio_o648);
		iocore_attachout(0x064a, ideio_o64a);
		iocore_attachout(0x064c, ideio_o64c);
		iocore_attachout(0x064e, ideio_o64e);
		iocore_attachinp(0x0642, ideio_i642);
		iocore_attachinp(0x0644, ideio_i644);
		iocore_attachinp(0x0646, ideio_i646);
		iocore_attachinp(0x0648, ideio_i648);
		iocore_attachinp(0x064a, ideio_i64a);
		iocore_attachinp(0x064c, ideio_i64c);
		iocore_attachinp(0x064e, ideio_i64e);

		iocore_attachout(0x074c, ideio_o74c);
		iocore_attachout(0x074e, ideio_o74e);
		iocore_attachinp(0x074c, ideio_i74c);
		iocore_attachinp(0x074e, ideio_i74e);

		iocore_attachout(0x1e8e, ideio_o1e8e); // 一部IDE BIOSはこれがないと起動時にフリーズしたりシリンダ数が0になる
		iocore_attachinp(0x1e8e, ideio_i1e8e); // 一部IDE BIOSはこれがないと起動時にフリーズしたりシリンダ数が0になる
		
		iocore_attachout(0x0433, ideio_o433);
		iocore_attachinp(0x0433, ideio_i433);
		iocore_attachout(0x0435, ideio_o435);
		iocore_attachinp(0x0435, ideio_i435);
	}
}

void ideio_notify(REG8 sxsidrv, UINT action) {

	SXSIDEV sxsi;
	IDEDRV	drv;
	REG8 i;

	sxsi = sxsi_getptr(sxsidrv);
	if ((sxsi == NULL)
	 || (!(sxsi->flag & SXSIFLAG_READY))
	 || (sxsi->devtype != SXSIDEV_CDROM)) {
		return;
	}

	for (i=0; i<4; i++) {
		drv = ideio.dev[i >> 1].drv + (i & 1);
		if ((drv != NULL) && (drv->sxsidrv == sxsidrv)) {
			goto do_notify;
		}
	}
	return;

do_notify:
	switch(action) {
		case 1:
			drv->media |= (IDEIO_MEDIA_CHANGED|IDEIO_MEDIA_LOADED);
			if (sxsi->mediatype & SXSIMEDIA_DATA)
				drv->media |= IDEIO_MEDIA_DATA;
			if (sxsi->mediatype & SXSIMEDIA_AUDIO)
				drv->media |= IDEIO_MEDIA_AUDIO;
			break;

		case 0:
			drv->media &= ~(IDEIO_MEDIA_LOADED|IDEIO_MEDIA_COMBINE);
			break;
	}
}

void ideio_mediachange(REG8 sxsidrv) {
	
	//SXSIDEV sxsi;
	//IDEDRV	drv;
	//REG8 i;
	//
	//sxsi = sxsi_getptr(sxsidrv);
	//if ((sxsi == NULL)
	// || (!(sxsi->flag & SXSIFLAG_READY))
	// || (sxsi->devtype != SXSIDEV_CDROM)) {
	//	return;
	//}

	//for (i=0; i<4; i++) {
	//	drv = ideio.dev[i >> 1].drv + (i & 1);
	//	if ((drv != NULL) && (drv->sxsidrv == sxsidrv)) {
	//		break;
	//	}
	//}
	//if(i==4) return;
	//
	//drv->status |= IDESTAT_ERR;
	//drv->error |= IDEERR_MCNG;
	//setintr(drv);
	////ideio_mediachangeflag[sxsidrv] = 1;
}

#endif	/* SUPPORT_IDEIO */

