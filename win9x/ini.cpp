/**
 *	@file	ini.cpp
 *	@brief	設定ファイル アクセスの動作の定義を行います
 */

#include "compiler.h"
#include "strres.h"
#include "profile.h"
#include "np2.h"
#include "np2arg.h"
#include "dosio.h"
#if defined(SUPPORT_BMS)
#include "bmsio.h"
#endif
#include "ini.h"
#include "winkbd.h"
#include "pccore.h"

// ---- user type

/**
 * 16ビット配列を読み込む
 * @param[in] lpString 文字列
 * @param[out] ini 設定テーブル
 */
static void inirdargs16(LPCTSTR lpString, const PFTBL* ini)
{
	SINT16* lpDst = static_cast<SINT16*>(ini->value);
	const int nCount = ini->arg;

	for (int i = 0; i < nCount; i++)
	{
		while (*lpString == ' ')
		{
			lpString++;
		}

		if (*lpString == '\0')
		{
			break;
		}

		lpDst[i] = static_cast<SINT16>(milstr_solveINT(lpString));
		while (*lpString != '\0')
		{
			const TCHAR c = *lpString++;
			if (c == ',')
			{
				break;
			}
		}
	}
}

/**
 * 3バイトを読み込む
 * @param[in] lpString 文字列
 * @param[out] ini 設定テーブル
 */
static void inirdbyte3(LPCTSTR lpString, const PFTBL* ini)
{
	for (int i = 0; i < 3; i++)
	{
		const TCHAR c = lpString[i];
		if (c == '\0')
		{
			break;
		}
		if ((((c - '0') & 0xff) < 9) || (((c - 'A') & 0xdf) < 26))
		{
			(static_cast<UINT8*>(ini->value))[i] = static_cast<UINT8>(c);
		}
	}
}

/**
 * キーボード設定を読み込む
 * @param[in] lpString 文字列
 * @param[out] ini 設定テーブル
 */
static void inirdkb(LPCTSTR lpString, const PFTBL* ini)
{
	if ((!milstr_extendcmp(lpString, TEXT("PC98"))) || (!milstr_cmp(lpString, TEXT("98"))))
	{
		*(static_cast<UINT8*>(ini->value)) = KEY_PC98;
	}
	else if ((!milstr_extendcmp(lpString, TEXT("DOS"))) || (!milstr_cmp(lpString, TEXT("PCAT"))) || (!milstr_cmp(lpString, TEXT("AT"))))
	{
		*(static_cast<UINT8*>(ini->value)) = KEY_KEY106;
	}
	else if ((!milstr_extendcmp(lpString, TEXT("KEY101"))) || (!milstr_cmp(lpString, TEXT("101"))))
	{
		*(static_cast<UINT8*>(ini->value)) = KEY_KEY101;
	}
}


// ---- Use WinAPI

#if !defined(_UNICODE)
/**
 * ビットを設定
 * @param[in,out] lpBuffer バッファ
 * @param[in] nPos 位置
 * @param[in] set セット or クリア
 */
static void bitmapset(void* lpBuffer, UINT nPos, BOOL set)
{
	const int nIndex = (nPos >> 3);
	const UINT8 cBit = 1 << (nPos & 7);
	if (set)
	{
		(static_cast<UINT8*>(lpBuffer))[nIndex] |= cBit;
	}
	else
	{
		(static_cast<UINT8*>(lpBuffer))[nIndex] &= ~cBit;
	}
}

/**
 * ビットを得る
 * @param[in] lpBuffer バッファ
 * @param[in] nPos 位置
 * @return ビット
 */
static BOOL bitmapget(const void* lpBuffer, UINT nPos)
{
	const int nIndex = (nPos >> 3);
	const UINT8 cBit = 1 << (nPos & 7);
	return (((static_cast<const UINT8*>(lpBuffer))[nIndex]) & cBit) ? TRUE : FALSE;
}

/**
 * バイナリをアンシリアライズ
 * @param[out] lpBin バイナリ
 * @param[in] cbBin バイナリのサイズ
 * @param[in] lpString 文字列バッファ
 */
static void binset(void* lpBin, UINT cbBin, LPCTSTR lpString)
{
	for (UINT i = 0; i < cbBin; i++)
	{
		while (*lpString == ' ')
		{
			lpString++;
		}

		LPTSTR lpStringEnd;
		const long lVal = _tcstol(lpString, &lpStringEnd, 16);
		if (lpString == lpStringEnd)
		{
			break;
		}

		(static_cast<UINT8*>(lpBin))[i] = static_cast<UINT8>(lVal);
		lpString = lpStringEnd;
	}
}

/**
 * バイナリをシリアライズ
 * @param[out] lpString 文字列バッファ
 * @param[in] cchString 文字列バッファ長
 * @param[in] lpBin バイナリ
 * @param[in] cbBin バイナリのサイズ
 */
static void binget(LPTSTR lpString, int cchString, const void* lpBin, UINT cbBin)
{

	if (cbBin)
	{
		TCHAR tmp[8];
		wsprintf(tmp, TEXT("%.2x"), (static_cast<const UINT8*>(lpBin))[0]);
		milstr_ncpy(lpString, tmp, cchString);
	}
	for (UINT i = 1; i < cbBin; i++)
	{
		TCHAR tmp[8];
		wsprintf(tmp, TEXT(" %.2x"), (static_cast<const UINT8*>(lpBin))[i]);
		milstr_ncat(lpString, tmp, cchString);
	}
}

/**
 * 設定読み出し
 * @param[in] lpPath パス
 * @param[in] lpTitle タイトル
 * @param[in] lpTable 設定テーブル
 * @param[in] nCount 設定テーブル アイテム数
 */
void ini_read(LPCTSTR lpPath, LPCTSTR lpTitle, const PFTBL* lpTable, UINT nCount)
{
	const PFTBL* p = lpTable;
	const PFTBL* pTerminate = p + nCount;
	while (p < pTerminate)
	{
		TCHAR szWork[512];
		UINT32 val;
		switch (p->itemtype & PFTYPE_MASK)
		{
			case PFTYPE_STR:
				GetPrivateProfileString(lpTitle, p->item, static_cast<LPCTSTR>(p->value), static_cast<LPTSTR>(p->value), p->arg, lpPath);
				break;

			case PFTYPE_BOOL:
				GetPrivateProfileString(lpTitle, p->item,
									(*(static_cast<const UINT8*>(p->value))) ? str_true : str_false,
									szWork, NELEMENTS(szWork), lpPath);
				*(static_cast<UINT8*>(p->value)) = (!milstr_cmp(szWork, str_true)) ? 1 : 0;
				break;

			case PFTYPE_BITMAP:
				GetPrivateProfileString(lpTitle, p->item,
									(bitmapget(p->value, p->arg)) ? str_true : str_false,
									szWork, _countof(szWork), lpPath);
				bitmapset(p->value, p->arg, (milstr_cmp(szWork, str_true) == 0));
				break;

			case PFTYPE_BIN:
				GetPrivateProfileString(lpTitle, p->item, str_null, szWork, _countof(szWork), lpPath);
				binset(p->value, p->arg, szWork);
				break;

			case PFTYPE_SINT8:
			case PFTYPE_UINT8:
				val = GetPrivateProfileInt(lpTitle, p->item, *(static_cast<const UINT8*>(p->value)), lpPath);
				*(static_cast<UINT8*>(p->value)) = static_cast<UINT8>(val);
				break;

			case PFTYPE_SINT16:
			case PFTYPE_UINT16:
				val = GetPrivateProfileInt(lpTitle, p->item, *(static_cast<const UINT16*>(p->value)), lpPath);
				*(static_cast<UINT16*>(p->value)) = static_cast<UINT16>(val);
				break;

			case PFTYPE_SINT32:
			case PFTYPE_UINT32:
				val = GetPrivateProfileInt(lpTitle, p->item, *(static_cast<const UINT32*>(p->value)), lpPath);
				*(static_cast<UINT32*>(p->value)) = static_cast<UINT32>(val);
				break;

			case PFTYPE_HEX8:
				wsprintf(szWork, str_x, *(static_cast<const UINT8*>(p->value)));
				GetPrivateProfileString(lpTitle, p->item, szWork, szWork, _countof(szWork), lpPath);
				*(static_cast<UINT8*>(p->value)) = static_cast<UINT8>(milstr_solveHEX(szWork));
				break;

			case PFTYPE_HEX16:
				wsprintf(szWork, str_x, *(static_cast<const UINT16*>(p->value)));
				GetPrivateProfileString(lpTitle, p->item, szWork, szWork, _countof(szWork), lpPath);
				*(static_cast<UINT16*>(p->value)) = static_cast<UINT16>(milstr_solveHEX(szWork));
				break;

			case PFTYPE_HEX32:
				wsprintf(szWork, str_x, *(static_cast<const UINT32*>(p->value)));
				GetPrivateProfileString(lpTitle, p->item, szWork, szWork, _countof(szWork), lpPath);
				*(static_cast<UINT32*>(p->value)) = static_cast<UINT32>(milstr_solveHEX(szWork));
				break;

			case PFTYPE_ARGS16:
				GetPrivateProfileString(lpTitle, p->item, str_null, szWork, _countof(szWork), lpPath);
				inirdargs16(szWork, p);
				break;

			case PFTYPE_BYTE3:
				GetPrivateProfileString(lpTitle, p->item, str_null, szWork, _countof(szWork), lpPath);
				inirdbyte3(szWork, p);
				break;

			case PFTYPE_KB:
				GetPrivateProfileString(lpTitle, p->item, str_null, szWork, _countof(szWork), lpPath);
				inirdkb(szWork, p);
				break;
		}
		p++;
	}
}

/**
 * 設定書き込み
 * @param[in] lpPath パス
 * @param[in] lpTitle タイトル
 * @param[in] lpTable 設定テーブル
 * @param[in] nCount 設定テーブル アイテム数
 */
void ini_write(LPCTSTR lpPath, LPCTSTR lpTitle, const PFTBL* lpTable, UINT nCount)
{
	const PFTBL* p = lpTable;
	const PFTBL* pTerminate = p + nCount;
	while (p < pTerminate)
	{
		if (!(p->itemtype & PFFLAG_RO))
		{
			TCHAR szWork[512];
			szWork[0] = '\0';

			LPCTSTR lpSet = szWork;
			switch(p->itemtype & PFTYPE_MASK) {
				case PFTYPE_STR:
					lpSet = static_cast<LPCTSTR>(p->value);
					break;

				case PFTYPE_BOOL:
					lpSet = (*(static_cast<const UINT8*>(p->value))) ? str_true : str_false;
					break;

				case PFTYPE_BITMAP:
					lpSet = (bitmapget(p->value, p->arg)) ? str_true : str_false;
					break;

				case PFTYPE_BIN:
					binget(szWork, _countof(szWork), p->value, p->arg);
					break;

				case PFTYPE_SINT8:
					wsprintf(szWork, str_d, *(static_cast<const SINT8*>(p->value)));
					break;

				case PFTYPE_SINT16:
					wsprintf(szWork, str_d, *(static_cast<const SINT16*>(p->value)));
					break;

				case PFTYPE_SINT32:
					wsprintf(szWork, str_d, *(static_cast<const SINT32*>(p->value)));
					break;

				case PFTYPE_UINT8:
					wsprintf(szWork, str_u, *(static_cast<const UINT8*>(p->value)));
					break;

				case PFTYPE_UINT16:
					wsprintf(szWork, str_u, *(static_cast<const UINT16*>(p->value)));
					break;

				case PFTYPE_UINT32:
					wsprintf(szWork, str_u, *(static_cast<const UINT32*>(p->value)));
					break;

				case PFTYPE_HEX8:
					wsprintf(szWork, str_x, *(static_cast<const UINT8*>(p->value)));
					break;

				case PFTYPE_HEX16:
					wsprintf(szWork, str_x, *(static_cast<const UINT16*>(p->value)));
					break;

				case PFTYPE_HEX32:
					wsprintf(szWork, str_x, *(static_cast<const UINT32*>(p->value)));
					break;

				default:
					lpSet = NULL;
					break;
			}
			if (lpSet)
			{
				::WritePrivateProfileString(lpTitle, p->item, lpSet, lpPath);
			}
		}
		p++;
	}
}

#else	// !defined(_UNICODE)

// ---- Use profile.c

/**
 * コールバック
 * @param[in] item アイテム
 * @param[in] lpString 文字列
 */
static void UserReadItem(const PFTBL* item, LPCTSTR lpString)
{
	switch (item->itemtype & PFTYPE_MASK)
	{
		case PFTYPE_ARGS16:
			inirdargs16(lpString, item);
			break;

		case PFTYPE_BYTE3:
			inirdbyte3(lpString, item);
			break;

		case PFTYPE_KB:
			inirdkb(lpString, item);
			break;
	}
}

/**
 * 設定読み取り
 * @param[in] lpPath パス
 * @param[in] lpTitle タイトル
 * @param[in] lpTable 設定テーブル
 * @param[in] nCount 設定テーブル アイテム数
 */
void ini_read(LPCTSTR lpPath, LPCTSTR lpTitle, const PFTBL* lpTable, UINT nCount)
{
	profile_iniread(lpPath, lpTitle, lpTable, nCount, UserReadItem);
}

/**
 * 設定書き込み
 * @param[in] lpPath パス
 * @param[in] lpTitle タイトル
 * @param[in] lpTable 設定テーブル
 * @param[in] nCount 設定テーブル アイテム数
 */
void ini_write(LPCTSTR lpPath, LPCTSTR lpTitle, const PFTBL* lpTable, UINT nCount)
{
	profile_iniwrite(lpPath, lpTitle, lpTable, nCount, NULL);
}

#endif	// !defined(_UNICODE)


// ----

#if !defined(SUPPORT_PC9821)
static const TCHAR s_szIniTitle[] = TEXT("NekoProjectII");		//!< アプリ名
#else
static const TCHAR s_szIniTitle[] = TEXT("NekoProject21");		//!< アプリ名
#endif

/**
 * 追加設定
 */
enum
{
	PFRO_STR		= PFFLAG_RO + PFTYPE_STR,
	PFRO_BOOL		= PFFLAG_RO + PFTYPE_BOOL,
	PFRO_BITMAP		= PFFLAG_RO + PFTYPE_BITMAP,
	PFRO_UINT8		= PFFLAG_RO + PFTYPE_UINT8,
	PFRO_SINT32		= PFFLAG_RO + PFTYPE_SINT32,
	PFRO_HEX8		= PFFLAG_RO + PFTYPE_HEX8,
	PFRO_HEX32		= PFFLAG_RO + PFTYPE_HEX32,
	PFRO_BYTE3		= PFFLAG_RO + PFTYPE_BYTE3,
	PFRO_KB			= PFFLAG_RO + PFTYPE_KB,
};

/**
 * OS 設定 テーブル
 */
static const PFTBL s_IniItems[] =
{
	PFSTR("np2title", PFRO_STR,			np2oscfg.titles),
	PFVAL("np2winid", PFRO_BYTE3,		np2oscfg.winid),
	PFVAL("WindposX", PFTYPE_SINT32,	&np2oscfg.winx),
	PFVAL("WindposY", PFTYPE_SINT32,	&np2oscfg.winy),
	PFMAX("paddingx", PFRO_SINT32,		&np2oscfg.paddingx,		32),
	PFMAX("paddingy", PFRO_SINT32,		&np2oscfg.paddingy,		32),
	PFVAL("Win_Snap", PFTYPE_BOOL,		&np2oscfg.WINSNAP),

	PFSTR("FDfolder", PFTYPE_STR,		fddfolder),
	PFSTR("HDfolder", PFTYPE_STR,		hddfolder),
	PFSTR("CDfolder", PFTYPE_STR,		cdfolder),
	PFSTR("bmap_Dir", PFTYPE_STR,		bmpfilefolder),
	PFSTR("npcfgDir", PFTYPE_STR,		npcfgfilefolder),
	PFSTR("fontfile", PFTYPE_STR,		np2cfg.fontfile),
	PFSTR("biospath", PFRO_STR,			np2cfg.biospath),

#if defined(SUPPORT_HOSTDRV)
	PFVAL("use_hdrv", PFTYPE_BOOL,		&np2cfg.hdrvenable),
	PFSTR("hdrvroot", PFTYPE_STR,		np2cfg.hdrvroot),
	PFVAL("hdrv_acc", PFTYPE_UINT8,		&np2cfg.hdrvacc),
#endif

	PFSTR("pc_model", PFTYPE_STR,		np2cfg.model),
	PFVAL("clk_base", PFTYPE_UINT32,	&np2cfg.baseclock),
	PFVAL("clk_mult", PFTYPE_UINT32,	&np2cfg.multiple),

	PFEXT("DIPswtch", PFTYPE_BIN,		np2cfg.dipsw,			3),
	PFEXT("MEMswtch", PFTYPE_BIN,		np2cfg.memsw,			8),
#if defined(SUPPORT_LARGE_MEMORY)
	PFMAX("ExMemory", PFTYPE_UINT16,	&np2cfg.EXTMEM,			MEMORY_MAXSIZE),
#else
	PFMAX("ExMemory", PFTYPE_UINT8,		&np2cfg.EXTMEM,			MEMORY_MAXSIZE),
#endif
	PFVAL("ITF_WORK", PFTYPE_BOOL,		&np2cfg.ITF_WORK),
	
	PFVAL("USE_BIOS", PFTYPE_BOOL,		&np2cfg.usebios),  // 実機BIOS使用
	
	PFVAL("SVFDFILE", PFTYPE_BOOL,		&np2cfg.savefddfile),
	PFSTR("FDD1FILE", PFTYPE_STR,		np2cfg.fddfile[0]),
	PFSTR("FDD2FILE", PFTYPE_STR,		np2cfg.fddfile[1]),
	PFSTR("FDD3FILE", PFTYPE_STR,		np2cfg.fddfile[2]),
	PFSTR("FDD4FILE", PFTYPE_STR,		np2cfg.fddfile[3]),

	PFSTR("HDD1FILE", PFTYPE_STR,		np2cfg.sasihdd[0]),
	PFSTR("HDD2FILE", PFTYPE_STR,		np2cfg.sasihdd[1]),
#if defined(SUPPORT_SCSI)
	PFSTR("SCSIHDD0", PFTYPE_STR,		np2cfg.scsihdd[0]),
	PFSTR("SCSIHDD1", PFTYPE_STR,		np2cfg.scsihdd[1]),
	PFSTR("SCSIHDD2", PFTYPE_STR,		np2cfg.scsihdd[2]),
	PFSTR("SCSIHDD3", PFTYPE_STR,		np2cfg.scsihdd[3]),
#endif
#if defined(SUPPORT_IDEIO)
	PFSTR("HDD3FILE", PFTYPE_STR,		np2cfg.sasihdd[2]),
	PFSTR("HDD4FILE", PFTYPE_STR,		np2cfg.sasihdd[3]),
	PFVAL("IDE1TYPE", PFTYPE_UINT8,		&np2cfg.idetype[0]),
	PFVAL("IDE2TYPE", PFTYPE_UINT8,		&np2cfg.idetype[1]),
	PFVAL("IDE3TYPE", PFTYPE_UINT8,		&np2cfg.idetype[2]),
	PFVAL("IDE4TYPE", PFTYPE_UINT8,		&np2cfg.idetype[3]),
	PFVAL("IDE_BIOS", PFTYPE_BOOL,		&np2cfg.idebios),  // 実機IDE BIOS使用
	PFVAL("AIDEBIOS", PFTYPE_BOOL,		&np2cfg.autoidebios),  // 実機IDE BIOS使用を自動設定する
	PFVAL("IDERWAIT", PFTYPE_UINT32,	&np2cfg.iderwait),
	PFVAL("IDEWWAIT", PFTYPE_UINT32,	&np2cfg.idewwait),
	PFVAL("IDEMWAIT", PFTYPE_UINT32,	&np2cfg.idemwait),
	PFVAL("CD_ASYNC", PFTYPE_BOOL,		&np2cfg.useasynccd),
	PFVAL("CDTRAYOP", PFTYPE_BOOL,		&np2cfg.allowcdtraycmd),
	PFVAL("SVCDFILE", PFTYPE_BOOL,		&np2cfg.savecdfile),
	PFVAL("HD_ASYNC", PFRO_BOOL,		&np2cfg.useasynchd),
	PFSTR("CD1_FILE", PFTYPE_STR,		np2cfg.idecd[0]),
	PFSTR("CD2_FILE", PFTYPE_STR,		np2cfg.idecd[1]),
	PFSTR("CD3_FILE", PFTYPE_STR,		np2cfg.idecd[2]),
	PFSTR("CD4_FILE", PFTYPE_STR,		np2cfg.idecd[3]),
#endif

	PFVAL("SampleHz", PFTYPE_UINT32,	&np2cfg.samplingrate),
	PFVAL("Latencys", PFTYPE_UINT16,	&np2cfg.delayms),
	PFVAL("SNDboard", PFTYPE_HEX8,		&np2cfg.SOUND_SW),
	PFAND("BEEP_vol", PFTYPE_UINT8,		&np2cfg.BEEP_VOL,		3),
	PFVAL("xspeaker", PFRO_BOOL,		&np2cfg.snd_x),

	PFEXT("SND14vol", PFTYPE_BIN,		np2cfg.vol14,			6),
//	PFEXT("opt14BRD", PFTYPE_BIN,		np2cfg.snd14opt,		3),
	PFVAL("opt26BRD", PFTYPE_HEX8,		&np2cfg.snd26opt),
	PFVAL("opt86BRD", PFTYPE_HEX8,		&np2cfg.snd86opt),
	PFVAL("optSPBRD", PFTYPE_HEX8,		&np2cfg.spbopt),
	PFVAL("optSPBVR", PFTYPE_HEX8,		&np2cfg.spb_vrc),
	PFMAX("optSPBVL", PFTYPE_UINT8,		&np2cfg.spb_vrl,		24),
	PFVAL("optSPB_X", PFTYPE_BOOL,		&np2cfg.spb_x),
	PFVAL("USEMPU98", PFTYPE_BOOL,		&np2cfg.mpuenable),
	PFVAL("optMPU98", PFTYPE_HEX8,		&np2cfg.mpuopt),
	PFVAL("optMPUAT", PFTYPE_BOOL,		&np2cfg.mpu_at),
#if defined(SUPPORT_SMPU98)
	PFVAL("USE_SMPU", PFTYPE_BOOL,		&np2cfg.smpuenable),
	PFVAL("opt_SMPU", PFTYPE_HEX8,		&np2cfg.smpuopt),
	PFVAL("SMPUMUTB", PFTYPE_BOOL,		&np2cfg.smpumuteB),
#endif
	
	PFVAL("opt118io", PFTYPE_HEX16,		&np2cfg.snd118io),
	PFVAL("opt118id", PFTYPE_HEX8,		&np2cfg.snd118id),
	PFVAL("opt118dm", PFTYPE_UINT8,		&np2cfg.snd118dma),
	PFVAL("opt118if", PFTYPE_UINT8,		&np2cfg.snd118irqf),
	PFVAL("opt118ip", PFTYPE_UINT8,		&np2cfg.snd118irqp),
	PFVAL("opt118im", PFTYPE_UINT8,		&np2cfg.snd118irqm),
	PFVAL("opt118ro", PFTYPE_UINT8,		&np2cfg.snd118rom),
	
	PFVAL("optwssid", PFTYPE_HEX8,		&np2cfg.sndwssid),
	PFVAL("optwssdm", PFTYPE_UINT8,		&np2cfg.sndwssdma),
	PFVAL("optwssip", PFTYPE_UINT8,		&np2cfg.sndwssirq),
	
#if defined(SUPPORT_SOUND_SB16)
	PFVAL("optsb16p", PFTYPE_HEX8,		&np2cfg.sndsb16io),
	PFVAL("optsb16d", PFTYPE_UINT8,		&np2cfg.sndsb16dma),
	PFVAL("optsb16i", PFTYPE_UINT8,		&np2cfg.sndsb16irq),
	PFVAL("optsb16A", PFTYPE_BOOL,		&np2cfg.sndsb16at),
#endif	/* SUPPORT_SOUND_SB16 */
	
	PFMAX("volume_M", PFTYPE_UINT8,		&np2cfg.vol_master,		100),
	PFMAX("volume_F", PFTYPE_UINT8,		&np2cfg.vol_fm,			128),
	PFMAX("volume_S", PFTYPE_UINT8,		&np2cfg.vol_ssg,		128),
	PFMAX("volume_A", PFTYPE_UINT8,		&np2cfg.vol_adpcm,		128),
	PFMAX("volume_P", PFTYPE_UINT8,		&np2cfg.vol_pcm,		128),
	PFMAX("volume_R", PFTYPE_UINT8,		&np2cfg.vol_rhythm,		128),

	PFVAL("Seek_Snd", PFTYPE_BOOL,		&np2cfg.MOTOR),
	PFMAX("Seek_Vol", PFTYPE_UINT8,		&np2cfg.MOTORVOL,		100),

	PFVAL("btnRAPID", PFTYPE_BOOL,		&np2cfg.BTN_RAPID),
	PFVAL("btn_MODE", PFTYPE_BOOL,		&np2cfg.BTN_MODE),
	PFVAL("Mouse_sw", PFTYPE_BOOL,		&np2oscfg.MOUSE_SW),
	PFVAL("MS_RAPID", PFTYPE_BOOL,		&np2cfg.MOUSERAPID),

	PFAND("backgrnd", PFTYPE_UINT8,		&np2oscfg.background,	3),
	PFEXT("VRAMwait", PFTYPE_BIN,		np2cfg.wait,			6),
	PFAND("DspClock", PFTYPE_UINT8,		&np2oscfg.DISPCLK,		3),
	PFVAL("DispSync", PFTYPE_BOOL,		&np2cfg.DISPSYNC),
	PFVAL("Real_Pal", PFTYPE_BOOL,		&np2cfg.RASTER),
	PFMAX("RPal_tim", PFTYPE_UINT8,		&np2cfg.realpal,		64),
	PFVAL("s_NOWAIT", PFTYPE_BOOL,		&np2oscfg.NOWAIT),
	PFVAL("SkpFrame", PFTYPE_UINT8,		&np2oscfg.DRAW_SKIP),
	PFVAL("uPD72020", PFTYPE_BOOL,		&np2cfg.uPD72020),
	PFAND("GRCG_EGC", PFTYPE_UINT8,		&np2cfg.grcg,			3),
	PFVAL("color16b", PFTYPE_BOOL,		&np2cfg.color16),
	PFVAL("skipline", PFTYPE_BOOL,		&np2cfg.skipline),
	PFVAL("skplight", PFTYPE_UINT16,	&np2cfg.skiplight),
	PFAND("LCD_MODE", PFTYPE_UINT8,		&np2cfg.LCD_MODE,		0x03),
	PFAND("BG_COLOR", PFRO_HEX32,		&np2cfg.BG_COLOR,		0xffffff),
	PFAND("FG_COLOR", PFRO_HEX32,		&np2cfg.FG_COLOR,		0xffffff),

	PFVAL("pc9861_e", PFTYPE_BOOL,		&np2cfg.pc9861enable),
	PFEXT("pc9861_s", PFTYPE_BIN,		np2cfg.pc9861sw,		3),
	PFEXT("pc9861_j", PFTYPE_BIN,		np2cfg.pc9861jmp,		6),
	
#if defined(SUPPORT_FMGEN)
	PFVAL("USEFMGEN", PFTYPE_BOOL,		&np2cfg.usefmgen),
#endif	/* SUPPORT_FMGEN */

	PFVAL("calendar", PFTYPE_BOOL,		&np2cfg.calendar),
	PFVAL("USE144FD", PFTYPE_BOOL,		&np2cfg.usefd144),
	PFEXT("FDDRIVE1", PFRO_BITMAP,		&np2cfg.fddequip,		0),
	PFEXT("FDDRIVE2", PFRO_BITMAP,		&np2cfg.fddequip,		1),
	PFEXT("FDDRIVE3", PFRO_BITMAP,		&np2cfg.fddequip,		2),
	PFEXT("FDDRIVE4", PFRO_BITMAP,		&np2cfg.fddequip,		3),

#if defined(SUPPORT_BMS)
	PFEXT("Use_BMS_", PFTYPE_BOOL,		&bmsiocfg.enabled,		0),
	PFEXT("BMS_Port", PFTYPE_HEX16,		&bmsiocfg.port,			0),
	PFEXT("BMS_Size", PFTYPE_UINT8,		&bmsiocfg.numbanks,		0),
#endif

#if defined(SUPPORT_NET)
	PFSTR("NP2NETTAP", PFTYPE_STR,		np2cfg.np2nettap),
	PFVAL("NP2NETPMM", PFTYPE_BOOL,		&np2cfg.np2netpmm),
#endif
#if defined(SUPPORT_LGY98)
	PFVAL("USELGY98", PFTYPE_BOOL,		&np2cfg.uselgy98),
	PFVAL("LGY98_IO", PFTYPE_UINT16,	&np2cfg.lgy98io),
	PFVAL("LGY98IRQ", PFTYPE_UINT8,		&np2cfg.lgy98irq),
	PFEXT("LGY98MAC", PFTYPE_BIN,		np2cfg.lgy98mac,		6),
#endif
#if defined(SUPPORT_WAB)
	PFVAL("WAB_ANSW", PFTYPE_UINT8,		&np2cfg.wabasw),
#endif
#if defined(SUPPORT_CL_GD5430)
	PFVAL("USEGD5430", PFTYPE_BOOL,		&np2cfg.usegd5430),
	PFVAL("GD5430TYPE",PFTYPE_UINT16,	&np2cfg.gd5430type),
	PFVAL("GD5430FCUR",PFTYPE_BOOL,		&np2cfg.gd5430fakecur),
	PFVAL("GDMELOFS", PFTYPE_UINT8,		&np2cfg.gd5430melofs),
	PFVAL("GANBBSEX", PFTYPE_BOOL,		&np2cfg.ga98nb_bigscrn_ex),
#endif
#if defined(SUPPORT_VGA_MODEX)
	PFVAL("USEMODEX", PFTYPE_BOOL,		&np2cfg.usemodex),
#endif
#if defined(SUPPORT_GPIB)
	PFVAL("USE_GPIB", PFTYPE_BOOL,		&np2cfg.usegpib),
	PFVAL("GPIB_IRQ", PFTYPE_UINT8,		&np2cfg.gpibirq),
	PFVAL("GPIBMODE", PFTYPE_UINT8,		&np2cfg.gpibmode),
	PFVAL("GPIBADDR", PFTYPE_UINT8,		&np2cfg.gpibaddr),
	PFVAL("GPIBEXIO", PFTYPE_UINT8,		&np2cfg.gpibexio),
#endif
#if defined(SUPPORT_PCI)
	PFVAL("USE98PCI", PFTYPE_BOOL,		&np2cfg.usepci),
	PFVAL("P_BIOS32", PFTYPE_BOOL,		&np2cfg.pci_bios32),
	PFVAL("PCI_PCMC", PFTYPE_UINT8,		&np2cfg.pci_pcmc),
#endif
	
	PFMAX("DAVOLUME", PFTYPE_UINT8,		&np2cfg.davolume,		255),
	PFMAX("MODELNUM", PFTYPE_HEX8,		&np2cfg.modelnum,		255),
	
	PFVAL("TIMERFIX", PFTYPE_BOOL,		&np2cfg.timerfix),
	
	PFVAL("WINNTFIX", PFTYPE_BOOL,		&np2cfg.winntfix),
	
	PFVAL("SYSIOMSK", PFTYPE_HEX16,		&np2cfg.sysiomsk), // システムIOマスク
	
	PFMAX("MEMCHKMX", PFTYPE_UINT8,		&np2cfg.memchkmx,		0), // メモリチェックする最大サイズ（最小は15MB・0は制限無し・メモリチェックが長いのが嫌だけど見かけ上カウントだけはしておきたい人向け）
	PFMAX("SBEEPLEN", PFTYPE_UINT8,		&np2cfg.sbeeplen,		0), // ピポ音の長さ（0でデフォルト・4がNP2標準）
	PFVAL("SBEEPADJ", PFTYPE_BOOL,		&np2cfg.sbeepadj), // ピポ音の長さ自動調整

	PFVAL("BIOSIOEM", PFTYPE_BOOL,		&np2cfg.biosioemu), // np21w ver0.86 rev46 BIOS I/O emulation
	
	PFSTR("cpu_vend", PFRO_STR,			np2cfg.cpu_vendor_o),
	PFVAL("cpu_fami", PFTYPE_UINT32,	&np2cfg.cpu_family),
	PFVAL("cpu_mode", PFTYPE_UINT32,	&np2cfg.cpu_model),
	PFVAL("cpu_step", PFTYPE_UINT32,	&np2cfg.cpu_stepping),
	PFVAL("cpu_feat", PFTYPE_HEX32,		&np2cfg.cpu_feature),
	PFVAL("cpu_f_ex", PFTYPE_HEX32,		&np2cfg.cpu_feature_ex),
	PFSTR("cpu_bran", PFRO_STR,			np2cfg.cpu_brandstring_o),
	PFVAL("cpu_brid", PFTYPE_HEX32,		&np2cfg.cpu_brandid),
	PFVAL("cpu_fecx", PFTYPE_HEX32,		&np2cfg.cpu_feature_ecx),
	PFVAL("cpu_eflg", PFTYPE_HEX32,		&np2cfg.cpu_eflags_mask),

	PFMAX("FPU_TYPE", PFTYPE_UINT8,		&np2cfg.fpu_type,		0), // FPU種類
	
#if defined(SUPPORT_FAST_MEMORYCHECK)
	PFVAL("memckspd", PFTYPE_UINT8,		&np2cfg.memcheckspeed),
#endif
	
	PFVAL("USERAM_D", PFTYPE_BOOL,		&np2cfg.useram_d), // EPSONでなくてもD0000h-DFFFFhをRAMに（ただしIDE BIOS D8000h-DBFFFhは駄目）
	PFVAL("USEPEGCP", PFTYPE_BOOL,		&np2cfg.usepegcplane), // PEGC プレーンモードサポート
	
	PFVAL("USECDECC", PFTYPE_BOOL,		&np2cfg.usecdecc), // CD-ROM EDC/ECC エミュレーションサポート
	PFVAL("CDDTSKIP", PFTYPE_BOOL,		&np2cfg.cddtskip), // CD-ROM オーディオ再生時にデータトラックをスキップ
	
#if defined(SUPPORT_ASYNC_CPU)
	PFVAL("ASYNCCPU", PFTYPE_BOOL,		&np2cfg.asynccpu), // 非同期CPUモード有効
#endif
#if defined(SUPPORT_IDEIO)
	PFVAL("IDEBADDR", PFRO_HEX8,		&np2cfg.idebaddr), // IDE BIOS アドレス（デフォルト：D8h(D8000h)）
#endif
#if defined(SUPPORT_GAMEPORT)
	PFVAL("GAMEPORT", PFTYPE_BOOL,		&np2cfg.gameport),
#endif

	

	// OS依存？
	PFVAL("keyboard", PFRO_KB,			&np2oscfg.KEYBOARD),
	PFVAL("usenlock", PFTYPE_BOOL,		&np2oscfg.USENUMLOCK),
	PFVAL("F12_COPY", PFTYPE_UINT8,		&np2oscfg.F12COPY),
	PFVAL("Joystick", PFTYPE_BOOL,		&np2oscfg.JOYPAD1),
	PFEXT("Joy1_btn", PFTYPE_BIN,		np2oscfg.JOY1BTN,		4),

	PFVAL("clocknow", PFTYPE_UINT8,		&np2oscfg.clk_x),
	PFVAL("clockfnt", PFTYPE_UINT8,		&np2oscfg.clk_fnt),
	PFAND("clock_up", PFRO_HEX32,		&np2oscfg.clk_color1,	0xffffff),
	PFAND("clock_dn", PFRO_HEX32,		&np2oscfg.clk_color2,	0xffffff),

	PFVAL("use_sstp", PFTYPE_BOOL,		&np2oscfg.sstp),
	PFVAL("sstpport", PFTYPE_UINT16,	&np2oscfg.sstpport),
	PFVAL("comfirm_", PFTYPE_BOOL,		&np2oscfg.comfirm),
	PFVAL("shortcut", PFTYPE_HEX8,		&np2oscfg.shortcut),

	PFSTR("mpu98map", PFTYPE_STR,		np2oscfg.mpu.mout),
	PFSTR("mpu98min", PFTYPE_STR,		np2oscfg.mpu.min),
	PFSTR("mpu98mdl", PFTYPE_STR,		np2oscfg.mpu.mdl),
	PFSTR("mpu98def", PFTYPE_STR,		np2oscfg.mpu.def),
	
#if defined(SUPPORT_SMPU98)
	PFSTR("smpuAmap", PFTYPE_STR,		np2oscfg.smpuA.mout),
	PFSTR("smpuAmin", PFTYPE_STR,		np2oscfg.smpuA.min),
	PFSTR("smpuAmdl", PFTYPE_STR,		np2oscfg.smpuA.mdl),
	PFSTR("smpuAdef", PFTYPE_STR,		np2oscfg.smpuA.def),
	PFSTR("smpuBmap", PFTYPE_STR,		np2oscfg.smpuB.mout),
	PFSTR("smpuBmin", PFTYPE_STR,		np2oscfg.smpuB.min),
	PFSTR("smpuBmdl", PFTYPE_STR,		np2oscfg.smpuB.mdl),
	PFSTR("smpuBdef", PFTYPE_STR,		np2oscfg.smpuB.def),
#endif

	PFMAX("com1port", PFTYPE_UINT8,		&np2oscfg.com1.port,	5),
	PFVAL("com1para", PFTYPE_UINT8,		&np2oscfg.com1.param),
	PFVAL("com1_bps", PFTYPE_UINT32,	&np2oscfg.com1.speed),
	PFVAL("com1fbps", PFTYPE_BOOL,		&np2oscfg.com1.fixedspeed),
	PFSTR("com1mmap", PFTYPE_STR,		np2oscfg.com1.mout),
	PFSTR("com1mmdl", PFTYPE_STR,		np2oscfg.com1.mdl),
	PFSTR("com1mdef", PFTYPE_STR,		np2oscfg.com1.def),
#if defined(SUPPORT_NAMED_PIPE)
	PFSTR("com1pnam", PFTYPE_STR,		np2oscfg.com1.pipename),
	PFSTR("com1psrv", PFTYPE_STR,		np2oscfg.com1.pipeserv),
#endif

	PFMAX("com2port", PFTYPE_UINT8,		&np2oscfg.com2.port,	5),
	PFVAL("com2para", PFTYPE_UINT8,		&np2oscfg.com2.param),
	PFVAL("com2_bps", PFTYPE_UINT32,	&np2oscfg.com2.speed),
	PFVAL("com2fbps", PFTYPE_BOOL,		&np2oscfg.com2.fixedspeed),
	PFSTR("com2mmap", PFTYPE_STR,		np2oscfg.com2.mout),
	PFSTR("com2mmdl", PFTYPE_STR,		np2oscfg.com2.mdl),
	PFSTR("com2mdef", PFTYPE_STR,		np2oscfg.com2.def),
#if defined(SUPPORT_NAMED_PIPE)
	PFSTR("com2pnam", PFTYPE_STR,		np2oscfg.com2.pipename),
	PFSTR("com2psrv", PFTYPE_STR,		np2oscfg.com2.pipeserv),
#endif

	PFMAX("com3port", PFTYPE_UINT8,		&np2oscfg.com3.port,	5),
	PFVAL("com3para", PFTYPE_UINT8,		&np2oscfg.com3.param),
	PFVAL("com3_bps", PFTYPE_UINT32,	&np2oscfg.com3.speed),
	PFVAL("com3fbps", PFTYPE_BOOL,		&np2oscfg.com3.fixedspeed),
	PFSTR("com3mmap", PFTYPE_STR,		np2oscfg.com3.mout),
	PFSTR("com3mmdl", PFTYPE_STR,		np2oscfg.com3.mdl),
	PFSTR("com3mdef", PFTYPE_STR,		np2oscfg.com3.def),
#if defined(SUPPORT_NAMED_PIPE)
	PFSTR("com3pnam", PFTYPE_STR,		np2oscfg.com3.pipename),
	PFSTR("com3psrv", PFTYPE_STR,		np2oscfg.com3.pipeserv),
#endif

	PFVAL("force400", PFRO_BOOL,		&np2oscfg.force400),
	PFVAL("e_resume", PFTYPE_BOOL,		&np2oscfg.resume),
	PFVAL("STATSAVE", PFRO_BOOL,		&np2oscfg.statsave),
#if !defined(_WIN64)
	PFVAL("nousemmx", PFTYPE_BOOL,		&np2oscfg.disablemmx),
#endif
	PFVAL("windtype", PFTYPE_UINT8,		&np2oscfg.wintype),
	PFVAL("toolwind", PFTYPE_BOOL,		&np2oscfg.toolwin),
	PFVAL("keydispl", PFTYPE_BOOL,		&np2oscfg.keydisp),
	PFVAL("skbdwind", PFTYPE_BOOL,		&np2oscfg.skbdwin),
	PFVAL("I286SAVE", PFRO_BOOL,		&np2oscfg.I286SAVE),
	PFVAL("jast_snd", PFTYPE_BOOL,		&np2oscfg.jastsnd),
	PFVAL("useromeo", PFTYPE_BOOL,		&np2oscfg.useromeo),
	PFVAL("thickfrm", PFTYPE_BOOL,		&np2oscfg.thickframe),
	PFVAL("xrollkey", PFTYPE_BOOL,		&np2oscfg.xrollkey),
	PFVAL("fscrn_cx", PFRO_SINT32,		&np2oscfg.fscrn_cx),
	PFVAL("fscrn_cy", PFRO_SINT32,		&np2oscfg.fscrn_cy),
	PFVAL("fscrnbpp", PFRO_UINT8,		&np2oscfg.fscrnbpp),
	PFVAL("fscrnmod", PFTYPE_HEX8,		&np2oscfg.fscrnmod),
	PFVAL("fsrescfg", PFTYPE_BOOL,		&np2oscfg.fsrescfg), // 解像度毎に設定保存する

#if defined(SUPPORT_SCRN_DIRECT3D)
	PFVAL("D3D_IMODE", PFTYPE_UINT8,	&np2oscfg.d3d_imode), // Direct3D 拡大縮小補間モード
	PFVAL("D3D_EXCLU", PFTYPE_BOOL,		&np2oscfg.d3d_exclusive), // Direct3D 排他モード使用
#endif

	PFVAL("snddev_t", PFTYPE_UINT8,		&np2oscfg.cSoundDeviceType),
	PFSTR("snddev_n", PFTYPE_STR,		np2oscfg.szSoundDeviceName),

#if defined(SUPPORT_VSTi)
	PFSTR("VSTiFile", PFRO_STR,			np2oscfg.szVSTiFile),
#endif	// defined(SUPPORT_VSTi)
	
	PFVAL("EMUDDRAW", PFTYPE_BOOL,		&np2oscfg.emuddraw), // 最近はEMULATIONONLYにした方速かったりする（特にピクセル操作する場合とか）
	PFVAL("DRAWTYPE", PFTYPE_UINT8,		&np2oscfg.drawtype), // 画面レンダラ (0: DirectDraw, 1: reserved(DirecrDraw), 2: Direct3D)
	
	PFVAL("DRAGDROP", PFRO_BOOL,		&np2oscfg.dragdrop), // ドラッグアンドドロップサポート
	PFVAL("MAKELHDD", PFRO_BOOL,		&np2oscfg.makelhdd), // 巨大HDDイメージ作成サポート
	PFVAL("SYSKHOOK", PFTYPE_BOOL,		&np2oscfg.syskhook), // システムキーフックサポート
	PFVAL("RAWMOUSE", PFTYPE_BOOL,		&np2oscfg.rawmouse), // 直接マウスデータ読み取り
	PFVAL("MOUSEMUL", PFTYPE_SINT16,	&np2oscfg.mousemul), // マウススピード倍率（分子）
	PFVAL("MOUSEDIV", PFTYPE_SINT16,	&np2oscfg.mousediv), // マウススピード倍率（分母）
	
	PFVAL("SCRNMODE", PFTYPE_UINT8,		&np2oscfg.scrnmode), // フルスクリーン設定
	PFVAL("SAVESCRN", PFTYPE_BOOL,		&np2oscfg.savescrn), // フルスクリーン設定を保存・復元する
	
	PFVAL("SVSCRMUL", PFTYPE_BOOL,		&np2oscfg.svscrmul), // 画面表示倍率を保存するか
	PFVAL("SCRN_MUL", PFTYPE_UINT8,		&np2oscfg.scrn_mul), // 画面表示倍率（8が等倍）
	
	PFVAL("MOUSE_NC", PFTYPE_BOOL,		&np2oscfg.mouse_nc), // マウスキャプチャ無しコントロール
	PFVAL("CPUSTABF", PFTYPE_UINT16,	&np2oscfg.cpustabf), // クロック安定器適用限界時間（フレーム）
	PFVAL("READONLY", PFRO_BOOL,		&np2oscfg.readonly), // 変更を設定ファイルに書き込まない
	PFVAL("TICKMODE", PFRO_UINT8,		&np2oscfg.tickmode), // Tickカウンタのモードを強制的に設定する
	PFVAL("USEWHEEL", PFRO_BOOL,		&np2oscfg.usewheel), // マウスホイールによる音量・マウス速度設定を使用する
	PFVAL("USE_MVOL", PFRO_BOOL,		&np2oscfg.usemastervolume), // マスタボリューム設定を使用する
	
	PFVAL("TWNDHIST", PFRO_UINT8,		&np2oscfg.toolwndhistory), // ツールウィンドウのFDファイル履歴の記憶数
	
#if defined(SUPPORT_WACOM_TABLET)
	PFVAL("PENTABFA", PFTYPE_BOOL,		&np2oscfg.pentabfa), // ペンタブレット アスペクト比固定モード
#endif
};

//! .ini 拡張子
static const TCHAR s_szExt[] = TEXT(".ini");

/**
 * 設定ファイルのパスを得る
 * @param[out] lpPath パス
 * @param[in] cchPath パス バッファの長さ
 */
void initgetfile(LPTSTR lpPath, UINT cchPath)
{
	LPCTSTR lpIni = Np2Arg::GetInstance()->iniFilename();
	if (lpIni)
	{
		file_cpyname(lpPath, lpIni, cchPath);
		//LPCTSTR lpExt = file_getext(lpPath);
		//if (lpExt[0] != '\0')
		//{
		//	file_catname(lpPath, s_szExt, cchPath);
		//}
	}
	else
	{
		file_cpyname(lpPath, modulefile, cchPath);
		file_cutext(lpPath);
		file_catname(lpPath, s_szExt, cchPath);
	}
}

/**
 * 読み込み
 */
void initload(void)
{
	TCHAR szPath[MAX_PATH];

	initgetfile(szPath, _countof(szPath));
	ini_read(szPath, s_szIniTitle, s_IniItems, _countof(s_IniItems));
}

/**
 * 書き出し
 */
void initsave(void)
{
	if(!np2oscfg.readonly){
		TCHAR szPath[MAX_PATH];
	
		initgetfile(szPath, _countof(szPath));
		ini_write(szPath, s_szIniTitle, s_IniItems, _countof(s_IniItems));
	}
}
