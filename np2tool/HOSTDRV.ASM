
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

main:			cld ; カウンタインクリメント指示
				push	cs ; CSをスタックに
				pop		ds ; スタックからDSにセット(CS -> DS)
				mov		ah, 9 ; $ 終端文字列表示
				mov		dx, title_msg ; タイトル
				int		21h ; 実行

				; 機種チェック
				cli ; 割り込み禁止
				call	np2_check ; np2確認
				mov		dx, macerr_msg ; 機種エラーメッセージ用意
				jne		short errexit_lv2 ; エラーなら終了

				mov		ax, 352fh ; 現在のINT 2Fh のベクタを取得
				int		21h ; 実行
				mov		si, id ; IDがHOSTDRVのものかを確認
				mov		di, si
				mov		cl, 4 ; WORDx4つまり8文字比較
				repe cmpsw ; 文字列比較
				jne		short stay ; 常駐処理へ飛ぶ
				jmp		release ; 既に常駐済み


stay:			mov		si, 0080h ; コマンドライン引数取得
				mov		dx, usage_msg ; コマンドライン引数エラーメッセージ用意
				xor		cx, cx
				lodsb
				add		cl, al ; 引数の長さ　-> CL
				je		short stay_error ; 引数指定がない（0文字）のでエラー
cmdlnchk:		lodsb
				cmp		al, 20h
				ja		short drvcheck ; スペース文字以外が現れたら処理続行
				loop	cmdlnchk
stay_error:		jmp		errexit_lv1 ; スペースが現れないのでエラー

drvcheck:		and		al, 0dfh ; 小文字 -> 大文字へ変換
				sub		al, 'A' ; Aから順に0, 1, 2, ...
				cmp		al, 26 ; Zより後ならエラー
				mov		dx, drverr0_msg ; ドライブ文字エラーメッセージ
				jae		short stay_error ; エラーなら終了

				mov		[inf + HDRVIF.drive_no], al ; ドライブ番号保存
				mov		ah, 30h ; DOSバージョン取得
				int		21h ; 実行
				mov		[inf + HDRVIF.dosver], ax ; DOSバージョン保存
				xchg	al, ah
				cmp		ax, 030ah ; DOSバージョンが3.1以上か？
				mov		dx, doserr_msg ; DOSバージョンエラーメッセージ
				jc		short stay_error ; DOSバージョンが駄目ならエラー
				mov		ax, 1100h					; HOSTDRV常駐前応答チェック
				int		2fh
				cmp		ax, 1 ; not installed, not OK to install かを判定
				mov		dx, cntsty_msg ; インストール不可エラーメッセージ
				je		short stay_error ; エラーなら終了
				mov		ax, 5d06h					; SDAアドレス取得
				int		21h
				mov		[cs:inf + HDRVIF.sda_off], si ; SDA offset保存
				mov		[cs:inf + HDRVIF.sda_seg], ds ; SDA segment保存
				push	cs
				pop		ds

				; バージョンチェック
				mov		si, stay_param
				call	sendnp2port ; NULL文字まで送る（文字列check_hostdrvが送られる）
				call	checknp2port ; NULL文字まで比較（文字列0.74と比較）
				mov		dx, np2err_msg ; バージョンエラーメッセージ準備
				jne		short stay_error ; エラーなら終了

				; assign_cds CDSの必要項目を格納
				call	getcds
				mov		dx, drverr1_msg
				jc		short stay_error ; ドライブ番号管理外エラー
				test	byte [es:di + CDS.flag + 1], 0c0h
				mov		dx, drverr2_msg
				jne		short stay_error ; ドライブ番号使用中エラー

				; 新プロトコルサポート確認　マウント前は新プロトコル有効化せずに文字列pokだけが返る
				mov		si, enable_newp
				call	sendnp2port ; NULL文字まで送る（文字列setn_hostdrvが送られる）
				call	checknp2port ; NULL文字まで比較（文字列pokと比較）
				jne		short protocol_old ; エラーなら旧プロトコル
				
				; 新プロトコル
				mov		al, [inf + HDRVIF.drive_no]
				add		al, 'A'
				mov		byte [stay_type2 + 1], al
				mov		si, stay_type2 ; SIを新プロトコル用に設定
				jmp		protocol_end
				
				; 旧プロトコル
protocol_old:	mov		si, stay_type1 ; SIを旧プロトコル用に設定
				
				; CDS設定の続き
protocol_end:	xor		cx, cx
				lodsb
				mov		cl, al
				or		word [es:di + CDS.flag], 0c080h
				mov		[es:di + CDS.root], cx
				rep movsb
				mov		si, stay_param2
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

				; 新プロトコル有効化
				push	si
				mov		si, enable_newp
				call	sendnp2port ; NULL文字まで送る（文字列setn_hostdrvが送られる）
				call	checknp2port ; NULL文字まで比較（文字列pokと比較）失敗しても無視
				jne		short protocol_skip ; エラーなら旧プロトコル
				mov		ah, 9 ; $ 終端文字列表示
				mov		dx, newpro_msg ; 新プロトコル表示文字列
				int		21h
protocol_skip:	pop		si
				
				sti
				mov		al, [inf + HDRVIF.drive_no] ; ドライブ番号 -> AL
				add		byte [sty_drive], al ; ドライブ文字書き込み
				mov		ah, 9 ; $ 終端文字列表示
				mov		dx, si ; 割当先ドライブ表示文字列
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
stay_type1		db	 9, "¥¥HOSTDRV" ; 旧np2互換
stay_type2		db	 2, "A:" ; 新プロトコル
stay_param2		db		"¥", 0
				db	12, "open_hostdrv"
				db	 3, "ok",0
				db	"assigned host-drive to "
sty_drive		db	"A:", 13, 10, "$"

enable_newp		db	12, "setn_hostdrv"
				db	 4, "pok",0


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
newpro_msg		db	"new protocol enabled"
				db	13, 10, "$"

	ends
