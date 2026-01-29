#ifndef SERVERMESSAGES_H
#define SERVERMESSAGES_H


#include <stdbool.h>
#include <wchar.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "list.h"
#include "server.h"


// Message start types
void start_broadcast(List connected_clients, struct sockaddr_in *sender_addr, int sockfd);
void start_message_one(List connected_clients, struct sockaddr_in *sender_addr, int sockfd, wchar_t *recipient_username);
void start_message_group(List registered_groups, List connected_clients, struct sockaddr_in *sender_addr, int sockfd, wchar_t *group_name);

// Message send types
void send_message_one(List connected_clients, int sockfd, struct sockaddr_in *sender_addr, wchar_t *message);
void send_message_many(List connected_clients, struct sockaddr_in *sender_addr, int sockfd, PacketType packet_type, wchar_t *rest);

// Message end
void message_end(List connected_clients, struct sockaddr_in *sender_addr, int sockfd);

// Message NACK
void nack_received(List connected_clients, struct sockaddr_in *sender_addr, int sockfd, wchar_t *rest);

// Message auxiliary functions
void buffer_insert(List buffer, int seq, wchar_t data);
bool buffer_pop(List buffer, int seq, wchar_t *out);

#endif /* SERVERMESSAGES_H */