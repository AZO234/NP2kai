#include	"compiler.h"
#include	"commng.h"
#include	"cpucore.h"
#include	"sound.h"
#include	"cmjasts.h"


#define	JSEVENTS	512

typedef struct {
	SINT32	clock;
	SINT32	pcm;
} JSEVT;

typedef struct {
	SINT32	pcm;
#if defined(JSEVENTS)
	SINT32	lastpcm;
	UINT	events;
	JSEVT	event[JSEVENTS];
#endif
} _CMJAST, *CMJAST;


static UINT jsread(COMMNG self, UINT8 *data) {

	(void)self;
	(void)data;
	return(0);
}

static UINT jswrite(COMMNG self, UINT8 data) {

	CMJAST	js;
	SINT32	pcm;

	js = (CMJAST)(self + 1);
	pcm = data << 5;
	js->pcm = pcm;
#if defined(JSEVENTS)
	if (js->events < JSEVENTS) {
		JSEVT *e;
		e = js->event + js->events;
		e->clock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK -
														soundcfg.lastclock;
		e->pcm = pcm;
		js->events++;
		if (js->events == JSEVENTS) {
			sound_sync();
		}
	}
#else
	sound_sync();
#endif
	return(1);
}

static UINT8 jsgetstat(COMMNG self) {

	(void)self;
	return(0);
}

static INTPTR jsmsg(COMMNG self, UINT msg, INTPTR param) {

	(void)self;
	(void)msg;
	(void)param;
	return(0);
}

static void jsrelease(COMMNG self) {

	_MFREE(self);
}

static void SOUNDCALL js_getpcm(CMJAST hdl, SINT32 *pcm, UINT count) {

	SINT32	pcmdata;

#if defined(JSEVENTS)
	UINT	pos;
	UINT	pterm;
	JSEVT	*e;
	JSEVT	*eterm;

	pos = 0;
	e = hdl->event;
	eterm = e + hdl->events;
	hdl->events = 0;
	pcmdata = hdl->lastpcm;
	hdl->lastpcm = hdl->pcm;
	while(e < eterm) {
		pterm = (e->clock * soundcfg.hzbase) / soundcfg.clockbase;
		if (pterm >= count) {
			break;
		}
		while(pos < pterm) {
			pos++;
			pcm[0] += pcmdata;
			pcm[1] += pcmdata;
			pcm += 2;
		}
		pcmdata = e->pcm;
		e++;
	}
	count -= pos;
	if (e >= eterm) {
		pcmdata = hdl->pcm;
	}
#else
	pcmdata = hdl->pcm;
#endif
	if (pcmdata) {
		while(count) {
			count--;
			pcm[0] += pcmdata;
			pcm[1] += pcmdata;
			pcm += 2;
		}
	}
}


COMMNG cmjasts_create(void) {

	COMMNG		ret;
	CMJAST		js;

	ret = (COMMNG)_MALLOC(sizeof(_COMMNG) + sizeof(_CMJAST), "JAST");
	if (ret == NULL) {
		goto cmjscre_err;
	}
	ret->connect = COMCONNECT_PARALLEL;
	ret->read = jsread;
	ret->write = jswrite;
	ret->getstat = jsgetstat;
	ret->msg = jsmsg;
	ret->release = jsrelease;
	js = (CMJAST)(ret + 1);
	ZeroMemory(js, sizeof(_CMJAST));
	sound_streamregist((void *)js, (SOUNDCB)js_getpcm);
	return(ret);

cmjscre_err:
	return(NULL);
}

