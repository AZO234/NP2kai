/*
 * BMSIO.C: I-O Bank Memory
 *
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bmsio.h"

		_BMSIOCFG	bmsiocfg = {FALSE, 0x00ec, 0xffff, 0x10};
		_BMSIO		bmsio;
		_BMSIOWORK	bmsiowork;


// ---- internal

void bmsio_setnumbanks(UINT8 num) {
	UINT32 memsize;

	memsize = ((UINT32)num) * 0x20000;
	if (bmsiowork.bmsmemsize != memsize) {
		if (bmsiowork.bmsmem) {
			_MFREE(bmsiowork.bmsmem);
			bmsiowork.bmsmem = NULL;
			bmsiowork.bmsmemsize = 0;
		}
	}
	if (bmsiowork.bmsmem == NULL) {
		if (memsize > 0) {
			bmsiowork.bmsmem = (BYTE *)_MALLOC(memsize, "BMSMEM");
			if (bmsiowork.bmsmem == NULL) {
				num = 0;
				memsize = 0;
			}
		}
	}
	bmsio.cfg.numbanks = num;
	bmsiowork.bmsmemsize = memsize;
}

// ---- I/O

static void IOOUTCALL bmsio_o00ec(UINT port, REG8 dat) {
	UINT8 bank;

	bank=dat;
	bmsio.bank=bank;
	if (bank<bmsio.cfg.numbanks)  {
		bmsio.nomem=0;
	}
	else {
		bmsio.nomem=1;
	}
}

static REG8 IOINPCALL bmsio_i00ec(UINT port) {
	return bmsio.bank;
}

// ---- I/F

/*
ダイアログで設定した内容を動作環境に反映する
	NP2リセット時に呼ばれる(STATSAVEのロード時は呼ばれない)
*/
void bmsio_set(void) {
	bmsio.cfg = bmsiocfg;
}

void bmsio_reset(void) {
	if (bmsio.cfg.enabled) {
		bmsio_setnumbanks(bmsio.cfg.numbanks);
		bmsio_o00ec(0,0);
	}
	else {
		bmsio_setnumbanks(1);
		bmsio_o00ec(0,0);
	}
}

void bmsio_bind(void) {
	if (bmsio.cfg.enabled) {
		iocore_attachout(bmsio.cfg.port, bmsio_o00ec);
		iocore_attachinp(bmsio.cfg.port, bmsio_i00ec);
	}
}

