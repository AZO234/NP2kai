/* === joystick management for wx port (SDL3) === */

#include <compiler.h>
#include "joymng.h"
#include "np2.h"

#if defined(SUPPORT_JOYSTICK)

static SDL_Gamepad  *s_gamepad[2]  = { NULL, NULL };
static REG8          s_joystat     = 0xff;

REG8 joymng_available(void)
{
	return (s_gamepad[0] != NULL) ? 1 : 0;
}

REG8 joymng_getstat(void)
{
	return s_joystat;
}

void joymng_initialize(void)
{
	SDL_InitSubSystem(SDL_INIT_GAMEPAD);
	/* open first available gamepad */
	int count = 0;
	SDL_JoystickID *ids = SDL_GetGamepads(&count);
	if (ids && count > 0) {
		s_gamepad[0] = SDL_OpenGamepad(ids[0]);
		if (count > 1)
			s_gamepad[1] = SDL_OpenGamepad(ids[1]);
		SDL_free(ids);
	}
}

void joymng_deinitialize(void)
{
	int i;
	for (i = 0; i < 2; i++) {
		if (s_gamepad[i]) {
			SDL_CloseGamepad(s_gamepad[i]);
			s_gamepad[i] = NULL;
		}
	}
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

void joymng_sync(void)
{
	REG8 stat = 0xff;
	if (!s_gamepad[0]) {
		s_joystat = 0xff;
		return;
	}
	SDL_UpdateGamepads();

	/* axis → direction */
	int ax = SDL_GetGamepadAxis(s_gamepad[0], SDL_GAMEPAD_AXIS_LEFTX);
	int ay = SDL_GetGamepadAxis(s_gamepad[0], SDL_GAMEPAD_AXIS_LEFTY);
	const int DEAD = 8000;
	if (ay < -DEAD) stat &= ~JOY_UP_BIT;
	if (ay >  DEAD) stat &= ~JOY_DOWN_BIT;
	if (ax < -DEAD) stat &= ~JOY_LEFT_BIT;
	if (ax >  DEAD) stat &= ~JOY_RIGHT_BIT;

	/* D-pad */
	if (SDL_GetGamepadButton(s_gamepad[0], SDL_GAMEPAD_BUTTON_DPAD_UP))    stat &= ~JOY_UP_BIT;
	if (SDL_GetGamepadButton(s_gamepad[0], SDL_GAMEPAD_BUTTON_DPAD_DOWN))  stat &= ~JOY_DOWN_BIT;
	if (SDL_GetGamepadButton(s_gamepad[0], SDL_GAMEPAD_BUTTON_DPAD_LEFT))  stat &= ~JOY_LEFT_BIT;
	if (SDL_GetGamepadButton(s_gamepad[0], SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) stat &= ~JOY_RIGHT_BIT;

	/* buttons */
	if (SDL_GetGamepadButton(s_gamepad[0], SDL_GAMEPAD_BUTTON_SOUTH)) stat &= ~JOY_BTN1_BIT;
	if (SDL_GetGamepadButton(s_gamepad[0], SDL_GAMEPAD_BUTTON_EAST))  stat &= ~JOY_BTN2_BIT;

	s_joystat = stat;
}

joymng_devinfo_t **joymng_get_devinfo_list(void)
{
	return NULL;
}

#endif /* SUPPORT_JOYSTICK */
