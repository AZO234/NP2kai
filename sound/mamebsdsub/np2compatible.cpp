/**
 * @file	np2compatible.cpp
 * @brief	Implementation of compatibility with old np2 mame opl 
 */

#ifdef USE_MAME_BSD

#include "compiler.h"
#include "pccore.h"
#include "cpucore.h"
#include "ymfm_opl.h"
#include "np2interop.h"
#include "sound.h"

#include "np2ymfm.h"

 // 旧np21wとのステートセーブ互換を維持する　旧→新のみ互換
#include "np2compatible.h"

#if defined(INTPTR_MAX) && defined(INT64_MAX)
#if INTPTR_MAX == INT64_MAX
#define IS_64BIT
#endif
#elif defined(_WIN64)
#define IS_64BIT
#endif

// ステートセーブデータのサイズや位置など
// 64bit版
#define NP2REV97OPL_64_SAVESIZE		14344
#define NP2REV97OPL_64_CHSIZE		528
#define NP2REV97OPL_64_SLOTSIZE		136
#define NP2REV97OPL_64_OFS_ADDRESS	14024
#define NP2REV97OPL_64_OFS_NTS		14030
#define NP2REV97OPL_64_OFS_4OPMODE	285
#define NP2REV97OPL_64_OFS_EXREG	14008
#define NP2REV97OPL_64_OFS_DAM		13976
#define NP2REV97OPL_64_OFS_DVB		13977
#define NP2REV97OPL_64_OFS_RHY		14009
#define NP2REV97OPL_64_OFS_SR7		9792
#define NP2REV97OPL_64_OFS_MULT		15
#define NP2REV97OPL_64_OFS_KSR		12
#define NP2REV97OPL_64_OFS_EGT		49
#define NP2REV97OPL_64_OFS_VIB		100
#define NP2REV97OPL_64_OFS_AM		96
#define NP2REV97OPL_64_OFS_KSL		13
#define NP2REV97OPL_64_OFS_TL		52
#define NP2REV97OPL_64_OFS_AR		0
#define NP2REV97OPL_64_OFS_DR		4
#define NP2REV97OPL_64_OFS_SL		64
#define NP2REV97OPL_64_OFS_RR		8
#define NP2REV97OPL_64_OFS_WS		101
// 32bit版
#define NP2REV97OPL_32_SAVESIZE		14032
#define NP2REV97OPL_32_CHSIZE		512
#define NP2REV97OPL_32_SLOTSIZE		128
#define NP2REV97OPL_32_OFS_ADDRESS	13736
#define NP2REV97OPL_32_OFS_NTS		13742
#define NP2REV97OPL_32_OFS_4OPMODE	269
#define NP2REV97OPL_32_OFS_EXREG	13720
#define NP2REV97OPL_32_OFS_DAM		13688
#define NP2REV97OPL_32_OFS_DVB		13689
#define NP2REV97OPL_32_OFS_RHY		13721
#define NP2REV97OPL_32_OFS_SR7		9504
#define NP2REV97OPL_32_OFS_MULT		15
#define NP2REV97OPL_32_OFS_KSR		12
#define NP2REV97OPL_32_OFS_EGT		41
#define NP2REV97OPL_32_OFS_VIB		92
#define NP2REV97OPL_32_OFS_AM		88
#define NP2REV97OPL_32_OFS_KSL		13
#define NP2REV97OPL_32_OFS_TL		44
#define NP2REV97OPL_32_OFS_AR		0
#define NP2REV97OPL_32_OFS_DR		4
#define NP2REV97OPL_32_OFS_SL		56
#define NP2REV97OPL_32_OFS_RR		8
#define NP2REV97OPL_32_OFS_WS		93

int YMF262FlagLoad_NP2REV97(opl3bsd* chipbsd, void* srcbuf, int size) {
	int NP2REV97OPL_SAVESIZE;
	int NP2REV97OPL_CHSIZE;
	int NP2REV97OPL_SLOTSIZE;
	int NP2REV97OPL_OFS_ADDRESS;
	int NP2REV97OPL_OFS_NTS;
	int NP2REV97OPL_OFS_4OPMODE;
	int NP2REV97OPL_OFS_EXREG;
	int NP2REV97OPL_OFS_DAM;
	int NP2REV97OPL_OFS_DVB;
	int NP2REV97OPL_OFS_RHY;
	int NP2REV97OPL_OFS_SR7;
	int NP2REV97OPL_OFS_MULT;
	int NP2REV97OPL_OFS_KSR;
	int NP2REV97OPL_OFS_EGT;
	int NP2REV97OPL_OFS_VIB;
	int NP2REV97OPL_OFS_AM;
	int NP2REV97OPL_OFS_KSL;
	int NP2REV97OPL_OFS_TL;
	int NP2REV97OPL_OFS_AR;
	int NP2REV97OPL_OFS_DR;
	int NP2REV97OPL_OFS_SL;
	int NP2REV97OPL_OFS_RR;
	int NP2REV97OPL_OFS_WS;

	if (size == NP2REV97OPL_64_SAVESIZE) {
		NP2REV97OPL_SAVESIZE    = NP2REV97OPL_64_SAVESIZE;
		NP2REV97OPL_CHSIZE      = NP2REV97OPL_64_CHSIZE;
		NP2REV97OPL_SLOTSIZE    = NP2REV97OPL_64_SLOTSIZE;
		NP2REV97OPL_OFS_ADDRESS = NP2REV97OPL_64_OFS_ADDRESS;
		NP2REV97OPL_OFS_NTS     = NP2REV97OPL_64_OFS_NTS;
		NP2REV97OPL_OFS_4OPMODE = NP2REV97OPL_64_OFS_4OPMODE;
		NP2REV97OPL_OFS_EXREG   = NP2REV97OPL_64_OFS_EXREG;
		NP2REV97OPL_OFS_DAM     = NP2REV97OPL_64_OFS_DAM;
		NP2REV97OPL_OFS_DVB     = NP2REV97OPL_64_OFS_DVB;
		NP2REV97OPL_OFS_RHY     = NP2REV97OPL_64_OFS_RHY;
		NP2REV97OPL_OFS_SR7     = NP2REV97OPL_64_OFS_SR7;
		NP2REV97OPL_OFS_MULT    = NP2REV97OPL_64_OFS_MULT;
		NP2REV97OPL_OFS_KSR     = NP2REV97OPL_64_OFS_KSR;
		NP2REV97OPL_OFS_EGT     = NP2REV97OPL_64_OFS_EGT;
		NP2REV97OPL_OFS_VIB     = NP2REV97OPL_64_OFS_VIB;
		NP2REV97OPL_OFS_AM      = NP2REV97OPL_64_OFS_AM;
		NP2REV97OPL_OFS_KSL     = NP2REV97OPL_64_OFS_KSL;
		NP2REV97OPL_OFS_TL      = NP2REV97OPL_64_OFS_TL;
		NP2REV97OPL_OFS_AR      = NP2REV97OPL_64_OFS_AR;
		NP2REV97OPL_OFS_DR      = NP2REV97OPL_64_OFS_DR;
		NP2REV97OPL_OFS_SL      = NP2REV97OPL_64_OFS_SL;
		NP2REV97OPL_OFS_RR      = NP2REV97OPL_64_OFS_RR;
		NP2REV97OPL_OFS_WS      = NP2REV97OPL_64_OFS_WS;
	}
	else if (size == NP2REV97OPL_32_SAVESIZE) {
		NP2REV97OPL_SAVESIZE    = NP2REV97OPL_32_SAVESIZE;
		NP2REV97OPL_CHSIZE      = NP2REV97OPL_32_CHSIZE;
		NP2REV97OPL_SLOTSIZE    = NP2REV97OPL_32_SLOTSIZE;
		NP2REV97OPL_OFS_ADDRESS = NP2REV97OPL_32_OFS_ADDRESS;
		NP2REV97OPL_OFS_NTS     = NP2REV97OPL_32_OFS_NTS;
		NP2REV97OPL_OFS_4OPMODE = NP2REV97OPL_32_OFS_4OPMODE;
		NP2REV97OPL_OFS_EXREG   = NP2REV97OPL_32_OFS_EXREG;
		NP2REV97OPL_OFS_DAM     = NP2REV97OPL_32_OFS_DAM;
		NP2REV97OPL_OFS_DVB     = NP2REV97OPL_32_OFS_DVB;
		NP2REV97OPL_OFS_RHY     = NP2REV97OPL_32_OFS_RHY;
		NP2REV97OPL_OFS_SR7     = NP2REV97OPL_32_OFS_SR7;
		NP2REV97OPL_OFS_MULT    = NP2REV97OPL_32_OFS_MULT;
		NP2REV97OPL_OFS_KSR     = NP2REV97OPL_32_OFS_KSR;
		NP2REV97OPL_OFS_EGT     = NP2REV97OPL_32_OFS_EGT;
		NP2REV97OPL_OFS_VIB     = NP2REV97OPL_32_OFS_VIB;
		NP2REV97OPL_OFS_AM      = NP2REV97OPL_32_OFS_AM;
		NP2REV97OPL_OFS_KSL     = NP2REV97OPL_32_OFS_KSL;
		NP2REV97OPL_OFS_TL      = NP2REV97OPL_32_OFS_TL;
		NP2REV97OPL_OFS_AR      = NP2REV97OPL_32_OFS_AR;
		NP2REV97OPL_OFS_DR      = NP2REV97OPL_32_OFS_DR;
		NP2REV97OPL_OFS_SL      = NP2REV97OPL_32_OFS_SL;
		NP2REV97OPL_OFS_RR      = NP2REV97OPL_32_OFS_RR;
		NP2REV97OPL_OFS_WS      = NP2REV97OPL_32_OFS_WS;
	}
	else {
		return 0; // 無効
	}

	ymfm::ymf262& chipcore = chipbsd->m_chip->GetChip();

	// レジスタへセットしていく
	// Expansion Register Set　先にこれを設定しておかないとhiのセットが出来ないので先に設定
	chipcore.write_address_hi(0x5);
	chipcore.write_data(
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_OFS_EXREG) ? 0x01 : 0)
	);
	// 4-Operator Mode Set
	chipcore.write_address_hi(0x4);
	chipcore.write_data(
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE *  0 + NP2REV97OPL_OFS_4OPMODE) ? 0x01 : 0) |
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE *  1 + NP2REV97OPL_OFS_4OPMODE) ? 0x02 : 0) |
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE *  2 + NP2REV97OPL_OFS_4OPMODE) ? 0x04 : 0) |
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE *  9 + NP2REV97OPL_OFS_4OPMODE) ? 0x08 : 0) |
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE * 10 + NP2REV97OPL_OFS_4OPMODE) ? 0x10 : 0) |
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE * 11 + NP2REV97OPL_OFS_4OPMODE) ? 0x20 : 0)
	);
	// Keyboard Split Selection Set
	chipcore.write_address(0x8);
	chipcore.write_data(
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_OFS_NTS))
	);
	// Rhythm Instrument Sel Set
	chipcore.write_address(0xbd);
	chipcore.write_data(
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_OFS_DAM)) |
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_OFS_DVB) << 3) | 
		(*(UINT8*)((char*)srcbuf + NP2REV97OPL_OFS_RHY))
	);
	// Slot Register 7 Set
	for (int i = 0; i < 9; i++) {
		chipcore.write_address(0xC0 + i);
		chipcore.write_data(
			*(UINT32*)((char*)srcbuf + NP2REV97OPL_OFS_SR7 + sizeof(UINT32) * i)
		);
		chipcore.write_address_hi(0xC0 + i);
		chipcore.write_data(
			*(UINT32*)((char*)srcbuf + NP2REV97OPL_OFS_SR7 + sizeof(UINT32) * (9 + i))
		);
	}
	// Slot Register 1 Set
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 6; j++) {
			const int regidx = i * 8 + j;
			const int ch = 3 * i + j % 3;
			const int slot = j / 3;
			for (int h = 0; h < 2; h++) {
				if (h == 0) {
					chipcore.write_address(0x20 + regidx);
				}
				else {
					chipcore.write_address_hi(0x20 + regidx);
				}
				const UINT8 mult = *(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_MULT);
				const UINT8 ksr = *(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_KSR);
				const UINT8 egt = *(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_EGT);
				const UINT8 vib = *(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_VIB);
				const UINT32 am = *(UINT32*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_AM);
				chipcore.write_data(
					(mult / 2) |
					(ksr ? 0 : 0x10) |
					(egt) |
					(vib) |
					(am ? 0x80 : 0)
				);
			}
		}
	}
	// Slot Register 2 Set
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 6; j++) {
			const int regidx = i * 8 + j;
			const int ch = 3 * i + j % 3;
			const int slot = j / 3;
			for (int h = 0; h < 2; h++) {
				if (h == 0) {
					chipcore.write_address(0x40 + regidx);
				}
				else {
					chipcore.write_address_hi(0x40 + regidx);
				}
				const UINT8 ksl = *(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_KSL);
				const UINT32 tl = *(UINT32*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_TL);
				chipcore.write_data(
					(ksl != 31 ? (ksl << 6) : 0) |
					(tl >> 2)
				);
			}
		}
	}
	// Slot Register 3 Set
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 6; j++) {
			const int regidx = i * 8 + j;
			const int ch = 3 * i + j % 3;
			const int slot = j / 3;
			for (int h = 0; h < 2; h++) {
				if (h == 0) {
					chipcore.write_address(0x60 + regidx);
				}
				else {
					chipcore.write_address_hi(0x60 + regidx);
				}
				const UINT32 ar = *(UINT32*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_AR);
				const UINT32 dr = *(UINT32*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_DR);
				chipcore.write_data(
					(ar ? (((ar - 16) << 2) & 0xf0) : 0) |
					(dr ? (((dr - 16) >> 2) & 0x0f) : 0)
				);
			}
		}
	}
	// Slot Register 4 Set
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 6; j++) {
			const int regidx = i * 8 + j;
			const int ch = 3 * i + j % 3;
			const int slot = j / 3;
			for (int h = 0; h < 2; h++) {
				if (h == 0) {
					chipcore.write_address(0x80 + regidx);
				}
				else {
					chipcore.write_address_hi(0x80 + regidx);
				}
				const UINT32 sl = *(UINT32*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_SL);
				const UINT32 rr = *(UINT32*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_RR);
				chipcore.write_data(
					(sl ? (((sl / 16) << 4) & 0xf0) : 0) |
					(rr ? (((rr - 16) >> 2) & 0x0f) : 0)
				);
			}
		}
	}
	// Slot Register 8 Set
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 6; j++) {
			const int regidx = i * 8 + j;
			const int ch = 3 * i + j % 3;
			const int slot = j / 3;
			for (int h = 0; h < 2; h++) {
				if (h == 0) {
					chipcore.write_address(0xE0 + regidx);
				}
				else {
					chipcore.write_address_hi(0xE0 + regidx);
				}
				const UINT8 ws = *(UINT8*)((char*)srcbuf + NP2REV97OPL_CHSIZE * (9 * h + ch) + NP2REV97OPL_SLOTSIZE * slot + NP2REV97OPL_OFS_WS);
				chipcore.write_data(ws);
			}
		}
	}
	// Address Register Set
	chipcore.write_address(*(UINT8*)((char*)srcbuf + NP2REV97OPL_OFS_ADDRESS));

	return size;
}

#endif 
