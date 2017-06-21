
	ORG		0
	USE16
	CPU		8086

FIXCS		equ		2eh

segment .startup start=0x0000

START:			jmp		short nosystems
START2:			mov		si, nobiosmsg
				jmp		short dispend

nosystems:		mov		si, nosysmsg

dispend:		mov		ax, 0x0a04
				int		0x18
				mov		ah, 0x0c
				int		0x18
				mov		ah, 0x16
				mov		dx, 0xe120
				int		0x18
				cli
				cld
				mov		ax, 0xa000
				mov		es, ax
				db		FIXCS
				lodsw
				mov		di, ax
.loop:			db		FIXCS
				lodsw
				test	ax, ax
				je		short .end
				stosw
				or		ah, ah
				je		short .loop
				inc		di
				inc		di
				jmp		short .loop

.end:			hlt
				jmp		short .end


				; システムディスクをセットしてください
nosysmsg	dw	12*160+44
			dw	3705h,3905h,4605h,6005h,4705h,2305h,3905h,2f05h
			dw	7204h,3b05h,4305h,4805h,3704h,4604h,2f04h,4004h
			dw	3504h,2404h,0

				; BASICの起動には BIOS.ROMが必要です
nobiosmsg	dw	12*160+46
			dw	0042h,0041h,0053h,0049h,0043h,4e04h,2f15h,3026h
			dw	4b04h,4f04h,0020h,0042h,0049h,004fh,0053h,002eh
			dw	0052h,004fh,004dh,2c04h,2c29h,572dh,4704h,3904h,0

	ends

