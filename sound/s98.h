/**
 * @file	s98.h
 * @brief	Interface of logging PC-98 sound
 */

#pragma once

enum {
	NORMAL2608	= 0,
	EXTEND2608	= 1
};


#ifdef __cplusplus
extern "C" {
#endif

#if !defined(SUPPORT_S98)		// ÉRÅ[ÉãÇ∑ÇÁñ ì|ÇæÅI

#define	S98_init()
#define	S98_trash()
#define	S98_open(f)			(FAILURE)
#define	S98_close()
#define	S98_put(m, a, d)
#define S98_sync()
#define S98_isopened()		(FALSE)

#else

void S98_init(void);
void S98_trash(void);
BRESULT S98_open(const OEMCHAR *filename);
void S98_close(void);
void S98_put(REG8 module, UINT addr, REG8 data);
void S98_sync(void);
BOOL S98_isopened(void);

#endif

#ifdef __cplusplus
}
#endif
