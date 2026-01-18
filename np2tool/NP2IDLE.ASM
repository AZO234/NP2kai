
	ORG		100h
	USE16
	CPU		8086
	
%define	VERSION_ID		"20251103"

START:			jmp		short setup
hlthandler:		out		64h, al ; 少なくともVSYNC割り込みが来るようにする
				sti
				hlt
				cli ; 要らないかも・・・？
				jmp 	far [cs:org_int28]
				
org_int28		dd		0

setup:			cld 		; カウンタインクリメント指示
				push	cs ; CSをスタックに
				pop		ds ; スタックからDSにセット(CS -> DS)
				mov		ah, 9 ; $ 終端文字列表示
				mov		dx, title_msg ; タイトル
				int		21h ; 表示実行
				
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

				; 常駐しておしまい
				mov		ax, 3100h
				mov		dx, setup + 15
				mov		cl, 4
				shr		dx, cl ; bytes -> paragraphs
				int		21h

title_msg		db	"HLT idle driver for np2+dos ("
				db	VERSION_ID
				db	 ")"
				db	13, 10, "$"

