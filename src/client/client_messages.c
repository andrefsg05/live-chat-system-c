#include <stdatomic.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>  
#include <stdlib.h>  
#include <unistd.h>   
#include <stdio.h>   

#include "client_messages.h"
#include "client_terminal.h"
#include "client_threads.h"


/**
 * Receives and displays messages from the server, handling ordering and retransmissions.
 *
 * This function manages the reception of messages, ensuring they are displayed in the correct order,
 * handling out-of-order packets using a buffer, and sending NACKs for missing packets.
 * It also handles special cases for broadcast, one-to-one, and group messages.
 *
 * @param mode The message mode (broadcast, one-to-one, group, etc.).
 */
void receive_message(int mode){
    int expected_seq_number = 0;
    int msg_number = 0;
    List seq_buffer = createList(); // Sequence buffer to keep track of received messages

    wchar_t received[MAX_LINE] = {0};
    wchar_t* seqnumber = NULL, *message = NULL, *rest = NULL;

    // Display the appropriate warning or prompt based on the message mode
    if (mode == MSG_BROADCAST_WARNING) {
        wprintf(L"\n%ls#broadcast: ", ThreadData.Warning);
        fflush(stdout);
    }
    else if (mode == MSG_ONE_WARNING) {
        wprintf(L"\n%ls: ", ThreadData.Warning);
        fflush(stdout);
    }
    else if (mode == MSG_GROUP_WARNING) {
        // Parse and display group and sender information
        wcpcpy(received, ThreadData.Warning);
        seqnumber = wcstok(received, L"|", &rest); 
        message = wcstok(NULL, L"|", &rest);
        wprintf(L"\n%ls@%ls: ", message, seqnumber );
        fflush(stdout);
        memset(received, 0, sizeof(received)); // Clear the received buffer
    }

    disable_input(); // Prevent user input while receiving messages
    
    // Main loop to process incoming message packets
    while (1) {
        if (atomic_load(&ThreadData.signalMessage)) {

            wcscpy(received, ThreadData.Message);
            seqnumber = wcstok(received, L"|", &rest);
            message = wcstok(NULL, L"|", &rest);

            // Check for malformed packets
            if(!seqnumber || !message){
                atomic_store(&ThreadData.signalMessage, 0);
                say_no_to_busy_waiting();
                continue;
            }
            msg_number = wcstol(seqnumber, NULL, 10);
            wchar_t letter = message[0];

            // If the expected packet arrives
            if(msg_number == expected_seq_number){

                if (letter == 127 || letter == 8) { // Handle backspace or delete
                    wprintf(L"\b \b");
                    fflush(stdout);
                }
                else {
                    wprintf(L"%lc", letter);
                    fflush(stdout);
                }
                expected_seq_number++;

                // Print any buffered packets that are now in order
                wchar_t buffered;
                while(buffer_pop(seq_buffer, expected_seq_number, &buffered)){
                    if (buffered == 127 || buffered == 8) {
                        wprintf(L"\b \b");
                        fflush(stdout);
                    } else {
                        wprintf(L"%lc", buffered);
                        fflush(stdout);
                    }
                    expected_seq_number++;
                }
                

            }
            // If an out-of-order packet arrives, buffer it and request missing packet
            else if(msg_number > expected_seq_number){ 
                buffer_insert(seq_buffer, msg_number, letter);
                wchar_t nack_packet[MAX_LINE] = {0};
                swprintf(nack_packet, MAX_LINE, L"%d|%d", MSG_NACK, expected_seq_number);
                send_packet(ThreadData.sockfd, &ThreadData.servaddr, nack_packet);
            }

            // Duplicate packets are ignored
            
            atomic_store(&ThreadData.signalMessage, 0);
            say_no_to_busy_waiting();
            
        }
        // If the end of the message is signaled, clean up and exit
        else if(atomic_load(&ThreadData.signalMessageEnd)) {
            wprintf(L"\n");
            fflush(stdout);
            atomic_store(&ThreadData.signalMessageEnd, 0);
            atomic_store(&ThreadData.signalWarning, 0); // Reset the warning signal
            break;
        }
    }            
    enable_input();

    // Free any remaining buffered packets
    Position curr = firstList(seq_buffer);
    while (curr != NULL) {
        SeqBuffer *packet = (SeqBuffer *)retrieveElementList(curr);
        free(packet);
        curr = advanceList(curr);
    }
    deleteList(seq_buffer); // Delete the sequence buffer list
}


/**
 * Handles user input and sends messages to the server, character by character.
 *
 * This function reads user input in raw mode, sends each character as a packet to the server,
 * and manages a NACK thread for retransmissions. It stops when the user presses Enter or the buffer is full.
 *
 * @param message_type The type of message being sent (broadcast, one-to-one, group, etc.).
 */
void send_messages(PacketType message_type) {
    wchar_t packet[MAX_LINE] = {0};
    int count = 0;

    NackThreadData *nack_thread_data = (NackThreadData*)malloc(sizeof(NackThreadData));

    atomic_store(&nack_thread_data->countedCharacters, count);
    nack_thread_data->packet_type = message_type;

    int sockfd = ThreadData.sockfd;
    struct sockaddr_in servaddr = ThreadData.servaddr; // Use the global server address
    
    // Start the NACK thread to handle retransmissions
    pthread_t nack_tid;
    pthread_create(&nack_tid, NULL, nack_thread, nack_thread_data);

    wprintf(L"\n> ");
    fflush(stdout);
    
    enable_raw_mode(); // Enable raw input mode for character-by-character reading
    int len = 0;
    while (count < MAX_LINE) { // Ensure we don't overflow the buffer
        
        wchar_t c = getwchar(); // Read a single character
        nack_thread_data->buffer[count] = c; // Store the character in the buffer

        if (c == L'\n') {
            // End of message, notify server
            swprintf(packet, MAX_LINE, L"%d", MSG_END);
            sleep(TIMEOUT_TIME); // Wait for a moment to ensure all packets are sent
            send_packet(sockfd, &servaddr, packet);
            
            wprintf(L"\nMessage sending stopped.\n");
            fflush(stdout);
            break;
        } else if ((c == 127 || c == 8)) {
            // Handle backspace/delete
            if (len > 0){
                wprintf(L"\b \b");
                len--;
                swprintf(packet, MAX_LINE, L"%d|%d|%lc", message_type, count, c);
                send_packet(sockfd, &servaddr, packet);
                fflush(stdout);
                count++;
            }
            
        }else {
            // Send the character to the server
            wprintf(L"%lc", c); // Display the character on the same line
            swprintf(packet, MAX_LINE, L"%d|%d|%lc", message_type, count, c);
            send_packet(sockfd, &servaddr, packet);
            fflush(stdout);  // Ensure the character is printed immediately
            len++;
            count++;
        }
        
        atomic_store(&nack_thread_data->countedCharacters, count);
    }

    // Disable raw mode and clean up
    disable_raw_mode();

    if (count >= MAX_LINE) {
        wprintf(L"\nMessage too long. Please limit to %d characters.\n", MAX_LINE);
        fflush(stdout);
    }

    pthread_cancel(nack_tid); // Cancel the NACK thread
    pthread_join(nack_tid, NULL); // Wait for the NACK thread to finish

    free(nack_thread_data); // Free the NackThreadData structure
}

/**
 * Sends a packet to the specified destination using UDP.
 *
 * This function sends a wide-character string packet to the given socket address using sendto.
 * It ensures the packet is null-terminated and handles errors.
 *
 * @param sockfd The socket file descriptor.
 * @param dest_addr Pointer to the destination sockaddr_in structure.
 * @param packet The wide-character packet to send.
 */
void send_packet (int sockfd, struct sockaddr_in *dest_addr, wchar_t *packet) {
    int packet_len = wcslen(packet);
    packet[packet_len] = L'\0'; // Ensure null-termination!

    int sent = sendto(
        sockfd,
        packet,
        (packet_len + 1) * sizeof(wchar_t),  // Send the text + L'\0', in bytes
        0,
        (struct sockaddr *)dest_addr,
        sizeof(*dest_addr)
    );

    if (sent < 0) {
        wprintf(L"Error sending packet");
    }
}


/**
 * Requests permission from the server to start a messaging session.
 *
 * This function sends a request to the server to start a message session (broadcast, one-to-one, or group).
 * It waits for the server's response, handling timeouts and displaying appropriate messages.
 *
 * @param message_type The type of message request (broadcast, one-to-one, group).
 * @param recipient_or_group The recipient username or group name (if applicable).
 * @return true if the request is accepted, false otherwise.
 */
bool request_message(PacketType message_type, wchar_t *recipient_or_group) {
    wchar_t packet[MAX_LINE] = {0};
    int answer = 0;

    // Build the request packet based on the message type
    if (message_type == MSG_BROADCAST_REQUEST && recipient_or_group == NULL){
        swprintf(packet, MAX_LINE, L"%d", message_type);
    }
    else if (message_type == MSG_ONE_REQUEST || message_type == MSG_GROUP_REQUEST) {
        swprintf(packet, MAX_LINE, L"%d|%ls", message_type, recipient_or_group);
    }
    else {
        return false;
    }

    atomic_store(&ThreadData.signalM_OK, 0); // Signal to wait for server acknowledgment
    send_packet(ThreadData.sockfd, &ThreadData.servaddr, packet);
    
    // Wait until the server acknowledges the request or timeout occurs
    int timeout_count = 0;
    while ((answer = atomic_load(&ThreadData.signalM_OK)) == 0) {
        if (timeout_count >= MAX_TIMEOUT) {
            wprintf(L"\nRequest timed out. Please try again later.\n");
            fflush(stdout);
            return false; // Timeout occurred
        }
        sleep(TIMEOUT_TIME); // Wait for a short period before checking again
        timeout_count++;
    }

    atomic_store(&ThreadData.signalM_OK, 0); // reset the signal after processing

    // Check if the request was successful
    if (answer == MSG_OK) {
        wprintf(L"\nYou can start sending messages.\n");
        fflush(stdout);
        return true;
    }else if (answer == MSG_NOK) {
        wcscpy(packet, ThreadData.NOK_Message);
        wprintf(L"\nYou cannot start sending messages: %ls\n", packet);
        fflush(stdout);
        return false; // Request was not successful
    } else {
        wprintf(L"\nYou cannot start sending messages.\n");
        fflush(stdout);
        return false;
    }
}


/**
 * Method to insert a new packet into the Client's order buffer.
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
 * Method to pop a packet from the Client's order buffer.
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