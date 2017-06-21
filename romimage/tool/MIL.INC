.xlist

ifdef LARGE
ARG1	equ		6
EARG1	equ		4
else
ARG1	equ		4
EARG1	equ		2
endif


ifdef LARGE
func	macro	name
	public name
	name proc far

	endfunc macro
		name endp
	endm
endm

else

func	macro	name
	public name
	name proc near

	endfunc macro
		name endp
	endm
endm
endif


nfunc	macro	name
	public name
	name proc near

	endfunc macro
		name endp
	endm
endm

ARG2	equ		ARG1+2
ARG3	equ		ARG1+4
ARG4	equ		ARG1+6
ARG5	equ		ARG1+8
ARG6	equ		ARG1+10
ARG7	equ		ARG1+12
ARG8	equ		ARG1+14

EARG2	equ		EARG1+2
EARG3	equ		EARG1+4
EARG4	equ		EARG1+6



pushall	macro
		pushf
		db		60h					; ひでー(笑) PUSHA命令です。
		push	ds
		push	es
		endm

popall	macro
		pop		es
		pop		ds
		db		61h					; こっちは POPAです。未使用だけど
		popf
		endm

jmps	macro	label				; N88BASIC MONの JMPSね
		jmp		short label
		endm

chg_srcdst	macro
		push	ds
		push	es
		pop		ds
		pop		es
		xchg	si,di
		endm

set_es	macro	segs
		push	ax
		mov		ax,segs
		mov		es,ax
		pop		ax
		endm

PC9801		EQU		0		; PC-9801
JDOS		EQU		2		; JDOS Japanese Mode
PCDOS		EQU		3		; JDOS English Mode
DOSV		EQU		4		; DOS/V Japanese Mode
DOSVUS		EQU		5		; DOS/V English Mode
AXJ			EQU		6		; AX Japanese Mode
J3100		EQU		8		; J-3100 Japanese Mode




KEYBUF_FLASH98	macro
if 1
				mov		ax,0c06h
				mov		dl,-1
				int		21h
else
				local	keylp
keylp:			mov		ah,6
				mov		dl,-1
				int		21h
				jne		keylp
endif
				endm

.list
