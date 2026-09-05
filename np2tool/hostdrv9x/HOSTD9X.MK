# Windows 98 DDK make rules for HOSTD9X.VXD.
# Run the DDK SETENV.BAT first, then run NMAKE in this directory.

!IFNDEF DDKBUILDENV
DDKBUILDENV=free
!ENDIF

!IF "$(DDKBUILDENV)" == "checked"
DEB=-DDEBUG -DDBG=1 -DH9X_DEBUG_DIAGNOSTICS=1 -Zi
!ELSE
DEB=-DNDEBUG -DH9X_DEBUG_DIAGNOSTICS=0
!ENDIF

BIN=obj\i386\$(DDKBUILDENV)
TARGETPATH=bin\$(DDKBUILDENV)
INCLUDE=$(BASEDIR)\inc\win98;$(INC32);$(INCLUDE);.

ASM=ml
CL=cl -bzalign
LINK=link
MAPSYM=mapsym
LIBVXD=$(BASEDIR)\lib\i386\free\vxdwraps.clb

# /Zp4 is required by the Win98 IFS ioreq and function-vector ABI.
CFLAGS=-nologo -W3 -Zp4 -Gs -Zl -c -Gz -DIS_32 -DWIN40_OR_LATER
AFLAGS=-DIS_32 -nologo -W2 -Zm -Cx -DMASM6 -DVMMSYS -DWIN40_OR_LATER -DWIN40COMPAT -c -coff -DBLD_COFF

.asm{$(BIN)}.obj:
	set INCLUDE=$(INCLUDE)
	$(ASM) $(AFLAGS) $(DEB) -Fo$*.obj $<

.c{$(BIN)}.obj:
	set INCLUDE=$(INCLUDE)
	$(CL) $(CFLAGS) $(DEB) -Fo$*.obj $<

target: $(BIN) $(TARGETPATH) $(TARGETPATH)\$(DEVICE).VXD

$(BIN):
	if not exist $(BIN)\nul md $(BIN)

$(TARGETPATH):
	if not exist bin\nul md bin
	if not exist $(TARGETPATH)\nul md $(TARGETPATH)

$(TARGETPATH)\$(DEVICE).VXD: $(OBJS) $(DEVICE).DEF $(LIBVXD)
	$(LINK) @<<
-MACHINE:i386
-DEBUG:NONE
-PDB:NONE
-NODEFAULTLIB
-DEF:$(DEVICE).DEF
-OUT:$@
-MAP:$(BIN)\$(DEVICE).map
-VXD
$(LIBVXD)
$(OBJS)
<<
	-copy HOSTD9X.INF $(TARGETPATH)
	-copy HOSTD9X.REG $(TARGETPATH)
	-cd $(BIN) && $(MAPSYM) $(DEVICE)

clean:
	-@if exist obj\i386\*.obj del obj\i386\*.obj
	-@if exist obj\i386\*.map del obj\i386\*.map
	-@if exist obj\i386\*.sym del obj\i386\*.sym
	-@if exist bin\checked\*.* del bin\checked\*.*
	-@if exist bin\free\*.* del bin\free\*.*
