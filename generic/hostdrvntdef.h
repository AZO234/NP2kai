/**
 * @file	hostdrvntdef.h
 * @brief	Definitions of host drive for Windows NT
 */

#pragma once

#if defined(SUPPORT_HOSTDRVNT)

typedef enum
{
	FileFsVolumeInformation = 1,
	FileFsLabelInformation = 2,
	FileFsSizeInformation = 3,
	FileFsDeviceInformation = 4,
	FileFsAttributeInformation = 5,
	FileFsControlInformation = 6,
	FileFsFullSizeInformation = 7,
	FileFsObjectIdInformation = 8,
	FileFsDriverPathInformation = 9,
	FileFsVolumeFlagsInformation = 10,
	FileFsMaximumInformation = 11
} FS_INFORMATION_CLASS;
typedef enum
{
	FileDirectoryInformation = 1,
	FileFullDirectoryInformation,   // 2
	FileBothDirectoryInformation,   // 3
	FileBasicInformation,           // 4
	FileStandardInformation,        // 5
	FileInternalInformation,        // 6
	FileEaInformation,              // 7
	FileAccessInformation,          // 8
	FileNameInformation,            // 9
	FileRenameInformation,          // 10
	FileLinkInformation,            // 11
	FileNamesInformation,           // 12
	FileDispositionInformation,     // 13
	FilePositionInformation,        // 14
	FileFullEaInformation,          // 15
	FileModeInformation,            // 16
	FileAlignmentInformation,       // 17
	FileAllInformation,             // 18
	FileAllocationInformation,      // 19
	FileEndOfFileInformation,       // 20
	FileAlternateNameInformation,   // 21
	FileStreamInformation,          // 22
	FilePipeInformation,            // 23
	FilePipeLocalInformation,       // 24
	FilePipeRemoteInformation,      // 25
	FileMailslotQueryInformation,   // 26
	FileMailslotSetInformation,     // 27
	FileCompressionInformation,     // 28
	FileObjectIdInformation,        // 29
	FileCompletionInformation,      // 30
	FileMoveClusterInformation,     // 31
	FileQuotaInformation,           // 32
	FileReparsePointInformation,    // 33
	FileNetworkOpenInformation,     // 34
	FileAttributeTagInformation,    // 35
	FileTrackingInformation,        // 36
	FileIdBothDirectoryInformation, // 37
	FileIdFullDirectoryInformation, // 38
	FileValidDataLengthInformation, // 39
	FileShortNameInformation,       // 40
	FileIoCompletionNotificationInformation, // 41
	FileIoStatusBlockRangeInformation,       // 42
	FileIoPriorityHintInformation,           // 43
	FileSfioReserveInformation,              // 44
	FileSfioVolumeInformation,               // 45
	FileHardLinkInformation,                 // 46
	FileProcessIdsUsingFileInformation,      // 47
	FileNormalizedNameInformation,           // 48
	FileNetworkPhysicalNameInformation,      // 49
	FileIdGlobalTxDirectoryInformation,      // 50
	FileMaximumInformation
} FILE_INFORMATION_CLASS;

// 使う物だけ用意
#define NP2_IRP_MJ_CREATE					0x00
#define NP2_IRP_MJ_CLOSE					0x02
#define NP2_IRP_MJ_READ						0x03
#define NP2_IRP_MJ_WRITE					0x04
#define NP2_IRP_MJ_QUERY_INFORMATION		0x05
#define NP2_IRP_MJ_SET_INFORMATION			0x06
#define NP2_IRP_MJ_QUERY_VOLUME_INFORMATION	0x0a
#define NP2_IRP_MJ_DIRECTORY_CONTROL		0x0c
#define NP2_IRP_MJ_FILE_SYSTEM_CONTROL		0x0d
#define NP2_IRP_MJ_DEVICE_CONTROL			0x0e
#define NP2_IRP_MJ_LOCK_CONTROL				0x11
#define NP2_IRP_MJ_CLEANUP					0x12
#define NP2_IRP_MJ_FLUSH_BUFFERS			0x09

#define NP2_IRP_MN_QUERY_DIRECTORY			0x01
#define NP2_IRP_MN_NOTIFY_CHANGE_DIRECTORY	0x02
#define NP2_IRP_MN_USER_FS_REQUEST			0x00

#define NP2_STATUS_SUCCESS		            0x00000000L
#define NP2_STATUS_ACCESS_DENIED            0xC0000022L
#define NP2_STATUS_INVALID_PARAMETER        0xC000000DL
#define NP2_STATUS_OBJECT_NAME_INVALID      0xC0000033L
#define NP2_STATUS_OBJECT_NAME_NOT_FOUND    0xC0000034L
#define NP2_STATUS_OBJECT_NAME_COLLISION    0xC0000035L
#define NP2_STATUS_OBJECT_PATH_NOT_FOUND    0xC000003AL
#define NP2_STATUS_FILE_IS_A_DIRECTORY      0xC00000BAL
#define NP2_STATUS_NOT_A_DIRECTORY          0xC0000103L
#define NP2_STATUS_SHARING_VIOLATION        0xC0000043L
#define NP2_STATUS_CANNOT_DELETE            0xC0000121L
#define NP2_STATUS_TOO_MANY_OPENED_FILES    0xC000011FL
#define NP2_STATUS_BUFFER_TOO_SMALL			0xC0000023L
#define NP2_STATUS_BUFFER_OVERFLOW			0x80000005L
#define NP2_STATUS_INVALID_DEVICE_REQUEST	0xC0000010L
#define NP2_STATUS_NO_MORE_FILES			0x80000006L
#define NP2_STATUS_NOT_SAME_DEVICE			0xC00000D4L
#define NP2_STATUS_NOT_IMPLEMENTED			0xC0000002L
#define NP2_STATUS_END_OF_FILE				0xC0000011L
#define NP2_STATUS_NOT_SUPPORTED			0xC00000BBL
#define NP2_STATUS_MEDIA_WRITE_PROTECTED	0xC00000A2L
#define NP2_STATUS_PENDING                  0x00000103L
#define NP2_STATUS_INSUFFICIENT_RESOURCES   0xC000009AL
#define NP2_STATUS_NOTIFY_ENUM_DIR          0x0000010CL
#define NP2_STATUS_DIRECTORY_NOT_EMPTY		0xC0000101L

#define NP2_FILE_USE_FILE_POINTER_POSITION	0xfffffffe

#define NP2_FO_SYNCHRONOUS_IO				0x00000002


#define NP2HOSTDRVNT_VOLUMELABEL	L"HOSTDRV"
#define NP2HOSTDRVNT_FILESYSTEM		L"HOSTFS"

#define NP2_FILE_SUPERSEDE                  0x00000000
#define NP2_FILE_OPEN                       0x00000001
#define NP2_FILE_CREATE                     0x00000002
#define NP2_FILE_OPEN_IF                    0x00000003
#define NP2_FILE_OVERWRITE                  0x00000004
#define NP2_FILE_OVERWRITE_IF               0x00000005
#define NP2_FILE_MAXIMUM_DISPOSITION        0x00000005

#define NP2_FILE_SUPERSEDED                 0x00000000
#define NP2_FILE_OPENED                     0x00000001
#define NP2_FILE_CREATED                    0x00000002
#define NP2_FILE_OVERWRITTEN                0x00000003
#define NP2_FILE_EXISTS                     0x00000004
#define NP2_FILE_DOES_NOT_EXIST             0x00000005

#define NP2_FILE_DIRECTORY_FILE                     0x00000001
#define NP2_FILE_WRITE_THROUGH                      0x00000002
#define NP2_FILE_SEQUENTIAL_ONLY                    0x00000004
#define NP2_FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define NP2_FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define NP2_FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define NP2_FILE_NON_DIRECTORY_FILE                 0x00000040
#define NP2_FILE_CREATE_TREE_CONNECTION             0x00000080

#define NP2_FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define NP2_FILE_NO_EA_KNOWLEDGE                    0x00000200
#define NP2_FILE_OPEN_REMOTE_INSTANCE               0x00000400
#define NP2_FILE_RANDOM_ACCESS                      0x00000800

#define NP2_FILE_DELETE_ON_CLOSE                    0x00001000
#define NP2_FILE_OPEN_BY_FILE_ID                    0x00002000
#define NP2_FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define NP2_FILE_NO_COMPRESSION                     0x00008000

#define NP2_FILE_ACTION_ADDED					0x00000001
#define NP2_FILE_ACTION_REMOVED					0x00000002
#define NP2_FILE_ACTION_MODIFIED				0x00000003
#define NP2_FILE_ACTION_RENAMED_OLD_NAME		0x00000004
#define NP2_FILE_ACTION_RENAMED_NEW_NAME		0x00000005
#define NP2_FILE_ACTION_ADDED_STREAM			0x00000006
#define NP2_FILE_ACTION_REMOVED_STREAM			0x00000007
#define NP2_FILE_ACTION_MODIFIED_STREAM			0x00000008
#define NP2_FILE_ACTION_REMOVED_BY_DELETE		0x00000009
#define NP2_FILE_ACTION_ID_NOT_TUNNELLED		0x0000000A
#define NP2_FILE_ACTION_TUNNELLED_ID_COLLISION	0x0000000B

#define NP2_FILE_NOTIFY_CHANGE_FILE_NAME    0x00000001   
#define NP2_FILE_NOTIFY_CHANGE_DIR_NAME     0x00000002   
#define NP2_FILE_NOTIFY_CHANGE_ATTRIBUTES   0x00000004   
#define NP2_FILE_NOTIFY_CHANGE_SIZE         0x00000008   
#define NP2_FILE_NOTIFY_CHANGE_LAST_WRITE   0x00000010   
#define NP2_FILE_NOTIFY_CHANGE_LAST_ACCESS  0x00000020   
#define NP2_FILE_NOTIFY_CHANGE_CREATION     0x00000040   
#define NP2_FILE_NOTIFY_CHANGE_SECURITY     0x00000100   

#define NP2_SL_FORCE_ACCESS_CHECK           0x01
#define NP2_SL_OPEN_PAGING_FILE             0x02
#define NP2_SL_OPEN_TARGET_DIRECTORY        0x04
#define NP2_SL_STOP_ON_SYMLINK              0x08
#define NP2_SL_CASE_SENSITIVE               0x80

#define NP2_SL_KEY_SPECIFIED                0x01
#define NP2_SL_OVERRIDE_VERIFY_VOLUME       0x02
#define NP2_SL_WRITE_THROUGH                0x04
#define NP2_SL_FT_SEQUENTIAL_WRITE          0x08
#define NP2_SL_FORCE_DIRECT_WRITE           0x10
#define NP2_SL_REALTIME_STREAM              0x20

#define NP2_SL_READ_ACCESS_GRANTED          0x01
#define NP2_SL_WRITE_ACCESS_GRANTED         0x04

#define NP2_SL_FAIL_IMMEDIATELY             0x01
#define NP2_SL_EXCLUSIVE_LOCK               0x02

#define NP2_SL_RESTART_SCAN                 0x01
#define NP2_SL_RETURN_SINGLE_ENTRY          0x02
#define NP2_SL_INDEX_SPECIFIED              0x04

#define NP2_SL_WATCH_TREE                   0x01

#define NP2_SL_ALLOW_RAW_MOUNT              0x01

#define NP2_FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define NP2_FILE_DEVICE_FILE_SYSTEM         0x00000009
#define NP2_FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014

#define NP2_FILE_REMOVABLE_MEDIA            0x00000001
#define NP2_FILE_READ_ONLY_DEVICE           0x00000002
#define NP2_FILE_FLOPPY_DISKETTE            0x00000004
#define NP2_FILE_WRITE_ONCE_MEDIA           0x00000008
#define NP2_FILE_REMOTE_DEVICE              0x00000010
#define NP2_FILE_DEVICE_IS_MOUNTED          0x00000020
#define NP2_FILE_VIRTUAL_VOLUME             0x00000040
#define NP2_FILE_AUTOGENERATED_DEVICE_NAME  0x00000080
#define NP2_FILE_DEVICE_SECURE_OPEN         0x00000100

#pragma pack(push, 4)
typedef struct
{
	UINT16 Length;
	UINT16 MaximumLength;
	UINT32 Buffer;
} NP2_UNICODE_STRING;
typedef struct
{
	UINT32 Flink;
	UINT32 Blink;
} NP2_LIST_ENTRY;
typedef struct
{
	UINT32 Lock;
	SINT32 SignalState;
	NP2_LIST_ENTRY WaitListHead;
} NP2_DISPATCHER_HEADER;
typedef struct
{
	NP2_DISPATCHER_HEADER Header;
} NP2_KEVENT;
typedef struct
{
	UINT8 majorFunction;
	UINT8 minorFunction;
	UINT8 flags;
	UINT8 control;
	union
	{
		struct
		{
			UINT32 length;
			UINT32 key;
			UINT64 byteOffset;
		} read;
		struct
		{
			UINT32 length;
			UINT32 key;
			UINT64 byteOffset;
		} write;
		struct
		{
			UINT32 securityContext;
			UINT32 options;
			UINT16 fileAttributes;
			UINT16 shareAccess;
			UINT32 eaLength;
		} create;
		struct
		{
			ULONG length;
			FS_INFORMATION_CLASS fsInformationClass;
		} queryVolume;
		struct
		{
			UINT32 Length;
			UINT32 FileName;
			UINT32 FileInformationClass;
			UINT32 FileIndex;
		} queryDirectory;
		struct
		{
			UINT32 Length;
			UINT32 FileInformationClass;
		} queryFile;
		struct
		{
			UINT32 OutputBufferLength;
			UINT32 InputBufferLength;
			UINT32 FsControlCode;
			UINT32 Type3InputBuffer;
		} fileSystemControl;
		struct
		{
			UINT32 OutputBufferLength;
			UINT32 InputBufferLength;
			UINT32 IoControlCode;
			UINT32 Type3InputBuffer;
		} deviceIoControl;
		struct
		{
			UINT32 Length;
			FILE_INFORMATION_CLASS FileInformationClass;
			UINT32 FileObject;
			union
			{
				struct
				{
					UINT8 ReplaceIfExists;
					UINT8 AdvanceOnly;
				};
				UINT32 ClusterCount;
				UINT32 DeleteHandle;
			};
		} setFile;
		struct
		{
			ULONG length;
			ULONG completionFilter;
		} notifyDirectory;
		struct
		{
			UINT32 argument1;
			UINT32 argument2;
			UINT32 argument3;
			UINT32 argument4;
		} others;
	} parameters;
	UINT32 deviceObject;
	UINT32 fileObject;
	UINT32 completionRoutine;
	UINT32 context;
} NP2_IO_STACK_LOCATION;
typedef struct
{
	UINT64 volumeCreationTime;
	UINT32 volumeSerialNumber;
	UINT32 volumeLabelLength;
	UINT8 supportsObjects;
	WCHAR volumeLabel[sizeof(NP2HOSTDRVNT_VOLUMELABEL) / sizeof(WCHAR)];
} NP2_FILE_FS_VOLUME_INFORMATION;
typedef struct
{
	UINT32 fileSystemAttributes;
	SINT32  maximumComponentNameLength;
	UINT32 fileSystemNameLength;
	WCHAR fileSystemName[sizeof(NP2HOSTDRVNT_FILESYSTEM) / sizeof(WCHAR)];
} NP2_FILE_FS_ATTRIBUTE_INFORMATION;
typedef struct
{
	UINT64 TotalAllocationUnits;
	UINT64 AvailableAllocationUnits;
	UINT32 SectorsPerAllocationUnit;
	UINT32 BytesPerSector;
} MP2_FILE_FS_SIZE_INFORMATION;
typedef struct
{
	UINT64 TotalAllocationUnits;
	UINT64 CallerAvailableAllocationUnits;
	UINT64 ActualAvailableAllocationUnits;
	UINT32 SectorsPerAllocationUnit;
	UINT32 BytesPerSector;
} MP2_FILE_FS_FULL_SIZE_INFORMATION;
typedef struct
{
	UINT32 DeviceType;
	UINT32 Characteristics;
} NP2_FILE_FS_DEVICE_INFORMATION;

// 使う範囲で定義　OSバージョンが新しくなると新しい項目が増えていたりする
typedef struct
{
	SINT16                                Type;
	SINT16                                Size;
	UINT32                        DeviceObject;
	UINT32                                  Vpb;
	UINT32                                 FsContext;
	UINT32                                 FsContext2;
	UINT32              SectionObjectPointer;
	UINT32                                 PrivateCacheMap;
	UINT32                              FinalStatus;
	UINT32 RelatedFileObject;
	UINT8                               LockOperation;
	UINT8                               DeletePending;
	UINT8                               ReadAccess;
	UINT8                               WriteAccess;
	UINT8                               DeleteAccess;
	UINT8                               SharedRead;
	UINT8                               SharedWrite;
	UINT8                               SharedDelete;
	UINT32                                 Flags;
	NP2_UNICODE_STRING                        FileName;
	UINT64                         CurrentByteOffset;
	// これ以降にもデータあるが、OSバージョンによって無かったりするので、読む範囲は最小限にする
} NP2_FILE_OBJECT;
typedef struct
{
	UINT32 NextEntryOffset;
	UINT32 FileIndex;
	UINT64 CreationTime;
	UINT64 LastAccessTime;
	UINT64 LastWriteTime;
	UINT64 ChangeTime;
	UINT64 EndOfFile;
	UINT64 AllocationSize;
	UINT32 FileAttributes;
	UINT32 FileNameLength;
	UINT32 EaSize;
	SINT8 ShortNameLength;
	WCHAR ShortName[12];
	WCHAR FileName[MAX_PATH];
} NP2_FILE_BOTH_DIR_INFORMATION;
typedef struct
{
	UINT32 NextEntryOffset;
	UINT32 FileIndex;
	UINT64 CreationTime;
	UINT64 LastAccessTime;
	UINT64 LastWriteTime;
	UINT64 ChangeTime;
	UINT64 EndOfFile;
	UINT64 AllocationSize;
	UINT32 FileAttributes;
	UINT32 FileNameLength;
	WCHAR FileName[MAX_PATH];
} NP2_FILE_DIRECTORY_INFORMATION;
typedef struct
{
	UINT64 EndOfFile;
} NP2_FILE_END_OF_FILE_INFORMATION;
typedef struct
{
	UINT32 FileAttributes;
	UINT32 ReparseTag;
} NP2_FILE_ATTRIBUTE_TAG_INFORMATION;
typedef struct
{
	UINT32 NextEntryOffset;
	UINT32 StreamNameLength;
	UINT64 StreamSize;
	UINT64 StreamAllocationSize;
} NP2_FILE_STREAM_INFORMATION;
#pragma pack(pop)
#pragma pack(push, 8)
typedef struct
{
	UINT64 CreationTime;
	UINT64 LastAccessTime;
	UINT64 LastWriteTime;
	UINT64 ChangeTime;
	UINT32 FileAttributes;
} NP2_FILE_BASIC_INFORMATION;
typedef struct
{
	UINT64 AllocationSize;
	UINT64 EndOfFile;
	UINT32 NumberOfLinks;
	UINT8 DeletePending;
	UINT8 Directory;
} NP2_FILE_STANDARD_INFORMATION;
typedef struct
{
	UINT64 IndexNumber;
} NP2_FILE_INTERNAL_INFORMATION;
typedef struct
{
	UINT32 EaSize;
} NP2_FILE_EA_INFORMATION;
typedef struct
{
	UINT32 AccessFlags;
} NP2_FILE_ACCESS_INFORMATION;
typedef struct
{
	UINT64 CurrentByteOffset;
} NP2_FILE_POSITION_INFORMATION;
typedef struct
{
	UINT32 Mode;
} NP2_FILE_MODE_INFORMATION;
typedef struct
{
	UINT32 AlignmentRequirement;
} NP2_FILE_ALIGNMENT_INFORMATION;
typedef struct
{
	UINT32 FileNameLength;
	WCHAR FileName[MAX_PATH];
} NP2_FILE_NAME_INFORMATION_FIXED;
typedef struct
{
	UINT8 DeleteFileOnClose;
} NP2_FILE_DISPOSITION_INFORMATION;
typedef struct
{
	ULONG NextEntryOffset;
	ULONG FileIndex;
	ULONG FileNameLength;
	WCHAR FileName[MAX_PATH];
} NP2_FILE_NAMES_INFORMATION;
typedef struct
{
	UINT8 ReplaceIfExists;
	UINT32 RootDirectory;
	UINT32 FileNameLength;
	//WCHAR FileName[0];
} NP2_FILE_RENAME_INFORMATION;
typedef struct
{
	UINT64 AllocationSize;
} NP2_FILE_ALLOCATION_INFORMATION;
typedef struct
{
	UINT32 NextEntryOffset;
	UINT32 Action;
	UINT32 FileNameLength;
	WCHAR FileName[MAX_PATH];
} NP2_FILE_NOTIFY_INFORMATION;
#pragma pack(pop)
#pragma pack(push, 1)
typedef struct
{
	NP2_FILE_BASIC_INFORMATION BasicInformation;
	NP2_FILE_STANDARD_INFORMATION StandardInformation;
	NP2_FILE_INTERNAL_INFORMATION InternalInformation;
	NP2_FILE_EA_INFORMATION EaInformation;
	NP2_FILE_ACCESS_INFORMATION AccessInformation;
	NP2_FILE_POSITION_INFORMATION PositionInformation;
	NP2_FILE_MODE_INFORMATION ModeInformation;
	NP2_FILE_ALIGNMENT_INFORMATION AlignmentInformation;
	NP2_FILE_NAME_INFORMATION_FIXED NameInformation;
} NP2_FILE_ALL_INFORMATION;
#pragma pack(pop)

typedef struct
{
	NP2_IO_STACK_LOCATION stack; // IoGetCurrentIrpStackLocation(Irp)で取得されるデータ
	UINT32 statusAddr; // Irp->IoStatusへのアドレス
	UINT32 inBufferAddr; // ゲストOS→エミュレータへのバッファ
	UINT32 deviceFlags; // irpSp->DeviceObject->Flagsの値
	UINT32 outBufferAddr; // エミュレータ→ゲストOSへのバッファ
	UINT32 sectionObjectPointerAddr; // irpSp->FileObject->SectionObjectPointerへのアドレス
	UINT32 version; // エミュレータ通信バージョン情報
} HOSTDRVNT_INVOKEINFO;

#endif