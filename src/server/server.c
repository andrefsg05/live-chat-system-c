#include "server.h"
#include "userRegister.h"
#include "userAuthentication.h"
#include "groups.h"
#include "serverGroups.h"
#include "serverMessages.h"
#include "serverFile_transfer.h"

#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <time.h> 
#include <locale.h>
#include <sys/select.h> 


int main(int argc, char const *argv[]) {

    setlocale(LC_ALL, "");

    List clientList = createList(); // List for connected clients
    List registeredUsers = load_users(); // List for registered users
    List registeredGroups = load_groups(); // List for groups

    int sockfd;

    wchar_t wbuffer[MAX_LINE] = {0};
    struct sockaddr_in servaddr, cliaddr;

    // Create socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Initialize server address structure
    create_address(&servaddr);
    
    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is connected. Waiting on port %d...\n", PORT);
    
    time_t last_check = time(NULL);

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        struct timeval tv;
        tv.tv_sec = 1;  // Wait up to 1 second for data
        tv.tv_usec = 0;

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
            // Receive packets from clients
            socklen_t len = sizeof(cliaddr);
            int bytes_received = recvfrom(sockfd, wbuffer, WCHAR_BUFFER_SIZE, 0, (struct sockaddr *) &cliaddr, &len);
            if (bytes_received < 0) {
                perror("recvfrom failed");
            }
            printf("%s:%d -> %ls\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), wbuffer);

            handle_packet(wbuffer, &cliaddr, sockfd, clientList, registeredUsers, registeredGroups);
        }

        // Check for inactive clients every CLIENT_TIMEOUT_CHECK_INTERVAL seconds
        if (time(NULL) - last_check > CLIENT_TIMEOUT_CHECK_INTERVAL) {
            check_inactive_clients(clientList, sockfd);
            last_check = time(NULL);
        }
    }

    // Close the socket
    close(sockfd);
    printf("Server shutting down...\n");

    deleteList(clientList);
    return 0;
}


/**
 * Initializes a server socket address structure to listen on a specific port.
 *
 * @param address Pointer to a sockaddr_in struct to initialize.
 */
void create_address(struct sockaddr_in *address) {
    // Initialize the address structure to zero
    memset(address, 0, sizeof(*address));

    // Set the address family, port, and IP address
    address->sin_family = AF_INET;  // IPv4
    address->sin_port = htons(PORT);  // Port number in network byte order
    address->sin_addr.s_addr = INADDR_ANY;  // Accept connections from any IP address
}


/**
 * Checks for clients who have not sent a heartbeat within the allowed time.
 * Removes inactive clients from the connected clients list.
 *
 * @param connected_clients List of connected clients.
 * @param sockfd Socket file descriptor used to notify client of removal.
 */
void check_inactive_clients(List connected_clients, int sockfd) {
    if (connected_clients == NULL) {
        printf("Invalid client list.\n");
        return;
    }

    Position P = firstList(connected_clients);
    while (P != NULL) {
        Client *client = (Client *)retrieveElementList(P);
        // Clients exceeding MAX_INACTIVE_TIME are disconnected.
        if (client && time(NULL) - client->last_heartbeat > MAX_INACTIVE_TIME) {
            printf("[Heartbeat] Client %ls removed due to inactivity.\n", client->username);
            P = advanceList(P);
            exit_client(connected_clients, sockfd, &client->clientAddress);
            continue; // Skip to next client after removing
        }
        P = advanceList(P);
    }
}


/**
 * Sends a wide-character message to a specified client via UDP.
 *
 * @param sockfd Socket file descriptor.
 * @param dest_addr Pointer to the destination client address.
 * @param packet Wide-character message to send.
 */
void send_packet (int sockfd, struct sockaddr_in *dest_addr, wchar_t *packet) {
    int packet_len = wcslen(packet);
    packet[packet_len] = L'\0';  

    int sent = sendto(
        sockfd,
        packet,
        (packet_len + 1) * sizeof(wchar_t),  // Sends the text + L'\0', in bytes
        0,
        (struct sockaddr *)dest_addr,
        sizeof(*dest_addr)
    );

    if (sent < 0) {
        perror("Error sending packet");
    }
}


/**
 * Sends a message from the server to all connected clients.
 *
 * @param connected_clients List of connected clients.
 * @param sockfd Socket file descriptor.
 * @param message Wide-character message to broadcast.
 */
void server_broadcast(List connected_clients, int sockfd, wchar_t *message) {

    if (connected_clients == NULL) {
        wprintf(L"Invalid client list.\n");
        return;
    }

    Position P = firstList(connected_clients);
    int count = 0;

    // Iterates over all clients and calls send_packet to broadcast.
    while (P != NULL) {
        Client *client = (Client *)retrieveElementList(P);
        if (client != NULL) {
            send_packet(sockfd, &client->clientAddress, message);
            count++;
        }
        P = advanceList(P);
    }
}


/**
 * Handles heartbeat messages from clients.
 * Updates the client's last heartbeat timestamp and replies with a heartbeat acknowledgment.
 *
 * @param connected_clients List of connected clients.
 * @param client_addr Address of the client sending the heartbeat.
 * @param sockfd Socket file descriptor for sending ACK.
 */
void handle_heartbeat(List connected_clients, struct sockaddr_in *client_addr, int sockfd) {

    if (connected_clients == NULL) {
        printf("Invalid client list.\n");
        return;
    }

    // Checks if client exists.
    Client *client = find_client_by_address(connected_clients, client_addr);
    if (client == NULL) {
        printf(" Client not found.\n");
        return;
    }

    // Update the last heartbeat time
    client->last_heartbeat = time(NULL);

    // Send an ACK back to the client 
    wchar_t message[MAX_LINE] = {0};
    swprintf(message, MAX_LINE, L"%d", HEARTBEAT);
    send_packet(sockfd, client_addr, message);

    printf("[Heartbeat] Client %ls is alive.\n", client->username);
}


/*****************************************************************************************************************/

//                              HANDLE PACKET

/*****************************************************************************************************************/


/**
 * Parses and routes incoming packets based on their type.
 * Handles authentication, messaging, group management, heartbeat, and file transfer.
 *
 * @param packet The received wide-character packet string.
 * @param cliaddr Address of the client that sent the packet.
 * @param sockfd Socket file descriptor.
 * @param connected_clients List of connected clients.
 * @param registered_users List of registered users.
 * @param registered_groups List of registered groups.
 */
void handle_packet(wchar_t *packet,
                   struct sockaddr_in *cliaddr, 
                   int sockfd,
                   List connected_clients,
                   List registered_users,
                   List registered_groups) {
                    
    // Parse the packet type
    if (packet == NULL || wcslen(packet) == 0) {
        wprintf(L"Empty packet received.\n");
        return;
    }
    wchar_t *context;
    wchar_t *type_str = wcstok(packet, L"|", &context);
    PacketType type = (PacketType)wcstol(type_str, NULL, 10);

    // The rest of the packet is in 'context'
    wchar_t *rest = context;

    switch(type) {
        // Authentication management
        case LOGIN: 
            authenticate_client(connected_clients, registered_users, sockfd, rest, cliaddr); 
            break;
        case REGISTER: 
            register_client(connected_clients, registered_users, sockfd, rest, cliaddr); 
            break;
        case EXIT: 
            exit_client(connected_clients, sockfd, cliaddr); 
            break;
        // Messages management
        case MSG_BROADCAST_REQUEST:
            start_broadcast(connected_clients, cliaddr, sockfd);
            break;
        case MSG_ONE_REQUEST: 
            start_message_one(connected_clients, cliaddr, sockfd, rest);
            break;
        case MSG_GROUP_REQUEST:
            start_message_group(registered_groups, connected_clients, cliaddr, sockfd, rest);
            break;
        case MSG_BROADCAST: 
            send_message_many(connected_clients, cliaddr, sockfd, type, rest);
            break;
        case MSG_ONE:
            send_message_one(connected_clients, sockfd, cliaddr, rest);
            break;
        case MSG_GROUP:
            send_message_many(connected_clients, cliaddr, sockfd, type, rest);
            break;
        case MSG_END: 
            message_end(connected_clients, cliaddr, sockfd);
            break;
        case MSG_NACK:
            nack_received(connected_clients, cliaddr, sockfd, rest);
            break;
        // Groups management
        case GROUP_CREATE:
            server_create_group(registered_groups, connected_clients, cliaddr, sockfd, rest);
            break;
        case GROUP_INVITE:
            server_invite_to_group(registered_groups, registered_users, connected_clients, cliaddr, sockfd, rest);
            break;
        case GROUP_KICK:
            server_kick_from_group(registered_groups, connected_clients, cliaddr, sockfd, rest);
            break;
        case GROUP_LEAVE:
            server_leave_from_group(registered_groups, connected_clients, cliaddr, sockfd, rest);
            break;
        case GROUP_DELETE:
            server_delete_group(registered_groups, connected_clients, cliaddr, sockfd, rest);
            break;
        // Heartbeat management
        case HEARTBEAT:
            handle_heartbeat(connected_clients, cliaddr, sockfd);
            break;
        // File transfer management
        case FILE_SEND_REQUEST:
            handle_file_send_request_server(connected_clients, sockfd, cliaddr, rest);
            break;
        case FILE_SEND_ACCEPT:
        case FILE_SEND_DECLINE:
            handle_file_send_accept_decline(connected_clients, sockfd, cliaddr, type, rest);
            break;
        default:
            wprintf(L"Unknown packet type: %d\n", type);
            return;
    }
}
