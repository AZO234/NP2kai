
typedef struct {
	_SYSTIME	dt;
	_SYSTIME	realc;
	UINT		steps;
	UINT		realchg;
} _CALENDAR, *CALENDAR;


#ifdef __cplusplus
extern "C" {
#endif

extern	_CALENDAR	cal;

void calendar_initialize(void);
void calendar_inc(void);
void calendar_set(const UINT8 *bcd);
void calendar_get(UINT8 *bcd);
void calendar_getreal(UINT8 *bcd);
void calendar_getvir(UINT8 *bcd);

#ifdef __cplusplus
}
#endif

