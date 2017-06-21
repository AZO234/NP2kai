
	ORG		100h
	USE16
	CPU		8086

%include	'np2tool.inc'


GETBIOS_BIOS	equ		0x01
GETBIOS_9821	equ		0x02
GETBIOS_ITF		equ		0x04
GETBIOS_HDD		equ		0x08
GETBIOS_SOUND	equ		0x10


main:			push	cs
				pop		ds
				cld

checkmachine:	mov		ah, 0fh
				int		10h
				cmp		ah, 0fh
				je		short .ed
				mov		dx, str_errnot98
				call	dosdisp
				mov		ax, 4c00h
				int		21h
.ed:


cmdline:		mov		si, 0x80
				lodsb
				and		ax, 7fh
				je		short .skip
				mov		dx, ax
.lp:			dec		dx
				js		short .lped
				lodsb
				call	isslash
				jne		short .lp
.slash:			dec		dx
				js		short .lped
				lodsb
				call	isslash
				je		short .slash
				and		al, 0xdf
				mov		di, .cmdtbl
.cmdlp:			mov		cx, [di]
				jcxz	.cmdup
				inc		di
				inc		di
				cmp		al, cl
				jne		short .cmdlp
.cmdup:			or		ah, ch
				jmp		short .lp

.cmdtbl			db	'A', 0xff
				db	'B', GETBIOS_BIOS
				db	'E', GETBIOS_9821
				db	'I', GETBIOS_ITF
				db	'H', GETBIOS_HDD
				db	'S', GETBIOS_SOUND
				db	0, 0

.lped:			mov		[getbios_cfg], ah
.skip:


; BIOS.ROM

makebiosrom:	test	byte [getbios_cfg], GETBIOS_BIOS
				je		short .skip
				mov		dx, str_biosrom
				call	filecreate
				jc		short .ed
				mov		bx, ax
				mov		cx, 08000h
				mov		dx, 0e800h
				call	filewrite
				jc		short .fclose
				mov		dx, 0f000h
				call	filewrite
				jc		short .fclose
				mov		dx, 0f800h
				call	filewrite
				jc		short .fclose
				mov		dx, str_done
				call	dosdisp
.fclose:		call	fileclose

.ed:			mov		dx, str_crlf
				call	dosdisp
.skip:


; BIOS9821.ROM

makepc9821rom:	test	byte [getbios_cfg], GETBIOS_9821
				je		short .skip
				call	ispc9821
				je		short .skip

				mov		dx, str_bios21rom
				mov		ax, 0xd800
				mov		cx, 0x2000
				call	memorydump
.skip:


; HDD

makehddrom:		test	byte [getbios_cfg], GETBIOS_HDD
				je		short .skip

				call	ispc9821
				jne		short .sasied
				mov		si, 0x04b0
				call	ishdd
				jc		short .sasied
				cli
				mov		ax, 0xa800
				mov		cx, 0x0800
				call	memorycopy
				sti
				mov		dx, str_sasirom
				mov		cx, 0x1000
				call	memorydump
.sasied:

				mov		si, 0x04b2
				call	ishdd
				jc		short .scsied
				cli
				mov		ah, 0x30
				call	scsirecv
				push	ax					; •\
				and		al, 0xbf
				call	scsirecv
				mov		ax, 0xa800
				mov		cx, 0x1000
				call	memorycopy
				pop		ax
				push	ax					; — 
				or		al, 0x40
				call	scsirecv
				mov		ax, 0xaa00
				mov		cx, 0x1000
				call	memorycopy
				pop		ax
				call	scsisend
				sti
				mov		dx, str_scsirom
				mov		ax, 0xa800
				mov		cx, 0x4000
				call	memorydump
.scsied:

.skip:


; SOUND.ROM

makesoundrom:	test	byte [getbios_cfg], GETBIOS_SOUND
				je		short .skip
				mov		ax, 0c800h
.sealp:			mov		es, ax
				mov		si, sndbios_tbl
				mov		di, 0x2e00
				mov		cx, 3
				repz cmpsw
				je		short .dump
				add		ah, 4
				cmp		ah, 0d8h
				jc		short .sealp
				jmp		short .skip
.dump:			mov		dx, str_soundrom
				mov		cx, 0x4000
				call	memorydump
.skip:


returndos:		mov		ax, 4c00h
				int		21h


; ---- memorydump / ax=dstseg, dx=srcseg, cx=size/2

memorycopy:		push	ds
				mov		ds, dx
				mov		es, ax
				xor		si, si
				xor		di, di
				rep movsw
				pop		ds
				ret


; ---- memorydump / ax=seg, cx=size, dx=filename

memorydump:		push	ax
				push	cx
				call	filecreate
				mov		bx, ax
				pop		cx
				pop		dx
				jc		short .ed
				call	filewrite
				jc		short .fclose
				mov		dx, str_done
				call	dosdisp
.fclose:		call	fileclose
.ed:			mov		dx, str_crlf
				jmp		dosdisp



; ---- sub

isslash:		cmp		al, '-'
				je		short .ed
				cmp		al, '/'
.ed:			ret

ispc9821:		xor		ax, ax
				mov		es, ax
				test	byte [es:0x045c], 0x40
				ret

ishdd:			xor		dx, dx
				mov		es, dx
				or		dh, [es:si]
				je		short .err
				mov		es, dx
				cmp		word [es:0x0009], 0xaa55
				je		short .ed
.err:			stc
.ed:			ret


; ---- print

dosdisp:		push	si
				mov		si, dx
.loop:			lodsb
				cmp		al, 0
				jne		short .loop
				dec		si
				mov		byte [si], '$'
				mov		ah, 9
				int		21h
				mov		byte [si], 0
				pop		si
				ret


; ---- file

filecreate:		push	dx
				call	dosdisp
				mov		dx, str_sepa
				call	dosdisp
				pop		dx
				mov		ax, 3c00h
				xor		cx, cx
				int		21h
				jnc		short .ed
				mov		dx, str_openerr
				call	dosdisp
				stc
.ed:			ret

filewrite:		push	ds
				push	cx
				push	bx
				mov		ds, dx
				xor		dx, dx
				mov		ah, 40h
				int		21h
				pop		bx
				pop		cx
				pop		ds
				jc		short .err
				cmp		ax, cx
				je		short .ed
.err:			mov		dx, str_writeerr
				call	dosdisp
				stc
.ed:			ret

fileclose:		mov		ah, 3eh
				int		21h
				ret


; ---- scsi cmd

scsiport:		xchg	al, ah
				mov		dx, 0xcc0
				out		dx, al
				xchg	al, ah
				inc		dx
				inc		dx
				ret

scsirecv:		push	dx
				call	scsiport
				in		al, dx
				pop		dx
				ret

scsisend:		push	dx
				call	scsiport
				out		dx, al
				pop		dx
				ret


; ---- values

getbios_cfg		db	GETBIOS_BIOS | GETBIOS_9821 | GETBIOS_SOUND


; ---- resources

str_biosrom		db	"BIOS.ROM", 0
str_bios21rom	db	"BIOS9821.ROM", 0
; str_itfrom		db	"ITF.ROM", 0
str_sasirom		db	"SASI.ROM", 0
str_scsirom		db	"SCSI.ROM", 0
str_soundrom	db	"SOUND.ROM", 0

str_errnot98	db	"Illegal hardware (work only PC-98x1)", 0

str_sepa		db	" : ", 0
str_openerr		db	"file open error", 0
str_writeerr	db	"file write error", 0
str_done		db	"done!", 0
str_crlf		db	13, 10, 0
sndbios_tbl		db	1, 0, 0, 0, 0d2h, 0


	ends

