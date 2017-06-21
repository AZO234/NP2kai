#ifndef	NP2_X11_SKBDWIN_H__
#define	NP2_X11_SKBDWIN_H__

#if defined(SUPPORT_SOFTKBD)

#include "softkbd.h"

G_BEGIN_DECLS

typedef struct {
	int	posx;
	int	posy;
	UINT8	type;
} SKBDCFG;

extern SKBDCFG skbdcfg;

BRESULT skbdwin_initialize(void);
void skbdwin_deinitialize(void);
void skbdwin_create(void);
void skbdwin_destroy(void);
void skbdwin_process(void);
void skbdwin_readini(void);
void skbdwin_writeini(void);

G_END_DECLS

#else	/* !SUPPORT_SOFTKBD */

#define	skbdwin_initialize()
#define	skbdwin_deinitialize()
#define	skbdwin_create()
#define	skbdwin_destroy()
#define	skbdwin_process()
#define	skbdwin_readini()
#define	skbdwin_writeini()

#endif	/* SUPPORT_SOFTKBD */

#endif	/* NP2_X11_SKBDWIN_H__ */
