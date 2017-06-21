AS		= masm
AOPT	= /ML /DNP2
OBJ		= ..\obj\romrel

..\bios\itfrom.res: $(OBJ) $(OBJ)\itf.bin
	bin2txt $(OBJ)\itf.bin itfrom > ..\bios\itfrom.res

$(OBJ):
	if not exist $(OBJ) mkdir $(OBJ)

$(OBJ)\itf.bin: $(OBJ)\itf.exe
	exe2bin $(OBJ)\itf $(OBJ)\itf.bin

$(OBJ)\itf.exe: $(OBJ)\itf.obj
	link $(OBJ)\itf,$(OBJ)\itf;

$(OBJ)\itf.obj: itf.asm itf.inc dataseg.inc process.mac debug.mac \
				resource.x86 itfsub.x86 \
				keyboard.inc keyboard.x86  textdisp.x86 \
				np2.x86 dipsw.x86 memsw.x86 \
				beep.x86 firmware.x86 memchk.x86 \
				ssp.x86 ssp_res.x86 ssp_sub.x86 ssp_dip.x86 ssp_msw.x86
	$(AS) $(AOPT) $(*B),$*,$*;

resource.x86: resource.txt
	txtpack resource.txt > resource.x86
