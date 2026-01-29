#ifndef SERVERFILE_TRANSFER_H
#define SERVERFILE_TRANSFER_H

#include "server.h"
#include <wchar.h>
#include <netinet/in.h>


// Function to handle file send request from the sender
void handle_file_send_request_server(List connected_clients, int sockfd, struct sockaddr_in *sender_addr, wchar_t *rest);

// Function to handle ACCEPT/DECLINE from recipients
void handle_file_send_accept_decline(List connected_clients, int sockfd, struct sockaddr_in *cliaddr, PacketType type, wchar_t *rest);

#endif // SERVERFILE_TRANSFER_H