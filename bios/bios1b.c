#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"scsicmd.h"
#include	"bios.h"
#include	"biosmem.h"
#include	"sxsibios.h"
#include	"diskimage/fddfile.h"
#include	"fdd/fdd_mtr.h"
#include	"fdd/sxsi.h"


enum {
	CACHE_TABLES	= 4,
	CACHE_BUFFER	= 32768
};

extern int sxsi_unittbl[];


// ---- FDD

static BRESULT setfdcmode(REG8 drv, REG8 type, REG8 rpm) {

	if (drv >= 4) {
		return(FAILURE);
	}
	if ((rpm) && (!fdc.support144)) {
		return(FAILURE);
	}
	if ((fdc.chgreg ^ type) & 1) {
		return(FAILURE);
	}
	fdc.chgreg = type;
	fdc.rpm[drv] = rpm;
	if (type & 2) {
		CTRL_FDMEDIA = DISKTYPE_2HD;
	}
	else {
		CTRL_FDMEDIA = DISKTYPE_2DD;
	}
	return(SUCCESS);
}

void fddbios_equip(REG8 type, BOOL clear) {

	REG16	diskequip;

	diskequip = GETBIOSMEM16(MEMW_DISK_EQUIP);
	if (clear) {
		diskequip &= 0x0f00;
	}
	if (type & 1) {
		diskequip &= 0xfff0;
		diskequip |= (fdc.equip & 0x0f);
	}
	else {
		diskequip &= 0x0fff;
		diskequip |= (fdc.equip & 0x0f) << 12;
	}
	SETBIOSMEM16(MEMW_DISK_EQUIP, diskequip);
}

static void biosfd_setchrn(void) {

	fdc.C = CPU_CL;
	fdc.H = CPU_DH;
	fdc.R = CPU_DL;
	fdc.N = CPU_CH;
}

#if 0
static void biosfd_resultout(UINT32 result) {

	UINT8	*ptr;

	ptr = mem + 0x00564 + (fdc.us*8);
	ptr[0] = (UINT8)(result & 0xff) | (fdc.hd << 2) | fdc.us;
	ptr[1] = (UINT8)(result >> 8);
	ptr[2] = (UINT8)(result >> 16);
	ptr[3] = fdc.C;
	ptr[4] = fdc.H;
	ptr[5] = fdc.R;
	ptr[6] = fdc.N;
	ptr[7] = fdc.ncn;
}
#endif

static BRESULT biosfd_seek(REG8 track, BOOL ndensity) {

	if (ndensity) {
		if (track < 42) {
			track <<= 1;
		}
		else {
			track = 42 * 2;
		}
	}
	fdc.ncn = track;
	if (fdd_seek()) {
		return(FAILURE);
	}
	return(SUCCESS);
}

static UINT16 fdfmt_biospara(REG8 type, REG8 rpm, REG8 fmt) {

	UINT	seg;
	UINT	off;
	UINT16	n;

	n = fdc.N;
	if (n >= 4) {
		n = 3;
	}
	if (type & 2) {
		seg = GETBIOSMEM16(MEMW_F2HD_P_SEG);
		off = GETBIOSMEM16(MEMW_F2HD_P_OFF);
	}
	else {
		seg = GETBIOSMEM16(MEMW_F2DD_P_SEG);
		off = GETBIOSMEM16(MEMW_F2DD_P_OFF);
	}
	if (rpm) {
		off = 0x2361;									// see bios.cpp
	}
	off += fdc.us * 2;
	off = MEMR_READ16(seg, off);
	off += n * 8;
	if (!(CPU_AH & 0x40)) {
		off += 4;
	}
	if (fmt) {
		off += 2;
	}
	return(MEMR_READ16(seg, off));
}


enum {
	FDCBIOS_NORESULT,
	FDCBIOS_SUCCESS,
	FDCBIOS_SEEKSUCCESS,
	FDCBIOS_ERROR,
	FDCBIOS_SEEKERROR,
	FDCBIOS_READERROR,
	FDCBIOS_WRITEERROR,
	FDCBIOS_NONREADY,
	FDCBIOS_WRITEPROTECT
};

static void fdd_int(int result) {

	if (result == FDCBIOS_NORESULT) {
		return;
	}
	switch(CPU_AH & 0x0f) {
		case 0x00:								// シーク
		case 0x01:								// ベリファイ
		case 0x02:								// 診断の為の読み込み
		case 0x05:								// データの書き込み
		case 0x06:								// データの読み込み
//		case 0x07:								// シリンダ０へシーク
		case 0x0a:								// READ ID
		case 0x0d:								// フォーマット
			break;

		default:
			return;
	}
//	kaiD
	if (fdd_fdcresult() == FALSE) {
		fdc.stat[fdc.us] = (fdc.hd << 2) | fdc.us;
	}
//
	switch(result) {
		case FDCBIOS_SUCCESS:
			fdcsend_success7();
			break;

		case FDCBIOS_SEEKSUCCESS:
		case FDCBIOS_SEEKERROR:
			fdc.stat[fdc.us] |= FDCRLT_SE;
			fdc_interrupt();
			fdc.event = FDCEVENT_NEUTRAL;
			fdc.status = FDCSTAT_RQM;
			break;

		case FDCBIOS_READERROR:
			fdc.stat[fdc.us] |= FDCRLT_IC0 | FDCRLT_ND;
			fdcsend_error7();
			break;

		case FDCBIOS_WRITEERROR:
			fdc.stat[fdc.us] |= FDCRLT_IC0 | FDCRLT_EN;
			fdcsend_error7();
			break;

		case FDCBIOS_NONREADY:
			fdc.stat[fdc.us] |= FDCRLT_IC0 | FDCRLT_NR;
			fdcsend_error7();
			break;

		case FDCBIOS_WRITEPROTECT:
			fdc.stat[fdc.us] |= FDCRLT_IC0 | FDCRLT_NW;
			fdcsend_error7();
			break;

		default:
			return;
	}
	if (fdc.chgreg & 1) {
		mem[MEMB_DISK_INTL] &= ~(0x01 << fdc.us);
	}
	else {
		mem[MEMB_DISK_INTH] &= ~(0x10 << fdc.us);
	}
	CPU_IP = BIOSOFST_WAIT;
}

#if 1
static struct {
	BOOL	flg;
	UINT16	cx;
	UINT16	dx;
	UINT	pos;
//	UINT	cnt;
} b0p;

static void b0patch(void) {

	if ((!b0p.flg) || (b0p.cx != CPU_CX) || (b0p.dx != CPU_DX)) {
		b0p.flg = TRUE;
		b0p.pos = 0;
		b0p.cx = CPU_CX;
		b0p.dx = CPU_DX;
	}
	else {
		if (!b0p.pos) {
			UINT32	addr;
			UINT	size;
			UINT	cnt;
			REG8	c;
			REG8	cl;
			REG8	last;
			addr = CPU_BP;
			size = CPU_BX;
			cnt = 0;
			last = 0;
			while(size--) {
				c = MEMR_READ8(CPU_ES, addr++);
				cl = 0;
				do {
					REG8 now = c & 0x80;
					c <<= 1;
					b0p.pos++;
					if (now == last) {
						cnt++;
						if (cnt > 4) {
							break;
						}
					}
					else {
						cnt = 0;
						last = now;
					}
					cl++;
				} while(cl < 8);
				if (cnt > 4) {
					break;
				}
			}
		}
		if ((b0p.pos >> 3) < CPU_BX) {
			UINT addr;
			REG8 c;
			addr = CPU_BP + (b0p.pos >> 3);
			c = MEMR_READ8(CPU_ES, addr);
			c ^= (1 << (b0p.pos & 7));
			b0p.pos++;
			MEMR_WRITE8(CPU_ES, addr, c);
		}
	}
}

static void b0clr(void) {
	b0p.flg = FALSE;
}
#endif

static REG8 fdd_operate(REG8 type, REG8 rpm, BOOL ndensity) {

	REG8	ret_ah = 0x60;
	UINT16	size;
	UINT16	pos;
	UINT16	accesssize;
	UINT16	secsize;
	UINT16	para;
	UINT8	s;
	UINT8	ID[4];
	UINT8	hd;
	int		result = FDCBIOS_NORESULT;
	UINT32	addr;
	UINT8	mtr_c;
	UINT	mtr_r;
	UINT	fmode;

	mtr_c = fdc.ncn;
	mtr_r = 0;

	// とりあえずBIOSの時は無視する
	fdc.mf = 0xff;

//	TRACE_("int 1Bh", CPU_AH);

	if (setfdcmode((REG8)(CPU_AL & 3), type, rpm) != SUCCESS) {
		return(0x40);
	}

	if ((CPU_AH & 0x0f) != 0x0a) {
		fdc.crcn = 0;
	}
	if ((CPU_AH & 0x0f) != 0x03) {
		if (type & 2) {
			if (pic.pi[1].imr & PIC_INT42) {
				return(0x40);
			}
		}
		else {
			if (pic.pi[1].imr & PIC_INT41) {
				return(0x40);
			}
		}
		if (fdc.us != (CPU_AL & 0x03)) {
			fdc.us = CPU_AL & 0x03;
			fdc.crcn = 0;
		}
		hd = ((CPU_DH) ^ (CPU_AL >> 2)) & 1;
		if (fdc.hd != hd) {
			fdc.hd = hd;
			fdc.crcn = 0;
		}
		if (!fdd_diskready(fdc.us)) {
			fdd_int(FDCBIOS_NONREADY);
			ret_ah = 0x60;
			if ((CPU_AX & 0x8f40) == 0x8400) {
				ret_ah |= 8;					// 1MB/640KB両用ドライブ
				if ((CPU_AH & 0x40) && (fdc.support144)) {
					ret_ah |= 4;				// 1.44対応ドライブ
				}
			}
			return(ret_ah);
		}
	}

	// モード選択													// ver0.78
	fmode = (type & 1)?MEMB_F2HD_MODE:MEMB_F2DD_MODE;
	if (!(CPU_AL & 0x80)) {
		if (!(mem[fmode] & (0x10 << fdc.us))) {
			ndensity = TRUE;
		}
	}

	switch(CPU_AH & 0x0f) {
		case 0x00:								// シーク
			if (CPU_AH & 0x10) {
				if (biosfd_seek(CPU_CL, ndensity) == SUCCESS) {
					result = FDCBIOS_SEEKSUCCESS;
				}
				else {
					result = FDCBIOS_SEEKERROR;
				}
			}
			ret_ah = 0x00;
			break;

		case 0x01:								// ベリファイ
			if (CPU_AH & 0x10) {
				if (biosfd_seek(CPU_CL, ndensity) == SUCCESS) {
					result = FDCBIOS_SEEKSUCCESS;
				}
				else {
					ret_ah = 0xe0;
					result = FDCBIOS_SEEKERROR;
					break;
				}
			}
			biosfd_setchrn();
			para = fdfmt_biospara(type, rpm, 0);
			if (!para) {
				ret_ah = 0xd0;
				break;
			}
			if (fdc.N < 8) {
				secsize = 128 << fdc.N;
			}
			else {
				secsize = 128 << 8;
			}
			size = CPU_BX;
			while(size) {
				if (size > secsize) {
					accesssize = secsize;
				}
				else {
					accesssize = size;
				}
				if (fdd_read()) {
					break;
				}
				size -= accesssize;
				mtr_r += accesssize;
				if ((fdc.R++ == (UINT8)para) &&
					(CPU_AH & 0x80) && (!fdc.hd)) {
					fdc.hd = 1;
					fdc.H = 1;
					fdc.R = 1;
					if (biosfd_seek(fdc.treg[fdc.us], 0) != SUCCESS) {
						break;
					}
				}
			}
			if (!size) {
				ret_ah = 0x00;
				result = FDCBIOS_SUCCESS;
			}
			else {
				ret_ah = 0xc0;
				result = FDCBIOS_READERROR;
			}
			break;

		case 0x03:								// 初期化
			fddbios_equip(type, FALSE);
			ret_ah = 0x00;
			break;

		case 0x04:								// センス
			ret_ah = 0x00;
			if (fdd_diskprotect(fdc.us))
			{
				ret_ah = 0x10;
			}
			if (CPU_AL & 0x80) {				// 2HD
				ret_ah |= 0x01;
			}
			else {								// 2DD
				if (mem[fmode] & (0x01 << fdc.us)) {
					ret_ah |= 0x01;
				}
				if (mem[fmode] & (0x10 << fdc.us)) {
					ret_ah |= 0x04;
				}
			}
			if ((CPU_AX & 0x8f40) == 0x8400) {
				ret_ah |= 8;					// 1MB/640KB両用ドライブ
				if ((CPU_AH & 0x40) && (fdc.support144)) {
					ret_ah |= 4;				// 1.44対応ドライブ
				}
			}
			break;

		case 0x05:								// データの書き込み
			if (CPU_AH & 0x10) {
				if (biosfd_seek(CPU_CL, ndensity) == SUCCESS) {
					result = FDCBIOS_SEEKSUCCESS;
				}
				else {
					ret_ah = 0xe0;
					result = FDCBIOS_SEEKERROR;
					break;
				}
			}
			biosfd_setchrn();
			para = fdfmt_biospara(type, rpm, 0);
			if (!para) {
				ret_ah = 0xd0;
				break;
			}
			if (fdd_diskprotect(fdc.us)) {
				ret_ah = 0x70;
				result = FDCBIOS_WRITEPROTECT;
				break;
			}
			if (fdc.N < 8) {
				secsize = 128 << fdc.N;
			}
			else {
				secsize = 128 << 8;
			}
			size = CPU_BX;
			addr = ES_BASE + CPU_BP;
			while(size) {
				if (size > secsize) {
					accesssize = secsize;
				}
				else {
					accesssize = size;
				}
				MEML_READS(addr, fdc.buf, accesssize);
				if (fdd_write()) {
					break;
				}
				addr += accesssize;
				size -= accesssize;
				mtr_r += accesssize;
				if ((fdc.R++ == (UINT8)para) &&
					(CPU_AH & 0x80) && (!fdc.hd)) {
					fdc.hd = 1;
					fdc.H = 1;
					fdc.R = 1;
					if (biosfd_seek(fdc.treg[fdc.us], 0) != SUCCESS) {
						break;
					}
				}
			}
			if (!size) {
				ret_ah = 0x00;
				result = FDCBIOS_SUCCESS;
			}
			else {
				ret_ah = fddlasterror;			// 0xc0
				result = FDCBIOS_WRITEERROR;
			}
			break;

		case 0x02:								// 診断の為の読み込み
		case 0x06:								// データの読み込み
			if (CPU_AH & 0x10) {
				if (biosfd_seek(CPU_CL, ndensity) == SUCCESS) {
					result = FDCBIOS_SEEKSUCCESS;
				}
				else {
					ret_ah = 0xe0;
					result = FDCBIOS_SEEKERROR;
					break;
				}
			}
			biosfd_setchrn();
			para = fdfmt_biospara(type, rpm, 0);
			if (!para) {
				ret_ah = 0xd0;
				break;
			}
#if 0
			if (fdc.R >= 0xf4) {
				ret_ah = 0xb0;
				break;
			}
#endif
			if (fdc.N < 8) {
				secsize = 128 << fdc.N;
			}
			else {
				secsize = 128 << 8;
			}
			size = CPU_BX;
			addr = ES_BASE + CPU_BP;
			while(size) {
				if (size > secsize) {
					accesssize = secsize;
				}
				else {
					accesssize = size;
				}
				if (fdd_read()) {
					break;
				}
				MEML_WRITES(addr, fdc.buf, accesssize);
				addr += accesssize;
				size -= accesssize;
				mtr_r += accesssize;
				if (fdc.R++ == (UINT8)para) {
					if ((CPU_AH & 0x80) && (!fdc.hd)) {
						fdc.hd = 1;
						fdc.H = 1;
						fdc.R = 1;
						if (biosfd_seek(fdc.treg[fdc.us], 0) != SUCCESS) {
							break;
						}
					}
#if 1
					else {
						fdc.C++;
						fdc.R = 1;
						break;
					}
#endif
				}
			}
			if (!size) {
				ret_ah = fddlasterror;				// 0x00;
				result = FDCBIOS_SUCCESS;
#if 1
				if (ret_ah == 0xb0) {
					b0patch();
				}
				else {
					b0clr();
				}
#endif
			}
#if 1
			else if ((CPU_AH & 0x0f) == 0x02) {		// ARS対策…
				ret_ah = 0x00;
				result = FDCBIOS_READERROR;
			}
#endif
			else {
				ret_ah = fddlasterror;				// 0xc0;
				result = FDCBIOS_READERROR;
			}
			break;

		case 0x07:						// シリンダ０へシーク
			biosfd_seek(0, 0);
			ret_ah = 0x00;
			result = FDCBIOS_SEEKSUCCESS;
			break;

		case 0x09:
			//	1001b	WRITE DELETED DATA
			TRACEOUT(("\tWRITE DELETED DATA not Support"));
			break;

		case 0x0a:						// READ ID
			fdc.mf = CPU_AH & 0x40;
			if (CPU_AH & 0x10) {
				if (biosfd_seek(CPU_CL, ndensity) == SUCCESS) {
					result = FDCBIOS_SEEKSUCCESS;
				}
				else {
					ret_ah = 0xe0;
					result = FDCBIOS_SEEKERROR;
					break;
				}
			}
			if (fdd_readid()) {
				ret_ah = fddlasterror;			// 0xa0;
				break;
			}
			if (fdc.N < 8) {
				mtr_r += 128 << fdc.N;
			}
			else {
				mtr_r += 128 << 8;
			}
			ret_ah = 0x00;
			CPU_CL = fdc.C;
			CPU_DH = fdc.H;
			CPU_DL = fdc.R;
			CPU_CH = fdc.N;
			result = FDCBIOS_SUCCESS;
			break;

		case 0x0c:
			//	1100b	READ DELETED DATA
			TRACEOUT(("\tREAD DELETED DATA not Support"));
			break;

		case 0x0d:						// フォーマット
			if (CPU_AH & 0x10) {
				biosfd_seek(CPU_CL, ndensity);
			}
			if (fdd_diskprotect(fdc.us)) {
				ret_ah = 0x70;
				break;
			}
			fdc.d = CPU_DL;
			fdc.N = CPU_CH;
			para = fdfmt_biospara(type, rpm, 1);
			if (!para) {
				ret_ah = 0xd0;
				break;
			}
			fdc.sc = (UINT8)para;
			fdd_formatinit();
			pos = CPU_BP;
			for (s=0; s<fdc.sc; s++) {
				MEMR_READS(CPU_ES, pos, ID, 4);
				fdd_formating(ID);
				pos += 4;
				if (ID[3] < 8) {
					mtr_r += 128 << ID[3];
				}
				else {
					mtr_r += 128 << 8;
				}
			}
			ret_ah = 0x00;
			break;

		case 0x0e:													// ver0.78
			if (CPU_AH & 0x80) {			// 密度設定
				mem[fmode] &= 0x0f;
				mem[fmode] |= (UINT8)((CPU_AH & 0x0f) << 4);
			}
			else {							// 面設定
				mem[fmode] &= 0xf0;
				mem[fmode] |= (UINT8)(CPU_AH & 0x0f);
			}
			ret_ah = 0x00;
			break;
	}
	fdd_int(result);
	if (mtr_c != fdc.ncn) {
		fddmtr_seek(fdc.us, mtr_c, mtr_r);
	}
	return(ret_ah);
}


// -------------------------------------------------------------------- BIOS

static UINT16 boot_fd1(REG8 type, REG8 rpm) {

	UINT	remain;
	UINT	size;
	UINT32	pos;
	UINT16	bootseg;

	if (setfdcmode(fdc.us, type, rpm) != SUCCESS) {
		return(0);
	}
	if (biosfd_seek(0, 0) != SUCCESS) {
		return(0);
	}
	fdc.hd = 0;
	fdc.mf = 0x40;			// とりあえず MFMモードでリード
	if (fdd_readid()) {
		fdc.mf = 0x00;		// FMモードでリトライ
		if (fdd_readid()) {
			return(0);
		}
	}
	remain = 0x400;
	pos = 0x1fc00;
	if ((!fdc.N) || (!fdc.mf) || (rpm)) {
		pos = 0x1fe00;
		remain = 0x200;
	}
	fdc.R = 1;
	bootseg = (UINT16)(pos >> 4);
	while(remain) {
		if (fdd_read()) {
			return(0);
		}
		if (fdc.N < 3) {
			size = 128 << fdc.N;
		}
		else {
			size = 128 << 3;
		}
		if (remain < size) {
			CopyMemory(mem + pos, fdc.buf, remain);
			break;
		}
		else {
			CopyMemory(mem + pos, fdc.buf, size);
			pos += size;
			remain -= size;
			fdc.R++;
		}
	}
	return(bootseg);
}

static UINT16 boot_fd(REG8 drv, REG8 type) {

	UINT16	bootseg;

	if (drv >= 4) {
		return(0);
	}
	fdc.us = drv;
	if (!fdd_diskready(fdc.us)) {
		return(0);
	}

	// 2HD
	if (type & 1) {
		fdc.chgreg |= 0x01;
		// 1.25MB
		bootseg = boot_fd1(3, 0);
		if (bootseg) {
			mem[MEMB_DISK_BOOT] = (UINT8)(0x90 + drv);
			fddbios_equip(3, TRUE);
			return(bootseg);
		}
		// 1.44MB
		bootseg = boot_fd1(3, 1);
		if (bootseg) {
			mem[MEMB_DISK_BOOT] = (UINT8)(0x30 + drv);
			fddbios_equip(3, TRUE);
			return(bootseg);
		}
	}
	if (type & 2) {
		fdc.chgreg &= ~0x01;
		// 2DD
		bootseg = boot_fd1(0, 0);
		if (bootseg) {
			mem[MEMB_DISK_BOOT] = (UINT8)(0x70 + drv);
			fddbios_equip(0, TRUE);
			return(bootseg);
		}
	}
	fdc.chgreg |= 0x01;
	return(0);
}

static REG16 boot_hd(REG8 drv) {

	REG8	ret;
	
	if(pccore.hddif & PCHDD_IDE){
		ret = sxsi_read((drv & 0xf0)==0x80 ? (0x80 | sxsi_unittbl[drv & 0x3]) : drv, 0, mem + 0x1fc00, 0x400);
	}else{
		ret = sxsi_read(drv, 0, mem + 0x1fc00, 0x400);
	}
	if (ret < 0x20) {
		mem[MEMB_DISK_BOOT] = drv;
		return(0x1fc0);
	}
	return(0);
}

REG16 bootstrapload(void) {

	UINT8	i;
	REG16	bootseg;

//	fdmode = 0;
	bootseg = 0;
	switch(mem[MEMB_MSW5] & 0xf0) {		// うぐぅ…本当はALレジスタの値から
		case 0x00:					// ノーマル
			break;

		case 0x20:					// 640KB FDD
			for (i=0; (i<4) && (!bootseg); i++) {
				if (fdd_diskready(i)) {
					bootseg = boot_fd(i, 2);
				}
			}
			break;

		case 0x40:					// 1.2MB FDD
			for (i=0; (i<4) && (!bootseg); i++) {
				if (fdd_diskready(i)) {
					bootseg = boot_fd(i, 1);
				}
			}
			break;

		case 0x60:					// MO
			break;

		case 0xa0:					// SASI 1
			bootseg = boot_hd(0x80);
			break;

		case 0xb0:					// SASI 2
			bootseg = boot_hd(0x81);
			break;

		case 0xc0:					// SCSI
			for (i=0; (i<4) && (!bootseg); i++) {
				bootseg = boot_hd((REG8)(0xa0 + i));
			}
			break;

		default:					// ROM
			return(0);
	}
	for (i=0; (i<4) && (!bootseg); i++) {
		if (fdd_diskready(i)) {
			bootseg = boot_fd(i, 3);
		}
	}
	if(pccore.hddif & PCHDD_IDE){
		for (i=0; (i<4) && (!bootseg); i++) {
			if(sxsi_getptr(sxsi_unittbl[i])->devtype == SXSIDEV_HDD){
				bootseg = boot_hd((REG8)(0x80 + i));
			}
		}
	}else if(pccore.hddif & PCHDD_SASI){
		for (i=0; (i<2) && (!bootseg); i++) {
			bootseg = boot_hd((REG8)(0x80 + i));
		}
	}
	for (i=0; (i<4) && (!bootseg); i++) {
		bootseg = boot_hd((REG8)(0xa0 + i));
	}
	return(bootseg);
}


// --------------------------------------------------------------------------

void bios0x1b(void) {

	REG8	ret_ah;
	REG8	flag;

#if 1			// bypass to disk bios
	REG8	seg;
	UINT	sp;

	seg = mem[MEMX_DISK_XROM + (CPU_AL >> 4)];
	if (seg) {
		sp = CPU_SP;
		MEMR_WRITE16(CPU_SS, sp - 2, CPU_DS);
		MEMR_WRITE16(CPU_SS, sp - 4, CPU_SI);
		MEMR_WRITE16(CPU_SS, sp - 6, CPU_DI);
		MEMR_WRITE16(CPU_SS, sp - 8, CPU_ES);		// +a
		MEMR_WRITE16(CPU_SS, sp - 10, CPU_BP);		// +8
		MEMR_WRITE16(CPU_SS, sp - 12, CPU_DX);		// +6
		MEMR_WRITE16(CPU_SS, sp - 14, CPU_CX);		// +4
		MEMR_WRITE16(CPU_SS, sp - 16, CPU_BX);		// +2
		MEMR_WRITE16(CPU_SS, sp - 18, CPU_AX);		// +0
#if 0
		TRACEOUT(("call by %.4x:%.4x",
							MEMR_READ16(CPU_SS, CPU_SP+2),
							MEMR_READ16(CPU_SS, CPU_SP)));
		TRACEOUT(("bypass to %.4x:0018", seg << 8));
		TRACEOUT(("AX=%04x BX=%04x %02x:%02x:%02x:%02x ES=%04x BP=%04x",
							CPU_AX, CPU_BX, CPU_CL, CPU_DH, CPU_DL, CPU_CH,
							CPU_ES, CPU_BP));
#endif
		sp -= 18;
		CPU_SP = sp;
		CPU_BP = sp;
		CPU_DS = 0x0000;
		CPU_BX = 0x04B0;
		CPU_AX = seg << 8;
		CPU_CS = seg << 8;
		CPU_IP = 0x18;
		return;
	}
#endif

#if defined(SUPPORT_SCSI)
	if ((CPU_AL & 0xf0) == 0xc0) {
		TRACEOUT(("%.4x:%.4x AX=%.4x BX=%.4x CX=%.4x DX=%.4 ES=%.4x BP=%.4x",
							MEMR_READ16(CPU_SS, CPU_SP+2),
							MEMR_READ16(CPU_SS, CPU_SP),
							CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_ES, CPU_BP));
		scsicmd_bios();
		return;
	}
#endif

	switch(CPU_AL & 0xf0) {
		case 0x90:
			ret_ah = fdd_operate(3, 0, FALSE);
			break;

		case 0x30:
		case 0xb0:
			ret_ah = fdd_operate(3, 1, FALSE);
			break;

		case 0x10:
			ret_ah = fdd_operate(1, 0, FALSE);
			break;

		case 0x70:
		case 0xf0:
			ret_ah = fdd_operate(0, 0, FALSE);
			break;

		case 0x50:
			ret_ah = fdd_operate(0, 0, TRUE);
			break;

		case 0x00:
		case 0x80:
			ret_ah = sasibios_operate();
			break;

#if defined(SUPPORT_SCSI)
		case 0x20:
		case 0xa0:
			ret_ah = scsibios_operate();
			break;
#endif

		default:
			ret_ah = 0x40;
			break;
	}
#if 0
	TRACEOUT(("%04x:%04x AX=%04x BX=%04x %02x:%02x:%02x:%02x\n"	\
						"ES=%04x BP=%04x \nret=%02x",
							MEMR_READ16(CPU_SS, CPU_SP+2),
							MEMR_READ16(CPU_SS, CPU_SP),
							CPU_AX, CPU_BX, CPU_CL, CPU_DH, CPU_DL, CPU_CH,
							CPU_ES, CPU_BP, ret_ah));
#endif
	CPU_AH = ret_ah;
	flag = MEMR_READ8(CPU_SS, CPU_SP+4) & 0xfe;
	if (ret_ah >= 0x20) {
		flag += 1;
	}
	MEMR_WRITE8(CPU_SS, CPU_SP + 4, flag);
}

UINT bios0x1b_wait(void) {

	UINT	addr;
	REG8	bit;
	static UINT32 int_timeout = 0; // np21w ver0.86 rev51 Win3.1用 暫定無限ループ回避

	if (fddmtr.busy) {
		CPU_REMCLOCK = -1;
	}
	else {
		if (fdc.chgreg & 1) {
			addr = MEMB_DISK_INTL;
			bit = 0x01;
		}
		else {
			addr = MEMB_DISK_INTH;
			bit = 0x10;
		}
		bit <<= fdc.us;
		if ((mem[addr] & bit) || int_timeout > pccore.realclock*3) {
			mem[addr] &= ~bit;
			int_timeout = 0;
			return(0);
		}
		else {
			CPU_REMCLOCK -= 1000;
#if defined(CPUCORE_IA32)
			// np21w ver0.86 rev51 Win3.1用 暫定無限ループ回避
			if (CPU_STAT_PM && CPU_STAT_VM86) {
				int_timeout += 1000;
			}
#endif
		}
	}
	CPU_IP--;
	return(1);
}

