#include "compiler.h"

#if defined(SUPPORT_GPIB)

typedef enum {
	STATE_TIDS, /* Talker Idle State */
	STATE_TADS, /* Talker Addressed State */
	STATE_TACS, /* Talker Active State */
	STATE_SPAS, /* Serial Poll Active State */
} talker_state_t;

typedef enum {
	STATE_LIDS, /* Listener Idle State */
	STATE_LADS, /* Listener Addressed State */
	STATE_LACS, /* Listener Idle State */
	STATE_LPAS, /* Listener Primary Addressed State */
	STATE_LPIS, /* Listener Primary Idle State */
} listener_state_t;

typedef enum {
	STATE_NPRS, /* Negative Poll response state */
	STATE_SRQS, /* Service Request State */
	STATE_APRS, /* Affirmate Poll Response State */
} sr_state_t;

typedef enum {
	STATE_LOCS, /* Local state */
	STATE_REMS, /* Remote State */
	STATE_RWLS, /* Remote with lockout state */
	STATE_LWLS, /* Local with lockout state */
} remote_local_function_state_t;

typedef enum {
	MESSAGE_PON, /* Power on */
	MESSAGE_RTL, /* Return to local */
	MESSAGE_RSV, /* Request service */
	MESSAGE_LON, /* Listen only */
	MESSAGE_LTN, /* Message Listen */
	MESSAGE_LUN, /* Local unlisten */
	MESSAGE_TON, /* Talk only */
	MESSAGE_GTS, /* Go to standby */
	MESSAGE_RPP, /* request parallel poll */
	MESSAGE_RSC, /* request system control */
	MESSAGE_SIC, /* send interface clear */
	MESSAGE_SRE, /* send remote enable */
	MESSAGE_TCA, /* take control async */
	MESSAGE_TCS, /* take control sync */

	/* external messages */
	MESSAGE_ATN, /* Attention */
	MESSAGE_IFC, /* Interface clear */
	MESSAGE_UNL, /* Message unlisten */
	MESSAGE_MLA, /* My listen address */
	MESSAGE_TLA, /* My Talk address */
	MESSAGE_MSA, /* My secondary address */
	MESSAGE_OTA, /* Other talk address */
	MESSAGE_OSA, /* Other service address */
	MESSAGE_SPE, /* Serial poll enable */
	MESSAGE_SPD, /* Serial poll disable */
	MESSAGE_RQS, /* Request service */
	MESSAGE_PCG, /* Primary Command group */
	MESSAGE_DAB, /* Data byte */
	MESSAGE_END, /* END */
	MESSAGE_STS, /* Status byte */
	MESSAGE_TCT, /* Take control */
	MESSAGE_IDY, /* Identify */
	MESSAGE_REN, /* Remote enable */
} message_t;

typedef enum {
	STATE_SDYS, /* Source delay state */
	STATE_SGNS, /* Source Generator state */
	STATE_SIDS, /* Source Idle state */
	STATE_SIWS, /* Source Idle wait state */
	STATE_STRS, /* Source transfer state */
	STATE_SWNS, /* Source wait for new cycle state */
} source_handshake_state_t;

typedef enum {
	STATE_ACDS, /* Accept Data state */
	STATE_AIDS, /* Acceptor idle state */
	STATE_ANRS, /* Acceptor not ready state */
	STATE_ACRS, /* Acceptor ready state */
	STATE_AWNS, /* Acceptor wait for new cycle state */
} ah_state_t;

typedef enum {
	STATE_CIDS, /* Controller idle state */
	STATE_CTRS, /* Controller active wait state */
	STATE_CADS, /* Controller Addressed state */
	STATE_CACS, /* Controller active state */
	STATE_CSBS, /* Controller standby state */
	STATE_CPWS, /* Controller parallel poll wait state */
	STATE_CAWS, /* Controller active wait state */
	STATE_CSWS, /* Controller synchronous wait state */
	STATE_CPPS, /* Controller parallel poll state */
} controller_state_t;

typedef enum {
	STATE_CSNS, /* Controller service not requested */
	STATE_CSRS, /* Controller service requested */
} srq_state_t;

typedef enum {
	STATE_SNAS, /* System control not active */
	STATE_SACS, /* System control active */
} rsc_state_t;

typedef enum {
	STATE_SRIS, /* System control remote enable idle */
	STATE_SRAS, /* system control remote enable active */
	STATE_SRNS, /* System control remote enable not active */
} sre_state_t;

typedef enum {
	STATE_SIIS, /* System control interface clear idle */
	STATE_SIAS, /* System control interface clear active */
	STATE_SINS, /* System control interface clear not active */
} sic_state_t;

typedef enum {
	STATE_DCIS, /* Device clear inactive */
	STATE_DCAS, /* Device clear active */
} dc_state_t;


typedef struct NIPCIIAState {
//	ISADevice dev;
	uint32_t  iobase;
	uint32_t isairq;
	uint32_t isadma;
	uint32_t typea;
	uint32_t debug_level;
//	qemu_irq  irq;
	uint8_t irq;
	uint8_t isr1;
	uint8_t isr2;
	uint8_t imr1;
	uint8_t imr2;
	uint8_t adr0;
	uint8_t adr1;
	uint8_t eos;
	uint8_t icr;
	uint8_t ppr;
	uint8_t auxra;
	uint8_t auxrb;
	uint8_t auxre;
	uint8_t admr;
	uint8_t adsr;
	uint8_t spsr;
	uint8_t dir;
	char write_buffer[4096];
	int write_buffer_used;
	int send_eoi;
	int page_in;
	int gpib_dev[30];
	int cur_talker;
	int cur_listener;

	talker_state_t talker_state;
	listener_state_t listener_state;
	sr_state_t service_request_state;
	remote_local_function_state_t remote_local_state;
	source_handshake_state_t source_handshake_state;
	ah_state_t ah_state;
	controller_state_t controller_state;
	srq_state_t srq_state;
	rsc_state_t rsc_state;
	sre_state_t sre_state;
	sic_state_t sic_state;
	dc_state_t dc_state;

	int atn;
	int ifc;
	int ren;
	int rsc;
	int sic;
	int sre;
	int adsc;
	int lpas;
	int tpas;
	int mjmn;
	int spe;
} _GPIBIO, *GPIBIO;

#ifdef __cplusplus
extern "C" {
#endif

extern	_GPIBIO		gpibio;

void gpibioint(NEVENTITEM item);

void gpibio_reset(const NP2CFG *pConfig);
void gpibio_bind(void);

#ifdef __cplusplus
}
#endif

#endif

