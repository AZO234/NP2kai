
FIXCS		equ		2eh


CALLNE		macro	calllabel
			je		short $+5
			call	calllabel
			endm

CALLNZ		macro	calllabel
			je		short $+5
			call	calllabel
			endm

CALLE		macro	calllabel
			jne		short $+5
			call	calllabel
			endm

CALLZ		macro	calllabel
			jne		short $+5
			call	calllabel
			endm

CALLNC		macro	calllabel
			jc		short $+5
			call	calllabel
			endm

CALLC		macro	calllabel
			jnc		short $+5
			call	calllabel
			endm



jmps	macro	label				; N88BASIC MON‚Ì JMPS‚Ë
		jmp		short label
		endm

BOOT_NORMAL		equ		00h
BOOT_REVISION	equ		01h
BOOT_FIRMWARE	equ		02h
BOOT_MENU		equ		03h

BOOT_WFLASH		equ		10h
BOOT_RFLASH		equ		11h
BOOT_DIPSWINIT	equ		12h

