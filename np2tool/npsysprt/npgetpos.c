#include <windows.h>
#include <stdio.h>
#include "devdef.h"
#include "npsysprt.h"

#pragma comment(lib, "user32.lib")

int main(int argc, char const *argv[]) 
{
    IOPORT_NP2_MOUSEPOS_DATA data;
    HANDLE hDevice;
    DWORD returned;
    int clockMul = 0;
    
    printf("Neko Project II マウス絶対座標読み取り\n");

    hDevice = CreateFile("\\\\.\\NP2SystemPort", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) 
    {
        printf("ドライバに接続できません。\n");
        return 1;
    }

	if(DeviceIoControl(hDevice, IOCTL_NP2_MOUSEPOS_READ, &data, sizeof(data), &data, sizeof(data), &returned, NULL)){
	    int scrWidth  = GetSystemMetrics(SM_CXSCREEN);
		int scrHeight = GetSystemMetrics(SM_CYSCREEN);
	    printf("絶対マウス座標（生の値）= (%d, %d)\n", data.absPosX, data.absPosY);
	    printf("絶対マウス座標（pixel）= (%d, %d)\n", (ULONG)data.absPosX * scrWidth / 65536, (ULONG)data.absPosY * scrHeight / 65536);
	}else{
		printf("マウス座標情報を取得できませんでした。");
	}

    CloseHandle(hDevice);
    
    return 0;
}