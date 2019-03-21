
#ifdef __cplusplus
extern "C" {
#endif

void timing_reset(void);
void timing_setrate(UINT lines, UINT crthz);
void timing_setcount(UINT value);
UINT timing_getcount(void);
UINT timing_getcount_baseclock(void);

#ifdef __cplusplus
}
#endif

