/*
 * HOSTD9X.VXD
 * Native Windows 95/98/Me IFSMgr remote file-system front end for NP2.
 *
 * The driver owns no host file-system policy and no open-file table.  Every
 * synchronous IFSMgr ioreq is passed to the emulator, which completes it in
 * place.  The only local work is registration and function-vector routing.
 */

#define WANTVXDWRAPS
#include <basedef.h>
#include <vmm.h>
#include <vmmreg.h>
#include <vxdwraps.h>
#include <ifs.h>
#include <winerror.h>

#include "hostdrv9xif.h"
#include "ifsmgrsvc.h"

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

#ifndef BLOCK_SVC_INTS
#define BLOCK_SVC_INTS 0x00000001
#endif

#ifndef REG_SZ
#define REG_SZ 1
#endif
#ifndef REG_DWORD
#define REG_DWORD 4
#endif

#define H9X_DEVICE_MAJOR 1
#define H9X_DEVICE_MINOR 0
#define H9X_REVISION     IFS_REVISION
#define H9X_REMOTE_NAME  "\\\\NP2HOST\\HOSTFS"
#define H9X_USE_DISKDEV  0
#define H9X_USE_RES_UNC  1


#define H9X_REG_PATH "System\\CurrentControlSet\\Services\\VxD\\HOSTD9X"
#define H9X_DRIVE_INHERIT 0UL
#define H9X_DRIVE_AUTO    1UL
#define H9X_DRIVE_FIXED   2UL

/* 52-byte Win9x IFSMgr InitUseAdd layout documented by IFSMGR.INC. */
#pragma pack(push, 1)
typedef struct _H9X_USE_INFO_2 {
    /* The 26-byte USE_INFO_1-compatible prefix is what IFSMgr consumes. */
    char ui2_local[9];
    char ui2_pad_1;
    char *ui2_remote;
    char *ui2_password;
    unsigned short ui2_status;
    short ui2_asg_type;
    unsigned short ui2_refcount;
    unsigned short ui2_usecount;
    unsigned short ui2_res_type;
    /* IFSMgr requires a 52-byte level-2 buffer; the tail is reserved/zero. */
    unsigned char ui2_reserved[24];
} H9X_USE_INFO_2;
#pragma pack(pop)

typedef char H9X_USE_INFO_2_must_be_52_bytes[(sizeof(H9X_USE_INFO_2) == 52) ? 1 : -1];
typedef char H9X_ioreq_must_be_116_bytes[(sizeof(ioreq) == 116) ? 1 : -1];
typedef char H9X_search_entry_must_be_48_bytes[(sizeof(srch_entry) == 48) ? 1 : -1];
typedef char H9X_find_data_must_be_592_bytes[(sizeof(_WIN32_FIND_DATA) == 592) ? 1 : -1];
typedef char H9X_handle_info_must_be_52_bytes[(sizeof(_BY_HANDLE_FILE_INFORMATION) == 52) ? 1 : -1];

static void h9x_zero(void *memory, unsigned int bytes)
{
    unsigned char *p = (unsigned char *)memory;
    while (bytes--) *p++ = 0;
}

typedef struct _H9X_CONFIG {
    unsigned long driveMode;
    char driveLetter;
    unsigned long fallbackToAuto;
    unsigned long useRealCapacity;
    unsigned long fakeTotalMB;
    unsigned long fakeFreeMB;
} H9X_CONFIG;

static int g_provider = -1;
static VMM_SEMAPHORE g_callSemaphore = 0;
static H9X_CALL g_call;
static H9X_CONTROL g_control;
static H9X_CONFIG g_config;
static char g_remoteName[] = H9X_REMOTE_NAME;
static int g_dosSuspended = 0;
static int g_dosCdsHidden = 0;
static int g_earlyHandoffTried = 0;
static unsigned long g_dosDrive = H9X_INVALID_DRIVE;
static char g_mappedDrive = 0;
static unsigned short g_vmmVersion = 0;

static int _cdecl H9X_Read(pioreq pir);
static int _cdecl H9X_Write(pioreq pir);
static int _cdecl H9X_FindNext(pioreq pir);
static int _cdecl H9X_Seek(pioreq pir);
static int _cdecl H9X_Close(pioreq pir);
static int _cdecl H9X_Commit(pioreq pir);
static int _cdecl H9X_FileLocks(pioreq pir);
static int _cdecl H9X_FileTimes(pioreq pir);
static int _cdecl H9X_HandleInfo(pioreq pir);
static int _cdecl H9X_EnumHandle(pioreq pir);
static int _cdecl H9X_FindClose(pioreq pir);
static int _cdecl H9X_Connect(pioreq pir);
static int _cdecl H9X_Delete(pioreq pir);
static int _cdecl H9X_Dir(pioreq pir);
static int _cdecl H9X_FileAttrib(pioreq pir);
static int _cdecl H9X_Flush(pioreq pir);
static int _cdecl H9X_GetDiskInfo(pioreq pir);
static int _cdecl H9X_Open(pioreq pir);
static int _cdecl H9X_Rename(pioreq pir);
static int _cdecl H9X_Search(pioreq pir);
static int _cdecl H9X_Query(pioreq pir);
static int _cdecl H9X_Disconnect(pioreq pir);
static int _cdecl H9X_FindOpen(pioreq pir);
static int _cdecl H9X_Invalid(pioreq pir);

static hndlmisc g_fileMisc = {
    IFS_VERSION,
    H9X_REVISION,
    NUM_HNDLMISC,
    {
        H9X_Seek,
        H9X_Close,
        H9X_Commit,
        H9X_FileLocks,
        H9X_FileTimes,
        H9X_Invalid,
        H9X_HandleInfo,
        H9X_EnumHandle
    }
};

static hndlmisc g_findMisc = {
    IFS_VERSION,
    H9X_REVISION,
    NUM_HNDLMISC,
    {
        H9X_Invalid,
        H9X_FindClose,
        H9X_Invalid,
        H9X_Invalid,
        H9X_Invalid,
        H9X_Invalid,
        H9X_Invalid,
        H9X_Invalid
    }
};

static volfunc g_volumeFunctions = {
    IFS_VERSION,
    H9X_REVISION,
    NUM_VOLFUNC,
    {
        H9X_Delete,
        H9X_Dir,
        H9X_FileAttrib,
        H9X_Flush,
        H9X_GetDiskInfo,
        H9X_Open,
        H9X_Rename,
        H9X_Search,
        H9X_Query,
        H9X_Disconnect,
        H9X_Invalid,
        H9X_Invalid,
        H9X_Invalid,
        H9X_FindOpen,
        H9X_Invalid
    }
};

static void h9x_out8(unsigned short port, unsigned char value)
{
    __asm mov dx, port
    __asm mov al, value
    __asm out dx, al
}

static unsigned char h9x_in8(unsigned short port)
{
    unsigned char value;
    __asm mov dx, port
    __asm in al, dx
    __asm mov value, al
    return value;
}

static unsigned short h9x_get_vmm_version(void)
{
    unsigned long value;
    VxDCall(Get_VMM_Version);
    __asm mov value, eax
    return (unsigned short)value;
}

static int h9x_can_deregister_fsd(void)
{
    if (!g_vmmVersion) g_vmmVersion = h9x_get_vmm_version();
    /* IFSMgr_DeregisterFSD is not present in the original Windows 95 IFSMgr. */
    return g_vmmVersion >= 0x040a;
}

/*
 * Diagnostic channel for checked/debug builds only.
 *
 * HOSTD9X.MK defines DBG=1 for DDKBUILDENV=checked and NDEBUG for free.
 * Keeping the implementation and calls behind H9X_DEBUG_DIAGNOSTICS means a
 * free build contains neither the diagnostic I/O code nor its text strings.
 */
#ifndef H9X_DEBUG_DIAGNOSTICS
#if defined(DBG) && (DBG != 0)
#define H9X_DEBUG_DIAGNOSTICS 1
#else
#define H9X_DEBUG_DIAGNOSTICS 0
#endif
#endif

#if H9X_DEBUG_DIAGNOSTICS
static void h9x_diag(unsigned char tag, unsigned long value)
{
    h9x_out8(H9X_IO_DIAG_TAG, tag);
    h9x_out8(H9X_IO_DIAG_DATA, (unsigned char)value);
    h9x_out8(H9X_IO_DIAG_DATA, (unsigned char)(value >> 8));
    h9x_out8(H9X_IO_DIAG_DATA, (unsigned char)(value >> 16));
    h9x_out8(H9X_IO_DIAG_DATA, (unsigned char)(value >> 24));
}

static void h9x_debug_string(const char *text)
{
    while (*text) {
        h9x_out8(H9X_IO_DIAG_TEXT, (unsigned char)*text);
        text++;
    }
}

#define H9X_DIAG(tag, value)       h9x_diag((tag), (value))
#define H9X_DEBUG_STRING(text)     h9x_debug_string((text))
#else
#define H9X_DIAG(tag, value)       ((void)0)
#define H9X_DEBUG_STRING(text)     ((void)0)
#endif

static int h9x_present(void)
{
    unsigned char a = h9x_in8(H9X_IO_ADDR);
    unsigned char c = h9x_in8(H9X_IO_CMD);
    H9X_DIAG(0x02, ((unsigned long)c << 8) | a);
    if (a != H9X_PROBE_ADDR || c != H9X_PROBE_CMD) {
        H9X_DIAG(0x82, ((unsigned long)c << 8) | a);
        H9X_DEBUG_STRING("[HOSTD9X] probe failed\r\n");
        return 0;
    }
    H9X_DIAG(0x03, ((unsigned long)c << 8) | a);
    H9X_DEBUG_STRING("[HOSTD9X] probe OK\r\n");
    return 1;
}

static void h9x_send_pointer(unsigned long address)
{
    h9x_out8(H9X_IO_ADDR, (unsigned char)(address));
    h9x_out8(H9X_IO_ADDR, (unsigned char)(address >> 8));
    h9x_out8(H9X_IO_ADDR, (unsigned char)(address >> 16));
    h9x_out8(H9X_IO_ADDR, (unsigned char)(address >> 24));
}

static void h9x_invoke_pointer(unsigned long function, unsigned long pointer)
{
    static const char command[] = H9X_COMMAND;
    unsigned int i;
    if (g_callSemaphore) Wait_Semaphore(g_callSemaphore, BLOCK_SVC_INTS);
    g_call.signature = H9X_CALL_SIGNATURE;
    g_call.version = H9X_CALL_VERSION;
    g_call.function = function;
    g_call.ioreq = pointer;
    h9x_send_pointer((unsigned long)&g_call);
    for (i = 0; i < sizeof(command) - 1; i++) h9x_out8(H9X_IO_CMD, command[i]);
    if (g_callSemaphore) Signal_Semaphore(g_callSemaphore);
}

static void h9x_invoke(unsigned long function, pioreq pir)
{
    h9x_invoke_pointer(function, (unsigned long)pir);
}

static int h9x_control(unsigned long function, H9X_CONTROL *control)
{
    if (!control) return ERROR_INVALID_PARAMETER;
    control->size = sizeof(*control);
    control->version = H9X_CONTROL_VERSION;
    control->result = ERROR_INVALID_FUNCTION;
    h9x_invoke_pointer(function, (unsigned long)control);
    return (int)control->result;
}

static int h9x_dispatch(unsigned long function, pioreq pir)
{
    if (!pir) return ERROR_INVALID_PARAMETER;
    pir->ir_error = ERROR_INVALID_FUNCTION;
    h9x_invoke(function, pir);
    return pir->ir_error;
}

static int _cdecl H9X_Invalid(pioreq pir)
{
    if (pir) pir->ir_error = ERROR_INVALID_FUNCTION;
    return ERROR_INVALID_FUNCTION;
}

static int _cdecl H9X_Read(pioreq pir)       { return h9x_dispatch(IFSFN_READ, pir); }
static int _cdecl H9X_Write(pioreq pir)      { return h9x_dispatch(IFSFN_WRITE, pir); }
static int _cdecl H9X_Seek(pioreq pir)       { return h9x_dispatch(IFSFN_SEEK, pir); }
static int _cdecl H9X_Close(pioreq pir)      { return h9x_dispatch(IFSFN_CLOSE, pir); }
static int _cdecl H9X_Commit(pioreq pir)     { return h9x_dispatch(IFSFN_COMMIT, pir); }
static int _cdecl H9X_FileLocks(pioreq pir)  { return h9x_dispatch(IFSFN_FILELOCKS, pir); }
static int _cdecl H9X_FileTimes(pioreq pir)  { return h9x_dispatch(IFSFN_FILETIMES, pir); }
static int _cdecl H9X_HandleInfo(pioreq pir) { return h9x_dispatch(IFSFN_HANDLEINFO, pir); }
static int _cdecl H9X_EnumHandle(pioreq pir) { return h9x_dispatch(IFSFN_ENUMHANDLE, pir); }
static int _cdecl H9X_FindClose(pioreq pir)  { return h9x_dispatch(IFSFN_FINDCLOSE, pir); }
static int _cdecl H9X_Delete(pioreq pir)     { return h9x_dispatch(IFSFN_DELETE, pir); }
static int _cdecl H9X_Dir(pioreq pir)        { return h9x_dispatch(IFSFN_DIR, pir); }
static int _cdecl H9X_FileAttrib(pioreq pir) { return h9x_dispatch(IFSFN_FILEATTRIB, pir); }
static int _cdecl H9X_Flush(pioreq pir)      { return h9x_dispatch(IFSFN_FLUSH, pir); }
static int _cdecl H9X_GetDiskInfo(pioreq pir){ return h9x_dispatch(IFSFN_GETDISKINFO, pir); }
static int _cdecl H9X_Rename(pioreq pir)     { return h9x_dispatch(IFSFN_RENAME, pir); }
static int _cdecl H9X_Search(pioreq pir)     { return h9x_dispatch(IFSFN_SEARCH, pir); }
static int _cdecl H9X_Query(pioreq pir)      { return h9x_dispatch(IFSFN_QUERY, pir); }
static int _cdecl H9X_Disconnect(pioreq pir) { return h9x_dispatch(IFSFN_DISCONNECT, pir); }

static int _cdecl H9X_Connect(pioreq pir)
{
    int error;
    H9X_DIAG(0x20, (unsigned long)pir);
    H9X_DEBUG_STRING("[HOSTD9X] H9X_Connect entered\r\n");
    error = h9x_dispatch(IFSFN_CONNECT, pir);
    H9X_DIAG(0x21, (unsigned long)error);
    if (!error) SetVolumeFunc(pir, &g_volumeFunctions);
    return error;
}

static int _cdecl H9X_Open(pioreq pir)
{
    int error = h9x_dispatch(IFSFN_OPEN, pir);
    if (!error) SetHandleFunc(pir, H9X_Read, H9X_Write, &g_fileMisc);
    return error;
}

static int _cdecl H9X_FindOpen(pioreq pir)
{
    int error = h9x_dispatch(IFSFN_FINDOPEN, pir);
    if (!error) SetHandleFunc(pir, H9X_FindNext, H9X_Invalid, &g_findMisc);
    return error;
}

static int _cdecl H9X_FindNext(pioreq pir)
{
    return h9x_dispatch(IFSFN_FINDNEXT, pir);
}

static void h9x_config_defaults(void)
{
    g_config.driveMode = H9X_DRIVE_INHERIT;
    g_config.driveLetter = 'F';
    g_config.fallbackToAuto = 1;
    g_config.useRealCapacity = 0;
    g_config.fakeTotalMB = H9X_FAKE_TOTAL_MB_DEFAULT;
    g_config.fakeFreeMB = H9X_FAKE_FREE_MB_DEFAULT;
}

static unsigned long h9x_reg_dword(VMMHKEY key, char *name, unsigned long defaultValue)
{
    unsigned long type = 0;
    unsigned long bytes = sizeof(unsigned long);
    unsigned long value = defaultValue;
    if (_RegQueryValueEx(key, name, 0, &type, (unsigned char *)&value, &bytes) == ERROR_SUCCESS &&
        type == REG_DWORD && bytes == sizeof(unsigned long))
        return value;
    return defaultValue;
}

static void h9x_load_registry(void)
{
    VMMHKEY key;
    unsigned long type;
    unsigned long bytes;
    char text[8];

    h9x_config_defaults();
    if (_RegOpenKey(HKEY_LOCAL_MACHINE, H9X_REG_PATH, &key) != ERROR_SUCCESS)
        return;

    g_config.driveMode = h9x_reg_dword(key, "DriveMode", g_config.driveMode);
    g_config.fallbackToAuto = h9x_reg_dword(key, "FallbackToAuto", g_config.fallbackToAuto);
    g_config.useRealCapacity = h9x_reg_dword(key, "UseRealCapacity", g_config.useRealCapacity);
    g_config.fakeTotalMB = h9x_reg_dword(key, "FakeCapacityMB", g_config.fakeTotalMB);
    g_config.fakeFreeMB = h9x_reg_dword(key, "FakeFreeMB", g_config.fakeFreeMB);

    h9x_zero(text, sizeof(text));
    type = 0;
    bytes = sizeof(text) - 1;
    if (_RegQueryValueEx(key, "DriveLetter", 0, &type, (unsigned char *)text, &bytes) == ERROR_SUCCESS &&
        type == REG_SZ) {
        text[sizeof(text) - 1] = 0;
        if (text[0] >= 'a' && text[0] <= 'z') text[0] -= ('a' - 'A');
        if (text[0] >= 'A' && text[0] <= 'Z') g_config.driveLetter = text[0];
    }
    _RegCloseKey(key);

    if (g_config.driveMode > H9X_DRIVE_FIXED) g_config.driveMode = H9X_DRIVE_INHERIT;
    if (!g_config.fakeTotalMB) g_config.fakeTotalMB = H9X_FAKE_TOTAL_MB_DEFAULT;
    if (g_config.fakeFreeMB > g_config.fakeTotalMB) g_config.fakeFreeMB = g_config.fakeTotalMB;
}

static int h9x_set_backend_config(void)
{
    int error;
    h9x_zero(&g_control, sizeof(g_control));
    if (g_config.useRealCapacity) g_control.flags |= H9X_CONTROL_REAL_CAPACITY;
    if (!g_vmmVersion) g_vmmVersion = h9x_get_vmm_version();
    if (g_vmmVersion < 0x040a) g_control.flags |= H9X_CONTROL_WIN95_COMPAT;
    g_control.fakeTotalMB = g_config.fakeTotalMB;
    g_control.fakeFreeMB = g_config.fakeFreeMB;
    error = h9x_control(H9X_CTL_SET_CONFIG, &g_control);
    H9X_DIAG(0x30, (unsigned long)error);
    return error == ERROR_SUCCESS;
}

static int h9x_query_dos(void)
{
    int error;
    h9x_zero(&g_control, sizeof(g_control));
    g_control.drive = H9X_INVALID_DRIVE;
    error = h9x_control(H9X_CTL_QUERY_DOS, &g_control);
    if (error != ERROR_SUCCESS) return -1;
    g_dosDrive = g_control.drive;
    g_dosCdsHidden = (g_control.flags & H9X_CONTROL_DOS_CDS_HIDDEN) != 0;
    return (g_control.flags & H9X_CONTROL_DOS_MOUNTED) != 0;
}

static int h9x_suspend_dos(void)
{
    int error;
    h9x_zero(&g_control, sizeof(g_control));
    g_control.drive = H9X_INVALID_DRIVE;
    error = h9x_control(H9X_CTL_SUSPEND_DOS, &g_control);
    if (error != ERROR_SUCCESS) return 0;
    g_dosDrive = g_control.drive;
    g_dosSuspended = (g_control.flags & H9X_CONTROL_DOS_SUSPENDED) != 0;
    g_dosCdsHidden = (g_control.flags & H9X_CONTROL_DOS_CDS_HIDDEN) != 0;
    return g_dosSuspended;
}

static void h9x_prepare_early_handoff(void)
{
    int dosMounted;

    if (g_earlyHandoffTried) return;
    g_earlyHandoffTried = 1;

    if (!h9x_present()) return;
    dosMounted = h9x_query_dos();
    if (dosMounted <= 0) return;

    if (!h9x_suspend_dos()) {
        H9X_DEBUG_STRING("[HOSTD9X] early DOS HOSTDRV suspend failed\r\n");
        return;
    }
    if (g_dosCdsHidden) {
        H9X_DIAG(0x0b, g_dosDrive);
        H9X_DEBUG_STRING("[HOSTD9X] DOS CDS hidden before IFSMgr init\r\n");
    }
    else {
        /* Old HOSTDRV.COM: INT 2Fh is suspended, but its CDS entry cannot be hidden. */
        H9X_DIAG(0x8c, g_dosDrive);
        H9X_DEBUG_STRING("[HOSTD9X] DOS HOSTDRV lacks CDS hand-off extension\r\n");
    }
}

static void h9x_resume_dos(void)
{
    if (!g_dosSuspended) return;
    h9x_zero(&g_control, sizeof(g_control));
    g_control.drive = H9X_INVALID_DRIVE;
    (void)h9x_control(H9X_CTL_RESUME_DOS, &g_control);
    g_dosSuspended = 0;
    g_dosCdsHidden = 0;
    g_dosDrive = H9X_INVALID_DRIVE;
}

static int h9x_try_map_drive(char drive)
{
    H9X_USE_INFO_2 use;
    int error;

    h9x_zero(&use, sizeof(use));
    use.ui2_local[0] = drive;
    use.ui2_local[1] = ':';
    use.ui2_local[2] = 0;
    use.ui2_remote = g_remoteName;
    use.ui2_password = 0;
    use.ui2_asg_type = H9X_USE_DISKDEV;
    use.ui2_res_type = 0;

    error = h9x_IFSMgr_InitUseAdd(&use, g_provider);
    H9X_DIAG(0x09, ((unsigned long)(unsigned char)drive << 24) | (unsigned long)(error & 0x00ffffff));
    if (!error) {
        g_mappedDrive = drive;
        return 1;
    }
    return 0;
}

static int h9x_auto_map_drive(char skipDrive)
{
    int drive;
    for (drive = 'Z'; drive >= 'D'; drive--) {
        if ((char)drive == skipDrive) continue;
        if (h9x_try_map_drive((char)drive)) return 1;
    }
    return 0;
}

static int h9x_register(void)
{
    unsigned long version;

    if (!g_vmmVersion) g_vmmVersion = h9x_get_vmm_version();
    H9X_DIAG(0x01, (unsigned long)g_provider);
    H9X_DEBUG_STRING("[HOSTD9X] h9x_register enter\r\n");

    if (g_provider != -1) {
        H9X_DIAG(0x11, (unsigned long)g_provider);
        H9X_DEBUG_STRING("[HOSTD9X] already registered\r\n");
        return 1;
    }

    if (!h9x_present()) {
        H9X_DIAG(0x81, 0);
        return 0;
    }

    version = h9x_IFSMgr_GetVersion();
    H9X_DIAG(0x04, version);
    if (version < H9X_IFSMGRVERSION) {
        H9X_DIAG(0x84, version);
        H9X_DEBUG_STRING("[HOSTD9X] IFSMgr version too old\r\n");
        return 0;
    }

    g_callSemaphore = Create_Semaphore(1);
    H9X_DIAG(0x05, (unsigned long)g_callSemaphore);
    if (!g_callSemaphore) {
        H9X_DIAG(0x85, 0);
        H9X_DEBUG_STRING("[HOSTD9X] Create_Semaphore failed\r\n");
        return 0;
    }

    h9x_load_registry();
    if (!h9x_set_backend_config()) {
        H9X_DIAG(0x86, 0);
        H9X_DEBUG_STRING("[HOSTD9X] backend control ABI unavailable\r\n");
        Destroy_Semaphore(g_callSemaphore);
        g_callSemaphore = 0;
        return 0;
    }

    H9X_DIAG(0x06, (unsigned long)H9X_Connect);
    H9X_DEBUG_STRING("[HOSTD9X] calling IFSMgr_RegisterNet\r\n");

    /*
     * Raw IFSMgr_RegisterNet VxD service takes two arguments in the Win98 DDK:
     * the connect entry and the IFSMgr interface version.
     */
    g_provider = h9x_IFSMgr_RegisterNet(H9X_Connect, H9X_NET_ID);

    H9X_DIAG(0x07, (unsigned long)g_provider);
    if (g_provider == -1) {
        H9X_DIAG(0x87, (unsigned long)g_provider);
        H9X_DEBUG_STRING("[HOSTD9X] IFSMgr_RegisterNet failed\r\n");
        Destroy_Semaphore(g_callSemaphore);
        g_callSemaphore = 0;
        return 0;
    }

    H9X_DEBUG_STRING("[HOSTD9X] register success\r\n");
    return 1;
}

static int h9x_map_drive(void)
{
    int dosMounted;
    char inheritedDrive = 0;
    int mapped = 0;

    H9X_DEBUG_STRING("[HOSTD9X] h9x_map_drive enter\r\n");

    if (g_provider == -1) {
        H9X_DIAG(0x88, 0xffffffffUL);
        H9X_DEBUG_STRING("[HOSTD9X] map failed: provider not registered\r\n");
        return 0;
    }
    if (g_mappedDrive) return 1;

    dosMounted = h9x_query_dos();
    if (dosMounted < 0) {
        H9X_DIAG(0x8b, 0);
        H9X_DEBUG_STRING("[HOSTD9X] DOS HOSTDRV query failed\r\n");
        return 0;
    }
    if (dosMounted && g_dosDrive < 26) inheritedDrive = (char)('A' + g_dosDrive);

    /*
     * SYS_CRITICAL_INIT normally suspended the TSR before IFSMgr copied the
     * DOS CDS array.  Keep this late call as a fallback for dynamic loading.
     */
    if (dosMounted && !g_dosSuspended && !h9x_suspend_dos()) {
        H9X_DIAG(0x8a, g_dosDrive);
        H9X_DEBUG_STRING("[HOSTD9X] DOS HOSTDRV suspend failed\r\n");
        return 0;
    }

    H9X_DIAG(0x08, (unsigned long)g_provider);
    if (g_config.driveMode == H9X_DRIVE_FIXED) {
        mapped = h9x_try_map_drive(g_config.driveLetter);
    }
    else if (g_config.driveMode == H9X_DRIVE_AUTO) {
        mapped = h9x_auto_map_drive(0);
    }
    else {
        if (inheritedDrive) {
            mapped = h9x_try_map_drive(inheritedDrive);
            if (!mapped && g_config.fallbackToAuto)
                mapped = h9x_auto_map_drive(inheritedDrive);
        }
        else {
            mapped = h9x_auto_map_drive(0);
        }
    }

    if (!mapped) {
        H9X_DIAG(0x89, 0xffffffffUL);
        H9X_DEBUG_STRING("[HOSTD9X] IFSMgr_InitUseAdd failed\r\n");
        h9x_resume_dos();
        return 0;
    }

    H9X_DIAG(0x0a, (unsigned long)(unsigned char)g_mappedDrive);
    H9X_DEBUG_STRING("[HOSTD9X] mapping success\r\n");
    return 1;
}

static void h9x_finish_exit(void)
{
    g_mappedDrive = 0;
    h9x_resume_dos();
    if (g_callSemaphore) {
        Destroy_Semaphore(g_callSemaphore);
        g_callSemaphore = 0;
    }
}

/*
 * SYSTEM_EXIT runs while IFSMgr itself is being torn down.  Do not call the
 * Win98-only IFSMgr_DeregisterFSD service here: original Win95 has no service
 * ordinal 75h and reports an invalid dynamic-link call if it is invoked.
 */
static void h9x_system_exit(void)
{
    g_provider = -1;
    h9x_finish_exit();
}

/*
 * A true dynamic unload must deregister the provider before our code vanishes.
 * This is supported by Win98/Me.  On Win95 refuse the unload instead of leaving
 * IFSMgr with function pointers into an unloaded VxD.
 */
static int h9x_dynamic_exit(void)
{
    if (g_provider != -1) {
        if (!h9x_can_deregister_fsd()) {
            H9X_DEBUG_STRING("[HOSTD9X] dynamic unload unsupported on Win95\r\n");
            return 0;
        }
        h9x_IFSMgr_DeregisterNet(g_provider, FORCE_LEV_BLAST);
        g_provider = -1;
    }
    h9x_finish_exit();
    return 1;
}

void __declspec(naked) HOSTD9X_Control(void)
{
    __asm {
        cmp eax, SYS_CRITICAL_INIT
        je h9x_control_critical_init

        cmp eax, DEVICE_INIT
        je h9x_control_device_init

        cmp eax, INIT_COMPLETE
        je h9x_control_init_complete

        cmp eax, SYS_DYNAMIC_DEVICE_INIT
        je h9x_control_dynamic_init

        cmp eax, SYS_DYNAMIC_DEVICE_EXIT
        je h9x_control_dynamic_exit

        cmp eax, SYSTEM_EXIT
        je h9x_control_system_exit

        clc
        ret

h9x_control_critical_init:
        /* Hide the DOS TSR CDS before IFSMgr builds its initial drive table. */
        call h9x_prepare_early_handoff
        clc
        ret

h9x_control_device_init:
        /* Register the network FSD only.  Do not map a drive yet. */
        call h9x_register
        test eax, eax
        jz h9x_control_fail
        clc
        ret

h9x_control_init_complete:
        /*
         * All static VxDs have completed DEVICE_INIT at this point, so IFSMgr's
         * drive-management state should be ready for InitUseAdd.
         * Mapping failure is logged, but does not unload the already registered
         * VxD; this is intentional while diagnosing the connection path.
         */
        call h9x_map_drive
        clc
        ret

h9x_control_dynamic_init:
        /* Dynamic load occurs after normal boot initialization. */
        call h9x_register
        test eax, eax
        jz h9x_control_fail
        call h9x_map_drive
        test eax, eax
        jnz h9x_control_dynamic_ok
        /*
         * Win98/Me can deregister and fail the dynamic load cleanly.  Win95
         * cannot safely unload a provider once RegisterNet has succeeded, so
         * keep the VxD resident (without a mapped drive) rather than leave a
         * dangling IFSMgr callback.
         */
        call h9x_dynamic_exit
        test eax, eax
        jz h9x_control_dynamic_ok
        stc
        ret

h9x_control_dynamic_ok:
        clc
        ret

h9x_control_dynamic_exit:
        call h9x_dynamic_exit
        test eax, eax
        jz h9x_control_dynamic_exit_fail
        clc
        ret

h9x_control_dynamic_exit_fail:
        stc
        ret

h9x_control_system_exit:
        call h9x_system_exit
        clc
        ret

h9x_control_fail:
        /* Undo an early DOS hand-off; no provider exists on this path. */
        call h9x_system_exit
        stc
        ret
    }
}

/* The VxD DDB is defined in hostdrv9xvxd.asm.  It must be exported
 * with the undecorated COFF symbol name HOSTD9X_DDB. */
