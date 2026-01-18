
#if defined(SUPPORT_SCSI)

typedef struct {
	UINT	port;
	UINT	phase;
	UINT8	reg[0x30];
	UINT8	auxstatus;
	UINT8	scsistatus;
	UINT8	membank;
	UINT8	memwnd;
	UINT8	resent;
	UINT8	datmap;
	UINT	cmdpos;
	UINT	wrdatpos;
	UINT	rddatpos;
	UINT8	cmd[12];
	UINT8	data[0x10000];
	UINT8	bios[2][0x2000];
} _SCSIIO, *SCSIIO;


#ifdef __cplusplus
extern "C" {
#endif

extern	_SCSIIO		scsiio;

void scsiioint(NEVENTITEM item);

void scsiio_reset(const NP2CFG *pConfig);
void scsiio_bind(void);

#ifdef __cplusplus
}
#endif

#if defined(SUPPORT_NP2SCSI)

typedef struct
{
	UINT32 enable;
	UINT32 ioenable;
	UINT32 maddr;
	UINT32 busyflag;
	UINT32 reserved;
} _NP2STOR, *NP2STOR;

extern	_NP2STOR	np2stor;

#define NP2STOR_CMD_ACTIVATE	0x98
#define NP2STOR_CMD_STARTIO		0x01

#define NP2STOR_INVOKECMD_DEFAULT		0x00
#define NP2STOR_INVOKECMD_NOBUSY		0x01

#define NP2STOR_SECTOR_SIZE		512

#define NP2_SRB_FUNCTION_EXECUTE_SCSI           0x00
#define NP2_SRB_FUNCTION_CLAIM_DEVICE           0x01
#define NP2_SRB_FUNCTION_IO_CONTROL             0x02
#define NP2_SRB_FUNCTION_RECEIVE_EVENT          0x03
#define NP2_SRB_FUNCTION_RELEASE_QUEUE          0x04
#define NP2_SRB_FUNCTION_ATTACH_DEVICE          0x05
#define NP2_SRB_FUNCTION_RELEASE_DEVICE         0x06
#define NP2_SRB_FUNCTION_SHUTDOWN               0x07
#define NP2_SRB_FUNCTION_FLUSH                  0x08
#define NP2_SRB_FUNCTION_ABORT_COMMAND          0x10
#define NP2_SRB_FUNCTION_RELEASE_RECOVERY       0x11
#define NP2_SRB_FUNCTION_RESET_BUS              0x12
#define NP2_SRB_FUNCTION_RESET_DEVICE           0x13
#define NP2_SRB_FUNCTION_TERMINATE_IO           0x14
#define NP2_SRB_FUNCTION_FLUSH_QUEUE            0x15
#define NP2_SRB_FUNCTION_REMOVE_DEVICE          0x16
#define NP2_SRB_FUNCTION_WMI                    0x17
#define NP2_SRB_FUNCTION_LOCK_QUEUE             0x18
#define NP2_SRB_FUNCTION_UNLOCK_QUEUE           0x19

#define NP2_SRB_STATUS_PENDING                  0x00
#define NP2_SRB_STATUS_SUCCESS                  0x01
#define NP2_SRB_STATUS_ABORTED                  0x02
#define NP2_SRB_STATUS_ABORT_FAILED             0x03
#define NP2_SRB_STATUS_ERROR                    0x04
#define NP2_SRB_STATUS_BUSY                     0x05
#define NP2_SRB_STATUS_INVALID_REQUEST          0x06
#define NP2_SRB_STATUS_INVALID_PATH_ID          0x07
#define NP2_SRB_STATUS_NO_DEVICE                0x08
#define NP2_SRB_STATUS_TIMEOUT                  0x09
#define NP2_SRB_STATUS_SELECTION_TIMEOUT        0x0A
#define NP2_SRB_STATUS_COMMAND_TIMEOUT          0x0B
#define NP2_SRB_STATUS_MESSAGE_REJECTED         0x0D
#define NP2_SRB_STATUS_BUS_RESET                0x0E
#define NP2_SRB_STATUS_PARITY_ERROR             0x0F
#define NP2_SRB_STATUS_REQUEST_SENSE_FAILED     0x10
#define NP2_SRB_STATUS_NO_HBA                   0x11
#define NP2_SRB_STATUS_DATA_OVERRUN             0x12
#define NP2_SRB_STATUS_UNEXPECTED_BUS_FREE      0x13
#define NP2_SRB_STATUS_PHASE_SEQUENCE_FAILURE   0x14
#define NP2_SRB_STATUS_BAD_SRB_BLOCK_LENGTH     0x15
#define NP2_SRB_STATUS_REQUEST_FLUSHED          0x16
#define NP2_SRB_STATUS_INVALID_LUN              0x20
#define NP2_SRB_STATUS_INVALID_TARGET_ID        0x21
#define NP2_SRB_STATUS_BAD_FUNCTION             0x22
#define NP2_SRB_STATUS_ERROR_RECOVERY           0x23
#define NP2_SRB_STATUS_NOT_POWERED              0x24

#define NP2_SCSIOP_TEST_UNIT_READY      0x00
#define NP2_SCSIOP_REZERO_UNIT          0x01
#define NP2_SCSIOP_REWIND               0x01
#define NP2_SCSIOP_REQUEST_BLOCK_ADDR   0x02
#define NP2_SCSIOP_REQUEST_SENSE        0x03
#define NP2_SCSIOP_FORMAT_UNIT          0x04
#define NP2_SCSIOP_READ_BLOCK_LIMITS    0x05
#define NP2_SCSIOP_REASSIGN_BLOCKS      0x07
#define NP2_SCSIOP_READ6                0x08
#define NP2_SCSIOP_RECEIVE              0x08
#define NP2_SCSIOP_WRITE6               0x0A
#define NP2_SCSIOP_PRINT                0x0A
#define NP2_SCSIOP_SEND                 0x0A
#define NP2_SCSIOP_SEEK6                0x0B
#define NP2_SCSIOP_TRACK_SELECT         0x0B
#define NP2_SCSIOP_SLEW_PRINT           0x0B
#define NP2_SCSIOP_SEEK_BLOCK           0x0C
#define NP2_SCSIOP_PARTITION            0x0D
#define NP2_SCSIOP_READ_REVERSE         0x0F
#define NP2_SCSIOP_WRITE_FILEMARKS      0x10
#define NP2_SCSIOP_FLUSH_BUFFER         0x10
#define NP2_SCSIOP_SPACE                0x11
#define NP2_SCSIOP_INQUIRY              0x12
#define NP2_SCSIOP_VERIFY6              0x13
#define NP2_SCSIOP_RECOVER_BUF_DATA     0x14
#define NP2_SCSIOP_MODE_SELECT          0x15
#define NP2_SCSIOP_RESERVE_UNIT         0x16
#define NP2_SCSIOP_RELEASE_UNIT         0x17
#define NP2_SCSIOP_COPY                 0x18
#define NP2_SCSIOP_ERASE                0x19
#define NP2_SCSIOP_MODE_SENSE           0x1A
#define NP2_SCSIOP_START_STOP_UNIT      0x1B
#define NP2_SCSIOP_STOP_PRINT           0x1B
#define NP2_SCSIOP_LOAD_UNLOAD          0x1B
#define NP2_SCSIOP_RECEIVE_DIAGNOSTIC   0x1C
#define NP2_SCSIOP_SEND_DIAGNOSTIC      0x1D
#define NP2_SCSIOP_MEDIUM_REMOVAL       0x1E
#define NP2_SCSIOP_READ_FORMAT_CAPACITY 0x23
#define NP2_SCSIOP_READ_CAPACITY        0x25
#define NP2_SCSIOP_READ                 0x28
#define NP2_SCSIOP_WRITE                0x2A
#define NP2_SCSIOP_WRITE_CD             0x2A
#define NP2_SCSIOP_SEEK                 0x2B
#define NP2_SCSIOP_LOCATE               0x2B
#define NP2_SCSIOP_ERASE10              0x2C
#define NP2_SCSIOP_WRITE_VERIFY         0x2E
#define NP2_SCSIOP_VERIFY               0x2F
#define NP2_SCSIOP_SEARCH_DATA_HIGH     0x30
#define NP2_SCSIOP_SEARCH_DATA_EQUAL    0x31
#define NP2_SCSIOP_SEARCH_DATA_LOW      0x32
#define NP2_SCSIOP_SET_LIMITS           0x33
#define NP2_SCSIOP_READ_POSITION        0x34
#define NP2_SCSIOP_SYNCHRONIZE_CACHE    0x35
#define NP2_SCSIOP_COMPARE              0x39
#define NP2_SCSIOP_COPY_COMPARE         0x3A
#define NP2_SCSIOP_COPY_VERIFY          0x3A
#define NP2_SCSIOP_WRITE_DATA_BUFF      0x3B
#define NP2_SCSIOP_READ_DATA_BUFF       0x3C
#define NP2_SCSIOP_CHANGE_DEFINITION    0x40
#define NP2_SCSIOP_PLAY_AUDIO10         0x41
#define NP2_SCSIOP_READ_SUB_CHANNEL     0x42
#define NP2_SCSIOP_READ_TOC             0x43
#define NP2_SCSIOP_READ_HEADER          0x44
#define NP2_SCSIOP_PLAY_AUDIO           0x45
#define NP2_SCSIOP_GET_CONFIGURATION    0x46
#define NP2_SCSIOP_PLAY_AUDIO_MSF       0x47
#define NP2_SCSIOP_PLAY_TRACK_INDEX     0x48
#define NP2_SCSIOP_PLAY_TRACK_RELATIVE  0x49
#define NP2_SCSIOP_GET_EVENT_STATUS     0x4A
#define NP2_SCSIOP_PAUSE_RESUME         0x4B
#define NP2_SCSIOP_LOG_SELECT           0x4C
#define NP2_SCSIOP_LOG_SENSE            0x4D
#define NP2_SCSIOP_STOP_PLAY_SCAN       0x4E
#define NP2_SCSIOP_READ_DISC_INFO       0x51
#define NP2_SCSIOP_READ_TRACK_INFO      0x52
#define NP2_SCSIOP_RESERVE_TRACK        0x53
#define NP2_SCSIOP_SEND_OPC_INFO        0x54
#define NP2_SCSIOP_MODE_SELECT10        0x55
#define NP2_SCSIOP_REPAIR_TRACK         0x58
#define NP2_SCSIOP_READ_MASTER_CUE      0x59
#define NP2_SCSIOP_MODE_SENSE10         0x5A
#define NP2_SCSIOP_CLOSE_TRACK_SESSION  0x5B
#define NP2_SCSIOP_READ_BUFFER_CAPACITY 0x5C
#define NP2_SCSIOP_SEND_CUE_SHEET       0x5D
#define NP2_SCSIOP_READ16               0x88
#define NP2_SCSIOP_WRITE16              0x8A
#define NP2_SCSIOP_VERIFY16             0x8F
#define NP2_SCSIOP_SERVICE_ACTION16     0x9E
#define NP2_SCSIOP_SA_READ_CAPACITY16   0x10
#define NP2_SCSIOP_REPORT_LUNS          0xA0
#define NP2_SCSIOP_BLANK                0xA1
#define NP2_SCSIOP_SEND_KEY             0xA3
#define NP2_SCSIOP_REPORT_KEY           0xA4
#define NP2_SCSIOP_PLAY_AUDIO12         0xA5
#define NP2_SCSIOP_LOAD_UNLOAD_SLOT     0xA6
#define NP2_SCSIOP_SET_READ_AHEAD       0xA7
#define NP2_SCSIOP_READ12               0xA8
#define NP2_SCSIOP_WRITE12              0xAA
#define NP2_SCSIOP_VERIFY12             0xAF
#define NP2_SCSIOP_SEEK12               0xAB
#define NP2_SCSIOP_GET_PERFORMANCE      0xAC
#define NP2_SCSIOP_READ_DVD_STRUCTURE   0xAD
#define NP2_SCSIOP_WRITE_VERIFY12       0xAE
#define NP2_SCSIOP_VERIFY12             0xAF
#define NP2_SCSIOP_SET_STREAMING        0xB6
#define NP2_SCSIOP_READ_CD_MSF          0xB9
#define NP2_SCSIOP_SET_CD_SPEED         0xBB
#define NP2_SCSIOP_MECHANISM_STATUS     0xBD
#define NP2_SCSIOP_READ_CD              0xBE
#define NP2_SCSIOP_SEND_DVD_STRUCTURE   0xBF
#define NP2_SCSIOP_DOORLOCK             0xDE
#define NP2_SCSIOP_DOORUNLOCK           0xDF

#define NP2_SCSISTAT_GOOD                  0x00
#define NP2_SCSISTAT_CHECK_CONDITION       0x02
#define NP2_SCSISTAT_CONDITION_MET         0x04
#define NP2_SCSISTAT_BUSY                  0x08
#define NP2_SCSISTAT_INTERMEDIATE          0x10
#define NP2_SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define NP2_SCSISTAT_RESERVATION_CONFLICT  0x18
#define NP2_SCSISTAT_COMMAND_TERMINATED    0x22
#define NP2_SCSISTAT_QUEUE_FULL            0x28

#define NP2_MODE_SENSE_RETURN_ALL           0x3f

#define NP2_MODE_PAGE_ERROR_RECOVERY        0x01
#define NP2_MODE_PAGE_DISCONNECT            0x02
#define NP2_MODE_PAGE_FORMAT_DEVICE         0x03
#define NP2_MODE_PAGE_RIGID_GEOMETRY        0x04

#pragma pack(push, 4)
typedef struct _NP2_INQUIRYDATA
{
	UINT8 DeviceType : 5;
	UINT8 DeviceTypeQualifier : 3;
	UINT8 DeviceTypeModifier : 7;
	UINT8 RemovableMedia : 1;
	UINT8 Versions;
	UINT8 ResponseDataFormat : 4;
	UINT8 HiSupport : 1;
	UINT8 NormACA : 1;
	UINT8 ReservedBit : 1;
	UINT8 AERC : 1;
	UINT8 AdditionalLength;
	UINT8 Reserved[2];
	UINT8 SoftReset : 1;
	UINT8 CommandQueue : 1;
	UINT8 Reserved2 : 1;
	UINT8 LinkedCommands : 1;
	UINT8 Synchronous : 1;
	UINT8 Wide16Bit : 1;
	UINT8 Wide32Bit : 1;
	UINT8 RelativeAddressing : 1;
	UINT8 VendorId[8];
	UINT8 ProductId[16];
	UINT8 ProductRevisionLevel[4];
	UINT8 VendorSpecific[20];
	UINT8 Reserved3[40];
} NP2_INQUIRYDATA;
typedef struct _NP2_READ_CAPACITY_DATA
{
	UINT32 LogicalBlockAddress;
	UINT32 BytesPerBlock;
} NP2_READ_CAPACITY_DATA;

typedef struct
{
	UINT32 version; // \83o\81[\83W\83\87\83\93 \8D\A1\82̂Ƃ\B1\82\EB1\82̂\DD
	UINT32 cmd; // \83R\83}\83\93\83h NP2STOR_INVOKECMD_xxx
	UINT32 srbAddr; // SCSI_REQUEST_BLOCK\82ւ̉\BC\91z\83\81\83\82\83\8A\83A\83h\83\8C\83X
} NP2STOR_INVOKEINFO;

typedef struct _NP2_SCSI_REQUEST_BLOCK
{
	UINT16 Length;                
	UINT8 Function;               
	UINT8 SrbStatus;              
	UINT8 ScsiStatus;             
	UINT8 PathId;                 
	UINT8 TargetId;               
	UINT8 Lun;                    
	UINT8 QueueTag;               
	UINT8 QueueAction;            
	UINT8 CdbLength;              
	UINT8 SenseInfoBufferLength;  
	UINT32 SrbFlags;              
	UINT32 DataTransferLength;    
	UINT32 TimeOutValue;          
	UINT32 DataBuffer;            
	UINT32 SenseInfoBuffer;       
	UINT32 NextSrb; // offset 20
	UINT32 OriginalRequest;       
	UINT32 SrbExtension;          
	union
	{
		UINT32 InternalStatus;    
		UINT32 QueueSortKey;      
	};
	UINT8 Cdb[16];                
} NP2_SCSI_REQUEST_BLOCK;

typedef struct _NP2_SRB_IO_CONTROL
{
	UINT32 HeaderLength;
	UINT8 Signature[8];
	UINT32 Timeout;
	UINT32 ControlCode;
	UINT32 ReturnCode;
	UINT32 Length;
} NP2_SRB_IO_CONTROL;

typedef struct _NP2_DISK_GEOMETRY
{
	UINT64 Cylinders;
	UINT32 MediaType;
	UINT32 TracksPerCylinder;
	UINT32 SectorsPerTrack;
	UINT32 BytesPerSector;
} NP2_DISK_GEOMETRY;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _NP2_MODE_PARAMETER_HEADER
{
	UINT8 ModeDataLength;
	UINT8 MediumType;
	UINT8 DeviceSpecificParameter;
	UINT8 BlockDescriptorLength;
} NP2_MODE_PARAMETER_HEADER;

typedef struct _NP2_MODE_PARAMETER_BLOCK
{
	UINT8 DensityCode;
	UINT8 NumberOfBlocks[3];
	UINT8 Reserved;
	UINT8 BlockLength[3];
} NP2_MODE_PARAMETER_BLOCK;

typedef struct _NP2_MODE_DUMMY_PAGE
{
	UINT8 PageCode : 6;
	UINT8 Reserved : 1;
	UINT8 PageSavable : 1;
	UINT8 PageLength;
} NP2_MODE_DUMMY_PAGE;

typedef struct _NP2_MODE_READ_RECOVERY_PAGE
{
	UINT8 PageCode : 6; // 0x01
	UINT8 Reserved1 : 1;
	UINT8 PSBit : 1;
	UINT8 PageLength; // sizeof(NP2_MODE_READ_RECOVERY_PAGE) - 2 = 6
	UINT8 DCRBit : 1;
	UINT8 DTEBit : 1;
	UINT8 PERBit : 1;
	UINT8 Reserved2 : 1;
	UINT8 RCBit : 1;
	UINT8 TBBit : 1;
	UINT8 Reserved3 : 2;
	UINT8 ReadRetryCount;
	UINT8 Reserved4[4];
} NP2_MODE_READ_RECOVERY_PAGE;

typedef struct _NP2_MODE_FORMAT_PAGE
{
	UINT8 PageCode : 6; // 0x03
	UINT8 Reserved : 1;
	UINT8 PageSavable : 1;
	UINT8 PageLength; // sizeof(NP2_MODE_FORMAT_PAGE) - 2 = 22
	UINT8 TracksPerZone[2];
	UINT8 AlternateSectorsPerZone[2];
	UINT8 AlternateTracksPerZone[2];
	UINT8 AlternateTracksPerLogicalUnit[2];
	UINT8 SectorsPerTrack[2];
	UINT8 BytesPerPhysicalSector[2];
	UINT8 Interleave[2];
	UINT8 TrackSkewFactor[2];
	UINT8 CylinderSkewFactor[2];
	UINT8 Reserved2 : 4;
	UINT8 SurfaceFirst : 1;
	UINT8 RemovableMedia : 1;
	UINT8 HardSectorFormating : 1;
	UINT8 SoftSectorFormating : 1;
	UINT8 Reserved3[3];
} NP2_MODE_FORMAT_PAGE;

typedef struct _NP2_MODE_RIGID_GEOMETRY_PAGE
{
	UINT8 PageCode : 6; // 0x04
	UINT8 Reserved : 1;
	UINT8 PageSavable : 1;
	UINT8 PageLength; // sizeof(NP2_MODE_READ_RECOVERY_PAGE) - 2 = 18 <-- NEC\82\CD22\82ł͂Ȃ\AD18
	UINT8 NumberOfCylinders[3];
	UINT8 NumberOfHeads;
	UINT8 StartWritePrecom[3];
	UINT8 StartReducedCurrent[3];
	UINT8 DriveStepRate[2];
	UINT8 LandZoneCyclinder[3];
	UINT8 RotationalPositionLock : 2;
	UINT8 Reserved2 : 6;
	UINT8 RotationOffset;
	UINT8 Reserved3;
} NP2_MODE_RIGID_GEOMETRY_PAGE;
#pragma pack(pop)

#endif

#endif

