#ifndef clientauthentication_h
#define clientauthentication_h


#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include "packet_types.h"
#include "client_terminal.h"
#include "client_messages.h"


#define MAX_LINE 1024
#define WCHAR_BUFFER_SIZE sizeof(wchar_t) * MAX_LINE  // Buffer size for wide character strings
#define MAX_USER_INFO 32


#define TIMEOUT_TIME 1
#define MAX_TIMEOUT 3


bool authenticate_user(PacketType packet_type, int sockfd, struct sockaddr_in *servaddr);


#endif // clientauthentication_h