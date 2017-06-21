/*
 *	@file	main.c
 */

#include "compiler.h"
#include "../np2/sdl2/dosio.h"
#include "../np2/sdl2/np2.h"

int SDL_main(int argc, char *argv[])
{
	file_setcd("/sdcard/application/np2/");
	return np2_main(argc, argv);
}
