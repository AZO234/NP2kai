/**
 * @file cbuspnp.h
 * @brief Generic PC-98 C-Bus Plug and Play bus
 */
#pragma once

#include "compiler.h"
#include "pccore.h"

#define CBUSPNP_MAX_CARDS 8
#define CBUSPNP_SERIAL_SIZE 9
#define CBUSPNP_DEFAULT_READ_PORT 0x0277

typedef struct {
	UINT8 csn;
	UINT8 ldn;
	UINT8 awake;
	UINT8 resource_eof_logged;
	UINT32 isolation_pos;
	UINT32 resource_pos;
} CBUSPNPCARDSTATE;

typedef struct {
	UINT8 address;
	UINT8 reserved;
	UINT16 read_port;
	CBUSPNPCARDSTATE card[CBUSPNP_MAX_CARDS];
} CBUSPNP;

typedef void (*CBUSPNP_RESET_CONFIG)(void *userdata);
typedef REG8 (*CBUSPNP_READ_CONFIG)(void *userdata, UINT8 ldn, UINT8 reg);
typedef void (*CBUSPNP_WRITE_CONFIG)(void *userdata, UINT8 ldn, UINT8 reg, REG8 value);

typedef struct {
	const char *name;
	const UINT8 *serial;
	const UINT8 *resources;
	UINT resources_size;
	UINT8 logical_devices;
	CBUSPNP_RESET_CONFIG reset_config;
	CBUSPNP_READ_CONFIG read_config;
	CBUSPNP_WRITE_CONFIG write_config;
	void *userdata;
} CBUSPNPCARD;

#ifdef __cplusplus
extern "C" {
#endif

extern CBUSPNP cbuspnp;

void cbuspnp_reset(const NP2CFG *pConfig);
void cbuspnp_bind(void);
void cbuspnp_bios_update(void);

int cbuspnp_register_card(const CBUSPNPCARD *card);
void cbuspnp_unregister_card(int slot);


#ifdef __cplusplus
}
#endif
