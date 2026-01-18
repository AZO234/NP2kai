#include <ntddk.h>
#include <ntddmou.h>  // MOUSE_INPUT_DATA など

#define IOCTL_INTERNAL_MOUSE_CONNECT \
  CTL_CODE(FILE_DEVICE_MOUSE, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)
 
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

KSPIN_LOCK g_IOSpinLock; // I/Oの排他ロック用

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
		KIRQL currentIrql;
		
		currentIrql = KeGetCurrentIrql();
		//KdPrint("IRQL %d\n", currentIrql);

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

NTSTATUS
FilterOtherDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    KdPrint("npmouse: pass %d\n", irpSp->MajorFunction);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->UpperConnectData.ClassDeviceObject, Irp);
}

NTSTATUS
FilterClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    KdPrint("npmouse: close %d\n", irpSp->MajorFunction);
    
    SendNP2ShowCursor();

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->UpperConnectData.ClassDeviceObject, Irp);
}

VOID
FilterUnload(IN PDRIVER_OBJECT DriverObject)
{
    IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS
FilterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
)
{
    NTSTATUS status;
    PDEVICE_OBJECT filterDeviceObject;
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT lowerDeviceObject;

    KdPrint("npmouse: FilterAddDevice\n");
    
	{
		UNICODE_STRING deviceName;
		WCHAR nameBuffer[512];
		ULONG resultLength = 0;
		deviceName.Buffer = nameBuffer;
		deviceName.Length = 0;
		deviceName.MaximumLength = sizeof(nameBuffer);

		status = IoGetDeviceProperty(
		    PhysicalDeviceObject,
		    DevicePropertyDriverKeyName,
		    sizeof(nameBuffer),
		    nameBuffer,
		    &resultLength
		);
		if (NT_SUCCESS(status)) {
		    KdPrint("npmouse: FilterAddDevice called for device %ws\n", nameBuffer);
		} else {
		    KdPrint("npmouse: FilterAddDevice IoGetDeviceProperty failed (0x%08X)\n", status);
		}
	}

    // 自身のデバイス作成
    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_MOUSE,
                            0,
                            FALSE,
                            &filterDeviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    KdPrint("npmouse: FilterAddDevice IoCreateDevice success\n");

    // 下位の PDO にアタッチ
    lowerDeviceObject = IoAttachDeviceToDeviceStack(filterDeviceObject, PhysicalDeviceObject);
    if (lowerDeviceObject == NULL) {
        IoDeleteDevice(filterDeviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    KdPrint("npmouse: FilterAddDevice IoAttachDeviceToDeviceStack success\n");

    // 拡張構造体の初期化
    devExt = (PDEVICE_EXTENSION)filterDeviceObject->DeviceExtension;
    RtlZeroMemory(devExt, sizeof(DEVICE_EXTENSION));
    devExt->Self = filterDeviceObject;
    devExt->UpperConnectData.ClassDeviceObject = lowerDeviceObject;

    // デバイスフラグの設定
    filterDeviceObject->Flags |= DO_BUFFERED_IO;
	if (lowerDeviceObject->Flags & DO_POWER_PAGABLE)
	    filterDeviceObject->Flags |= DO_POWER_PAGABLE;
    filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    SendNP2HideCursor();

    KdPrint("npmouse: FilterAddDevice success\n");

    return STATUS_SUCCESS;
}

NTSTATUS
FilterPower(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
)
{
    PIO_STACK_LOCATION  irpStack;
    PDEVICE_EXTENSION   devExt;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;

    PAGED_CODE();

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    KdPrint("npmouse: Power %d\n", irpStack->MinorFunction);
    
	if(irpStack->MinorFunction == IRP_MN_SET_POWER) {
	    if (irpStack->Parameters.Power.Type == SystemPowerState) {
	        if (irpStack->Parameters.Power.State.SystemState == PowerSystemShutdown) {
	            // シャットダウン直前の処理 カーソル表示を再有効化
    			SendNP2ShowCursor();
	        }
	    }
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->UpperConnectData.ClassDeviceObject, Irp);
}

NTSTATUS
FilterPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    NTSTATUS status;

    KdPrint("npmouse: FilterPnP %d\n", irpStack->MinorFunction);
    switch (irpStack->MinorFunction) {
    case IRP_MN_REMOVE_DEVICE:
        // アタッチ解除
        if (devExt->UpperConnectData.ClassDeviceObject) {
            IoDetachDevice(devExt->UpperConnectData.ClassDeviceObject);
        }

        // 自分のデバイス削除
        IoDeleteDevice(DeviceObject);

    	SendNP2ShowCursor();

    	KdPrint("npmouse: IRP_MN_REMOVE_DEVICE\n");
        // 次のドライバへ送る
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(devExt->UpperConnectData.ClassDeviceObject, Irp);

    default:
        // それ以外はすべてパススルー
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(devExt->UpperConnectData.ClassDeviceObject, Irp);
    }
}

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = NULL;
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
    
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = FilterOtherDispatch;
    }
    DriverObject->MajorFunction[IRP_MJ_READ] = FilterRead;
    DriverObject->MajorFunction[IRP_MJ_POWER] = FilterPower;
    DriverObject->MajorFunction[IRP_MJ_PNP] = FilterPnP;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FilterClose;
    DriverObject->DriverUnload = FilterUnload;

    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_MOUSE,
                            0,
                            FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) return status;

    KdPrint("npmouse: IoCreateDevice success\n");

    devExt = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
    RtlZeroMemory(devExt, sizeof(DEVICE_EXTENSION));
    devExt->Self = deviceObject;
    
    DriverObject->DriverExtension->AddDevice = FilterAddDevice;

    KdPrint("npmouse: loaded\n");

    return STATUS_SUCCESS;
}