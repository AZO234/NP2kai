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

int main(int argc, char *argv[]) {
    const char *filePath;
    LARGE_INTEGER fileSize;
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    
    filePath = (argc > 1) ? argv[1] : "Z:\\test.txt";

    if (!GetFileAttributesExA(filePath, GetFileExInfoStandard, &fileInfo)) {
        printf("ファイル情報の取得に失敗しました。エラーコード: %lu\n", GetLastError());
        return 1;
    }

    fileSize.HighPart = fileInfo.nFileSizeHigh;
    fileSize.LowPart = fileInfo.nFileSizeLow;

    printf("ファイル: %s\n", filePath);
    printf("サイズ: %lld バイト\n", fileSize.QuadPart);

    printf("作成日時: ");
    PrintFileTime(fileInfo.ftCreationTime);

    printf("最終アクセス日時: ");
    PrintFileTime(fileInfo.ftLastAccessTime);

    printf("最終書き込み日時: ");
    PrintFileTime(fileInfo.ftLastWriteTime);

    return 0;
}