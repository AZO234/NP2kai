/* === serial communication manager for wx port === */

#ifndef NP2_WX_CMSERIAL_H__
#define NP2_WX_CMSERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern const UINT32 cmserial_speed[10];

COMMNG cmserial_create(UINT port, UINT8 param, UINT32 speed);

#if defined(SUPPORT_PC9861K)
#define	MAX_SERIAL_PORT_NUM	3
#else
#define	MAX_SERIAL_PORT_NUM	1
#endif

#ifdef __cplusplus
}
#endif

#endif /* NP2_WX_CMSERIAL_H__ */
