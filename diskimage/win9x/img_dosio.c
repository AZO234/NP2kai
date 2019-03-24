#include "compiler.h"
#include "img_dosio.h"

INT64 DOSIOCALL file_seeki64(FILEH handle, INT64 pointer, int method) {

	LARGE_INTEGER	li;

	li.QuadPart = pointer;

	li.LowPart = SetFilePointer(handle, li.LowPart, &li.HighPart, method);

	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
		li.QuadPart = -1;
	}

	return(li.QuadPart);
}

INT64 DOSIOCALL file_getsizei64(FILEH handle) {

	LARGE_INTEGER	li;

	if (GetFileSizeEx(handle, &li) == 0) {
		li.QuadPart = -1;
	}

	return(li.QuadPart);
}
