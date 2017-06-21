
	ORG		0
	USE16
	CPU		8086

segment .sasi start=0x0000

				retf						; 00
				nop
				nop
				retf						; 03
				nop
				nop
				retf						; 06
				nop
				nop
				db	055h, 0aah, 002h		; 09
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
				mov		word [0x0044], sasiint
				mov		[0x0046], ax
				mov		[0x04b0], ah
				mov		[0x04b8], ah
				mov		ax, 0x0300
				int		0x1b
				retf

bios:			cld
				mov		dx, cs
				mov		ds, dx
				mov		cx, 8
				mov		si, sasibiosstr
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
				je		short .main
				cmp		al, 0x0b
				je		short .main
				retf

.main:			sub		al, 9
				test	[0x055d], al			; sasi
				je		short .exit
				dec		al
				mov		ah, 0x06
				mov		cx, 0x1fc0
				mov		es, cx
				xor		bp, bp
				mov		bx, 0x0400
				xor		cx, cx
				xor		dx, dx
				int		0x1b
				jc		short .exit
				or		al, 0x80
				mov		[0x0584], al
				db		0x9a					; call far
				dw		0x0000
				dw		0x1fc0
.exit:			retf


; MS-DOSÇ™Ç±ÇÃäÑÇËçûÇ›ÇégópÇ∑ÇÈ...

sasiint:		push	ax
				in		al, 0x82
				and		al, 0xfd
				cmp		al, 0xad
				je		short .readstat
				and		al, 0xf9
				cmp		al, 0xa1
				jne		short .eoi
.readstat:		push	ds
				xor		ax, ax
				mov		ds, ax
				or		byte [0x055f], 1
				mov		al, 0xc0
				out		0x82, al
				pop		ds
.eoi:			mov		al, 0x20
				out		0x08, al
				mov		al, 0x0b
				out		0x08, al
				in		al, 0x08
				test	al, al
				jne		short .exit
				mov		al, 0x20
				out		0x00, al
.exit:			pop		ax
				iret

sasibiosstr		db		"sasibios"

	ends

