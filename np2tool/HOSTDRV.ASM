
	ORG		100h
	USE16
	CPU		186


%include	'np2tool.inc'
%include	'hostdrv.inc'

START:			jmp		short keep_addr
id				db		'host_drv'

int2f:			cmp		ah, 11h
				jne		short i2f_chain
				cmp		al, 2eh
				ja		short i2f_chain
				pusha
				push	ds
				push	es
				mov		bp, sp

				mov		cl, al
				shr		al, 3
				and		cl, 7
				inc		cl
				mov		bx, i2f_nochkbmp
				db		FIXCS
				xlat
				rcr		al, cl
				jc		short i2f_callnp2
				mov		ax, 1223h	; check char dev
				int		2fh

i2f_callnp2:	pushf
				mov		ax, 1218h
				int		2fh
				popf
				mov		bx, si
				mov		si, intr_hostdrv
				mov		dx, NP2PORT
				mov		cx, 12
				cld
				db		FIXCS
				rep		outsb
				jnc		short i2f_end
				pushf
				mov		ax, 120ch	; set sft owner
				int		2fh
				popf
i2f_end:		pop		es
				pop		ds
				popa
				jne		short i2f_chain
				iret

i2f_chain:		db		0eah
org_int2f		dd		0

inf				db		0
				dw		0
				dw		0
				dw		0

;						76543210   fedcba98
i2f_nochkbmp	db		11010101b, 00111111b
				db		00110101b, 11110111b
				db		11111111b, 10111111b

intr_hostdrv	db		'intr_hostdrv'


; ----

keep_addr:		jmp		short main

errexit_lv2:	sti
				mov		ah, 9
				int		21h
				mov		dx, bx
errexit_lv1:	sti
				mov		ah, 9
				int		21h
				mov		dx, str_crlf
				mov		ah, 9
				int		21h
				mov		ax, 4c01h
				int		21h

main:			cld
				push	cs
				pop		ds
				mov		ah, 9
				mov		dx, title_msg
				int		21h

				; 機種チェック
				cli
				call	np2_check
				mov		dx, macerr_msg
				jne		short errexit_lv2

				mov		ax, 352fh
				int		21h
				mov		si, id
				mov		di, si
				mov		cl, 4
				repe cmpsw
				jne		short stay
				jmp		release


stay:			mov		si, 0080h
				mov		dx, usage_msg
				xor		cx, cx
				lodsb
				add		cl, al
				je		short stay_error
cmdlnchk:		lodsb
				cmp		al, 20h
				ja		short drvcheck
				loop	cmdlnchk
stay_error:		jmp		errexit_lv1

drvcheck:		and		al, 0dfh
				sub		al, 'A'
				cmp		al, 26
				mov		dx, drverr0_msg
				jae		short stay_error

				mov		[inf + HDRVIF.drive_no], al
				mov		ah, 30h
				int		21h
				mov		[inf + HDRVIF.dosver], ax
				xchg	al, ah
				cmp		ax, 030ah
				mov		dx, doserr_msg
				jc		short stay_error
				mov		ax, 1100h					; inst check
				int		2fh
				cmp		ax, 1
				mov		dx, cntsty_msg
				je		short stay_error
				mov		ax, 5d06h					; get SDA addr
				int		21h
				mov		[cs:inf + HDRVIF.sda_off], si
				mov		[cs:inf + HDRVIF.sda_seg], ds
				push	cs
				pop		ds

				mov		si, stay_param
				call	sendnp2port
				call	checknp2port
				mov		dx, np2err_msg
				jne		short stay_error

				; assign_cds
				call	getcds
				mov		dx, drverr1_msg
				jc		short stay_error
				test	byte [es:di + CDS.flag + 1], 0c0h
				mov		dx, drverr2_msg
				jne		short stay_error

				xor		cx, cx
				lodsb
				mov		cl, al
				or		word [es:di + CDS.flag], 0c080h
				mov		[es:di + CDS.root], cx
				rep movsb
				movsw

				mov		bx, inf
				mov		di, cs
				xor		cx, cx
				mov		es, cx
				xchg	bx, [es:0600h]
				xchg	di, [es:0602h]
				call	sendnp2port
				call	checknp2port
				mov		[es:0600h], bx
				mov		[es:0602h], di
				jne		short ass_err

				mov		ax, 352fh
				int		21h
				mov		[org_int2f + 0], bx
				mov		[org_int2f + 2], es
				mov		ax, 252fh
				mov		dx, int2f
				int		21h

				mov		cl, 13
				mov		dx, str_user
				mov		ax, 5e01h
				int		21h

				sti
				mov		al, [inf + HDRVIF.drive_no]
				add		byte [sty_drive], al
				mov		ah, 9
				mov		dx, si
				int		21h
				mov		ax, 3100h
				mov		dx, keep_addr + 15
				mov		cl, 4
				shr		dx, cl
				int		21h

ass_err:		call	getcds
				mov		word [es:di + CDS.flag], 0
				mov		dx, styerr_msg
				jmp		errexit_lv1



release:		mov		ax, 252fh
				lds		dx, [es:org_int2f]
				int		21h
				push	es
				pop		ds
				call	getcds
				mov		word [es:di + CDS.flag], 0
				mov		es, [002ch]
				mov		ah, 49h
				int		21h
				push	ds
				pop		es
				mov		ah, 49h
				int		21h
				push	cs
				pop		ds
				mov		si, rel_param
				call	sendnp2port
				sti
				mov		ah, 9
				mov		dx, si
				int		21h
				mov		ax, 4c00h
				int		21h



getcds:			mov		ax, 5200h	; get lol addr
				int		21h
				mov		al, [inf + HDRVIF.drive_no]
				cmp		al, [es:bx + LOL.lastdrv]
				jae		short gcds_err
				les		di, [es:bx + LOL.cds]
				mov		ah, CDS.size
				cmp		byte [inf + HDRVIF.dosver], 3
				je		short gcds_calc
				add		ah, 7
gcds_calc:		mul		ah
				add		di, ax
				ret
gcds_err:		stc
				ret


%include	'np2tool.x86'

title_msg		db	"host-drive driver for np2+dos ("
				db	VERSION_ID
				db	 ")"
str_crlf		db	13, 10, "$"
str_user		db	"NekoProjectII  "


stay_param		db	13, "check_hostdrv"
				db	 5, "0.74",0
				db	 9, "\\HOSTDRV"
				db		"\", 0
				db	12, "open_hostdrv"
				db	 3, "ok",0
				db	"assigned host-drive to "
sty_drive		db	"A:", 13, 10, "$"


rel_param		db	13, "close_hostdrv"
				db	"unassigned", 13, 10, "$"


usage_msg		db	"usage: hostdrv.com [drive letter]$"
macerr_msg		db	"Illegal hardware - $"
doserr_msg		db	"unsupported dos version$"
np2err_msg		db	"unsupported np2 version$"
cntsty_msg		db	"cannot stay$"
drverr0_msg		db	"drive is bad$"
drverr1_msg		db	"drive higher than last drive$"
drverr2_msg		db	"drive already assigned$"
styerr_msg		db	"cannot assign$"

	ends

