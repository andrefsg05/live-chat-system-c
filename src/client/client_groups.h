#ifndef CLIENT_GROUPS_H
#define CLIENT_GROUPS_H


#include <wchar.h>

#include "packet_types.h"


#define MAX_LINE 1024
#define WCHAR_BUFFER_SIZE sizeof(wchar_t) * MAX_LINE  // Buffer size for wide character strings
#define MAX_USER_INFO 32


#define TIMEOUT_TIME 1
#define MAX_TIMEOUT 3


// Functions for group management
void group_page();
void request_group_action(PacketType group_type, const wchar_t *group_name, const wchar_t *username);

#endif // CLIENT_GROUPS_H