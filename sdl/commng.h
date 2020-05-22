#ifndef	NP2_COMMNG_H__
#define	NP2_COMMNG_H__

// ---- com manager interface

enum {
	COMCREATE_SERIAL		= 0,
	COMCREATE_PC9861K1,
	COMCREATE_PC9861K2,
	COMCREATE_PRINTER,
	COMCREATE_MPU98II,
#if defined(SUPPORT_SMPU98)
	COMCREATE_SMPU98_A,
	COMCREATE_SMPU98_B,
#endif
	COMCREATE_NULL			= 0xffff,
};

enum {
	COMCONNECT_OFF			= 0,
	COMCONNECT_SERIAL,
	COMCONNECT_MIDI,
	COMCONNECT_PARALLEL
};

enum {
	COMMSG_MIDIRESET		= 0,
	COMMSG_SETFLAG,
	COMMSG_GETFLAG,
#if defined(VAEG_FIX)
	COMMSG_SETRSFLAG,
#endif
	COMMSG_CHANGESPEED,
	COMMSG_CHANGEMODE,
	COMMSG_USER			    = 0x80,
};

struct _commng;
typedef struct _commng	_COMMNG;
typedef struct _commng	*COMMNG;

struct _commng {
	UINT	connect;
	UINT	(*read)(COMMNG self, UINT8 *data);
	UINT	(*write)(COMMNG self, UINT8 data);
	UINT	(*writeretry)(COMMNG self);
	UINT	(*lastwritesuccess)(COMMNG self);
	UINT8	(*getstat)(COMMNG self);
	INTPTR	(*msg)(COMMNG self, UINT msg, INTPTR param);
	void	(*release)(COMMNG self);
	UINT8 lastdata;
	UINT8 lastdatafail;
	UINT lastdatatime;
};

typedef struct {
	UINT32	size;
	UINT32	sig;
	UINT32	ver;
	UINT32	param;
} _COMFLAG, *COMFLAG;


#ifdef __cplusplus
extern "C" {
#endif

COMMNG commng_create(UINT device);
void commng_destroy(COMMNG hdl);

#ifdef __cplusplus
}
#endif


// ----

enum {
	COMPORT_NONE			= 0,
	COMPORT_COM1,
	COMPORT_COM2,
	COMPORT_COM3,
	COMPORT_COM4,
	COMPORT_MIDI
};

enum {
	COMSIG_COM1			= 0x314d4f43,
	COMSIG_COM2			= 0x324d4f43,
	COMSIG_COM3			= 0x334d4f43,
	COMSIG_COM4			= 0x344d4f43,
	COMSIG_MIDI			= 0x4944494d
};

enum {
	COMMSG_MIMPIDEFFILE		= COMMSG_USER,
	COMMSG_MIMPIDEFEN
};

void commng_initialize(void);

#include "cmmidi.h"
#include "cmserial.h"
#include "cmpara.h"

#endif	/* NP2_COMMNG_H__ */
