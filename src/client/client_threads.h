#ifndef CLIENT_THREADS_H
#define CLIENT_THREADS_H

#include <stdatomic.h>
#include <arpa/inet.h>
#include <wchar.h>


#include "packet_types.h"


#define MAX_LINE 1024
#define CHAR_PACKET_SIZE 128  
#define WCHAR_BUFFER_SIZE sizeof(wchar_t) * MAX_LINE  // Buffer size for wide character strings

#define HEARTBEAT_TIME 60 // Half of the server heartbeat timeout

#define TIMEOUT_TIME 1
#define MAX_TIMEOUT 3


// Global communication structure for threads
typedef struct {
    int sockfd; // Socket file descriptor
    struct sockaddr_in servaddr; // Server address structure

    atomic_int signalHeartbeat;
    atomic_int signalWarning;  
    atomic_int signalM_OK;
    atomic_int signalMessage;
    atomic_int signalMessageEnd;
    atomic_int signalAnnouncement; //Server announcement, etc...
    atomic_int signalGroupInfo; // Group information packet
    atomic_int signalGroupAction; // Group action packet (create, invite, kick, leave, delete)
    atomic_int signalNACK; // -1 indicates no NACK signal, 0+ indicates NACK received (index missing)
    atomic_int signalFilePrint; // 0 no print, 1 print
    atomic_int signalFileReceiveWarning; // 0 no warning, 1 warning, 
    
    
    wchar_t Warning[MAX_LINE]; // Only accessed when signalWarning
    wchar_t Message[MAX_LINE]; // Only accessed when signalMessage
    wchar_t Announcement[MAX_LINE]; // Only accessed when signalAnnouncement 
    wchar_t GroupInfo[MAX_LINE]; // Only accessed when signalGroupInfo 
    wchar_t GroupAction[MAX_LINE]; // Only accessed when signalGroupAction 
    wchar_t NOK_Message[MAX_LINE]; // Only accessed when signalM_OK 
    wchar_t FileReceiveWarning[MAX_LINE]; // Only accessed when signalFileReceiveWarning 
    wchar_t FilePrint[MAX_LINE]; // Only accessed when signalFilePrint 

} ThreadStruct;


// Structure to hold data for the NACK thread
typedef struct {
    wchar_t buffer[MAX_LINE];
    atomic_int countedCharacters;
    PacketType packet_type;
} NackThreadData;


extern ThreadStruct ThreadData; // Global thread data structure


// Function to initialize the thread data structure with socket and server address
void initialize_thread_data(ThreadStruct *data, int sockfd, struct sockaddr_in *servaddr);


// Thread starter functions
void* heartbeat_thread(void* socket);
void* receiver_thread(void* arg);
void* nack_thread(void* nack_thread_data_type);


// Function to handle incoming packets in the receiver thread
void handle_packet(wchar_t *packet);


// Utility functions
void say_no_to_busy_waiting();


#endif // CLIENT_THREADS_H