#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serverMessages.h"
#include "groups.h"
#include "userAuthentication.h"


/*********************************************************************************************************/
/*                                               START MSG                                               */
/*********************************************************************************************************/


/**
 * Starts a broadcast message to all connected clients available for chatting.
 * This function checks if the sender is valid and then iterates through the list of connected clients.
 * It sends a broadcast warning message to all clients that are not currently in a chat.
 * The sender is also marked as chatting, and a confirmation message is sent back to the sender.
 *
 * @param connected_clients List of connected clients.
 * @param sender_addr The address of the sender client.
 * @param sockfd The socket file descriptor for sending messages.
 *
 */
void start_broadcast(List connected_clients, struct sockaddr_in *sender_addr, int sockfd) {
    if (connected_clients == NULL) {
        wprintf(L"Invalid client list.\n");
        return;
    }

    Client *sender = find_client_by_address(connected_clients, sender_addr);
    if (sender == NULL) {
        wprintf(L"Sender client not found.\n");
        return;
    }

    wchar_t* sender_username = sender->username;
    wchar_t message[MAX_LINE] = {0}; 
    swprintf(message, MAX_LINE, L"%d|%ls", MSG_BROADCAST_WARNING, sender_username);


    // Iterate through the list of connected clients and send the message to each client
    Position P = firstList(connected_clients);  
    while (P != NULL) {
        Client *client = (Client *)retrieveElementList(P);
        if (client != NULL && client->is_chating == IDLE && client != sender) {

            // Mark the client as chatting
            client->is_chating = RECEIVING_MSG; // Set the client to chatting state
            client->chatPartner = sender; // Set the chat partner to the sender
            
            send_packet(sockfd, &client->clientAddress, message);
        }
        P = advanceList(P);
    }

    // Mark the sender as chatting
    sender->is_chating = SENDING_MSG; // Set the sender to chatting state
    sender->chatPartner = NULL; // Set the chat partner to NULL (broadcasting)

    // Send confirmation to the sender
    wchar_t sender_confirmation[MAX_LINE] = {0};
    swprintf(sender_confirmation, MAX_LINE, L"%d", MSG_OK);
    send_packet(sockfd, sender_addr, sender_confirmation);
}


/**
 * Starts a one-to-one message between the sender and the recipient.
 * This function checks if the recipient is available for chatting.
 * If the recipient is already in a chat, it sends an error message to the sender.
 * If the recipient is available, it sets both clients to a chatting state and sends confirmation messages.
 *
 * @param connected_clients List of connected clients.
 * @param sender_addr The address of the sender client.
 * @param sockfd The socket file descriptor for sending messages.
 * @param recipient_username The username of the recipient client.
 */
void start_message_one(List connected_clients, struct sockaddr_in *sender_addr, int sockfd, wchar_t *recipient_username) {
    if (connected_clients == NULL) {
        wprintf(L"Invalid client list.\n");
        return;
    }


    Client *sender = find_client_by_address(connected_clients, sender_addr);
    if (sender == NULL) {
        wprintf(L"Sender client not found.\n");
        return;
    }


    Client *recipient = find_client_by_username(connected_clients, recipient_username);
    if (recipient == NULL) {
        wchar_t error_message[MAX_LINE] = {0};
        swprintf(error_message, MAX_LINE, L"%d|Recipient %ls not found.", MSG_NOK, recipient_username);
        send_packet(sockfd, &sender->clientAddress, error_message);
        wprintf(L"Recipient %ls not found.\n", recipient_username);
        return;
    }

    if (sender == recipient) {
        wchar_t error_message[MAX_LINE] = {0};
        swprintf(error_message, MAX_LINE, L"%d|You cannot message yourself.", MSG_NOK);
        send_packet(sockfd, &sender->clientAddress, error_message);
        wprintf(L"Sender is trying to start a conversation with themselves.\n");
        return;
    }


    if(recipient->is_chating != IDLE) {
        wchar_t error_message[MAX_LINE] = {0};
        swprintf(error_message, MAX_LINE, L"%d|Recipient is already chatting.", MSG_NOK);
        send_packet(sockfd, &sender->clientAddress, error_message);
        return;
    }
    
    // Set both clients to chatting state
    sender->is_chating = SENDING_MSG;
    sender->chatPartner = recipient;

    recipient->is_chating = RECEIVING_MSG;
    recipient->chatPartner = sender;

    // Send confirmation to sender
    wchar_t recipientConfirmation[MAX_LINE] = {0};
    swprintf(recipientConfirmation, MAX_LINE, L"%d", MSG_OK);
    send_packet(sockfd, &sender->clientAddress, recipientConfirmation);

    // Send warning to recipient
    wchar_t senderConfirmation[MAX_LINE] = {0};
    swprintf(senderConfirmation, MAX_LINE, L"%d|%ls", MSG_ONE_WARNING, sender->username);
    send_packet(sockfd, &recipient->clientAddress, senderConfirmation);
}


/**
 * Starts a group message to all clients in a specified group.
 * This function checks if the group exists and if the sender is part of the group.
 * It sends a warning message to all clients in the group that are available for chatting.
 * The sender is also marked as chatting, and a confirmation message is sent back to the sender.
 *
 * @param registered_groups List of registered groups.
 * @param connected_clients List of connected clients.
 * @param sender_addr The address of the sender client.
 * @param sockfd The socket file descriptor for sending messages.
 * @param group_name The name of the group to send the message to.
 */
void start_message_group(List registered_groups, List connected_clients, struct sockaddr_in *sender_addr, int sockfd, wchar_t *group_name) {
    
    wchar_t sender_confirmation[MAX_LINE] = {0};

    if (connected_clients == NULL || registered_groups == NULL) {
        wprintf(L"Invalid lists.\n");
        return;
    }

    Client *sender = find_client_by_address(connected_clients, sender_addr);
    if (sender == NULL) {
        wprintf(L"Sender client not found.\n");
        return;
    }

    Group *group = find_group_by_name(registered_groups, group_name);
    if (group == NULL) {
        wprintf(L"Group does not exist.\n");
        swprintf(sender_confirmation, MAX_LINE, L"%d|Group does not exist.", MSG_NOK);
        send_packet(sockfd, sender_addr, sender_confirmation);
        return;
    }

    bool is_ok = false;

    // Check if the sender is part of the group
    Position posGroup = firstList(group->members);
    while (posGroup != NULL) {
        wchar_t *client_username = (wchar_t *)retrieveElementList(posGroup);
        if (wcscmp(client_username, sender->username) == 0) {
            // The sender is part of the group
            is_ok = true;
            break;
        }
        posGroup = advanceList(posGroup);

    }

    if (!is_ok) {
        wprintf(L"Sender is not a member of the group.\n");
        swprintf(sender_confirmation, MAX_LINE, L"%d|You are not a member of group %ls.", MSG_NOK, group_name);
        send_packet(sockfd, sender_addr, sender_confirmation);
        return;
    }

    is_ok = false; // Reset is_ok for sending messages to group members

    wchar_t* sender_username = sender->username;
    wchar_t message[MAX_LINE] = {0}; 
    swprintf(message, MAX_LINE, L"%d|%ls|%ls", MSG_GROUP_WARNING, group_name, sender_username);

    // Iterate through the list of clients in the group and send the message to each client
    posGroup = firstList(group->members);
    while (posGroup != NULL) {
        wchar_t *client_username = (wchar_t *)retrieveElementList(posGroup);
        Client *client = find_client_by_username(connected_clients, client_username);
        if (client != NULL && client->is_chating == IDLE && client != sender) {
            // Mark the client as chatting
            client->is_chating = RECEIVING_MSG; // Set the client to chatting state
            client->chatPartner = sender; // Set the chat partner to the sender
            
            is_ok = true;
            send_packet(sockfd, &client->clientAddress, message);
        }

        posGroup = advanceList(posGroup);

    }

    if (!is_ok) {
        swprintf(sender_confirmation, MAX_LINE, L"%d|No clients in group %ls are available for chat.", MSG_NOK, group_name);
        wprintf(sender_confirmation);
        send_packet(sockfd, sender_addr, sender_confirmation);
        return;
    }

    // Mark the sender as chatting
    sender->is_chating = SENDING_MSG; // Set the sender to chatting state
    sender->chatPartner = NULL; // Set the chat partner to NULL

    // Send confirmation to the sender
    swprintf(sender_confirmation, MAX_LINE, L"%d", MSG_OK);
    send_packet(sockfd, sender_addr, sender_confirmation);
}


/*********************************************************************************************************/
/*                                               SEND MSG                                                */
/*********************************************************************************************************/


/**
 * Sends a message to the chat partner of the sender.
 * This function checks if the sender is valid and if they are currently in a chat.
 * Checks the sequence number of the message to ensure it is in order.
 * - If the sequence number is less than the expected sequence, it ignores the message.
 * - If the sequence number matches the expected sequence, it sends the message to the chat partner
 *   and updates the expected sequence number.
 * - If the sequence number is greater than the expected sequence, it buffers the message and sends 
 *   a NACK.
 *
 * @param connected_clients List of connected clients.
 * @param sender_addr The address of the sender client.
 * @param sockfd The socket file descriptor for sending messages.
 * @param message The message to be sent.
 */
void send_message_one(List connected_clients, int sockfd, struct sockaddr_in *sender_addr, wchar_t *message) {
    wchar_t *context;
    wchar_t *seq_str = wcstok(message, L"|", &context);
    wchar_t *letter_str = wcstok(NULL, L"|", &context);

    // Check if seq_str and letter_str are not NULL
    if (!seq_str || !letter_str) 
        return;


    wchar_t letter = letter_str[0];
    int seq = wcstol(seq_str, NULL, 10);

    Client *sender_client = find_client_by_address(connected_clients, sender_addr);
    if (sender_client == NULL) 
        return;

    // Check if the sender is currently in a chat
    Client *receiver_client = sender_client->chatPartner;
    if (!receiver_client || receiver_client->is_chating != RECEIVING_MSG)  
        return;


    if (seq < sender_client->expected_seq) {
        // Duplicate packet, ignore 
        // It has already been processed
        return;
    }

    // Insert the message into the sender's sent message buffer
    SeqBuffer *sentMessage = malloc(sizeof(SeqBuffer));
    sentMessage->seq = seq;
    sentMessage->data = letter;
    insert(sentMessage, sender_client->sentMessageBuffer, headerList(sender_client->sentMessageBuffer));


    if (seq == sender_client->expected_seq) {
        wchar_t msg[MAX_LINE] = {0};
        swprintf(msg, MAX_LINE, L"%d|%d|%lc", MSG_ONE, seq, letter);
        send_packet(sockfd, &receiver_client->clientAddress, msg);
        sender_client->expected_seq++;

        // Deliver buffered messages (if any)
        wchar_t buffered;
        while (buffer_pop(sender_client->orderBuffer, sender_client->expected_seq, &buffered)) {
            swprintf(msg, MAX_LINE, L"%d|%d|%lc", MSG_ONE, sender_client->expected_seq, buffered);
            send_packet(sockfd, &receiver_client->clientAddress, msg);
            sender_client->expected_seq++;
        }
    } else if (seq > sender_client->expected_seq) {

        // Out of order, store and send NACK
        buffer_insert(sender_client->orderBuffer, seq, letter);
        wchar_t nack[MAX_LINE];
        swprintf(nack, MAX_LINE, L"%d|%d", MSG_NACK, sender_client->expected_seq);
        send_packet(sockfd, sender_addr, nack);
    }
}


/**
 * Sends a message to all clients that are currently in a chat with the sender.
 * This function checks if the sender is valid and if they are currently in a chat.
 * It also checks the sequence number of the message to ensure it is in order.
 * - If the sequence number is less than the expected sequence, it ignores the message.
 * - If the sequence number matches the expected sequence, it sends the message to all clients
 *   that are chatting with the sender and updates the expected sequence number.
 * - If the sequence number is greater than the expected sequence, it buffers the message and sends 
 *   a NACK.
 *
 * @param connected_clients List of connected clients.
 * @param sender_addr The address of the sender client.
 * @param sockfd The socket file descriptor for sending messages.
 * @param packet_type The type of packet being sent (e.g., MSG_MANY).
 * @param rest The rest of the message containing sequence number and letter.
 */
void send_message_many(List connected_clients, struct sockaddr_in *sender_addr, int sockfd, PacketType packet_type, wchar_t *rest) {
    wchar_t *context;
    wchar_t *seq_str = wcstok(rest, L"|", &context);
    wchar_t *letter_str = wcstok(NULL, L"|", &context);
    if (!seq_str || !letter_str) return;
    wchar_t letter = letter_str[0];

    int seq = wcstol(seq_str, NULL, 10);

    Client *sender = find_client_by_address(connected_clients, sender_addr);
    if (sender == NULL) {
        wprintf(L"Sender client not found.\n");
        return;
    }

    // Check if the sender is currently in a chat
    if (sender->chatPartner != NULL) {
        wprintf(L"Sender is already in a chat.\n");
        return;
    }

    // Check if duplicate packet
    if (seq < sender->expected_seq) {
        return;
    }


    // Insert the message into the sender's sent message buffer
    SeqBuffer *sentMessage = malloc(sizeof(SeqBuffer));
    sentMessage->seq = seq;
    sentMessage->data = letter;
    insert(sentMessage, sender->sentMessageBuffer, headerList(sender->sentMessageBuffer));


    // If expected sequence number matches, send the message to all clients
    if (seq == sender->expected_seq) {
        wchar_t msg[MAX_LINE];
        swprintf(msg, MAX_LINE, L"%d|%d|%lc", packet_type, seq, letter);

        Position P = firstList(connected_clients);
        while (P != NULL) {
            Client *client = (Client *)retrieveElementList(P);
            if (client && client->is_chating == RECEIVING_MSG && client->chatPartner == sender) {
                send_packet(sockfd, &client->clientAddress, msg);
            }
            P = advanceList(P);
        }
        sender->expected_seq++;

        // Deliver buffered messages if any
        wchar_t buffered;
        while (buffer_pop(sender->orderBuffer, sender->expected_seq, &buffered)) {
            swprintf(msg, MAX_LINE, L"%d|%d|%lc", packet_type, sender->expected_seq, buffered);
            P = firstList(connected_clients);
            while (P != NULL) {
                Client *client = (Client *)retrieveElementList(P);
                if (client && client->is_chating == RECEIVING_MSG && client->chatPartner == sender) {
                    send_packet(sockfd, &client->clientAddress, msg);
                }
                P = advanceList(P);
            }
            sender->expected_seq++;
        }

    // If the sequence number is greater than expected (out of order), store the message in buffer and send NACK
    } else if (seq > sender->expected_seq) {
        buffer_insert(sender->orderBuffer, seq, letter);
        wchar_t nack[MAX_LINE];
        swprintf(nack, MAX_LINE, L"%d|%d", MSG_NACK, sender->expected_seq);
        send_packet(sockfd, sender_addr, nack);
    }
}


/*********************************************************************************************************/
/*                                               END MSG                                                 */
/*********************************************************************************************************/


/**
 * Ends the chat session for all clients that were chatting with the sender.
 * This function resets the chat state of the sender and all clients that were in a chat with the sender.
 * It sends an end message to all clients that were chatting with the sender.
 * It also resets the expected sequence number for the sender.
 *
 * @param connected_clients List of connected clients.
 * @param sender_addr The address of the sender client.
 * @param sockfd The socket file descriptor for sending messages.
 */
void message_end(List connected_clients, struct sockaddr_in *sender_addr, int sockfd) {
    if (connected_clients == NULL) {
        wprintf(L"Invalid client list.\n");
        return;
    }

    Client *sender = find_client_by_address(connected_clients, sender_addr);
    if (sender == NULL) {
        wprintf(L"Sender client not found.\n");
        return;
    }

    Position P = firstList(connected_clients);
    while (P != NULL) {
        Client *client = (Client *)retrieveElementList(P);
        if (client != NULL && client->is_chating == RECEIVING_MSG && client->chatPartner == sender) {
            // Reset the client's chat state 
            client->is_chating = IDLE;
            client->chatPartner = NULL;

            // Send end message to the client
            wchar_t end_message[MAX_LINE] = {0};
            swprintf(end_message, MAX_LINE, L"%d", MSG_END);
            send_packet(sockfd, &client->clientAddress, end_message);

        }
        P = advanceList(P);
    }

    // Delete the sender's message buffer
    P = firstList(sender->sentMessageBuffer);
    while (P != NULL) {
        SeqBuffer *sentMessage = (SeqBuffer *)retrieveElementList(P);
        P = advanceList(P);
        delete(sentMessage, sender->sentMessageBuffer); // Remove from the list
        free(sentMessage); // Free each SentMessage in the sent message buffer
    }

    // Reset the sender's chat state
    sender->is_chating = IDLE;
    sender->chatPartner = NULL;
    sender->expected_seq = 0; // Reset expected sequence number
}


/*********************************************************************************************************/
/*                                               NACKS                                                   */
/*********************************************************************************************************/


/**
 * Handles the NACK received from a client.
 * This function checks if the sender is valid and if they are currently in a chat.
 * If the sender is in a chat, it searches for the missing packet in the sender's sent message buffer
 * and sends it to the receiver.
 *
 * @param connected_clients List of connected clients.
 * @param sender_addr The address of the sender client.
 * @param sockfd The socket file descriptor for sending messages.
 * @param rest The rest of the message containing the missing sequence number.
 */
void nack_received(List connected_clients, struct sockaddr_in *sender_addr, int sockfd, wchar_t *rest) {

    wchar_t packet[MAX_LINE] = {0};

    if (connected_clients == NULL) {
        wprintf(L"Invalid client list.\n");
        return;
    }

    Client *receiver = find_client_by_address(connected_clients, sender_addr);
    if (receiver == NULL) {
        wprintf(L"Sender client not found.\n");
        return;
    }

    // Check if the receiver is currently in a chat
    Client *sender = receiver->chatPartner;
    if (sender == NULL || sender->is_chating != SENDING_MSG) {
        wprintf(L"Sender client is not in a chat.\n");
        return;
    }

    int missing_seq = wcstol(rest, NULL, 10);

    // Check if the sender has the missing packet in their sent message buffer
    Position P = firstList(sender->sentMessageBuffer);
    while (P != NULL) {
        SeqBuffer *sentMessage = (SeqBuffer *)retrieveElementList(P);
        if (sentMessage->seq == missing_seq) {
            // Found the missing packet, send it to the receiver
            swprintf(packet, MAX_LINE, L"%d|%d|%lc", MSG_ONE, sentMessage->seq, sentMessage->data);
            send_packet(sockfd, &receiver->clientAddress, packet);
            return; // Exit after sending the missing packet
        }
        P = advanceList(P);
    }

}


/*********************************************************************************************************/
/*                                               AUX                                                     */
/*********************************************************************************************************/


/**
 * Function to insert a new packet into the Client's order buffer.
 *
 * Inserts a new packet into the buffer at the specified sequence number.
 * This function allocates memory for a new SeqBuffer packet, sets its sequence and data,
 * and inserts it into the buffer at the specified position.
 *
 * @param buffer The buffer to insert the packet into.
 * @param seq The sequence number of the packet.
 * @param data The data to be stored in the packet.
 */
void buffer_insert(List buffer, int seq, wchar_t data) {
    SeqBuffer *new_packet = malloc(sizeof(SeqBuffer));
    new_packet->seq = seq;
    new_packet->data = data;
    insert(new_packet, buffer, headerList(buffer));
}


/**
 * Function to pop a packet from the Client's order buffer.
 *
 * Searches for a packet with the specified sequence number in the buffer.
 * If found, it retrieves the data, deletes the packet from the buffer, and frees the memory.
 * Returns true if the packet was found and popped, false otherwise.
 *
 * @param buffer The buffer to pop the packet from.
 * @param seq The sequence number of the packet to pop.
 * @param out Pointer to store the data of the popped packet.
 * @return true if the packet was found and popped
           false otherwise.
 */
bool buffer_pop(List buffer, int seq, wchar_t *out) {
    Position curr = firstList(buffer);
    while (curr != NULL) {
        SeqBuffer *packet = (SeqBuffer *)retrieveElementList(curr);
        if (packet->seq == seq) {
            *out = packet->data;
            delete(packet, buffer);
            free(packet);
            return true;
        }
        curr = advanceList(curr);
    }
    return false;
}
