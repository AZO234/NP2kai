KEYVECT		equ		9

; 一回しかコールしない物は マクロにする


; キーボード処理開始
KEYINT_INIT		macro
				cli
				xor		ax, ax
				mov		es, ax
				mov		ax, offset keyint_proc
				xchg	ax, es:[KEYVECT*4+0]
				mov		ds:[int09off], ax
				mov		ax, cs
				xchg	ax, es:[KEYVECT*4+2]
				mov		ds:[int09seg], ax
ifndef DEBUG
				in		al, 02h
				mov		ds:[IMR0], al
				in		al, 0ah
				mov		ds:[IMR1], al
				mov		al, 0fdh
				out		02h, al
				mov		al, 0ffh
				out		0ah, al

				mov		al, 0
				mov		cx, 11h
				mov		di, MEM_KB_KY_STS
				rep stosb
				mov		al, 3ah
				out		43h, al
				mov		cx, 20
				loop	$
				mov		al, 32h
				out		43h, al
endif
				sti
				endm


; キーボード終了 / CLIでコールされる筈
KEYINT_TERM		macro
				xor		ax, ax
				mov		es, ax
				mov		ax, ds:[int09off]
				mov		es:[KEYVECT*4+0], ax
				mov		ax, ds:[int09seg]
				mov		es:[KEYVECT*4+2], ax
ifndef DEBUG
				mov		al, ds:[IMR0]
				out		02h, al
				mov		al, ds:[IMR1]
				out		0ah, al
endif
				endm

