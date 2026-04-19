
#ifdef __cplusplus
extern "C" {
#endif

REG8 joymng_getstat(void);
REG8 joymng_available(void);
UINT32 joymng_getAnalogX(void);
UINT32 joymng_getAnalogY(void);

#ifdef __cplusplus
}
#endif


// ----

void joymng_initialize(void);
void joymng_sync(void);

