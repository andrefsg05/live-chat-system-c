#include "serverFile_transfer.h"
#include "server.h"
#include "serverMessages.h"
#include "userAuthentication.h"


/**
 * Handles a file send request from a client.
 *
 * Parses the file send request, validates its contents, and forwards a file transfer warning
 * to all connected clients except the sender. 
 * Updates the chat status of each recipient to indicate they are receiving a file.
 *
 * @param connected_clients List of currently connected clients.
 * @param sockfd Socket file descriptor for sending packets.
 * @param sender_addr Address of the client sending the request.
 * @param rest The rest of the packet containing the file details.
 */
void handle_file_send_request_server(List connected_clients, int sockfd, struct sockaddr_in *sender_addr, wchar_t *rest) {
    wchar_t *context;
    wchar_t *filename = wcstok(rest, L"|", &context);
    wchar_t *filesize_str = wcstok(NULL, L"|", &context);
    wchar_t *ip = wcstok(NULL, L"|", &context);
    wchar_t *tcp_port_str = wcstok(NULL, L"|", &context);

    // Validate packet contents
    if (!filename || !filesize_str || !ip || !tcp_port_str) {
        wprintf(L"Invalid FILE_SEND_REQUEST packet format.\n");
        return;
    }

    // Find the sender in the connected clients list
    Client *sender = find_client_by_address(connected_clients, sender_addr);
    if (!sender) {
        wprintf(L"Sender not found in connected clients.\n"); 
        return;
    }

    // Prepare the notification packet to forward to other clients
    wchar_t notify_packet[MAX_LINE];
    swprintf(notify_packet, MAX_LINE, L"%d|%ls|%ls|%ls|%ls|%ls", FILE_SEND_WARNING, filename, filesize_str, ip, tcp_port_str, sender->username);

    // Forward to all connected clients except the sender
    Position pos = firstList(connected_clients);
    while (pos != NULL) {
        Client *dest = (Client *)retrieveElementList(pos);

        // Only notify clients who are not the sender and are idle
        if (wcscmp(dest->username, sender->username) != 0 && dest->is_chating == IDLE) {
            dest->is_chating = RECEIVING_FILE; // Mark as receiving file
            send_packet(sockfd, &dest->clientAddress, notify_packet);
        }
        pos = advanceList(pos);
    }
}


/**
 * Handles FILE_SEND_ACCEPT and FILE_SEND_DECLINE responses from clients.
 *
 * Parses the accept/decline packet, finds the receiver in the client list, and resets their chat status.
 *
 * @param connected_clients List of currently connected clients.
 * @param sockfd Socket file descriptor for sending packets.
 * @param cliaddr Address of the client sending the accept/decline.
 * @param type Packet type (accept or decline).
 * @param rest The rest of the packet containing the file name.
 */
void handle_file_send_accept_decline(List connected_clients, int sockfd, struct sockaddr_in *cliaddr, PacketType type, wchar_t *rest) {

    wchar_t *context;
    wchar_t *filename = wcstok(rest, L"|", &context);
    if (!filename) {
        wprintf(L"Invalid FILE_SEND_ACCEPT/DECLINE packet format.\n");
        return;
    }

    // Find receiver and sender
    Client *cli = find_client_by_address(connected_clients, cliaddr);
    if (!cli) {
        wprintf(L"Receiver not found in connected clients.\n");
        return;
    }

    cli->is_chating = IDLE; // Reset chat status
}