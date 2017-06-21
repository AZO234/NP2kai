
	ORG		0
	USE16
	CPU		8086

FIXCS		equ		2eh

segment .hddboot start=0x0000

START:			jmp		short main
				nop
				nop
				db		"IPL1", 0, 0, 0, 0x1e

main:			mov		ax, 0x0a04
				int		0x18
				mov		ah, 0x16
				mov		dx, 0xe120
				int		0x18
				cli
				cld
				mov		ax, 0xa000
				mov		es, ax
				mov		si, nosysmsg
				xor		di, di
				call	putmsg
				mov		di, 160
				call	putmsg
.hlt:			hlt
				jmp		short .hlt

putmsg:			db		FIXCS
				lodsw
				test	ax, ax
				je		short .end
				stosw
				inc		di
				inc		di
				jmp		short putmsg
.end:			ret


				; このハードディスクイメージはフォーマットされていません．
nosysmsg	dw	3304h,4e04h,4f05h,3c01h,4905h,4705h,2305h,3905h
			dw	2f05h,2405h,6105h,3c01h,3805h,4f04h,5505h,2905h
			dw	3c01h,5e05h,4305h,4805h,3504h,6c04h,4604h,2404h
			dw	5e04h,3b04h,7304h,2501h, 0

				; ディスクイメージを挿入後，リセットして下さい．
			dw	4705h,2305h,3905h,2f05h,2405h,6105h,3c01h,3805h
			dw	7204h,5e21h,7e26h,6518h,2401h,6a05h,3b05h,4305h
			dw	4805h,3704h,4604h,3c12h,3504h,2404h,2501h,0

	ends

