//
// FDC μPD765A
//

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"diskimage/fddfile.h"
#if defined(SUPPORT_SWSEEKSND)
#include	"sound/pcmmix.h"
#include	"fdd_mtr.res"
#endif
#include	"soundmng.h"

#if 1
#undef	TRACEOUT
#define	TRACEOUT(s)	(void)(s)
#endif	/* 0 */

extern void fddmtrsnd_play(UINT num, BOOL play);

enum {
	FDC_DMACH2HD	= 2,
	FDC_DMACH2DD	= 3
};

static const UINT8 FDCCMD_TABLE[32] = {
						0, 0, 8, 2, 1, 8, 8, 1, 0, 8, 1, 0, 8, 5, 0, 2,
						0, 8, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0};


#define FDC_FORCEREADY (1)
#define	FDC_DELAYERROR7

#define FDC_INT_DELAY		5			/*!< Delay 100ms */

void fdc_intwait(NEVENTITEM item) {
	
	if (item->flag & NEVENT_SETEVENT) {
		fdc.intreq = TRUE;
		if (fdc.chgreg & 1) {
			pic_setirq(0x0b);
		}
		else {
			pic_setirq(0x0a);
		}
	}
}

void fdc_interrupt(void) {
	
	nevent_set(NEVENT_FDCINT, 512, fdc_intwait, NEVENT_ABSOLUTE);
}

static void fdc_interruptreset(void) {

	fdc.intreq = FALSE;
}

static BOOL fdc_isfdcinterrupt(void) {

	return(fdc.intreq);
}

REG8 DMACCALL fdc_dmafunc(REG8 func) {

//	TRACEOUT(("fdc_dmafunc = %d", func));
	switch(func) {
		case DMAEXT_START:
			return(1);

		case DMAEXT_END:				// TC
			fdc.tc = 1;
			break;
	}
	return(0);
}

static void fdc_dmaready(REG8 enable) {

	if (fdc.chgreg & 1) {
		dmac.dmach[FDC_DMACH2HD].ready = enable;
	}
	else {
		dmac.dmach[FDC_DMACH2DD].ready = enable;
	}
}


// ----------------------------------------------------------------------

void fdcsend_error7(void) {

	fdc.tc = 0;
	fdc.event = FDCEVENT_BUFSEND;
	fdc.bufp = 0;
	fdc.bufcnt = 7;
	fdc.buf[0] = (UINT8)(fdc.stat[fdc.us] >>  0);
	fdc.buf[1] = (UINT8)(fdc.stat[fdc.us] >>  8);
	fdc.buf[2] = (UINT8)(fdc.stat[fdc.us] >> 16);
	fdc.buf[3] = fdc.C;
	fdc.buf[4] = fdc.H;
	fdc.buf[5] = fdc.R;
	fdc.buf[6] = fdc.N;
	fdc.status &= 0x0f;
	fdc.status |= (1 << fdc.us);
	fdc.status |= FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;
	fdc.stat[fdc.us] = 0;										// ver0.29
	fdc_dmaready(0);
	dmac_check();
	fdc_interrupt();
}

void fdcsend_success7(void) {

	fdc.tc = 0;
	fdc.event = FDCEVENT_BUFSEND;
	fdc.bufp = 0;
	fdc.bufcnt = 7;
#ifdef SUPPORT_KAI_IMAGES
	if (fdd_fdcresult() == FALSE) {
		fdc.buf[0] = (fdc.hd << 2) | fdc.us;
		fdc.buf[1] = 0;
		fdc.buf[2] = 0;
	}
	else {
		//	FDイメージファイルがFDCリザルトコードを持っている場合は
		//	(fdc.statに設定しているはずなので)そちらを設定
		fdc.buf[0] = (fdc.hd << 2) | fdc.us | (UINT8)(fdc.stat[fdc.us] >>  0);
		fdc.buf[1] = (UINT8)(fdc.stat[fdc.us] >>  8);
		fdc.buf[2] = (UINT8)(fdc.stat[fdc.us] >> 16);
	}
#else
	fdc.buf[0] = (fdc.hd << 2) | fdc.us;
	fdc.buf[1] = 0;
	fdc.buf[2] = 0;
#endif
	fdc.buf[3] = fdc.C;
	fdc.buf[4] = fdc.H;
	fdc.buf[5] = fdc.R;
	fdc.buf[6] = fdc.N;
	fdc.status &= 0x0f;
	fdc.status |= (1 << fdc.us);
	fdc.status |= FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;
	fdc.stat[fdc.us] = 0;										// ver0.29
	fdc_dmaready(0);
	dmac_check();
	fdc_interrupt();
}

#if 0
// FDCのタイムアウト			まぁ本当はこんなんじゃダメだけど…	ver0.29
void fdctimeoutproc(NEVENTITEM item) {

	if (item->flag & NEVENT_SETEVENT) {
		fdc.stat[fdc.us] = FDCRLT_IC0 | FDCRLT_EN | (fdc.hd << 2) | fdc.us;
		fdcsend_error7();
	}
}

static void fdc_timeoutset(void) {

	nevent_setbyms(NEVENT_FDCTIMEOUT, 166, fdctimeoutproc, NEVENT_ABSOLUTE);
}
#endif



static BOOL FDC_DriveCheck(BOOL protectcheck) {

//	if (!fddfile[fdc.us].fname[0]) {
	if (!fdd_diskready(fdc.us)) {
		fdc.stat[fdc.us] = FDCRLT_IC0 | FDCRLT_NR | (fdc.hd << 2) | fdc.us;
		fdcsend_error7();
		return(FALSE);
	}
//	else if ((protectcheck) && (fddfile[fdc.us].protect)) {
	else if ((protectcheck) && fdd_diskprotect(fdc.us)) {
		fdc.stat[fdc.us] = FDCRLT_IC0 | FDCRLT_NW | (fdc.hd << 2) | fdc.us;
		fdcsend_error7();
		return(FALSE);
	}
	return(TRUE);
}

// ----------------------------------------------------------------------

static void get_mtmfsk(void) {

	fdc.mt = (fdc.cmd >> 7) & 1;
	fdc.mf = fdc.cmd & 0x40;							// ver0.29
	fdc.sk = (fdc.cmd >> 5) & 1;
}

static void get_hdus(void) {

	fdc.hd = (fdc.cmds[0] >> 2) & 1;
	fdc.us = fdc.cmds[0] & 3;
}

static void get_chrn(void) {

	fdc.C = fdc.cmds[1];
	fdc.H = fdc.cmds[2];
	fdc.R = fdc.cmds[3];
	fdc.N = fdc.cmds[4];
}

static void get_eotgsldtl(void) {

	fdc.eot = fdc.cmds[5];
	fdc.gpl = fdc.cmds[6];
	fdc.dtl = fdc.cmds[7];
}

// --------------------------------------------------------------------------

static void FDC_Invalid(void) {							// cmd: xx

	fdc.event = FDCEVENT_BUFSEND;
	fdc.bufcnt = 1;
	fdc.bufp = 0;
	fdc.buf[0] = 0x80;
	fdc.status &= 0x0f;
	fdc.status |= FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;
}

#ifdef SUPPORT_KAI_IMAGES
static void FDC_ReadDiagnostic(void) {					// cmd: 02

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			get_chrn();
			get_eotgsldtl();
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;

			if (FDC_DriveCheck(FALSE)) {
				fdc.event = FDCEVENT_BUFSEND;
//				fdc.bufcnt = makedianosedata();
				fdc.bufp = 0;
			}
			break;

		default:
			fdc.event = FDCEVENT_NEUTRAL;
			break;
	}
}
#endif

static void FDC_Specify(void) {							// cmd: 03

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			fdc.srt = fdc.cmds[0] >> 4;
			fdc.hut = fdc.cmds[0] & 0x0f;
			fdc.hlt = fdc.cmds[1] >> 1;
			fdc.nd = fdc.cmds[1] & 1;
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			break;
	}
	fdc.event = FDCEVENT_NEUTRAL;
	fdc.status &= 0x0f;
	fdc.status |= FDCSTAT_RQM;
}

static void FDC_SenseDeviceStatus(void) {				// cmd: 04

	switch (fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			fdc.buf[0] = (fdc.hd << 2) | fdc.us;
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			if (fdc.equip & (1 << fdc.us)) {
				fdc.buf[0] |= 0x08;
				if (!fdc.treg[fdc.us]) {
					fdc.buf[0] |= 0x10;
				}
				if (fdd_diskready(fdc.us) || (fdc.ctrlreg & 0x40)) {/* 170107 念のため */
					fdc.buf[0] |= 0x20;
				}
				if (fddfile[fdc.us].protect) {
					fdc.buf[0] |= 0x40;
				}
			}
			else {
				fdc.buf[0] |= 0x80;
			}
//			TRACEOUT(("FDC_SenseDeviceStatus %.2x", fdc.buf[0]));
			fdc.event = FDCEVENT_BUFSEND;
			fdc.bufcnt = 1;
			fdc.bufp = 0;
			fdc.status &= 0x0f;
			fdc.status |= (1 << fdc.us);
			fdc.status |= FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;
			break;

		default:
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status &= 0x0f;
			fdc.status |= FDCSTAT_RQM;
			break;
	}
}

static BRESULT writesector(void) {

	fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
	if (!FDC_DriveCheck(TRUE)) {
		return(FAILURE);
	}
	if (fdd_write()) {
		fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) | FDCRLT_IC0 | FDCRLT_ND;
		fdcsend_error7();
		return(FAILURE);
	}
	fdc.event = FDCEVENT_BUFRECV;
	fdc.bufcnt = 128 << fdc.N;
	fdc.bufp = 0;
//	fdc.status = FDCSTAT_RQM | FDCSTAT_NDM | FDCSTAT_CB;
#if defined(SUPPORT_SWSEEKSND)
	if(np2cfg.MOTOR) fddmtrsnd_play(1, TRUE);
#else
	if(np2cfg.MOTOR) soundmng_pcmplay(SOUND_PCMSEEK, FALSE);
#endif
	fdc_dmaready(1);
	dmac_check();
	return(SUCCESS);
}

static void FDC_WriteData(void) {						// cmd: 05
														// cmd: 09
	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			get_chrn();
			get_eotgsldtl();
			fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
			fdc.treg[fdc.us] = fdc.C;	/* 170101 ST modified to work on Windows 9x/2000 */
			if (FDC_DriveCheck(TRUE)) {
				fdc.event = FDCEVENT_BUFRECV;
				fdc.bufcnt = 128 << fdc.N;
				fdc.bufp = 0;
#if 1															// ver0.27 ??
				fdc.status &= 0x0f;
				fdc.status |= (1 << fdc.us);
				fdc.status |= FDCSTAT_RQM | FDCSTAT_CB;
				if (fdc.nd) {
					fdc.status |= FDCSTAT_NDM;
				}
#else
				fdc.status = FDCSTAT_RQM | FDCSTAT_NDM | FDCSTAT_CB;
#endif
				fdc_dmaready(1);
				dmac_check();
			}
			break;

		case FDCEVENT_BUFRECV:
			if (writesector()) {
				return;
			}
			/* 170101 ST modified to work on Windows 9x/2000 form ... */
			//if (fdc.tc) {
			//	fdcsend_success7();
			//	return;
			//}
			if ((fdc.tc) || (fdc.R++ == fdc.eot)) {
				fdc.R = 1;
				if (fdc.mt) {
					fdc.hd ^= 1;
					fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
					fdc.H ^= 1;
					if (fdc.tc || !fdc.H) {
						fdc.C++;
						fdcsend_success7();
						break;
					}
				}
				else {
					fdc.C++;
					fdcsend_success7();
				}
				//fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) |
				//									FDCRLT_IC0 | FDCRLT_EN;
				//fdcsend_error7();
				break;
			}
			/* 170101 ST modified to work on Windows 9x/2000 ... to */
			break;

		default:
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status &= 0x0f;
			fdc.status |= FDCSTAT_RQM;
			break;
	}
}

static void readsector(void) {

	fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
	if (!FDC_DriveCheck(FALSE)) {
		return;
	}
	if (fdd_read()) {
		fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) | FDCRLT_IC0 | FDCRLT_ND;
		fdcsend_error7();
		return;
	}
#if defined(SUPPORT_SWSEEKSND)
	if(np2cfg.MOTOR) fddmtrsnd_play(1, TRUE);
#else
	if(np2cfg.MOTOR) soundmng_pcmplay(SOUND_PCMSEEK, FALSE);
#endif

	fdc.event = FDCEVENT_BUFSEND2;
	fdc.bufp = 0;
#if 0															// ver0.27 ??
	fdc.status = FDCSTAT_NDM | FDCSTAT_CB;
	if (!(fdc.ctrlreg & 0x10)) {
		fdc.status |= FDCSTAT_RQM | FDCSTAT_DIO;
	}
#else
	fdc.status &= 0x0f;
	fdc.status |= (1 << fdc.us);
	fdc.status |= FDCSTAT_RQM | FDCSTAT_DIO | FDCSTAT_CB;
	if (fdc.nd) {
		fdc.status |= FDCSTAT_NDM;
	}
#endif
	fdc_dmaready(1);
	dmac_check();
}

static void FDC_ReadData(void) {						// cmd: 06
														// cmd: 0c
	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			get_chrn();
			get_eotgsldtl();
			fdc.treg[fdc.us] = fdc.C;	/* 170101 ST modified to work on Windows 9x/2000 */
			readsector();
			break;

		case FDCEVENT_NEXTDATA:
			fdc.bufcnt = 0;
			if (fdc.R++ == fdc.eot) {
				/* 170101 ST modified to work on Windows 9x/2000 form ... */
				fdc.R = 1;
				if (fdc.mt) {
					fdc.hd ^= 1;
					fdc.H ^= 1;
					if (fdc.H) {
						readsector();
						break;
					}
				}
				fdc.C++;
				fdcsend_success7();
				//fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) |
				//									FDCRLT_IC0 | FDCRLT_EN;
				//fdcsend_error7();
				break;
				/* 170101 ST modified to work on Windows 9x/2000 ... to */
			}
			readsector();
			break;

#ifdef FDC_DELAYERROR7
		case FDCEVENT_BUSY:
			break;
#endif

		default:
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status &= 0x0f;
			fdc.status |= FDCSTAT_RQM;
			break;
	}
}

static void FDC_Recalibrate(void) {						// cmd: 07

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
//			fdc.ncn = 0;
//			fdc.R = 1;						/* 170107 for Windows95 */
//			fdc.crcn = fdc.R;				/* 170107 for Windows95 */
			fdc.int_stat[fdc.us] = /* (fdc.hd << 2) | */ fdc.us;
			fdc.int_stat[fdc.us] |= FDCRLT_SE;
			if (!(fdc.equip & (1 << fdc.us))) {
				fdc.int_stat[fdc.us] |= FDCRLT_IC0;
				if (!(fdc.ctrlreg & 0x40)) {
					// 強制ReadyでなければNR
					fdc.int_stat[fdc.us] |= FDCRLT_NR;
				}
				else {
					// 強制ReadyならばNRは立たないがECが立つ(sector0が見つからないので)
					fdc.int_stat[fdc.us] |= FDCRLT_EC;
				}
			}
			else if (!fdd_diskready(fdc.us)) {
				// Win95, 98用
				fdc.ncn = 0;
				fdc.R = 1;						/* 170107 for Windows95 */
				fdc.crcn = fdc.R;				/* 170107 for Windows95 */
				if (fdc.ctrlreg & 0x40) {
					fdc.treg[fdc.us] = fdc.ncn;
				}else{
					fdc.int_stat[fdc.us] |= FDCRLT_NR;
				}
			}
			else {
				fdc.ncn = 0;
				fdc.treg[fdc.us] = fdc.ncn;
				fdc.R = 1;						/* 170107 for Windows95 */
				fdc.crcn = fdc.R;				/* 170107 for Windows95 */
				/* 170107 for Windows95 form ... */
//				if (fdd_seek()) {
//					fdc.stat[fdc.us] |= FDCRLT_IC0;
				if (fdd_diskready(fdc.us)) {
#if defined(SUPPORT_SWSEEKSND)
					if(np2cfg.MOTOR) fddmtrsnd_play(1, TRUE);
#else
					if(np2cfg.MOTOR) soundmng_pcmplay(SOUND_PCMSEEK1, FALSE);
#endif
				}
//				}
				/* 170107 for Windows95 ... to */
			}
			fdc.int_timer[fdc.us] = FDC_INT_DELAY;
			fdc.status &= 0x0f;
			fdc.status |= (1 << fdc.us);

//			fdc_interrupt();
			break;
//		default:
//		fdc_interrupt();
//		break;
	}
	fdc.event = FDCEVENT_NEUTRAL;
	fdc.status |= FDCSTAT_RQM;
	fdc.status &= ~FDCSTAT_CB;
}

static void FDC_SenceintStatus(void) {					// cmd: 08

	int		i;

	fdc.event = FDCEVENT_BUFSEND;
	fdc.bufp = 0;
	fdc.bufcnt = 0;
	fdc.status &= 0x0f;
	fdc.status |= FDCSTAT_RQM | FDCSTAT_CB | FDCSTAT_DIO;


//	if (fdc_isfdcinterrupt()) {
//		i = 0;
		if (fdc.stat[fdc.us]) {
			fdc.buf[0] = (UINT8)fdc.stat[fdc.us];
			fdc.buf[1] = fdc.treg[fdc.us];
			fdc.bufcnt = 2;
			fdc.stat[fdc.us] = 0;
//			TRACEOUT(("fdc stat - %d [%.2x]", fdc.us, fdc.buf[0]));
		}
		else {
			for (i=0; i<4; i++) {
				if (fdc.stat[i]) {
					fdc.buf[0] = (UINT8)fdc.stat[i];
					fdc.buf[1] = fdc.treg[i];
					fdc.bufcnt = 2;
					fdc.stat[i] = 0;
//					TRACEOUT(("fdc stat - %d [%.2x]", i, fdc.buf[0]));
					break;
				}
			}
		}
//		for (; i<4; i++) {
//			if (fdc.stat[i]) {
//				break;
//			}
//		}
//		if (i >= 4) {
//			fdc_interruptreset();
//		}
//
//	}
	if (!fdc.bufcnt) {
		fdc.buf[0] = FDCRLT_IC1;
		fdc.bufcnt = 1;
	}
	/* 170101 ST modified to work on Windows 9x/2000 form ... */
	else if ((fdc.ctrlreg & 0x08) == 0) {
		//fdc.buf[0] |= 0x08;
		//fdc.bufcnt = 1;
		fdc.ctrlreg |= 0x08; // アクセスされたらモータ回す･･･ np21w ver0.86 rev39
	}
	/* 170101 ST modified to work on Windows 9x/2000 ... to */
}

static void FDC_ReadID(void) {							// cmd: 0a

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			fdc.mf = fdc.cmd & 0x40;
			get_hdus();
			if (fdd_readid() == SUCCESS) {
				fdcsend_success7();
				//fdc.status = 0x80; // TEST
#if defined(SUPPORT_SWSEEKSND)
				if(np2cfg.MOTOR) fddmtrsnd_play(1, TRUE);
#else
				if(np2cfg.MOTOR) soundmng_pcmplay(SOUND_PCMSEEK, FALSE);
#endif
			}
			else {
				fdc.stat[fdc.us] = fdc.us | (fdc.hd << 2) |
//													FDCRLT_IC0 | FDCRLT_MA;
													FDCRLT_IC0 | FDCRLT_ND;

				fdcsend_error7();
			}
			break;
	}
}

static void FDC_WriteID(void) {							// cmd: 0d

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
//			TRACE_("FDC_WriteID FDCEVENT_CMDRECV", 0);
			get_hdus();
			fdc.N = fdc.cmds[1];
			fdc.sc = fdc.cmds[2];
			fdc.gpl = fdc.cmds[3];
			fdc.d = fdc.cmds[4];
			if (FDC_DriveCheck(TRUE)) {
//				TRACE_("FDC_WriteID FDC_DriveCheck", 0);
				if (fdd_formatinit()) {
//					TRACE_("FDC_WriteID fdd_formatinit", 0);
					fdcsend_error7();
					break;
				}
				/* 170107 modified to work on Windows 9x/2000 form ... */
				if (!fdd_isformating()) {
					fdc.crcn = 1;
					fdcsend_success7();
					break;
				}
				/* 170107 modified to work on Windows 9x/2000 ... to */
//				TRACE_("FDC_WriteID FDCEVENT_BUFRECV", 0);
				fdc.event = FDCEVENT_BUFRECV;
				fdc.bufcnt = 4;
				fdc.bufp = 0;
#if 1															// ver0.27 ??
				fdc.status &= 0x0f;
				fdc.status |= (1 << fdc.us);
				fdc.status |= FDCSTAT_RQM | FDCSTAT_CB;
				if (fdc.nd) {
					fdc.status |= FDCSTAT_NDM;
				}
#else
				fdc.status = FDCSTAT_RQM | FDCSTAT_NDM | FDCSTAT_CB;
#endif
				fdc_dmaready(1);
				dmac_check();
			}
			break;

		case FDCEVENT_BUFRECV:
			if (fdd_formating(fdc.buf)) {
				fdcsend_error7();
				break;
			}
			if ((fdc.tc) || (!fdd_isformating())) {
				fdcsend_success7();
				return;
			}
			fdc.event = FDCEVENT_BUFRECV;
			fdc.bufcnt = 4;
			fdc.bufp = 0;
#if 1															// ver0.27 ??
			fdc.status &= 0x0f;
			fdc.status |= (1 << fdc.us);
			fdc.status |= FDCSTAT_RQM | FDCSTAT_CB;
			if (fdc.nd) {
				fdc.status |= FDCSTAT_NDM;
			}
#else
			fdc.status = FDCSTAT_RQM | FDCSTAT_NDM | FDCSTAT_CB;
#endif
			break;

		default:
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status = FDCSTAT_RQM;
			break;
	}
}

static void FDC_Seek(void) {							// cmd: 0f

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
//			fdc.ncn = fdc.cmds[1];
//			fdc.treg[fdc.us] = fdc.ncn;
//			fdc.R = 1;
//			fdc.crcn = fdc.R;				/* 170107 for Windows95 */
			/* 170101 ST modified to work on Windows 9x/2000 */
			fdc.int_stat[fdc.us] = /*(fdc.hd << 2) |*/ fdc.us;
			fdc.int_stat[fdc.us] |= FDCRLT_SE;
			if (!fdd_diskready(fdc.us)) {
				fdc.int_stat[fdc.us] |= FDCRLT_IC0 | FDCRLT_NR;
			}
			else {
				fdc.ncn = fdc.cmds[1];
				fdc.treg[fdc.us] = fdc.ncn;
				fdc.R = 1;
				fdc.crcn = fdc.R;				/* 170107 for Windows95 */
				/* 170107 for Windows95 form ... */
				//if (fdd_seek()) {
				//	fdc.stat[fdc.us] |= FDCRLT_IC0;
#if defined(SUPPORT_SWSEEKSND)
					if(np2cfg.MOTOR) fddmtrsnd_play(1, TRUE);
#else
					if(np2cfg.MOTOR) soundmng_pcmplay(SOUND_PCMSEEK1, FALSE);
#endif
				//}
				/* 170107 for Windows95 ... to */
			}
			fdc.int_timer[fdc.us] = FDC_INT_DELAY;
			fdc.status &= 0x0f;
			fdc.status |= (1 << fdc.us);
//			fdc_interrupt();
			break;
	}
	fdc.event = FDCEVENT_NEUTRAL;
	fdc.status |= FDCSTAT_RQM;
	fdc.status &= ~FDCSTAT_CB;
}

#ifdef SUPPORT_KAI_IMAGES
static void FDC_ScanEqual(void) {						// cmd: 11, 19, 1d

	switch(fdc.event) {
		case FDCEVENT_CMDRECV:
			get_hdus();
			get_chrn();
			fdc.eot = fdc.cmds[5];
			fdc.gpl = fdc.cmds[6];
			fdc.stp = fdc.cmds[7];
			break;
	}
}
#endif

// --------------------------------------------------------------------------

typedef void (*FDCOPE)(void);

static const FDCOPE FDC_Ope[0x20] = {
				FDC_Invalid,
				FDC_Invalid,
				FDC_ReadData,			// FDC_ReadDiagnostic,
				FDC_Specify,
				FDC_SenseDeviceStatus,
				FDC_WriteData,
				FDC_ReadData,
				FDC_Recalibrate,
				FDC_SenceintStatus,
				FDC_WriteData,
				FDC_ReadID,
				FDC_Invalid,
				FDC_ReadData,
				FDC_WriteID,
				FDC_Invalid,
				FDC_Seek,
				FDC_Invalid,					// 10
#ifdef SUPPORT_KAI_IMAGES
				FDC_ScanEqual,
#else
				FDC_Invalid,			// FDC_ScanEqual,
#endif
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
#ifdef SUPPORT_KAI_IMAGES
				FDC_ScanEqual,
#else
				FDC_Invalid,			// FDC_ScanEqual,
#endif
				FDC_Invalid,
				FDC_Invalid,
				FDC_Invalid,
#ifdef SUPPORT_KAI_IMAGES
				FDC_ScanEqual,
#else
				FDC_Invalid,			// FDC_ScanEqual,
#endif
				FDC_Invalid,
				FDC_Invalid};


// --------------------------------------------------------------------------

static void fdcstatusreset(void) {

	fdc.event = FDCEVENT_NEUTRAL;
	fdc.status = FDCSTAT_RQM;
}

void DMACCALL fdc_datawrite(REG8 data) {

//	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO)) == FDCSTAT_RQM) {
		switch(fdc.event) {
			case FDCEVENT_BUFRECV:
//				TRACE_("write", fdc.bufp);
				fdc.buf[fdc.bufp++] = data;
				if ((!(--fdc.bufcnt)) || (fdc.tc)) {
					fdc.status &= ~FDCSTAT_RQM;
					FDC_Ope[fdc.cmd & 0x1f]();
				}
				break;

			case FDCEVENT_CMDRECV:
				fdc.cmds[fdc.cmdp++] = data;
				if (!(--fdc.cmdcnt)) {
					fdc.status &= ~FDCSTAT_RQM;
					FDC_Ope[fdc.cmd & 0x1f]();
				}
				break;

			default:
				fdc.cmd = data;
				get_mtmfsk();
				fdc.status |= FDCSTAT_CB;	// コマンド受けたらCBをセット
				if (FDCCMD_TABLE[data & 0x1f]) {
					fdc.event = FDCEVENT_CMDRECV;
					fdc.cmdp = 0;
					fdc.cmdcnt = FDCCMD_TABLE[data & 0x1f];
					fdc.status |= FDCSTAT_RQM;
				}
				else {
					fdc.status &= ~FDCSTAT_RQM;
					FDC_Ope[fdc.cmd & 0x1f]();
				}
				break;
		}
//	}
}

REG8 DMACCALL fdc_dataread(void) {

	int		i;

//	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO))
//									== (FDCSTAT_RQM | FDCSTAT_DIO)) {
		switch(fdc.event) {
			case FDCEVENT_BUFSEND:
				fdc.lastdata = fdc.buf[fdc.bufp++];
				if (!(--fdc.bufcnt)) {
					fdc.event = FDCEVENT_NEUTRAL;
					fdc.status &= ~(1 << fdc.us);
					fdc.status &= ~(FDCSTAT_DIO | FDCSTAT_CB);
					fdc.status |= FDCSTAT_RQM;
				}
				// 割り込みクリア処理
				for (i = 0; i < 4; i++) {
					if (fdc.stat[i]) {
						break;
					}
				}
				if (i >= 4) {
					fdc_interruptreset();
				}
				break;

			case FDCEVENT_BUFSEND2:
				if (fdc.bufcnt) {
					fdc.lastdata = fdc.buf[fdc.bufp++];
					fdc.bufcnt--;
				}
				if (fdc.tc) {
					if (!fdc.bufcnt) {						// ver0.26
						fdc.R++;
						if ((fdc.cmd & 0x80) && fdd_seeksector()) {
							fdc.C += fdc.hd;
							fdc.H = fdc.hd ^ 1;
							fdc.R = 1;
						}
					}
					/* 170101 ST modified to work on Windows 9x/2000 */
					fdc.R = 1;
					fdcsend_success7();
				}
				if (!fdc.bufcnt) {
					fdc.event = FDCEVENT_NEXTDATA;
					fdc.status &= ~(FDCSTAT_RQM | FDCSTAT_NDM);
					FDC_Ope[fdc.cmd & 0x1f]();
				}
				break;
		}
//	}
	return(fdc.lastdata);
}

/* SEEKとRECALIBRATEの割り込み通知処理 */
void fdc_intdelay(void) {

	int		i;

	for (i = 0; i < 4; i++) {
		if (fdc.int_timer[i]) {
			fdc.int_timer[i]--;
			if (!fdc.int_timer[i]) {
				fdc.stat[i] = fdc.int_stat[i];
				fdc_interrupt();
			}
		}
	}
}

// ---- I/O

static void IOOUTCALL fdc_o92(UINT port, REG8 dat) {

	TRACEOUT(("fdc out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return;
	}
//	/* 170101 ST modified to work on Windows 9x/2000 form ... */
//	if (fdc.status & FDCSTAT_DIO) {
//		fdc.status &= ~FDCSTAT_DIO;
//		fdc.status |= FDCSTAT_RQM;
//	}
//	/* 170101 ST modified to work on Windows 9x/2000 ... to */

	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO)) == FDCSTAT_RQM) {
		fdc_datawrite(dat);
	}
}

static void IOOUTCALL fdc_o94(UINT port, REG8 dat) {

	UINT8	i;

	TRACEOUT(("fdc out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return;
	}
	if ((fdc.ctrlreg ^ dat) & 0x10) {
		fdcstatusreset();
		fdc_dmaready(0);
		dmac_check();
	}
	/* 170101 ST modified to work on Windows 9x/2000 form ... */
	if ((fdc.ctrlreg ^ dat) & 0x80) {
		if (dat & 0x80) {
			fdcstatusreset();

			if(dat & 0x08){ // XXX: Workaround for OSASK
				// FDCリセット後 1,024ms以内にINTがアクティブになる
				for (i = 0; i < 4; i++) {
					fdc.int_stat[i] = fdc.stat[i];
					fdc.int_timer[i] = FDC_INT_DELAY;
				}
			}
		}
	}
	if (fdc.chgreg & 4) {
		if ((fdc.ctrlreg ^ dat) & 0x08) {
			if (dat & 0x08) {
				for (i = 0; i < 4; i++) {
					if (fdd_diskready(i)) {
						fdc.int_stat[i] = FDCRLT_AI | i;
						fdc.int_timer[i] = FDC_INT_DELAY;
						fdc.status |= (1 << i);
					}
				}
			}
		}
	}
	/* 170101 ST modified to work on Windows 9x/2000 ... to */
	fdc.ctrlreg = dat;
}

static REG8 IOINPCALL fdc_i90(UINT port) {

	TRACEOUT(("fdc in %.2x %.2x [%.4x:%.4x]", port, fdc.status,
															CPU_CS, CPU_IP));

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return(0xff);
	}
	return(fdc.status);
}

static REG8 IOINPCALL fdc_i92(UINT port) {

	REG8	ret;

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return(0xff);
	}
	if ((fdc.status & (FDCSTAT_RQM | FDCSTAT_DIO))
										== (FDCSTAT_RQM | FDCSTAT_DIO)) {
		ret = fdc_dataread();
	}
	else {
		ret = fdc.lastdata;
	}
	TRACEOUT(("fdc in %.2x %.2x [%.4x:%.4x]", port, ret, CPU_CS, CPU_IP));
	return(ret);
}

static REG8 IOINPCALL fdc_i94(UINT port) {

	REG8 ret;

	if (((port >> 4) ^ fdc.chgreg) & 1) {
		return(0xff);
	}

	ret = 0x40;
	if (!(port & 0x10))		/* CC */
	{
		ret |= 0x20;		/* DMA */
		ret |= 0x10;		/* readyを立てるるる */
	}

	if (pccore.dipsw[0] & 8)
	{
		ret |= 0x04;		/* 内蔵優先 */
	}
	else
	{
		ret |= 0x08;		/* 外付け優先 */
	}
	TRACEOUT(("fdc in %.2x %.2x [%.4x:%.4x]", port, ret,
														CPU_CS, CPU_IP));

	return ret;
}


static void IOOUTCALL fdc_obe(UINT port, REG8 dat) {
	
	TRACEOUT(("fdc out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	fdc.chgreg = dat;
	if (fdc.chgreg & 2) {
		CTRL_FDMEDIA = DISKTYPE_2HD;
	}
	else {
		CTRL_FDMEDIA = DISKTYPE_2DD;
	}
	(void)port;
}

static REG8 IOINPCALL fdc_ibe(UINT port) {

	(void)port;
	TRACEOUT(("fdc in %.2x %.2x [%.4x:%.4x]", port, ((fdc.chgreg & 3) | 8),
														CPU_CS, CPU_IP));

	return((fdc.chgreg & 3) | 8 | 0xf0);
}

static void IOOUTCALL fdc_o4be(UINT port, REG8 dat) {
	
	TRACEOUT(("fdc out %.2x %.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));

	fdc.reg144 = dat;
	if (dat & 0x10) {
		fdc.rpm[(dat >> 5) & 3] = dat & 1;
	}
	(void)port;
}

static REG8 IOINPCALL fdc_i4be(UINT port) {

	(void)port;
	TRACEOUT(("fdc in %.2x %.2x [%.4x:%.4x]", port, (fdc.rpm[(fdc.reg144 >> 5) & 3] | 0xf0),
													CPU_CS, CPU_IP));
	return(fdc.rpm[(fdc.reg144 >> 5) & 3] | 0xfe);
}


// ---- I/F

static const IOOUT fdco90[4] = {
					NULL,		fdc_o92,	fdc_o94,	NULL};
static const IOINP fdci90[4] = {
					fdc_i90,	fdc_i92,	fdc_i94,	NULL};
static const IOOUT fdcobe[1] = {fdc_obe};
static const IOINP fdcibe[1] = {fdc_ibe};

void fdc_reset(const NP2CFG *pConfig) {

	ZeroMemory(&fdc, sizeof(fdc));
	fdc.equip = pConfig->fddequip;
#if defined(SUPPORT_PC9821)
	fdc.support144 = 1;
#else
	fdc.support144 = pConfig->usefd144;
#endif
	fdcstatusreset();
	dmac_attach(DMADEV_2HD, FDC_DMACH2HD);
	dmac_attach(DMADEV_2DD, FDC_DMACH2DD);
	CTRL_FDMEDIA = DISKTYPE_2HD;
	fdc.chgreg = 3;
}

void fdc_bind(void) {

	iocore_attachcmnoutex(0x0090, 0x00f9, fdco90, 4);
	iocore_attachcmninpex(0x0090, 0x00f9, fdci90, 4);
	iocore_attachcmnoutex(0x00c8, 0x00f9, fdco90, 4);
	iocore_attachcmninpex(0x00c8, 0x00f9, fdci90, 4);

	if (fdc.support144) {
		iocore_attachout(0x04be, fdc_o4be);
		iocore_attachinp(0x04be, fdc_i4be);
	}
	iocore_attachsysoutex(0x00be, 0x0cff, fdcobe, 1);
	iocore_attachsysinpex(0x00be, 0x0cff, fdcibe, 1);
}

