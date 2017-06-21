#ifndef	NP2_X11_COMMNG_H__
#define	NP2_X11_COMMNG_H__

// ---- com manager interface

G_BEGIN_DECLS

enum {
	COMCREATE_SERIAL		= 0,
	COMCREATE_PC9861K1,
	COMCREATE_PC9861K2,
	COMCREATE_PRINTER,
	COMCREATE_MPU98II
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
	COMMSG_USER
};

struct _commng;
typedef struct _commng	_COMMNG;
typedef struct _commng	*COMMNG;

struct _commng {
	UINT	connect;
	UINT	(*read)(COMMNG self, UINT8 *data);
	UINT	(*write)(COMMNG self, UINT8 data);
	UINT8	(*getstat)(COMMNG self);
	INTPTR	(*msg)(COMMNG self, UINT msg, INTPTR param);
	void	(*release)(COMMNG self);
};

typedef struct {
	UINT32	size;
	UINT32	sig;
	UINT32	ver;
	UINT32	param;
} _COMFLAG, *COMFLAG;


COMMNG commng_create(UINT device);
void commng_destroy(COMMNG hdl);


// ---- com manager for unix

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

G_END_DECLS

#include "cmmidi.h"
#include "cmserial.h"
#include "cmpara.h"

#endif	/* NP2_X11_COMMNG_H__ */
