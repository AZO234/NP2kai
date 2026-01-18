/*
 *	irplock.c
 *	IrpStackがページングされないようにするためのユーティリティ
 */

#include <ntddk.h>
#include "irplock.h"

// ページフォールトを起こしそうな危ないものをコピー とりあえず使っている範囲で
PIO_STACK_LOCATION CreateNonPagedPoolIrpStack(PIO_STACK_LOCATION src) {
    PIO_STACK_LOCATION dst = NULL;
    PFILE_OBJECT fileObject = NULL;
    PWSTR fileObjectFileNameBuffer = NULL;
    PUNICODE_STRING queryDirectoryFileName = NULL;
    PWSTR queryDirectoryFileNameBuffer = NULL;
    
    // PIO_STACK_LOCATION IrpSp
    dst = (PIO_STACK_LOCATION)ExAllocatePoolWithTag(NonPagedPool, sizeof(IO_STACK_LOCATION), 'cPSL');
    if (!dst) {
        goto failed;
    }
    RtlCopyMemory(dst, src, sizeof(IO_STACK_LOCATION));
    // IrpSp->FileObject
    if(src->FileObject){
	    fileObject = (PFILE_OBJECT)ExAllocatePool(NonPagedPool, sizeof(FILE_OBJECT));
	    if (!fileObject) {
	        goto failed;
	    }
	    RtlCopyMemory(fileObject, src->FileObject, sizeof(FILE_OBJECT)); // src->FileObjectが必ずしもFILE_OBJECTのサイズとは限らないが、少なくともnp2で使う範囲はコピーする
	    dst->FileObject = fileObject;
	    
	    // IrpSp->FileObject->FileName.Buffer
	    if (fileObject->FileName.MaximumLength > 0){
		    fileObjectFileNameBuffer = (PWSTR)ExAllocatePool(NonPagedPool, fileObject->FileName.MaximumLength);
		    if (!fileObjectFileNameBuffer) {
		        goto failed;
		    }
		    RtlCopyMemory(fileObjectFileNameBuffer, fileObject->FileName.Buffer, fileObject->FileName.MaximumLength);
		    fileObject->FileName.Buffer = fileObjectFileNameBuffer;
	    }
    }
    
    // IRP_MJ_DIRECTORY_CONTROL IRP_MN_QUERY_DIRECTORYの場合
    if(src->MajorFunction == IRP_MJ_DIRECTORY_CONTROL && src->MinorFunction == IRP_MN_QUERY_DIRECTORY && src->Parameters.Others.Argument2 != NULL){
    	// IrpSp->Parameters.QueryDirectory.FileName
	    queryDirectoryFileName = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
	    if (!queryDirectoryFileName) {
	        goto failed;
	    }
	    RtlCopyMemory(queryDirectoryFileName, src->Parameters.Others.Argument2, sizeof(UNICODE_STRING));
	    dst->Parameters.Others.Argument2 = queryDirectoryFileName;
	    
    	// IrpSp->Parameters.QueryDirectory.FileName->Buffer
	    queryDirectoryFileNameBuffer = (PWSTR)ExAllocatePool(NonPagedPool, queryDirectoryFileName->MaximumLength);
	    if (!queryDirectoryFileNameBuffer) {
	        goto failed;
	    }
	    RtlCopyMemory(queryDirectoryFileNameBuffer, queryDirectoryFileName->Buffer, queryDirectoryFileName->MaximumLength);
	    queryDirectoryFileName->Buffer = queryDirectoryFileNameBuffer;
    }
	return dst;
	
failed:
	if (dst) ExFreePool(dst);
	if (fileObject) ExFreePool(fileObject);
	if (fileObjectFileNameBuffer) ExFreePool(fileObjectFileNameBuffer);
	if (queryDirectoryFileName) ExFreePool(queryDirectoryFileName);
	if (queryDirectoryFileNameBuffer) ExFreePool(queryDirectoryFileNameBuffer);
    return NULL; // メモリ確保失敗
}
// 書き戻し
VOID ReleaseNonPagedPoolIrpStack(PIO_STACK_LOCATION src, PIO_STACK_LOCATION back) {
	// 末端から書き戻し＆解放してもとのアドレスに戻していく

    // IRP_MJ_DIRECTORY_CONTROL IRP_MN_QUERY_DIRECTORYの場合
    if(src->MajorFunction == IRP_MJ_DIRECTORY_CONTROL && src->MinorFunction == IRP_MN_QUERY_DIRECTORY && back->Parameters.Others.Argument2 != NULL){
    	// IrpSp->Parameters.QueryDirectory.FileName->Buffer
	    ExFreePool(((PUNICODE_STRING)src->Parameters.Others.Argument2)->Buffer); // 書き戻し不要　解放するだけ
	    ((PUNICODE_STRING)src->Parameters.Others.Argument2)->Buffer = ((PUNICODE_STRING)back->Parameters.Others.Argument2)->Buffer;
    	// IrpSp->Parameters.QueryDirectory.FileName
	    ExFreePool(src->Parameters.Others.Argument2); // 書き戻し不要　解放するだけ
	    src->Parameters.Others.Argument2 = back->Parameters.Others.Argument2;
    }
    if(src->FileObject){
	    // IrpSp->FileObject->FileName.Buffer
	    if (src->FileObject->FileName.MaximumLength > 0){
			ExFreePool(src->FileObject->FileName.Buffer); // 書き戻し不要　解放するだけ
			src->FileObject->FileName.Buffer = back->FileObject->FileName.Buffer;
	    }
	    // IrpSp->FileObject
	    RtlCopyMemory(back->FileObject, src->FileObject, sizeof(FILE_OBJECT)); // 書き戻し
		ExFreePool(src->FileObject);
		src->FileObject = back->FileObject;
    }
    // PIO_STACK_LOCATION IrpSp
    RtlCopyMemory(back, src, sizeof(IO_STACK_LOCATION)); // 書き戻し
	ExFreePool(src);
}

// ページングされないIrpStackを取得する。NULLの場合失敗
// src: ロックしたいrpStack
// return -> IRPロック情報
IRPSTACKLOCK_INFO LockIrpStack(PIO_STACK_LOCATION src) {
	IRPSTACKLOCK_INFO info = {0};
	
	info.mdlIrpStack = IoAllocateMdl(src, sizeof(IO_STACK_LOCATION), FALSE, FALSE, NULL);
	if (!info.mdlIrpStack) goto failed;
	MmProbeAndLockPages(info.mdlIrpStack, KernelMode, IoModifyAccess);

    if(src->FileObject){
		info.mdlFileObject = IoAllocateMdl(src->FileObject, sizeof(FILE_OBJECT), FALSE, FALSE, NULL);
		if (!info.mdlFileObject) goto failed;
		MmProbeAndLockPages(info.mdlFileObject, KernelMode, IoModifyAccess);
	    
	    // IrpSp->FileObject->FileName.Buffer
	    if (src->FileObject->FileName.MaximumLength > 0){
			info.mdlFileObjectFileNameBuffer = IoAllocateMdl(src->FileObject->FileName.Buffer, src->FileObject->FileName.MaximumLength, FALSE, FALSE, NULL);
			if (!info.mdlFileObjectFileNameBuffer) goto failed;
			MmProbeAndLockPages(info.mdlFileObjectFileNameBuffer, KernelMode, IoModifyAccess);
	    }
    }
    // IRP_MJ_DIRECTORY_CONTROL IRP_MN_QUERY_DIRECTORYの場合
    if(src->MajorFunction == IRP_MJ_DIRECTORY_CONTROL && src->MinorFunction == IRP_MN_QUERY_DIRECTORY && src->Parameters.Others.Argument2 != NULL){
    	PUNICODE_STRING queryDirectoryFileName = NULL;
    	
    	// IrpSp->Parameters.QueryDirectory.FileName
		info.mdlQueryDirectoryFileName = IoAllocateMdl(src->Parameters.Others.Argument2, sizeof(UNICODE_STRING), FALSE, FALSE, NULL);
		if (!info.mdlQueryDirectoryFileName) goto failed;
		MmProbeAndLockPages(info.mdlQueryDirectoryFileName, KernelMode, IoModifyAccess);
    	
    	// IrpSp->Parameters.QueryDirectory.FileName->Buffer
    	queryDirectoryFileName = (PUNICODE_STRING)(src->Parameters.Others.Argument2);
	    if (queryDirectoryFileName->MaximumLength > 0){
			info.mdlQueryDirectoryFileNameBuffer = IoAllocateMdl(queryDirectoryFileName->Buffer, queryDirectoryFileName->MaximumLength, FALSE, FALSE, NULL);
			if (!info.mdlQueryDirectoryFileNameBuffer) goto failed;
			MmProbeAndLockPages(info.mdlQueryDirectoryFileNameBuffer, KernelMode, IoModifyAccess);
	    }
    }
    
    info.isValid = 1;

	return info; 
failed:

	UnlockIrpStack(&info);
	return info; 
}

// ページングされないIrpStackを破棄する
// src: ロックされたIrpStack
// lpInfo: IRPロック情報へのポインタ
VOID UnlockIrpStack(PIRPSTACKLOCK_INFO lpInfo) {
	if(lpInfo->mdlQueryDirectoryFileNameBuffer){
		MmUnlockPages(lpInfo->mdlQueryDirectoryFileNameBuffer);
		IoFreeMdl(lpInfo->mdlQueryDirectoryFileNameBuffer);
		lpInfo->mdlQueryDirectoryFileNameBuffer = NULL;
	}
	if(lpInfo->mdlQueryDirectoryFileName){
		MmUnlockPages(lpInfo->mdlQueryDirectoryFileName);
		IoFreeMdl(lpInfo->mdlQueryDirectoryFileName);
		lpInfo->mdlQueryDirectoryFileName = NULL;
	}
	if(lpInfo->mdlFileObjectFileNameBuffer){
		MmUnlockPages(lpInfo->mdlFileObjectFileNameBuffer);
		IoFreeMdl(lpInfo->mdlFileObjectFileNameBuffer);
		lpInfo->mdlFileObjectFileNameBuffer = NULL;
	}
	if(lpInfo->mdlFileObject){
		MmUnlockPages(lpInfo->mdlFileObject);
		IoFreeMdl(lpInfo->mdlFileObject);
		lpInfo->mdlFileObject = NULL;
	}
	if(lpInfo->mdlIrpStack){
		MmUnlockPages(lpInfo->mdlIrpStack);
		IoFreeMdl(lpInfo->mdlIrpStack);
		lpInfo->mdlIrpStack = NULL;
	}
    
    lpInfo->isValid = 0;
}
