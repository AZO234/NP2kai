/**
 * @file	serial.c
 * @brief	Keyboard & RS-232C Interface
 *
 * �֘A�Fpit.c, sysport.c
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
	
// RS-232C ��M�o�b�t�@�T�C�Y
#define RS232C_BUFFER		(1 << 6)
#define RS232C_BUFFER_MASK	(RS232C_BUFFER - 1)
// RS-232C ��M�o�b�t�@�ɂ���f�[�^���iRS232C_BUFFER_CLRC�~8���{�[���[�g�j�b�ԓǂݎ���Ȃ���Ύ̂Ă�i�����Ȃ��Ŋ��o�I�Ɏw��j
#define RS232C_BUFFER_CLRC	16

static UINT8 rs232c_buf[RS232C_BUFFER]; // RS-232C ��M�����O�o�b�t�@�@�{���͑��݂��ׂ��łȂ���Windows�o�R�̒ʐM�̓��A���^�C���ɂȂ�Ȃ��ȏ�d���Ȃ��i�l�ɍ����Ȃ��j
static UINT8 rs232c_buf_rpos = 0; // RS-232C ��M�����O�o�b�t�@ �ǂݎ��ʒu
static UINT8 rs232c_buf_wpos = 0; // RS-232C ��M�����O�o�b�t�@ �������݈ʒu
static int rs232c_removecounter = 0; // �f�[�^�j���`�F�b�N�p�J�E���^

// RS-232C FIFO���M�o�b�t�@�T�C�Y
#define RS232C_FIFO_WRITEBUFFER		256
#define RS232C_FIFO_WRITEBUFFER_MASK	(RS232C_FIFO_WRITEBUFFER - 1)
static UINT8 rs232c_fifo_writebuf[RS232C_FIFO_WRITEBUFFER]; // RS-232C FIFO���M�����O�o�b�t�@
static int rs232c_fifo_writebuf_rpos = 0; // RS-232C FIFO���M�����O�o�b�t�@ �ǂݎ��ʒu
static int rs232c_fifo_writebuf_wpos = 0; // RS-232C FIFO���M�����O�o�b�t�@ �������݈ʒu

// RS-232C ���M���g���C
static void rs232c_writeretry() {

	int ret;
//#if defined(SUPPORT_RS232C_FIFO)
//	// FIFO���[�h�̎�
//	if(rs232cfifo.port138 & 0x1){
//		if(rs232c_fifo_writebuf_wpos == rs232c_fifo_writebuf_rpos) return; // �o�b�t�@��Ȃ牽�����Ȃ�
//	}else
//#endif
//	{
		if((rs232c.result & 0x4) != 0) {
			return; // TxEMP�����Ċ��ɑ��M�������Ă��邩�m�F�i�����Ȃ瑗�M�s�v�j
		}
	//}
	if (cm_rs232c) {
		cm_rs232c->writeretry(cm_rs232c); // ���M���g���C
		ret = cm_rs232c->lastwritesuccess(cm_rs232c); // ���M�������`�F�b�N
		if(ret==0){
			return; // ���s���Ă����玟�Ɏ����z��
		}
#if defined(SUPPORT_RS232C_FIFO)
		// FIFO���[�h�̎�
		if(rs232cfifo.port138 & 0x1){
			int fifowbufused;
			
			// �o�b�t�@�ǂݎ��ʒu��i�߂�
			rs232c_fifo_writebuf_rpos = (rs232c_fifo_writebuf_rpos+1) & RS232C_FIFO_WRITEBUFFER_MASK;
			
			fifowbufused = (rs232c_fifo_writebuf_wpos - rs232c_fifo_writebuf_rpos) & RS232C_FIFO_WRITEBUFFER_MASK;
			if(fifowbufused > RS232C_FIFO_WRITEBUFFER * 3 / 4){
				rs232c.result &= ~0x1; // �o�b�t�@�t���Ȃ�TxRDY�����낷
			}else{
				rs232c.result |= 0x1; // �o�b�t�@�󂫂Ȃ�TxRDY�𗧂Ă�
			}
			if(!(rs232c.result & 0x5)){
				// �o�b�t�@�t���Ȃ犄�荞�݂͎~�߂Ă���
			}else{
				if (sysport.c & 6) { // TxEMP, TxRDY�Ŋ��荞�݁H
					rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
					rs232cfifo.irqflag = 1;
#endif
					pic_setirq(4); // ���荞��
				}
			}
			// �o�b�t�@�ɗ��܂��Ă���f�[�^�������邾������
			while(fifowbufused = ((rs232c_fifo_writebuf_wpos - rs232c_fifo_writebuf_rpos) & RS232C_FIFO_WRITEBUFFER_MASK)){
				if(fifowbufused > RS232C_FIFO_WRITEBUFFER * 3 / 4){
					rs232c.result &= ~0x1; // �o�b�t�@�t���Ȃ�TxRDY�����낷
				}else{
					rs232c.result |= 0x1; // �o�b�t�@�󂫂Ȃ�TxRDY�𗧂Ă�
				}
				cm_rs232c->write(cm_rs232c, (UINT8)rs232c_fifo_writebuf[rs232c_fifo_writebuf_rpos]);
				ret = cm_rs232c->lastwritesuccess(cm_rs232c);
				if(!ret){
					return; // �܂��������߂Ȃ��̂ő҂�
				}else{
					// �o�b�t�@�ǂݎ��ʒu��i�߂�
					rs232c_fifo_writebuf_rpos = (rs232c_fifo_writebuf_rpos+1) & RS232C_FIFO_WRITEBUFFER_MASK;
				}
			}
			rs232c.result |= 0x5; // �o�b�t�@�󂫂Ȃ�TxEMP,TxRDY�𗧂Ă�
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
			cm_rs232c->endblocktranster(cm_rs232c); // �u���b�N�]�����[�h����
#endif
		}else
#endif
		{
			rs232c.result |= 0x5; // ���M����������TxEMP, TxRDY�𗧂Ă�
		}
		if (sysport.c & 6) { // TxEMP, TxRDY�Ŋ��荞�݁H
			rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
			rs232cfifo.irqflag = 1;
#endif
			pic_setirq(4); // ���荞��
		}
		else {
			rs232c.send = 1; // ���M�ς݃t���O�𗧂Ă�iTxRE, TxEE�r�b�g�����Ɗ��荞�ݔ����j
		}
	}
}

// RS-232C ������ np2�N������1�񂾂��Ă΂��
void rs232c_construct(void) {

	if(cm_rs232c){
		commng_destroy(cm_rs232c);
	}
	cm_rs232c = NULL;
}

// RS-232C ������ np2�I������1�񂾂��Ă΂��
void rs232c_destruct(void) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
}

// RS-232C �|�[�g�I�[�v�� ���ۂɃA�N�Z�X�����܂Ń|�[�g�I�[�v������Ȃ��d�l
void rs232c_open(void) {

	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL, FALSE);
#if defined(VAEG_FIX)
		cm_rs232c->msg(cm_rs232c, COMMSG_SETRSFLAG, rs232c.cmd & 0x22); /* RTS, DTR */
#endif
	}
}

// RS-232C �ʐM�p�R�[���o�b�N �{�[���[�g��8 ��/�b�ŌĂ΂��i��: 2400bps�Ȃ�2400/8 = 300��/�b�j
void rs232c_callback(void) {

	BOOL	intr = FALSE; // ���荞�݃t���O
	BOOL	fifomode = FALSE; // FIFO���[�h�t���O
	int		bufused; // ��M�����O�o�b�t�@�g�p��
	BOOL	bufremoved = FALSE;
	
	// �J���Ă��Ȃ����RS-232C�|�[�g�I�[�v��
	rs232c_open();

	// ���M�Ɏ��s���Ă����烊�g���C
	rs232c_writeretry();

	// ��M�����O�o�b�t�@�̎g�p�󋵂��擾
	bufused = (rs232c_buf_wpos - rs232c_buf_rpos) & RS232C_BUFFER_MASK;
	if(bufused==0){
		rs232c_removecounter = 0; // �o�b�t�@����Ȃ�Â��f�[�^�폜�J�E���^�����Z�b�g
	}
	
	// ��M�iuPD8251 Recieve Enable�j���`�F�b�N
	if(!(rs232c.cmd & 0x04) && bufused==0) {
		// ��M�֎~�Ŏ�M�o�b�t�@����Ȃ�Ȃ��M���������Ȃ�
	}else{
		// ��M�\���邢�͎�M�o�b�t�@�Ɏc�肪����Ώ�������
#if defined(SUPPORT_RS232C_FIFO)
		// FIFO���[�h�`�F�b�N
		fifomode = (rs232cfifo.port138 & 0x1);
		if(fifomode){
			rs232c_removecounter = 0; // FIFO���[�h�ł͌Â��f�[�^�������Ȃ�
			if(bufused == RS232C_BUFFER-1){
				if(!rs232cfifo.irqflag){
					// ���荞�݂������Ă����犄�荞�݌������Z�b�g���Ċ��荞��
					rs232cfifo.irqflag = 2;
					pic_setirq(4);
				}
				return; // �o�b�t�@�������ς��Ȃ�ҋ@
			}
			if(rs232cfifo.irqflag){
				return; // ���荞�݌����t���O�������Ă���Αҋ@
			}
		}
#endif

		// �Â��f�[�^�폜�J�E���^���C���N�������g
		rs232c_removecounter = (rs232c_removecounter + 1) % RS232C_BUFFER_CLRC;
		if (bufused > 0 && rs232c_removecounter==0 || bufused == RS232C_BUFFER-1){
			rs232c_buf_rpos = (rs232c_buf_rpos+1) & RS232C_BUFFER_MASK; // �J�E���^��1���������ԌÂ����̂��̂Ă�
			bufremoved = TRUE;
		}
		// ��M�iuPD8251 Recieve Enable�j�̎��A�|�[�g���玟�̃f�[�^�ǂݎ��
		if ((rs232c.cmd & 0x04) && (cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c_buf[rs232c_buf_wpos]))) {
			rs232c_buf_wpos = (rs232c_buf_wpos+1) & RS232C_BUFFER_MASK; // �ǂݎ�ꂽ��o�b�t�@�������݈ʒu��i�߂�
		}
		// �o�b�t�@�Ƀf�[�^�������I/O�|�[�g�ǂݎ��f�[�^�ɃZ�b�g���Ċ��荞��
		if (rs232c_buf_rpos != rs232c_buf_wpos) {
			rs232c.data = rs232c_buf[rs232c_buf_rpos]; // �f�[�^��1���o��
			if(!(rs232c.result & 2) || bufremoved) { // RxRDY�����ɗ����Ă���Ή������Ȃ� �� �ꕔ�\�t�g�s������̂��߃o�b�t�@���j�����ꂽ�Ƃ��̂ݍĊ��荞��
				rs232c.result |= 2; // RxRDY�𗧂Ă�
#if defined(SUPPORT_RS232C_FIFO)
				if(fifomode){
					// FIFO���[�h�̎����荞�݌������Z�b�g
					rs232cfifo.irqflag = 2;
					//OutputDebugString(_T("READ INT!\n"));
					intr = TRUE;
				}else
#endif
				if (sysport.c & 1) {
					// FIFO���[�h�łȂ��Ƃ��ARxRE�r�b�g�iRxRDY���荞�ݗL���j�������Ă����犄�荞��
					intr = TRUE;
				}
			}
		}
	}

	// ���M�ς݃t���O�������Ă���Ƃ��ATxRE, TxEE�r�b�g�iTxRDY, TxEMP���荞�ݗL���j�������Ă����犄�荞��
	if (sysport.c & 6) {
		if (rs232c.send) {
			rs232c.send = 0; // ���M�ς݃t���O����
#if defined(SUPPORT_RS232C_FIFO)
			// FIFO���[�h�̎����荞�݌������Z�b�g
			rs232cfifo.irqflag = 1;
#endif
			intr = TRUE;
		}
	}

	// WORKAROUND: TxEMP���荞�ݗL���̎��A�o�b�t�@����Ȃ�Ђ����犄�荞�ݑ�����iWin3.1�����M���ɉi�v�Ɋ��荞�ݑ҂��ɂȂ�̂�����j
//#if defined(SUPPORT_RS232C_FIFO)
//	if(!fifomode)
//#endif
	if (sysport.c & 2) {
		if (!rs232c.send) {
			intr = TRUE;
		}
	}

	// ���荞�݃t���O�������Ă���Ί��荞��
	if (intr) {
		pic_setirq(4);
	}
}

// �X�e�[�^�X�擾
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

// �G���[��Ԏ擾 (bit0: �p���e�B, bit1: �I�[�o�[����, bit2: �t���[�~���O, bit3: �u���[�N�M��)
UINT8 rs232c_geterror(void) {
	
	if (cm_rs232c) {
		UINT8 errorcode = 0;
		cm_rs232c->msg(cm_rs232c, COMMSG_GETERROR, (INT_PTR)(&errorcode));
		return errorcode;
	}
	return 0;
}

// �G���[����
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

// I/O 30h �f�[�^���W�X�^ Write
static void IOOUTCALL rs232c_o30(UINT port, REG8 dat) {

	static int lastfail = 0;
	int ret;
	BOOL	fifomode = FALSE; // FIFO���[�h�t���O

#if defined(SUPPORT_RS232C_FIFO)
	// FIFO���[�h�łȂ��Ƃ�130h�͖���
	fifomode = (rs232cfifo.port138 & 0x1);
	if(!fifomode && port==0x130){
		lastfail = 0;
		return;
	}
#endif
	if(!(rs232c.cmd & 0x01)) return; // ���M�֎~�Ȃ甲����
	if (cm_rs232c) {
		rs232c_writeretry();
#if !defined(__LIBRETRO__)
#if defined(SUPPORT_RS232C_FIFO)
		// FIFO���[�h�̎�
		if(fifomode){
			int fifowbufused;

			// �o�b�t�@�ɓ����
			fifowbufused = (rs232c_fifo_writebuf_wpos - rs232c_fifo_writebuf_rpos) & RS232C_FIFO_WRITEBUFFER_MASK;
			if(fifowbufused == RS232C_FIFO_WRITEBUFFER-1){
				rs232c_fifo_writebuf_rpos = (rs232c_fifo_writebuf_rpos+1) & RS232C_FIFO_WRITEBUFFER_MASK; // �J�E���^��1���������ԌÂ����̂��̂Ă�
			}
			rs232c.result &= ~0x4; // TxEMP������
			if(fifowbufused > RS232C_FIFO_WRITEBUFFER * 3 / 4){
				rs232c.result &= ~0x1; // �o�b�t�@�t���Ȃ�TxRDY�����낷
			}else{
				rs232c.result |= 0x1; // �o�b�t�@�󂫂Ȃ�TxRDY���Ă�
			}
			rs232c_fifo_writebuf[rs232c_fifo_writebuf_wpos] = dat;
			rs232c_fifo_writebuf_wpos = (rs232c_fifo_writebuf_wpos+1) & RS232C_FIFO_WRITEBUFFER_MASK; // �o�b�t�@�������݈ʒu��i�߂�
			// �o�b�t�@�ɗ��܂��Ă���f�[�^�������邾������
			while(fifowbufused = ((rs232c_fifo_writebuf_wpos - rs232c_fifo_writebuf_rpos) & RS232C_FIFO_WRITEBUFFER_MASK)){
				if(fifowbufused > RS232C_FIFO_WRITEBUFFER * 3 / 4){
					rs232c.result &= ~0x1; // �o�b�t�@�t���Ȃ�TxRDY�����낷
				}else{
					rs232c.result |= 0x1; // �o�b�t�@�󂫂Ȃ�TxRDY���Ă�
				}
				cm_rs232c->write(cm_rs232c, (UINT8)rs232c_fifo_writebuf[rs232c_fifo_writebuf_rpos]);
				ret = cm_rs232c->lastwritesuccess(cm_rs232c);
				if(!ret){
					if(fifowbufused > RS232C_FIFO_WRITEBUFFER / 2){
						cm_rs232c->beginblocktranster(cm_rs232c); // �o�b�t�@�������ȏ㖄�܂��Ă�����u���b�N�]�����[�h�ɕύX
					}
					if(!(rs232c.result & 0x5)){
						// �o�b�t�@�t���Ȃ犄�荞�݂͎~�߂Ă���
					}else{
						// 1byte�ł������Ă����犄�荞��
						if (sysport.c & 6) { // TxEMP, TxRDY�Ŋ��荞�݁H
							rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
							rs232cfifo.irqflag = 1;
#endif
							pic_setirq(4); // ���荞��
						}
						lastfail = 1;
					}
					return; // �܂��������߂Ȃ��̂ő҂�
				}else{
					// �o�b�t�@�ǂݎ��ʒu��i�߂�
					rs232c_fifo_writebuf_rpos = (rs232c_fifo_writebuf_rpos+1) & RS232C_FIFO_WRITEBUFFER_MASK;
				}
			}
			rs232c.result |= 0x5; // �o�b�t�@��Ȃ�TxEMP,TxRDY�𗧂Ă�
			cm_rs232c->endblocktranster(cm_rs232c); // �u���b�N�]�����[�h����
		}else
#endif
		{
			cm_rs232c->write(cm_rs232c, (UINT8)dat);
			ret = cm_rs232c->lastwritesuccess(cm_rs232c);
			rs232c.result &= ~0x5; // ���M����TxEMP, TxRDY�����낷
			if(!ret){
				lastfail = 1;
				return; // �܂��������߂Ȃ��̂ő҂�
			}
			rs232c.result |= 0x5; // ���M����������TxEMP, TxRDY�𗧂Ă�
		}
#endif
	}
	if (lastfail && (sysport.c & 6)) {
		// �O�񎸔s���Ă����瑦���荞��
		rs232c.send = 0;
#if defined(SUPPORT_RS232C_FIFO)
		rs232cfifo.irqflag = 1;
#endif
		pic_setirq(4);
	}
	else {
		rs232c.send = 1; // ���荞�݂��{�[���[�g���������ɂȂ�Ȃ��悤�ɂ���
	}
	lastfail = 0;
	(void)port;
}

// I/O 32h ���[�h�Z�b�g,�R�}���h���[�h Write
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
				cm_rs232c->msg(cm_rs232c, COMMSG_PURGE, (INTPTR)&rs232c.cmd);
			}
			rs232c.cmd = dat;
			if (cm_rs232c) {
				cm_rs232c->msg(cm_rs232c, COMMSG_SETCOMMAND, (INTPTR)&rs232c.cmd);
			}
			break;
	}
	(void)port;
}

// I/O 30h �f�[�^���W�X�^ Read
static REG8 IOINPCALL rs232c_i30(UINT port) {

	UINT8 ret = rs232c.data;

#if defined(SUPPORT_RS232C_FIFO)
	// FIFO���[�h�łȂ��Ƃ�130h�͖���
	if(!(rs232cfifo.port138 & 0x1) && port==0x130){
		return 0xff;
	}
#endif
	
	rs232c_writeretry();

#if defined(SUPPORT_RS232C_FIFO)
	if(port==0x130){
		if (rs232c_buf_rpos == rs232c_buf_wpos) {
			// ������ǂ�
			if ((cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c_buf[rs232c_buf_wpos]))) {
				rs232c_buf_wpos = (rs232c_buf_wpos+1) & RS232C_BUFFER_MASK;
				rs232c.data = rs232c_buf[rs232c_buf_rpos]; // �f�[�^��1���o��
			}
		}
	}
#endif
	if (rs232c_buf_rpos != rs232c_buf_wpos) {
		rs232c_buf_rpos = (rs232c_buf_rpos+1) & RS232C_BUFFER_MASK; // �o�b�t�@�ǂݎ��ʒu��1�i�߂�
		rs232c.data = rs232c_buf[rs232c_buf_rpos]; // �f�[�^��1���o��
	}
#if defined(SUPPORT_RS232C_FIFO)
	if(port==0x130){
		if (rs232c_buf_rpos != rs232c_buf_wpos) { // ��M���ׂ��f�[�^�����邩�m�F
			int bufused; // ��M�����O�o�b�t�@�g�p��
			// ��M�����O�o�b�t�@�̎g�p�󋵂��擾
			bufused = (rs232c_buf_wpos - rs232c_buf_rpos) & RS232C_BUFFER_MASK;

			rs232c.data = rs232c_buf[rs232c_buf_rpos]; // ���̃f�[�^�����o��

			if(bufused > RS232C_BUFFER * 3 / 4){
				// �o�b�t�@�c�肪���Ȃ��̂ŋ}���Ŋ��荞��
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
		int bufused; // ��M�����O�o�b�t�@�g�p��
		// ��M�����O�o�b�t�@�̎g�p�󋵂��擾
		bufused = (rs232c_buf_wpos - rs232c_buf_rpos) & RS232C_BUFFER_MASK;
		if(bufused > RS232C_BUFFER * 3 / 4){
			// �o�b�t�@�c�肪���Ȃ��̂ŋ}���Ŋ��荞��
			if (sysport.c & 1) {
				// FIFO���[�h�łȂ��Ƃ��ARxRE�r�b�g�iRxRDY���荞�ݗL���j�������Ă����犄�荞��
				pic_setirq(4);
			}
		}else{
			// �]�T������̂Ŏ���Callback�̃^�C�~���O�Ŋ��荞��
			rs232c.result &= ~0x2; // RxRDY������
		}
	}
	rs232c_removecounter = 0;
	return(ret);
}

// I/O 32h �X�e�[�^�X Read
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
 * I/O 132h FIFO ���C���X�e�[�^�X
 * bit 3�`7�ɂ��� UNDOCUMENTED 9801/9821 Vol.2�ɋL�ڂ̓��e�͌��
 * 
 * bit 7: �s��
 * bit 6: �u���C�N�M����M
 * bit 5: �t���[�~���O�G���[
 * bit 4: �I�[�o�[�����G���[
 * bit 3: �p���e�B�G���[
 * bit 2: RxRDY
 * bit 1: TxRDY
 * bit 0: TxEMP
 */
static REG8 IOINPCALL rs232c_i132(UINT port) {

	UINT8 ret;
	UINT8 err; // �G���[���(bit0: �p���e�B, bit1: �I�[�o�[����, bit2: �t���[�~���O, bit3: �u���[�N�M��)
	
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

// I/O 134h ���f���X�e�[�^�X���W�X�^
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

// I/O 136h FIFO���荞�ݎQ�ƃ��W�X�^
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

// I/O 138h FIFO�R���g���[�����W�X�^
static void IOOUTCALL rs232c_o138(UINT port, REG8 dat) {

	if(dat & 0x2){
		//int i;
		//// ��MFIFO���Z�b�g
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

// V-FAST���[�h�ʐM���x�ݒ�@�֘A: pit.c pit_setrs232cspeed
void rs232c_vfast_setrs232cspeed(UINT8 value) {
	if(value == 0) return;
	if(!(rs232cfifo.vfast & 0x80)) return; // V FAST���[�h�łȂ��ꍇ�͂Ȃɂ����Ȃ�
	if (cm_rs232c) {
		int speedtbl[16] = {
			0, 115200, 57600, 38400,
			28800, 0, 19200, 0,
			14400, 0, 0, 0,
			9600, 0, 0, 0,
		}; // V-FAST�ʐM���x�e�[�u��
		int newspeed;
		newspeed = speedtbl[rs232cfifo.vfast & 0xf];
		if(newspeed != 0){
			// �ʐM���x�ύX
			cm_rs232c->msg(cm_rs232c, COMMSG_CHANGESPEED, (INTPTR)&newspeed);
		}else{
			// V-FAST�ʐM���x�e�[�u���ɂȂ��Ƃ��A�ʏ�̑��x�ݒ�ɂ���
			PITCH	pitch;
			pitch = pit.ch + 2;
			pit_setrs232cspeed(pitch->value);
		}
	}
}

// V-FAST���[�h���W�X�^
static void IOOUTCALL rs232c_o13a(UINT port, REG8 dat) {

	if((rs232cfifo.vfast & 0x80) && !(dat & 0x80)){
		PITCH	pitch;
		// V FAST���[�h����
		rs232cfifo.vfast = dat;
		pitch = pit.ch + 2;
		pit_setrs232cspeed(pitch->value);
	}else{
		// V FAST���[�h�Z�b�g
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
	rs232c.cmd = 0x27; // �f�t�H���g�ő���M����
#if defined(VAEG_FIX)
	rs232c.cmd = 0;
#endif
	rs232c.cmdvalid = 1; // �X�e�[�g�Z�[�u�݊����ێ��p
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

	// �X�e�[�g�Z�[�u�݊����ێ��p
	if(!rs232c.cmdvalid){
		rs232c.cmd = 0x27; // �f�t�H���g�ő���M����
		rs232c.cmdvalid = 1;
	}
}

