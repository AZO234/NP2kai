
#ifndef	NP2_I386HAX_HAXCORE_H__
#define	NP2_I386HAX_HAXCORE_H__

#if defined(SUPPORT_IA32_HAXM)

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
	UINT8	available; // HAXM使用可
	UINT8	enable; // HAXM有効
	UINT8	emumode; // 猫CPUで代替処理中
	HANDLE	hDevice; // HAXMデバイスのハンドル
	HANDLE	hVMDevice; // HAXM仮想マシンデバイスのハンドル
	HANDLE	hVCPUDevice; // HAXM仮想CPUデバイスのハンドル
	UINT32	vm_id; // HAXM仮想マシンID
	HAX_TUNNEL_INFO	tunnel; // HAXM仮想マシンとのデータやりとり用tunnel

	UINT8 bioshookenable; // デバッグレジスタによるエミュレーションBIOSフック有効
} NP2_HAX;
typedef struct {
	HAX_VCPU_STATE	state; // HAXM仮想CPUのレジスタ
	HAX_FX_LAYOUT	fpustate; // HAXM仮想CPUのFPUレジスタ
	HAX_MSR_DATA	msrstate; // HAXM仮想CPUのMSR
	HAX_VCPU_STATE	default_state; // HAXM仮想CPUのレジスタ（デフォルト値）
	HAX_FX_LAYOUT	default_fpustate; // HAXM仮想CPUのFPUレジスタ（デフォルト値）
	HAX_MSR_DATA	default_msrstate; // HAXM仮想CPUのMSR（デフォルト値）
	UINT8 update_regs; // 要レジスタ更新
	UINT8 update_segment_regs; // 要セグメントレジスタ更新
	UINT8 update_fpu; // 要FPUレジスタ更新

	UINT8 irq_req[256]; // 割り込み待機バッファ。大気中の割り込みベクタが格納される
	UINT8 irq_reqidx_cur; // 割り込み待機バッファの読み取り位置
	UINT8 irq_reqidx_end; // 割り込み待機バッファの書き込み位置
} NP2_HAX_STAT;
typedef struct {
	UINT8 running; // HAXM CPU実行中フラグ

	// タイミング調整用（performance counter使用）
	int64_t lastclock; // 前回のクロック
	int64_t clockpersec; // 1秒あたりクロック数
	int64_t clockcount; // 現在のクロック

	UINT8 I_ratio;

	UINT32 lastA20en; // 前回A20ラインが有効だったか
	UINT32 lastITFbank; // 前回ITFバンクを使用していたか
	UINT32 lastVGA256linear; // 前回256色モードのリニアアドレスを使用していたか
	UINT32 lastVRAMMMIO; // VRAMのメモリアドレスがMMIOモードか
	
	UINT8 hurryup; // タイミングが遅れているので急ぐべし

	UINT8 hltflag; // HLT命令で停止中フラグ

	UINT8 allocwabmem; // WAB vramptr登録済みなら1

	UINT8 ready_for_reset;
} NP2_HAX_CORE;

#define NP2HAX_I_RATIO_MAX	1024

extern	NP2_HAX			np2hax;
extern	NP2_HAX_STAT	np2haxstat;
extern	NP2_HAX_CORE	np2haxcore;

#ifdef __cplusplus
}
#endif

UINT8 i386hax_check(void); // HAXM使用可能チェック
void i386hax_initialize(void); // HAXM初期化
void i386hax_createVM(void); // HAXM仮想マシン作成
void i386hax_resetVMMem(void); // HAXM仮想マシンリセット（メモリ周り）
void i386hax_resetVMCPU(void); // HAXM仮想マシンリセット（CPU周り）
void i386hax_disposeVM(void); // HAXM仮想マシン破棄
void i386hax_deinitialize(void); // HAXM解放

void i386hax_vm_exec(void); // HAXM仮想CPUの実行

void i386hax_vm_allocmemory(void); // メモリ領域を登録（基本領域）
void i386hax_vm_allocmemoryex(UINT8 *vramptr, UINT32 size); // メモリ領域を登録（汎用）

// ゲスト物理アドレス(Guest Physical Address; GPA)にホストの仮想アドレス(Host Virtual Address; HVA)を割り当て
void i386hax_vm_setmemory(void); // 00000h～BFFFFhまで
void i386hax_vm_setbankmemory(void); // A0000h～F7FFFhまで
void i386hax_vm_setitfmemory(UINT8 isitfbank); // F8000h～FFFFFhまで
void i386hax_vm_sethmemory(UINT8 a20en); // 100000h～10FFFFhまで
void i386hax_vm_setextmemory(void); // 110000h以降
void i386hax_vm_setvga256linearmemory(void); // 0xF00000～0xF80000
//void i386hax_vm_setwabmemory(UINT8 *vramptr, UINT32 addr, UINT32 size);

// 汎用 メモリ領域割り当て
void i386hax_vm_setmemoryarea(UINT8 *vramptr, UINT32 addr, UINT32 size);
void i386hax_vm_removememoryarea(UINT8 *vramptr, UINT32 addr, UINT32 size);

void ia32hax_copyregHAXtoNP2(void);
void ia32hax_copyregNP2toHAX(void);

#endif

#endif	/* !NP2_I386HAX_HAXCORE_H__ */
