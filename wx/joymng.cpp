/* === joystick management for wx port (SDL3) === */

#include <compiler.h>
#include "joymng.h"
#include "np2.h"

#if defined(SUPPORT_JOYSTICK)

static SDL_Joystick  *s_joystick[2]  = { NULL, NULL };
static REG8           s_joystat      = 0xff;

REG8 joymng_available(void)
{
	return (s_joystick[0] != NULL) ? 1 : 0;
}

REG8 joymng_getstat(void)
{
	return s_joystat;
}

void joymng_initialize(void)
{
	/* Disable modern HIDAPI which requires udev, force classic linux /dev/input/js* */
	SDL_SetHint("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1");
	SDL_SetHint("SDL_JOYSTICK_HIDAPI", "0");
	SDL_SetHint("SDL_JOYSTICK_LINUX_CLASSIC", "1");

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
		return;
	}

	int count = 0;
	SDL_JoystickID *ids = SDL_GetJoysticks(&count);
	if (ids && count > 0) {
		int target = 0;
		/* prioritize device name in config */
		if (np2oscfg.JOYDEV[0][0]) {
			for (int i = 0; i < count; i++) {
				const char *name = SDL_GetJoystickNameForID(ids[i]);
				if (name && strcmp(name, np2oscfg.JOYDEV[0]) == 0) {
					target = i;
					break;
				}
			}
		}
		s_joystick[0] = SDL_OpenJoystick(ids[target]);
		if (count > 1) {
			int second = (target == 0) ? 1 : 0;
			s_joystick[1] = SDL_OpenJoystick(ids[second]);
		}
		SDL_free(ids);
	}
}

void joymng_deinitialize(void)
{
	int i;
	for (i = 0; i < 2; i++) {
		if (s_joystick[i]) {
			SDL_CloseJoystick(s_joystick[i]);
			s_joystick[i] = NULL;
		}
	}
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

void joymng_sync(void)
{
	REG8 stat = 0xff;
	if (!s_joystick[0]) {
		s_joystat = 0xff;
		return;
	}
	SDL_UpdateJoysticks();

	/* axes → direction */
	int ax = SDL_GetJoystickAxis(s_joystick[0], 0);
	int ay = SDL_GetJoystickAxis(s_joystick[0], 1);
	const int DEAD = 8000;
	if (ay < -DEAD) stat &= ~JOY_UP_BIT;
	if (ay >  DEAD) stat &= ~JOY_DOWN_BIT;
	if (ax < -DEAD) stat &= ~JOY_LEFT_BIT;
	if (ax >  DEAD) stat &= ~JOY_RIGHT_BIT;

	/* check POV hats for D-pad if present */
	int hat_count = SDL_GetNumJoystickHats(s_joystick[0]);
	if (hat_count > 0) {
		Uint8 hat = SDL_GetJoystickHat(s_joystick[0], 0);
		if (hat & SDL_HAT_UP)    stat &= ~JOY_UP_BIT;
		if (hat & SDL_HAT_DOWN)  stat &= ~JOY_DOWN_BIT;
		if (hat & SDL_HAT_LEFT)  stat &= ~JOY_LEFT_BIT;
		if (hat & SDL_HAT_RIGHT) stat &= ~JOY_RIGHT_BIT;
	}

	/* buttons (check first 4 buttons) */
	int btn_count = SDL_GetNumJoystickButtons(s_joystick[0]);
	if (btn_count > 0 && SDL_GetJoystickButton(s_joystick[0], 0)) stat &= ~JOY_BTN1_BIT;
	if (btn_count > 1 && SDL_GetJoystickButton(s_joystick[0], 1)) stat &= ~JOY_BTN2_BIT;

	s_joystat = stat;
}

joymng_devinfo_t **joymng_get_devinfo_list(void)
{
	static joymng_devinfo_t *list[16];
	static joymng_devinfo_t  infos[16];
	static char              names[16][256];

	/* ensure SDL's internal device list is updated */
	SDL_PumpEvents();
	SDL_UpdateJoysticks();

	int count = 0;
	SDL_JoystickID *ids = SDL_GetJoysticks(&count);

	if (!ids || count <= 0) {
		if (ids) SDL_free(ids);
		return NULL;
	}

	int n = (count < 15) ? count : 15;
	int i;
	for (i = 0; i < n; i++) {
		const char *name = SDL_GetJoystickNameForID(ids[i]);
		strncpy(names[i], name ? name : "Unknown Joystick", sizeof(names[i]));
		infos[i].devindex = i;
		infos[i].devname = names[i];
		list[i] = &infos[i];
	}
	list[i] = NULL;
	SDL_free(ids);
	return list;
}

#endif /* SUPPORT_JOYSTICK */
