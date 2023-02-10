/**
 * @file	serial.c
 * @brief	Keyboard & RS-232C Interface
 *
 * 関連：pit.c, sysport.c
 */

#include	<compiler.h>
#include	<cpucore.h>
#include	<commng.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<keystat.h>


// ---- Keyboard

void keyboard_callback(NEVENTITEM item) {

	if (item->flag & NEVENT_SETEVENT) {
		if ((keybrd.ctrls) || (keybrd.buffers)) {
			if (!(keybrd.status & 2)) {
				keybrd.status |= 2;
				if (keybrd.ctrls) {
					keybrd.ctrls--;
					keybrd.data = keybrd.ctr[keybrd.ctrpos];
					keybrd.ctrpos = (keybrd.ctrpos + 1) & KB_CTRMASK;
				}
				else if (keybrd.buffers) {
					keybrd.buffers--;
					keybrd.data = keybrd.buf[keybrd.bufpos];
					keybrd.bufpos = (keybrd.bufpos + 1) & KB_BUFMASK;
				}
//				TRACEOUT(("recv -> %02x", keybrd.data));
			}
			pic_setirq(1);
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_RELATIVE);
		}
	}
}

static void IOOUTCALL keyboard_o41(UINT port, REG8 dat) {

	if (keybrd.cmd & 1) {
//		TRACEOUT(("send -> %02x", dat));
		keystat_ctrlsend(dat);
	}
	else {
		keybrd.mode = dat;
	}
	(void)port;
}

static void IOOUTCALL keyboard_o43(UINT port, REG8 dat) {

//	TRACEOUT(("out43 -> %02x %.4x:%.8x", dat, CPU_CS, CPU_EIP));
	if ((!(dat & 0x08)) && (keybrd.cmd & 0x08)) {
		keyboard_resetsignal();
	}
	if (dat & 0x10) {
		keybrd.status &= ~(0x38);
	}
	keybrd.cmd = dat;
	(void)port;
}

static REG8 IOINPCALL keyboard_i41(UINT port) {

	(void)port;
	keybrd.status &= ~2;
	pic_resetirq(1);
//	TRACEOUT(("in41 -> %02x %.4x:%.8x", keybrd.data, CPU_CS, CPU_EIP));
	return(keybrd.data);
}

static REG8 IOINPCALL keyboard_i43(UINT port) {

	(void)port;
//	TRACEOUT(("in43 -> %02x %.4x:%.8x", keybrd.status, CPU_CS, CPU_EIP));
	return(keybrd.status | 0x85);
}


// ----

static const IOOUT keybrdo41[2] = {
					keyboard_o41,	keyboard_o43};

static const IOINP keybrdi41[2] = {
					keyboard_i41,	keyboard_i43};


void keyboard_reset(const NP2CFG *pConfig) {

	ZeroMemory(&keybrd, sizeof(keybrd));
	keybrd.data = 0xff;
	keybrd.mode = 0x5e;

	(void)pConfig;
}

void keyboard_bind(void) {

	keystat_ctrlreset();
	keyboard_changeclock();
	iocore_attachsysoutex(0x0041, 0x0cf1, keybrdo41, 2);
	iocore_attachsysinpex(0x0041, 0x0cf1, keybrdi41, 2);
}

void keyboard_resetsignal(void) {

	nevent_reset(NEVENT_KEYBOARD);
	keybrd.cmd = 0;
	keybrd.status = 0;
	keybrd.ctrls = 0;
	keybrd.buffers = 0;
	keystat_ctrlreset();
	keystat_resendstat();
}

void keyboard_ctrl(REG8 data) {

	if ((data == 0xfa) || (data == 0xfc)) {
		keybrd.ctrls = 0;
	}
	if (keybrd.ctrls < KB_CTR) {
		keybrd.ctr[(keybrd.ctrpos + keybrd.ctrls) & KB_CTRMASK] = data;
		keybrd.ctrls++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_ABSOLUTE);
		}
	}
}

void keyboard_send(REG8 data) {

	if (keybrd.buffers < KB_BUF) {
		keybrd.buf[(keybrd.bufpos + keybrd.buffers) & KB_BUFMASK] = data;
		keybrd.buffers++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
										keyboard_callback, NEVENT_ABSOLUTE);
		}
	}
	else {
		keybrd.status |= 0x10;
	}
}

void keyboard_changeclock(void) {

	keybrd.xferclock = pccore.realclock / 1920;
}



// ---- RS-232C

	COMMNG	cm_rs232c = NULL;
	
// RS-232C 受信バッファサイズ
#define RS232C_BUFFER		(1 << 6)
#define RS232C_BUFFER_MASK	(RS232C_BUFFER - 1)
// RS-232C 受信バッファにあるデータが（RS232C_BUFFER_CLRC×8÷ボーレート）秒間読み取られなければ捨てる（根拠なしで感覚的に指定）
#define RS232C_BUFFER_CLRC	16

static UINT8 rs232c_buf[RS232C_BUFFER]; // RS-232C 受信リングバッファ　本来は存在すべきでないがWindows経由の通信はリアルタイムにならない以上仕方なし（値に根拠なし）
static UINT8 rs232c_buf_rpos = 0; // RS-232C 受信リングバッファ 読み取り位置
static UINT8 rs232c_buf_wpos = 0; // RS-232C 受信リングバッファ 書き込み位置
static int rs232c_removecounter = 0; // データ破棄チェック用カウンタ

// RS-232C FIFO送信バッファサイズ
#define RS232C_FIFO_WRITEBUFFER		256
#define RS232C_FIFO_WRITEBUFFER_MASK	(RS232C_FIFO_WRITEBUFFER - 1)
static UINT8 rs232c_fifo_writebuf[RS232C_FIFO_WRITEBUFFER]; // RS-232C FIFO送信リングバッファ
static int rs232c_fifo_writebuf_rpos = 0; // RS-232C FIFO送信リングバッファ 読み取り位置
static int rs232c_fifo_writebuf_wpos = 0; // RS-232C FIFO送信リングバッファ 書き込み位置

// RS-232C 送信リトライ
static void rs232c_writeretry() {

	int ret;
//#if defined(SUPPORT_RS232C_FIFO)
//	// FIFOモードの時
//	if(rs232cfifo.port138 & 0x1){
//		if(rs232c_fifo_writebuf_wpos == rs232c_fifo_writebuf_rpos) return; // バッファ空なら何もしない
//	}else
//#endif
//	{
		if((rs232c.result & 0x4) != 0) {
			return; // TxEMPを見て既に送信完了しているか確認（完了なら送信不要）
		}
	//}
	if (cm_rs232c) {
		cm_rs232c->writeretry(cm_rs232c); // 送信リトライ
		ret = cm_rs232c->lastwritesuccess(cm_rs232c); // 送信成功かチェック
		if(ret==0){
			return; // 失敗していたら次に持ち越し
		}
#if defined(SUPPORT_RS232C_FIFO)
		// FIFOモードの時
		if(rs232cfifo.port138 & 0x1){
			int fifowbufused;
			
			// バッファ読み取り位置を進める
			rs232c_fifo_writebuf_rpos = (rs232c_fifo_writebuf_rpos+1) & RS232C_FIFO_WRITEBUFFER_MASK;
			
			fifowbufused = (rs232c_fifo_writebuf_wpos - rs232c_fifo_writebuf_rpos) & RS232C_FIFO_WRITEBUFFER_MASK;
			if(fifowbufused > RS232C_FIFO_WRITEBUFFER * 3 / 4){
				rs232c.result &= ~0x1; // バッファフルならTxRDYを下ろす
			}else{
				rs232c.result |= 0x1; // バッファ空きならTxRDYを立てる
			}
			if(!(rs232c.result & 0x5)){
				// バッファフルなら割り込みは止めておく
			}else{
				if (sysport.c & 6) { // TxEMP, TxRDYで割り込み？
					rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
					rs232cfifo.irqflag = 1;
#endif
					pic_setirq(4); // 割り込み
				}
			}
			// バッファに溜まっているデータを書けるだけ書く
			while(fifowbufused = ((rs232c_fifo_writebuf_wpos - rs232c_fifo_writebuf_rpos) & RS232C_FIFO_WRITEBUFFER_MASK)){
				if(fifowbufused > RS232C_FIFO_WRITEBUFFER * 3 / 4){
					rs232c.result &= ~0x1; // バッファフルならTxRDYを下ろす
				}else{
					rs232c.result |= 0x1; // バッファ空きならTxRDYを立てる
				}
				cm_rs232c->write(cm_rs232c, (UINT8)rs232c_fifo_writebuf[rs232c_fifo_writebuf_rpos]);
				ret = cm_rs232c->lastwritesuccess(cm_rs232c);
				if(!ret){
					return; // まだ書き込めないので待つ
				}else{
					// バッファ読み取り位置を進める
					rs232c_fifo_writebuf_rpos = (rs232c_fifo_writebuf_rpos+1) & RS232C_FIFO_WRITEBUFFER_MASK;
				}
			}
			rs232c.result |= 0x5; // バッファ空きならTxEMP,TxRDYを立てる
			cm_rs232c->endblocktranster(cm_rs232c); // ブロック転送モード解除
		}else
#endif
		{
			rs232c.result |= 0x5; // 送信成功したらTxEMP, TxRDYを立てる
		}
		if (sysport.c & 6) { // TxEMP, TxRDYで割り込み？
			rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
			rs232cfifo.irqflag = 1;
#endif
			pic_setirq(4); // 割り込み
		}
		else {
			rs232c.send = 1; // 送信済みフラグを立てる（TxRE, TxEEビットが立つと割り込み発生）
		}
	}
}

// RS-232C 初期化 np2起動時に1回だけ呼ばれる
void rs232c_construct(void) {

	if(cm_rs232c){
		commng_destroy(cm_rs232c);
	}
	cm_rs232c = NULL;
}

// RS-232C 初期化 np2終了時に1回だけ呼ばれる
void rs232c_destruct(void) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
}

// RS-232C ポートオープン 実際にアクセスされるまでポートオープンされない仕様
void rs232c_open(void) {

	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL, FALSE);
#if defined(VAEG_FIX)
		cm_rs232c->msg(cm_rs232c, COMMSG_SETRSFLAG, rs232c.cmd & 0x22); /* RTS, DTR */
#endif
	}
}

// RS-232C 通信用コールバック ボーレート÷8 回/秒で呼ばれる（例: 2400bpsなら2400/8 = 300回/秒）
void rs232c_callback(void) {

	BOOL	intr = FALSE; // 割り込みフラグ
	BOOL	fifomode = FALSE; // FIFOモードフラグ
	int		bufused; // 受信リングバッファ使用量
	BOOL	bufremoved = FALSE;
	
	// 開いていなければRS-232Cポートオープン
	rs232c_open();

	// 送信に失敗していたらリトライ
	rs232c_writeretry();

	// 受信リングバッファの使用状況を取得
	bufused = (rs232c_buf_wpos - rs232c_buf_rpos) & RS232C_BUFFER_MASK;
	if(bufused==0){
		rs232c_removecounter = 0; // バッファが空なら古いデータ削除カウンタをリセット
	}
	
	// 受信可（uPD8251 Recieve Enable）をチェック
	if(!(rs232c.cmd & 0x04) && bufused==0) {
		// 受信禁止で受信バッファが空ならなら受信処理をしない
	}else{
		// 受信可能あるいは受信バッファに残りがあれば処理する
#if defined(SUPPORT_RS232C_FIFO)
		// FIFOモードチェック
		fifomode = (rs232cfifo.port138 & 0x1);
		if(fifomode){
			rs232c_removecounter = 0; // FIFOモードでは古いデータを消さない
			if(bufused == RS232C_BUFFER-1){
				if(!rs232cfifo.irqflag){
					// 割り込みが消えていたら割り込み原因をセットして割り込み
					rs232cfifo.irqflag = 2;
					pic_setirq(4);
				}
				return; // バッファがいっぱいなら待機
			}
			if(rs232cfifo.irqflag){
				return; // 割り込み原因フラグが立っていれば待機
			}
		}
#endif

		// 古いデータ削除カウンタをインクリメント
		rs232c_removecounter = (rs232c_removecounter + 1) % RS232C_BUFFER_CLRC;
		if (bufused > 0 && rs232c_removecounter==0 || bufused == RS232C_BUFFER-1){
			rs232c_buf_rpos = (rs232c_buf_rpos+1) & RS232C_BUFFER_MASK; // カウンタが1周したら一番古いものを捨てる
			bufremoved = TRUE;
		}
		// 受信可（uPD8251 Recieve Enable）の時、ポートから次のデータ読み取り
		if ((rs232c.cmd & 0x04) && (cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c_buf[rs232c_buf_wpos]))) {
			rs232c_buf_wpos = (rs232c_buf_wpos+1) & RS232C_BUFFER_MASK; // 読み取れたらバッファ書き込み位置を進める
		}
		// バッファにデータがあればI/Oポート読み取りデータにセットして割り込み
		if (rs232c_buf_rpos != rs232c_buf_wpos) {
			rs232c.data = rs232c_buf[rs232c_buf_rpos]; // データを1つ取り出し
			if(!(rs232c.result & 2) || bufremoved) { // RxRDYが既に立っていれば何もしない → 一部ソフト不具合発生のためバッファが破棄されたときのみ再割り込み
				rs232c.result |= 2; // RxRDYを立てる
#if defined(SUPPORT_RS232C_FIFO)
				if(fifomode){
					// FIFOモードの時割り込み原因をセット
					rs232cfifo.irqflag = 2;
					//OutputDebugString(_T("READ INT!\n"));
					intr = TRUE;
				}else
#endif
				if (sysport.c & 1) {
					// FIFOモードでないとき、RxREビット（RxRDY割り込み有効）が立っていたら割り込み
					intr = TRUE;
				}
			}
		}
	}

	// 送信済みフラグが立っているとき、TxRE, TxEEビット（TxRDY, TxEMP割り込み有効）が立っていたら割り込み
	if (sysport.c & 6) {
		if (rs232c.send) {
			rs232c.send = 0; // 送信済みフラグ解除
#if defined(SUPPORT_RS232C_FIFO)
			// FIFOモードの時割り込み原因をセット
			rs232cfifo.irqflag = 1;
#endif
			intr = TRUE;
		}
	}

	// WORKAROUND: TxEMP割り込み有効の時、バッファが空ならひたすら割り込み続ける（Win3.1が送信時に永久に割り込み待ちになるのを回避）
//#if defined(SUPPORT_RS232C_FIFO)
//	if(!fifomode)
//#endif
	if (sysport.c & 2) {
		if (!rs232c.send) {
			intr = TRUE;
		}
	}

	// 割り込みフラグが立っていれば割り込み
	if (intr) {
		pic_setirq(4);
	}
}

// ステータス取得
// bit 7: ~CI (RI, RING)
// bit 6: ~CS (CTS)
// bit 5: ~CD (DCD, RLSD)
// bit 4: reserved
// bit 3: reserved
// bit 2: reserved
// bit 1: reserved
// bit 0: ~DSR (DR)
UINT8 rs232c_stat(void) {

	rs232c_open();
	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL, FALSE);
		return(cm_rs232c->getstat(cm_rs232c));
	}
	return 0;
}

// エラー状態取得 (bit0: パリティ, bit1: オーバーラン, bit2: フレーミング, bit3: ブレーク信号)
UINT8 rs232c_geterror(void) {
	
	if (cm_rs232c) {
		UINT8 errorcode = 0;
		cm_rs232c->msg(cm_rs232c, COMMSG_GETERROR, (INT_PTR)(&errorcode));
		return errorcode;
	}
	return 0;
}

// エラー消去
void rs232c_clearerror(void) {
	
	if (cm_rs232c) {
		cm_rs232c->msg(cm_rs232c, COMMSG_CLRERROR, 0);
	}
}

// MIDI panic
void rs232c_midipanic(void) {

	if (cm_rs232c) {
		cm_rs232c->msg(cm_rs232c, COMMSG_MIDIRESET, 0);
	}
}


// ----

// I/O 30h データレジスタ Write
static void IOOUTCALL rs232c_o30(UINT port, REG8 dat) {

	static int lastfail = 0;
	int ret;
	BOOL	fifomode = FALSE; // FIFOモードフラグ

#if defined(SUPPORT_RS232C_FIFO)
	// FIFOモードでないとき130hは無効
	fifomode = (rs232cfifo.port138 & 0x1);
	if(!fifomode && port==0x130){
		lastfail = 0;
		return;
	}
#endif
	if(!(rs232c.cmd & 0x01)) return; // 送信禁止なら抜ける
	if (cm_rs232c) {
		rs232c_writeretry();
#if !defined(__LIBRETRO__)
#if defined(SUPPORT_RS232C_FIFO)
		// FIFOモードの時
		if(fifomode){
			int fifowbufused;

			// バッファに入れる
			fifowbufused = (rs232c_fifo_writebuf_wpos - rs232c_fifo_writebuf_rpos) & RS232C_FIFO_WRITEBUFFER_MASK;
			if(fifowbufused == RS232C_FIFO_WRITEBUFFER-1){
				rs232c_fifo_writebuf_rpos = (rs232c_fifo_writebuf_rpos+1) & RS232C_FIFO_WRITEBUFFER_MASK; // カウンタが1周したら一番古いものを捨てる
			}
			rs232c.result &= ~0x4; // TxEMPを消す
			if(fifowbufused > RS232C_FIFO_WRITEBUFFER * 3 / 4){
				rs232c.result &= ~0x1; // バッファフルならTxRDYを下ろす
			}else{
				rs232c.result |= 0x1; // バッファ空きならTxRDY立てる
			}
			rs232c_fifo_writebuf[rs232c_fifo_writebuf_wpos] = dat;
			rs232c_fifo_writebuf_wpos = (rs232c_fifo_writebuf_wpos+1) & RS232C_FIFO_WRITEBUFFER_MASK; // バッファ書き込み位置を進める
			// バッファに溜まっているデータを書けるだけ書く
			while(fifowbufused = ((rs232c_fifo_writebuf_wpos - rs232c_fifo_writebuf_rpos) & RS232C_FIFO_WRITEBUFFER_MASK)){
				if(fifowbufused > RS232C_FIFO_WRITEBUFFER * 3 / 4){
					rs232c.result &= ~0x1; // バッファフルならTxRDYを下ろす
				}else{
					rs232c.result |= 0x1; // バッファ空きならTxRDY立てる
				}
				cm_rs232c->write(cm_rs232c, (UINT8)rs232c_fifo_writebuf[rs232c_fifo_writebuf_rpos]);
				ret = cm_rs232c->lastwritesuccess(cm_rs232c);
				if(!ret){
					if(fifowbufused > RS232C_FIFO_WRITEBUFFER / 2){
						cm_rs232c->beginblocktranster(cm_rs232c); // バッファが半分以上埋まっていたらブロック転送モードに変更
					}
					if(!(rs232c.result & 0x5)){
						// バッファフルなら割り込みは止めておく
					}else{
						// 1byteでも書けていたら割り込み
						if (sysport.c & 6) { // TxEMP, TxRDYで割り込み？
							rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
							rs232cfifo.irqflag = 1;
#endif
							pic_setirq(4); // 割り込み
						}
						lastfail = 1;
					}
					return; // まだ書き込めないので待つ
				}else{
					// バッファ読み取り位置を進める
					rs232c_fifo_writebuf_rpos = (rs232c_fifo_writebuf_rpos+1) & RS232C_FIFO_WRITEBUFFER_MASK;
				}
			}
			rs232c.result |= 0x5; // バッファ空ならTxEMP,TxRDYを立てる
			cm_rs232c->endblocktranster(cm_rs232c); // ブロック転送モード解除
		}else
#endif
		{
			cm_rs232c->write(cm_rs232c, (UINT8)dat);
			ret = cm_rs232c->lastwritesuccess(cm_rs232c);
			rs232c.result &= ~0x5; // 送信中はTxEMP, TxRDYを下ろす
			if(!ret){
				lastfail = 1;
				return; // まだ書き込めないので待つ
			}
			rs232c.result |= 0x5; // 送信成功したらTxEMP, TxRDYを立てる
		}
#endif
	}
	if (lastfail && (sysport.c & 6)) {
		// 前回失敗していたら即割り込み
		rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
		rs232cfifo.irqflag = 1;
#endif
		pic_setirq(4);
	}
	else {
		rs232c.send = 1; // 割り込みがボーレートよりも高速にならないようにする
	}
	lastfail = 0;
	(void)port;
}

// I/O 32h モードセット,コマンドワード Write
static void IOOUTCALL rs232c_o32(UINT port, REG8 dat) {

	if (!(dat & 0xfd)) {
		rs232c.dummyinst++;
	}
	else {
		if ((rs232c.dummyinst >= 3) && (dat == 0x40)) {
			rs232c.pos = 0;
		}
		rs232c.dummyinst = 0;
	}
	switch(rs232c.pos) {
		case 0x00:			// reset
			rs232c_clearerror();
			rs232c.pos++;
			break;

		case 0x01:			// mode
			rs232c.rawmode = dat;
			if (!(dat & 0x03)) {
				rs232c.mul = 10 * 16;
			}
			else {
				rs232c.mul = ((dat >> 1) & 6) + 10;
				if (dat & 0x10) {
					rs232c.mul += 2;
				}
				switch(dat & 0xc0) {
					case 0x80:
						rs232c.mul += 3;
						break;
					case 0xc0:
						rs232c.mul += 4;
						break;
					default:
						rs232c.mul += 2;
						break;
				}
				switch(dat & 0x03) {
					case 0x01:
						rs232c.mul >>= 1;
						break;
					case 0x03:
						rs232c.mul *= 32;
						break;
					default:
						rs232c.mul *= 8;
						break;
				}
			}
			pit_setrs232cspeed((pit.ch + 2)->value);
			rs232c.pos++;
			break;

		case 0x02:			// cmd
			//sysport.c &= ~7;
			//sysport.c |= (dat & 7);
			//rs232c.pos++;
			if(dat & 0x40){
				// reset
				rs232c.pos = 1;
				rs232c_clearerror();
			}
			if(dat & 0x10){
				rs232c_clearerror();
			}
			if(!(rs232c.cmd & 0x04) && (dat & 0x04)){
				if (cm_rs232c) {
					cm_rs232c->msg(cm_rs232c, COMMSG_PURGE, (INTPTR)&rs232c.cmd);
				}
			}
			rs232c.cmd = dat;
			if (cm_rs232c) {
				cm_rs232c->msg(cm_rs232c, COMMSG_SETCOMMAND, (INTPTR)&rs232c.cmd);
			}
			break;
	}
	(void)port;
}

// I/O 30h データレジスタ Read
static REG8 IOINPCALL rs232c_i30(UINT port) {

	UINT8 ret = rs232c.data;

#if defined(SUPPORT_RS232C_FIFO)
	// FIFOモードでないとき130hは無効
	if(!(rs232cfifo.port138 & 0x1) && port==0x130){
		return 0xff;
	}
#endif
	
	rs232c_writeretry();

#if defined(SUPPORT_RS232C_FIFO)
	if(port==0x130){
		if (rs232c_buf_rpos == rs232c_buf_wpos) {
			// 無理矢理読む
			if ((cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c_buf[rs232c_buf_wpos]))) {
				rs232c_buf_wpos = (rs232c_buf_wpos+1) & RS232C_BUFFER_MASK;
				rs232c.data = rs232c_buf[rs232c_buf_rpos]; // データを1つ取り出し
			}
		}
	}
#endif
	if (rs232c_buf_rpos != rs232c_buf_wpos) {
		rs232c_buf_rpos = (rs232c_buf_rpos+1) & RS232C_BUFFER_MASK; // バッファ読み取り位置を1進める
		rs232c.data = rs232c_buf[rs232c_buf_rpos]; // データを1つ取り出し
	}
#if defined(SUPPORT_RS232C_FIFO)
	if(port==0x130){
		if (rs232c_buf_rpos != rs232c_buf_wpos) { // 受信すべきデータがあるか確認
			int bufused; // 受信リングバッファ使用量
			// 受信リングバッファの使用状況を取得
			bufused = (rs232c_buf_wpos - rs232c_buf_rpos) & RS232C_BUFFER_MASK;

			rs232c.data = rs232c_buf[rs232c_buf_rpos]; // 次のデータを取り出し

			if(bufused > RS232C_BUFFER * 3 / 4){
				// バッファ残りが少ないので急いで割り込み
				//if (sysport.c & 1) {
					rs232cfifo.irqflag = 2;
					pic_setirq(4);
				//}
			}
			//OutputDebugString(_T("READ!\n"));
		}else{
			rs232c.result &= ~0x2;
			rs232cfifo.irqflag = 3;
			pic_setirq(4);
			//rs232c.data = 0xff;
			//pic_resetirq(4);
			//OutputDebugString(_T("READ END!\n"));
		}
	}else
#endif
	{
		int bufused; // 受信リングバッファ使用量
		// 受信リングバッファの使用状況を取得
		bufused = (rs232c_buf_wpos - rs232c_buf_rpos) & RS232C_BUFFER_MASK;
		if(bufused > RS232C_BUFFER * 3 / 4){
			// バッファ残りが少ないので急いで割り込み
			if (sysport.c & 1) {
				// FIFOモードでないとき、RxREビット（RxRDY割り込み有効）が立っていたら割り込み
				pic_setirq(4);
			}
		}else{
			// 余裕があるので次のCallbackのタイミングで割り込み
			rs232c.result &= ~0x2; // RxRDYを消す
		}
	}
	rs232c_removecounter = 0;
	return(ret);
}

// I/O 32h ステータス Read
static REG8 IOINPCALL rs232c_i32(UINT port) {

	UINT8 ret;

	rs232c_writeretry();
	
	ret = rs232c.result;
	ret |= (rs232c_geterror() & 7) << 3;
	if (!(rs232c_stat() & 0x01)) {
		return(ret | 0x80);
	}
	else {
		(void)port;
		return(ret | 0x00);
	}
}

/*
 * I/O 132h FIFO ラインステータス
 * bit 3～7について UNDOCUMENTED 9801/9821 Vol.2に記載の内容は誤り
 * 
 * bit 7: 不明
 * bit 6: ブレイク信号受信
 * bit 5: フレーミングエラー
 * bit 4: オーバーランエラー
 * bit 3: パリティエラー
 * bit 2: RxRDY
 * bit 1: TxRDY
 * bit 0: TxEMP
 */
static REG8 IOINPCALL rs232c_i132(UINT port) {

	UINT8 ret;
	UINT8 err; // エラー状態(bit0: パリティ, bit1: オーバーラン, bit2: フレーミング, bit3: ブレーク信号)
	
	rs232c_writeretry();
	
	ret = rs232c.result; // bit0: TxRDY, bit1: RxRDY, bit2: TxEMP
	err = rs232c_geterror();
	ret = ((ret >> 2) & 0x1) | ((ret << 1) & 0x6) | ((err << 3) & 0x78);
	
	if (!(rs232c_stat() & 0x01)) {
		return(ret | 0x80);
	}
	else {
		(void)port;
		return(ret | 0x00);
	}
}

// I/O 134h モデムステータスレジスタ
#if defined(SUPPORT_RS232C_FIFO)
static REG8 IOINPCALL rs232c_i134(UINT port) {
	
	REG8	ret = 0;
	static UINT8	lastret = 0;
	UINT8	stat = rs232c_stat();
	
	/* stat
	 * bit 7: ~CI (RI, RING)
	 * bit 6: ~CS (CTS)
	 * bit 5: ~CD (DCD, RLSD)
	 * bit 0: ~DSR (DR)
	 */
	if(~stat & 0x20){
		ret |= 0x80; // CD
	}
	if(~stat & 0x80){
		ret |= 0x40; // CI
	}
	if(~stat & 0x01){
		ret |= 0x20; // DR
	}
	if(~stat & 0x40){
		ret |= 0x10; // CS
	}
	ret |= ((lastret >> 4) ^ ret) & 0xf; // diff
	lastret = ret;
	(void)port;
	return(ret);
}

// I/O 136h FIFO割り込み参照レジスタ
static REG8 IOINPCALL rs232c_i136(UINT port) {

	rs232cfifo.port136 ^= 0x40;
	
	if(rs232cfifo.irqflag){
		rs232cfifo.port136 &= ~0xf;
		if(rs232cfifo.irqflag == 3){
			rs232cfifo.port136 |= 0x6;
			//rs232cfifo.irqflag = 0;
			//OutputDebugString(_T("CHECK READ END INT!\n"));
		}else if(rs232cfifo.irqflag == 2){
			rs232cfifo.port136 |= 0x4;
			//OutputDebugString(_T("CHECK READ INT!\n"));
		}else if(rs232cfifo.irqflag == 1){
			rs232cfifo.port136 |= 0x2;
			//rs232cfifo.irqflag = 0;
			//OutputDebugString(_T("CHECK WRITE INT!\n"));
		}
		rs232cfifo.irqflag &= ~0x7;
		pic_resetirq(4);
	}else{
		rs232cfifo.port136 |= 0x1;
		pic_resetirq(4);
		//OutputDebugString(_T("NULL INT!\n"));
	}

	return(rs232cfifo.port136);
}

// I/O 138h FIFOコントロールレジスタ
static void IOOUTCALL rs232c_o138(UINT port, REG8 dat) {

	if(dat & 0x2){
		//int i;
		//// 受信FIFOリセット
		//rs232c_buf_rpos = rs232c_buf_wpos;
		//if(rs232cfifo.irqflag==2) rs232cfifo.irqflag = 0;
		//if(cm_rs232c){
		//	for(i=0;i<256;i++){
		//		cm_rs232c->read(cm_rs232c, &rs232c_buf[rs232c_buf_wpos]);
		//	}
		//}
		//pic_resetirq(4);
	}
	rs232cfifo.port138 = dat;
	(void)port;
}
static REG8 IOINPCALL rs232c_i138(UINT port) {

	UINT8 ret = rs232cfifo.port138;
	
	return(ret);
}

// V-FASTモード通信速度設定　関連: pit.c pit_setrs232cspeed
void rs232c_vfast_setrs232cspeed(UINT8 value) {
	if(value == 0) return;
	if(!(rs232cfifo.vfast & 0x80)) return; // V FASTモードでない場合はなにもしない
	if (cm_rs232c) {
		int speedtbl[16] = {
			0, 115200, 57600, 38400,
			28800, 0, 19200, 0,
			14400, 0, 0, 0,
			9600, 0, 0, 0,
		}; // V-FAST通信速度テーブル
		int newspeed;
		newspeed = speedtbl[rs232cfifo.vfast & 0xf];
		if(newspeed != 0){
			// 通信速度変更
			cm_rs232c->msg(cm_rs232c, COMMSG_CHANGESPEED, (INTPTR)&newspeed);
		}else{
			// V-FAST通信速度テーブルにないとき、通常の速度設定にする
			PITCH	pitch;
			pitch = pit.ch + 2;
			pit_setrs232cspeed(pitch->value);
		}
	}
}

// V-FASTモードレジスタ
static void IOOUTCALL rs232c_o13a(UINT port, REG8 dat) {

	if((rs232cfifo.vfast & 0x80) && !(dat & 0x80)){
		PITCH	pitch;
		// V FASTモード解除
		rs232cfifo.vfast = dat;
		pitch = pit.ch + 2;
		pit_setrs232cspeed(pitch->value);
	}else{
		// V FASTモードセット
		rs232cfifo.vfast = dat;
		rs232c_vfast_setrs232cspeed(rs232cfifo.vfast);
	}
	rs232cfifo.irqflag &= ~0x7;
	(void)port;
}
static REG8 IOINPCALL rs232c_i13a(UINT port) {

	UINT8 ret = rs232cfifo.vfast;
	return(ret);
}
#endif



// ----

static const IOOUT rs232co30[2] = {
					rs232c_o30,	rs232c_o32};

static const IOINP rs232ci30[2] = {
					rs232c_i30,	rs232c_i32};

void rs232c_reset(const NP2CFG *pConfig) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
	rs232c.result = 0x05;
	rs232c.data = 0xff;
	rs232c.send = 1;
	rs232c.pos = 0;
	rs232c.cmd = 0x27; // デフォルトで送受信許可
#if defined(VAEG_FIX)
	rs232c.cmd = 0;
#endif
	rs232c.cmdvalid = 1; // ステートセーブ互換性維持用
	rs232c.reserved = 0;
	rs232c.dummyinst = 0;
	rs232c.mul = 10 * 16;
	rs232c.rawmode = 0;
	
#if defined(SUPPORT_RS232C_FIFO)
	ZeroMemory(&rs232cfifo, sizeof(rs232cfifo));
#endif
	
	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL, TRUE);
	}
	
	(void)pConfig;
}

void rs232c_bind(void) {

	iocore_attachsysoutex(0x0030, 0x0cf1, rs232co30, 2);
	iocore_attachsysinpex(0x0030, 0x0cf1, rs232ci30, 2);
	
#if defined(SUPPORT_RS232C_FIFO)
	iocore_attachout(0x130, rs232c_o30);
	iocore_attachinp(0x130, rs232c_i30);
	iocore_attachout(0x132, rs232c_o32);
	iocore_attachinp(0x132, rs232c_i132);
	//iocore_attachout(0x134, rs232c_o134);
	iocore_attachinp(0x134, rs232c_i134);
	//iocore_attachout(0x136, rs232c_o136);
	iocore_attachinp(0x136, rs232c_i136);
	iocore_attachout(0x138, rs232c_o138);
	iocore_attachinp(0x138, rs232c_i138);
	iocore_attachout(0x13a, rs232c_o13a);
	iocore_attachinp(0x13a, rs232c_i13a);
#endif

	// ステートセーブ互換性維持用
	if(!rs232c.cmdvalid){
		rs232c.cmd = 0x27; // デフォルトで送受信許可
		rs232c.cmdvalid = 1;
	}
}

