# Minimal VC98/VC6 makefile for HOSTD9X.DLL
# Run from a Visual C++ command prompt, then: NMAKE /F HOSTD9X.MAK

CC=cl
RC=rc
LINK=link

H9XNP_DEBUG_LOG=0
CFLAGS=/nologo /W3 /O2 /Gz /DWIN32 /D_WINDOWS /DH9XNP_DEBUG_LOG=$(H9XNP_DEBUG_LOG) /c
RCFLAGS=
LFLAGS=/nologo /DLL /MACHINE:I386 /DEF:HOSTD9X.DEF /OUT:HOSTD9X.DLL

all: HOSTD9X.DLL

hostd9x.obj: hostd9x.c
	$(CC) $(CFLAGS) hostd9x.c

hostd9x.res: hostd9x.rc
	$(RC) $(RCFLAGS) /fohostd9x.res hostd9x.rc

HOSTD9X.DLL: hostd9x.obj hostd9x.res HOSTD9X.DEF
	$(LINK) $(LFLAGS) hostd9x.obj hostd9x.res kernel32.lib advapi32.lib user32.lib

clean:
	-if exist hostd9x.obj del hostd9x.obj
	-if exist hostd9x.res del hostd9x.res
	-if exist HOSTD9X.DLL del HOSTD9X.DLL
	-if exist HOSTD9X.EXP del HOSTD9X.EXP
	-if exist HOSTD9X.LIB del HOSTD9X.LIB
