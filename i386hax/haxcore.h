
#ifndef	NP2_I386HAX_HAXCORE_H__
#define	NP2_I386HAX_HAXCORE_H__

#if defined(SUPPORT_IA32_HAXM)

#include "haxfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#if !defined(_WINDOWS)
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int64_t LONGLONG;

typedef int HANDLE;
#endif

typedef struct _NP2_HAX {
	UINT8	available; // HAXM�g�p��
	UINT8	enable; // HAXM�L��
	UINT8	emumode; // �LCPU�ő�֏�����
	HANDLE	hDevice; // HAXM�f�o�C�X�̃n���h��
	HANDLE	hVMDevice; // HAXM���z�}�V���f�o�C�X�̃n���h��
	HANDLE	hVCPUDevice; // HAXM���zCPU�f�o�C�X�̃n���h��
	UINT32	vm_id; // HAXM���z�}�V��ID
	HAX_TUNNEL_INFO	tunnel; // HAXM���z�}�V���Ƃ̃f�[�^���Ƃ�ptunnel

	UINT8 bioshookenable; // �f�o�b�O���W�X�^�ɂ��G�~�����[�V����BIOS�t�b�N�L��
} NP2_HAX;
typedef struct {
	HAX_VCPU_STATE	state; // HAXM���zCPU�̃��W�X�^
	HAX_FX_LAYOUT	fpustate; // HAXM���zCPU��FPU���W�X�^
	HAX_MSR_DATA	msrstate; // HAXM���zCPU��MSR
	HAX_VCPU_STATE	default_state; // HAXM���zCPU�̃��W�X�^�i�f�t�H���g�l�j
	HAX_FX_LAYOUT	default_fpustate; // HAXM���zCPU��FPU���W�X�^�i�f�t�H���g�l�j
	HAX_MSR_DATA	default_msrstate; // HAXM���zCPU��MSR�i�f�t�H���g�l�j
	UINT8 update_regs; // �v���W�X�^�X�V
	UINT8 update_segment_regs; // �v�Z�O�����g���W�X�^�X�V
	UINT8 update_fpu; // �vFPU���W�X�^�X�V

	UINT8 irq_req[256]; // ���荞�ݑҋ@�o�b�t�@�B��C���̊��荞�݃x�N�^���i�[�����
	UINT8 irq_reqidx_cur; // ���荞�ݑҋ@�o�b�t�@�̓ǂݎ��ʒu
	UINT8 irq_reqidx_end; // ���荞�ݑҋ@�o�b�t�@�̏������݈ʒu
} NP2_HAX_STAT;
typedef struct {
	UINT8 running; // HAXM CPU���s���t���O

	// �^�C�~���O�����p�iperformance counter�g�p�j
	LARGE_INTEGER lastclock; // �O��̃N���b�N
	LARGE_INTEGER clockpersec; // 1�b������N���b�N��
	LARGE_INTEGER clockcount; // ���݂̃N���b�N

	UINT8 I_ratio;

	UINT32 lastA20en; // �O��A20���C�����L����������
	UINT32 lastITFbank; // �O��ITF�o���N���g�p���Ă�����
	UINT32 lastVGA256linear; // �O��256�F���[�h�̃��j�A�A�h���X���g�p���Ă�����
	UINT32 lastVRAMMMIO; // VRAM�̃������A�h���X��MMIO���[�h��
	
	UINT8 hurryup; // �^�C�~���O���x��Ă���̂ŋ}���ׂ�

	UINT8 hltflag; // HLT���߂Œ�~���t���O

	UINT8 allocwabmem; // WAB vramptr�o�^�ς݂Ȃ�1

	UINT8 ready_for_reset;
} NP2_HAX_CORE;

#define NP2HAX_I_RATIO_MAX	1024

extern	NP2_HAX			np2hax;
extern	NP2_HAX_STAT	np2haxstat;
extern	NP2_HAX_CORE	np2haxcore;

#ifdef __cplusplus
}
#endif

UINT8 i386hax_check(void); // HAXM�g�p�\�`�F�b�N
void i386hax_initialize(void); // HAXM������
void i386hax_createVM(void); // HAXM���z�}�V���쐬
void i386hax_resetVMMem(void); // HAXM���z�}�V�����Z�b�g�i����������j
void i386hax_resetVMCPU(void); // HAXM���z�}�V�����Z�b�g�iCPU����j
void i386hax_disposeVM(void); // HAXM���z�}�V���j��
void i386hax_deinitialize(void); // HAXM���

void i386hax_vm_exec(void); // HAXM���zCPU�̎��s

void i386hax_vm_allocmemory(void); // �������̈��o�^�i��{�̈�j
void i386hax_vm_allocmemoryex(UINT8 *vramptr, UINT32 size); // �������̈��o�^�i�ėp�j

// �Q�X�g�����A�h���X(Guest Physical Address; GPA)�Ƀz�X�g�̉��z�A�h���X(Host Virtual Address; HVA)�����蓖��
void i386hax_vm_setmemory(void); // 00000h�`BFFFFh�܂�
void i386hax_vm_setbankmemory(void); // A0000h�`F7FFFh�܂�
void i386hax_vm_setitfmemory(UINT8 isitfbank); // F8000h�`FFFFFh�܂�
void i386hax_vm_sethmemory(UINT8 a20en); // 100000h�`10FFFFh�܂�
void i386hax_vm_setextmemory(void); // 110000h�ȍ~
void i386hax_vm_setvga256linearmemory(void); // 0xF00000�`0xF80000
//void i386hax_vm_setwabmemory(UINT8 *vramptr, UINT32 addr, UINT32 size);

// �ėp �������̈抄�蓖��
void i386hax_vm_setmemoryarea(UINT8 *vramptr, UINT32 addr, UINT32 size);
void i386hax_vm_removememoryarea(UINT8 *vramptr, UINT32 addr, UINT32 size);

void ia32hax_copyregHAXtoNP2(void);
void ia32hax_copyregNP2toHAX(void);

#endif

#endif	/* !NP2_I386HAX_HAXCORE_H__ */
