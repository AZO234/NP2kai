#include <ntddk.h>
#include <ntddmou.h>  // MOUSE_INPUT_DATA など

#define MOUSEPORT_DEVNAME        L"\\Device\\PointerClass0"
 
typedef VOID (*PSERVICE_CALLBACK_ROUTINE)(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
);

typedef struct _CONNECT_DATA {
  IN PDEVICE_OBJECT ClassDeviceObject;
  IN PSERVICE_CALLBACK_ROUTINE ClassService;
} CONNECT_DATA, *PCONNECT_DATA;

typedef struct _DEVICE_EXTENSION {
    CONNECT_DATA UpperConnectData;
    PDEVICE_OBJECT Self;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

BOOLEAN g_unloaded = FALSE;

KSPIN_LOCK g_IOSpinLock; // I/Oの排他ロック用

#define HOSTDRVNT_IO_ADDR	0x7EC
#define HOSTDRVNT_IO_CMD	0x7EE

#define NP2_PARAM_PORT	0x7ED
#define NP2_CMD_PORT	0x7EF

#define NP2_CMD_MAXLEN	16 // np2仕様で16byteまで
#define NP2_READ_MAXLEN	16 // np2仕様で16byteまで

#define NP2_COMMAND_NP2CHECK        "NP2"
#define NP2_COMMAND_GETMPOS         "getmpos"
#define NP2_COMMAND_CHANGECONFIG    "changeconfig"

BOOLEAN SendNP2Check()
{
    int i;
    char tmp;
    char commandText[] = NP2_COMMAND_NP2CHECK;
    KIRQL oldIrql;
    
    // 排他領域開始
	KeAcquireSpinLock(&g_IOSpinLock, &oldIrql);
	
    for(i=0;i<sizeof(commandText)/sizeof(commandText[0])-1;i++)
    {
        WRITE_PORT_UCHAR((PUCHAR)NP2_CMD_PORT, (UCHAR)commandText[i]);
    }
    for(i=0;i<sizeof(commandText)/sizeof(commandText[0])-1;i++)
    {
        tmp = READ_PORT_UCHAR((PUCHAR)NP2_CMD_PORT);
        if (tmp != commandText[i]) return FALSE;
    }
    
    // 排他領域終了
	KeReleaseSpinLock(&g_IOSpinLock, oldIrql);
	
    return TRUE;
}
// ホストカーソル非表示を送る
VOID SendNP2HideCursor()
{
    int i;
    char commandText[] = NP2_COMMAND_CHANGECONFIG;
    KIRQL oldIrql;
    
    // 排他領域開始
	KeAcquireSpinLock(&g_IOSpinLock, &oldIrql);
    
    // パラメータ 有効(1)
    WRITE_PORT_UCHAR((PUCHAR)NP2_PARAM_PORT, 1);
    // パラメータ 機能番号
    WRITE_PORT_UCHAR((PUCHAR)NP2_PARAM_PORT, 9);
    // コマンド送信
    for(i=0;i<sizeof(commandText)/sizeof(commandText[0])-1;i++)
    {
        WRITE_PORT_UCHAR((PUCHAR)NP2_CMD_PORT, (UCHAR)commandText[i]);
    }
    
    // 排他領域終了
	KeReleaseSpinLock(&g_IOSpinLock, oldIrql);
}
// ホストカーソル表示を送る
VOID SendNP2ShowCursor()
{
    int i;
    char commandText[] = NP2_COMMAND_CHANGECONFIG;
    KIRQL oldIrql;
    
    // 排他領域開始
	KeAcquireSpinLock(&g_IOSpinLock, &oldIrql);
    
    // パラメータ 無効(0)
    WRITE_PORT_UCHAR((PUCHAR)NP2_PARAM_PORT, 0);
    // パラメータ 機能番号
    WRITE_PORT_UCHAR((PUCHAR)NP2_PARAM_PORT, 9);
    // コマンド送信
    for(i=0;i<sizeof(commandText)/sizeof(commandText[0])-1;i++)
    {
        WRITE_PORT_UCHAR((PUCHAR)NP2_CMD_PORT, (UCHAR)commandText[i]);
    }
    
    // 排他領域終了
	KeReleaseSpinLock(&g_IOSpinLock, oldIrql);
}

ULONG SendNP2GetMousePos(PUSHORT x, PUSHORT y)
{
    int i;
    char tmp;
    char commandText[] = NP2_COMMAND_GETMPOS;
    KIRQL oldIrql;
    
    if(!x || !y) return 0;
    
    // 排他領域開始
	KeAcquireSpinLock(&g_IOSpinLock, &oldIrql);
    
    // 古いコマンド実行結果があれば掃除
    for(i=0;i<NP2_READ_MAXLEN;i++)
    {
        tmp = READ_PORT_UCHAR((PUCHAR)NP2_CMD_PORT);
        if(tmp == '\0') break;
    }
    
    // 将来拡張用パラメータ 一旦は0
    WRITE_PORT_UCHAR((PUCHAR)NP2_PARAM_PORT, 0);
    
    // コマンド送信
    for(i=0;i<sizeof(commandText)/sizeof(commandText[0])-1;i++)
    {
        WRITE_PORT_UCHAR((PUCHAR)NP2_CMD_PORT, (UCHAR)commandText[i]);
    }
    
    // 読み取り
    *x = 0;
    for(i=0;i<NP2_READ_MAXLEN;i++)
    {
        tmp = READ_PORT_UCHAR((PUCHAR)NP2_CMD_PORT);
        if (tmp == ',') break;
        if (tmp < '0' || '9' < tmp){
        	// 数値ではないので異常
			KeReleaseSpinLock(&g_IOSpinLock, oldIrql);
        	return 0;
        }
        *x = (*x * 10) + (tmp - '0');
    }
    *y = 0;
    for(;i<NP2_READ_MAXLEN;i++)
    {
        tmp = READ_PORT_UCHAR((PUCHAR)NP2_CMD_PORT);
        if (tmp == '\0') break;
        if (tmp < '0' || '9' < tmp){
        	// 数値ではないので異常
			KeReleaseSpinLock(&g_IOSpinLock, oldIrql);
        	return 0;
        }
        *y = (*y * 10) + (tmp - '0');
    }
    
    // 排他領域終了
	KeReleaseSpinLock(&g_IOSpinLock, oldIrql);
    
    if(i==NP2_READ_MAXLEN) return 0;
    
    return 1;
}

NTSTATUS
ReadCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
)
{
    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        ULONG bytes = (ULONG)Irp->IoStatus.Information;
        ULONG count = bytes / sizeof(MOUSE_INPUT_DATA);
		USHORT x, y;
		
		//KdPrint("FilterRead %d\n", bytes);
		if(count > 0 && SendNP2GetMousePos(&x, &y)){
        	ULONG i;
	        PMOUSE_INPUT_DATA data = (PMOUSE_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
	        // WORKAROUND: 0だと無視されることがあるので最低でも1を入れておく
	        if(x < 1) x = 1;
	        if(y < 1) y = 1;
	        for (i = 0; i < count; i++) {
	            data[i].LastX = x;
	            data[i].LastY = y;
	            data[i].Flags = (data[i].Flags & ~MOUSE_MOVE_RELATIVE) | MOUSE_MOVE_ABSOLUTE;
	        	//KdPrint("Mouse X:%d Y:%d Buttons:%x\n", data[i].LastX, data[i].LastY, data[i].ButtonFlags);
	        }
    	}
    }
    
    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FilterRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
	if(g_unloaded){
	    IoSkipCurrentIrpStackLocation(Irp);
	    return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->UpperConnectData.ClassDeviceObject, Irp);
	}else{
    	NTSTATUS status;
    	
	    IoCopyCurrentIrpStackLocationToNext(Irp);
	    IoSetCompletionRoutine(
	        Irp,
	        ReadCompletionRoutine,
	        DeviceObject,
	        TRUE,
	        TRUE,
	        TRUE
	    );

	    return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->UpperConnectData.ClassDeviceObject, Irp);
	}
}

NTSTATUS
FilterOtherDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->UpperConnectData.ClassDeviceObject, Irp);
}

VOID
FilterUnload(IN PDRIVER_OBJECT DriverObject)
{
	g_unloaded = TRUE;
	if(DriverObject->DeviceObject){
    	IoDetachDevice(((PDEVICE_EXTENSION)DriverObject->DeviceObject->DeviceExtension)->UpperConnectData.ClassDeviceObject);
	}
    IoDeleteDevice(DriverObject->DeviceObject);
    
    SendNP2ShowCursor();
}

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING devName;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_OBJECT lowerDevice = NULL;
    PDEVICE_EXTENSION devExt;
    int i;
    
    KdPrint("npmouse: loading...\n");

    // 排他ロック初期化　初期化のみで破棄処理はいらない
    KeInitializeSpinLock(&g_IOSpinLock);
    
    if(!SendNP2Check())
    {
    	KdPrint("npmouse: not np2\n");
        return STATUS_NO_SUCH_DEVICE;
    }
    
    RtlInitUnicodeString(&devName, MOUSEPORT_DEVNAME);
    
    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_MOUSE,
                            0,
                            FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) return status;

    DriverObject->DriverUnload = FilterUnload;
    
    KdPrint("npmouse: IoCreateDevice success\n");

    // アタッチ
    status = IoAttachDevice(deviceObject, &devName, &lowerDevice);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        return status;
    }
    
    // 拡張構造体の初期化
    devExt = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
    RtlZeroMemory(devExt, sizeof(DEVICE_EXTENSION));
    devExt->Self = deviceObject;
    devExt->UpperConnectData.ClassDeviceObject = lowerDevice;

    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = FilterOtherDispatch;
    }
    DriverObject->MajorFunction[IRP_MJ_READ] = FilterRead;
    
    // 初期化完了フラグを立てる　これをしないとブルースクリーン
    deviceObject->Flags |= DO_BUFFERED_IO;
	if (deviceObject->Flags & DO_POWER_PAGABLE)
	    deviceObject->Flags |= DO_POWER_PAGABLE;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    SendNP2HideCursor();

    KdPrint("npmouse: loaded\n");

    return STATUS_SUCCESS;
}