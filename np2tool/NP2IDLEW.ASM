
	ORG		100h
	USE16
	CPU		8086
	
%define	VERSION_ID		"20260408"

%define	HLTFLAG_WINDOWS		01h

START:			jmp		short setup

hlthandler:		test	byte [cs:hlt_flags], HLTFLAG_WINDOWS
				jz		.do_hlt
				push	ax
				push	bx
				mov		ax, 1683h
				int		2fh
				cmp		[cs:system_vm_id], bx
				pop		bx
				pop		ax
				jne		.skip_hlt_init
				cmp		byte [cs:system_vm_count], 0
				jne		.skip_hlt
.do_hlt:
				out		64h, al ; 少なくともVSYNC割り込みが来るようにする
				sti
				hlt
				cli ; 要らないかも・・・？
				jmp 	far [cs:org_int28]
.skip_hlt_init:
				mov		byte [cs:system_vm_count], 255
				jmp		.skip_hlt_chain
.skip_hlt:
				cmp		byte [cs:system_vm_count], 0
				je		.skip_hlt_chain
				dec		byte [cs:system_vm_count]
.skip_hlt_chain:
				jmp 	far [cs:org_int28]
				
org_int28		dd		0

int2f_handler:
				; --- Windows Windows Initialization Complete Notification INT 2Fh (AX=1608h) ---
				cmp		ax, 1608h
				je		.wininit

				; --- Windows Termination Notification INT 2Fh (AX=1606h) ---
				cmp		ax, 1606h
				je		.winterm

.chain:
				jmp		far [cs:org_int2f]

.wininit:
				or		byte [cs:hlt_flags], HLTFLAG_WINDOWS
				push	ax
				push	bx
				mov		ax, 1683h
				int		2fh
				mov		[cs:system_vm_id], bx ; System VM ID
				pop		bx
				pop		ax
				jmp		.chain

.winterm:
				and		byte [cs:hlt_flags], ~HLTFLAG_WINDOWS
				jmp		.chain

org_int2f		dd		0

system_vm_id	dw		1
system_vm_count	db		0
hlt_flags		db		0

setup:			cld ; カウンタインクリメント指示
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

				; 元のINT 2fhを退避
				mov		ax, 352fh
				int		21h
				mov		[org_int2f + 0], bx
				mov		[org_int2f + 2], es
				
				; INT 2fhハンドラを登録
				mov		ax, 252fh
				mov		dx, int2f_handler
				int		21h

				; ENVは使わないので先に捨ててOK
				mov		ax, [002ch] ; ENV セグメント取得
				or		ax, ax
				jz		.skip_env ; ENVがなかったらスキップ
				mov		es, ax
				mov		ax, 4900h
				int		21h ; ENV 解放
				mov		word [002ch], 0 ; 一応0にしておく
.skip_env:

				; 常駐しておしまい
				mov		ax, 3100h
				mov		dx, setup + 15
				mov		cl, 4
				shr		dx, cl ; bytes -> paragraphs
				int		21h

title_msg		db	"HLT idle driver for np2+dos+win ("
				db	VERSION_ID
				db	 ")"
				db	13, 10, "$"

