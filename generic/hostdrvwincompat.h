#ifndef NP2_HOSTDRV_WINCOMPAT_H
#define NP2_HOSTDRV_WINCOMPAT_H

/*
 * Win32 file-API compatibility surface used only by HOSTDRV9X/HOSTDRVNT on
 * non-Windows hosts.  The Windows build continues to call the real Win32 API.
 *
 * Keep this header independent of the GUI backend.  Paths exposed to the
 * caller use Windows separators and UTF-16 (16-bit WCHAR).  The implementation
 * converts them to the native UTF-8 filesystem namespace at the syscall edge.
 */

#if !defined(WIN32) && !defined(_WIN32)

#include <stddef.h>
#include <stdint.h>

#include "codecnv/codecnv.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef _countof
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#endif
#ifndef _tcslen
#define _tcslen(s) strlen((const char *)(s))
#endif
#ifndef _tcsrchr
#define _tcsrchr(s,c) ((TCHAR *)strrchr((const char *)(s), (c)))
#endif
#ifndef _tcscat
#define _tcscat(d,s) strcat((char *)(d), (const char *)(s))
#endif
#ifndef __stdcall
#define __stdcall
#endif

typedef uint32_t DWORD;
typedef uint16_t WORD;
/* Win32 ULONG remains 32-bit even on LP64 Unix hosts. */
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef uint16_t WCHAR;
typedef const WCHAR *LPCWSTR;
typedef WCHAR *LPWSTR;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef unsigned char UCHAR;

struct _HDRVWIN_HANDLE;
typedef struct _HDRVWIN_HANDLE *HANDLE;

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

/* The sources use only ASCII characters in HD_WC(). */
#define HD_WC(c) ((WCHAR)(unsigned char)(c))
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define HD_W(s) u##s
#else
/* GCC/Clang support the u"" extension in the C modes used by NP2kai. */
#define HD_W(s) u##s
#endif

/* Fixed-layout Win32 structures used by the host-drive code. */
typedef struct {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME;

typedef union {
	struct { DWORD LowPart; LONG HighPart; };
	int64_t QuadPart;
} LARGE_INTEGER;

typedef union {
	struct { DWORD LowPart; DWORD HighPart; };
	uint64_t QuadPart;
} ULARGE_INTEGER;

typedef struct {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD dwVolumeSerialNumber;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD nNumberOfLinks;
	DWORD nFileIndexHigh;
	DWORD nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION;

typedef struct {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA;

typedef struct {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	WCHAR cFileName[MAX_PATH];
	WCHAR cAlternateFileName[14];
} WIN32_FIND_DATAW;
typedef WIN32_FIND_DATAW WIN32_FIND_DATA;

/* Code pages used by the original implementation. */
#define CP_ACP   0
#define CP_OEMCP 1
#define CP_UTF8  65001

/* Win32 attributes. */
#define FILE_ATTRIBUTE_READONLY      0x00000001UL
#define FILE_ATTRIBUTE_HIDDEN        0x00000002UL
#define FILE_ATTRIBUTE_SYSTEM        0x00000004UL
#define FILE_ATTRIBUTE_DIRECTORY     0x00000010UL
#define FILE_ATTRIBUTE_ARCHIVE       0x00000020UL
#define FILE_ATTRIBUTE_NORMAL        0x00000080UL
#define FILE_ATTRIBUTE_TEMPORARY     0x00000100UL
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400UL
#define INVALID_FILE_ATTRIBUTES      0xffffffffUL

/* Desired access / sharing. */
#define DELETE                 0x00010000UL
#define READ_CONTROL           0x00020000UL
#define GENERIC_READ           0x80000000UL
#define GENERIC_WRITE          0x40000000UL
#define GENERIC_EXECUTE        0x20000000UL
#define FILE_SHARE_READ        0x00000001UL
#define FILE_SHARE_WRITE       0x00000002UL
#define FILE_SHARE_DELETE      0x00000004UL
#define FILE_READ_DATA         0x00000001UL
#define FILE_LIST_DIRECTORY    0x00000001UL
#define FILE_WRITE_DATA        0x00000002UL
#define FILE_ADD_FILE          0x00000002UL
#define FILE_APPEND_DATA       0x00000004UL
#define FILE_ADD_SUBDIRECTORY  0x00000004UL
#define FILE_READ_EA           0x00000008UL
#define FILE_WRITE_EA          0x00000010UL
#define FILE_EXECUTE           0x00000020UL
#define FILE_TRAVERSE          0x00000020UL
#define FILE_DELETE_CHILD      0x00000040UL
#define FILE_READ_ATTRIBUTES   0x00000080UL
#define FILE_WRITE_ATTRIBUTES  0x00000100UL
#define SYNCHRONIZE            0x00100000UL
#define STANDARD_RIGHTS_REQUIRED 0x000f0000UL
#define STANDARD_RIGHTS_ALL      0x001f0000UL
#define FILE_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1ffUL)

/* FSCTL values used only for dispatch comparisons in HOSTDRVNT. */
#define FSCTL_REQUEST_OPLOCK_LEVEL_1 0x00090000UL
#define FSCTL_REQUEST_OPLOCK_LEVEL_2 0x00090004UL
#define FSCTL_REQUEST_BATCH_OPLOCK   0x00090008UL
#define FSCTL_OPLOCK_BREAK_ACKNOWLEDGE 0x0009000cUL
#define FSCTL_OPBATCH_ACK_CLOSE_PENDING 0x00090010UL
#define FSCTL_OPLOCK_BREAK_NOTIFY    0x00090014UL
#define FSCTL_LOCK_VOLUME            0x00090018UL
#define FSCTL_UNLOCK_VOLUME          0x0009001cUL
#define FSCTL_DISMOUNT_VOLUME        0x00090020UL
#define FSCTL_IS_VOLUME_MOUNTED      0x00090028UL
#define FSCTL_IS_PATHNAME_VALID      0x0009002cUL
#define FSCTL_MARK_VOLUME_DIRTY      0x00090030UL
#define FSCTL_QUERY_RETRIEVAL_POINTERS 0x0009003bUL
#define FSCTL_GET_COMPRESSION        0x0009003cUL
#define FSCTL_SET_COMPRESSION        0x0009c040UL
#define FSCTL_MARK_AS_SYSTEM_HIVE    0x0009004fUL
#define FSCTL_OPLOCK_BREAK_ACK_NO_2  0x00090050UL
#define FSCTL_INVALIDATE_VOLUMES     0x00090054UL
#define FSCTL_REQUEST_FILTER_OPLOCK  0x0009008cUL

/* Creation disposition. */
#define CREATE_NEW        1
#define CREATE_ALWAYS     2
#define OPEN_EXISTING     3
#define OPEN_ALWAYS       4
#define TRUNCATE_EXISTING 5

#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000UL

#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2
#define INVALID_SET_FILE_POINTER 0xffffffffUL

#define MOVEFILE_REPLACE_EXISTING 0x00000001UL
#define MOVEFILE_COPY_ALLOWED     0x00000002UL

#define GetFileExInfoStandard 0

/* Win32 errors required by HOSTDRV. */
#define ERROR_SUCCESS               0
#define NO_ERROR                    0
#define ERROR_INVALID_FUNCTION      1
#define ERROR_FILE_NOT_FOUND        2
#define ERROR_PATH_NOT_FOUND        3
#define ERROR_TOO_MANY_OPEN_FILES   4
#define ERROR_ACCESS_DENIED         5
#define ERROR_INVALID_HANDLE        6
#define ERROR_NOT_ENOUGH_MEMORY     8
#define ERROR_INVALID_DATA          13
#define ERROR_OUTOFMEMORY           14
#define ERROR_INVALID_DRIVE         15
#define ERROR_NOT_READY             21
#define ERROR_WRITE_PROTECT         19
#define ERROR_NOT_SUPPORTED         50
#define ERROR_FILE_EXISTS           80
#define ERROR_INVALID_PARAMETER     87
#define ERROR_DISK_FULL             112
#define ERROR_INVALID_NAME          123
#define ERROR_DIR_NOT_EMPTY         145
#define ERROR_BAD_NETPATH           53
#define ERROR_BAD_NET_NAME          67
#define ERROR_ALREADY_EXISTS        183
#define ERROR_FILENAME_EXCED_RANGE  206
#define ERROR_NO_MORE_FILES         18
#define ERROR_LOCK_VIOLATION        33
#define ERROR_SHARING_VIOLATION     32
#define ERROR_HANDLE_EOF            38
#define ERROR_CANNOT_MAKE           82
#define ERROR_NOT_SAME_DEVICE       17

/* FILESYSTEM attribute bits returned to the guest by HOSTDRVNT. */
#ifndef FILE_CASE_SENSITIVE_SEARCH
#define FILE_CASE_SENSITIVE_SEARCH 0x00000001UL
#endif
#ifndef FILE_CASE_PRESERVED_NAMES
#define FILE_CASE_PRESERVED_NAMES  0x00000002UL
#endif
#ifndef FILE_UNICODE_ON_DISK
#define FILE_UNICODE_ON_DISK       0x00000004UL
#endif

/* Utility macros matching the generic Windows API names used by hostdrvnt.c. */
#define FindFirstFile        FindFirstFileW
#define FindNextFile         FindNextFileW
#define GetFileAttributesEx  GetFileAttributesExW
#define CreateDirectory      CreateDirectoryW
#define GetDiskFreeSpaceEx   GetDiskFreeSpaceExW

/* UTF-16 helpers.  Deliberately ASCII-case-insensitive, matching the DOS/Win
 * names relevant to this layer without depending on host wchar_t size. */
size_t hd_wcslen(const WCHAR *s);
WCHAR *hd_wcscpy(WCHAR *d, const WCHAR *s);
WCHAR *hd_wcsncpy(WCHAR *d, const WCHAR *s, size_t n);
WCHAR *hd_wcscat(WCHAR *d, const WCHAR *s);
int hd_wcscmp(const WCHAR *a, const WCHAR *b);
int hd_wcsncmp(const WCHAR *a, const WCHAR *b, size_t n);
int hd_wcsicmp(const WCHAR *a, const WCHAR *b);
int hd_wcsnicmp(const WCHAR *a, const WCHAR *b, size_t n);
WCHAR *hd_wcschr(const WCHAR *s, WCHAR c);
WCHAR *hd_wcsrchr(const WCHAR *s, WCHAR c);

#define wcslen   hd_wcslen
#define wcscpy   hd_wcscpy
#define wcsncpy  hd_wcsncpy
#define wcscat   hd_wcscat
#define wcscmp   hd_wcscmp
#define wcsncmp  hd_wcsncmp
#define _wcsicmp hd_wcsicmp
#define _wcsnicmp hd_wcsnicmp
#define wcsicmp  hd_wcsicmp
#define wcsnicmp hd_wcsnicmp
#define wcschr   hd_wcschr
#define wcsrchr  hd_wcsrchr

DWORD GetLastError(void);
void SetLastError(DWORD error);

int MultiByteToWideChar(UINT codePage, DWORD flags, const char *src, int srcLen,
	WCHAR *dst, int dstChars);
int WideCharToMultiByte(UINT codePage, DWORD flags, const WCHAR *src, int srcLen,
	char *dst, int dstBytes, const char *defaultChar, BOOL *usedDefaultChar);

BOOL PathIsRelativeA(const char *path);
#define PathIsRelative PathIsRelativeA
DWORD GetFullPathNameA(const char *path, DWORD size, char *out, char **filePart);
#define GetFullPathName GetFullPathNameA
BOOL PathCombineW(WCHAR *out, const WCHAR *dir, const WCHAR *file);
BOOL PathCanonicalizeW(WCHAR *out, const WCHAR *path);
BOOL PathRemoveFileSpecW(WCHAR *path);
BOOL PathMatchSpecW(const WCHAR *name, const WCHAR *pattern);

DWORD GetFileAttributesW(const WCHAR *path);
BOOL GetFileAttributesExW(const WCHAR *path, int infoLevel, WIN32_FILE_ATTRIBUTE_DATA *data);
BOOL SetFileAttributesW(const WCHAR *path, DWORD attributes);
BOOL CreateDirectoryW(const WCHAR *path, void *securityAttributes);
BOOL RemoveDirectoryW(const WCHAR *path);
BOOL DeleteFileW(const WCHAR *path);
BOOL MoveFileW(const WCHAR *oldPath, const WCHAR *newPath);
BOOL MoveFileExW(const WCHAR *oldPath, const WCHAR *newPath, DWORD flags);

HANDLE CreateFileW(const WCHAR *path, DWORD desiredAccess, DWORD shareMode,
	void *securityAttributes, DWORD creationDisposition, DWORD flagsAndAttributes,
	HANDLE templateFile);
BOOL CloseHandle(HANDLE handle);
BOOL ReadFile(HANDLE handle, void *buffer, DWORD bytesToRead, DWORD *bytesRead, void *overlapped);
BOOL WriteFile(HANDLE handle, const void *buffer, DWORD bytesToWrite, DWORD *bytesWritten, void *overlapped);
DWORD SetFilePointer(HANDLE handle, LONG distanceLow, LONG *distanceHigh, DWORD moveMethod);
BOOL SetFilePointerEx(HANDLE handle, LARGE_INTEGER distance, LARGE_INTEGER *newPosition, DWORD moveMethod);
BOOL SetEndOfFile(HANDLE handle);
BOOL FlushFileBuffers(HANDLE handle);
BOOL LockFile(HANDLE handle, DWORD offsetLow, DWORD offsetHigh, DWORD bytesLow, DWORD bytesHigh);
BOOL UnlockFile(HANDLE handle, DWORD offsetLow, DWORD offsetHigh, DWORD bytesLow, DWORD bytesHigh);
BOOL GetFileInformationByHandle(HANDLE handle, BY_HANDLE_FILE_INFORMATION *info);
BOOL GetFileSizeEx(HANDLE handle, LARGE_INTEGER *size);
BOOL GetFileTime(HANDLE handle, FILETIME *creation, FILETIME *access, FILETIME *write);
BOOL SetFileTime(HANDLE handle, const FILETIME *creation, const FILETIME *access, const FILETIME *write);

HANDLE FindFirstFileW(const WCHAR *pattern, WIN32_FIND_DATAW *data);
BOOL FindNextFileW(HANDLE find, WIN32_FIND_DATAW *data);
BOOL FindClose(HANDLE find);

BOOL GetDiskFreeSpaceW(const WCHAR *root, DWORD *sectorsPerCluster, DWORD *bytesPerSector,
	DWORD *freeClusters, DWORD *totalClusters);
BOOL GetDiskFreeSpaceExW(const WCHAR *root, ULARGE_INTEGER *freeBytesAvailable,
	ULARGE_INTEGER *totalNumberOfBytes, ULARGE_INTEGER *totalNumberOfFreeBytes);

BOOL FileTimeToDosDateTime(const FILETIME *ft, WORD *dosDate, WORD *dosTime);
BOOL DosDateTimeToFileTime(WORD dosDate, WORD dosTime, FILETIME *ft);
BOOL FileTimeToLocalFileTime(const FILETIME *ft, FILETIME *localft);
BOOL LocalFileTimeToFileTime(const FILETIME *localft, FILETIME *ft);

#endif /* !WIN32 */
#endif /* NP2_HOSTDRV_WINCOMPAT_H */
