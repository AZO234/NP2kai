/**
 * @file	soundrom.c
 * @brief	Sound BIOS support
 *
 * 実機SOUND.ROMがない場合は命令フックを利用してnp2側がSound BIOSの動作を再現します。
 * 実機SOUND.ROMがある場合はそれを使用します。
 * 
 * 参考文献
 * ・PC-9800シリーズテクニカルデータブック
 */

#include "compiler.h"
#include "cpucore.h"
#include "pccore.h"
#include "dosio.h"
#include "soundrom.h"
#include <stddef.h>
#if defined(SUPPORT_EMU_SOUNDBIOS)
#include <io/iocore.h>
#if !defined(DISABLE_SOUND)
#include "fmboard.h"
#endif
#include "bios/bios.h"
#endif

SOUNDROM soundrom;

static const OEMCHAR file_sound[] = OEMTEXT("sound");
static const OEMCHAR file_extrom[] = OEMTEXT(".rom");

#if defined(SUPPORT_EMU_SOUNDBIOS)

#define SB_INIT_OFF		0x0000
#define SB_DESC_OFF		0x2e00
#define SB_API_OFF		0x2e08
#define SB_IRQ_OFF		0x2e0a
#define SB_CB_OFF		0x2e0c
/*
 * エミュレーションSound BIOSの状態はゲストRAMへ格納する
 * Sound BIOSにはユーザー利用不可のワークエリアがあるので、そこをデータ保存用に借りている。
 * 実際のSound BIOSの形式ではないのでそれに依存するソフトは恐らく動かないが、文献において
 * ユーザー利用不可と明記されている（＆そもそも形式も不明な）エリアなので通常問題ないと思われる。
 *
 * 0000:05E0h  4 bytes : INITIALIZEで渡された作業領域セグメントとHLE識別子
 * ES:0000h   256 bytes : Sound BIOS公開共通制御情報
 * ES:0100h   512 bytes : HLE内部で必要な非公開状態
 */
#define SOUNDBIOS_CHANNELS           6
#define SOUNDBIOS_PARAMS             50
#define SB_EMU_SYS_BASE              0x05e0
#define SB_EMU_LOCAL_BASE            0x0100
#define SB_EMU_STATE_MAGIC           0x3253  /* "S2" little-endian */
#define SB_EMU_COMMON_CH_SIZE        0x20
#define SB_EMU_COMMON_GLOBAL_BASE    0x00c0
#define SB_EMU_COMMON_GLOBAL_SIZE    2
#define SB_EMU_LOCAL_GLOBAL_SIZE     0x18
#define SB_EMU_LOCAL_CH_SIZE         0x18
#define SB_EMU_SSG_PARAM_COUNT       5

#define SB_EMU_LOCAL_CH_BASE         (SB_EMU_LOCAL_GLOBAL_SIZE)
#define SB_EMU_LOCAL_FMPARAM_BASE    (SB_EMU_LOCAL_CH_BASE + \
                                      SOUNDBIOS_CHANNELS * SB_EMU_LOCAL_CH_SIZE)
#define SB_EMU_LOCAL_FMPARAM_SIZE    (SOUNDBIOS_PARAMS * 2)
#define SB_EMU_LOCAL_SSGPARAM_BASE   (SB_EMU_LOCAL_FMPARAM_BASE + \
                                      3 * SB_EMU_LOCAL_FMPARAM_SIZE)
#define SB_EMU_LOCAL_USED            (SB_EMU_LOCAL_SSGPARAM_BASE + \
                                      3 * SB_EMU_SSG_PARAM_COUNT * 2)

#define SB_CALLBACK_ENABLED(c)       (((c)->callback_cond & 0x8000) != 0)
#define SB_CALLBACK_LENGTH(c)        ((UINT16)((c)->callback_cond & 0x7fff))

#pragma pack(push, 1)
typedef struct {
	UINT16 workseg;
	UINT16 magic;
} SBEMU_SYS_STATE;

typedef struct {
	UINT16 seg;
	UINT16 start;
	UINT16 capacity;
} SBEMU_BUFFER_INFO;

typedef struct {
	/* 上位プログラムから参照できるチャネル別共通制御情報(20h bytes)。 */
	UINT16 seg;
	UINT16 start;
	UINT16 capacity;
	UINT16 ptr;
	UINT16 remain;
	UINT16 callback_cond;	/* D15=enable, D0..14=threshold */
	UINT16 callback_off;
	UINT16 callback_seg;
	UINT8 key;
	UINT8 length;
	UINT8 touch;
	UINT8 active;		/* 00h / FFh */
	UINT8 public_reserved[12];

	/* 公開領域だけでは再構築できないチャネル状態。ローカル作業領域へ保存する。 */
	UINT16 wait;
	UINT16 gate;
	UINT16 base_fnum;
	UINT16 lfo_phase;
	UINT16 lfo_delay;
	SINT16 lfo_sample;
	UINT8 volume;
	UINT8 volume_valid;
	UINT8 block;
	UINT8 keyon;
	UINT8 keymask;
	UINT8 saved_keymask;
	UINT8 ssg_level;
	UINT8 lfo;
	UINT8 callback_pending;
	UINT8 lfo_sample_valid;
	UINT8 private_reserved[2];

	UINT16 params[SOUNDBIOS_PARAMS];
} SOUNDBIOS_CHANNEL;

typedef struct {
	/* IRQ、タイマ、PLAY制御などSound BIOS全体に属する非公開状態。 */
	UINT8 initialized;
	UINT8 playing;
	UINT8 paused;
	UINT8 irq_hooked;
	UINT8 irq;
	UINT8 irq_vector;
	UINT8 old_imr_master;
	UINT8 old_imr_slave;
	UINT8 timer_a_running;
	UINT8 timer_b_running;
	UINT8 stop_request;
	UINT8 resume_request;
	UINT16 old_irq_off;
	UINT16 old_irq_seg;
	UINT16 tempo_accum;
	UINT8 local_reserved[6];

	/* システム共通域または公開共通制御情報に対応する作業状態。 */
	UINT16 workseg;
	UINT8 tempo;
	UINT8 saved_keys;
	SOUNDBIOS_CHANNEL ch[SOUNDBIOS_CHANNELS];
} SOUNDBIOSEMU;
#pragma pack(pop)

// 作業領域 原本はゲストRAMから読み書きする
static SOUNDBIOSEMU soundbiosemu;

/* ブロック転送する構造体範囲のサイズと境界が問題ないかビルド時に検査 */
typedef char SBEMU_SYS_STATE_size_check[(sizeof(SBEMU_SYS_STATE) == 4) ? 1 : -1];
typedef char SBEMU_BUFFER_INFO_size_check[(sizeof(SBEMU_BUFFER_INFO) == 6) ? 1 : -1];
typedef char SBEMU_CHANNEL_public_size_check[(offsetof(SOUNDBIOS_CHANNEL, wait) == 0x20) ? 1 : -1];
typedef char SBEMU_CHANNEL_local_size_check[((offsetof(SOUNDBIOS_CHANNEL, params) - offsetof(SOUNDBIOS_CHANNEL, wait)) == SB_EMU_LOCAL_CH_SIZE) ? 1 : -1];
typedef char SBEMU_GLOBAL_local_size_check[(offsetof(SOUNDBIOSEMU, workseg) == SB_EMU_LOCAL_GLOBAL_SIZE) ? 1 : -1];
typedef char SBEMU_LOCAL_used_check[(SB_EMU_LOCAL_USED <= 0x0200) ? 1 : -1];

/*
 * HLEの時間進行にはOPNタイマを使用する。Timer-AはStep Time/PLAY、
 * Timer-Bは固定周期のハードウェア制御/LFO処理を担当する。
 */
#define SB_TIMERA_BASE		0x01c0
#define SB_TIMERB_BASE		0x00e3

/*
 * SET/READ/WRITE PARAで公開されているパラメータ番号は0〜49。
 *
 * SET PARA BLOCKの表にはNo.50としてINT_KY_SAVも記載されているが、外部から
 * 渡すブロック長はWORD形式100バイトまたはBYTE形式51バイトと定義され、
 * READ/WRITE PARAもNo.0〜49だけを受け付ける。このためINT_KY_SAVは
 * 呼び出し側から設定するパラメータではなく、Sound BIOS側の作業情報として扱う。
 */
#define SB_P_FB_ALG		0
#define SB_P_AR1		1
#define SB_P_OPR_MSK	5
#define SB_P_DR1		6
#define SB_P_WAVE_LFO	10
#define SB_P_SR1		11
#define SB_P_SYNC_LFO	15
#define SB_P_RR1		16
#define SB_P_SPEED_LFO	20
#define SB_P_SL1		21
#define SB_P_PMOD_LFO	25
#define SB_P_OPLEVEL1	26
#define SB_P_AMOD_LFO	30
#define SB_P_KEYSCL1	31
#define SB_P_PMOS_LFO	35
#define SB_P_MULT1		36
#define SB_P_RESERVED1	40
#define SB_P_DETUN1		41
#define SB_P_RESERVED2	45
#define SB_P_AMOS1		46

/* YM2203のオペレータレジスタ順: OP1, OP2, OP3, OP4 */
static const UINT8 sb_opoff[4] = {0x00, 0x08, 0x04, 0x0c};

/* C〜BのF-number。オクターブ/ブロックは別に与える */
static const UINT16 sb_fnum[12] = {
	0x269, 0x28e, 0x2b4, 0x2de, 0x309, 0x338,
	0x369, 0x39c, 0x3d4, 0x40e, 0x44b, 0x48d
};

/* Sound BIOSで使用するSSGのO1、C〜Bのトーン周期 */
static const UINT16 sb_psgperiod[12] = {
	3820, 3604, 3404, 3212, 3032, 2860,
	2700, 2548, 2404, 2272, 2144, 2024
};

#if !defined(DISABLE_SOUND)
static void sb_timerupdate(BOOL force);
static void sb_keyoff(UINT ch);
static BOOL sb_anytrack(void);
static BOOL sb_anylfo(void);
static void sb_timerprograma(void);
static void sb_timerprogramb(void);
static void sb_hwregwrite(UINT reg, UINT8 value);
#endif

/* Sound BIOS HLE入口に置くフック命令を取得 */
static UINT8 sb_hookinst(void)
{
#if defined(USE_CUSTOM_HOOKINST)
	return bioshookinfo.hookinst;
#else
	return 0x90;
#endif
}

#endif	/* SUPPORT_EMU_SOUNDBIOS */

/* 指定名の16KB SOUND.ROMをBIOSパスから読み込み、指定アドレスへ配置 */
static BRESULT loadsoundrom(UINT address, const OEMCHAR *name)
{
	OEMCHAR romname[24];
	OEMCHAR path[MAX_PATH];
	FILEH fh;
	UINT rsize;

	file_cpyname(romname, file_sound, NELEMENTS(romname));
	if (name) {
		file_catname(romname, name, NELEMENTS(romname));
	}
	file_catname(romname, file_extrom, NELEMENTS(romname));
	getbiospath(path, romname, NELEMENTS(path));
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		goto lsr_err;
	}
	rsize = file_read(fh, mem + address, 0x4000);
	file_close(fh);
	if (rsize != 0x4000) {
		goto lsr_err;
	}
	file_cpyname(soundrom.name, romname, NELEMENTS(soundrom.name));
	soundrom.address = address;
	if (address == 0xd0000) {
		CPU_RAM_D000 &= ~(0x0f << 0);
	}
	else if (address == 0xd4000) {
		CPU_RAM_D000 &= ~(0x0f << 4);
	}
	return SUCCESS;

lsr_err:
	return FAILURE;
}

#if defined(SUPPORT_EMU_SOUNDBIOS)

#if !defined(DISABLE_SOUND)

static const UINT8 sb_ssg_param_index[SB_EMU_SSG_PARAM_COUNT] = {
	SB_P_WAVE_LFO, SB_P_SYNC_LFO, SB_P_SPEED_LFO,
	SB_P_PMOD_LFO, SB_P_PMOS_LFO
};

/* Sound BIOS HLEが使用する512バイトのローカル作業領域を初期化 */
static void sb_clear_local_area(UINT16 workseg)
{
	static const UINT8 zero[0x0200] = {0};
	MEMR_WRITES(workseg, SB_EMU_LOCAL_BASE, zero, sizeof(zero));
}

/* ゲストRAM上のHLE識別子を消去し、保存状態を無効化 */
static void sb_state_invalidate(void)
{
	SBEMU_SYS_STATE state;

	MEMR_READS(0, SB_EMU_SYS_BASE, &state, sizeof(state));
	state.magic = 0;
	MEMR_WRITES(0, SB_EMU_SYS_BASE, &state, sizeof(state));
}

/* CPUフック処理に必要なSound BIOS状態をゲストRAMから読む */
static BOOL sb_state_load(void)
{
	UINT ch;
	UINT slot;
	SBEMU_SYS_STATE sys;
	SOUNDBIOS_CHANNEL *c;

	ZeroMemory(&soundbiosemu, sizeof(soundbiosemu));
	MEMR_READS(0, SB_EMU_SYS_BASE, &sys, sizeof(sys));
	if (sys.magic != SB_EMU_STATE_MAGIC) {
		return FALSE;
	}

	soundbiosemu.workseg = sys.workseg;
	MEMR_READS(sys.workseg, SB_EMU_LOCAL_BASE, &soundbiosemu.initialized, SB_EMU_LOCAL_GLOBAL_SIZE);
	if (!soundbiosemu.initialized) {
		return FALSE;
	}
	MEMR_READS(sys.workseg, SB_EMU_COMMON_GLOBAL_BASE, &soundbiosemu.tempo, SB_EMU_COMMON_GLOBAL_SIZE);

	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		c = &soundbiosemu.ch[ch];
		MEMR_READS(sys.workseg, (UINT16)(ch * SB_EMU_COMMON_CH_SIZE), &c->seg, SB_EMU_COMMON_CH_SIZE);
		MEMR_READS(sys.workseg, (UINT16)(SB_EMU_LOCAL_BASE + SB_EMU_LOCAL_CH_BASE + ch * SB_EMU_LOCAL_CH_SIZE), &c->wait, SB_EMU_LOCAL_CH_SIZE);
		ZeroMemory(c->params, sizeof(c->params));
		if (ch < 3) {
			MEMR_READS(sys.workseg, (UINT16)(SB_EMU_LOCAL_BASE + SB_EMU_LOCAL_FMPARAM_BASE + ch * SB_EMU_LOCAL_FMPARAM_SIZE), c->params, SB_EMU_LOCAL_FMPARAM_SIZE);
		}
		else {
			for (slot = 0; slot < SB_EMU_SSG_PARAM_COUNT; slot++) {
				c->params[sb_ssg_param_index[slot]] = MEMR_READ16(sys.workseg, (UINT16)(SB_EMU_LOCAL_BASE + SB_EMU_LOCAL_SSGPARAM_BASE + (ch - 3) * SB_EMU_SSG_PARAM_COUNT * 2 + slot * 2));
			}
		}
	}
	return TRUE;
}

/* CPUフック処理で更新したSound BIOS状態をゲストRAMへ書く */
static void sb_state_store(void)
{
	UINT ch;
	UINT slot;
	SBEMU_SYS_STATE sys;
	SOUNDBIOS_CHANNEL *c;

	if (!soundbiosemu.initialized) return;

	sys.workseg = soundbiosemu.workseg;
	sys.magic = SB_EMU_STATE_MAGIC;
	MEMR_WRITES(0, SB_EMU_SYS_BASE, &sys, sizeof(sys));
	MEMR_WRITES(soundbiosemu.workseg, SB_EMU_LOCAL_BASE, &soundbiosemu.initialized, SB_EMU_LOCAL_GLOBAL_SIZE);

	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		c = &soundbiosemu.ch[ch];
		MEMR_WRITES(soundbiosemu.workseg, (UINT16)(ch * SB_EMU_COMMON_CH_SIZE), &c->seg, SB_EMU_COMMON_CH_SIZE);
		MEMR_WRITES(soundbiosemu.workseg, (UINT16)(SB_EMU_LOCAL_BASE + SB_EMU_LOCAL_CH_BASE + ch * SB_EMU_LOCAL_CH_SIZE), &c->wait, SB_EMU_LOCAL_CH_SIZE);
		if (ch < 3) {
			MEMR_WRITES(soundbiosemu.workseg, (UINT16)(SB_EMU_LOCAL_BASE + SB_EMU_LOCAL_FMPARAM_BASE + ch * SB_EMU_LOCAL_FMPARAM_SIZE), c->params, SB_EMU_LOCAL_FMPARAM_SIZE);
		}
		else {
			for (slot = 0; slot < SB_EMU_SSG_PARAM_COUNT; slot++) {
				MEMR_WRITE16(soundbiosemu.workseg, (UINT16)(SB_EMU_LOCAL_BASE + SB_EMU_LOCAL_SSGPARAM_BASE + (ch - 3) * SB_EMU_SSG_PARAM_COUNT * 2 + slot * 2), c->params[sb_ssg_param_index[slot]]);
			}
		}
	}
	MEMR_WRITES(soundbiosemu.workseg, SB_EMU_COMMON_GLOBAL_BASE, &soundbiosemu.tempo, SB_EMU_COMMON_GLOBAL_SIZE);
}

/* ROM上のSound BIOS公開エントリを指すセグメント値を求める */
static UINT16 sb_gethookseg(void)
{
	/* 通常のCC000hマッピングでは公開エントリテーブルはCEE00hにある。
	 * テーブル中のエントリオフセットはこの16バイト段落を基準とする。 */
	return (UINT16)((soundrom.address + SB_DESC_OFF) >> 4);
}

/* Sound BIOS APIの既定入口をINT D2hへ登録 */
static void sb_int_init(void)
{
	/* Sound BIOSではデフォルトでINT D2hを使用する */
	MEMR_WRITE16(0, 0x00d2 * 4, (UINT16)(SB_API_OFF - SB_DESC_OFF));
	MEMR_WRITE16(0, 0x00d2 * 4 + 2, sb_gethookseg());
}

/* 現在のPIC設定から、指定IRQに対応する割り込みベクタ番号を求める */
static UINT8 sb_irqvector(UINT8 irq)
{
	if (irq < 8) {
		return (UINT8)((pic.pi[0].icw[1] & 0xf8) | (irq & 7));
	}
	return (UINT8)((pic.pi[1].icw[1] & 0xf8) | (irq & 7));
}

/* Sound BIOSが処理したIRQについてPICへEOIを通知 */
static void sb_eoi(void)
{
	if (!soundbiosemu.irq_hooked) {
		return;
	}
	if (soundbiosemu.irq >= 8) {
		iocore_out8(0x08, 0x20);
	}
	iocore_out8(0x00, 0x20);
}

/* OPNのIRQベクタとPICマスクを退避し、Sound BIOSのIRQ入口へ差し替える
 * これによってOPNタイマー割り込みなどを捕捉して代行処理できる */
static void sb_hookirq(void)
{
	UINT16 vecoff;
	UINT16 hookseg;
	UINT8 master;
	UINT8 slave;

	if (soundbiosemu.irq_hooked) {
		return;
	}
	if (g_opna[0].s.irq > 15) {
		return;
	}

	soundbiosemu.irq = g_opna[0].s.irq;
	soundbiosemu.irq_vector = sb_irqvector(soundbiosemu.irq);
	vecoff = (UINT16)(soundbiosemu.irq_vector * 4);
	soundbiosemu.old_irq_off = MEMR_READ16(0, vecoff);
	soundbiosemu.old_irq_seg = MEMR_READ16(0, vecoff + 2);

	/* IRQ解除時に元へ戻せるよう、フック前のベクタをSound BIOS状態として保持する。 */

	hookseg = sb_gethookseg();
	MEMR_WRITE16(0, vecoff, 0x000a);
	MEMR_WRITE16(0, vecoff + 2, hookseg);

	soundbiosemu.old_imr_master = pic.pi[0].imr;
	soundbiosemu.old_imr_slave = pic.pi[1].imr;
	master = soundbiosemu.old_imr_master;
	slave = soundbiosemu.old_imr_slave;
	if (soundbiosemu.irq < 8) {
		master &= (UINT8)~(1 << soundbiosemu.irq);
	}
	else {
		slave &= (UINT8)~(1 << (soundbiosemu.irq - 8));
		master &= (UINT8)~PIC_SLAVE;
	}
	iocore_out8(0x02, master);
	iocore_out8(0x0a, slave);
	soundbiosemu.irq_hooked = 1;
}

/* Sound BIOSが変更したIRQベクタとPICマスクを元の状態へ戻す */
static void sb_unhookirq(void)
{
	UINT16 vecoff;
	UINT8 master;
	UINT8 slave;
	UINT8 bit;

	if (!soundbiosemu.irq_hooked) {
		return;
	}
	vecoff = (UINT16)(soundbiosemu.irq_vector * 4);
	MEMR_WRITE16(0, vecoff, soundbiosemu.old_irq_off);
	MEMR_WRITE16(0, vecoff + 2, soundbiosemu.old_irq_seg);

	/* Sound BIOSが使用するIRQのマスクビットだけを復元する。Sound BIOS動作中に
	 * 他デバイスが変更したマスク状態を上書きしない。 */
	master = pic.pi[0].imr;
	slave = pic.pi[1].imr;
	if (soundbiosemu.irq < 8) {
		bit = (UINT8)(1 << soundbiosemu.irq);
		if (soundbiosemu.old_imr_master & bit) master |= bit;
		else master &= (UINT8)~bit;
	}
	else {
		bit = (UINT8)(1 << (soundbiosemu.irq - 8));
		if (soundbiosemu.old_imr_slave & bit) slave |= bit;
		else slave &= (UINT8)~bit;
	}
	iocore_out8(0x02, master);
	iocore_out8(0x0a, slave);
	soundbiosemu.irq_hooked = 0;
}

/* OPNレジスタへ値を書き込む */
static void sb_regwrite(UINT reg, UINT8 value)
{
	if (reg >= 0x100) {
		return;
	}
	opna_writeRegister(&g_opna[0], reg, value);
}

/* OPNレジスタから現在のレジスタ値を取得 */
static UINT8 sb_regread(UINT reg)
{
	if (reg >= 0x100) {
		return 0;
	}
	return opna_readRegister(&g_opna[0], reg);
}

/* WRITE REGで直接変更されたキー状態やSSG音量をHLE論理状態へ反映 */
static void sb_track_userregwrite(UINT reg, UINT8 value)
{
	SOUNDBIOS_CHANNEL *c;
	UINT ch;

	if (reg == 0x28) {
		ch = value & 3;
		if (ch < 3) {
			c = &soundbiosemu.ch[ch];
			c->keymask = value & 0xf0;
			c->keyon = c->keymask ? 1 : 0;
			if (!c->keyon) {
				c->key = 0x80;
			}
		}
	}
	else if (reg == 0x07) {
		for (ch = 3; ch < 6; ch++) {
			c = &soundbiosemu.ch[ch];
			c->keyon = (value & (1 << (ch - 3))) ? 0 : 1;
			if (!c->keyon) {
				c->key = 0x80;
			}
		}
	}
	else if ((reg >= 0x08) && (reg <= 0x0a)) {
		ch = 3 + reg - 0x08;
		soundbiosemu.ch[ch].ssg_level = value & 0x1f;
		/* Key-OFFで実レジスタを変更しても要求された論理音量を保持する */
		soundbiosemu.ch[ch].volume = soundbiosemu.ch[ch].ssg_level;
	}
}

/* WRITE REG相当のOPN書き込みを行い、必要なHLE論理状態も反映 */
static void sb_userregwrite(UINT reg, UINT8 value)
{
	if (reg >= 0x100) {
		return;
	}
	opna_writeRegister(&g_opna[0], reg, value);
	sb_track_userregwrite(reg, value);
}

/* WRITE REG相当のOPN書き込みを行うのみ　HLE論理状態は触らない */
static void sb_hwregwrite(UINT reg, UINT8 value)
{
	if (reg >= 0x100) {
		return;
	}
	opna_writeRegister(&g_opna[0], reg, value);
}

/* OPNを既知の無音状態へ初期化 */
static void sb_soundinitregs(void)
{
	UINT reg;
	UINT ch;

	/* INITIALIZEでは音源側の状態を初期化する。タイマ制御はsb_timerupdate()に
	 * 任せるが、呼び出し側が音色パラメータを設定する前にFM/SSGの可聴状態を
	 * 一定の初期値へそろえる。 */
	for (reg = 0x00; reg <= 0x23; reg++) {
		sb_hwregwrite(reg, 0x00);
	}
	for (reg = 0x29; reg <= 0x2c; reg++) {
		sb_hwregwrite(reg, 0x00);
	}
	for (reg = 0x30; reg <= 0xb1; reg++) {
		sb_hwregwrite(reg, 0x00);
	}

	/* SSGレジスタをSound BIOS互換の無音初期値へ設定する。
	 * ミキサでは全トーン/ノイズ経路を無効のままとする。 */
	sb_hwregwrite(0x07, 0xbf);
	sb_hwregwrite(0x08, 0x08);
	sb_hwregwrite(0x09, 0x08);
	sb_hwregwrite(0x0a, 0x08);
	sb_hwregwrite(0x0b, 0xff);
	sb_hwregwrite(0x0c, 0x00);
	sb_hwregwrite(0x0d, 0x01);
	for (ch = 3; ch < 6; ch++) {
		soundbiosemu.ch[ch].ssg_level = 0x08;
		soundbiosemu.ch[ch].volume = 0x08;
	}
}

/* 周期処理が必要なLFOが1チャネルでも存在するかを調査 */
static BOOL sb_anylfo(void)
{
	UINT ch;
	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		/* 非同期LFOはKey-ON状態とは独立して動作させる */
		if (soundbiosemu.ch[ch].lfo &&
			(soundbiosemu.ch[ch].params[SB_P_SPEED_LFO] & 0x3fff)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* PLAY/NOTEの時間進行が必要なチャネルが存在するかを調査 */
static BOOL sb_anytrack(void)
{
	UINT ch;
	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		if (soundbiosemu.ch[ch].active || soundbiosemu.ch[ch].wait ||
			soundbiosemu.ch[ch].gate) {
			return TRUE;
		}
	}
	return FALSE;
}

/* PLAYのStep Time処理に使用するOPN Timer-Aの周期を設定 */
static void sb_timerprograma(void)
{
	UINT16 v;
	v = SB_TIMERA_BASE;
	sb_hwregwrite(0x24, (UINT8)(v >> 2));
	sb_hwregwrite(0x25, (UINT8)(v & 3));
}

/* LFOや停止・再開処理に使用するOPN Timer-Bの周期を設定 */
static void sb_timerprogramb(void)
{
	sb_hwregwrite(0x26, SB_TIMERB_BASE);
}

/* 現在の演奏状態に応じてTimer-A/Timer-Bの起動・停止状態を更新 */
static void sb_timerupdate(BOOL force)
{
	BOOL needa;
	BOOL needb;
	UINT8 mode;
	UINT8 run;

	if (!soundbiosemu.initialized) {
		return;
	}
	/* 公開仕様上のStep Time/PLAY処理と固定4msのハードウェア制御処理を
	 * 分離するため、HLEではOPNの2本のタイマを使い分ける。 */
	needa = !soundbiosemu.paused && soundbiosemu.playing && sb_anytrack();
	needb = soundbiosemu.stop_request || soundbiosemu.resume_request || (!soundbiosemu.paused && sb_anylfo());
	if (!force && (needa == (BOOL)soundbiosemu.timer_a_running) && (needb == (BOOL)soundbiosemu.timer_b_running)) {
		return;
	}
	if (needa && (!soundbiosemu.timer_a_running || force)) {
		sb_timerprograma();
	}
	if (needb && (!soundbiosemu.timer_b_running || force)) {
		sb_timerprogramb();
	}

	mode = g_opna[0].s.reg[0x27] & 0xc0;
	run = 0;
	if (needa) run |= 0x05;
	if (needb) run |= 0x0a;
	/* 残っているステータスを先に消去し、必要なタイマだけを開始/許可する */
	sb_hwregwrite(0x27, (UINT8)(mode | 0x30));
	sb_hwregwrite(0x27, (UINT8)(mode | run));
	soundbiosemu.timer_a_running = needa ? 1 : 0;
	soundbiosemu.timer_b_running = needb ? 1 : 0;
}

/* FMアルゴリズム番号からキャリアとなるオペレータのビットマスクを返す */
static UINT8 sb_carriermask(UINT8 alg)
{
	static const UINT8 mask[8] = {
		0x08, 0x08, 0x08, 0x08, 0x0a, 0x0e, 0x0e, 0x0f
	};
	return mask[alg & 7];
}

/* Sound BIOSのFM音色パラメータを対応するOPNレジスタへ反映 */
static void sb_apply_voice(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;
	UINT op;
	UINT8 dt;
	UINT8 mul;
	UINT8 ar;
	UINT8 ks;
	UINT8 dr;
	UINT8 sr;
	UINT8 rr;
	UINT8 sl;

	if (ch >= 3) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	for (op = 0; op < 4; op++) {
		dt = (UINT8)c->params[SB_P_DETUN1 + op];
		mul = (UINT8)c->params[SB_P_MULT1 + op] & 0x0f;
		/* Sound BIOSのエンベロープ時間は値が小さいほど短いが、OPNのrate値は逆方向 */
		ar = (UINT8)(0x1f - ((UINT8)c->params[SB_P_AR1 + op] & 0x1f));
		ks = (UINT8)c->params[SB_P_KEYSCL1 + op] & 3;
		dr = (UINT8)(0x1f - ((UINT8)c->params[SB_P_DR1 + op] & 0x1f));
		sr = (UINT8)(0x1f - ((UINT8)c->params[SB_P_SR1 + op] & 0x1f));
		rr = (UINT8)(0x0f - ((UINT8)c->params[SB_P_RR1 + op] & 0x0f));
		sl = (UINT8)(0x0f - ((UINT8)c->params[SB_P_SL1 + op] & 0x0f));
		/* Sound BIOSの符号付きDETUN(-4〜+3)をOPNのDTフィールドへ変換する */
		sb_regwrite(0x30 + sb_opoff[op] + ch, (UINT8)(((UINT8)(dt << 4)) | mul));
		sb_regwrite(0x50 + sb_opoff[op] + ch, (UINT8)((ks << 6) | ar));
		sb_regwrite(0x60 + sb_opoff[op] + ch, dr);
		sb_regwrite(0x70 + sb_opoff[op] + ch, sr);
		sb_regwrite(0x80 + sb_opoff[op] + ch, (UINT8)((sl << 4) | rr));
	}
	sb_regwrite(0xb0 + ch, (UINT8)c->params[SB_P_FB_ALG] & 0x3f);
}

/* チャネル音量と音色OPLEVELから各FMオペレータのTLを設定 */
static void sb_apply_volume(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;
	UINT8 carriers;
	UINT op;
	UINT level;
	UINT tl;

	if (ch >= 3) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	carriers = sb_carriermask((UINT8)c->params[SB_P_FB_ALG]);
	for (op = 0; op < 4; op++) {
		/* SET VOLUMEはキャリア音量を直接制御し、モジュレータは音色パラメータの
		 * OPLVL値を維持する。 */
		if ((carriers & (1 << op)) && c->volume_valid) {
			level = c->volume & 0x7f;
		}
		else {
			/* INITIALIZEだけではSET VOLUMEを実行した扱いにしない */
			level = c->params[SB_P_OPLEVEL1 + op] & 0x7f;
		}
		tl = 0x7f - level;
		sb_regwrite(0x40 + sb_opoff[op] + ch, (UINT8)tl);
	}
}

/* 変更された1個のFMパラメータを対応するOPNレジスタへ即時反映 */
static void sb_apply_param(UINT ch, UINT index)
{
	SOUNDBIOS_CHANNEL *c;
	UINT op;
	UINT reg;
	UINT8 v;

	if (ch >= 3 || index >= SOUNDBIOS_PARAMS) return;
	c = &soundbiosemu.ch[ch];
	if (index == SB_P_FB_ALG) {
		sb_regwrite(0xb0 + ch, (UINT8)c->params[index] & 0x3f);
		/* アルゴリズム変更に応じてSET VOLUMEでキャリアとみなすオペレータも変わる */
		sb_apply_volume(ch);
	}
	else if ((index >= SB_P_AR1) && (index < SB_P_AR1 + 4)) {
		op = index - SB_P_AR1;
		reg = 0x50 + sb_opoff[op] + ch;
		v = sb_regread(reg);
		v = (UINT8)((v & 0xc0) | (0x1f - (c->params[index] & 0x1f)));
		sb_regwrite(reg, v);
	}
	else if ((index >= SB_P_DR1) && (index < SB_P_DR1 + 4)) {
		op = index - SB_P_DR1;
		reg = 0x60 + sb_opoff[op] + ch;
		v = sb_regread(reg);
		v = (UINT8)((v & 0xe0) | (0x1f - (c->params[index] & 0x1f)));
		sb_regwrite(reg, v);
	}
	else if ((index >= SB_P_SR1) && (index < SB_P_SR1 + 4)) {
		op = index - SB_P_SR1;
		reg = 0x70 + sb_opoff[op] + ch;
		v = sb_regread(reg);
		v = (UINT8)((v & 0xe0) | (0x1f - (c->params[index] & 0x1f)));
		sb_regwrite(reg, v);
	}
	else if ((index >= SB_P_RR1) && (index < SB_P_RR1 + 4)) {
		op = index - SB_P_RR1;
		reg = 0x80 + sb_opoff[op] + ch;
		v = sb_regread(reg);
		v = (UINT8)((v & 0xf0) | (0x0f - (c->params[index] & 0x0f)));
		sb_regwrite(reg, v);
	}
	else if ((index >= SB_P_SL1) && (index < SB_P_SL1 + 4)) {
		op = index - SB_P_SL1;
		reg = 0x80 + sb_opoff[op] + ch;
		v = sb_regread(reg);
		v = (UINT8)((v & 0x0f) | ((0x0f - (c->params[index] & 0x0f)) << 4));
		sb_regwrite(reg, v);
	}
	else if ((index >= SB_P_OPLEVEL1) && (index < SB_P_OPLEVEL1 + 4)) {
		sb_apply_volume(ch);
	}
	else if ((index >= SB_P_KEYSCL1) && (index < SB_P_KEYSCL1 + 4)) {
		op = index - SB_P_KEYSCL1;
		reg = 0x50 + sb_opoff[op] + ch;
		v = sb_regread(reg);
		v = (UINT8)((v & 0x1f) | ((c->params[index] & 3) << 6));
		sb_regwrite(reg, v);
	}
	else if ((index >= SB_P_MULT1) && (index < SB_P_MULT1 + 4)) {
		op = index - SB_P_MULT1;
		reg = 0x30 + sb_opoff[op] + ch;
		v = sb_regread(reg);
		v = (UINT8)((v & 0x70) | (c->params[index] & 0x0f));
		sb_regwrite(reg, v);
	}
	else if ((index >= SB_P_DETUN1) && (index < SB_P_DETUN1 + 4)) {
		op = index - SB_P_DETUN1;
		reg = 0x30 + sb_opoff[op] + ch;
		v = sb_regread(reg);
		v = (UINT8)((v & 0x0f) | ((UINT8)c->params[index] << 4));
		sb_regwrite(reg, v);
	}
	/* OPR_MSKおよびLFO専用/予約フィールドは、NOTEまたはLFO制御側で使用するまで
	 * エミュレータ内部状態として保持する。 */
}

/* Sound BIOSパラメータ1個を論理状態へ保存し、必要なら音源へ反映 */
static void sb_setparam(UINT ch, UINT index, UINT16 value)
{
	SOUNDBIOS_CHANNEL *c;

	if ((ch >= SOUNDBIOS_CHANNELS) || (index >= SOUNDBIOS_PARAMS)) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	/* 公開パラメータのうちSPEED_LFOだけがWORDサイズ */
	if (index != SB_P_SPEED_LFO) {
		value &= 0x00ff;
	}
	c->params[index] = value;
	/* SSGで意味を持たないFM専用パラメータもREAD PARAで返せるよう値は保持する。
	 * 音源動作には、公開仕様でFM/SSG共通とされる項目だけを使用する。 */
	if (ch < 3) sb_apply_param(ch, index);
}

/* ゲストメモリ上のSET PARA BLOCKを読み込み、FMチャネルの音色を更新 */
static void sb_loadparams(UINT ch, UINT16 seg, UINT16 off, UINT8 type)
{
	SOUNDBIOS_CHANNEL *c;
	UINT i;
	UINT16 pos;

	/* SET PARA BLOCKはFM 3チャネルだけに定義されている */
	if (ch >= 3) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	/* 100バイトのWORD形式または51バイトのBYTE形式パラメータブロックを
	 * 置き換える前に、対象チャネルを停止する。 */
	sb_keyoff(ch);
	pos = off;
	for (i = 0; i < SOUNDBIOS_PARAMS; i++) {
		if (type == 1) {
			if (i == SB_P_SPEED_LFO) {
				c->params[i] = MEMR_READ16(seg, pos);
				pos = (UINT16)(pos + 2);
			}
			else {
				c->params[i] = MEMR_READ8(seg, pos);
				pos = (UINT16)(pos + 1);
			}
		}
		else {
			c->params[i] = MEMR_READ16(seg, (UINT16)(off + i * 2));
			if (i != SB_P_SPEED_LFO) c->params[i] &= 0x00ff;
		}
	}
	sb_apply_voice(ch);
	sb_apply_volume(ch);
	c->lfo = 1;
	c->lfo_phase = 0;
	c->lfo_sample_valid = 0;
	c->lfo_delay = 0;
}

/* 指定チャネルを論理的にKey-OFFし、音源と公開状態を停止状態へ更新 */
static void sb_keyoff(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;
	UINT8 mixer;

	if (ch >= SOUNDBIOS_CHANNELS) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	if (ch < 3) {
		sb_regwrite(0x28, (UINT8)ch);
		c->keymask = 0;
	}
	else {
		UINT psgch = ch - 3;
		/* SSGの発音制御には音量とミキサの両方を使用する */
		sb_regwrite(0x08 + psgch, 0x00);
		mixer = sb_regread(0x07);
		mixer |= (UINT8)(1 << psgch);
		sb_regwrite(0x07, mixer);
		/* 設定されたSSG音量を次のNOTE用に保持する */
	}
	c->keyon = 0;
	c->key = 0x80;
}

/* ALL STOP用 論理的なキー状態を保ったまま音源だけを消音 */
static void sb_hwkeyoff(UINT ch)
{
	UINT8 mixer;

	if (ch >= SOUNDBIOS_CHANNELS) {
		return;
	}
	/* ALL STOPは物理的な消音として扱う。CONT PLAYで後続の4msハードウェア制御時に
	 * 復帰できるよう、論理的なKey ON/OFF状態は保持する。 */
	if (ch < 3) {
		sb_hwregwrite(0x28, (UINT8)ch);
	}
	else {
		UINT psgch = ch - 3;
		sb_hwregwrite(0x08 + psgch, 0x00);
		mixer = sb_regread(0x07);
		mixer |= (UINT8)(1 << psgch);
		sb_hwregwrite(0x07, mixer);
	}
}

/* ALL STOP解除時に、保存してある論理キー状態を音源へ復元 */
static void sb_hwkeyrestore(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;
	UINT8 mixer;
	UINT8 mask;

	if (ch >= SOUNDBIOS_CHANNELS) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	if (!(soundbiosemu.saved_keys & (1 << ch))) {
		return;
	}
	if (ch < 3) {
		mask = c->saved_keymask;
		if (!mask) mask = (UINT8)((c->params[SB_P_OPR_MSK] & 0x0f) << 4);
		sb_hwregwrite(0x28, (UINT8)(mask | ch));
	}
	else {
		UINT psgch = ch - 3;
		mixer = sb_regread(0x07);
		mixer &= (UINT8)~(1 << psgch);
		sb_hwregwrite(0x07, mixer);
		sb_hwregwrite(0x08 + psgch, c->ssg_level & 0x1f);
		if (c->ssg_level & 0x10) {
			/* 現在のエンベロープ形状を書き直してエンベロープを再開始する */
			sb_hwregwrite(0x0d, sb_regread(0x0d));
		}
	}
}

/* Key No.から周波数を求め、指定チャネルの発音を開始 */
static void sb_noteon(UINT ch, UINT8 key)
{
	SOUNDBIOS_CHANNEL *c;
	UINT note;
	UINT octave;
	UINT16 fnum;
	UINT16 period;
	UINT8 mixer;
	UINT8 mask;
	UINT psgch;

	if (ch >= SOUNDBIOS_CHANNELS) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	/* 互換性のため61h〜7FhもSSGの拡張音程として受け付ける */
	if ((ch < 3) ? (key > 0x60) : (key >= 0x80)) {
		return;
	}
	note = key % 12;
	octave = key / 12;
	c->key = key;
	if (ch < 3) {
		fnum = sb_fnum[note];
		if (octave > 7) {
			/* Key No.96はblock 7より1オクターブ上の半音テーブル位置 */
			fnum = (UINT16)(fnum << (octave - 7));
			octave = 7;
			if (fnum > 0x07ff) fnum = 0x07ff;
		}
		c->base_fnum = fnum;
		c->block = (UINT8)octave;
		/* 過渡的な混在値を避けるためblock/F-numberは上位側を先に書き込む */
		sb_regwrite(0xa4 + ch,
			(UINT8)((octave << 3) | ((fnum >> 8) & 7)));
		sb_regwrite(0xa0 + ch, (UINT8)fnum);
		mask = (UINT8)((c->params[SB_P_OPR_MSK] & 0x0f) << 4);
		c->keymask = mask;
		sb_regwrite(0x28, (UINT8)(mask | ch));
		c->keyon = mask ? 1 : 0;
		if (!c->keyon) c->key = 0x80;
		/* LFO再開前のKey-ON時に音色TLの基準値を復元する */
		sb_apply_volume(ch);
	}
	else {
		psgch = ch - 3;
		period = sb_psgperiod[note];
		if (octave < 16) period = (UINT16)(period >> octave);
		if (!period) period = 1;
		if (period > 0x0fff) period = 0x0fff;
		c->base_fnum = period;
		sb_regwrite(psgch * 2, (UINT8)period);
		sb_regwrite(psgch * 2 + 1, (UINT8)((period >> 8) & 0x0f));
		mixer = sb_regread(0x07);
		mixer &= (UINT8)~(1 << psgch);
		sb_regwrite(0x07, mixer);
		/* AH=11h/PLAY 81hで08h〜0Ahが事前設定される場合がある。ハードウェア
		 * エンベロープ有効ビットを含め、その音量をNOTEのKey-OFF/ON間で保持する。 */
		sb_regwrite(0x08 + psgch, c->ssg_level & 0x1f);
		if (c->ssg_level & 0x10) {
			/* 呼び出し側が選択したエンベロープ形状を維持して再トリガする */
			sb_regwrite(0x0d, sb_regread(0x0d));
		}
		c->keyon = 1;
	}
}

/* NOTE/RESTを開始し、音長とTOUCHからwaitおよびgate時間を設定 */
static void sb_note(UINT ch, UINT8 key, UINT8 duration)
{
	SOUNDBIOS_CHANNEL *c;
	UINT16 gate;

	if (ch >= SOUNDBIOS_CHANNELS) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	if (duration == 0) duration = c->length;
	if (duration == 0) duration = 1;

	/* NOTE開始時は先に直前の発音を終了する */
	sb_keyoff(ch);
	/* SYNC_LFOの待ち時間はRESTを含む各NOTE境界で再開始する */
	if (c->lfo) {
		UINT16 syncdelay = c->params[SB_P_SYNC_LFO] & 0xff;
		if (syncdelay) {
			c->lfo_phase = 0;
			c->lfo_sample_valid = 0;
			c->lfo_delay = (UINT16)(syncdelay << 2);
		}
	}
	c->wait = duration;
	if (key == 0x80) {
		c->gate = 0;
	}
	else {
		gate = (UINT16)(((UINT32)duration * ((c->touch & 7) + 1) + 7) / 8);
		if (!gate) gate = 1;
		if (gate > duration) gate = duration;
		c->gate = gate;
		sb_noteon(ch, key);
	}
}

/* 現在のキー状態を維持したまま、指定時間だけ次の処理を待機させる */
static void sb_hold(UINT ch, UINT8 duration)
{
	SOUNDBIOS_CHANNEL *c;

	if (ch >= SOUNDBIOS_CHANNELS) return;
	c = &soundbiosemu.ch[ch];
	if (!duration) duration = c->length;
	if (!duration) duration = 1;
	/* HOLD中は現在のKey ON/OFF状態を明示的に保持するため、HOLD区間中は
	 * 保留中のNOTEゲートOFFを取り消す。 */
	c->gate = 0;
	c->wait = duration;
}

/* 現在のLFO位相と波形設定から正規化した変調波形値を生成 */
static SINT16 sb_lfowave(SOUNDBIOS_CHANNEL *c, UINT ch, BOOL wrapped)
{
	UINT16 phase;
	UINT8 wave;
	SINT32 v;
	UINT16 seed;

	phase = c->lfo_phase & 0x3fff;
	wave = (UINT8)c->params[SB_P_WAVE_LFO] & 3;
	switch (wave) {
	case 0: /* のこぎり波 */
		v = ((SINT32)phase >> 6) - 128;
		break;
	case 1: /* 矩形波、デューティ50% */
		v = (phase < 0x2000) ? 127 : -128;
		break;
	case 2: /* 三角波 */
		if (phase < 0x2000) {
			v = ((SINT32)phase >> 5) - 128;
		}
		else {
			v = 383 - ((SINT32)phase >> 5);
		}
		break;
	default: /* サンプルホールド */
		if (wrapped || !c->lfo_sample_valid) {
			seed = (UINT16)((UINT16)c->lfo_sample +
				(UINT16)(0x41u + ch * 37u));
			seed = (UINT16)(seed * 109u + 89u);
			c->lfo_sample = (SINT16)((seed & 0xff) - 128);
			c->lfo_sample_valid = 1;
		}
		v = c->lfo_sample;
		break;
	}
	return (SINT16)v;
}

/* LFO停止時に周波数と音量を変調前の基準値へ戻す */
static void sb_restore_lfo_base(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;
	UINT psgch;
	UINT16 period;

	if (ch >= SOUNDBIOS_CHANNELS) return;
	c = &soundbiosemu.ch[ch];
	if (!c->keyon) return;
	if (ch < 3) {
		sb_regwrite(0xa4 + ch,
			(UINT8)((c->block << 3) | ((c->base_fnum >> 8) & 7)));
		sb_regwrite(0xa0 + ch, (UINT8)c->base_fnum);
		sb_apply_volume(ch);
	}
	else {
		psgch = ch - 3;
		period = c->base_fnum;
		sb_regwrite(psgch * 2, (UINT8)period);
		sb_regwrite(psgch * 2 + 1, (UINT8)((period >> 8) & 0x0f));
		sb_regwrite(0x08 + psgch, c->ssg_level);
	}
}

/* 1回分のLFO位相を進め、ピッチ/振幅変調を音源へ反映 */
static void sb_lfotick(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;
	UINT16 oldphase;
	UINT16 speed;
	BOOL wrapped;
	SINT16 wave;
	SINT32 fine;
	SINT32 coarse;
	SINT32 depth;
	SINT32 fnum;
	SINT32 amp;
	SINT32 level;
	UINT op;
	UINT8 carriers;
	UINT tl;
	UINT psgch;

	if (ch >= SOUNDBIOS_CHANNELS) return;
	c = &soundbiosemu.ch[ch];
	if (!c->lfo) return;
	speed = c->params[SB_P_SPEED_LFO] & 0x3fff;
	if (!speed) return;

	/* SYNC_LFOは16ms単位の待ち時間。SYNC=0では非同期の
	 * フリーランニングLFOとする。 */
	if (c->lfo_delay) {
		c->lfo_delay--;
		return;
	}

	oldphase = c->lfo_phase;
	c->lfo_phase = (UINT16)((c->lfo_phase + speed) & 0x3fff);
	wrapped = (c->lfo_phase < oldphase);
	/* Key-OFFだけでは非同期LFOを停止しない */
	wave = sb_lfowave(c, ch, wrapped);

	fine = (SINT8)(c->params[SB_P_PMOD_LFO] & 0xff);
	coarse = c->params[SB_P_PMOS_LFO] & 0x0f;
	/* PMODは変調深度、PMOSはチャネル単位の感度 */
	depth = fine * coarse;
	if (ch < 3) {
		fnum = c->base_fnum;
		if (depth != 0) {
			if (c->base_fnum) {
				fnum = (SINT32)c->base_fnum +
					((SINT32)c->base_fnum * depth * wave) / (128 * 4096);
				if (fnum < 1) fnum = 1;
				if (fnum > 0x7ff) fnum = 0x7ff;
			}
			/* 未初期化のbase_fnumは0のままとする */
			sb_regwrite(0xa4 + ch,
				(UINT8)((c->block << 3) | ((fnum >> 8) & 7)));
			sb_regwrite(0xa0 + ch, (UINT8)fnum);
		}

		carriers = sb_carriermask((UINT8)c->params[SB_P_FB_ALG]);
		fine = (SINT8)(c->params[SB_P_AMOD_LFO] & 0xff);
		/* 深度0のオペレータも基準値へ戻るよう全オペレータのTLを更新する */
		for (op = 0; op < 4; op++) {
			coarse = c->params[SB_P_AMOS1 + op] & 0x0f;
			if ((carriers & (1 << op)) && c->volume_valid)
				level = c->volume & 0x7f;
			else
				level = c->params[SB_P_OPLEVEL1 + op] & 0x7f;

			/* AMODは細かい変調深度、AMOSはオペレータ単位の粗い感度 */
			amp = fine * coarse;
			level += (amp * wave) / (128 * 128);
			if (level < 0) level = 0;
			if (level > 0x7f) level = 0x7f;
			tl = (UINT)(0x7f - level);
			sb_regwrite(0x40 + sb_opoff[op] + ch, (UINT8)tl);
		}
	}
	else if (depth != 0) {
		psgch = ch - 3;
		fnum = (SINT32)c->base_fnum -
			((SINT32)c->base_fnum * depth * wave) / (128 * 4096);
		if (fnum < 1) fnum = 1;
		if (fnum > 0x0fff) fnum = 0x0fff;
		sb_regwrite(psgch * 2, (UINT8)fnum);
		sb_regwrite(psgch * 2 + 1, (UINT8)((fnum >> 8) & 0x0f));
		/* 公開パラメータ表では波形、位相同期、速度、ピッチ変調、ピッチ粗深度は
		 * FM/SSG共通だが、A_MOD_LFOとA_MOS_LFOはFM専用とされる。
		 * SSG用の独自振幅LFOは追加しない。 */
	}
}

/* 全Sound BIOSチャネルについて1回分のLFO処理を実行 */
static void sb_lfoall(void)
{
	UINT ch;
	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		sb_lfotick(ch);
	}
}

/* チャネルのリングPLAYバッファから1バイト取得し、読出位置と残量を進める */
static UINT8 sb_getbyte(SOUNDBIOS_CHANNEL *c)
{
	UINT8 v;

	if (!c->remain) {
		return 0;
	}
	v = MEMR_READ8(c->seg, (UINT16)(c->start + c->ptr));
	c->ptr++;
	if (c->capacity && (c->ptr >= c->capacity)) {
		c->ptr = 0;
	}
	c->remain--;
	return v;
}

/* PLAYバッファからリトルエンディアンの16ビット値を取得 */
static UINT16 sb_getword(SOUNDBIOS_CHANNEL *c)
{
	UINT16 lo;
	UINT16 hi;

	lo = sb_getbyte(c);
	hi = sb_getbyte(c);
	return (UINT16)(lo | (hi << 8));
}

/* PLAYバッファ残量をしきい値と比較し、必要ならコールバック要求を保留 */
static void sb_checkcallback(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;

	if (ch >= SOUNDBIOS_CHANNELS) {
		return;
	}
	c = &soundbiosemu.ch[ch];
	if (!SB_CALLBACK_ENABLED(c)) {
		return;
	}

	/* SET INT CONDはディレイドコマンドのバッファ消費を反映した後に判定する。
	 * 有効バイト数が設定しきい値以下で、未処理のコールバックがなければ
	 * コールバック要求を発生させる。 */
	if ((c->remain <= SB_CALLBACK_LENGTH(c)) && !c->callback_pending) {
		c->callback_pending = 1;
	}
}

/* PLAYバッファを使い切ったチャネルの演奏処理を終了状態にする */
static void sb_finish(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;

	if (ch >= SOUNDBIOS_CHANNELS) return;
	c = &soundbiosemu.ch[ch];
	c->active = 0;
	c->wait = 0;
	c->gate = 0;
	/* PLAYバッファを使い切ること自体はKey-OFF命令ではない。NOTEによる予定済みの
	 * ゲートOFFは別途処理され、HOLD/WRITE REGでは意図的にキー状態を残す場合がある。 */
}

/* PLAYバッファのディレイドコマンドを、待ち時間が発生するまで順に解釈 */
static void sb_parse(UINT ch)
{
	SOUNDBIOS_CHANNEL *c;
	UINT8 cmd;
	UINT8 a;
	UINT8 b;
	UINT16 off;
	UINT16 seg;
	UINT16 value;

	if (ch >= SOUNDBIOS_CHANNELS) return;
	c = &soundbiosemu.ch[ch];
	while (c->active && !c->wait) {
		if (!c->remain) {
			sb_finish(ch);
			break;
		}
		cmd = sb_getbyte(c);
		/* SSGではREST(80h)未満の拡張音程範囲も受け付ける */
		if ((cmd == 0x80) || (cmd <= 0x60) || ((ch >= 3) && (cmd < 0x80))) {
			a = c->remain ? sb_getbyte(c) : 0;
			sb_note(ch, cmd, a);
			sb_checkcallback(ch);
			break;
		}
		switch (cmd) {
		case 0x81:
			if (c->remain < 2) { sb_finish(ch); break; }
			a = sb_getbyte(c);
			b = sb_getbyte(c);
			sb_userregwrite(a, b);
			break;
		case 0x82:
			if (!c->remain) { sb_finish(ch); break; }
			c->touch = sb_getbyte(c) & 7;
			break;
		case 0x83:
			if (!c->remain) { sb_finish(ch); break; }
			c->length = sb_getbyte(c);
			break;
		case 0x84:
			if (!c->remain) { sb_finish(ch); break; }
			a = sb_getbyte(c);
			if (a) soundbiosemu.tempo = a;
			soundbiosemu.tempo_accum = 0;
			break;
		case 0x85:
			if (c->remain < 5) { sb_finish(ch); break; }
			a = sb_getbyte(c);
			off = sb_getword(c);
			seg = sb_getword(c);
			sb_loadparams(ch, seg, off, a);
			break;
		case 0x86:
			if (c->remain < 3) { sb_finish(ch); break; }
			a = sb_getbyte(c);
			value = sb_getword(c);
			sb_setparam(ch, a, value);
			break;
		case 0x87:
			c->lfo = 1;
			c->lfo_phase = 0;
			c->lfo_sample_valid = 0;
			c->lfo_delay = 0;
			break;
		case 0x88:
			c->lfo = 0;
			sb_restore_lfo_base(ch);
			break;
		case 0x89:
			if (!c->remain) { sb_finish(ch); break; }
			a = sb_getbyte(c);
			sb_hold(ch, a);
			break;
		case 0x8a:
			if (!c->remain) { sb_finish(ch); break; }
			a = sb_getbyte(c) & 0x7f;
			if (ch < 3) {
				c->volume = a;
				c->volume_valid = 1;
				sb_apply_volume(ch);
			}
			break;
		default:
			/* 未定義のディレイドコマンドは対象チャネルを安全に終了する */
			sb_finish(ch);
			break;
		}
		sb_checkcallback(ch);
	}
}

/* テンポに従って1Step分のgate/waitとPLAYコマンド処理を進める。 */
static void sb_metrotick(void)
{
	UINT ch;
	SOUNDBIOS_CHANNEL *c;

	if (soundbiosemu.paused) return;
	/* テンポ120ではTimer-A 1回の処理で音楽上のStep Timeを1つ進める */
	soundbiosemu.tempo_accum = (UINT16)(soundbiosemu.tempo_accum +
		soundbiosemu.tempo);
	if (soundbiosemu.tempo_accum < 120) return;
	soundbiosemu.tempo_accum %= 120;

	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		c = &soundbiosemu.ch[ch];
		if (c->gate) {
			c->gate--;
			if (!c->gate && c->keyon) sb_keyoff(ch);
		}
		if (c->wait) c->wait--;
		if (c->active && !c->wait) sb_parse(ch);
	}
	if (!sb_anytrack()) {
		soundbiosemu.playing = 0;
		sb_timerupdate(FALSE);
	}
}

/* 保留中のバッファ空きコールバックをゲストコードへディスパッチ */
static void sb_callback(BOOL save_context)
{
	UINT ch;
	SOUNDBIOS_CHANNEL *c;
	UINT16 nextip;
	UINT16 nextcs;

	if (soundbiosemu.paused) return;
	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		c = &soundbiosemu.ch[ch];
		if (!c->callback_pending) continue;
		c->callback_pending = 0;
		if (!SB_CALLBACK_ENABLED(c) || (!c->callback_seg && !c->callback_off))
			continue;

		/*
		 * バッファ空き処理はタイマIRQ中からAX=チャネル、BX=有効バイト数で呼び出す。
		 * コールバック側ではその他のレジスタを保存する必要がないため、割り込まれた
		 * プログラムのレジスタはSound BIOS側で保護する。
		 *
		 * 同じIRQ中の最初のコールバックだけで保存し、継続フックから呼ぶ2個目以降は
		 * 同じ保存フレームを共有する。
		 */
		if (save_context) {
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_AX);
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_BX);
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_CX);
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_DX);
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_SI);
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_DI);
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_BP);
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_DS);
			CPU_SP = (UINT16)(CPU_SP - 2);
			MEMR_WRITE16(CPU_SS, CPU_SP, CPU_ES);
		}

		/* コールバックのRETFは固定の継続フックへ戻す。CPUコアごとのフック命令
		 * 実行時IPの差に依存せず、最後に保存レジスタを復元してIRETできる。 */
		nextcs = sb_gethookseg();
		nextip = (UINT16)(SB_CB_OFF - SB_DESC_OFF);
		CPU_SP = (UINT16)(CPU_SP - 2);
		MEMR_WRITE16(CPU_SS, CPU_SP, nextcs);
		CPU_SP = (UINT16)(CPU_SP - 2);
		MEMR_WRITE16(CPU_SS, CPU_SP, nextip);

		/* 公開されているコールバック入口状態 */
		CPU_AX = (UINT16)ch;
		CPU_BX = SB_CALLBACK_LENGTH(c);
		CPU_CS = c->callback_seg;
		CPU_IP = c->callback_off;
		break;
	}
}

/* Sound BIOS以前に登録されていたIRQハンドラへ処理をチェーン */
static void sb_chainirq(void)
{
	CPU_CS = soundbiosemu.old_irq_seg;
	CPU_IP = soundbiosemu.old_irq_off;
}

/* 固定周期側でALL STOP/CONT PLAYとLFOのハードウェア制御を進める */
static void sb_hardtick(void)
{
	UINT ch;
	SOUNDBIOS_CHANNEL *c;

	if (soundbiosemu.stop_request && !soundbiosemu.paused) {
		for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
			c = &soundbiosemu.ch[ch];
			if (c->keyon) soundbiosemu.saved_keys |= (UINT8)(1 << ch);
			else soundbiosemu.saved_keys &= (UINT8)~(1 << ch);
			c->saved_keymask = c->keymask;
			if (c->keyon) sb_hwkeyoff(ch);
		}
		soundbiosemu.stop_request = 0;
		soundbiosemu.resume_request = 0;
		soundbiosemu.paused = 1;
		sb_timerupdate(FALSE);
		return;
	}
	if (soundbiosemu.resume_request) {
		if (soundbiosemu.paused) {
			for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++)
				sb_hwkeyrestore(ch);
			soundbiosemu.paused = 0;
		}
		soundbiosemu.resume_request = 0;
		soundbiosemu.stop_request = 0;
		sb_timerupdate(FALSE);
	}
}

/* OPNタイマIRQを判定し、Step処理・LFO・コールバック・EOIを実行 */
static void sb_timerirq(void)
{
	UINT8 status;
	UINT8 mode;
	UINT8 loads;
	UINT8 enables;
	UINT8 reset;
	BOOL doa;
	BOOL dob;
	BOOL handled;
	BOOL sharedpcm;

	status = g_opna[0].s.status;
	doa = soundbiosemu.timer_a_running && (status & 0x01);
	dob = soundbiosemu.timer_b_running && (status & 0x02);
	handled = doa || dob;
	sharedpcm = ((g_pcm86.irq == soundbiosemu.irq) && g_pcm86.irqflag);

	if (handled) {
		/* 発生したタイマフラグを確認応答し、タイマ許可状態を復元する */
		mode = g_opna[0].s.reg[0x27] & 0xc0;
		loads = 0;
		enables = 0;
		if (soundbiosemu.timer_a_running) { loads |= 0x01; enables |= 0x04; }
		if (soundbiosemu.timer_b_running) { loads |= 0x02; enables |= 0x08; }
		reset = 0;
		if (doa) reset |= 0x10;
		if (dob) reset |= 0x20;
		sb_hwregwrite(0x27, (UINT8)(mode | loads));
		sb_hwregwrite(0x27, (UINT8)(mode | loads | reset));
		sb_hwregwrite(0x27, (UINT8)(mode | loads | enables));

		if (doa) {
			sb_metrotick();
		}
		if (dob) {
			/* 4ms LFO更新より先にSTOP/CONTを処理する */
			sb_hardtick();
			if (!soundbiosemu.paused) sb_lfoall();
		}
	}

	if (!handled || sharedpcm) {
		sb_chainirq();
		return;
	}
	sb_eoi();
	/* PLAYバッファ空きコールバックはテンポクロック側で処理する。Timer-Bは独立した
	 * 4msハードウェア/LFOクロックなので、このコールバックは発生させない。 */
	if (doa) sb_callback(TRUE);
}

/* 呼び出し側が用意した256バイトの公開共通制御情報領域を初期化 */
static void sb_clear_common_area(UINT16 workseg)
{
	UINT off;
	for (off = 0; off < 0x100; off++) {
		MEMR_WRITE8(workseg, (UINT16)off, 0);
	}
}

/* Sound BIOSの作業領域・音源・既定値・IRQ環境を初期化 */
static void sb_init(void)
{
	UINT ch;
	UINT16 workseg;
	SBEMU_BUFFER_INFO buffer[SOUNDBIOS_CHANNELS];
	SOUNDBIOS_CHANNEL *c;

	workseg = CPU_ES;
	/* INITIALIZEでは呼び出し側が用意したPLAYバッファ位置/長さだけを退避し、
	 * その後256バイトの公開共通領域を初期化する。 */
	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		MEMR_READS(workseg, (UINT16)(ch * SB_EMU_COMMON_CH_SIZE),
			&buffer[ch], sizeof(buffer[ch]));
	}
	if (soundbiosemu.irq_hooked) sb_unhookirq();
	ZeroMemory(&soundbiosemu, sizeof(soundbiosemu));
	soundbiosemu.workseg = workseg;
	soundbiosemu.tempo = 120;
	soundbiosemu.initialized = 1;
	sb_clear_common_area(workseg);
	sb_clear_local_area(workseg);

	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		c = &soundbiosemu.ch[ch];
		c->seg = buffer[ch].seg;
		c->start = buffer[ch].start;
		c->capacity = buffer[ch].capacity;
		c->ptr = 0;
		c->remain = 0;
		c->length = 48;
		c->touch = 7;
		c->volume = 0x7f;
		c->key = 0x80;
		/* INITIALIZE後の音色パラメータは未定義なので、エミュレータ私有領域は
		 * 決定的な値に初期化するがOPNへは適用しない。 */
		ZeroMemory(c->params, sizeof(c->params));
	}

	/* 残存しているSound BIOSタイマを停止し、BIOS管理下の発音を消音する */
	/* INITIALIZE後はOPNチャネル3の特殊/CSMモードも無効にする */
	sb_hwregwrite(0x27, 0x30);
	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) sb_keyoff(ch);
	sb_soundinitregs();
	sb_hookirq();
	sb_timerupdate(TRUE);
}

/* チャネルのPLAYリングバッファ内の指定位置へ1バイト書き込む */
static void sb_putplaybyte(SOUNDBIOS_CHANNEL *c, UINT16 pos, UINT8 value)
{
	MEMR_WRITE8(c->seg, (UINT16)(c->start + pos), value);
}

/* PLAYパラメータリストから各チャネルのデータをPLAYバッファへ追加 */
static void sb_play(void)
{
	UINT ch;
	UINT i;
	UINT16 songseg;
	UINT16 srcoff;
	UINT16 length;
	UINT16 freebytes;
	UINT16 writepos;
	SOUNDBIOS_CHANNEL *c;

	if (!soundbiosemu.initialized) return;
	songseg = MEMR_READ16(CPU_ES, CPU_BX);

	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		c = &soundbiosemu.ch[ch];
		srcoff = MEMR_READ16(CPU_ES, (UINT16)(CPU_BX + 4 + ch * 4));
		length = MEMR_READ16(CPU_ES, (UINT16)(CPU_BX + 6 + ch * 4));
		if (!length || !c->capacity) {
			continue;
		}
		if (c->remain > c->capacity) c->remain = c->capacity;
		freebytes = (UINT16)(c->capacity - c->remain);
		/* 規定を超えるブロックは仕様外なので、壊れたゲストデータでメモリ範囲外へ
		 * 進まないよう利用可能サイズへ制限する。 */
		if (length > freebytes) length = freebytes;
		writepos = (UINT16)(c->ptr + c->remain);
		if (c->capacity) writepos %= c->capacity;
		for (i = 0; i < length; i++) {
			sb_putplaybyte(c, writepos,
				MEMR_READ8(songseg, (UINT16)(srcoff + i)));
			writepos++;
			if (writepos >= c->capacity) writepos = 0;
		}
		c->remain = (UINT16)(c->remain + length);
		if (c->remain) c->active = 0xff;
		c->callback_pending = 0;
	}

	/* PLAYはALL STOP後のインターバル割り込みも再開する */
	if (soundbiosemu.paused) soundbiosemu.resume_request = 1;
	else soundbiosemu.stop_request = 0;
	if (sb_anytrack()) soundbiosemu.playing = 1;
	sb_hookirq();
	sb_timerupdate(TRUE);
}

/* 演奏を停止し、指定モードに応じてバッファ状態や既定値をクリア */
static void sb_clear(UINT8 mode)
{
	UINT ch;
	SOUNDBIOS_CHANNEL *c;

	if (!soundbiosemu.initialized) return;
	if (mode == 1) sb_clear_common_area(soundbiosemu.workseg);
	for (ch = 0; ch < SOUNDBIOS_CHANNELS; ch++) {
		sb_keyoff(ch);
		c = &soundbiosemu.ch[ch];
		c->ptr = 0;
		c->remain = 0;
		c->wait = 0;
		c->gate = 0;
		c->active = 0;
		c->callback_pending = 0;
		c->saved_keymask = 0;
		if (mode == 1) {
			ZeroMemory(c->public_reserved, sizeof(c->public_reserved));
			c->callback_cond = 0;
			c->callback_off = 0;
			c->callback_seg = 0;
			c->length = 48;
			c->touch = 7;
		}
	}
	if (mode == 1) {
		soundbiosemu.tempo = 120;
	}
	soundbiosemu.playing = 0;
	soundbiosemu.paused = 0;
	soundbiosemu.stop_request = 0;
	soundbiosemu.resume_request = 0;
	soundbiosemu.tempo_accum = 0;
	soundbiosemu.saved_keys = 0;
	sb_timerupdate(TRUE);
	sb_unhookirq();
}


/* CPUレジスタで渡されたSound BIOS機能番号を対応するHLE処理へ転送 */
static void sb_command(void)
{
	UINT ch;
	UINT index;
	SOUNDBIOS_CHANNEL *c;

	switch (CPU_AH) {
	case 0x00:
		sb_init();
		break;
	case 0x01:
		sb_play();
		break;
	case 0x02:
		if (soundbiosemu.initialized) sb_clear(CPU_AL);
		break;
	case 0x10:
		CPU_BL = sb_regread(CPU_AL);
		CPU_BH = 0;
		break;
	case 0x11:
		sb_userregwrite(CPU_AL, CPU_BL);
		break;
	case 0x12:
		ch = CPU_AL;
		if (ch < SOUNDBIOS_CHANNELS) {
			soundbiosemu.ch[ch].touch = CPU_BL & 7;
		}
		break;
	case 0x13:
		ch = CPU_AL;
		if (ch < SOUNDBIOS_CHANNELS) {
			sb_note(ch, CPU_BH, CPU_BL);
			soundbiosemu.playing = 1;
			sb_hookirq();
			sb_timerupdate(TRUE);
		}
		break;
	case 0x14:
		ch = CPU_AL;
		if (ch < SOUNDBIOS_CHANNELS && CPU_BL) {
			soundbiosemu.ch[ch].length = CPU_BL;
		}
		break;
	case 0x15:
		if (CPU_BL) soundbiosemu.tempo = CPU_BL;
		soundbiosemu.tempo_accum = 0;
		break;
	case 0x16:
		sb_loadparams(CPU_AL, CPU_ES, CPU_BX, CPU_DL);
		sb_timerupdate(TRUE);
		break;
	case 0x17:
		ch = CPU_AL;
		index = CPU_BL;
		CPU_BX = ((ch < SOUNDBIOS_CHANNELS) && (index < SOUNDBIOS_PARAMS)) ?
			soundbiosemu.ch[ch].params[index] : 0;
		if (index != SB_P_SPEED_LFO) CPU_BH = 0;
		break;
	case 0x18:
		sb_setparam(CPU_AL, CPU_BL, CPU_DX);
		break;
	case 0x19:
		if (!soundbiosemu.paused && !soundbiosemu.stop_request) {
			soundbiosemu.stop_request = 1;
			soundbiosemu.resume_request = 0;
			sb_hookirq();
			sb_timerupdate(TRUE);
		}
		break;
	case 0x1a:
		if (soundbiosemu.paused) {
			soundbiosemu.resume_request = 1;
			soundbiosemu.stop_request = 0;
			sb_hookirq();
			sb_timerupdate(TRUE);
		}
		else if (soundbiosemu.stop_request) {
			/* STOP要求がまだタイマ処理境界へ到達していない */
			soundbiosemu.stop_request = 0;
			sb_timerupdate(TRUE);
		}
		break;
	case 0x1b:
		ch = CPU_AL;
		if (ch < SOUNDBIOS_CHANNELS) {
			c = &soundbiosemu.ch[ch];
			c->lfo = 1;
			c->lfo_phase = 0;
			c->lfo_sample_valid = 0;
			c->lfo_delay = 0;
			sb_hookirq();
			sb_timerupdate(TRUE);
		}
		break;
	case 0x1c:
		ch = CPU_AL;
		if (ch < SOUNDBIOS_CHANNELS) {
			c = &soundbiosemu.ch[ch];
			c->lfo = 0;
			sb_restore_lfo_base(ch);
			sb_timerupdate(TRUE);
		}
		break;
	case 0x1d:
		ch = CPU_AL;
		if (ch < SOUNDBIOS_CHANNELS) {
			c = &soundbiosemu.ch[ch];
			/* 文献ではCXとなっているが、テストソフトによる実機確認の結果としてDXを採用 */
			c->callback_cond = CPU_DX;
			c->callback_off = CPU_BX;
			c->callback_seg = CPU_ES;
			c->callback_pending = 0;
		}
		break;
	case 0x1e:
		ch = CPU_AL;
		if (ch < SOUNDBIOS_CHANNELS) {
			sb_hold(ch, CPU_BL);
			soundbiosemu.playing = 1;
			sb_hookirq();
			sb_timerupdate(TRUE);
		}
		break;
	case 0x1f:
		ch = CPU_AL;
		if (ch < 3) {
			soundbiosemu.ch[ch].volume = CPU_BL & 0x7f;
			soundbiosemu.ch[ch].volume_valid = 1;
			sb_apply_volume(ch);
		}
		break;
	default:
		break;
	}
}

#endif /* !defined(DISABLE_SOUND) */

#endif	/* SUPPORT_EMU_SOUNDBIOS */

/* Sound ROM/HLEのIRQ占有を解除し、Sound ROM管理状態をリセット */
void soundrom_reset(void)
{
#if defined(SUPPORT_EMU_SOUNDBIOS)
#if !defined(DISABLE_SOUND)
	/* HLEが取得したIRQベクタとPICマスクを、ROMを無効化する前に元へ戻す。 */
	if (soundrom.address && !soundrom.name[0] && sb_state_load()) {
		sb_clear(0);
		sb_state_invalidate();
	}
#endif
	ZeroMemory(&soundbiosemu, sizeof(soundbiosemu));
#endif
	ZeroMemory(&soundrom, sizeof(soundrom));
}

#if defined(SUPPORT_EMU_SOUNDBIOS)
/* エミュレーションSound BIOS ROMの各HLE入口を現在のCPU用フック命令へ置き換え */
void soundrom_patchhookinst(void)
{
	UINT8 inst;

	if (!soundrom.address || soundrom.name[0]) {
		return;
	}
	inst = sb_hookinst();
	mem[soundrom.address + SB_INIT_OFF] = inst;
	mem[soundrom.address + SB_API_OFF] = inst;
	mem[soundrom.address + SB_IRQ_OFF] = inst;
	mem[soundrom.address + SB_CB_OFF] = inst;
}
#endif

/* 外部SOUND.ROMを読み込み、存在しなければエミュレーションSound BIOS用を配置 */
void soundrom_load(UINT32 address, const OEMCHAR *primary)
{
#if defined(SUPPORT_EMU_SOUNDBIOS)
	/* SOUND.ROM未配置時にCPUフックへ制御を渡す最小ROMイメージ。 */
	UINT8 defsoundbios[23] = {
		0x01, 0x00, 0x00, 0x00,
		0xd2, 0x00, 0x08, 0x00,
		0x90, 0xcf,
		0x90, 0xcf,
		/* CB継続: hook後に割り込み元レジスタを復元してIRETする。 */
		0x90, 0x07, 0x1f, 0x5d, 0x5f, 0x5e, 0x5a, 0x59, 0x5b, 0x58, 0xcf
	};

	ZeroMemory(&soundbiosemu, sizeof(soundbiosemu));
#else
	/* HLE無効時にSound BIOSエントリ情報だけを提供する最小ROMイメージ。 */
	static const UINT8 defsoundbios[9] = {
		0x01, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x08, 0x00, 0xcb
	};
#endif
	if (primary != NULL) {
		if (loadsoundrom(address, primary) == SUCCESS) {
			return;
		}
	}
	if (loadsoundrom(address, NULL) == SUCCESS) {
		return;
	}

#if defined(SUPPORT_EMU_SOUNDBIOS)
	/*
	 * メモリ上には公開エントリ情報と、CPUコアがHLE入口として識別するスタブを置く。
	 * API/IRQはフック処理後にIRETし、CB継続はゲストコールバックがRETFした後に
	 * 割り込み元レジスタを復元してIRETする。
	 */
	mem[address + SB_INIT_OFF] = 0x90;
	mem[address + SB_INIT_OFF + 1] = 0xcb;
	CopyMemory(mem + address + SB_DESC_OFF, defsoundbios, sizeof(defsoundbios));
	soundrom.name[0] = '\0';
	soundrom.address = address;
	soundrom_patchhookinst();
#else
	CopyMemory(mem + address + 0x2e00, defsoundbios, sizeof(defsoundbios));
	soundrom.name[0] = '\0';
	soundrom.address = address;
#endif
}

/* 設定されたスイッチ位置からSound ROM配置先を選択しロードまたは無効化 */
void soundrom_loadex(UINT sw, const OEMCHAR *primary)
{
	if (sw < 4) {
		soundrom_load((0xc8000 + ((UINT32)sw << 14)), primary);
	}
	else {
		soundrom_reset();
	}
}

#if defined(SUPPORT_EMU_SOUNDBIOS)
/* 指定物理アドレスがエミュレーションSound BIOSのHLE入口かどうかを判定 */
UINT soundrom_isbiosaddr(UINT32 adrs)
{
	if (!soundrom.address || soundrom.name[0]) {
		return 0;
	}
	return ((adrs == soundrom.address + SB_INIT_OFF) ||
		(adrs == soundrom.address + SB_API_OFF) ||
		(adrs == soundrom.address + SB_IRQ_OFF) ||
		(adrs == soundrom.address + SB_CB_OFF));
}

/* CPUがSound BIOSフックへ到達した際のAPI/IRQ/コールバック処理を実行 */
UINT soundrom_biosfunc(UINT32 adrs)
{
#if !defined(DISABLE_SOUND)
	BOOL statevalid;

	if (!soundrom_isbiosaddr(adrs)) {
		return 0;
	}
	CPU_REMCLOCK -= 200;

	/* オプションROM初期化エントリでは、公開Sound BIOS入口をINT D2hへ登録する。 */
	if (adrs == soundrom.address + SB_INIT_OFF) {
		sb_int_init();
		return 1;
	}

	/* ゲストRAMを状態の正本とし、各HLE入口では処理前に復元、処理後に確定する。 */
	statevalid = sb_state_load();
	if (adrs == soundrom.address + SB_API_OFF) {
		sb_command();
	}
	else if (adrs == soundrom.address + SB_CB_OFF) {
		/* ゲストコールバックから戻った後、同じIRQで保留中のコールバックを続けて処理する。
		 * 全処理完了後はROM上の継続スタブが割り込み元レジスタを復元する。 */
		if (statevalid) sb_callback(FALSE);
	}
	else {
		if (!statevalid) {
			/* 状態を復元できないIRQでもEOIを返し、PICを割り込み待ち状態に残さない。 */
			if (g_opna[0].s.irq >= 8 && g_opna[0].s.irq <= 15) {
				iocore_out8(0x08, 0x20);
			}
			iocore_out8(0x00, 0x20);
			return 1;
		}
		sb_timerirq();
	}
	if (soundbiosemu.initialized) {
		sb_state_store();
	}
	return 1;
#else
	(void)adrs;
	return 0;
#endif
}
#endif	/* SUPPORT_EMU_SOUNDBIOS */
