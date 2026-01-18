#include <windows.h>
#include <stdio.h>

void PrintFileSize(DWORD high, DWORD low) {
    ULONGLONG size = ((ULONGLONG)high << 32) | low;
    printf("サイズ: %llu バイト\n", size);
}

int main(int argc, char *argv[]) {
    const char *searchPath = (argc > 1) ? argv[1] : "Z:\\*.txt";
    DWORD err;

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("ファイル列挙に失敗しました。エラーコード: %lu\n", GetLastError());
        return 1;
    }

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            printf("[ディレクトリ] %s\n", findData.cFileName);
        } else {
            printf("[ファイル] %s\n", findData.cFileName);
            PrintFileSize(findData.nFileSizeHigh, findData.nFileSizeLow);
        }
    } while (FindNextFileA(hFind, &findData));

    err = GetLastError();
    if (err != ERROR_NO_MORE_FILES) {
        printf("FindNextFile でエラーが発生しました: %lu\n", err);
    }

    FindClose(hFind);
    return 0;
}