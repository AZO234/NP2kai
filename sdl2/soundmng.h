
enum {
	SOUND_PCMSEEK		= 0,
	SOUND_PCMSEEK1		= 1,

	SOUND_MAXPCM
};

enum {
	SNDDRV_NODRV,
	SNDDRV_SDL,
	SNDDRV_DRVMAX
};

#ifdef __cplusplus
extern "C" {
#endif

UINT8 snddrv_drv2num(const char *);
const char *snddrv_num2drv(UINT8);

UINT soundmng_create(UINT rate, UINT ms);
void soundmng_destroy(void);
#define soundmng_reset()
void soundmng_play(void);
void soundmng_stop(void);
#define soundmng_sync()
#define soundmng_setreverse(r)

#define	soundmng_pcmplay(a, b)
#define	soundmng_pcmstop(a)



// ---- for SDL

void soundmng_initialize(void);
void soundmng_deinitialize(void);

#ifdef __cplusplus
}
#endif
