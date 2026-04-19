/**
 * @file	commng.h
 * @brief	COM マネージャの宣言およびインターフェイスの定義をします
 */

#pragma once

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
#if defined(SUPPORT_WACOM_TABLET)
	COMCREATE_TABLET,
#endif
#if defined(SUPPORT_NAMED_PIPE)
	COMCREATE_PIPE,
#endif
	COMCREATE_NULL			= 0xffff,
};

enum {
	COMCONNECT_OFF = 0,
	COMCONNECT_SERIAL = 1,
	COMCONNECT_MIDI = 2,
	COMCONNECT_PARALLEL = 3,
#if defined(SUPPORT_WACOM_TABLET)
	COMCONNECT_TABLET = 4,
#endif
#if defined(SUPPORT_NAMED_PIPE)
	COMCONNECT_PIPE = 5,
#endif
	COMCONNECT_FILE = 6,
	COMCONNECT_SPOOLER = 7,
};

enum {
	COMMSG_MIDIRESET		= 0,
	COMMSG_SETFLAG,
	COMMSG_GETFLAG,
	COMMSG_CHANGESPEED,
	COMMSG_CHANGEMODE,
	COMMSG_SETCOMMAND,
	COMMSG_PURGE,
	COMMSG_GETERROR,
	COMMSG_CLRERROR,
	COMMSG_REOPEN,
	COMMSG_USER			    = 0x80,
};

struct _commng;
typedef struct _commng	_COMMNG;		/*!< defines the instance of COMMNG */
typedef struct _commng	*COMMNG;		/*!< defines the instance of COMMNG */

/**
 * @brief COMMNG
 */
struct _commng
{
	UINT	connect;											/*!< flags */
	UINT	(*read)(COMMNG self, UINT8 *data);					/*!< read */
	UINT	(*write)(COMMNG self, UINT8 data);					/*!< write */
	UINT	(*writeretry)(COMMNG self);							/*!< write retry */
	void	(*beginblocktranster)(COMMNG self);					/*!< begin block transfer */
	void	(*endblocktranster)(COMMNG self);					/*!< end block transfer */
	UINT	(*lastwritesuccess)(COMMNG self);					/*!< last write success */
	UINT8	(*getstat)(COMMNG self);							/*!< get status */
	INTPTR	(*msg)(COMMNG self, UINT msg, INTPTR param);		/*!< message */
	void	(*release)(COMMNG self);							/*!< release */
};

typedef struct {
	UINT32	size;
	UINT32	sig;
	UINT32	ver;
	UINT32	param;
} _COMFLAG, *COMFLAG;


#ifdef __cplusplus
extern "C"
{
#endif

COMMNG commng_create(UINT device, BOOL onReset);
void commng_destroy(COMMNG hdl);

#ifdef __cplusplus
}
#endif


// ---- com manager for windows

enum {
	COMPORT_NONE = 0,
	COMPORT_COM1 = 1,
	COMPORT_COM2 = 2,
	COMPORT_COM3 = 3,
	COMPORT_COM4 = 4,
	COMPORT_MIDI = 5,
#if defined(SUPPORT_WACOM_TABLET)
	COMPORT_TABLET = 6,
#endif
#if defined(SUPPORT_NAMED_PIPE)
	COMPORT_PIPE = 7,
#endif
	COMPORT_LPT1 = 8,
	COMPORT_LPT2 = 9,
	COMPORT_LPT3 = 10,
	COMPORT_LPT4 = 11,
	COMPORT_FILE = 12,
	COMPORT_SPOOLER = 13,
};

enum {
	COMSIG_COM1				= 0x314d4f43,
	COMSIG_COM2				= 0x324d4f43,
	COMSIG_COM3				= 0x334d4f43,
	COMSIG_COM4				= 0x344d4f43,
	COMSIG_MIDI				= 0x4944494d,
	COMSIG_TABLET			= 0x5944494d,
	COMSIG_LPT1				= 0x614d4f43,
	COMSIG_LPT2				= 0x624d4f43,
	COMSIG_LPT3				= 0x634d4f43,
	COMSIG_LPT4				= 0x644d4f43,
	COMSIG_FILE				= 0x7944494d,
	COMSIG_SPOOLER			= 0x644d4f43,
};

enum {
	COMMSG_MIMPIDEFFILE		= COMMSG_USER,
	COMMSG_MIMPIDEFEN
};

void commng_initialize(void);
void commng_finalize(void);
