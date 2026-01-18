#include <windows.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    const char *filePath = (argc > 1) ? argv[1] : "z:\\mmap.txt";
    const DWORD fileSize = 1024;  // 最小1KB確保
    HANDLE hFile;
    HANDLE hMapping;
    LPVOID pMap;
    char *newData;

    // ファイルを開くまたは作成
    hFile = CreateFileA(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("ファイルを開けませんでした。エラー: %lu\n", GetLastError());
        return 1;
    }

	getchar();
	
    // メモリマッピングオブジェクトの作成
    hMapping = CreateFileMappingA(
        hFile,
        NULL,
        PAGE_READWRITE,
        0,
        fileSize,
        NULL
    );

    if (hMapping == NULL) {
        printf("CreateFileMapping 失敗。エラー: %lu\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }

    // メモリにマップ
    pMap = MapViewOfFile(
        hMapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0
    );

    if (pMap == NULL) {
        printf("MapViewOfFile 失敗。エラー: %lu\n", GetLastError());
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 1;
    }

    // データ表示・変更（例: 先頭にメッセージを挿入）
    printf("現在の内容（先頭100バイト）:\n");
    fwrite(pMap, 1, 100, stdout);
    printf("\n\nデータを書き換え中...\n");

    newData = "Hello from memory-mapped file!\n";
    memcpy(pMap, newData, strlen(newData));

    // 後始末
    UnmapViewOfFile(pMap);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    printf("完了しました。\n");
    return 0;
}