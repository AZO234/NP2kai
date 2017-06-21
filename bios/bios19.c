#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"biosmem.h"
#include	"rsbios.h"


static const UINT rs_speed[] = {
						// 5MHz
						0x0800, 0x0400, 0x0200, 0x0100,
						0x0080, 0x0040, 0x0020, 0x0010,
						0x0008, 0x0004, 0x0002, 0x0001,
						// 4MHz
						0x0680, 0x0340, 0x01a0, 0x00d0,
						0x0068, 0x0034, 0x001a, 0x000d};


void bios0x19(void) {

	UINT8	speed;
	UINT8	mode;
	RSBIOS	rsb;
	UINT16	doff;
	UINT16	cnt;
	UINT16	dseg;
	UINT8	flag;

	if (CPU_AH < 2) {
		// 通信速度…
		mode = CPU_CH | 0x02;
		speed = CPU_AL;
		if (speed >= 8) {
			speed = 4;						// 1200bps
		}
		if (mem[MEMB_BIOS_FLAG1] & 0x80) {	// 4MHz?
			speed += 12;
		}

#if 1	// NP2では未サポートの為　強行(汗
		mode &= ~1;
#else
		if (mode & 1) {
			if (speed < (12 + 6)) {
				speed += 2;
			}
			else {
				mode &= ~1;
			}
		}
		// シリアルリセット
		iocore_out8(0x32, 0x00);		// dummy instruction
		iocore_out8(0x32, 0x00);		// dummy instruction
		iocore_out8(0x32, 0x00);		// dummy instruction
		iocore_out8(0x32, 0x40);		// reset
		iocore_out8(0x32, mode);		// mode
		iocore_out8(0x32, CPU_CL);	// cmd
#endif
		iocore_out8(0x77, 0xb6);
		iocore_out8(0x75, (UINT8)rs_speed[speed]);
		iocore_out8(0x75, (UINT8)(rs_speed[speed] >> 8));

		ZeroMemory(&rsb, sizeof(rsb));
		rsb.FLAG = (CPU_AH << 4);
		rsb.CMD = CPU_CL;
		sysport.c &= ~7;
		if (!(CPU_CL & RCMD_IR)) {
			rsb.FLAG |= RFLAG_INIT;
			if (CPU_CL & RCMD_RXE) {
				sysport.c |= 1;
				pic.pi[0].imr &= ~PIC_RS232C;
			}
		}

		rsb.STIME = CPU_BH;
		if (!rsb.STIME) {
			rsb.STIME = 0x04;
		}
		rsb.RTIME = CPU_BL;
		if (!rsb.RTIME) {
			rsb.RTIME = 0x40;
		}
		doff = CPU_DI + sizeof(RSBIOS);
		STOREINTELWORD(rsb.HEADP, doff);
		STOREINTELWORD(rsb.PUTP, doff);
		STOREINTELWORD(rsb.GETP, doff);
		doff += CPU_DX;
		STOREINTELWORD(rsb.TAILP, doff);
		cnt = CPU_DX >> 3;
		STOREINTELWORD(rsb.XOFF, cnt);
		cnt += CPU_DX >> 2;
		STOREINTELWORD(rsb.XON, cnt);

		// ポインタ～
		SETBIOSMEM16(MEMW_RS_CH0_OFST, CPU_DI);
		SETBIOSMEM16(MEMW_RS_CH0_SEG, CPU_ES);
		MEMR_WRITES(CPU_ES, CPU_DI, &rsb, sizeof(rsb));

		CPU_AH = 0;
	}
	else if (CPU_AH < 7) {
		doff = GETBIOSMEM16(MEMW_RS_CH0_OFST);
		dseg = GETBIOSMEM16(MEMW_RS_CH0_SEG);
		if ((!doff) && (!dseg)) {
			CPU_AH = 1;
			return;
		}
		flag = MEMR_READ8(dseg, doff + R_FLAG);
		if (!(flag & RFLAG_INIT)) {
			CPU_AH = 1;
			return;
		}
		switch(CPU_AH) {
			case 0x02:
				CPU_CX = MEMR_READ16(dseg, doff + R_CNT);
				break;

			case 0x03:
				iocore_out8(0x30, CPU_AL);
				break;

			case 0x04:
				cnt = MEMR_READ16(dseg, doff + R_CNT);
				if (cnt) {
					UINT16	pos;

					// データ引き取り
					pos = MEMR_READ16(dseg, doff + R_GETP);
					CPU_CX = MEMR_READ16(dseg, pos);

					// 次のポインタをストア
					pos += 2;
					if (pos >= MEMR_READ16(dseg, doff + R_TAILP)) {
						pos = MEMR_READ16(dseg, doff + R_HEADP);
					}
					MEMR_WRITE16(dseg, doff + R_GETP, pos);

					// カウンタをデクリメント
					cnt--;
					MEMR_WRITE16(dseg, doff + R_CNT, cnt);

					// XONを送信？
					if ((flag & RFLAG_XOFF) && 
						(cnt < MEMR_READ16(dseg, doff + R_XOFF))) {
						iocore_out8(0x30, RSCODE_XON);
						flag &= ~RFLAG_XOFF;
					}
					flag &= ~RFLAG_BOVF;
					CPU_AH = 0;
					MEMR_WRITE8(dseg, doff + R_FLAG, flag);
					return;
				}
				else {
					CPU_AH = 3;
				}
				break;

			case 0x05:
				iocore_out8(0x32, CPU_AL);
				if (CPU_AL & RCMD_IR) {
					flag &= ~RFLAG_INIT;
					MEMR_WRITE8(dseg, doff + R_FLAG, flag);
					sysport.c &= ~1;
					pic.pi[0].imr |= PIC_RS232C;
				}
				else if (!(CPU_AL & RCMD_RXE)) {
					sysport.c &= ~1;
					pic.pi[0].imr |= PIC_RS232C;
				}
				else {
					sysport.c |= 1;
					pic.pi[0].imr &= ~PIC_RS232C;
				}
				MEMR_WRITE8(dseg, doff + R_CMD, CPU_AL);
				break;

			case 0x06:
				CPU_CH = iocore_inp8(0x32);
				CPU_CL = iocore_inp8(0x33);
				break;
		}
		CPU_AH = 0;
		if (flag & RFLAG_BOVF) {
			MEMR_WRITE8(dseg, doff + R_FLAG, (UINT8)(flag & (~RFLAG_BOVF)));
			CPU_AH = 2;
		}
	}
	else {
		CPU_AH = 0;
	}
}

