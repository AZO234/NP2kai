#ifndef	NP2_X_TIMEMNG_H__
#define	NP2_X_TIMEMNG_H__

G_BEGIN_DECLS

// Win32 SYSTEMTIME 繧ｹ繝医Λ繧ｯ繝√Ε

typedef struct {
	UINT16	year;
	UINT16	month;
	UINT16	week;
	UINT16	day;
	UINT16	hour;
	UINT16	minute;
	UINT16	second;
	UINT16	milli;
} _SYSTIME;

BRESULT timemng_gettime(_SYSTIME *systime);

G_END_DECLS

#endif	/* NP2_X_TIMEMNG_H__ */
