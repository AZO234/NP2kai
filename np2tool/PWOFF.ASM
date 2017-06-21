
	ORG		100h
	USE16
	CPU		8086

%include	'np2tool.inc'

START:			cld
				push	cs
				pop		ds
				cli
				call	np2_check
				jne		short err_nonnp2
				call	sendnp2port
				mov		ax, 4c00h
				int		21h

err_nonnp2:		mov		ah, 9
				mov		dx, str_illegal
				int		21h
				mov		ah, 9
				mov		dx, bx
				int		21h
				mov		ah, 9
				mov		dx, str_crlf
				int		21h
				mov		ax, 4c01h
				int		21h


%include	'np2tool.x86'
			db	8, 'poweroff'


str_illegal	db	'Illegal hardware - $'
str_crlf	db	0dh, 0ah, '$'

	ends

