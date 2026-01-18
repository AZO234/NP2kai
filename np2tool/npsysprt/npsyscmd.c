#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "devdef.h"
#include "npsysprt.h"

int main(int argc, char const *argv[]) 
{
	int i;
    DWORD returned;
    DWORD cmdLength;
    IOPORT_NP2_SIMPLE_DATA data = {0};
    HANDLE hDevice;

	// 引数の処理
	if(argc <= 1)
	{
        printf("Neko Project II システムポート アクセスツール\n");
        printf("usage: %s <コマンド> [パラメータ値byte1] [byte2] [byte3] [byte4] \n", argv[0]);
        return 1;
	}
	cmdLength = strlen(argv[1]);
	if(cmdLength > 16)
	{
        printf("Error: コマンドは16文字までです。\n");
		return 1;
	}
	data.paramLength = argc - 2;
	if(data.paramLength > 4)
	{
        printf("Error: パラメータ値は4byteまでです。\n");
		return 1;
	}

	// パラメータセット
	memcpy(data.command, argv[1], cmdLength);
	for(i=0;i<data.paramLength;i++)
	{
		data.param.b[i] = atoi(argv[2 + i]);
	}

	// デバイスオープン
    hDevice = CreateFile("\\\\.\\NP2SystemPort", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) 
    {
        printf("Error: ドライバに接続できません。\n");
        return 1;
    }

	// コマンド送信
    if(DeviceIoControl(hDevice, IOCTL_NP2_SIMPLE, &data, sizeof(IOPORT_NP2_SIMPLE_DATA), &data, sizeof(IOPORT_NP2_SIMPLE_DATA), &returned, NULL))
    {
    	char readData[17] = {0}; // NULL文字を付けるための仮バッファ
		memcpy(readData, data.readBuffer, 16);
        printf("%s\n", readData);
    }
    else
    {
        printf("Error: ポートアクセスに失敗しました。\n");
    	CloseHandle(hDevice);
        return 1;
    }
    CloseHandle(hDevice);
    
    return 0;
}