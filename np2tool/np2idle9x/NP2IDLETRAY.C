/* NP2IDLETRAY.C
 *
 * Windows 95/98/Me task tray controller for NP2IDLE.VXD.
 */

#define WIN32_LEAN_AND_MEAN
#ifndef WINVER
#define WINVER 0x0400
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>

#ifndef NIM_ADD
#define NIM_ADD        0x00000000
#define NIM_MODIFY     0x00000001
#define NIM_DELETE     0x00000002
#endif

#ifndef NIF_MESSAGE
#define NIF_MESSAGE    0x00000001
#define NIF_ICON       0x00000002
#define NIF_TIP        0x00000004
#endif

#ifdef __cplusplus
extern "C" {
#endif
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PVOID lpData);
#ifdef __cplusplus
}
#endif

#include "NP2IDLE.H"
#include "RESOURCE.H"
#include "VERSION.H"

#define APP_CLASS_NAME       "NP2IDLETrayWindow"
#define APP_WINDOW_NAME      "NP2IDLE Tray Controller"
#define APP_MUTEX_NAME       "NP2IDLETrayControllerMutex"

#define WM_TRAYICON          (WM_USER + 10)
#define TRAY_ICON_ID         1
#define TRAY_RETRY_TIMER_ID  1
#define TRAY_RETRY_INTERVAL  1000
#define TRAY_RETRY_LIMIT     10

typedef struct tagNP2_NOTIFYICONDATAA_V1 {
    DWORD cbSize;
    HWND  hWnd;
    UINT  uID;
    UINT  uFlags;
    UINT  uCallbackMessage;
    HICON hIcon;
    CHAR  szTip[64];
} NP2_NOTIFYICONDATAA_V1;

static HICON     g_icon;
static BOOL      g_icon_owned;
static BOOL      g_icon_added;
static UINT      g_taskbar_created;
static UINT      g_tray_retry_count;
static HANDLE    g_mutex;

static HANDLE open_np2idle(void)
{
    HANDLE h;

    h = CreateFileA(NP2IDLE_DEVICE_NAME,
                    0,
                    0,
                    NULL,
                    0,
                    0,
                    NULL);

    if (h == INVALID_HANDLE_VALUE) {
        h = CreateFileA(NP2IDLE_DEVICE_NAME,
                        0,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
    }

    return h;
}

static BOOL query_np2idle_state(DWORD *state, DWORD *error_code)
{
    HANDLE h;
    DWORD returned;
    DWORD value;
    BOOL ok;

    if (state != NULL) {
        *state = NP2IDLE_STATE_OFF;
    }
    if (error_code != NULL) {
        *error_code = ERROR_SUCCESS;
    }

    h = open_np2idle();
    if (h == INVALID_HANDLE_VALUE) {
        if (error_code != NULL) {
            *error_code = GetLastError();
        }
        return FALSE;
    }

    returned = 0;
    value = NP2IDLE_STATE_OFF;
    ok = DeviceIoControl(h,
                         NP2IDLE_DIOC_GETSTATE,
                         NULL,
                         0,
                         &value,
                         sizeof(value),
                         &returned,
                         NULL);

    if (!ok) {
        if (error_code != NULL) {
            *error_code = GetLastError();
        }
        CloseHandle(h);
        return FALSE;
    }

    CloseHandle(h);

    if (returned != sizeof(value) ||
        (value != NP2IDLE_STATE_OFF && value != NP2IDLE_STATE_ON)) {
        if (error_code != NULL) {
            *error_code = ERROR_INVALID_DATA;
        }
        return FALSE;
    }

    if (state != NULL) {
        *state = value;
    }
    return TRUE;
}

static BOOL set_np2idle_state(DWORD control_code, DWORD *error_code)
{
    HANDLE h;
    DWORD returned;
    BOOL ok;

    if (error_code != NULL) {
        *error_code = ERROR_SUCCESS;
    }

    h = open_np2idle();
    if (h == INVALID_HANDLE_VALUE) {
        if (error_code != NULL) {
            *error_code = GetLastError();
        }
        return FALSE;
    }

    returned = 0;
    ok = DeviceIoControl(h,
                         control_code,
                         NULL,
                         0,
                         NULL,
                         0,
                         &returned,
                         NULL);

    if (!ok && error_code != NULL) {
        *error_code = GetLastError();
    }

    CloseHandle(h);
    return ok;
}

static void format_system_error(DWORD error_code, CHAR *buffer, DWORD size)
{
    DWORD length;

    if (buffer == NULL || size == 0) {
        return;
    }

    buffer[0] = '\0';
    length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            error_code,
                            0,
                            buffer,
                            size,
                            NULL);

    if (length == 0) {
        wsprintfA(buffer, "Win32 error %lu", (unsigned long)error_code);
        return;
    }

    while (length != 0 &&
           (buffer[length - 1] == '\r' || buffer[length - 1] == '\n')) {
        buffer[--length] = '\0';
    }
}

static void show_vxd_error(HWND hwnd, const CHAR *operation, DWORD error_code)
{
    CHAR system_text[256];
    CHAR message[512];

    format_system_error(error_code, system_text, sizeof(system_text));
    wsprintfA(message,
              "%s failed.\r\n\r\nError %lu: %s\r\n\r\n"
              "Make sure NP2IDLE.VXD is loaded from SYSTEM.INI.",
              operation,
              (unsigned long)error_code,
              system_text);

    MessageBoxA(hwnd,
                message,
                "NP2IDLE",
                MB_OK | MB_ICONEXCLAMATION);
}

static void get_tooltip_text(CHAR *text, DWORD size)
{
    DWORD state;
    DWORD error_code;

    if (query_np2idle_state(&state, &error_code)) {
        lstrcpynA(text,
                  (state == NP2IDLE_STATE_ON) ?
                      "NP2IDLE - ON" : "NP2IDLE - OFF",
                  size);
    } else {
        lstrcpynA(text, "NP2IDLE - VxD unavailable", size);
    }
}

static void initialize_notify_data(HWND hwnd,
                                   NP2_NOTIFYICONDATAA_V1 *nid,
                                   UINT flags)
{
    ZeroMemory(nid, sizeof(*nid));
    nid->cbSize = sizeof(*nid);
    nid->hWnd = hwnd;
    nid->uID = TRAY_ICON_ID;
    nid->uFlags = flags;
    nid->uCallbackMessage = WM_TRAYICON;
    nid->hIcon = g_icon;
    get_tooltip_text(nid->szTip, sizeof(nid->szTip));
}

static BOOL add_tray_icon(HWND hwnd)
{
    NP2_NOTIFYICONDATAA_V1 nid;

    initialize_notify_data(hwnd,
                           &nid,
                           NIF_MESSAGE | NIF_ICON | NIF_TIP);

    if (!Shell_NotifyIconA(NIM_ADD, (PVOID)&nid)) {
        g_icon_added = FALSE;
        return FALSE;
    }

    g_icon_added = TRUE;
    return TRUE;
}

static void update_tray_tooltip(HWND hwnd)
{
    NP2_NOTIFYICONDATAA_V1 nid;

    if (!g_icon_added) {
        return;
    }

    initialize_notify_data(hwnd, &nid, NIF_TIP);
    Shell_NotifyIconA(NIM_MODIFY, (PVOID)&nid);
}

static void remove_tray_icon(HWND hwnd)
{
    NP2_NOTIFYICONDATAA_V1 nid;

    if (!g_icon_added) {
        return;
    }

    initialize_notify_data(hwnd, &nid, 0);
    Shell_NotifyIconA(NIM_DELETE, (PVOID)&nid);
    g_icon_added = FALSE;
}

static void begin_tray_retry(HWND hwnd)
{
    g_tray_retry_count = 0;
    SetTimer(hwnd,
             TRAY_RETRY_TIMER_ID,
             TRAY_RETRY_INTERVAL,
             NULL);
}

static void show_status(HWND hwnd)
{
    DWORD state;
    DWORD error_code;
    CHAR message[160];

    if (!query_np2idle_state(&state, &error_code)) {
        show_vxd_error(hwnd, "Status query", error_code);
        update_tray_tooltip(hwnd);
        return;
    }

    wsprintfA(message,
              "NP2IDLE.VXD is loaded.\r\n\r\nIdle HLT: %s",
              (state == NP2IDLE_STATE_ON) ? "ON" : "OFF");

    MessageBoxA(hwnd,
                message,
                "NP2IDLE Status",
                MB_OK | MB_ICONINFORMATION);
}

static void show_about(HWND hwnd)
{
    CHAR message[400];

    wsprintfA(message,
              "NP2IDLE Tray Controller\r\n"
              "Version %s\r\n\r\n"
              "Controls NP2IDLE.VXD idle HLT handling for "
              "Neko Project 21/W.\r\n\r\n"
              "Windows 95 / 98 / 98SE",
              NP2IDLE_VERSION_STRING);

    MessageBoxA(hwnd,
                message,
                "About NP2IDLE",
                MB_OK | MB_ICONINFORMATION);
}

static void change_state(HWND hwnd, DWORD control_code)
{
    DWORD error_code;
    const CHAR *operation;

    operation = (control_code == NP2IDLE_DIOC_ENABLE) ?
                    "Enable" : "Disable";

    if (!set_np2idle_state(control_code, &error_code)) {
        show_vxd_error(hwnd, operation, error_code);
    }

    update_tray_tooltip(hwnd);
}

static void show_tray_menu(HWND hwnd)
{
    HMENU menu;
    POINT point;
    DWORD state;
    DWORD error_code;
    BOOL available;
    UINT command;
    CHAR status_text[80];

    available = query_np2idle_state(&state, &error_code);

    menu = CreatePopupMenu();
    if (menu == NULL) {
        return;
    }

    if (available) {
        wsprintfA(status_text,
                  "Status: %s...",
                  (state == NP2IDLE_STATE_ON) ? "ON" : "OFF");
    } else {
        lstrcpynA(status_text,
                  "Status: VxD unavailable...",
                  sizeof(status_text));
    }

    AppendMenuA(menu, MF_STRING, IDM_STATUS, status_text);
    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);

    AppendMenuA(menu,
                MF_STRING | (available ? 0 : MF_GRAYED),
                IDM_ENABLE,
                "ON");
    AppendMenuA(menu,
                MF_STRING | (available ? 0 : MF_GRAYED),
                IDM_DISABLE,
                "OFF");

    if (available) {
        CheckMenuRadioItem(menu,
                           IDM_ENABLE,
                           IDM_DISABLE,
                           (state == NP2IDLE_STATE_ON) ?
                               IDM_ENABLE : IDM_DISABLE,
                           MF_BYCOMMAND);
    }

    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu, MF_STRING, IDM_ABOUT, "About...");
    AppendMenuA(menu, MF_STRING, IDM_EXIT, "Exit");

    GetCursorPos(&point);

    /* Required for notification-area context menus to dismiss correctly
     * when the user clicks elsewhere. */
    SetForegroundWindow(hwnd);

    command = TrackPopupMenu(menu,
                             TPM_RIGHTBUTTON |
                             TPM_RETURNCMD |
                             TPM_NONOTIFY,
                             point.x,
                             point.y,
                             0,
                             hwnd,
                             NULL);

    PostMessageA(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);

    if (command != 0) {
        SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(command, 0), 0);
    }
}

static LRESULT CALLBACK tray_window_proc(HWND hwnd,
                                         UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam)
{
    if (g_taskbar_created != 0 && message == g_taskbar_created) {
        g_icon_added = FALSE;
        if (!add_tray_icon(hwnd)) {
            begin_tray_retry(hwnd);
        }
        return 0;
    }

    switch (message) {
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDM_STATUS:
            show_status(hwnd);
            return 0;

        case IDM_ENABLE:
            change_state(hwnd, NP2IDLE_DIOC_ENABLE);
            return 0;

        case IDM_DISABLE:
            change_state(hwnd, NP2IDLE_DIOC_DISABLE);
            return 0;

        case IDM_ABOUT:
            show_about(hwnd);
            return 0;

        case IDM_EXIT:
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_TRAYICON:
        switch ((UINT)lparam) {
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            show_tray_menu(hwnd);
            return 0;

        case WM_LBUTTONDBLCLK:
            show_status(hwnd);
            return 0;
        }
        break;

    case WM_TIMER:
        if (wparam == TRAY_RETRY_TIMER_ID) {
            if (add_tray_icon(hwnd)) {
                KillTimer(hwnd, TRAY_RETRY_TIMER_ID);
                return 0;
            }

            ++g_tray_retry_count;
            if (g_tray_retry_count >= TRAY_RETRY_LIMIT) {
                KillTimer(hwnd, TRAY_RETRY_TIMER_ID);
                MessageBoxA(hwnd,
                            "The task tray icon could not be created.",
                            "NP2IDLE",
                            MB_OK | MB_ICONEXCLAMATION);
                DestroyWindow(hwnd);
            }
            return 0;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, TRAY_RETRY_TIMER_ID);
        remove_tray_icon(hwnd);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance,
                   HINSTANCE previous_instance,
                   LPSTR command_line,
                   int show_command)
{
    WNDCLASSA window_class;
    HWND hwnd;
    MSG message;
    int result;

    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    g_icon_owned = FALSE;
    g_icon_added = FALSE;
    g_tray_retry_count = 0;

    g_mutex = CreateMutexA(NULL, FALSE, APP_MUTEX_NAME);
    if (g_mutex == NULL) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(g_mutex);
        g_mutex = NULL;
        return 0;
    }

    g_icon = (HICON)LoadImageA(instance,
                               MAKEINTRESOURCEA(IDI_NP2IDLE),
                               IMAGE_ICON,
                               GetSystemMetrics(SM_CXSMICON),
                               GetSystemMetrics(SM_CYSMICON),
                               LR_DEFAULTCOLOR);

    if (g_icon != NULL) {
        g_icon_owned = TRUE;
    } else {
        g_icon = LoadIconA(NULL, IDI_APPLICATION);
        g_icon_owned = FALSE;
    }

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.style = 0;
    window_class.lpfnWndProc = tray_window_proc;
    window_class.hInstance = instance;
    window_class.hIcon = g_icon;
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_class.hbrBackground = NULL;
    window_class.lpszClassName = APP_CLASS_NAME;

    if (!RegisterClassA(&window_class)) {
        if (g_icon_owned && g_icon != NULL) {
            DestroyIcon(g_icon);
        }
        CloseHandle(g_mutex);
        return 1;
    }

    g_taskbar_created = RegisterWindowMessageA("TaskbarCreated");

    hwnd = CreateWindowExA(0,
                           APP_CLASS_NAME,
                           APP_WINDOW_NAME,
                           WS_OVERLAPPED,
                           0,
                           0,
                           0,
                           0,
                           NULL,
                           NULL,
                           instance,
                           NULL);

    if (hwnd == NULL) {
        UnregisterClassA(APP_CLASS_NAME, instance);
        if (g_icon_owned && g_icon != NULL) {
            DestroyIcon(g_icon);
        }
        CloseHandle(g_mutex);
        return 1;
    }

    if (!add_tray_icon(hwnd)) {
        begin_tray_retry(hwnd);
    }

    while ((result = GetMessageA(&message, NULL, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    if (result < 0) {
        result = 1;
    } else {
        result = (int)message.wParam;
    }

    if (IsWindow(hwnd)) {
        DestroyWindow(hwnd);
    }

    UnregisterClassA(APP_CLASS_NAME, instance);

    if (g_icon_owned && g_icon != NULL) {
        DestroyIcon(g_icon);
    }
    g_icon = NULL;
    g_icon_owned = FALSE;

    if (g_mutex != NULL) {
        CloseHandle(g_mutex);
        g_mutex = NULL;
    }

    return result;
}
