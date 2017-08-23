/**
 * @file	board118.c
 * @brief	Implementation of PC-9801-118
 */

#include "compiler.h"
#include "board118.h"
#include "pccore.h"
#include "iocore.h"
#include "cbuscore.h"
#include "cs4231io.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/soundrom.h"

static int opna_idx = 0;

static void IOOUTCALL ymf_o188(UINT port, REG8 dat)
{
	g_opna[opna_idx].s.addrl = dat;
	g_opna[opna_idx].s.addrh = 0;
	g_opna[opna_idx].s.data = dat;
	(void)port;
}

static void IOOUTCALL ymf_o18a(UINT port, REG8 dat)
{
	g_opna[opna_idx].s.data = dat;
	if (g_opna[opna_idx].s.addrh != 0) {
		return;
	}

	opna_writeRegister(&g_opna[opna_idx], g_opna[opna_idx].s.addrl, dat);

	(void)port;
}

static void IOOUTCALL ymf_o18c(UINT port, REG8 dat)
{
	if (g_opna[opna_idx].s.extend)
	{
		g_opna[opna_idx].s.addrl = dat;
		g_opna[opna_idx].s.addrh = 1;
		g_opna[opna_idx].s.data = dat;
	}
	(void)port;
}

static void IOOUTCALL ymf_o18e(UINT port, REG8 dat)
{
	if (!g_opna[opna_idx].s.extend)
	{
		return;
	}
	g_opna[opna_idx].s.data = dat;

	if (g_opna[opna_idx].s.addrh != 1)
	{
		return;
	}

	opna_writeExtendedRegister(&g_opna[opna_idx], g_opna[opna_idx].s.addrl, dat);

	(void)port;
}

static REG8 IOINPCALL ymf_i188(UINT port)
{
	(void)port;
	return g_opna[opna_idx].s.status;
}

static REG8 IOINPCALL ymf_i18a(UINT port)
{
	UINT nAddress;

	if (g_opna[opna_idx].s.addrh == 0)
	{
		nAddress = g_opna[opna_idx].s.addrl;
		if (nAddress == 0x0e)
		{
			return fmboard_getjoy(&g_opna[opna_idx]);
		}
		else if (nAddress < 0x10)
		{
			return opna_readRegister(&g_opna[opna_idx], nAddress);
		}
		else if (nAddress == 0xff)
		{
			return 1;
		}
	}

	(void)port;
	return g_opna[opna_idx].s.data;
}

static REG8 IOINPCALL ymf_i18c(UINT port)
{
	if (g_opna[opna_idx].s.extend)
	{
		return (g_opna[opna_idx].s.status & 3);
	}

	(void)port;
	return 0xff;
}

static void extendchannel(REG8 enable)
{
	g_opna[opna_idx].s.extend = enable;
	if (enable)
	{
		opngen_setcfg(&g_opna[opna_idx].opngen, 6, OPN_STEREO | 0x007);
	}
	else
	{
		opngen_setcfg(&g_opna[opna_idx].opngen, 3, OPN_MONORAL | 0x007);
		rhythm_setreg(&g_opna[opna_idx].rhythm, 0x10, 0xff);
	}
}

static void IOOUTCALL ymf_oa460(UINT port, REG8 dat)
{
	cs4231.extfunc = dat;
	extendchannel((REG8)(dat & 1));
	(void)port;
}

static REG8 IOINPCALL ymf_ia460(UINT port)
{
	(void)port;
	if(g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_PC_9801_86_WSS){
		return (0x70 | (cs4231.extfunc & 1));
	}else{
		return (0x80 | (cs4231.extfunc & 1));
	}
}



static REG8 IOINPCALL wss_i881e(UINT port)
{
	if(g_nSoundID==SOUNDID_MATE_X_PCM || g_nSoundID==SOUNDID_PC_9801_86_WSS){
		int ret = 0x64;
		ret |= (cs4231.dmairq-1) << 3;
		if((cs4231.dmairq-1)==0x1 || (cs4231.dmairq-1)==0x2){
			ret |= 0x80; // 奇数パリティ
		}
		return (ret);
	}else{
		return (0xDC);
	}
}
REG8 ymf701;
static void IOOUTCALL wss_o548e(UINT port, REG8 dat)
{
	ymf701 = dat;
}
static REG8 IOINPCALL wss_i548e(UINT port)
{
	return (ymf701); 
}
static REG8 IOINPCALL wss_i548f(UINT port)
{
	if(ymf701 == 0) return 0xe8;
	else if(ymf701 == 0x1) return 0xfe;
	else if(ymf701 == 0x2) return 0x40;
	else if(ymf701 == 0x3) return 0x30;
	else if(ymf701 == 0x4) return 0xff;
	else if(ymf701 == 0x20) return 0x04;
	else if(ymf701 == 0x40) return 0x20;
	else return 0;// from PC-9821Nr166
}

/*static REG8 IOINPCALL wss_ff(UINT port)
{
	(void)port;
	return (0xFF);
}*/

static REG8 IOINPCALL wss_i148c(UINT port)
{
	return (0xFE);
}

static REG8 IOINPCALL wss_i148d(UINT port)
{

	return (0x80);
}


static void IOOUTCALL ym_o1488(UINT port, REG8 dat) //FM Music Register Address Port
{
	g_opl3.s.addrl = dat;
	(void)port;
}

static void IOOUTCALL ym_o1489(UINT port, REG8 dat) //FM Music Data Port
{
	opl3_writeRegister(&g_opl3, g_opl3.s.addrl, dat);
	(void)port;
}


static void IOOUTCALL ym_o148a(UINT port, REG8 dat) // Advanced FM Music Register Address	 Port
{
	g_opl3.s.addrh = dat;
	(void)port;
}
static void IOOUTCALL ym_o148b(UINT port, REG8 dat) //Advanced FM Music Data Port
{
	opl3_writeExtendedRegister(&g_opl3, g_opl3.s.addrh, dat);
	(void)port;
}

static REG8 IOINPCALL ym_i1488(UINT port) //FM Music Status Port
{
	return 0;
}
static REG8 IOINPCALL ym_i1489(UINT port) //  ???
{
	return opl3_readRegister(&g_opl3, g_opl3.s.addrl);
}
static REG8 IOINPCALL ym_i148a(UINT port) //Advanced FM Music Status Port
{
	return opl3_readStatus(&g_opl3);

}

static REG8 IOINPCALL ym_i148b(UINT port) //  ???
{
	return opl3_readExtendedRegister(&g_opl3, g_opl3.s.addrh);
}
REG8 sound118;
static void IOOUTCALL csctrl_o148e(UINT port, REG8 dat) {
	sound118 = dat;
}

static REG8 IOINPCALL csctrl_i148e(UINT port) {
	return(sound118);
}
REG8 control118;
static REG8 IOINPCALL csctrl_i148f(UINT port) {

	(void)port;
	if(sound118 == 0)	return(0x03);
	if(sound118 == 0x05) {
		if(control118 == 0x04)return (0x04);
		else if(control118 == 0) return 0;}
	if(sound118 == 0x04) return (0x00);
	if(sound118 == 0x21) return (0x00);
	if(sound118 == 0xff) return (0x00);
	else
	return(0xff);
}

static void IOOUTCALL csctrl_o148f(UINT port, REG8 dat) {
	control118 = dat;
}
/*

ポート148E/148Fを操作することによってPC-9801-118の設定を読み書きすることが
できる。PnPモード時はポートが移動するので適切なPnPマネージャを利用することで
リソースを取得すること。PnPモード時は((wIOPort_Base[1])+0xe)が148Eに相当する
アドレスとなる。以下、非PnPモード時のポートである148E/148Fで説明する。

　ポート148Eにレジスタ番号をOUTし、ポート148FをIN/OUTすることによってアクセス
する。現在判明しているレジスタ内容は以下の通り。

　　00　ステータス取得(IN)
　　　　　b7～b4:不明
　　　　　b3    :PC-9801-118ボード存在確認　0:ボード存在　1:不在
　　　　　b2～b0:不明

　　05　ボード設定変更(OUT)　詳細不明
　　　　　04　設定変更開始
　　　　　0C　OPL設定変更開始
　　　　　00　設定変更終了

　　05　PnP設定変更F/F?(IN) 　詳細不明
　　　　　04　設定変更開始状態に遷移(4をout後に読む)
　　　　　00　設定変更終了状態に遷移(0をout後に読む)

　　20　MIDI割り込み変更F/F(OUT)
　　　　　04　設定変更許可
　　　　　00　設定変更禁止

　　21　MIDI割り込みレベル設定(OUT)
　　　　　00　IRQ10(INT41)
　　　　　01　IRQ6 (INT2)
　　　　　02　IRQ5 (INT1)
　　　　　03　IRQ3 (INT0)

　　21　MIDI割り込みレベル取得(IN)
　　　　　下位2ビット有効、上位ビット不定
　　　　　00　IRQ10(INT41)既定値
　　　　　01　IRQ6 (INT2)
　　　　　02　IRQ5 (INT1)
　　　　　03　IRQ3 (INT0)

　　FF　ステータス取得(IN)
　　　　　b7～b3:不明
　　　　　b2～b0:MIDI割り込み設定可能かどうか
　　　　　　xx0　設定不可
　　　　　　xx1　設定可
*/
static REG8 IOINPCALL csctrl_i486(UINT port) {
	return(0);
}

static REG8 IOINPCALL sb98_i2ad2(UINT port) {
	return(0xaa);
}

static REG8 IOINPCALL sb98_i2ed2(UINT port) {
	return(0xff);
}
// ----


static const IOOUT ymf_o[4] = {
			ymf_o188,	ymf_o18a,	ymf_o18c,	ymf_o18e};

static const IOINP ymf_i[4] = {
			ymf_i188,	ymf_i18a,	ymf_i18c,	NULL};



/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void board118_reset(const NP2CFG *pConfig)
{
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS){
		opna_idx = 1;
	}else{
		opna_idx = 0;
	}
	opna_reset(&g_opna[opna_idx], OPNA_MODE_2608 | OPNA_HAS_TIMER | OPNA_S98);
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS){
		opna_timer(&g_opna[opna_idx], 0x10, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	}else{
		opna_timer(&g_opna[opna_idx], 0xd0, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	}
	opl3_reset(&g_opl3, OPL3_HAS_OPL3L|OPL3_HAS_OPL3);
	opngen_setcfg(&g_opna[opna_idx].opngen, 3, OPN_STEREO | 0x038);
	cs4231io_reset();
	soundrom_load(0xcc000, OEMTEXT("118"));
	fmboard_extreg(extendchannel);
	(void)pConfig;
}

/**
 * Bind
 */
void board118_bind(void)
{
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS){
		opna_idx = 1;
	}else{
		opna_idx = 0;
	}
	if(g_nSoundID==SOUNDID_PC_9801_86_WSS){
		opna_bind(&g_opna[opna_idx]);
		cs4231io_bind();
		//cbuscore_attachsndex(0x188, ymf_o, ymf_i);
		
		iocore_attachout(0xb460, ymf_oa460);
		iocore_attachinp(0xb460, ymf_ia460);
		
		//iocore_attachout(0x1488, ym_o1488);
		//iocore_attachinp(0x1488, ym_i1488);
		//iocore_attachout(0x1489, ym_o1489);
		//iocore_attachout(0x148a, ym_o148a);
		//iocore_attachout(0x148b, ym_o148b);
		opl3_bind(&g_opl3);

		//iocore_attachout(0x148e,csctrl_o148e);
		//iocore_attachinp(0x148e,csctrl_i148e);
		//iocore_attachout(0x148f,csctrl_o148f);
		//iocore_attachinp(0x148f,csctrl_i148f);
		//iocore_attachout(0x548e, wss_o548e);// YMF-701
		//iocore_attachinp(0x548e, wss_i548e);// YMF-701
		//iocore_attachinp(0x548f, wss_i548f);// YMF-701

		//iocore_attachinp(0x486,csctrl_i486);
		//iocore_attachinp(0x1480, ymf_ia460);
		//iocore_attachinp(0x1481, ymf_ia460);
	//	
	////偽SB-16 mode
	//	iocore_attachout(0x20d2, ym_o1488);//sb16 opl
	//	iocore_attachinp(0x20d2, ym_i1488);//sb16 opl
	//	iocore_attachout(0x21d2, ym_o1489);//sb16 opl
	//	iocore_attachout(0x22d2, ym_o148a);//sb16 opl3
	//	iocore_attachout(0x23d2, ym_o148b);//sb16 opl3
	//	iocore_attachout(0x28d2, ym_o1488);//sb16 opl?
	//	iocore_attachinp(0x28d2, ym_i1488);//sb16 opl
	//	iocore_attachout(0x29d2, ym_o1489);//sb16 opl?
	//	iocore_attachinp(0x81d2, csctrl_i486);// sb16 midi port
	//	iocore_attachinp(0x2ad2, sb98_i2ad2);// DSP Read Data Port
	//	iocore_attachinp(0x2ed2, sb98_i2ed2);// DSP Read Buffer Status (Bit 7)

		//iocore_attachinp(0x881e, wss_i881e);
		//iocore_attachinp(0x148c, wss_i148c);
		//iocore_attachinp(0x148d, wss_i148d);
	}else{
		opna_bind(&g_opna[opna_idx]);
		cs4231io_bind();
		cbuscore_attachsndex(0x188, ymf_o, ymf_i);

		iocore_attachout(0x1488, ym_o1488);
		iocore_attachinp(0x1488, ym_i1488);
		iocore_attachout(0x1489, ym_o1489);
		iocore_attachout(0x148a, ym_o148a);
		iocore_attachout(0x148b, ym_o148b);
		opl3_bind(&g_opl3);

		iocore_attachout(0xa460, ymf_oa460);
		iocore_attachinp(0xa460, ymf_ia460);

		iocore_attachout(0x148e,csctrl_o148e);
		iocore_attachinp(0x148e,csctrl_i148e);
		iocore_attachout(0x148f,csctrl_o148f);
		iocore_attachinp(0x148f,csctrl_i148f);
		iocore_attachout(0x548e, wss_o548e);// YMF-701
		iocore_attachinp(0x548e, wss_i548e);// YMF-701
		iocore_attachinp(0x548f, wss_i548f);// YMF-701

		iocore_attachinp(0x486,csctrl_i486);
		iocore_attachinp(0x1480, ymf_ia460);
		iocore_attachinp(0x1481, ymf_ia460);

	//偽SB-16 mode
		iocore_attachout(0x20d2, ym_o1488);//sb16 opl
		iocore_attachinp(0x20d2, ym_i1488);//sb16 opl
		iocore_attachout(0x21d2, ym_o1489);//sb16 opl
		iocore_attachout(0x22d2, ym_o148a);//sb16 opl3
		iocore_attachout(0x23d2, ym_o148b);//sb16 opl3
		iocore_attachout(0x28d2, ym_o1488);//sb16 opl?
		iocore_attachinp(0x28d2, ym_i1488);//sb16 opl
		iocore_attachout(0x29d2, ym_o1489);//sb16 opl?
		iocore_attachinp(0x81d2, csctrl_i486);// sb16 midi port
		iocore_attachinp(0x2ad2, sb98_i2ad2);// DSP Read Data Port
		iocore_attachinp(0x2ed2, sb98_i2ed2);// DSP Read Buffer Status (Bit 7)

		iocore_attachinp(0x881e, wss_i881e);
		iocore_attachinp(0x148c, wss_i148c);
		iocore_attachinp(0x148d, wss_i148d);
	}
}
