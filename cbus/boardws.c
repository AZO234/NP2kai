/**
 * @file boardws.c
 * @brief Q-Vision WaveStar sound board
 */

#include "compiler.h"
#include "cpucore.h"
#include "pccore.h"
#include <io/iocore.h>

#ifndef _countof
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#endif

#include "cbuscore.h"
#include "cbuspnp.h"
#include "boardws.h"
#include "pcm86io.h"
#include "cs4231io.h"
#include "mpu98ii.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/pcm86.h"
#include "sound/cs4231.h"

#define WAVESTAR_MPU_PORT      0xe0d0
#define WAVESTAR_MPU_IRQ       10

WAVESTAR wavestar;

static int s_wavestar_bound = 0;

static const UINT8 s_unlock_sequence[] = {0xa6, 0xd3, 0x69, 0xb4, 0x5a};

static void boardws_detach_runtime_io(void);
static void boardws_attach_runtime_io(void);

/* ISA/C-Bus PnP serial identifier: WBT0211, serial 00000001h, checksum EEh. */
static const UINT8 s_pnp_serial[9] = {
	0x5c, 0x54, 0x02, 0x11, 0x01, 0x00, 0x00, 0x00, 0xee
};

/* Resource stream.  Configurable fields are rebuilt from the WaveStar options. */
static UINT8 s_pnp_resources[] = {
	0x0a, 0x10, 0x00,
	0x15, 0x5c, 0x54, 0x02, 0x11, 0x00,
	0x4b, 0x60, 0xa4, 0x0e,
	0x4b, 0xd0, 0xe0, 0x04,
	0x4b, 0x88, 0x01, 0x08,
	0x4b, 0xd2, 0x04, 0x01,
	0x22, 0x00, 0x10,
	0x22, 0x00, 0x04,
	0x2a, 0x08, 0x00,
	0x79, 0x00
};

static void boardws_build_pnp_resources(void)
{
	UINT16 irqmask;
	UINT i;
	UINT8 sum;

	/* FM/86-compatible I/O range. */
	s_pnp_resources[18] = (UINT8)(wavestar.fm_port & 0xff);
	s_pnp_resources[19] = (UINT8)(wavestar.fm_port >> 8);

	/* Sound IRQ. */
	irqmask = (UINT16)(1U << wavestar.irq);
	s_pnp_resources[26] = (UINT8)(irqmask & 0xff);
	s_pnp_resources[27] = (UINT8)(irqmask >> 8);

	/* MPU IRQ remains the board default; its MIDI engine/mapping is MPU-PC98II. */
	irqmask = (UINT16)(1U << wavestar.mpu_irq);
	s_pnp_resources[29] = (UINT8)(irqmask & 0xff);
	s_pnp_resources[30] = (UINT8)(irqmask >> 8);

	/* WSS DMA channel. */
	s_pnp_resources[32] = (UINT8)(1U << wavestar.dma);

	/* End-tag checksum makes the complete resource stream sum to zero. */
	s_pnp_resources[35] = 0;
	sum = 0;
	for (i = 0; i < sizeof(s_pnp_resources); i++) {
		sum = (UINT8)(sum + s_pnp_resources[i]);
	}
	s_pnp_resources[35] = (UINT8)(0 - sum);
}

static void boardws_setfmvolume(UINT8 volume)
{
	UINT i;

	if (volume > 15) {
		volume = 15;
	}
	wavestar.fm_volume = volume;

	opngen_setvol(np2cfg.vol_fm * volume / 15 * np2cfg.vol_master / 100);
	psggen_setvol(np2cfg.vol_ssg * volume / 15 * np2cfg.vol_master / 100);
	rhythm_setvol(np2cfg.vol_rhythm * volume / 15 * np2cfg.vol_master / 100);
#if defined(SUPPORT_FMGEN)
	if (np2cfg.usefmgen) {
		opna_fmgen_setallvolumeFM_linear(np2cfg.vol_fm * volume / 15 * np2cfg.vol_master / 100);
		opna_fmgen_setallvolumePSG_linear(np2cfg.vol_ssg * volume / 15 * np2cfg.vol_master / 100);
		opna_fmgen_setallvolumeRhythmTotal_linear(np2cfg.vol_rhythm * volume / 15 * np2cfg.vol_master / 100);
	}
#endif
	for (i = 0; i < _countof(g_opna); i++) {
		rhythm_update(&g_opna[i].rhythm);
	}
}

UINT8 boardws_getfmvolume(void)
{
	return wavestar.fm_volume;
}

UINT8 boardws_getirq(void)
{
	return wavestar.irq;
}

BOOL boardws_setirq(UINT8 irq)
{
	if ((irq != 12) && (irq != 13)) {
		return FALSE;
	}
	if (wavestar.irq == irq) {
		return TRUE;
	}

	if (s_wavestar_bound) {
		boardws_detach_runtime_io();
	}
	wavestar.irq = irq;
	if (wavestar.pnp_enabled) {
		wavestar.pnp_config[0x70] = irq;
		boardws_build_pnp_resources();
	}
	opna_timer(&g_opna[0], (irq == 13) ? 0x50 : 0xd0,
		NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	wavestar.saved_opna_irq = irq;
	wavestar.saved_pcm_irq = wavestar.pcm86_enabled ? irq : 0xff;
	cs4231.dmairq = irq;
	if (s_wavestar_bound) {
		boardws_attach_runtime_io();
	}
	return TRUE;
}

UINT8 boardws_getmpuirq(void)
{
	return wavestar.mpu_irq;
}

BOOL boardws_setmpuirq(UINT8 irq)
{
	if ((irq != 3) && (irq != 5) && (irq != 6) &&
		(irq != 10) && (irq != 12)) {
		return FALSE;
	}
	if (wavestar.mpu_irq == irq) {
		return TRUE;
	}

	if (s_wavestar_bound) {
		boardws_detach_runtime_io();
	}
	wavestar.mpu_irq = irq;
	if (wavestar.pnp_enabled) {
		wavestar.pnp_config[0x72] = irq;
		boardws_build_pnp_resources();
	}
	if (s_wavestar_bound) {
		boardws_attach_runtime_io();
	}
	return TRUE;
}

/* ---- YMF288/OPNA-compatible FM block ---- */

static void IOOUTCALL boardws_opna_o188(UINT port, REG8 dat)
{
	g_opna[0].s.addrl = dat;
	g_opna[0].s.data = dat;
	(void)port;
}

static void IOOUTCALL boardws_opna_o18a(UINT port, REG8 dat)
{
	g_opna[0].s.data = dat;
	opna_writeRegister(&g_opna[0], g_opna[0].s.addrl, dat);
	(void)port;
}

static void IOOUTCALL boardws_opna_o18c(UINT port, REG8 dat)
{
	if (g_opna[0].s.extend) {
		g_opna[0].s.addrh = dat;
		g_opna[0].s.data = dat;
	}
	(void)port;
}

static void IOOUTCALL boardws_opna_o18e(UINT port, REG8 dat)
{
	if (g_opna[0].s.extend) {
		g_opna[0].s.data = dat;
		opna_writeExtendedRegister(&g_opna[0], g_opna[0].s.addrh, dat);
	}
	(void)port;
}

static REG8 IOINPCALL boardws_opna_i188(UINT port)
{
	(void)port;
	CPU_REMCLOCK -= (SINT32)(pccore.realclock / 800000); // WORKAROUND: WaveStar�h���C�o��I/O�A�N�Z�X����������ƃG���[�ɂȂ�
	return g_opna[0].s.status;
}

static REG8 IOINPCALL boardws_opna_i18a(UINT port)
{
	UINT addr = g_opna[0].s.addrl;
	(void)port;

	if (addr == 0x0e) {
		return fmboard_getjoy(&g_opna[0]);
	}
	if (addr < 0x10) {
		return opna_readRegister(&g_opna[0], addr);
	}
	if (addr == 0xff) {
		return 1;
	}
	return g_opna[0].s.data;
}

static REG8 IOINPCALL boardws_opna_i18c(UINT port)
{
	(void)port;
	return g_opna[0].s.extend ? opna_readExtendedStatus(&g_opna[0]) : 0xff;
}

static REG8 IOINPCALL boardws_opna_i18e(UINT port)
{
	UINT addr;
	(void)port;

	if (!g_opna[0].s.extend) {
		return 0xff;
	}
	addr = g_opna[0].s.addrh;
	if ((addr == 0x08) || (addr == 0x0f)) {
		return opna_readExtendedRegister(&g_opna[0], addr);
	}
	return g_opna[0].s.data;
}

static void boardws_extendchannel(REG8 enable)
{
	g_opna[0].s.extend = enable;
	if (enable) {
		opngen_setcfg(&g_opna[0].opngen, 6, OPN_STEREO | 0x07);
	}
	else {
		opngen_setcfg(&g_opna[0].opngen, 3, OPN_MONORAL | 0x07);
		rhythm_setreg(&g_opna[0].rhythm, 0x10, 0xff);
	}
}

static const IOOUT s_opna_out[4] = {
	boardws_opna_o188, boardws_opna_o18a, boardws_opna_o18c, boardws_opna_o18e
};
static const IOINP s_opna_in[4] = {
	boardws_opna_i188, boardws_opna_i18a, boardws_opna_i18c, boardws_opna_i18e
};

/* ---- A460h compatibility / codec switching ---- */

static void IOOUTCALL boardws_o464(UINT port, REG8 val);
static void IOOUTCALL boardws_o466(UINT port, REG8 val);
static REG8 IOINPCALL boardws_i464(UINT port);

static void IOOUTCALL boardws_dummy_out(UINT port, REG8 value)
{
	(void)port;
	(void)value;
}

static REG8 IOINPCALL boardws_dummy_in(UINT port)
{
	(void)port;
	return 0;
}

static void IOOUTCALL boardws_o462(UINT port, REG8 val)
{
	if ((wavestar.unlock_index < sizeof(s_unlock_sequence)) &&
		(val == s_unlock_sequence[wavestar.unlock_index])) {
		wavestar.unlock_index++;
		if (wavestar.unlock_index == sizeof(s_unlock_sequence)) {
			wavestar.compat_status = 0x0b;
		}
	}
	else if (val == s_unlock_sequence[0]) {
		wavestar.unlock_index = 1;
	}
	else {
		wavestar.unlock_index = 0;
	}
	(void)port;
}

static REG8 IOINPCALL boardws_i462(UINT port)
{
	(void)port;
	return 0xff;
}

static BOOL boardws_device_active(void)
{
	if (!wavestar.pnp_enabled) {
		return TRUE;
	}
	return (wavestar.pnp_config[0x30] & 1) ? TRUE : FALSE;
}

static UINT boardws_codec_port(UINT port)
{
	return ((port - wavestar.compat_port) >> 1) + cs4231.port[0] + 1;
}

static void IOOUTCALL boardws_codec_out(UINT port, REG8 value)
{
	UINT codec_port = boardws_codec_port(port);

	/* WaveStar routes CS4231 AUX1-L to its FM/SSG/rhythm level control. */
	if (((codec_port - cs4231.port[0]) == 0x05) &&
		((cs4231.index & 0x1f) == 0x02)) {
		REG8 codec_value = value;
		if (codec_value >= 0x10) {
			codec_value = 15;
		}
		boardws_setfmvolume((UINT8)((~codec_value) & 15));
		oplgen_setvol(np2cfg.vol_fm * np2cfg.vol_master / 100);
		value = codec_value;
	}
	cs4231io0_w8(codec_port, value);
}

static REG8 IOINPCALL boardws_codec_in(UINT port)
{
	return cs4231io0_r8(boardws_codec_port(port));
}

static void boardws_detach_compat_ports(UINT16 base)
{
	UINT off;

	for (off = 0; off <= 0x0e; off += 2) {
		iocore_detachout(base + off);
		iocore_detachinp(base + off);
	}
}

static void boardws_update_ports(void)
{
	UINT16 base = wavestar.compat_port;

	boardws_detach_compat_ports(base);
	if (!boardws_device_active()) {
		g_pcm86.irq = 0xff;
		g_opna[0].s.irq = 0xff;
		return;
	}

	iocore_attachout(base + 2, boardws_o462);
	iocore_attachout(base + 4, boardws_o464);
	iocore_attachinp(base + 2, boardws_i462);

	if (wavestar.wss_mode) {
		iocore_attachout(base + 6, boardws_codec_out);
		iocore_attachout(base + 8, boardws_codec_out);
		iocore_attachout(base + 0x0a, boardws_codec_out);
		iocore_attachout(base + 0x0c, boardws_codec_out);
		iocore_attachinp(base + 4, boardws_codec_in);
		iocore_attachinp(base + 6, boardws_codec_in);
		iocore_attachinp(base + 8, boardws_codec_in);
		iocore_attachinp(base + 0x0a, boardws_codec_in);
		iocore_attachinp(base + 0x0c, boardws_codec_in);

		g_pcm86.irq = 0xff;
		g_opna[0].s.irq = 0xff;
	}
	else {
		iocore_attachout(base, pcm86_oa460);
		iocore_attachout(base + 6, boardws_o466);
		if (wavestar.pcm86_enabled) {
			iocore_attachout(base + 8, pcm86_oa468);
			iocore_attachout(base + 0x0a, pcm86_oa46a);
			iocore_attachout(base + 0x0c, pcm86_oa46c);
		}
		else {
			iocore_attachout(base + 8, boardws_dummy_out);
			iocore_attachout(base + 0x0a, boardws_dummy_out);
			iocore_attachout(base + 0x0c, boardws_dummy_out);
		}
		iocore_attachinp(base, pcm86_ia460);
		iocore_attachinp(base + 4, boardws_i464);
		iocore_attachinp(base + 6, pcm86_ia466);
		if (wavestar.pcm86_enabled) {
			iocore_attachinp(base + 8, pcm86_ia468);
			iocore_attachinp(base + 0x0a, pcm86_ia46a);
		}
		else {
			iocore_attachinp(base + 8, boardws_dummy_in);
			iocore_attachinp(base + 0x0a, boardws_dummy_in);
		}
		iocore_attachinp(base + 0x0c, boardws_dummy_in);
		iocore_attachinp(base + 0x0e, boardws_dummy_in);

		g_pcm86.irq = wavestar.pcm86_enabled ? wavestar.saved_pcm_irq : 0xff;
		g_opna[0].s.irq = wavestar.saved_opna_irq;
	}
}

static void IOOUTCALL boardws_o464(UINT port, REG8 val)
{
	if (wavestar.unlock_index == sizeof(s_unlock_sequence)) {
		if (val == 0x04) {
			wavestar.wss_mode = 1;
			wavestar.compat_status = 0x0c;
		}
		else {
			wavestar.wss_mode = 0;
			wavestar.compat_status = 0x08;
		}
		boardws_update_ports();
	}
	if (val == 0x09) {
		wavestar.compat_status = 0xff;
	}
	(void)port;
}

static REG8 IOINPCALL boardws_i464(UINT port)
{
	REG8 ret;
	(void)port;

	if (wavestar.unlock_index != sizeof(s_unlock_sequence)) {
		wavestar.compat_status = 0xff;
	}
	ret = wavestar.compat_status;
	wavestar.compat_status = (wavestar.compat_status == 0x00) ? 0xff : 0x00;
	return ret;
}

static void IOOUTCALL boardws_o466(UINT port, REG8 val)
{
	pcm86_oa466(port, val);
	if ((val & 0xe0) == 0x00) {
		boardws_setfmvolume((UINT8)((~val) & 15));
	}
}

/* ---- 04D2h board-specific register ---- */

static void IOOUTCALL boardws_o4d2(UINT port, REG8 dat)
{
	wavestar.trap_4d2 = dat;
	if ((dat >= 0x20) && (dat < 0x7f)) {
		TRACEOUT(("WaveStar VXD DIAG: port=%04x marker='%c' (0x%02x)",
			port, dat, dat));
	}
}

static REG8 IOINPCALL boardws_i4d2(UINT port)
{
	(void)port;
	return wavestar.trap_4d2;
}

/* ---- C-Bus Plug and Play board description ---- */

static int s_pnp_slot = -1;

static void boardws_pnp_reset_config(void *userdata)
{
	WAVESTAR *ws;

	ws = (WAVESTAR *)userdata;
	ZeroMemory(ws->pnp_config, sizeof(ws->pnp_config));
	ws->pnp_config[0x07] = 0;
	ws->pnp_config[0x30] = 1;
	ws->pnp_config[0x60] = (UINT8)(wavestar.compat_port >> 8); ws->pnp_config[0x61] = (UINT8)wavestar.compat_port;
	ws->pnp_config[0x62] = (UINT8)(wavestar.mpu_port >> 8); ws->pnp_config[0x63] = (UINT8)wavestar.mpu_port;
	ws->pnp_config[0x64] = (UINT8)(wavestar.fm_port >> 8); ws->pnp_config[0x65] = (UINT8)wavestar.fm_port;
	ws->pnp_config[0x66] = (UINT8)(wavestar.diag_port >> 8); ws->pnp_config[0x67] = (UINT8)wavestar.diag_port;
	ws->pnp_config[0x70] = wavestar.irq;
	ws->pnp_config[0x71] = 0;
	ws->pnp_config[0x72] = wavestar.mpu_irq;
	ws->pnp_config[0x73] = 0;
	ws->pnp_config[0x74] = wavestar.dma;
}

static UINT16 boardws_pnp_get_io(const WAVESTAR *ws, UINT8 reg)
{
	return (UINT16)(((UINT16)ws->pnp_config[reg] << 8) |
		ws->pnp_config[(UINT8)(reg + 1)]);
}

static void boardws_detach_runtime_io(void)
{
	cbuscore_detachsndex(wavestar.fm_port);
	boardws_detach_compat_ports(wavestar.compat_port);
	iocore_detachout(wavestar.diag_port);
	iocore_detachinp(wavestar.diag_port);
	iocore_detachout(wavestar.mpu_port);
	iocore_detachinp(wavestar.mpu_port);
	iocore_detachout(wavestar.mpu_port + 2);
	iocore_detachinp(wavestar.mpu_port + 2);
	dmac_detach(DMADEV_CS4231);
}

static void boardws_attach_runtime_io(void)
{
	if (!boardws_device_active()) {
		g_pcm86.irq = 0xff;
		g_opna[0].s.irq = 0xff;
		mpu98ii_setaux(0, 0);
		return;
	}

	cbuscore_attachsndex(wavestar.fm_port, s_opna_out, s_opna_in);
	boardws_update_ports();
	iocore_attachout(wavestar.diag_port, boardws_o4d2);
	iocore_attachinp(wavestar.diag_port, boardws_i4d2);

	if (np2cfg.mpuenable) {
		mpu98ii_setaux(wavestar.mpu_port, wavestar.mpu_irq);
		iocore_attachout(wavestar.mpu_port, mpu98ii_o0);
		iocore_attachinp(wavestar.mpu_port, mpu98ii_i0);
		iocore_attachout(wavestar.mpu_port + 2, mpu98ii_o2);
		iocore_attachinp(wavestar.mpu_port + 2, mpu98ii_i2);
	}
	else {
		mpu98ii_setaux(0, 0);
	}

	if (cs4231.dmach != 0xff) {
		dmac_attach(DMADEV_CS4231, cs4231.dmach);
	}
}

static void boardws_apply_pnp_config(void)
{
	UINT16 io;
	UINT8 irq;
	UINT8 dma;

	if (!wavestar.pnp_enabled) {
		return;
	}

	if (s_wavestar_bound) {
		boardws_detach_runtime_io();
	}

	io = boardws_pnp_get_io(&wavestar, 0x60);
	if (io != 0) {
		wavestar.compat_port = io;
	}
	io = boardws_pnp_get_io(&wavestar, 0x62);
	if (io != 0) {
		wavestar.mpu_port = io;
	}
	io = boardws_pnp_get_io(&wavestar, 0x64);
	if (io != 0) {
		wavestar.fm_port = io;
	}
	io = boardws_pnp_get_io(&wavestar, 0x66);
	if (io != 0) {
		wavestar.diag_port = io;
	}

	irq = wavestar.pnp_config[0x70];
	if ((irq == 12) || (irq == 13)) {
		wavestar.irq = irq;
	}
	irq = wavestar.pnp_config[0x72];
	if ((irq > 0) && (irq < 16)) {
		wavestar.mpu_irq = irq;
	}
	dma = wavestar.pnp_config[0x74];
	if ((dma == 0) || (dma == 3)) {
		wavestar.dma = dma;
	}

	opna_timer(&g_opna[0], (wavestar.irq == 13) ? 0x50 : 0xd0,
		NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	g_opna[0].s.base = (wavestar.fm_port == 0x0288) ? 0x100 : 0;
	wavestar.saved_opna_irq = wavestar.irq;
	wavestar.saved_pcm_irq = wavestar.pcm86_enabled ? wavestar.irq : 0xff;
	cs4231.dmairq = wavestar.irq;
	cs4231.dmach = wavestar.dma;
	cs4231.port[1] = wavestar.compat_port;
	cs4231.port[4] = wavestar.fm_port;
	cs4231.port[10] = wavestar.mpu_port;
	cs4231.port[15] = wavestar.compat_port;
	boardws_build_pnp_resources();

	if (s_wavestar_bound) {
		boardws_attach_runtime_io();
	}
}

static REG8 boardws_pnp_read_config(void *userdata, UINT8 ldn, UINT8 reg)
{
	WAVESTAR *ws;

	ws = (WAVESTAR *)userdata;
	if (ldn != 0) {
		return 0xff;
	}
	return ws->pnp_config[reg];
}

static void boardws_pnp_write_config(void *userdata, UINT8 ldn, UINT8 reg, REG8 value)
{
	WAVESTAR *ws;

	ws = (WAVESTAR *)userdata;
	if (ldn != 0) {
		return;
	}
	ws->pnp_config[reg] = value;
	if ((reg == 0x30) || ((reg >= 0x60) && (reg <= 0x75))) {
		TRACEOUT(("WaveStar PnP: LDN0 reg[%02x]=%02x", reg, value));
		boardws_apply_pnp_config();
	}
}

static const CBUSPNPCARD s_wavestar_pnp_card = {
	"WaveStar",
	s_pnp_serial,
	s_pnp_resources,
	sizeof(s_pnp_resources),
	1,
	boardws_pnp_reset_config,
	boardws_pnp_read_config,
	boardws_pnp_write_config,
	&wavestar
};

void boardws_reset(const NP2CFG *pConfig)
{
	ZeroMemory(&wavestar, sizeof(wavestar));
	wavestar.fm_volume = 15;
	wavestar.compat_status = 0xff;
	wavestar.trap_4d2 = 0xff;

	wavestar.compat_port = 0xa460;
	wavestar.mpu_port = WAVESTAR_MPU_PORT;
	wavestar.fm_port = (pConfig->sndwsio == 0x0288) ? 0x0288 : 0x0188;
	wavestar.diag_port = 0x04d2;
	wavestar.irq = (pConfig->sndwsirq == 13) ? 13 : 12;
	wavestar.mpu_irq = WAVESTAR_MPU_IRQ;
	wavestar.dma = (pConfig->sndwsdma == 0) ? 0 : 3;
	wavestar.pnp_enabled = pConfig->sndwspnp ? 1 : 0;
	wavestar.pcm86_enabled = pConfig->sndwspcm ? 1 : 0;

	opna_reset(&g_opna[0], OPNA_MODE_2608 | OPNA_HAS_FM | OPNA_HAS_TIMER | OPNA_S98);
	opna_timer(&g_opna[0], (wavestar.irq == 13) ? 0x50 : 0xd0, NEVENT_FMTIMERA, NEVENT_FMTIMERB);
	opngen_setcfg(&g_opna[0].opngen, 3, OPN_STEREO | 0x38);
	g_opna[0].s.base = (wavestar.fm_port == 0x0288) ? 0x100 : 0;
	fmboard_extreg(boardws_extendchannel);

	g_pcm86.soundflags = 0x41;
	g_pcm86.irq = wavestar.pcm86_enabled ? wavestar.irq : 0xff;
	fmboard_extenable(1);
	wavestar.saved_pcm_irq = g_pcm86.irq;
	wavestar.saved_opna_irq = g_opna[0].s.irq;

	cs4231io_reset_ex(wavestar.irq, wavestar.dma, wavestar.compat_port, wavestar.mpu_port, 0x80);
	cs4231.dmairq = wavestar.irq;
	cs4231.port[4] = wavestar.fm_port;
	if (pConfig->mpuenable) {
		mpu98ii_setaux(wavestar.mpu_port, wavestar.mpu_irq);
	}
	else {
		mpu98ii_setaux(0, 0);
	}

	s_pnp_slot = -1;
	if (wavestar.pnp_enabled) {
		boardws_pnp_reset_config(&wavestar);
		boardws_build_pnp_resources();
		s_pnp_slot = cbuspnp_register_card(&s_wavestar_pnp_card);
	}
	boardws_setfmvolume(15);
	oplgen_setvol(np2cfg.vol_fm * np2cfg.vol_master / 100);
}

void boardws_bind(void)
{
	if (wavestar.pnp_enabled) {
		boardws_apply_pnp_config();
	}
	else {
		if ((wavestar.saved_opna_irq == 12) || (wavestar.saved_opna_irq == 13)) {
			wavestar.irq = wavestar.saved_opna_irq;
		}
		if ((mpu98.irqnum > 0) && (mpu98.irqnum < 16)) {
			wavestar.mpu_irq = mpu98.irqnum;
		}
		if ((cs4231.dmach == 0) || (cs4231.dmach == 3)) {
			wavestar.dma = cs4231.dmach;
		}
	}

	opna_bind(&g_opna[0]);
	sound_streamregist(&g_pcm86, (SOUNDCB)pcm86gen_getpcm);
	sound_streamregist(&cs4231, (SOUNDCB)cs4231_getpcm);

	s_wavestar_bound = 1;
	boardws_attach_runtime_io();
	boardws_setfmvolume(wavestar.fm_volume);
}

void boardws_unbind(void)
{
	if (s_wavestar_bound) {
		boardws_detach_runtime_io();
		s_wavestar_bound = 0;
	}
	mpu98ii_setaux(0, 0);
	if (s_pnp_slot >= 0) {
		cbuspnp_unregister_card(s_pnp_slot);
		s_pnp_slot = -1;
	}
}
