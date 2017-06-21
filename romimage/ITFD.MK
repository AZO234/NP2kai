AS		= masm
AOPT	= /ML /DMSDOS /DDEBUG
OBJ		= DEBUG

itf.com: $(OBJ)\itf.exe
	exe2bin $(OBJ)\itf itf.com

$(OBJ)\itf.exe: $(OBJ)\itf.obj
	link $(OBJ)\itf,$(OBJ)\itf;

$(OBJ)\itf.obj: itf.asm itf.inc dataseg.inc process.mac debug.mac \
				resource.x86 itfsub.x86 \
				keyboard.inc keyboard.x86 textdisp.x86 \
				np2.x86 dipsw.x86 memsw.x86 \
				beep.x86 firmware.x86 memchk.x86		\
				ssp.x86 ssp_res.x86 ssp_sub.x86 ssp_dip.x86 ssp_msw.x86
	$(AS) $(AOPT) $(*B),$*;

resource.x86: resource.txt
	tool\txtpack resource.txt > resource.x86
