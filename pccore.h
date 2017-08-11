/**
 * @file	pccore.h
 * @brief	Interface of the core
 */

#pragma once

#include "nevent.h"

enum {
	PCBASECLOCK25		= 2457600,
	PCBASECLOCK20		= 1996800
};

enum {
	CPUMODE_8MHZ		= 0x20,

	PCMODEL_VF			= 0,
	PCMODEL_VM			= 1,
	PCMODEL_VX			= 2,
	PCMODELMASK			= 0x3f,
	PCMODEL_PC9821		= 0x40,
	PCMODEL_EPSON		= 0x80,

	PCHDD_SASI			= 0x01,
	PCHDD_SCSI			= 0x02,
	PCHDD_IDE			= 0x04,

	PCROM_BIOS			= 0x01,
	PCROM_SOUND			= 0x02,
	PCROM_SASI			= 0x04,
	PCROM_SCSI			= 0x08,
	PCROM_BIOS9821		= 0x10,

	PCCBUS_PC9861K		= 0x0001,
	PCCBUS_MPU98		= 0x0002
};

/**
 * Sound ID
 */
enum tagSoundId
{
	SOUNDID_NONE				= 0,		/*!< No boards */
	SOUNDID_PC_9801_14			= 0x01,		/*!< PC-9801-14 */ 
	SOUNDID_PC_9801_26K			= 0x02,		/*!< PC-9801-26K */ 
	SOUNDID_PC_9801_86			= 0x04,		/*!< PC-9801-86 */ 
	SOUNDID_PC_9801_86_26K		= 0x06,		/*!< PC-9801-86 + 26K */ 
	SOUNDID_PC_9801_118			= 0x08,		/*!< PC-9801-118 */
	SOUNDID_PC_9801_86_ADPCM	= 0x14,		/*!< PC-9801-86 with ADPCM */
	SOUNDID_SPEAKBOARD			= 0x20,		/*!< Speak board */
	SOUNDID_SPARKBOARD			= 0x40,		/*!< Spark board */
	SOUNDID_AMD98				= 0x80,		/*!< AMD-98 */
	SOUNDID_SOUNDORCHESTRA		= 0x32,		/*!< SOUND ORCHESTRA */
	SOUNDID_SOUNDORCHESTRAV		= 0x82,		/*!< SOUND ORCHESTRA-V */

#if defined(SUPPORT_PX)
	SOUNDID_PX1					= 0x30,
	SOUNDID_PX2					= 0x50,
#endif	/* defined(SUPPORT_PX) */
};
typedef enum tagSoundId		SOUNDID;

/**
 * @brief config
 */
struct tagNP2Config
{
	// エミュレート中によく参照される奴
	UINT8	uPD72020;
	UINT8	DISPSYNC;
	UINT8	RASTER;
	UINT8	realpal;
	UINT8	LCD_MODE;
	UINT8	skipline;
	UINT16	skiplight;

	UINT8	KEY_MODE;
	UINT8	XSHIFT;
	UINT8	BTN_RAPID;
	UINT8	BTN_MODE;

	UINT8	dipsw[3];
	UINT8	MOUSERAPID;

	UINT8	calendar;
	UINT8	usefd144;
	UINT8	wait[6];
	
	UINT8	davolume;
	UINT8	modelnum;

	UINT8	timerfix;
	
	// リセット時とかあんまり参照されない奴
	OEMCHAR	model[8];
	UINT	baseclock;
	UINT	multiple;
	
	UINT8	usebios;

	UINT8	memsw[8];

	UINT8	ITF_WORK;
	UINT8	EXTMEM;
	UINT8	grcg;
	UINT8	color16;
	UINT32	BG_COLOR;
	UINT32	FG_COLOR;

	UINT32	samplingrate;
	UINT16	delayms;
	UINT8	SOUND_SW;
	UINT8	snd_x;

	UINT8	snd14opt[3];
	UINT8	snd26opt;
	UINT8	snd86opt;
	UINT8	spbopt;
	UINT8	spb_vrc;												// ver0.30
	UINT8	spb_vrl;												// ver0.30
	UINT8	spb_x;													// ver0.30

	UINT8	BEEP_VOL;
	UINT8	vol14[6];
	UINT8	vol_fm;
	UINT8	vol_ssg;
	UINT8	vol_adpcm;
	UINT8	vol_pcm;
	UINT8	vol_rhythm;

	UINT8	mpuenable;
	UINT8	mpuopt;
	UINT8	mpu_at;

	UINT8	pc9861enable;
	UINT8	pc9861sw[3];
	UINT8	pc9861jmp[6];

#if defined(SUPPORT_FMGEN)
	UINT8	fmgen;
#endif	/* SUPPORT_FMGEN */

	UINT8	fddequip;
	UINT8	MOTOR;
	UINT8	MOTORVOL;
	UINT8	PROTECTMEM;
	UINT8	hdrvacc;
	UINT8	hdrvenable;
	
	UINT8	savefddfile;											// ver0.86w rev20
	OEMCHAR	fddfile[4][MAX_PATH];									// ver0.86w rev20

#if defined(SUPPORT_IDEIO)
	OEMCHAR	sasihdd[4][MAX_PATH];									// ver0.86w
	UINT8	idetype[4];												// ver0.86w
	OEMCHAR	idecd[4][MAX_PATH];										// ver0.85w
	UINT8	idebios;												// ver0.86w rev20
	UINT32	iderwait; // IDE読み取りの割り込み遅延時間(clock)。  np21w ver0.86 rev19
	UINT32	idewwait; // IDE書き込みの割り込み遅延時間(clock)。  np21w ver0.86 rev18
	UINT32	idemwait; // IDE BIOSがある場合の割り込み遅延最小値  np21w ver0.86 rev26
#else
	OEMCHAR	sasihdd[2][MAX_PATH];									// ver0.74
#endif
#if defined(SUPPORT_SCSI)
	OEMCHAR	scsihdd[4][MAX_PATH];									// ver0.74
#endif
	OEMCHAR	fontfile[MAX_PATH];
	OEMCHAR	biospath[MAX_PATH];
	OEMCHAR	hdrvroot[MAX_PATH];

#ifdef SUPPORT_NET
	OEMCHAR	np2nettap[MAX_PATH];
	UINT8	np2netpmm;
#endif
#ifdef SUPPORT_LGY98
	UINT8	uselgy98;
	UINT8	lgy98mac[6];
#endif

	UINT8	winntfix;
#if defined(SUPPORT_STATSAVE)
	UINT8	statsave;
#endif
	UINT8	fddrive3;
	UINT8	fddrive4;
};
typedef struct tagNP2Config  NP2CFG;		/*!< The define of config */

typedef struct {
	UINT32	baseclock;
	UINT	multiple;

	UINT8	cpumode;
	UINT8	model;
	UINT8	hddif;
	UINT8	extmem; // LARGEMEM // UINT16	extmem;
	UINT8	dipsw[3];		// リセット時のDIPSW
	UINT8	rom;

	SOUNDID sound;
	UINT32	device;

	UINT32	realclock;
} PCCORE;

enum {
	COREEVENT_SHUT		= 0,
	COREEVENT_RESET		= 1,
	COREEVENT_EXIT		= 2
};

typedef struct
{
	UINT8	screenupdate;
	UINT8	screendispflag;
	UINT8	drawframe;
	UINT8	hardwarereset;
} PCSTAT;


#ifdef __cplusplus
extern "C" {
#endif

extern const OEMCHAR np2version[];

extern	NP2CFG	np2cfg;
extern	PCCORE	pccore;
extern	PCSTAT	pcstat;
extern	UINT8	soundrenewal;
extern	UINT	drawcount;
#if defined(SUPPORT_FMGEN)
extern	UINT8	enable_fmgen;
#endif	/* SUPPORT_FMGEN */

void getbiospath(OEMCHAR *path, const OEMCHAR *fname, int maxlen);
void screendisp(NEVENTITEM item);
void screenvsync(NEVENTITEM item);


void pccore_cfgupdate(void);

void pccore_init(void);
void pccore_term(void);
void pccore_reset(void);
void pccore_exec(BOOL draw);

void pccore_postevent(UINT32 event);

#ifdef __cplusplus
}
#endif

