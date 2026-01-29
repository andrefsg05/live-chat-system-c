#include "userAuthentication.h"
#include "userRegister.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * Search for a client in the list by its username.
 * 
 * @param clients The list of clients.
 * @param username The username of the client to search for.
 * @return A pointer to the client structure if found, NULL otherwise.
 */
Client* find_client_by_username(List clients, wchar_t *username) {
    Position pos = firstList(clients);
    while (pos != NULL) {
        Client *client = (Client *)retrieveElementList(pos);
        if (wcscmp(client->username, username) == 0) {
            return client; // Return the client if the username matches
        }
        pos = advanceList(pos);
    }
    return NULL; // Client not found
}


/**
 * Search for a client in the list by its address.
 * 
 * @param clients The list of clients.
 * @param cliaddr The address of the client to search for.
 * @return A pointer to the Client structure if found, NULL otherwise.
 */
Client* find_client_by_address(List connected_clients, struct sockaddr_in *cliaddr) {
    Position pos = firstList(connected_clients); // Get the first position in the list

    const char* ip_str = inet_ntoa(cliaddr->sin_addr); // Convert IP address to string
    const int port = ntohs(cliaddr->sin_port); // Get the port number

    while (pos != NULL) {
        Client *client = (Client *)retrieveElementList(pos);
        if (strcmp(client->ip, ip_str) == 0 && client->port == port) {
            return client;
        }
        pos = advanceList(pos);
    }
    return NULL; // Client not found
}


/**
 * Add a new client to the list of connected clients.
 * 
 * @param clients The list of connected clients.
 * @param username The username of the client.
 * @param cliaddr The address of the client.
*/
void add_client(List connected_clients, wchar_t *username, struct sockaddr_in *cliaddr) {
    // Allocate memory for a new client
    Client *new_client = (Client *)malloc(sizeof(Client));
    if (new_client == NULL) {
        perror("Out of memory!");
        exit(EXIT_FAILURE);
    }
    
    // Populate the client structure
    wcscpy(new_client->username, username);
    strcpy(new_client->ip, inet_ntoa(cliaddr->sin_addr));
    new_client->port = ntohs(cliaddr->sin_port);
    new_client->last_heartbeat = time(NULL);
    new_client->clientAddress = *cliaddr;
    new_client->is_chating = IDLE;
    new_client->chatPartner = NULL;
    new_client->expected_seq = 0;
    new_client->orderBuffer = createList();
    new_client->sentMessageBuffer = createList();

    // Insert the client into the linked list
    insert(new_client, connected_clients, headerList(connected_clients));
}


/**
 * Register a new user and add them to the list of connected clients.
 * 
 * @param connected_clients The list of connected clients.
 * @param registered_users The list of registered users.
 * @param sockfd The socket file descriptor.
 * @param buffer The received packet data.
 * @param cliaddr The address of the client.
*/
void register_client(List connected_clients, List registered_users, int sockfd, wchar_t *buffer, struct sockaddr_in *cliaddr) {
    wchar_t *context;
    wchar_t *username = wcstok(buffer, L"|", &context);
    wchar_t *password = wcstok(NULL, L"|", &context);
    wchar_t conf_message[MAX_LINE] = {0};
    wchar_t warn_message[MAX_LINE] = {0};

    // Check if the client is already registered
    if (user_exists(registered_users, username) != 0) {
        swprintf(conf_message, MAX_LINE, L"%d", LOGIN_NOK, username);
        send_packet(sockfd, cliaddr, conf_message);
        wprintf(L"User already exists.\n");
        return;
    }

    create_user(username, password, registered_users);

    // After registering the user, add them to the connected clients list
    add_client(connected_clients, username, cliaddr);

    // Send confirmation message to the client
    swprintf(conf_message, MAX_LINE, L"%d", LOGIN_OK);
    send_packet(sockfd, cliaddr, conf_message);
    
    // Send notification message to all clients
    swprintf(warn_message, MAX_LINE, L"%d|User %ls has joined the chat.", SERVER_ANNOUNCEMENT, username);
    server_broadcast(connected_clients, sockfd, warn_message);    

}


/**
 * Authenticate a client by checking their username and password.
 * 
 * @param connected_clients The list of connected clients.
 * @param registered_users The list of registered users.
 * @param sockfd The socket file descriptor.
 * @param buffer The received packet data.
 * @param cliaddr The address of the client.
*/
void authenticate_client(List connected_clients, List registered_users, int sockfd, wchar_t *buffer, struct sockaddr_in *cliaddr) {
    wchar_t *context;
    wchar_t *username = wcstok(buffer, L"|", &context);
    wchar_t *password = wcstok(NULL, L"|", &context);
    wchar_t conf_message[MAX_LINE] = {0};
    wchar_t warn_message[MAX_LINE] = {0};
    
    // Check if the username is already logged in
    if (find_client_by_username(connected_clients, username) != NULL) {
        // Error message: already logged in
        swprintf(conf_message, MAX_LINE, L"%d", LOGIN_NOK);
        send_packet(sockfd, cliaddr, conf_message);
        printf("Client is already logged in.\n");
        return;
    }
    // Check if the username and password are valid
    if (authenticateUser(registered_users, username, password) != 1) {
        swprintf(conf_message, MAX_LINE, L"%d", LOGIN_NOK);
        send_packet(sockfd, cliaddr, conf_message);
        printf("Invalid credentials.\n");
        return;
    }

    // Add the client to the list of connected clients
    add_client(connected_clients, username, cliaddr);
    
    swprintf(conf_message, MAX_LINE, L"%d", LOGIN_OK);
    send_packet(sockfd, cliaddr, conf_message);

    swprintf(warn_message, MAX_LINE, L"%d|User %ls has joined the chat.", SERVER_ANNOUNCEMENT, username);
    server_broadcast(connected_clients, sockfd, warn_message);
}


/**
 * Exit a client from the server.
 * This function finds the client by its address in the list of connected clients,
 * deletes it from the list, and frees the allocated memory for the client.
 * 
 * @param clients The list of connected clients.
 * @param cliaddr The address of the client to exit.
 */
void exit_client(List connected_clients, int sockfd, struct sockaddr_in *cliaddr) {
    wchar_t conf_message[MAX_LINE] = {0};
    wchar_t warn_message[MAX_LINE] = {0};
    
    Client *client = find_client_by_address(connected_clients, cliaddr);

    if (client != NULL) {
        delete(client, connected_clients);

        // Free the memory allocated for the client's order buffer
        Position pos = firstList(client->orderBuffer);
        while (pos != NULL) {
            SeqBuffer *seqBuffer = (SeqBuffer *)retrieveElementList(pos);
            free(seqBuffer); // Free each SeqBuffer in the order buffer
            pos = advanceList(pos);
        }
        deleteList(client->orderBuffer); // Free the order buffer list

        // Free the memory allocated for the client's sent message buffer
        pos = firstList(client->sentMessageBuffer);
        while (pos != NULL) {
            SeqBuffer *sentMessage = (SeqBuffer *)retrieveElementList(pos);
            pos = advanceList(pos);
            free(sentMessage); // Free each SentMessage in the sent message buffer
        }
        deleteList(client->sentMessageBuffer); // Free the sent message buffer list

        // If the client was chatting with someone, reset their chat status
        if (client->is_chating == SENDING_MSG) {
            swprintf(warn_message, MAX_LINE, L"%d", MSG_END);
            Position pos = firstList(connected_clients);
            while (pos != NULL) {
                Client *other_client = (Client *)retrieveElementList(pos);
                if (other_client->chatPartner == client) {
                    other_client->is_chating = IDLE; // Reset chat status
                    other_client->chatPartner = NULL; // Reset chat partner
                    send_packet(sockfd, &other_client->clientAddress, warn_message); // Notify the chat partner
                }
                pos = advanceList(pos);
            }
        }
        
    
        // Send confirmation message to the client
        swprintf(conf_message, MAX_LINE, L"%d", LOGOUT_OK);
        send_packet(sockfd, cliaddr, conf_message);
        
        // Send notification message to all clients
        swprintf(warn_message, MAX_LINE, L"%d|Client %ls has exited.", SERVER_ANNOUNCEMENT, client->username);
        server_broadcast(connected_clients, sockfd, warn_message);    
        
        wprintf(L"Client %ls has exited.\n", client->username);
        free(client); // Free the memory allocated for the client
        return;
    }
    swprintf(conf_message, MAX_LINE, L"%d", LOGOUT_NOK);
    send_packet(sockfd, cliaddr, conf_message);
}

