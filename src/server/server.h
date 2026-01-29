/**
 * @file server.h
 * @brief Header file for the server module.
*/

#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <time.h>
#include <wchar.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "list.h"

#define PORT 12345
#define MAX_LINE 1024
#define WCHAR_BUFFER_SIZE sizeof(wchar_t) * MAX_LINE // Buffer size for wide character strings

#define CLIENT_TIMEOUT_CHECK_INTERVAL 10 // Interval in seconds to check for inactive clients
#define MAX_INACTIVE_TIME 120 // Maximum time in seconds a client can be inactive before being disconnected

typedef enum {

    // Login and Registration Packets
    LOGIN = 1, // Authentication packet
    REGISTER, // Registration packet
    EXIT, // Exit packet

    // Message Requests (I'm going to start sending you a message) Sender -> Server
    MSG_ONE_REQUEST, // One-to-one message packet
    MSG_BROADCAST_REQUEST, // Broadcast message packet
    MSG_GROUP_REQUEST, // Group message packet

    // Server -> Destination Client
    MSG_ONE_WARNING, // One-to-one message packet
    MSG_BROADCAST_WARNING, // Broadcast message packet
    MSG_GROUP_WARNING, // Group message packet

    // Message Char Types:  Sender -> Server -> Destination Client(s)
    MSG_ONE, // One-to-one message packet
    MSG_BROADCAST, // Broadcast message packet
    MSG_GROUP, // Group message packet

    // Message Requests Responses
    MSG_OK, // You can start sending messages packet
    MSG_NOK, // You cannot start sending messages packet

    MSG_END, // End of message packet

    // Message Acknowledgments
    MSG_NACK, // Negative acknowledgment packet

    //Groups Management
    GROUP_CREATE, // Create group packet Group creator -> Server
    GROUP_INVITE, // Invite to group packet Group creator -> Server
    GROUP_KICK, // Kick from group packet
    GROUP_LEAVE, // Leave group packet
    GROUP_DELETE, // Delete group packet
    
    GROUP_INFO, // Group information packet
    GROUP_OK, // Group Management success packet
    GROUP_NOK, // Group Management failure packet

    //File transfer
    FILE_SEND_REQUEST, // Request to send a file packet
    FILE_SEND_WARNING, // File packet being sent
    FILE_SEND_ACCEPT, // Accept to receive a file packet
    FILE_SEND_DECLINE, // Decline to receive a file packet

    HEARTBEAT, // Heartbeat packet to check if the client is still active

    SERVER_ANNOUNCEMENT, // Server announcement packet

    LOGIN_OK, // Login successful packet
    LOGIN_NOK, // Login failed packet

    LOGOUT_OK, // Logout successful packet
    LOGOUT_NOK, // Logout failed packet

} PacketType;

// Server structure prototypes
void create_address(struct sockaddr_in *address);

//Server warnings
void server_broadcast(List connected_clients, int sockfd, wchar_t *message);

// Client timeout management
void check_inactive_clients(List connected_clients, int sockfd);
void handle_heartbeat(List connected_clients, struct sockaddr_in *client_addr, int sockfd);

// Packet handling
void send_packet (int sockfd, struct sockaddr_in *dest_addr, wchar_t *packet);
void handle_packet(wchar_t *packet, struct sockaddr_in *cliaddr, int sockfd, List connected_clients, List registered_users, List registered_groups);

#endif /* SERVER_H */