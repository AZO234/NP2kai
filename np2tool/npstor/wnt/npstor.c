#include <ntddk.h>
#include <scsi.h>

#define SECTOR_SIZE 512

#define NPSTOR_IO_ADDR	0x7EA
#define NPSTOR_IO_CMD	0x7EB

typedef struct _DEVICE_EXTENSION {
    ULONG dummy[8];
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _HW_LU_EXTENSION {
    PSCSI_REQUEST_BLOCK CurrentSrb;
} HW_LU_EXTENSION, *PHW_LU_EXTENSION;

typedef struct _CCB {
    ULONG dummy[16];
} CCB, *PCCB;

typedef struct
{
	ULONG version;
	ULONG cmd;
	PSCSI_REQUEST_BLOCK srbAddr;
} NP2STOR_INVOKEINFO;

ULONG DriverEntry(PVOID DriverObject, PVOID RegistryPath);
ULONG HwFindAdapter(PVOID DeviceExtension, PVOID Context,
                    PVOID BusInfo, PCHAR ArgumentString,
                    PPORT_CONFIGURATION_INFORMATION ConfigInfo,
                    PBOOLEAN Again);
BOOLEAN HwInitialize(PVOID DeviceExtension);
BOOLEAN HwStartIo(PVOID DeviceExtension, PSCSI_REQUEST_BLOCK Srb);
BOOLEAN HwResetBus(PVOID DeviceExtension, ULONG PathId);
BOOLEAN HwInterrupt(PVOID DeviceExtension);

KSPIN_LOCK g_spinLock; // I/Oの排他ロック用

ULONG DriverEntry(PVOID DriverObject, PVOID RegistryPath) {
    HW_INITIALIZATION_DATA hwInit = {0};
    CHAR vendorId[] = "9800";
    CHAR deviceId[] = "SCSI\\NP2_____FASTSTORAGE_____1.00";
    ULONG status;
    
    // 対応かを簡易チェック
    if(READ_PORT_UCHAR((PUCHAR)NPSTOR_IO_ADDR) != 98 || READ_PORT_UCHAR((PUCHAR)NPSTOR_IO_CMD) != 21){
        return STATUS_NO_SUCH_DEVICE;
	}
	
    // 排他ロック初期化　初期化のみで破棄処理はいらない
    KeInitializeSpinLock(&g_spinLock);
    
	//DbgPrint("DriverEntry\n");
    hwInit.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);
    hwInit.AdapterInterfaceType = Isa;
    hwInit.HwInitialize     = HwInitialize;
    hwInit.HwStartIo        = HwStartIo;
    hwInit.HwInterrupt      = HwInterrupt;
    hwInit.HwFindAdapter    = HwFindAdapter;
    hwInit.HwResetBus       = HwResetBus;
    hwInit.HwDmaStarted     = NULL;
    hwInit.HwAdapterState   = NULL;
    hwInit.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
    hwInit.SpecificLuExtensionSize = sizeof(HW_LU_EXTENSION);
    hwInit.SrbExtensionSize = sizeof(CCB);
    hwInit.NumberOfAccessRanges = 1;
    hwInit.MapBuffers = TRUE;
    hwInit.NeedPhysicalAddresses = FALSE;
    hwInit.TaggedQueuing = TRUE;
    hwInit.AutoRequestSense = FALSE;
    hwInit.MultipleRequestPerLu = TRUE;
    hwInit.ReceiveEvent = FALSE;
    hwInit.VendorIdLength = sizeof(vendorId)-1;
    hwInit.VendorId = vendorId;
    hwInit.DeviceIdLength = sizeof(deviceId)-1;
    hwInit.DeviceId = deviceId;
    //hwInit.HwAdapterControl = NULL;

    status = ScsiPortInitialize(DriverObject, RegistryPath, &hwInit, NULL);
	//DbgPrint("  status = 0x%08x\n", status);
    return status;
}

ULONG HwFindAdapter(PVOID DeviceExtension, PVOID Context,
                    PVOID BusInfo, PCHAR ArgumentString,
                    PPORT_CONFIGURATION_INFORMATION ConfigInfo,
                    PBOOLEAN Again) {
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(BusInfo);
    UNREFERENCED_PARAMETER(ArgumentString);
    UNREFERENCED_PARAMETER(Again);

    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->InitiatorBusId[0] = 7;
    ConfigInfo->InterruptMode = LevelSensitive;
    ConfigInfo->BusInterruptLevel = 0;
    ConfigInfo->BusInterruptVector = 0;
    ConfigInfo->DmaChannel = SP_UNINITIALIZED_VALUE;
    //ConfigInfo->MaximumNumberOfLogicalUnits = 1;
    
    (*ConfigInfo->AccessRanges)[0].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0x7EA);
    (*ConfigInfo->AccessRanges)[0].RangeLength = 2;
    (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE; // FALSE=I/Oポート

    *Again = FALSE;
    
    return SP_RETURN_FOUND;
}

BOOLEAN HwInitialize(PVOID DeviceExtension) {
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)DeviceExtension;

    return TRUE;
}

BOOLEAN HwStartIo(PVOID DeviceExtension, PSCSI_REQUEST_BLOCK Srb) {
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)DeviceExtension;
    PUCHAR buffer;
    ULONG offset, length;
    KIRQL oldIrql;
    NP2STOR_INVOKEINFO invokeInfo;
    ULONG invokeInfoAddr;

    Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
       
    // ねこー本体に処理を委任する
    // データ格納先の仮想メモリアドレスをI/OポートNPSTOR_IO_ADDRへ書き込み、
    // NPSTOR_IO_CMDに0x98→0x01の順で書き込むと猫本体が処理を実行する。
	invokeInfo.version = 1;
	invokeInfo.cmd = 0;
	invokeInfo.srbAddr = Srb;
	KeAcquireSpinLock(&g_spinLock, &oldIrql);
	invokeInfoAddr = (ULONG)(&invokeInfo);
    WRITE_PORT_UCHAR((PUCHAR)NPSTOR_IO_ADDR, (UCHAR)(invokeInfoAddr));
    WRITE_PORT_UCHAR((PUCHAR)NPSTOR_IO_ADDR, (UCHAR)(invokeInfoAddr >> 8));
    WRITE_PORT_UCHAR((PUCHAR)NPSTOR_IO_ADDR, (UCHAR)(invokeInfoAddr >> 16));
    WRITE_PORT_UCHAR((PUCHAR)NPSTOR_IO_ADDR, (UCHAR)(invokeInfoAddr >> 24));
    WRITE_PORT_UCHAR((PUCHAR)NPSTOR_IO_CMD, (UCHAR)0x98);
    WRITE_PORT_UCHAR((PUCHAR)NPSTOR_IO_CMD, (UCHAR)0x01);
	KeReleaseSpinLock(&g_spinLock, oldIrql);
    
    ScsiPortNotification(RequestComplete, DeviceExtension, Srb);
    ScsiPortNotification(NextRequest, DeviceExtension, NULL);
    return TRUE;
}

BOOLEAN HwResetBus(PVOID DeviceExtension, ULONG PathId) {
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(PathId);
    
    //KeBugCheckEx(0xDEADDEAD, 0x3, 0x01, 0x98, 0x01);

    return TRUE;
}

BOOLEAN HwInterrupt(PVOID DeviceExtension)
{
    UNREFERENCED_PARAMETER(DeviceExtension);
    return FALSE;
}
