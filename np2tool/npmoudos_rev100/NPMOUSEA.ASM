

	ORG		100h
	USE16
	CPU		8086

%include	'np2tool.inc'

%define	VERSION_ID		"20260225"
%define	NP2_MAGIC		4e50h

%define	NPMFLAG_NONE		00h
%define	NPMFLAG_NODOSCUR	01h ; DOSカーソル常時非表示（NPMFLAG_FIXHOSTCURがある時は0=ホストカーソル常時表示, 1=ホストカーソル常時非表示）
%define	NPMFLAG_FIXHOSTCUR	02h ; ホストマウス表示状態固定（現在の表示状態を維持）
%define	NPMFLAG_FULLRANGE	04h ; レンジ設定を無視
%define	NPMFLAG_INT33MON	08h ; インテリジェントモード（常駐を監視する）
%define	NPMFLAG_AUTOMTYPE	10h ; 自動でマウス種別判定
%define	NPMFLAG_REQMTYPE	20h ; 最初に飛んできた範囲コマンドの時にマウス種別判定
%define	NPMFLAG_UNCHECK		40h ; マウスドライバの有無を確認せずに常駐する

START:			jmp		setup

;NP2_MOUSESIG	db 		"*** This is Copyright 2026 NekoDevT. ***"
int33_handler:
				test	ah, ah
				jnz		int33_chain ; ahが0でないならchain
				
				cmp		al, 00h
				je		resetdriver
                
				cmp		al, 03h
				jb		showhidecursor ; AX=1またはAX=2  0は既に判定済みなのでこれでOK
                
				je		getpos ; AX=3
                
				cmp		al, 0ch
				je		set_int_handler
				
                ; マウスタイプ確認要求があれば確認
				test	byte [cs:npmflags], NPMFLAG_REQMTYPE
				jz		.ok_mousetype
				call	HookCheckMouseDriverType
.ok_mousetype:
                push	ax
                sub		al, [cs:setrange_ofs]
				cmp		al, 2
				jb		set_range
                pop		ax
                
int33_chain:
				jmp		far [cs:org_int33]
        		
; ----- AX=0000h RESET DRIVER AND READ STATUS -----
resetdriver:	; リセット時にはホストのマウスカーソルを出す
				push	ds
				push	cs
				pop		ds
				push	ax
				mov		al, byte [npmflags]
				test	al, NPMFLAG_INT33MON
				jz		.skipreqmtype ; インテリジェントモードでないなら再調査不要
				test	al, NPMFLAG_AUTOMTYPE
				jz		.skipreqmtype ; 自動でマウス種別判定でないなら再調査不要
				or		byte [npmflags], NPMFLAG_REQMTYPE ; マウスタイプ調査指示
.skipreqmtype:
				test	al, NPMFLAG_FULLRANGE
				jnz		.skiprange ; 全範囲なら抜ける
				mov		word [x_min], 0
				mov		word [x_max], 639
				mov		word [y_min], 0
				mov		word [y_max], 399
.skiprange:
				and		al, NPMFLAG_FIXHOSTCUR
				jnz		.tochain ; 表示状態固定なら現状維持
				call	SendShowHideCursor ; 上のANDでAL=0になるのでこれでよい
.tochain:
				pop		ax
				pop		ds
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
				
; ----- AX=0001h, 0002h SHOW/HIDE MOUSE CURSOR -----
showhidecursor:		; ホストのマウスカーソルを出す、ホストのマウスカーソルを消す or DOSマウスカーソル表示を無視
				mov		byte [cs:doscuron], al
				push	ax
				mov		ah, byte [cs:npmflags]
				test	ah, NPMFLAG_FIXHOSTCUR
				jnz		.chain ; 表示状態固定は元へ投げる
				test	ah, NPMFLAG_NODOSCUR
				jnz		.usehostcursor ; DOSマウスカーソル表示・非表示を無視（ホストカーソル）
				and		al, 1 ; AL 1=SHOW -> 1=HOST HIDE, AL 2=HIDE -> 0=HOST SHOWに変換
				call	SendShowHideCursor ; HOST SHOW=0, HIDE=1
.chain:
				pop		ax
				mov		byte [cs:doscuronstat], al ; 実際の表示状態更新
				jmp		int33_chain
.usehostcursor:
				pop		ax
				iret

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

; ----- AX=0007h, 0008h DEFINE HORIZONTAL/VERTICAL CURSOR RANGE -----
; ----- AX=0010h, 0011h DEFINE HORIZONTAL/VERTICAL CURSOR RANGE (NEC) -----
; ここに飛んできたとき、ALからオフセットが引かれて AL=0:横、AL=1:縦の状態になっている。
set_range:
				; レンジ無視全範囲かを判定
				test	byte [cs:npmflags], NPMFLAG_FULLRANGE
				jnz		.tochain ; 全範囲なら抜ける
				
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
				xor		di, di

				; 絶対座標を読み出し
				call	ReadAbsXY ; CF=1 なら BX=X絶対座標, CX=Y絶対座標 が入る　座標値は画面全体幅・高さを0～65535でマップ
				jnc		.use_original ; CF=0 なら元の座標値を保持
				
				; ここからシームレスマウス状態処理

				mov		si, cx ; CX(Y絶対座標)を一旦SIへ退避
				
				; DOSマウス座標系への変換 X = (X絶対座標 * DOSマウス画面幅) / 65535
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
				
				; ホストカーソルモードの時、常にDOSカーソル表示を消す
				mov		al, 02h ; DOSカーソル消去
				jmp		.update_doscursor
				
.use_original:
				; 通常マウス状態
				pop		cx
				pop		dx
				
				; ホストカーソルモードの時、内部的なカーソル表示状態がONならDOSカーソル表示を出す
				mov		al, 01h ; DOSカーソル表示
				cmp		byte [cs:doscuron], al
				jne		.exit_handler ; 内部的なカーソル表示状態がOFFならそもそも表示させる必要はないので抜ける
				
.update_doscursor:
				mov		ah, byte [cs:npmflags]
				test	ah, NPMFLAG_FIXHOSTCUR
				jnz		.exit_handler ; ホストマウス表示状態固定なら何もしない（このときDOSカーソルは常にフラグ通りに表示されている）
				test	ah, NPMFLAG_NODOSCUR
				jz		.exit_handler ; DOSカーソル常時非表示でないなら何もしない（それ以外のモードではDOSカーソルは常にフラグ通りに表示されている）
				cmp		byte [cs:doscuronstat], al
				je		.exit_handler ; 実際の表示状態が設定したいものに合っていれば抜ける
				mov		byte [cs:doscuronstat], al
				xor		ah, ah
				pushf
				call	far [cs:org_int33] ; AL=1（表示） or 2（非表示）で実行
.exit_handler:
				pop		di
				pop		si
				pop		bx
				pop		ax
				ret

; ---------------------------------------------------------------
; ねこーマウス座標取得コマンド発行 
; returns on success: CF=1, BX=絶対座標X, CX=絶対座標Y
; returns on failure: CF=0, BX/CX undefined
; 絶対座標は画面全体幅・高さを0～65535にマップした値
; AX, DX, SIの内容は維持されない
; ---------------------------------------------------------------
ReadAbsXY:
				push	ds

				; 絶対座標を取得する
				push	cs
				pop		ds ; DS <- CS
				
				mov		dx, 07EDh
				mov		al, 02h ; ねこマウス mode=2 レジスタへ格納、成否はCFで
				mov		si, NP2_CMD_GETMPOS
				call	SendNPMCmd
				jnc		.fin ; CF=0はコマンド実行失敗

				; コマンド実行成功
				; BX=絶対座標X  CX=絶対座標Y
.fin:
    			pop		ds
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
				mov		dx, 07EDh
				and		al, 01h ; 1つ目のパラメータ ALのbit0だけ取り出し
				out		dx, al
				mov		al, 09h ; 2つ目のパラメータ
				mov		si, NP2_CMD_CHANGECONFIG
				call	SendNPMCmd

				pop		ds
				pop		si
				pop		dx
				pop		ax
				ret

; ねこーコマンド発行 
; 最初のDXは07EDhになっている前提、ALをパラメータとして07EDhへ送信し、
; SIの指しているデータを07EFhに送る。AL, DX, SIは復元されない
SendNPMCmd:
				out		dx, al
				or		dl, 02h ; 07EDh -> 07EFh
.sendnpmcmd_loop:
			    lodsb
			    cmp		al, '$'
			    je		.sendnpmcmd_done
				clc
			    out		dx, al
				jc		.sendnpmcmd_done ; CF=1ならコマンド実行成功
			    jmp		.sendnpmcmd_loop
.sendnpmcmd_done:
				ret

resident_psp    dw		0

org_int33		dd		0
org_int_handler	dd		0

x_min			dw		0
x_max			dw		639

y_min			dw		0
y_max			dw		399

setrange_ofs	db		7

npmflags		db		NPMFLAG_AUTOMTYPE ; 動作モードフラグ

doscuron		db		2 ; 1ならDOSカーソル表示状態, 2は非表示
doscuronstat	db		2 ; 1なら実際にDOSカーソル表示状態, 2は非表示

NP2_CMD_CHANGECONFIG    db 'changeconfig$'
NP2_CMD_GETMPOS			db 'getmpos$'

int21_handler:
				; 特殊判定：CX=NP2_MAGIC のときは常に元の INT21 へ
				cmp		cx, NP2_MAGIC
				je		.chain

				; --- SETVEC INT33? (AX=2533h) ---
				cmp		ax, 2533h
				je		.set33

				; --- SETVEC INT21? (AX=2521h) ---
				cmp		ax, 2521h
				je		.set21

				; --- GETVEC INT33? (AX=3533h) ---
				cmp		ax, 3533h
				je		.get33

				; --- GETVEC INT21? (AX=3521h) ---
				cmp		ax, 3521h
				je		.get21

.chain:
				jmp		far [cs:org_int21]

.set33:
				mov		[cs:org_int33+0], dx
				mov		[cs:org_int33+2], ds
				test	byte [cs:npmflags], NPMFLAG_AUTOMTYPE ; 自動でマウス種別判定？
				jz		.noautomtype
				or		byte [cs:npmflags], NPMFLAG_REQMTYPE ; 調査要求フラグ立てる
.noautomtype:
				iret

.set21:
				mov     [cs:org_int21+0], dx
				mov     [cs:org_int21+2], ds
				iret

.get33:
				mov     bx, [cs:org_int33+0]
				mov     es, [cs:org_int33+2]
				iret

.get21:
				mov     bx, [cs:org_int21+0]
				mov     es, [cs:org_int21+2]
				iret

;   NEC系かをチェックしてsetrange_ofsをセット（ハンドラ内専用）
HookCheckMouseDriverType:
				push	ax
				push	bx
				push	cx
				push	dx
				push	di
				
				mov		di, 1 ; 1にセット
				
				; NEC系か調査
.retry:
				mov		ax, 07h ; MSならDEFINE HORIZONTAL CURSOR RANGE、NECなら右ボタン情報
				mov		cx, [cs:x_min]
				add		cx, di
				mov		dx, [cs:x_max]
				pushf
				call	far [cs:org_int33] ; 元のINT 33hルーチンを呼ぶ
				sub		cx, di
				cmp		cx, [cs:x_min]
				jne		.isnec ; ここでcxレジスタが変更されていたらNEC確定
				and		di, di
				jz		.isms ; 常にcxレジスタが維持されるのでMS確定
				dec		di
				jmp		.retry
.isms:
				mov		byte [cs:setrange_ofs], 7h ; MSマウスドライバ準拠モードのオフセットは7h
				jmp		.fin
.isnec:
				mov		byte [cs:setrange_ofs], 10h ; NECマウスドライバ準拠モードのオフセットは10h
.fin:
				and		byte [cs:npmflags], ~NPMFLAG_REQMTYPE
				pop		di
				pop		dx
				pop		cx
				pop		bx
				pop		ax
				ret

org_int21		dd		0

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

				; コマンドライン解析
				call	ParseCmdTail ; AL=0(常駐解除) / 1(常駐)
				jnc		ArgError ; 引数無効なら抜ける
				
				cmp 	al, 1
				je		DoRemove ; /r 常駐解除
				
				; マウスドライバ有無を確認しないモードなら存在確認とドライバタイプチェックを省略
				test	byte [cs:npmflags], NPMFLAG_UNCHECK
				jnz		.skipchkmousetype
				
				; インテリジェントモードならマウスドライバ不在でもOK
				test	byte [cs:npmflags], NPMFLAG_INT33MON
				jnz		.ok_mouse
				
				; マウスドライバ存在チェック
				call	CheckMouseDriver
				jc		.ok_mouse

				; いなければエラー表示して終了
				mov		dx, msg_nomouse
				call	PrintMsg
				mov		ax, 4C00h
				int		21h
.ok_mouse:
				; マウスドライバタイプ確認
				test	byte [cs:npmflags], NPMFLAG_AUTOMTYPE ; マウスタイプ自動判定がオンか確認
				jz		.skipchkmousetype
				call	CheckMouseDriverType
.skipchkmousetype:

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
				
				; 一旦カーソル表示
				xor		ax, ax
				call	SendShowHideCursor

				; ホストカーソル／DOSカーソルのどちらかを判定
				mov		al, [npmflags]
				and		al, NPMFLAG_NODOSCUR|NPMFLAG_FIXHOSTCUR
				jz		.usedoscursor ; 0 = DOSカーソル自動、ホストカーソル自動
				cmp		al, 02h
				jb		.usehostcursor ; 1 = DOSカーソル非表示、ホストカーソル自動
				je		.usedualcursor ; 2 = ホストカーソル固定（常時表示）
				 ; 3 = ホストカーソル固定（常時非表示）
				
				; ホストカーソル常時消去
				mov		al, 1 ; ホストカーソル非表示
				call	SendShowHideCursor
				mov		dx, msg_setnonecur
				jmp		.toinstall
.usedoscursor:
				; DOSカーソルを使用
				mov		dx, msg_setdoscur
				jmp		.toinstall
.usehostcursor:
				; ホストカーソルを使用
				mov		dx, msg_sethostcur
				jmp		.toinstall
.usedualcursor:
				; 両カーソルを使用 
				xor		ax, ax ; ホストカーソル表示
				call	SendShowHideCursor
				mov		dx, msg_setdualcur
				jmp		.toinstall
.toinstall:
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

; ------------------------------------------------------
; CheckMouseDriverType:
;   NEC系かをチェックしてsetrange_ofsをセット
; ------------------------------------------------------
CheckMouseDriverType:
				push	ax
				push	bx
				push	cx
				push	dx

				; とりあえずMSマウスドライバ準拠モードで
				mov		byte [setrange_ofs], 7h ; MSマウスドライバ準拠モードのオフセットは7h
				
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
				jz		.fin ; やはりcxレジスタが変更されていないならMS確定
.isnec:
				mov		byte [setrange_ofs], 10h ; NECマウスドライバ準拠モードのオフセットは10h
.fin:
				xor		ax, ax ; AX=0000h Reset/Initialize
				int		33h ; 色々いじくったので一応初期化しておく
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
				mov		dx, 07EDh
				mov		al, 02h ; ねこマウス mode=2 レジスタへ格納、成否はCFで
				mov		si, NP2_CMD_GETMPOS ; 文字列getmposを送る
				call	SendNPMCmd
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
				; メモリ節約のためPSPのコマンドライン引数領域に放り込む
				mov		word [80h], 0D00h ; コマンドライン引数なし扱いにする 念のため0Dh終端も付ける
				push	ds
				push	es
				mov		ax, es
				mov		ds, ax ; DS = 元ハンドラのセグメント（コピー元）
				push	cs
				pop		es ; ES = 自分のセグメント（コピー先）
				mov		di, int33_handler-40 ; コピー先DI = 自分のバッファ
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
				
				; インテリジェントモードでないならINT 21hは見なくてよい
				test	byte [npmflags], NPMFLAG_INT33MON
				jz		.noint33mon
				
				mov		dx, msg_int33hlock
				call	PrintMsg
				
				; 元の INT 21h を退避 (ES:BX)
				mov		ax, 3521h
				int		21h
				mov		[org_int21 + 0], bx
				mov		[org_int21 + 2], es

				; INT 21h ハンドラ登録 (DS:DX)
				mov		ax, 2521h
				mov		dx, int21_handler
				int		21h
.noint33mon:
				
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
				mov		dx, int21_handler + 15
				test	byte [npmflags], NPMFLAG_INT33MON
				jz		.noint33monres
				mov		dx, setup + 15 ; インテリジェントモードはINT 21hハンドラ部分も必要
.noint33monres:
				mov		cl, 4
				shr		dx, cl ; bytes -> paragraphs
				int		21h


; ------------------------------------------------------------
; ParseCmdTail:
;   DS=PSP 前提　フラグ系は処置中に設定する
;   returns AL: 0=常駐, 1=常駐解除
;           CF=1 成功、CF=0 無効な引数
; AHは上書きされる
; ------------------------------------------------------------
ParseCmdTail:
				push	si
				push	di
				push	bx
				push	cx
				push	dx;
				
				; 戻り値（0にセット）
				xor		bx, bx

				; コマンドライン引数取得 なければオプション指定なし扱い
				mov		si, 81h
				mov		cl, [80h]
				xor		ch, ch
				
				; 動作フラグをAHレジスタへ
				mov		ah, [npmflags]

.skip_spaces:
				; スペースを飛ばす
				jcxz	.success
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
				cmp		al, 'h'
				je		.opt_h ; ホストカーソル使用オプション
				cmp		al, 'o'
				je		.opt_o ; 両カーソル使用オプション
				cmp		al, 'x'
				je		.opt_x ; ホストカーソル常時消去
				cmp		al, 'f'
				je		.opt_f ; レンジ無視全範囲オプション
				cmp		al, 'n'
				je		.opt_n ; NECマウスドライバ準拠モード
				cmp		al, 'm'
				je		.opt_m ; MSマウスドライバ準拠モード
				cmp		al, 'i'
				je		.opt_i ; インテリジェントモード
				cmp		al, 'u'
				je		.opt_u ; マウスドライバの有無を確認しない
				cmp		al, 'b'
				je		.opt_b ; レンジ強制指定オプション
				jmp		.error

.opt_r:
				; 常駐解除オプション
				mov		bx, 1
				jmp		.opt_continue

.opt_h:
				; ホストカーソル使用オプション
				or		ah, NPMFLAG_NODOSCUR
				and		ah, ~NPMFLAG_FIXHOSTCUR
				jmp		.opt_continue

.opt_o:
				; 両カーソル使用オプション
				and		ah, ~NPMFLAG_NODOSCUR
				or		ah, NPMFLAG_FIXHOSTCUR
				jmp		.opt_continue

.opt_x:
				; ホストカーソル常時消去オプション
				or		ah, NPMFLAG_NODOSCUR|NPMFLAG_FIXHOSTCUR
				jmp		.opt_continue

.opt_f:
				; レンジ無視全範囲オプション
				or		ah, NPMFLAG_FULLRANGE
				jmp		.opt_continue
				
.opt_continue:
				inc		si
				dec		cx
				jmp		.skip_spaces

.error:
				; エラー
				clc
				jmp		.ret

.success:
				; 正常終了
				; 動作フラグを書き戻し
				mov		[npmflags], ah
				stc

.ret:
				mov		ax, bx
				pop		dx;
				pop		cx
				pop		bx
				pop		di
				pop		si
				ret

; 巨大なのでshort jump範囲内になるようにここに配置
.opt_n:
				; NECマウスドライバ準拠モード
				mov		byte [setrange_ofs], 10h ; オフセットは10h
				and		ah, ~NPMFLAG_AUTOMTYPE ; マウスタイプ自動決定しない
				jmp		.opt_continue

.opt_m:
				; MSマウスドライバ準拠モード
				mov		byte [setrange_ofs], 07h ; オフセットは07h
				and		ah, ~NPMFLAG_AUTOMTYPE ; マウスタイプ自動決定しない
				jmp		.opt_continue

.opt_i:
				; インテリジェントモード
				or		ah, NPMFLAG_INT33MON
				jmp		.opt_continue

.opt_u:
				; マウスドライバの有無を確認しない
				or		ah, NPMFLAG_UNCHECK
				jmp		.opt_continue

.opt_b:
				; レンジ強制指定オプション Xmin,Xmax,Ymin,Ymaxを000,000,000,000の形式で指定
				push	ax ; 退避
				xor		ax, ax
				mov		di, x_min
.opt_b_continue:
				; x_min -> x_max -> y_min -> y_max の順に格納
				xor		dx, dx
.opt_b_loop:
				inc		si
				dec		cx
				jcxz	.opt_b_loopend
				mov		al, [si]
				cmp		al, ' '
				je		.opt_b_loopend
				cmp		al, ','
				je		.opt_b_loopend
				sub		al, '0'
				cmp		al, 9
				ja		.opt_b_error ; 数値でない
				push	ax
				mov		ax, dx
				shl		dx, 1 ; 元値 * 2
				shl		ax, 1
				shl		ax, 1
				shl		ax, 1 ; 元値 * 8
				add		dx, ax ; 元値 * 10 = 元値 * 2 + 元値 * 8
				pop		ax
				add		dx, ax ; 新値 = 元値 * 10 + 新しい桁の値
				jmp		.opt_b_loop
.opt_b_loopend:
				mov		word [di], dx ; 最終結果格納
				cmp		di, y_max
				je		.opt_b_complete ; y_maxまで到達したら終了
				and		cx, cx
				jz		.opt_b_error ; y_maxまで到達していないのに末尾ならエラー
				cmp		al, ' '
				je		.opt_b_error ; y_maxまで到達していないのにスペースならエラー
				add		di, 2
				jmp		.opt_b_continue ; x_min -> x_max -> y_min -> y_max の順に格納
.opt_b_complete:
				cmp		al, ','
				je		.opt_b_error ; カンマが多すぎるならエラー
				pop		ax
				jmp		.skip_spaces
.opt_b_error:
				pop		ax
				jmp		.error


; ------------------------------------------------------------
; IsInstalled:
;   INT33ベクタが自分のint33_handlerを指しているかで簡易判定
;   returns CF=1 if installed, ES=resident seg
; ------------------------------------------------------------
IsInstalled:
				; INT 33hを確認
				push	bx
				push	cx
				mov		cx, NP2_MAGIC ; 元のハンドラ取得を指示
				mov		ax, 3533h
				int		21h

				; INT 33hのアドレスが自分のint33_handlerか
				cmp		bx, int33_handler
				jne		.no
				stc
				pop		cx
				pop		bx
				ret
.no:
				clc
				pop		cx
				pop		bx
				ret


; ------------------------------------------------------------
; UninstallResident:
;   ES = resident seg
; ------------------------------------------------------------
UninstallResident:
				push	ax
				push	bx
				push	cx
				push	dx
				push	ds
				push	es
				
				xor		ax, ax
				call	SendShowHideCursor ; 常駐解除時の表示は常に行う
				
				; INT 33h監視モードでないならINT 21hは戻さなくてよい
				test	byte [es:npmflags], NPMFLAG_INT33MON
				jz		.noint33mon

				; INT 21h 復元 これによりINT 33hの書き戻しが可能になる
				mov		cx, NP2_MAGIC ; 元のハンドラ設定を指示
				mov		bx, [es:org_int21 + 2]
				mov		ds, bx
				mov		dx, [es:org_int21 + 0]
				mov		ax, 2521h
				int		21h

.noint33mon:

				; INT 33h 復元
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
				pop		cx
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

; サポートしているかのテスト用
cmd_np2			db	3,'NP2',0

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
msg_installed	db	"installed. ($"
msg_setdoscur	db	"DOS cursor)",13,10,"$"
msg_sethostcur	db	"host cursor)",13,10,"$"
msg_setdualcur	db	"dual cursor)",13,10,"$"
msg_setnonecur	db	"no host cursor)",13,10,"$"
msg_invalidarg	db	"invalid argument.",13,10,"$"
msg_int33hlock	db	"intelligent mode",13,10,"$"
