#include <windows.h>
#include <math.h>

/**
 * Attempts to convert a string into a long
 */
int convert_string_to_long(char * string, long * number, int * veredict) {
	if (string == NULL || number == NULL) {
		return 0;
	}
	char *endptr;
	*number = strtol(string, &endptr, 10);

	if (endptr == string) {
		// Not a valid number at all
		if (veredict != NULL) {
			*veredict = 0;
		}
		return 0;
	}

	if (*endptr != '\0') {
		// String begins with a valid number, but also contains something else after the number
		if (veredict != NULL) {
			*veredict = 1;
		}
		return 0;
	}

	// String is a number
	if (veredict != NULL) {
		*veredict = 2;
	}
	return 1;
}