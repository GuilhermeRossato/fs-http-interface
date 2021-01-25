#include <windows.h>

void print_timestamp(int with_brackets, int is_local_time) {
	if (with_brackets) {
		printf("[ ");
	}
    SYSTEMTIME tm;

	if (is_local_time) {
    	GetLocalTime(&tm);
	} else {
    	GetSystemTime(&tm);
	}

	printf("%04d/%02d/%02d", (int) tm.wYear, (int) tm.wMonth, (int) tm.wDay);
	printf(" %02d:%02d:%02d", (int) tm.wHour, (int) tm.wMinute, (int) tm.wSecond);

	if (with_brackets) {
		printf(" ] ");
	}
}