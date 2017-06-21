
CODE	SEGMENT
			ASSUME CS:CODE
ifndef MSDOS
			ORG 0000h
else
			ORG 0100h
endif

include	itf.inc
include	pc98.inc
include	dataseg.inc
include	keyboard.inc
include	process.mac
include	debug.mac

dipitem		struc
items		db	?
item_y		db	?
item_mask	db	?
item_shr	db	?
item_x		db	4 dup(?)
item_title	dw	?
item_info	dw	4 dup(?)
item_sw		dw	4 dup(?)
dipitem		ends


START:			jmp		short vxmain
				dw		offset dipswflag
				mov		dl, 1
				jmp		short main
				mov		dl, 2
				jmp		short main

vxmain:			mov		dl, 0
main:			cli
				cld
				mov		ax, 0030h
				mov		ss, ax
				mov		sp, 00feh

				mov		ax, DATASEG
				mov		ds, ax
				mov		ds:[MACTYPE], dl

ifdef DEBUG_INIT
				DEBUG_INIT
endif
				KEYINT_INIT

				xor		ax, ax
				mov		es, ax
				test	byte ptr es:[0531h], 80h
				je		short main2
				mov		byte ptr ds:[KEYSTAT], 0ffh

main2:			call	SCREEN_CLEAR

				mov		ah, 24
key1cklp:		call	boot_keycheck
				sub		al, BOOT_WFLASH
				jae		short boot_ext
				call	WAITVSYNC1
				dec		ah
				jne		key1cklp

;				jmp		ssp_start

				call	BEEP_TEST

				mov		ah, 32
key2cklp:		call	boot_keycheck
				cmp		al, BOOT_MENU
				je		short boot_menujmp
				jb		short key2cklp2
				mov		al, 0
key2cklp2:		call	WAITVSYNC1
				dec		ah
				jne		key2cklp
				jmps	boot_normalprc

boot_menujmp:	jmp		ssp_start


bootproc		dw	offset FLASH_WRITE
				dw	offset FLASH_CLEAR
				dw	offset RESET_ALLSW

boot_ext:		xor		dx, dx
				mov		ah, 0
				add		ax, ax
				mov		bx, ax
				call	cs:bootproc[bx]
				mov		al, 90
				call	WAITVSYNC
				jmp		REBOOT_PROCESS

firmproc		dw	offset CPUMODE_DISP
				dw	offset BIOS_REVISON
				dw	offset FIRMWARE_TEST

boot_normalprc:	push	ax
				in		al, 31h
				test	al, 10h
				CALLNZ	MEMSW_INIT

				mov		cl, 0e1h
				xor		dx, dx
				pop		bx

				mov		al, ds:[MACTYPE]
				cmp		al, 0
				je		short necmemchk
				dec		al
				jne		short retbioswait

				call	epson_memtest
				jmp		short retbioswait

necmemchk:		add		bx, bx
				call	cs:firmproc[bx]
				call	MEMORY_TEST

retbioswait:	mov		ah, 90
				call	WAITVSYNC

exitprocess:	cli
				pushf
ifdef DEBUG_TERM
				DEBUG_TERM
endif
				KEYINT_TERM
				call	SCREEN_CLEAR
				mov		di, int09off + 8000h
				mov		cx, (DATASEGEND - int09off) / 2
				xor		ax, ax
				rep stosw
				popf
				ITF_EXIT



include resource.x86
include itfsub.x86
include textdisp.x86
include	keyboard.x86
include	np2.x86
include	dipsw.x86
include	memsw.x86
include	beep.x86
include	firmware.x86
include memchk.x86

; SSP
include ssp_res.x86
include ssp_sub.x86
include ssp.x86
include	ssp_dip.x86
include	ssp_msw.x86

; ---------------------------------------------------------------------------

disptextjis:	push	dx
				push	di
				call	TEXTOUT_CS
				pop		di
				pop		dx
				ret


setdips:		push	bx
				push	cx
				mov		dl, al
				mov		dh, 0
				mov		di, dx
				mov		ch, [di+DIPSW_1]
				mov		di, 160*20+20
				mov		cl, 4
				shl		dx, cl
				add		di, dx
				shl		dx, 1
				add		di, dx

				mov		cl, 80h
dirput_lp:		mov		bx, 0a1h
				test	ah, cl
				je		dipcured
				mov		bl, 0e5h
dipcured:		mov		dx, 2101h
				test	ch, cl
				je		diphited
				mov		dx, 2202h
				xchg	bl, bh
diphited:		mov		es:[di+0000h], dx
				mov		es:[di+2000h], bh
				mov		es:[di+2002h], bh
				xor		dx, 0303h
				mov		es:[di+00a0h], dx
				mov		es:[di+20a0h], bl
				mov		es:[di+20a2h], bl
				add		di, 4
				shr		cl, 1
				jne		dirput_lp
				pop		cx
				pop		bx
				ret


dipitemadrs:	mov		bx, dx
				and		bx, 3
				mov		si, bx
				add		si, bx
				mov		si, cs:dipitems[si]
				mov		al, type dipitem
				mul		dh
				add		si, ax
				ret

setdipitem:		push	bx
				push	dx
				push	si
				push	di
				call	dipitemadrs
				mov		cl, cs:[si].item_shr
				shl		ch, cl
				mov		cl, cs:[si].item_mask
				and		ch, cl
				not		cl
				and		[bx+DIPSW_1], cl
				or		[bx+DIPSW_1], ch
				jmp		short dispitemmov

dispdipitem:	push	bx
				push	dx
				push	si
				push	di
				call	dipitemadrs
				mov		ch, [bx+DIPSW_1]
				and		ch, cs:[si].item_mask
dispitemmov:	push	bx
				mov		cl, cs:[si].item_shr
				shr		ch, cl
				mov		cl, 0a1h
				test	dl, 080h
				je		putitems_pos
				mov		cl, 0e5h
putitems_pos:	mov		dl, 4
				mov		dh, cs:[si].item_y
				push	si
				mov		si, cs:[si].item_title
				call	disptextjis
				pop		si
				and		cl, not 4
				xor		bx, bx
putitems_lp:	cmp		bl, cs:[si].items
				jae		short putitems_info
				mov		dl, cs:[si+bx].item_x
				add		bl, bl
				push	si
				mov		si, cs:[si+bx].item_sw
				shr		bl, 1
				cmp		ch, bl
				jne		short nocurs
				or		cl, 4
nocurs:			call	disptextjis
				and		cl, not 4
				pop		si
				inc		bl
				jmp		short putitems_lp
putitems_info:	pop		bx
				cmp		cl, 0e1h
				jne		putitems_ed
				push	cx
				mov		di, 17*160
				mov		cx, 0150h
				call	boxclear
				pop		cx
				mov		ah, cs:[si].item_mask
				mov		al, bl
				call	setdips
				mov		bl, ch
				mov		dl, cs:[si+bx].item_x
				add		bl, bl
				mov		dx, 1100h
				push	si
				mov		si, cs:[si+bx].item_info
				call	disptextjis
				pop		si
putitems_ed:	mov		cl, cs:[si].items
				pop		di
				pop		si
				pop		dx
				pop		bx
				ret

dippage_set:	push	dx
				mov		dh, dl
				and		dx, 0300h
				add		dx, 3103h
				mov		word ptr es:[160*1+2fh*2], dx
				mov		ax, 0h
				call	setdips
				mov		ax, 1h
				call	setdips
				mov		ax, 2h
				call	setdips
				mov		di, 2*160+2
				mov		cx, 0e4eh
				call	boxclear
				pop		dx
				push	dx
				and		dx, 7fh
itemdisplps:	call	dispdipitem
				inc		dh
				or		cl, cl
				jne		itemdisplps
				mov		bl, dh
				pop		dx
				cmp		dh, bl
				jb		short curitemputs
				mov		dh, bl
				dec		dh
curitemputs:	or		dl, 80h
				call	dispdipitem
				ret





; ---------------------------------------------------------------------------

REBOOT_PROCESS:
ifdef NP2
				cli
				mov		si, offset np2str_hwreset
				call	sendnp2port
hltlp:			hlt
				jmp		short hltlp
endif
				jmp		exitprocess

CODE	ENDS
	END START

