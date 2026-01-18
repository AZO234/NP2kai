
#ifdef __cplusplus
extern "C" {
#endif

#define	TIMING_MSSHIFT			16
#define	TIMING_MSSHIFT_VALUE	(1 << TIMING_MSSHIFT)
#define	TIMING_MSSHIFT_MASK		(TIMING_MSSHIFT_VALUE - 1)

void timing_reset(void);
void timing_setrate(UINT lines, UINT crthz);
void timing_setcount(UINT value);
UINT32 timing_getmsstep(void);
UINT timing_getcount(void);
UINT timing_getcount_baseclock(void);
UINT32 timing_getcount_raw(void);
void timing_setspeed(UINT32 speed);

#ifdef __cplusplus
}
#endif
