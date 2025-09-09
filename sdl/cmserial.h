#ifndef	NP2_X_CMSERIAL_H__
#define	NP2_X_CMSERIAL_H__

// ---- com manager serial for unix

extern const UINT32 cmserial_speed[10];

COMMNG cmserial_create(UINT port, UINT8 param, UINT32 speed);

#if defined(SUPPORT_PC9861K)
#define	MAX_SERIAL_PORT_NUM	3
#else
#define	MAX_SERIAL_PORT_NUM	1
#endif

#endif	/* NP2_X_CMSERIAL_H__ */
