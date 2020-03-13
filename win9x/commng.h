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
	COMCONNECT_OFF			= 0,
	COMCONNECT_SERIAL,
	COMCONNECT_MIDI,
	COMCONNECT_PARALLEL,
#if defined(SUPPORT_WACOM_TABLET)
	COMCONNECT_TABLET,
#endif
#if defined(SUPPORT_NAMED_PIPE)
	COMCONNECT_PIPE,
#endif

};

enum {
	COMMSG_MIDIRESET		= 0,
	COMMSG_SETFLAG,
	COMMSG_GETFLAG,
	COMMSG_CHANGESPEED,
	COMMSG_CHANGEMODE,
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

COMMNG commng_create(UINT device);
void commng_destroy(COMMNG hdl);

#ifdef __cplusplus
}
#endif


// ---- com manager for windows

enum {
	COMPORT_NONE			= 0,
	COMPORT_COM1,
	COMPORT_COM2,
	COMPORT_COM3,
	COMPORT_COM4,
	COMPORT_MIDI,
#if defined(SUPPORT_WACOM_TABLET)
	COMPORT_TABLET,
#endif
#if defined(SUPPORT_NAMED_PIPE)
	COMPORT_PIPE,
#endif
};

enum {
	COMSIG_COM1				= 0x314d4f43,
	COMSIG_COM2				= 0x324d4f43,
	COMSIG_COM3				= 0x334d4f43,
	COMSIG_COM4				= 0x344d4f43,
	COMSIG_MIDI				= 0x4944494d,
	COMSIG_TABLET			= 0x5944494d // XXX: なぞ
};

enum {
	COMMSG_MIMPIDEFFILE		= COMMSG_USER,
	COMMSG_MIMPIDEFEN
};

void commng_initialize(void);
void commng_finalize(void);
