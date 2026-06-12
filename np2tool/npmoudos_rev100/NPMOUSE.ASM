
	ORG		100h
	USE16
	CPU		8086

%include	'np2tool.inc'

%define	VERSION_ID		"20260225"

START:			jmp		setup
NP2_MOUSESIG	db 		"*** This is Copyright 2026 NekoDevT. ***"
int33_handler:
				cmp		ax, 0000h
				je		resetdriver
                
				cmp		ax, 0001h
				je		showcursor
                
				cmp		ax, 0002h
				je		hidecursor
                
				cmp		ax, 0003h
				je		getpos
                
                push	ax
                sub		al, [cs:setrange_ofs]
				cmp		al, 2
				jb		set_range
                pop		ax
				
				cmp		ax, 000ch
				je		set_int_handler
                
int33_chain:
				jmp		far [cs:org_int33]
        		
; ----- AX=0000h RESET DRIVER AND READ STATUS -----
resetdriver:	; リセット時にはホストのマウスカーソルを出す
				call	SendShowHideCursor ; ALが初めから0なのでこれでよい
				jmp		int33_chain

; ----- AX=0001h SHOW MOUSE CURSOR -----
showcursor:		; ホストのマウスカーソルを消す or DOSマウスカーソル表示を無視
				call	SendShowHideCursor ; ALが初めから1なのでこれでよい
				jmp		int33_chain
				iret

; ----- AX=0002h HIDE MOUSE CURSOR -----
hidecursor:		; ホストのマウスカーソルを出す
				xor		al, 02h ; AL=0にしたいのでbit1を反転させて消す
				call	SendShowHideCursor ; 消え放しを防ぐために表示は常に行う
				xor		al, 02h ; 反転させて戻す
				jmp		int33_chain

; ----- AX=0003h RETURN POSITION AND BUTTON STATUS -----
; ポーリング方式で座標を取得するプログラム用
getpos:
				; 元のルーチンを先に呼ぶ
				pushf
				call	far [cs:org_int33]

				; 必要に応じて絶対座標へ書き換える
				call	ReadAndAdjustXY
				
				iret

; ----- AX=0007h, 0008h DEFINE HORIZONTAL/VERTICAL CURSOR RANGE -----
; ----- AX=0010h, 0011h DEFINE HORIZONTAL/VERTICAL CURSOR RANGE (NEC) -----
; ここに飛んできたとき、ALからオフセットが引かれて AL=0:横、AL=1:縦の状態になっている。
set_range:
				; 大小が逆の場合は入れ替え
				cmp		dx, cx
				jae		.x_ok
				xchg	dx, cx
.x_ok:
				push	di
				
				mov		di, x_min
				and		al, al ; AX=0007h,0010h(NEC) DEFINE HORIZONTAL CURSOR RANGE
				jz		.is_x
				add		di, 4  ; AX=0008h,0011h(NEC) DEFINE VERTICAL CURSOR RANGE
.is_x:
				mov		[cs:di], cx ; min
				mov		[cs:di + 2], dx ; max
				
				pop		di
.tochain:
                pop		ax ; chainの前にオフセットを戻す
				jmp		int33_chain

; ----- AX=000ch DEFINE INTERRUPT SUBROUTINE PARAMETERS -----
; コールバック方式で座標を取得するプログラム用
set_int_handler:
				; 元の処理を記憶
				mov		[cs:org_int_handler + 0], dx
				mov		[cs:org_int_handler + 2], es
				
				push	dx
				push	es
				
				; ラッパーに飛ばす
				push	cs
				pop		es ; ES <- CS
				mov		dx, int_handler
				
				pushf
				call	far [cs:org_int33]

				pop		es
				pop		dx
				
				iret
				
int_handler:
				; 必要に応じて絶対座標へ書き換える
				call	ReadAndAdjustXY
				
				; 元の処理を呼び出し
				jmp		far [cs:org_int_handler]

; ---------------------------------------------------------------
; ねこーマウス座標を取得し、マウスドライバの座標系へ換算し、
; 変更がある場合はマウスドライバへ座標を書き戻す
; 絶対座標が取れない場合は元のCX, DXを維持する
; returns: CX=absX, DX=absY
; ---------------------------------------------------------------
ReadAndAdjustXY:
				push	ax
				push	bx
				push	si
				push	di
				push	dx
				push	cx

				; DIは座標変更があったかの判定用
				xor		ax, ax
				mov		di, ax

				; 絶対座標を読み出し
				call	ReadAbsXY ; CF=1 なら AX=X絶対座標, DX=Y絶対座標 が入る　座標値は画面全体幅・高さを0～65535でマップ
				jnc		.use_original ; CF=0 なら元の座標値を保持

				mov		si, dx ; DX(Y絶対座標)を一旦SIへ退避
				
				; DOSマウス座標系への変換 X = (X絶対座標 * DOSマウス画面幅) / 65535
				mov		bx, ax ; BX = X絶対座標
				mov		ax, 639 ; AX = DOSマウス画面幅 = 639固定
				xor		dx, dx
				mul		bx ; DX:AX = DOSマウス画面幅 * X絶対座標
				mov		bx, 65535
				div		bx ; AX = (DOSマウス画面幅 * X絶対座標) / 65535
				; 値の範囲を最小～最大の範囲に調整
				cmp		ax, [cs:x_min]
				jae		.xmin_ok
				mov		ax, [cs:x_min]
.xmin_ok:
				cmp		ax, [cs:x_max]
				jbe		.xmax_ok
				mov		ax, [cs:x_max]
.xmax_ok:
				pop		cx
				cmp		cx, ax
				je		.skipx
				mov		cx, ax ; CX = DOSマウス座標X
				inc		di ; 変更があればDIをインクリメント
.skipx:

				; DOSマウス座標系への変換 Y = (Y絶対座標 * DOSマウス画面高さ) / 65535
				; 画面の縦の長さを取得 -> AXへ
				push	ds
				xor		ax, ax
				mov		ds, ax
				mov		al, [0597h]
				pop		ds
				and		al, 3
				cmp		al, 2
				jb		.h199
				je		.h399
				mov		ax, 479 ; AX = DOSマウス画面高さ = 479
				jmp		.store
.h399: 
				mov		ax, 399 ; AX = DOSマウス画面高さ = 399
				jmp		.store
.h199: 
				mov		ax, 199 ; AX = DOSマウス画面高さ = 199
.store:
				mov		bx, si ; BX = Y絶対座標（SIに退避していた値を戻す）
				xor		dx, dx
				mul		bx ; DX:AX = DOSマウス画面高さ * Y絶対座標
				mov		bx, 65535
				div		bx ; AX = (DOSマウス画面幅 * X絶対座標) / 65535
				; 値の範囲を最小～最大の範囲に調整
				cmp		ax, [cs:y_min]
				jae		.ymin_ok
				mov		ax, [cs:y_min]
.ymin_ok:
				cmp		ax, [cs:y_max]
				jbe		.ymax_ok
				mov		ax, [cs:y_max]
.ymax_ok:
				pop		dx
				cmp		dx, ax
				je		.skipy
				mov		dx, ax ; DX = DOSマウス座標Y
				inc		di ; 変更があればDIをインクリメント
.skipy:

				test	di, di
				jz		.exit_handler ; DIが0なら座標変更無しなので抜ける
				
				; マウスドライバへ絶対座標を書き戻しに行く
				mov		ax, 04h
				pushf
				call	far [cs:org_int33]
				jmp		.exit_handler
.use_original:
				pop		cx
				pop		dx
.exit_handler:
				pop		di
				pop		si
				pop		bx
				pop		ax
				ret

; ---------------------------------------------------------------
; ねこーマウス座標取得コマンド発行 
; success: CF=1, BX=X, CX=Y
; returns on success: CF=1, AX=絶対座標X, DX=絶対座標Y
; returns on failure: CF=0, AX/DX undefined
; 絶対座標は画面全体幅・高さを0～65535にマップした値
; ---------------------------------------------------------------
ReadAbsXY:
				push	bx
				push	cx

				; 絶対座標を取得する
				push	ds
				push	cs
				pop		ds ; DS <- CS
				mov		dx,07EDh
				mov		al,02h ; ねこマウス mode=2 レジスタへ格納、成否はCFで
				out		dx,al
				mov		dx,07EFh
				mov		si, NP2_CMD_GETMPOS
.getmpos_loop:
			    lodsb
			    cmp		al, '$'
			    je		.getmpos_done
				clc
			    out		dx, al
				jc		.ok ; CF=1ならコマンド実行成功
			    jmp		.getmpos_loop
.getmpos_done:
				; CF=0なのでコマンド実行失敗
    			pop		ds  ; DS を元に戻す
				pop		cx
				pop		bx
				clc
				ret

.ok:
				; コマンド実行成功
    			pop		ds  ; DS を元に戻す

				; BX/CX を AX/DX へ送る
				mov		ax, bx                  ; AX = 絶対座標X
				mov		dx, cx                  ; DX = 絶対座標Y

				pop		cx
				pop		bx
				stc
				ret
				
; ---------------------------------------------------------------
; ねこーマウス非表示コマンド発行 
; AL bit0=1: 非表示, AL bit0=0: 表示
; ---------------------------------------------------------------
SendShowHideCursor:
				push	ax
				push	dx
				push	si
				push	ds
				
				push	cs
				pop		ds ; DS <- CS
				
				; 動的設定変更 マウス非表示コマンドを送る
				; AL=1: 非表示, AL=0: 表示
				mov		dx,07EDh
				and		al,01h ; ALのbit0だけ取り出し
				out		dx,al
				mov		al,09h
				out		dx,al
				mov		dx,07EFh
				mov		si, NP2_CMD_CHANGECONFIG
.changeconfig_loop:
				lodsb            ; AL ← [SI], SI++
				cmp		al, '$'
				je		.changeconfig_done
				out		dx, al
				jmp		.changeconfig_loop
.changeconfig_done:

				pop		ds
				pop		si
				pop		dx
				pop		ax
				ret

org_int33		dd		0
org_int_handler	dd		0

x_min			dw		0
x_max			dw		639

y_min			dw		0
y_max			dw		399

setrange_ofs	db		7

NP2_CMD_CHANGECONFIG    db 'changeconfig$', 0
NP2_CMD_GETMPOS	db 'getmpos$', 0

resident_psp    dw		0

; ------------------------------------------------------------
; setup: 起動時エントリポイント　STARTからjmpでやってくる
; ------------------------------------------------------------
setup:
				cld

				; タイトル表示
				mov		dx, title_msg
				call	PrintMsg

				; 機種チェック
				cli
				call	np2_check ; np2確認
				sti
				jne		NotNP2Error ; エラーなら終了
				
				; 機能存在チェック
				call CheckFeatureSupport
				jc  .ok_feature

				; なければエラー表示して終了
				mov dx, msg_nofeature
				call PrintMsg
				mov ax, 4C00h
				int 21h
				
.ok_feature:

				; マウスドライバ存在チェック
				call CheckMouseDriver
				jc  .ok_mouse

				; いなければエラー表示して終了
				mov dx, msg_nomouse
				call PrintMsg
				mov ax, 4C00h
				int 21h
				
.ok_mouse:
				; コマンドライン解析
				call	ParseCmdTail ; AL=0(no) / 1(/r) / 2(/h)
				jnc		ArgError ; 引数無効なら抜ける
				
				cmp 	al, 1
				je		DoRemove ; /r 常駐解除

DoInstall:
				; 未常駐なら常駐、常駐済みなら終了
				call	IsInstalled ; 常駐していたらCF=1, ES=常駐セグメント
				jc		AlreadyInstalled ; 常駐済みなら抜ける
				
				; NECドライバ準拠モードなら N mode と表示
				cmp		byte [setrange_ofs], 10h
				jne		.notnecmode
				mov		dx, msg_necmode
				call	PrintMsg
.notnecmode:
				; Installed.表示
				mov		dx, msg_installed
				call	PrintMsg
				jmp		InstallTSR ; 常駐処理へ飛ぶ（行きっぱなし）

DoRemove:
				; /r: 常駐していれば解除、いなければ終了
				call	IsInstalled ; 常駐していたらCF=1, ES=常駐セグメント
				jnc		NotInstalled ; 常駐していないなら抜ける
				
				; ESは常駐中セグメント
				call	UninstallResident ; 常駐解除する
				mov		dx, msg_removed
				call	PrintMsg
				mov		ax, 4C00h
				int		21h

AlreadyInstalled:
				mov		dx, msg_already
				call	PrintMsg
				mov		ax, 4C00h
				int		21h

NotInstalled:
				mov		dx, msg_notinst
				call	PrintMsg
				mov		ax, 4C00h
				int		21h

NotNP2Error:
				mov		dx, macerr_msg
				call	PrintMsg
				mov		ax, 4C00h
				int		21h

ArgError:
				mov		dx, msg_invalidarg
				call	PrintMsg
				mov		ax, 4C00h
				int		21h


; ------------------------------------------------------
; CheckMouseDriver:
;   returns CF=1 if installed
;   ついでにNEC系かをチェックしてsetrange_ofsをセット
; ------------------------------------------------------
CheckMouseDriver:
				push	ax
				push	bx
				push	cx
				push	dx

				; 初期化が通るか確認
				xor		ax, ax ; AX=0000h Reset/Initialize
				int		33h

				cmp		ax, 0FFFFh
				jne		.no ; 初期化が通らなければマウスドライバ不在
				
				; NEC系か調査
				mov		ax, 07h ; MSならDEFINE HORIZONTAL CURSOR RANGE、NECなら右ボタン情報
				mov		cx, 128 ; 適当な値にする
				mov		dx, 639
				int		33h
				cmp		cx, 128
				jne		.isnec ; ここでcxレジスタが変更されていたらNEC確定
				mov		ax, 07h ; 再度07hをセット
				xor		cx, cx ; 0にしてみる
				mov		dx, 639
				int		33h
				and		cx, cx
				jz		.yes ; やはりcxレジスタが変更されていないならMS確定
.isnec:
				mov		byte [setrange_ofs], 10h ; NECマウスドライバ準拠モードのオフセットは10h
.yes:
				xor		ax, ax ; AX=0000h Reset/Initialize
				int		33h ; 色々いじくったので一応初期化しておく
				stc ; installed
				jmp		.ret
.no:
				clc ; not installed
.ret:
				pop		dx
				pop		cx
				pop		bx
				pop		ax
				ret


; ----------------------------------------------------------
; CheckFeatureSupport:　シームレスマウス機能が使えるか確認
;   returns CF=1 if supported
; ----------------------------------------------------------
CheckFeatureSupport:
				push	ax
				push	bx
				push	cx
				push	dx
				push	si
				cli

				mov		si, cmd_np2
				call	sendnp2port ; NULL文字まで送る（文字列NP2が送られる）
				mov		dx,07EDh
				mov		al,02h ; ねこマウス mode=2 レジスタへ格納、成否はCFで
				out		dx,al
				mov		si, NP2_CMD_GETMPOS ; 文字列getmposを送る
				mov		dx,07EFh
.getmpos_loop:
			    lodsb
			    cmp		al, '$'
			    je		.getmpos_done
			    out		dx, al
			    jmp		.getmpos_loop
.getmpos_done:
				mov		si, cmd_np2
				call	checknp2port ; NULL文字まで比較（文字列NP2と比較）
				je		short .no ; getmposが無視されてNP2の結果が返ってきていたら未サポート

				stc ; feature supported
				jmp		.ret
.no:
				clc ; feature not supported
.ret:
				sti
				pop		si
				pop		dx
				pop		cx
				pop		bx
				pop		ax
				ret

; ------------------------------------------------------------
; InstallTSR:
;   - DS=CS にしてデータ参照
;   - 既存のINT33ハンドラをorg_int33へ退避
;   - シグネチャ40バイトコピー
;   - INT33ハンドラを登録
;   - 常駐 (INT 21h AH=31h)
; ------------------------------------------------------------
InstallTSR:
				push	cs
				pop		ds

				; 常駐解除用に PSP セグメント保存（COM なら通常 CS=PSP）
				mov		ax, cs
				mov		[resident_psp], ax

				; ES <- 0
				xor		ax, ax
				mov		es, ax

				; 元の INT 33h を退避 (ES:BX)
				mov		ax, 3533h
				int		21h
				mov		[org_int33 + 0], bx
				mov		[org_int33 + 2], es

				; ハンドラ手前にあるシグネチャをコピー ここを確認に使っているソフトがある
				push	ds
				push	es
				mov		ax, es
				mov		ds, ax ; DS = 元ハンドラのセグメント（コピー元）
				push	cs
				pop		es ; ES = 自分のセグメント（コピー先）
				mov		di, NP2_MOUSESIG ; コピー先DI = 自分のバッファ
				sub		bx, 40 ; 元ハンドラの40バイト手前からスタート
				mov		si, bx ; コピー元 ES:SI = 指定アドレス
				mov		cx, 40 ; 40バイトコピー
				cld
				rep		movsb
				pop		es
				pop		ds
				
				; INT 33h ハンドラ登録 (DS:DX)
				mov		ax, 2533h
				mov		dx, int33_handler
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


; ------------------------------------------------------------
; ParseCmdTail:
;   DS=PSP 前提　今のところ複数指定は考慮しない
;   returns AL: 0=なし, 1='/r'オプション
;           CF=1 成功、CF=0 無効な引数
; ------------------------------------------------------------
ParseCmdTail:
				push	si
				push	cx

				; コマンドライン引数取得 なければオプション指定なし扱い
				mov		si, 81h
				mov		cl, [80h]
				xor		ch, ch
				jcxz	.none

.skip_spaces:
				; スペースを飛ばす
				jcxz	.none
				mov		al, [si]
				cmp		al, ' '
				jne		.check
				inc		si
				dec		cx
				jmp		.skip_spaces

.check:
				; '/'または'-'なことを確認 それ以外は失敗
				cmp		al, '/'
				je		.check_ok
				cmp		al, '-'
				jne		.error
.check_ok:
				inc		si
				dec		cx
				jcxz	.error

				; 有効なオプションか確認 それ以外は失敗
				mov		al, [si]
				or		al, 20h ; 小文字化
				cmp		al, 'r'
				je		.opt_r ; 常駐解除オプション

.error:
				; エラー
				xor		al, al
				clc
				jmp		.ret

.none:
				; オプション無し
				xor		al, al
				stc
				jmp		.ret

.opt_r:
				; 常駐解除オプション
				mov		al, 1
				stc
				jmp		.ret

.ret:
				pop		cx
				pop		si
				ret


; ------------------------------------------------------------
; IsInstalled:
;   INT33ベクタが自分のint33_handlerを指しているかで簡易判定
;   returns CF=1 if installed, ES=resident seg
; ------------------------------------------------------------
IsInstalled:
				; INT33ベクタを確認
				push	bx
				mov		ax, 3533h
				int		21h

				; INT33ベクタのアドレスが自分のint33_handlerか
				cmp		bx, int33_handler
				jne		.no
				stc
				pop		bx
				ret
.no:
				clc
				pop		bx
				ret


; ------------------------------------------------------------
; UninstallResident:
;   ES = resident seg
; ------------------------------------------------------------
UninstallResident:
				push	ax
				push	bx
				push	dx
				push	ds
				push	es
				
				xor		ax, ax
				call	SendShowHideCursor ; 常駐解除時の表示は常に行う

				; INT33 復元
				mov		bx, [es:org_int33 + 2]
				mov		ds, bx
				mov		dx, [es:org_int33 + 0]
				mov		ax, 2533h
				int		21h
				
				; メモリ解放
				mov		bx, [es:resident_psp]  ; PSP セグメント取得
				mov		es, bx
				mov		ax, 4900h
				int		21h ; PSP 解放

				pop		es
				pop		ds
				pop		dx
				pop		bx
				pop		ax
				ret


; ------------------------------------------------------------
; PrintMsg: 　DXの指す$終端文字列を画面に表示
; ------------------------------------------------------------
PrintMsg:
				push	ax
				mov		ah, 9
				int		21h
				pop		ax
				ret

%include	'np2tool.x86'

macerr_msg		db	"Illegal hardware - $"
title_msg		db	"seamless mouse driver for np2+dos ("
				db	VERSION_ID
				db	 ")"
				db	13, 10, "$"

msg_nomouse		db	"mouse driver not installed.",13,10,"$"
msg_nofeature	db	"feature not supported.",13,10,"$"
msg_already		db	"already installed.",13,10,"$"
msg_notinst		db	"not installed.",13,10,"$"
msg_removed		db	"removed.",13,10,"$"
msg_necmode		db	"N mode $"
msg_installed	db	"installed.",13,10,"$"
msg_invalidarg	db	"invalid argument.",13,10,"$"

; サポートしているかのテスト用
cmd_np2			db	3,'NP2',0
