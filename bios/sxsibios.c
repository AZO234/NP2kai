/**
 * @file	sxsibios.c
 * @brief	Implementation of SxSI BIOS
 */

#include "compiler.h"
#include "sxsibios.h"
#include "biosmem.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"scsicmd.h"
#include	"fdd/sxsi.h"
#include	"timing.h"


typedef REG8 (*SXSIFUNC)(UINT type, SXSIDEV sxsi);

static REG8 sxsi_pos(UINT type, SXSIDEV sxsi, FILEPOS *ppos) {

	REG8	ret;
	FILEPOS	pos;

	ret = 0;
	pos = 0;
	if (CPU_AL & 0x80) {
		if ((CPU_DL >= sxsi->sectors) ||
			(CPU_DH >= sxsi->surfaces) ||
			(CPU_CX >= sxsi->cylinders)) {
			ret = 0xd0;
		}
		pos = ((CPU_CX * sxsi->surfaces) + CPU_DH) * sxsi->sectors
															+ CPU_DL;
	}
	else {
		pos = (CPU_DL << 16) | CPU_CX;
		if (type == SXSIBIOS_SASI) {
			pos &= 0x1fffff;
		}
		if (pos >= sxsi->totals) {
			ret = 0xd0;
		}
	}

	*ppos = pos;
	if (sxsi->size > 1024) {
		ret = 0xd0;
	}
	return(ret);
}

static REG8 sxsibios_write(UINT type, SXSIDEV sxsi) {

	REG8	ret;
	UINT	size;
	FILEPOS	pos;
	UINT32	addr;
	UINT	r;
	UINT8	work[1024];

	size = CPU_BX;
	if (!size) {
		size = 0x10000;
	}
	ret = sxsi_pos(type, sxsi, &pos);
	if (!ret) {
		addr = (CPU_ES << 4) + CPU_BP;
		while(size) {
			r = min(size, sxsi->size);
			MEML_READS(addr, work, r);
			ret = sxsi_write(CPU_AL, pos, work, r);
			if (ret >= 0x20) {
				break;
			}
			addr += r;
			size -= r;
			pos++;
		}
	}
	return(ret);
}

static REG8 sxsibios_read(UINT type, SXSIDEV sxsi) {

	REG8	ret;
	UINT	size;
	FILEPOS	pos;
	UINT32	addr;
	UINT	r;
	UINT8	work[1024];

	size = CPU_BX;
	if (!size) {
		size = 0x10000;
	}
	ret = sxsi_pos(type, sxsi, &pos);
	if (!ret) {
		addr = (CPU_ES << 4) + CPU_BP;
		while(size) {
			r = min(size, sxsi->size);
			ret = sxsi_read(CPU_AL, pos, work, r);
			if (ret >= 0x20) {
				break;
			}
			MEML_WRITES(addr, work, r);
			addr += r;
			size -= r;
			pos++;
		}
	}
	return(ret);
}

static REG8 sxsibios_format(UINT type, SXSIDEV sxsi) {

	REG8	ret;
	FILEPOS	pos;

	if (CPU_AH & 0x80) {
		if (type == SXSIBIOS_SCSI) {		// とりあえずSCSIのみ
			UINT count;
			FILEPOS posmax;
			count = timing_getcount();			// 時間を止める
			ret = 0;
			pos = 0;
			posmax = (FILEPOS)sxsi->surfaces * sxsi->cylinders;
			while(pos < posmax) {
				ret = sxsi_format(CPU_AL, pos * sxsi->sectors);
				if (ret) {
					break;
				}
				pos++;
			}
			timing_setcount(count);							// 再開
		}
		else {
			ret = 0xd0;
		}
	}
	else {
		if (CPU_DL) {
			ret = 0x30;
		}
		else {
//			i286_memstr_read(CPU_ES, CPU_BP, work, CPU_BX);
			ret = sxsi_pos(type, sxsi, &pos);
			if (!ret) {
				ret = sxsi_format(CPU_AL, pos);
			}
		}
	}
	return(ret);
}

static REG8 sxsibios_succeed(UINT type, SXSIDEV sxsi) {

	(void)type;
	(void)sxsi;
	return(0x00);
}

static REG8 sxsibios_failed(UINT type, SXSIDEV sxsi) {

	(void)type;
	(void)sxsi;
	return(0x40);
}


// ---- sasi & IDE

static REG8 sasibios_init(UINT type, SXSIDEV sxsi) {

	UINT16	diskequip;
	UINT8	i;
	UINT16	bit;

	diskequip = GETBIOSMEM16(MEMW_DISK_EQUIP);
	diskequip &= 0xf0ff;
	
#if defined(SUPPORT_IDEIO)
	for (i=0x00, bit=0x0100; i<0x04; i++, bit<<=1) {
#else
	for (i=0x00, bit=0x0100; i<0x02; i++, bit<<=1) {
#endif
		sxsi = sxsi_getptr(i);
		if ((sxsi) && (sxsi->flag & SXSIFLAG_READY) && sxsi->devtype==SXSIDEV_HDD) {
			diskequip |= bit;
		}
	}
	SETBIOSMEM16(MEMW_DISK_EQUIP, diskequip);

	(void)type;
	return(0x00);
}

static REG8 sasibios_sense(UINT type, SXSIDEV sxsi) {

	if (type == SXSIBIOS_SASI) {
		return((REG8)(sxsi->mediatype & 7));
	}
	else {
		if (CPU_AH == 0x84) {
			CPU_BX = sxsi->size;
			CPU_CX = sxsi->cylinders;
			CPU_DH = sxsi->surfaces;
			CPU_DL = sxsi->sectors;
		}
		return(0x0f);
	}
}

static const SXSIFUNC sasifunc[16] = {
			sxsibios_failed,		// SASI 0:
			sxsibios_succeed,		// SASI 1: ベリファイ
			sxsibios_failed,		// SASI 2:
			sasibios_init,			// SASI 3: イニシャライズ
			sasibios_sense,			// SASI 4: センス
			sxsibios_write,			// SASI 5: データの書き込み
			sxsibios_read,			// SASI 6: データの読み込み
			sxsibios_succeed,		// SASI 7: リトラクト
			sxsibios_failed,		// SASI 8:
			sxsibios_failed,		// SASI 9:
			sxsibios_failed,		// SASI a:
			sxsibios_failed,		// SASI b:
			sxsibios_failed,		// SASI c:
			sxsibios_format,		// SASI d: フォーマット
			sxsibios_failed,		// SASI e:
			sxsibios_succeed};		// SASI f: リトラクト

REG8 sasibios_operate(void) {

	UINT	type;
	SXSIDEV	sxsi;

	if (pccore.hddif & PCHDD_IDE) {
		type = SXSIBIOS_IDE;
	}
#if defined(SUPPORT_SASI)
	else if (pccore.hddif & PCHDD_SASI) {
		type = SXSIBIOS_SASI;
	}
#endif
	else {
		return(0x60);
	}
	sxsi = sxsi_getptr(CPU_AL);
	if (sxsi == NULL) {
		return(0x60);
	}
	return((*sasifunc[CPU_AH & 0x0f])(type, sxsi));
}


// ---- scsi

#if defined(SUPPORT_SCSI)

static void scsibios_set(REG8 drv, REG8 sectors, REG8 surfaces,
									REG16 cylinders, REG16 size, BOOL hwsec) {

	UINT8	*scsiinf;
	UINT16	inf;

	scsiinf = mem + 0x00460 + ((drv & 7) * 4);
	inf = 0;

	inf = (UINT16)(cylinders & 0xfff);
	if (cylinders >= 0x1000) {
		inf |= 0x4000;
		surfaces |= (cylinders >> 8) & 0xf0;
	}
	if (size == 512) {
		inf |= 0x1000;
	}
	else if (size == 1024) {
		inf |= 0x2000;
	}
	if (hwsec) {
		inf |= 0x8000;
	}
	scsiinf[0] = (UINT8)sectors;
	scsiinf[1] = (UINT8)surfaces;
	STOREINTELWORD(scsiinf + 2, inf);
}

static REG8 scsibios_init(UINT type, SXSIDEV sxsi) {

	UINT8	i;
	UINT8	bit;

	mem[MEMB_DISK_EQUIPS] = 0;
	ZeroMemory(&mem[0x00460], 0x20);
	for (i=0, bit=1; i<4; i++, bit<<=1) {
		sxsi = sxsi_getptr((REG8)(0x20 + i));
		if ((sxsi) && (sxsi->flag & SXSIFLAG_READY)) {
			mem[MEMB_DISK_EQUIPS] |= bit;
			scsibios_set(i, sxsi->sectors, sxsi->surfaces,
							sxsi->cylinders, sxsi->size, TRUE);
		}
	}
	(void)type;
	return(0x00);
}

static REG8 scsibios_sense(UINT type, SXSIDEV sxsi) {

	UINT8	*scsiinf;

	scsiinf = mem + 0x00460 + ((CPU_AL & 7) * 4);
	if (CPU_AH == 0x24) {
		scsibios_set(CPU_AL, CPU_DL, CPU_DH, CPU_CX, CPU_BX, FALSE);
	}
	else if (CPU_AH == 0x44) {
		CPU_BX = (scsiinf[3] & 0x80)?2:1;
	}
	else if (CPU_AH == 0x84) {
		CPU_DL = scsiinf[0];
		CPU_DH = scsiinf[1] & 0x0f;
		CPU_CX = scsiinf[2] + ((scsiinf[3] & 0xf) << 8);
		if (scsiinf[3] & 0x40) {
			CPU_CX += (scsiinf[1] & 0xf0) << 8;
		}
		CPU_BX = 256 << ((scsiinf[3] >> 4) & 3);
	}
	(void)type;
	(void)sxsi;
	return(0x00);
}

static REG8 scsibios_setsec(UINT type, SXSIDEV sxsi) {

	if (sxsi->size != (128 << (CPU_BH & 3))) {
		return(0x40);
	}
	(void)type;
	return(0x00);
}

static REG8 scsibios_chginf(UINT type, SXSIDEV sxsi) {

	CPU_CX = 0;
	(void)type;
	(void)sxsi;
	return(0x00);
}

static const SXSIFUNC scsifunc[16] = {
			sxsibios_failed,		// SCSI 0:
			sxsibios_succeed,		// SCSI 1: ベリファイ
			sxsibios_failed,		// SCSI 2:
			scsibios_init,			// SCSI 3: イニシャライズ
			scsibios_sense,			// SCSI 4: センス
			sxsibios_write,			// SCSI 5: データの書き込み
			sxsibios_read,			// SCSI 6: データの読み込み
			sxsibios_succeed,		// SCSI 7: リトラクト
			sxsibios_failed,		// SCSI 8:
			sxsibios_failed,		// SCSI 9:
			scsibios_setsec,		// SCSI a: セクタ長設定
			sxsibios_failed,		// SCSI b:
			scsibios_chginf,		// SCSI c: 代替情報取得
			sxsibios_format,		// SCSI d: フォーマット
			sxsibios_failed,		// SCSI e:
			sxsibios_succeed};		// SCSI f: リトラクト

REG8 scsibios_operate(void) {

	SXSIDEV	sxsi;

	if (!(pccore.hddif & PCHDD_SCSI)) {
		return(0x60);
	}
	sxsi = sxsi_getptr(CPU_AL);
	if (sxsi == NULL) {
		return(0x60);
	}
	return((*scsifunc[CPU_AH & 0x0f])(SXSIBIOS_SCSI, sxsi));
}


// あとで scsicmdから移動
#endif


// ---- np2sysp

#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI) || defined(SUPPORT_SCSI)
typedef struct {
	UINT16	ax;
	UINT16	cx;
	UINT16	dx;
	UINT16	bx;
	UINT16	bp;
	UINT16	si;
	UINT16	di;
	UINT16	es;
	UINT16	ds;
	UINT16	flag;
} REGBAK;

static void reg_push(REGBAK *r) {

	r->ax = CPU_AX;
	r->cx = CPU_CX;
	r->dx = CPU_DX;
	r->bx = CPU_BX;
	r->bp = CPU_BP;
	r->si = CPU_SI;
	r->di = CPU_DI;
	r->es = CPU_ES;
	r->ds = CPU_DS;
	r->flag = CPU_FLAG;
}

static void reg_pop(const REGBAK *r) {

	CPU_AX = r->ax;
	CPU_CX = r->cx;
	CPU_DX = r->dx;
	CPU_BX = r->bx;
	CPU_BP = r->bp;
	CPU_SI = r->si;
	CPU_DI = r->di;
	CPU_ES = r->es;
	CPU_DS = r->ds;
	CPU_FLAG = r->flag;
}

typedef struct {
	UINT8	r_ax[2];
	UINT8	r_bx[2];
	UINT8	r_cx[2];
	UINT8	r_dx[2];
	UINT8	r_bp[2];
	UINT8	r_es[2];
	UINT8	r_di[2];
	UINT8	r_si[2];
	UINT8	r_ds[2];
} B1BREG;

static void reg_load(UINT seg, UINT off) {

	B1BREG	r;

	MEMR_READS(seg, off, &r, sizeof(r));
	CPU_FLAGL = MEMR_READ8(seg, off + 0x16);
	CPU_AX = LOADINTELWORD(r.r_ax);
	CPU_BX = LOADINTELWORD(r.r_bx);
	CPU_CX = LOADINTELWORD(r.r_cx);
	CPU_DX = LOADINTELWORD(r.r_dx);
	CPU_BP = LOADINTELWORD(r.r_bp);
	CPU_ES = LOADINTELWORD(r.r_es);
	CPU_DI = LOADINTELWORD(r.r_di);
	CPU_SI = LOADINTELWORD(r.r_si);
	CPU_DS = LOADINTELWORD(r.r_ds);
}

static void reg_store(UINT seg, UINT off) {

	B1BREG	r;

	STOREINTELWORD(r.r_ax, CPU_AX);
	STOREINTELWORD(r.r_bx, CPU_BX);
	STOREINTELWORD(r.r_cx, CPU_CX);
	STOREINTELWORD(r.r_dx, CPU_DX);
	STOREINTELWORD(r.r_bp, CPU_BP);
	STOREINTELWORD(r.r_es, CPU_ES);
	STOREINTELWORD(r.r_di, CPU_DI);
	STOREINTELWORD(r.r_si, CPU_SI);
	STOREINTELWORD(r.r_ds, CPU_DS);
	MEMR_WRITES(seg, off, &r, sizeof(r));
	MEMR_WRITE8(seg, off + 0x16, CPU_FLAGL);
}
#endif

#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI)
void np2sysp_sasi(const void *arg1, long arg2) {

	UINT	seg;
	UINT	off;
	REGBAK	regbak;
	REG8	ret;

	seg = CPU_SS;
	off = CPU_BP;
	reg_push(&regbak);
	reg_load(seg, off);

	ret = sasibios_operate();
	CPU_AH = ret;
	CPU_FLAGL &= ~C_FLAG;
	if (ret >= 0x20) {
		CPU_FLAGL |= C_FLAG;
	}

	reg_store(seg, off);
	reg_pop(&regbak);
	(void)arg1;
	(void)arg2;
}
#endif

#if defined(SUPPORT_SCSI)
void np2sysp_scsi(const void *arg1, long arg2) {

	UINT	seg;
	UINT	off;
	REGBAK	regbak;
	REG8	ret;

	seg = CPU_SS;
	off = CPU_BP;
	reg_push(&regbak);
	reg_load(seg, off);

	ret = scsibios_operate();
	CPU_AH = ret;
	CPU_FLAGL &= ~C_FLAG;
	if (ret >= 0x20) {
		CPU_FLAGL |= C_FLAG;
	}

	reg_store(seg, off);
	reg_pop(&regbak);
	(void)arg1;
	(void)arg2;
}

void np2sysp_scsidev(const void *arg1, long arg2) {

	UINT	seg;
	UINT	off;
	REGBAK	regbak;

	seg = CPU_SS;
	off = CPU_BP;
	reg_push(&regbak);
	reg_load(seg, off);

	scsicmd_bios();

	reg_store(seg, off);
	reg_pop(&regbak);
	(void)arg1;
	(void)arg2;
}
#endif

