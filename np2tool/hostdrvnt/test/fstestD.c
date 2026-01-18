#include <windows.h>
#include <stdio.h>

void PrintFileTime(FILETIME ft) {
    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&ft, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    printf("%04d/%02d/%02d %02d:%02d:%02d\n",
           stLocal.wYear, stLocal.wMonth, stLocal.wDay,
           stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
}

void PrintAttributes(DWORD attrs) {
    printf("ファイル属性: 0x%08lX (", attrs);
    if (attrs & FILE_ATTRIBUTE_READONLY)        printf("READONLY ");
    if (attrs & FILE_ATTRIBUTE_HIDDEN)          printf("HIDDEN ");
    if (attrs & FILE_ATTRIBUTE_SYSTEM)          printf("SYSTEM ");
    if (attrs & FILE_ATTRIBUTE_DIRECTORY)       printf("DIRECTORY ");
    if (attrs & FILE_ATTRIBUTE_ARCHIVE)         printf("ARCHIVE ");
    if (attrs & FILE_ATTRIBUTE_DEVICE)          printf("DEVICE ");
    if (attrs & FILE_ATTRIBUTE_NORMAL)          printf("NORMAL ");
    if (attrs & FILE_ATTRIBUTE_TEMPORARY)       printf("TEMPORARY ");
    if (attrs & FILE_ATTRIBUTE_SPARSE_FILE)     printf("SPARSE ");
    if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)   printf("REPARSE ");
    if (attrs & FILE_ATTRIBUTE_COMPRESSED)      printf("COMPRESSED ");
    if (attrs & FILE_ATTRIBUTE_OFFLINE)         printf("OFFLINE ");
    if (attrs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) printf("NOT_INDEXED ");
    if (attrs & FILE_ATTRIBUTE_ENCRYPTED)       printf("ENCRYPTED ");
    printf(")\n");
}

int main(int argc, char *argv[]) {
    const char *filePath = (argc > 1) ? argv[1] : "z:\\test.txt";
    BY_HANDLE_FILE_INFORMATION info;
    ULONGLONG fileSize;
    ULONGLONG index;

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
        printf("ファイルを開けませんでした: エラーコード %lu\n", GetLastError());
        return 1;
    }

    if (!GetFileInformationByHandle(hFile, &info)) {
        printf("GetFileInformationByHandle 失敗: エラーコード %lu\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }

    fileSize = ((ULONGLONG)info.nFileSizeHigh << 32) | info.nFileSizeLow;
    index = ((ULONGLONG)info.nFileIndexHigh << 32) | info.nFileIndexLow;

    printf("ファイル: %s\n", filePath);
    PrintAttributes(info.dwFileAttributes);
    printf("作成日時: ");
    PrintFileTime(info.ftCreationTime);
    printf("最終アクセス日時: ");
    PrintFileTime(info.ftLastAccessTime);
    printf("最終書き込み日時: ");
    PrintFileTime(info.ftLastWriteTime);
    printf("ボリュームシリアル番号: 0x%08lX\n", info.dwVolumeSerialNumber);
    printf("ファイルサイズ: %llu バイト\n", fileSize);
    printf("リンク数: %lu\n", info.nNumberOfLinks);
    printf("ファイルインデックス: %llu (High: 0x%08lX, Low: 0x%08lX)\n",
           index, info.nFileIndexHigh, info.nFileIndexLow);

    CloseHandle(hFile);
    return 0;
}