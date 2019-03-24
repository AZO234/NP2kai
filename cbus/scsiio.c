#include	"compiler.h"

#if defined(SUPPORT_SCSI)

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"scsiio.h"
#include	"scsiio.tbl"
#include	"scsicmd.h"
#include	"scsibios.res"


	_SCSIIO		scsiio;

static const UINT8 scsiirq[] = {0x03, 0x05, 0x06, 0x09, 0x0c, 0x0d, 3, 3};


void scsiioint(NEVENTITEM item) {

	TRACEOUT(("scsiioint"));
	if (scsiio.membank & 4) {
		pic_setirq(scsiirq[(scsiio.resent >> 3) & 7]);
		TRACEOUT(("scsi intr"));
	}
	scsiio.auxstatus = 0x80;
	(void)item;
}

static void scsiintr(REG8 status) {

	scsiio.scsistatus = status;
	nevent_set(NEVENT_SCSIIO, 4000, scsiioint, NEVENT_ABSOLUTE);
}

static void scsicmd(REG8 cmd) {

	REG8	ret;
	UINT8	id;

	id = scsiio.reg[SCSICTR_DSTID] & 7;
	switch(cmd) {
		case SCSICMD_RESET:
			scsiintr(SCSISTAT_RESET);
			break;

		case SCSICMD_NEGATE:
			ret = scsicmd_negate(id);
			scsiintr(ret);
			break;

		case SCSICMD_SEL:
			ret = scsicmd_select(id);
			if (ret & 0x80) {
				scsiintr(0x11);
				// で retはどーやって割り込みさせるの？
			}
			else {
				scsiintr(ret);
			}
			break;

		case SCSICMD_SEL_TR:
			ret = scsicmd_transfer(id, scsiio.reg + SCSICTR_CDB);
			if (ret != 0xff) {
				scsiintr(ret);
			}
			break;
	}
}



// ----

static void IOOUTCALL scsiio_occ0(UINT port, REG8 dat) {

	scsiio.port = dat;
	(void)port;
}

static void IOOUTCALL scsiio_occ2(UINT port, REG8 dat) {

	UINT8	bit;

	if (scsiio.port < 0x40) {
		TRACEOUT(("scsi ctrl write %s %.2x", scsictr[scsiio.port], dat));
	}
	if (scsiio.port <= 0x19) {
		scsiio.reg[scsiio.port] = dat;
		if (scsiio.port == SCSICTR_CMD) {
			scsicmd(dat);
		}
		scsiio.port++;
	}
	else {
		switch(scsiio.port) {
			case SCSICTR_MEMBANK:
				scsiio.membank = dat;
				if (!(dat & 0x40)) {
					CopyMemory(mem + 0xd2000, scsiio.bios[0], 0x2000);
				}
				else {
					CopyMemory(mem + 0xd2000, scsiio.bios[1], 0x2000);
				}
				break;

			case 0x3f:
				bit = 1 << (dat & 7);
				if (dat & 8) {
					scsiio.datmap |= bit;
				}
				else {
					if (scsiio.datmap & bit) {
						scsiio.datmap &= ~bit;
						if (bit == (1 << 1)) {
							scsiio.wrdatpos = 0;
						}
						else if (bit == (1 << 5)) {
							scsiio.rddatpos = 0;
						}
					}
				}
				break;
		}
	}
	(void)port;
}

static void IOOUTCALL scsiio_occ4(UINT port, REG8 dat) {

	TRACEOUT(("scsiio_occ4 %.2x", dat));
	(void)port;
	(void)dat;
}

static void IOOUTCALL scsiio_occ6(UINT port, REG8 dat) {

	scsiio.data[scsiio.wrdatpos & 0x7fff] = dat;
	scsiio.wrdatpos++;
	(void)port;
}

static REG8 IOINPCALL scsiio_icc0(UINT port) {

	REG8	ret;

	ret = scsiio.auxstatus;
	scsiio.auxstatus = 0;
	(void)port;
	return(ret);
}

static REG8 IOINPCALL scsiio_icc2(UINT port) {

	REG8	ret;

	switch(scsiio.port) {
		case SCSICTR_STATUS:
			scsiio.port++;
			return(scsiio.scsistatus);

		case SCSICTR_MEMBANK:
			return(scsiio.membank);

		case SCSICTR_MEMWND:
			return(scsiio.memwnd);

		case SCSICTR_RESENT:
			return(scsiio.resent);

		case 0x36:
			return(0);					// ２枚刺しとか…
	}
	if (scsiio.port <= 0x19) {
		ret = scsiio.reg[scsiio.port];
		TRACEOUT(("scsi ctrl read %s %.2x [%.4x:%.4x]",
							scsictr[scsiio.port], ret, CPU_CS, CPU_IP));
		scsiio.port++;
		return(ret);
	}
	(void)port;
	return(0xff);
}

static REG8 IOINPCALL scsiio_icc4(UINT port) {

	TRACEOUT(("scsiio_icc4"));
	(void)port;
	return(0x00);
}

static REG8 IOINPCALL scsiio_icc6(UINT port) {

	REG8	ret;

	ret = scsiio.data[scsiio.rddatpos & 0x7fff];
	scsiio.rddatpos++;
	(void)port;
	return(ret);
}


// ----

void scsiio_reset(const NP2CFG *pConfig) {

	FILEH	fh;
	UINT	r;

	ZeroMemory(&scsiio, sizeof(scsiio));
	if (pccore.hddif & PCHDD_SCSI) {
		scsiio.memwnd = (0xd200 & 0x0e00) >> 9;
		scsiio.resent = (3 << 3) + (7 << 0);

		CPU_RAM_D000 |= (3 << 2);				// ramにする
		fh = file_open_rb_c(OEMTEXT("scsi.rom"));
		r = 0;
		if (fh != FILEH_INVALID) {
			r = file_read(fh, scsiio.bios, 0x4000);
			file_close(fh);
		}
		if (r != 0) { // if (r == 0x4000) {
			TRACEOUT(("load scsi.rom"));
		}
		else {
			ZeroMemory(mem + 0xd2000, 0x4000);
			CopyMemory(scsiio.bios, scsibios, sizeof(scsibios));
			TRACEOUT(("use simulate scsi.rom"));
		}
		CopyMemory(mem + 0xd2000, scsiio.bios[0], 0x2000);
	}

	(void)pConfig;
}

void scsiio_bind(void) {

	if (pccore.hddif & PCHDD_SCSI) {
		iocore_attachout(0x0cc0, scsiio_occ0);
		iocore_attachout(0x0cc2, scsiio_occ2);
		iocore_attachout(0x0cc4, scsiio_occ4);
		iocore_attachout(0x0cc6, scsiio_occ6);
		iocore_attachinp(0x0cc0, scsiio_icc0);
		iocore_attachinp(0x0cc2, scsiio_icc2);
		iocore_attachinp(0x0cc4, scsiio_icc4);
		iocore_attachinp(0x0cc6, scsiio_icc6);
	}
}

#endif

