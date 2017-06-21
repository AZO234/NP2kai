#include	"compiler.h"

#if defined(SUPPORT_SASI)

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sasiio.h"
#include	"fdd/sxsi.h"
#include	"sasibios.res"


enum {
	SASI_IRQ		= 0x09,
	SASI_DMACH		= 0x00,

	SASIPHASE_FREE	= 0,
	SASIPHASE_CMD	= 1,
	SASIPHASE_C2	= 2,
	SASIPHASE_SENSE	= 3,
	SASIPHASE_READ	= 4,
	SASIPHASE_WRITE	= 5,
	SASIPHASE_STAT	= 6,
	SASIPHASE_MSG	= 7,

	SASIOCR_INTE	= 0x01,
	SASIOCR_DMAE	= 0x02,
	SASIOCR_RST		= 0x08,
	SASIOCR_SEL		= 0x20,
	SASIOCR_NRDSW	= 0x40,
	SASIOCR_CHEN	= 0x80,

	SASIISR_INT		= 0x01,
	SASIISR_IXO		= 0x04,
	SASIISR_CXD		= 0x08,
	SASIISR_MSG		= 0x10,
	SASIISR_BSY		= 0x20,
	SASIISR_ACK		= 0x40,
	SASIISR_REQ		= 0x80
};

	_SASIIO		sasiio;


static BRESULT sasiseek(void) {

	SXSIDEV	sxsi;

	sasiio.datpos = 0;
	sasiio.datsize = 0;
	sxsi = sxsi_getptr(sasiio.unit);
	if ((sxsi == NULL) || (sxsi->size != 256) ||
		(sxsi_read(sasiio.unit, sasiio.sector, sasiio.dat, sxsi->size))) {
		return(FAILURE);
	}
	TRACEOUT(("sasi read - drv:%d sec:%x", sasiio.unit, sasiio.sector));
	sasiio.datsize = sxsi->size;
	return(SUCCESS);
}

static BRESULT sasiflash(void) {

	SXSIDEV	sxsi;

	sasiio.datpos = 0;
	sasiio.datsize = 0;
	sxsi = sxsi_getptr(sasiio.unit);
	if ((sxsi == NULL) || (sxsi->size != 256) ||
		(sxsi_write(sasiio.unit, sasiio.sector, sasiio.dat, sxsi->size))) {
		return(FAILURE);
	}
	TRACEOUT(("sasi write - drv:%d sec:%x", sasiio.unit, sasiio.sector));
	return(SUCCESS);
}

static void sasidmac(void) {

	REG8	en;

	if ((sasiio.ocr & SASIOCR_DMAE) &&
		((sasiio.phase == SASIPHASE_READ)
			|| (sasiio.phase == SASIPHASE_WRITE))) {
		en = TRUE;
	}
	else {
		en = FALSE;
	}
	dmac.dmach[SASI_DMACH].ready = en;
	dmac_check();
}

void sasiioint(NEVENTITEM item) {

	sasiio.phase = SASIPHASE_STAT;
	sasiio.isrint = SASIISR_INT;
	if (sasiio.ocr & SASIOCR_INTE) {
		TRACEOUT(("sasi set irq"));
		pic_setirq(SASI_IRQ);
	}
	sasidmac();
	(void)item;
}

static void sasisetstat(REG8 errorcode) {

	sasiio.error = errorcode;
	nevent_set(NEVENT_SASIIO, 4000, sasiioint, NEVENT_ABSOLUTE);
}

static void checkcmd(void) {

	UINT8	unit;

	unit = (sasiio.cmd[1] >> 5) & 1;
	sasiio.unit = unit;
	switch(sasiio.cmd[0]) {
		case 0x00:		// Test Drive Ready
			TRACEOUT(("Test Drive Ready"));
			if (sxsi_getptr(unit)) {
				sasiio.stat = 0x00;
				sasisetstat(0x00);
			}
			else {
				sasiio.stat = 0x02;
				sasisetstat(0x7f);
			}
			break;

		case 0x01:		// Recalibrate
			TRACEOUT(("Recalibrate"));
			if (sxsi_getptr(unit)) {
				sasiio.sector = 0;
				sasiio.stat = 0x00;
				sasisetstat(0x00);
			}
			else {
				sasiio.stat = 0x02;
				sasisetstat(0x7f);
			}
			break;

		case 0x03:		// Request Sense Status
			TRACEOUT(("Request Sense Status"));
			sasiio.phase = SASIPHASE_SENSE;
			sasiio.senspos = 0;
			sasiio.sens[0] = sasiio.error;
			sasiio.sens[1] = (UINT8)((sasiio.unit << 5) + 
									((sasiio.sector >> 16) & 0x1f));
			sasiio.sens[2] = (UINT8)(sasiio.sector >> 8);
			sasiio.sens[3] = (UINT8)sasiio.sector;
			sasiio.error = 0x00;
			sasiio.stat = 0x00;
			break;

		case 0x04:		// Format Drive
			TRACEOUT(("Format Drive"));
			sasiio.sector = 0;
			sasiio.stat = 0;
			sasisetstat(0x0f);
			break;

		case 0x06:		// Format Track
			TRACEOUT(("Format Track"));
			sasiio.sector = (sasiio.cmd[1] & 0x1f) << 16;
			sasiio.sector += (sasiio.cmd[2] << 8);
			sasiio.sector += sasiio.cmd[3];
			sasiio.blocks = sasiio.cmd[4];
			sasiio.stat = 0;
			if (!sxsi_format(unit, sasiio.sector)) {
				sasisetstat(0x00);
			}
			else {
				sasisetstat(0x0f);
			}
			break;

		case 0x08:		// Read Data
			TRACEOUT(("Read Data"));
			sasiio.sector = (sasiio.cmd[1] & 0x1f) << 16;
			sasiio.sector += (sasiio.cmd[2] << 8);
			sasiio.sector += sasiio.cmd[3];
			sasiio.blocks = sasiio.cmd[4];
			sasiio.stat = 0;
			if ((sasiio.blocks != 0) && (sasiseek() == SUCCESS)) {
				sasiio.phase = SASIPHASE_READ;
				sasidmac();
			}
			else {
				sasisetstat(0x0f);
			}
			break;

		case 0x0a:		// Write Data
			TRACEOUT(("Write Data"));
			sasiio.sector = (sasiio.cmd[1] & 0x1f) << 16;
			sasiio.sector += (sasiio.cmd[2] << 8);
			sasiio.sector += sasiio.cmd[3];
			sasiio.blocks = sasiio.cmd[4];
			sasiio.stat = 0;
			if ((sasiio.blocks != 0) && (sasiseek() == SUCCESS)) {
				sasiio.phase = SASIPHASE_WRITE;
				sasidmac();
			}
			else {
				sasisetstat(0x0f);
			}
			break;

		case 0x0b:
			sasiio.sector = (sasiio.cmd[1] & 0x1f) << 16;
			sasiio.sector += (sasiio.cmd[2] << 8);
			sasiio.sector += sasiio.cmd[3];
			sasiio.blocks = sasiio.cmd[4];
			sasiio.stat = 0x00;
			sasisetstat(0x00);
			break;

		case 0xc2:
			sasiio.phase = SASIPHASE_C2;
			sasiio.c2pos = 0;
			sasiio.stat = 0x00;
			break;

		default:
			TRACEOUT(("!!! unknown command %.2x", sasiio.cmd[0]));
			sasisetstat(0x00);
			break;
	}
}

REG8 DMACCALL sasi_dmafunc(REG8 func) {

	switch(func) {
		case DMAEXT_START:
			TRACEOUT(("sasi dma transfer!! %.5x:%.4x",
										dmac.dmach[0].adrs.d,
										dmac.dmach[0].leng.w));
			return(1);

		case DMAEXT_BREAK:
			break;
	}
	return(0);
}

REG8 DMACCALL sasi_dataread(void) {

	REG8	ret;

	if (sasiio.phase == SASIPHASE_READ) {
		ret = sasiio.dat[sasiio.datpos];
		sasiio.datpos++;
		if (sasiio.datpos >= sasiio.datsize) {
			sasiio.blocks--;
			if (sasiio.blocks == 0) {
				sasisetstat(0x00);
			}
			else {
				sasiio.sector++;
				if (sasiseek() != SUCCESS) {
					sasisetstat(0x0f);
				}
			}
		}
	}
	else {
		ret = 0;
	}
	return(ret);
}

void DMACCALL sasi_datawrite(REG8 data) {

	if (sasiio.phase == SASIPHASE_WRITE) {
		sasiio.dat[sasiio.datpos] = data;
		sasiio.datpos++;
		if (sasiio.datpos >= sasiio.datsize) {
			if (sasiflash() != SUCCESS) {
				sasisetstat(0x0f);
			}
			else {
				sasiio.blocks--;
				if (sasiio.blocks == 0) {
					sasisetstat(0x00);
				}
				else {
					sasiio.sector++;
					if (sasiseek() != SUCCESS) {
						sasisetstat(0x0f);
					}
				}
			}
		}
	}
}


// ----

static void IOOUTCALL sasiio_o80(UINT port, REG8 dat) {

	switch(sasiio.phase) {
		case SASIPHASE_FREE:
			if (dat == 1) {
//				TRACEOUT(("select controller - 1"));
				sasiio.phase = SASIPHASE_CMD;
				sasiio.cmdpos = 0;
			}
			break;

		case SASIPHASE_CMD:
//			TRACEOUT(("sasi cmd = %.2x", dat));
			sasiio.cmd[sasiio.cmdpos] = (UINT8)dat;
			sasiio.cmdpos++;
			if (sasiio.cmdpos >= 6) {
				checkcmd();
			}
			break;

		case SASIPHASE_C2:
			sasiio.c2pos++;
			if (sasiio.c2pos >= 10) {
				sasisetstat(0x00);
			}
			break;

		case SASIPHASE_WRITE:
			sasi_datawrite(dat);
			break;
	}
	(void)port;
}

static void IOOUTCALL sasiio_o82(UINT port, REG8 dat) {

	UINT8	oldocr;

	oldocr = sasiio.ocr;
	sasiio.ocr = (UINT8)dat;

	if ((oldocr & SASIOCR_RST) && (!(dat & SASIOCR_RST))) {
		sasiio.phase = SASIPHASE_FREE;
		TRACEOUT(("SASI reset"));
	}
	sasidmac();
	(void)port;
}

static REG8 IOINPCALL sasiio_i80(UINT port) {

	REG8	ret;

	ret = 0;
	switch(sasiio.phase) {
		case SASIPHASE_READ:
			ret = sasi_dataread();
			break;

		case SASIPHASE_STAT:
			if (!sasiio.error) {
				ret = sasiio.stat;
			}
			else {
				ret = 0x02;
			}
			sasiio.phase = SASIPHASE_MSG;
			TRACEOUT(("sasi status: %.2x", ret));
			break;

		case SASIPHASE_MSG:
			sasiio.phase = SASIPHASE_FREE;
			TRACEOUT(("sasi message"));
			break;

		case SASIPHASE_SENSE:
			ret = sasiio.sens[sasiio.senspos];
			sasiio.senspos++;
			if (sasiio.senspos >= 4) {
				sasisetstat(0x00);
			}
			break;
	}
	(void)port;
	return(ret);
}

static REG8 IOINPCALL sasiio_i82(UINT port) {

	REG8	ret;
	SXSIDEV	sxsi;

	if (sasiio.ocr & SASIOCR_NRDSW) {
		ret = sasiio.isrint;
		sasiio.isrint = 0;
		if (sasiio.phase != SASIPHASE_FREE) {
			ret += SASIISR_BSY;
			ret += SASIISR_REQ;
			switch(sasiio.phase) {
				case SASIPHASE_CMD:
					ret += SASIISR_CXD;
					break;

				case SASIPHASE_SENSE:
				case SASIPHASE_READ:
					ret += SASIISR_IXO;
					break;

				case SASIPHASE_STAT:
					ret += SASIISR_CXD + SASIISR_IXO;
					break;

				case SASIPHASE_MSG:
					ret += SASIISR_MSG + SASIISR_CXD + SASIISR_IXO;
					break;
			}
		}
	}
	else {
		ret = 0;
		sxsi = sxsi_getptr(0x00);		// SASI-1
		if (sxsi) {
			ret |= (sxsi->mediatype & 7) << 3;
		}
		else {
			ret |= (7 << 3);
		}
		sxsi = sxsi_getptr(0x01);		// SASI-2
		if (sxsi) {
			ret |= (sxsi->mediatype & 7);
		}
		else {
			ret |= 7;
		}
	}
	(void)port;
	return(ret);
}


// ----

void sasiio_reset(const NP2CFG *pConfig) {

	FILEH	fh;
	UINT	r;

	ZeroMemory(&sasiio, sizeof(sasiio));
	if (pccore.hddif & PCHDD_SASI) {
		dmac_attach(DMADEV_SASI, SASI_DMACH);

		CPU_RAM_D000 &= ~(1 << 0);
		fh = file_open_rb_c(OEMTEXT("sasi.rom"));
		r = 0;
		if (fh != FILEH_INVALID) {
			r = file_read(fh, mem + 0xd0000, 0x1000);
			file_close(fh);
		}
		if (r == 0x1000) {
			TRACEOUT(("load sasi.rom"));
		}
		else {
			CopyMemory(mem + 0xd0000, sasibios, sizeof(sasibios));
			TRACEOUT(("use simulate sasi.rom"));
		}
	}

	(void)pConfig;
}

void sasiio_bind(void) {

	if (pccore.hddif & PCHDD_SASI) {
		iocore_attachout(0x0080, sasiio_o80);
		iocore_attachout(0x0082, sasiio_o82);
		iocore_attachinp(0x0080, sasiio_i80);
		iocore_attachinp(0x0082, sasiio_i82);
	}
}
#endif

