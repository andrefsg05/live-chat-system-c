#ifndef clientmessages_h
#define clientmessages_h

#include <stdbool.h>
#include <wchar.h>

#include "list.h"
#include "packet_types.h"


#define MAX_LINE 1024
#define WCHAR_BUFFER_SIZE sizeof(wchar_t) * MAX_LINE  // Buffer size for wide character strings
#define MAX_USER_INFO 32

#define TIMEOUT_TIME 1
#define MAX_TIMEOUT 3


// For the sequence buffer to keep keep track of the sequence of packages recieved
// This is used to ensure that messages are processed in the correct order
// and to handle out-of-order packets.
typedef struct {
    int seq;
    wchar_t data;
} SeqBuffer;


// Client Messages
void receive_message(int mode);
void send_messages(PacketType message_type);
bool request_message(PacketType message_type, wchar_t *recipient_or_group);

// Utility functions
void send_packet(int sockfd, struct sockaddr_in *dest_addr, wchar_t *packet);
void buffer_insert(List buffer, int seq, wchar_t data);
bool buffer_pop(List buffer, int seq, wchar_t *out);

#endif // clientmessages_h