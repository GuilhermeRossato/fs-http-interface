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
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "d3d9.lib")

#define DEBUG

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

int process_and_reply(
	char * recv,
	size_t recv_size,
	/* OUT */ char * reply,
	size_t reply_max_size,
	/* OUT */ size_t * reply_length
) {
	*reply_length = snprintf(
		reply,
		reply_max_size,
		"Hi!"
	);
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

		int64_t recv_length = recv(msg_sock, recv_buffer, sizeof(recv_buffer), 0);

		// Check empty data as they are exploratory requests
		if (recv_length == 0) {
			closesocket(msg_sock);
			continue;
		}

		print_timestamp(1, 1);
		printf("Info: Connection %I64d received %I64d bytes from \"%s\" at port %d\n", ++count, (int64_t) recv_length, inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));

		int is_get_request_method = does_first_start_with_second(recv_buffer, "GET /");
		int is_post_request_method = does_first_start_with_second(recv_buffer, "POST /");
		int is_put_request_method = does_first_start_with_second(recv_buffer, "PUT /");
		int is_head_request_method = does_first_start_with_second(recv_buffer, "HEAD /");
		int is_delete_request_method = does_first_start_with_second(recv_buffer, "DELETE /");
		int is_patch_request_method = does_first_start_with_second(recv_buffer, "PATCH /");
		int is_options_request_method = does_first_start_with_second(recv_buffer, "OPTIONS /");

		if (
			recv_length <= 6 ||
			(
				!is_get_request_method &&
				!is_post_request_method &&
				!is_put_request_method &&
				!is_head_request_method &&
				!is_delete_request_method &&
				!is_patch_request_method &&
				!is_options_request_method
			)
		) {
			print_timestamp(1, 1);
			printf("Info: Client sent unexpected HTTP packet beggining\n");

			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 405 Method Not Allowed\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n"
				"%s",
				snprintf(
					content_buffer,
					sizeof(content_buffer),
					"%s\r\n",
					"Unknown HTTP request method"
				),
				content_buffer
			);

			send(
				msg_sock,
				buffer,
				buffer_size,
				0
			);
			closesocket(msg_sock);
			continue;
		}

		size_t content_length = 0;
		int result = process_and_reply(recv_buffer, recv_length, content_buffer, sizeof(content_buffer), &content_length);
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
		} else {
			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 %s\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n"
				"%s",
				"500 Internal Server Error",
				snprintf(
					content_buffer,
					sizeof(content_buffer),
					"call to \"process_and_reply\" returned %d\r\n",
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
