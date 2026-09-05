/**
 * @file cbuspnp.c
 * @brief Generic PC-98 C-Bus Plug and Play bus
 */

#include "compiler.h"
#include "pccore.h"
#include "cpucore.h"
#include <io/iocore.h>
#include "cbuspnp.h"

CBUSPNP cbuspnp;

static const CBUSPNPCARD *s_cards[CBUSPNP_MAX_CARDS];
static UINT8 s_bound;
static UINT8 s_io_attached;

static REG8 IOINPCALL cbuspnp_read_data_port(UINT port);

static UINT cbuspnp_card_count(void)
{
	UINT i;
	UINT count;

	count = 0;
	for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
		if (s_cards[i] != NULL) {
			count++;
		}
	}
	return count;
}

static const char *cbuspnp_card_name(UINT slot)
{
	if ((slot < CBUSPNP_MAX_CARDS) && (s_cards[slot] != NULL) &&
		(s_cards[slot]->name != NULL)) {
		return s_cards[slot]->name;
	}
	return "C-Bus PnP";
}

static int cbuspnp_find_isolation_card(void)
{
	UINT i;

	for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
		if ((s_cards[i] != NULL) && cbuspnp.card[i].awake &&
			(cbuspnp.card[i].csn == 0)) {
			return (int)i;
		}
	}
	return -1;
}

static int cbuspnp_find_config_card(void)
{
	UINT i;

	for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
		if ((s_cards[i] != NULL) && cbuspnp.card[i].awake &&
			(cbuspnp.card[i].csn != 0)) {
			return (int)i;
		}
	}
	return cbuspnp_find_isolation_card();
}

static void cbuspnp_reset_card_config(UINT slot, BOOL deactivate)
{
	const CBUSPNPCARD *card;
	UINT ldn;

	card = s_cards[slot];
	if (card == NULL) {
		return;
	}
	if (card->reset_config != NULL) {
		card->reset_config(card->userdata);
	}
	if (deactivate && (card->write_config != NULL)) {
		for (ldn = 0; ldn < card->logical_devices; ldn++) {
			card->write_config(card->userdata, (UINT8)ldn, 0x30, 0);
		}
	}
}

static REG8 cbuspnp_read_config(UINT slot, UINT8 reg)
{
	const CBUSPNPCARD *card;
	CBUSPNPCARDSTATE *state;

	card = s_cards[slot];
	state = &cbuspnp.card[slot];
	if (card == NULL) {
		return 0xff;
	}

	switch (reg) {
	case 0x00:
		return (REG8)(cbuspnp.read_port >> 2);
	case 0x05:
		return (state->awake && (state->csn != 0)) ? 0x01 : 0x00;
	case 0x06:
		return state->csn;
	case 0x07:
		return state->ldn;
	default:
		if ((state->ldn < card->logical_devices) && (card->read_config != NULL)) {
			return card->read_config(card->userdata, state->ldn, reg);
		}
		break;
	}
	return 0xff;
}

static void cbuspnp_set_read_port(UINT newport)
{
	UINT oldport;

	oldport = cbuspnp.read_port;
	if (newport == 0) {
		newport = CBUSPNP_DEFAULT_READ_PORT;
	}
	if (oldport == newport) {
		mem[0x5b7] = (UINT8)(newport >> 2);
		return;
	}

	if (s_io_attached) {
		iocore_detachinp(oldport);
	}
	cbuspnp.read_port = (UINT16)newport;
	mem[0x5b7] = (UINT8)(newport >> 2);
	if (s_io_attached) {
		iocore_attachinp(newport, cbuspnp_read_data_port);
	}
}

static REG8 IOINPCALL cbuspnp_read_data_port(UINT port)
{
	int slot;
	const CBUSPNPCARD *card;
	CBUSPNPCARDSTATE *state;
	UINT bitno;
	UINT bytepos;
	UINT phase;
	UINT8 bit;
	UINT pos;
	REG8 ret;

	(void)port;

	if (cbuspnp.address == 0x01) {
		slot = cbuspnp_find_isolation_card();
		if (slot < 0) {
			return 0x00;
		}
		card = s_cards[slot];
		state = &cbuspnp.card[slot];
		if ((card->serial == NULL) ||
			(state->isolation_pos >= (CBUSPNP_SERIAL_SIZE * 8 * 2))) {
			return 0x00;
		}
		bitno = state->isolation_pos >> 1;
		phase = state->isolation_pos & 1;
		bytepos = bitno >> 3;
		bit = (UINT8)((card->serial[bytepos] >> (bitno & 7)) & 1);
		state->isolation_pos++;
		if (!bit) {
			return 0x00;
		}
		return phase ? 0xaa : 0x55;
	}

	slot = cbuspnp_find_config_card();
	if (slot < 0) {
		return (cbuspnp.address == 0x05) ? 0x00 : 0xff;
	}
	card = s_cards[slot];
	state = &cbuspnp.card[slot];

	switch (cbuspnp.address) {
	case 0x04:
		if (!state->awake || (state->csn == 0)) {
			return 0xff;
		}
		if (state->resource_pos < CBUSPNP_SERIAL_SIZE) {
			pos = state->resource_pos++;
			ret = card->serial[pos];
			TRACEOUT(("CBus PnP[%s]: RESOURCE[%u] serial=%02x",
				cbuspnp_card_name((UINT)slot), pos, ret));
			return ret;
		}
		pos = state->resource_pos - CBUSPNP_SERIAL_SIZE;
		if ((card->resources != NULL) && (pos < card->resources_size)) {
			ret = card->resources[pos];
			TRACEOUT(("CBus PnP[%s]: RESOURCE[%u] data=%02x",
				cbuspnp_card_name((UINT)slot), state->resource_pos, ret));
			state->resource_pos++;
			return ret;
		}
		state->resource_pos = CBUSPNP_SERIAL_SIZE + card->resources_size;
		if (!state->resource_eof_logged) {
			TRACEOUT(("CBus PnP[%s]: RESOURCE read past End Tag (pos=%u)",
				cbuspnp_card_name((UINT)slot), state->resource_pos));
			state->resource_eof_logged = 1;
		}
		return 0xff;

	default:
		return cbuspnp_read_config((UINT)slot, cbuspnp.address);
	}
}

static void IOOUTCALL cbuspnp_o259(UINT port, REG8 dat)
{
	cbuspnp.address = dat;
	(void)port;
}

static REG8 IOINPCALL cbuspnp_i259(UINT port)
{
	(void)port;
	return cbuspnp.address;
}

static void IOOUTCALL cbuspnp_oa59(UINT port, REG8 dat)
{
	UINT i;
	UINT newport;
	int slot;
	const CBUSPNPCARD *card;
	CBUSPNPCARDSTATE *state;

	(void)port;

	switch (cbuspnp.address) {
	case 0x00:
		newport = ((UINT)dat << 2) | 0x3;
		cbuspnp_set_read_port(newport);
		break;

	case 0x02:
		for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
			if (s_cards[i] == NULL) {
				continue;
			}
			state = &cbuspnp.card[i];
			if (dat & 0x01) {
				cbuspnp_reset_card_config(i, TRUE);
			}
			if (dat & 0x04) {
				state->csn = 0;
			}
			if (dat & 0x02) {
				state->awake = 0;
			}
			state->isolation_pos = 0;
			state->resource_pos = 0;
			state->resource_eof_logged = 0;
			TRACEOUT(("CBus PnP[%s]: CONFIG_CONTROL=%02x csn=%u awake=%u",
				cbuspnp_card_name(i), dat, state->csn, state->awake));
		}
		break;

	case 0x03:
		for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
			if (s_cards[i] == NULL) {
				continue;
			}
			state = &cbuspnp.card[i];
			state->awake = (dat == state->csn) ? 1 : 0;
			state->isolation_pos = 0;
			state->resource_pos = 0;
			state->resource_eof_logged = 0;
			TRACEOUT(("CBus PnP[%s]: WAKE(%u) card_csn=%u awake=%u",
				cbuspnp_card_name(i), dat, state->csn, state->awake));
		}
		break;

	case 0x06:
		slot = cbuspnp_find_isolation_card();
		if (slot >= 0) {
			state = &cbuspnp.card[slot];
			state->csn = dat;
			state->awake = 1;
			state->resource_pos = CBUSPNP_SERIAL_SIZE;
			state->resource_eof_logged = 0;
			TRACEOUT(("CBus PnP[%s]: SET_CSN=%u -> Config, resource pos=%u",
				cbuspnp_card_name((UINT)slot), state->csn, state->resource_pos));
		}
		break;

	case 0x07:
		for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
			if ((s_cards[i] != NULL) && cbuspnp.card[i].awake) {
				cbuspnp.card[i].ldn = dat;
			}
		}
		break;

	default:
		for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
			if ((s_cards[i] == NULL) || !cbuspnp.card[i].awake) {
				continue;
			}
			card = s_cards[i];
			state = &cbuspnp.card[i];
			if ((state->ldn < card->logical_devices) && (card->write_config != NULL)) {
				card->write_config(card->userdata, state->ldn, cbuspnp.address, dat);
			}
		}
		break;
	}
}

static REG8 IOINPCALL cbuspnp_ia59(UINT port)
{
	int slot;

	(void)port;
	if (cbuspnp.address == 0) {
		return mem[0x5b7];
	}
	slot = cbuspnp_find_config_card();
	if (slot < 0) {
		return 0xff;
	}
	return cbuspnp_read_config((UINT)slot, cbuspnp.address);
}

void cbuspnp_bios_update(void)
{
	UINT count;

	count = cbuspnp_card_count();
	if (count != 0) {
		if (cbuspnp.read_port == 0) {
			cbuspnp.read_port = CBUSPNP_DEFAULT_READ_PORT;
		}
		mem[0x5b7] = (UINT8)(cbuspnp.read_port >> 2);
		mem[0x5b8] = (UINT8)count;
	}
	else {
		mem[0x5b8] = 0x00;
	}
}

static void cbuspnp_attach_io(void)
{
	if (!s_bound || s_io_attached || (cbuspnp_card_count() == 0)) {
		return;
	}
	if (cbuspnp.read_port == 0) {
		cbuspnp.read_port = CBUSPNP_DEFAULT_READ_PORT;
	}
	cbuspnp_bios_update();
	iocore_attachinp(cbuspnp.read_port, cbuspnp_read_data_port);
	iocore_attachout(0x259, cbuspnp_o259);
	iocore_attachout(0xa59, cbuspnp_oa59);
	iocore_attachinp(0x259, cbuspnp_i259);
	iocore_attachinp(0xa59, cbuspnp_ia59);
	s_io_attached = 1;
}

static void cbuspnp_detach_io(void)
{
	if (!s_io_attached) {
		return;
	}
	iocore_detachinp(cbuspnp.read_port);
	iocore_detachout(0x259);
	iocore_detachout(0xa59);
	iocore_detachinp(0x259);
	iocore_detachinp(0xa59);
	s_io_attached = 0;
	cbuspnp_bios_update();
}

void cbuspnp_reset(const NP2CFG *pConfig)
{
	(void)pConfig;
	ZeroMemory(&cbuspnp, sizeof(cbuspnp));
	ZeroMemory(s_cards, sizeof(s_cards));
	cbuspnp.read_port = CBUSPNP_DEFAULT_READ_PORT;
	mem[0x5b7] = (UINT8)(CBUSPNP_DEFAULT_READ_PORT >> 2);
	mem[0x5b8] = 0x00;
	s_bound = 0;
	s_io_attached = 0;
}

void cbuspnp_bind(void)
{
	s_bound = 1;
	cbuspnp_attach_io();
}

int cbuspnp_register_card(const CBUSPNPCARD *card)
{
	UINT i;

	if ((card == NULL) || (card->serial == NULL) ||
		(card->logical_devices == 0)) {
		return -1;
	}
	for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
		if (s_cards[i] == card) {
			return (int)i;
		}
	}
	for (i = 0; i < CBUSPNP_MAX_CARDS; i++) {
		if (s_cards[i] == NULL) {
			s_cards[i] = card;
			ZeroMemory(&cbuspnp.card[i], sizeof(cbuspnp.card[i]));
			cbuspnp.card[i].csn = (UINT8)(i + 1);
			cbuspnp_reset_card_config(i, FALSE);
			cbuspnp_bios_update();
			cbuspnp_attach_io();
			return (int)i;
		}
	}
	return -1;
}

void cbuspnp_unregister_card(int slot)
{
	if ((slot < 0) || (slot >= CBUSPNP_MAX_CARDS) || (s_cards[slot] == NULL)) {
		return;
	}
	s_cards[slot] = NULL;
	ZeroMemory(&cbuspnp.card[slot], sizeof(cbuspnp.card[slot]));
	if (cbuspnp_card_count() == 0) {
		cbuspnp_detach_io();
	}
	else {
		cbuspnp_bios_update();
	}
}

