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
#include	"fdd/sxsi.h"

#if defined(_WIN32) && defined(TRACE)
extern void iptrace_out(void);
#define	SCSICMD_ERR		MessageBox(NULL, "SCSI error", "?", MB_OK);	\
						exit(1);
#else
#define	SCSICMD_ERR
#endif


static const UINT8 hdd_inquiry[0x20] = {
			0x00,0x00,0x02,0x02,0x1c,0x00,0x00,0x18,
			'N', 'E', 'C', 0x20,0x20,0x20,0x20,0x20,
			'N', 'P', '2', '-', 'H', 'D', 'D', 0x20,
			0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};


static UINT scsicmd_datain(SXSIDEV sxsi, UINT8 *cdb) {

	UINT	length;

	switch(cdb[0]) {
		case 0x12:				// Inquiry
			TRACEOUT(("Inquiry"));
			// Logical unit number = cdb[1] >> 5;
			// EVPD = cdb[1] & 1;
			// Page code = cdb[2];
			length = cdb[4];
			if (length) {
				CopyMemory(scsiio.data, hdd_inquiry, min(length, 0x20));
			}
			break;

		default:
			length = 0;
	}
	(void)sxsi;
	return(length);
}






// ----

REG8 scsicmd_negate(REG8 id) {

	scsiio.phase = 0;
	(void)id;
	return(0x85);			// disconnect
}

REG8 scsicmd_select(REG8 id) {

	SXSIDEV	sxsi;

	TRACEOUT(("scsicmd_select"));
	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		TRACEOUT(("LUN = %d", scsiio.reg[SCSICTR_TARGETLUN] & 7));
		return(0x42);
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi) && (sxsi->flag & SXSIFLAG_READY)) {
		scsiio.phase = SCSIPH_COMMAND;
		return(0x8a);			// Transfer Command要求
	}
	return(0x42);				// Timeout
}

REG8 scsicmd_transfer(REG8 id, UINT8 *cdb) {

	SXSIDEV	sxsi;
	UINT	leng;

	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42);
	}

	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (!(sxsi->flag & SXSIFLAG_READY))) {
		return(0x42);
	}

	TRACEOUT(("sel ope code = %.2x", cdb[0]));
	switch(cdb[0]) {
		case 0x00:				// Test Unit Ready
			return(0x16);		// Succeed

		case 0x12:				// Inquiry
			leng = scsicmd_datain(sxsi, cdb);
#if 0
			if (leng > scsiio.transfer) {
				return(0x2b);		// Abort
			}
			else if (leng < scsiio.transfer) {
				return(0x20);		// Pause
			}
#endif
			return(0x16);			// Succeed
	}

	SCSICMD_ERR
	return(0xff);
}

static REG8 scsicmd_cmd(REG8 id) {

	SXSIDEV	sxsi;

	TRACEOUT(("scsicmd_cmd = %.2x", scsiio.cmd[0]));
	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42);
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (!(sxsi->flag & SXSIFLAG_READY))) {
		return(0x42);
	}
	switch(scsiio.cmd[0]) {
		case 0x00:
			return(0x8b);		// Transfer Status要求

		case 0x12:				// inquiry
			scsicmd_datain(sxsi, scsiio.cmd);
			scsiio.phase = SCSIPH_DATAIN;
			return(0x89);		// Transfer Data要求
	}

	SCSICMD_ERR
	return(0xff);
}

BRESULT scsicmd_send(void) {

	switch(scsiio.phase) {
		case SCSIPH_COMMAND:
			scsiio.cmdpos = 0;
			return(SUCCESS);
	}
	return(FAILURE);
}


// ---- BIOS から

static const UINT8 stat2ret[16] = {
				0x40, 0x00, 0x10, 0x00,
				0x20, 0x00, 0x10, 0x00,
				0x30, 0x00, 0x10, 0x00,
				0x20, 0x00, 0x10, 0x00};

static REG8 bios1bc_seltrans(REG8 id) {

	UINT8	cdb[16];
	REG8	ret;

	MEMR_READS(CPU_DS, CPU_DX, cdb, 16);
	scsiio.reg[SCSICTR_TARGETLUN] = cdb[0];
	if ((cdb[1] & 0x0c) == 0x08) {			// OUT
		MEMR_READS(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
	}
	ret = scsicmd_transfer(id, cdb + 4);
	if ((cdb[1] & 0x0c) == 0x04) {			// IN
		MEMR_WRITES(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
	}
	return(ret);
}

void scsicmd_bios(void) {

	UINT8	flag;
	UINT8	ret;
	REG8	stat;
	UINT	cmd;
	REG8	dstid;

	TRACEOUT(("BIOS 1B-C* CPU_AH %.2x", CPU_AH));

	if (CPU_AH & 0x80) {		// エラーぽ
		return;
	}

	flag = MEMR_READ8(CPU_SS, CPU_SP+4) & 0xbe;
	ret = mem[0x0483];
	cmd = CPU_AH & 0x1f;
	dstid = CPU_AL & 7;
	if (ret & 0x80) {
		mem[0x0483] &= 0x7f;
	}
	else if (cmd < 0x18) {
		switch(cmd) {
			case 0x00:		// reset
				stat = 0x00;
				break;

			case 0x03:		// Negate ACK
				stat = scsicmd_negate(dstid);
				break;

			case 0x07:		// Select Without AMN
				stat = scsicmd_select(dstid);
				break;

			case 0x09:		// Select Without AMN and Transfer
				stat = bios1bc_seltrans(dstid);
				break;

			default:
				TRACEOUT(("cmd = %.2x", CPU_AH));
				SCSICMD_ERR
				stat = 0x42;
				break;
		}
		ret = stat2ret[stat >> 4] + (stat & 0x0f);
		TRACEOUT(("BIOS 1B-C* CPU_AH %.2x ret = %.2x", CPU_AH, ret));
		mem[0x0483] = ret;
	}
	else {
		if ((ret ^ cmd) & 0x0f) {
			ret = cmd | 0x80;
		}
		else {
			switch(cmd) {
				case 0x19:		// Data In
					MEMR_WRITES(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
					scsiio.phase = SCSIPH_STATUS;
					stat = 0x8b;
					break;

				case 0x1a:		// Transfer command
					MEMR_READS(CPU_ES, CPU_BX, scsiio.cmd, 12);
					stat = scsicmd_cmd(dstid);
					break;

				case 0x1b:		// Status In
					scsiio.phase = SCSIPH_MSGIN;
					stat = 0x8f;
					break;

				case 0x1f:		// Message In
					scsiio.phase = 0;
					stat = 0x80;
					break;

				default:
					TRACEOUT(("cmd = %.2x", CPU_AH));
					SCSICMD_ERR
					stat = 0x42;
					break;
			}
			ret = stat2ret[stat >> 4] + (stat & 0x0f);
		}
		TRACEOUT(("BIOS 1B-C* CPU_AH %.2x ret = %.2x", CPU_AH, ret));
		mem[0x0483] = ret;
	}
	flag |= ret & Z_FLAG;
	if (ret & 0x80) {
		flag |= C_FLAG;
		ret &= 0x7f;
	}
	CPU_AH = ret;
	MEMR_WRITE8(CPU_SS, CPU_SP + 4, flag);
}
#endif

