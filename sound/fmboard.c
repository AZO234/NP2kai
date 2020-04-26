#include	"compiler.h"
#include	"joymng.h"
#include	"soundmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"board14.h"
#include	"board26k.h"
#include	"board86.h"
#include	"boardx2.h"
#include	"board118.h"
#include	"boardspb.h"
#if defined(SUPPORT_PX)
#include	"boardpx.h"
#endif	// defined(SUPPORT_PX)
#include	"boardso.h"
#include	"boardmo.h"
#include	"boardlol.h"
#include	"amd98.h"
#if defined(SUPPORT_SOUND_SB16)
#include	"boardsb16.h"
#endif	// defined(SUPPORT_SOUND_SB16)
#include	"pcm86io.h"
#include	"cs4231io.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"beep.h"
#include	"soundrom.h"
#include	"keydisp.h"
#include	"keystat.h"


	SOUNDID g_nSoundID;
	OPL3 g_opl3[OPL3_MAX]; // index: 0=PC-9891-118, 1=Sound Blaster 16, 2=Sound Orchestra
	OPNA g_opna[OPNA_MAX];
#ifdef USE_MAME
	void *g_mame_opl3[OPL3_MAX] = {0};
#endif
	
#if defined(SUPPORT_SOUND_SB16)
	SB16		g_sb16;
#endif	// defined(SUPPORT_SOUND_SB16)

	_PCM86		g_pcm86;
	_CS4231		cs4231;

static void	(*extfn)(REG8 enable);


// ----

static	REG8	s_rapids = 0;

REG8 fmboard_getjoy(POPNA opna)
{
	REG8 ret;

	s_rapids ^= 0xf0;											// ver0.28
	ret = 0xff;
	if (!(opna->s.reg[15] & 0x40))
	{
//		ret &= (joymng_getstat() | (s_rapids & 0x30));
		ret &= joymng_getstat();
		if (np2cfg.KEY_MODE == 1)
		{
			ret &= keystat_getjoy();
		}
	}
	else
	{
		if (np2cfg.KEY_MODE == 2)
		{
			ret &= keystat_getjoy();
		}
	}
	if (np2cfg.BTN_RAPID)
	{
		ret |= s_rapids;
	}

	// rapidと非rapidを合成								// ver0.28
	ret &= ((ret >> 2) | (~0x30));

	if (np2cfg.BTN_MODE)
	{
		UINT8 bit1 = (ret & 0x20) >> 1;					// ver0.28
		UINT8 bit2 = (ret & 0x10) << 1;
		ret = (ret & (~0x30)) | bit1 | bit2;
	}

	// intr 反映して終わり								// ver0.28
	ret &= 0x3f;
	ret |= opna->s.intr;
	return ret;
}


// ----

void fmboard_extreg(void (*ext)(REG8 enable)) {

	extfn = ext;
}

void fmboard_extenable(REG8 enable) {

	if (extfn) {
		(*extfn)(enable);
	}
}



// ----

/**
 * Constructor
 */
void fmboard_construct(void)
{
	UINT i;

	for (i = 0; i < NELEMENTS(g_opna); i++)
	{
		opna_construct(&g_opna[i]);
	}
	for (i = 0; i < NELEMENTS(g_opl3); i++)
	{
		opl3_construct(&g_opl3[i]);
	}
}

/**
 * Destructor
 */
void fmboard_destruct(void)
{
	UINT i;

	for (i = 0; i < NELEMENTS(g_opna); i++)
	{
		opna_destruct(&g_opna[i]);
	}
	for (i = 0; i < NELEMENTS(g_opl3); i++)
	{
		opl3_destruct(&g_opl3[i]);
	}

	board118_finalize();
#ifdef SUPPORT_SOUND_SB16
	boardsb16_finalize();
#endif
}

/**
 * Reset
 * @param[in] pConfig The pointer of config
 * @param[in] nSoundId The sound ID
 */
void fmboard_reset(const NP2CFG *pConfig, SOUNDID nSoundID)
{
	UINT i;

	soundrom_reset();
	beep_reset();												// ver0.27a

	if (g_nSoundID != nSoundID)
	{
		for (i = 0; i < NELEMENTS(g_opna); i++)
		{
			opna_reset(&g_opna[i], 0);
		}
		for (i = 0; i < NELEMENTS(g_opl3); i++)
		{
			opl3_reset(&g_opl3[i], 0);
		}
	}

	extfn = NULL;
	pcm86_reset();
	cs4231_reset();

	board14_reset(pConfig, (nSoundID == SOUNDID_PC_9801_14) ? TRUE : FALSE);
	amd98_reset(pConfig);
	
	g_nSoundID = nSoundID; // XXX: 先に設定
	switch (nSoundID)
	{
		case SOUNDID_PC_9801_14:
			break;

		case SOUNDID_PC_9801_26K:
			board26k_reset(pConfig);
			break;

		case SOUNDID_PC_9801_86:
			board86_reset(pConfig, FALSE);
			break;

		case SOUNDID_PC_9801_86_26K:
			boardx2_reset(pConfig);
			break;

		case SOUNDID_PC_9801_118:
			board118_reset(pConfig);
			break;
			
		case SOUNDID_PC_9801_86_WSS:
			board118_reset(pConfig);
			board86_reset(pConfig, FALSE);
			break;
			
		case SOUNDID_PC_9801_86_118:
			board118_reset(pConfig);
			board86_reset(pConfig, FALSE);
			break;
			
		case SOUNDID_MATE_X_PCM:
			board118_reset(pConfig);
			break;
			
		case SOUNDID_PC_9801_86_ADPCM:
			board86_reset(pConfig, TRUE);
			break;

		case SOUNDID_WAVESTAR:
			board118_reset(pConfig);
			board86_reset(pConfig, FALSE);
			break;

		case SOUNDID_SPEAKBOARD:
			boardspb_reset(pConfig, 0);
			break;

		case SOUNDID_86_SPEAKBOARD:
			boardspb_reset(pConfig, 1);
			board86_reset(pConfig, FALSE);
			break;

		case SOUNDID_SPARKBOARD:
			boardspr_reset(pConfig);
			break;

		case SOUNDID_AMD98:
			amd98_reset(pConfig);
			break;

		case SOUNDID_SOUNDORCHESTRA:
			boardso_reset(pConfig, FALSE);
			break;

		case SOUNDID_SOUNDORCHESTRAV:
			boardso_reset(pConfig, TRUE);
			break;
			
		case SOUNDID_LITTLEORCHESTRAL:
			boardlol_reset(pConfig);
			break;

		case SOUNDID_MMORCHESTRA:
			boardmo_reset(pConfig);
			break;
			
#if defined(SUPPORT_SOUND_SB16)
		case SOUNDID_SB16:
			boardsb16_reset(pConfig);
			break;
			
		case SOUNDID_PC_9801_86_SB16:
			boardsb16_reset(pConfig);
			board86_reset(pConfig, FALSE);
			break;
			
		case SOUNDID_WSS_SB16:
			boardsb16_reset(pConfig);
			board118_reset(pConfig);
			break;
			
		case SOUNDID_PC_9801_86_WSS_SB16:
			boardsb16_reset(pConfig);
			board118_reset(pConfig);
			board86_reset(pConfig, FALSE);
			break;
			break;
			
		case SOUNDID_PC_9801_118_SB16:
			boardsb16_reset(pConfig);
			board118_reset(pConfig);
			break;
			
		case SOUNDID_PC_9801_86_118_SB16:
			boardsb16_reset(pConfig);
			board118_reset(pConfig);
			board86_reset(pConfig, FALSE);
			break;
#endif

#if defined(SUPPORT_PX)
		case SOUNDID_PX1:
			boardpx1_reset(pConfig);
			break;

		case SOUNDID_PX2:
			boardpx2_reset(pConfig);
			break;
#endif	// defined(SUPPORT_PX)

		default:
			g_nSoundID = SOUNDID_NONE;
			break;
	}
	soundmng_setreverse(pConfig->snd_x);
	opngen_setVR(pConfig->spb_vrc, pConfig->spb_vrl);
}

void fmboard_bind(void) {

	keydisp_reset();
	switch (g_nSoundID)
	{
		case SOUNDID_PC_9801_14:
			board14_bind();
			break;

		case SOUNDID_PC_9801_26K:
			board26k_bind();
			break;

		case SOUNDID_PC_9801_86:
			board86_bind();
			break;

		case SOUNDID_PC_9801_86_26K:
			boardx2_bind();
			break;

		case SOUNDID_PC_9801_118:
			board118_bind();
			break;
			
		case SOUNDID_PC_9801_86_WSS:
			board118_bind();
			board86_bind();
			break;
			
		case SOUNDID_PC_9801_86_118:
			board118_bind();
			board86_bind();
			break;
			
		case SOUNDID_MATE_X_PCM:
			board118_bind();
			break;
			
		case SOUNDID_PC_9801_86_ADPCM:
			board86_bind();
			break;

		case SOUNDID_WAVESTAR:
			board118_bind();
			board86_bind();
			break;
			
		case SOUNDID_SPEAKBOARD:
			boardspb_bind();
			break;
			
		case SOUNDID_86_SPEAKBOARD:
			boardspb_bind();
			board86_bind();
			break;

		case SOUNDID_SPARKBOARD:
			boardspr_bind();
			break;

		case SOUNDID_AMD98:
			amd98_bind();
			break;

		case SOUNDID_SOUNDORCHESTRA:
		case SOUNDID_SOUNDORCHESTRAV:
			boardso_bind();
			break;
			
		case SOUNDID_LITTLEORCHESTRAL:
			boardlol_bind();
			break;

		case SOUNDID_MMORCHESTRA:
			boardmo_bind();
			break;

#if defined(SUPPORT_SOUND_SB16)
		case SOUNDID_SB16:
			boardsb16_bind();
			break;
			
		case SOUNDID_PC_9801_86_SB16:
			boardsb16_bind();
			board86_bind();
			break;
			
		case SOUNDID_WSS_SB16:
			boardsb16_bind();
			board118_bind();
			break;
			
		case SOUNDID_PC_9801_86_WSS_SB16:
			boardsb16_bind();
			board118_bind();
			board86_bind();
			break;
			
		case SOUNDID_PC_9801_118_SB16:
			boardsb16_bind();
			board118_bind();
			break;
			
		case SOUNDID_PC_9801_86_118_SB16:
			boardsb16_bind();
			board118_bind();
			board86_bind();
			break;
#endif	// defined(SUPPORT_SOUND_SB16)

#if defined(SUPPORT_PX)
		case SOUNDID_PX1:
			boardpx1_bind();
			break;

		case SOUNDID_PX2:
			boardpx2_bind();
			break;
#endif	// defined(SUPPORT_PX)

		default:
			break;
	}

	sound_streamregist(&g_beep, (SOUNDCB)beep_getpcm);
}

void fmboard_unbind(void) {

	switch (g_nSoundID)
	{
		case SOUNDID_PC_9801_14:
			board14_unbind();
			break;

		case SOUNDID_PC_9801_26K:
			board26k_unbind();
			break;

		case SOUNDID_PC_9801_86:
			board86_unbind();
			break;

		case SOUNDID_PC_9801_86_26K:
			boardx2_unbind();
			break;

		case SOUNDID_PC_9801_118:
			board118_unbind();
			break;
			
		case SOUNDID_PC_9801_86_WSS:
			board118_unbind();
			board86_unbind();
			break;
			
		case SOUNDID_PC_9801_86_118:
			board118_unbind();
			board86_unbind();
			break;
			
		case SOUNDID_MATE_X_PCM:
			board118_unbind();
			break;
			
		case SOUNDID_PC_9801_86_ADPCM:
			board86_unbind();
			break;

		case SOUNDID_WAVESTAR:
			board118_unbind();
			board86_unbind();
			break;
			
		case SOUNDID_SPEAKBOARD:
			boardspb_unbind();
			break;
			
		case SOUNDID_86_SPEAKBOARD:
			board86_unbind();
			boardspb_unbind();
			break;

		case SOUNDID_SPARKBOARD:
			boardspr_unbind();
			break;

		case SOUNDID_AMD98:
			amd98_unbind();
			break;

		case SOUNDID_SOUNDORCHESTRA:
		case SOUNDID_SOUNDORCHESTRAV:
			boardso_unbind();
			break;
			
		case SOUNDID_LITTLEORCHESTRAL:
			boardlol_unbind();
			break;

		case SOUNDID_MMORCHESTRA:
			boardmo_unbind();
			break;

#if defined(SUPPORT_SOUND_SB16)
		case SOUNDID_SB16:
			boardsb16_unbind();
			break;
			
		case SOUNDID_PC_9801_86_SB16:
			boardsb16_unbind();
			board86_unbind();
			break;
			
		case SOUNDID_WSS_SB16:
			boardsb16_unbind();
			board118_unbind();
			break;
			
		case SOUNDID_PC_9801_86_WSS_SB16:
			boardsb16_unbind();
			board118_unbind();
			board86_unbind();
			break;
			
		case SOUNDID_PC_9801_118_SB16:
			boardsb16_unbind();
			board118_unbind();
			break;
			
		case SOUNDID_PC_9801_86_118_SB16:
			boardsb16_unbind();
			board118_unbind();
			board86_unbind();
			break;
#endif	// defined(SUPPORT_SOUND_SB16)

#if defined(SUPPORT_PX)
		case SOUNDID_PX1:
			boardpx1_unbind();
			break;

		case SOUNDID_PX2:
			boardpx2_unbind();
			break;
#endif	// defined(SUPPORT_PX)

		default:
			break;
	}
}
