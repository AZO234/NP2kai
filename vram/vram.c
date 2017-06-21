#include	"compiler.h"
#include	"cpucore.h"
#include	"vram.h"


	_VRAMOP	vramop;
	UINT8	tramupdate[0x1000];
	UINT8	vramupdate[0x8000];
#if defined(SUPPORT_PC9821)
	UINT8	vramex[0x80000];
#endif


void vram_initialize(void) {

	ZeroMemory(&vramop, sizeof(vramop));
	MEMM_VRAM(0);
}

