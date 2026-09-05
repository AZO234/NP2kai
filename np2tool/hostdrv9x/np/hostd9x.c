/*
 * HOSTD9X.DLL - minimal Windows 9x Network Provider for HOSTD9X
 *
 * Network Provider companion for HOSTD9X.VXD.
 *
 * The local drive letter is discovered at run time.  NPGetConnection and
 * NPGetUniversalName verify the drive by querying its file-system name
 * (HOSTFS), so fixed, automatic, and DOS-HOSTDRV-inherited mappings work
 * without a hard-coded Z: drive.
 *
 * The file system itself is provided by HOSTD9X.VXD.  This DLL only lets
 * MPR/WNet identify the redirected drive and translate it to a UNC name.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnetwk.h>
#ifndef H9XNP_DEBUG_LOG
#define H9XNP_DEBUG_LOG 0
#endif

#if H9XNP_DEBUG_LOG
#include <stdarg.h>
#endif

#ifndef WNNC_SPEC_VERSION
#define WNNC_SPEC_VERSION          0x00000001
#define WNNC_SPEC_VERSION51        0x00050001
#define WNNC_NET_TYPE              0x00000002
#define WNNC_DRIVER_VERSION        0x00000003
#define WNNC_USER                  0x00000004
#define WNNC_CONNECTION            0x00000006
#define WNNC_CON_GETCONNECTIONS    0x00000004
#define WNNC_DIALOG                0x00000008
#define WNNC_ADMIN                 0x00000009
#define WNNC_ENUMERATION           0x0000000b
#define WNNC_START                 0x0000000c
#endif

#ifndef WNNC_NET_RDR2SAMPLE
#define WNNC_NET_RDR2SAMPLE        0x00250000
#endif

#ifndef UNIVERSAL_NAME_INFO_LEVEL
#define UNIVERSAL_NAME_INFO_LEVEL  0x00000001
#endif
#ifndef REMOTE_NAME_INFO_LEVEL
#define REMOTE_NAME_INFO_LEVEL     0x00000002
#endif

#ifndef WN_SUCCESS
#define WN_SUCCESS                 NO_ERROR
#endif
#ifndef WN_NOT_SUPPORTED
#define WN_NOT_SUPPORTED           ERROR_NOT_SUPPORTED
#endif
#ifndef WN_BAD_VALUE
#define WN_BAD_VALUE               ERROR_INVALID_PARAMETER
#endif
#ifndef WN_BAD_LOCALNAME
#define WN_BAD_LOCALNAME           ERROR_BAD_DEVICE
#endif
#ifndef WN_NOT_CONNECTED
#define WN_NOT_CONNECTED           ERROR_NOT_CONNECTED
#endif
#ifndef WN_MORE_DATA
#define WN_MORE_DATA               ERROR_MORE_DATA
#endif

#ifndef WN_NETWORK_CLASS
#define WN_NETWORK_CLASS           0x00000001
#endif

#define H9XNP_PROVIDER_ID          "HOSTD9XNP"
#define H9XNP_PROVIDER_NAME        "NP2 HOSTD9X Network Provider"
#define H9XNP_NET_ID               0x00250000UL
#define H9XNP_CALL_ORDER         0x30000000UL /* REG_BINARY: 00 00 00 30 */
#define H9XNP_ORDER_KEY            "System\\CurrentControlSet\\Control\\NetworkProvider\\Order"
#define H9XNP_SERVICE_KEY          "System\\CurrentControlSet\\Services\\HOSTD9XNP"
#define H9XNP_SERVICE_NP_KEY       "System\\CurrentControlSet\\Services\\HOSTD9XNP\\NetworkProvider"
#define H9XNP_LEGACY_ORDER_VALUE   "ProviderOrder"
#define H9XNP_ORDER_MAX            2048
#define H9XNP_FS_NAME              "HOSTFS"

static HINSTANCE s_instance;

#if H9XNP_DEBUG_LOG
static void h9xnp_log(const char *fmt, ...)
{
    char path[MAX_PATH];
    char line[1024];
    char body[768];
    SYSTEMTIME st;
    HANDLE h;
    DWORD written;
    int n;
    va_list ap;

    if (GetWindowsDirectoryA(path, MAX_PATH) == 0 ||
        lstrlenA(path) + 16 >= MAX_PATH) {
        lstrcpyA(path, "C:\\HOSTD9X.LOG");
    } else {
        if (path[lstrlenA(path) - 1] != '\\')
            lstrcatA(path, "\\");
        lstrcatA(path, "HOSTD9X.LOG");
    }

    va_start(ap, fmt);
    wvsprintfA(body, fmt, ap);
    va_end(ap);

    GetLocalTime(&st);
    n = wsprintfA(line, "%04u-%02u-%02u %02u:%02u:%02u.%03u %s\r\n",
                  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
                  st.wSecond, st.wMilliseconds, body);

    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    SetFilePointer(h, 0, NULL, FILE_END);
    WriteFile(h, line, (DWORD)n, &written, NULL);
    CloseHandle(h);
}

#else
static void h9xnp_log(const char *fmt, ...)
{
    (void)fmt;
}
#endif

static const char s_remote_name[] = "\\\\NP2HOST\\HOSTFS";

/*
 * Windows 95 MPR calls the Network Provider entry points with ANSI strings.
 * Do not use LPWSTR here: interpreting the ANSI local name "Z:" as UTF-16
 * turns the first WORD into 0x3A5A and makes every connection look unknown.
 */
typedef struct _H9X_UNIVERSAL_NAME_INFOA {
    LPSTR lpUniversalName;
} H9X_UNIVERSAL_NAME_INFOA;

typedef struct _H9X_REMOTE_NAME_INFOA {
    LPSTR lpUniversalName;
    LPSTR lpConnectionName;
    LPSTR lpRemainingPath;
} H9X_REMOTE_NAME_INFOA;

static DWORD h9xnp_alen(const char *s)
{
    DWORD n;

    n = 0;
    if (s != NULL) {
        while (s[n] != 0) {
            ++n;
        }
    }
    return n;
}

static void h9xnp_acopy(char *dst, const char *src)
{
    while ((*dst++ = *src++) != 0) {
        /* nothing */
    }
}

static void h9xnp_acopy_n(char *dst, const char *src, DWORD count)
{
    DWORD i;

    for (i = 0; i < count; ++i) {
        dst[i] = src[i];
    }
}

static int h9xnp_ascii_streq_ci(const char *a, const char *b)
{
    char ca;
    char cb;

    if (a == NULL || b == NULL) {
        return 0;
    }
    while (*a != 0 && *b != 0) {
        ca = *a++;
        cb = *b++;
        if (ca >= 'a' && ca <= 'z') {
            ca = (char)(ca - ('a' - 'A'));
        }
        if (cb >= 'a' && cb <= 'z') {
            cb = (char)(cb - ('a' - 'A'));
        }
        if (ca != cb) {
            return 0;
        }
    }
    return (*a == 0 && *b == 0);
}

static int h9xnp_is_hostd9x_drive(const char *name)
{
    char drive;
    char root[4];
    char volume[MAX_PATH];
    char fsname[32];
    DWORD serial;
    DWORD max_component;
    DWORD flags;

    if (name == NULL || name[0] == 0 || name[1] != ':') {
        return 0;
    }
    drive = name[0];
    if (drive >= 'a' && drive <= 'z') {
        drive = (char)(drive - ('a' - 'A'));
    }
    if (drive < 'A' || drive > 'Z') {
        return 0;
    }

    root[0] = drive;
    root[1] = ':';
    root[2] = '\\';
    root[3] = 0;
    volume[0] = 0;
    fsname[0] = 0;

    if (!GetVolumeInformationA(root, volume, sizeof(volume),
                               &serial, &max_component, &flags,
                               fsname, sizeof(fsname))) {
        return 0;
    }
    return h9xnp_ascii_streq_ci(fsname, H9XNP_FS_NAME);
}

static const char *h9xnp_remaining_path(const char *local_path)
{
    const char *p;

    p = local_path + 2;
    if (*p == 0) {
        return p;
    }
    if (*p == '\\' || *p == '/') {
        return p;
    }
    return NULL;
}

DWORD APIENTRY NPGetCaps(DWORD nIndex)
{
    h9xnp_log("NPGetCaps index=0x%08lX", nIndex);
    switch (nIndex) {
    case WNNC_SPEC_VERSION:
        return WNNC_SPEC_VERSION51;

    case WNNC_NET_TYPE:
        /* Private test provider: use Microsoft's redirector sample type. */
        return H9XNP_NET_ID;

    case WNNC_DRIVER_VERSION:
        return 0x00010000;       /* 1.0 */

    case WNNC_CONNECTION:
        return WNNC_CON_GETCONNECTIONS;

    case WNNC_START:
        return 1;                /* provider is already available */

    case WNNC_USER:
    case WNNC_DIALOG:
    case WNNC_ADMIN:
    case WNNC_ENUMERATION:
    default:
        return 0;
    }
}

DWORD APIENTRY NPGetConnection(LPSTR lpLocalName,
                               LPSTR lpRemoteName,
                               LPDWORD lpnBufferLen)
{
    DWORD need;

    h9xnp_log("NPGetConnection local=\"%s\" buflen=%lu",
              lpLocalName ? lpLocalName : "(null)",
              lpnBufferLen ? *lpnBufferLen : 0);

    if (lpnBufferLen == NULL) {
        return WN_BAD_VALUE;
    }
    if (!h9xnp_is_hostd9x_drive(lpLocalName) || lpLocalName[2] != 0) {
        h9xnp_log("NPGetConnection -> WN_NOT_CONNECTED");
        return WN_NOT_CONNECTED;
    }

    need = h9xnp_alen(s_remote_name) + 1;  /* bytes/chars including NUL */
    if (lpRemoteName == NULL || *lpnBufferLen < need) {
        *lpnBufferLen = need;
        h9xnp_log("NPGetConnection -> WN_MORE_DATA need=%lu", need);
        return WN_MORE_DATA;
    }

    h9xnp_acopy(lpRemoteName, s_remote_name);
    *lpnBufferLen = need;
    h9xnp_log("NPGetConnection -> WN_SUCCESS remote=\\\\NP2HOST\\HOSTFS");
    return WN_SUCCESS;
}

DWORD APIENTRY NPGetUniversalName(LPSTR lpLocalPath,
                                  DWORD dwInfoLevel,
                                  LPVOID lpBuffer,
                                  LPDWORD lpBufferSize)
{
    const char *remaining;
    DWORD remote_len;
    DWORD remaining_len;
    DWORD universal_len;
    DWORD need;
    BYTE *base;
    char *text;

    h9xnp_log("NPGetUniversalName path=\"%s\" level=%lu size=%lu",
              lpLocalPath ? lpLocalPath : "(null)",
              dwInfoLevel, lpBufferSize ? *lpBufferSize : 0);

    if (lpBufferSize == NULL) {
        return WN_BAD_VALUE;
    }
    if (!h9xnp_is_hostd9x_drive(lpLocalPath)) {
        return WN_NOT_CONNECTED;
    }

    remaining = h9xnp_remaining_path(lpLocalPath);
    if (remaining == NULL) {
        return WN_BAD_LOCALNAME;
    }

    remote_len = h9xnp_alen(s_remote_name);
    remaining_len = h9xnp_alen(remaining);
    universal_len = remote_len + remaining_len;

    if (dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL) {
        H9X_UNIVERSAL_NAME_INFOA *info;

        need = sizeof(H9X_UNIVERSAL_NAME_INFOA) + universal_len + 1;
        if (lpBuffer == NULL || *lpBufferSize < need) {
            *lpBufferSize = need;
            return WN_MORE_DATA;
        }

        base = (BYTE *)lpBuffer;
        info = (H9X_UNIVERSAL_NAME_INFOA *)base;
        text = (char *)(base + sizeof(H9X_UNIVERSAL_NAME_INFOA));

        info->lpUniversalName = text;
        h9xnp_acopy_n(text, s_remote_name, remote_len);
        h9xnp_acopy(text + remote_len, remaining);

        *lpBufferSize = need;
        h9xnp_log("NPGetUniversalName -> WN_SUCCESS universal=\"%s\"", text);
        return WN_SUCCESS;
    }

    if (dwInfoLevel == REMOTE_NAME_INFO_LEVEL) {
        H9X_REMOTE_NAME_INFOA *info;
        char *universal;
        char *connection;
        char *rest;

        need = sizeof(H9X_REMOTE_NAME_INFOA) +
               (universal_len + 1) +
               (remote_len + 1) +
               (remaining_len + 1);
        if (lpBuffer == NULL || *lpBufferSize < need) {
            *lpBufferSize = need;
            return WN_MORE_DATA;
        }

        base = (BYTE *)lpBuffer;
        info = (H9X_REMOTE_NAME_INFOA *)base;
        text = (char *)(base + sizeof(H9X_REMOTE_NAME_INFOA));

        universal = text;
        connection = universal + universal_len + 1;
        rest = connection + remote_len + 1;

        info->lpUniversalName = universal;
        info->lpConnectionName = connection;
        info->lpRemainingPath = rest;

        h9xnp_acopy_n(universal, s_remote_name, remote_len);
        h9xnp_acopy(universal + remote_len, remaining);
        h9xnp_acopy(connection, s_remote_name);
        h9xnp_acopy(rest, remaining);

        *lpBufferSize = need;
        h9xnp_log("NPGetUniversalName -> WN_SUCCESS universal=\"%s\"", universal);
        return WN_SUCCESS;
    }

    return WN_BAD_VALUE;
}

static int h9xnp_ascii_equal_ci(const char *a, DWORD alen, const char *b)
{
    DWORD i;
    char ca;
    char cb;

    for (i = 0; i < alen; ++i) {
        if (b[i] == 0) {
            return 0;
        }
        ca = a[i];
        cb = b[i];
        if (ca >= 'a' && ca <= 'z') {
            ca = (char)(ca - ('a' - 'A'));
        }
        if (cb >= 'a' && cb <= 'z') {
            cb = (char)(cb - ('a' - 'A'));
        }
        if (ca != cb) {
            return 0;
        }
    }
    return b[alen] == 0;
}

/*
 * Windows 95 MPR provider order format
 * ------------------------------------
 * Windows 95 network client INFs register providers as individual value
 * names under:
 *
 *   HKLM\System\CurrentControlSet\Control\NetworkProvider\Order
 *
 * For this provider the value is:
 *
 *   HOSTD9XNP=""
 *
 * This is the registration form used by the Win95 providers tested here.
 */
static LONG h9xnp_register_win95_order(void)
{
    HKEY key;
    LONG err;
    static const char empty[] = "";

    err = RegCreateKeyExA(HKEY_LOCAL_MACHINE, H9XNP_ORDER_KEY, 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,
                          NULL, &key, NULL);
    if (err != ERROR_SUCCESS) {
        return err;
    }

    err = RegSetValueExA(key, H9XNP_PROVIDER_ID, 0, REG_SZ,
                         (const BYTE *)empty, sizeof(empty));
    RegCloseKey(key);
    return err;
}

static LONG h9xnp_unregister_win95_order(void)
{
    HKEY key;
    LONG err;

    err = RegOpenKeyExA(HKEY_LOCAL_MACHINE, H9XNP_ORDER_KEY, 0,
                        KEY_SET_VALUE, &key);
    if (err != ERROR_SUCCESS) {
        return err;
    }

    err = RegDeleteValueA(key, H9XNP_PROVIDER_ID);
    RegCloseKey(key);
    return err;
}

/*
 * Remove HOSTD9XNP from the comma-separated ProviderOrder value if an older
 * test build registered it there.  This is cleanup only; the Win95 build does
 * not add itself to ProviderOrder.
 */
static LONG h9xnp_remove_legacy_provider_order(void)
{
    HKEY key;
    LONG err;
    DWORD type;
    DWORD size;
    char order[H9XNP_ORDER_MAX];
    char result[H9XNP_ORDER_MAX];
    const char *p;
    const char *start;
    const char *end;
    DWORD outlen;
    DWORD toklen;
    int changed;

    err = RegOpenKeyExA(HKEY_LOCAL_MACHINE, H9XNP_ORDER_KEY, 0,
                        KEY_QUERY_VALUE | KEY_SET_VALUE, &key);
    if (err != ERROR_SUCCESS) {
        return err;
    }

    type = REG_SZ;
    size = sizeof(order);
    err = RegQueryValueExA(key, H9XNP_LEGACY_ORDER_VALUE, NULL, &type,
                           (LPBYTE)order, &size);
    if (err == ERROR_FILE_NOT_FOUND) {
        RegCloseKey(key);
        return ERROR_SUCCESS;
    }
    if (err != ERROR_SUCCESS || type != REG_SZ) {
        RegCloseKey(key);
        return err;
    }

    order[sizeof(order) - 1] = 0;
    p = order;
    outlen = 0;
    changed = 0;
    result[0] = 0;

    while (*p != 0) {
        while (*p == ' ' || *p == ',') {
            ++p;
        }
        start = p;
        while (*p != 0 && *p != ',') {
            ++p;
        }
        end = p;
        while (end > start && end[-1] == ' ') {
            --end;
        }
        toklen = (DWORD)(end - start);

        if (toklen != 0 &&
            h9xnp_ascii_equal_ci(start, toklen, H9XNP_PROVIDER_ID)) {
            changed = 1;
        } else if (toklen != 0) {
            if (outlen != 0) {
                result[outlen++] = ',';
            }
            if (outlen + toklen + 1 > sizeof(result)) {
                RegCloseKey(key);
                return ERROR_MORE_DATA;
            }
            CopyMemory(result + outlen, start, toklen);
            outlen += toklen;
            result[outlen] = 0;
        }

        if (*p == ',') {
            ++p;
        }
    }

    if (changed) {
        size = (outlen + 1) * sizeof(char);
        err = RegSetValueExA(key, H9XNP_LEGACY_ORDER_VALUE, 0, REG_SZ,
                             (const BYTE *)result, size);
    } else {
        err = ERROR_SUCCESS;
    }

    RegCloseKey(key);
    return err;
}

HRESULT APIENTRY DllRegisterServer(void)
{
    HKEY provider;
    LONG err;
    char module[MAX_PATH];

    h9xnp_log("DllRegisterServer entered");
    if (GetModuleFileNameA(s_instance, module, sizeof(module)) == 0) {
        return E_FAIL;
    }
    module[sizeof(module) - 1] = 0;

    /*
     * Match the Windows 95 Network Provider registry layout used by actual
     * Win95 network clients: Services\<provider>\NetworkProvider contains
     * Name, ProviderPath, NetID, and CallOrder.  Creating the subkey also creates its parent.
     */
    err = RegCreateKeyExA(HKEY_LOCAL_MACHINE, H9XNP_SERVICE_NP_KEY, 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,
                          NULL, &provider, NULL);
    if (err != ERROR_SUCCESS) {
        return E_FAIL;
    }

    err = RegSetValueExA(provider, "Name", 0, REG_SZ,
                         (const BYTE *)H9XNP_PROVIDER_NAME,
                         sizeof(H9XNP_PROVIDER_NAME));
    if (err == ERROR_SUCCESS) {
        err = RegSetValueExA(provider, "ProviderPath", 0, REG_SZ,
                             (const BYTE *)module,
                             (lstrlenA(module) + 1) * sizeof(char));
    }
    if (err == ERROR_SUCCESS) {
        err = RegSetValueExA(provider, "Description", 0, REG_SZ,
                             (const BYTE *)"NP2 HOSTD9X Provider for Windows 95",
                             sizeof("NP2 HOSTD9X Provider for Windows 95"));
    }
    if (err == ERROR_SUCCESS) {
        DWORD netid = H9XNP_NET_ID;
        err = RegSetValueExA(provider, "NetID", 0, REG_BINARY,
                             (const BYTE *)&netid, sizeof(netid));
    }
    if (err == ERROR_SUCCESS) {
        DWORD callorder = H9XNP_CALL_ORDER;
        err = RegSetValueExA(provider, "CallOrder", 0, REG_BINARY,
                             (const BYTE *)&callorder, sizeof(callorder));
    }
    RegCloseKey(provider);
    if (err != ERROR_SUCCESS) {
        return E_FAIL;
    }

    /* Remove the registration written by the previous NT-style test build. */
    err = h9xnp_remove_legacy_provider_order();
    if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND) {
        return E_FAIL;
    }

    err = h9xnp_register_win95_order();
    if (err != ERROR_SUCCESS) {
        return E_FAIL;
    }

    h9xnp_log("DllRegisterServer -> S_OK");
    return S_OK;
}

HRESULT APIENTRY DllUnregisterServer(void)
{
    LONG err;

    h9xnp_log("DllUnregisterServer entered");
    err = h9xnp_unregister_win95_order();
    if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND) {
        return E_FAIL;
    }

    /* Also clean up an older test build if it was installed previously. */
    err = h9xnp_remove_legacy_provider_order();
    if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND) {
        return E_FAIL;
    }

    RegDeleteKeyA(HKEY_LOCAL_MACHINE, H9XNP_SERVICE_NP_KEY);
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, H9XNP_SERVICE_KEY);
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        s_instance = hinstDLL;
        h9xnp_log("DllMain DLL_PROCESS_ATTACH module=0x%08lX", (DWORD)hinstDLL);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        h9xnp_log("DllMain DLL_PROCESS_DETACH");
    }
    (void)lpvReserved;
    return TRUE;
}
