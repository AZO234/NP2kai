
	ORG		0
	USE16
	CPU		8086
	
%define	VERSION_ID		"20251103"

next_dev		dd		-1 ; 次のデバイスドライバなし
attribute		dw		8000h ; キャラクタデバイスということにする
strategy		dw		dev_strategy ; 1st DOS CALL Address
interrupt		dw		setup ; 2nd DOS CALL Address
dev_name		db		'NP2IDLE$'

hlthandler:		out		64h, al ; 少なくともVSYNC割り込みが来るようにする
				sti
				hlt
				cli ; 要らないかも・・・？
				jmp 	far [cs:org_int28]
				
org_int28		dd		0 ; 退避した元のINT28h

reqheader		dd		0 ; リクエストヘッダ

dev_strategy:	mov		word [cs:reqheader + 0], bx ; リクエストヘッダ offset
				mov		word [cs:reqheader + 2], es ; リクエストヘッダ segment
				retf

setup:			push	ax
				push	ds
				push	bx
				push	cx
				push	dx
				push	si
				push	di
				push	bp
				push	es
				
				les		bx, [cs:reqheader] ; リクエストヘッダアドレス
				mov		al, byte [es:bx + 02h] ; AL <- リクエストコード
				cmp		al, 0x0 ; INIT要求か
				jnz		endsetup ; INITでないなら抜ける
				mov		ax, reqheader ; 常駐する終端アドレス reqheaderの直前まで保持すればあとは破棄でOK
				mov		word [es:bx + 0eh], ax ; 常駐領域の終端 offset
				mov		word [es:bx + 10h], cs ; 常駐領域の終端 segment

				cld 	; カウンタインクリメント指示
				push	cs ; CSをスタックに
				pop		ds ; スタックからDSにセット(CS -> DS)
				mov		ah, 9 ; $ 終端文字列表示
				mov		dx, title_msg ; タイトル
				int		21h ; 表示実行

				; ES退避
				push es
				
				; ES <- 0
				xor ax, ax
				mov es, ax
				
				; 元のINT 28hを退避
				mov		ax, 3528h
				int		21h
				mov		[org_int28 + 0], bx
				mov		[org_int28 + 2], es
				
				; INT 28hハンドラを登録
				mov		ax, 2528h
				mov		dx, hlthandler
				int		21h

				; ES戻す
				pop es
				
endsetup:		xor		ax,ax
				les		bx,[cs:reqheader] ; リクエストヘッダアドレス
				mov		word [es:bx + 03h],ax ; ステータス　0=正常終了とする
				
				pop		es
				pop		bp
				pop		di
				pop		si
				pop		dx
				pop		cx
				pop		bx
				pop		ds
				pop		ax
				retf


title_msg		db	"HLT idle driver for np2+dos ("
				db	VERSION_ID
				db	 ")"
				db	13, 10, "$"
