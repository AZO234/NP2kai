/* === joystick management for wx port (SDL3) === */

#ifndef NP2_WX_JOYMNG_H
#define NP2_WX_JOYMNG_H

#include <compiler.h>

#define JOY_NAXIS        2
#define JOY_NBUTTON      4
#define JOY_AXIS_INVALID  0xff
#define JOY_BUTTON_INVALID 0xff
#define JOY_NAXIS_MAX    (JOY_AXIS_INVALID - 1)
#define JOY_NBUTTON_MAX  (JOY_BUTTON_INVALID - 1)

enum {
	JOY_UP_BIT      = (1 << 0),
	JOY_DOWN_BIT    = (1 << 1),
	JOY_LEFT_BIT    = (1 << 2),
	JOY_RIGHT_BIT   = (1 << 3),
	JOY_RAPIDBTN1_BIT = (1 << 4),
	JOY_RAPIDBTN2_BIT = (1 << 5),
	JOY_BTN1_BIT    = (1 << 6),
	JOY_BTN2_BIT    = (1 << 7)
};

typedef struct {
	int  devindex;
	char *devname;
	int  naxis;
	int  axis[JOY_NAXIS];
	int  nbutton;
	int  button[JOY_NBUTTON];
} joymng_devinfo_t;

#if defined(SUPPORT_JOYSTICK)

#ifdef __cplusplus
extern "C" {
#endif

REG8 joymng_getstat(void);
REG8 joymng_available(void);
void joymng_initialize(void);
void joymng_deinitialize(void);
void joymng_sync(void);
joymng_devinfo_t **joymng_get_devinfo_list(void);

#ifdef __cplusplus
}
#endif

#else /* !SUPPORT_JOYSTICK */

#define joymng_getstat()             (REG8)0xff
#define joymng_available()           (REG8)0
#define joymng_initialize()          (np2oscfg.JOYPAD1 |= 2)
#define joymng_deinitialize()        (np2oscfg.JOYPAD1 &= 1)
#define joymng_get_devinfo_list()    NULL
#define joymng_sync()

#endif /* SUPPORT_JOYSTICK */

#endif /* NP2_WX_JOYMNG_H */
