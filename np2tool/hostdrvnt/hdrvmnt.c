#include <windows.h>
#include <stdio.h>

#define HOSTDRV_PATH	"\\Device\\HOSTDRV"

int main(int argc, char const* argv[]) {
	int i;
	int remove = 0;
	int hasarg = 0;
	int isAllocated = 0;
	char driveLetter[] = " :";
	char driveLetterTmp[] = " :";
	char target[MAX_PATH] = {0};
	DWORD result;

    printf("Neko Project II ホスト共有ドライブ for Windows NT\n");
	for(i=1;i<argc;i++){
		int slen = strlen(argv[i]);
		if(slen==1 || slen==2 && argv[i][1]==':'){
			char drive = argv[i][0];
			if('a' <= drive && drive <= 'z' || 'A' <= drive && drive <= 'Z'){
				hasarg = 1;
				driveLetter[0] = drive;
			}else{
				printf("ドライブ文字指定が異常です。\n");
        		return 1;
			}
		}else if(stricmp(argv[i], "-r")==0 || stricmp(argv[i], "/r")==0){
			remove = 1;
			hasarg = 1;
		}else if(stricmp(argv[i], "-h")==0 || stricmp(argv[i], "/h")==0 || stricmp(argv[i], "-?")==0 || stricmp(argv[i], "/?")==0){
	        printf("usage: %s <ドライブ文字> [/r] \n", argv[0]);
	        printf("/rをつけた場合、割り当て解除します。\n");
	        return 0;
		}
	}
	
	// そもそもデバイスが使えるか確認
    if (GetFileAttributes("\\\\.\\HOSTDRV") == 4294967295) 
    {
    	DWORD err = GetLastError();
        printf("Error: ドライバに接続できません。(code: %d)\n", err);
        return 1;
    }
	
	// 割り当て済みチェック
	for(i='A';i<='Z';i++){
		driveLetterTmp[0] = i;
	    if (QueryDosDevice(driveLetterTmp, target, MAX_PATH)) {
	    	if(stricmp(target,HOSTDRV_PATH)==0){
	    		isAllocated = 1;
	    		break;
	    	}
	    }
	}
	
	if(hasarg && !remove){
		if(isAllocated){
		    printf("現在ドライブ%sに割り当てられています。\n", driveLetterTmp);
		    printf("先に/rオプションで登録解除してください。\n");
	        return 1;
		}
	    if (!DefineDosDeviceA(DDD_RAW_TARGET_PATH, driveLetter, HOSTDRV_PATH)) {
	        printf("割り当てに失敗しました。\n");
	        return 1;
	    }
	    printf("HOSTDRVを%sに割り当てました。\n", driveLetter);
	}else if(hasarg && remove){
		if(!isAllocated){
			printf("現在割り当てられていません。\n");
	        return 1;
		}
	    if (!DefineDosDeviceA(DDD_RAW_TARGET_PATH|DDD_REMOVE_DEFINITION, driveLetterTmp, HOSTDRV_PATH)) {
	        printf("割り当て解除に失敗しました。\n");
	        return 1;
	    }
	    printf("HOSTDRVを%sから割り当て解除しました。\n", driveLetterTmp);
	}else if(argc < 1){
        printf("引数が正しくありません。\n", argv[0]);
        printf("/?オプションで使い方を表示します。\n");
		return 1;
	}else{
		if(isAllocated){
		    printf("現在ドライブ%sに割り当てられています。\n", driveLetterTmp);
		}else{
			printf("現在割り当てられていません。\n");
		}
	}
    
	return 0;
}