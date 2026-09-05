#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "NP2IDLE.H"

static void print_usage(void)
{
    printf("NP2ICTRL - control NP2IDLE.VXD\n"
           "Usage:\n"
           "  NP2ICTRL ON\n"
           "  NP2ICTRL OFF\n"
           "  NP2ICTRL STATUS\n"
           "\n"
           "With no argument, STATUS is used.\n");
}

static void print_error(const char *what, DWORD error)
{
    char buffer[256];
    DWORD n;

    buffer[0] = '\0';
    n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       error,
                       0,
                       buffer,
                       sizeof(buffer),
                       NULL);

    if (n != 0) {
        while (n != 0 && (buffer[n - 1] == '\r' || buffer[n - 1] == '\n')) {
            buffer[--n] = '\0';
        }
        fprintf(stderr, "%s failed: error %lu: %s\n", what,
                (unsigned long)error, buffer);
    } else {
        fprintf(stderr, "%s failed: error %lu\n", what,
                (unsigned long)error);
    }
}

static HANDLE open_np2idle(void)
{
    HANDLE h;

    /* The Win95 VxD interface historically accepts zero for the creation
     * disposition when opening a registered/static VxD device name. */
    h = CreateFileA(NP2IDLE_DEVICE_NAME,
                    0,
                    0,
                    NULL,
                    0,
                    0,
                    NULL);

    /* OPEN_EXISTING also works on common Win9x configurations.  Keep it as
     * a fallback because it is the conventional Win32 device form. */
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

static int set_state(HANDLE h, DWORD code, const char *name)
{
    DWORD returned = 0;

    if (!DeviceIoControl(h,
                         code,
                         NULL,
                         0,
                         NULL,
                         0,
                         &returned,
                         NULL)) {
        print_error("DeviceIoControl", GetLastError());
        return 3;
    }

    printf("NP2IDLE: %s\n", name);
    return 0;
}

static int show_state(HANDLE h)
{
    DWORD state = 0;
    DWORD returned = 0;

    if (!DeviceIoControl(h,
                         NP2IDLE_DIOC_GETSTATE,
                         NULL,
                         0,
                         &state,
                         sizeof(state),
                         &returned,
                         NULL)) {
        print_error("DeviceIoControl", GetLastError());
        return 3;
    }

    if (returned != sizeof(state)) {
        fprintf(stderr,
                "NP2IDLE: invalid STATUS reply size (%lu bytes)\n",
                (unsigned long)returned);
        return 4;
    }

    if (state == NP2IDLE_STATE_ON) {
        printf("NP2IDLE: ON\n");
        return 0;
    }

    if (state == NP2IDLE_STATE_OFF) {
        printf("NP2IDLE: OFF\n");
        return 0;
    }

    fprintf(stderr, "NP2IDLE: invalid STATUS value %lu\n",
            (unsigned long)state);
    return 4;
}

int main(int argc, char **argv)
{
    const char *command;
    HANDLE h;
    int result;

    if (argc > 2) {
        print_usage();
        return 1;
    }

    command = (argc == 1) ? "STATUS" : argv[1];

    if (_stricmp(command, "/?") == 0 ||
        _stricmp(command, "-?") == 0 ||
        _stricmp(command, "HELP") == 0) {
        print_usage();
        return 0;
    }

    if (_stricmp(command, "ON") != 0 &&
        _stricmp(command, "OFF") != 0 &&
        _stricmp(command, "STATUS") != 0) {
        fprintf(stderr, "Unknown command: %s\n\n", command);
        print_usage();
        return 1;
    }

    h = open_np2idle();
    if (h == INVALID_HANDLE_VALUE) {
        print_error("CreateFile(" NP2IDLE_DEVICE_NAME ")", GetLastError());
        fprintf(stderr,
                "NP2IDLE.VXD may not be loaded. Check the [386Enh] "
                "device= line and reboot Windows.\n");
        return 2;
    }

    if (_stricmp(command, "ON") == 0) {
        result = set_state(h, NP2IDLE_DIOC_ENABLE, "ON");
    } else if (_stricmp(command, "OFF") == 0) {
        result = set_state(h, NP2IDLE_DIOC_DISABLE, "OFF");
    } else {
        result = show_state(h);
    }

    CloseHandle(h);
    return result;
}
