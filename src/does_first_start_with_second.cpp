#include <windows.h>
#include <stdint.h>

int does_first_start_with_second(const char * a, const char * b) {
	int i;
	for (i = 0; a[i] != '\0' && b[i] != '\0'; i++) {
		if (a[i] != b[i]) {
			return 0;
		}
	}
	if (a[i] != '\0' && b[i] != '\0') {
		return 0;
	}
	if (b[i] == '\0') {
		return 1;
	}
	return 0;
}