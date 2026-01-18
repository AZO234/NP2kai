#include <windows.h>
#include <stdio.h>

void PrintFileSizeUsingGetFileSize(HANDLE hFile) {
    DWORD highPart = 0;
    DWORD lowPart = GetFileSize(hFile, &highPart);
    ULONGLONG size;
    if (lowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        printf("GetFileSize: エラー %lu\n", GetLastError());
        return;
    }
    size = ((ULONGLONG)highPart << 32) | lowPart;
    printf("[GetFileSize]      サイズ: %llu バイト\n", size);
}

void PrintFileSizeUsingGetFileSizeEx(HANDLE hFile) {
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        printf("GetFileSizeEx: エラー %lu\n", GetLastError());
        return;
    }
    printf("[GetFileSizeEx]    サイズ: %lld バイト\n", fileSize.QuadPart);
}

void PrintFileSizeUsingGetFileAttributesEx(LPCSTR filePath) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    ULONGLONG size;
    if (!GetFileAttributesExA(filePath, GetFileExInfoStandard, &fileInfo)) {
        printf("GetFileAttributesEx: エラー %lu\n", GetLastError());
        return;
    }

    size = ((ULONGLONG)fileInfo.nFileSizeHigh << 32) | fileInfo.nFileSizeLow;
    printf("[GetFileAttributesEx] サイズ: %llu バイト\n", size);
}

int main(int argc, char *argv[]) {
    const char *filePath = (argc > 1) ? argv[1] : "Z:\\test.txt";

    HANDLE hFile = CreateFileA(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("ファイルを開けませんでした: %lu\n", GetLastError());
        return 1;
    }

    printf("ファイル: %s\n", filePath);

    PrintFileSizeUsingGetFileSize(hFile);
    PrintFileSizeUsingGetFileSizeEx(hFile);
    PrintFileSizeUsingGetFileAttributesEx(filePath);

    CloseHandle(hFile);
    return 0;
}