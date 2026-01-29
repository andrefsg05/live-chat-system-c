#ifndef userAuthentication_h
#define userAuthentication_h


#include <stdbool.h>
#include <time.h>
#include <wchar.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "list.h"
#include "userRegister.h"


#define MAX_USER_INFO 32


typedef struct {
    int seq;
    wchar_t data;
} SeqBuffer;

// Client state
typedef enum {
    IDLE = 0,
    SENDING_MSG,
    RECEIVING_MSG,
    RECEIVING_FILE,
} ClientState;

typedef struct Client Client;

// Client structure
struct Client {
    wchar_t username [MAX_USER_INFO]; // Username of the client
    char ip[INET_ADDRSTRLEN]; // IP address of the client
    int port; // Port number of the client
    struct sockaddr_in clientAddress; // Address of the client
    time_t last_heartbeat; // Last active time of the client
    ClientState is_chating; // Current state of the client (chatting, idle, etc.)
    Client *chatPartner; // Pointer to the chat partner (if any)
    int expected_seq; // Expected sequence number for message handling
    List orderBuffer; // List of messages waiting to be sent (out of order handling)
    List sentMessageBuffer; // Message being sent by the client
};


// Utility functions to manage clients
Client* find_client_by_username(List connected_clients, wchar_t *username);
Client* find_client_by_address(List connected_clients, struct sockaddr_in *cliaddr);
void add_client(List connected_clients, wchar_t *username, struct sockaddr_in *cliaddr);

// Authenticate a client by checking their username and password
void authenticate_client(List connected_clients, List registered_users, int sockfd, wchar_t *buffer, struct sockaddr_in *cliaddr);

// Register a new user and add them to the list of connected clients
void register_client(List connected_clients, List registered_users, int sockfd, wchar_t *buffer, struct sockaddr_in *cliaddr);

// Remove a client from the list of connected clients
void exit_client(List connected_clients, int sockfd, struct sockaddr_in *cliaddr);

#endif // userAuthentication_h