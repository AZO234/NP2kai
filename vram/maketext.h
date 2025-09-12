
enum {
	TEXTXMAX		= 80,
	TEXTYMAX		= 400,

	TXTATR_ST		= 0x01,		// ~�V�[�N���b�g
	TXTATR_BL		= 0x02,		// �u�����N
	TXTATR_RV		= 0x04,		// ���o�[�X
	TXTATR_UL		= 0x08,		// �A���_�[���C��
	TXTATR_VL		= 0x10,		// �o�[�`�J�����C��
	TXTATR_BG		= 0x10,		// �ȈՃO���t
	TEXTATR_RGB		= 0xe0		// �r�b�g���т�GRB�̏�
};

typedef struct {
	UINT8	timing;
	UINT8	count;
	UINT8	renewal;
	UINT8	gaiji;
	UINT8	attr;
	UINT8	curdisp;
	UINT8	curdisplast;
	UINT8	blink;
	UINT8	blinkdisp;
	UINT16	curpos;
} TRAM_T;


#ifdef __cplusplus
extern "C" {
#endif

extern	TRAM_T	tramflag;

void maketext_initialize(void);
void maketext_reset(void);
UINT8 maketext_curblink(void);
void maketext(int text_renewal);
void maketext40(int text_renewal);

#ifdef __cplusplus
}
#endif

