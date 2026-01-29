#include "client_threads.h"
#include "client_messages.h"

#include <pthread.h>
#include <stdatomic.h>
#include <wchar.h>
#include <unistd.h>


/**
 * Function to initialize the thread data structure with socket and server address.
 * This function sets up the socket file descriptor, server address, and initializes
 * atomic signals for communication between threads.
 *
 * @param data Pointer to the ThreadStruct to be initialized.
 * @param sockfd The socket file descriptor for communication with the server.
 * @param servaddr Pointer to the sockaddr_in structure containing server address information.
 */
void initialize_thread_data(ThreadStruct *data, int sockfd, struct sockaddr_in *servaddr) {
    data->sockfd = sockfd;
    data->servaddr = *servaddr;
    atomic_init(&data->signalHeartbeat, 0);
    atomic_init(&data->signalWarning, 0);
    atomic_init(&data->signalM_OK, 0);
    atomic_init(&data->signalMessage, 0);
    atomic_init(&data->signalMessageEnd, 0);
    atomic_init(&data->signalAnnouncement, 0);
    atomic_init(&data->signalGroupInfo, 0);
    atomic_init(&data->signalNACK, -1); // Initialize to -1 to indicate no NACK signal
    atomic_init(&data->signalFileReceiveWarning, 0);
    atomic_init(&data->signalFilePrint, 0);

    data->Warning[0] = L'\0';
    data->Message[0] = L'\0';
    data->Announcement[0] = L'\0';
    data->GroupInfo[0] = L'\0';
    data->FileReceiveWarning[0] = L'\0';
    data->NOK_Message[0] = L'\0';
    data->FilePrint[0] = L'\0';
}


/**
 * Thread function to handle NACK (Negative Acknowledgment) packets.
 * This function waits for a NACK signal and sends the corresponding packet
 * to the server when a NACK is received.
 *
 * @param nack_thread_data_type Pointer to the NackThreadData structure containing packet information.
 *
 * @return NULL
 */
void* nack_thread(void* nack_thread_data_type) {

    NackThreadData *nack_thread_data = (NackThreadData *)nack_thread_data_type;
    
    wchar_t packet[CHAR_PACKET_SIZE] = {0};
    
    int sockfd = ThreadData.sockfd;
    struct sockaddr_in servaddr = ThreadData.servaddr;

    while (1) {

        // Wait for a NACK signal
        int packetNumber = atomic_load(&ThreadData.signalNACK);
        int countedCharacters = atomic_load(&nack_thread_data->countedCharacters);

        if (packetNumber > -1 && packetNumber <= countedCharacters) {
            // Message packet with the specified packet type and character Type|<packetNumber>|<character>
            swprintf(packet, MAX_LINE, L"%d|%d|%lc", nack_thread_data->packet_type, packetNumber, nack_thread_data->buffer[packetNumber]);
            send_packet(sockfd, &servaddr, packet);

            atomic_store(&ThreadData.signalNACK, -1); // Reset the NACK signal
        }

        say_no_to_busy_waiting();
    }
    return NULL;
}



/**
 * Function to send a heartbeat packet to the server at regular intervals.
 * This function runs in a separate thread and sends a heartbeat packet
 * every HEARTBEAT_TIME seconds.
 *
 * @param socket Pointer to the socket file descriptor.

 * @return NULL
*/
void* heartbeat_thread(void* socket) {
    int sockfd = * (int*)socket;

    struct sockaddr_in servaddr = ThreadData.servaddr; // Use the global server address

    wchar_t heartbeat_packet[MAX_LINE];

    // Prepare heartbeat packet
    swprintf(heartbeat_packet, MAX_LINE, L"%d", HEARTBEAT);

    while (1) {
        // Send heartbeat packet to server

        // If heartbeat signal is set, reset it (duplicate server response)
        if (atomic_load(&ThreadData.signalHeartbeat) == 1) {
            atomic_store(&ThreadData.signalHeartbeat, 0);
        }

        while (atomic_load(&ThreadData.signalHeartbeat) == 0) {
            send_packet(sockfd, &servaddr, heartbeat_packet);
            sleep(MAX_TIMEOUT); //rests for 3 seconds
        }

        // Sleep for a defined interval
        sleep(HEARTBEAT_TIME);
    }

    return NULL;
}


/**
 * Thread function to receive packets from the server.
 *
 * Continuously receives packets from the server, processes them, and
 * passes them to the packet handler function.
 *
 * @param arg Pointer to the socket file descriptor.
 * @return NULL
 */
void* receiver_thread(void* arg) {
    int sockfd = *(int*)arg;
    wchar_t buffer[MAX_LINE] = {0};
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    while (1) {

        int n = recvfrom(sockfd, buffer, WCHAR_BUFFER_SIZE, 0, (struct sockaddr*)&from_addr, &from_len);
        if (n > 0) {
            size_t chars_received = n / sizeof(wchar_t);

            if (chars_received >= MAX_LINE) 
                chars_received = MAX_LINE - 1;
            
            buffer[chars_received] = L'\0';

            handle_packet(buffer);
        }
    }
    return NULL;
}


/**
 * Function to handle incoming packets from the server.
 * It parses the packet type and processes the packet accordingly.
 *
 * @param packet Pointer to the received packet string.
 */
void handle_packet(wchar_t *packet){

    wchar_t *rest;

    wchar_t *packet_type = wcstok(packet, L"|", &rest);
    PacketType type = (PacketType)wcstol(packet_type, NULL, 10);

    switch (type) {
        case HEARTBEAT:
            atomic_store(&ThreadData.signalHeartbeat, 1);
            break;

        case MSG_ONE_WARNING:
        case MSG_BROADCAST_WARNING:
        case MSG_GROUP_WARNING:
            atomic_store(&ThreadData.signalMessage, 0); // Reset the message signal
            wcscpy(ThreadData.Warning, rest);
            atomic_store(&ThreadData.signalWarning, type);
            break;

        case MSG_OK:
            atomic_store(&ThreadData.signalM_OK, MSG_OK);
            break;
        case MSG_NOK:
            wcscpy(ThreadData.NOK_Message, rest);
            atomic_store(&ThreadData.signalM_OK, MSG_NOK);
            break;
        case MSG_END:
            atomic_store(&ThreadData.signalMessageEnd, 1);
            break;
        case MSG_NACK:
            // Store the NACK signal
            while (atomic_load(&ThreadData.signalNACK) != -1) {
                say_no_to_busy_waiting();
            }   
            int packetNumber = wcstol(rest, NULL, 10);
            atomic_store(&ThreadData.signalNACK, packetNumber);

            break;

        case MSG_ONE:
        case MSG_BROADCAST:
        case MSG_GROUP:
            // Check if the previous message has been processed and wait until it is
            while (atomic_load(&ThreadData.signalMessage)) {
                say_no_to_busy_waiting();
            }
            wcscpy(ThreadData.Message, rest);
            atomic_store(&ThreadData.signalMessage, 1);
            break;
            
        case SERVER_ANNOUNCEMENT:
            wcscpy(ThreadData.Announcement, rest);
            atomic_store(&ThreadData.signalAnnouncement, 1);
            break;

        case FILE_SEND_WARNING:
            wcscpy(ThreadData.FileReceiveWarning, rest);
            atomic_store(&ThreadData.signalFileReceiveWarning, 1);
        case GROUP_INFO:
            wcscpy(ThreadData.GroupInfo, rest);
            atomic_store(&ThreadData.signalGroupInfo, 1);
            break;
        case GROUP_OK:
            atomic_store(&ThreadData.signalGroupAction, GROUP_OK);
            break;
        case GROUP_NOK:
            wcscpy(ThreadData.GroupAction, rest);
            atomic_store(&ThreadData.signalGroupAction, GROUP_NOK);
            break;
        default:
            break;
    }
}


/**
 * Function to avoid busy waiting by sleeping for a short duration.
 * This function is used to prevent the CPU from being overloaded by busy waiting loops.
 */
void say_no_to_busy_waiting() {
    usleep(10000); // Sleep for 10ms to avoid busy waiting
}