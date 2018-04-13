/*
 * NI PCII/A support
 *
 * Copyright (c) 2014 Sven Schnelle <svens@stackframe.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * ported for NP2
 * Copyright (c) 2018 AZO
 */

#include	"compiler.h"

#if defined(SUPPORT_GPIB)

#include	<unistd.h>
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"gpibio.h"

enum {
	GPIB_IRQ		= 0x09,	// equal SASI
	GPIB_DMACH		= 0x00,	// equal SASI

/*
	GPIBPHASE_FREE	= 0,
	GPIBPHASE_CMD	= 1,
	GPIBPHASE_C2	= 2,
	GPIBPHASE_SENSE	= 3,
	GPIBPHASE_READ	= 4,
	GPIBPHASE_WRITE	= 5,
	GPIBPHASE_STAT	= 6,
	GPIBPHASE_MSG	= 7,
*/
};

_GPIBIO		gpibio;

void gpibioint(NEVENTITEM item) {

//	gpibio.phase = GPIBPHASE_STAT;
//	gpibio.isrint = GPIBISR_INT;
//	if (gpibio.ocr & GPIBOCR_INTE) {
		TRACEOUT(("gpib set irq"));
		pic_setirq(GPIB_IRQ);
//	}
	(void)item;
}


// ----

#define GPIB_ADSR_CIC 0x80
#define GPIB_ADSR_ATN 0x40

#define GPIB_ISR2_CO 0x08
#define GPIB_ISR2_SRQ 0x40

#define GPIB_IMR2_DMAO 0x20
#define GPIB_IMR2_DMAI 0x10

#define GPIB_ISR1_ENDRX 0x10

static void controller_function(GPIBIO s);

static void gpib_debug(GPIBIO s, int level,
		       const char *fmt, ...)
{
	va_list va;

//	if (level > s->debug_level)
//		return;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
}


static const char *controller_state_to_string(controller_state_t state)
{
	switch(state) {
	case STATE_CIDS:
		return "CIDS";
	case STATE_CTRS:
		return "CTRS";
	case STATE_CADS:
		return "CADS";
	case STATE_CACS:
		return "CACS";
	case STATE_CSBS:
		return "CSBS";
	case STATE_CPWS:
		return "CPWS";
	case STATE_CAWS:
		return "CAWS";
	case STATE_CSWS:
		return "CSWS";
	case STATE_CPPS:
		return "CPPS";
	}
	return "UNKNOWN CONTROLLER STATE";
}

static const char *ah_state_to_string(ah_state_t state)
{
	switch(state) {
	case STATE_ACDS:
		return "ACDS";
	case STATE_AIDS:
		return "AIDS";
	case STATE_ANRS:
		return "ANRS";
	case STATE_ACRS:
		return "ACRS";
	case STATE_AWNS:
		return "AWNS";
	}
	return "UNKNOWN ACCEPTOR_HANDSHAKE STATE";
}

static const char *sre_state_to_string(sre_state_t state)
{
	switch(state) {
	case STATE_SRIS:
		return "SRIS";
	case STATE_SRAS:
		return "SRAS";
	case STATE_SRNS:
		return "SRNS";
	}
	return "UNKNOWN SRE STATE";
}

static const char *srq_state_to_string(srq_state_t state)
{
	switch(state) {
	case STATE_CSNS:
		return "CSNS";
	case STATE_CSRS:
		return "CSRS";
	}
	return "UNKNOWN SRQ STATE";
}

static const char *rsc_state_to_string(rsc_state_t state)
{
	switch(state) {
	case STATE_SNAS:
		return "SNAS";
	case STATE_SACS:
		return "SACS";
	}
	return "UNKNOWN RSC STATE";
}

static const char *sic_state_to_string(sic_state_t state)
{
	switch(state) {
	case STATE_SIIS:
		return "SIIS";
	case STATE_SIAS:
		return "SIAS";
	case STATE_SINS:
		return "SINS";
	}
	return "UNKNOWN SIC STATE";
}

static const char *sr_state_to_string(sr_state_t state)
{
	switch(state) {
	case STATE_APRS:
		return "APRS";
	case STATE_NPRS:
		return "NPRS";
	case STATE_SRQS:
		return "SRQS";
	}
	return "UNKNOWN SERVICE REQUEST STATE";
}

static const char *source_handshake_state_to_string(source_handshake_state_t state)
{
	switch(state) {
	case STATE_SDYS:
		return "SDYS";
	case STATE_SGNS:
		return "SGNS";
	case STATE_SIDS:
		return "SIDS";
	case STATE_SIWS:
		return "SIWS";
	case STATE_STRS:
		return "STRS";
	case STATE_SWNS:
		return "SWNS";
	}
	return "UNKNOWN SOURCE HANDSHALE STATE";
}

static const char *talker_state_to_string(talker_state_t state)
{
	switch(state) {
	case STATE_TIDS:
		return "TIDS";
	case STATE_TADS:
		return "TADS";
	case STATE_TACS:
		return "TACS";
	case STATE_SPAS:
		return "SPAS";
	}
	return "UNKNOWN TALKER STATE";
}

static const char *listener_state_to_string(listener_state_t state)
{
	switch(state) {
	case STATE_LIDS:
		return "LIDS";
	case STATE_LADS:
		return "LADS";
	case STATE_LACS:
		return "LACS";
	case STATE_LPAS:
		return "LPAS";
	case STATE_LPIS:
		return "LPIS";
	}
	return "UNKNOWN LISTENER STATE";
}

static const char *remote_local_state_to_string(remote_local_function_state_t state)
{
	switch(state) {
	case STATE_LOCS:
		return "LOCS";
	case STATE_REMS:
		return "REMS";
	case STATE_RWLS:
		return "RWLS";
	case STATE_LWLS:
		return "LWLS";
	}
	return "UNKNOWN REMOTE/LOCAL STATE";
}

static const char *dc_state_to_string(dc_state_t state)
{
	switch(state) {
	case STATE_DCIS:
		return "DCIS";
	case STATE_DCAS:
		return "DCAS";
	}
	return "UNKNOWN DC STATE";
}

static void update_talker_state(GPIBIO s, talker_state_t state)
{
	if (state == s->talker_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   talker_state_to_string(s->talker_state),
		   talker_state_to_string(state));
	s->talker_state = state;
}

static void update_listener_state(GPIBIO s, listener_state_t state)
{
	if (state == s->listener_state)
		return;
	gpib_debug(s, 2, "%s -> %s\n",
		   listener_state_to_string(s->listener_state),
		   listener_state_to_string(state));
	s->listener_state = state;
}

static void update_remote_local_function_state(GPIBIO s, remote_local_function_state_t state)
{
	if (state == s->remote_local_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   remote_local_state_to_string(s->remote_local_state),
		   remote_local_state_to_string(state));
	s->remote_local_state = state;
}

static void update_ah_state(GPIBIO s, ah_state_t state)
{
	if (state == s->ah_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   ah_state_to_string(s->ah_state),
		   ah_state_to_string(state));
	s->ah_state = state;
}

static void update_source_handshake_function_state(GPIBIO s,
						   source_handshake_state_t state)
{
	if (state == s->source_handshake_state)
		return;
	gpib_debug(s, 2, "%s -> %s\n",
		   source_handshake_state_to_string(s->source_handshake_state),
		   source_handshake_state_to_string(state));
	s->source_handshake_state = state;
}

static void update_sr_state(GPIBIO s, sr_state_t state)
{
	if (state == s->service_request_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   sr_state_to_string(s->srq_state),
		   sr_state_to_string(state));
	s->srq_state = state;
}

static void update_controller_state(GPIBIO s,
				    controller_state_t state)
{
	if (state == s->controller_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   controller_state_to_string(s->controller_state),
		   controller_state_to_string(state));
	s->controller_state = state;
	controller_function(s);
}

static void update_rsc_state(GPIBIO s, rsc_state_t state)
{
	if (state == s->rsc_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   rsc_state_to_string(s->rsc_state),
		   rsc_state_to_string(state));
	s->rsc_state = state;
}

static void update_sre_state(GPIBIO s, sre_state_t state)
{
	if (state == s->sre_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   sre_state_to_string(s->sre_state),
		   sre_state_to_string(state));
	s->sre_state = state;
}

static void update_sic_state(GPIBIO s, sic_state_t state)
{
	if (state == s->sic_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   sic_state_to_string(s->sic_state),
		   sic_state_to_string(state));
	s->sic_state = state;
}

static void update_srq_state(GPIBIO s, srq_state_t state)
{
	if (state == s->srq_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   srq_state_to_string(s->srq_state),
		   srq_state_to_string(state));
	s->srq_state = state;
}

static void update_dc_state(GPIBIO s, dc_state_t state)
{
	if (state == s->dc_state)
		return;

	gpib_debug(s, 2, "%s -> %s\n",
		   dc_state_to_string(s->dc_state),
		   dc_state_to_string(state));
	s->dc_state = state;
}


static void controller_function(GPIBIO s)
{
	char stb;
	switch(s->controller_state) {
	case STATE_CIDS:
		s->atn = 0;
		if (s->ifc && s->rsc_state == STATE_SACS) {
			update_controller_state(s, STATE_CACS);
			s->adsc = 1;
		}
		break;
	case STATE_CADS:
		update_controller_state(s, STATE_CACS);
		s->atn = 0;
		break;

	case STATE_CACS:
		s->atn = 1;
		break;

	case STATE_CSBS:
		s->atn = 0;
		if (s->spe) {
			s->spe = 0;
			ibrsp(s->gpib_dev[s->cur_talker], &stb);
			gpib_debug(s, 2, "ibrsp: dev %d, s->cur_talker %d, ibsta %x, stb %x\n", s->gpib_dev[s->cur_talker], s->cur_talker, ibsta, stb);
			if (s->cur_talker == 16 && stb & 0x1) {
				gpib_debug(s, 2, "changing stb byte\n");
				stb = 0x50;
			}
			s->dir = stb;
			s->isr1 |= 1; /* Data ready */
		}
		break;
	default:
		break;

	}
}

static void talker_function(GPIBIO s)
{
	switch(s->talker_state) {
	case STATE_TIDS:
		break;
	case STATE_TADS:
		if (!s->atn)
			s->talker_state = STATE_TACS;
	case STATE_TACS:
		if (s->atn)
			s->talker_state = STATE_TADS;
	case STATE_SPAS:
		break;
	}
}

static void listener_function(GPIBIO s)
{
}

static void sh_function(GPIBIO s)
{
}

static void ah_function(GPIBIO s)
{
}
static void sr_function(GPIBIO s)
{
}

static void rl_function(GPIBIO s)
{
}

static void dc_function(GPIBIO s)
{
	update_dc_state(s, s->ifc ? STATE_DCAS : STATE_DCIS);
}

static void sic_function(GPIBIO s)
{
	if (s->rsc_state != STATE_SACS)
		update_sic_state(s, STATE_SIIS);

	switch(s->sic_state) {
	case STATE_SIIS:
		s->ifc = 0;
		if (s->rsc_state == STATE_SACS)
			update_sic_state(s, s->sic ? STATE_SIAS : STATE_SINS);
		break;
	case STATE_SIAS:
		s->ifc = 1;
		if (!s->sic)
			update_sic_state(s, STATE_SINS);
		break;
	case STATE_SINS:
		s->ifc = 0;
		if (s->sic)
			update_sic_state(s, STATE_SIAS);
		break;
	}
}

static void sre_function(GPIBIO s)
{
	switch(s->sre_state) {
	case STATE_SRIS:
		s->ren = 0;
		if (s->rsc_state == STATE_SACS)
			update_sre_state(s, s->sre ? STATE_SRAS : STATE_SRNS);
		break;
	case STATE_SRAS:
		if (!s->sre)
			update_sre_state(s, STATE_SRNS);
		s->ren = 1;
		break;

	case STATE_SRNS:
		if (s->sre)
			update_sre_state(s, STATE_SRAS);
		s->ren = 0;
		break;
	}
}

static void rsc_function(GPIBIO s)
{
	update_rsc_state(s, s->rsc ? STATE_SACS : STATE_SNAS);
	sic_function(s);
	sre_function(s);
}

static const char *message_to_string(message_t msg)
{
	switch(msg) {
	case MESSAGE_PON:
		return "PON";
	case MESSAGE_TON:
		return "TON";
	case MESSAGE_LON:
		return "LON";
	case MESSAGE_IFC:
		return "IFC";
	case MESSAGE_RSV:
		return "RSV";
	case MESSAGE_LTN:
		return "LTN";
	case MESSAGE_UNL:
		return "UNL";
	case MESSAGE_RSC:
		return "RSC";
	case MESSAGE_LUN:
		return "LUN";
	case MESSAGE_RTL:
		return "RTL";
	case MESSAGE_GTS:
		return "GTS";
	case MESSAGE_RPP:
		return "RPP";
	case MESSAGE_SIC:
		return "SIC";
	case MESSAGE_SRE:
		return "SRE";
	case MESSAGE_TCA:
		return "TCA";
	case MESSAGE_ATN:
		return "ATN";
	case MESSAGE_MLA:
		return "MLA";
	case MESSAGE_TLA:
		return "TLA";
	case MESSAGE_TCS:
		return "TCS";
	case MESSAGE_MSA:
		return "MSA";
	case MESSAGE_OTA:
		return "OTA";
	case MESSAGE_OSA:
		return "OSA";
	case MESSAGE_SPE:
		return "SPE";
	case MESSAGE_SPD:
		return "SPD";
	case MESSAGE_RQS:
		return "RQS";
	case MESSAGE_DAB:
		return "DAB";
	case MESSAGE_END:
		return "END";
	case MESSAGE_STS:
		return "STS";
	case MESSAGE_TCT:
		return "TCT";
	case MESSAGE_IDY:
		return "IDY";
	case MESSAGE_PCG:
		return "PCG";
	case MESSAGE_REN:
		return "REN";
	}
	return "UNKNOWN";
}



static void post_message(GPIBIO s, message_t message, int value)
{

	gpib_debug(s, 2, "%s, value %x ifc %d\n",
		   message_to_string(message), value, s->ifc);
	switch(message) {
	case MESSAGE_PON:

		s->sic = 0;
		s->ren = 0;
		s->rsc = 0;
		s->lpas = 0;
		s->tpas = 0;
		s->mjmn = 0;

		s->isr1 = 0;
		s->isr2 = 0;

		s->imr1 = 0;
		s->imr2 = 0;
#if 0
		// FIXME
		s->adr0 = 0;
		s->adr1 = 0;
#endif
		s->eos = 0;
		s->icr = 0;
		s->ppr = 0;
		s->auxra = 0;
		s->auxrb = 0;
		s->auxre = 0;
		s->admr = 0;
		s->spsr = 0;
		s->adsr = 0x40;
		s->send_eoi = 0;
//#endif

		update_talker_state(s, STATE_TIDS);
		update_listener_state(s, STATE_LIDS);
		update_remote_local_function_state(s, STATE_LOCS);
		update_source_handshake_function_state(s, STATE_SIDS);
		update_ah_state(s, STATE_ACDS);
		update_sr_state(s, STATE_NPRS);
		update_controller_state(s, STATE_CIDS);
		update_sre_state(s, STATE_SRIS);
		update_sic_state(s, STATE_SIIS);
		update_srq_state(s, STATE_CSNS);
		update_dc_state(s, STATE_DCIS);
		break;

	case MESSAGE_TCA:
		if (s->controller_state == STATE_CSBS)
			update_controller_state(s, STATE_CACS);
		break;

	case MESSAGE_TON:
		if (!s->ifc && value && s->ah_state == STATE_ACDS)
			update_talker_state(s, STATE_TADS);
		break;

	case MESSAGE_LON:
		if (!s->ifc && value)
			update_listener_state(s, STATE_LADS);
		break;

	case MESSAGE_GTS:
		update_controller_state(s, STATE_CSBS);
		break;

	case MESSAGE_LTN:
		break;
	case MESSAGE_LUN:
		break;

	case MESSAGE_REN:
		s->ren = value ? 1 : 0;
		break;
	case MESSAGE_RSC:
		s->rsc = value ? 1 : 0;
		break;
	case MESSAGE_SIC:
		s->sic = value ? 1 : 0;
		break;
	default:
		break;
	}
	talker_function(s);
	listener_function(s);
	sh_function(s);
	ah_function(s);
	sr_function(s);
	rl_function(s);
	sic_function(s);
	sre_function(s);
	rsc_function(s);
	dc_function(s);
	controller_function(s);
}

static int gpib_printf(GPIBIO s, int dev, const char *fmt, ...)
{
	va_list va;
	char buf[512];

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);
	gpib_debug(s, 2, "DEV %d, %s\n", dev, buf);
	ibwrt(dev, buf, strlen(buf));
	gpib_debug(s, 1, "%s: %s\n", __FUNCTION__, buf);
	if(ibsta & ERR) {
		gpib_debug(s, 2, "SEND failed: %04X\n", ibsta);
		s->isr1 |= 0x04;
	}
	return 0;
}

static void gpib_map_commands(GPIBIO s, int address,
			      char *command, int len)
{
	int dev = s->gpib_dev[address];
	float freq, level;
	char unit;
	int channel;
	char pol;
	int range;

	gpib_debug(s, 1, "listener: %d, dev %d\n", address, dev);

	switch(address) {
	case 2:
	case 3:
		/* FG */
		if (!strncmp(command, "AE0,AI0,FI0,FE0\n", len)) {
			gpib_printf(s, dev, "*RST\n");
			gpib_printf(s, dev, ":OUTP OFF\n");
			gpib_printf(s, dev, ":SOURCE:CORRECTION ON\n");
			gpib_printf(s, dev, ":SOURCE:POWER:OFFSET 2.3DB\n");
			break;
		}

		if (!strncmp(command, "RO0\n", len)) {
			gpib_printf(s, dev, ":OUTP OFF\n");
			break;
		}

		if (!strncmp(command, "RO1\n", len)) {
			gpib_printf(s, dev, ":OUTP ON\n");
			break;
		}

		if (sscanf(command, "FR%fMZ,AP%fV", &freq, &level) == 2) {
			gpib_printf(s, dev, ":SOURCE:FREQUENCY %fE6\n", freq);
			if (level > -144)
				gpib_printf(s, dev, ":SOURCE:POWER %fV\n", level);
			break;
		}

		if (sscanf(command, "AP%f%c", &level, &unit) == 2) {
			if (unit == 'V')
				gpib_printf(s, dev, ":SOURCE:POWER %fV\n", level);
			if (unit == 'D')
				gpib_printf(s, dev, ":SOURCE:POWER %fDB\n", level);
			break;
		}

//		abort();
		break;
	case 16:
		/* DMM */
		if (!strncasecmp(command, "F1", 2)) {
			/* VDC */
			gpib_printf(s, dev, "VD\n");
			break;
		}

		if (!strncasecmp(command, "F2", 2)) {
			/* VAC */
			gpib_printf(s, dev, "VA\n");
			break;
		}

		if (!strncasecmp(command, "S1", 2)) {
			/* Reading Rate Medium */
			gpib_printf(s, dev, "T2\n");
			break;
		}

		if (!strncasecmp(command, "S0", 2)) {
			/* Reading Rate Medium */
			gpib_printf(s, dev, "T1\n");
			break;
		}

		if (!strncasecmp(command, "S1", 2)) {
			/* Reading Rate Medium */
			gpib_printf(s, dev, "T3\n");
			break;
		}

		if (!strncasecmp(command, "S2", 2)) {
			/* Reading Rate Medium */
			gpib_printf(s, dev, "T4\n");
			break;
		}

		if (!strncasecmp(command, "R0", 2)) {
			/* Reading Rate Medium */
			gpib_printf(s, dev, "A1\n");
			break;
		}

		if (!strncasecmp(command, "*", 1)) {
			/* Reset */
			gpib_printf(s, dev, "VDA1Q1\n");
			break;
		}

		if (strchr(command, '?')) {
			/* trigger measurement */
			gpib_printf(s, dev, "S1Q1\n");
			break;
		}

		if (!strncasecmp(command, "N16P1", 5)) {
			break;
		}

		if (!strncasecmp(command, "N0P1", 4)) {
			break;
		}
		break;
	case 19:
		/* CALIB */
		if (sscanf(command, "V%d%c%f\n", &range, &pol, &level) == 3) {

			if (range == 0)
				level /= 1000000;
			if (range == 1)
				level /= 100000;
			if (range == 2)
				level /= 10000;
			if (range == 3)
				level /= 1000;
			if (pol == '-')
				level *= -1;
			gpib_printf(s, dev, "OUTPUT:SET %fV\n", level);
			sleep(1);
			gpib_printf(s, dev, "OUTPUT:STATE ON\n");
		} if (sscanf(command, "A%c%f\n", &pol, &level) == 2) {
			level /= 1000;
			if (pol == '-')
				level *= -1;

			gpib_printf(s, dev, "OUTPUT:SET %fmA\n", level);
			sleep(1);
			gpib_printf(s, dev, "OUTPUT:STATE ON\n");
		}
		break;
	case 23:
		/* Switch 1 */
		/* ignore RQS command */
		if (!strncmp(command, "RQS", 3))
			break;

		if (sscanf(command, "OP %d\n", &channel) == 1) {
			gpib_printf(s, dev, "OPEN (@%d)\n", channel);
			break;
		}

		if (sscanf(command, "CL %d\n", &channel) == 1) {
			gpib_printf(s, dev, "CLOSE (@%d)\n", channel);
			break;
		}
	case 24:
		/* Switch 2 */
		/* ignore RQS command */
		if (!strncmp(command, "RQS", 3))
			break;

		if (sscanf(command, "OP %d\n", &channel) == 1) {
			gpib_printf(s, dev, "OPEN (@%d)\n", channel);
			break;
		}

		if (sscanf(command, "CL %d\n", &channel) == 1) {
			gpib_printf(s, dev, "CLOS (@%d)\n", channel);
			break;
		}

	default:
		/* DUT */
		ibwrt(dev, command, len);
		if (ibsta & ERR) {
			gpib_debug(s, 0, "ibwrt failed: ibsta %04x\n", ibsta);
			s->isr1 |= 0x04;
		}
		break;
	}
}

static void gpib_send_write_buffer(GPIBIO s)
{
	gpib_debug(s, 1, "%s: dev %d, %d bytes [%.*s]\n", __FUNCTION__, s->cur_listener,
		   s->write_buffer_used, s->write_buffer_used, s->write_buffer);
	if (s->cur_listener >= 0 && s->gpib_dev[s->cur_listener] >= 0) {
		gpib_map_commands(s, s->cur_listener, s->write_buffer, s->write_buffer_used);
	} else {
		gpib_debug(s, 0, "%s: device %d not opened\n", __FUNCTION__, s->cur_listener);
		s->isr1 |= 0x04;
	}
	s->write_buffer_used = 0;
}

static const char *gpib_command_name(uint8_t byte, char *buf)
{
	if (byte >= 0x20 && byte <= 0x3e) {
		sprintf(buf, "MLA%d", byte - 0x20);
		return buf;
	}

	if (byte >= 0x40 && byte <= 0x5e) {
		sprintf(buf, "MTA%d", byte - 0x40);
		return buf;
	}

	switch(byte) {
	case 0x01:
		return "GTL";
	case 0x04:
		return "SDC";
	case 0x05:
		return "PPC";
	case 0x08:
		return "GET";
	case 0x11:
		return "LLO";
	case 0x14:
		return "DCL";
	case 0x15:
		return "PPU";
	case 0x18:
		return "SPE";
	case 0x19:
		return "SPD";
	case 0x3f:
		return "UNL";
	case 0x5f:
		return "UNT";
	case 0x09:
		return "TCT";
	default:
		return "UNKNOWN";
	}
}

static void check_listener_addresses(GPIBIO s, uint8_t address)
{
	int dev;
	gpib_debug(s, 2, "address %02x ADR0 %02X ADR1 %02X\n",
		   address, s->adr0, s->adr1);

	if (address == 0x1f && s->listener_state == STATE_LADS) {
		update_listener_state(s, STATE_LIDS);
		s->lpas = 0;
		s->adsc |= 1;
	}

	if (address == 0x1f)
		return;

	if (address == (s->adr1 & 0x3f)) {
		gpib_debug(s, 2, "MLA command: %02x ADR1 %02x\n", address, s->adr1);
		update_listener_state(s, STATE_LADS);
		s->adsc |= 1;
		s->lpas = 1;
		s->mjmn = 1;
	}

	if (address == (s->adr0 & 0x3f)) {
		gpib_debug(s, 2, "MLA command: %02x ADR0 %02x\n", address, s->adr0);
		update_listener_state(s, STATE_LADS);
		s->adsc |= 1;
		s->lpas = 1;
		s->mjmn = 0;
	}

	s->cur_listener = address;

	if (address != (s->adr0 & 0x1f) && s->gpib_dev[address] == -1) {
		gpib_debug(s, 1, "trying to open address %d\n", address);
		switch(address) {
		case 23:
			dev = ibdev(0, 9, 0x61, T1000s, 1, 0);
			gpib_debug(s, 1, "opened address %d, dev %d, ibsta %04x\n",
				   address, dev, ibsta);
			break;
		case 24:
			dev = ibdev(0, 9, 0x62, T1000s, 1, 0);
			gpib_debug(s, 1, "opened address %d, dev %d, ibsta %04x\n",
				   address, dev, ibsta);

			break;
		default:
			dev = ibdev(0, address, 0, T1000s, 1, 0);
			break;
		}
		if (ibsta & ERR) {
			gpib_debug(s, 2, "dev %d: initialization failed: %04X\n", address, ibsta);
		} else {
			gpib_debug(s, 2, "opened dev %d, address %d, ibsta %04X\n",
				   dev, address, ibsta);
			s->gpib_dev[address] = dev;
		}
	}

}

static void check_talker_addresses(GPIBIO s, uint8_t address)
{
	int dev;
	gpib_debug(s, 2, "address %02x\n", address);

	if (address == 0x1f && s->talker_state == STATE_TADS) {
		update_talker_state(s, STATE_TIDS);
		s->cur_listener = -1;
		s->tpas = 0;
		s->adsc |= 1;
	}

	if (address == 0x1f)
		return;


	if (address == (s->adr1 & 0x1f) && !(s->adr1 & 0x40)) {

		gpib_debug(s, 2, "MLA command: %02x ADR1 %02x\n", address, s->adr1);
		update_talker_state(s, STATE_TADS);
		s->adsc |= 1;
		s->tpas = 1;
		s->mjmn = 1;
	}

	if (address == (s->adr0 & 0x1f) && !(s->adr0 & 0x40)) {
		gpib_debug(s, 2, "MLA command: %02x ADR0 %02x\n", address, s->adr0);
		update_talker_state(s, STATE_TADS);
		s->cur_listener = address;
		s->adsc |= 1;
		s->tpas = 1;
		s->mjmn = 0;
	}

	s->cur_talker = address;

	if (address != (s->adr0 & 0x5f) && s->gpib_dev[address] == -1) {
		gpib_debug(s, 2, "trying to open address %d\n", address);
		switch(address) {
		case 23:
			dev = ibdev(0, 9, 0x61, T1000s, 1, 0);
			gpib_debug(s, 1, "opened address %d, dev %d, ibsta %04x\n",
				   address, dev, ibsta);
			break;
		case 24:
			dev = ibdev(0, 9, 0x62, T1000s, 1, 0);
			gpib_debug(s, 1, "opened address %d, dev %d, ibsta %04x\n",
				   address, dev, ibsta);

			break;
		default:
			dev = ibdev(0, address, 0, T1000s, 1, 0);
			break;
		}

		if (ibsta & ERR) {
			gpib_debug(s, 0, "initialization failed: %04X\n", ibsta);
		} else {
			gpib_debug(s, 1,  "opened dev %d, address %d\n",
				   dev, address);
			s->gpib_dev[address] = dev;
		}
	}

}

static REG8 IOINPCALL gpibio_i99(UINT port)
{
	return np2cfg.gpibcfg;
}

static REG8 IOINPCALL gpibio_i9b(UINT port)
{
	return (np2cfg.gpibcfg & ~0x80) | 0x80;
}

static REG8 IOINPCALL gpibio_ic1(UINT port)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 3, "READ DIR: 0x%02x\n", s->dir);
	return s->dir;
}

static REG8 IOINPCALL gpibio_ic3(UINT port)
{
	GPIBIO s = &gpibio;
	if (s->talker_state == STATE_TACS)
		s->isr1 |= 0x02;
	else
		s->isr1 &= ~0x02;

	gpib_debug(s, 2, "ISR1 READ 0x%02x\n", s->isr1);

	return s->isr1;
}

static REG8 IOINPCALL gpibio_ic5(UINT port)
{
	GPIBIO s = &gpibio;
	uint8_t ret = 0;

	if (s->controller_state == STATE_CACS)
		ret |= 0x08;

	ret |= s->adsc;
	s->adsc = 0;
	if (s->isr2 & 0x40) {
		ret |= 0x40;
		s->isr2 &= ~0x40;
	}
	gpib_debug(s, 2, "%s(%04X): ISR2 READ: 0x%02x\n", __FUNCTION__, port, ret);
	return ret;
}

static REG8 IOINPCALL gpibio_ic7(UINT port)
{

	GPIBIO s = &gpibio;
	uint8_t ret = 0;
	gpib_debug(s, 2, "%s: %04X\n", __FUNCTION__, port);

	if (s->page_in)
		ret = 0x80; /* Version */
	else
		ret = s->spsr;

	gpib_debug(s, 2, "READ %s: 0x%02x\n", s->page_in ? "VSR" : "SPSR", ret);
	s->page_in = 0;
	return ret;
}

static REG8 IOINPCALL gpibio_ic9(UINT port)
{
	GPIBIO s = &gpibio;
	uint8_t adsr = 0;

	if (s->controller_state != STATE_CIDS &&
	    s->controller_state != STATE_CADS)
		adsr |= 0x80;

	if (!s->atn)
		s->adsr |= 0x40;
#if 0
	if (s->spms)
		s->adsr = 0x20;
#endif
	if (s->lpas)
		s->adsr |= 0x10;

	if (s->tpas)
		s->adsr |= 0x08;

	if (s->listener_state == STATE_LACS ||
	    s->listener_state == STATE_LADS)
		s->adsr |= 0x04;

	if (s->talker_state == STATE_TACS ||
	    s->talker_state == STATE_TADS)
		s->adsr |= 0x02;

	if (s->mjmn)
		s->adsr |= 0x01;

	gpib_debug(s, 2, "0x%02x\n", adsr);

	return s->adsr;
}

static REG8 IOINPCALL gpibio_icb(UINT port)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "%s: 0x%02x\n", s->page_in ? "SASR" : "CPTR", 0);
	s->page_in = 0;
	return 0;
}

static REG8 IOINPCALL gpibio_icd(UINT port)
{
	GPIBIO s = &gpibio;
	uint8_t ret = 0;
	if (s->page_in) {
		ret = 0;
	} else {
		ret = s->adr0;
	}
	s->page_in = 0;
	gpib_debug(s, 2, "ADR0 READ: %x\n", ret);
	return ret;
}

static REG8 IOINPCALL gpibio_icf(UINT port)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "READ: %s 0x%02x\n", s->page_in ? "BSR" : "ADR1", s->adr1);
	s->page_in = 0;
	return s->adr1;
}

static void IOOUTCALL gpibio_oc1(UINT port, REG8 byte)
{
	GPIBIO s = &gpibio;
	char buf[16];

	if (s->atn) {
		gpib_debug(s, 2, "COMMAND 0x%02X (%s)\n", byte, gpib_command_name(byte, buf));
		if (byte == 0x09) {
			/* TCT */
			update_controller_state(s, STATE_CIDS);
			s->adsc = 1;
		}

		if (byte == 0x18) {
			/* SPE */
			s->spe = 1;
		}

		if (byte == 0x19) {
			/* SPD */
			s->spe = 0;
		}


		if ((byte & 0xe0) == 0x20) {
			check_listener_addresses(s, byte & 0x1f);
			return;
		}

		if ((byte & 0xe0) == 0x40) {
			check_talker_addresses(s, byte & 0x1f);
			return;
		}
	} else {
		gpib_debug(s, 2, "WRITE CDOR: 0x%02x\n", byte);

		if (s->write_buffer_used == sizeof(s->write_buffer))
			return;

		s->write_buffer[s->write_buffer_used++] = byte;

		if (s->send_eoi) {
			gpib_send_write_buffer(s);
			s->send_eoi = 0;
		}
	}
}

static void IOOUTCALL gpibio_oc3(UINT port, REG8 byte)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "IMR1 WRITE 0x%02x\n", byte);
	s->imr1 = byte;
}

static void IOOUTCALL gpibio_oc5(UINT port, REG8 byte)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "%s(%04X): WRITE IMR2 %02X\n", __FUNCTION__, port, byte);
	s->imr2 = byte;
	if (s->imr2 & (GPIB_IMR2_DMAO|GPIB_IMR2_DMAI)) {
		DMA_hold_DREQ(s->isadma);
		DMA_schedule(s->isadma);
	}
}

static void IOOUTCALL gpibio_oc7(UINT port, REG8 byte)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "%s: WRITE %s(%04X): 0x%02x\n", __FUNCTION__,
		s->page_in ? "ICR2" : "SPMR", port, byte);

	if (!s->page_in) {
		s->spsr = byte;
		if (byte & 0x40 && s->controller_state == STATE_CSBS)
			s->isr2 |= 0x40;
	}
	s->page_in = 0;
}

static void IOOUTCALL gpibio_oc9(UINT port, REG8 byte)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "WRITE ADMR: 0x%02x\n", byte);

	post_message(s, MESSAGE_TON, byte & 0x80);
	post_message(s, MESSAGE_LON, byte & 0x40);
	s->admr = byte;
}

static void IOOUTCALL gpibio_ocb(UINT port, REG8 byte)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "WRITE AUXMR: 0x%02x \n", byte);
	if (!(byte & 0xe0)) {
		switch(byte & 0x1f) {
		case 0:
			gpib_debug(s, 2, "  Immediate Execute Pon\n");
			post_message(s, MESSAGE_PON, 1);
			break;
		case 1:
			gpib_debug(s, 2, "  Clear parallel poll Flag\n");
			break;
		case 2:
			gpib_debug(s, 2, "  Chip Reset\n");
			post_message(s, MESSAGE_PON, 1);
			break;
		case 3:
			gpib_debug(s, 2, "  Finish Handshake\n");
			break;
		case 4:
			gpib_debug(s, 2, "  Trigger\n");
			break;
		case 5:
			gpib_debug(s, 2, "  Return To Local\n");
			post_message(s, MESSAGE_RTL, 1);
			break;
		case 6:
			gpib_debug(s, 2, "  Send EOI\n");
			s->send_eoi = 1;
			break;
		case 7:
			gpib_debug(s, 2, "  Non-Valid Secondary Command or Address\n");
			break;
		case 9:
			gpib_debug(s, 2, "  Set parallel poll Flag\n");
			post_message(s, MESSAGE_RPP, 1);
			break;
		case 0x0f:
			gpib_debug(s, 2, "  Valid Secondary Command or Address\n");
			break;
		case 0x10:
			gpib_debug(s, 2, "  Go To Standby\n");
			post_message(s, MESSAGE_GTS, 1);
			break;
		case 0x11:
			gpib_debug(s, 2, "  Take Control Async\n");
			post_message(s, MESSAGE_TCA, 0);
			break;
		case 0x12:
			gpib_debug(s, 2, "  Take control Sync\n");
			post_message(s, MESSAGE_TCS, 0);
			break;
		case 0x13:
			gpib_debug(s, 2, "  Listen\n");
			post_message(s, MESSAGE_LTN, 1);
			break;
		case 0x14:
			gpib_debug(s, 2, "  Disable System Control\n");
			break;
		case 0x16:
			gpib_debug(s, 2, "  Clear IFC\n");
			post_message(s, MESSAGE_RSC, 1);
			post_message(s, MESSAGE_SIC, 0);
			break;
		case 0x17:
			gpib_debug(s, 2, "  Clear REN\n");
			post_message(s, MESSAGE_REN, 0);
			break;
		case 0x1a:
			gpib_debug(s, 2, "  Take control Sync on End\n");
			break;
		case 0x1b:
			gpib_debug(s, 2, "  Listen in Continous Mode\n");
			break;
		case 0x1c:
			gpib_debug(s, 2, "  Local Unlisten\n");
			post_message(s, MESSAGE_UNL, 1);
			break;
		case 0x1d:
			gpib_debug(s, 2, "  Execute Parallel Poll\n");
			break;
		case 0x1e:
			gpib_debug(s, 2, "  Set IFC\n");
			post_message(s, MESSAGE_RSC, 1);
			post_message(s, MESSAGE_SIC, 1);
			break;
		case 0x1f:
			gpib_debug(s, 2, "  Set REN\n");
			post_message(s, MESSAGE_REN, 1);
			break;
		}
	} else {
		switch((byte >> 5) & 0x07) {
		case 1:
			gpib_debug(s, 2, "  ICR\n");
			s->icr = (byte & 0x1f);
			break;
		case 2:
			gpib_debug(s, 2, "  PAGE IN\n");
			s->page_in = 1;
			break;
		case 3:
			gpib_debug(s, 2, "  PPR\n");
			s->ppr = (byte & 0x1f);
			break;
		case 4:
			gpib_debug(s, 2, "  AUXRA\n");
			s->auxra = (byte & 0x1f);
			break;
		case 5:
			gpib_debug(s, 2, "  AUXRB\n");
			s->auxrb = (byte & 0x1f);
			break;
		case 6:
			gpib_debug(s, 2, "  AUXRE\n");
			s->auxre = (byte & 0x1f);
			break;
		case 7:
			gpib_debug(s, 2, "  AUR\n");
			break;
		default:
			gpib_debug(s, 2, "  UNKNOWN %02X\n", byte >> 5);
			break;
		}
	}
}

static void IOOUTCALL gpibio_ocd(UINT port, REG8 byte)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "WRITE %s: 0x%02x\n", s->page_in ? "IMR0" : "ADR", byte);

	if (!s->page_in) {
		if (byte & 0x80)
			s->adr1 = byte & 0x7f;
		else
			s->adr0 = byte & 0x7f;
	}
	s->page_in = 0;
}

static void IOOUTCALL gpibio_ocf(UINT port, REG8 byte)
{
	GPIBIO s = &gpibio;
	gpib_debug(s, 2, "WRITE %s: 0x%02x\n", s->page_in ? "BCR" : "EOSR", byte);
	if (!s->page_in)
		s->eos = byte;
	s->page_in = 0;
}


// ----

void gpibio_initialize(void) {

//	cm_pc9861ch1 = NULL;
//	cm_pc9861ch2 = NULL;
}

void gpibio_deinitialize(void) {

//	commng_destroy(cm_pc9861ch1);
//	cm_pc9861ch1 = NULL;
//	commng_destroy(cm_pc9861ch2);
//	cm_pc9861ch2 = NULL;
}

void gpibio_reset(const NP2CFG *pConfig) {

	ZeroMemory(&gpibio, sizeof(_GPIBIO));

	(void)pConfig;
}

void gpibio_bind(void) {

	iocore_attachout(0x00c1, gpibio_oc1);
	iocore_attachout(0x00c3, gpibio_oc3);
	iocore_attachout(0x00c5, gpibio_oc5);
	iocore_attachout(0x00c7, gpibio_oc7);
	iocore_attachout(0x00cb, gpibio_ocb);
	iocore_attachout(0x00cd, gpibio_ocd);
	iocore_attachout(0x00cf, gpibio_ocf);
	iocore_attachinp(0x0099, gpibio_i99);
	iocore_attachinp(0x009b, gpibio_i9b);
	iocore_attachinp(0x00c1, gpibio_ic1);
	iocore_attachinp(0x00c3, gpibio_ic3);
	iocore_attachinp(0x00c5, gpibio_ic5);
	iocore_attachinp(0x00c7, gpibio_ic7);
	iocore_attachinp(0x00c9, gpibio_ic9);
	iocore_attachinp(0x00cb, gpibio_icb);
	iocore_attachinp(0x00cd, gpibio_icd);
	iocore_attachinp(0x00cf, gpibio_icf);
}
#endif

