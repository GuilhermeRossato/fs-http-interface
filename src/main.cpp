#include <windows.h>
#include <wingdi.h>
#include <stdio.h>
#include <stdbool.h>
#include <cstdint>
#include <string>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define DEFAULT_PORT "8084"
#define INPUT_BUFFER_SIZE 4 * 1024 - 1
#define OUTPUT_BUFFER_SIZE 1000000

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")

// #define DEBUG

#ifdef DEBUG
	#define ASSERT(explanation, condition) if (!(condition)) { printf("Assertation failed at %s:%d\nIncoherence at: %s\n", __FILE__, __LINE__, explanation); exit(__LINE__); }
#else
	#define ASSERT(explanation, condition)
#endif

#include "./print_timestamp.cpp"
#include "./show_help.cpp"
#include "./convert_string_to_long.cpp"
#include "./are_same_string.cpp"
#include "./does_first_start_with_second.cpp"
#include "./separate_string_by_char.cpp"
#include "./str_index_of.cpp"

void convert_char_array_to_LPCWSTR(const char* char_array, LPCWSTR wide_char_array, int wide_char_array_size) {
    MultiByteToWideChar(CP_ACP, 0, char_array, -1, (wchar_t*) wide_char_array, wide_char_array_size);
}

#define WIDE_CHAR_ARRAY_SIZE 4096

wchar_t * wide_char_array = new wchar_t[WIDE_CHAR_ARRAY_SIZE];

int is_path_directory(char* path) {
	wide_char_array[0] = '\0';
	convert_char_array_to_LPCWSTR(path, wide_char_array, WIDE_CHAR_ARRAY_SIZE);
	DWORD dwAttrib = GetFileAttributesW(wide_char_array);

	return (
		(dwAttrib != INVALID_FILE_ATTRIBUTES) &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
	);
}

int is_path_file(char* path) {
	wide_char_array[0] = '\0';
	convert_char_array_to_LPCWSTR(path, wide_char_array, WIDE_CHAR_ARRAY_SIZE);
	DWORD dwAttrib = GetFileAttributesW(wide_char_array);

	return (
		(dwAttrib != INVALID_FILE_ATTRIBUTES) &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
	);
}

int does_path_exists(char* path) {
	wide_char_array[0] = '\0';
	convert_char_array_to_LPCWSTR(path, wide_char_array, WIDE_CHAR_ARRAY_SIZE);
	DWORD dwAttrib = GetFileAttributesW(wide_char_array);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES);
}

int64_t get_file_size(char* path)
{
	wide_char_array[0] = '\0';
	convert_char_array_to_LPCWSTR(path, wide_char_array, WIDE_CHAR_ARRAY_SIZE);
    HANDLE fp = CreateFileW(
		wide_char_array,
		GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
		NULL
	);
    if (fp == INVALID_HANDLE_VALUE)
        return -__LINE__;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size))
    {
        CloseHandle(fp);
        return -__LINE__;
    }

    CloseHandle(fp);
    return size.QuadPart;
}


#define MAX_HTTP_HEADER_POINTERS 128

struct http_request_t {
	char * method;
	int method_length;
	char * path;
	int path_length;
	char * fragment;
	int fragment_length;
	char * parameters[MAX_HTTP_HEADER_POINTERS];
	int parameters_size;
	char * headers[MAX_HTTP_HEADER_POINTERS];
	int headers_size;
	char * body;
	int body_size;
} http_request_t;

char * get_parameter_from_request(struct http_request_t * request, const char * parameter) {
	for (int i = 0; i < request->parameters_size; i++) {
		if (does_first_start_with_second(request->parameters[i], parameter)) {
			int s = strlen(parameter);
			if (s < 128 && parameter[s] == '\0' && request->parameters[i][s] == '=') {
				return ((char *) request->parameters[i]) + s + 1;
			}
		}
	}
	return NULL;
}

int read_file_into_buffer(
	char * path,
	/* OUT */ char * buffer,
	size_t buffer_max_size,
	/* OUT */ size_t * buffer_length
) {
    DWORD bytes_read = 0;

	wide_char_array[0] = '\0';
	convert_char_array_to_LPCWSTR(path, wide_char_array, WIDE_CHAR_ARRAY_SIZE);
    HANDLE fp = CreateFileW(
		wide_char_array,
		GENERIC_READ, // open for reading
        FILE_SHARE_READ, // share for reading
		NULL, // default security
		OPEN_EXISTING, // existing  file only
        FILE_ATTRIBUTE_NORMAL, // normal file
		NULL // no template
	);
	if (fp == INVALID_HANDLE_VALUE) {
		return -1;
	}
	if (ReadFile(fp, buffer, (buffer_max_size-1), &bytes_read, NULL) == FALSE) {
        CloseHandle(fp);
        return -2;
    }
	if (bytes_read+1 < buffer_max_size) {
		buffer[bytes_read+1] = '\0'; // Just to make sure
	}
	*buffer_length = bytes_read;
    CloseHandle(fp);
	return 1;
}

/**
 * Returns positive for success
 * Returns negative for errors
 * Returns zero for not found
 */
int process_and_reply(
	struct http_request_t * request,
	/* OUT */ char * reply,
	size_t reply_max_size,
	/* OUT */ size_t * reply_length
) {
	char * path = get_parameter_from_request(request, "path");

	if (strcmp(request->method, "GET") == 0) {
		if (strcmp(request->path, "/") == 0) {
			return 0;
		}

		if (strcmp(request->path, "/file/exists/") == 0) {
			if (path == NULL) {
				*reply_length = snprintf(reply, reply_max_size, "Error: Missing \"path\" parameter");
				return 1;
			}
			int veredict = is_path_file(path);
			*reply_length = snprintf(reply, reply_max_size, "%d", veredict);
			return 1;
		}

		if (strcmp(request->path, "/file/size/") == 0) {
			if (path == NULL) {
				*reply_length = snprintf(reply, reply_max_size, "Error: Missing \"path\" parameter");
				return 1;
			}
			if (is_path_file(path)) {
				return 0;
			}
			int64_t size = get_file_size(path);
			if (size < 0 && ((int) size) < 0) {
				return (int) size;
			}
			*reply_length = snprintf(reply, reply_max_size, "%" PRId64, size);
			return 1;
		}

		if (strcmp(request->path, "/file/contents/") == 0) {
			if (path == NULL) {
				*reply_length = snprintf(reply, reply_max_size, "Error: Missing \"path\" parameter");
				return 1;
			}
			if (!read_file_into_buffer(path, reply, reply_max_size, reply_length)) {
				return 0;
			}
			return 1;
		}

		if (strcmp(request->path, "/directory/exists/") == 0 || strcmp(request->path, "/folder/exists/") == 0) {
			if (path == NULL) {
				*reply_length = snprintf(reply, reply_max_size, "Error: Missing \"path\" parameter");
				return 1;
			}
			int veredict = is_path_directory(path);
			*reply_length = snprintf(reply, reply_max_size, "%d", veredict);
			return 1;
		}

		if (strcmp(request->path, "/path/exists/") == 0) {
			if (path == NULL) {
				*reply_length = snprintf(reply, reply_max_size, "Error: Missing \"path\" parameter");
				return 1;
			}
			int veredict = does_path_exists(path);
			*reply_length = snprintf(reply, reply_max_size, "%d", veredict);
			return 1;
		}
	}
	return 0;
}

/**
 * Parses an HTTP request and writes the parts in the output
 * Obs: This overwrites some bytes in the recv buffer and assigns it to output.
 * Do not free recv while using the http_request_t structure.
 *
 * Returns 1 if successfull
 * Returns negative value if the parsing fails. Positive output is line number of error
 */
int parse_http_request(struct http_request_t * output, char * recv, int recv_length) {
	int index = 0;
	output->method = NULL;
	output->method_length = 0;
	output->path = NULL;
	output->path_length = 0;
	output->fragment = NULL;
	output->fragment_length = 0;
	output->parameters[0] = NULL;
	output->parameters_size = 0;
	output->headers[0] = NULL;
	output->headers_size = 0;
	output->body = NULL;
	output->body_size = NULL;

	enum {METHOD, PATH, FRAGMENT, PARAMETERS, AFTER_URI, HEADERS, BODY} state = METHOD;
	int i = 0;
	for (i = 0; i < recv_length; i++) {
		if (state == METHOD) {
			if (recv[i] == ' ') {
				if (i <= 1) {
					return -__LINE__;
				}
				recv[i] = '\0';
				output->method = &recv[0];
				output->method_length = i;
				output->path = &recv[i + 1];
				state = PATH;
			}
			continue;
		}

		int state_allows_uri_encoding = (state == PATH || state == FRAGMENT || state == PARAMETERS);

		if (state_allows_uri_encoding && recv[i] == '%') {
			if (recv[i+1] == '2' && recv[i+2] == '0') {
				recv[i] = ' ';
			} else if (recv[i+1] == '2' && recv[i+2] == '1') { // %21
				recv[i] = '!';
			} else if (recv[i+1] == '2' && recv[i+2] == '2') { // %22
				recv[i] = '"';
			} else if (recv[i+1] == '2' && recv[i+2] == '4') { // %24
				recv[i] = '%';
			} else if (recv[i+1] == '2' && recv[i+2] == '7') { // %27
				recv[i] = '\'';
			} else if (recv[i+1] == '2' && recv[i+2] == '8') { // %28
				recv[i] = '(';
			} else if (recv[i+1] == '2' && recv[i+2] == '9') { // %29
				recv[i] = ')';
			} else if (recv[i+1] == '2' && recv[i+2] == 'C') { // %2C
				recv[i] = ',';
			} else if (recv[i+1] == '2' && recv[i+2] == 'D') { // %2D
				recv[i] = '-';
			} else if (recv[i+1] == '2' && recv[i+2] == 'E') { // %2E
				recv[i] = '.';
			} else if (recv[i+1] == '3' && recv[i+2] == 'C') { // %3C
				recv[i] = '<';
			} else if (recv[i+1] == '3' && recv[i+2] == 'E') { // %3E
				recv[i] = '>';
			} else if (recv[i+1] == '3' && recv[i+2] == 'D') { // %3D
				recv[i] = '=';
			} else if (recv[i+1] == '4' && recv[i+2] == '0') { // %40
				recv[i] = '@';
			} else if (recv[i+1] == '6' && recv[i+2] == '0') { // %5F
				recv[i] = '_';
			} else if (recv[i+1] == '6' && recv[i+2] == '0') { // %60
				recv[i] = '`';
			} else if (recv[i+1] == '7' && recv[i+2] == 'E') { // %7E
				recv[i] = '~';
			} else if (recv[i+1] == '7' && recv[i+2] == 'F') { // %7F
				recv[i] = ' '; // Close enough
			} else if (recv[i+1] == '8' && recv[i+2] == '2') { // %82
				recv[i] = ','; // Close enough
			} else {
				recv[i] = '?'; // Fallback
			}
			// Move buffer data to remove the "explanation bytes" of the URI encoding
			for (int j = i + 1; j < recv_length - 2; j++) {
				if (recv[j + 2] == ' ' || recv[j + 2] == '\0' || recv[j + 2] == '\r' || recv[j + 2] == '\n') {
					recv[j] = ' ';
					recv[j + 1] = ' ';
					break;
				}
				recv[j] = recv[j + 2];
			}
			continue;
		}

		if (state == PATH) {
			if (recv[i] == '#') {
				state = FRAGMENT;
				recv[i] = '\0';
				output->path_length = (&recv[i]) - output->path;
				output->fragment = &recv[i];
				continue;
			}
			if (recv[i] == '?') {
				state = PARAMETERS;
				recv[i] = '\0';
				output->path_length = (&recv[i]) - output->path;
				output->fragment = NULL;
				output->fragment_length = 0;
				output->parameters[0] = NULL;
				output->parameters_size = 0;
				continue;
			}
			if (recv[i] == ' ') {
				recv[i] = '\0';
				output->path_length = (&recv[i]) - output->path;
				output->fragment = NULL;
				output->fragment_length = 0;
				output->parameters[0] = NULL;
				output->parameters_size = 0;
				state = AFTER_URI;
				continue;
			}
			continue;
		}

		if (state == FRAGMENT) {
			if (recv[i] == '?') {
				state = PARAMETERS;
				recv[i] = '\0';
				output->fragment_length = (&recv[i]) - output->fragment;
				output->parameters[0] = NULL;
				output->parameters_size = 0;
				output->path_length = i - output->method_length - 1;
				continue;
			}
			if (recv[i] == ' ') {
				recv[i] = '\0';
				output->fragment_length = (&recv[i]) - output->fragment;
				output->parameters[0] = NULL;
				output->parameters_size = 0;
				state = AFTER_URI;
				continue;
			}
			continue;
		}

		if (state == PARAMETERS) {
			if (recv[i] == ' ') {
				recv[i] = '\0';
				state = AFTER_URI;
				continue;
			}
			if (output->parameters[0] == NULL && recv[i] != '&') {
				output->parameters[0] = &recv[i];
				output->parameters_size = 1;
			}
			if (recv[i] == '&') {
				recv[i] = '\0';
				if (recv[i + 1] != ' ') {
					if (output->parameters_size + 1 < MAX_HTTP_HEADER_POINTERS) {
						output->parameters[output->parameters_size] = &recv[i + 1];
						output->parameters_size += 1;
					}
				}
			}
			continue;
		}

		if (state == AFTER_URI) {
			if (recv[i] == '\0') {
				return -__LINE__;
			}
			if (recv[i] == '\r' && recv[i + 1] == '\n') {
				i += 1;
				state = HEADERS;
				output->headers[0] = NULL;
				output->headers_size = 0;
			}
			continue;
		}

		if (state == HEADERS) {
			if (output->headers[0] == NULL && recv[i] != '\r') {
				output->headers[0] = &recv[i];
				output->headers_size = 1;
			}
			if (recv[i] == '\r' && recv[i+1] == '\n') {
				recv[i] = '\0';
				i += 1;
				if (recv[i + 1] != '\r') {
					if (output->headers_size + 1 < MAX_HTTP_HEADER_POINTERS) {
						output->headers[output->headers_size] = &recv[i + 1];
						output->headers_size += 1;
					}
				} else {
					i += 3;
					state = BODY;
					break;
				}
			}
			continue;
		}

		return -__LINE__;
	}
	if (state != BODY) {
		return -__LINE__;
	}
	if (i >= recv_length) {
		output->body = NULL;
		output->body_size = 0;
		// No body
		return 1;
	}
	output->body = &recv[i];
	output->body_size = recv_length - i;
	return 1;
}

int main(int argn, char ** argc) {
	char * portnumber_str = argn >= 2 ? argc[2] : DEFAULT_PORT;

	if (portnumber_str != NULL && strcmp(portnumber_str, "--help") == 0) {
		show_help(argc[0]);
		printf("\nFor more information access the project's repository:\n\nhttps://github.com/GuilhermeRossato/fs-http-interface\n");
		return 1;
	}

	long portnumber;
	if (!convert_string_to_long(portnumber_str, &portnumber, NULL)) {
		printf("The portnumber parameter must contain a number, got invalid input: (%s)\n", portnumber_str);
		return 1;
	}

	int err_code;

    SOCKET sock, msg_sock;
    WSADATA wsaData;

	// Initiate use of the Winsock DLL by this process
	err_code = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err_code == SOCKET_ERROR) {
        printf("WSAStartup returned SOCKET_ERROR. Program will exit.\n");
		return 1;
	} else if (err_code != 0) {
		printf("WSAStartup returned %d. Program will exit.\n", err_code);
		return 1;
	}

    int addr_len;
    struct sockaddr_in local;

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		printf("Warning: WinSock DLL doest not seem to support version 2.2 in which this code has been tested with\n");
	}

    // Fill in the address structure
    local.sin_family        = AF_INET;
    local.sin_addr.s_addr   = INADDR_ANY;
    local.sin_port          = htons(portnumber);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET) {
        printf("Error: Socket function returned a invalid socket\n");
		WSACleanup();
		return 1;
	}

    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR) {
		printf("Error: Bind function failed\n");
		WSACleanup();
		return 1;
	}

	char * buffer = (char *) malloc(OUTPUT_BUFFER_SIZE);
	if (!buffer) {
		printf("Error: Could not allocate buffer for output of size %d\n", OUTPUT_BUFFER_SIZE);
		return 1;
	}

	char recv_buffer[INPUT_BUFFER_SIZE];
    int64_t count = 0;
	struct sockaddr_in client_addr;
	int i, buffer_size;

	char content_buffer[OUTPUT_BUFFER_SIZE];

	print_timestamp(1, 1);
    printf("Info: Waiting for connection at port %I64d\n", (int64_t) portnumber);

	struct http_request_t http;

    while (1) {
		if (listen(sock, 10) == SOCKET_ERROR) {
			print_timestamp(1, 1);
			printf("Error: Listen function failed\n");
			WSACleanup();
			return 1;
		}

        addr_len = sizeof(client_addr);
        msg_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);

        if (msg_sock == INVALID_SOCKET || msg_sock == -1) {
			print_timestamp(1, 1);
            printf("Error: Accept function returned a invalid receiving socket: %I64d\n", (int64_t)msg_sock);
    		WSACleanup();
			return 1;
		}

		int recv_length = recv(msg_sock, recv_buffer, sizeof(recv_buffer), 0);

		// Ignore empty connections (might jsut be exploratory requests)
		if (recv_length <= 0) {
			closesocket(msg_sock);
			continue;
		}

		// printf("----------RAW START\n%s\n----------RAW END\n", recv_buffer);

		print_timestamp(1, 1);
		printf("Info: Connection %I64d received %I64d bytes from \"%s\" at port %d\n", ++count, (int64_t) recv_length, inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));

		int parse_veredict = parse_http_request(&http, recv_buffer, recv_length);
		if (parse_veredict <= 0) {
			print_timestamp(1, 1);
			printf("Info: Failed at parsing HTTP request: parse_http_request returned %d\n", parse_veredict);
			closesocket(msg_sock);
			continue;
		}

		/*
		printf("----------\n");
		printf("Method: %s\n", http.method);
		printf("Path: %s\n", http.path);
		printf("Fragment: %s\n", http.fragment);
		printf("Parameters: %d\n", http.parameters_size);
		for (int i = 0; i < http.parameters_size; i++) {
			printf("    %s\n", http.parameters[i]);
		}
		printf("Headers: %d\n", http.headers_size);
		for (int i = 0; i < http.headers_size; i++) {
			printf("    %s\n", http.headers[i]);
		}
		printf("Body size: %d\n", http.body_size);
		printf("----------\n");
		*/

		size_t content_length = 0;
		int result = process_and_reply(&http, content_buffer, sizeof(content_buffer), &content_length);
		if (result > 0) {
			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: %zd\r\n"
				"Connection: close\r\n"
				"\r\n",
				content_length
			);
			for (int i = 0; i < content_length; i++) {
				if (buffer_size >= OUTPUT_BUFFER_SIZE) {
					break;
				}
				buffer[buffer_size] = content_buffer[i];
				buffer_size++;
			}
			buffer[buffer_size < OUTPUT_BUFFER_SIZE ? buffer_size : OUTPUT_BUFFER_SIZE - 1] = '\0';
		} else if (result == 0) {

			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 404 Not Found\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: 0\r\n"
				"Connection: close\r\n"
				"\r\n"
			);
		} else {
			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 500 Internal Server Error\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n"
				"%s",
				snprintf(
					content_buffer,
					sizeof(content_buffer),
					"Error: Call to \"process_and_reply\" returned %d\r\n",
					result
				),
				content_buffer
			);
			buffer[buffer_size < OUTPUT_BUFFER_SIZE ? buffer_size : OUTPUT_BUFFER_SIZE - 1] = '\0';
		}

		send(
			msg_sock,
			buffer,
			buffer_size,
			0
		);

        closesocket(msg_sock);
    }

    WSACleanup();
	return 0;
}
