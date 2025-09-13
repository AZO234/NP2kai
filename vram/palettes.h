// palette
//	 00		text palette				(NP2PAL_TEXT: pal0s + text)
//	+0A		skipline palette			(NP2PAL_SKIP: skiplines)
//	+10		grph palette				(NP2PAL_GRPH: grph only)
//	+80		text+grph					(NP2PAL_TEXT2:grph+text)
//	+0A		black + text palette		(NP2PAL_TEXT3: text/grph=black)

enum {
	NP2PALS_TXT		= 10,
	NP2PALS_GRPH	= 16,

	NP2PAL_TEXT		= 0,
	NP2PAL_SKIP		= (NP2PAL_TEXT + NP2PALS_TXT),
	NP2PAL_GRPH		= (NP2PAL_SKIP + NP2PALS_GRPH),
	NP2PAL_TEXT2	= (NP2PAL_GRPH + NP2PALS_GRPH),
	NP2PAL_TEXT3	= (NP2PAL_TEXT2 + (8 * NP2PALS_GRPH)),
	NP2PAL_NORMAL	= (NP2PAL_TEXT3 + NP2PALS_TXT),

#ifdef SUPPORT_PC9821
	NP2PAL_TEXTEX	= NP2PAL_NORMAL,
	NP2PAL_TEXTEX3	= (NP2PAL_TEXTEX + NP2PALS_TXT),
	NP2PAL_GRPHEX	= (NP2PAL_TEXTEX3 + NP2PALS_TXT),
	NP2PAL_EXTEND	= (NP2PAL_GRPHEX + 256),
#endif		/* SUPPORT_PC9821 */

#ifdef SUPPORT_PC9821
	NP2PAL_MAX		= NP2PAL_EXTEND
#else		/* SUPPORT_PC9821 */
	NP2PAL_MAX		= NP2PAL_NORMAL
#endif		/* SUPPORT_PC9821 */
};

#define	PALEVENTMAX		1024

typedef struct {
	SINT32	clock;
	UINT16	color;
	UINT8	value;
	UINT8	reserve;
} PAL1EVENT;

typedef struct {
	UINT16		anabit;
	UINT16		degbit;
	RGB32		pal[16];
	UINT		vsyncpal;
	UINT		events;
	PAL1EVENT	event[PALEVENTMAX];
} PALEVENT;


#ifdef __cplusplus
extern "C" {
#endif

extern	RGB32		np2_pal32[NP2PAL_MAX];
#if defined(SUPPORT_16BPP)
extern	RGB16		np2_pal16[NP2PAL_MAX];
#endif
extern	PALEVENT	palevent;
extern	UINT8		pal_monotable[16];

void pal_makegrad(RGB32 *pal, int pals, UINT32 bg, UINT32 fg);

void pal_initlcdtable(void);
void pal_makelcdpal(void);
void pal_makeskiptable(void);
void pal_change(UINT8 textpalset);

void pal_eventclear(void);

void pal_makeanalog(RGB32 *pal, UINT16 bit);
void pal_makeanalog_lcd(RGB32 *pal, UINT16 bit);

#ifdef __cplusplus
}
#endif

