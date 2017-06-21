;
;	DOS I/O
;


include 	mil.inc

.model small
.386

FileFind32		struc
dwFileAttr		dd	?
ftCreate		dd	2 dup(?)
ftLastAccess	dd	2 dup(?)
ftLastWrite		dd	2 dup(?)
nFileSize		dd	2 dup(?)
dwReserved0		dd	2 dup(?)
cFileName		db	230 dup(?)
	reserved0		db	21 dup(?)
	ff16_attr		db	?
	ff16_date		dd	?
	ff16_size		dd	?
cDOSName		db	14 dup(?)
ff32_handle		dw	?
FileFind32		ends

.data

	public	_CHICAGO

	_CHICAGO	db	1
	routp		dw	0
	exedir		db	80 dup(0)
	old24off	dw	0
	old24seg	dw	0
	currentdrv	db	"A:\", 0, 16 dup(0)

.code

	extrn	_strcpy:near

nfunc	chk_kanji
				or		ah, ah
				je		short ret_ascii
				cmp		al, 81h
				jb		short ret_ascii
				cmp		al, 0a0h
				jb		short ret_kanji
				cmp		al, 0e0h
				jb		short ret_ascii
				cmp		al, 0fdh
				jb		short ret_kanji
ret_ascii:		mov		ah, 0
				clc
				ret
ret_kanji:		stc
				ret
endfunc


func	_dosio_init
				mov		ah, 30h						; get dos version
				int		21h
				cmp		al, 7
				jb		short nochigago
				mov		ah, 19h
				int		21h
				add		al, 'A'
				mov		di, offset currentdrv
				mov		[di], al
				mov		dx, di
				add		di, 4
				mov		cx, 16
				xor		bx, bx
				mov		ax, 71a0h					; get volume info
				int		21h
				jc		short nochigago
				cmp		ax, 7100h
				je		short nochigago
				test	bh, 40h
				je		short nochigago
				and		_CHICAGO, not 1
nochigago:		ret
endfunc


func	_file_exname
				push	ds
				pop		es
				push	si
				push	di
				mov		si, [esp+EARG1+4]
				mov		di, [esp+EARG2+4]
				cmp		_CHICAGO, 0
				jne		short frn_nochicago
				mov		byte ptr [di], 0
				mov		ax, 7160h
				mov		cx, [esp+EARG3+4]
				int		21h
				cmp		byte ptr [di], 0
				jne		short frn_end
frn_nochicago:	pushf
				cld
frn_strcpy_lp:	lodsb
				stosb
				or		al, al
				jne		frn_strcpy_lp
				popf
frn_end:		pop		di
				pop		si
				ret
endfunc



nfunc	currentdircat
				mov		ax, [esp+EARG1+2]
				push	ax
				push	routp
				call	_strcpy
				add		sp, 4
				mov		word ptr [esp+EARG1+2], offset exedir
				ret
endfunc


	public	_file_open

func	_file_open_c
				call	currentdircat
_file_open:		mov		dx, [esp+EARG1]
				mov		ax, 3d02h
				int		21h
				jnc		short FopenEnd
				mov		ax, 3d00h
				int		21h
				jnc		short FopenEnd
				mov		ax, -1
FopenEnd:		ret
endfunc


	public	_file_create

func	_file_create_c
				call	currentdircat
_file_create:	mov		dx, [esp+EARG1]
				mov		ax, 3c00h
				xor		cx, cx
				int		21h
				jnc		short FcreateEnd
				mov		ax, -1
FcreateEnd:		ret
endfunc


func	_file_seek
				mov		bx, [esp+EARG1]
				les		dx, [esp+EARG2]
				mov		cx, es
				mov		ax, [esp+EARG4]
				mov		ah, 42h
				int		21h
				jnc		short FseekEnd
				mov		ax, -1
				mov		dx, ax
FseekEnd:		ret
endfunc


func	_file_lread
				push	ds
				push	si
				mov		bx, [esp+EARG1+4]
				mov		ax, 4201h
				xor		cx, cx
				xor		dx, dx
				push	bx
				int		21h
				pop		bx
				jc		short lreaderr

				push	ax
				push	dx
				mov		cx, [esp+EARG4+8]
				sub		cx, 1
				jc		short lreaderr2
				lds		dx, [esp+EARG2+8]
				mov		si, dx
				mov		ah, 3fh
				int		21h
				jnc		short lreadok
lreaderr2:		pop		ax
				pop		ax
lreaderr:		pop		si
				pop		ds
				xor		ax, ax
				ret

lreadok:		mov		cx, ax
				jcxz	short lreaderr2
				xor		dx, dx
lreadlp:		mov		ax, [si]
				cmp		al, 0ah
				je		short lreadlped
				cmp		al, 0dh
				je		short lreadlped2
				inc		dx
				inc		si
				loop	short lreadlp
				jmps	lreadlped
lreadlped2:		cmp		cx, 1
				je		short lreadlped
				cmp		ah, 0ah
				inc		dx
lreadlped:		mov		byte ptr [si], 0
				mov		ax, dx
				inc		ax
				pop		cx
				pop		dx
				add		dx, ax
				adc		cx, 0
				push	ax
				mov		ax, 4200h
				int		21h
				pop		ax
				pop		si
				pop		ds
				ret
endfunc


func	_file_read
				push	ds
				mov		bx, [esp+EARG1+2]
				mov		cx, [esp+EARG4+2]
				lds		dx, [esp+EARG2+2]
				mov		ah, 3fh
				int		21h
				jnc		short FreadEnd
				mov		ax, -1
FreadEnd:		pop		ds
				ret
endfunc


func	_file_write
				push	ds
				mov		bx, [esp+EARG1+2]
				mov		cx, [esp+EARG4+2]
				lds		dx, [esp+EARG2+2]
				mov		ah, 40h
				int		21h
				jnc		short FwriteEnd
				mov		ax, -1
FwriteEnd:		pop		ds
				ret
endfunc


func	_file_close
				mov		bx, [esp+EARG1]
				mov		ah, 3eh
				int		21h
				mov		ah, 0
				setnc	al
				dec		ax
				ret
endfunc


	public	_file_attr

func	_file_attr_c
				call	currentdircat
_file_attr:		mov		dx, [esp+EARG1]
				mov		ax, 4300h
				int		21h
				mov		ax, -1
				jc		short attrnotexist
				mov		ax, cx
attrnotexist:	ret
endfunc


func	_file_setcd
				mov		ax, [esp+EARG1]
				push	ax
				push	offset exedir
				call	_strcpy
				add		sp, 4
				mov		dx, offset exedir
				mov		di, dx
yenchk_lp:		mov		ax, [di]
				inc		di
				or		al, al
				je		short yenchk_ed
				cmp		al, '\'
				jne		short noyenchk
				mov		dx, di
noyenchk:		call	chk_kanji
				jnc		yenchk_lp
				inc		di
				jmps	yenchk_lp
yenchk_ed:		mov		routp, dx
				ret
endfunc




func	_dosdateset
				mov		bx, [esp+EARG1]
				mov		ah, 2ah
				int		21h
				shl		dh, 4
				and		al, 7
				or		al, dh
				mov		[bx+1], al
				mov		ax, cx
				mov		cl, 100
				div		cl
				mov		al, ah
				aam
				shl		ah, 4
				or		al, ah
				mov		[bx], al
				mov		al, dl
				aam
				shl		ah, 4
				or		al, ah
				mov		[bx+2], al
				ret
endfunc


func	_dostimeset
				mov		bx, [esp+EARG1]
				mov		ah, 2ch
				int		21h
				mov		al, ch
				call	set10time
				mov		al, cl
				call	set10time
				mov		al, dh
set10time:		aam
				shl		ah, 4
				or		al, ah
				mov		[bx], al
				inc		bx
				ret
endfunc



func	_int24_set
				push	ds
				mov		ax, 3524h
				int		21h
				mov		old24off, bx
				mov		old24seg, es
				push	cs
				pop		ds
				mov		dx, offset new_24h
				mov		ax, 2524h
				int		21h
				pop		ds
				ret

error24h		dw		0

new_24h:		mov		cs:error24h, di
				add		cs:error24h, 2
				mov		al, 3					; システムコールの失敗
				iret
endfunc


func	_int24_reset
				push	ds
				mov		ax, 2524h
				lds		dx, dword ptr old24off
				int		21h
				pop		ds
				ret
endfunc


; ------------------------------------------------------------------ file find

nfunc	ff_dosname_chk
				lea		si, [di].cFileName
				add		di, cDOSName
				cmp		byte ptr [di], 0
				jne		short ffdosnm_cked
				pushf
				cld
				mov		cx, 13
				rep movsb
				mov		al, 0
				stosb
				popf
ffdosnm_cked:	ret
endfunc


nfunc	ff_cnv16to32
				jnc		short cnv_16to32
				mov		bx, ax
				ret
cnv_16to32:		pushf
				push	si
				cld
				mov		ax, ds
				mov		es, ax
				push	di
				mov		cx, dwReserved0 / 2
				xor		ax, ax
				rep stosw
				pop		di
				lea		si, [di].ff16_attr
				movsb
				add		di, 3
				lodsd
				stosd
				add		di, 4
				stosd
				add		di, 4
				stosd
				add		di, 4
				movsd
				add		di, 12
				mov		cx, 14/2
				rep movsw
				pop		si
				popf
				xor		bx, bx
				ret
endfunc


func	_file_find1st
				mov		dx, [esp+EARG1]		; pathname
				mov		cx, [esp+EARG2]		; attr
				mov		di, [esp+EARG3]		; dta
				mov		[di].ff32_handle, -1

				cmp		_CHICAGO, 0
				jne		short ff1st_nochicago

				push	si
				mov		ax, ds
				mov		es, ax				; es:di = DTA
				mov		si, 1				;    si = dos コンパチ
				mov		ax, 714eh			;    ax = findfirst
				int		21h
				jc		short ff1st_chicago_r
				mov		[di].ff32_handle, ax
				call	ff_dosname_chk
				xor		ax, ax
ff1st_chicago_r:pop		si
				ret

ff1st_nochicago:push	ds
				mov		ah, 2fh
				int		21h
				push	es
				push	bx
				push	dx
				mov		ah, 1ah
				mov		dx, di
				add		dx, reserved0
				int		21h
				pop		dx
				mov		ah, 4eh
				int		21h
				call	ff_cnv16to32
				pop		dx
				pop		ds
				mov		ah, 1ah
				int		21h
				pop		ds
				mov		ax, bx
				ret
endfunc


func	_file_findnext
				mov		di, [esp+EARG1]		; dta

				cmp		_CHICAGO, 0
				jne		short ffnxt_nochicago

				push	si
				mov		ax, ds
				mov		es, ax					; es:di = dta
				mov		ax, 714fh				;    ax = find next
				mov		si, 1					;    si = dos コンパチ
				mov		bx, [di].ff32_handle	;    bx = handle
				int		21h
				jc		short ffnxt_cend
				call	ff_dosname_chk
				xor		ax, ax
ffnxt_cend:		pop		si
				ret

ffnxt_nochicago:push	ds
				mov		ah, 2fh
				int		21h
				push	es
				push	bx
				mov		ah, 1ah
				mov		dx, di
				add		dx, reserved0
				int		21h
				mov		ah, 4fh
				int		21h
				call	ff_cnv16to32
				pop		dx
				pop		ds
				mov		ah, 1ah
				int		21h
				pop		ds
				mov		ax, bx
				ret
endfunc


func	_file_findclose
				cmp		_CHICAGO, 0
				jne		short ffcls_nochicago
				mov		bx, [di].ff32_handle
				cmp		bx, -1
				je		short ffcls_nochicago
				mov		ax, 71a1h
				int		21h
ffcls_nochicago:ret
endfunc


func _sjis2jis
				mov		ax, [esp+EARG1]
				xchg	al, ah
				mov		dx, 211fh
				cmp		al, 40h
				jb		short s2j_err
				cmp		al, 7fh
				jb		short s2j_a
				cmp		al, 80h
				jb		short s2j_err
				inc		dl
				cmp		al, 9fh
				jb		short s2j_a
				mov		dx, 227eh
				cmp		al, 0fch
				ja		short s2j_err
s2j_a:			sub		al, dl
				mov		dl, 81h
				cmp		ah, 81h
				jb		short s2j_err
				cmp		ah, 0a0h
				jb		short s2j_b
				mov		dl, 0e0h
				add		dh, 5fh - 21h
				cmp		ah, 0e0h
				jb		short s2j_err
				cmp		ah, 0efh
				ja		short s2j_err
s2j_b:			sub		ah, dl
				shl		ah, 1
				add		ah, dh

				sub		ah, 20h
				ret
s2j_err:		xor		ax, ax
				ret
endfunc

	end

