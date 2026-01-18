/*
 *	irplock.h
 */
 
// 先に#include <ntddk.h>が必要。

// IrpStackのロック情報
typedef struct tagIRPSTACKLOCK_INFO {
	ULONG isValid;
    PMDL mdlIrpStack;
    PMDL mdlFileObject;
    PMDL mdlFileObjectFileNameBuffer;
    PMDL mdlQueryDirectoryFileName;
    PMDL mdlQueryDirectoryFileNameBuffer;
} IRPSTACKLOCK_INFO, *PIRPSTACKLOCK_INFO;

// ページングされないIrpStackを取得する。NULLの場合失敗
// src: コピー元
// return -> ページングされないIrpStack
PIO_STACK_LOCATION CreateNonPagedPoolIrpStack(PIO_STACK_LOCATION src);

// ページングされないIrpStackを破棄する
// src: ページングされないIrpStack
// back: 書き戻し先IrpStack
VOID ReleaseNonPagedPoolIrpStack(PIO_STACK_LOCATION src, PIO_STACK_LOCATION back);

// ページングされないIrpStackを取得する。NULLの場合失敗
// src: ロックしたいrpStack
// return -> IRPロック情報
IRPSTACKLOCK_INFO LockIrpStack(PIO_STACK_LOCATION src);

// ページングされないIrpStackを破棄する
// src: ロックされたIrpStack
// lpInfo: IRPロック情報へのポインタ
VOID UnlockIrpStack(PIRPSTACKLOCK_INFO lpInfo);
