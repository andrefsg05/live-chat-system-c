#ifndef client_h
#define client_h

#include <stdbool.h>
#include <wchar.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdatomic.h>

#include "list.h"
#include "packet_types.h"
#include "client_threads.h"
#include "client_authentication.h"
#include "client_terminal.h"
#include "client_messages.h"
#include "client_groups.h"
#include "clientFile_transfer.h"


#define MAX_LINE 1024
#define CHAR_PACKET_SIZE 128  // Buffer size for wide character strings

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

#define MAX_LINE 1024
#define WCHAR_BUFFER_SIZE sizeof(wchar_t) * MAX_LINE  // Buffer size for wide character strings
#define MAX_USER_INFO 32

#define TIMEOUT_TIME 1
#define MAX_TIMEOUT 3


// Client functions
void send_packet (int sockfd, struct sockaddr_in *dest_addr, wchar_t *packet);
void create_address(struct sockaddr_in *address);
void read_command_with_interrupt(wchar_t *command, size_t max_len);


#endif // client_h