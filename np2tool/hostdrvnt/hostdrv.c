#include <ntddk.h>
#include "miniifs.h"
#include "minisop.h"
#include "irplock.h"

// NonPagedPoolへのコピーではなくページロックを使う
#define USE_FAST_IRPSTACKLOCK

#define HOSTDRVNT_IO_ADDR	0x7EC
#define HOSTDRVNT_IO_CMD	0x7EE

// このデバイスドライバのデバイス名
#define DEVICE_NAME     L"\\Device\\HOSTDRV"
#define DOS_DEVICE_NAME L"\\DosDevices\\HOSTDRV"
#define DOS_DRIVE_NAME  L"\\DosDevices\\Z:"

#define PENDING_IRP_MAX	256

#define HOSTDRVNTOPTIONS_NONE				0x0
#define HOSTDRVNTOPTIONS_REMOVABLEDEVICE	0x1
#define HOSTDRVNTOPTIONS_USEREALCAPACITY	0x2
#define HOSTDRVNTOPTIONS_USECHECKNOTIFY		0x4
#define HOSTDRVNTOPTIONS_AUTOMOUNTDRIVE		0x8
#define HOSTDRVNTOPTIONS_DISKDEVICE			0x10

#define HOSTDRVNT_VERSION		4

// エミュレータとの通信用
typedef struct tagHOSTDRV_INFO {
    PIO_STACK_LOCATION stack; // IoGetCurrentIrpStackLocation(Irp)で取得されるデータへのアドレス
    PIO_STATUS_BLOCK status; // Irp->IoStatusへのアドレス
    PVOID systemBuffer; // ゲストOS→エミュレータへのバッファ
    ULONG deviceFlags; // irpSp->DeviceObject->Flagsの値
    PVOID outBuffer; // エミュレータ→ゲストOSへのバッファ
    PVOID sectionObjectPointer; // irpSp->FileObject->SectionObjectPointerへのアドレス
    ULONG version; // エミュレータ通信バージョン情報
    ULONG pendingListCount; // 待機用IRPのリストの要素数
    PIRP  *pendingIrpList; // 待機用IRPのリストへのアドレス
    ULONG *pendingAliveList; // 待機用生存フラグ（猫側ファイルオブジェクトインデックス）のリストへのアドレス
    union{
    	LONG pendingIndex; // STATUS_PENDINGのとき、どのインデックスに待機用IRPを追加するかを表す（猫側がセット）
    	LONG pendingCompleteCount; // STATUS_PENDING以外の時、待機完了したものの数を表す（猫側がセット）
    } pending;
    ULONG hostdrvNTOptions; // HOSTDRV for NTオプション
} HOSTDRV_INFO, *PHOSTDRV_INFO;
typedef struct tagHOSTDRV_NOTIFYINFO {
    ULONG version; // エミュレータ通信バージョン情報
    ULONG pendingListCount; // 待機用IRPのリストの要素数
    PIRP  *pendingIrpList; // 待機用IRPのリストへのアドレス
    ULONG *pendingAliveList; // 待機用生存フラグ（猫側ファイルオブジェクトインデックス）のリストへのアドレス
    union{
    	LONG pendingCompleteCount; // 待機完了したものの数を表す（猫側がセット）
    } pending;
} HOSTDRV_NOTIFYINFO, *PHOSTDRV_NOTIFYINFO;

// irpSp->FileObject->FsContextへ格納する情報
// 参考情報：irpSp->FileObject->FsContextへ適当なIDを入れるのはNG。色々動かなくなる。
// 必ずExAllocatePoolWithTag(NonPagedPool, ～で割り当てたメモリである必要がある。
// [Undocumented] 構造体サイズは少なくとも64byteないと駄目？
// ないとOSが決め打ちの範囲外参照してIRQL_NOT_LESS_OR_EQUALが頻発。どこまであれば安全かは不明。
// FSRTL_COMMON_FCB_HEADERを構造体の最初に含めなければならないという条件が必須のようにも思えます。
typedef struct tagHOSTDRV_FSCONTEXT {
	FSRTL_COMMON_FCB_HEADER header; // OSが使うので触ってはいけない。32bit環境では40byte
    ULONG fileIndex; // エミュレータ本体が管理するファイルID
    ULONG reserved[5]; // 予約
} HOSTDRV_FSCONTEXT, *PHOSTDRV_FSCONTEXT;

// グローバルに管理する変数群
static FAST_MUTEX g_Mutex; // I/Oの排他ロック用
static PIRP g_pendingIrpList[PENDING_IRP_MAX] = {0}; // I/O待機用IRPのリスト
static ULONG g_pendingAliveList[PENDING_IRP_MAX] = {0}; // I/O待機用生存フラグのリスト
static ULONG g_hostdrvNTOptions = HOSTDRVNTOPTIONS_NONE; // HOSTDRV for NTオプション
static ULONG g_checkNotifyInterval = 10; // ホストのファイルシステム変更通知を何秒間隔でチェックするか
static KTIMER g_checkNotifyTimer = {0}; // ホストのファイルシステム変更通知を監視するタイマー
static KDPC g_checkNotifyTimerDpc = {0}; // ホストのファイルシステム変更通知を監視するタイマーDPC
static WORK_QUEUE_ITEM g_RescheduleTimerWorkItem = {0}; // ホストのファイルシステム変更通知を監視するタイマー再起動用
static int g_checkNotifyTimerEnabled = 0; // ホストのファイルシステム変更通知を監視するタイマーが開始状態
static ULONG g_pendingCounter = 0; // ファイルシステム監視でSTATUS_PENDING状態のものの数
static WCHAR g_autoMountDriveLetter = 0; // 自動マウント時のドライブレター文字。0の場合はZから順に使える場所を探して割り当て。

// 関数定義
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS HostdrvDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID HostdrvUnload(IN PDRIVER_OBJECT DriverObject);
VOID HostdrvCancelRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp);
VOID HostdrvTimerDpcRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
VOID HostdrvRescheduleTimer(IN PVOID Context);

// レジストリのDWORD値を読む
ULONG HostdrvReadDWORDReg(HANDLE hKey, WCHAR *valueName) {
	NTSTATUS status;
	UNICODE_STRING valueNameUnicode;
	ULONG resultLength;
	PKEY_VALUE_PARTIAL_INFORMATION pInfo;
	
	RtlInitUnicodeString(&valueNameUnicode, valueName);
	status = ZwQueryValueKey(hKey, &valueNameUnicode, KeyValuePartialInformation, NULL, 0, &resultLength);
	if (status != STATUS_BUFFER_TOO_SMALL && status != STATUS_BUFFER_OVERFLOW) {
	    return 0;
	}
	pInfo = ExAllocatePoolWithTag(PagedPool, resultLength, 'prmT');
	if (!pInfo) {
	    return 0;
	}
	status = ZwQueryValueKey(hKey, &valueNameUnicode, KeyValuePartialInformation, pInfo, resultLength, &resultLength);
	if (NT_SUCCESS(status) && pInfo->Type == REG_DWORD) {
    	ULONG retValue = *(ULONG *)pInfo->Data;
		ExFreePool(pInfo);
    	return retValue;
	}
	ExFreePool(pInfo);
	
	return 0;
}
// レジストリのドライブ文字をあらわすREG_SZを読む
WCHAR HostdrvReadDriveLetterReg(HANDLE hKey, WCHAR *valueName) {
	NTSTATUS status;
	UNICODE_STRING valueNameUnicode;
	ULONG resultLength;
	PKEY_VALUE_PARTIAL_INFORMATION pInfo;
	
	RtlInitUnicodeString(&valueNameUnicode, valueName);
	status = ZwQueryValueKey(hKey, &valueNameUnicode, KeyValuePartialInformation, NULL, 0, &resultLength);
	if (status != STATUS_BUFFER_TOO_SMALL && status != STATUS_BUFFER_OVERFLOW) {
	    return 0;
	}
	pInfo = ExAllocatePoolWithTag(PagedPool, resultLength, 'prmT');
	if (!pInfo) {
	    return 0;
	}
	status = ZwQueryValueKey(hKey, &valueNameUnicode, KeyValuePartialInformation, pInfo, resultLength, &resultLength);
	if (NT_SUCCESS(status) && pInfo->Type == REG_SZ && pInfo->DataLength == 2 * sizeof(WCHAR)) { // NULL文字含むバイト数
    	WCHAR retValue = ((WCHAR *)(pInfo->Data))[0];
		ExFreePool(pInfo);
    	return retValue;
	}
	ExFreePool(pInfo);
	
	return 0;
}

// オプションのレジストリキーをチェック
VOID HostdrvCheckOptions(IN PUNICODE_STRING RegistryPath) {
	OBJECT_ATTRIBUTES attributes;
	HANDLE hKey;
	NTSTATUS status;

	InitializeObjectAttributes(&attributes, RegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
	
	status = ZwOpenKey(&hKey, KEY_READ, &attributes);
	if (!NT_SUCCESS(status)) {
	    return;
	}
	
	if (HostdrvReadDWORDReg(hKey, L"IsDiskDevice")){
		g_hostdrvNTOptions |= HOSTDRVNTOPTIONS_DISKDEVICE;
	}
	if (HostdrvReadDWORDReg(hKey, L"IsRemovableDevice")){
		g_hostdrvNTOptions |= HOSTDRVNTOPTIONS_REMOVABLEDEVICE;
	}
	if (HostdrvReadDWORDReg(hKey, L"UseRealCapacity")){
		g_hostdrvNTOptions |= HOSTDRVNTOPTIONS_USEREALCAPACITY;
	}
	if (HostdrvReadDWORDReg(hKey, L"UseCheckNotify")){
		g_hostdrvNTOptions |= HOSTDRVNTOPTIONS_USECHECKNOTIFY;
	}
	g_checkNotifyInterval = HostdrvReadDWORDReg(hKey, L"CheckNotifyInterval");
	if (g_checkNotifyInterval <= 0){
		g_checkNotifyInterval = 5; // デフォルト（5秒）にする
	}
	if (g_checkNotifyInterval > 60){
		g_checkNotifyInterval = 60; // 最大は60秒にする
	}
	if (HostdrvReadDWORDReg(hKey, L"AutoMount")){
		g_hostdrvNTOptions |= HOSTDRVNTOPTIONS_AUTOMOUNTDRIVE;
	}
	g_autoMountDriveLetter = HostdrvReadDriveLetterReg(hKey, L"AutoMountDriveLetter");
	if('a' <= g_autoMountDriveLetter && g_autoMountDriveLetter <= 'z'){
		// 大文字とする
		g_autoMountDriveLetter = g_autoMountDriveLetter - 'a' + 'A';
	}
	if(!('A' <= g_autoMountDriveLetter && g_autoMountDriveLetter <= 'Z')){
		// 無効なので自動割り当てとする
		g_autoMountDriveLetter = 0;
	}
	
	ZwClose(hKey);
}

VOID HostdrvStartTimer()
{
	LARGE_INTEGER dueTime;
	
    if (g_checkNotifyTimerEnabled) {
        return;
    }
    
	g_checkNotifyTimerEnabled = 1;
	
	dueTime.QuadPart = (LONGLONG)(-(LONG)g_checkNotifyInterval * 1000 * 10000);
	KeSetTimer(&g_checkNotifyTimer, dueTime, &g_checkNotifyTimerDpc);
}

VOID HostdrvStopTimer()
{
    if (!g_checkNotifyTimerEnabled) {
        return;
    }
    
	KeCancelTimer(&g_checkNotifyTimer);
	
	g_checkNotifyTimerEnabled = 0;
}

NTSTATUS ReserveIoPortRange(PDRIVER_OBJECT DriverObject)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResourceDescriptor;
    PCM_PARTIAL_RESOURCE_LIST pPartialResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR pFullResourceDescriptor;
    PCM_RESOURCE_LIST pResourceList;
    ULONG listSize;
    UNICODE_STRING className;

    BOOLEAN conflictDetected = FALSE;
    NTSTATUS status;
    
    listSize = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    pResourceList = ExAllocatePoolWithTag(PagedPool, listSize, 'resl');
    if(!pResourceList){
    	return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pResourceList, listSize);
    pResourceList->Count = 1;

	pFullResourceDescriptor = &(pResourceList->List[0]);
    pFullResourceDescriptor->InterfaceType = Internal;
    pFullResourceDescriptor->BusNumber = 0;
    
    pPartialResourceList = &(pFullResourceDescriptor->PartialResourceList);
    pPartialResourceList->Version = 1;
    pPartialResourceList->Revision = 1;
    pPartialResourceList->Count = 2;
    
    pResourceDescriptor = &(pPartialResourceList->PartialDescriptors[0]);
    pResourceDescriptor->Type = CmResourceTypePort;
    pResourceDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
    pResourceDescriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
    pResourceDescriptor->u.Port.Start.QuadPart = HOSTDRVNT_IO_ADDR;
    pResourceDescriptor->u.Port.Length = 1;
    
    pResourceDescriptor = &(pPartialResourceList->PartialDescriptors[1]);
    pResourceDescriptor->Type = CmResourceTypePort;
    pResourceDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
    pResourceDescriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
    pResourceDescriptor->u.Port.Start.QuadPart = HOSTDRVNT_IO_CMD;
    pResourceDescriptor->u.Port.Length = 1;
    
    // リソース使用の報告
	RtlInitUnicodeString(&className, L"LegacyDriver");
    status = IoReportResourceUsage(
        &className,           // DriverClassName
        DriverObject,         // OwningDriverObject
        pResourceList,        // ResourceList
        listSize,             // ResourceListSize
        NULL,                 // PhysicalDeviceObject
        NULL,                 // ConflictList
        0,                    // ConflictCount
        FALSE,                // ArbiterRequest
        &conflictDetected     // ConflictDetected
    );
    
	ExFreePool(pResourceList);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (conflictDetected) {
        return STATUS_CONFLICTING_ADDRESSES;
    }

    return STATUS_SUCCESS;
}

// デバイスドライバのエントリポイント
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath) {
    UNICODE_STRING deviceNameUnicodeString, dosDeviceNameUnicodeString;
    PDEVICE_OBJECT deviceObject = NULL;
    DEVICE_TYPE deviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
    ULONG deviceCharacteristics = FILE_DEVICE_IS_MOUNTED;
    NTSTATUS status;
    int i;
    
    // I/Oポートが使えるかを確認
    status = ReserveIoPortRange(DriverObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // hostdrv for NT対応かを簡易チェック
    if(READ_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR) != 98 || READ_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD) != 21){
        return STATUS_NO_SUCH_DEVICE;
	}
	
    // hostdrv for NTをリセット
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)0);
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)0);
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)0);
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)0);
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'H');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'D');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'R');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'9');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'8');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'0');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'1');

    // 排他ロック初期化　初期化のみで破棄処理はいらない
    ExInitializeFastMutex(&g_Mutex);
    
    // オプションチェック
    HostdrvCheckOptions(RegistryPath);
    
    // デバイスタイプなどの設定
    if(g_hostdrvNTOptions & HOSTDRVNTOPTIONS_REMOVABLEDEVICE){
    	// リムーバブルデバイスの振りをするモード
    	deviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
    	deviceCharacteristics |= FILE_REMOVABLE_MEDIA;
    }else if(g_hostdrvNTOptions & HOSTDRVNTOPTIONS_DISKDEVICE){
    	// ローカルディスクの振りをするモード
    	deviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
    }else{
    	// ネットワークファイルシステムの振りをするモード
    	deviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
    	deviceCharacteristics |= FILE_REMOTE_DEVICE;
    }
    
    // デバイスを作成　ローカルディスクの振りをするならFILE_DEVICE_DISK_FILE_SYSTEM
    RtlInitUnicodeString(&deviceNameUnicodeString, DEVICE_NAME);
    status = IoCreateDevice(DriverObject, 0, &deviceNameUnicodeString,
                            deviceType, deviceCharacteristics, FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // デバイスのDOS名を登録　\\.\HOSTDRVのようにアクセスできる
    RtlInitUnicodeString(&dosDeviceNameUnicodeString, DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&dosDeviceNameUnicodeString, &deviceNameUnicodeString);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        return status;
    }
    
    // 自動ドライブ文字割り当ての場合、割り当て
    if(g_hostdrvNTOptions & HOSTDRVNTOPTIONS_AUTOMOUNTDRIVE){
		UNICODE_STRING dosDriveNameUnicodeString;
    	WCHAR dosDriveName[] = DOS_DRIVE_NAME;
		RtlInitUnicodeString(&dosDriveNameUnicodeString, dosDriveName);
    	if(g_autoMountDriveLetter== 0){
    		// 自動で使える文字を探す
    		for(g_autoMountDriveLetter='Z';g_autoMountDriveLetter>='A';g_autoMountDriveLetter--){
    			dosDriveNameUnicodeString.Buffer[12] = g_autoMountDriveLetter;
	    		status = IoCreateSymbolicLink(&dosDriveNameUnicodeString, &deviceNameUnicodeString);
    			if (NT_SUCCESS(status)) {
    				break;
    			}
    		}
    		if(g_autoMountDriveLetter < 'A'){
    			// 空きがないので割り当てできなかった
    			g_autoMountDriveLetter = 0;
    		}
    	}else{
    		// 固定文字指定　使えなくてもエラーにはしない
    		dosDriveNameUnicodeString.Buffer[12] = g_autoMountDriveLetter;
	    	status = IoCreateSymbolicLink(&dosDriveNameUnicodeString, &deviceNameUnicodeString);
		    if (!NT_SUCCESS(status)) {
		    	// 割り当て失敗
		        g_autoMountDriveLetter = 0;
		    }
    	}
    }

    // デバイスのIRP処理関数を登録　要素番号が各IRP_MJ_～の番号に対応。
    // 全部エミュレータ本体に投げるので全部に同じ関数を割り当て
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = HostdrvDispatch;
    }

    // ドライバの終了処理を登録
    DriverObject->DriverUnload = HostdrvUnload;
    
    // キャッシュ未実装でもメモリマップトファイルが使えるようにするWORKAROUNDキャッシュを初期化
    MiniSOP_InitializeCache(DriverObject);
    
    // その他追加フラグを設定
    deviceObject->Flags |= DO_BUFFERED_IO; // データ受け渡しでSystemBufferを基本とする？指定しても他方式で来るときは来る気がする。
    
    if(g_hostdrvNTOptions & HOSTDRVNTOPTIONS_USECHECKNOTIFY){
    	KeInitializeTimer(&g_checkNotifyTimer);
		KeInitializeDpc(&g_checkNotifyTimerDpc, HostdrvTimerDpcRoutine, NULL);
        ExInitializeWorkItem(&g_RescheduleTimerWorkItem, HostdrvRescheduleTimer, NULL);
	}

    KdPrint(("Hostdrv: Loaded successfully\n"));
    return STATUS_SUCCESS;
}

VOID HostdrvUnload(IN PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING dosDeviceNameUnicodeString;
    int i;
    
    // 待機中のIRPを全部キャンセルする
    for (i = 0; i < PENDING_IRP_MAX; i++) {
        if (g_pendingIrpList[i] != NULL) {
    		KIRQL oldIrql;
        	PIRP Irp = g_pendingIrpList[i];
		    g_pendingIrpList[i] = NULL;
		    g_pendingAliveList[i] = 0;
        	IoAcquireCancelSpinLock(&oldIrql);
        	if(Irp->CancelRoutine){
				Irp->CancelRoutine = NULL;
    			IoReleaseCancelSpinLock(Irp->CancelIrql);
			    Irp->IoStatus.Status = STATUS_CANCELLED;
			    Irp->IoStatus.Information = 0;
	        	IoCompleteRequest(Irp, IO_NO_INCREMENT); // キャンセルする
        	}else{
    			IoReleaseCancelSpinLock(Irp->CancelIrql); // 何故かキャンセル済み　通常はないはず
        	}
        }
    }
	g_pendingCounter = 0;
    
    // 排他領域開始
    ExAcquireFastMutex(&g_Mutex);
    
    HostdrvStopTimer();
    
    // SOPリスト解放
    MiniSOP_ReleaseSOPList();
    
    // 排他領域終了
    ExReleaseFastMutex(&g_Mutex);
    
    // 自動ドライブ文字割り当ての場合、解除
    if(g_hostdrvNTOptions & HOSTDRVNTOPTIONS_AUTOMOUNTDRIVE){
    	if(g_autoMountDriveLetter != 0){
			UNICODE_STRING dosDriveNameUnicodeString;
	    	WCHAR dosDriveName[] = DOS_DRIVE_NAME;
			RtlInitUnicodeString(&dosDriveNameUnicodeString, dosDriveName);
			dosDriveNameUnicodeString.Buffer[12] = g_autoMountDriveLetter;
    		IoDeleteSymbolicLink(&dosDriveNameUnicodeString);
		}
    }
    
    // DOS名を登録解除
    RtlInitUnicodeString(&dosDeviceNameUnicodeString, DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&dosDeviceNameUnicodeString);
    
    // デバイスを削除
    IoDeleteDevice(DriverObject->DeviceObject);
    
    KdPrint(("Hostdrv: Unloaded\n"));
}

VOID HostdrvCancelRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    int i;
    
    // 待機キャンセル要求登録解除
    //IoSetCancelRoutine(Irp, NULL); // 旧OSで動かないが本当はこちらが推奨
    Irp->CancelRoutine = NULL;
    
    // キャンセルのロックを解除
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    
    // 排他領域開始
    ExAcquireFastMutex(&g_Mutex);
    
	// 指定されたIRPを登録解除
    for (i = 0; i < PENDING_IRP_MAX; i++) {
    	if(g_pendingIrpList[i] == Irp){
    		g_pendingIrpList[i] = NULL;
    		g_pendingAliveList[i] = 0;
			if(g_pendingCounter > 0) g_pendingCounter--;
		    break;
    	}
    }
    
	if(g_pendingCounter == 0){
		// 待機中がなければ動かす必要なし
		HostdrvStopTimer();
	}
    
    // 排他領域終了
    ExReleaseFastMutex(&g_Mutex);
    
    // キャンセル実行
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
}

NTSTATUS HostdrvDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    HOSTDRV_INFO hostdrvInfo;
    HOSTDRV_INFO *lpHostdrvInfo;
    ULONG hostdrvInfoAddr;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
#ifdef USE_FAST_IRPSTACKLOCK
	IRPSTACKLOCK_INFO irpLockInfo;
#else
    PIO_STACK_LOCATION irpSpNPP = NULL;
    PIO_STACK_LOCATION irpSpNPPBefore = NULL;
#endif
    BOOLEAN pending = FALSE;
	ULONG completeIrpCount = 0; // I/O待機で今回完了したものの数
	PIRP *completeIrpList = NULL; // I/O待機で今回完了したIRPのリスト
	ULONG sopIndex = -1;
	NTSTATUS status;
	int i;

    // IRP_MJ_CREATEの時、必要なメモリを仮割り当て
    if(irpSp->MajorFunction == IRP_MJ_CREATE) {
        // FileObjectがNULLなのは異常なので弾く
        if(irpSp->FileObject == NULL){
            Irp->IoStatus.Status = status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }
        // FsContextとSectionObjectPointerにExAllocatePoolWithTag(NonPagedPool,～でメモリ割り当て
        // NULLのままだったり違う方法で割り当てると正常に動かないので注意
        irpSp->FileObject->FsContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(HOSTDRV_FSCONTEXT), "HSFC");
        if(irpSp->FileObject->FsContext == NULL){
		    Irp->IoStatus.Status = status = STATUS_INSUFFICIENT_RESOURCES;
		    Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }
        RtlZeroMemory(irpSp->FileObject->FsContext, sizeof(HOSTDRV_FSCONTEXT));
        
    	// SOP取得
	    ExAcquireFastMutex(&g_Mutex);
    	sopIndex = MiniSOP_GetSOPIndex(irpSp->FileObject->FileName);
	    ExReleaseFastMutex(&g_Mutex);
	    
    	if(sopIndex == -1){
        	ExFreePool(irpSp->FileObject->FsContext);
		    Irp->IoStatus.Status = status = STATUS_INSUFFICIENT_RESOURCES;
		    Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
		}
		irpSp->FileObject->SectionObjectPointer = MiniSOP_GetSOP(sopIndex);
    }
    
    // デフォルトのステータス設定
    Irp->IoStatus.Status = status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;
    
    // エミュレータ側に渡すデータ設定
#ifdef USE_FAST_IRPSTACKLOCK
	// ページアウトしないようにロック
    irpLockInfo = LockIrpStack(irpSp);
    if(!irpLockInfo.isValid){
        Irp->IoStatus.Status = status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
#else
	// ページアウトしない領域へコピー
    irpSpNPP = CreateNonPagedPoolIrpStack(irpSp); // NonPagedコピー作成
    if(irpSpNPP == NULL){
        Irp->IoStatus.Status = status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
	}
	irpSpNPPBefore = irpSp;
	irpSp = irpSpNPP; // ポインタ書き換え
#endif
    lpHostdrvInfo = &hostdrvInfo;
    lpHostdrvInfo->stack = irpSp;
    lpHostdrvInfo->status = &(Irp->IoStatus);
    lpHostdrvInfo->systemBuffer = Irp->AssociatedIrp.SystemBuffer;
    lpHostdrvInfo->deviceFlags = irpSp->DeviceObject->Flags;
    if (Irp->MdlAddress != NULL) {
        // MdlAddressを使った直接的な転送
        lpHostdrvInfo->outBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress); // 古いOS対応用
        //lpHostdrvInfo->outBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority); // 最近のOSならこれも可能（より安全？）
        if(lpHostdrvInfo->systemBuffer == NULL){
        	lpHostdrvInfo->systemBuffer = lpHostdrvInfo->outBuffer;
        }
    } else if (Irp->AssociatedIrp.SystemBuffer != NULL) {
        // システムバッファ経由での間接的な転送
        lpHostdrvInfo->outBuffer = Irp->AssociatedIrp.SystemBuffer;
    } else {
        // ユーザー指定バッファへの転送
        lpHostdrvInfo->outBuffer = Irp->UserBuffer;
    }
    if (irpSp->FileObject) {
        lpHostdrvInfo->sectionObjectPointer = irpSp->FileObject->SectionObjectPointer;
    } else {
        lpHostdrvInfo->sectionObjectPointer = NULL;
    }
    lpHostdrvInfo->version = HOSTDRVNT_VERSION;
    lpHostdrvInfo->pendingListCount = PENDING_IRP_MAX;
    lpHostdrvInfo->pendingIrpList = g_pendingIrpList;
    lpHostdrvInfo->pendingAliveList = g_pendingAliveList;
    lpHostdrvInfo->pending.pendingIndex = -1;
    lpHostdrvInfo->hostdrvNTOptions = g_hostdrvNTOptions;
    
    if(irpSp->MajorFunction == IRP_MJ_SET_INFORMATION) {
	    MiniSOP_HandlePreMjSetInformationCache(irpSp);
 	}
 	
    // 排他領域開始
    ExAcquireFastMutex(&g_Mutex);
    
    // 構造体アドレスを書き込んでエミュレータで処理させる（ハイパーバイザーコール）
    // エミュレータ側でステータスやバッファなどの値がセットされる
    hostdrvInfoAddr = (ULONG)lpHostdrvInfo;
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)(hostdrvInfoAddr));
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)(hostdrvInfoAddr >> 8));
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)(hostdrvInfoAddr >> 16));
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)(hostdrvInfoAddr >> 24));
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'H');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'D');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'R');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'9');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'8');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'0');
    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'1');
    if(Irp->IoStatus.Status == STATUS_PENDING){
    	// 待機希望
    	if(lpHostdrvInfo->pending.pendingIndex < 0 || PENDING_IRP_MAX <= lpHostdrvInfo->pending.pendingIndex){
    		// 登録できる場所がない
		    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		    Irp->IoStatus.Information = 0;
    	}else{
    		// 登録OK　ここではg_pendingIrpListに代入しない（入れると他スレッドに完了されてしまう恐れがある）
    		pending = TRUE;
    	}
    }else if(lpHostdrvInfo->pending.pendingCompleteCount > 0){
    	// 待機解除が存在する
    	completeIrpCount = lpHostdrvInfo->pending.pendingCompleteCount;
        completeIrpList = ExAllocatePoolWithTag(NonPagedPool, sizeof(PIRP) * completeIrpCount, "HSIP");
        if(completeIrpList){
        	int ci = 0;
		    for (i = 0; i < PENDING_IRP_MAX; i++) {
		        if (g_pendingAliveList[i] == 0 && g_pendingIrpList[i] != NULL) {
		        	completeIrpList[ci] = g_pendingIrpList[i];
				    g_pendingIrpList[i] = NULL;
				    if(g_pendingCounter > 0) g_pendingCounter--;
				    ci++;
				    if (ci==completeIrpCount) break;
		        }
		    }
        }
		if(g_pendingCounter == 0){
			// 待機中がなければ動かす必要なし
			HostdrvStopTimer();
		}
    }
    
    // 排他領域終了
    ExReleaseFastMutex(&g_Mutex);
    
    
#ifdef USE_FAST_IRPSTACKLOCK
	// ロック解除
    UnlockIrpStack(&irpLockInfo);
#else
    // NonPagedコピーを書き戻し
    ReleaseNonPagedPoolIrpStack(irpSpNPP, irpSpNPPBefore);
	irpSp = irpSpNPPBefore; // ポインタ書き戻し
    irpSpNPP = NULL;
#endif
    
    // ファイルオープン・クローズなどで割り当てたメモリの処理
    if(Irp->IoStatus.Status == STATUS_SUCCESS){
        // ファイルを閉じたので破棄
        if(irpSp->MajorFunction == IRP_MJ_CLOSE) {
            if(irpSp->FileObject->SectionObjectPointer){
            	PSECTION_OBJECT_POINTERS sop;
            		
			    // SOP解放
			    ExAcquireFastMutex(&g_Mutex);
    			sop = irpSp->FileObject->SectionObjectPointer;
                irpSp->FileObject->SectionObjectPointer = NULL;
                MiniSOP_ReleaseSOP(sop);
			    ExReleaseFastMutex(&g_Mutex);
            }
            if(irpSp->FileObject->FsContext){
                ExFreePool(irpSp->FileObject->FsContext);
                irpSp->FileObject->FsContext = NULL;
            }
        }else if(irpSp->MajorFunction == IRP_MJ_CREATE) {
			MiniSOP_HandleMjCreateCache(irpSp);
        }else if(irpSp->MajorFunction == IRP_MJ_CLEANUP) {
		    MiniSOP_HandleMjCleanupCache(irpSp);
        }
    }else{
        // 上手く行かなかったら破棄
        if(irpSp->MajorFunction == IRP_MJ_CREATE) {
            if(sopIndex != -1){
			    // SOP解放
			    ExAcquireFastMutex(&g_Mutex);
                irpSp->FileObject->SectionObjectPointer = NULL;
                MiniSOP_ReleaseSOPByIndex(sopIndex);
			    ExReleaseFastMutex(&g_Mutex);
            }
            if(irpSp->FileObject->FsContext){
                ExFreePool(irpSp->FileObject->FsContext);
                irpSp->FileObject->FsContext = NULL;
            }
        }
    }
    
    // 待機解除が存在する場合、それらを完了させる
    if(completeIrpList){
	    for (i = 0; i < completeIrpCount; i++) {
    		KIRQL oldIrql;
        	PIRP Irp = completeIrpList[i];
        	IoAcquireCancelSpinLock(&oldIrql);
        	if(Irp->CancelRoutine){
    			Irp->CancelRoutine = NULL;
    			IoReleaseCancelSpinLock(Irp->CancelIrql);
	        	IoCompleteRequest(Irp, IO_NO_INCREMENT); // 全部猫側でセットされているので完了を呼ぶだけでよい
        	}else{
    			IoReleaseCancelSpinLock(Irp->CancelIrql); // 何故かキャンセル済み　通常はないはず
        	}
	    }
    	ExFreePool(completeIrpList);
    	completeIrpList = NULL;
    }
    
    if(pending){
    	KIRQL oldIrql;
    	
    	// 待機中にセット
        IoMarkIrpPending(Irp);
        
        IoAcquireCancelSpinLock(&oldIrql);  // 保護開始
        
	    // 既にキャンセル済みならすぐに処理
		if (Irp->Cancel) {
    		IoReleaseCancelSpinLock(oldIrql);  // 保護解除
	        
		    // 排他領域開始
		    ExAcquireFastMutex(&g_Mutex);
    
    		// リストに入れる必要なし。解除する
    		g_pendingIrpList[lpHostdrvInfo->pending.pendingIndex] = NULL;
    		g_pendingAliveList[lpHostdrvInfo->pending.pendingIndex] = 0;

		    // 排他領域終了
		    ExReleaseFastMutex(&g_Mutex);
    
	        Irp->IoStatus.Status = status = STATUS_CANCELLED;
	        Irp->IoStatus.Information = 0;
	        IoCompleteRequest(Irp, IO_NO_INCREMENT);
	    }else{
		    // 待機キャンセル要求登録
		    //IoSetCancelRoutine(Irp, HostdrvCancelRoutine); // 旧OSで動かないが本当はこちらが推奨
    		Irp->CancelRoutine = HostdrvCancelRoutine;
		    
    		IoReleaseCancelSpinLock(oldIrql);  // 保護解除
    		
		    // 排他領域開始
		    ExAcquireFastMutex(&g_Mutex);
		    
    		// 実際に登録
    		g_pendingIrpList[lpHostdrvInfo->pending.pendingIndex] = Irp;
			g_pendingCounter++;
			// 必要なら監視タイマーを動かす
		    if(g_hostdrvNTOptions & HOSTDRVNTOPTIONS_USECHECKNOTIFY){
		    	HostdrvStartTimer();
			}
    
		    // 排他領域終了
		    ExReleaseFastMutex(&g_Mutex);
		    
    		status = Irp->IoStatus.Status;
	    }
    }else{
    	status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    return status;
}

VOID HostdrvTimerDpcRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
    if (!g_checkNotifyTimerEnabled) {
        return;
    }
    
	if(g_pendingCounter > 0 && g_checkNotifyTimerEnabled) {
    	// WorkItemへ投げる IRQLを落とす
    	ExQueueWorkItem(&g_RescheduleTimerWorkItem, DelayedWorkQueue);
	}else{
		// 停止
		g_checkNotifyTimerEnabled = 0;
	}
}
VOID HostdrvRescheduleTimer(IN PVOID Context)
{
	PHOSTDRV_NOTIFYINFO lpHostdrvInfo;
	ULONG hostdrvInfoAddr;
	ULONG completeIrpCount = 0; // I/O待機で今回完了したものの数
	PIRP *completeIrpList = NULL; // I/O待機で今回完了したIRPのリスト
	int i;
   
    lpHostdrvInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(HOSTDRV_NOTIFYINFO), "HSIM");
    
	// 排他領域開始
    ExAcquireFastMutex(&g_Mutex);
    
    if(lpHostdrvInfo){
    	// エミュレータ側に渡すデータ設定
	    lpHostdrvInfo->version = HOSTDRVNT_VERSION;
	    lpHostdrvInfo->pendingListCount = PENDING_IRP_MAX;
	    lpHostdrvInfo->pendingIrpList = g_pendingIrpList;
	    lpHostdrvInfo->pendingAliveList = g_pendingAliveList;
	    lpHostdrvInfo->pending.pendingCompleteCount = -1;
	    
	    // 構造体アドレスを書き込んでエミュレータで処理させる（ハイパーバイザーコール）
	    // エミュレータ側でステータスやバッファなどの値がセットされる
	    hostdrvInfoAddr = (ULONG)lpHostdrvInfo;
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)(hostdrvInfoAddr));
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)(hostdrvInfoAddr >> 8));
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)(hostdrvInfoAddr >> 16));
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_ADDR, (UCHAR)(hostdrvInfoAddr >> 24));
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'H');
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'D');
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'R');
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'9');
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'8');
	    WRITE_PORT_UCHAR((PUCHAR)HOSTDRVNT_IO_CMD, (UCHAR)'M');
	    if(lpHostdrvInfo->pending.pendingCompleteCount > 0){
	    	// 待機解除が存在する
	    	completeIrpCount = lpHostdrvInfo->pending.pendingCompleteCount;
	        completeIrpList = ExAllocatePoolWithTag(NonPagedPool, sizeof(PIRP) * completeIrpCount, "HSIP");
	        if(completeIrpList){
	        	int ci = 0;
			    for (i = 0; i < PENDING_IRP_MAX; i++) {
			        if (g_pendingAliveList[i] == 0 && g_pendingIrpList[i] != NULL) {
			        	completeIrpList[ci] = g_pendingIrpList[i];
					    g_pendingIrpList[i] = NULL;
					    if(g_pendingCounter > 0) g_pendingCounter--;
					    ci++;
					    if (ci==completeIrpCount) break;
			        }
			    }
	        }
	    }
    }

    // タイマー再設定
	if(g_pendingCounter > 0 && g_checkNotifyTimerEnabled) {
    	// 再設定
		LARGE_INTEGER dueTime = {0};
		dueTime.QuadPart = (LONGLONG)(-(LONG)g_checkNotifyInterval * 1000 * 10000);
		KeSetTimer(&g_checkNotifyTimer, dueTime, &g_checkNotifyTimerDpc);
	}else{
		// 停止
		g_checkNotifyTimerEnabled = 0;
	}
    
    // 排他領域終了
    ExReleaseFastMutex(&g_Mutex);
    
    if(lpHostdrvInfo){
    	ExFreePool(lpHostdrvInfo);
    }
    
    // 待機解除が存在する場合、それらを完了させる
    if(completeIrpList){
	    for (i = 0; i < completeIrpCount; i++) {
    		KIRQL oldIrql;
        	PIRP Irp = completeIrpList[i];
        	IoAcquireCancelSpinLock(&oldIrql);
        	if(Irp->CancelRoutine){
    			Irp->CancelRoutine = NULL;
    			IoReleaseCancelSpinLock(Irp->CancelIrql);
	        	IoCompleteRequest(Irp, IO_NO_INCREMENT); // 全部猫側でセットされているので完了を呼ぶだけでよい
        	}else{
    			IoReleaseCancelSpinLock(Irp->CancelIrql); // 何故かキャンセル済み　通常はないはず
        	}
	    }
    	ExFreePool(completeIrpList);
    	completeIrpList = NULL;
    }
}
