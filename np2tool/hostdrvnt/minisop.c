/*
 *	minisop.c
 *	ファイル単位のFileObject->SectionObjectPointer(SOP)を簡易実装します。
 *	与えられたパスが一致しているものを同じファイルと見做して、一致している場合は同じSOPを返します。
 *
 *	※ファイル単位で共通にする要件は判明している限りMemory-Mapped File（EXEの実行時にも使用）で影響します。
 *	それ以外ではIRP_MJ_CREATE単位でSOP生成しても問題なく動きます。
 *	
 *	HACK: おまけ機能　minisop.hで#define USE_CACHEHACKすると、
 *	キャッシュ未実装でもメモリマップトファイルが使えるWORKAROUND関数が使える。
 */

#include <ntddk.h>
#include "minisop.h"

#ifdef USE_CACHEHACK
#include "miniifs.h"
#endif  /* USE_CACHEHACK */

#define PENDING_SOP_MAX	65535

static VOID MiniSOP_ReleaseAllSOP(void);
static BOOLEAN MiniSOP_ExpandSOPList(void);

// SECTION_OBJECT_POINTERS保持用
typedef struct tagMINISOP_SOP {
	UNICODE_STRING path; // 対象ファイルのパス　SOPはファイル毎に割り当てるので重複はない
    PSECTION_OBJECT_POINTERS pSOP; // 割り当てたSECTION_OBJECT_POINTERSへのポインタ
    ULONG refCount; // 参照カウント 0になったらpath文字列とpSOPを解放する
} MINISOP_SOP, *PMINISOP_SOP;

// SOPはファイル・ディレクトリ単位で管理する必要あり
static ULONG g_pendingSOPListCount = 0;
static PMINISOP_SOP g_pendingSOPList = NULL;

// 参照カウントを増やしてSOPを取得　ない場合は新規作成　作れなければ-1
static BOOLEAN MiniSOP_ExpandSOPList(){
	PMINISOP_SOP oldBuffer = g_pendingSOPList;
	PMINISOP_SOP newBuffer = NULL;
	ULONG oldCount = g_pendingSOPListCount;
	ULONG newCount;
	if(g_pendingSOPListCount == PENDING_SOP_MAX){
		return FALSE; // 確保できない
	}
	newCount = oldCount + 8; // あまり頻繁だと大変なのでとりあえず8ファイルずつ拡張
	if(newCount > PENDING_SOP_MAX){
		newCount = PENDING_SOP_MAX;
	}
    newBuffer = ExAllocatePool(NonPagedPool, sizeof(MINISOP_SOP) * newCount);
    if(newBuffer == NULL){
		return FALSE; // 確保できない
    }
	RtlZeroMemory(newBuffer, sizeof(MINISOP_SOP) * newCount);
    if(g_pendingSOPList != NULL){
    	// 現在の内容をコピー
		RtlCopyMemory(newBuffer, oldBuffer, sizeof(MINISOP_SOP) * oldCount);
    }
    g_pendingSOPList = newBuffer;
    g_pendingSOPListCount = newCount;
    if(oldBuffer != NULL){
    	ExFreePool(oldBuffer);
    }
	return TRUE; // OK
}

// SOPリストを解放する。中身のメモリも解放される。ドライバのアンロード時に呼び出す。
BOOLEAN MiniSOP_ReleaseSOPList(){
	PMINISOP_SOP oldBuffer = g_pendingSOPList;
	MiniSOP_ReleaseAllSOP(); // SOPを全てリリース
	g_pendingSOPListCount = 0;
	g_pendingSOPList = NULL;
    if(oldBuffer != NULL){
    	ExFreePool(oldBuffer);
    }
	return TRUE; // OK
}

// 指定したファイル名に対応するSOPリストのインデックスを返す。
// リストに無い場合は新規割り当てする。リソース不足で割り当てできない場合は-1を返す。
// 呼ぶ度に参照カウントされるので、不要になったら必ず同じ回数HostdrvReleaseSOPByIndexまたはHostdrvReleaseSOPをすること。
LONG MiniSOP_GetSOPIndex(UNICODE_STRING path){
	int i;
	for(i=0;i<g_pendingSOPListCount;i++){
		if(g_pendingSOPList[i].path.Buffer && RtlEqualUnicodeString(&path, &g_pendingSOPList[i].path, FALSE)){
			g_pendingSOPList[i].refCount++;
			return i; // 既にある
		}
	}
	// なかったので新規作成
	for(i=0;i<g_pendingSOPListCount;i++){
		if(!g_pendingSOPList[i].path.Buffer){
			ULONG allocSize = path.Length;
			g_pendingSOPList[i].path.Length = path.Length;
			if(allocSize == 0) allocSize = 1; // 必ず1byteは確保
			g_pendingSOPList[i].path.MaximumLength = allocSize;
	        g_pendingSOPList[i].path.Buffer = ExAllocatePool(NonPagedPool, allocSize);
	        if(g_pendingSOPList[i].path.Buffer == NULL){
				return -1;
	        }
	        RtlCopyMemory(g_pendingSOPList[i].path.Buffer, path.Buffer, g_pendingSOPList[i].path.Length);
	        g_pendingSOPList[i].pSOP = ExAllocatePool(NonPagedPool, sizeof(SECTION_OBJECT_POINTERS));
	        if(g_pendingSOPList[i].pSOP == NULL){
	        	ExFreePool(g_pendingSOPList[i].path.Buffer);
	        	g_pendingSOPList[i].path.Buffer = NULL;
	        	g_pendingSOPList[i].path.Length = 0;
	        	g_pendingSOPList[i].path.MaximumLength = 0;
				return -1;
	        }
	        RtlZeroMemory(g_pendingSOPList[i].pSOP, sizeof(SECTION_OBJECT_POINTERS));
			g_pendingSOPList[i].refCount++;
	        return i;
		}
	}
	// 空きがなかったので拡張を試みる
	if(MiniSOP_ExpandSOPList()){
		// 拡張できたらリトライ
		return MiniSOP_GetSOPIndex(path);
	}
	return -1;
}

// 指定したSOPリストのインデックスに対応するSECTION_OBJECT_POINTERSへのポインタを返す。
// 無効なインデックスであればNULLが返る
PSECTION_OBJECT_POINTERS MiniSOP_GetSOP(LONG i){
	if(i < 0 || g_pendingSOPListCount <= i){
		return NULL; // 無効
	}
	return g_pendingSOPList[i].pSOP;
}

// 指定した番号のSOPの参照カウントを減らす　0になったら解放
VOID MiniSOP_ReleaseSOPByIndex(LONG i){
	if(i < 0 || g_pendingSOPListCount <= i){
		return; // 無効
	}
	if(g_pendingSOPList[i].refCount > 0){
		g_pendingSOPList[i].refCount--;
	}
	if(g_pendingSOPList[i].refCount==0){
		// 参照無くなったので消す
    	ExFreePool(g_pendingSOPList[i].path.Buffer);
    	g_pendingSOPList[i].path.Buffer = NULL;
    	g_pendingSOPList[i].path.Length = 0;
    	ExFreePool(g_pendingSOPList[i].pSOP);
    	g_pendingSOPList[i].pSOP = NULL;
	}
}

// 指定したSOPの参照カウントを減らす　0になったら解放
VOID MiniSOP_ReleaseSOP(PSECTION_OBJECT_POINTERS lpSOP){
	int i;
	for(i=0;i<g_pendingSOPListCount;i++){
		if(g_pendingSOPList[i].pSOP == lpSOP){
			MiniSOP_ReleaseSOPByIndex(i);
			return;
		}
	}
}

// 全てのSOPを解放
static VOID MiniSOP_ReleaseAllSOP(){
	int i;
    // SOPリスト消す
    for (i = 0; i < g_pendingSOPListCount; i++) {
        if (g_pendingSOPList[i].pSOP != NULL) {
        	ExFreePool(g_pendingSOPList[i].path.Buffer);
        	g_pendingSOPList[i].path.Buffer = NULL;
        	g_pendingSOPList[i].path.Length = 0;
        	ExFreePool(g_pendingSOPList[i].pSOP);
        	g_pendingSOPList[i].pSOP = NULL;
        	g_pendingSOPList[i].refCount = 0;
        }
    }
}

VOID MiniSOP_SendFlushSOP(){
	int i;
	IO_STATUS_BLOCK iosb = {0};
    for (i = 0; i < g_pendingSOPListCount; i++) {
        if (g_pendingSOPList[i].pSOP != NULL) {
			if(g_pendingSOPList[i].pSOP->DataSectionObject){
				CcFlushCache(g_pendingSOPList[i].pSOP, NULL, 0, &iosb);
				MmFlushImageSection(g_pendingSOPList[i].pSOP, MmFlushForDelete);
			}
        }
    }
}


// HACK: おまけ機能　キャッシュ未実装でもメモリマップトファイルが使えるようにする。
#ifdef USE_CACHEHACK

// 高速I/Oの処理可否を返す関数。使わないので常時FALSEを返す。
BOOLEAN HostdrvFastIoCheckIfPossible (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject)
{
    return FALSE;
}

static FAST_IO_DISPATCH g_FastIoDispatch; // 常時高速I/O不可を返すだけのダミー

// 未実装の中途半端キャッシュのための初期化。DriverEntryで呼ぶこと。
// DriverObject: DriverEntryの引数で受け取ったDriverObject
VOID MiniSOP_InitializeCache(PDRIVER_OBJECT DriverObject){
    // 基本的にキャッシュ未対応なのでチェック関数以外はNULLで
    RtlZeroMemory( &g_FastIoDispatch, sizeof( FAST_IO_DISPATCH ));
    g_FastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    g_FastIoDispatch.FastIoCheckIfPossible = HostdrvFastIoCheckIfPossible;
    DriverObject->FastIoDispatch = &g_FastIoDispatch;
}
// IRP_MJ_CREATEが成功する度に呼ぶ。キャッシュ消去してディスクからの再READを強制。
// irpSp: IRPスタックポインタ
VOID MiniSOP_HandleMjCreateCache(PIO_STACK_LOCATION irpSp){
    CC_FILE_SIZES sizes = {0};
	PFILE_OBJECT pFileObject = irpSp->FileObject;
    
	if(!pFileObject || !pFileObject->SectionObjectPointer){
		return;
	}
	
	// キャッシュサイズを0にすることで強制的にディスクから再読込させる
	CcSetFileSizes(pFileObject, &sizes); 
}
// IRP_MJ_CLEANUPが成功する度に呼ぶ。キャッシュの内容を強制的にディスクにWRITEさせる。
// irpSp: IRPスタックポインタ
VOID MiniSOP_HandleMjCleanupCache(PIO_STACK_LOCATION irpSp){
	IO_STATUS_BLOCK iosb = {0};
	PFILE_OBJECT pFileObject = irpSp->FileObject;

	if(!pFileObject || !pFileObject->SectionObjectPointer || !pFileObject->SectionObjectPointer->DataSectionObject){
		return;
	}
	
	// キャッシュの内容を強制的にディスクへ書き戻させる
	CcFlushCache(pFileObject->SectionObjectPointer, NULL, 0, &iosb);
	//MmFlushImageSection(pFileObject->SectionObjectPointer, MmFlushForDelete);
	//CcUninitializeCacheMap(pFileObject, NULL, NULL);
}
// IRP_MJ_SET_INFORMATIONで実際のファイルサイズを変更する 前に 呼ぶ。キャッシュのファイル長さ変更を通知。
// 実際の処理は、指定されたファイル長さ以上で一旦WRITEされる→SetInformationで切り捨てる　という操作になる
// irpSp: IRPスタックポインタ
VOID MiniSOP_HandlePreMjSetInformationCache(PIO_STACK_LOCATION irpSp){
    if(irpSp->Parameters.QueryFile.FileInformationClass == FileEndOfFileInformation || 
       irpSp->Parameters.QueryFile.FileInformationClass == FileAllocationInformation){
		PFILE_OBJECT pFileObject = irpSp->FileObject;
    	if(pFileObject && pFileObject->SectionObjectPointer){
			IO_STATUS_BLOCK iosb = {0};
            CC_FILE_SIZES sizes = {0};
			CcFlushCache(pFileObject->SectionObjectPointer, NULL, 0, &iosb); // 現在のキャッシュをディスクへ書き戻し
			CcSetFileSizes(pFileObject, &sizes); // キャッシュをリセット
    	}
	}
}

#endif  /* USE_CACHEHACK */

