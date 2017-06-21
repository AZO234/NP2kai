#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"biosmem.h"
#include	"rsbios.h"


void bios0x0c(void) {

	UINT16	doff;
	UINT16	dseg;
	REG8	flag;
	UINT8	data;
	UINT8	status;
	REG16	pos;
	REG16	cnt;

	doff = GETBIOSMEM16(MEMW_RS_CH0_OFST);
	dseg = GETBIOSMEM16(MEMW_RS_CH0_SEG);

	flag = MEMR_READ8(dseg, doff + R_FLAG);
	data = iocore_inp8(0x30);							// データ引き取り
	status = iocore_inp8(0x32) & 0xfc;					// ステータス
	status |= (iocore_inp8(0x33) & 3);

#if 0
	if (status & 0x38) {
		iocore_out8(0x32, flag | 0x10);
	}
#endif

	if (!(flag & RFLAG_BFULL)) {
		// SI/SO変換
		if (mem[MEMB_RS_S_FLAG] & 0x80) {
			if (data >= 0x20) {
				if (mem[MEMB_RS_S_FLAG] & 0x10) {
					data |= 0x80;
				}
				else {
					data &= 0x7f;
				}
			}
			else if (data == RSCODE_SO) {
				mem[MEMB_RS_S_FLAG] |= 0x10;
				iocore_out8(0x00, 0x20);
				return;
			}
			else if (data == RSCODE_SI) {
				mem[MEMB_RS_S_FLAG] &= ~0x10;
				iocore_out8(0x00, 0x20);
				return;
			}
		}

		// DELコードの扱い
		if (mem[MEMB_RS_D_FLAG] & 0x01) {					// CH0 -> bit0
			if (((data & 0x7f) == 0x7f) && (mem[MEMB_MSW3] & 0x80)) {
				data = 0;
			}
		}
		// データ投棄
		pos = MEMR_READ16(dseg, doff + R_PUTP);
		MEMR_WRITE16(dseg, pos, (UINT16)((data << 8) | status));

		// 次のポインタをストア
		pos = (UINT16)(pos + 2);
		if (pos >= MEMR_READ16(dseg, doff + R_TAILP)) {
			pos = MEMR_READ16(dseg, doff + R_HEADP);
		}
		MEMR_WRITE16(dseg, doff + R_PUTP, pos);

		// カウンタのインクリメント
		cnt = (UINT16)(MEMR_READ16(dseg, doff + R_CNT) + 1);
		MEMR_WRITE16(dseg, doff + R_CNT, cnt);

		// オーバーフローを見張る
		if (pos == MEMR_READ16(dseg, doff + R_GETP)) {
			flag |= RFLAG_BFULL;
		}

		// XOFFを送信？
		if (((flag & (RFLAG_XON | RFLAG_XOFF)) == RFLAG_XON) &&
			(cnt >= MEMR_READ16(dseg, doff + R_XON))) {
			iocore_out8(0x30, RSCODE_XOFF);
			flag |= RFLAG_XOFF;
		}
	}
	else {
		MEMR_WRITE8(dseg, doff + R_CMD,
						(REG8)(MEMR_READ8(dseg, doff + R_CMD) | RFLAG_BOVF));
	}
	MEMR_WRITE8(dseg, doff + R_INT,
						(REG8)(MEMR_READ8(dseg, doff + R_INT) | RINT_INT));
	MEMR_WRITE8(dseg, doff + R_FLAG, flag);
	iocore_out8(0x00, 0x20);
}

