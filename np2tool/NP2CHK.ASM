
	ORG		100h
	USE16
	CPU		8086

START:			xor		ax, ax
				cli
				
				; 機種チェック
				cld
				mov		dx, 07EFh
				xor		cx, cx
				call	CheckNP2 ; np2確認
				and		al, al
				jz		.finish ; np2ではない
				
				; Neko Project 21/Wチェック
				call	Check21W
.finish:
				sti
				mov		ah, 4Ch
				int		21h

; ----------------------------------------------------------
; CheckNP2:　Neko Project II系 かを確認
;   set AL=1 if Neko Project II series
; ----------------------------------------------------------
CheckNP2:
				mov		si, cmd_np2 ; 文字列NP2を送る
				push	si
				mov		cl, 3
.lp:			lodsb
				out		dx, al
				loop	.lp
				pop		si ; 文字列NP2と比較
				mov		cl, 3
.lp2:			in		al, dx
				cmp		al, [si]
				jne		.end
				inc		si
				loope	.lp2
				mov		al, 1
				ret
.end:
				xor		al, al
				ret

; ----------------------------------------------------------
; Check21W:　Neko Project 21/W かを確認
;   inc AX if Neko Project 21/W
; ----------------------------------------------------------
Check21W:
				and		dl, ~02h ; 07EFh -> 07EDh
				push	ax
				mov		al, 02h
				out		dx, al
				or		dl, 02h ; 07EDh -> 07EFh
				mov		si, cmd_np2 ; 文字列NP2+getconfigを送る
				mov		cl, 12
.lp:			lodsb
				out		dx, al
				loop	.lp
				in		al, dx
				cmp		al, 'N' ; サポートなしならNP2コマンドの応答が来る
				pop		ax
				jne		short .yes
				ret
.yes:
				inc		al
				ret

; サポートしているかのテスト用
cmd_np2			db	'NP2'
cmd_getconfig	db	'getconfig'
