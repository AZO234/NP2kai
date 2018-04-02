
typedef struct {
	UINT8	head[4];
	UINT	nextevent;
	UINT8	curevent;
} _WABRLY, *WABRLY;


#ifdef __cplusplus
extern "C" {
#endif

void wabrly_initialize(void);
void wabrly_switch(void);


#if defined(SUPPORT_SWWABRLYSND)
void wabrlysnd_initialize(UINT rate);
void wabrlysnd_bind(void);
void wabrlysnd_deinitialize(void);
#else
#define	wabrlysnd_initialize(r)
#define	wabrlysnd_bind()
#define	wabrlysnd_deinitialize()
#endif

#ifdef __cplusplus
}
#endif

