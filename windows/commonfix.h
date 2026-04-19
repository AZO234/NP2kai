/************************************************************************
*                                                                       *
*   commonfix.h v 2.10-- This module is supported Windows 95/98/Me/2000 *
*                                           on VC++2005/2008/2010       *
*                                                                       *
*   Copyright (c) 2012, BlackWingCat/PFW . All rights reserved.         *
*                                                                       *
*   Blog: http://blog.livedoor.jp/blackwingcat/  Twitter:BlackWingCat   *
*  Usage:                                                               *
*#define WINVER2 0x0400 // or 0x0410 or 0x0500                          *
*#define DLLMODE        // DLL                                          *
*#include "commonfix.h"                                                 *
*  Add Link Option /FILEALIGN:4096 /MT or /MTd                          *
* if you want run Win9x with VC2008/VC2010                              *
*  use fcwin enabled execute flag after link                            *
************************************************************************/
#include <windows.h>
#ifdef __cplusplus 
extern "C"{
#endif
#ifdef _CONSOLE
#pragma comment(linker,"/entry:mainCRTStartup2") 
#else
#pragma comment(linker,"/entry:WinMainCRTStartup2") 
//#pragma comment(lib, "shlwapi.lib")
#endif

#if (WINVER2 <= 0x0400)
 #ifdef _CONSOLE
 #pragma comment(linker,"/CONSOLE:Windows,4.00") 
 #else
 #pragma comment(linker,"/SUBSYSTEM:Windows,4.00") 
 #endif
#else
	#if (WINVER2 <= 0x0410)
	 #ifdef _CONSOLE
		#pragma comment(linker,"/CONSOLE:Windows,4.10") 
	 #else
		#pragma comment(linker,"/SUBSYSTEM:Windows,4.10") 
	 #endif
	#else
	 #ifdef _CONSOLE
		#pragma comment(linker,"/CONSOLE:Windows,5.00") 
	 #else
		#pragma comment(linker,"/SUBSYSTEM:Windows,5.00") 
	 #endif
	#endif
#endif


const char strKERNEL32DLL[]="KERNEL32.DLL";
#if (WINVER2 <= 0x0400)//WIN95
#if _MSC_VER >=1400 //VC2005
const char strIsDebuggerPresent[]="IsDebuggerPresent";
BOOL _declspec(naked ) WINAPI IsDebuggerPresent_stub(void)
{
	_asm{
		push offset strKERNEL32DLL
		call DWORD PTR[GetModuleHandleA]
		and eax,eax
		jnz L1
		retn
L1:
		push offset strIsDebuggerPresent
		push eax		
		call DWORD PTR[GetProcAddress]
		and eax,eax
		jz L2
		call eax
L2:		retn
	}
}
int _declspec(naked ) WINAPI _imp__IsDebuggerPresent(void){
	_asm{
		_emit 0xc3
		_emit 0xc3
		_emit 0xc3
		_emit 0xc3
	}
}
#endif//VC2005
#if _MSC_VER >=1600 //VC2010
BOOL WINAPI IsProcessorFeaturePresent_stub(DWORD flg1){
	typedef BOOL (WINAPI *IPFP )(DWORD flg1);
	IPFP PIPFP=NULL;
	HINSTANCE hDll;
	hDll=GetModuleHandleA(strKERNEL32DLL);
	if(hDll){
		PIPFP=(IPFP)GetProcAddress(hDll,"IsProcessorFeaturePresent");
	}
	if(PIPFP==NULL){
		int FP=0;
#define ACPI_FLAG 0x400000
	_asm{
		xor eax,eax
		cpuid
		cmp eax,0x756e6547
		jnz L1
		mov eax,1
		cpuid
		and ah,0xf
		cmp ah,5
		jnz L1
		and al,0xf0
		cmp al,0x20
		jz L2
		cmp al,0x70
		jnz L1
L2:			
		or FP,1<<PF_FLOATING_POINT_PRECISION_ERRATA
L1:
		test edx,1
		jnz NOFPU
		or FP,1<<PF_FLOATING_POINT_EMULATED
NOFPU:
		test edx,0x10
		jnz NORDTSC
		or FP,1<<PF_RDTSC_INSTRUCTION_AVAILABLE
NORDTSC	:
		test edx,0x800000
		jnz NOMMX
		or FP,1<<PF_MMX_INSTRUCTIONS_AVAILABLE
NOMMX:
		test edx,0x01000000
		jnz NOFXSR
		or FP,1<<PF_XSAVE_ENABLED
NOFXSR:
		test edx,0x02000000//SSE
		jnz NOSSE
		or FP,1<<PF_XMMI_INSTRUCTIONS_AVAILABLE
NOSSE:	test edx,0x04000000//SSE2
		jnz NOSSE2
		or FP,1<<PF_XMMI64_INSTRUCTIONS_AVAILABLE
NOSSE2:	test ecx,1
		jnz NOSSE3
		or FP,1<<PF_SSE3_INSTRUCTIONS_AVAILABLE
NOSSE3:	test ecx,1<<13
		jnz NOCMPXCHG16B
		or FP,1<<PF_COMPARE_EXCHANGE128
NOCMPXCHG16B:
		mov eax,0x80000000
		cpuid
		cmp eax,0x80000001
		jb NO3DNOW
		mov eax,0x80000001
		cpuid
		test edx,0x80000000
		jnz NO3DNOW
		or FP,1<<PF_3DNOW_INSTRUCTIONS_AVAILABLE
NO3DNOW:
			mov ecx,flg1
			mov eax,1
			shl eax,cl
			and FP,eax
		}
		if((GetVersion()&0xf)<5){
	//Windows 95 ‚Å SSE2 –½—ß‚ðŽÀs‚·‚é‚Æ—Ž‚¿‚é‚Ì‚Å”O‚Ì‚½‚ß
			FP&=~(1<<PF_SSE3_INSTRUCTIONS_AVAILABLE);
			FP&=~(1<<PF_XMMI64_INSTRUCTIONS_AVAILABLE);
			FP&=~(1<<PF_COMPARE_EXCHANGE128);
			FP&=~(1<<PF_XSAVE_ENABLED);
		}
		return FP;

	}
	return PIPFP(flg1);
}
int _declspec(naked ) WINAPI _imp__IsProcessorFeaturePresent(DWORD flg1)
{
	_asm{
		_emit 0xc3
		_emit 0x90
		_emit 0xc3
		_emit 0xc3
	}
}
#endif//VC++2010
#endif//WIN95

#if (WINVER2 <= 0x0500)//WIN2000
int __declspec(naked) WINAPI EncodePointer_stub( int flg1){
	_asm{
        call DWORD PTR[GetCurrentProcessId]
     	xor eax,0x12345678
		ror eax,4
		xor eax,[esp+4]
		ror eax,4
        retn 4
	}
}
int __declspec(naked) WINAPI DecodePointer_stub( int flg1){
  _asm{

        call DWORD PTR[GetCurrentProcessId]
		xor eax,0x12345678
		ror eax,8
		xor eax,DWORD PTR[esp+4]
        rol eax,4
        retn 4
  }
}
int WINAPI FindActCtxSectionStringA_stub( int flg1,
					int flg2,
					int flg3,
					char *pszFilename,
					void *data)
{
	typedef int (WINAPI *FACSS)(int, int,int,char*,void*);
	FACSS FACSSA;
	HINSTANCE hDll;
	hDll=GetModuleHandleA(strKERNEL32DLL);
	if(hDll){
		FACSSA=(FACSS)GetProcAddress(hDll,"FindActCtxSectionStringA");
		if(FACSSA){
			return FACSSA(flg1,flg2,flg3,pszFilename,data);
		}
	}
	return 0;
}
int WINAPI FindActCtxSectionStringW_stub( int flg1,
					int flg2,
					int flg3,
					wchar_t *pszFilename,
					void *data)
{
	typedef int (WINAPI *FACSS )(int, int,int,wchar_t*,void*);
	FACSS FACSSW;
	HINSTANCE hDll;
	hDll=GetModuleHandleA(strKERNEL32DLL);
	if(hDll){
		FACSSW=(FACSS)GetProcAddress(hDll,"FindActCtxSectionStringW");
		if(FACSSW){
			return FACSSW(flg1,flg2,flg3,pszFilename,data);
		}
	}
	return 0;
}

#if (WINVER2 < 0x0500)//WINME
#if _MSC_VER >=1500 //VC2008

typedef BOOL (WINAPI *ICSAC)(HANDLE flg1,int flg2);
BOOL WINAPI InitializeCriticalSectionAndSpinCount_stub(HANDLE flg1,DWORD flg2){
	ICSAC PICSAC=NULL;
	HINSTANCE hDll;
	hDll=GetModuleHandleA(strKERNEL32DLL);
	if(hDll){
		PICSAC=(ICSAC)GetProcAddress(hDll,"InitializeCriticalSectionAndSpinCount");
	}
	if((GetVersion()&0xf)<5 || PICSAC==NULL){
		InitializeCriticalSection((LPCRITICAL_SECTION)flg1);
		return 1;
	}
	return PICSAC(flg1,flg2);
}
typedef BOOL (WINAPI *HSIS )(HANDLE flg1,int flg2,void *flg3,DWORD flg4);
BOOL WINAPI HeapSetInformation_stub(HANDLE flg1,int flg2,void *flg3,DWORD flg4){
	HSIS PHSIS;
	HINSTANCE hDll;
	hDll=GetModuleHandleA(strKERNEL32DLL);
	if(hDll){
		PHSIS=(HSIS)GetProcAddress(hDll,"HeapSetInformation");
		if(PHSIS){
			return PHSIS(flg1,flg2,flg3,flg4);
		}
	}
	return 0;

}
typedef BOOL (WINAPI *HQIS )(HANDLE flg1,int flg2,void*flg3,DWORD flg4,DWORD *flg5);
BOOL WINAPI HeapQueryInformation_stub(HANDLE flg1,int flg2,void*flg3,DWORD flg4,DWORD *flg5){
	HQIS PHQIS;
	HINSTANCE hDll;
	hDll=GetModuleHandleA(strKERNEL32DLL);
	if(hDll){
		PHQIS=(HQIS)GetProcAddress(hDll,"HeapQueryInformation");
		if(PHQIS){
			return PHQIS(flg1,flg2,flg3,flg4,flg5);
		}
	}
	return 0;
}
int _declspec(naked ) WINAPI _imp__InitializeCriticalSectionAndSpinCount( void* flg1,
					DWORD flg2)
{
	_asm{
		_emit 0x90
		_emit 0x90
		_emit 0xc3
		_emit 0x90
	}
}
int _declspec(naked ) WINAPI _imp__HeapSetInformation( HANDLE flg1,int flg2,void *flg3,DWORD flg4)
{
	_asm{
		_emit 0x33
		_emit 0xc0
		_emit 0xc3
		_emit 0x90
	}
}
int _declspec(naked ) WINAPI _imp__HeapQueryInformation( HANDLE flg1,int flg2,void*flg3,DWORD flg4,DWORD *flg5)
{
	_asm{
		_emit 0x33
		_emit 0xc0
		_emit 0x90
		_emit 0xc3
	}
}
#endif//VC2008
#endif//WINME


#if _MSC_VER >=1600 //VC2010
int _declspec(naked ) WINAPI _imp__FindActCtxSectionStringA( int flg1,
					int flg2,
					int flg3,
					wchar_t *pszFilename,
					void *data)
{
	_asm{
		_emit 0x33
		_emit 0xc0
		_emit 0xc3
		_emit 0xc3
	}
}
int _declspec(naked ) WINAPI _imp__FindActCtxSectionStringW( int flg1,
					int flg2,
					int flg3,
					wchar_t *pszFilename,
					void *data)
{
	_asm{
		_emit 0xc3
		_emit 0x33
		_emit 0xc0
		_emit 0xc3
	}
}
int _declspec(naked ) WINAPI _imp__EncodePointer( int flg1)
{
	_asm{
		_emit 0xc3
		_emit 0xc3
		_emit 0x33
		_emit 0xc0
	}
}
int _declspec(naked ) WINAPI _imp__DecodePointer( int flg1)
{
	_asm{
		_emit 0x33
		_emit 0xc0
		_emit 0x33
		_emit 0xc0
	}
}
#endif//VC2010
#endif//Win2000

void initialize_findacx
(){
//	LoadLibraryA("unicows.lib");
	FARPROC i;
	DWORD dw;
#if (WINVER2 <= 0x0400) //Win95
#if _MSC_VER >=1400 //VC2005
	i=(FARPROC)&_imp__IsDebuggerPresent;
    VirtualProtect(i, sizeof(FARPROC), PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)i=(FARPROC)&IsDebuggerPresent_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);
#if _MSC_VER >=1600 //VC2010
	i=(FARPROC)&_imp__IsProcessorFeaturePresent;
    VirtualProtect(i, sizeof(FARPROC), PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)i=(FARPROC)&IsProcessorFeaturePresent_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);
#endif//VC2010
#endif//VC2005
#endif//Win95

#if _MSC_VER >=1600 //VC2010
	i=(FARPROC)& _imp__FindActCtxSectionStringW;
    VirtualProtect(i, sizeof(FARPROC), PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)
		i=(FARPROC)&FindActCtxSectionStringW_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);

	i=(FARPROC)&_imp__FindActCtxSectionStringA;
    VirtualProtect(i, sizeof(FARPROC), PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)i=(FARPROC)&FindActCtxSectionStringA_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);

	i=(FARPROC)&_imp__EncodePointer;
    VirtualProtect(i, sizeof(FARPROC), PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)i=(FARPROC)&EncodePointer_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);

	i=(FARPROC)&_imp__DecodePointer;
    VirtualProtect(i, sizeof(FARPROC),PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)i=(FARPROC)&DecodePointer_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);
#endif //VC2010
#if _MSC_VER >=1500 //VC2008
#if (WINVER2 < 0x0500) //WinMe
	i=(FARPROC)&_imp__HeapQueryInformation;
    VirtualProtect(i, sizeof(FARPROC),PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)i=(FARPROC)&HeapQueryInformation_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);

	i=(FARPROC)&_imp__HeapSetInformation;
    VirtualProtect(i, sizeof(FARPROC),PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)i=(FARPROC)&HeapSetInformation_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);

	i=(FARPROC)&_imp__InitializeCriticalSectionAndSpinCount;
    VirtualProtect(i, sizeof(FARPROC),PAGE_EXECUTE_READWRITE	, &dw);
	*(FARPROC*)i=(FARPROC)&InitializeCriticalSectionAndSpinCount_stub;
	VirtualProtect(i, sizeof(FARPROC), dw, &dw);
#endif//Me
#endif//VC2008
}

#ifdef DLLMODE
int WINAPI _DllMainCRTStartup(int hm,int rs,void *rev);
int WINAPI WinMainCRTStartup2(int hm,int rs,void *rev)
{
	if(rs==DLL_PROCESS_ATTACH)
		initialize_findacx();
	return _DllMainCRTStartup(hm,rs,rev);

}
#else
#ifdef _CONSOLE
extern void wmainCRTStartup();
extern void mainCRTStartup();
void mainCRTStartup2()
{
	initialize_findacx();
#ifdef _UNICODE
	wmainCRTStartup();
#else
	mainCRTStartup();
#endif
}
#else/ /CONSOLE
//extern void wWinMainCRTStartup();
extern void WinMainCRTStartup();
void WinMainCRTStartup2()
{
	initialize_findacx();
#ifdef _UNICODE
	WinMainCRTStartup(); // Unicode‚¾‚¯‚Çw•s—v
#else
	WinMainCRTStartup();
#endif
}
#endif

#endif//DLLMODE


#ifdef __cplusplus 
}
#endif

