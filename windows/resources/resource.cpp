#include <windows.h>

#pragma comment(linker, "/nodefaultlib")

#ifdef __cplusplus
extern "C"
#endif
BOOL APIENTRY _DllMainCRTStartup(HANDLE hModule, DWORD ulReasonForCall, LPVOID lpvReserved)
{
	return TRUE;
}

