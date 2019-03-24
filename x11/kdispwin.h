#ifndef	NP2_X11_KDISPWIN_H__
#define	NP2_X11_KDISPWIN_H__

#include "keydisp.h"

#if defined(SUPPORT_KEYDISP)

G_BEGIN_DECLS

enum {
	KDISPCFG_FM	= 0x00,
	KDISPCFG_MIDI	= 0x80
};

typedef struct {
	int	posx;
	int	posy;
	UINT8	mode;
	UINT8	type;
} KDISPCFG;

extern KDISPCFG kdispcfg;

BRESULT kdispwin_initialize(void);
void kdispwin_create(void);
void kdispwin_destroy(void);
void kdispwin_draw(UINT8 cnt);
void kdispwin_readini(void);
void kdispwin_writeini(void);

G_END_DECLS

#else	/* !SUPPORT_KEYDISP */

#define	kdispwin_initialize()
#define	kdispwin_create()
#define	kdispwin_destroy()
#define	kdispwin_draw(cnt)
#define	kdispwin_readini()
#define	kdispwin_writeini()

#endif	/* SUPPORT_KEYDISP */

#endif	/* NP2_X11_KDISPWIN_H__ */
