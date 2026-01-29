#ifndef CLIENTFILE_TRANSFER_H
#define CLIENTFILE_TRANSFER_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <wchar.h>


#define MAX_LINE 1024
#define FILE_TIMEOUT_SECS 60 // Maximum wait time for TCP transfers (in seconds)
#define MAX_FILENAME 64
#define MAX_IP_SIZE 64


typedef struct FileReceiveArgs {
    char ip[MAX_IP_SIZE]; // IP address of the sender
    int port; // TCP port to connect to for file transfer
    char filename[MAX_FILENAME];// Name of the file to be received
    size_t filesize; // Size of the file to be received
} FileReceiveArgs;


// To start a TCP file server to accept file transfers
typedef struct FileServerArgs {
    char filename[MAX_FILENAME]; // Name of the file to be sent
    int server_sock; // TCP socket for the file server
    int timeout_secs; // Time (in seconds) the server port will remain open to receive files
}FileServerArgs;


// To send a file to a specific client
typedef struct FileSendArgs {
    int sockfd; // Socket file descriptor for the TCP connection
    char filename[MAX_FILENAME]; // Name of the file to be sent
}FileSendArgs;


// File transfer functions
void send_file(int sockfd, struct sockaddr_in *servaddr, wchar_t *filename);
void* file_tcp_server_thread(void* arg);
void* send_file_thread(void* send_file_args);
void handle_file_receive_warning();
int yn_with_timeout(const wchar_t *prompt, int timeout_secs);
void* receive_file_tcp(void* fileReceiveArg);


#endif /* CLIENTFILE_TRANSFER_H */