#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memegc.h"
#include	"vram.h"


enum {
	EGCADDR_L		= 0,
	EGCADDR_H		= 1
};
#define	EGCADDR(a)	(a)



static	EGCQUAD		egc_src;
static	EGCQUAD		egc_data;

static const UINT planead[4] = {VRAM_B, VRAM_R, VRAM_G, VRAM_E};


static const UINT8 bytemask_u0[64] =	// dir:right by startbit + (len-1)*8
					{0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
					 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x01,
					 0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0x03, 0x01,
					 0xf0, 0x78, 0x3c, 0x1e, 0x0f, 0x07, 0x03, 0x01,
					 0xf8, 0x7c, 0x3e, 0x1f, 0x0f, 0x07, 0x03, 0x01,
					 0xfc, 0x7e, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01,
					 0xfe, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01,
					 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

static const UINT8 bytemask_u1[8] =		// dir:right by length
					{0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

static const UINT8 bytemask_d0[64] =	// dir:left by startbit + (len-1)*8
					{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
					 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x80,
					 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xc0, 0x80,
					 0x0f, 0x1e, 0x3c, 0x78, 0xf0, 0xe0, 0xc0, 0x80,
					 0x1f, 0x3e, 0x7c, 0xf8, 0xf0, 0xe0, 0xc0, 0x80,
					 0x3f, 0x7e, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80,
					 0x7f, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80,
					 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80};

static const UINT8 bytemask_d1[8] =		// dir:left by length
					{0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};


void egcshift(void) {

	UINT8	src8, dst8;

	egc.remain = LOW12(egc.leng) + 1;
	egc.func = (egc.sft >> 12) & 1;
	if (!egc.func) {
		egc.inptr = egc.buf;
		egc.outptr = egc.buf;
	}
	else {
		egc.inptr = egc.buf + 4096/8 + 3;
		egc.outptr = egc.buf + 4096/8 + 3;
	}
	egc.srcbit = egc.sft & 0x0f;
	egc.dstbit = (egc.sft >> 4) & 0x0f;

	src8 = egc.srcbit & 0x07;
	dst8 = egc.dstbit & 0x07;
	if (src8 < dst8) {

// dir:inc
// ****---4 -------8 --------
// ******-- -4------ --8----- --
// 1st -> data[0] >> (dst - src)
// 2nd -> (data[0] << (8 - (dst - src))) | (data[1] >> (dst - src))

// dir:dec
//          -------- 8------- 6-----**
//      --- -----8-- -----6-- ---*****
// 1st -> data[0] << (dst - src)
// 2nd -> (data[0] >> (8 - (dst - src))) | (data[1] << (dst - src))

		egc.func += 2;
		egc.sft8bitr = dst8 - src8;
		egc.sft8bitl = 8 - egc.sft8bitr;
	}
	else if (src8 > dst8) {

// dir:inc
// ****---4 -------8 --------
// **---4-- -----8-- ------
// 1st -> (data[0] << (src - dst)) | (data[1] >> (8 - (src - dst))
// 2nd -> (data[0] << (src - dst)) | (data[1] >> (8 - (src - dst))

// dir:dec
//          -------- 8------- 3--*****
//             ----- ---8---- ---3--**
// 1st -> (data[0] >> (dst - src)) | (data[-1] << (8 - (src - dst))
// 2nd -> (data[0] >> (dst - src)) | (data[-1] << (8 - (src - dst))

		egc.func += 4;
		egc.sft8bitl = src8 - dst8;
		egc.sft8bitr = 8 - egc.sft8bitl;
	}
	egc.stack = 0;
}


static void MEMCALL egcsftb_upn_sub(UINT ext) {

	if (egc.dstbit >= 8) {
		egc.dstbit -= 8;
		egc.srcmask._b[ext] = 0;
		return;
	}
	if (egc.dstbit) {
		if ((egc.dstbit + egc.remain) >= 8) {
			egc.srcmask._b[ext] = bytemask_u0[egc.dstbit + (7*8)];
			egc.remain -= (8 - egc.dstbit);
			egc.dstbit = 0;
		}
		else {
			egc.srcmask._b[ext] = bytemask_u0[egc.dstbit +
														(egc.remain - 1) * 8];
			egc.remain = 0;
			egc.dstbit = 0;
		}
	}
	else {
		if (egc.remain >= 8) {
			egc.remain -= 8;
		}
		else {
			egc.srcmask._b[ext] = bytemask_u1[egc.remain - 1];
			egc.remain = 0;
		}
	}
	egc_src._b[0][ext] = egc.outptr[0];
	egc_src._b[1][ext] = egc.outptr[4];
	egc_src._b[2][ext] = egc.outptr[8];
	egc_src._b[3][ext] = egc.outptr[12];
	egc.outptr++;
}

static void MEMCALL egcsftb_dnn_sub(UINT ext) {

	if (egc.dstbit >= 8) {
		egc.dstbit -= 8;
		egc.srcmask._b[ext] = 0;
		return;
	}
	if (egc.dstbit) {
		if ((egc.dstbit + egc.remain) >= 8) {
			egc.srcmask._b[ext] = bytemask_d0[egc.dstbit + (7*8)];
			egc.remain -= (8 - egc.dstbit);
			egc.dstbit = 0;
		}
		else {
			egc.srcmask._b[ext] = bytemask_d0[egc.dstbit +
														(egc.remain - 1) * 8];
			egc.remain = 0;
			egc.dstbit = 0;
		}
	}
	else {
		if (egc.remain >= 8) {
			egc.remain -= 8;
		}
		else {
			egc.srcmask._b[ext] = bytemask_d1[egc.remain - 1];
			egc.remain = 0;
		}
	}
	egc_src._b[0][ext] = egc.outptr[0];
	egc_src._b[1][ext] = egc.outptr[4];
	egc_src._b[2][ext] = egc.outptr[8];
	egc_src._b[3][ext] = egc.outptr[12];
	egc.outptr--;
}


// ****---4 -------8 --------
// ******-- -4------ --8----- --
// 1st -> data[0] >> (dst - src)
// 2nd -> (data[0] << (8 - (dst - src))) | (data[1] >> (dst - src))

static void MEMCALL egcsftb_upr_sub(UINT ext) {

	if (egc.dstbit >= 8) {
		egc.dstbit -= 8;
		egc.srcmask._b[ext] = 0;
		return;
	}
	if (egc.dstbit) {
		if ((egc.dstbit + egc.remain) >= 8) {
			egc.srcmask._b[ext] = bytemask_u0[egc.dstbit + (7*8)];
			egc.remain -= (8 - egc.dstbit);
		}
		else {
			egc.srcmask._b[ext] = bytemask_u0[egc.dstbit +
														(egc.remain - 1) * 8];
			egc.remain = 0;
		}
		egc.dstbit = 0;
		egc_src._b[0][ext] = (egc.outptr[0] >> egc.sft8bitr);
		egc_src._b[1][ext] = (egc.outptr[4] >> egc.sft8bitr);
		egc_src._b[2][ext] = (egc.outptr[8] >> egc.sft8bitr);
		egc_src._b[3][ext] = (egc.outptr[12] >> egc.sft8bitr);
	}
	else {
		if (egc.remain >= 8) {
			egc.remain -= 8;
		}
		else {
			egc.srcmask._b[ext] = bytemask_u1[egc.remain - 1];
			egc.remain = 0;
		}
		egc_src._b[0][ext] = (egc.outptr[0] << egc.sft8bitl) |
							(egc.outptr[1] >> egc.sft8bitr);
		egc_src._b[1][ext] = (egc.outptr[4] << egc.sft8bitl) |
							(egc.outptr[5] >> egc.sft8bitr);
		egc_src._b[2][ext] = (egc.outptr[8] << egc.sft8bitl) |
							(egc.outptr[9] >> egc.sft8bitr);
		egc_src._b[3][ext] = (egc.outptr[12] << egc.sft8bitl) |
							(egc.outptr[13] >> egc.sft8bitr);
		egc.outptr++;
	}
}


//          -------- 8------- 6-----**
//      --- -----8-- -----6-- ---*****
// 1st -> data[0] << (dst - src)
// 2nd -> (data[0] >> (8 - (dst - src))) | (data[-1] << (dst - src))

static void MEMCALL egcsftb_dnr_sub(UINT ext) {

	if (egc.dstbit >= 8) {
		egc.dstbit -= 8;
		egc.srcmask._b[ext] = 0;
		return;
	}
	if (egc.dstbit) {
		if ((egc.dstbit + egc.remain) >= 8) {
			egc.srcmask._b[ext] = bytemask_d0[egc.dstbit + (7*8)];
			egc.remain -= (8 - egc.dstbit);
		}
		else {
			egc.srcmask._b[ext] = bytemask_d0[egc.dstbit +
														(egc.remain - 1) * 8];
			egc.remain = 0;
		}
		egc.dstbit = 0;
		egc_src._b[0][ext] = (egc.outptr[0] << egc.sft8bitr);
		egc_src._b[1][ext] = (egc.outptr[4] << egc.sft8bitr);
		egc_src._b[2][ext] = (egc.outptr[8] << egc.sft8bitr);
		egc_src._b[3][ext] = (egc.outptr[12] << egc.sft8bitr);
	}
	else {
		if (egc.remain >= 8) {
			egc.remain -= 8;
		}
		else {
			egc.srcmask._b[ext] = bytemask_d1[egc.remain - 1];
			egc.remain = 0;
		}
		egc.outptr--;
		egc_src._b[0][ext] = (egc.outptr[1] >> egc.sft8bitl) |
							(egc.outptr[0] << egc.sft8bitr);
		egc_src._b[1][ext] = (egc.outptr[5] >> egc.sft8bitl) |
							(egc.outptr[4] << egc.sft8bitr);
		egc_src._b[2][ext] = (egc.outptr[9] >> egc.sft8bitl) |
							(egc.outptr[8] << egc.sft8bitr);
		egc_src._b[3][ext] = (egc.outptr[13] >> egc.sft8bitl) |
							(egc.outptr[12] << egc.sft8bitr);
	}
}


// ****---4 -------8 --------
// **---4-- -----8-- ------
// 1st -> (data[0] << (src - dst)) | (data[1] >> (8 - (src - dst))
// 2nd -> (data[0] << (src - dst)) | (data[1] >> (8 - (src - dst))

static void MEMCALL egcsftb_upl_sub(UINT ext) {

	if (egc.dstbit >= 8) {
		egc.dstbit -= 8;
		egc.srcmask._b[ext] = 0;
		return;
	}
	if (egc.dstbit) {
		if ((egc.dstbit + egc.remain) >= 8) {
			egc.srcmask._b[ext] = bytemask_u0[egc.dstbit + (7*8)];
			egc.remain -= (8 - egc.dstbit);
			egc.dstbit = 0;
		}
		else {
			egc.srcmask._b[ext] = bytemask_u0[egc.dstbit +
														(egc.remain - 1) * 8];
			egc.remain = 0;
			egc.dstbit = 0;
		}
	}
	else {
		if (egc.remain >= 8) {
			egc.remain -= 8;
		}
		else {
			egc.srcmask._b[ext] = bytemask_u1[egc.remain - 1];
			egc.remain = 0;
		}
	}
	egc_src._b[0][ext] = (egc.outptr[0] << egc.sft8bitl) |
						(egc.outptr[1] >> egc.sft8bitr);
	egc_src._b[1][ext] = (egc.outptr[4] << egc.sft8bitl) |
						(egc.outptr[5] >> egc.sft8bitr);
	egc_src._b[2][ext] = (egc.outptr[8] << egc.sft8bitl) |
						(egc.outptr[9] >> egc.sft8bitr);
	egc_src._b[3][ext] = (egc.outptr[12] << egc.sft8bitl) |
						(egc.outptr[13] >> egc.sft8bitr);
	egc.outptr++;
}


//          -------- 8------- 3--*****
//             ----- ---8---- ---3--**
// 1st -> (data[0] >> (dst - src)) | (data[-1] << (8 - (src - dst))
// 2nd -> (data[0] >> (dst - src)) | (data[-1] << (8 - (src - dst))

static void MEMCALL egcsftb_dnl_sub(UINT ext) {

	if (egc.dstbit >= 8) {
		egc.dstbit -= 8;
		egc.srcmask._b[ext] = 0;
		return;
	}
	if (egc.dstbit) {
		if ((egc.dstbit + egc.remain) >= 8) {
			egc.srcmask._b[ext] = bytemask_d0[egc.dstbit + (7*8)];
			egc.remain -= (8 - egc.dstbit);
			egc.dstbit = 0;
		}
		else {
			egc.srcmask._b[ext] = bytemask_d0[egc.dstbit +
														(egc.remain - 1) * 8];
			egc.remain = 0;
			egc.dstbit = 0;
		}
	}
	else {
		if (egc.remain >= 8) {
			egc.remain -= 8;
		}
		else {
			egc.srcmask._b[ext] = bytemask_d1[egc.remain - 1];
			egc.remain = 0;
		}
	}
	egc.outptr--;
	egc_src._b[0][ext] = (egc.outptr[1] >> egc.sft8bitl) |
						(egc.outptr[0] << egc.sft8bitr);
	egc_src._b[1][ext] = (egc.outptr[5] >> egc.sft8bitl) |
						(egc.outptr[4] << egc.sft8bitr);
	egc_src._b[2][ext] = (egc.outptr[9] >> egc.sft8bitl) |
						(egc.outptr[8] << egc.sft8bitr);
	egc_src._b[3][ext] = (egc.outptr[13] >> egc.sft8bitl) |
						(egc.outptr[12] << egc.sft8bitr);
}


static void MEMCALL egcsftb_upn0(UINT ext) {

	if (egc.stack < (UINT)(8 - egc.dstbit)) {
		egc.srcmask._b[ext] = 0;
		return;
	}
	egc.stack -= (8 - egc.dstbit);
	egcsftb_upn_sub(ext);
	if (!egc.remain) {
		egcshift();
	}
}

static void MEMCALL egcsftw_upn0(void) {

	if (egc.stack < (UINT)(16 - egc.dstbit)) {
		egc.srcmask.w = 0;
		return;
	}
	egc.stack -= (16 - egc.dstbit);
	egcsftb_upn_sub(EGCADDR_L);
	if (egc.remain) {
		egcsftb_upn_sub(EGCADDR_H);
		if (egc.remain) {
			return;
		}
	}
	else {
		egc.srcmask._b[EGCADDR_H] = 0;
	}
	egcshift();
}

static void MEMCALL egcsftb_dnn0(UINT ext) {

	if (egc.stack < (UINT)(8 - egc.dstbit)) {
		egc.srcmask._b[ext] = 0;
		return;
	}
	egc.stack -= (8 - egc.dstbit);
	egcsftb_dnn_sub(ext);
	if (!egc.remain) {
		egcshift();
	}
}

static void MEMCALL egcsftw_dnn0(void) {

	if (egc.stack < (UINT)(16 - egc.dstbit)) {
		egc.srcmask.w = 0;
		return;
	}
	egc.stack -= (16 - egc.dstbit);
	egcsftb_dnn_sub(EGCADDR_H);
	if (egc.remain) {
		egcsftb_dnn_sub(EGCADDR_L);
		if (egc.remain) {
			return;
		}
	}
	else {
		egc.srcmask._b[EGCADDR_L] = 0;
	}
	egcshift();
}


static void MEMCALL egcsftb_upr0(UINT ext) {		// dir:up srcbit < dstbit

	if (egc.stack < (UINT)(8 - egc.dstbit)) {
		egc.srcmask._b[ext] = 0;
		return;
	}
	egc.stack -= (8 - egc.dstbit);
	egcsftb_upr_sub(ext);
	if (!egc.remain) {
		egcshift();
	}
}

static void MEMCALL egcsftw_upr0(void) {			// dir:up srcbit < dstbit

	if (egc.stack < (UINT)(16 - egc.dstbit)) {
		egc.srcmask.w = 0;
		return;
	}
	egc.stack -= (16 - egc.dstbit);
	egcsftb_upr_sub(EGCADDR_L);
	if (egc.remain) {
		egcsftb_upr_sub(EGCADDR_H);
		if (egc.remain) {
			return;
		}
	}
	else {
		egc.srcmask._b[EGCADDR_H] = 0;
	}
	egcshift();
}

static void MEMCALL egcsftb_dnr0(UINT ext) {		// dir:up srcbit < dstbit

	if (egc.stack < (UINT)(8 - egc.dstbit)) {
		egc.srcmask._b[ext] = 0;
		return;
	}
	egc.stack -= (8 - egc.dstbit);
	egcsftb_dnr_sub(ext);
	if (!egc.remain) {
		egcshift();
	}
}

static void MEMCALL egcsftw_dnr0(void) {			// dir:up srcbit < dstbit

	if (egc.stack < (UINT)(16 - egc.dstbit)) {
		egc.srcmask.w = 0;
		return;
	}
	egc.stack -= (16 - egc.dstbit);
	egcsftb_dnr_sub(EGCADDR_H);
	if (egc.remain) {
		egcsftb_dnr_sub(EGCADDR_L);
		if (egc.remain) {
			return;
		}
	}
	else {
		egc.srcmask._b[EGCADDR_L] = 0;
	}
	egcshift();
}


static void MEMCALL egcsftb_upl0(UINT ext) {		// dir:up srcbit > dstbit

	if (egc.stack < (UINT)(8 - egc.dstbit)) {
		egc.srcmask._b[ext] = 0;
		return;
	}
	egc.stack -= (8 - egc.dstbit);
	egcsftb_upl_sub(ext);
	if (!egc.remain) {
		egcshift();
	}
}

static void MEMCALL egcsftw_upl0(void) {			// dir:up srcbit > dstbit

	if (egc.stack < (UINT)(16 - egc.dstbit)) {
		egc.srcmask.w = 0;
		return;
	}
	egc.stack -= (16 - egc.dstbit);
	egcsftb_upl_sub(EGCADDR_L);
	if (egc.remain) {
		egcsftb_upl_sub(EGCADDR_H);
		if (egc.remain) {
			return;
		}
	}
	else {
		egc.srcmask._b[EGCADDR_H] = 0;
	}
	egcshift();
}

static void MEMCALL egcsftb_dnl0(UINT ext) {		// dir:up srcbit > dstbit

	if (egc.stack < (UINT)(8 - egc.dstbit)) {
		egc.srcmask._b[ext] = 0;
		return;
	}
	egc.stack -= (8 - egc.dstbit);
	egcsftb_dnl_sub(ext);
	if (!egc.remain) {
		egcshift();
	}
}

static void MEMCALL egcsftw_dnl0(void) {			// dir:up srcbit > dstbit

	if (egc.stack < (UINT)(16 - egc.dstbit)) {
		egc.srcmask.w = 0;
		return;
	}
	egc.stack -= (16 - egc.dstbit);
	egcsftb_dnl_sub(EGCADDR_H);
	if (egc.remain) {
		egcsftb_dnl_sub(EGCADDR_L);
		if (egc.remain) {
			return;
		}
	}
	else {
		egc.srcmask._b[EGCADDR_L] = 0;
	}
	egcshift();
}


typedef void (MEMCALL * EGCSFTB)(UINT ext);
typedef void (MEMCALL * EGCSFTW)(void);

static const EGCSFTB egcsftb[6] = {
		egcsftb_upn0,	egcsftb_dnn0,
		egcsftb_upr0,	egcsftb_dnr0,
		egcsftb_upl0,	egcsftb_dnl0};

static const EGCSFTW egcsftw[6] = {
		egcsftw_upn0,	egcsftw_dnn0,
		egcsftw_upr0,	egcsftw_dnr0,
		egcsftw_upl0,	egcsftw_dnl0};



// ---------------------------------------------------------------------------

static void MEMCALL shiftinput_byte(UINT ext) {

	if (egc.stack <= 16) {
		if (egc.srcbit >= 8) {
			egc.srcbit -= 8;
		}
		else {
			egc.stack += (8 - egc.srcbit);
			egc.srcbit = 0;
		}
		if (!(egc.sft & 0x1000)) {
			egc.inptr++;
		}
		else {
			egc.inptr--;
		}
	}
	egc.srcmask._b[ext] = 0xff;
	(*egcsftb[egc.func])(ext);
}

static void MEMCALL shiftinput_incw(void) {

	if (egc.stack <= 16) {
		egc.inptr += 2;
		if (egc.srcbit >= 8) {
			egc.outptr++;
		}
		egc.stack += (16 - egc.srcbit);
		egc.srcbit = 0;
	}
	egc.srcmask.w = 0xffff;
	(*egcsftw[egc.func])();
}

static void MEMCALL shiftinput_decw(void) {

	if (egc.stack <= 16) {
		egc.inptr -= 2;
		if (egc.srcbit >= 8) {
			egc.outptr--;
		}
		egc.stack += (16 - egc.srcbit);
		egc.srcbit = 0;
	}
	egc.srcmask.w = 0xffff;
	(*egcsftw[egc.func])();
}

#define	EGCOPE_SHIFTB											\
	do {														\
		if (egc.ope & 0x400) {									\
			egc.inptr[ 0] = (UINT8)value;						\
			egc.inptr[ 4] = (UINT8)value;						\
			egc.inptr[ 8] = (UINT8)value;						\
			egc.inptr[12] = (UINT8)value;						\
			shiftinput_byte(EGCADDR(ad & 1));					\
		}														\
	} while(0)

#define	EGCOPE_SHIFTW											\
	do {														\
		if (egc.ope & 0x400) {									\
			if (!(egc.sft & 0x1000)) {							\
				egc.inptr[ 0] = (UINT8)value;					\
				egc.inptr[ 1] = (UINT8)(value >> 8);			\
				egc.inptr[ 4] = (UINT8)value;					\
				egc.inptr[ 5] = (UINT8)(value >> 8);			\
				egc.inptr[ 8] = (UINT8)value;					\
				egc.inptr[ 9] = (UINT8)(value >> 8);			\
				egc.inptr[12] = (UINT8)value;					\
				egc.inptr[13] = (UINT8)(value >> 8);			\
				shiftinput_incw();								\
			}													\
			else {												\
				egc.inptr[-1] = (UINT8)value;					\
				egc.inptr[ 0] = (UINT8)(value >> 8);			\
				egc.inptr[ 3] = (UINT8)value;					\
				egc.inptr[ 4] = (UINT8)(value >> 8);			\
				egc.inptr[ 7] = (UINT8)value;					\
				egc.inptr[ 8] = (UINT8)(value >> 8);			\
				egc.inptr[11] = (UINT8)value;					\
				egc.inptr[12] = (UINT8)(value >> 8);			\
				shiftinput_decw();								\
			}													\
		}														\
	} while(0)

// ----

static const UINT8 data_00[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const UINT8 data_ff[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

static const EGCQUAD * MEMCALL ope_00(REG8 ope, UINT32 ad) {

	(void)ope;
	(void)ad;
	return((EGCQUAD *)data_00);
}

static const EGCQUAD * MEMCALL ope_0f(REG8 ope, UINT32 ad) {

	egc_data.d[0] = ~egc_src.d[0];
	egc_data.d[1] = ~egc_src.d[1];

	(void)ope;
	(void)ad;
	return(&egc_data);
}

static const EGCQUAD * MEMCALL ope_c0(REG8 ope, UINT32 ad) {

	EGCQUAD	dst;

	dst.w[0] = *(UINT16 *)(&mem[ad + VRAM_B]);
	dst.w[1] = *(UINT16 *)(&mem[ad + VRAM_R]);
	dst.w[2] = *(UINT16 *)(&mem[ad + VRAM_G]);
	dst.w[3] = *(UINT16 *)(&mem[ad + VRAM_E]);
	egc_data.d[0] = (egc_src.d[0] & dst.d[0]);
	egc_data.d[1] = (egc_src.d[1] & dst.d[1]);

	(void)ope;
	(void)ad;
	return(&egc_data);
}

static const EGCQUAD * MEMCALL ope_f0(REG8 ope, UINT32 ad) {

	(void)ope;
	(void)ad;
	return(&egc_src);
}

static const EGCQUAD * MEMCALL ope_fc(REG8 ope, UINT32 ad) {

	EGCQUAD	dst;

	dst.w[0] = *(UINT16 *)(&mem[ad + VRAM_B]);
	dst.w[1] = *(UINT16 *)(&mem[ad + VRAM_R]);
	dst.w[2] = *(UINT16 *)(&mem[ad + VRAM_G]);
	dst.w[3] = *(UINT16 *)(&mem[ad + VRAM_E]);
	egc_data.d[0] = egc_src.d[0];
	egc_data.d[0] |= ((~egc_src.d[0]) & dst.d[0]);
	egc_data.d[1] = egc_src.d[1];
	egc_data.d[1] |= ((~egc_src.d[1]) & dst.d[1]);

	(void)ope;
	(void)ad;
	return(&egc_data);
}

static const EGCQUAD * MEMCALL ope_ff(REG8 ope, UINT32 ad) {

	(void)ope;
	(void)ad;
	return((EGCQUAD *)data_ff);
}

static const EGCQUAD * MEMCALL ope_nd(REG8 ope, UINT32 ad) {

	EGCQUAD	pat;

	switch(egc.fgbg & 0x6000) {
		case 0x2000:
			pat.d[0] = egc.bgc.d[0];
			pat.d[1] = egc.bgc.d[1];
			break;

		case 0x4000:
			pat.d[0] = egc.fgc.d[0];
			pat.d[1] = egc.fgc.d[1];
			break;

		default:
			if ((egc.ope & 0x0300) == 0x0100) {
				pat.d[0] = egc_src.d[0];
				pat.d[1] = egc_src.d[1];
			}
			else {
				pat.d[0] = egc.patreg.d[0];
				pat.d[1] = egc.patreg.d[1];
			}
			break;
	}

	egc_data.d[0] = 0;
	egc_data.d[1] = 0;
	if (ope & 0x80) {
		egc_data.d[0] |= (pat.d[0] & egc_src.d[0]);
		egc_data.d[1] |= (pat.d[1] & egc_src.d[1]);
	}
	if (ope & 0x40) {
		egc_data.d[0] |= ((~pat.d[0]) & egc_src.d[0]);
		egc_data.d[1] |= ((~pat.d[1]) & egc_src.d[1]);
	}
	if (ope & 0x08) {
		egc_data.d[0] |= (pat.d[0] & (~egc_src.d[0]));
		egc_data.d[1] |= (pat.d[1] & (~egc_src.d[1]));
	}
	if (ope & 0x04) {
		egc_data.d[0] |= ((~pat.d[0]) & (~egc_src.d[0]));
		egc_data.d[1] |= ((~pat.d[1]) & (~egc_src.d[1]));
	}
	(void)ad;
	return(&egc_data);
}

static const EGCQUAD * MEMCALL ope_np(REG8 ope, UINT32 ad) {

	EGCQUAD	dst;

	dst.w[0] = *(UINT16 *)(&mem[ad + VRAM_B]);
	dst.w[1] = *(UINT16 *)(&mem[ad + VRAM_R]);
	dst.w[2] = *(UINT16 *)(&mem[ad + VRAM_G]);
	dst.w[3] = *(UINT16 *)(&mem[ad + VRAM_E]);

	egc_data.d[0] = 0;
	egc_data.d[1] = 0;
	if (ope & 0x80) {
		egc_data.d[0] |= (egc_src.d[0] & dst.d[0]);
		egc_data.d[1] |= (egc_src.d[1] & dst.d[1]);
	}
	if (ope & 0x20) {
		egc_data.d[0] |= (egc_src.d[0] & (~dst.d[0]));
		egc_data.d[1] |= (egc_src.d[1] & (~dst.d[1]));
	}
	if (ope & 0x08) {
		egc_data.d[0] |= ((~egc_src.d[0]) & dst.d[0]);
		egc_data.d[1] |= ((~egc_src.d[1]) & dst.d[1]);
	}
	if (ope & 0x02) {
		egc_data.d[0] |= ((~egc_src.d[0]) & (~dst.d[0]));
		egc_data.d[1] |= ((~egc_src.d[1]) & (~dst.d[1]));
	}
	return(&egc_data);
}

static const EGCQUAD * MEMCALL ope_xx(REG8 ope, UINT32 ad) {

	EGCQUAD	pat;
	EGCQUAD	dst;

	switch(egc.fgbg & 0x6000) {
		case 0x2000:
			pat.d[0] = egc.bgc.d[0];
			pat.d[1] = egc.bgc.d[1];
			break;

		case 0x4000:
			pat.d[0] = egc.fgc.d[0];
			pat.d[1] = egc.fgc.d[1];
			break;

		default:
			if ((egc.ope & 0x0300) == 0x0100) {
				pat.d[0] = egc_src.d[0];
				pat.d[1] = egc_src.d[1];
			}
			else {
				pat.d[0] = egc.patreg.d[0];
				pat.d[1] = egc.patreg.d[1];
			}
			break;
	}
	dst.w[0] = *(UINT16 *)(&mem[ad + VRAM_B]);
	dst.w[1] = *(UINT16 *)(&mem[ad + VRAM_R]);
	dst.w[2] = *(UINT16 *)(&mem[ad + VRAM_G]);
	dst.w[3] = *(UINT16 *)(&mem[ad + VRAM_E]);

	egc_data.d[0] = 0;
	egc_data.d[1] = 0;
	if (ope & 0x80) {
		egc_data.d[0] |= (pat.d[0] & egc_src.d[0] & dst.d[0]);
		egc_data.d[1] |= (pat.d[1] & egc_src.d[1] & dst.d[1]);
	}
	if (ope & 0x40) {
		egc_data.d[0] |= ((~pat.d[0]) & egc_src.d[0] & dst.d[0]);
		egc_data.d[1] |= ((~pat.d[1]) & egc_src.d[1] & dst.d[1]);
	}
	if (ope & 0x20) {
		egc_data.d[0] |= (pat.d[0] & egc_src.d[0] & (~dst.d[0]));
		egc_data.d[1] |= (pat.d[1] & egc_src.d[1] & (~dst.d[1]));
	}
	if (ope & 0x10) {
		egc_data.d[0] |= ((~pat.d[0]) & egc_src.d[0] & (~dst.d[0]));
		egc_data.d[1] |= ((~pat.d[1]) & egc_src.d[1] & (~dst.d[1]));
	}
	if (ope & 0x08) {
		egc_data.d[0] |= (pat.d[0] & (~egc_src.d[0]) & dst.d[0]);
		egc_data.d[1] |= (pat.d[1] & (~egc_src.d[1]) & dst.d[1]);
	}
	if (ope & 0x04) {
		egc_data.d[0] |= ((~pat.d[0]) & (~egc_src.d[0]) & dst.d[0]);
		egc_data.d[1] |= ((~pat.d[1]) & (~egc_src.d[1]) & dst.d[1]);
	}
	if (ope & 0x02) {
		egc_data.d[0] |= (pat.d[0] & (~egc_src.d[0]) & (~dst.d[0]));
		egc_data.d[1] |= (pat.d[1] & (~egc_src.d[1]) & (~dst.d[1]));
	}
	if (ope & 0x01) {
		egc_data.d[0] |= ((~pat.d[0]) & (~egc_src.d[0]) & (~dst.d[0]));
		egc_data.d[1] |= ((~pat.d[1]) & (~egc_src.d[1]) & (~dst.d[1]));
	}
	return(&egc_data);
}

typedef const EGCQUAD * (MEMCALL * OPEFN)(REG8 ope, UINT32 ad);

static const OPEFN opefn[256] = {
			ope_00, ope_xx, ope_xx, ope_np, ope_xx, ope_nd, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_nd, ope_xx, ope_np, ope_xx, ope_xx, ope_0f,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_np, ope_xx, ope_xx, ope_np, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_np, ope_xx, ope_xx, ope_np,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_nd, ope_xx, ope_xx, ope_xx, ope_xx, ope_nd, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_nd, ope_xx, ope_xx, ope_xx, ope_xx, ope_nd,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_nd, ope_xx, ope_xx, ope_xx, ope_xx, ope_nd, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_nd, ope_xx, ope_xx, ope_xx, ope_xx, ope_nd,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_c0, ope_xx, ope_xx, ope_np, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_np, ope_xx, ope_xx, ope_np,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx, ope_xx,
			ope_f0, ope_xx, ope_xx, ope_np, ope_xx, ope_nd, ope_xx, ope_xx,
			ope_xx, ope_xx, ope_nd, ope_xx, ope_fc, ope_xx, ope_xx, ope_ff};


// ----

static const EGCQUAD * MEMCALL egc_opeb(UINT32 ad, REG8 value) {

	UINT	tmp;

	egc.mask2.w = egc.mask.w;
	switch(egc.ope & 0x1800) {
		case 0x0800:
			EGCOPE_SHIFTB;
			egc.mask2.w &= egc.srcmask.w;
			tmp = egc.ope & 0xff;
			return((*opefn[tmp])((REG8)tmp, ad & (~1)));

		case 0x1000:
			switch(egc.fgbg & 0x6000) {
				case 0x2000:
					return(&egc.bgc);

				case 0x4000:
					return(&egc.fgc);

				default:
					EGCOPE_SHIFTB;
					egc.mask2.w &= egc.srcmask.w;
					return(&egc_src);
			}
			break;

		default:
			tmp = value & 0xff;
			tmp = tmp | (tmp << 8);
			egc_data.w[0] = (UINT16)tmp;
			egc_data.w[1] = (UINT16)tmp;
			egc_data.w[2] = (UINT16)tmp;
			egc_data.w[3] = (UINT16)tmp;
			return(&egc_data);
	}
}

static const EGCQUAD * MEMCALL egc_opew(UINT32 ad, REG16 value) {

	UINT	tmp;

	egc.mask2.w = egc.mask.w;
	switch(egc.ope & 0x1800) {
		case 0x0800:
			EGCOPE_SHIFTW;
			egc.mask2.w &= egc.srcmask.w;
			tmp = egc.ope & 0xff;
			return((*opefn[tmp])((REG8)tmp, ad));

		case 0x1000:
			switch(egc.fgbg & 0x6000) {
				case 0x2000:
					return(&egc.bgc);

				case 0x4000:
					return(&egc.fgc);

				default:
					EGCOPE_SHIFTW;
					egc.mask2.w &= egc.srcmask.w;
					return(&egc_src);
			}
			break;

		default:
#if defined(BYTESEX_BIG)
			value = ((value >> 8) & 0xff) | ((value & 0xff) << 8);
#endif
			egc_data.w[0] = (UINT16)value;
			egc_data.w[1] = (UINT16)value;
			egc_data.w[2] = (UINT16)value;
			egc_data.w[3] = (UINT16)value;
			return(&egc_data);
	}
}


// ----

REG8 MEMCALL egc_readbyte(UINT32 addr) {

	UINT32	ad;
	UINT	ext;

	if (gdcs.access) {
		addr += VRAM_STEP;
	}
	ad = VRAMADDRMASKEX(addr);
	ext = EGCADDR(addr & 1);
	egc.lastvram._b[0][ext] = mem[ad + VRAM_B];
	egc.lastvram._b[1][ext] = mem[ad + VRAM_R];
	egc.lastvram._b[2][ext] = mem[ad + VRAM_G];
	egc.lastvram._b[3][ext] = mem[ad + VRAM_E];

	// shift input
	if (!(egc.ope & 0x400)) {
		egc.inptr[0] = egc.lastvram._b[0][ext];
		egc.inptr[4] = egc.lastvram._b[1][ext];
		egc.inptr[8] = egc.lastvram._b[2][ext];
		egc.inptr[12] = egc.lastvram._b[3][ext];
		shiftinput_byte(ext);
	}

	if ((egc.ope & 0x0300) == 0x0100) {
		egc.patreg._b[0][ext] = mem[ad + VRAM_B];
		egc.patreg._b[1][ext] = mem[ad + VRAM_R];
		egc.patreg._b[2][ext] = mem[ad + VRAM_G];
		egc.patreg._b[3][ext] = mem[ad + VRAM_E];
	}
	if (!(egc.ope & 0x2000)) {
		int pl = (egc.fgbg >> 8) & 3;
		if (!(egc.ope & 0x400)) {
			return(egc_src._b[pl][ext]);
		}
		else {
			return(mem[ad + planead[pl]]);
		}
	}
	return(mem[addr]);
}


void MEMCALL egc_writebyte(UINT32 addr, REG8 value) {

	UINT		ext;
const EGCQUAD	*data;

	addr = LOW15(addr);
	ext = EGCADDR(addr & 1);
	if (!gdcs.access) {
		gdcs.grphdisp |= 1;
		vramupdate[addr] |= 0x01;
	}
	else {
		gdcs.grphdisp |= 2;
		vramupdate[addr] |= 0x02;
		addr += VRAM_STEP;
	}
	if ((egc.ope & 0x0300) == 0x0200) {
		egc.patreg._b[0][ext] = mem[addr + VRAM_B];
		egc.patreg._b[1][ext] = mem[addr + VRAM_R];
		egc.patreg._b[2][ext] = mem[addr + VRAM_G];
		egc.patreg._b[3][ext] = mem[addr + VRAM_E];
	}

	data = egc_opeb(addr, value);
	if (egc.mask2._b[ext]) {
		if (!(egc.access & 1)) {
			mem[addr + VRAM_B] &= ~egc.mask2._b[ext];
			mem[addr + VRAM_B] |= data->_b[0][ext] & egc.mask2._b[ext];
		}
		if (!(egc.access & 2)) {
			mem[addr + VRAM_R] &= ~egc.mask2._b[ext];
			mem[addr + VRAM_R] |= data->_b[1][ext] & egc.mask2._b[ext];
		}
		if (!(egc.access & 4)) {
			mem[addr + VRAM_G] &= ~egc.mask2._b[ext];
			mem[addr + VRAM_G] |= data->_b[2][ext] & egc.mask2._b[ext];
		}
		if (!(egc.access & 8)) {
			mem[addr + VRAM_E] &= ~egc.mask2._b[ext];
			mem[addr + VRAM_E] |= data->_b[3][ext] & egc.mask2._b[ext];
		}
	}
}

REG16 MEMCALL egc_readword(UINT32 addr) {

	UINT32	ad;

	__ASSERT(!(addr & 1));
	if (gdcs.access) {
		addr += VRAM_STEP;
	}
	ad = VRAMADDRMASKEX(addr);
	egc.lastvram.w[0] = *(UINT16 *)(&mem[ad + VRAM_B]);
	egc.lastvram.w[1] = *(UINT16 *)(&mem[ad + VRAM_R]);
	egc.lastvram.w[2] = *(UINT16 *)(&mem[ad + VRAM_G]);
	egc.lastvram.w[3] = *(UINT16 *)(&mem[ad + VRAM_E]);

	// shift input
	if (!(egc.ope & 0x400)) {
		if (!(egc.sft & 0x1000)) {
			egc.inptr[ 0] = egc.lastvram._b[0][EGCADDR_L];
			egc.inptr[ 1] = egc.lastvram._b[0][EGCADDR_H];
			egc.inptr[ 4] = egc.lastvram._b[1][EGCADDR_L];
			egc.inptr[ 5] = egc.lastvram._b[1][EGCADDR_H];
			egc.inptr[ 8] = egc.lastvram._b[2][EGCADDR_L];
			egc.inptr[ 9] = egc.lastvram._b[2][EGCADDR_H];
			egc.inptr[12] = egc.lastvram._b[3][EGCADDR_L];
			egc.inptr[13] = egc.lastvram._b[3][EGCADDR_H];
			shiftinput_incw();
		}
		else {
			egc.inptr[-1] = egc.lastvram._b[0][EGCADDR_L];
			egc.inptr[ 0] = egc.lastvram._b[0][EGCADDR_H];
			egc.inptr[ 3] = egc.lastvram._b[1][EGCADDR_L];
			egc.inptr[ 4] = egc.lastvram._b[1][EGCADDR_H];
			egc.inptr[ 7] = egc.lastvram._b[2][EGCADDR_L];
			egc.inptr[ 8] = egc.lastvram._b[2][EGCADDR_H];
			egc.inptr[11] = egc.lastvram._b[3][EGCADDR_L];
			egc.inptr[12] = egc.lastvram._b[3][EGCADDR_H];
			shiftinput_decw();
		}
	}

	if ((egc.ope & 0x0300) == 0x0100) {
		egc.patreg.d[0] = egc.lastvram.d[0];
		egc.patreg.d[1] = egc.lastvram.d[1];
	}
	if (!(egc.ope & 0x2000)) {
		int pl = (egc.fgbg >> 8) & 3;
		if (!(egc.ope & 0x400)) {
			return(LOADINTELWORD(egc_src._b[pl]));
		}
		else {
			return(LOADINTELWORD(mem + ad + planead[pl]));
		}
	}
	return(LOADINTELWORD(mem + addr));
}

void MEMCALL egc_writeword(UINT32 addr, REG16 value) {

const EGCQUAD	*data;

	__ASSERT(!(addr & 1));
	addr = LOW15(addr);
	if (!gdcs.access) {
		gdcs.grphdisp |= 1;
		*(UINT16 *)(vramupdate + addr) |= 0x0101;
	}
	else {
		gdcs.grphdisp |= 2;
		*(UINT16 *)(vramupdate + addr) |= 0x0202;
		addr += VRAM_STEP;
	}
	if ((egc.ope & 0x0300) == 0x0200) {
		egc.patreg.w[0] = *(UINT16 *)(&mem[addr + VRAM_B]);
		egc.patreg.w[1] = *(UINT16 *)(&mem[addr + VRAM_R]);
		egc.patreg.w[2] = *(UINT16 *)(&mem[addr + VRAM_G]);
		egc.patreg.w[3] = *(UINT16 *)(&mem[addr + VRAM_E]);
	}
	data = egc_opew(addr, value);
	if (egc.mask2.w) {
		if (!(egc.access & 1)) {
			*(UINT16 *)(&mem[addr + VRAM_B]) &= ~egc.mask2.w;
			*(UINT16 *)(&mem[addr + VRAM_B]) |= data->w[0] & egc.mask2.w;
		}
		if (!(egc.access & 2)) {
			*(UINT16 *)(&mem[addr + VRAM_R]) &= ~egc.mask2.w;
			*(UINT16 *)(&mem[addr + VRAM_R]) |= data->w[1] & egc.mask2.w;
		}
		if (!(egc.access & 4)) {
			*(UINT16 *)(&mem[addr + VRAM_G]) &= ~egc.mask2.w;
			*(UINT16 *)(&mem[addr + VRAM_G]) |= data->w[2] & egc.mask2.w;
		}
		if (!(egc.access & 8)) {
			*(UINT16 *)(&mem[addr + VRAM_E]) &= ~egc.mask2.w;
			*(UINT16 *)(&mem[addr + VRAM_E]) |= data->w[3] & egc.mask2.w;
		}
	}
}


// ----

REG8 MEMCALL memegc_rd8(UINT32 addr) {

	CPU_REMCLOCK -= MEMWAIT_GRCG;
	return(egc_readbyte(addr));
}

void MEMCALL memegc_wr8(UINT32 addr, REG8 value) {

	CPU_REMCLOCK -= MEMWAIT_GRCG;
	egc_writebyte(addr, value);
}

REG16 MEMCALL memegc_rd16(UINT32 addr) {

	CPU_REMCLOCK -= MEMWAIT_GRCG;
	if (!(addr & 1)) {
		return(egc_readword(addr));
	}
	else if (!(egc.sft & 0x1000)) {
		REG16 ret;
		ret = egc_readbyte(addr);
		ret |= egc_readbyte(addr+1) << 8;
		return(ret);
	}
	else {
		REG16 ret;
		ret = egc_readbyte(addr+1) << 8;
		ret |= egc_readbyte(addr);
		return(ret);
	}
}

void MEMCALL memegc_wr16(UINT32 addr, REG16 value) {

	CPU_REMCLOCK -= MEMWAIT_GRCG;
	if (!(addr & 1)) {
		egc_writeword(addr, value);
	}
	else if (!(egc.sft & 0x1000)) {
		egc_writebyte(addr, (REG8)value);
		egc_writebyte(addr+1, (REG8)(value >> 8));
	}
	else {
		egc_writebyte(addr+1, (REG8)(value >> 8));
		egc_writebyte(addr, (REG8)value);
	}
}
