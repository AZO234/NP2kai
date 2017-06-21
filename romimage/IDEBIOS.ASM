
	ORG		0
	USE16
	CPU		8086

segment .ide start=0x0000

				retf						; 00
				nop
				nop
				retf						; 03
				nop
				nop
				retf						; 06
				nop
				nop
				db	0x55, 0xaa, 0x02		; 09
				jmp		short init1			; 0c
				nop
				jmp		short init2			; 0f
				nop
				retf						; 12
				nop
				nop
				jmp		short boot			; 15
				nop
				jmp		short bios			; 18
				nop
				retf						; 1b
				nop
				nop
				retf						; 1e
				nop
				nop
				retf						; 21
				nop
				nop
				retf						; 24
				nop
				nop
				retf						; 27
				nop
				nop
				retf						; 2a
				nop
				nop
				retf						; 2d
				nop
				nop

init1:			mov		byte [bx], 0xd9
				retf

init2:			mov		ax, cs
				or		byte [0x0480], 0x80
				mov		[0x04b0], ah
				mov		[0x04b8], ah
				mov		ax, 0x0300
				int		0x1b
				retf

bios:			cld
				mov		dx, cs
				mov		ds, dx
				mov		cx, 8
				mov		si, idebiosstr
				mov		dx, 0x07ef
				cli
.loop:			lodsb
				out		dx, al
				loop	.loop
				sti
				pop		ax
				pop		bx
				pop		cx
				pop		dx
				pop		bp
				pop		es
				pop		di
				pop		si
				pop		ds
				iret

boot:			cmp		al, 0x0a
				je		short .boot2
				cmp		al, 0x0b
				je		short .boot2
				retf

.boot2:			sub		al, 9
				test	[0x055d], al
				je		short .exit
				dec		al
				mov		ah, 0x06
				mov		cx, 0x1fc0
				mov		es, cx
				xor		bx, bx
				mov		bp, bx
				mov		cx, bx
				mov		dx, bx
				mov		bh, 4
				int		0x1b
				jc		short .exit
				or		al, 0x80
				mov		byte [0x0584], al
				db		0x9a				; call far
				dw		0x0000
				dw		0x1fc0
.exit:			retf

idebiosstr		db		"sasibios"

	ends

