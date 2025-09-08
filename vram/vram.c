#include	"compiler.h"
#include	"cpucore.h"
#include	"vram.h"


	_VRAMOP	vramop;
	UINT8	tramupdate[0x1000];
	UINT8	vramupdate[0x8000];
#if defined(SUPPORT_PC9821)
#if defined(SUPPORT_IA32_HAXM)
	UINT8	vramex_base[0x80000]; // PEGC VRAM
	UINT8	*vramex = vramex_base; // PEGC VRAM  Alloc in pccore_mem_malloc()
#else
	UINT8	vramex[0x80000]; // PEGC VRAM
#endif
#endif


void vram_initialize(void) {

	ZeroMemory(&vramop, sizeof(vramop));
	MEMM_VRAM(0);
}

