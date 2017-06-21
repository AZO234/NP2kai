AOPT   = /MX
COPT   = -c -AS -W3 -Gs -Ot -Zp1 -G2
LOPT   = /NOI /CP:1 /ST:8192

CC     = cl
AS     = masm
OBJ    = ..\..\obj\romtool

BTOBJ	=	$(OBJ)\bin2txt.obj	\
			$(OBJ)\dosio32.obj
BTBIN	= ..\bin2txt.exe

TPOBJ	=	$(OBJ)\txtpack.obj	\
			$(OBJ)\dosio32.obj
TPBIN	= ..\txtpack.exe

all: $(OBJ) $(BTBIN) $(TPBIN)

$(OBJ):
	if not exist $(OBJ) mkdir $(OBJ)

$(BTBIN): $(BTOBJ)
	link $(LOPT) @bin2txt.lk;

$(TPBIN): $(TPOBJ)
	link $(LOPT) @txtpack.lk;

.asm{$(OBJ)}.obj:
	$(AS) $(AOPT) $(*B),$*;

.c{$(OBJ)}.obj:
	$(CC) $(COPT) -Fo$* $(*B).c

$(OBJ)\bin2txt.obj	: common.h dosio.h
$(OBJ)\txtpack.obj	: common.h dosio.h
$(OBJ)\dosio32.obj	: mil.inc

