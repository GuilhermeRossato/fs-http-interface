#include <windows.h>

void show_help(char * executable_name) {
	printf(
		"This utility answers HTTP requests to expose another interface\n"
		"\n"
		"\n%s "
		"[portnumber] The port number of the HTTP server to listen to, default " DEFAULT_PORT "\n"
		"\n",
		executable_name
	);
}