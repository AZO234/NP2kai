
USE_HIMEM		equ		0x110000

VRAM_STEP		equ		0x100000
VRAM_B			equ		0x0a8000
VRAM_R			equ		0x0b0000
VRAM_G			equ		0x0b8000
VRAM_E			equ		0x0e0000

VRAM0_B			equ		VRAM_B
VRAM0_R			equ		VRAM_R
VRAM0_G			equ		VRAM_G
VRAM0_E			equ		VRAM_E
VRAM1_B			equ		VRAM_B + VRAM_STEP
VRAM1_R			equ		VRAM_R + VRAM_STEP
VRAM1_G			equ		VRAM_G + VRAM_STEP
VRAM1_E			equ		VRAM_E + VRAM_STEP

FONT_ADRS		equ		0x110000
ITF_ADRS		equ		0x1f8000

%macro VRAMPOS	1
				and		%1, VRAM_STEP | 7fffh
%endmacro


				struc	cpucore_t
.reg			resw	14
.es_base		resd	1
.cs_base		resd	1
.ss_base		resd	1
.ds_base		resd	1
.ss_fix			resd	1
.ds_fix			resd	1
.adrsmask		resd	1
.prefix			resw	1
.trap			resb	1
.resetreq		resb	1
.prefetchque	resd	1
.GDTR			resb	6
.MSW			resw	1
.IDTR			resb	6
.LDTR			resw	1
.LDTRC			resb	6
.TR				resw	1
.TRC			resb	6
				resb	2

.cpu_type		resb	1
.itfbank		resb	1
.ram_d0			resw	1
.remainclock	resd	1
.baseclock		resd	1
.clock			resd	1

.ext			resd	1
.extsize		resd	1
.ems			resd	4
.repbak			resd	1
.inport			resd	1
				endstruc

