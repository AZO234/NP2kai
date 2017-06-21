
	ORG		0
	USE16
	CPU		8086

segment .scsi start=0x0000

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

init1:			mov		word [bx], 0x82c2
				retf

init2:			mov		ax, cs
				mov		[0x04b2], ah
				mov		[0x04ba], ah
				mov		[0x04bc], ah
				mov		ax, 0x0320
				int		0x1b
				retf

bios:			cld
				mov		dx, cs
				mov		ds, dx
				mov		cx, 8
				mov		si, scsibiosstr
				test	byte [bp], 0x40
				je		short .send
				lea		si, [si + 8]
.send:			mov		dx, 0x07ef
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

boot:			cmp		al, 0x0c
				jne		short .exit
				mov		ax, 0x4620
				mov		cx, 0x1fc0
				mov		es, cx
				xor		bp, bp
				mov		bx, 0x400
				xor		cx, cx
				xor		dx, dx
				int		0x1b
				jc		short .exit
				or		al, 0x80
				mov		[0x0584], al
				db		0x9a				; call far
				dw		0x0000
				dw		0x1fc0
.exit:			retf

scsibiosstr		db		"scsibios"
				db		"scsi_dev"

	ends

